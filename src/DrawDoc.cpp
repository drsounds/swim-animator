#include "DrawDoc.h"
#include "NewDrawingDialog.h"
#include <wx/app.h>
#include <wx/cmdproc.h>
#include <wx/wxcrtvararg.h>
#include <wx/xml/xml.h>
#include <algorithm>
#include <cmath>

wxIMPLEMENT_DYNAMIC_CLASS(DrawDoc, wxDocument);

// ---------------------------------------------------------------------------
// Construction
// ---------------------------------------------------------------------------

DrawDoc::DrawDoc() {
    SetCommandProcessor(new wxCommandProcessor);
}

bool DrawDoc::OnNewDocument() {
    if (!wxDocument::OnNewDocument()) return false;
    NewDrawingDialog dlg(wxTheApp->GetTopWindow());
    if (dlg.ShowModal() != wxID_OK) return false;
    m_pageWidth  = dlg.GetPageWidth();
    m_pageHeight = dlg.GetPageHeight();
    m_bgColour   = dlg.GetBgColour();
    return true;
}

// ---------------------------------------------------------------------------
// Flat root-level shape mutations (legacy API)
// ---------------------------------------------------------------------------

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

// ---------------------------------------------------------------------------
// Path-aware tree API
// ---------------------------------------------------------------------------

// Returns the vector that the LAST element of `path` indexes into.
//   path = {2, 1} → m_shapes[2].children  (the vector containing child 1)
//   path = {2}    → m_shapes              (the vector containing root shape 2)
std::vector<DrawShape>* DrawDoc::VectorFor(const ShapePath& path) {
    if (path.empty()) return nullptr;
    std::vector<DrawShape>* vec = &m_shapes;
    for (size_t i = 0; i + 1 < path.size(); ++i) {
        int idx = path[i];
        if (idx < 0 || idx >= static_cast<int>(vec->size())) return nullptr;
        vec = &(*vec)[idx].children;
    }
    return vec;
}

const std::vector<DrawShape>* DrawDoc::VectorFor(const ShapePath& path) const {
    return const_cast<DrawDoc*>(this)->VectorFor(path);
}

DrawShape* DrawDoc::ShapeAt(const ShapePath& path) {
    std::vector<DrawShape>* vec = VectorFor(path);
    if (!vec) return nullptr;
    int leaf = ShapePathLeaf(path);
    if (leaf < 0 || leaf >= static_cast<int>(vec->size())) return nullptr;
    return &(*vec)[leaf];
}

const DrawShape* DrawDoc::ShapeAt(const ShapePath& path) const {
    return const_cast<DrawDoc*>(this)->ShapeAt(path);
}

std::vector<DrawShape>* DrawDoc::ContainerOf(const ShapePath& path) {
    return VectorFor(path);
}

void DrawDoc::InsertShapeAt(const ShapePath& parentPath, int idx, const DrawShape& s) {
    // Navigate to the parent's children vector.
    std::vector<DrawShape>* vec = &m_shapes;
    for (int i : parentPath) {
        if (i < 0 || i >= static_cast<int>(vec->size())) return;
        vec = &(*vec)[i].children;
    }
    if (idx < 0 || idx > static_cast<int>(vec->size()))
        idx = static_cast<int>(vec->size());
    vec->insert(vec->begin() + idx, s);
    Modify(true);
    UpdateAllViews();
}

void DrawDoc::RemoveShapeAt(const ShapePath& path) {
    std::vector<DrawShape>* vec = VectorFor(path);
    if (!vec) return;
    int leaf = ShapePathLeaf(path);
    if (leaf < 0 || leaf >= static_cast<int>(vec->size())) return;
    vec->erase(vec->begin() + leaf);
    Modify(true);
    UpdateAllViews();
}

void DrawDoc::UpdateShapeAt(const ShapePath& path, const DrawShape& s) {
    DrawShape* shape = ShapeAt(path);
    if (!shape) return;
    *shape = s;
    Modify(true);
    UpdateAllViews();
}

void DrawDoc::MoveShape(const ShapePath& from, const ShapePath& toParent, int toIdx) {
    DrawShape* src = ShapeAt(from);
    if (!src) return;
    DrawShape moved = *src;  // copy before removal

    // Remove from original position.
    RemoveShapeAt(from);

    // Adjust toIdx if moving within the same parent and to a later position
    // (removal shifted indices by -1).
    ShapePath adjustedParent = toParent;
    int adjustedIdx = toIdx;
    if (ShapePathParent(from) == toParent && ShapePathLeaf(from) < toIdx)
        --adjustedIdx;

    InsertShapeAt(adjustedParent, adjustedIdx, moved);
}

// ---------------------------------------------------------------------------
// Page properties
// ---------------------------------------------------------------------------

