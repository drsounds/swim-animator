#include "SmilDoc.h"
#include "DrawDoc.h"
#include <wx/app.h>
#include <wx/cmdproc.h>
#include <wx/spinctrl.h>
#include <wx/sizer.h>
#include <wx/stattext.h>
#include <wx/dialog.h>
#include <wx/button.h>
#include <algorithm>
#include <cmath>

wxIMPLEMENT_DYNAMIC_CLASS(SmilDoc, wxDocument);

// ---------------------------------------------------------------------------
// Internal helpers shared with DrawDoc serialisation
// ---------------------------------------------------------------------------

static wxString ColToHex(const wxColour& c) {
    return wxString::Format("#%02X%02X%02X",
                            (int)c.Red(), (int)c.Green(), (int)c.Blue());
}

static wxColour HexToCol(const wxString& hex) {
    long r=0,g=0,b=0;
    if (hex.Length()==7 && hex[0]=='#') {
        hex.Mid(1,2).ToLong(&r,16);
        hex.Mid(3,2).ToLong(&g,16);
        hex.Mid(5,2).ToLong(&b,16);
    }
    return wxColour((unsigned char)r,(unsigned char)g,(unsigned char)b);
}

static wxString InterpToCalcMode(SmilInterp i) {
    return (i == SmilInterp::Linear) ? "linear" : "spline";
}

static SmilInterp CalcModeToInterp(const wxString& cm, const wxString& swimInterp) {
    if (swimInterp == "cubic")  return SmilInterp::Cubic;
    if (swimInterp == "bezier") return SmilInterp::Bezier;
    return SmilInterp::Linear;
}

// ---------------------------------------------------------------------------
// Construction
// ---------------------------------------------------------------------------

SmilDoc::SmilDoc() {
    SetCommandProcessor(new wxCommandProcessor);
}

bool SmilDoc::OnNewDocument() {
    if (!wxDocument::OnNewDocument()) return false;

    // Ask for FPS via a simple dialog.
    wxDialog dlg(wxTheApp->GetTopWindow(), wxID_ANY, "New SMIL Animation",
                 wxDefaultPosition, wxDefaultSize, wxDEFAULT_DIALOG_STYLE);
    auto* sizer = new wxBoxSizer(wxVERTICAL);
    auto* grid  = new wxFlexGridSizer(2, 2, 4, 8);
    grid->AddGrowableCol(1);

    auto* fpsLabel = new wxStaticText(&dlg, wxID_ANY, "Frame rate (fps):");
    auto* fpsSpin  = new wxSpinCtrl(&dlg, wxID_ANY, "24", wxDefaultPosition,
                                    wxDefaultSize, wxSP_ARROW_KEYS, 1, 120, 24);
    grid->Add(fpsLabel, 0, wxALIGN_CENTER_VERTICAL);
    grid->Add(fpsSpin,  1, wxEXPAND);

    sizer->Add(grid, 0, wxEXPAND | wxALL, 12);
    sizer->Add(dlg.CreateButtonSizer(wxOK | wxCANCEL), 0, wxEXPAND | wxBOTTOM | wxLEFT | wxRIGHT, 12);
    dlg.SetSizerAndFit(sizer);
    dlg.Centre();

    if (dlg.ShowModal() != wxID_OK) return false;

    m_fps = fpsSpin->GetValue();
    m_scenes.clear();

    SmilScene first;
    first.id            = "scene_1";
    first.name          = "Scene 1";
    first.startFrame    = 0;
    first.durationFrames = m_fps * 10;  // 10 seconds
    m_scenes.push_back(first);
    m_currentScene = 0;
    m_currentFrame = 0;
    return true;
}

void SmilDoc::InitDefaults(int fps) {
    m_fps = fps;
    m_scenes.clear();
    SmilScene first;
    first.id             = "scene_1";
    first.name           = "Scene 1";
    first.startFrame     = 0;
    first.durationFrames = fps * 10;
    m_scenes.push_back(first);
    m_currentScene = 0;
    m_currentFrame = 0;
}

// ---------------------------------------------------------------------------
// Scene accessors
// ---------------------------------------------------------------------------

SmilScene* SmilDoc::GetCurrentScene() {
    if (m_currentScene < 0 || m_currentScene >= (int)m_scenes.size())
        return nullptr;
    return &m_scenes[m_currentScene];
}

const SmilScene* SmilDoc::GetCurrentScene() const {
    if (m_currentScene < 0 || m_currentScene >= (int)m_scenes.size())
        return nullptr;
    return &m_scenes[m_currentScene];
}

