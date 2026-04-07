#include "ColorSwatchPanel.h"
#include "MainFrame.h"
#include "App.h"
#include "DrawView.h"
#include "DrawDoc.h"
#include "DrawCommands.h"
#include <wx/dcbuffer.h>
#include <wx/colordlg.h>
#include <wx/sizer.h>
#include <algorithm>

// ---------------------------------------------------------------------------
// Helper: apply a colour to the currently selected shape via undo command.
// ---------------------------------------------------------------------------
static void ApplyColourToSelection(MainFrame* frame, const wxColour& colour, bool fg) {
    DrawView* view = frame->GetActiveDrawView();
    if (!view) return;
    DrawCanvas* canvas = view->GetCanvas();
    if (!canvas) return;
    int idx = canvas->GetSelectedIndex();
    if (idx < 0) return;
    auto* doc = wxDynamicCast(view->GetDocument(), DrawDoc);
    if (!doc || idx >= (int)doc->GetShapes().size()) return;

    DrawShape before = doc->GetShapes()[idx];
    DrawShape after  = before;
    if (fg) after.fgColour = colour;
    else    after.bgColour = colour;

    doc->GetCommandProcessor()->Submit(
        new UpdateShapeCmd(doc, idx, before, after,
                           fg ? "Set Foreground Colour" : "Set Background Colour"));
}

// ---------------------------------------------------------------------------
// FgBgIndicatorCtrl – draws overlapping FG/BG squares; click to pick colour.
// ---------------------------------------------------------------------------

class FgBgIndicatorCtrl : public wxWindow {
public:
    FgBgIndicatorCtrl(wxWindow* parent, MainFrame* frame)
        : wxWindow(parent, wxID_ANY, wxDefaultPosition, wxSize(-1, 52))
        , m_frame(frame)
    {
        SetBackgroundStyle(wxBG_STYLE_PAINT);
        Bind(wxEVT_PAINT,     &FgBgIndicatorCtrl::OnPaint,    this);
        Bind(wxEVT_LEFT_DOWN, &FgBgIndicatorCtrl::OnLeftDown, this);
    }

private:
    // FG square top-left at (kPad, kPad); BG square offset by kOff.
    static constexpr int kSz  = 32;
    static constexpr int kOff = 12;
    static constexpr int kPad =  6;

    wxRect FgRect() const { return {kPad,        kPad,        kSz, kSz}; }
    wxRect BgRect() const { return {kPad + kOff, kPad + kOff, kSz, kSz}; }

    void OnPaint(wxPaintEvent&) {
        wxAutoBufferedPaintDC dc(this);
        dc.SetBackground(wxBrush(GetParent()->GetBackgroundColour()));
        dc.Clear();

        // BG square drawn first (behind).
        dc.SetPen(*wxBLACK_PEN);
        dc.SetBrush(wxBrush(wxGetApp().GetBgColour()));
        dc.DrawRectangle(BgRect());

        // FG square drawn on top.
        dc.SetBrush(wxBrush(wxGetApp().GetFgColour()));
        dc.DrawRectangle(FgRect());
    }

    void OnLeftDown(wxMouseEvent& e) {
        wxPoint pt = e.GetPosition();
        // FG is on top — test it first.
        if (FgRect().Contains(pt))      PickColour(true);
        else if (BgRect().Contains(pt)) PickColour(false);
    }

    void PickColour(bool fg) {
        wxColour cur = fg ? wxGetApp().GetFgColour() : wxGetApp().GetBgColour();
        wxColourData cd;
        cd.SetChooseFull(true);
        cd.SetColour(cur);
        wxColourDialog dlg(this, &cd);
        if (dlg.ShowModal() != wxID_OK) return;

        wxColour picked = dlg.GetColourData().GetColour();
        if (fg) wxGetApp().SetFgColour(picked);
        else    wxGetApp().SetBgColour(picked);

        ApplyColourToSelection(m_frame, picked, fg);
        Refresh();
    }

    MainFrame* m_frame;
};

// ---------------------------------------------------------------------------
// SwatchGridCtrl – scrolled grid of colour swatches from the active Palette.
// ---------------------------------------------------------------------------

class SwatchGridCtrl : public wxScrolledWindow {
public:
    static constexpr int kSwatchSz = 18;
    static constexpr int kGap      =  1;

