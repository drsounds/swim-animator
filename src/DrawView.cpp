#include "DrawView.h"
#include "DrawDoc.h"
#include "DrawCommands.h"
#include "App.h"
#include "MainFrame.h"
#include "PropPanel.h"
#include <wx/dcbuffer.h>
#include <wx/settings.h>
#include <wx/colordlg.h>
#include <wx/textdlg.h>
#include <wx/msgdlg.h>
#include <wx/sizer.h>
#include <wx/stattext.h>
#include <wx/button.h>
#include <wx/textctrl.h>
#include <wx/spinctrl.h>
#include <algorithm>
#include <cmath>
#include <vector>

// ---------------------------------------------------------------------------
// Helpers
// ---------------------------------------------------------------------------

static wxRect NormalizedRect(const wxPoint& a, const wxPoint& b) {
    int x = a.x < b.x ? a.x : b.x;
    int y = a.y < b.y ? a.y : b.y;
    int w = a.x < b.x ? b.x - a.x : a.x - b.x;
    int h = a.y < b.y ? b.y - a.y : a.y - b.y;
    return wxRect(x, y, w, h);
}

// ---------------------------------------------------------------------------
// Rounded rectangle helper
// ---------------------------------------------------------------------------

// Samples a rounded rectangle with separate horizontal (rx) and vertical (ry)
// corner radii into a polygon. Each of the 4 corner arcs is sampled at
// CORNER_STEPS points; straight sides are implicit polygon edges between arcs.
static constexpr int CORNER_STEPS = 8;

static std::vector<wxPoint> BuildRoundedRectPolygon(const wxRect& r, int rx, int ry) {
    rx = std::min(rx, r.width  / 2);
    ry = std::min(ry, r.height / 2);

    std::vector<wxPoint> pts;
    pts.reserve(4 * CORNER_STEPS);

    // Each arc sweeps 90° around its corner centre. We sample CORNER_STEPS points
    // per arc (exclusive of the endpoint, which is the start of the next arc) so
    // there are no duplicate adjacent vertices in the resulting polygon.
    auto addArc = [&](float cx, float cy, float startDeg, float endDeg) {
        for (int i = 0; i < CORNER_STEPS; i++) {
            float a = (startDeg + (endDeg - startDeg) * i / CORNER_STEPS)
                      * (float)M_PI / 180.f;
            pts.push_back({ (int)(cx + rx * std::cos(a)),
                            (int)(cy + ry * std::sin(a)) });
        }
    };

    addArc(r.x + r.width  - rx, r.y          + ry, -90.f,   0.f); // top-right
    addArc(r.x + r.width  - rx, r.y + r.height - ry,  0.f,  90.f); // bottom-right
    addArc(r.x          + rx, r.y + r.height - ry, 90.f, 180.f); // bottom-left
    addArc(r.x          + rx, r.y          + ry, 180.f, 270.f); // top-left

    return pts;
}

// ---------------------------------------------------------------------------
// Bezier curve helpers
// ---------------------------------------------------------------------------

// Sample a cubic Bezier and draw it as polyline segments.
static void DrawBezierCurve(wxDC& dc, const wxPoint p[4]) {
    wxPoint prev = p[0];
    for (int i = 1; i <= 48; i++) {
        float t = i / 48.0f, mt = 1.0f - t;
        int x = (int)(mt*mt*mt*p[0].x + 3*mt*mt*t*p[1].x +
                      3*mt*t*t*p[2].x  + t*t*t*p[3].x);
        int y = (int)(mt*mt*mt*p[0].y + 3*mt*mt*t*p[1].y +
                      3*mt*t*t*p[2].y  + t*t*t*p[3].y);
        wxPoint cur(x, y);
        dc.DrawLine(prev, cur);
        prev = cur;
    }
}

// Bounding box of the 4 Bezier control points (conservative approximation).
static wxRect BezierBounds(const wxPoint p[4]) {
    int minX = p[0].x, maxX = p[0].x, minY = p[0].y, maxY = p[0].y;
    for (int i = 1; i < 4; i++) {
        minX = std::min(minX, p[i].x); maxX = std::max(maxX, p[i].x);
        minY = std::min(minY, p[i].y); maxY = std::max(maxY, p[i].y);
    }
    return wxRect(minX, minY, maxX - minX, maxY - minY);
}

