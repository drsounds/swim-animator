#pragma once
#include <wx/panel.h>
#include <wx/treectrl.h>
#include <vector>
#include "DrawShape.h"
#include "ShapePath.h"

class DrawDoc;
class MainFrame;

// ---------------------------------------------------------------------------
// HierarchyPanel – dockable pane that shows the shape tree for the active
// drawing document.
//
// Shapes are listed in Figma-style Z order: topmost shape (drawn last) appears
// at the top of the tree; Groups show their children nested below them.
//
// Bidirectional selection sync:
//   Canvas → hierarchy : call RefreshTree() after any selection change.
//   Hierarchy → canvas : emits a custom wxCommandEvent handled by MainFrame.
// ---------------------------------------------------------------------------
class HierarchyPanel : public wxPanel {
public:
    HierarchyPanel(wxWindow* parent);

    // Rebuild the entire tree from `doc`.  Pass the current canvas selection
    // so the matching tree items can be pre-selected.
    void RefreshTree(DrawDoc* doc, const std::vector<int>& selection);

    // Tree item data – stores the ShapePath so we can map tree items back to
    // shapes in O(1) without searching.
    struct ItemData : public wxTreeItemData {
        ShapePath path;
        explicit ItemData(ShapePath p) : path(std::move(p)) {}
    };

private:
    // Recursively add shapes to the tree under `parentItem`.
    // parentPath is the path prefix for all items at this level.
    void PopulateLevel(wxTreeItemId parentItem,
                       const std::vector<DrawShape>& shapes,
                       const ShapePath& parentPath,
                       const std::vector<int>& selection);

    wxString ShapeLabel(const DrawShape& s, int idx) const;

    void OnSelChanged(wxTreeEvent&);
    void OnBeginDrag(wxTreeEvent&);
    void OnEndDrag(wxTreeEvent&);
    void OnItemMenu(wxTreeEvent&);
    void OnContextItem(wxCommandEvent&);

    // Find the tree item whose ItemData has the given path.
    wxTreeItemId FindItemForPath(const ShapePath& path) const;
    wxTreeItemId FindItemForPathIn(wxTreeItemId root, const ShapePath& path) const;

    wxTreeCtrl*  m_tree    {nullptr};
    DrawDoc*     m_doc     {nullptr};
    bool         m_syncing {false};   // prevent re-entrant selection notifications
    wxTreeItemId m_dragItem;           // item being dragged (invalid when not dragging)

    wxDECLARE_EVENT_TABLE();
};
