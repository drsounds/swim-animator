#include "SpaView.h"
#include "SpaDoc.h"
#include "App.h"
#include "MainFrame.h"
#include "SmilView.h"
#include "SmilDoc.h"
#include <wx/sizer.h>

// ---------------------------------------------------------------------------
// SpaCanvas
// ---------------------------------------------------------------------------

SpaCanvas::SpaCanvas(SpaView* owner, wxWindow* parent)
    : wxPanel(parent, wxID_ANY)
    , m_owner(owner)
{
    Bind(wxEVT_SIZE, [this](wxSizeEvent& e) { Layout(); e.Skip(); });
}

// ---------------------------------------------------------------------------
// SpaView
// ---------------------------------------------------------------------------

wxIMPLEMENT_DYNAMIC_CLASS(SpaView, wxView);

SpaView::~SpaView() {
    // m_embeddedSmilView is cleaned up in OnClose(); guard against double-free
    // if the framework destroys the view without calling OnClose first.
    if (m_embeddedSmilView) {
        m_embeddedSmilView->DetachCanvas();
        if (m_embeddedSmilView->GetDocument())
            m_embeddedSmilView->GetDocument()->RemoveView(m_embeddedSmilView);
        delete m_embeddedSmilView;
        m_embeddedSmilView = nullptr;
    }
}

bool SpaView::OnCreate(wxDocument* doc, long flags) {
    if (!wxView::OnCreate(doc, flags))
        return false;

    auto* mainFrame = wxDynamicCast(wxGetApp().GetTopWindow(), MainFrame);
    wxCHECK_MSG(mainFrame, false, "Top window is not a MainFrame");

    wxAuiNotebook* nb = mainFrame->GetNotebook();
    wxCHECK_MSG(nb, false, "MainFrame has no AUI notebook");

    m_canvas = new SpaCanvas(this, nb);

    // Embed the index.smil SmilView inside the SpaCanvas before adding the
    // page to the notebook, so the sizer is in place for the notebook's
    // initial layout pass.
    auto* spaDoc = wxDynamicCast(doc, SpaDoc);
    SmilDoc* indexSmilDoc = spaDoc ? spaDoc->GetIndexSmilDoc() : nullptr;
    if (indexSmilDoc) {
        m_embeddedSmilView = new SmilView();
        SmilCanvas* smilCanvas = m_embeddedSmilView->CreateEmbedded(indexSmilDoc, m_canvas);
        auto* sizer = new wxBoxSizer(wxVERTICAL);
        sizer->Add(smilCanvas, 1, wxEXPAND);
        m_canvas->SetSizer(sizer);
    }

    nb->AddPage(m_canvas, doc->GetUserReadableName(), /*select=*/true);
    m_canvas->Layout();   // force layout after the notebook has sized the page

    // Wire the embedded SmilView to MainFrame immediately, before Activate(),
    // so panels update even if activation is delayed.
    if (m_embeddedSmilView)
        mainFrame->SetActiveSmilView(m_embeddedSmilView);

    Activate(true);
    return true;
}

void SpaView::OnDraw(wxDC* /*dc*/) {
    // Drawing is handled by the embedded SmilCanvas.
}

void SpaView::OnUpdate(wxView* /*sender*/, wxObject* /*hint*/) {
    if (m_embeddedSmilView && m_embeddedSmilView->GetCanvas())
        m_embeddedSmilView->GetCanvas()->RefreshFromDoc();
}

void SpaView::OnActivateView(bool activate, wxView* /*active*/, wxView* /*deactive*/) {
    auto* mainFrame = wxDynamicCast(wxGetApp().GetTopWindow(), MainFrame);
    if (!mainFrame) return;

    if (activate) {
        if (!m_canvas) return;

        // Sync notebook tab selection.
        wxAuiNotebook* nb = mainFrame->GetNotebook();
        if (nb) {
            int idx = nb->GetPageIndex(m_canvas);
            if (idx != wxNOT_FOUND && nb->GetSelection() != idx)
                nb->SetSelection(idx);
        }

        // Drive the asset manager and scene/draw-tool panels.
        mainFrame->SetActiveSpaDoc(wxDynamicCast(GetDocument(), SpaDoc));
        mainFrame->SetActiveSmilView(m_embeddedSmilView);
    } else {
        mainFrame->SetActiveSpaDoc(nullptr);
        mainFrame->SetActiveSmilView(nullptr);
    }
}

bool SpaView::OnClose(bool deleteWindow) {
    // Detach the embedded canvas before the window hierarchy destroys it.
    if (m_embeddedSmilView)
        m_embeddedSmilView->DetachCanvas();

    if (!wxView::OnClose(deleteWindow))
        return false;

    Activate(false);

    auto* mainFrame = wxDynamicCast(wxGetApp().GetTopWindow(), MainFrame);
    if (mainFrame) {
        mainFrame->SetActiveSpaDoc(nullptr);
        mainFrame->SetActiveSmilView(nullptr);
    }

    if (deleteWindow && m_canvas) {
        if (mainFrame) {
            wxAuiNotebook* nb = mainFrame->GetNotebook();
            if (nb) {
                int idx = nb->GetPageIndex(m_canvas);
                if (idx != wxNOT_FOUND)
                    nb->DeletePage(idx);  // destroys SpaCanvas and SmilCanvas
            }
        }
        m_canvas = nullptr;
    }

    // Unregister and delete the embedded SmilView now that its canvas is gone.
    if (m_embeddedSmilView) {
        if (m_embeddedSmilView->GetDocument())
            m_embeddedSmilView->GetDocument()->RemoveView(m_embeddedSmilView);
        delete m_embeddedSmilView;
        m_embeddedSmilView = nullptr;
    }

    return true;
}
