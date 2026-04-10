#pragma once
#include <wx/docview.h>
#include <wx/panel.h>
#include "DrawView.h"
#include "DrawDoc.h"
#include "KeyframePanel.h"

class SmilView;
class SmilDoc;

// ---------------------------------------------------------------------------
// SyntheticDrawDoc
//
// A DrawDoc subclass used internally by SmilCanvas as a scratchpad that
// bridges the DrawCanvas editing API with SmilDoc scene data.  It fires an
// onChange callback whenever UpdateAllViews() is called so SmilCanvas can
// write edited shapes back to the real SmilDoc.
// ---------------------------------------------------------------------------
class SyntheticDrawDoc : public DrawDoc {
    wxDECLARE_DYNAMIC_CLASS(SyntheticDrawDoc);
public:
    std::function<void()> onChange;
    void UpdateAllViews(wxView* sender = nullptr,
                        wxObject* hint  = nullptr) override;
};

// ---------------------------------------------------------------------------
// SmilProxyView
//
// A minimal DrawView that binds a DrawCanvas to a SyntheticDrawDoc without
// creating a notebook tab.  Overrides NotifySelectionChanged to forward
// selection events to MainFrame using the SmilView's context.
// ---------------------------------------------------------------------------
class SmilProxyView : public DrawView {
public:
    SmilView* m_smilView{nullptr};  // back-pointer, set by SmilCanvas

    bool OnCreate(wxDocument* doc, long flags) override;
    void OnDraw(wxDC*) override {}
    void OnUpdate(wxView*, wxObject*) override {}
    bool OnClose(bool) override;
    void OnActivateView(bool, wxView*, wxView*) override {}
    void NotifySelectionChanged() override;

    // Allow SmilCanvas to wire up the DrawCanvas without exposing DrawView internals.
    void SetCanvas(DrawCanvas* c) { m_canvas = c; }
    DrawCanvas* GetCanvas() const { return m_canvas; }
};

// ---------------------------------------------------------------------------
// SmilCanvas
//
// The notebook tab panel for a SmilDoc.  Hosts a DrawCanvas child window that
// edits the shapes of the current scene.  When the scene or playhead changes,
// SmilCanvas reloads the SyntheticDrawDoc from the scene data (with animated
// property values applied at the current frame).  When DrawCanvas edits a
// shape, SmilCanvas writes the result back to the scene and, in REC mode,
// creates keyframes for changed properties.
// ---------------------------------------------------------------------------
class SmilCanvas : public wxPanel {
public:
    SmilCanvas(SmilView* owner, wxWindow* parent, SmilDoc* smilDoc);
    ~SmilCanvas();

    SmilView*   GetView()       { return m_owner; }
    SmilDoc*    GetSmilDoc()    { return m_smilDoc; }
    DrawCanvas* GetDrawCanvas() { return m_drawCanvas; }

    // Reload the SyntheticDrawDoc from the current scene/frame state.
    void RefreshFromDoc();

    // Refresh the embedded keyframe timeline.
    void RefreshKeyframes();

    KeyframePanel* GetKeyframePanel() { return m_keyframePanel; }

private:
    // Copy current scene shapes → SyntheticDrawDoc (applying animated values).
    void LoadFromScene();
    // Copy SyntheticDrawDoc shapes back to the current scene (+keyframes if REC).
    void WriteBackToScene();

    void OnSize(wxSizeEvent& e);

    SmilView*         m_owner;
    SmilDoc*          m_smilDoc;
    SyntheticDrawDoc* m_syntheticDoc{nullptr};
    SmilProxyView*    m_proxyView{nullptr};
    DrawCanvas*       m_drawCanvas{nullptr};
    KeyframePanel*    m_keyframePanel{nullptr};
    bool              m_syncing{false};

    wxDECLARE_EVENT_TABLE();
};

// ---------------------------------------------------------------------------
// SmilView
//
// wxView subclass for SmilDoc.  Creates a SmilCanvas notebook tab, wires up
// the scene/keyframe panels in MainFrame on activation.
// ---------------------------------------------------------------------------
class SmilView : public wxView {
    wxDECLARE_DYNAMIC_CLASS(SmilView);
public:
    SmilView() = default;

    bool OnCreate(wxDocument* doc, long flags) override;
    void OnDraw(wxDC* dc) override;
    void OnUpdate(wxView* sender, wxObject* hint) override;
    bool OnClose(bool deleteWindow) override;
    void OnActivateView(bool activate, wxView* active, wxView* deactive) override;

    SmilCanvas* GetCanvas() { return m_canvas; }

    // Embedded-mode creation: creates SmilCanvas as a child of `parent` and
    // registers this view with `doc`, without adding a notebook tab.
    // Returns the new SmilCanvas (owned by `parent`'s window hierarchy).
    SmilCanvas* CreateEmbedded(SmilDoc* doc, wxWindow* parent);

    // Null out the canvas pointer without destroying the window.
    // Call this before the parent window destroys the SmilCanvas.
    void DetachCanvas() { m_canvas = nullptr; }

private:
    SmilCanvas* m_canvas{nullptr};
};
