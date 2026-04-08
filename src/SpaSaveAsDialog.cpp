#include "SpaSaveAsDialog.h"
#include <wx/sizer.h>
#include <wx/stattext.h>
#include <wx/filedlg.h>
#include <wx/dirdlg.h>
#include <wx/filename.h>

SpaSaveAsDialog::SpaSaveAsDialog(wxWindow* parent,
                                 const wxString& currentPath,
                                 bool folderBundle)
    : wxDialog(parent, wxID_ANY, "Save Project As",
               wxDefaultPosition, wxDefaultSize, wxDEFAULT_DIALOG_STYLE)
{
    auto* outer = new wxBoxSizer(wxVERTICAL);

    // Mode radio: ZIP or folder
    wxArrayString modes;
    modes.Add("ZIP archive (.spa)");
    modes.Add("Folder bundle");
    m_modeBox = new wxRadioBox(this, wxID_ANY, "Bundle format",
                               wxDefaultPosition, wxDefaultSize,
                               modes, 1, wxRA_SPECIFY_COLS);
    m_modeBox->SetSelection(folderBundle ? 1 : 0);
    outer->Add(m_modeBox, 0, wxEXPAND | wxALL, 8);

    // Path row
    auto* pathSizer = new wxBoxSizer(wxHORIZONTAL);
    pathSizer->Add(new wxStaticText(this, wxID_ANY, "Location:"),
                   0, wxALIGN_CENTER_VERTICAL | wxRIGHT, 6);
    m_pathCtrl = new wxTextCtrl(this, wxID_ANY, currentPath,
                                wxDefaultPosition, wxSize(320, -1));
    pathSizer->Add(m_pathCtrl, 1, wxEXPAND);
    auto* browseBtn = new wxButton(this, wxID_ANY, "Browse...");
    pathSizer->Add(browseBtn, 0, wxLEFT, 6);
    outer->Add(pathSizer, 0, wxEXPAND | wxLEFT | wxRIGHT | wxBOTTOM, 8);

    outer->Add(CreateButtonSizer(wxOK | wxCANCEL), 0,
               wxEXPAND | wxLEFT | wxRIGHT | wxBOTTOM, 8);
    SetSizerAndFit(outer);
    Centre();

    browseBtn->Bind(wxEVT_BUTTON,    &SpaSaveAsDialog::OnBrowse,      this);
    m_modeBox->Bind(wxEVT_RADIOBOX,  &SpaSaveAsDialog::OnModeChanged,  this);
}

wxString SpaSaveAsDialog::GetPath() const {
    return m_pathCtrl->GetValue();
}

bool SpaSaveAsDialog::IsFolderBundle() const {
    return m_modeBox->GetSelection() == 1;
}

void SpaSaveAsDialog::OnBrowse(wxCommandEvent&) {
    if (IsFolderBundle()) {
        wxDirDialog dlg(this, "Choose project folder",
                        m_pathCtrl->GetValue(),
                        wxDD_DEFAULT_STYLE | wxDD_NEW_DIR_BUTTON);
        if (dlg.ShowModal() == wxID_OK)
            m_pathCtrl->SetValue(dlg.GetPath());
    } else {
        wxFileDialog dlg(this, "Save project as",
                         wxFileName(m_pathCtrl->GetValue()).GetPath(),
                         wxFileName(m_pathCtrl->GetValue()).GetFullName(),
                         "Swim Animator Project (*.spa)|*.spa",
                         wxFD_SAVE | wxFD_OVERWRITE_PROMPT);
        if (dlg.ShowModal() == wxID_OK)
            m_pathCtrl->SetValue(dlg.GetPath());
    }
}

void SpaSaveAsDialog::OnModeChanged(wxCommandEvent&) {
    // Adjust extension when switching ZIP ↔ folder.
    wxString path = m_pathCtrl->GetValue();
    if (!path.IsEmpty()) {
        if (IsFolderBundle() && path.EndsWith(".spa"))
            m_pathCtrl->SetValue(path.Left(path.Len() - 4));
        else if (!IsFolderBundle() && !path.EndsWith(".spa"))
            m_pathCtrl->SetValue(path + ".spa");
    }
}
