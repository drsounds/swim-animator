#include "PropPanel.h"
#include "DrawDoc.h"
#include "DrawCommands.h"
#include <wx/colordlg.h>
#include <wx/sizer.h>
#include <wx/colour.h>
#include <wx/statline.h>
#include <algorithm>

// ---------------------------------------------------------------------------
// Helpers
// ---------------------------------------------------------------------------

static wxString KindName(ShapeKind k) {
    switch (k) {
        case ShapeKind::Rect:   return "Rectangle";
        case ShapeKind::Circle: return "Circle";
        case ShapeKind::Text:   return "Text";
        case ShapeKind::Bezier: return "Bezier";
    }
    return "";
}

// Add a label + control pair as one row in a 2-column FlexGridSizer.
static void AddRow(wxWindow* parent, wxSizer* grid,
                   const wxString& label, wxWindow* ctrl)
{
    grid->Add(new wxStaticText(parent, wxID_ANY, label),
              0, wxALIGN_CENTER_VERTICAL | wxRIGHT, 4);
    grid->Add(ctrl, 1, wxEXPAND | wxALIGN_CENTER_VERTICAL);
}

// ---------------------------------------------------------------------------
// Construction
// ---------------------------------------------------------------------------

PropPanel::PropPanel(wxWindow* parent)
    : wxScrolledWindow(parent, wxID_ANY)
{
    SetScrollRate(0, 5);
    BuildUI();
}

// Preset table shared with NewDrawingDialog
struct DocPreset { const char* label; int w; int h; };
static const DocPreset kDocPresets[] = {
    { "1920 × 1080  (Full HD landscape)", 1920, 1080 },
    { "1080 × 1920  (Full HD portrait)",  1080, 1920 },
    { "1024 × 768",                       1024,  768 },
    { "640 × 480",                         640,  480 },
    { "Custom",                              -1,   -1 },
};
static const int kDocPresetCount = (int)(sizeof(kDocPresets) / sizeof(kDocPresets[0]));
static const int kDocCustomIndex = kDocPresetCount - 1;

