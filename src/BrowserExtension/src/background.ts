// AccessOS/src/BrowserExtension/src/background.ts
//
// MV3 Service Worker — the extension's background process.
//
// Responsibilities:
//   - Connect to the AccessOS native messaging host (C++ core bridge).
//   - Receive messages from content scripts and forward to the native host.
//   - Maintain last-known status (focused element, page, connection state).
//   - Respond to popup status requests.
//   - Reconnect the native port if it disconnects.
//
// Threading model:
//   - Single service worker thread.
//   - All chrome.runtime callbacks are already on the extension's thread.
//
// Privacy:
//   - Password values are NEVER forwarded to the native host.
//   - The native host receives only role, name, and structural data.

import type {
  ContentToBackgroundMessage,
  BackgroundToPopupMessage,
  PopupToBackgroundMessage,
  NativeMessage,
  ConnectionState,
  FocusChangedMessage,
  PageLoadedMessage,
  AccessRole,
} from './types.js';

// ── Native messaging host name ────────────────────────────────────────────────
// Must match the name registered in the Windows registry by the AccessOS installer.
const NATIVE_HOST = 'com.accessos.nativehost';

// ── Reconnect policy ──────────────────────────────────────────────────────────
const RECONNECT_DELAY_MS = 3_000;
const MAX_RECONNECT_ATTEMPTS = 5;

// ── State ─────────────────────────────────────────────────────────────────────

let nativePort: chrome.runtime.Port | null = null;
let connectionState: ConnectionState = 'disconnected';
let reconnectAttempts = 0;
let reconnectTimer: ReturnType<typeof setTimeout> | null = null;

let lastFocusedName: string = '';
let lastFocusedRole: AccessRole | '' = '';
let lastPageUrl: string = '';

// ── Native host connection ────────────────────────────────────────────────────

function connectToNativeHost(): void {
  if (nativePort !== null) return;

  try {
    nativePort = chrome.runtime.connectNative(NATIVE_HOST);
    connectionState = 'connected';
    reconnectAttempts = 0;

    nativePort.onMessage.addListener((msg: unknown) => {
      // Native host can send commands back — reserved for future use.
      console.debug('[AccessOS background] Native message received:', msg);
    });

    nativePort.onDisconnect.addListener(() => {
      nativePort = null;
      const err = chrome.runtime.lastError?.message ?? 'unknown';
      console.warn(`[AccessOS background] Native port disconnected: ${err}`);

      if (err.includes('Specified native messaging host not found')) {
        // Host is not installed — do not reconnect.
        connectionState = 'error';
        return;
      }

      connectionState = 'disconnected';
      scheduleReconnect();
    });

  } catch (e) {
    console.warn('[AccessOS background] connectNative failed:', e);
    connectionState = 'error';
  }
}

function scheduleReconnect(): void {
  if (reconnectAttempts >= MAX_RECONNECT_ATTEMPTS) {
    console.warn('[AccessOS background] Max reconnect attempts reached.');
    connectionState = 'error';
    return;
  }
  if (reconnectTimer !== null) return;
  reconnectAttempts++;
  reconnectTimer = setTimeout(() => {
    reconnectTimer = null;
    connectToNativeHost();
  }, RECONNECT_DELAY_MS);
}

function sendToNativeHost(payload: ContentToBackgroundMessage): void {
  if (nativePort === null) return;
  const msg: NativeMessage = { version: 1, payload };
  try {
    nativePort.postMessage(msg);
  } catch (e) {
    console.warn('[AccessOS background] postMessage failed:', e);
    nativePort = null;
    connectionState = 'disconnected';
    scheduleReconnect();
  }
}

// ── Content script message handler ───────────────────────────────────────────

function handleContentMessage(
  msg: ContentToBackgroundMessage,
  sender: chrome.runtime.MessageSender,
): void {
  // Stamp tab ID onto the message.
  if (sender.tab?.id !== undefined) {
    (msg as { tabId?: number }).tabId = sender.tab.id;
  }

  switch (msg.type) {
    case 'FOCUS_CHANGED': {
      const m = msg as FocusChangedMessage;
      lastFocusedName = m.node.name;
      lastFocusedRole = m.node.role;
      sendToNativeHost(msg);
      break;
    }
    case 'PAGE_LOADED': {
      const m = msg as PageLoadedMessage;
      lastPageUrl = m.url;
      lastFocusedName = '';
      lastFocusedRole = '';
      sendToNativeHost(msg);
      break;
    }
    case 'TREE_SNAPSHOT':
    case 'KEY_PRESS':
      sendToNativeHost(msg);
      break;
  }
}

// ── Popup message handler ─────────────────────────────────────────────────────

function handlePopupMessage(
  msg: PopupToBackgroundMessage,
  sendResponse: (response: BackgroundToPopupMessage) => void,
): void {
  if (msg.type === 'REQUEST_STATUS') {
    const response: BackgroundToPopupMessage = {
      type:            'STATUS_UPDATE',
      connectionState: connectionState,
      focusedName:     lastFocusedName,
      focusedRole:     lastFocusedRole,
      pageUrl:         lastPageUrl,
    };
    sendResponse(response);
  }
}

// ── Chrome runtime listeners ──────────────────────────────────────────────────

chrome.runtime.onMessage.addListener(
  (
    msg: ContentToBackgroundMessage | PopupToBackgroundMessage,
    sender: chrome.runtime.MessageSender,
    sendResponse: (r: BackgroundToPopupMessage) => void,
  ): boolean | undefined => {
    if (msg.type === 'REQUEST_STATUS') {
      handlePopupMessage(msg as PopupToBackgroundMessage, sendResponse);
      return true;   // keep the channel open for async response
    }

    handleContentMessage(msg as ContentToBackgroundMessage, sender);
    return undefined;
  },
);

// Connect to the native host when the service worker starts.
connectToNativeHost();
