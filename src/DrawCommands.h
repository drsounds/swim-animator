#pragma once
#include <wx/cmdproc.h>
#include <memory>
#include <vector>
#include "DrawShape.h"
#include "ShapePath.h"

class DrawDoc;

// ---------------------------------------------------------------------------
// Legacy flat-index commands (kept for backward compat; still used by
// DrawView for single-shape creation / deletion / update at root level).
// ---------------------------------------------------------------------------

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

// ---------------------------------------------------------------------------
// Path-aware commands (Approach B tree model)
// ---------------------------------------------------------------------------

// Insert a shape as child `idx` of `parentPath` (empty = root level).
class InsertShapeAtCmd : public wxCommand {
public:
    InsertShapeAtCmd(DrawDoc* doc, const ShapePath& parentPath, int idx, const DrawShape& s);
    bool Do() override;
    bool Undo() override;
    // Returns the path of the inserted shape (only valid after Do()).
    ShapePath GetInsertedPath() const;
private:
    DrawDoc*  m_doc;
    ShapePath m_parentPath;
    int       m_idx;
    DrawShape m_shape;
    int       m_actualIdx { -1 }; // actual (post-clamping) insertion index, set in Do()
};

// Remove the shape at `path` (saved internally for undo).
class RemoveShapeAtCmd : public wxCommand {
public:
    RemoveShapeAtCmd(DrawDoc* doc, const ShapePath& path, const DrawShape& saved);
    bool Do() override;
    bool Undo() override;
private:
    DrawDoc*  m_doc;
    ShapePath m_path;
    DrawShape m_saved;
};

// Replace the shape at `path` with `after`.
class UpdateShapeAtCmd : public wxCommand {
public:
    UpdateShapeAtCmd(DrawDoc* doc, const ShapePath& path,
                     const DrawShape& before, const DrawShape& after,
                     const wxString& name = "Edit Shape");
    bool Do() override;
    bool Undo() override;
private:
    DrawDoc*  m_doc;
    ShapePath m_path;
    DrawShape m_before;
    DrawShape m_after;
};

// Move a shape from `from` path to child `toIdx` of `toParent`.
class MoveShapeCmd : public wxCommand {
public:
    MoveShapeCmd(DrawDoc* doc, const ShapePath& from,
                 const ShapePath& toParent, int toIdx);
    bool Do() override;
    bool Undo() override;
private:
    DrawDoc*  m_doc;
    ShapePath m_from;
    ShapePath m_toParent;
    int       m_toIdx;
    // Saved for undo: original parent + index.
    ShapePath m_origParent;
    int       m_origIdx    { -1 };
    // Actual index at which the shape landed after Do() (accounts for
    // DrawDoc::MoveShape's internal index adjustment on same-parent moves).
    int       m_landedIdx  { -1 };
};

// Group a set of root-level indices into a new Group shape.
// The group is inserted at the position of the lowest index.
class GroupShapesCmd : public wxCommand {
public:
    // `indices` must all be root-level (depth 1 paths) and non-empty.
    GroupShapesCmd(DrawDoc* doc, std::vector<int> indices);
    bool Do() override;
    bool Undo() override;
    // Returns the path of the created group (only valid after Do()).
    ShapePath GetGroupPath() const { return { m_insertIdx }; }
private:
    DrawDoc*            m_doc;
    std::vector<int>    m_indices;          // sorted ascending
    int                 m_insertIdx { -1 };
    std::vector<DrawShape> m_saved;         // copies of the grouped shapes, populated once
    bool                m_savedInitialized { false };
};

// Ungroup: dissolves a Group shape, re-inserting its children at the group's position.
class UngroupCmd : public wxCommand {
public:
    UngroupCmd(DrawDoc* doc, const ShapePath& groupPath, const DrawShape& group);
    bool Do() override;
    bool Undo() override;
private:
    DrawDoc*  m_doc;
    ShapePath m_path;
    DrawShape m_group;   // full group shape saved for undo
    int       m_firstInsertedIdx { -1 };
    int       m_childCount       { 0  };
};

// ---------------------------------------------------------------------------
// BatchCmd – wraps multiple commands as a single undoable step.
// Owns the sub-commands; they are deleted when BatchCmd is destroyed.
// ---------------------------------------------------------------------------
class BatchCmd : public wxCommand {
public:
    explicit BatchCmd(const wxString& name = "Batch");
    ~BatchCmd() override;
    void Add(wxCommand* cmd);
    bool Do() override;
    bool Undo() override;
private:
    std::vector<wxCommand*> m_cmds;
};
