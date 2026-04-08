#pragma once
#include <wx/docview.h>
#include <wx/panel.h>

class SpaView;
class SpaDoc;

// ---------------------------------------------------------------------------
// SpaCanvas – the notebook tab panel that represents an open .spa project.
// Shows a read-only project overview (scene list, asset count, file path).
// ---------------------------------------------------------------------------
class SpaCanvas : public wxPanel {
public:
    SpaCanvas(SpaView* owner, wxWindow* parent);
    SpaView* GetView() { return m_owner; }

private:
    void OnPaint(wxPaintEvent&);
    void OnSize(wxSizeEvent& e) { Refresh(); e.Skip(); }
    SpaView* m_owner;
    wxDECLARE_EVENT_TABLE();
};

// ---------------------------------------------------------------------------
// SpaView – connects SpaDoc to a SpaCanvas notebook tab.
// ---------------------------------------------------------------------------
class SpaView : public wxView {
    wxDECLARE_DYNAMIC_CLASS(SpaView);
public:
    SpaView() = default;

    bool OnCreate(wxDocument* doc, long flags) override;
    void OnDraw(wxDC* dc) override;
    void OnUpdate(wxView* sender, wxObject* hint) override;
    bool OnClose(bool deleteWindow) override;
    void OnActivateView(bool activate, wxView* active, wxView* deactive) override;

    SpaCanvas* GetCanvas() { return m_canvas; }

private:
    SpaCanvas* m_canvas{nullptr};
};