void PropPanel::BuildUI() {
    const int M = 6;  // outer margin
    const int G = 4;  // inner grid gap

    auto* outer = new wxBoxSizer(wxVERTICAL);

    // --- No-selection label ---
    m_noSelLabel = new wxStaticText(this, wxID_ANY, "No selection",
                                    wxDefaultPosition, wxDefaultSize,
                                    wxALIGN_CENTER_HORIZONTAL);
    outer->Add(m_noSelLabel, 0, wxALL | wxEXPAND, 10);

    // --- Document properties panel (shown when a doc is active, no shape selected) ---
    m_docPanel = new wxPanel(this, wxID_ANY);
    {
        auto* sizer = new wxBoxSizer(wxVERTICAL);
        sizer->Add(new wxStaticText(m_docPanel, wxID_ANY, "Document"),
                   0, wxLEFT | wxTOP, M);
        sizer->Add(new wxStaticLine(m_docPanel), 0, wxEXPAND | wxLEFT | wxRIGHT | wxBOTTOM, M);

        auto* grid = new wxFlexGridSizer(2, wxSize(G, 3));
        grid->AddGrowableCol(1);

        wxArrayString presetLabels;
        for (int i = 0; i < kDocPresetCount; i++)
            presetLabels.Add(kDocPresets[i].label);

        m_docPreset = new wxChoice(m_docPanel, wxID_ANY,
                                   wxDefaultPosition, wxDefaultSize, presetLabels);
        AddRow(m_docPanel, grid, "Preset:", m_docPreset);

        m_docWidthSpin  = new wxSpinCtrl(m_docPanel, wxID_ANY, "",
                                         wxDefaultPosition, wxDefaultSize,
                                         wxSP_ARROW_KEYS, 1, 32768, 1920);
        m_docHeightSpin = new wxSpinCtrl(m_docPanel, wxID_ANY, "",
                                         wxDefaultPosition, wxDefaultSize,
                                         wxSP_ARROW_KEYS, 1, 32768, 1080);
        AddRow(m_docPanel, grid, "Width:",  m_docWidthSpin);
        AddRow(m_docPanel, grid, "Height:", m_docHeightSpin);

        m_docBgBtn = new wxButton(m_docPanel, wxID_ANY, "",
                                  wxDefaultPosition, wxSize(-1, 22));
        AddRow(m_docPanel, grid, "Background:", m_docBgBtn);

        sizer->Add(grid, 0, wxEXPAND | wxALL, M);
        m_docPanel->SetSizer(sizer);

        m_docPreset->Bind(wxEVT_CHOICE, [this](wxCommandEvent&) {
            if (m_updating) return;
            int sel = m_docPreset->GetSelection();
            const bool isCustom = (sel == kDocCustomIndex);
            if (!isCustom && sel >= 0 && sel < kDocCustomIndex) {
                m_updating = true;
                m_docWidthSpin->SetValue(kDocPresets[sel].w);
                m_docHeightSpin->SetValue(kDocPresets[sel].h);
                m_updating = false;
                SubmitDocSizeChange();
            }
            m_docWidthSpin->Enable(isCustom);
            m_docHeightSpin->Enable(isCustom);
        });

        auto onDocSpin = [this](wxSpinEvent&) {
            if (m_updating) return;
            // Mark as custom when the user manually edits a spin
            m_docPreset->SetSelection(kDocCustomIndex);
            m_docWidthSpin->Enable(true);
            m_docHeightSpin->Enable(true);
            SubmitDocSizeChange();
        };
        m_docWidthSpin->Bind(wxEVT_SPINCTRL,  onDocSpin);
        m_docHeightSpin->Bind(wxEVT_SPINCTRL, onDocSpin);
        m_docBgBtn->Bind(wxEVT_BUTTON, [this](wxCommandEvent&) { PickDocBgColour(); });
    }
    outer->Add(m_docPanel, 0, wxEXPAND);

    // --- Shape panel (shown when a shape is selected) ---
    m_shapePanel = new wxPanel(this, wxID_ANY);
    auto* shapeSizer = new wxBoxSizer(wxVERTICAL);
    m_shapePanel->SetSizer(shapeSizer);
    outer->Add(m_shapePanel, 1, wxEXPAND);

    // Kind row
    {
        auto* grid = new wxFlexGridSizer(2, wxSize(G, 3));
        grid->AddGrowableCol(1);
        m_kindValue = new wxStaticText(m_shapePanel, wxID_ANY, "");
        AddRow(m_shapePanel, grid, "Kind:", m_kindValue);
        shapeSizer->Add(grid, 0, wxEXPAND | wxALL, M);
    }

    // Bounds panel (X, Y, W, H) — hidden for Bezier
    m_boundsPanel = new wxPanel(m_shapePanel, wxID_ANY);
    {
        auto* grid = new wxFlexGridSizer(2, wxSize(G, 3));
        grid->AddGrowableCol(1);

        auto mkSpin = [&](int lo, int hi) {
            return new wxSpinCtrl(m_boundsPanel, wxID_ANY, "",
                                  wxDefaultPosition, wxDefaultSize,
                                  wxSP_ARROW_KEYS, lo, hi, 0);
        };
        m_xSpin = mkSpin(-5000, 5000);
        m_ySpin = mkSpin(-5000, 5000);
        m_wSpin = mkSpin(4, 5000);
        m_hSpin = mkSpin(4, 5000);

        AddRow(m_boundsPanel, grid, "X:", m_xSpin);
        AddRow(m_boundsPanel, grid, "Y:", m_ySpin);
        AddRow(m_boundsPanel, grid, "W:", m_wSpin);
        AddRow(m_boundsPanel, grid, "H:", m_hSpin);
        m_boundsPanel->SetSizer(grid);

        auto onSpin = [this](wxSpinEvent&) { SubmitBoundsChange(); };
        m_xSpin->Bind(wxEVT_SPINCTRL, onSpin);
        m_ySpin->Bind(wxEVT_SPINCTRL, onSpin);
        m_wSpin->Bind(wxEVT_SPINCTRL, onSpin);
        m_hSpin->Bind(wxEVT_SPINCTRL, onSpin);
    }
    shapeSizer->Add(m_boundsPanel, 0, wxEXPAND | wxLEFT | wxRIGHT | wxBOTTOM, M);

    // Foreground color row
    {
        auto* grid = new wxFlexGridSizer(2, wxSize(G, 3));
        grid->AddGrowableCol(1);
        m_fgBtn = new wxButton(m_shapePanel, wxID_ANY, "",
                               wxDefaultPosition, wxSize(-1, 22));
        AddRow(m_shapePanel, grid, "Foreground:", m_fgBtn);
        shapeSizer->Add(grid, 0, wxEXPAND | wxLEFT | wxRIGHT | wxBOTTOM, M);
        m_fgBtn->Bind(wxEVT_BUTTON, [this](wxCommandEvent&) { PickColour(true); });
    }

    // Background color panel — hidden for Bezier
    m_bgPanel = new wxPanel(m_shapePanel, wxID_ANY);
    {
        auto* grid = new wxFlexGridSizer(2, wxSize(G, 3));
        grid->AddGrowableCol(1);
        m_bgBtn = new wxButton(m_bgPanel, wxID_ANY, "",
                               wxDefaultPosition, wxSize(-1, 22));
        AddRow(m_bgPanel, grid, "Background:", m_bgBtn);
        m_bgPanel->SetSizer(grid);
        m_bgBtn->Bind(wxEVT_BUTTON, [this](wxCommandEvent&) { PickColour(false); });
    }
    shapeSizer->Add(m_bgPanel, 0, wxEXPAND | wxLEFT | wxRIGHT | wxBOTTOM, M);

    // Stroke width panel — hidden for Text shapes
    m_strokePanel = new wxPanel(m_shapePanel, wxID_ANY);
    {
        auto* grid = new wxFlexGridSizer(2, wxSize(G, 3));
        grid->AddGrowableCol(1);
        m_strokeWidthSpin = new wxSpinCtrl(m_strokePanel, wxID_ANY, "",
                                           wxDefaultPosition, wxDefaultSize,
                                           wxSP_ARROW_KEYS, 0, 100, 1);
        AddRow(m_strokePanel, grid, "Border width:", m_strokeWidthSpin);
        m_strokePanel->SetSizer(grid);
        m_strokeWidthSpin->Bind(wxEVT_SPINCTRL,
            [this](wxSpinEvent&) { SubmitBorderChange(); });
    }
    shapeSizer->Add(m_strokePanel, 0, wxEXPAND | wxLEFT | wxRIGHT | wxBOTTOM, M);

    // Border radius panel — Rect shapes only
    m_borderRadiusPanel = new wxPanel(m_shapePanel, wxID_ANY);
    {
        auto* grid = new wxFlexGridSizer(2, wxSize(G, 3));
        grid->AddGrowableCol(1);
        m_borderRadiusXSpin = new wxSpinCtrl(m_borderRadiusPanel, wxID_ANY, "",
                                             wxDefaultPosition, wxDefaultSize,
                                             wxSP_ARROW_KEYS, 0, 2500, 0);
        m_borderRadiusYSpin = new wxSpinCtrl(m_borderRadiusPanel, wxID_ANY, "",
                                             wxDefaultPosition, wxDefaultSize,
                                             wxSP_ARROW_KEYS, 0, 2500, 0);
        AddRow(m_borderRadiusPanel, grid, "Radius X:", m_borderRadiusXSpin);
        AddRow(m_borderRadiusPanel, grid, "Radius Y:", m_borderRadiusYSpin);
        m_borderRadiusPanel->SetSizer(grid);
        auto onSpin = [this](wxSpinEvent&) { SubmitBorderChange(); };
        m_borderRadiusXSpin->Bind(wxEVT_SPINCTRL, onSpin);
        m_borderRadiusYSpin->Bind(wxEVT_SPINCTRL, onSpin);
    }
    shapeSizer->Add(m_borderRadiusPanel, 0, wxEXPAND | wxLEFT | wxRIGHT | wxBOTTOM, M);

    // Label panel — Text shapes only
    m_labelPanel = new wxPanel(m_shapePanel, wxID_ANY);
    {
        auto* grid = new wxFlexGridSizer(2, wxSize(G, 3));
        grid->AddGrowableCol(1);
        m_labelCtrl = new wxTextCtrl(m_labelPanel, wxID_ANY, "",
                                     wxDefaultPosition, wxDefaultSize,
                                     wxTE_PROCESS_ENTER);
        AddRow(m_labelPanel, grid, "Label:", m_labelCtrl);
        m_labelPanel->SetSizer(grid);

        m_labelCtrl->Bind(wxEVT_TEXT_ENTER,  [this](wxCommandEvent&) { SubmitLabelChange(); });
        m_labelCtrl->Bind(wxEVT_KILL_FOCUS,  [this](wxFocusEvent& e) { SubmitLabelChange(); e.Skip(); });
    }
    shapeSizer->Add(m_labelPanel, 0, wxEXPAND | wxLEFT | wxRIGHT | wxBOTTOM, M);

    // Bezier points panel — Bezier shapes only
    m_bezierPanel = new wxPanel(m_shapePanel, wxID_ANY);
    {
        auto* grid = new wxFlexGridSizer(3, wxSize(G, 3));
        grid->AddGrowableCol(1);
        grid->AddGrowableCol(2);

        static const char* ptLabels[4] = {"P0:", "P1:", "P2:", "P3:"};
        for (int pi = 0; pi < 4; pi++) {
            grid->Add(new wxStaticText(m_bezierPanel, wxID_ANY, ptLabels[pi]),
                      0, wxALIGN_CENTER_VERTICAL | wxRIGHT, 4);
            for (int xy = 0; xy < 2; xy++) {
                m_ptSpin[pi][xy] = new wxSpinCtrl(m_bezierPanel, wxID_ANY, "",
                                                  wxDefaultPosition, wxDefaultSize,
                                                  wxSP_ARROW_KEYS, -5000, 5000, 0);
                grid->Add(m_ptSpin[pi][xy], 1, wxEXPAND | wxALIGN_CENTER_VERTICAL);
                m_ptSpin[pi][xy]->Bind(wxEVT_SPINCTRL,
                    [this](wxSpinEvent&) { SubmitPtChange(); });
            }
        }
        m_bezierPanel->SetSizer(grid);
    }
    shapeSizer->Add(m_bezierPanel, 0, wxEXPAND | wxLEFT | wxRIGHT | wxBOTTOM, M);

    SetSizer(outer);
    ShowShape(nullptr, -1);
}

