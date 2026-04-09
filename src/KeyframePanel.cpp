#include "KeyframePanel.h"
#include "SmilView.h"
#include "SmilDoc.h"
#include "DrawShape.h"
#include <wx/sizer.h>
#include <wx/dcclient.h>
#include <wx/dcmemory.h>
#include <wx/dcbuffer.h>
#include <wx/settings.h>
#include <algorithm>
#include <set>

// ---------------------------------------------------------------------------
// Property names shown as sub-rows for each shape kind.
// ---------------------------------------------------------------------------

static const wxString kCommonProps[] = { "x", "y", "width", "height", "fill", "stroke", "stroke-width" };
static const wxString kBezierProps[] = { "d", "fill", "stroke", "stroke-width" };

static std::vector<wxString> PropsForShape(const DrawShape& s) {
    if (s.kind == ShapeKind::Bezier)
        return std::vector<wxString>(std::begin(kBezierProps), std::end(kBezierProps));
    return std::vector<wxString>(std::begin(kCommonProps), std::end(kCommonProps));
}

// ---------------------------------------------------------------------------
// Event table
// ---------------------------------------------------------------------------

wxBEGIN_EVENT_TABLE(KeyframePanel, wxPanel)
wxEND_EVENT_TABLE()

// ---------------------------------------------------------------------------
// Construction
// ---------------------------------------------------------------------------

KeyframePanel::KeyframePanel(wxWindow* parent)
    : wxPanel(parent, wxID_ANY)
{
    SetBackgroundColour(wxSystemSettings::GetColour(wxSYS_COLOUR_BTNFACE));

    auto* sizer = new wxBoxSizer(wxHORIZONTAL);

    // Label column.
    m_labelPanel = new wxPanel(this, wxID_ANY, wxDefaultPosition,
                               wxSize(kLabelWidth, -1));
    m_labelPanel->SetBackgroundStyle(wxBG_STYLE_PAINT);
    m_labelPanel->Bind(wxEVT_PAINT,      &KeyframePanel::OnPaintLabels,   this);
    m_labelPanel->Bind(wxEVT_LEFT_DOWN,  &KeyframePanel::OnLabelLeftDown, this);
    sizer->Add(m_labelPanel, 0, wxEXPAND);

    // Timeline area.
    m_timelinePanel = new wxPanel(this, wxID_ANY);
    m_timelinePanel->SetBackgroundStyle(wxBG_STYLE_PAINT);
    m_timelinePanel->Bind(wxEVT_PAINT,      &KeyframePanel::OnPaintTimeline,    this);
    m_timelinePanel->Bind(wxEVT_LEFT_DOWN,  &KeyframePanel::OnTimelineLeftDown, this);
    m_timelinePanel->Bind(wxEVT_LEFT_UP,    &KeyframePanel::OnTimelineLeftUp,   this);
    m_timelinePanel->Bind(wxEVT_MOTION,     &KeyframePanel::OnTimelineMotion,   this);
    sizer->Add(m_timelinePanel, 1, wxEXPAND);

    SetSizer(sizer);
}

// ---------------------------------------------------------------------------
// Public API
// ---------------------------------------------------------------------------

void KeyframePanel::Refresh(SmilView* view) {
    m_view = view;
    BuildRows();
    if (m_labelPanel)   m_labelPanel->Refresh();
    if (m_timelinePanel) m_timelinePanel->Refresh();
}

void KeyframePanel::UpdateRecButton(bool /*recActive*/) {
    // REC button appearance is managed by MainFrame toolbar; nothing to do here.
}

// ---------------------------------------------------------------------------
// Row building
// ---------------------------------------------------------------------------

