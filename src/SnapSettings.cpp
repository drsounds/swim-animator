#include "SnapSettings.h"
#include <wx/stdpaths.h>
#include <wx/filename.h>
#include <wx/filefn.h>
#include <wx/xml/xml.h>

// ---------------------------------------------------------------------------
// Colour helpers (same pattern as DrawDoc.cpp)
// ---------------------------------------------------------------------------

static wxString ColourToHex(const wxColour& c) {
    return wxString::Format("#%02X%02X%02X",
                            (int)c.Red(), (int)c.Green(), (int)c.Blue());
}

static wxColour HexToColour(const wxString& hex) {
    long r = 0, g = 0, b = 0;
    if (hex.Length() == 7 && hex[0] == '#') {
        hex.Mid(1, 2).ToLong(&r, 16);
        hex.Mid(3, 2).ToLong(&g, 16);
        hex.Mid(5, 2).ToLong(&b, 16);
    }
    return wxColour((unsigned char)r, (unsigned char)g, (unsigned char)b);
}

// ---------------------------------------------------------------------------

wxString SnapSettings::DefaultPath() {
    return wxStandardPaths::Get().GetUserDataDir()
           + wxFILE_SEP_PATH + "snap_settings.xml";
}

bool SnapSettings::SaveToXml(const wxString& path) const {
    wxString dir = wxFileName(path).GetPath();
    wxFileName::Mkdir(dir, 0755, wxPATH_MKDIR_FULL);

    wxXmlDocument doc;
    auto* root = new wxXmlNode(nullptr, wxXML_ELEMENT_NODE, "spacely-snap-settings");
    root->AddAttribute("version",            "1");
    root->AddAttribute("enabled",            enabled            ? "true" : "false");
    root->AddAttribute("snapEdgeToEdge",     snapEdgeToEdge     ? "true" : "false");
    root->AddAttribute("snapCenterToCenter", snapCenterToCenter ? "true" : "false");
    root->AddAttribute("snapEdgeToCenter",   snapEdgeToCenter   ? "true" : "false");
    root->AddAttribute("snapEqualSpacing",   snapEqualSpacing   ? "true" : "false");
    root->AddAttribute("snapPercentage",     snapPercentage     ? "true" : "false");
    root->AddAttribute("snapBezierPoints",   snapBezierPoints   ? "true" : "false");
    root->AddAttribute("snapLockedShapes",   snapLockedShapes   ? "true" : "false");
    root->AddAttribute("pixelThreshold",     wxString::Format("%d", pixelThreshold));
    root->AddAttribute("guideColour",        ColourToHex(guideColour));
    root->AddAttribute("guideLineWidth",     wxString::Format("%d", guideLineWidth));
    doc.SetRoot(root);
    return doc.Save(path);
}

bool SnapSettings::LoadFromXml(const wxString& path) {
    if (!wxFileExists(path)) return false;
    wxXmlDocument doc;
    if (!doc.Load(path)) return false;
    wxXmlNode* root = doc.GetRoot();
    if (!root || root->GetName() != "spacely-snap-settings") return false;

    auto readBool = [&](const wxString& attr, bool def) {
        wxString v = root->GetAttribute(attr, def ? "true" : "false");
        return v == "true";
    };
    auto readInt = [&](const wxString& attr, int def) {
        long v = def;
        root->GetAttribute(attr, wxString::Format("%d", def)).ToLong(&v);
        return (int)v;
    };

    enabled            = readBool("enabled",            enabled);
    snapEdgeToEdge     = readBool("snapEdgeToEdge",     snapEdgeToEdge);
    snapCenterToCenter = readBool("snapCenterToCenter",  snapCenterToCenter);
    snapEdgeToCenter   = readBool("snapEdgeToCenter",   snapEdgeToCenter);
    snapEqualSpacing   = readBool("snapEqualSpacing",   snapEqualSpacing);
    snapPercentage     = readBool("snapPercentage",     snapPercentage);
    snapBezierPoints   = readBool("snapBezierPoints",   snapBezierPoints);
    snapLockedShapes   = readBool("snapLockedShapes",   snapLockedShapes);
    pixelThreshold     = readInt("pixelThreshold",      pixelThreshold);
    guideColour        = HexToColour(root->GetAttribute("guideColour", ColourToHex(guideColour)));
    guideLineWidth     = readInt("guideLineWidth",      guideLineWidth);
    return true;
}
