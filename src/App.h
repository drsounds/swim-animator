#pragma once
#include <wx/app.h>
#include <wx/docview.h>
#include <wx/colour.h>
#include "Palette.h"
#include "SnapSettings.h"

class MainFrame;

class App : public wxApp {
public:
    // Called before GTK is initialised — used to inject the Trinity GTK2 rc.
    bool Initialize(int& argc, wxChar** argv) override;

    bool OnInit() override;
    int  OnExit() override;

    wxDocManager* GetDocManager() { return m_docManager; }

    // Active foreground / background colours (used when creating new shapes
    // and shown in the colour-swatch indicator).
    wxColour GetFgColour() const          { return m_fgColour; }
    wxColour GetBgColour() const          { return m_bgColour; }
    void     SetFgColour(const wxColour& c) { m_fgColour = c; }
    void     SetBgColour(const wxColour& c) { m_bgColour = c; }

    // The active colour palette (global, shared across all documents).
    Palette& GetPalette() { return m_palette; }

    // Persist the current palette to the user data directory.
    void SavePaletteToConfig();

    // Global snap settings — read/written by SettingsDialog.
    SnapSettings&       GetSnapSettings()       { return m_snapSettings; }
    const SnapSettings& GetSnapSettings() const { return m_snapSettings; }

private:
    wxDocManager* m_docManager{nullptr};
    wxColour      m_fgColour{  0,   0,   0};
    wxColour      m_bgColour{255, 255, 255};
    Palette       m_palette;
    SnapSettings  m_snapSettings;
};

wxDECLARE_APP(App);
