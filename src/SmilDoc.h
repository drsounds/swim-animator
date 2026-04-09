#pragma once
#include <wx/docview.h>
#include <wx/xml/xml.h>
#include "SmilTypes.h"
#include <vector>
#include <map>

// ---------------------------------------------------------------------------
// SmilDoc – wxDocument subclass for .smil SMIL animation files.
//
// A SMIL document holds one or more scenes.  Each scene has SVG shape content
// (reusing the DrawShape model) plus per-property keyframe animation data stored
// as SMIL <animate> elements in the file.  Scenes may be embedded inline or
// reference an external .smil file via refPath.
// ---------------------------------------------------------------------------
class SmilDoc : public wxDocument {
    wxDECLARE_DYNAMIC_CLASS(SmilDoc);
public:
    SmilDoc();

    bool OnNewDocument() override;
    bool DoSaveDocument(const wxString& filename) override;
    bool DoOpenDocument(const wxString& filename) override;
    bool IsModified() const override;
    void Modify(bool modified) override;

    // ---- Frame rate ----
    int  GetFps() const  { return m_fps; }
    void SetFps(int fps) { m_fps = fps; Modify(true); }

    // ---- Scenes ----
    const std::vector<SmilScene>& GetScenes()            const { return m_scenes; }
    int                           GetCurrentSceneIndex() const { return m_currentScene; }
    SmilScene*       GetCurrentScene();
    const SmilScene* GetCurrentScene() const;

    // Total frame span across all scenes.
    int GetTotalFrames() const;

    void SetCurrentScene(int idx);          // fires UpdateAllViews
    int  AddScene(const SmilScene& s);      // appends; returns new index
    void RemoveScene(int idx);
    void UpdateScene(int idx, const SmilScene& s);
    void MoveScene(int fromIdx, int toIdx);

    // ---- Playhead ----
    int  GetCurrentFrame() const { return m_currentFrame; }
    void SetCurrentFrame(int f); // clamps; fires UpdateAllViews

    // ---- Record mode ----
    bool GetRecMode() const   { return m_recMode; }
    void SetRecMode(bool rec) { m_recMode = rec; }

    // ---- Keyframe editing (on the current scene) ----
    void     SetKeyframe(const wxString& elemId, const wxString& prop,
                         int frame, const wxString& value,
                         SmilInterp interp = SmilInterp::Linear);
    void     RemoveKeyframe(const wxString& elemId, const wxString& prop, int frame);
    bool     HasKeyframe(const wxString& elemId, const wxString& prop, int frame) const;
    wxString GetAnimatedValue(const wxString& elemId, const wxString& prop) const;

    // Derive a stable element id from a shape and its index in the scene.
    static wxString ShapeElementId(const DrawShape& s, int idx);

private:
    wxXmlNode* SceneToXml(const SmilScene& scene) const;
    bool       XmlToScene(wxXmlNode* node, SmilScene& scene);
    wxXmlNode* ShapeToSmilXml(const DrawShape& s, const wxString& elemId,
                               const SmilScene& scene) const;
    void WriteAnimateNodes(wxXmlNode* svgElem, const wxString& elemId,
                           const SmilScene& scene) const;
    void ReadAnimateNodes(wxXmlNode* svgElem, const wxString& elemId,
                          SmilScene& scene);

    int                    m_fps           {24};
    std::vector<SmilScene> m_scenes;
    int                    m_currentScene  {0};
    int                    m_currentFrame  {0};
    bool                   m_recMode       {false};
    bool                   m_modified      {false};
};
