#include "SmilView.h"
#include "SmilDoc.h"
#include "App.h"
#include "MainFrame.h"
#include "DrawCommands.h"
#include <wx/dcbuffer.h>
#include <wx/settings.h>
#include <algorithm>

// ---------------------------------------------------------------------------
// SyntheticDrawDoc
// ---------------------------------------------------------------------------

wxIMPLEMENT_DYNAMIC_CLASS(SyntheticDrawDoc, DrawDoc);

void SyntheticDrawDoc::UpdateAllViews(wxView* sender, wxObject* hint) {
    DrawDoc::UpdateAllViews(sender, hint);
    if (onChange) onChange();
}

// ---------------------------------------------------------------------------
// SmilProxyView
// ---------------------------------------------------------------------------

bool SmilProxyView::OnCreate(wxDocument* doc, long) {
    SetDocument(doc);
    doc->AddView(this);
    return true;
}

bool SmilProxyView::OnClose(bool) {
    if (GetDocument()) GetDocument()->RemoveView(this);
    return true;
}

void SmilProxyView::NotifySelectionChanged() {
    auto* mf = wxDynamicCast(wxGetApp().GetTopWindow(), MainFrame);
    if (!mf || !m_smilView) return;
    DrawDoc* doc = m_canvas ? wxDynamicCast(GetDocument(), DrawDoc) : nullptr;
    static const std::vector<int> kEmpty;
    const std::vector<int>& sel = m_canvas ? m_canvas->GetSelection() : kEmpty;
    // Forward to MainFrame as if this were a normal DrawDoc selection.
    mf->OnSelectionChanged(doc, sel);
    // Also notify the SMIL-specific keyframe panel.
    mf->OnSmilSelectionChanged(m_smilView, sel);
}

// ---------------------------------------------------------------------------
// SmilCanvas
// ---------------------------------------------------------------------------

wxBEGIN_EVENT_TABLE(SmilCanvas, wxPanel)
    EVT_SIZE(SmilCanvas::OnSize)
wxEND_EVENT_TABLE()

SmilCanvas::SmilCanvas(SmilView* owner, wxWindow* parent, SmilDoc* smilDoc)
    : wxPanel(parent, wxID_ANY)
    , m_owner(owner)
    , m_smilDoc(smilDoc)
{
    // Create a synthetic DrawDoc + a proxy DrawView that won't create a tab.
    m_syntheticDoc = new SyntheticDrawDoc();
    m_syntheticDoc->onChange = [this]() { WriteBackToScene(); };

    m_proxyView = new SmilProxyView();
    m_proxyView->m_smilView = m_owner;
    m_proxyView->OnCreate(m_syntheticDoc, 0L);

    // Create DrawCanvas as a child filling this panel.
    m_drawCanvas = new DrawCanvas(m_proxyView, this);
    m_proxyView->SetCanvas(m_drawCanvas);

    // Load initial scene content.
    LoadFromScene();
}

SmilCanvas::~SmilCanvas() {
    // Disconnect onChange before destruction to prevent callbacks into dead objects.
    if (m_syntheticDoc) m_syntheticDoc->onChange = nullptr;
    // proxyView and syntheticDoc are owned by wxDocument cleanup chain;
    // just close the view gracefully.
    if (m_proxyView) {
        m_proxyView->OnClose(false);
        delete m_proxyView;
        m_proxyView = nullptr;
    }
    delete m_syntheticDoc;
    m_syntheticDoc = nullptr;
}

void SmilCanvas::OnSize(wxSizeEvent& e) {
    if (m_drawCanvas)
        m_drawCanvas->SetSize(GetClientSize());
    e.Skip();
}

void SmilCanvas::LoadFromScene() {
    if (!m_smilDoc || !m_syntheticDoc || m_syncing) return;
    m_syncing = true;

    const SmilScene* sc = m_smilDoc->GetCurrentScene();
    if (!sc) { m_syncing = false; return; }

    // Clear synthetic doc and populate with current scene shapes.
    const auto& shapes = sc->shapes;
    // Remove all existing shapes.
    while (!m_syntheticDoc->GetShapes().empty())
        m_syntheticDoc->RemoveShape(0);

    // Build shapes with animated property values applied.
    int frame = m_smilDoc->GetCurrentFrame() - sc->startFrame;
    for (int i = 0; i < (int)shapes.size(); ++i) {
        DrawShape s = shapes[i];
        wxString eid = SmilDoc::ShapeElementId(s, i);

        // Apply animated attribute overrides at current frame.
        auto eit = sc->elements.find(eid);
        if (eit != sc->elements.end()) {
            for (const auto& [prop, track] : eit->second.tracks) {
                wxString val = track.GetValueAt(frame);
                if (val.IsEmpty()) continue;
                long lv = 0; double dv = 0;
                if (prop == "x" && val.ToLong(&lv))  s.bounds.x = (int)lv;
                else if (prop == "y" && val.ToLong(&lv)) s.bounds.y = (int)lv;
                else if (prop == "width"  && val.ToLong(&lv)) s.bounds.width  = (int)lv;
                else if (prop == "height" && val.ToLong(&lv)) s.bounds.height = (int)lv;
                else if (prop == "fill")   s.bgColour = wxColour(val);
                else if (prop == "stroke") s.fgColour = wxColour(val);
                else if (prop == "stroke-width" && val.ToLong(&lv)) s.strokeWidth = (int)lv;
                else if (prop == "opacity" && val.ToDouble(&dv)) { /* future alpha */ }
            }
        }
        m_syntheticDoc->AddShape(s);
    }

    // Set page dimensions from scene.
    m_syntheticDoc->SetPageSize(sc->pageWidth, sc->pageHeight);
    m_syntheticDoc->SetBgColour(sc->bgColour);

    m_syncing = false;
    if (m_drawCanvas) m_drawCanvas->Refresh();
}

