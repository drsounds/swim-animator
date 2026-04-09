#pragma once
#include <vector>
#include <wx/gdicmn.h>
#include <wx/colour.h>
#include "DrawShape.h"
#include "SnapSettings.h"

// A guide line to be rendered over the canvas after all shapes are drawn.
// Coordinates are in document space (the DC transform handles viewport mapping).
struct SnapGuideLine {
    bool     isVertical;   // true → vertical line at x=pos; false → horizontal at y=pos
    int      pos;          // doc-space coordinate of the line
    int      extentMin;    // start of the line along the perpendicular axis (doc space)
    int      extentMax;    // end   of the line along the perpendicular axis (doc space)
    wxColour colour;
    int      lineWidth;
};

// Output returned by every SnapEngine query.
struct SnapResult {
    wxPoint                    snappedDelta;  // corrected delta to apply instead of raw
    std::vector<SnapGuideLine> guides;        // guide lines for this frame (empty = no snap)
};

// ---------------------------------------------------------------------------
// SnapEngine – pure computation, no GUI state.
// Construct on the stack per query; all settings are captured by const-ref.
// ---------------------------------------------------------------------------
class SnapEngine {
public:
    explicit SnapEngine(const SnapSettings& settings);

    // Snap a move/resize delta.
    // movingShapes   – shapes at their ORIGINAL positions.
    // proposedDelta  – raw dx/dy from mouse position.
    // allShapes      – the full scope (root shapes or active group children).
    // excludeIndices – indices in allShapes that are being moved (skip as targets).
    // parentBounds   – page rect, or active group bounds in isolation mode.
    // zoom           – current zoom for threshold conversion.
    SnapResult Snap(
        const std::vector<DrawShape>& movingShapes,
        wxPoint                       proposedDelta,
        const std::vector<DrawShape>& allShapes,
        const std::vector<int>&       excludeIndices,
        const wxRect&                 parentBounds,
        double                        zoom
    ) const;

    // Snap a single Bezier control-point handle.
    // proposedPt  – where the handle would land without snapping (original + raw delta).
    // originalPt  – the handle's original position (used to compute returned snappedDelta).
    // Returns snappedDelta = snappedPt - originalPt (drop-in replacement for raw dx/dy).
    SnapResult SnapBezierHandle(
        wxPoint                       proposedPt,
        wxPoint                       originalPt,
        const std::vector<DrawShape>& allShapes,
        const std::vector<int>&       excludeIndices,
        const wxRect&                 parentBounds,
        double                        zoom
    ) const;

    // Snap the moving corner of a creation rubber-band rectangle.
    // proposedCorner – m_dragCurrent in doc space.
    // Returns snappedDelta = correction to add to proposedCorner.
    SnapResult SnapRubberBandCorner(
        wxPoint                       proposedCorner,
        const std::vector<DrawShape>& allShapes,
        const wxRect&                 parentBounds,
        double                        zoom
    ) const;

private:
    const SnapSettings& m_s;

    int ThresholdDocSpace(double zoom) const;
};
