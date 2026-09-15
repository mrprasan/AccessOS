// AccessOS/src/BrowserExtension/src/popup.ts
//
// Popup script — requests status from the background service worker
// and renders it into popup.html.

import type {
  BackgroundToPopupMessage,
  PopupToBackgroundMessage,
  StatusUpdateMessage,
} from './types.js';

// ── DOM refs ─────────────────────────────────────────────────────────────────

function el<T extends HTMLElement>(id: string): T {
  return document.getElementById(id) as T;
}

const statusDot       = el<HTMLDivElement>('statusDot');
const connectionLabel = el<HTMLSpanElement>('connectionLabel');
const focusedName     = el<HTMLSpanElement>('focusedName');
const focusedRole     = el<HTMLSpanElement>('focusedRole');
const pageUrl         = el<HTMLSpanElement>('pageUrl');
const timestamp       = el<HTMLSpanElement>('timestamp');

// ── Render ────────────────────────────────────────────────────────────────────

function render(status: StatusUpdateMessage): void {
  // Connection state
  statusDot.className = `dot ${status.connectionState}`;
  connectionLabel.textContent = {
    connected:    'Connected',
    disconnected: 'Disconnected',
    error:        'Error',
  }[status.connectionState];

  // Focused element
  if (status.focusedName) {
    focusedName.textContent = status.focusedName;
    focusedName.className   = 'value';
  } else {
    focusedName.textContent = '(none)';
    focusedName.className   = 'value empty';
  }

  if (status.focusedRole) {
    focusedRole.innerHTML   = `<span class="role-badge">${status.focusedRole}</span>`;
    focusedRole.className   = 'value';
  } else {
    focusedRole.textContent = '—';
    focusedRole.className   = 'value empty';
  }

  // Page URL (strip protocol for brevity)
  if (status.pageUrl) {
    pageUrl.textContent = status.pageUrl.replace(/^https?:\/\//, '');
    pageUrl.className   = 'value';
  } else {
    pageUrl.textContent = '(none)';
    pageUrl.className   = 'value empty';
  }

  // Timestamp
  timestamp.textContent = new Date().toLocaleTimeString();
}

// ── Request status from background ───────────────────────────────────────────

async function fetchStatus(): Promise<void> {
  try {
    const req: PopupToBackgroundMessage = { type: 'REQUEST_STATUS' };
    const response = await chrome.runtime.sendMessage<
      PopupToBackgroundMessage,
      BackgroundToPopupMessage
    >(req);

    if (response?.type === 'STATUS_UPDATE') {
      render(response);
    }
  } catch {
    // Background may not be ready — show disconnected state.
    statusDot.className       = 'dot error';
    connectionLabel.textContent = 'Unavailable';
  }
}

fetchStatus();