void KeyframePanel::BuildRows() {
    m_rows.clear();
    SmilDoc* doc = m_view ? wxDynamicCast(m_view->GetDocument(), SmilDoc) : nullptr;
    const SmilScene* sc = doc ? doc->GetCurrentScene() : nullptr;
    if (!sc) return;

    // Ensure expanded array matches shape count.
    m_expanded.resize(sc->shapes.size(), false);

    for (int i = 0; i < (int)sc->shapes.size(); ++i) {
        const DrawShape& s = sc->shapes[i];
        wxString eid = SmilDoc::ShapeElementId(s, i);

        Row r;
        r.isProperty = false;
        r.shapeIdx   = i;
        r.elemId     = eid;
        r.label      = s.name.IsEmpty() ? wxString::Format("Shape %d", i + 1) : s.name;
        m_rows.push_back(r);

        if (i < (int)m_expanded.size() && m_expanded[i]) {
            for (const wxString& prop : PropsForShape(s)) {
                Row pr;
                pr.isProperty = true;
                pr.shapeIdx   = i;
                pr.elemId     = eid;
                pr.propName   = prop;
                pr.label      = "  " + prop;
                m_rows.push_back(pr);
            }
        }
    }
}

// ---------------------------------------------------------------------------
// Helpers
// ---------------------------------------------------------------------------

int KeyframePanel::TotalFrames() const {
    SmilDoc* doc = m_view ? wxDynamicCast(m_view->GetDocument(), SmilDoc) : nullptr;
    if (!doc) return 240;
    const SmilScene* sc = doc->GetCurrentScene();
    return sc ? sc->durationFrames : 240;
}

int KeyframePanel::FrameAtX(int x) const {
    return std::max(0, x / kFrameWidth);
}

int KeyframePanel::XAtFrame(int frame) const {
    return frame * kFrameWidth;
}

void KeyframePanel::MovePlayhead(int frame) {
    SmilDoc* doc = m_view ? wxDynamicCast(m_view->GetDocument(), SmilDoc) : nullptr;
    if (!doc) return;
    const SmilScene* sc = doc->GetCurrentScene();
    if (!sc) return;
    int abs = sc->startFrame + std::max(0, std::min(frame, sc->durationFrames - 1));
    doc->SetCurrentFrame(abs);
    if (m_timelinePanel) m_timelinePanel->Refresh();
}

void KeyframePanel::DrawDiamond(wxDC& dc, int cx, int cy, bool selected, bool hasKf) {
    const int R = 5;
    wxPoint pts[4] = {
        {cx,    cy - R},
        {cx + R, cy   },
        {cx,    cy + R},
        {cx - R, cy   }
    };
    if (hasKf)
        dc.SetBrush(wxBrush(selected ? wxColour(255, 200, 0) : wxColour(200, 150, 50)));
    else
        dc.SetBrush(*wxTRANSPARENT_BRUSH);
    dc.SetPen(wxPen(wxColour(100, 80, 20)));
    dc.DrawPolygon(4, pts);
}

// ---------------------------------------------------------------------------
// Paint: label column
// ---------------------------------------------------------------------------

