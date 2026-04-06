#include "DrawView.h"
#include "DrawDoc.h"
#include "App.h"
#include "MainFrame.h"
#include <wx/dcbuffer.h>
#include <wx/settings.h>
#include <wx/colordlg.h>
#include <wx/textdlg.h>
#include <wx/msgdlg.h>
#include <wx/sizer.h>
#include <wx/stattext.h>
#include <wx/button.h>
#include <wx/textctrl.h>

// ---------------------------------------------------------------------------
// Helpers
// ---------------------------------------------------------------------------

static wxRect NormalizedRect(const wxPoint& a, const wxPoint& b) {
    int x = a.x < b.x ? a.x : b.x;
    int y = a.y < b.y ? a.y : b.y;
    int w = a.x < b.x ? b.x - a.x : a.x - b.x;
    int h = a.y < b.y ? b.y - a.y : a.y - b.y;
    return wxRect(x, y, w, h);
}

// ---------------------------------------------------------------------------
// ShapePropertiesDialog
// ---------------------------------------------------------------------------

class ShapePropertiesDialog : public wxDialog {
public:
    ShapePropertiesDialog(wxWindow* parent, const DrawShape& s)
        : wxDialog(parent, wxID_ANY, "Shape Properties",
                   wxDefaultPosition, wxDefaultSize, wxDEFAULT_DIALOG_STYLE)
        , m_fg(s.fgColour), m_bg(s.bgColour), m_label(s.label)
        , m_kind(s.kind)
    {
        auto* outer = new wxBoxSizer(wxVERTICAL);
        auto* grid  = new wxFlexGridSizer(2, wxSize(8, 6));
        grid->AddGrowableCol(1);

        // Label field (Text shapes only)
        if (s.kind == ShapeKind::Text) {
            grid->Add(new wxStaticText(this, wxID_ANY, "Label:"),
                      0, wxALIGN_CENTER_VERTICAL);
            m_labelCtrl = new wxTextCtrl(this, wxID_ANY, s.label);
            grid->Add(m_labelCtrl, 1, wxEXPAND);
        }

        // Foreground
        grid->Add(new wxStaticText(this, wxID_ANY, "Foreground:"),
                  0, wxALIGN_CENTER_VERTICAL);
        m_fgBtn = new wxButton(this, wxID_ANY, wxEmptyString,
                               wxDefaultPosition, wxSize(80, 22));
        m_fgBtn->SetBackgroundColour(m_fg);
        grid->Add(m_fgBtn, 0);

        // Background
        grid->Add(new wxStaticText(this, wxID_ANY, "Background:"),
                  0, wxALIGN_CENTER_VERTICAL);
        m_bgBtn = new wxButton(this, wxID_ANY, wxEmptyString,
                               wxDefaultPosition, wxSize(80, 22));
        m_bgBtn->SetBackgroundColour(m_bg);
        grid->Add(m_bgBtn, 0);

        outer->Add(grid, 1, wxEXPAND | wxALL, 10);
        outer->Add(CreateButtonSizer(wxOK | wxCANCEL), 0, wxEXPAND | wxLEFT | wxRIGHT | wxBOTTOM, 10);
        SetSizerAndFit(outer);

        m_fgBtn->Bind(wxEVT_BUTTON, [this](wxCommandEvent&) { PickColour(m_fg, m_fgBtn); });
        m_bgBtn->Bind(wxEVT_BUTTON, [this](wxCommandEvent&) { PickColour(m_bg, m_bgBtn); });
    }

    // Apply the dialog result back to a shape.
    void Apply(DrawShape& s) const {
        s.fgColour = m_fg;
        s.bgColour = m_bg;
        if (m_kind == ShapeKind::Text && m_labelCtrl)
            s.label = m_labelCtrl->GetValue();
    }

private:
    void PickColour(wxColour& colour, wxButton* btn) {
        wxColourData cd;
        cd.SetChooseFull(true);
        cd.SetColour(colour);
        wxColourDialog dlg(this, &cd);
        if (dlg.ShowModal() == wxID_OK) {
            colour = dlg.GetColourData().GetColour();
            btn->SetBackgroundColour(colour);
            btn->Refresh();
        }
    }

