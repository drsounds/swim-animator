#pragma once
#include <wx/docview.h>
#include <wx/panel.h>
#include "DrawShape.h"
#include "DrawIds.h"

class DrawDoc;
class DrawView;

enum class DrawTool { Select, Rect, Circle, Text };

// ---------------------------------------------------------------------------
// DrawCanvas – interactive wxPanel living inside the AUI notebook tab.
// ---------------------------------------------------------------------------
class DrawCanvas : public wxPanel {
public:
    DrawCanvas(DrawView* owner, wxWindow* parent);

    DrawTool  GetTool() const    { return m_tool; }
    void      SetTool(DrawTool t){ m_tool = t; }
    DrawView* GetView()          { return m_owner; }

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
    bool      m_dragging{ false };
    wxPoint   m_dragStart;
    wxPoint   m_dragCurrent;

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
    void PushHandler();
    void PopHandler();

    void OnToolSelect(wxCommandEvent&);
    void OnToolRect(wxCommandEvent&);
    void OnToolCircle(wxCommandEvent&);
    void OnToolText(wxCommandEvent&);
    void OnUpdateDrawTool(wxUpdateUIEvent&);

    DrawCanvas* m_canvas       { nullptr };
    bool        m_handlerPushed{ false  };

    wxDECLARE_EVENT_TABLE();
};
