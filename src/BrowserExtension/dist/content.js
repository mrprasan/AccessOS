// AccessOS/src/BrowserExtension/src/content.ts
//
// Content script — runs in the context of every web page.
//
// Responsibilities:
//   - Walk the DOM to extract accessible elements.
//   - Observe focus changes and send FocusChangedMessage to the service worker.
//   - Observe page loads and send PageLoadedMessage.
//   - Observe keyboard events and forward relevant keystrokes.
//   - NEVER log passwords, form values, auth tokens, or sensitive content.
//
// Threading model:
//   - Runs on the page's main thread.
//   - Messages are sent async via chrome.runtime.sendMessage.
//   - All DOM access is read-only — content script never modifies the page.
import { getRoleForElement, getHeadingLevel } from './roleMap.js';
// ── Constants ─────────────────────────────────────────────────────────────────
/** Elements to include in the accessibility tree walk. */
const INTERACTIVE_SELECTOR = [
    'a[href]',
    'button',
    'input:not([type="hidden"])',
    'select',
    'textarea',
    'h1', 'h2', 'h3', 'h4', 'h5', 'h6',
    '[role]',
    '[tabindex]',
    'label',
    'img[alt]',
    'summary',
].join(',');
/** How long to debounce tree snapshot sends after DOM mutations. */
const SNAPSHOT_DEBOUNCE_MS = 300;
// ── Helpers ───────────────────────────────────────────────────────────────────
/** Returns a sanitized page URL (no query params, no fragment). */
function safeUrl() {
    try {
        const u = new URL(window.location.href);
        return `${u.protocol}//${u.hostname}${u.pathname}`;
    }
    catch {
        return '';
    }
}
/** Returns the accessible name for an element (never includes passwords). */
function getAccessibleName(el) {
    // Password fields — never expose the value.
    if (el instanceof HTMLInputElement && el.type === 'password') {
        return el.getAttribute('aria-label') ?? el.getAttribute('placeholder') ?? '';
    }
    // aria-labelledby
    const labelledBy = el.getAttribute('aria-labelledby');
    if (labelledBy) {
        const names = labelledBy
            .split(/\s+/)
            .map(id => document.getElementById(id)?.textContent?.trim() ?? '')
            .filter(Boolean);
        if (names.length)
            return names.join(' ');
    }
    // aria-label
    const ariaLabel = el.getAttribute('aria-label')?.trim();
    if (ariaLabel)
        return ariaLabel;
    // <label for="..."> association
    if (el instanceof HTMLElement && el.id) {
        const label = document.querySelector(`label[for="${CSS.escape(el.id)}"]`);
        if (label)
            return label.textContent?.trim() ?? '';
    }
    // alt text for images
    if (el instanceof HTMLImageElement)
        return el.alt ?? '';
    // Button value / text
    if (el instanceof HTMLButtonElement || el instanceof HTMLInputElement) {
        if ('value' in el && el.type !== 'password') {
            return el.value?.trim() ||
                el.textContent?.trim() || '';
        }
    }
    return el.textContent?.trim().slice(0, 200) ?? '';
}
/** Returns the accessible value for an element — NEVER for password fields. */
function getAccessibleValue(el) {
    if (el instanceof HTMLInputElement) {
        if (el.type === 'password')
            return ''; // Privacy rule
        if (el.type === 'checkbox' || el.type === 'radio') {
            return el.checked ? 'checked' : 'unchecked';
        }
        return el.value?.slice(0, 500) ?? '';
    }
    if (el instanceof HTMLSelectElement) {
        return el.options[el.selectedIndex]?.text?.trim() ?? '';
    }
    if (el instanceof HTMLTextAreaElement) {
        return el.value?.slice(0, 500) ?? '';
    }
    const ariaValueNow = el.getAttribute('aria-valuenow');
    if (ariaValueNow)
        return ariaValueNow;
    return '';
}
let _nodeIdCounter = 0;
const _elementToId = new WeakMap();
function getOrAssignId(el) {
    if (!_elementToId.has(el)) {
        _elementToId.set(el, ++_nodeIdCounter);
    }
    return _elementToId.get(el);
}
/** Build an AccessNodeSnapshot from a DOM element. */
function snapshotElement(el, parentId) {
    const role = getRoleForElement(el);
    const level = getHeadingLevel(el);
    const rect = el.getBoundingClientRect();
    const childEls = Array.from(el.querySelectorAll(':scope > *'))
        .filter(c => c.matches(INTERACTIVE_SELECTOR));
    return {
        id: getOrAssignId(el),
        role,
        name: getAccessibleName(el),
        value: getAccessibleValue(el),
        description: el.getAttribute('aria-description') ??
            el.getAttribute('title') ?? '',
        level,
        parentId,
        childIds: childEls.map(c => getOrAssignId(c)),
        isFocused: el === document.activeElement,
        isDisabled: el.getAttribute('aria-disabled') === 'true' ||
            ('disabled' in el && el.disabled),
        isHidden: el.getAttribute('aria-hidden') === 'true',
        bounds: {
            x: Math.round(rect.x),
            y: Math.round(rect.y),
            width: Math.round(rect.width),
            height: Math.round(rect.height),
        },
        pageUrl: safeUrl(),
    };
}
/** Walk the DOM and collect all interactive/semantic elements. */
function buildTreeSnapshot() {
    const elements = Array.from(document.querySelectorAll(INTERACTIVE_SELECTOR));
    return elements.map(el => {
        const parentEl = el.parentElement?.closest(INTERACTIVE_SELECTOR);
        const parentId = parentEl ? getOrAssignId(parentEl) : 0;
        return snapshotElement(el, parentId);
    });
}
// ── Message sending ───────────────────────────────────────────────────────────
function send(msg) {
    try {
        chrome.runtime.sendMessage(msg).catch(() => {
            // Background service worker may not be ready — silently ignore.
        });
    }
    catch {
        // Extension context invalidated — silently ignore.
    }
}
// ── Focus observer ────────────────────────────────────────────────────────────
function onFocusIn(event) {
    const el = event.target;
    if (!(el instanceof Element))
        return;
    const parentEl = el.parentElement?.closest(INTERACTIVE_SELECTOR);
    const parentId = parentEl ? getOrAssignId(parentEl) : 0;
    const node = snapshotElement(el, parentId);
    const msg = { type: 'FOCUS_CHANGED', node };
    send(msg);
}
// ── Keyboard observer ─────────────────────────────────────────────────────────
function onKeyDown(event) {
    // Only forward keystrokes that use modifier keys or function keys.
    const hasMod = event.ctrlKey || event.altKey || event.metaKey;
    const isFn = /^F\d{1,2}$/.test(event.key);
    if (!hasMod && !isFn)
        return;
    const msg = {
        type: 'KEY_PRESS',
        key: event.key,
        code: event.code,
        ctrlKey: event.ctrlKey,
        shiftKey: event.shiftKey,
        altKey: event.altKey,
        metaKey: event.metaKey,
    };
    send(msg);
}
// ── Tree snapshot (debounced) ─────────────────────────────────────────────────
let _snapshotTimer = null;
function scheduleSnapshot() {
    if (_snapshotTimer !== null)
        clearTimeout(_snapshotTimer);
    _snapshotTimer = setTimeout(() => {
        _snapshotTimer = null;
        const nodes = buildTreeSnapshot();
        if (nodes.length === 0)
            return;
        const msg = { type: 'TREE_SNAPSHOT', nodes };
        send(msg);
    }, SNAPSHOT_DEBOUNCE_MS);
}
// ── MutationObserver for dynamic content ──────────────────────────────────────
const _mutationObserver = new MutationObserver(() => {
    scheduleSnapshot();
});
// ── Initialisation ────────────────────────────────────────────────────────────
function init() {
    // Notify background that the page has loaded.
    const loadMsg = {
        type: 'PAGE_LOADED',
        url: safeUrl(),
        title: document.title,
    };
    send(loadMsg);
    // Send initial tree snapshot.
    scheduleSnapshot();
    // Watch for DOM mutations.
    _mutationObserver.observe(document.body, {
        childList: true,
        subtree: true,
        attributes: true,
        attributeFilter: [
            'aria-label', 'aria-labelledby', 'aria-disabled',
            'aria-hidden', 'aria-checked', 'aria-expanded',
            'aria-selected', 'value', 'disabled',
        ],
    });
    // Listen for focus and keyboard events.
    document.addEventListener('focusin', onFocusIn, { capture: true, passive: true });
    document.addEventListener('keydown', onKeyDown, { capture: true, passive: true });
}
// Run after the document is interactive.
if (document.readyState === 'loading') {
    document.addEventListener('DOMContentLoaded', init);
}
else {
    init();
}
//# sourceMappingURL=content.js.map