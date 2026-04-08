#include "NewDrawingDialog.h"
#include <wx/sizer.h>
#include <wx/stattext.h>
#include <wx/colordlg.h>
#include <wx/valnum.h>

// Preset table: {label, width, height}
struct Preset { const char* label; int w; int h; };
static const Preset kPresets[] = {
    { "1920 × 1080  (Full HD landscape)", 1920, 1080 },
    { "1080 × 1920  (Full HD portrait)",  1080, 1920 },
    { "1024 × 768",                       1024,  768 },
    { "640 × 480",                         640,  480 },
    { "Custom",                              -1,   -1 },
};
static const int kPresetCount = (int)(sizeof(kPresets) / sizeof(kPresets[0]));
static const int kCustomIndex = kPresetCount - 1;

NewDrawingDialog::NewDrawingDialog(wxWindow* parent)
    : wxDialog(parent, wxID_ANY, "New Drawing",
               wxDefaultPosition, wxDefaultSize, wxDEFAULT_DIALOG_STYLE)
{
    auto* outer = new wxBoxSizer(wxVERTICAL);
    auto* grid  = new wxFlexGridSizer(2, wxSize(8, 6));
    grid->AddGrowableCol(1);

    // Preset choice
    wxArrayString presetLabels;
    for (int i = 0; i < kPresetCount; i++)
        presetLabels.Add(kPresets[i].label);

    m_presetChoice = new wxChoice(this, wxID_ANY, wxDefaultPosition, wxDefaultSize, presetLabels);
    m_presetChoice->SetSelection(0);
    grid->Add(new wxStaticText(this, wxID_ANY, "Size:"), 0, wxALIGN_CENTER_VERTICAL);
    grid->Add(m_presetChoice, 1, wxEXPAND);

    // Width / height spinners
    m_widthSpin  = new wxSpinCtrl(this, wxID_ANY, "", wxDefaultPosition, wxDefaultSize,
                                  wxSP_ARROW_KEYS, 1, 32768, 1920);
    m_heightSpin = new wxSpinCtrl(this, wxID_ANY, "", wxDefaultPosition, wxDefaultSize,
                                  wxSP_ARROW_KEYS, 1, 32768, 1080);
    grid->Add(new wxStaticText(this, wxID_ANY, "Width:"),  0, wxALIGN_CENTER_VERTICAL);
    grid->Add(m_widthSpin,  1, wxEXPAND);
    grid->Add(new wxStaticText(this, wxID_ANY, "Height:"), 0, wxALIGN_CENTER_VERTICAL);
    grid->Add(m_heightSpin, 1, wxEXPAND);

    // Background color button
    m_bgBtn = new wxButton(this, wxID_ANY, "", wxDefaultPosition, wxSize(60, 22));
    m_bgBtn->SetBackgroundColour(m_bgColour);
    grid->Add(new wxStaticText(this, wxID_ANY, "Background:"), 0, wxALIGN_CENTER_VERTICAL);
    grid->Add(m_bgBtn, 0);

    outer->Add(grid, 0, wxEXPAND | wxALL, 10);
    outer->Add(CreateButtonSizer(wxOK | wxCANCEL), 0, wxEXPAND | wxLEFT | wxRIGHT | wxBOTTOM, 10);
    SetSizerAndFit(outer);
    Centre();

    // Start with first preset (locked spinners)
    ApplyPreset(0);

    m_presetChoice->Bind(wxEVT_CHOICE, &NewDrawingDialog::OnPresetChanged, this);
    m_bgBtn->Bind(wxEVT_BUTTON, [this](wxCommandEvent&) {
        wxColourData cd;
        cd.SetChooseFull(true);
        cd.SetColour(m_bgColour);
        wxColourDialog dlg(this, &cd);
        if (dlg.ShowModal() == wxID_OK) {
            m_bgColour = dlg.GetColourData().GetColour();
            m_bgBtn->SetBackgroundColour(m_bgColour);
            m_bgBtn->Refresh();
        }
    });

    Bind(wxEVT_BUTTON, &NewDrawingDialog::OnOK, this, wxID_OK);
}

void NewDrawingDialog::OnPresetChanged(wxCommandEvent&) {
    ApplyPreset(m_presetChoice->GetSelection());
}

void NewDrawingDialog::ApplyPreset(int index) {
    if (index < 0 || index >= kPresetCount) return;
    const bool isCustom = (index == kCustomIndex);
    if (!isCustom) {
        m_widthSpin->SetValue(kPresets[index].w);
        m_heightSpin->SetValue(kPresets[index].h);
    }
    m_widthSpin->Enable(isCustom);
    m_heightSpin->Enable(isCustom);
}

void NewDrawingDialog::OnOK(wxCommandEvent& event) {
    m_width  = m_widthSpin->GetValue();
    m_height = m_heightSpin->GetValue();
    event.Skip();
}