    SwatchGridCtrl(wxWindow* parent, MainFrame* frame)
        : wxScrolledWindow(parent, wxID_ANY)
        , m_frame(frame)
    {
        SetBackgroundStyle(wxBG_STYLE_PAINT);
        Bind(wxEVT_PAINT,      &SwatchGridCtrl::OnPaint,     this);
        Bind(wxEVT_SIZE,       &SwatchGridCtrl::OnSize,      this);
        Bind(wxEVT_LEFT_DOWN,  &SwatchGridCtrl::OnLeftDown,  this);
        Bind(wxEVT_RIGHT_DOWN, &SwatchGridCtrl::OnRightDown, this);
    }

    void RefreshGrid() {
        RecomputeCols();
        UpdateScrollbars();
        Refresh();
    }

private:
    void RecomputeCols() {
        int w = GetClientSize().GetWidth();
        m_cols = std::max(1, w / (kSwatchSz + kGap));
    }

    void UpdateScrollbars() {
        const Palette& pal = wxGetApp().GetPalette();
        int n = (int)pal.entries.size();
        if (n == 0 || m_cols == 0) { SetScrollbars(1, 1, 0, 0); return; }
        int rows   = (n + m_cols - 1) / m_cols;
        int totalH = rows * (kSwatchSz + kGap);
        SetScrollbars(1, 1, 0, totalH);
    }

    int HitTest(const wxPoint& pt) const {
        int ux, uy;
        CalcUnscrolledPosition(pt.x, pt.y, &ux, &uy);
        if (m_cols == 0) return -1;
        int col = ux / (kSwatchSz + kGap);
        int row = uy / (kSwatchSz + kGap);
        int lx  = ux - col * (kSwatchSz + kGap);
        int ly  = uy - row * (kSwatchSz + kGap);
        if (lx >= kSwatchSz || ly >= kSwatchSz || col >= m_cols) return -1;
        int idx = row * m_cols + col;
        const Palette& pal = wxGetApp().GetPalette();
        return (idx < (int)pal.entries.size()) ? idx : -1;
    }

    void OnPaint(wxPaintEvent&) {
        wxAutoBufferedPaintDC dc(this);
        DoPrepareDC(dc);
        dc.SetBackground(wxBrush(GetBackgroundColour()));
        dc.Clear();
        if (m_cols == 0) return;

        const Palette& pal = wxGetApp().GetPalette();
        for (int i = 0; i < (int)pal.entries.size(); i++) {
            int col = i % m_cols;
            int row = i / m_cols;
            int x   = col * (kSwatchSz + kGap);
            int y   = row * (kSwatchSz + kGap);
            dc.SetPen(*wxBLACK_PEN);
            dc.SetBrush(wxBrush(pal.entries[i].colour));
            dc.DrawRectangle(x, y, kSwatchSz, kSwatchSz);
        }
    }

    void OnSize(wxSizeEvent& e) {
        RecomputeCols();
        UpdateScrollbars();
        Refresh();
        e.Skip();
    }

    void ApplySwatch(int idx, bool fg) {
        const Palette& pal = wxGetApp().GetPalette();
        if (idx < 0 || idx >= (int)pal.entries.size()) return;
        wxColour col = pal.entries[idx].colour;
        if (fg) wxGetApp().SetFgColour(col);
        else    wxGetApp().SetBgColour(col);

        ApplyColourToSelection(m_frame, col, fg);

        // Refresh indicator whether or not there was a selection.
        // (When there is a selection, UpdateAllViews also triggers this via
        // OnSelectionChanged, but we need it for the no-selection case too.)
        if (auto* panel = wxDynamicCast(GetParent(), ColorSwatchPanel))
            panel->UpdateColors();
    }

    void OnLeftDown(wxMouseEvent& e)  { ApplySwatch(HitTest(e.GetPosition()), true);  }
    void OnRightDown(wxMouseEvent& e) { ApplySwatch(HitTest(e.GetPosition()), false); }

    MainFrame* m_frame;
    int        m_cols{8};
};

// ---------------------------------------------------------------------------
// ColorSwatchPanel
// ---------------------------------------------------------------------------

ColorSwatchPanel::ColorSwatchPanel(wxWindow* parent, MainFrame* frame)
    : wxPanel(parent, wxID_ANY)
{
    m_indicator = new FgBgIndicatorCtrl(this, frame);
    m_grid      = new SwatchGridCtrl(this, frame);

    auto* sizer = new wxBoxSizer(wxVERTICAL);
    sizer->Add(m_indicator, 0, wxEXPAND | wxLEFT | wxRIGHT | wxTOP, 4);
    sizer->Add(m_grid,      1, wxEXPAND);
    SetSizer(sizer);
}

void ColorSwatchPanel::UpdateColors() {
    m_indicator->Refresh();
}

void ColorSwatchPanel::RefreshPalette() {
    m_grid->RefreshGrid();
    m_indicator->Refresh();
}
