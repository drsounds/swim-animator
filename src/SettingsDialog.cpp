#include "SettingsDialog.h"
#include "DrawIds.h"
#include <wx/sizer.h>
#include <wx/statbox.h>
#include <wx/colordlg.h>
#include <wx/panel.h>

wxBEGIN_EVENT_TABLE(SettingsDialog, wxDialog)
    EVT_BUTTON(wxID_OK,            SettingsDialog::OnOK)
    EVT_BUTTON(ID_SETTINGS_GUIDE_COLOUR, SettingsDialog::OnGuideColour)
wxEND_EVENT_TABLE()

SettingsDialog::SettingsDialog(wxWindow* parent, const SnapSettings& current)
    : wxDialog(parent, wxID_ANY, "Settings",
               wxDefaultPosition, wxDefaultSize,
               wxDEFAULT_DIALOG_STYLE | wxRESIZE_BORDER)
    , m_guideColour(current.guideColour)
{
    auto* outer = new wxBoxSizer(wxVERTICAL);

    auto* nb = new wxNotebook(this, wxID_ANY);
    BuildSnapPage(nb);

    // Populate controls from current settings after the page is built.
    m_chkEnabled->SetValue(current.enabled);
    m_chkEdgeEdge->SetValue(current.snapEdgeToEdge);
    m_chkCenterCenter->SetValue(current.snapCenterToCenter);
    m_chkEdgeCenter->SetValue(current.snapEdgeToCenter);
    m_chkEqualSpacing->SetValue(current.snapEqualSpacing);
    m_chkPercentage->SetValue(current.snapPercentage);
    m_chkBezierPts->SetValue(current.snapBezierPoints);
    m_chkSnapLocked->SetValue(current.snapLockedShapes);
    m_spinThreshold->SetValue(current.pixelThreshold);
    m_spinGuideWidth->SetValue(current.guideLineWidth);
    UpdateColourButton();

    outer->Add(nb, 1, wxEXPAND | wxALL, 8);

    // Standard OK/Cancel buttons
    auto* btnSizer = CreateStdDialogButtonSizer(wxOK | wxCANCEL);
    // Wire OK to our handler (Cancel closes without saving via default wxDialog behaviour).
    outer->Add(btnSizer, 0, wxEXPAND | wxLEFT | wxRIGHT | wxBOTTOM, 8);

    SetSizerAndFit(outer);
    SetMinSize(GetSize());
}

