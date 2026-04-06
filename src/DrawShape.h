#pragma once
#include <wx/colour.h>
#include <wx/gdicmn.h>
#include <wx/string.h>

enum class ShapeKind { Rect, Circle, Text };

struct DrawShape {
    ShapeKind kind     = ShapeKind::Rect;
    wxRect    bounds;
    wxColour  fgColour { 0,   0,   0   };   // pen / text colour
    wxColour  bgColour { 255, 255, 255 };   // fill colour
    wxString  label;                         // text content (Text kind only)

    bool HitTest(const wxPoint& pt) const { return bounds.Contains(pt); }
};
