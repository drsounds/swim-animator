#include "SnapEngine.h"
#include <algorithm>
#include <cmath>
#include <limits>

// ---------------------------------------------------------------------------
// Internal types
// ---------------------------------------------------------------------------

// A snap candidate value on one axis (X or Y).
// type: 0=edge (left/right/top/bottom, bezier pts)
//       1=center
//       2=virtual (proportional / percentage) – treated as center for pair validation
struct AxisVal {
    int    v;      // doc-space coordinate
    int    type;   // 0=edge, 1=center, 2=virtual-center
    wxRect bounds; // source shape bounds (for guide extent)
};

// ---------------------------------------------------------------------------
// Helpers: collect axis values from shapes
// ---------------------------------------------------------------------------

static void AppendShapeXVals(const DrawShape& s, std::vector<AxisVal>& out) {
    wxRect b = s.bounds;
    out.push_back({ b.x,               0, b });
    out.push_back({ b.x + b.width,     0, b });
    out.push_back({ b.x + b.width / 2, 1, b });
}

static void AppendShapeYVals(const DrawShape& s, std::vector<AxisVal>& out) {
    wxRect b = s.bounds;
    out.push_back({ b.y,                0, b });
    out.push_back({ b.y + b.height,     0, b });
    out.push_back({ b.y + b.height / 2, 1, b });
}

// Recursively collect target axis values from a shape tree.
// excludeIndices applies only to the top-level call (depth==0).
static void CollectTargetXVals(const std::vector<DrawShape>& shapes,
                                const std::vector<int>& exclude,
                                bool snapLocked, bool snapBezier,
                                std::vector<AxisVal>& out,
                                int depth = 0)
{
    for (int i = 0; i < (int)shapes.size(); i++) {
        if (depth == 0) {
            if (std::find(exclude.begin(), exclude.end(), i) != exclude.end()) continue;
        }
        const DrawShape& s = shapes[i];
        if (!s.visible) continue;
        if (s.locked && !snapLocked) continue;

        AppendShapeXVals(s, out);

        if (snapBezier && s.kind == ShapeKind::Bezier) {
            for (int k = 0; k < 4; k++)
                out.push_back({ s.pts[k].x, 0, s.bounds });
        }
        if (s.kind == ShapeKind::Group)
            CollectTargetXVals(s.children, {}, snapLocked, snapBezier, out, depth + 1);
    }
}

static void CollectTargetYVals(const std::vector<DrawShape>& shapes,
                                const std::vector<int>& exclude,
                                bool snapLocked, bool snapBezier,
                                std::vector<AxisVal>& out,
                                int depth = 0)
{
    for (int i = 0; i < (int)shapes.size(); i++) {
        if (depth == 0) {
            if (std::find(exclude.begin(), exclude.end(), i) != exclude.end()) continue;
        }
        const DrawShape& s = shapes[i];
        if (!s.visible) continue;
        if (s.locked && !snapLocked) continue;

        AppendShapeYVals(s, out);

        if (snapBezier && s.kind == ShapeKind::Bezier) {
            for (int k = 0; k < 4; k++)
                out.push_back({ s.pts[k].y, 0, s.bounds });
        }
        if (s.kind == ShapeKind::Group)
            CollectTargetYVals(s.children, {}, snapLocked, snapBezier, out, depth + 1);
    }
}

// Add equal-spacing candidates: midpoints between each adjacent pair of
// non-excluded shapes (sorted by their left edge for X, top edge for Y).
static void AppendEqualSpacingX(const std::vector<DrawShape>& shapes,
                                  const std::vector<int>& exclude,
                                  const wxRect& parentBounds,
                                  std::vector<AxisVal>& out)
{
    // Gather non-excluded, visible right edges and left edges.
    struct Edge { int left, right; };
    std::vector<Edge> edges;
    for (int i = 0; i < (int)shapes.size(); i++) {
        if (std::find(exclude.begin(), exclude.end(), i) != exclude.end()) continue;
        if (!shapes[i].visible) continue;
        edges.push_back({ shapes[i].bounds.x, shapes[i].bounds.x + shapes[i].bounds.width });
    }
    std::sort(edges.begin(), edges.end(), [](const Edge& a, const Edge& b){ return a.left < b.left; });

    wxRect pb = parentBounds;
    for (int i = 0; i + 1 < (int)edges.size(); i++) {
        int gapCenter = (edges[i].right + edges[i + 1].left) / 2;
        out.push_back({ gapCenter, 2, pb });
    }
    // Also snap to midpoint of parent edges (left-neighbor to parent-right, etc.)
    if (!edges.empty()) {
        out.push_back({ (edges.back().right + pb.x + pb.width) / 2, 2, pb });
        out.push_back({ (pb.x + edges.front().left)             / 2, 2, pb });
    }
}

