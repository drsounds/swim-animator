#include "HierarchyPanel.h"
#include "DrawDoc.h"
#include "DrawCommands.h"
#include "DrawIds.h"
#include "App.h"
#include "MainFrame.h"
#include <wx/menu.h>
#include <wx/filename.h>
#include <wx/textdlg.h>
#include <algorithm>

// ---------------------------------------------------------------------------
// Event table
// ---------------------------------------------------------------------------

wxBEGIN_EVENT_TABLE(HierarchyPanel, wxPanel)
    EVT_TREE_SEL_CHANGED(wxID_ANY, HierarchyPanel::OnSelChanged)
    EVT_TREE_BEGIN_DRAG( wxID_ANY, HierarchyPanel::OnBeginDrag)
    EVT_TREE_END_DRAG(   wxID_ANY, HierarchyPanel::OnEndDrag)
    EVT_TREE_ITEM_MENU(  wxID_ANY, HierarchyPanel::OnItemMenu)
    EVT_MENU(ID_HIER_TOGGLE_VISIBLE, HierarchyPanel::OnContextItem)
    EVT_MENU(ID_HIER_TOGGLE_LOCKED,  HierarchyPanel::OnContextItem)
    EVT_MENU(ID_HIER_RENAME,         HierarchyPanel::OnContextItem)
wxEND_EVENT_TABLE()

// ---------------------------------------------------------------------------
// Construction
// ---------------------------------------------------------------------------

HierarchyPanel::HierarchyPanel(wxWindow* parent)
    : wxPanel(parent, wxID_ANY)
{
    m_tree = new wxTreeCtrl(this, wxID_ANY, wxDefaultPosition, wxDefaultSize,
                            wxTR_DEFAULT_STYLE | wxTR_HIDE_ROOT |
                            wxTR_FULL_ROW_HIGHLIGHT | wxTR_MULTIPLE |
                            wxTR_EDIT_LABELS);

    auto* sizer = new wxBoxSizer(wxVERTICAL);
    sizer->Add(m_tree, 1, wxEXPAND);
    SetSizer(sizer);
}

// ---------------------------------------------------------------------------
// Label
// ---------------------------------------------------------------------------

wxString HierarchyPanel::ShapeLabel(const DrawShape& s, int idx) const {
    if (!s.name.IsEmpty()) return s.name;

    switch (s.kind) {
        case ShapeKind::Rect:   return wxString::Format("Rect %d",   idx + 1);
        case ShapeKind::Circle: return wxString::Format("Circle %d", idx + 1);
        case ShapeKind::Text:   return s.label.IsEmpty()
                                       ? wxString::Format("Text %d", idx + 1)
                                       : wxString::Format("\"%s\"", s.label.Left(24));
        case ShapeKind::Bezier: return wxString::Format("Bezier %d", idx + 1);
        case ShapeKind::Group:  return wxString::Format("Group %d",  idx + 1);
        case ShapeKind::SVGRef: return s.svgRefPath.IsEmpty()
                                       ? wxString::Format("SVGRef %d", idx + 1)
                                       : wxFileName(s.svgRefPath).GetName();
    }
    return wxString::Format("Shape %d", idx + 1);
}

// ---------------------------------------------------------------------------
// Tree population
// ---------------------------------------------------------------------------

void HierarchyPanel::PopulateLevel(wxTreeItemId parentItem,
                                   const std::vector<DrawShape>& shapes,
                                   const ShapePath& parentPath,
                                   const std::vector<int>& selection)
{
    // Add in reverse order: highest index (topmost Z) appears first in the tree.
    for (int i = (int)shapes.size() - 1; i >= 0; --i) {
        const DrawShape& s = shapes[i];

        ShapePath path = parentPath;
        path.push_back(i);

        wxString label = ShapeLabel(s, i);
        if (!s.visible) label = "[hidden] " + label;
        if (s.locked)   label = "[locked] " + label;

        wxTreeItemId item = m_tree->AppendItem(parentItem, label, -1, -1,
                                               new ItemData(path));

        // For root-level items, check if they are in the canvas selection.
        if (parentPath.empty()) {
            bool sel = std::find(selection.begin(), selection.end(), i) != selection.end();
            if (sel) m_tree->SelectItem(item, true);
        }

        if (s.kind == ShapeKind::Group && !s.children.empty())
            PopulateLevel(item, s.children, path, selection);
    }
}

void HierarchyPanel::RefreshTree(DrawDoc* doc, const std::vector<int>& selection) {
    m_syncing = true;
    m_doc = doc;
    m_tree->DeleteAllItems();

    if (!doc) {
        m_syncing = false;
        return;
    }

    wxTreeItemId root = m_tree->AddRoot("Document");
    PopulateLevel(root, doc->GetShapes(), {}, selection);
    m_tree->ExpandAll();

    m_syncing = false;
}

// ---------------------------------------------------------------------------
// Tree item lookup
// ---------------------------------------------------------------------------

wxTreeItemId HierarchyPanel::FindItemForPath(const ShapePath& path) const {
    wxTreeItemId root = m_tree->GetRootItem();
    if (!root.IsOk()) return {};
    return FindItemForPathIn(root, path);
}