int SmilDoc::GetTotalFrames() const {
    if (m_scenes.empty()) return 1;
    const SmilScene& last = m_scenes.back();
    return last.startFrame + last.durationFrames;
}

void SmilDoc::SetCurrentScene(int idx) {
    if (idx < 0 || idx >= (int)m_scenes.size()) return;
    m_currentScene = idx;
    UpdateAllViews();
}

int SmilDoc::AddScene(const SmilScene& s) {
    m_scenes.push_back(s);
    Modify(true);
    UpdateAllViews();
    return (int)m_scenes.size() - 1;
}

void SmilDoc::RemoveScene(int idx) {
    if (idx < 0 || idx >= (int)m_scenes.size()) return;
    m_scenes.erase(m_scenes.begin() + idx);
    if (m_currentScene >= (int)m_scenes.size())
        m_currentScene = (int)m_scenes.size() - 1;
    if (m_currentScene < 0) m_currentScene = 0;
    Modify(true);
    UpdateAllViews();
}

void SmilDoc::UpdateScene(int idx, const SmilScene& s) {
    if (idx < 0 || idx >= (int)m_scenes.size()) return;
    m_scenes[idx] = s;
    Modify(true);
    UpdateAllViews();
}

void SmilDoc::MoveScene(int fromIdx, int toIdx) {
    if (fromIdx < 0 || fromIdx >= (int)m_scenes.size()) return;
    if (toIdx   < 0 || toIdx   >= (int)m_scenes.size()) return;
    SmilScene s = m_scenes[fromIdx];
    m_scenes.erase(m_scenes.begin() + fromIdx);
    if (toIdx > fromIdx) --toIdx;
    m_scenes.insert(m_scenes.begin() + toIdx, s);
    Modify(true);
    UpdateAllViews();
}

// ---------------------------------------------------------------------------
// Playhead
// ---------------------------------------------------------------------------

void SmilDoc::SetCurrentFrame(int f) {
    int total = GetTotalFrames();
    if (f < 0) f = 0;
    if (f >= total && total > 0) f = total - 1;
    m_currentFrame = f;
    UpdateAllViews();
}

// ---------------------------------------------------------------------------
// Keyframe editing
// ---------------------------------------------------------------------------

/*static*/ wxString SmilDoc::ShapeElementId(const DrawShape& s, int idx) {
    if (!s.name.IsEmpty()) return s.name;
    return wxString::Format("elem_%d", idx);
}

void SmilDoc::SetKeyframe(const wxString& elemId, const wxString& prop,
                           int frame, const wxString& value, SmilInterp interp) {
    SmilScene* sc = GetCurrentScene();
    if (!sc) return;

    SmilElement& elem = sc->elements[elemId];
    elem.elementId = elemId;
    elem.tracks[prop].SetKeyframe(frame, value, interp);
    Modify(true);
    UpdateAllViews();
}

void SmilDoc::RemoveKeyframe(const wxString& elemId, const wxString& prop, int frame) {
    SmilScene* sc = GetCurrentScene();
    if (!sc) return;
    auto eit = sc->elements.find(elemId);
    if (eit == sc->elements.end()) return;
    auto tit = eit->second.tracks.find(prop);
    if (tit == eit->second.tracks.end()) return;
    tit->second.RemoveKeyframe(frame);
    Modify(true);
    UpdateAllViews();
}

bool SmilDoc::HasKeyframe(const wxString& elemId, const wxString& prop, int frame) const {
    const SmilScene* sc = GetCurrentScene();
    if (!sc) return false;
    auto eit = sc->elements.find(elemId);
    if (eit == sc->elements.end()) return false;
    auto tit = eit->second.tracks.find(prop);
    if (tit == eit->second.tracks.end()) return false;
    return tit->second.HasKeyframeAt(frame);
}

wxString SmilDoc::GetAnimatedValue(const wxString& elemId, const wxString& prop) const {
    const SmilScene* sc = GetCurrentScene();
    if (!sc) return wxString();
    auto eit = sc->elements.find(elemId);
    if (eit == sc->elements.end()) return wxString();
    auto tit = eit->second.tracks.find(prop);
    if (tit == eit->second.tracks.end()) return wxString();
    // Frame relative to scene start.
    int relFrame = m_currentFrame - sc->startFrame;
    return tit->second.GetValueAt(relFrame);
}

// ---------------------------------------------------------------------------
// Modified / undo
// ---------------------------------------------------------------------------

bool SmilDoc::IsModified() const {
    return wxDocument::IsModified() || m_modified;
}