// Hit-test the 4 Bezier control point handles; returns 0-3 or -1.
static int HitTestBezierPt(const wxPoint& pt, const wxPoint p[4]) {
    for (int i = 0; i < 4; i++) {
        int dx = pt.x - p[i].x, dy = pt.y - p[i].y;
        if (dx*dx + dy*dy <= 5*5) return i;
    }
    return -1;
}

// ---------------------------------------------------------------------------
// Handle helpers (selection handles for move/resize)
// ---------------------------------------------------------------------------

// Returns the 8 handle centres (NW, N, NE, E, SE, S, SW, W) for a shape rect.
static void GetHandleCentres(const wxRect& bounds, wxPoint out[8]) {
    wxRect sel = bounds;
    sel.Inflate(3);
    int cx = sel.x + sel.width  / 2;
    int cy = sel.y + sel.height / 2;
    out[0] = { sel.x,          sel.y          }; // NW
    out[1] = { cx,             sel.y          }; // N
    out[2] = { sel.GetRight(), sel.y          }; // NE
    out[3] = { sel.GetRight(), cy             }; // E
    out[4] = { sel.GetRight(), sel.GetBottom()}; // SE
    out[5] = { cx,             sel.GetBottom()}; // S
    out[6] = { sel.x,          sel.GetBottom()}; // SW
    out[7] = { sel.x,          cy             }; // W
}

static wxRect HandleRect(const wxPoint& c) {
    return wxRect(c.x - 3, c.y - 3, 7, 7);
}

// Hit-test the 8 handles; returns the matching DragMode or None.
static DrawCanvas::DragMode HitTestHandle(const wxPoint& pt, const wxRect& bounds) {
    using DM = DrawCanvas::DragMode;
    static const DM modes[8] = {
        DM::ResizeNW, DM::ResizeN, DM::ResizeNE, DM::ResizeE,
        DM::ResizeSE, DM::ResizeS, DM::ResizeSW, DM::ResizeW
    };
    wxPoint centres[8];
    GetHandleCentres(bounds, centres);
    for (int i = 0; i < 8; i++)
        if (HandleRect(centres[i]).Contains(pt))
            return modes[i];
    return DM::None;
}

// Apply a delta to a rect according to which handle is being dragged.
static wxRect ApplyDragMode(DrawCanvas::DragMode dm, const wxRect& orig,
                             int dx, int dy) {
    using DM = DrawCanvas::DragMode;
    wxRect r = orig;
    switch (dm) {
        case DM::Move:
            r.x += dx; r.y += dy;
            break;
        case DM::ResizeNW:
            r.x += dx; r.y += dy; r.width -= dx; r.height -= dy;
            break;
        case DM::ResizeN:
            r.y += dy; r.height -= dy;
            break;
        case DM::ResizeNE:
            r.y += dy; r.width += dx; r.height -= dy;
            break;
        case DM::ResizeE:
            r.width += dx;
            break;
        case DM::ResizeSE:
            r.width += dx; r.height += dy;
            break;
        case DM::ResizeS:
            r.height += dy;
            break;
        case DM::ResizeSW:
            r.x += dx; r.width -= dx; r.height += dy;
            break;
        case DM::ResizeW:
            r.x += dx; r.width -= dx;
            break;
        default:
            break;
    }
    r.width  = std::max(r.width,  4);
    r.height = std::max(r.height, 4);
    return r;
}

// ---------------------------------------------------------------------------
// ShapePropertiesDialog
// ---------------------------------------------------------------------------

