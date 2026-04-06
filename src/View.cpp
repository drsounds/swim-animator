#include "View.h"
#include "Document.h"
#include "App.h"
#include "MainFrame.h"
#include <wx/dcbuffer.h>
#include <wx/settings.h>

// ---------------------------------------------------------------------------
// ViewCanvas
// ---------------------------------------------------------------------------

wxBEGIN_EVENT_TABLE(ViewCanvas, wxPanel)
    EVT_PAINT(ViewCanvas::OnPaint)
    EVT_SIZE(ViewCanvas::OnSize)
wxEND_EVENT_TABLE()

ViewCanvas::ViewCanvas(View* owner, wxWindow* parent, wxWindowID id)
    : wxPanel(parent, id, wxDefaultPosition, wxDefaultSize,
              wxFULL_REPAINT_ON_RESIZE)
    , m_owner(owner)
{
    SetBackgroundStyle(wxBG_STYLE_PAINT);
}

Document* ViewCanvas::GetDocument() {
    return m_owner ? wxDynamicCast(m_owner->GetDocument(), Document) : nullptr;
}

void ViewCanvas::OnPaint(wxPaintEvent& /*event*/) {
    wxAutoBufferedPaintDC dc(this);
    dc.SetBackground(*wxWHITE_BRUSH);
    dc.Clear();

    if (m_owner)
        m_owner->OnDraw(&dc);
}

void ViewCanvas::OnSize(wxSizeEvent& event) {
    Refresh();
    event.Skip();
}

// ---------------------------------------------------------------------------
// View
// ---------------------------------------------------------------------------

wxIMPLEMENT_DYNAMIC_CLASS(View, wxView);

bool View::OnCreate(wxDocument* doc, long flags) {
    if (!wxView::OnCreate(doc, flags))
        return false;

    auto* mainFrame = wxDynamicCast(wxGetApp().GetTopWindow(), MainFrame);
    wxCHECK_MSG(mainFrame, false, "Top window is not a MainFrame");

    wxAuiNotebook* nb = mainFrame->GetNotebook();
    wxCHECK_MSG(nb, false, "MainFrame has no AUI notebook");

    m_canvas = new ViewCanvas(this, nb);

    // Use document title as the tab label; the title updates via OnChangeFilename.
    nb->AddPage(m_canvas, doc->GetUserReadableName(), /*select=*/true);

    Activate(true);
    return true;
}

void View::OnDraw(wxDC* dc) {
    auto* doc = wxDynamicCast(GetDocument(), Document);
    if (!doc)
        return;

    // Placeholder rendering – replace with your actual drawing logic.
    dc->SetFont(wxSystemSettings::GetFont(wxSYS_DEFAULT_GUI_FONT));
    dc->SetTextForeground(*wxBLACK);

    const wxString& content = doc->GetContent();
    if (content.IsEmpty()) {
        dc->DrawText("<empty document>", 8, 8);
    } else {
        wxCoord y = 8;
        for (const wxString& line : wxSplit(content, '\n')) {
            dc->DrawText(line, 8, y);
            y += dc->GetCharHeight();
        }
    }
}

void View::OnUpdate(wxView* /*sender*/, wxObject* /*hint*/) {
    if (m_canvas)
        m_canvas->Refresh();
}

void View::OnActivateView(bool activate, wxView* /*activeView*/,
                          wxView* /*deactiveView*/) {
    if (!m_canvas || !activate)
        return;

    // Sync the notebook selection to this view when it is activated
    // programmatically (e.g. via the Window menu).
    auto* mainFrame = wxDynamicCast(wxGetApp().GetTopWindow(), MainFrame);
    if (!mainFrame)
        return;

    wxAuiNotebook* nb = mainFrame->GetNotebook();
    if (!nb)
        return;

    int idx = nb->GetPageIndex(m_canvas);
    if (idx != wxNOT_FOUND && nb->GetSelection() != idx)
        nb->SetSelection(idx);
}

bool View::OnClose(bool deleteWindow) {
    if (!wxView::OnClose(deleteWindow))
        return false;

    Activate(false);

    if (deleteWindow && m_canvas) {
        auto* mainFrame = wxDynamicCast(wxGetApp().GetTopWindow(), MainFrame);
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