static void AppendEqualSpacingY(const std::vector<DrawShape>& shapes,
                                  const std::vector<int>& exclude,
                                  const wxRect& parentBounds,
                                  std::vector<AxisVal>& out)
{
    struct Edge { int top, bottom; };
    std::vector<Edge> edges;
    for (int i = 0; i < (int)shapes.size(); i++) {
        if (std::find(exclude.begin(), exclude.end(), i) != exclude.end()) continue;
        if (!shapes[i].visible) continue;
        edges.push_back({ shapes[i].bounds.y, shapes[i].bounds.y + shapes[i].bounds.height });
    }
    std::sort(edges.begin(), edges.end(), [](const Edge& a, const Edge& b){ return a.top < b.top; });

    wxRect pb = parentBounds;
    for (int i = 0; i + 1 < (int)edges.size(); i++) {
        int gapCenter = (edges[i].bottom + edges[i + 1].top) / 2;
        out.push_back({ gapCenter, 2, pb });
    }
    if (!edges.empty()) {
        out.push_back({ (edges.back().bottom + pb.y + pb.height) / 2, 2, pb });
        out.push_back({ (pb.y + edges.front().top)               / 2, 2, pb });
    }
}

// Add 25%, 50%, 75% positions within the parent rect.
static void AppendPercentageX(const wxRect& parent, std::vector<AxisVal>& out) {
    for (int k : {1, 2, 3})
        out.push_back({ parent.x + parent.width * k / 4, 2, parent });
}
static void AppendPercentageY(const wxRect& parent, std::vector<AxisVal>& out) {
    for (int k : {1, 2, 3})
        out.push_back({ parent.y + parent.height * k / 4, 2, parent });
}

// ---------------------------------------------------------------------------
// Core snap: find best (emitter, target) pair on one axis within threshold.
// Returns true if a snap was found; populates correction and guide.
// ---------------------------------------------------------------------------

static bool FindSnap1D(const std::vector<AxisVal>& emitters,
                        const std::vector<AxisVal>& targets,
                        bool snapEdgeEdge,
                        bool snapCenterCenter,
                        bool snapEdgeCenter,
                        int threshold,
                        int& outCorrection,
                        wxRect& outEmitterBounds,
                        wxRect& outTargetBounds,
                        int& outSnapPos)
{
    int bestDist = threshold + 1;
    bool found = false;

    for (const auto& em : emitters) {
        for (const auto& tg : targets) {
            // Type compatibility: virtual (type 2) is treated as center (1)
            int et = (em.type == 2) ? 1 : em.type;
            int tt = (tg.type == 2) ? 1 : tg.type;

            bool ok = false;
            if (et == 0 && tt == 0) ok = snapEdgeEdge;
            else if (et == 1 && tt == 1) ok = snapCenterCenter;
            else if ((et == 0 && tt == 1) || (et == 1 && tt == 0)) ok = snapEdgeCenter;
            if (!ok) continue;

            int dist = std::abs(em.v - tg.v);
            if (dist < bestDist) {
                bestDist = dist;
                outCorrection   = tg.v - em.v;
                outEmitterBounds = em.bounds;
                outTargetBounds  = tg.bounds;
                outSnapPos       = tg.v;
                found = true;
            }
        }
    }
    return found;
}

// Build a guide line for an X-axis snap (vertical guide) or Y-axis snap (horizontal guide).
static SnapGuideLine MakeGuideX(int snapX,
                                  const wxRect& movingBounds,
                                  const wxRect& targetBounds,
                                  const wxRect& pageBounds,
                                  const SnapSettings& ss)
{
    SnapGuideLine g;
    g.isVertical  = true;
    g.pos         = snapX;
    g.extentMin   = std::min({ movingBounds.y, targetBounds.y, pageBounds.y });
    g.extentMax   = std::max({ movingBounds.y + movingBounds.height,
                               targetBounds.y + targetBounds.height,
                               pageBounds.y + pageBounds.height });
    g.colour      = ss.guideColour;
    g.lineWidth   = ss.guideLineWidth;
    return g;
}

static SnapGuideLine MakeGuideY(int snapY,
                                  const wxRect& movingBounds,
                                  const wxRect& targetBounds,
                                  const wxRect& pageBounds,
                                  const SnapSettings& ss)
{
    SnapGuideLine g;
    g.isVertical  = false;
    g.pos         = snapY;
    g.extentMin   = std::min({ movingBounds.x, targetBounds.x, pageBounds.x });
    g.extentMax   = std::max({ movingBounds.x + movingBounds.width,
                               targetBounds.x + targetBounds.width,
                               pageBounds.x + pageBounds.width });
    g.colour      = ss.guideColour;
    g.lineWidth   = ss.guideLineWidth;
    return g;
}

