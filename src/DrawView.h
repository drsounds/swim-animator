#pragma once
#include <wx/docview.h>
#include <wx/panel.h>
#include <vector>
#include "DrawShape.h"
#include "DrawIds.h"
#include "ShapePath.h"

class DrawDoc;
class DrawView;

enum class DrawTool { Select, Rect, Circle, Text, Bezier };

// ---------------------------------------------------------------------------
// DrawCanvas – interactive wxPanel living inside the AUI notebook tab.
// ---------------------------------------------------------------------------
class DrawCanvas : public wxPanel {
public:
    DrawCanvas(DrawView* owner, wxWindow* parent);

    DrawTool  GetTool() const       { return m_tool; }
    void      SetTool(DrawTool t)   { m_tool = t; m_bezierStep = 0; Refresh(); }
    DrawView* GetView()             { return m_owner; }

    // Returns the single "primary" selected shape index (-1 if none or multi).
    int GetSelectedIndex() const    { return m_selected; }

    // Returns the full selection set (root-level indices).
    const std::vector<int>& GetSelection() const { return m_selection; }

    // Set the selection externally (e.g. from the hierarchy panel).
    void SetSelection(const std::vector<int>& sel) {
        m_selection = sel;
        m_selected  = sel.empty() ? -1 : sel.back();
        Refresh();
    }

    void ValidateSelection();

    // Edit menu operations (called by MainFrame handlers).
    void CutShapes();
    void CopyShapes();
    void PasteShapes();
    void SelectAll();

    // Which select-mode drag is active (public so file-scope helpers can use it)
    enum class DragMode {
        None,
        Move,
        ResizeNW, ResizeN, ResizeNE,
        ResizeE,  ResizeSE, ResizeS,
        ResizeSW, ResizeW,
        BezierPt  // dragging one Bezier control point in Select mode
    };

private:
    DrawDoc* GetDoc();
    void DrawShapeOnDC(wxDC& dc, const DrawShape& s, bool selected);
    void DrawShapeOnDCRecursive(wxDC& dc, const DrawShape& s, bool selected);
    int  HitTest(const wxPoint& pt) const;
    void OpenPropertiesDialog(int idx);
    void ClearSelection();
    void SetSingleSelection(int idx);

    // Returns the shapes list currently in scope (root shapes, or active group's children).
    const std::vector<DrawShape>* CurrentShapes(DrawDoc* doc) const;
    // Returns the command path for shape `idx` in current scope.
    ShapePath CurrentPath(int idx) const;

    void OnPaint(wxPaintEvent&);
    void OnSize(wxSizeEvent&);
    void OnLeftDown(wxMouseEvent&);
    void OnLeftUp(wxMouseEvent&);
    void OnMotion(wxMouseEvent&);
    void OnMiddleDown(wxMouseEvent&);
    void OnMiddleUp(wxMouseEvent&);
    void OnMouseWheel(wxMouseEvent&);
    void OnLeftDClick(wxMouseEvent&);
    void OnKeyDown(wxKeyEvent&);
    void OnContextMenu(wxContextMenuEvent&);
    void OnCtxDelete(wxCommandEvent&);
    void OnCtxGroup(wxCommandEvent&);
    void OnCtxUngroup(wxCommandEvent&);
    void OnCtxProperties(wxCommandEvent&);

    // Convert between screen pixels and document (logical) coordinates.
    wxPoint ScreenToDoc(wxPoint screenPt) const;
    wxPoint DocToScreen(wxPoint docPt)    const;

    DrawView* m_owner;
    DrawTool  m_tool    { DrawTool::Select };

    // ---- Selection state ----
    int             m_selected  { -1 };         // primary selected index (single-select / drag)
    std::vector<int> m_selection;               // full selection set (multi-select)

    // ---- Group isolation mode ----
    // When >= 0: we are editing inside the root group at this index.
    // m_selected / m_selection then refer to indices within that group's children.
    int m_activeGroupIdx { -1 };

    // Shape-creation rubber-band state
    bool      m_dragging{ false };
    wxPoint   m_dragStart;
    wxPoint   m_dragCurrent;

    // Select-mode move/resize state (single shape)
    DragMode  m_dragMode         { DragMode::None };
    wxPoint   m_dragOrigin;
    wxRect    m_dragOrigBounds;
    wxPoint   m_dragOrigPts[4];  // original Bezier pts for Move / BezierPt drags
    DrawShape m_dragOrigShape;   // full before-state of the dragged shape (for undo)
    DrawShape m_dragPreview;     // current drag position rendered without touching doc
    int       m_bezierHandleIdx  { -1 };

    // Multi-shape drag state
    bool                   m_isMultiDrag  { false };
    wxPoint                m_multiDragOrigin;
    wxPoint                m_multiDragDelta;
    std::vector<DrawShape> m_multiDragShapes;  // original shapes for all selected

    // Rubber-band selection state (select tool, drag on empty area)
    bool    m_rubberBanding  { false };
    wxPoint m_rubberBandStart;    // doc coords
    wxPoint m_rubberBandCurrent;  // doc coords

    // Bezier creation state (active while DrawTool::Bezier is selected)
    int       m_bezierStep  { 0 };   // how many pts have been placed (0–3 during creation)
    wxPoint   m_bezierPts[4];        // pts placed so far

    // ---- Viewport state ----
    wxPoint m_viewOffset{0, 0};   // screen-space origin of the document (0,0) point
    double  m_zoom{1.0};          // current zoom factor

    // ---- Pan state (middle-mouse drag) ----
    bool    m_panning{false};
    wxPoint m_panStart;           // screen position where pan began
    wxPoint m_panStartOffset;     // m_viewOffset at the start of the pan

    wxDECLARE_EVENT_TABLE();
};

// ---------------------------------------------------------------------------
// DrawView – connects DrawDoc to a DrawCanvas notebook tab.
// ---------------------------------------------------------------------------
class DrawView : public wxView {
    wxDECLARE_DYNAMIC_CLASS(DrawView);
public:
    DrawView() = default;

    bool OnCreate(wxDocument*, long flags) override;
    void OnDraw(wxDC*) override;
    void OnUpdate(wxView*, wxObject* hint) override;
    bool OnClose(bool deleteWindow) override;
    void OnActivateView(bool activate, wxView*, wxView*) override;

    DrawCanvas* GetCanvas() { return m_canvas; }

    // Called by DrawCanvas and OnUpdate to push selection state to the properties pane.
    void NotifySelectionChanged();

private:
    DrawCanvas* m_canvas{ nullptr };
};
