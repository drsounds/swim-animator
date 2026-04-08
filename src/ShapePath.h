#pragma once
#include <vector>
#include <wx/string.h>

// A ShapePath uniquely identifies a shape in the recursive DrawShape tree.
// Each element is an index into the children vector at that depth.
//   {}      = invalid / root level reference
//   {2}     = root shape at index 2
//   {2, 1}  = child 1 of root shape 2
using ShapePath = std::vector<int>;

// Returns a human-readable string like "2/1/0" (used for debugging/logging).
inline wxString ShapePathToString(const ShapePath& path) {
    wxString s;
    for (size_t i = 0; i < path.size(); ++i) {
        if (i) s += '/';
        s += wxString::Format("%d", path[i]);
    }
    return s.IsEmpty() ? wxString("(root)") : s;
}

// Returns the parent path (all elements except the last).
inline ShapePath ShapePathParent(const ShapePath& path) {
    if (path.empty()) return {};
    return ShapePath(path.begin(), path.end() - 1);
}

// Returns the last index (the shape's position within its parent's children).
// Returns -1 if path is empty.
inline int ShapePathLeaf(const ShapePath& path) {
    return path.empty() ? -1 : path.back();
}

// Returns true if `ancestor` is a proper prefix of `path`
// (i.e. `ancestor` contains `path` somewhere in its subtree).
inline bool ShapePathIsAncestor(const ShapePath& ancestor, const ShapePath& path) {
    if (ancestor.size() >= path.size()) return false;
    return ShapePath(path.begin(), path.begin() + ancestor.size()) == ancestor;
}
