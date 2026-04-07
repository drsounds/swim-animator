#include "DrawCommands.h"
#include "DrawDoc.h"

// ---------------------------------------------------------------------------
// AddShapeCmd
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
// RemoveShapeCmd
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
// UpdateShapeCmd
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
