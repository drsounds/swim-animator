#pragma once
#include <wx/panel.h>
#include <wx/listbox.h>
#include <wx/toolbar.h>
#include "DrawIds.h"

class SmilDoc;
class SmilView;

// ---------------------------------------------------------------------------
// ScenePanel – dockable panel that lists the scenes in the active SmilDoc.
//
// Selecting a scene switches the SmilView to edit that scene.
// Toolbar buttons: Add, Remove, Rename, Move Up, Move Down.
// ---------------------------------------------------------------------------
class ScenePanel : public wxPanel {
public:
    explicit ScenePanel(wxWindow* parent);

    // Called by MainFrame when the active SmilView changes.
    void Refresh(SmilView* view);

private:
    void RebuildList();
    wxString SceneLabel(int idx) const;

    void OnListSelection(wxCommandEvent& e);
    void OnAdd(wxCommandEvent&);
    void OnRemove(wxCommandEvent&);
    void OnRename(wxCommandEvent&);
    void OnMoveUp(wxCommandEvent&);
    void OnMoveDown(wxCommandEvent&);

    wxListBox* m_list{nullptr};
    SmilView*  m_view{nullptr};   // currently active view (may be null)

    wxDECLARE_EVENT_TABLE();
};