// Compute the union bounds of a set of shapes offset by a delta.
static wxRect MovingUnionBounds(const std::vector<DrawShape>& shapes, wxPoint delta) {
    if (shapes.empty()) return wxRect(0, 0, 0, 0);
    wxRect u = shapes[0].bounds;
    u.x += delta.x; u.y += delta.y;
    for (size_t i = 1; i < shapes.size(); i++) {
        wxRect r = shapes[i].bounds;
        r.x += delta.x; r.y += delta.y;
        u = u.Union(r);
    }
    return u;
}

// ---------------------------------------------------------------------------
// SnapEngine
// ---------------------------------------------------------------------------

SnapEngine::SnapEngine(const SnapSettings& settings) : m_s(settings) {}

int SnapEngine::ThresholdDocSpace(double zoom) const {
    return std::max(1, (int)std::ceil(m_s.pixelThreshold / zoom));
}

SnapResult SnapEngine::Snap(
    const std::vector<DrawShape>& movingShapes,
    wxPoint                       proposedDelta,
    const std::vector<DrawShape>& allShapes,
    const std::vector<int>&       excludeIndices,
    const wxRect&                 parentBounds,
    double                        zoom) const
{
    if (!m_s.enabled || movingShapes.empty())
        return { proposedDelta, {} };

    const int thresh = ThresholdDocSpace(zoom);

    // --- Build target candidates ---
    std::vector<AxisVal> targX, targY;
    CollectTargetXVals(allShapes, excludeIndices, m_s.snapLockedShapes, m_s.snapBezierPoints, targX);
    CollectTargetYVals(allShapes, excludeIndices, m_s.snapLockedShapes, m_s.snapBezierPoints, targY);
    if (m_s.snapEqualSpacing) {
        AppendEqualSpacingX(allShapes, excludeIndices, parentBounds, targX);
        AppendEqualSpacingY(allShapes, excludeIndices, parentBounds, targY);
    }
    if (m_s.snapPercentage) {
        AppendPercentageX(parentBounds, targX);
        AppendPercentageY(parentBounds, targY);
    }

    // --- Build emitter candidates at proposed position ---
    wxRect movingBounds = MovingUnionBounds(movingShapes, proposedDelta);

    std::vector<AxisVal> emitX, emitY;
    // Use the union bounds for the moving group as emitter
    emitX.push_back({ movingBounds.x,                       0, movingBounds });
    emitX.push_back({ movingBounds.x + movingBounds.width,  0, movingBounds });
    emitX.push_back({ movingBounds.x + movingBounds.width/2, 1, movingBounds });
    emitY.push_back({ movingBounds.y,                        0, movingBounds });
    emitY.push_back({ movingBounds.y + movingBounds.height,  0, movingBounds });
    emitY.push_back({ movingBounds.y + movingBounds.height/2, 1, movingBounds });

    // Also add bezier pts of moving shapes as emitters (if bezier)
    if (m_s.snapBezierPoints) {
        for (const DrawShape& s : movingShapes) {
            if (s.kind == ShapeKind::Bezier) {
                for (int k = 0; k < 4; k++) {
                    wxPoint p = { s.pts[k].x + proposedDelta.x, s.pts[k].y + proposedDelta.y };
                    wxRect ptBounds(p.x, p.y, 0, 0);
                    emitX.push_back({ p.x, 0, ptBounds });
                    emitY.push_back({ p.y, 0, ptBounds });
                }
            }
        }
    }

    // --- Snap X axis ---
    int corrX = 0, corrY = 0;
    wxRect emBoundsX, tgBoundsX, emBoundsY, tgBoundsY;
    int snapPosX = 0, snapPosY = 0;
    bool didX = FindSnap1D(emitX, targX,
                            m_s.snapEdgeToEdge, m_s.snapCenterToCenter, m_s.snapEdgeToCenter,
                            thresh, corrX, emBoundsX, tgBoundsX, snapPosX);
    bool didY = FindSnap1D(emitY, targY,
                            m_s.snapEdgeToEdge, m_s.snapCenterToCenter, m_s.snapEdgeToCenter,
                            thresh, corrY, emBoundsY, tgBoundsY, snapPosY);

    wxPoint snappedDelta = { proposedDelta.x + corrX, proposedDelta.y + corrY };
    wxRect snappedMoving = MovingUnionBounds(movingShapes, snappedDelta);

    std::vector<SnapGuideLine> guides;
    if (didX)
        guides.push_back(MakeGuideX(snapPosX, snappedMoving, tgBoundsX, parentBounds, m_s));
    if (didY)
        guides.push_back(MakeGuideY(snapPosY, snappedMoving, tgBoundsY, parentBounds, m_s));

    return { snappedDelta, std::move(guides) };
}

