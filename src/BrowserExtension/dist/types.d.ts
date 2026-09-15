/**
 * Semantic roles that map to AccessOS AccessRole values on the native side.
 * Kept as string literals so they survive JSON round-trips without a schema.
 */
export type AccessRole = 'button' | 'checkbox' | 'combobox' | 'document' | 'edit' | 'group' | 'heading' | 'image' | 'link' | 'listbox' | 'listitem' | 'menu' | 'menubar' | 'menuitem' | 'radiobutton' | 'statictext' | 'tab' | 'tabcontrol' | 'tree' | 'treeitem' | 'window' | 'unknown';
/** A snapshot of one accessible element extracted from the web page. */
export interface AccessNodeSnapshot {
    /** Stable identifier within this page snapshot (1-based index or hash). */
    id: number;
    role: AccessRole;
    name: string;
    value: string;
    description: string;
    level: number;
    parentId: number;
    childIds: number[];
    isFocused: boolean;
    isDisabled: boolean;
    isHidden: boolean;
    /** Bounding rect in viewport coordinates (px). */
    bounds: {
        x: number;
        y: number;
        width: number;
        height: number;
    };
    /** Source URL of the page. Never includes query params or fragments. */
    pageUrl: string;
}
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
    key: string;
    code: string;
    ctrlKey: boolean;
    shiftKey: boolean;
    altKey: boolean;
    metaKey: boolean;
    tabId?: number;
}
export type ContentToBackgroundMessage = FocusChangedMessage | PageLoadedMessage | TreeSnapshotMessage | KeyPressMessage;
/** Wrapper sent over the native messaging channel to the C++ host. */
export interface NativeMessage {
    version: 1;
    payload: ContentToBackgroundMessage;
}
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
export interface RequestStatusMessage {
    type: 'REQUEST_STATUS';
}
export type PopupToBackgroundMessage = RequestStatusMessage;
//# sourceMappingURL=types.d.ts.map