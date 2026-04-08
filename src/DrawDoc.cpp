#include "DrawDoc.h"
#include <wx/cmdproc.h>
#include <wx/wxcrtvararg.h>
#include <wx/xml/xml.h>
#include <algorithm>
#include <cmath>

wxIMPLEMENT_DYNAMIC_CLASS(DrawDoc, wxDocument);

// ---------------------------------------------------------------------------
// Shape mutations
// ---------------------------------------------------------------------------

// Each document gets its own independent undo/redo stack.
DrawDoc::DrawDoc() {
    SetCommandProcessor(new wxCommandProcessor);
}

int DrawDoc::AddShape(const DrawShape& s) {
    m_shapes.push_back(s);
    Modify(true);
    UpdateAllViews();
    return static_cast<int>(m_shapes.size()) - 1;
}

void DrawDoc::UpdateShape(int idx, const DrawShape& s) {
    if (idx < 0 || idx >= static_cast<int>(m_shapes.size())) return;
    m_shapes[idx] = s;
    Modify(true);
    UpdateAllViews();
}

void DrawDoc::InsertShape(int idx, const DrawShape& s) {
    if (idx < 0 || idx > static_cast<int>(m_shapes.size()))
        idx = static_cast<int>(m_shapes.size());
    m_shapes.insert(m_shapes.begin() + idx, s);
    Modify(true);
    UpdateAllViews();
}

void DrawDoc::RemoveShape(int idx) {
    if (idx < 0 || idx >= static_cast<int>(m_shapes.size())) return;
    m_shapes.erase(m_shapes.begin() + idx);
    Modify(true);
    UpdateAllViews();
}

bool DrawDoc::IsModified() const {
    return wxDocument::IsModified() || m_modified;
}

void DrawDoc::Modify(bool modified) {
    wxDocument::Modify(modified);
    m_modified = modified;
}

// ---------------------------------------------------------------------------
// Serialisation — SVG / XML
//
// Shapes map to standard SVG elements; Spacely-specific data that SVG does
// not natively express is stored as attributes in the swim: namespace:
//   xmlns:swim="https://swim.spacify.se/TR/"
//
// Mapping:
//   Rect   → <rect>    fill=bgColour stroke=fgColour  swim:label (if set)
//   Circle → <ellipse> fill=bgColour stroke=fgColour  swim:label (if set)
//   Text   → <text>    fill=fgColour text-content=label
//                      swim:bgColor swim:width swim:height
//   Bezier → <path d="M x0,y0 C x1,y1 x2,y2 x3,y3">
//                      fill=bgColour stroke=fgColour
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

bool DrawDoc::DoSaveDocument(const wxString& filename) {
    wxXmlDocument doc;

    wxXmlNode* root = new wxXmlNode(nullptr, wxXML_ELEMENT_NODE, "svg");
    root->AddAttribute("xmlns",      "http://www.w3.org/2000/svg");
    root->AddAttribute("xmlns:swim", "https://swim.spacify.se/TR/");
    root->AddAttribute("version",    "1.1");
    doc.SetRoot(root);

    for (const DrawShape& s : m_shapes) {
        wxXmlNode* node = nullptr;

        switch (s.kind) {
            case ShapeKind::Rect: {
                node = new wxXmlNode(nullptr, wxXML_ELEMENT_NODE, "rect");
                node->AddAttribute("x",            wxString::Format("%d", s.bounds.x));
                node->AddAttribute("y",            wxString::Format("%d", s.bounds.y));
                node->AddAttribute("width",        wxString::Format("%d", s.bounds.width));
                node->AddAttribute("height",       wxString::Format("%d", s.bounds.height));
                node->AddAttribute("fill",         ColourToHex(s.bgColour));
                node->AddAttribute("stroke",       ColourToHex(s.fgColour));
                node->AddAttribute("stroke-width", "2");
                if (!s.label.IsEmpty())
                    node->AddAttribute("swim:label", s.label);
                break;
            }
            case ShapeKind::Circle: {
                // Use float division so odd-pixel bounds round-trip exactly:
                // rx=w/2.0 on save, bounds.width=round(2*rx) on load → lossless.
                double cx = s.bounds.x + s.bounds.width  / 2.0;
                double cy = s.bounds.y + s.bounds.height / 2.0;
                double rx = s.bounds.width  / 2.0;
                double ry = s.bounds.height / 2.0;
                node = new wxXmlNode(nullptr, wxXML_ELEMENT_NODE, "ellipse");
                node->AddAttribute("cx",           wxString::Format("%g", cx));
                node->AddAttribute("cy",           wxString::Format("%g", cy));
                node->AddAttribute("rx",           wxString::Format("%g", rx));
                node->AddAttribute("ry",           wxString::Format("%g", ry));
                node->AddAttribute("fill",         ColourToHex(s.bgColour));
                node->AddAttribute("stroke",       ColourToHex(s.fgColour));
                node->AddAttribute("stroke-width", "2");
                if (!s.label.IsEmpty())
                    node->AddAttribute("swim:label", s.label);
                break;
            }
            case ShapeKind::Text: {
                node = new wxXmlNode(nullptr, wxXML_ELEMENT_NODE, "text");
                node->AddAttribute("x",            wxString::Format("%d", s.bounds.x));
                node->AddAttribute("y",            wxString::Format("%d", s.bounds.y));
                node->AddAttribute("fill",         ColourToHex(s.fgColour));
                node->AddAttribute("swim:bgColor", ColourToHex(s.bgColour));
                node->AddAttribute("swim:width",   wxString::Format("%d", s.bounds.width));
                node->AddAttribute("swim:height",  wxString::Format("%d", s.bounds.height));
                {
                    wxXmlNode* textContent = new wxXmlNode(nullptr, wxXML_TEXT_NODE,
                                                           wxEmptyString, s.label);
                    node->AddChild(textContent);
                }
                break;
            }
            case ShapeKind::Bezier: {
                node = new wxXmlNode(nullptr, wxXML_ELEMENT_NODE, "path");
                node->AddAttribute("d", wxString::Format(
                    "M %d,%d C %d,%d %d,%d %d,%d",
                    s.pts[0].x, s.pts[0].y,
                    s.pts[1].x, s.pts[1].y,
                    s.pts[2].x, s.pts[2].y,
                    s.pts[3].x, s.pts[3].y));
                node->AddAttribute("fill",         ColourToHex(s.bgColour));
                node->AddAttribute("stroke",       ColourToHex(s.fgColour));
                node->AddAttribute("stroke-width", "2");
                break;
            }
        }

        root->AddChild(node);
    }

    if (!doc.Save(filename)) return false;
    Modify(false);
    GetCommandProcessor()->MarkAsSaved();
    return true;
}

