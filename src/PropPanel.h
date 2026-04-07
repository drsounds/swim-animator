#pragma once
#include <wx/scrolwin.h>
#include <wx/spinctrl.h>
#include <wx/button.h>
#include <wx/stattext.h>
#include <wx/textctrl.h>
#include <wx/panel.h>
#include "DrawShape.h"

class DrawDoc;

// ---------------------------------------------------------------------------
// PropPanel – dockable right-side pane showing editable properties for the
// currently selected shape.  All edits are submitted through the document's
// wxCommandProcessor so they participate in undo/redo.
// ---------------------------------------------------------------------------
class PropPanel : public wxScrolledWindow {
public:
    PropPanel(wxWindow* parent);

    // Call whenever selection or document state changes.
    // Pass doc=nullptr or idx=-1 to show "No selection".
    void ShowShape(DrawDoc* doc, int idx);

private:
    void BuildUI();

    // Submit helpers – read current control values → build after-state → Submit cmd.
    void SubmitBoundsChange();
    void SubmitPtChange();
    void SubmitLabelChange();
    void PickColour(bool fg);

    // ---- Controls ----
    wxStaticText* m_noSelLabel   {nullptr};
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
    wxPanel*      m_bgPanel      {nullptr};  // hidden for Bezier
    wxButton*     m_bgBtn        {nullptr};

    // Label (Text shapes only)
    wxPanel*      m_labelPanel   {nullptr};
    wxTextCtrl*   m_labelCtrl    {nullptr};

    // Bezier control points (hidden for non-Bezier)
    wxPanel*      m_bezierPanel  {nullptr};
    wxSpinCtrl*   m_ptSpin[4][2] {};  // [point 0-3][x=0, y=1]

    // ---- State ----
    DrawDoc* m_doc      {nullptr};
    int      m_idx      {-1};
    bool     m_updating {false};  // guard against re-entry during pane refresh
};
