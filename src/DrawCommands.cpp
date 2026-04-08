#include "DrawCommands.h"
#include "DrawDoc.h"
#include <algorithm>
#include <numeric>

// ---------------------------------------------------------------------------
// AddShapeCmd (legacy)
// ---------------------------------------------------------------------------

AddShapeCmd::AddShapeCmd(DrawDoc* doc, const DrawShape& s)
    : wxCommand(true, "Add Shape"), m_doc(doc), m_shape(s) {}

bool AddShapeCmd::Do() {
    m_idx = m_doc->AddShape(m_shape);
    return true;
}

bool AddShapeCmd::Undo() {
    m_doc->RemoveShape(m_idx);
    return true;
}

// ---------------------------------------------------------------------------
// RemoveShapeCmd (legacy)
// ---------------------------------------------------------------------------

RemoveShapeCmd::RemoveShapeCmd(DrawDoc* doc, int idx, const DrawShape& s)
    : wxCommand(true, "Delete Shape"), m_doc(doc), m_idx(idx), m_shape(s) {}

bool RemoveShapeCmd::Do() {
    m_doc->RemoveShape(m_idx);
    return true;
}

bool RemoveShapeCmd::Undo() {
    m_doc->InsertShape(m_idx, m_shape);
    return true;
}

// ---------------------------------------------------------------------------
// UpdateShapeCmd (legacy)
// ---------------------------------------------------------------------------

UpdateShapeCmd::UpdateShapeCmd(DrawDoc* doc, int idx,
                               const DrawShape& before, const DrawShape& after,
                               const wxString& name)
    : wxCommand(true, name), m_doc(doc), m_idx(idx)
    , m_before(before), m_after(after) {}

bool UpdateShapeCmd::Do() {
    m_doc->UpdateShape(m_idx, m_after);
    return true;
}

bool UpdateShapeCmd::Undo() {
    m_doc->UpdateShape(m_idx, m_before);
    return true;
}

// ---------------------------------------------------------------------------
// InsertShapeAtCmd
// ---------------------------------------------------------------------------

InsertShapeAtCmd::InsertShapeAtCmd(DrawDoc* doc, const ShapePath& parentPath,
                                   int idx, const DrawShape& s)
    : wxCommand(true, "Add Shape"), m_doc(doc)
    , m_parentPath(parentPath), m_idx(idx), m_shape(s) {}

bool InsertShapeAtCmd::Do() {
    // Compute the actual clamped index before inserting (mirrors InsertShapeAt logic).
    {
        const DrawShape* parent = m_parentPath.empty() ? nullptr
                                                        : m_doc->ShapeAt(m_parentPath);
        int parentSize = m_parentPath.empty()
            ? (int)m_doc->GetShapes().size()
            : (parent ? (int)parent->children.size() : 0);
        m_actualIdx = (m_idx < 0 || m_idx > parentSize) ? parentSize : m_idx;
    }
    m_doc->InsertShapeAt(m_parentPath, m_idx, m_shape);
    return true;
}

bool InsertShapeAtCmd::Undo() {
    m_doc->RemoveShapeAt(GetInsertedPath());
    return true;
}

ShapePath InsertShapeAtCmd::GetInsertedPath() const {
    ShapePath p = m_parentPath;
    p.push_back(m_actualIdx >= 0 ? m_actualIdx : m_idx);
    return p;
}

// ---------------------------------------------------------------------------
// RemoveShapeAtCmd
// ---------------------------------------------------------------------------

RemoveShapeAtCmd::RemoveShapeAtCmd(DrawDoc* doc, const ShapePath& path,
                                   const DrawShape& saved)
    : wxCommand(true, "Delete Shape"), m_doc(doc), m_path(path), m_saved(saved) {}

bool RemoveShapeAtCmd::Do() {
    m_doc->RemoveShapeAt(m_path);
    return true;
}

bool RemoveShapeAtCmd::Undo() {
    ShapePath parentPath = ShapePathParent(m_path);
    int idx = ShapePathLeaf(m_path);
    m_doc->InsertShapeAt(parentPath, idx, m_saved);
    return true;
}

// ---------------------------------------------------------------------------
// UpdateShapeAtCmd
// ---------------------------------------------------------------------------

UpdateShapeAtCmd::UpdateShapeAtCmd(DrawDoc* doc, const ShapePath& path,
                                   const DrawShape& before, const DrawShape& after,
                                   const wxString& name)
    : wxCommand(true, name), m_doc(doc), m_path(path)
    , m_before(before), m_after(after) {}

bool UpdateShapeAtCmd::Do() {
    m_doc->UpdateShapeAt(m_path, m_after);
    return true;
}

bool UpdateShapeAtCmd::Undo() {
    m_doc->UpdateShapeAt(m_path, m_before);
    return true;
}

// ---------------------------------------------------------------------------
// MoveShapeCmd
// ---------------------------------------------------------------------------

MoveShapeCmd::MoveShapeCmd(DrawDoc* doc, const ShapePath& from,
                           const ShapePath& toParent, int toIdx)
    : wxCommand(true, "Move Shape"), m_doc(doc)
    , m_from(from), m_toParent(toParent), m_toIdx(toIdx)
    , m_origParent(ShapePathParent(from)), m_origIdx(ShapePathLeaf(from)) {}

