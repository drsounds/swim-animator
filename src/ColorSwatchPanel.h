#pragma once
#include <wx/panel.h>

class MainFrame;

// ---------------------------------------------------------------------------
// ColorSwatchPanel – dockable pane for colour palette management.
//
// Layout (top to bottom):
//   - FgBgIndicatorCtrl : overlapping FG/BG colour squares (like GIMP).
//     Left-click FG square or BG square opens wxColourDialog for that slot.
//   - SwatchGridCtrl    : scrollable grid of colour swatches loaded from the
//     active Palette.  Left-click sets FG, right-click sets BG of the
//     selected shape (if any) and the app-level active colours.
// ---------------------------------------------------------------------------
class ColorSwatchPanel : public wxPanel {
public:
    ColorSwatchPanel(wxWindow* parent, MainFrame* frame);

    // Refresh the FG/BG indicator (call when App FG/BG changes).
    void UpdateColors();

    // Rebuild the swatch grid (call after the palette is replaced).
    void RefreshPalette();

private:
    class FgBgIndicatorCtrl* m_indicator{nullptr};
    class SwatchGridCtrl*    m_grid{nullptr};
};
