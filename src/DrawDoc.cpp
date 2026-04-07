#include "DrawDoc.h"
#include <wx/cmdproc.h>
#include <wx/wfstream.h>
#include <wx/txtstrm.h>
#include <wx/wxcrtvararg.h>
#include <algorithm>

wxIMPLEMENT_DYNAMIC_CLASS(DrawDoc, wxDocument);

// ---------------------------------------------------------------------------
// Shape mutations
// ---------------------------------------------------------------------------

// Each document gets its own independent undo/redo stack.
DrawDoc::DrawDoc() {
    SetCommandProcessor(new wxCommandProcessor);
}

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

bool DrawDoc::IsModified() const {
    return wxDocument::IsModified() || m_modified;
}

void DrawDoc::Modify(bool modified) {
    wxDocument::Modify(modified);
    m_modified = modified;
}

// ---------------------------------------------------------------------------
// Serialisation
//
// Line format (one shape per line):
//   KIND x y w h fgR fgG fgB bgR bgG bgB "label"
// KIND is RECT, CIRCLE, or TEXT.
// ---------------------------------------------------------------------------

static const char* KindStr(ShapeKind k) {
    switch (k) {
        case ShapeKind::Rect:   return "RECT";
        case ShapeKind::Circle: return "CIRCLE";
        case ShapeKind::Text:   return "TEXT";
        case ShapeKind::Bezier: return "BEZIER";
    }
    return "RECT";
}

static ShapeKind StrKind(const wxString& s) {
    if (s == "CIRCLE") return ShapeKind::Circle;
    if (s == "TEXT")   return ShapeKind::Text;
    if (s == "BEZIER") return ShapeKind::Bezier;
    return ShapeKind::Rect;
}

bool DrawDoc::DoSaveDocument(const wxString& filename) {
    wxFileOutputStream fos(filename);
    if (!fos.IsOk()) return false;
    wxTextOutputStream out(fos);

    for (const DrawShape& s : m_shapes) {
        if (s.kind == ShapeKind::Bezier) {
            // BEZIER x0 y0 x1 y1 x2 y2 x3 y3  fgR fgG fgB  bgR bgG bgB
            out.WriteString(wxString::Format(
                "BEZIER %d %d %d %d %d %d %d %d  %d %d %d  %d %d %d\n",
                s.pts[0].x, s.pts[0].y, s.pts[1].x, s.pts[1].y,
                s.pts[2].x, s.pts[2].y, s.pts[3].x, s.pts[3].y,
                (int)s.fgColour.Red(), (int)s.fgColour.Green(), (int)s.fgColour.Blue(),
                (int)s.bgColour.Red(), (int)s.bgColour.Green(), (int)s.bgColour.Blue()));
        } else {
            out.WriteString(wxString::Format(
                "%s %d %d %d %d  %d %d %d  %d %d %d  \"%s\"\n",
                KindStr(s.kind),
                s.bounds.x, s.bounds.y, s.bounds.width, s.bounds.height,
                (int)s.fgColour.Red(), (int)s.fgColour.Green(), (int)s.fgColour.Blue(),
                (int)s.bgColour.Red(), (int)s.bgColour.Green(), (int)s.bgColour.Blue(),
                s.label));
        }
    }
    Modify(false);
    GetCommandProcessor()->MarkAsSaved();
    return true;
}

bool DrawDoc::DoOpenDocument(const wxString& filename) {
    wxFileInputStream fis(filename);
    if (!fis.IsOk()) return false;
    wxTextInputStream in(fis);
    m_shapes.clear();
    GetCommandProcessor()->ClearCommands();

    while (!fis.Eof()) {
        wxString line = in.ReadLine().Trim(false).Trim(true);
        if (line.IsEmpty()) continue;

        DrawShape s;
        s.kind = StrKind(line.BeforeFirst(' '));

        wxString rest = line.AfterFirst(' ').Trim(false);

        if (s.kind == ShapeKind::Bezier) {
            int x0,y0,x1,y1,x2,y2,x3,y3, fgR,fgG,fgB, bgR,bgG,bgB;
            if (wxSscanf(rest, "%d %d %d %d %d %d %d %d %d %d %d %d %d %d",
                         &x0,&y0,&x1,&y1,&x2,&y2,&x3,&y3,
                         &fgR,&fgG,&fgB,&bgR,&bgG,&bgB) != 14)
                continue;
            s.pts[0] = {x0,y0}; s.pts[1] = {x1,y1};
            s.pts[2] = {x2,y2}; s.pts[3] = {x3,y3};
            int minX = std::min({x0,x1,x2,x3}), maxX = std::max({x0,x1,x2,x3});
            int minY = std::min({y0,y1,y2,y3}), maxY = std::max({y0,y1,y2,y3});
            s.bounds   = wxRect(minX, minY, maxX-minX, maxY-minY);
            s.fgColour = wxColour(fgR, fgG, fgB);
            s.bgColour = wxColour(bgR, bgG, bgB);
        } else {
            int x, y, w, h, fgR, fgG, fgB, bgR, bgG, bgB;
            if (wxSscanf(rest, "%d %d %d %d %d %d %d %d %d %d",
                         &x, &y, &w, &h, &fgR, &fgG, &fgB, &bgR, &bgG, &bgB) != 10)
                continue;
            s.bounds   = wxRect(x, y, w, h);
            s.fgColour = wxColour(fgR, fgG, fgB);
            s.bgColour = wxColour(bgR, bgG, bgB);
            int q1 = line.Find('"');
            int q2 = line.Find('"', /*fromEnd=*/true);
            if (q1 != wxNOT_FOUND && q2 > q1)
                s.label = line.Mid(q1 + 1, q2 - q1 - 1);
        }

        m_shapes.push_back(s);
    }

    Modify(false);
    UpdateAllViews();
    return true;
}