bool DrawDoc::DoOpenDocument(const wxString& filename) {
    wxXmlDocument doc;
    if (!doc.Load(filename)) return false;

    wxXmlNode* root = doc.GetRoot();
    if (!root || root->GetName() != "svg") return false;

    m_shapes.clear();
    GetCommandProcessor()->ClearCommands();

    for (wxXmlNode* child = root->GetChildren(); child; child = child->GetNext()) {
        if (child->GetType() != wxXML_ELEMENT_NODE) continue;

        const wxString name = child->GetName();
        DrawShape s;

        if (name == "rect") {
            s.kind = ShapeKind::Rect;
            long x = 0, y = 0, w = 0, h = 0;
            child->GetAttribute("x").ToLong(&x);
            child->GetAttribute("y").ToLong(&y);
            child->GetAttribute("width").ToLong(&w);
            child->GetAttribute("height").ToLong(&h);
            s.bounds   = wxRect(x, y, w, h);
            s.bgColour = HexToColour(child->GetAttribute("fill"));
            s.fgColour = HexToColour(child->GetAttribute("stroke"));
            s.label    = child->GetAttribute("swim:label");

        } else if (name == "ellipse") {
            s.kind = ShapeKind::Circle;
            double cx = 0, cy = 0, rx = 0, ry = 0;
            child->GetAttribute("cx").ToDouble(&cx);
            child->GetAttribute("cy").ToDouble(&cy);
            child->GetAttribute("rx").ToDouble(&rx);
            child->GetAttribute("ry").ToDouble(&ry);
            s.bounds   = wxRect((int)std::lround(cx - rx), (int)std::lround(cy - ry),
                                (int)std::lround(2 * rx),  (int)std::lround(2 * ry));
            s.bgColour = HexToColour(child->GetAttribute("fill"));
            s.fgColour = HexToColour(child->GetAttribute("stroke"));
            s.label    = child->GetAttribute("swim:label");

        } else if (name == "text") {
            s.kind = ShapeKind::Text;
            long x = 0, y = 0, w = 0, h = 0;
            child->GetAttribute("x").ToLong(&x);
            child->GetAttribute("y").ToLong(&y);
            child->GetAttribute("swim:width").ToLong(&w);
            child->GetAttribute("swim:height").ToLong(&h);
            s.bounds   = wxRect(x, y, w, h);
            s.fgColour = HexToColour(child->GetAttribute("fill"));
            s.bgColour = HexToColour(child->GetAttribute("swim:bgColor"));
            wxXmlNode* textNode = child->GetChildren();
            if (textNode && textNode->GetType() == wxXML_TEXT_NODE)
                s.label = textNode->GetContent();

        } else if (name == "path") {
            s.kind = ShapeKind::Bezier;
            wxString d = child->GetAttribute("d");
            int x0,y0,x1,y1,x2,y2,x3,y3;
            if (wxSscanf(d, "M %d,%d C %d,%d %d,%d %d,%d",
                         &x0,&y0,&x1,&y1,&x2,&y2,&x3,&y3) != 8)
                continue;
            s.pts[0] = {x0,y0}; s.pts[1] = {x1,y1};
            s.pts[2] = {x2,y2}; s.pts[3] = {x3,y3};
            int minX = std::min({x0,x1,x2,x3}), maxX = std::max({x0,x1,x2,x3});
            int minY = std::min({y0,y1,y2,y3}), maxY = std::max({y0,y1,y2,y3});
            s.bounds   = wxRect(minX, minY, maxX-minX, maxY-minY);
            s.bgColour = HexToColour(child->GetAttribute("fill"));
            s.fgColour = HexToColour(child->GetAttribute("stroke"));

        } else {
            continue; // unknown element — skip
        }

        m_shapes.push_back(s);
    }

    Modify(false);
    UpdateAllViews();
    return true;
}