// ---------------------------------------------------------------------------
// Public interface
// ---------------------------------------------------------------------------

void PropPanel::ShowShape(DrawDoc* doc, int idx) {
    m_updating = true;

    m_doc = doc;
    m_idx = idx;

    const bool hasSel = (doc != nullptr && idx >= 0 &&
                         idx < (int)doc->GetShapes().size());
    const bool hasDoc = (doc != nullptr && !hasSel);

    m_noSelLabel->Show(!hasSel && !hasDoc);
    m_docPanel->Show(hasDoc);
    m_shapePanel->Show(hasSel);

    if (hasDoc) {
        // Populate document properties
        int w = doc->GetPageWidth(), h = doc->GetPageHeight();
        m_docWidthSpin->SetValue(w);
        m_docHeightSpin->SetValue(h);
        m_docBgBtn->SetBackgroundColour(doc->GetBgColour());
        m_docBgBtn->Refresh();

        // Select matching preset or "Custom"
        int presetSel = kDocCustomIndex;
        for (int i = 0; i < kDocCustomIndex; i++) {
            if (kDocPresets[i].w == w && kDocPresets[i].h == h) {
                presetSel = i;
                break;
            }
        }
        m_docPreset->SetSelection(presetSel);
        m_docWidthSpin->Enable(presetSel == kDocCustomIndex);
        m_docHeightSpin->Enable(presetSel == kDocCustomIndex);

        m_docPanel->Layout();
    }

    if (hasSel) {
        const DrawShape& s = doc->GetShapes()[idx];
        const bool isBezier = (s.kind == ShapeKind::Bezier);
        const bool isText   = (s.kind == ShapeKind::Text);

        m_kindValue->SetLabel(KindName(s.kind));

        const bool isRect = (s.kind == ShapeKind::Rect);

        // Show/hide sections per shape kind
        m_shapePanel->GetSizer()->Show(m_boundsPanel,       !isBezier);
        m_shapePanel->GetSizer()->Show(m_bgPanel,           true);
        m_shapePanel->GetSizer()->Show(m_strokePanel,       !isText);
        m_shapePanel->GetSizer()->Show(m_borderRadiusPanel, isRect);
        m_shapePanel->GetSizer()->Show(m_labelPanel,        isText);
        m_shapePanel->GetSizer()->Show(m_bezierPanel,       isBezier);

        if (!isBezier) {
            m_xSpin->SetValue(s.bounds.x);
            m_ySpin->SetValue(s.bounds.y);
            m_wSpin->SetValue(s.bounds.width);
            m_hSpin->SetValue(s.bounds.height);
        }

        if (!isText)
            m_strokeWidthSpin->SetValue(s.strokeWidth);

        if (isRect) {
            m_borderRadiusXSpin->SetRange(0, s.bounds.width  / 2);
            m_borderRadiusYSpin->SetRange(0, s.bounds.height / 2);
            m_borderRadiusXSpin->SetValue(s.borderRadiusX);
            m_borderRadiusYSpin->SetValue(s.borderRadiusY);
        }

        m_fgBtn->SetBackgroundColour(s.fgColour);
        m_fgBtn->Refresh();

        m_bgBtn->SetBackgroundColour(s.bgColour);
        m_bgBtn->Refresh();

        if (isText)
            m_labelCtrl->ChangeValue(s.label);

        if (isBezier) {
            for (int pi = 0; pi < 4; pi++) {
                m_ptSpin[pi][0]->SetValue(s.pts[pi].x);
                m_ptSpin[pi][1]->SetValue(s.pts[pi].y);
            }
        }

        m_shapePanel->Layout();
    }

    GetSizer()->Layout();
    FitInside();
    Refresh();

    m_updating = false;
}

