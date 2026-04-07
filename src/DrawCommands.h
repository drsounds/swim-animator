#pragma once
#include <wx/cmdproc.h>
#include "DrawShape.h"

class DrawDoc;

// Undo/redo command for adding a new shape.
class AddShapeCmd : public wxCommand {
public:
    AddShapeCmd(DrawDoc* doc, const DrawShape& s);
    bool Do() override;
    bool Undo() override;
private:
    DrawDoc*  m_doc;
    DrawShape m_shape;
    int       m_idx { -1 };
};

// Undo/redo command for deleting a shape.
class RemoveShapeCmd : public wxCommand {
public:
    RemoveShapeCmd(DrawDoc* doc, int idx, const DrawShape& s);
    bool Do() override;
    bool Undo() override;
private:
    DrawDoc*  m_doc;
    int       m_idx;
    DrawShape m_shape;
};

// Undo/redo command for any in-place shape mutation (move, resize, properties).
class UpdateShapeCmd : public wxCommand {
public:
    UpdateShapeCmd(DrawDoc* doc, int idx,
                   const DrawShape& before, const DrawShape& after,
                   const wxString& name = "Edit Shape");
    bool Do() override;
    bool Undo() override;
private:
    DrawDoc*  m_doc;
    int       m_idx;
    DrawShape m_before;
    DrawShape m_after;
};
