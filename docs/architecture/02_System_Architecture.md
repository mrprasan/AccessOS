# AccessOS — System Architecture

**Version:** 0.1.0
**Status:** ACCESSOS-001 — Repository & Build Foundation
**Last updated:** Phase 0

---

## 1. Platform Identity

AccessOS is a native Windows accessibility platform whose central capability is a screen reader.

It is NOT a clone of any existing screen reader. It is NOT a web application, Electron app, or browser extension.

The three first-class capabilities are:

| Capability | Purpose |
|---|---|
| **READER** | Speech and Braille output for blind/low-vision users |
| **INSPECT** | Accessibility tree inspection for developers and testers |
| **TEST** | Accessibility testing and WCAG analysis |

All three capabilities are built on a single shared semantic engine.

---

## 2. Architecture Pipeline

```
ACCESSIBILITY SOURCE
        │
        ▼
ACCESSIBILITY ACQUISITION LAYER     ← IAccessibilityProvider implementations
        │
        ▼
SEMANTIC NORMALIZATION LAYER        ← Provider adapters → AccessNode
        │
        ▼
ACCESSOS SEMANTIC MODEL             ← AccessNode graph
        │
        ▼
CONTEXT ENGINE                      ← Application/document context
        │
        ▼
NAVIGATION ENGINE                   ← Returns AccessNode, never speaks
        │
        ▼
PRESENTATION MANAGER                ← Speech policy, verbosity, formatter
        │
        ▼
SPEECH / BRAILLE / OTHER OUTPUT
```

**Hard decoupling rules:**
- Speech engine has NO direct dependency on raw UIA objects.
- Navigation engine has NO direct dependency on any speech implementation.
- Semantic model is independent from presentation.
- Raw provider objects NEVER leave their acquisition boundary.

---

## 3. Component Boundaries

### 3.1 AccessOS Core (C++)

Responsibility: All accessibility acquisition, semantic modeling, navigation, speech, and event processing.

Must NOT contain: Any UI framework code (WinUI 3, XAML, Win32 window management).

### 3.2 AccessOS UI (C# / WinUI 3)

Responsibility: Settings, Inspector UI, Event Monitor UI, Testing Mode UI.

Must NOT contain: Accessibility acquisition logic, speech engine code, or UIA event handlers.

### 3.3 Browser Extension (TypeScript / MV3)

Responsibility: DOM/ARIA acquisition in the browser content process.

Must NOT contain: Core navigation logic, speech logic, or Windows API calls.

---

## 4. Threading Model

| Thread | Responsibility | Must NOT do |
|---|---|---|
| UI Thread | WinUI 3 rendering, user input | Block on accessibility calls |
| Accessibility Event Thread | UIA/MSAA event callbacks | Expensive processing, UI updates |
| Worker Thread(s) | Semantic processing, normalization | Direct UI calls |
| Speech Thread | Speech queue, SAPI calls | Block on UIA |
| Browser Communication Thread | Native messaging I/O | Speech or UIA calls |
| Logging Thread | Async log writes | Block callers |

---

## 5. IPC Boundaries

| Boundary | Transport | Protocol |
|---|---|---|
| Core ↔ UI | In-process COM / events | Defined in IPC documentation |
| Core ↔ Browser Extension | Named pipe / Native Messaging | AccessOS Wire Protocol v1 (NOT YET DEFINED) |

---

## 6. Privacy Boundary

The privacy filter sits between all data sources and all external outputs:

```
Any user data
      │
      ▼
Privacy Filter (sensitive data detection + redaction)
      │
      ▼
Logging / Analytics / AI / Telemetry
```

**Content classified as Protected (AccessState::Protected) is NEVER logged.**

---

## 7. Source Providers

| Provider | Status |
|---|---|
| UIAProvider | NOT IMPLEMENTED |
| MSAAProvider | NOT IMPLEMENTED |
| IA2Provider | NOT IMPLEMENTED |
| BrowserProvider | NOT IMPLEMENTED |
| OCRProvider | NOT IMPLEMENTED |

---

## 8. Known Limitations — Phase 0

- No provider implementations exist yet (ACCESSOS-002).
- No speech engine exists yet (ACCESSOS-006).
- No navigation engine exists yet (ACCESSOS-007).
- No UI exists yet (ACCESSOS-008).
- No browser integration exists yet (ACCESSOS-009).
- IPC protocol is NOT YET DEFINED.
- Wire protocol between extension and host is NOT YET DEFINED.