void DrawDoc::SetPageSize(int w, int h) {
    m_pageWidth  = w;
    m_pageHeight = h;
    Modify(true);
    UpdateAllViews();
}

void DrawDoc::SetBgColour(const wxColour& c) {
    m_bgColour = c;
    Modify(true);
    UpdateAllViews();
}

// ---------------------------------------------------------------------------
// Modified / undo
// ---------------------------------------------------------------------------

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
//   Group  → <g>       (children serialized recursively)
//   SVGRef → <use swim:kind="svgref" swim:href="…">
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

// Adds swim:name / swim:visible / swim:locked attributes to any element.
static void AddCommonAttrs(wxXmlNode* node, const DrawShape& s) {
    if (!s.name.IsEmpty())
        node->AddAttribute("swim:name", s.name);
    if (!s.visible)
        node->AddAttribute("swim:visible", "false");
    if (s.locked)
        node->AddAttribute("swim:locked", "true");
}

// Reads swim:name / swim:visible / swim:locked from element.
static void ReadCommonAttrs(wxXmlNode* node, DrawShape& s) {
    s.name    = node->GetAttribute("swim:name");
    s.visible = node->GetAttribute("swim:visible", "true") != "false";
    s.locked  = node->GetAttribute("swim:locked",  "false") == "true";
}

// Forward declaration for recursion.
static wxXmlNode* ShapeToXmlNode(const DrawShape& s);
static bool XmlNodeToShape(wxXmlNode* node, DrawShape& s);

static wxXmlNode* ShapeToXmlNode(const DrawShape& s) {
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
            node->AddAttribute("stroke-width", wxString::Format("%d", s.strokeWidth));
            if (s.borderRadiusX > 0)
                node->AddAttribute("rx", wxString::Format("%d", s.borderRadiusX));
            if (s.borderRadiusY > 0)
                node->AddAttribute("ry", wxString::Format("%d", s.borderRadiusY));
            if (!s.label.IsEmpty())
                node->AddAttribute("swim:label", s.label);
            break;
        }
        case ShapeKind::Circle: {
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
            node->AddAttribute("stroke-width", wxString::Format("%d", s.strokeWidth));
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
            node->AddAttribute("stroke-width", wxString::Format("%d", s.strokeWidth));
            break;
        }
        case ShapeKind::Group: {
            node = new wxXmlNode(nullptr, wxXML_ELEMENT_NODE, "g");
            for (const DrawShape& child : s.children) {
                wxXmlNode* childNode = ShapeToXmlNode(child);
                if (childNode) node->AddChild(childNode);
            }
            break;
        }
        case ShapeKind::SVGRef: {
            node = new wxXmlNode(nullptr, wxXML_ELEMENT_NODE, "use");
            node->AddAttribute("swim:kind", "svgref");
            node->AddAttribute("swim:href", s.svgRefPath);
            node->AddAttribute("x",      wxString::Format("%d", s.bounds.x));
            node->AddAttribute("y",      wxString::Format("%d", s.bounds.y));
            node->AddAttribute("width",  wxString::Format("%d", s.bounds.width));
            node->AddAttribute("height", wxString::Format("%d", s.bounds.height));
            for (const auto& kv : s.overrides) {
                wxXmlNode* ov = new wxXmlNode(nullptr, wxXML_ELEMENT_NODE, "swim:override");
                ov->AddAttribute("name",  kv.first);
                ov->AddAttribute("value", kv.second);
                node->AddChild(ov);
            }
            break;
        }
    }

    if (node) AddCommonAttrs(node, s);
    return node;
}

