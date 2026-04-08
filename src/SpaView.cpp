#include "SpaView.h"
#include "SpaDoc.h"
#include "App.h"
#include "MainFrame.h"
#include <wx/dcbuffer.h>
#include <wx/settings.h>

// ---------------------------------------------------------------------------
// SpaCanvas
// ---------------------------------------------------------------------------

wxBEGIN_EVENT_TABLE(SpaCanvas, wxPanel)
    EVT_PAINT(SpaCanvas::OnPaint)
    EVT_SIZE(SpaCanvas::OnSize)
wxEND_EVENT_TABLE()

SpaCanvas::SpaCanvas(SpaView* owner, wxWindow* parent)
    : wxPanel(parent, wxID_ANY, wxDefaultPosition, wxDefaultSize,
              wxFULL_REPAINT_ON_RESIZE)
    , m_owner(owner)
{
    SetBackgroundStyle(wxBG_STYLE_PAINT);
}

void SpaCanvas::OnPaint(wxPaintEvent&) {
    wxAutoBufferedPaintDC dc(this);
    dc.SetBackground(*wxWHITE_BRUSH);
    dc.Clear();
    if (m_owner)
        m_owner->OnDraw(&dc);
}

// ---------------------------------------------------------------------------
// SpaView
// ---------------------------------------------------------------------------

wxIMPLEMENT_DYNAMIC_CLASS(SpaView, wxView);

bool SpaView::OnCreate(wxDocument* doc, long flags) {
    if (!wxView::OnCreate(doc, flags))
        return false;

    auto* mainFrame = wxDynamicCast(wxGetApp().GetTopWindow(), MainFrame);
    wxCHECK_MSG(mainFrame, false, "Top window is not a MainFrame");

    wxAuiNotebook* nb = mainFrame->GetNotebook();
    wxCHECK_MSG(nb, false, "MainFrame has no AUI notebook");

    m_canvas = new SpaCanvas(this, nb);
    nb->AddPage(m_canvas, doc->GetUserReadableName(), /*select=*/true);

    Activate(true);
    return true;
}

void SpaView::OnDraw(wxDC* dc) {
    auto* doc = wxDynamicCast(GetDocument(), SpaDoc);
    if (!doc) return;

    dc->SetFont(wxSystemSettings::GetFont(wxSYS_DEFAULT_GUI_FONT));
    dc->SetTextForeground(*wxBLACK);

    int y = 12;
    const int lineH = dc->GetCharHeight() + 2;

    auto line = [&](const wxString& s) { dc->DrawText(s, 12, y); y += lineH; };

    line(wxString::Format("Project: %s", doc->GetFilename().IsEmpty()
                          ? "[Unsaved]" : doc->GetFilename()));
    y += 4;
    line(wxString::Format("Scenes:  %d", (int)doc->GetSceneOrder().size()));
    line(wxString::Format("Assets:  %d", (int)doc->GetAssets().size()));

    if (!doc->GetSceneOrder().empty()) {
        y += 6;
        line("--- Scenes ---");
        for (const auto& p : doc->GetSceneOrder())
            line("  " + p);
    }
}

void SpaView::OnUpdate(wxView* /*sender*/, wxObject* /*hint*/) {
    if (m_canvas) m_canvas->Refresh();
}

void SpaView::OnActivateView(bool activate, wxView* /*active*/, wxView* /*deactive*/) {
    if (!activate || !m_canvas) return;

    auto* mainFrame = wxDynamicCast(wxGetApp().GetTopWindow(), MainFrame);
    if (!mainFrame) return;

    // Sync notebook tab selection.
    wxAuiNotebook* nb = mainFrame->GetNotebook();
    if (nb) {
        int idx = nb->GetPageIndex(m_canvas);
        if (idx != wxNOT_FOUND && nb->GetSelection() != idx)
            nb->SetSelection(idx);
    }

    // Drive the asset manager pane.
    mainFrame->SetActiveSpaDoc(wxDynamicCast(GetDocument(), SpaDoc));
}

bool SpaView::OnClose(bool deleteWindow) {
    if (!wxView::OnClose(deleteWindow))
        return false;

    Activate(false);

    auto* mainFrame = wxDynamicCast(wxGetApp().GetTopWindow(), MainFrame);
    if (mainFrame)
        mainFrame->SetActiveSpaDoc(nullptr);

    if (deleteWindow && m_canvas) {
        if (mainFrame) {
            wxAuiNotebook* nb = mainFrame->GetNotebook();
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
