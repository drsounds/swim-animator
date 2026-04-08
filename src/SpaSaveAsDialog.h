#pragma once
#include <wx/dialog.h>
#include <wx/textctrl.h>
#include <wx/radiobox.h>
#include <wx/button.h>

// ---------------------------------------------------------------------------
// SpaSaveAsDialog – shown when the user saves a .spa project for the first
// time or uses "Save As".  Lets them choose between a single ZIP archive
// (.spa) and an unpacked folder bundle.
// ---------------------------------------------------------------------------
class SpaSaveAsDialog : public wxDialog {
public:
    // currentPath: pre-fill the path field (may be empty for new docs).
    // folderBundle: initial state of the ZIP / folder radio selection.
    SpaSaveAsDialog(wxWindow* parent,
                    const wxString& currentPath,
                    bool folderBundle);

    wxString GetPath()        const;
    bool     IsFolderBundle() const;

private:
    void OnBrowse(wxCommandEvent&);
    void OnModeChanged(wxCommandEvent&);

    wxTextCtrl* m_pathCtrl  {nullptr};
    wxRadioBox* m_modeBox   {nullptr};
};
