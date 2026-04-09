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
#include <wx/clipbrd.h>
#include <wx/xml/xml.h>
#include <wx/sstream.h>
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

static constexpr int CORNER_STEPS = 8;

static std::vector<wxPoint> BuildRoundedRectPolygon(const wxRect& r, int rx, int ry) {
    rx = std::min(rx, r.width  / 2);
    ry = std::min(ry, r.height / 2);

    std::vector<wxPoint> pts;
    pts.reserve(4 * CORNER_STEPS);

    auto addArc = [&](float cx, float cy, float startDeg, float endDeg) {
        for (int i = 0; i < CORNER_STEPS; i++) {
            float a = (startDeg + (endDeg - startDeg) * i / CORNER_STEPS)
                      * (float)M_PI / 180.f;
            pts.push_back({ (int)(cx + rx * std::cos(a)),
                            (int)(cy + ry * std::sin(a)) });
        }
    };

    addArc(r.x + r.width  - rx, r.y          + ry, -90.f,   0.f);
    addArc(r.x + r.width  - rx, r.y + r.height - ry,  0.f,  90.f);
    addArc(r.x          + rx, r.y + r.height - ry, 90.f, 180.f);
    addArc(r.x          + rx, r.y          + ry, 180.f, 270.f);

    return pts;
}

// ---------------------------------------------------------------------------
// Bezier curve helpers
// ---------------------------------------------------------------------------

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

static wxRect BezierBounds(const wxPoint p[4]) {
    int minX = p[0].x, maxX = p[0].x, minY = p[0].y, maxY = p[0].y;
    for (int i = 1; i < 4; i++) {
        minX = std::min(minX, p[i].x); maxX = std::max(maxX, p[i].x);
        minY = std::min(minY, p[i].y); maxY = std::max(maxY, p[i].y);
    }
    return wxRect(minX, minY, maxX - minX, maxY - minY);
}

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

        if (s.kind == ShapeKind::Text) {
            grid->Add(new wxStaticText(this, wxID_ANY, "Label:"),
                      0, wxALIGN_CENTER_VERTICAL);
            m_labelCtrl = new wxTextCtrl(this, wxID_ANY, s.label);
            grid->Add(m_labelCtrl, 1, wxEXPAND);
        }

        grid->Add(new wxStaticText(this, wxID_ANY, "Foreground:"),
                  0, wxALIGN_CENTER_VERTICAL);
        m_fgBtn = new wxButton(this, wxID_ANY, wxEmptyString,
                               wxDefaultPosition, wxSize(80, 22));
        m_fgBtn->SetBackgroundColour(m_fg);
        grid->Add(m_fgBtn, 0);

        grid->Add(new wxStaticText(this, wxID_ANY, "Background:"),
                  0, wxALIGN_CENTER_VERTICAL);
        m_bgBtn = new wxButton(this, wxID_ANY, wxEmptyString,
                               wxDefaultPosition, wxSize(80, 22));
        m_bgBtn->SetBackgroundColour(m_bg);
        grid->Add(m_bgBtn, 0);

        if (s.kind != ShapeKind::Text) {
            grid->Add(new wxStaticText(this, wxID_ANY, "Border width:"),
                      0, wxALIGN_CENTER_VERTICAL);
            m_strokeSpin = new wxSpinCtrl(this, wxID_ANY, "",
                                          wxDefaultPosition, wxDefaultSize,
                                          wxSP_ARROW_KEYS, 0, 100, s.strokeWidth);
            grid->Add(m_strokeSpin, 1, wxEXPAND);
        }

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
    EVT_PAINT(      DrawCanvas::OnPaint)
    EVT_SIZE(       DrawCanvas::OnSize)
    EVT_LEFT_DOWN(  DrawCanvas::OnLeftDown)
    EVT_LEFT_UP(    DrawCanvas::OnLeftUp)
    EVT_MOTION(     DrawCanvas::OnMotion)
    EVT_MIDDLE_DOWN(DrawCanvas::OnMiddleDown)
    EVT_MIDDLE_UP(  DrawCanvas::OnMiddleUp)
    EVT_MOUSEWHEEL( DrawCanvas::OnMouseWheel)
    EVT_LEFT_DCLICK(   DrawCanvas::OnLeftDClick)
    EVT_KEY_DOWN(      DrawCanvas::OnKeyDown)
    EVT_CONTEXT_MENU(  DrawCanvas::OnContextMenu)
    EVT_MENU(ID_CTX_DELETE,     DrawCanvas::OnCtxDelete)
    EVT_MENU(ID_CTX_GROUP,      DrawCanvas::OnCtxGroup)
    EVT_MENU(ID_CTX_UNGROUP,    DrawCanvas::OnCtxUngroup)
    EVT_MENU(ID_CTX_PROPERTIES, DrawCanvas::OnCtxProperties)
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
// Colour dimming for isolation mode
// ---------------------------------------------------------------------------

static wxColour DimColour(const wxColour& c) {
    // Blend 75% toward light grey (#C0C0C0) to make out-of-scope shapes recede.
    return wxColour(
        (unsigned char)((c.Red()   + 192 * 3) / 4),
        (unsigned char)((c.Green() + 192 * 3) / 4),
        (unsigned char)((c.Blue()  + 192 * 3) / 4));
}

static DrawShape DimShape(DrawShape s) {
    s.fgColour = DimColour(s.fgColour);
    s.bgColour = DimColour(s.bgColour);
    return s;
}

// ---------------------------------------------------------------------------
// Selection helpers
// ---------------------------------------------------------------------------

void DrawCanvas::ClearSelection() {
    m_selection.clear();
    m_selected = -1;
}

void DrawCanvas::SetSingleSelection(int idx) {
    m_selection = { idx };
    m_selected  = idx;
}

