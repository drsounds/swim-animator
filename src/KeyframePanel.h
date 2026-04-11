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

    // Keyframe drag/edit state
    struct KeyframeDrag {
        wxString elemId;
        wxString propName;
        size_t   kfIdx{0};
        int      originalFrame{0};
        bool     isDragging{false};
    };

    // Clipboard for copy/paste operations
    struct KeyframeClip {
        wxString value;
        SmilInterp interp;
    };

    // Helper: find keyframe at given pixel coordinates
    // Returns (elemId, propName, kfIdx, frame) or empty if no hit
    bool HitTestKeyframe(int x, int y, wxString& elemId, wxString& propName, size_t& kfIdx, int& frame);

    // Helper: interpolate value at frame between two keyframes
    wxString InterpolateKeyframeValue(const wxString& elemId, const wxString& propName, int frame);

    // Context menu handlers
    void OnTimelineRightDown(wxMouseEvent&);
    void OnKeyframeContextMenu(wxCommandEvent&);

    wxPanel*         m_labelPanel{nullptr};
    wxPanel*         m_timelinePanel{nullptr};
    SmilView*        m_view{nullptr};
    std::vector<Row> m_rows;
    std::vector<bool> m_expanded;  // one per shape (true = sub-rows visible)
    bool             m_draggingPlayhead{false};
    KeyframeDrag     m_kfDrag;       // current keyframe drag state
    KeyframeClip     m_clipboard;    // clipboard for copy/paste
    wxString         m_clipElemId;   // which element the clipboard came from

    // Event IDs for context menu
    enum {
        ID_KF_DELETE = 10001,
        ID_KF_COPY = 10002,
        ID_KF_DUPLICATE = 10003,
        ID_KF_PASTE = 10004,
        ID_KF_PASTE_OFFSET = 10005,
    };

    wxDECLARE_EVENT_TABLE();
};