class ShapePropertiesDialog : public wxDialog {
public:
    ShapePropertiesDialog(wxWindow* parent, const DrawShape& s)
        : wxDialog(parent, wxID_ANY, "Shape Properties",
                   wxDefaultPosition, wxDefaultSize, wxDEFAULT_DIALOG_STYLE)
        , m_fg(s.fgColour), m_bg(s.bgColour), m_label(s.label)
        , m_kind(s.kind)
    {
        auto* outer = new wxBoxSizer(wxVERTICAL);
        auto* grid  = new wxFlexGridSizer(2, wxSize(8, 6));
        grid->AddGrowableCol(1);

        // Label field (Text shapes only)
        if (s.kind == ShapeKind::Text) {
            grid->Add(new wxStaticText(this, wxID_ANY, "Label:"),
                      0, wxALIGN_CENTER_VERTICAL);
            m_labelCtrl = new wxTextCtrl(this, wxID_ANY, s.label);
            grid->Add(m_labelCtrl, 1, wxEXPAND);
        }

        // Foreground
        grid->Add(new wxStaticText(this, wxID_ANY, "Foreground:"),
                  0, wxALIGN_CENTER_VERTICAL);
        m_fgBtn = new wxButton(this, wxID_ANY, wxEmptyString,
                               wxDefaultPosition, wxSize(80, 22));
        m_fgBtn->SetBackgroundColour(m_fg);
        grid->Add(m_fgBtn, 0);

        // Background
        grid->Add(new wxStaticText(this, wxID_ANY, "Background:"),
                  0, wxALIGN_CENTER_VERTICAL);
        m_bgBtn = new wxButton(this, wxID_ANY, wxEmptyString,
                               wxDefaultPosition, wxSize(80, 22));
        m_bgBtn->SetBackgroundColour(m_bg);
        grid->Add(m_bgBtn, 0);

        // Border width (not for Text)
        if (s.kind != ShapeKind::Text) {
            grid->Add(new wxStaticText(this, wxID_ANY, "Border width:"),
                      0, wxALIGN_CENTER_VERTICAL);
            m_strokeSpin = new wxSpinCtrl(this, wxID_ANY, "",
                                          wxDefaultPosition, wxDefaultSize,
                                          wxSP_ARROW_KEYS, 0, 100, s.strokeWidth);
            grid->Add(m_strokeSpin, 1, wxEXPAND);
        }

        // Border radius X/Y (Rect only)
        if (s.kind == ShapeKind::Rect) {
            int maxRx = s.bounds.width  / 2;
            int maxRy = s.bounds.height / 2;
            grid->Add(new wxStaticText(this, wxID_ANY, "Radius X:"),
                      0, wxALIGN_CENTER_VERTICAL);
            m_radiusXSpin = new wxSpinCtrl(this, wxID_ANY, "",
                                           wxDefaultPosition, wxDefaultSize,
                                           wxSP_ARROW_KEYS, 0, maxRx, s.borderRadiusX);
            grid->Add(m_radiusXSpin, 1, wxEXPAND);

            grid->Add(new wxStaticText(this, wxID_ANY, "Radius Y:"),
                      0, wxALIGN_CENTER_VERTICAL);
            m_radiusYSpin = new wxSpinCtrl(this, wxID_ANY, "",
                                           wxDefaultPosition, wxDefaultSize,
                                           wxSP_ARROW_KEYS, 0, maxRy, s.borderRadiusY);
            grid->Add(m_radiusYSpin, 1, wxEXPAND);
        }

        outer->Add(grid, 1, wxEXPAND | wxALL, 10);
        outer->Add(CreateButtonSizer(wxOK | wxCANCEL), 0, wxEXPAND | wxLEFT | wxRIGHT | wxBOTTOM, 10);
        SetSizerAndFit(outer);

        m_fgBtn->Bind(wxEVT_BUTTON, [this](wxCommandEvent&) { PickColour(m_fg, m_fgBtn); });
        m_bgBtn->Bind(wxEVT_BUTTON, [this](wxCommandEvent&) { PickColour(m_bg, m_bgBtn); });
    }

    // Apply the dialog result back to a shape.
    void Apply(DrawShape& s) const {
        s.fgColour = m_fg;
        s.bgColour = m_bg;
        if (m_kind == ShapeKind::Text && m_labelCtrl)
            s.label = m_labelCtrl->GetValue();
        if (m_kind != ShapeKind::Text && m_strokeSpin)
            s.strokeWidth = m_strokeSpin->GetValue();
        if (m_kind == ShapeKind::Rect && m_radiusXSpin && m_radiusYSpin) {
            s.borderRadiusX = std::min(m_radiusXSpin->GetValue(), s.bounds.width  / 2);
            s.borderRadiusY = std::min(m_radiusYSpin->GetValue(), s.bounds.height / 2);
        }
    }

private:
    void PickColour(wxColour& colour, wxButton* btn) {
        wxColourData cd;
        cd.SetChooseFull(true);
        cd.SetColour(colour);
        wxColourDialog dlg(this, &cd);
        if (dlg.ShowModal() == wxID_OK) {
            colour = dlg.GetColourData().GetColour();
            btn->SetBackgroundColour(colour);
            btn->Refresh();
        }
    }