const std::vector<DrawShape>* DrawCanvas::CurrentShapes(DrawDoc* doc) const {
    if (!doc) return nullptr;
    if (m_activeGroupIdx >= 0 &&
        m_activeGroupIdx < (int)doc->GetShapes().size() &&
        doc->GetShapes()[m_activeGroupIdx].kind == ShapeKind::Group)
    {
        return &doc->GetShapes()[m_activeGroupIdx].children;
    }
    return &doc->GetShapes();
}

ShapePath DrawCanvas::CurrentPath(int idx) const {
    if (m_activeGroupIdx >= 0)
        return { m_activeGroupIdx, idx };
    return { idx };
}

// ---------------------------------------------------------------------------
// DrawCanvas – rendering
// ---------------------------------------------------------------------------

// Draw a single leaf shape (not Group/SVGRef).
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
        case ShapeKind::Group:
            // Draw children recursively; selection overlay drawn below.
            for (const DrawShape& child : s.children)
                DrawShapeOnDCRecursive(dc, child, false);
            break;
        case ShapeKind::SVGRef:
            // Placeholder: draw a hatched rectangle until SVGRef is parsed.
            dc.SetBrush(wxBrush(wxColour(220, 220, 255)));
            dc.SetPen(wxPen(wxColour(100, 100, 200), 1));
            dc.DrawRectangle(s.bounds);
            dc.DrawLine(s.bounds.GetTopLeft(),     s.bounds.GetBottomRight());
            dc.DrawLine(s.bounds.GetTopRight(),    s.bounds.GetBottomLeft());
            break;
    }

    if (selected) {
        if (s.kind == ShapeKind::Bezier) {
            dc.SetPen(wxPen(wxColour(100, 100, 100), 1, wxPENSTYLE_DOT));
            dc.DrawLine(s.pts[0], s.pts[1]);
            dc.DrawLine(s.pts[3], s.pts[2]);

            dc.SetPen(*wxBLACK_PEN);
            dc.SetBrush(*wxWHITE_BRUSH);
            for (int i : {0, 3})
                dc.DrawRectangle(HandleRect(s.pts[i]));
            dc.SetBrush(wxBrush(wxColour(180, 220, 255)));
            for (int i : {1, 2})
                dc.DrawEllipse(HandleRect(s.pts[i]));
        } else {
            dc.SetPen(wxPen(*wxBLUE, 1, wxPENSTYLE_DOT));
            dc.SetBrush(*wxTRANSPARENT_BRUSH);
            wxRect sel = s.bounds;
            sel.Inflate(3);
            dc.DrawRectangle(sel);

            wxPoint centres[8];
            GetHandleCentres(s.bounds, centres);
            dc.SetPen(*wxBLACK_PEN);
            dc.SetBrush(*wxWHITE_BRUSH);
            for (int i = 0; i < 8; i++)
                dc.DrawRectangle(HandleRect(centres[i]));
        }
    }
}

// Draw a shape recursively (used for Group children — no outer selection markup).
void DrawCanvas::DrawShapeOnDCRecursive(wxDC& dc, const DrawShape& s, bool selected) {
    DrawShapeOnDC(dc, s, selected);
}

// ---------------------------------------------------------------------------
// Viewport coordinate helpers
// ---------------------------------------------------------------------------

wxPoint DrawCanvas::ScreenToDoc(wxPoint pt) const {
    return wxPoint(
        (int)std::round((pt.x - m_viewOffset.x) / m_zoom),
        (int)std::round((pt.y - m_viewOffset.y) / m_zoom));
}

wxPoint DrawCanvas::DocToScreen(wxPoint pt) const {
    return wxPoint(
        (int)std::round(pt.x * m_zoom + m_viewOffset.x),
        (int)std::round(pt.y * m_zoom + m_viewOffset.y));
}

