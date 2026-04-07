#pragma once
#include <wx/docview.h>
#include <wx/panel.h>
#include "DrawShape.h"
#include "DrawIds.h"

class DrawDoc;
class DrawView;

enum class DrawTool { Select, Rect, Circle, Text, Bezier };

// ---------------------------------------------------------------------------
// DrawCanvas – interactive wxPanel living inside the AUI notebook tab.
// ---------------------------------------------------------------------------
class DrawCanvas : public wxPanel {
public:
    DrawCanvas(DrawView* owner, wxWindow* parent);

    DrawTool  GetTool() const    { return m_tool; }
    void      SetTool(DrawTool t){ m_tool = t; m_bezierStep = 0; Refresh(); }
    DrawView* GetView()          { return m_owner; }
    void      ValidateSelection();

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
    int  HitTest(const wxPoint& pt) const;
    void OpenPropertiesDialog(int idx);

    void OnPaint(wxPaintEvent&);
    void OnSize(wxSizeEvent&);
    void OnLeftDown(wxMouseEvent&);
    void OnLeftUp(wxMouseEvent&);
    void OnMotion(wxMouseEvent&);
    void OnLeftDClick(wxMouseEvent&);
    void OnKeyDown(wxKeyEvent&);

    DrawView* m_owner;
    DrawTool  m_tool    { DrawTool::Select };
    int       m_selected{ -1 };

    // Shape-creation rubber-band state
    bool      m_dragging{ false };
    wxPoint   m_dragStart;
    wxPoint   m_dragCurrent;

    // Select-mode move/resize state
    DragMode  m_dragMode         { DragMode::None };
    wxPoint   m_dragOrigin;
    wxRect    m_dragOrigBounds;
    wxPoint   m_dragOrigPts[4];  // original Bezier pts for Move / BezierPt drags
    DrawShape m_dragOrigShape;   // full before-state of the dragged shape (for undo)
    DrawShape m_dragPreview;     // current drag position rendered without touching doc
    int       m_bezierHandleIdx  { -1 };

    // Bezier creation state (active while DrawTool::Bezier is selected)
    int       m_bezierStep  { 0 };   // how many pts have been placed (0–3 during creation)
    wxPoint   m_bezierPts[4];        // pts placed so far

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

private:
    DrawCanvas* m_canvas{ nullptr };
};