void KeyframePanel::OnPaintLabels(wxPaintEvent&) {
    wxAutoBufferedPaintDC dc(m_labelPanel);
    wxSize sz = m_labelPanel->GetClientSize();

    dc.SetBackground(wxBrush(wxSystemSettings::GetColour(wxSYS_COLOUR_BTNFACE)));
    dc.Clear();

    // Header.
    dc.SetBrush(wxBrush(wxSystemSettings::GetColour(wxSYS_COLOUR_BTNHIGHLIGHT)));
    dc.SetPen(*wxTRANSPARENT_PEN);
    dc.DrawRectangle(0, 0, sz.x, kHeaderH);
    dc.SetPen(wxPen(wxSystemSettings::GetColour(wxSYS_COLOUR_BTNSHADOW)));
    dc.DrawLine(0, kHeaderH - 1, sz.x, kHeaderH - 1);

    dc.SetFont(wxSystemSettings::GetFont(wxSYS_DEFAULT_GUI_FONT));
    dc.SetTextForeground(wxSystemSettings::GetColour(wxSYS_COLOUR_BTNTEXT));
    dc.DrawText("Object / Property", 4, (kHeaderH - dc.GetCharHeight()) / 2);

    // Rows.
    int y = kHeaderH;
    for (const Row& r : m_rows) {
        bool isShape = !r.isProperty;
        wxColour bg = isShape
            ? wxSystemSettings::GetColour(wxSYS_COLOUR_BTNFACE)
            : wxSystemSettings::GetColour(wxSYS_COLOUR_WINDOW);

        dc.SetBrush(wxBrush(bg));
        dc.SetPen(*wxTRANSPARENT_PEN);
        dc.DrawRectangle(0, y, sz.x, kRowHeight);

        dc.SetPen(wxPen(wxSystemSettings::GetColour(wxSYS_COLOUR_BTNSHADOW)));
        dc.DrawLine(0, y + kRowHeight - 1, sz.x, y + kRowHeight - 1);

        // Expand/collapse triangle for shape rows.
        if (isShape) {
            int si = r.shapeIdx;
            bool exp = (si < (int)m_expanded.size() && m_expanded[si]);
            wxPoint tri[3];
            if (exp) {
                tri[0]={6, y+7}; tri[1]={12,y+7}; tri[2]={9,y+13};
            } else {
                tri[0]={6,y+6}; tri[1]={6,y+14}; tri[2]={12,y+10};
            }
            dc.SetBrush(*wxBLACK_BRUSH);
            dc.SetPen(*wxBLACK_PEN);
            dc.DrawPolygon(3, tri);
        }

        dc.SetTextForeground(wxSystemSettings::GetColour(wxSYS_COLOUR_BTNTEXT));
        dc.DrawText(r.label, 16, y + (kRowHeight - dc.GetCharHeight()) / 2);

        y += kRowHeight;
    }
}

// ---------------------------------------------------------------------------
// Paint: timeline area
// ---------------------------------------------------------------------------

