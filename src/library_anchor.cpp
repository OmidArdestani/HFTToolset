// ============================================================================
// HFTToolset — Library Anchor
// Empty translation unit that ensures the static library has at least one
// object file, allowing header-only components to link correctly.
// ============================================================================

namespace HFTToolsetLib {
// Anchor translation unit to allow building a static library from headers.
void library_anchor() {}
} // namespace HFTToolsetLib
