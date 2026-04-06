#pragma once
#include <wx/docview.h>
#include <vector>
#include "DrawShape.h"

class DrawDoc : public wxDocument {
    wxDECLARE_DYNAMIC_CLASS(DrawDoc);
public:
    DrawDoc() = default;

    const std::vector<DrawShape>& GetShapes() const { return m_shapes; }

    int  AddShape(const DrawShape& s);          // returns new index
    void UpdateShape(int idx, const DrawShape& s);
    void RemoveShape(int idx);

    bool IsModified() const override;
    void Modify(bool modified) override;
    bool DoSaveDocument(const wxString& filename) override;
    bool DoOpenDocument(const wxString& filename) override;

private:
    std::vector<DrawShape> m_shapes;
    bool                   m_modified{false};
};
