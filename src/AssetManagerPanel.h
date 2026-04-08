#pragma once
#include <wx/panel.h>
#include <wx/treectrl.h>
#include <wx/notebook.h>
#include <wx/stattext.h>
#include <wx/button.h>
#include "SpaDoc.h"

// ---------------------------------------------------------------------------
// AssetManagerPanel – dockable pane for browsing and managing assets in the
// currently active .spa project document.
//
// Shows two views of the asset list:
//   "By Storage" – reflects the physical directory structure inside the bundle
//   "By Type"    – groups assets by media category (Images, Audio, Video, …)
//
// When no .spa document is active the panel shows a "No project open" label.
// ---------------------------------------------------------------------------
class AssetManagerPanel : public wxPanel {
public:
    explicit AssetManagerPanel(wxWindow* parent);

    // Called by MainFrame whenever the active SpaDoc changes (including nullptr).
    void Refresh(SpaDoc* doc);

private:
    // Tree population
    void PopulateStorageTree();
    void PopulateTypeTree();

    // Event handlers
    void OnAdd(wxCommandEvent&);
    void OnRemove(wxCommandEvent&);
    void OnRename(wxCommandEvent&);
    void OnItemActivated(wxTreeEvent&);

    // ---- Controls ----
    wxStaticText* m_noDocLabel   {nullptr};
    wxPanel*      m_contentPanel {nullptr};
    wxNotebook*   m_viewBook     {nullptr};
    wxTreeCtrl*   m_storageTree  {nullptr};
    wxTreeCtrl*   m_typeTree     {nullptr};
    wxButton*     m_addBtn       {nullptr};
    wxButton*     m_removeBtn    {nullptr};
    wxButton*     m_renameBtn    {nullptr};

    // ---- State ----
    SpaDoc* m_doc{nullptr};
};

// Tree item data – stores the index into SpaDoc::GetAssets(), or -1 for
// scene items and folder/category nodes.
class AssetItemData : public wxTreeItemData {
public:
    int      assetIdx{-1};
    bool     isScene{false};
    wxString logicalPath;   // set for both assets and scenes

    AssetItemData(int idx, const wxString& lp)
        : assetIdx(idx), logicalPath(lp) {}
    AssetItemData(bool scene, const wxString& lp)
        : isScene(scene), logicalPath(lp) {}
};