// ---------------------------------------------------------------------------
// Submit helpers
// ---------------------------------------------------------------------------

void PropPanel::SubmitBoundsChange() {
    if (m_updating || !m_doc || m_idx < 0) return;
    if (m_idx >= (int)m_doc->GetShapes().size()) return;

    DrawShape before = m_doc->GetShapes()[m_idx];
    DrawShape after  = before;
    after.bounds.x      = m_xSpin->GetValue();
    after.bounds.y      = m_ySpin->GetValue();
    after.bounds.width  = m_wSpin->GetValue();
    after.bounds.height = m_hSpin->GetValue();

    if (after.kind == ShapeKind::Rect) {
        after.borderRadiusX = std::min(after.borderRadiusX, after.bounds.width  / 2);
        after.borderRadiusY = std::min(after.borderRadiusY, after.bounds.height / 2);
    }
    if (after.bounds == before.bounds &&
        after.borderRadiusX == before.borderRadiusX &&
        after.borderRadiusY == before.borderRadiusY) return;
    m_doc->GetCommandProcessor()->Submit(
        new UpdateShapeCmd(m_doc, m_idx, before, after, "Edit Position/Size"));
}

void PropPanel::SubmitPtChange() {
    if (m_updating || !m_doc || m_idx < 0) return;
    if (m_idx >= (int)m_doc->GetShapes().size()) return;

    DrawShape before = m_doc->GetShapes()[m_idx];
    DrawShape after  = before;
    for (int pi = 0; pi < 4; pi++) {
        after.pts[pi].x = m_ptSpin[pi][0]->GetValue();
        after.pts[pi].y = m_ptSpin[pi][1]->GetValue();
    }
    // Recompute bounding box from new control points.
    int minX = after.pts[0].x, maxX = after.pts[0].x;
    int minY = after.pts[0].y, maxY = after.pts[0].y;
    for (int i = 1; i < 4; i++) {
        minX = std::min(minX, after.pts[i].x); maxX = std::max(maxX, after.pts[i].x);
        minY = std::min(minY, after.pts[i].y); maxY = std::max(maxY, after.pts[i].y);
    }
    after.bounds = wxRect(minX, minY, maxX - minX, maxY - minY);

    bool changed = false;
    for (int i = 0; i < 4; i++)
        changed |= (after.pts[i].x != before.pts[i].x || after.pts[i].y != before.pts[i].y);
    if (!changed) return;

    m_doc->GetCommandProcessor()->Submit(
        new UpdateShapeCmd(m_doc, m_idx, before, after, "Edit Bezier Points"));
}