    wxColour    m_fg, m_bg;
    wxString    m_label;
    ShapeKind   m_kind;
    wxButton*   m_fgBtn   { nullptr };
    wxButton*   m_bgBtn   { nullptr };
    wxTextCtrl* m_labelCtrl{ nullptr };
};

// ---------------------------------------------------------------------------
// DrawCanvas – event table
// ---------------------------------------------------------------------------

wxBEGIN_EVENT_TABLE(DrawCanvas, wxPanel)
    EVT_PAINT(    DrawCanvas::OnPaint)
    EVT_SIZE(     DrawCanvas::OnSize)
    EVT_LEFT_DOWN(DrawCanvas::OnLeftDown)
    EVT_LEFT_UP(  DrawCanvas::OnLeftUp)
    EVT_MOTION(   DrawCanvas::OnMotion)
    EVT_LEFT_DCLICK(DrawCanvas::OnLeftDClick)
    EVT_KEY_DOWN( DrawCanvas::OnKeyDown)
wxEND_EVENT_TABLE()

DrawCanvas::DrawCanvas(DrawView* owner, wxWindow* parent)
    : wxPanel(parent, wxID_ANY, wxDefaultPosition, wxDefaultSize,
              wxFULL_REPAINT_ON_RESIZE | wxWANTS_CHARS)
    , m_owner(owner)
{
    SetBackgroundStyle(wxBG_STYLE_PAINT);
}

DrawDoc* DrawCanvas::GetDoc() {
    return m_owner ? wxDynamicCast(m_owner->GetDocument(), DrawDoc) : nullptr;
}

// ---------------------------------------------------------------------------
// DrawCanvas – rendering
// ---------------------------------------------------------------------------

void DrawCanvas::DrawShapeOnDC(wxDC& dc, const DrawShape& s, bool selected) {
    dc.SetPen(wxPen(s.fgColour, 2));
    dc.SetBrush(wxBrush(s.bgColour));

    switch (s.kind) {
        case ShapeKind::Rect:
            dc.DrawRectangle(s.bounds);
            break;
        case ShapeKind::Circle:
            dc.DrawEllipse(s.bounds);
            break;
        case ShapeKind::Text:
            dc.SetPen(*wxTRANSPARENT_PEN);
            dc.DrawRectangle(s.bounds);
            dc.SetFont(wxSystemSettings::GetFont(wxSYS_DEFAULT_GUI_FONT));
            dc.SetTextForeground(s.fgColour);
            dc.SetTextBackground(s.bgColour);
            dc.DrawLabel(s.label, s.bounds, wxALIGN_CENTER);
            break;
    }

    if (selected) {
        dc.SetPen(wxPen(*wxBLUE, 1, wxPENSTYLE_DOT));
        dc.SetBrush(*wxTRANSPARENT_BRUSH);
        wxRect sel = s.bounds;
        dc.DrawRectangle(sel.Inflate(3));
    }
}

void DrawCanvas::OnPaint(wxPaintEvent&) {
    wxAutoBufferedPaintDC dc(this);
    dc.SetBackground(*wxWHITE_BRUSH);
    dc.Clear();

    auto* doc = GetDoc();
    if (!doc) return;

    const auto& shapes = doc->GetShapes();
    for (int i = 0; i < (int)shapes.size(); i++)
        DrawShapeOnDC(dc, shapes[i], i == m_selected);

    // Rubber-band preview while drawing a new shape
    if (m_dragging && m_tool != DrawTool::Select) {
        wxRect r = NormalizedRect(m_dragStart, m_dragCurrent);
        if (r.width > 0 && r.height > 0) {
            dc.SetPen(wxPen(*wxBLACK, 1, wxPENSTYLE_DOT));
            dc.SetBrush(*wxTRANSPARENT_BRUSH);
            if (m_tool == DrawTool::Circle)
                dc.DrawEllipse(r);
            else
                dc.DrawRectangle(r);
        }
    }
}

void DrawCanvas::OnSize(wxSizeEvent& e) {
    Refresh();
    e.Skip();
}

// ---------------------------------------------------------------------------
// DrawCanvas – mouse & keyboard interaction
// ---------------------------------------------------------------------------

