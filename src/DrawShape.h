#pragma once
#include <wx/colour.h>
#include <wx/gdicmn.h>
#include <wx/string.h>
#include <map>
#include <vector>

enum class ShapeKind { Rect, Circle, Text, Bezier, Group, SVGRef };

struct DrawShape {
    ShapeKind kind          = ShapeKind::Rect;
    wxRect    bounds;                              // bounding box (all kinds)
    wxColour  fgColour { 0,   0,   0   };          // pen / text colour
    wxColour  bgColour { 255, 255, 255 };          // fill colour
    wxString  label;                               // text content (Text kind only)
    wxPoint   pts[4]   {};                         // cubic Bezier control points (Bezier kind only)
    int       strokeWidth    = 1;                  // border width in pixels (Rect/Circle/Bezier)
    int       borderRadiusX  = 0;                  // horizontal corner radius, pixels (Rect only)
    int       borderRadiusY  = 0;                  // vertical   corner radius, pixels (Rect only)

    // ---- Hierarchy / component fields (Approach B) ----
    wxString  name;                                // display name shown in hierarchy pane
    bool      visible = true;                      // visibility toggle
    bool      locked  = false;                     // lock prevents selection / editing

    // Group kind: child shapes owned recursively.
    // bounds = union of all children bounds (maintained by DrawDoc helpers).
    std::vector<DrawShape> children;

    // SVGRef kind: path to an external SVG file used as a reusable component.
    wxString svgRefPath;

    // Per-instance overrides for SVGRef: property-path → value string.
    // Example: "label:0" → "Custom text" overrides the first Text child's label.
    std::map<wxString, wxString> overrides;

    // Transient cache: resolved native shapes for an SVGRef (not serialized).
    // Populated lazily by SVGRefCache in DrawDoc.
    std::vector<DrawShape> resolvedChildren;

    bool HitTest(const wxPoint& pt) const {
        switch (kind) {
        case ShapeKind::Group:
        case ShapeKind::SVGRef:
        case ShapeKind::Rect:
        case ShapeKind::Circle:
        case ShapeKind::Text:
            return bounds.Contains(pt);

        case ShapeKind::Bezier: {
            // Sample the cubic Bezier and check against a 6-pixel tolerance.
            const int TOL2 = 6 * 6;
            for (int i = 0; i <= 50; i++) {
                float t  = i / 50.0f, mt = 1.0f - t;
                int x = (int)(mt*mt*mt*pts[0].x + 3*mt*mt*t*pts[1].x +
                              3*mt*t*t*pts[2].x  + t*t*t*pts[3].x);
                int y = (int)(mt*mt*mt*pts[0].y + 3*mt*mt*t*pts[1].y +
                              3*mt*t*t*pts[2].y  + t*t*t*pts[3].y);
                int dx = x - pt.x, dy = y - pt.y;
                if (dx*dx + dy*dy <= TOL2) return true;
            }
            return false;
        }
        }
        return false;
    }
};

// Returns the union of all children's bounds.
// Used to keep a Group's bounds up-to-date.
inline wxRect ComputeGroupBounds(const std::vector<DrawShape>& children) {
    if (children.empty()) return wxRect(0, 0, 0, 0);
    wxRect r = children[0].bounds;
    for (size_t i = 1; i < children.size(); ++i)
        r = r.Union(children[i].bounds);
    return r;
}
