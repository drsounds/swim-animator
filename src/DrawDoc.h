#pragma once
#include <wx/docview.h>
#include <wx/colour.h>
#include <vector>
#include "DrawShape.h"

class DrawDoc : public wxDocument {
    wxDECLARE_DYNAMIC_CLASS(DrawDoc);
public:
    DrawDoc();

    const std::vector<DrawShape>& GetShapes() const { return m_shapes; }

    int  AddShape(const DrawShape& s);          // appends, returns new index
    void InsertShape(int idx, const DrawShape& s); // inserts at idx (for undo)
    void UpdateShape(int idx, const DrawShape& s);
    void RemoveShape(int idx);

    // Page dimensions and background colour
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
    std::vector<DrawShape> m_shapes;
    int                    m_pageWidth  {1920};
    int                    m_pageHeight {1080};
    wxColour               m_bgColour   {255, 255, 255};
    bool                   m_modified{false};
};