SnapResult SnapEngine::SnapBezierHandle(
    wxPoint                       proposedPt,
    wxPoint                       originalPt,
    const std::vector<DrawShape>& allShapes,
    const std::vector<int>&       excludeIndices,
    const wxRect&                 parentBounds,
    double                        zoom) const
{
    if (!m_s.enabled)
        return { proposedPt - originalPt, {} };

    const int thresh = ThresholdDocSpace(zoom);

    // Collect all target X/Y values (no type validation for point snap — match anything)
    std::vector<AxisVal> targX, targY;
    CollectTargetXVals(allShapes, excludeIndices, m_s.snapLockedShapes, m_s.snapBezierPoints, targX);
    CollectTargetYVals(allShapes, excludeIndices, m_s.snapLockedShapes, m_s.snapBezierPoints, targY);
    if (m_s.snapPercentage) {
        AppendPercentageX(parentBounds, targX);
        AppendPercentageY(parentBounds, targY);
    }

    // The emitter is the single proposed point — treat as edge type, matching edge targets.
    wxRect ptBounds(proposedPt.x, proposedPt.y, 1, 1);
    std::vector<AxisVal> emitX = {{ proposedPt.x, 0, ptBounds }};
    std::vector<AxisVal> emitY = {{ proposedPt.y, 0, ptBounds }};

    // For point snap, accept any (edge,edge), (center,center), (edge,center) pair.
    int corrX = 0, corrY = 0;
    wxRect emBX, tgBX, emBY, tgBY;
    int snapPosX = 0, snapPosY = 0;
    bool didX = FindSnap1D(emitX, targX, true, true, true, thresh, corrX, emBX, tgBX, snapPosX);
    bool didY = FindSnap1D(emitY, targY, true, true, true, thresh, corrY, emBY, tgBY, snapPosY);

    wxPoint snappedPt = { proposedPt.x + corrX, proposedPt.y + corrY };
    wxPoint snappedDelta = snappedPt - originalPt;

    wxRect snappedBounds(snappedPt.x, snappedPt.y, 1, 1);
    std::vector<SnapGuideLine> guides;
    if (didX) guides.push_back(MakeGuideX(snapPosX, snappedBounds, tgBX, parentBounds, m_s));
    if (didY) guides.push_back(MakeGuideY(snapPosY, snappedBounds, tgBY, parentBounds, m_s));

    return { snappedDelta, std::move(guides) };
}

SnapResult SnapEngine::SnapRubberBandCorner(
    wxPoint                       proposedCorner,
    const std::vector<DrawShape>& allShapes,
    const wxRect&                 parentBounds,
    double                        zoom) const
{
    if (!m_s.enabled)
        return { {0, 0}, {} };

    const int thresh = ThresholdDocSpace(zoom);

    std::vector<AxisVal> targX, targY;
    CollectTargetXVals(allShapes, {}, m_s.snapLockedShapes, m_s.snapBezierPoints, targX);
    CollectTargetYVals(allShapes, {}, m_s.snapLockedShapes, m_s.snapBezierPoints, targY);
    if (m_s.snapPercentage) {
        AppendPercentageX(parentBounds, targX);
        AppendPercentageY(parentBounds, targY);
    }

    wxRect ptBounds(proposedCorner.x, proposedCorner.y, 1, 1);
    std::vector<AxisVal> emitX = {{ proposedCorner.x, 0, ptBounds }};
    std::vector<AxisVal> emitY = {{ proposedCorner.y, 0, ptBounds }};

    int corrX = 0, corrY = 0;
    wxRect emBX, tgBX, emBY, tgBY;
    int snapPosX = 0, snapPosY = 0;
    bool didX = FindSnap1D(emitX, targX, true, true, true, thresh, corrX, emBX, tgBX, snapPosX);
    bool didY = FindSnap1D(emitY, targY, true, true, true, thresh, corrY, emBY, tgBY, snapPosY);

    wxPoint correction = { corrX, corrY };
    wxRect snappedBounds(proposedCorner.x + corrX, proposedCorner.y + corrY, 1, 1);
    std::vector<SnapGuideLine> guides;
    if (didX) guides.push_back(MakeGuideX(snapPosX, snappedBounds, tgBX, parentBounds, m_s));
    if (didY) guides.push_back(MakeGuideY(snapPosY, snappedBounds, tgBY, parentBounds, m_s));

    return { correction, std::move(guides) };
}