    wxColour    m_fg, m_bg;
    wxString    m_label;
    ShapeKind   m_kind;
    wxButton*   m_fgBtn      { nullptr };
    wxButton*   m_bgBtn      { nullptr };
    wxTextCtrl* m_labelCtrl  { nullptr };
    wxSpinCtrl* m_strokeSpin { nullptr };
    wxSpinCtrl* m_radiusXSpin{ nullptr };
    wxSpinCtrl* m_radiusYSpin{ nullptr };
};

// ---------------------------------------------------------------------------
// DrawCanvas – event table
// ---------------------------------------------------------------------------

wxBEGIN_EVENT_TABLE(DrawCanvas, wxPanel)
    EVT_PAINT(    DrawCanvas::OnPaint)
    EVT_SIZE(     DrawCanvas::OnSize)
    EVT_LEFT_DOWN(DrawCanvas::OnLeftDown)
    EVT_LEFT_UP(  DrawCanvas::OnLeftUp)
    EVT_MOTION(   DrawCanvas::OnMotion)
    EVT_LEFT_DCLICK(DrawCanvas::OnLeftDClick)
    EVT_KEY_DOWN( DrawCanvas::OnKeyDown)
wxEND_EVENT_TABLE()

DrawCanvas::DrawCanvas(DrawView* owner, wxWindow* parent)
    : wxPanel(parent, wxID_ANY, wxDefaultPosition, wxDefaultSize,
              wxFULL_REPAINT_ON_RESIZE | wxWANTS_CHARS)
    , m_owner(owner)
{
    SetBackgroundStyle(wxBG_STYLE_PAINT);
}

DrawDoc* DrawCanvas::GetDoc() {
    return m_owner ? wxDynamicCast(m_owner->GetDocument(), DrawDoc) : nullptr;
}

// ---------------------------------------------------------------------------
// DrawCanvas – rendering
// ---------------------------------------------------------------------------

void DrawCanvas::DrawShapeOnDC(wxDC& dc, const DrawShape& s, bool selected) {
    dc.SetPen(s.strokeWidth > 0 ? wxPen(s.fgColour, s.strokeWidth)
                                : *wxTRANSPARENT_PEN);
    dc.SetBrush(wxBrush(s.bgColour));

    switch (s.kind) {
        case ShapeKind::Rect:
            if (s.borderRadiusX > 0 || s.borderRadiusY > 0) {
                auto poly = BuildRoundedRectPolygon(s.bounds, s.borderRadiusX, s.borderRadiusY);
                dc.DrawPolygon((int)poly.size(), poly.data());
            } else {
                dc.DrawRectangle(s.bounds);
            }
            break;
        case ShapeKind::Circle:
            dc.DrawEllipse(s.bounds);
            break;
        case ShapeKind::Text:
            dc.SetPen(*wxTRANSPARENT_PEN);
            dc.DrawRectangle(s.bounds);
            dc.SetFont(wxSystemSettings::GetFont(wxSYS_DEFAULT_GUI_FONT));
            dc.SetTextForeground(s.fgColour);
            dc.SetTextBackground(s.bgColour);
            dc.DrawLabel(s.label, s.bounds, wxALIGN_CENTER);
            break;
        case ShapeKind::Bezier: {
            // Sample the curve into a polygon and fill, then draw the outline.
            const int N = 48;
            wxPoint poly[N + 1];
            for (int i = 0; i <= N; i++) {
                float t = i / (float)N, mt = 1.0f - t;
                poly[i].x = (int)(mt*mt*mt*s.pts[0].x + 3*mt*mt*t*s.pts[1].x +
                                  3*mt*t*t*s.pts[2].x  + t*t*t*s.pts[3].x);
                poly[i].y = (int)(mt*mt*mt*s.pts[0].y + 3*mt*mt*t*s.pts[1].y +
                                  3*mt*t*t*s.pts[2].y  + t*t*t*s.pts[3].y);
            }
            dc.SetBrush(wxBrush(s.bgColour));
            dc.SetPen(*wxTRANSPARENT_PEN);
            dc.DrawPolygon(N + 1, poly);
            if (s.strokeWidth > 0) {
                dc.SetPen(wxPen(s.fgColour, s.strokeWidth));
                DrawBezierCurve(dc, s.pts);
            }
            break;
        }
    }

    if (selected) {
        if (s.kind == ShapeKind::Bezier) {
            // Tangent handle lines
            dc.SetPen(wxPen(wxColour(100, 100, 100), 1, wxPENSTYLE_DOT));
            dc.DrawLine(s.pts[0], s.pts[1]);
            dc.DrawLine(s.pts[3], s.pts[2]);

            // Control point handles: squares for anchors, circles for handles
            dc.SetPen(*wxBLACK_PEN);
            dc.SetBrush(*wxWHITE_BRUSH);
            // Anchors (pt[0], pt[3]) — filled squares
            for (int i : {0, 3})
                dc.DrawRectangle(HandleRect(s.pts[i]));
            // Control points (pt[1], pt[2]) — filled circles
            dc.SetBrush(wxBrush(wxColour(180, 220, 255)));
            for (int i : {1, 2})
                dc.DrawEllipse(HandleRect(s.pts[i]));
        } else {
            // Dashed selection outline
            dc.SetPen(wxPen(*wxBLUE, 1, wxPENSTYLE_DOT));
            dc.SetBrush(*wxTRANSPARENT_BRUSH);
            wxRect sel = s.bounds;
            sel.Inflate(3);
            dc.DrawRectangle(sel);

            // Resize handles
            wxPoint centres[8];
            GetHandleCentres(s.bounds, centres);
            dc.SetPen(*wxBLACK_PEN);
            dc.SetBrush(*wxWHITE_BRUSH);
            for (int i = 0; i < 8; i++)
                dc.DrawRectangle(HandleRect(centres[i]));
        }
    }
}