void PropPanel::SubmitLabelChange() {
    if (m_updating || !m_doc || m_idx < 0) return;
    if (m_idx >= (int)m_doc->GetShapes().size()) return;

    DrawShape before = m_doc->GetShapes()[m_idx];
    DrawShape after  = before;
    after.label = m_labelCtrl->GetValue();

    if (after.label == before.label) return;
    m_doc->GetCommandProcessor()->Submit(
        new UpdateShapeCmd(m_doc, m_idx, before, after, "Edit Label"));
}

void PropPanel::SubmitBorderChange() {
    if (m_updating || !m_doc || m_idx < 0) return;
    if (m_idx >= (int)m_doc->GetShapes().size()) return;

    DrawShape before = m_doc->GetShapes()[m_idx];
    DrawShape after  = before;

    if (before.kind != ShapeKind::Text)
        after.strokeWidth = m_strokeWidthSpin->GetValue();

    if (before.kind == ShapeKind::Rect) {
        int maxRx = before.bounds.width  / 2;
        int maxRy = before.bounds.height / 2;
        after.borderRadiusX = std::min(m_borderRadiusXSpin->GetValue(), maxRx);
        after.borderRadiusY = std::min(m_borderRadiusYSpin->GetValue(), maxRy);
    }

    if (after.strokeWidth == before.strokeWidth &&
        after.borderRadiusX == before.borderRadiusX &&
        after.borderRadiusY == before.borderRadiusY) return;
    // Note: for non-Rect shapes borderRadius fields are never modified above,
    // so the radius comparisons trivially hold and only strokeWidth is checked.

    m_doc->GetCommandProcessor()->Submit(
        new UpdateShapeCmd(m_doc, m_idx, before, after, "Edit Border"));
}

