#pragma once
#include <wx/docview.h>
#include <wx/panel.h>

class SpaView;
class SpaDoc;
class SmilView;

// ---------------------------------------------------------------------------
// SpaCanvas – the notebook tab panel for an open .spa project.
// Hosts an embedded SmilCanvas (via SmilView) that shows index.smil.
// ---------------------------------------------------------------------------
class SpaCanvas : public wxPanel {
public:
    SpaCanvas(SpaView* owner, wxWindow* parent);
    SpaView* GetView() { return m_owner; }

private:
    SpaView* m_owner;
};

// ---------------------------------------------------------------------------
// SpaView – connects SpaDoc to a SpaCanvas notebook tab that embeds the
// index.smil SmilView.
// ---------------------------------------------------------------------------
class SpaView : public wxView {
    wxDECLARE_DYNAMIC_CLASS(SpaView);
public:
    SpaView() = default;
    ~SpaView() override;

    bool OnCreate(wxDocument* doc, long flags) override;
    void OnDraw(wxDC* dc) override;
    void OnUpdate(wxView* sender, wxObject* hint) override;
    bool OnClose(bool deleteWindow) override;
    void OnActivateView(bool activate, wxView* active, wxView* deactive) override;

    SpaCanvas* GetCanvas() { return m_canvas; }

private:
    SpaCanvas* m_canvas{nullptr};
    SmilView*  m_embeddedSmilView{nullptr};
};
