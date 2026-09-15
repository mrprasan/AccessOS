// AccessOS/src/BrowserExtension/src/popup.ts
//
// Popup script — requests status from the background service worker
// and renders it into popup.html.
// ── DOM refs ─────────────────────────────────────────────────────────────────
function el(id) {
    return document.getElementById(id);
}
const statusDot = el('statusDot');
const connectionLabel = el('connectionLabel');
const focusedName = el('focusedName');
const focusedRole = el('focusedRole');
const pageUrl = el('pageUrl');
const timestamp = el('timestamp');
// ── Render ────────────────────────────────────────────────────────────────────
function render(status) {
    // Connection state
    statusDot.className = `dot ${status.connectionState}`;
    connectionLabel.textContent = {
        connected: 'Connected',
        disconnected: 'Disconnected',
        error: 'Error',
    }[status.connectionState];
    // Focused element
    if (status.focusedName) {
        focusedName.textContent = status.focusedName;
        focusedName.className = 'value';
    }
    else {
        focusedName.textContent = '(none)';
        focusedName.className = 'value empty';
    }
    if (status.focusedRole) {
        focusedRole.innerHTML = `<span class="role-badge">${status.focusedRole}</span>`;
        focusedRole.className = 'value';
    }
    else {
        focusedRole.textContent = '—';
        focusedRole.className = 'value empty';
    }
    // Page URL (strip protocol for brevity)
    if (status.pageUrl) {
        pageUrl.textContent = status.pageUrl.replace(/^https?:\/\//, '');
        pageUrl.className = 'value';
    }
    else {
        pageUrl.textContent = '(none)';
        pageUrl.className = 'value empty';
    }
    // Timestamp
    timestamp.textContent = new Date().toLocaleTimeString();
}
// ── Request status from background ───────────────────────────────────────────
async function fetchStatus() {
    try {
        const req = { type: 'REQUEST_STATUS' };
        const response = await chrome.runtime.sendMessage(req);
        if (response?.type === 'STATUS_UPDATE') {
            render(response);
        }
    }
    catch {
        // Background may not be ready — show disconnected state.
        statusDot.className = 'dot error';
        connectionLabel.textContent = 'Unavailable';
    }
}
fetchStatus();
export {};
//# sourceMappingURL=popup.js.map