void KeyframePanel::OnPaintTimeline(wxPaintEvent&) {
    wxAutoBufferedPaintDC dc(m_timelinePanel);
    wxSize sz = m_timelinePanel->GetClientSize();

    dc.SetBackground(*wxWHITE_BRUSH);
    dc.Clear();

    SmilDoc* doc = m_view ? wxDynamicCast(m_view->GetDocument(), SmilDoc) : nullptr;
    const SmilScene* sc = doc ? doc->GetCurrentScene() : nullptr;
    int totalFrames = TotalFrames();

    // ---- Header: frame ruler ----
    dc.SetBrush(wxBrush(wxColour(220, 220, 220)));
    dc.SetPen(*wxTRANSPARENT_PEN);
    dc.DrawRectangle(0, 0, sz.x, kHeaderH);

    dc.SetPen(wxPen(wxColour(140, 140, 140)));
    dc.SetFont(wxFont(7, wxFONTFAMILY_DEFAULT, wxFONTSTYLE_NORMAL, wxFONTWEIGHT_NORMAL));
    dc.SetTextForeground(wxColour(40, 40, 40));

    int fps = doc ? doc->GetFps() : 24;
    // Tick every second.
    for (int f = 0; f <= totalFrames; f += fps) {
        int x = XAtFrame(f);
        if (x > sz.x) break;
        dc.DrawLine(x, 0, x, kHeaderH);
        double sec = (fps > 0) ? (double)f / fps : 0;
        dc.DrawText(wxString::Format("%.0fs", sec), x + 2, 2);
    }

    // ---- Row backgrounds and keyframe diamonds ----
    int y = kHeaderH;
    for (const Row& r : m_rows) {
        bool isShape = !r.isProperty;
        dc.SetBrush(wxBrush(isShape ? wxColour(235, 235, 235) : wxColour(248, 248, 248)));
        dc.SetPen(*wxTRANSPARENT_PEN);
        dc.DrawRectangle(0, y, sz.x, kRowHeight);
        dc.SetPen(wxPen(wxColour(200, 200, 200)));
        dc.DrawLine(0, y + kRowHeight - 1, sz.x, y + kRowHeight - 1);

        if (r.isProperty && sc) {
            // Draw keyframe diamonds for this property.
            auto eit = sc->elements.find(r.elemId);
            if (eit != sc->elements.end()) {
                auto tit = eit->second.tracks.find(r.propName);
                if (tit != eit->second.tracks.end()) {
                    int cy = y + kRowHeight / 2;
                    for (const SmilKeyframe& kf : tit->second.keyframes) {
                        int cx = XAtFrame(kf.frame);
                        if (cx >= 0 && cx < sz.x)
                            DrawDiamond(dc, cx, cy, false, true);
                    }
                }
            }
        } else if (isShape && sc) {
            // For shape rows: draw a dot at any frame where any property has a keyframe.
            auto eit = sc->elements.find(r.elemId);
            if (eit != sc->elements.end()) {
                // Collect all unique frames.
                std::set<int> frames;
                for (const auto& [prop, track] : eit->second.tracks)
                    for (const SmilKeyframe& kf : track.keyframes)
                        frames.insert(kf.frame);
                int cy = y + kRowHeight / 2;
                dc.SetBrush(wxBrush(wxColour(180, 130, 30)));
                dc.SetPen(*wxTRANSPARENT_PEN);
                for (int f : frames) {
                    int cx = XAtFrame(f);
                    if (cx >= 0 && cx < sz.x)
                        dc.DrawCircle(cx, cy, 3);
                }
            }
        }

        y += kRowHeight;
    }

    // ---- Playhead ----
    if (doc && sc) {
        int relFrame = doc->GetCurrentFrame() - sc->startFrame;
        int px = XAtFrame(relFrame);
        dc.SetPen(wxPen(wxColour(255, 80, 80), 2));
        dc.DrawLine(px, 0, px, sz.y);
        // Playhead triangle at top.
        wxPoint tri[3] = {{px-5,0},{px+5,0},{px,8}};
        dc.SetBrush(wxBrush(wxColour(255, 80, 80)));
        dc.SetPen(*wxTRANSPARENT_PEN);
        dc.DrawPolygon(3, tri);
    }
}

// ---------------------------------------------------------------------------
// Mouse interaction: timeline
// ---------------------------------------------------------------------------

void KeyframePanel::OnTimelineLeftDown(wxMouseEvent& e) {
    m_draggingPlayhead = true;
    m_timelinePanel->CaptureMouse();
    MovePlayhead(FrameAtX(e.GetX()));
    e.Skip();
}

void KeyframePanel::OnTimelineLeftUp(wxMouseEvent& e) {
    if (m_draggingPlayhead) {
        m_draggingPlayhead = false;
        if (m_timelinePanel->HasCapture())
            m_timelinePanel->ReleaseMouse();
    }
    e.Skip();
}

void KeyframePanel::OnTimelineMotion(wxMouseEvent& e) {
    if (m_draggingPlayhead && e.LeftIsDown())
        MovePlayhead(FrameAtX(e.GetX()));
    e.Skip();
}

// ---------------------------------------------------------------------------
// Mouse interaction: labels (expand/collapse)
// ---------------------------------------------------------------------------

void KeyframePanel::OnLabelLeftDown(wxMouseEvent& e) {
    int y = e.GetY() - kHeaderH;
    if (y < 0) { e.Skip(); return; }

    int row = y / kRowHeight;
    if (row < 0 || row >= (int)m_rows.size()) { e.Skip(); return; }

    const Row& r = m_rows[row];
    if (!r.isProperty && e.GetX() < 16) {
        int si = r.shapeIdx;
        if (si >= 0 && si < (int)m_expanded.size()) {
            m_expanded[si] = !m_expanded[si];
            BuildRows();
            m_labelPanel->Refresh();
            m_timelinePanel->Refresh();
        }
    }
    e.Skip();
}
