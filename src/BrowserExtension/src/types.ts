// AccessOS/src/BrowserExtension/src/types.ts
//
// Shared message types used between:
//   content.ts  → background.ts  (content → service worker)
//   background.ts → native host  (service worker → C++ core)
//   background.ts → popup.ts     (service worker → popup)
//
// All messages are discriminated unions keyed on `type`.
// No passwords, PINs, auth tokens, or private form data are ever included.

// ── Roles ─────────────────────────────────────────────────────────────────────

/**
 * Semantic roles that map to AccessOS AccessRole values on the native side.
 * Kept as string literals so they survive JSON round-trips without a schema.
 */
export type AccessRole =
  | 'button'
  | 'checkbox'
  | 'combobox'
  | 'document'
  | 'edit'
  | 'group'
  | 'heading'
  | 'image'
  | 'link'
  | 'listbox'
  | 'listitem'
  | 'menu'
  | 'menubar'
  | 'menuitem'
  | 'radiobutton'
  | 'statictext'
  | 'tab'
  | 'tabcontrol'
  | 'tree'
  | 'treeitem'
  | 'window'
  | 'unknown';

// ── AccessNode snapshot ────────────────────────────────────────────────────────

/** A snapshot of one accessible element extracted from the web page. */
export interface AccessNodeSnapshot {
  /** Stable identifier within this page snapshot (1-based index or hash). */
  id: number;
  role: AccessRole;
  name: string;
  value: string;
  description: string;
  level: number;          // heading level 1–6; 0 for non-headings
  parentId: number;       // 0 = root
  childIds: number[];
  isFocused: boolean;
  isDisabled: boolean;
  isHidden: boolean;
  /** Bounding rect in viewport coordinates (px). */
  bounds: { x: number; y: number; width: number; height: number };
  /** Source URL of the page. Never includes query params or fragments. */
  pageUrl: string;
}

// ── Content → Background messages ─────────────────────────────────────────────

/** Sent when the focused element changes inside the web page. */
export interface FocusChangedMessage {
  type: 'FOCUS_CHANGED';
  node: AccessNodeSnapshot;
  tabId?: number;
}

/** Sent when a page navigation completes (new document). */
export interface PageLoadedMessage {
  type: 'PAGE_LOADED';
  url: string;
  title: string;
  tabId?: number;
}

/** Sent when the full accessibility tree of the page is available. */
export interface TreeSnapshotMessage {
  type: 'TREE_SNAPSHOT';
  nodes: AccessNodeSnapshot[];
  tabId?: number;
}

/** Sent when the user presses a key inside the browser. */
export interface KeyPressMessage {
  type: 'KEY_PRESS';
  key: string;         // KeyboardEvent.key
  code: string;        // KeyboardEvent.code
  ctrlKey: boolean;
  shiftKey: boolean;
  altKey: boolean;
  metaKey: boolean;
  tabId?: number;
}

export type ContentToBackgroundMessage =
  | FocusChangedMessage
  | PageLoadedMessage
  | TreeSnapshotMessage
  | KeyPressMessage;

// ── Background → Native host messages ─────────────────────────────────────────

/** Wrapper sent over the native messaging channel to the C++ host. */
export interface NativeMessage {
  version: 1;
  payload: ContentToBackgroundMessage;
}

// ── Background → Popup messages ───────────────────────────────────────────────

/** Connection state reported to the popup. */
export type ConnectionState = 'connected' | 'disconnected' | 'error';

export interface StatusUpdateMessage {
  type: 'STATUS_UPDATE';
  connectionState: ConnectionState;
  focusedName: string;
  focusedRole: AccessRole | '';
  pageUrl: string;
}

export type BackgroundToPopupMessage = StatusUpdateMessage;

// ── Popup → Background messages ───────────────────────────────────────────────

export interface RequestStatusMessage {
  type: 'REQUEST_STATUS';
}

export type PopupToBackgroundMessage = RequestStatusMessage;
