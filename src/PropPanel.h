#pragma once
#include <wx/panel.h>
#include <wx/notebook.h>
#include <wx/scrolwin.h>
#include <wx/spinctrl.h>
#include <wx/button.h>
#include <wx/stattext.h>
#include <wx/textctrl.h>
#include <wx/choice.h>
#include <wx/sizer.h>
#include <wx/xml/xml.h>
#include <vector>
#include "DrawShape.h"

class DrawDoc;
class SmilDoc;

// ---------------------------------------------------------------------------
// PropPanel – dockable pane with a tab bar.  Currently a single "Properties"
// tab holds shape and document properties; future tabs can be added without
// restructuring existing code.
//
// When a DrawDoc is active but no shape is selected, the Properties tab shows
// document properties (page size, background colour).  When a shape is
// selected it shows shape properties.
// ---------------------------------------------------------------------------
class PropPanel : public wxPanel {
public:
    PropPanel(wxWindow* parent);

    // Call whenever selection or document state changes.
    // doc=nullptr, idx=-1  → "No selection"
    // doc!=nullptr, idx=-1 → document properties
    // doc!=nullptr, idx>=0 → shape properties
    void ShowShape(DrawDoc* doc, int idx);

    // Set SMIL context for keyframe diamond buttons.
    // Pass nullptr to clear (non-SMIL mode).
    void SetSmilContext(SmilDoc* smilDoc, const wxString& elemId);

private:
    void BuildUI();
    void BuildXmlTab();
    void PopulateXmlTab();
    void OnXmlAttrChanged(wxCommandEvent& e);

    // Submit helpers – read current control values → build after-state → Submit cmd.
    void SubmitBoundsChange();
    void SubmitPtChange();
    void SubmitLabelChange();
    void SubmitBorderChange();
    void PickColour(bool fg);

    // Document-property helpers
    void SubmitDocSizeChange();
    void PickDocBgColour();

    // ---- Tab structure ----
    wxNotebook*      m_notebook   {nullptr};
    wxScrolledWindow* m_propsPage {nullptr};  // "Properties" tab page

    // ---- Controls (all children of m_propsPage) ----
    wxStaticText* m_noSelLabel   {nullptr};

    // Document properties panel (shown when a DrawDoc is active, no shape selected)
    wxPanel*      m_docPanel     {nullptr};
    wxChoice*     m_docPreset    {nullptr};
    wxSpinCtrl*   m_docWidthSpin {nullptr};
    wxSpinCtrl*   m_docHeightSpin{nullptr};
    wxButton*     m_docBgBtn     {nullptr};

    wxPanel*      m_shapePanel   {nullptr};

    wxStaticText* m_kindValue    {nullptr};

    // Position/size (hidden for Bezier)
    wxPanel*      m_boundsPanel  {nullptr};
    wxSpinCtrl*   m_xSpin        {nullptr};
    wxSpinCtrl*   m_ySpin        {nullptr};
    wxSpinCtrl*   m_wSpin        {nullptr};
    wxSpinCtrl*   m_hSpin        {nullptr};

    // Colors
    wxButton*     m_fgBtn        {nullptr};
    wxPanel*      m_bgPanel      {nullptr};
    wxButton*     m_bgBtn        {nullptr};

    // Label (Text shapes only)
    wxPanel*      m_labelPanel   {nullptr};
    wxTextCtrl*   m_labelCtrl    {nullptr};

    // Border (stroke width + corner radius) — stroke hidden for Text, radius hidden for non-Rect
    wxPanel*      m_strokePanel       {nullptr};
    wxSpinCtrl*   m_strokeWidthSpin   {nullptr};
    wxPanel*      m_borderRadiusPanel {nullptr};
    wxSpinCtrl*   m_borderRadiusXSpin {nullptr};
    wxSpinCtrl*   m_borderRadiusYSpin {nullptr};

    // Bezier control points (hidden for non-Bezier)
    wxPanel*      m_bezierPanel  {nullptr};
    wxSpinCtrl*   m_ptSpin[4][2] {};  // [point 0-3][x=0, y=1]

    // ---- XML Attributes tab ----
    wxScrolledWindow*        m_xmlPage       {nullptr};
    wxFlexGridSizer*         m_xmlGrid       {nullptr};
    std::vector<wxString>    m_xmlAttrNames;
    std::vector<wxTextCtrl*> m_xmlValueCtrls;

    // ---- SMIL keyframe context ----
    SmilDoc*  m_smilDoc   {nullptr};
    wxString  m_smilElemId;
    wxPanel*  m_kfPanel   {nullptr};  // keyframe section inside Properties tab
    wxStaticText* m_kfLabel{nullptr};

    // ---- State ----
    DrawDoc* m_doc      {nullptr};
    int      m_idx      {-1};
    bool     m_updating {false};  // guard against re-entry during pane refresh
};