wxTreeItemId HierarchyPanel::FindItemForPathIn(wxTreeItemId parent,
                                                const ShapePath& path) const {
    wxTreeItemIdValue cookie;
    for (wxTreeItemId child = m_tree->GetFirstChild(parent, cookie);
         child.IsOk();
         child = m_tree->GetNextChild(parent, cookie))
    {
        auto* data = dynamic_cast<ItemData*>(m_tree->GetItemData(child));
        if (data && data->path == path)
            return child;
        // Recurse.
        wxTreeItemId found = FindItemForPathIn(child, path);
        if (found.IsOk()) return found;
    }
    return {};
}

// ---------------------------------------------------------------------------
// Selection sync: tree → canvas
// ---------------------------------------------------------------------------

void HierarchyPanel::OnSelChanged(wxTreeEvent& event) {
    if (m_syncing) { event.Skip(); return; }
    if (!m_doc) { event.Skip(); return; }

    wxArrayTreeItemIds selectedItems;
    m_tree->GetSelections(selectedItems);

    std::vector<int> indices;
    for (auto& itemId : selectedItems) {
        auto* data = dynamic_cast<ItemData*>(m_tree->GetItemData(itemId));
        if (data && data->path.size() == 1)  // only root-level for now
            indices.push_back(data->path[0]);
    }

    auto* mf = wxDynamicCast(wxGetApp().GetTopWindow(), MainFrame);
    if (mf) mf->OnHierarchySelectionChanged(indices);

    event.Skip();
}

// ---------------------------------------------------------------------------
// Drag-and-drop reorder
// ---------------------------------------------------------------------------

void HierarchyPanel::OnBeginDrag(wxTreeEvent& event) {
    m_dragItem = event.GetItem();
    if (m_dragItem.IsOk())
        event.Allow();
}

void HierarchyPanel::OnEndDrag(wxTreeEvent& event) {
    wxTreeItemId dropTarget = event.GetItem();
    if (!m_dragItem.IsOk() || !dropTarget.IsOk() || dropTarget == m_dragItem) return;
    if (!m_doc) return;

    auto* fromData = dynamic_cast<ItemData*>(m_tree->GetItemData(m_dragItem));
    auto* toData   = dynamic_cast<ItemData*>(m_tree->GetItemData(dropTarget));
    if (!fromData || !toData) return;

    const ShapePath& fromPath = fromData->path;

    // Prevent dragging a shape into itself or one of its own descendants.
    if (fromPath == toData->path || ShapePathIsAncestor(fromPath, toData->path))
        return;
    const DrawShape* dropShape = m_doc->ShapeAt(toData->path);
    if (!dropShape) return;

    ShapePath toParent;
    int toIdx = 0;

    if (dropShape->kind == ShapeKind::Group) {
        // Drop INTO the group (as first child).
        toParent = toData->path;
        toIdx    = 0;
    } else {
        // Drop BEFORE the target (insert at target's position).
        toParent = ShapePathParent(toData->path);
        toIdx    = ShapePathLeaf(toData->path);
    }

    if (fromPath == toParent) return; // no-op

    m_doc->GetCommandProcessor()->Submit(
        new MoveShapeCmd(m_doc, fromPath, toParent, toIdx));
}

// ---------------------------------------------------------------------------
// Context menu
// ---------------------------------------------------------------------------

void HierarchyPanel::OnItemMenu(wxTreeEvent& event) {
    wxTreeItemId item = event.GetItem();
    if (!item.IsOk()) return;
    m_tree->SelectItem(item);

    wxMenu menu;
    menu.Append(ID_HIER_RENAME,         "Rename...");
    menu.AppendSeparator();
    menu.Append(ID_HIER_TOGGLE_VISIBLE, "Toggle Visibility");
    menu.Append(ID_HIER_TOGGLE_LOCKED,  "Toggle Lock");

    PopupMenu(&menu);
    event.Skip();
}

void HierarchyPanel::OnContextItem(wxCommandEvent& event) {
    if (!m_doc) return;

    wxArrayTreeItemIds selected;
    m_tree->GetSelections(selected);
    if (selected.IsEmpty()) return;

    wxTreeItemId item = selected[0];
    auto* data = dynamic_cast<ItemData*>(m_tree->GetItemData(item));
    if (!data) return;

    DrawShape* shape = m_doc->ShapeAt(data->path);
    if (!shape) return;
    // Copy the before-state before Submit() may invalidate the pointer.
    DrawShape before = *shape;

    if (event.GetId() == ID_HIER_RENAME) {
        wxTextEntryDialog dlg(this, "Name:", "Rename Shape", before.name);
        if (dlg.ShowModal() != wxID_OK) return;
        DrawShape after = before;
        after.name = dlg.GetValue();
        m_doc->GetCommandProcessor()->Submit(
            new UpdateShapeAtCmd(m_doc, data->path, before, after, "Rename Shape"));

    } else if (event.GetId() == ID_HIER_TOGGLE_VISIBLE) {
        DrawShape after = before;
        after.visible = !after.visible;
        m_doc->GetCommandProcessor()->Submit(
            new UpdateShapeAtCmd(m_doc, data->path, before, after, "Toggle Visibility"));

    } else if (event.GetId() == ID_HIER_TOGGLE_LOCKED) {
        DrawShape after = before;
        after.locked = !after.locked;
        m_doc->GetCommandProcessor()->Submit(
            new UpdateShapeAtCmd(m_doc, data->path, before, after, "Toggle Lock"));
    }
}
