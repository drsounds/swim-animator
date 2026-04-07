#include "Palette.h"
#include <wx/wfstream.h>
#include <wx/txtstrm.h>
#include <wx/tokenzr.h>
#include <algorithm>
#include <sstream>

Palette DefaultPalette() {
    Palette p;
    p.name = "Default";
    static const struct { unsigned char r, g, b; const char* name; } kColors[] = {
        {  0,   0,   0, "Black"},
        {255, 255, 255, "White"},
        {128, 128, 128, "Gray"},
        {192, 192, 192, "Silver"},
        {255,   0,   0, "Red"},
        {  0, 128,   0, "Green"},
        {  0,   0, 255, "Blue"},
        {255, 255,   0, "Yellow"},
        {  0, 255, 255, "Cyan"},
        {255,   0, 255, "Magenta"},
        {255, 165,   0, "Orange"},
        {128,   0, 128, "Purple"},
        {165,  42,  42, "Brown"},
        {255, 192, 203, "Pink"},
        {  0, 128, 128, "Teal"},
        {128, 128,   0, "Olive"},
    };
    for (const auto& c : kColors)
        p.entries.push_back({wxColour(c.r, c.g, c.b), c.name});
    return p;
}

bool LoadGplPalette(const wxString& path, Palette& out) {
    wxFileInputStream fis(path);
    if (!fis.IsOk()) return false;
    wxTextInputStream in(fis);

    wxString first = in.ReadLine().Trim(false).Trim(true);
    if (first != "GIMP Palette") return false;

    Palette p;
    bool inHeader = true;
    wxString comments;

    // Use a read-then-check-eof loop so that the last line is not dropped when
    // the file has no trailing newline (wx sets Eof after the read, not before).
    for (;;) {
        wxString line = in.ReadLine();
        wxString trimmed = line.Trim(false).Trim(true);

        if (!trimmed.IsEmpty()) {
            if (inHeader && trimmed.StartsWith("Name:")) {
                p.name = trimmed.AfterFirst(':').Trim(false).Trim(true);
            } else if (inHeader && trimmed.StartsWith("Columns:")) {
                // ignored
            } else if (trimmed.StartsWith("#")) {
                wxString text = trimmed.Mid(1).Trim(false);
                if (!text.IsEmpty()) {
                    if (!comments.IsEmpty()) comments += "\n";
                    comments += text;
                }
            } else {
                inHeader = false;
                // Colour line: R G B<whitespace>Name
                std::istringstream iss(trimmed.ToStdString());
                int r, g, b;
                if (iss >> r >> g >> b) {
                    std::string name;
                    std::getline(iss, name);
                    size_t start = name.find_first_not_of(" \t");
                    if (start != std::string::npos)
                        name = name.substr(start);
                    auto clamp = [](int v) {
                        return (unsigned char)std::max(0, std::min(255, v));
                    };
                    p.entries.push_back({wxColour(clamp(r), clamp(g), clamp(b)),
                                         wxString::FromUTF8(name)});
                }
            }
        }

        if (fis.Eof()) break;
    }

    p.comment = comments;
    if (p.name.IsEmpty()) p.name = "Imported";
    out = std::move(p);
    return true;
}

bool SaveGplPalette(const wxString& path, const Palette& palette) {
    wxFileOutputStream fos(path);
    if (!fos.IsOk()) return false;
    wxTextOutputStream out(fos);

    out.WriteString("GIMP Palette\n");
    out.WriteString("Name: " + palette.name + "\n");
    if (!palette.comment.IsEmpty()) {
        wxStringTokenizer tok(palette.comment, "\n");
        while (tok.HasMoreTokens())
            out.WriteString("# " + tok.GetNextToken() + "\n");
    }
    out.WriteString("#\n");

    for (const PaletteEntry& e : palette.entries) {
        out.WriteString(wxString::Format("%3d %3d %3d\t%s\n",
            (int)e.colour.Red(), (int)e.colour.Green(), (int)e.colour.Blue(),
            e.name));
    }
    return true;
}