void DrawCanvas::OnPaint(wxPaintEvent&) {
    wxAutoBufferedPaintDC dc(this);
    dc.SetBackground(*wxWHITE_BRUSH);
    dc.Clear();

    auto* doc = GetDoc();
    if (!doc) return;

    const auto& shapes = doc->GetShapes();
    for (int i = 0; i < (int)shapes.size(); i++) {
        // During a drag, render the live preview instead of the stored shape.
        const DrawShape& s = (m_dragMode != DragMode::None && i == m_selected)
                             ? m_dragPreview : shapes[i];
        DrawShapeOnDC(dc, s, i == m_selected);
    }

    // Rubber-band preview while drawing a new shape
    if (m_dragging && m_tool != DrawTool::Select && m_tool != DrawTool::Bezier) {
        wxRect r = NormalizedRect(m_dragStart, m_dragCurrent);
        if (r.width > 0 && r.height > 0) {
            dc.SetPen(wxPen(*wxBLACK, 1, wxPENSTYLE_DOT));
            dc.SetBrush(*wxTRANSPARENT_BRUSH);
            if (m_tool == DrawTool::Circle)
                dc.DrawEllipse(r);
            else
                dc.DrawRectangle(r);
        }
    }

    // Bezier creation preview
    if (m_tool == DrawTool::Bezier && m_bezierStep > 0) {
        // Fill in unset points with the current mouse position.
        wxPoint p[4];
        for (int i = 0; i < 4; i++)
            p[i] = (i < m_bezierStep) ? m_bezierPts[i] : m_dragCurrent;

        dc.SetPen(wxPen(*wxBLACK, 1, wxPENSTYLE_DOT));
        dc.SetBrush(*wxTRANSPARENT_BRUSH);
        DrawBezierCurve(dc, p);

        // Tangent handle lines
        dc.SetPen(wxPen(wxColour(130, 130, 130), 1, wxPENSTYLE_DOT));
        dc.DrawLine(p[0], p[1]);
        dc.DrawLine(p[3], p[2]);

        // Already-placed control points
        dc.SetPen(*wxBLACK_PEN);
        for (int i = 0; i < m_bezierStep; i++) {
            if (i == 0 || i == 3) {
                dc.SetBrush(*wxWHITE_BRUSH);
                dc.DrawRectangle(HandleRect(p[i]));
            } else {
                dc.SetBrush(wxBrush(wxColour(180, 220, 255)));
                dc.DrawEllipse(HandleRect(p[i]));
            }
        }
    }
}