void SmilDoc::Modify(bool modified) {
    wxDocument::Modify(modified);
    m_modified = modified;
}

// ---------------------------------------------------------------------------
// Serialisation — SMIL XML
//
// Root element: <smil xmlns="http://www.w3.org/ns/SMIL"
//                     xmlns:swim="https://spacify.se/swim/1.0"
//                     swim:fps="24">
//   <body>
//     <swim:scene id="s1" swim:name="Intro" swim:start="0"
//                  swim:duration="240" swim:pageWidth="1920"
//                  swim:pageHeight="1080" swim:bgColor="#FFFFFF">
//       <svg xmlns="http://www.w3.org/2000/svg"
//            xmlns:swim="https://swim.spacify.se/TR/">
//         <rect id="elem_0" x="100" ... >
//           <animate attributeName="x" values="100;300" keyTimes="0;1"
//                    begin="0s" dur="10s" calcMode="linear" fill="freeze"
//                    swim:keyInterps="linear;linear"/>
//         </rect>
//       </svg>
//     </swim:scene>
//     <swim:scene id="s2" swim:refPath="other.smil"/>
//   </body>
// </smil>
// ---------------------------------------------------------------------------

wxXmlNode* SmilDoc::ShapeToSmilXml(const DrawShape& s, const wxString& elemId,
                                    const SmilScene& scene) const {
    // Build base SVG node from DrawDoc helper.
    wxXmlNode* node = DrawDoc::ShapeToXml(s);
    if (!node) return nullptr;

    // Add element id.
    node->DeleteAttribute("id");
    node->AddAttribute("id", elemId);

    // Add <animate> children for any tracked properties.
    WriteAnimateNodes(node, elemId, scene);
    return node;
}

void SmilDoc::WriteAnimateNodes(wxXmlNode* svgElem, const wxString& elemId,
                                 const SmilScene& scene) const {
    auto eit = scene.elements.find(elemId);
    if (eit == scene.elements.end()) return;

    double durSec = (m_fps > 0)
        ? (double)scene.durationFrames / (double)m_fps
        : (double)scene.durationFrames;

    for (const auto& [prop, track] : eit->second.tracks) {
        if (track.keyframes.empty()) continue;

        wxXmlNode* anim = new wxXmlNode(nullptr, wxXML_ELEMENT_NODE, "animate");
        anim->AddAttribute("attributeName", prop);
        anim->AddAttribute("begin", "0s");
        anim->AddAttribute("dur",   wxString::Format("%.4gs", durSec));
        anim->AddAttribute("fill",  "freeze");

        // Build values, keyTimes, keySplines, and swim:keyInterps lists.
        wxString values, keyTimes, keySplines, keyInterps;
        bool     needSplines = false;

        for (size_t i = 0; i < track.keyframes.size(); ++i) {
            const SmilKeyframe& kf = track.keyframes[i];
            double kt = (scene.durationFrames > 0)
                ? (double)kf.frame / (double)scene.durationFrames
                : 0.0;
            if (i > 0) { values += ";"; keyTimes += ";"; keyInterps += ";"; }
            values   += kf.value;
            keyTimes += wxString::Format("%.4g", kt);
            switch (kf.interp) {
                case SmilInterp::Linear: keyInterps += "linear"; break;
                case SmilInterp::Bezier: keyInterps += "bezier"; needSplines = true; break;
                case SmilInterp::Cubic:  keyInterps += "cubic";  needSplines = true; break;
            }
            if (i + 1 < track.keyframes.size()) {
                // keySplines has N-1 entries.
                if (i > 0) keySplines += ";";
                float cx1=kf.cx1, cy1=kf.cy1, cx2=kf.cx2, cy2=kf.cy2;
                if (kf.interp == SmilInterp::Cubic) { cx1=0.42f; cy1=0.0f; cx2=0.58f; cy2=1.0f; }
                if (kf.interp == SmilInterp::Linear){ cx1=0.0f;  cy1=0.0f; cx2=1.0f;  cy2=1.0f; }
                keySplines += wxString::Format("%g %g %g %g", cx1, cy1, cx2, cy2);
                needSplines = true;
            }
        }

        anim->AddAttribute("values",   values);
        anim->AddAttribute("keyTimes", keyTimes);
        anim->AddAttribute("calcMode", needSplines ? "spline" : "linear");
        if (needSplines && !keySplines.IsEmpty())
            anim->AddAttribute("keySplines", keySplines);
        anim->AddAttribute("swim:keyInterps", keyInterps);

        svgElem->AddChild(anim);
    }
}

