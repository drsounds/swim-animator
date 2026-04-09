#pragma once
#include <wx/panel.h>
#include <wx/scrolwin.h>
#include "SmilTypes.h"
#include "DrawShape.h"
#include <vector>

class SmilDoc;
class SmilView;

// ---------------------------------------------------------------------------
// KeyframePanel – dockable Flash-style timeline panel docked at the top.
//
// Layout (horizontal, left to right):
//   [ Label column (160 px) ] [ Timeline area (scrollable) ]
//
// Rows: one per DrawShape in the current scene, each expandable to show
//       per-property sub-rows.  A draggable playhead marks the current frame.
//       Keyframes are drawn as diamonds; clicking a diamond selects it.
//       Clicking empty space moves the playhead.
// ---------------------------------------------------------------------------
class KeyframePanel : public wxPanel {
public:
    explicit KeyframePanel(wxWindow* parent);

    // Called by MainFrame when the active SmilView or its state changes.
    void Refresh(SmilView* view);

    // Called after REC button toggled; updates button appearance.
    void UpdateRecButton(bool recActive);

    static constexpr int kLabelWidth  = 160;
    static constexpr int kRowHeight   = 20;
    static constexpr int kHeaderH     = 24;
    static constexpr int kFrameWidth  = 8;   // pixels per frame at scale 1

private:
    // Per-row entry: either a shape row or a property sub-row.
    struct Row {
        bool      isProperty{false};
        int       shapeIdx{-1};
        wxString  label;
        wxString  elemId;
        wxString  propName;   // empty for shape rows
    };

    void BuildRows();

    // Timeline painting helpers.
    void OnPaintLabels(wxPaintEvent&);
    void OnPaintTimeline(wxPaintEvent&);

    // Mouse interaction on the timeline panel.
    void OnTimelineLeftDown(wxMouseEvent&);
    void OnTimelineLeftUp(wxMouseEvent&);
    void OnTimelineMotion(wxMouseEvent&);

    // Toggle expand/collapse of a shape row.
    void OnLabelLeftDown(wxMouseEvent&);

    int  FrameAtX(int x) const;
    int  XAtFrame(int frame) const;
    int  TotalFrames() const;
    void MovePlayhead(int frame);

    // Draw a keyframe diamond centred at (cx, cy).
    void DrawDiamond(wxDC& dc, int cx, int cy, bool selected, bool hasKf);

    wxPanel*         m_labelPanel{nullptr};
    wxPanel*         m_timelinePanel{nullptr};
    SmilView*        m_view{nullptr};
    std::vector<Row> m_rows;
    std::vector<bool> m_expanded;  // one per shape (true = sub-rows visible)
    bool             m_draggingPlayhead{false};

    wxDECLARE_EVENT_TABLE();
};