bool MoveShapeCmd::Do() {
    m_doc->MoveShape(m_from, m_toParent, m_toIdx);
    // Track the actual index at which the shape landed.
    // DrawDoc::MoveShape applies a -1 adjustment when moving within the same parent
    // to a higher index (because removing the source shifts subsequent indices).
    m_landedIdx = m_toIdx;
    if (m_origParent == m_toParent && m_origIdx < m_toIdx)
        --m_landedIdx;
    return true;
}

bool MoveShapeCmd::Undo() {
    // Locate the shape at its landed position.
    ShapePath landedPath = m_toParent;
    landedPath.push_back(m_landedIdx);

    const DrawShape* shape = m_doc->ShapeAt(landedPath);
    if (!shape) return false;
    DrawShape saved = *shape;

    // Use RemoveShapeAt + InsertShapeAt directly so that DrawDoc::MoveShape's
    // internal same-parent index adjustment is not applied a second time.
    m_doc->RemoveShapeAt(landedPath);
    m_doc->InsertShapeAt(m_origParent, m_origIdx, saved);
    return true;
}

// ---------------------------------------------------------------------------
// GroupShapesCmd
// ---------------------------------------------------------------------------

GroupShapesCmd::GroupShapesCmd(DrawDoc* doc, std::vector<int> indices)
    : wxCommand(true, "Group"), m_doc(doc)
{
    std::sort(indices.begin(), indices.end());
    m_indices = std::move(indices);
}

bool GroupShapesCmd::Do() {
    if (m_indices.empty()) return false;

    const auto& shapes = m_doc->GetShapes();

    // Capture the original shapes only on the very first Do() call.
    // On subsequent Redo() calls the document already contains the restored
    // originals (placed there by Undo()), so m_saved is still valid.
    if (!m_savedInitialized) {
        for (int idx : m_indices) {
            if (idx < 0 || idx >= static_cast<int>(shapes.size())) return false;
            m_saved.push_back(shapes[idx]);
        }
        m_savedInitialized = true;
    }

    DrawShape group;
    group.kind     = ShapeKind::Group;
    group.children = m_saved;
    group.bounds   = ComputeGroupBounds(group.children);
    group.name     = "Group";

    // Remove shapes in reverse order so indices stay valid.
    for (int i = static_cast<int>(m_indices.size()) - 1; i >= 0; --i)
        m_doc->RemoveShape(m_indices[i]);

    // Insert the group at the position of the first (lowest) removed index.
    m_insertIdx = m_indices[0];
    m_doc->InsertShape(m_insertIdx, group);
    return true;
}

bool GroupShapesCmd::Undo() {
    // Remove the group.
    m_doc->RemoveShape(m_insertIdx);

    // Re-insert the original shapes in their original order.
    for (int i = 0; i < static_cast<int>(m_indices.size()); ++i)
        m_doc->InsertShape(m_indices[i], m_saved[i]);

    return true;
}

// ---------------------------------------------------------------------------
// UngroupCmd
// ---------------------------------------------------------------------------

UngroupCmd::UngroupCmd(DrawDoc* doc, const ShapePath& groupPath, const DrawShape& group)
    : wxCommand(true, "Ungroup"), m_doc(doc), m_path(groupPath), m_group(group) {}

bool UngroupCmd::Do() {
    // Only root-level groups supported for now.
    if (m_path.size() != 1) return false;
    int groupIdx = m_path[0];

    const DrawShape* g = m_doc->ShapeAt(m_path);
    if (!g || g->kind != ShapeKind::Group) return false;

    m_childCount = static_cast<int>(g->children.size());
    m_firstInsertedIdx = groupIdx;

    std::vector<DrawShape> children = g->children;

    // Remove the group.
    m_doc->RemoveShape(groupIdx);

    // Insert children at the group's former position (in original order).
    for (int i = 0; i < static_cast<int>(children.size()); ++i)
        m_doc->InsertShape(groupIdx + i, children[i]);

    return true;
}

bool UngroupCmd::Undo() {
    // Remove the ungrouped children.
    for (int i = m_childCount - 1; i >= 0; --i)
        m_doc->RemoveShape(m_firstInsertedIdx + i);

    // Re-insert the original group.
    m_doc->InsertShape(m_firstInsertedIdx, m_group);
    return true;
}

// ---------------------------------------------------------------------------
// BatchCmd
// ---------------------------------------------------------------------------

BatchCmd::BatchCmd(const wxString& name)
    : wxCommand(true, name) {}

BatchCmd::~BatchCmd() {
    for (wxCommand* cmd : m_cmds)
        delete cmd;
}

void BatchCmd::Add(wxCommand* cmd) {
    m_cmds.push_back(cmd);
}

bool BatchCmd::Do() {
    for (wxCommand* cmd : m_cmds) {
        if (!cmd->Do()) return false;
    }
    return true;
}

bool BatchCmd::Undo() {
    // Undo in reverse order.
    for (int i = static_cast<int>(m_cmds.size()) - 1; i >= 0; --i) {
        if (!m_cmds[i]->Undo()) return false;
    }
    return true;
}