void SettingsDialog::BuildSnapPage(wxNotebook* nb) {
    auto* page   = new wxPanel(nb);
    auto* vbox   = new wxBoxSizer(wxVERTICAL);

    // Master enable
    m_chkEnabled = new wxCheckBox(page, wxID_ANY, "Enable snapping");
    vbox->Add(m_chkEnabled, 0, wxALL, 6);
    vbox->AddSpacer(4);

    // Snap types group
    auto* typeBox  = new wxStaticBoxSizer(wxVERTICAL, page, "Snap types");
    m_chkEdgeEdge     = new wxCheckBox(page, wxID_ANY, "Edge to edge");
    m_chkCenterCenter = new wxCheckBox(page, wxID_ANY, "Center to center");
    m_chkEdgeCenter   = new wxCheckBox(page, wxID_ANY, "Edge to center (and center to edge)");
    m_chkEqualSpacing = new wxCheckBox(page, wxID_ANY, "Equal spacing between shapes");
    m_chkPercentage   = new wxCheckBox(page, wxID_ANY, "Percentage positions (25 / 50 / 75 %)");
    m_chkBezierPts    = new wxCheckBox(page, wxID_ANY, "Bezier control points");
    typeBox->Add(m_chkEdgeEdge,     0, wxALL, 4);
    typeBox->Add(m_chkCenterCenter, 0, wxALL, 4);
    typeBox->Add(m_chkEdgeCenter,   0, wxALL, 4);
    typeBox->Add(m_chkEqualSpacing, 0, wxALL, 4);
    typeBox->Add(m_chkPercentage,   0, wxALL, 4);
    typeBox->Add(m_chkBezierPts,    0, wxALL, 4);
    vbox->Add(typeBox, 0, wxEXPAND | wxLEFT | wxRIGHT | wxBOTTOM, 6);

    // Targets group
    auto* targBox = new wxStaticBoxSizer(wxVERTICAL, page, "Snap targets");
    m_chkSnapLocked = new wxCheckBox(page, wxID_ANY, "Include locked shapes as snap targets");
    targBox->Add(m_chkSnapLocked, 0, wxALL, 4);
    vbox->Add(targBox, 0, wxEXPAND | wxLEFT | wxRIGHT | wxBOTTOM, 6);

    // Threshold
    {
        auto* row = new wxBoxSizer(wxHORIZONTAL);
        row->Add(new wxStaticText(page, wxID_ANY, "Snap threshold (pixels):"),
                 0, wxALIGN_CENTER_VERTICAL | wxRIGHT, 8);
        m_spinThreshold = new wxSpinCtrl(page, wxID_ANY, "",
                                          wxDefaultPosition, wxSize(60, -1),
                                          wxSP_ARROW_KEYS, 1, 64, 8);
        row->Add(m_spinThreshold, 0, wxALIGN_CENTER_VERTICAL);
        vbox->Add(row, 0, wxLEFT | wxRIGHT | wxBOTTOM, 6);
    }

    // Guide line appearance
    auto* guideBox = new wxStaticBoxSizer(wxVERTICAL, page, "Guide lines");
    {
        auto* row = new wxBoxSizer(wxHORIZONTAL);
        row->Add(new wxStaticText(page, wxID_ANY, "Colour:"),
                 0, wxALIGN_CENTER_VERTICAL | wxRIGHT, 8);
        m_btnGuideColour = new wxButton(page, ID_SETTINGS_GUIDE_COLOUR, "      ");
        row->Add(m_btnGuideColour, 0, wxALIGN_CENTER_VERTICAL);
        guideBox->Add(row, 0, wxALL, 4);
    }
    {
        auto* row = new wxBoxSizer(wxHORIZONTAL);
        row->Add(new wxStaticText(page, wxID_ANY, "Line width:"),
                 0, wxALIGN_CENTER_VERTICAL | wxRIGHT, 8);
        m_spinGuideWidth = new wxSpinCtrl(page, wxID_ANY, "",
                                           wxDefaultPosition, wxSize(60, -1),
                                           wxSP_ARROW_KEYS, 1, 8, 1);
        row->Add(m_spinGuideWidth, 0, wxALIGN_CENTER_VERTICAL);
        guideBox->Add(row, 0, wxALL, 4);
    }
    vbox->Add(guideBox, 0, wxEXPAND | wxLEFT | wxRIGHT | wxBOTTOM, 6);

    page->SetSizer(vbox);
    nb->AddPage(page, "Snapping");
}

SnapSettings SettingsDialog::GetSettings() const {
    SnapSettings s;
    s.enabled            = m_chkEnabled->GetValue();
    s.snapEdgeToEdge     = m_chkEdgeEdge->GetValue();
    s.snapCenterToCenter = m_chkCenterCenter->GetValue();
    s.snapEdgeToCenter   = m_chkEdgeCenter->GetValue();
    s.snapEqualSpacing   = m_chkEqualSpacing->GetValue();
    s.snapPercentage     = m_chkPercentage->GetValue();
    s.snapBezierPoints   = m_chkBezierPts->GetValue();
    s.snapLockedShapes   = m_chkSnapLocked->GetValue();
    s.pixelThreshold     = m_spinThreshold->GetValue();
    s.guideColour        = m_guideColour;
    s.guideLineWidth     = m_spinGuideWidth->GetValue();
    return s;
}

void SettingsDialog::OnOK(wxCommandEvent&) {
    EndModal(wxID_OK);
}

void SettingsDialog::OnGuideColour(wxCommandEvent&) {
    wxColourData data;
    data.SetColour(m_guideColour);
    wxColourDialog dlg(this, &data);
    if (dlg.ShowModal() == wxID_OK) {
        m_guideColour = dlg.GetColourData().GetColour();
        UpdateColourButton();
    }
}

void SettingsDialog::UpdateColourButton() {
    m_btnGuideColour->SetBackgroundColour(m_guideColour);
    m_btnGuideColour->Refresh();
}