int DrawCanvas::HitTest(const wxPoint& pt) const {
    auto* doc = m_owner ? wxDynamicCast(m_owner->GetDocument(), DrawDoc) : nullptr;
    if (!doc) return -1;
    const auto& shapes = doc->GetShapes();
    // Test in reverse draw order so topmost shape wins.
    for (int i = (int)shapes.size() - 1; i >= 0; i--)
        if (shapes[i].HitTest(pt))
            return i;
    return -1;
}

void DrawCanvas::OnLeftDown(wxMouseEvent& e) {
    SetFocus();

    if (m_tool == DrawTool::Select) {
        m_selected = HitTest(e.GetPosition());
        Refresh();
    } else {
        m_dragging    = true;
        m_dragStart   = e.GetPosition();
        m_dragCurrent = m_dragStart;
        CaptureMouse();
    }
    e.Skip();
}

void DrawCanvas::OnMotion(wxMouseEvent& e) {
    if (m_dragging) {
        m_dragCurrent = e.GetPosition();
        Refresh();
    }
    e.Skip();
}

void DrawCanvas::OnLeftUp(wxMouseEvent& e) {
    if (!m_dragging) { e.Skip(); return; }

    m_dragging = false;
    if (HasCapture()) ReleaseMouse();

    wxRect r = NormalizedRect(m_dragStart, e.GetPosition());
    if (r.width < 4 || r.height < 4) { Refresh(); return; } // too small

    DrawShape s;
    s.bounds = r;

    switch (m_tool) {
        case DrawTool::Rect:   s.kind = ShapeKind::Rect;   break;
        case DrawTool::Circle: s.kind = ShapeKind::Circle; break;
        case DrawTool::Text: {
            wxTextEntryDialog dlg(this, "Label:", "Add Text");
            if (dlg.ShowModal() != wxID_OK) { Refresh(); return; }
            s.kind  = ShapeKind::Text;
            s.label = dlg.GetValue();
            break;
        }
        default: return;
    }

    auto* doc = GetDoc();
    if (doc)
        m_selected = doc->AddShape(s);

    Refresh();
    e.Skip();
}

void DrawCanvas::OnLeftDClick(wxMouseEvent& e) {
    if (m_tool == DrawTool::Select && m_selected >= 0)
        OpenPropertiesDialog(m_selected);
    e.Skip();
}

void DrawCanvas::OnKeyDown(wxKeyEvent& e) {
    if (e.GetKeyCode() == WXK_DELETE && m_selected >= 0) {
        auto* doc = GetDoc();
        if (doc) {
            doc->RemoveShape(m_selected);
            m_selected = -1;
            Refresh();
        }
    } else {
        e.Skip();
    }
}

void DrawCanvas::OpenPropertiesDialog(int idx) {
    auto* doc = GetDoc();
    if (!doc) return;
    const auto& shapes = doc->GetShapes();
    if (idx < 0 || idx >= (int)shapes.size()) return;

    DrawShape s = shapes[idx];
    ShapePropertiesDialog dlg(this, s);
    if (dlg.ShowModal() == wxID_OK) {
        dlg.Apply(s);
        doc->UpdateShape(idx, s);
    }
}

// ---------------------------------------------------------------------------
// DrawView – event table
// ---------------------------------------------------------------------------

wxBEGIN_EVENT_TABLE(DrawView, wxView)
    EVT_MENU(ID_TOOL_SELECT, DrawView::OnToolSelect)
    EVT_MENU(ID_TOOL_RECT,   DrawView::OnToolRect)
    EVT_MENU(ID_TOOL_CIRCLE, DrawView::OnToolCircle)
    EVT_MENU(ID_TOOL_TEXT,   DrawView::OnToolText)
    EVT_UPDATE_UI(ID_TOOL_SELECT, DrawView::OnUpdateDrawTool)
    EVT_UPDATE_UI(ID_TOOL_RECT,   DrawView::OnUpdateDrawTool)
    EVT_UPDATE_UI(ID_TOOL_CIRCLE, DrawView::OnUpdateDrawTool)
    EVT_UPDATE_UI(ID_TOOL_TEXT,   DrawView::OnUpdateDrawTool)
wxEND_EVENT_TABLE()