void SmilCanvas::WriteBackToScene() {
    if (!m_smilDoc || !m_syntheticDoc || m_syncing) return;
    m_syncing = true;

    SmilScene* sc = m_smilDoc->GetCurrentScene();
    if (!sc) { m_syncing = false; return; }

    const std::vector<DrawShape>& newShapes = m_syntheticDoc->GetShapes();

    bool recMode = m_smilDoc->GetRecMode();
    int  frame   = m_smilDoc->GetCurrentFrame() - sc->startFrame;

    // For each shape, compare with old shape and, if REC, create keyframes.
    for (int i = 0; i < (int)newShapes.size() && i < (int)sc->shapes.size(); ++i) {
        const DrawShape& oldS = sc->shapes[i];
        const DrawShape& newS = newShapes[i];
        wxString eid = SmilDoc::ShapeElementId(newS, i);

        if (recMode) {
            auto addKf = [&](const wxString& prop, const wxString& val) {
                m_smilDoc->SetKeyframe(eid, prop, frame, val);
            };
            // Check which numeric properties changed.
            if (newS.bounds.x != oldS.bounds.x)
                addKf("x", wxString::Format("%d", newS.bounds.x));
            if (newS.bounds.y != oldS.bounds.y)
                addKf("y", wxString::Format("%d", newS.bounds.y));
            if (newS.bounds.width != oldS.bounds.width)
                addKf("width", wxString::Format("%d", newS.bounds.width));
            if (newS.bounds.height != oldS.bounds.height)
                addKf("height", wxString::Format("%d", newS.bounds.height));
            if (newS.bgColour != oldS.bgColour)
                addKf("fill", newS.bgColour.GetAsString(wxC2S_HTML_SYNTAX));
            if (newS.fgColour != oldS.fgColour)
                addKf("stroke", newS.fgColour.GetAsString(wxC2S_HTML_SYNTAX));
            if (newS.strokeWidth != oldS.strokeWidth)
                addKf("stroke-width", wxString::Format("%d", newS.strokeWidth));
        }
    }

    // Write shapes back to scene.
    sc->shapes.clear();
    for (const DrawShape& s : newShapes)
        sc->shapes.push_back(s);

    sc->bgColour   = m_syntheticDoc->GetBgColour();
    sc->pageWidth  = m_syntheticDoc->GetPageWidth();
    sc->pageHeight = m_syntheticDoc->GetPageHeight();

    m_smilDoc->Modify(true);

    m_syncing = false;
}

void SmilCanvas::RefreshFromDoc() {
    LoadFromScene();
}

// ---------------------------------------------------------------------------
// SmilView
// ---------------------------------------------------------------------------

wxIMPLEMENT_DYNAMIC_CLASS(SmilView, wxView);

bool SmilView::OnCreate(wxDocument* doc, long flags) {
    if (!wxView::OnCreate(doc, flags)) return false;

    auto* mf = wxDynamicCast(wxGetApp().GetTopWindow(), MainFrame);
    wxCHECK_MSG(mf, false, "Top window is not a MainFrame");

    wxAuiNotebook* nb = mf->GetNotebook();
    wxCHECK_MSG(nb, false, "MainFrame has no AUI notebook");

    SmilDoc* smilDoc = wxDynamicCast(doc, SmilDoc);
    wxCHECK_MSG(smilDoc, false, "Document is not a SmilDoc");

    m_canvas = new SmilCanvas(this, nb, smilDoc);
    nb->AddPage(m_canvas, doc->GetUserReadableName(), /*select=*/true);

    Activate(true);
    return true;
}

void SmilView::OnDraw(wxDC* /*dc*/) {}

void SmilView::OnUpdate(wxView* /*sender*/, wxObject* /*hint*/) {
    if (m_canvas) m_canvas->RefreshFromDoc();
}

void SmilView::OnActivateView(bool activate, wxView*, wxView*) {
    auto* mf = wxDynamicCast(wxGetApp().GetTopWindow(), MainFrame);
    if (!mf) return;

    if (activate) {
        mf->SetActiveSmilView(this);

        // Sync notebook tab.
        wxAuiNotebook* nb = mf->GetNotebook();
        if (nb && m_canvas) {
            int idx = nb->GetPageIndex(m_canvas);
            if (idx != wxNOT_FOUND && nb->GetSelection() != idx)
                nb->SetSelection(idx);
        }
    } else {
        mf->SetActiveSmilView(nullptr);
    }
}

bool SmilView::OnClose(bool deleteWindow) {
    if (!wxView::OnClose(deleteWindow)) return false;

    Activate(false);

    auto* mf = wxDynamicCast(wxGetApp().GetTopWindow(), MainFrame);
    if (mf) mf->SetActiveSmilView(nullptr);

    if (deleteWindow && m_canvas) {
        if (mf) {
            wxAuiNotebook* nb = mf->GetNotebook();
            if (nb) {
                int idx = nb->GetPageIndex(m_canvas);
                if (idx != wxNOT_FOUND) nb->DeletePage(idx);
            }
        }
        m_canvas = nullptr;
    }
    return true;
}