void DrawCanvas::OnSize(wxSizeEvent& e) {
    Refresh();
    e.Skip();
}

// ---------------------------------------------------------------------------
// DrawCanvas – mouse & keyboard interaction
// ---------------------------------------------------------------------------

int DrawCanvas::HitTest(const wxPoint& pt) const {
    auto* doc = m_owner ? wxDynamicCast(m_owner->GetDocument(), DrawDoc) : nullptr;
    if (!doc) return -1;
    const auto& shapes = doc->GetShapes();
    // Test in reverse draw order so topmost shape wins.
    for (int i = (int)shapes.size() - 1; i >= 0; i--)
        if (shapes[i].HitTest(pt))
            return i;
    return -1;
}

void DrawCanvas::OnLeftDown(wxMouseEvent& e) {
    SetFocus();

    if (m_tool == DrawTool::Bezier) {
        // Place the next control point.
        m_bezierPts[m_bezierStep] = e.GetPosition();
        m_bezierStep++;

        if (m_bezierStep == 4) {
            // All 4 points placed — commit the shape.
            DrawShape s;
            s.kind     = ShapeKind::Bezier;
            s.fgColour = wxGetApp().GetFgColour();
            s.bgColour = wxGetApp().GetBgColour();
            for (int i = 0; i < 4; i++) s.pts[i] = m_bezierPts[i];
            s.bounds = BezierBounds(s.pts);
            auto* doc = GetDoc();
            if (doc) {
                doc->GetCommandProcessor()->Submit(new AddShapeCmd(doc, s));
                m_selected = static_cast<int>(doc->GetShapes().size()) - 1;
                m_owner->NotifySelectionChanged();
            }
            m_bezierStep = 0;
        }
        Refresh();
        e.Skip();
        return;
    }

    if (m_tool == DrawTool::Select) {
        wxPoint pt = e.GetPosition();
        auto* doc = GetDoc();

        // 1. Check control-point handles on a selected Bezier first.
        if (m_selected >= 0 && doc &&
            m_selected < (int)doc->GetShapes().size())
        {
            const DrawShape& sel = doc->GetShapes()[m_selected];
            if (sel.kind == ShapeKind::Bezier) {
                int hi = HitTestBezierPt(pt, sel.pts);
                if (hi >= 0) {
                    m_dragMode         = DragMode::BezierPt;
                    m_bezierHandleIdx  = hi;
                    m_dragOrigin       = pt;
                    for (int i = 0; i < 4; i++) m_dragOrigPts[i] = sel.pts[i];
                    m_dragOrigShape    = sel;
                    m_dragPreview      = sel;
                    CaptureMouse();
                    e.Skip();
                    return;
                }
            } else {
                // Check resize handles for non-Bezier shapes.
                DragMode dm = HitTestHandle(pt, sel.bounds);
                if (dm != DragMode::None) {
                    m_dragMode       = dm;
                    m_dragOrigin     = pt;
                    m_dragOrigBounds = sel.bounds;
                    m_dragOrigShape  = sel;
                    m_dragPreview    = sel;
                    CaptureMouse();
                    e.Skip();
                    return;
                }
            }
        }

        // 2. Hit-test all shapes.
        int hit = HitTest(pt);
        if (hit >= 0 && doc) {
            m_selected   = hit;
            const DrawShape& hs = doc->GetShapes()[hit];
            m_dragMode       = DragMode::Move;
            m_dragOrigin     = pt;
            m_dragOrigBounds = hs.bounds;
            for (int i = 0; i < 4; i++) m_dragOrigPts[i] = hs.pts[i];
            m_dragOrigShape  = hs;
            m_dragPreview    = hs;
            CaptureMouse();
        } else {
            m_selected = -1;
        }
        m_owner->NotifySelectionChanged();
        Refresh();
    } else {
        m_dragging    = true;
        m_dragStart   = e.GetPosition();
        m_dragCurrent = m_dragStart;
        CaptureMouse();
    }
    e.Skip();
}