void SmilDoc::ReadAnimateNodes(wxXmlNode* svgElem, const wxString& elemId,
                                SmilScene& scene) {
    for (wxXmlNode* ch = svgElem->GetChildren(); ch; ch = ch->GetNext()) {
        if (ch->GetType() != wxXML_ELEMENT_NODE) continue;
        if (ch->GetName() != "animate") continue;

        wxString prop     = ch->GetAttribute("attributeName");
        wxString values   = ch->GetAttribute("values");
        wxString ktStr    = ch->GetAttribute("keyTimes");
        wxString durStr   = ch->GetAttribute("dur");
        wxString interps  = ch->GetAttribute("swim:keyInterps");
        wxString splines  = ch->GetAttribute("keySplines");

        if (prop.IsEmpty() || values.IsEmpty() || ktStr.IsEmpty()) continue;

        // Parse duration in seconds (strip trailing 's').
        double durSec = (double)scene.durationFrames / (double)(m_fps > 0 ? m_fps : 24);
        if (durStr.EndsWith("s")) durStr = durStr.Left(durStr.Length() - 1);
        double d = 0;
        if (durStr.ToDouble(&d) && d > 0) durSec = d;

        wxArrayString vals  = wxSplit(values,  ';');
        wxArrayString kts   = wxSplit(ktStr,   ';');
        wxArrayString interp= wxSplit(interps, ';');
        wxArrayString spl   = wxSplit(splines, ';');

        SmilElement& elem = scene.elements[elemId];
        elem.elementId = elemId;
        SmilTrack& track = elem.tracks[prop];

        size_t n = std::min(vals.size(), kts.size());
        for (size_t i = 0; i < n; ++i) {
            double kt = 0;
            kts[i].ToDouble(&kt);
            int frame = (int)std::round(kt * scene.durationFrames);

            wxString interpStr = (i < interp.size()) ? interp[i] : "linear";
            SmilInterp si = SmilInterp::Linear;
            if (interpStr == "bezier") si = SmilInterp::Bezier;
            else if (interpStr == "cubic") si = SmilInterp::Cubic;

            track.SetKeyframe(frame, vals[i], si);

            // Read Bezier control points.
            if (si == SmilInterp::Bezier && i < spl.size()) {
                wxString& sp = spl[i];
                float cx1,cy1,cx2,cy2;
                if (sscanf(sp.c_str(), "%f %f %f %f", &cx1,&cy1,&cx2,&cy2) == 4) {
                    for (SmilKeyframe& kf : track.keyframes)
                        if (kf.frame == frame)
                            { kf.cx1=cx1; kf.cy1=cy1; kf.cx2=cx2; kf.cy2=cy2; }
                }
            }
        }
    }
}

wxXmlNode* SmilDoc::SceneToXml(const SmilScene& scene) const {
    wxXmlNode* sceneNode = new wxXmlNode(nullptr, wxXML_ELEMENT_NODE, "swim:scene");
    sceneNode->AddAttribute("id",            scene.id);
    sceneNode->AddAttribute("swim:name",     scene.name);
    sceneNode->AddAttribute("swim:start",    wxString::Format("%d", scene.startFrame));
    sceneNode->AddAttribute("swim:duration", wxString::Format("%d", scene.durationFrames));

    if (!scene.embedded) {
        sceneNode->AddAttribute("swim:refPath", scene.refPath);
        return sceneNode;
    }

    sceneNode->AddAttribute("swim:pageWidth",  wxString::Format("%d", scene.pageWidth));
    sceneNode->AddAttribute("swim:pageHeight", wxString::Format("%d", scene.pageHeight));
    sceneNode->AddAttribute("swim:bgColor",    ColToHex(scene.bgColour));

    // Build inner <svg> element.
    wxXmlNode* svgNode = new wxXmlNode(nullptr, wxXML_ELEMENT_NODE, "svg");
    svgNode->AddAttribute("xmlns",      "http://www.w3.org/2000/svg");
    svgNode->AddAttribute("xmlns:swim", "https://swim.spacify.se/TR/");
    svgNode->AddAttribute("version",    "1.1");
    svgNode->AddAttribute("width",  wxString::Format("%d", scene.pageWidth));
    svgNode->AddAttribute("height", wxString::Format("%d", scene.pageHeight));

    for (int i = 0; i < (int)scene.shapes.size(); ++i) {
        wxString eid = ShapeElementId(scene.shapes[i], i);
        wxXmlNode* shapeNode = ShapeToSmilXml(scene.shapes[i], eid, scene);
        if (shapeNode) svgNode->AddChild(shapeNode);
    }

    sceneNode->AddChild(svgNode);
    return sceneNode;
}

