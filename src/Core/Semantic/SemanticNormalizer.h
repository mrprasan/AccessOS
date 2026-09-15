// AccessOS/src/Core/Semantic/SemanticNormalizer.h
//
// Normalizes raw AccessNode snapshots into canonical AccessOS form.
//
// Why: Different providers (UIA, MSAA, Browser) produce AccessNode snapshots
//      with varying completeness. The normalizer applies uniform rules:
//       - Fill missing names from fallback sources
//       - Normalize roles to canonical AccessRole values
//       - Derive heading level from role/ARIA data
//       - Set MultiLine state for multi-line edit controls
//       - Mark password fields as Protected
//       - Trim and clean string fields
//
// Must NOT:
//   - Query live provider objects
//   - Perform speech or navigation decisions
//   - Have side effects on the SemanticCache directly
//
// Threading: Stateless — safe to call from any thread.

#pragma once

#include "AccessNode.h"

namespace AccessOS {

class SemanticNormalizer {
public:
    // Apply normalization rules to a raw snapshot.
    // Returns the normalized node. Does not modify the cache.
    static AccessNode Normalize(AccessNode node);

private:
    // Trim leading/trailing whitespace from a string.
    static std::string Trim(std::string s);

    // Derive the MultiLine state flag for edit controls.
    // UIA does not always set this explicitly — infer from role.
    static void DeriveMultiLineState(AccessNode& node);

    // Ensure name is never purely whitespace.
    static void NormalizeName(AccessNode& node);

    // Ensure role falls back to a reasonable default
    // rather than Unknown when a name is present.
    static void NormalizeRole(AccessNode& node);
};

} // namespace AccessOS
