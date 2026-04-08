#pragma once
#include <wx/docview.h>
#include <wx/colour.h>
#include <vector>
#include "DrawShape.h"
#include "ShapePath.h"

class DrawDoc : public wxDocument {
    wxDECLARE_DYNAMIC_CLASS(DrawDoc);
public:
    DrawDoc();

    // ---- Flat root-level API (legacy, kept for backward compat) ----
    const std::vector<DrawShape>& GetShapes() const { return m_shapes; }

    int  AddShape(const DrawShape& s);            // appends at root, returns new index
    void InsertShape(int idx, const DrawShape& s); // inserts at root idx (for undo)
    void UpdateShape(int idx, const DrawShape& s);
    void RemoveShape(int idx);

    // ---- Path-aware tree API ----

    // Returns a mutable pointer to the shape at `path`, or nullptr if invalid.
    DrawShape*       ShapeAt(const ShapePath& path);
    const DrawShape* ShapeAt(const ShapePath& path) const;

    // Returns the children vector that *contains* the shape at `path`.
    // For a root-level path {i}, this is m_shapes.
    // For {i, j}, this is m_shapes[i].children.
    // Returns nullptr if path is empty or parent doesn't exist.
    std::vector<DrawShape>* ContainerOf(const ShapePath& path);

    // Insert `s` as child `idx` of the shape identified by `parentPath`.
    // An empty parentPath inserts at the root level.
    void InsertShapeAt(const ShapePath& parentPath, int idx, const DrawShape& s);

    // Remove the shape at `path`.
    void RemoveShapeAt(const ShapePath& path);

    // Replace the shape at `path` with `s`.
    void UpdateShapeAt(const ShapePath& path, const DrawShape& s);

    // Move shape at `from` to be child `toIdx` of `toParent`.
    // toParent may equal ShapePathParent(from) for plain reordering.
    void MoveShape(const ShapePath& from, const ShapePath& toParent, int toIdx);

    // ---- Page dimensions and background colour ----
    int            GetPageWidth()  const { return m_pageWidth;  }
    int            GetPageHeight() const { return m_pageHeight; }
    const wxColour& GetBgColour() const  { return m_bgColour;   }
    void SetPageSize(int w, int h);
    void SetBgColour(const wxColour& c);

    bool OnNewDocument() override;
    bool IsModified() const override;
    void Modify(bool modified) override;
    bool DoSaveDocument(const wxString& filename) override;
    bool DoOpenDocument(const wxString& filename) override;

private:
    // Returns the children vector that `path` indexes into (i.e. the parent's children).
    // For path = {2, 1}: returns m_shapes[2].children.
    // For path = {2}:    returns m_shapes (root).
    std::vector<DrawShape>* VectorFor(const ShapePath& path);
    const std::vector<DrawShape>* VectorFor(const ShapePath& path) const;

    std::vector<DrawShape> m_shapes;
    int                    m_pageWidth  {1920};
    int                    m_pageHeight {1080};
    wxColour               m_bgColour   {255, 255, 255};
    bool                   m_modified{false};
};
