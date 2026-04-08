#pragma once
#include <wx/dialog.h>
#include <wx/choice.h>
#include <wx/spinctrl.h>
#include <wx/button.h>
#include <wx/colour.h>

// ---------------------------------------------------------------------------
// NewDrawingDialog – shown when the user creates a new drawing.
// Lets the user choose a canvas size (from presets or custom) and a
// background colour.
// ---------------------------------------------------------------------------
class NewDrawingDialog : public wxDialog {
public:
    NewDrawingDialog(wxWindow* parent);

    int      GetPageWidth()  const { return m_width;    }
    int      GetPageHeight() const { return m_height;   }
    wxColour GetBgColour()   const { return m_bgColour; }

private:
    void OnPresetChanged(wxCommandEvent& event);
    void OnOK(wxCommandEvent& event);
    void ApplyPreset(int index);

    wxChoice*   m_presetChoice   {nullptr};
    wxSpinCtrl* m_widthSpin      {nullptr};
    wxSpinCtrl* m_heightSpin     {nullptr};
    wxButton*   m_bgBtn          {nullptr};

    int      m_width    {1920};
    int      m_height   {1080};
    wxColour m_bgColour {255, 255, 255};
};
