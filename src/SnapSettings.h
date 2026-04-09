#pragma once
#include <wx/colour.h>
#include <wx/string.h>

struct SnapSettings {
    // Master toggle
    bool     enabled            = true;

    // Snap type toggles
    bool     snapEdgeToEdge     = true;   // left/right/top/bottom edges align
    bool     snapCenterToCenter = true;   // centers align
    bool     snapEdgeToCenter   = true;   // edge of moving ↔ center of target (and vice versa)
    bool     snapEqualSpacing   = true;   // equal gap between shapes
    bool     snapPercentage     = true;   // 25/50/75% positions within parent/page
    bool     snapBezierPoints   = true;   // bezier control pts as snap targets
    bool     snapLockedShapes   = true;   // locked shapes serve as snap targets

    // Snap capture distance in screen pixels; converted to doc space at query time.
    int      pixelThreshold     = 8;

    // Guide line appearance
    wxColour guideColour        { 255, 0, 120 };
    int      guideLineWidth     = 1;

    // Returns the canonical path for persistent storage.
    static wxString DefaultPath();

    bool SaveToXml(const wxString& path) const;
    bool LoadFromXml(const wxString& path);
};