void PropPanel::SubmitDocSizeChange() {
    if (!m_doc) return;
    int w = m_docWidthSpin->GetValue();
    int h = m_docHeightSpin->GetValue();
    if (w == m_doc->GetPageWidth() && h == m_doc->GetPageHeight()) return;
    m_doc->SetPageSize(w, h);
}

void PropPanel::PickDocBgColour() {
    if (!m_doc) return;
    wxColourData cd;
    cd.SetChooseFull(true);
    cd.SetColour(m_doc->GetBgColour());
    wxColourDialog dlg(this, &cd);
    if (dlg.ShowModal() != wxID_OK) return;
    wxColour c = dlg.GetColourData().GetColour();
    m_docBgBtn->SetBackgroundColour(c);
    m_docBgBtn->Refresh();
    m_doc->SetBgColour(c);
}

void PropPanel::PickColour(bool fg) {
    if (!m_doc || m_idx < 0) return;
    if (m_idx >= (int)m_doc->GetShapes().size()) return;

    DrawShape before = m_doc->GetShapes()[m_idx];
    wxColourData cd;
    cd.SetChooseFull(true);
    cd.SetColour(fg ? before.fgColour : before.bgColour);

    wxColourDialog dlg(this, &cd);
    if (dlg.ShowModal() != wxID_OK) return;

    DrawShape after = before;
    if (fg) after.fgColour = dlg.GetColourData().GetColour();
    else    after.bgColour = dlg.GetColourData().GetColour();

    m_doc->GetCommandProcessor()->Submit(
        new UpdateShapeCmd(m_doc, m_idx, before, after,
                           fg ? "Edit Foreground Color" : "Edit Background Color"));
}
