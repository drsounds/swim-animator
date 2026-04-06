#include "Document.h"
#include <wx/wfstream.h>
#include <wx/txtstrm.h>

wxIMPLEMENT_DYNAMIC_CLASS(Document, wxDocument);

bool Document::IsModified() const {
    return wxDocument::IsModified() || m_modified;
}

void Document::Modify(bool modified) {
    wxDocument::Modify(modified);
    m_modified = modified;
}

void Document::SetContent(const wxString& s) {
    if (s == m_content)
        return;
    m_content = s;
    Modify(true);
    UpdateAllViews();
}

bool Document::DoSaveDocument(const wxString& filename) {
    wxFileOutputStream fos(filename);
    if (!fos.IsOk())
        return false;

    wxTextOutputStream tos(fos);
    tos << m_content;

    Modify(false);
    return true;
}

bool Document::DoOpenDocument(const wxString& filename) {
    wxFileInputStream fis(filename);
    if (!fis.IsOk())
        return false;

    wxTextInputStream tis(fis);
    m_content.clear();
    while (!fis.Eof())
        m_content += tis.ReadLine() + "\n";

    Modify(false);
    UpdateAllViews();
    return true;
}