wxIMPLEMENT_DYNAMIC_CLASS(DrawView, wxView);

// ---------------------------------------------------------------------------
// DrawView – lifecycle
// ---------------------------------------------------------------------------

void DrawView::PushHandler() {
    if (!m_handlerPushed) {
        wxGetApp().GetTopWindow()->PushEventHandler(this);
        m_handlerPushed = true;
    }
}

void DrawView::PopHandler() {
    if (m_handlerPushed) {
        wxGetApp().GetTopWindow()->RemoveEventHandler(this);
        m_handlerPushed = false;
    }
}

bool DrawView::OnCreate(wxDocument* doc, long flags) {
    if (!wxView::OnCreate(doc, flags)) return false;

    auto* mf = wxDynamicCast(wxGetApp().GetTopWindow(), MainFrame);
    wxCHECK_MSG(mf, false, "Top window is not a MainFrame");

    wxAuiNotebook* nb = mf->GetNotebook();
    wxCHECK_MSG(nb, false, "MainFrame has no AUI notebook");

    m_canvas = new DrawCanvas(this, nb);
    nb->AddPage(m_canvas, doc->GetUserReadableName(), /*select=*/true);

    Activate(true);
    return true;
}

void DrawView::OnDraw(wxDC* /*dc*/) {
    // Rendering is handled entirely inside DrawCanvas::OnPaint.
}

void DrawView::OnUpdate(wxView* /*sender*/, wxObject* /*hint*/) {
    if (m_canvas) m_canvas->Refresh();
}

void DrawView::OnActivateView(bool activate, wxView* /*activeView*/,
                              wxView* /*deactiveView*/) {
    if (activate) {
        PushHandler();
        // Sync notebook selection when activated programmatically.
        if (m_canvas) {
            auto* mf = wxDynamicCast(wxGetApp().GetTopWindow(), MainFrame);
            if (mf) {
                wxAuiNotebook* nb = mf->GetNotebook();
                if (nb) {
                    int idx = nb->GetPageIndex(m_canvas);
                    if (idx != wxNOT_FOUND && nb->GetSelection() != idx)
                        nb->SetSelection(idx);
                }
            }
        }
    } else {
        PopHandler();
    }
}

bool DrawView::OnClose(bool deleteWindow) {
    if (!wxView::OnClose(deleteWindow)) return false;

    Activate(false);  // triggers OnActivateView(false) → PopHandler()
    PopHandler();     // safe no-op if already popped

    if (deleteWindow && m_canvas) {
        auto* mf = wxDynamicCast(wxGetApp().GetTopWindow(), MainFrame);
        if (mf) {
            wxAuiNotebook* nb = mf->GetNotebook();
            if (nb) {
                int idx = nb->GetPageIndex(m_canvas);
                if (idx != wxNOT_FOUND)
                    nb->DeletePage(idx);
            }
        }
        m_canvas = nullptr;
    }
    return true;
}

// ---------------------------------------------------------------------------
// DrawView – toolbar command handlers
// ---------------------------------------------------------------------------

void DrawView::OnToolSelect(wxCommandEvent&) { if (m_canvas) m_canvas->SetTool(DrawTool::Select); }
void DrawView::OnToolRect  (wxCommandEvent&) { if (m_canvas) m_canvas->SetTool(DrawTool::Rect);   }
void DrawView::OnToolCircle(wxCommandEvent&) { if (m_canvas) m_canvas->SetTool(DrawTool::Circle); }
void DrawView::OnToolText  (wxCommandEvent&) { if (m_canvas) m_canvas->SetTool(DrawTool::Text);   }

void DrawView::OnUpdateDrawTool(wxUpdateUIEvent& e) {
    e.Enable(true);
    if (!m_canvas) return;
    DrawTool cur = m_canvas->GetTool();
    e.Check((e.GetId() == ID_TOOL_SELECT && cur == DrawTool::Select) ||
            (e.GetId() == ID_TOOL_RECT   && cur == DrawTool::Rect)   ||
            (e.GetId() == ID_TOOL_CIRCLE && cur == DrawTool::Circle) ||
            (e.GetId() == ID_TOOL_TEXT   && cur == DrawTool::Text));
}