void DrawCanvas::OnPaint(wxPaintEvent&) {
    wxAutoBufferedPaintDC dc(this);

    dc.SetBackground(wxBrush(wxColour(160, 160, 160)));
    dc.Clear();

    auto* doc = GetDoc();
    if (!doc) return;

    dc.SetDeviceOrigin(m_viewOffset.x, m_viewOffset.y);
    dc.SetUserScale(m_zoom, m_zoom);

    // Page background + contrast border.
    wxRect pageRect(0, 0, doc->GetPageWidth(), doc->GetPageHeight());
    const wxColour& bg = doc->GetBgColour();
    int lum = (299 * bg.Red() + 587 * bg.Green() + 114 * bg.Blue()) / 1000;
    wxColour borderColour = (lum > 128) ? wxColour(0, 0, 0) : wxColour(255, 255, 255);

    dc.SetBrush(wxBrush(bg));
    dc.SetPen(*wxTRANSPARENT_PEN);
    dc.DrawRectangle(pageRect);

    dc.SetBrush(*wxTRANSPARENT_BRUSH);
    dc.SetPen(wxPen(borderColour, 1));
    dc.DrawRectangle(pageRect.x, pageRect.y, pageRect.width - 1, pageRect.height - 1);

    const auto& shapes = doc->GetShapes();

    auto isSelected = [&](int i) -> bool {
        return std::find(m_selection.begin(), m_selection.end(), i) != m_selection.end();
    };

    if (m_activeGroupIdx >= 0 &&
        m_activeGroupIdx < (int)shapes.size() &&
        shapes[m_activeGroupIdx].kind == ShapeKind::Group)
    {
        // --- Isolation mode ---
        // 1. Draw all root shapes EXCEPT the active group, dimmed.
        for (int i = 0; i < (int)shapes.size(); i++) {
            if (i == m_activeGroupIdx) continue;
            DrawShapeOnDC(dc, DimShape(shapes[i]), false);
        }

        // 2. Draw a dashed bounding box around the active group's bounds.
        const DrawShape& grp = shapes[m_activeGroupIdx];
        dc.SetPen(wxPen(wxColour(0, 100, 200), 1, wxPENSTYLE_DOT));
        dc.SetBrush(*wxTRANSPARENT_BRUSH);
        wxRect grpRect = grp.bounds;
        grpRect.Inflate(4);
        dc.DrawRectangle(grpRect);

        // 3. Draw the group's children at normal colors.
        const auto& children = grp.children;
        for (int i = 0; i < (int)children.size(); i++) {
            bool sel = isSelected(i);
            const DrawShape& child = children[i];

            if (m_isMultiDrag && sel) {
                auto it = std::find(m_selection.begin(), m_selection.end(), i);
                DrawShape preview = m_multiDragShapes[it - m_selection.begin()];
                preview.bounds.x += m_multiDragDelta.x;
                preview.bounds.y += m_multiDragDelta.y;
                if (preview.kind == ShapeKind::Bezier) {
                    for (int k = 0; k < 4; k++) {
                        preview.pts[k].x += m_multiDragDelta.x;
                        preview.pts[k].y += m_multiDragDelta.y;
                    }
                    preview.bounds = BezierBounds(preview.pts);
                }
                DrawShapeOnDC(dc, preview, sel);
            } else if (m_dragMode != DragMode::None && i == m_selected) {
                DrawShapeOnDC(dc, m_dragPreview, true);
            } else {
                DrawShapeOnDC(dc, child, sel);
            }
        }
    } else {
        // --- Normal mode ---
        for (int i = 0; i < (int)shapes.size(); i++) {
            bool sel = isSelected(i);

            if (m_isMultiDrag && sel) {
                auto it = std::find(m_selection.begin(), m_selection.end(), i);
                DrawShape preview = m_multiDragShapes[it - m_selection.begin()];
                preview.bounds.x += m_multiDragDelta.x;
                preview.bounds.y += m_multiDragDelta.y;
                if (preview.kind == ShapeKind::Bezier) {
                    for (int k = 0; k < 4; k++) {
                        preview.pts[k].x += m_multiDragDelta.x;
                        preview.pts[k].y += m_multiDragDelta.y;
                    }
                    preview.bounds = BezierBounds(preview.pts);
                }
                DrawShapeOnDC(dc, preview, sel);
            } else if (m_dragMode != DragMode::None && i == m_selected) {
                DrawShapeOnDC(dc, m_dragPreview, true);
            } else {
                DrawShapeOnDC(dc, shapes[i], sel);
            }
        }
    }

    // New-shape rubber-band preview (creation tools).
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

    // Selection rubber-band.
    if (m_rubberBanding) {
        wxRect rb = NormalizedRect(m_rubberBandStart, m_rubberBandCurrent);
        dc.SetPen(wxPen(wxColour(0, 120, 215), 1, wxPENSTYLE_DOT));
        dc.SetBrush(wxBrush(wxColour(0, 120, 215, 40)));
        dc.DrawRectangle(rb);
    }

    // Bezier creation preview.
    if (m_tool == DrawTool::Bezier && m_bezierStep > 0) {
        wxPoint p[4];
        for (int i = 0; i < 4; i++)
            p[i] = (i < m_bezierStep) ? m_bezierPts[i] : m_dragCurrent;

        dc.SetPen(wxPen(*wxBLACK, 1, wxPENSTYLE_DOT));
        dc.SetBrush(*wxTRANSPARENT_BRUSH);
        DrawBezierCurve(dc, p);

        dc.SetPen(wxPen(wxColour(130, 130, 130), 1, wxPENSTYLE_DOT));
        dc.DrawLine(p[0], p[1]);
        dc.DrawLine(p[3], p[2]);

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

    const std::vector<DrawShape>* scope = CurrentShapes(doc);
    if (!scope) return -1;

    for (int i = (int)scope->size() - 1; i >= 0; i--)
        if ((*scope)[i].HitTest(pt))
            return i;
    return -1;
}

void DrawCanvas::OnLeftDown(wxMouseEvent& e) {
    SetFocus();

    if (m_tool == DrawTool::Bezier) {
        m_bezierPts[m_bezierStep] = ScreenToDoc(e.GetPosition());
        m_bezierStep++;

        if (m_bezierStep == 4) {
            DrawShape s;
            s.kind     = ShapeKind::Bezier;
            s.fgColour = wxGetApp().GetFgColour();
            s.bgColour = wxGetApp().GetBgColour();
            for (int i = 0; i < 4; i++) s.pts[i] = m_bezierPts[i];
            s.bounds = BezierBounds(s.pts);
            auto* doc = GetDoc();
            if (doc) {
                const std::vector<DrawShape>* scope = CurrentShapes(doc);
                int newIdx = scope ? (int)scope->size() : 0;
                ShapePath parentPath = m_activeGroupIdx >= 0
                    ? ShapePath{ m_activeGroupIdx }
                    : ShapePath{};
                doc->GetCommandProcessor()->Submit(
                    new InsertShapeAtCmd(doc, parentPath, newIdx, s));
                SetSingleSelection(newIdx);
                m_owner->NotifySelectionChanged();
            }
            m_bezierStep = 0;
            SetTool(DrawTool::Select);  // revert to pointer after creation
        }
        Refresh();
        e.Skip();
        return;
    }

    if (m_tool == DrawTool::Select) {
        wxPoint pt = ScreenToDoc(e.GetPosition());
        auto* doc = GetDoc();
        bool ctrlHeld = e.ControlDown() || e.CmdDown();

        // --- Single-selection drag handles (only when exactly one shape selected) ---
        if (!ctrlHeld && m_selection.size() == 1 && doc) {
            int si = m_selection[0];
            const std::vector<DrawShape>* scope = CurrentShapes(doc);
            if (scope && si >= 0 && si < (int)scope->size()) {
                const DrawShape& sel = (*scope)[si];

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
                } else if (sel.kind != ShapeKind::Group) {
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
        }

        // --- Hit-test shapes ---
        int hit = HitTest(pt);

        if (hit >= 0 && doc) {
            if (ctrlHeld) {
                // Toggle this shape in the selection.
                auto it = std::find(m_selection.begin(), m_selection.end(), hit);
                if (it != m_selection.end()) {
                    m_selection.erase(it);
                    m_selected = m_selection.empty() ? -1 : m_selection.back();
                } else {
                    m_selection.push_back(hit);
                    m_selected = hit;
                }
                m_owner->NotifySelectionChanged();
                Refresh();
                e.Skip();
                return;
            }

            // Not Ctrl: if the hit shape is NOT already in selection, replace selection.
            bool alreadySelected = std::find(m_selection.begin(), m_selection.end(), hit)
                                   != m_selection.end();
            if (!alreadySelected)
                SetSingleSelection(hit);
            else
                m_selected = hit;

            if (m_selection.size() == 1) {
                // Single-shape move drag.
                const std::vector<DrawShape>* scope = CurrentShapes(doc);
                if (!scope || hit >= (int)scope->size()) { e.Skip(); return; }
                const DrawShape& hs = (*scope)[hit];
                m_dragMode       = DragMode::Move;
                m_dragOrigin     = pt;
                m_dragOrigBounds = hs.bounds;
                for (int i = 0; i < 4; i++) m_dragOrigPts[i] = hs.pts[i];
                m_dragOrigShape  = hs;
                m_dragPreview    = hs;
            } else {
                // Multi-shape move drag.
                m_isMultiDrag     = true;
                m_multiDragOrigin = pt;
                m_multiDragDelta  = {0, 0};
                m_multiDragShapes.clear();
                const std::vector<DrawShape>* scope = CurrentShapes(doc);
                for (int idx : m_selection)
                    m_multiDragShapes.push_back((*scope)[idx]);
            }
            CaptureMouse();
        } else if (!ctrlHeld) {
            // No hit, no Ctrl: start rubber-band selection, clear existing selection.
            ClearSelection();
            m_rubberBanding    = true;
            m_rubberBandStart   = pt;
            m_rubberBandCurrent = pt;
            CaptureMouse();
        } else {
            // No hit, Ctrl: start rubber-band that ADDS to selection.
            m_rubberBanding    = true;
            m_rubberBandStart   = pt;
            m_rubberBandCurrent = pt;
            CaptureMouse();
        }

        m_owner->NotifySelectionChanged();
        Refresh();
    } else {
        m_dragging    = true;
        m_dragStart   = ScreenToDoc(e.GetPosition());
        m_dragCurrent = m_dragStart;
        CaptureMouse();
    }
    e.Skip();
}

void DrawCanvas::OnMotion(wxMouseEvent& e) {
    m_dragCurrent = ScreenToDoc(e.GetPosition());

    // Pan mode.
    if (m_panning) {
        if (!e.MiddleIsDown()) {
            m_panning = false;
            if (HasCapture()) ReleaseMouse();
            SetCursor(wxNullCursor);
        } else {
            wxPoint delta = e.GetPosition() - m_panStart;
            m_viewOffset = m_panStartOffset + delta;
            Refresh();
        }
        e.Skip();
        return;
    }

    if (m_rubberBanding) {
        m_rubberBandCurrent = ScreenToDoc(e.GetPosition());
        Refresh();
        e.Skip();
        return;
    }

    if (m_tool == DrawTool::Bezier && m_bezierStep > 0) {
        Refresh();
    } else if (m_tool == DrawTool::Select) {
        if (m_isMultiDrag) {
            wxPoint pt = ScreenToDoc(e.GetPosition());
            m_multiDragDelta = pt - m_multiDragOrigin;
            Refresh();
        } else if (m_dragMode != DragMode::None) {
            wxPoint pt = ScreenToDoc(e.GetPosition());
            int dx = pt.x - m_dragOrigin.x;
            int dy = pt.y - m_dragOrigin.y;

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
    }
    e.Skip();
}

void DrawCanvas::OnLeftUp(wxMouseEvent& e) {
    // Finalize rubber-band selection.
    if (m_rubberBanding) {
        m_rubberBanding = false;
        if (HasCapture()) ReleaseMouse();

        wxRect rb = NormalizedRect(m_rubberBandStart, ScreenToDoc(e.GetPosition()));
        if (rb.width > 2 && rb.height > 2) {
            auto* doc = GetDoc();
            bool ctrlHeld = e.ControlDown() || e.CmdDown();
            if (!ctrlHeld) m_selection.clear();

            if (doc) {
                const std::vector<DrawShape>* scope = CurrentShapes(doc);
                if (scope) {
                    for (int i = 0; i < (int)scope->size(); i++) {
                        if (rb.Intersects((*scope)[i].bounds)) {
                            if (std::find(m_selection.begin(), m_selection.end(), i)
                                == m_selection.end())
                                m_selection.push_back(i);
                        }
                    }
                }
            }
            m_selected = m_selection.empty() ? -1 : m_selection.back();
        }

        m_owner->NotifySelectionChanged();
        Refresh();
        e.Skip();
        return;
    }

    // Finalize multi-shape drag.
    if (m_isMultiDrag) {
        m_isMultiDrag = false;
        if (HasCapture()) ReleaseMouse();

        auto* doc = GetDoc();
        if (doc && (m_multiDragDelta.x != 0 || m_multiDragDelta.y != 0)) {
            auto* batch = new BatchCmd("Move Shapes");
            for (int i = 0; i < (int)m_selection.size(); i++) {
                int idx = m_selection[i];
                DrawShape before = m_multiDragShapes[i];
                DrawShape after  = before;
                after.bounds.x += m_multiDragDelta.x;
                after.bounds.y += m_multiDragDelta.y;
                if (after.kind == ShapeKind::Bezier) {
                    for (int k = 0; k < 4; k++) {
                        after.pts[k].x += m_multiDragDelta.x;
                        after.pts[k].y += m_multiDragDelta.y;
                    }
                    after.bounds = BezierBounds(after.pts);
                }
                ShapePath path = CurrentPath(idx);
                batch->Add(new UpdateShapeAtCmd(doc, path, before, after, "Move Shape"));
            }
            doc->GetCommandProcessor()->Submit(batch);
        }
        Refresh();
        e.Skip();
        return;
    }

    // Finalize single-shape move/resize drag.
    if (m_tool == DrawTool::Select && m_dragMode != DragMode::None) {
        auto* doc = GetDoc();
        const std::vector<DrawShape>* scope = CurrentShapes(doc);
        if (doc && scope && m_selected >= 0 && m_selected < (int)scope->size()) {
            if (m_dragPreview.bounds != m_dragOrigShape.bounds ||
                m_dragPreview.pts[0] != m_dragOrigShape.pts[0] ||
                m_dragPreview.pts[1] != m_dragOrigShape.pts[1] ||
                m_dragPreview.pts[2] != m_dragOrigShape.pts[2] ||
                m_dragPreview.pts[3] != m_dragOrigShape.pts[3])
            {
                wxString name = (m_dragMode == DragMode::Move) ? "Move Shape" : "Resize Shape";
                ShapePath path = CurrentPath(m_selected);
                doc->GetCommandProcessor()->Submit(
                    new UpdateShapeAtCmd(doc, path, m_dragOrigShape, m_dragPreview, name));
            }
        }
        m_dragMode = DragMode::None;
        if (HasCapture()) ReleaseMouse();
        e.Skip();
        return;
    }

    // Finalize new-shape creation drag.
    if (!m_dragging) { e.Skip(); return; }

    m_dragging = false;
    if (HasCapture()) ReleaseMouse();

    wxRect r = NormalizedRect(m_dragStart, ScreenToDoc(e.GetPosition()));
    if (r.width < 4 || r.height < 4) { Refresh(); return; }

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
        const std::vector<DrawShape>* scope = CurrentShapes(doc);
        int newIdx = scope ? (int)scope->size() : 0;
        ShapePath parentPath = m_activeGroupIdx >= 0
            ? ShapePath{ m_activeGroupIdx }
            : ShapePath{};
        doc->GetCommandProcessor()->Submit(
            new InsertShapeAtCmd(doc, parentPath, newIdx, s));
        SetSingleSelection(newIdx);
        m_owner->NotifySelectionChanged();
        SetTool(DrawTool::Select);  // revert to pointer after creation
    }

    Refresh();
    e.Skip();
}

void DrawCanvas::OnLeftDClick(wxMouseEvent& e) {
    if (m_tool != DrawTool::Select) { e.Skip(); return; }

    auto* doc = GetDoc();

    // Double-click on a Group in normal mode → enter isolation mode.
    if (m_activeGroupIdx < 0 && m_selected >= 0 && doc) {
        const auto& shapes = doc->GetShapes();
        if (m_selected < (int)shapes.size() &&
            shapes[m_selected].kind == ShapeKind::Group)
        {
            m_activeGroupIdx = m_selected;
            ClearSelection();
            m_owner->NotifySelectionChanged();
            Refresh();
            e.Skip();
            return;
        }
    }

    // In isolation mode, double-click on a child Group → enter sub-group.
    if (m_activeGroupIdx >= 0 && m_selected >= 0 && doc) {
        const std::vector<DrawShape>* scope = CurrentShapes(doc);
        if (scope && m_selected < (int)scope->size() &&
            (*scope)[m_selected].kind == ShapeKind::Group)
        {
            // Only one level of isolation supported (Phase 4).
            // For now, open properties dialog instead.
        }
    }

    // Normal double-click → properties dialog.
    if (m_selected >= 0)
        OpenPropertiesDialog(m_selected);

    e.Skip();
}

void DrawCanvas::OnKeyDown(wxKeyEvent& e) {
    auto* doc = GetDoc();

    if (e.GetKeyCode() == WXK_ESCAPE) {
        if (m_activeGroupIdx >= 0) {
            // Exit group isolation mode.
            m_activeGroupIdx = -1;
            ClearSelection();
            m_owner->NotifySelectionChanged();
            Refresh();
        } else if (m_tool == DrawTool::Bezier && m_bezierStep > 0) {
            m_bezierStep = 0;
            Refresh();
        }
        e.Skip();
        return;
    }

    if (e.GetKeyCode() == WXK_DELETE && !m_selection.empty() && doc) {
        const std::vector<DrawShape>* scope = CurrentShapes(doc);
        if (!scope) { e.Skip(); return; }

        std::vector<int> sorted = m_selection;
        std::sort(sorted.begin(), sorted.end(), std::greater<int>());

        if (sorted.size() == 1) {
            int idx = sorted[0];
            DrawShape shape = (*scope)[idx];
            ShapePath path = CurrentPath(idx);
            ClearSelection();
            m_owner->NotifySelectionChanged();
            doc->GetCommandProcessor()->Submit(new RemoveShapeAtCmd(doc, path, shape));
        } else {
            auto* batch = new BatchCmd("Delete Shapes");
            for (int idx : sorted) {
                DrawShape shape = (*scope)[idx];
                ShapePath path = CurrentPath(idx);
                batch->Add(new RemoveShapeAtCmd(doc, path, shape));
            }
            ClearSelection();
            m_owner->NotifySelectionChanged();
            doc->GetCommandProcessor()->Submit(batch);
        }
        Refresh();
        e.Skip();
        return;
    }

    // Ctrl+G – group selected shapes (only in normal mode, root level)
    if (e.GetKeyCode() == 'G' && (e.ControlDown() || e.CmdDown()) &&
        !e.ShiftDown() && m_selection.size() >= 2 && doc &&
        m_activeGroupIdx < 0)
    {
        // Only root-level indices supported.
        std::vector<int> indices = m_selection;
        std::sort(indices.begin(), indices.end());

        auto* cmd = new GroupShapesCmd(doc, indices);
        ClearSelection();
        doc->GetCommandProcessor()->Submit(cmd);
        // Select the newly created group.
        SetSingleSelection(cmd->GetGroupPath()[0]);
        m_owner->NotifySelectionChanged();
        Refresh();
        e.Skip();
        return;
    }

    // Ctrl+Shift+G – ungroup selected group (only in normal mode, root level)
    if (e.GetKeyCode() == 'G' && (e.ControlDown() || e.CmdDown()) &&
        e.ShiftDown() && m_selection.size() == 1 && doc &&
        m_activeGroupIdx < 0)
    {
        int idx = m_selection[0];
        const auto& shapes = doc->GetShapes();
        if (idx >= 0 && idx < (int)shapes.size() &&
            shapes[idx].kind == ShapeKind::Group)
        {
            DrawShape group = shapes[idx];
            int childCount = (int)group.children.size();
            ClearSelection();
            doc->GetCommandProcessor()->Submit(
                new UngroupCmd(doc, ShapePath{idx}, group));
            // Select the ungrouped children.
            for (int i = idx; i < idx + childCount; i++)
                m_selection.push_back(i);
            m_selected = m_selection.empty() ? -1 : m_selection.back();
            m_owner->NotifySelectionChanged();
            Refresh();
        }
        e.Skip();
        return;
    }

    e.Skip();
}

// ---------------------------------------------------------------------------
// Context menu
// ---------------------------------------------------------------------------

void DrawCanvas::OnContextMenu(wxContextMenuEvent& e) {
    auto* doc = GetDoc();
    if (!doc) { e.Skip(); return; }

    // If the click landed on a shape not yet selected, select it first.
    wxPoint screenPt = e.GetPosition();
    if (screenPt != wxDefaultPosition) {
        wxPoint docPt = ScreenToDoc(ScreenToClient(screenPt));
        int hit = HitTest(docPt);
        if (hit >= 0) {
            bool alreadySelected = std::find(m_selection.begin(),
                                             m_selection.end(), hit)
                                   != m_selection.end();
            if (!alreadySelected) {
                SetSingleSelection(hit);
                m_owner->NotifySelectionChanged();
                Refresh();
            }
        }
    }

    bool hasSel     = !m_selection.empty();
    bool canGroup   = (m_selection.size() >= 2) && (m_activeGroupIdx < 0);
    bool canUngroup = false;
    if (m_selection.size() == 1 && m_activeGroupIdx < 0) {
        const auto& shapes = doc->GetShapes();
        int idx = m_selection[0];
        canUngroup = (idx >= 0 && idx < (int)shapes.size() &&
                      shapes[idx].kind == ShapeKind::Group);
    }
    bool hasSingle = (m_selection.size() == 1);

    // Check clipboard for Paste enablement.
    static wxDataFormat clipFmt("application/x-spacely-shapes");
    bool hasClip = false;
    if (wxClipboard::Get()->Open()) {
        hasClip = wxClipboard::Get()->IsSupported(clipFmt);
        wxClipboard::Get()->Close();
    }

    wxMenu menu;
    menu.Append(wxID_CUT,   "&Cut\tCtrl+X")->Enable(hasSel);
    menu.Append(wxID_COPY,  "C&opy\tCtrl+C")->Enable(hasSel);
    menu.Append(wxID_PASTE, "&Paste\tCtrl+V")->Enable(hasClip);
    menu.Append(ID_CTX_DELETE, "&Delete\tDel")->Enable(hasSel);
    menu.AppendSeparator();
    menu.Append(wxID_SELECTALL, "Select &All\tCtrl+A");
    menu.AppendSeparator();
    menu.Append(ID_CTX_GROUP,   "&Group\tCtrl+G")->Enable(canGroup);
    menu.Append(ID_CTX_UNGROUP, "&Ungroup\tCtrl+Shift+G")->Enable(canUngroup);
    menu.AppendSeparator();
    menu.Append(ID_CTX_PROPERTIES, "&Properties…")->Enable(hasSingle);

    PopupMenu(&menu);
}

void DrawCanvas::OnCtxDelete(wxCommandEvent&) {
    auto* doc = GetDoc();
    if (!doc || m_selection.empty()) return;

    const std::vector<DrawShape>* scope = CurrentShapes(doc);
    if (!scope) return;

    std::vector<int> sorted = m_selection;
    std::sort(sorted.begin(), sorted.end(), std::greater<int>());

    if (sorted.size() == 1) {
        int idx = sorted[0];
        DrawShape shape = (*scope)[idx];
        ShapePath path = CurrentPath(idx);
        ClearSelection();
        m_owner->NotifySelectionChanged();
        doc->GetCommandProcessor()->Submit(new RemoveShapeAtCmd(doc, path, shape));
    } else {
        auto* batch = new BatchCmd("Delete Shapes");
        for (int idx : sorted) {
            DrawShape shape = (*scope)[idx];
            ShapePath path = CurrentPath(idx);
            batch->Add(new RemoveShapeAtCmd(doc, path, shape));
        }
        ClearSelection();
        m_owner->NotifySelectionChanged();
        doc->GetCommandProcessor()->Submit(batch);
    }
    Refresh();
}

void DrawCanvas::OnCtxGroup(wxCommandEvent&) {
    auto* doc = GetDoc();
    if (!doc || m_selection.size() < 2 || m_activeGroupIdx >= 0) return;

    std::vector<int> indices = m_selection;
    std::sort(indices.begin(), indices.end());
    auto* cmd = new GroupShapesCmd(doc, indices);
    ClearSelection();
    doc->GetCommandProcessor()->Submit(cmd);
    SetSingleSelection(cmd->GetGroupPath()[0]);
    m_owner->NotifySelectionChanged();
    Refresh();
}

void DrawCanvas::OnCtxUngroup(wxCommandEvent&) {
    auto* doc = GetDoc();
    if (!doc || m_selection.size() != 1 || m_activeGroupIdx >= 0) return;

    int idx = m_selection[0];
    const auto& shapes = doc->GetShapes();
    if (idx < 0 || idx >= (int)shapes.size() ||
        shapes[idx].kind != ShapeKind::Group) return;

    DrawShape group = shapes[idx];
    int childCount = (int)group.children.size();
    ClearSelection();
    doc->GetCommandProcessor()->Submit(new UngroupCmd(doc, ShapePath{idx}, group));
    for (int i = idx; i < idx + childCount; i++)
        m_selection.push_back(i);
    m_selected = m_selection.empty() ? -1 : m_selection.back();
    m_owner->NotifySelectionChanged();
    Refresh();
}

void DrawCanvas::OnCtxProperties(wxCommandEvent&) {
    if (m_selected >= 0)
        OpenPropertiesDialog(m_selected);
}

// ---------------------------------------------------------------------------
// Cut / Copy / Paste / SelectAll
// ---------------------------------------------------------------------------

// Custom clipboard format used to avoid conflicts with plain-text clipboard.
static const wxDataFormat& ShapesClipboardFormat() {
    static wxDataFormat fmt("application/x-spacely-shapes");
    return fmt;
}

// Recursively offset a shape's bounds (and Bezier points) by (dx, dy).
static void OffsetShape(DrawShape& s, int dx, int dy) {
    s.bounds.Offset(dx, dy);
    if (s.kind == ShapeKind::Bezier) {
        for (auto& pt : s.pts)
            pt += wxPoint(dx, dy);
    }
    for (auto& child : s.children)
        OffsetShape(child, dx, dy);
}

// Serialise a list of shapes to an XML string.
static wxString SerializeShapes(const std::vector<DrawShape>& shapes) {
    wxXmlDocument doc;
    auto* root = new wxXmlNode(nullptr, wxXML_ELEMENT_NODE, "spacely-clipboard");
    doc.SetRoot(root);
    for (const auto& s : shapes) {
        wxXmlNode* node = DrawDoc::ShapeToXml(s);
        if (node) root->AddChild(node);
    }
    wxStringOutputStream stream;
    doc.Save(stream);
    return stream.GetString();
}

// Deserialise a list of shapes from an XML string produced by SerializeShapes.
static std::vector<DrawShape> DeserializeShapes(const wxString& xml) {
    std::vector<DrawShape> result;
    wxStringInputStream stream(xml);
    wxXmlDocument doc(stream);
    if (!doc.IsOk()) return result;
    wxXmlNode* root = doc.GetRoot();
    if (!root || root->GetName() != "spacely-clipboard") return result;
    for (wxXmlNode* child = root->GetChildren(); child; child = child->GetNext()) {
        if (child->GetType() != wxXML_ELEMENT_NODE) continue;
        DrawShape s;
        if (DrawDoc::XmlToShape(child, s))
            result.push_back(s);
    }
    return result;
}

void DrawCanvas::CopyShapes() {
    auto* doc = GetDoc();
    if (!doc || m_selection.empty()) return;

    const std::vector<DrawShape>* scope = CurrentShapes(doc);
    if (!scope) return;

    std::vector<int> sorted = m_selection;
    std::sort(sorted.begin(), sorted.end());

    std::vector<DrawShape> toCopy;
    for (int idx : sorted) {
        if (idx < (int)scope->size())
            toCopy.push_back((*scope)[idx]);
    }

    wxString xml = SerializeShapes(toCopy);
    wxCharBuffer buf = xml.utf8_str();

    wxClipboardLocker locker;
    if (!locker) return;
    auto* obj = new wxCustomDataObject(ShapesClipboardFormat());
    obj->SetData(buf.length(), buf.data());
    wxClipboard::Get()->SetData(obj);
}

void DrawCanvas::CutShapes() {
    auto* doc = GetDoc();
    if (!doc || m_selection.empty()) return;

    // Copy first, then delete the selection (mirrors the Delete key handler).
    CopyShapes();

    const std::vector<DrawShape>* scope = CurrentShapes(doc);
    if (!scope) return;

    std::vector<int> sorted = m_selection;
    std::sort(sorted.begin(), sorted.end(), std::greater<int>());

    if (sorted.size() == 1) {
        int idx = sorted[0];
        DrawShape shape = (*scope)[idx];
        ShapePath path = CurrentPath(idx);
        ClearSelection();
        m_owner->NotifySelectionChanged();
        doc->GetCommandProcessor()->Submit(new RemoveShapeAtCmd(doc, path, shape));
    } else {
        auto* batch = new BatchCmd("Cut Shapes");
        for (int idx : sorted) {
            DrawShape shape = (*scope)[idx];
            ShapePath path = CurrentPath(idx);
            batch->Add(new RemoveShapeAtCmd(doc, path, shape));
        }
        ClearSelection();
        m_owner->NotifySelectionChanged();
        doc->GetCommandProcessor()->Submit(batch);
    }
    Refresh();
}

void DrawCanvas::PasteShapes() {
    auto* doc = GetDoc();
    if (!doc) return;

    // Read from clipboard.
    std::vector<DrawShape> shapes;
    {
        wxClipboardLocker locker;
        if (!locker) return;
        if (!wxClipboard::Get()->IsSupported(ShapesClipboardFormat())) return;
        wxCustomDataObject obj(ShapesClipboardFormat());
        if (!wxClipboard::Get()->GetData(obj)) return;
        wxString xml = wxString::FromUTF8(
            static_cast<const char*>(obj.GetData()),
            obj.GetSize());
        shapes = DeserializeShapes(xml);
    }
    if (shapes.empty()) return;

    // Offset pasted shapes so they don't land exactly on top of the originals.
    constexpr int kPasteOffset = 10;
    for (auto& s : shapes)
        OffsetShape(s, kPasteOffset, kPasteOffset);

    const std::vector<DrawShape>* scope = CurrentShapes(doc);
    int baseIdx = scope ? (int)scope->size() : 0;
    ShapePath parentPath = m_activeGroupIdx >= 0
        ? ShapePath{ m_activeGroupIdx }
        : ShapePath{};

    ClearSelection();

    if (shapes.size() == 1) {
        doc->GetCommandProcessor()->Submit(
            new InsertShapeAtCmd(doc, parentPath, baseIdx, shapes[0]));
        SetSingleSelection(baseIdx);
    } else {
        auto* batch = new BatchCmd("Paste Shapes");
        for (int i = 0; i < (int)shapes.size(); i++) {
            batch->Add(new InsertShapeAtCmd(doc, parentPath, baseIdx + i, shapes[i]));
            m_selection.push_back(baseIdx + i);
        }
        m_selected = m_selection.empty() ? -1 : m_selection.back();
        doc->GetCommandProcessor()->Submit(batch);
    }

    m_owner->NotifySelectionChanged();
    Refresh();
}

void DrawCanvas::SelectAll() {
    auto* doc = GetDoc();
    if (!doc) return;
    const std::vector<DrawShape>* scope = CurrentShapes(doc);
    if (!scope || scope->empty()) return;

    m_selection.clear();
    for (int i = 0; i < (int)scope->size(); i++)
        m_selection.push_back(i);
    m_selected = m_selection.back();
    m_owner->NotifySelectionChanged();
    Refresh();
}

// ---------------------------------------------------------------------------
// Viewport navigation — pan (middle-mouse drag) and zoom (Ctrl+wheel)
// ---------------------------------------------------------------------------

void DrawCanvas::OnMiddleDown(wxMouseEvent& e) {
    m_panning         = true;
    m_panStart        = e.GetPosition();
    m_panStartOffset  = m_viewOffset;
    CaptureMouse();
    SetCursor(wxCursor(wxCURSOR_HAND));
    e.Skip();
}

void DrawCanvas::OnMiddleUp(wxMouseEvent& e) {
    if (m_panning) {
        m_panning = false;
        if (HasCapture()) ReleaseMouse();
        SetCursor(wxNullCursor);
    }
    e.Skip();
}

void DrawCanvas::OnMouseWheel(wxMouseEvent& e) {
    if (!e.ControlDown()) { e.Skip(); return; }

    static constexpr double kFactor  = 1.15;
    static constexpr double kMinZoom = 0.05;
    static constexpr double kMaxZoom = 20.0;

    const wxPoint mouse  = e.GetPosition();
    const double  logX   = (mouse.x - m_viewOffset.x) / m_zoom;
    const double  logY   = (mouse.y - m_viewOffset.y) / m_zoom;
    const double  newZoom = std::max(kMinZoom, std::min(kMaxZoom,
                              m_zoom * (e.GetWheelRotation() > 0 ? kFactor : 1.0/kFactor)));

    m_viewOffset.x = (int)std::round(mouse.x - logX * newZoom);
    m_viewOffset.y = (int)std::round(mouse.y - logY * newZoom);
    m_zoom         = newZoom;

    Refresh();
}

void DrawCanvas::OpenPropertiesDialog(int idx) {
    auto* doc = GetDoc();
    if (!doc) return;
    const std::vector<DrawShape>* scope = CurrentShapes(doc);
    if (!scope || idx < 0 || idx >= (int)scope->size()) return;

    DrawShape before = (*scope)[idx];
    DrawShape after  = before;
    // Groups and SVGRef don't have a leaf-properties dialog yet.
    if (before.kind == ShapeKind::Group || before.kind == ShapeKind::SVGRef) return;
    ShapePropertiesDialog dlg(this, after);
    if (dlg.ShowModal() == wxID_OK) {
        dlg.Apply(after);
        ShapePath path = CurrentPath(idx);
        doc->GetCommandProcessor()->Submit(
            new UpdateShapeAtCmd(doc, path, before, after, "Edit Properties"));
    }
}

void DrawCanvas::ValidateSelection() {
    auto* doc = GetDoc();
    const std::vector<DrawShape>* scope = CurrentShapes(doc);
    int shapeCount = scope ? (int)scope->size() : 0;
    m_selection.erase(
        std::remove_if(m_selection.begin(), m_selection.end(),
                       [shapeCount](int i){ return i < 0 || i >= shapeCount; }),
        m_selection.end());
    if (!m_selection.empty())
        m_selected = m_selection.back();
    else
        m_selected = -1;

    // Validate the active group index.
    if (m_activeGroupIdx >= 0) {
        int rootCount = doc ? (int)doc->GetShapes().size() : 0;
        if (m_activeGroupIdx >= rootCount ||
            doc->GetShapes()[m_activeGroupIdx].kind != ShapeKind::Group)
        {
            m_activeGroupIdx = -1;
        }
    }
}

// ---------------------------------------------------------------------------
// DrawView
// ---------------------------------------------------------------------------

void DrawView::NotifySelectionChanged() {
    auto* mf = wxDynamicCast(wxGetApp().GetTopWindow(), MainFrame);
    if (!mf) return;
    DrawDoc* doc = m_canvas ? wxDynamicCast(GetDocument(), DrawDoc) : nullptr;
    static const std::vector<int> kEmpty;
    const std::vector<int>& sel = m_canvas ? m_canvas->GetSelection() : kEmpty;
    mf->OnSelectionChanged(doc, sel);
}

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

    if (activate) {
        mf->SetActiveDrawView(this);
        NotifySelectionChanged();
    } else {
        mf->SetActiveDrawView(nullptr);
        mf->OnSelectionChanged(nullptr, {});
    }
}

bool DrawView::OnClose(bool deleteWindow) {
    if (!wxView::OnClose(deleteWindow)) return false;

    auto* mf = wxDynamicCast(wxGetApp().GetTopWindow(), MainFrame);
    if (mf && m_canvas) {
        wxAuiNotebook* nb = mf->GetNotebook();
        if (nb) {
            int idx = nb->GetPageIndex(m_canvas);
            if (idx != wxNOT_FOUND)
                nb->DeletePage(idx);
        }
    }
    return true;
}