void DrawCanvas::OnMotion(wxMouseEvent& e) {
    // Always track mouse for the Bezier creation preview.
    m_dragCurrent = e.GetPosition();

    if (m_tool == DrawTool::Bezier && m_bezierStep > 0) {
        Refresh();
    } else if (m_tool == DrawTool::Select && m_dragMode != DragMode::None) {
        wxPoint pt = e.GetPosition();
        int dx = pt.x - m_dragOrigin.x;
        int dy = pt.y - m_dragOrigin.y;

        // Update the local preview without touching the document.
        // The document is only committed once in OnLeftUp.
        m_dragPreview = m_dragOrigShape;
        if (m_dragMode == DragMode::BezierPt) {
            m_dragPreview.pts[m_bezierHandleIdx].x += dx;
            m_dragPreview.pts[m_bezierHandleIdx].y += dy;
            m_dragPreview.bounds = BezierBounds(m_dragPreview.pts);
        } else if (m_dragOrigShape.kind == ShapeKind::Bezier) {
            for (int i = 0; i < 4; i++) {
                m_dragPreview.pts[i].x += dx;
                m_dragPreview.pts[i].y += dy;
            }
            m_dragPreview.bounds = BezierBounds(m_dragPreview.pts);
        } else {
            m_dragPreview.bounds = ApplyDragMode(m_dragMode, m_dragOrigBounds, dx, dy);
        }
        Refresh();
    } else if (m_dragging) {
        Refresh();
    }
    e.Skip();
}

void DrawCanvas::OnLeftUp(wxMouseEvent& e) {
    // End a select-mode move/resize drag — commit the preview to the document.
    if (m_tool == DrawTool::Select && m_dragMode != DragMode::None) {
        auto* doc = GetDoc();
        if (doc && m_selected >= 0 && m_selected < (int)doc->GetShapes().size()) {
            if (m_dragPreview.bounds != m_dragOrigShape.bounds ||
                m_dragPreview.pts[0] != m_dragOrigShape.pts[0] ||
                m_dragPreview.pts[1] != m_dragOrigShape.pts[1] ||
                m_dragPreview.pts[2] != m_dragOrigShape.pts[2] ||
                m_dragPreview.pts[3] != m_dragOrigShape.pts[3])
            {
                wxString name = (m_dragMode == DragMode::Move) ? "Move Shape" : "Resize Shape";
                doc->GetCommandProcessor()->Submit(
                    new UpdateShapeCmd(doc, m_selected, m_dragOrigShape, m_dragPreview, name));
            }
        }
        m_dragMode = DragMode::None;
        if (HasCapture()) ReleaseMouse();
        e.Skip();
        return;
    }

    if (!m_dragging) { e.Skip(); return; }

    m_dragging = false;
    if (HasCapture()) ReleaseMouse();

    wxRect r = NormalizedRect(m_dragStart, e.GetPosition());
    if (r.width < 4 || r.height < 4) { Refresh(); return; } // too small

    DrawShape s;
    s.bounds   = r;
    s.fgColour = wxGetApp().GetFgColour();
    s.bgColour = wxGetApp().GetBgColour();

    switch (m_tool) {
        case DrawTool::Rect:   s.kind = ShapeKind::Rect;   break;
        case DrawTool::Circle: s.kind = ShapeKind::Circle; break;
        case DrawTool::Text: {
            wxTextEntryDialog dlg(this, "Label:", "Add Text");
            if (dlg.ShowModal() != wxID_OK) { Refresh(); return; }
            s.kind  = ShapeKind::Text;
            s.label = dlg.GetValue();
            break;
        }
        default: return;
    }

    auto* doc = GetDoc();
    if (doc) {
        doc->GetCommandProcessor()->Submit(new AddShapeCmd(doc, s));
        m_selected = static_cast<int>(doc->GetShapes().size()) - 1;
        m_owner->NotifySelectionChanged();
    }

    Refresh();
    e.Skip();
}

void DrawCanvas::OnLeftDClick(wxMouseEvent& e) {
    if (m_tool == DrawTool::Select && m_selected >= 0)
        OpenPropertiesDialog(m_selected);
    e.Skip();
}

