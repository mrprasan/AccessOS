// AccessOS/src/Core/Accessibility/UIAutomation/UIAIncludes.h
//
// Canonical include order for UI Automation headers.
//
// Why: UIAutomationCore.h requires IUnknown (from unknwn.h / objbase.h)
//      to be declared before it is included. WIN32_LEAN_AND_MEAN suppresses
//      some of these transitive includes from windows.h. Including this
//      header instead of <uiautomation.h> directly ensures correct order
//      in every translation unit.
//
// Rule: All UIAProvider translation units must include this header FIRST,
//       before any other Windows or UIA headers.

#pragma once

// objbase.h provides IUnknown and COM base types.
// It must precede uiautomation.h / UIAutomationCore.h.
#ifndef WIN32_LEAN_AND_MEAN
#define WIN32_LEAN_AND_MEAN
#endif

#ifndef NOMINMAX
#define NOMINMAX
#endif

#ifndef UNICODE
#define UNICODE
#endif

#include <objbase.h>   // IUnknown, CoCreateInstance, HRESULT, etc.
#include <unknwn.h>    // IUnknown interface definition
#include <windows.h>   // Full Win32 surface
#include <uiautomation.h>  // IUIAutomation, IUIAutomationElement, patterns
#include <wrl/client.h>    // Microsoft::WRL::ComPtr