bool SmilDoc::XmlToScene(wxXmlNode* node, SmilScene& scene) {
    scene.id            = node->GetAttribute("id");
    scene.name          = node->GetAttribute("swim:name", "Scene");
    long sv=0, dv=0;
    node->GetAttribute("swim:start",    "0").ToLong(&sv);
    node->GetAttribute("swim:duration", "240").ToLong(&dv);
    scene.startFrame    = (int)sv;
    scene.durationFrames = (int)dv;

    wxString refPath = node->GetAttribute("swim:refPath");
    if (!refPath.IsEmpty()) {
        scene.embedded = false;
        scene.refPath  = refPath;
        return true;
    }
    scene.embedded = true;

    long pw=1920, ph=1080;
    node->GetAttribute("swim:pageWidth",  "1920").ToLong(&pw);
    node->GetAttribute("swim:pageHeight", "1080").ToLong(&ph);
    scene.pageWidth  = (int)pw;
    scene.pageHeight = (int)ph;
    scene.bgColour   = HexToCol(node->GetAttribute("swim:bgColor", "#FFFFFF"));

    // Find inner <svg>.
    for (wxXmlNode* ch = node->GetChildren(); ch; ch = ch->GetNext()) {
        if (ch->GetType() != wxXML_ELEMENT_NODE) continue;
        if (ch->GetName() != "svg") continue;

        scene.shapes.clear();
        int idx = 0;
        for (wxXmlNode* shNode = ch->GetChildren(); shNode; shNode = shNode->GetNext()) {
            if (shNode->GetType() != wxXML_ELEMENT_NODE) continue;
            // Skip <animate> and other SMIL children.
            const wxString& nm = shNode->GetName();
            if (nm == "animate" || nm == "animateTransform") continue;

            DrawShape s;
            if (!DrawDoc::XmlToShape(shNode, s)) continue;
            scene.shapes.push_back(s);

            wxString eid = shNode->GetAttribute("id");
            if (eid.IsEmpty()) eid = ShapeElementId(s, idx);
            ReadAnimateNodes(shNode, eid, scene);
            ++idx;
        }
        break;
    }
    return true;
}

bool SmilDoc::DoSaveDocument(const wxString& filename) {
    wxXmlDocument doc;

    wxXmlNode* root = new wxXmlNode(nullptr, wxXML_ELEMENT_NODE, "smil");
    root->AddAttribute("xmlns",      "http://www.w3.org/ns/SMIL");
    root->AddAttribute("xmlns:swim", "https://spacify.se/swim/1.0");
    root->AddAttribute("swim:fps",   wxString::Format("%d", m_fps));
    doc.SetRoot(root);

    wxXmlNode* body = new wxXmlNode(nullptr, wxXML_ELEMENT_NODE, "body");
    root->AddChild(body);

    for (const SmilScene& sc : m_scenes) {
        wxXmlNode* scNode = SceneToXml(sc);
        if (scNode) body->AddChild(scNode);
    }

    if (!doc.Save(filename)) return false;
    Modify(false);
    GetCommandProcessor()->MarkAsSaved();
    return true;
}

bool SmilDoc::DoOpenDocument(const wxString& filename) {
    wxXmlDocument doc;
    if (!doc.Load(filename)) return false;

    wxXmlNode* root = doc.GetRoot();
    if (!root || root->GetName() != "smil") return false;

    long fps = 24;
    root->GetAttribute("swim:fps", "24").ToLong(&fps);
    m_fps = (int)fps;

    m_scenes.clear();
    GetCommandProcessor()->ClearCommands();

    // Find <body>.
    for (wxXmlNode* ch = root->GetChildren(); ch; ch = ch->GetNext()) {
        if (ch->GetType() != wxXML_ELEMENT_NODE) continue;
        if (ch->GetName() != "body") continue;

        for (wxXmlNode* sc = ch->GetChildren(); sc; sc = sc->GetNext()) {
            if (sc->GetType() != wxXML_ELEMENT_NODE) continue;
            if (sc->GetName() != "swim:scene") continue;
            SmilScene scene;
            if (XmlToScene(sc, scene))
                m_scenes.push_back(scene);
        }
        break;
    }

    if (m_scenes.empty()) {
        SmilScene def;
        def.id = "scene_1"; def.name = "Scene 1";
        m_scenes.push_back(def);
    }
    m_currentScene = 0;
    m_currentFrame = 0;
    Modify(false);
    UpdateAllViews();
    return true;
}