void DrawCanvas::OnKeyDown(wxKeyEvent& e) {
    if (e.GetKeyCode() == WXK_ESCAPE && m_tool == DrawTool::Bezier && m_bezierStep > 0) {
        m_bezierStep = 0;
        Refresh();
    } else if (e.GetKeyCode() == WXK_DELETE && m_selected >= 0) {
        auto* doc = GetDoc();
        if (doc && m_selected < (int)doc->GetShapes().size()) {
            DrawShape shape = doc->GetShapes()[m_selected];
            int idx = m_selected;
            m_selected = -1;
            m_owner->NotifySelectionChanged();
            doc->GetCommandProcessor()->Submit(new RemoveShapeCmd(doc, idx, shape));
            Refresh();
        }
    } else {
        e.Skip();
    }
}

void DrawCanvas::OpenPropertiesDialog(int idx) {
    auto* doc = GetDoc();
    if (!doc) return;
    const auto& shapes = doc->GetShapes();
    if (idx < 0 || idx >= (int)shapes.size()) return;

    DrawShape before = shapes[idx];
    DrawShape after  = before;
    ShapePropertiesDialog dlg(this, after);
    if (dlg.ShowModal() == wxID_OK) {
        dlg.Apply(after);
        doc->GetCommandProcessor()->Submit(
            new UpdateShapeCmd(doc, idx, before, after, "Edit Properties"));
    }
}

void DrawCanvas::ValidateSelection() {
    auto* doc = GetDoc();
    if (!doc || m_selected >= (int)doc->GetShapes().size())
        m_selected = -1;
}

void DrawView::NotifySelectionChanged() {
    auto* mf = wxDynamicCast(wxGetApp().GetTopWindow(), MainFrame);
    if (!mf) return;
    DrawDoc* doc = m_canvas ? wxDynamicCast(GetDocument(), DrawDoc) : nullptr;
    int idx = m_canvas ? m_canvas->GetSelectedIndex() : -1;
    mf->OnSelectionChanged(doc, idx);
}

// ---------------------------------------------------------------------------
// DrawView
// ---------------------------------------------------------------------------

wxIMPLEMENT_DYNAMIC_CLASS(DrawView, wxView);

bool DrawView::OnCreate(wxDocument* doc, long flags) {
    if (!wxView::OnCreate(doc, flags)) return false;

    auto* mf = wxDynamicCast(wxGetApp().GetTopWindow(), MainFrame);
    wxCHECK_MSG(mf, false, "Top window is not a MainFrame");

    wxAuiNotebook* nb = mf->GetNotebook();
    wxCHECK_MSG(nb, false, "MainFrame has no AUI notebook");

    m_canvas = new DrawCanvas(this, nb);
    nb->AddPage(m_canvas, doc->GetUserReadableName(), /*select=*/true);

    Activate(true);
    return true;
}

void DrawView::OnDraw(wxDC* /*dc*/) {}

void DrawView::OnUpdate(wxView* /*sender*/, wxObject* /*hint*/) {
    if (m_canvas) {
        m_canvas->ValidateSelection();
        m_canvas->Refresh();
    }
    NotifySelectionChanged();
}

void DrawView::OnActivateView(bool activate, wxView*, wxView*) {
    auto* mf = wxDynamicCast(wxGetApp().GetTopWindow(), MainFrame);
    if (!mf) return;

    mf->SetActiveDrawView(activate ? this : nullptr);

    if (activate && m_canvas) {
        wxAuiNotebook* nb = mf->GetNotebook();
        if (nb) {
            int idx = nb->GetPageIndex(m_canvas);
            if (idx != wxNOT_FOUND && nb->GetSelection() != idx)
                nb->SetSelection(idx);
        }
    }
}

bool DrawView::OnClose(bool deleteWindow) {
    if (!wxView::OnClose(deleteWindow)) return false;

    // Always clear the properties pane for this document — even if this view
    // was not the active one — to prevent a dangling DrawDoc* in PropPanel.
    auto* mf = wxDynamicCast(wxGetApp().GetTopWindow(), MainFrame);
    if (mf) mf->OnSelectionChanged(nullptr, -1);

    Activate(false);

    if (deleteWindow && m_canvas) {
        auto* mf = wxDynamicCast(wxGetApp().GetTopWindow(), MainFrame);
        if (mf) {
            wxAuiNotebook* nb = mf->GetNotebook();
            if (nb) {
                int idx = nb->GetPageIndex(m_canvas);
                if (idx != wxNOT_FOUND)
                    nb->DeletePage(idx);
            }
        }
        m_canvas = nullptr;
    }
    return true;
}
