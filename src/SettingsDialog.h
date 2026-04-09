#pragma once
#include <wx/dialog.h>
#include <wx/notebook.h>
#include <wx/checkbox.h>
#include <wx/spinctrl.h>
#include <wx/button.h>
#include <wx/stattext.h>
#include "SnapSettings.h"

// ---------------------------------------------------------------------------
// SettingsDialog – modal dialog for application preferences.
// Uses a wxNotebook so future preference categories can be added as new tabs.
// ---------------------------------------------------------------------------
class SettingsDialog : public wxDialog {
public:
    // Initialises all controls from `current`.
    SettingsDialog(wxWindow* parent, const SnapSettings& current);

    // Call only after ShowModal() returns wxID_OK.
    SnapSettings GetSettings() const;

private:
    void BuildSnapPage(wxNotebook* nb);
    void OnOK(wxCommandEvent&);
    void OnGuideColour(wxCommandEvent&);
    void UpdateColourButton();

    // Snapping tab controls
    wxCheckBox* m_chkEnabled;
    wxCheckBox* m_chkEdgeEdge;
    wxCheckBox* m_chkCenterCenter;
    wxCheckBox* m_chkEdgeCenter;
    wxCheckBox* m_chkEqualSpacing;
    wxCheckBox* m_chkPercentage;
    wxCheckBox* m_chkBezierPts;
    wxCheckBox* m_chkSnapLocked;
    wxSpinCtrl* m_spinThreshold;
    wxButton*   m_btnGuideColour;
    wxSpinCtrl* m_spinGuideWidth;

    wxColour m_guideColour;

    wxDECLARE_EVENT_TABLE();
};
