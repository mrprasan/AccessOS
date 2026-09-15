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
// ── Native messaging host name ────────────────────────────────────────────────
// Must match the name registered in the Windows registry by the AccessOS installer.
const NATIVE_HOST = 'com.accessos.nativehost';
// ── Reconnect policy ──────────────────────────────────────────────────────────
const RECONNECT_DELAY_MS = 3000;
const MAX_RECONNECT_ATTEMPTS = 5;
// ── State ─────────────────────────────────────────────────────────────────────
let nativePort = null;
let connectionState = 'disconnected';
let reconnectAttempts = 0;
let reconnectTimer = null;
let lastFocusedName = '';
let lastFocusedRole = '';
let lastPageUrl = '';
// ── Native host connection ────────────────────────────────────────────────────
function connectToNativeHost() {
    if (nativePort !== null)
        return;
    try {
        nativePort = chrome.runtime.connectNative(NATIVE_HOST);
        connectionState = 'connected';
        reconnectAttempts = 0;
        nativePort.onMessage.addListener((msg) => {
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
    }
    catch (e) {
        console.warn('[AccessOS background] connectNative failed:', e);
        connectionState = 'error';
    }
}
function scheduleReconnect() {
    if (reconnectAttempts >= MAX_RECONNECT_ATTEMPTS) {
        console.warn('[AccessOS background] Max reconnect attempts reached.');
        connectionState = 'error';
        return;
    }
    if (reconnectTimer !== null)
        return;
    reconnectAttempts++;
    reconnectTimer = setTimeout(() => {
        reconnectTimer = null;
        connectToNativeHost();
    }, RECONNECT_DELAY_MS);
}
function sendToNativeHost(payload) {
    if (nativePort === null)
        return;
    const msg = { version: 1, payload };
    try {
        nativePort.postMessage(msg);
    }
    catch (e) {
        console.warn('[AccessOS background] postMessage failed:', e);
        nativePort = null;
        connectionState = 'disconnected';
        scheduleReconnect();
    }
}
// ── Content script message handler ───────────────────────────────────────────
function handleContentMessage(msg, sender) {
    // Stamp tab ID onto the message.
    if (sender.tab?.id !== undefined) {
        msg.tabId = sender.tab.id;
    }
    switch (msg.type) {
        case 'FOCUS_CHANGED': {
            const m = msg;
            lastFocusedName = m.node.name;
            lastFocusedRole = m.node.role;
            sendToNativeHost(msg);
            break;
        }
        case 'PAGE_LOADED': {
            const m = msg;
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
function handlePopupMessage(msg, sendResponse) {
    if (msg.type === 'REQUEST_STATUS') {
        const response = {
            type: 'STATUS_UPDATE',
            connectionState: connectionState,
            focusedName: lastFocusedName,
            focusedRole: lastFocusedRole,
            pageUrl: lastPageUrl,
        };
        sendResponse(response);
    }
}
// ── Chrome runtime listeners ──────────────────────────────────────────────────
chrome.runtime.onMessage.addListener((msg, sender, sendResponse) => {
    if (msg.type === 'REQUEST_STATUS') {
        handlePopupMessage(msg, sendResponse);
        return true; // keep the channel open for async response
    }
    handleContentMessage(msg, sender);
    return undefined;
});
// Connect to the native host when the service worker starts.
connectToNativeHost();
export {};
//# sourceMappingURL=background.js.map