#pragma once
#include <wx/colour.h>
#include <wx/gdicmn.h>
#include <wx/string.h>

enum class ShapeKind { Rect, Circle, Text, Bezier };

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

    bool HitTest(const wxPoint& pt) const {
        if (kind != ShapeKind::Bezier)
            return bounds.Contains(pt);
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
};
