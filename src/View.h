#pragma once
#include <wx/docview.h>
#include <wx/panel.h>
#include <wx/dc.h>

// Forward declarations
class View;
class Document;

// ---------------------------------------------------------------------------
// ViewCanvas – the actual window that lives inside the AUI notebook tab.
// Add your drawing / widget code here.
// ---------------------------------------------------------------------------
class ViewCanvas : public wxPanel {
public:
    ViewCanvas(View* owner, wxWindow* parent, wxWindowID id = wxID_ANY);

    View*     GetView()     { return m_owner; }
    Document* GetDocument();

private:
    void OnPaint(wxPaintEvent& event);
    void OnSize(wxSizeEvent& event);

    View* m_owner;

    wxDECLARE_EVENT_TABLE();
};

// ---------------------------------------------------------------------------
// View – connects a Document to a ViewCanvas tab in the AUI notebook.
// ---------------------------------------------------------------------------
class View : public wxView {
    wxDECLARE_DYNAMIC_CLASS(View);

public:
    View() = default;

    // Called by wxDocManager when a new view is needed.
    bool OnCreate(wxDocument* doc, long flags) override;

    // Called whenever the view should repaint its data.
    void OnDraw(wxDC* dc) override;

    // Called when the document signals that its data changed.
    void OnUpdate(wxView* sender, wxObject* hint = nullptr) override;

    // Called when the view (and optionally its window) should be destroyed.
    bool OnClose(bool deleteWindow = true) override;

    // Activate / deactivate this view (notebook page selection).
    void OnActivateView(bool activate, wxView* activeView,
                        wxView* deactiveView) override;

    ViewCanvas* GetCanvas() { return m_canvas; }

private:
    ViewCanvas* m_canvas{nullptr};
};
