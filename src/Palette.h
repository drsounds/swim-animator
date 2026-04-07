#pragma once
#include <wx/colour.h>
#include <wx/string.h>
#include <vector>

struct PaletteEntry {
    wxColour colour;
    wxString name;
};

struct Palette {
    wxString                  name;
    wxString                  comment;  // preserves GPL header comment block
    std::vector<PaletteEntry> entries;
};

// Returns a built-in 16-colour default palette.
Palette DefaultPalette();

// Load/save GIMP Palette (.gpl) format.
bool LoadGplPalette(const wxString& path, Palette& out);
bool SaveGplPalette(const wxString& path, const Palette& palette);
