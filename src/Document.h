#pragma once
#include <wx/docview.h>
#include <wx/string.h>

// Document holds the data model.
// Subclass this and add your application-specific data members.
class Document : public wxDocument {
    wxDECLARE_DYNAMIC_CLASS(Document);

public:
    Document() = default;

    // --- wxDocument overrides ---

    // Called by the framework to check for unsaved changes.
    bool IsModified() const override;

    // Called by the framework to mark the document clean/dirty.
    void Modify(bool modified) override;

    // Serialise to stream (Save).
    bool DoSaveDocument(const wxString& filename) override;

    // Deserialise from stream (Open).
    bool DoOpenDocument(const wxString& filename) override;

    // --- Application data ---
    // Replace / extend with your real model.
    const wxString& GetContent() const { return m_content; }
    void SetContent(const wxString& s);

private:
    wxString m_content;
    bool     m_modified{false};
};