static bool XmlNodeToShape(wxXmlNode* child, DrawShape& s) {
    const wxString name = child->GetName();

    if (name == "rect") {
        s.kind = ShapeKind::Rect;
        long x = 0, y = 0, w = 0, h = 0, sw = 1, brx = 0, bry = 0;
        child->GetAttribute("x").ToLong(&x);
        child->GetAttribute("y").ToLong(&y);
        child->GetAttribute("width").ToLong(&w);
        child->GetAttribute("height").ToLong(&h);
        child->GetAttribute("stroke-width", "1").ToLong(&sw);
        child->GetAttribute("rx", "0").ToLong(&brx);
        child->GetAttribute("ry", "0").ToLong(&bry);
        s.bounds        = wxRect(x, y, w, h);
        s.bgColour      = HexToColour(child->GetAttribute("fill"));
        s.fgColour      = HexToColour(child->GetAttribute("stroke"));
        s.strokeWidth   = (int)sw;
        s.borderRadiusX = (int)brx;
        s.borderRadiusY = (int)bry;
        s.label         = child->GetAttribute("swim:label");

    } else if (name == "ellipse") {
        s.kind = ShapeKind::Circle;
        double cx = 0, cy = 0, rx = 0, ry = 0;
        long sw = 1;
        child->GetAttribute("cx").ToDouble(&cx);
        child->GetAttribute("cy").ToDouble(&cy);
        child->GetAttribute("rx").ToDouble(&rx);
        child->GetAttribute("ry").ToDouble(&ry);
        child->GetAttribute("stroke-width", "1").ToLong(&sw);
        s.bounds      = wxRect((int)std::lround(cx - rx), (int)std::lround(cy - ry),
                               (int)std::lround(2 * rx),  (int)std::lround(2 * ry));
        s.bgColour    = HexToColour(child->GetAttribute("fill"));
        s.fgColour    = HexToColour(child->GetAttribute("stroke"));
        s.strokeWidth = (int)sw;
        s.label       = child->GetAttribute("swim:label");

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
            return false;
        long sw = 1;
        child->GetAttribute("stroke-width", "1").ToLong(&sw);
        s.pts[0] = {x0,y0}; s.pts[1] = {x1,y1};
        s.pts[2] = {x2,y2}; s.pts[3] = {x3,y3};
        int minX = std::min({x0,x1,x2,x3}), maxX = std::max({x0,x1,x2,x3});
        int minY = std::min({y0,y1,y2,y3}), maxY = std::max({y0,y1,y2,y3});
        s.bounds      = wxRect(minX, minY, maxX-minX, maxY-minY);
        s.bgColour    = HexToColour(child->GetAttribute("fill"));
        s.fgColour    = HexToColour(child->GetAttribute("stroke"));
        s.strokeWidth = (int)sw;

    } else if (name == "g") {
        s.kind = ShapeKind::Group;
        for (wxXmlNode* gchild = child->GetChildren(); gchild; gchild = gchild->GetNext()) {
            if (gchild->GetType() != wxXML_ELEMENT_NODE) continue;
            DrawShape cs;
            if (XmlNodeToShape(gchild, cs))
                s.children.push_back(cs);
        }
        s.bounds = ComputeGroupBounds(s.children);

    } else if (name == "use" && child->GetAttribute("swim:kind") == "svgref") {
        s.kind = ShapeKind::SVGRef;
        s.svgRefPath = child->GetAttribute("swim:href");
        long x = 0, y = 0, w = 0, h = 0;
        child->GetAttribute("x").ToLong(&x);
        child->GetAttribute("y").ToLong(&y);
        child->GetAttribute("width").ToLong(&w);
        child->GetAttribute("height").ToLong(&h);
        s.bounds = wxRect(x, y, w, h);
        for (wxXmlNode* ov = child->GetChildren(); ov; ov = ov->GetNext()) {
            if (ov->GetType() != wxXML_ELEMENT_NODE) continue;
            if (ov->GetName() == "swim:override")
                s.overrides[ov->GetAttribute("name")] = ov->GetAttribute("value");
        }

    } else {
        return false;  // unknown element — skip
    }

    ReadCommonAttrs(child, s);
    return true;
}

bool DrawDoc::DoSaveDocument(const wxString& filename) {
    wxXmlDocument doc;

    wxXmlNode* root = new wxXmlNode(nullptr, wxXML_ELEMENT_NODE, "svg");
    root->AddAttribute("xmlns",      "http://www.w3.org/2000/svg");
    root->AddAttribute("xmlns:swim", "https://swim.spacify.se/TR/");
    root->AddAttribute("version",    "1.1");
    root->AddAttribute("width",      wxString::Format("%d", m_pageWidth));
    root->AddAttribute("height",     wxString::Format("%d", m_pageHeight));
    root->AddAttribute("swim:bgColor", ColourToHex(m_bgColour));
    doc.SetRoot(root);

    for (const DrawShape& s : m_shapes) {
        wxXmlNode* node = ShapeToXmlNode(s);
        if (node) root->AddChild(node);
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

    // Read page dimensions and background (with backward-compat defaults).
    {
        long pw = 1920, ph = 1080;
        root->GetAttribute("width",  "1920").ToLong(&pw);
        root->GetAttribute("height", "1080").ToLong(&ph);
        m_pageWidth  = (int)pw;
        m_pageHeight = (int)ph;
        m_bgColour   = HexToColour(root->GetAttribute("swim:bgColor", "#FFFFFF"));
    }

    m_shapes.clear();
    GetCommandProcessor()->ClearCommands();

    for (wxXmlNode* child = root->GetChildren(); child; child = child->GetNext()) {
        if (child->GetType() != wxXML_ELEMENT_NODE) continue;
        DrawShape s;
        if (XmlNodeToShape(child, s))
            m_shapes.push_back(s);
    }

    Modify(false);
    UpdateAllViews();
    return true;
}
