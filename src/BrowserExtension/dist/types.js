// AccessOS/src/BrowserExtension/src/types.ts
//
// Shared message types used between:
//   content.ts  → background.ts  (content → service worker)
//   background.ts → native host  (service worker → C++ core)
//   background.ts → popup.ts     (service worker → popup)
//
// All messages are discriminated unions keyed on `type`.
// No passwords, PINs, auth tokens, or private form data are ever included.
export {};
//# sourceMappingURL=types.js.map