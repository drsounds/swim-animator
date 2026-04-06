#include "DrawDoc.h"
#include <wx/wfstream.h>
#include <wx/txtstrm.h>
#include <wx/wxcrtvararg.h>

wxIMPLEMENT_DYNAMIC_CLASS(DrawDoc, wxDocument);

// ---------------------------------------------------------------------------
// Shape mutations
// ---------------------------------------------------------------------------

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
    }
    return "RECT";
}

static ShapeKind StrKind(const wxString& s) {
    if (s == "CIRCLE") return ShapeKind::Circle;
    if (s == "TEXT")   return ShapeKind::Text;
    return ShapeKind::Rect;
}

bool DrawDoc::DoSaveDocument(const wxString& filename) {
    wxFileOutputStream fos(filename);
    if (!fos.IsOk()) return false;
    wxTextOutputStream out(fos);

    for (const DrawShape& s : m_shapes) {
        out.WriteString(wxString::Format(
            "%s %d %d %d %d  %d %d %d  %d %d %d  \"%s\"\n",
            KindStr(s.kind),
            s.bounds.x, s.bounds.y, s.bounds.width, s.bounds.height,
            (int)s.fgColour.Red(), (int)s.fgColour.Green(), (int)s.fgColour.Blue(),
            (int)s.bgColour.Red(), (int)s.bgColour.Green(), (int)s.bgColour.Blue(),
            s.label));
    }
    Modify(false);
    return true;
}

bool DrawDoc::DoOpenDocument(const wxString& filename) {
    wxFileInputStream fis(filename);
    if (!fis.IsOk()) return false;
    wxTextInputStream in(fis);
    m_shapes.clear();

    while (!fis.Eof()) {
        wxString line = in.ReadLine().Trim(false).Trim(true);
        if (line.IsEmpty()) continue;

        DrawShape s;
        s.kind = StrKind(line.BeforeFirst(' '));

        wxString rest = line.AfterFirst(' ').Trim(false);
        int x, y, w, h, fgR, fgG, fgB, bgR, bgG, bgB;
        if (wxSscanf(rest, "%d %d %d %d %d %d %d %d %d %d",
                     &x, &y, &w, &h, &fgR, &fgG, &fgB, &bgR, &bgG, &bgB) != 10)
            continue;

        s.bounds   = wxRect(x, y, w, h);
        s.fgColour = wxColour(fgR, fgG, fgB);
        s.bgColour = wxColour(bgR, bgG, bgB);

        // Extract quoted label
        int q1 = line.Find('"');
        int q2 = line.Find('"', /*fromEnd=*/true);
        if (q1 != wxNOT_FOUND && q2 > q1)
            s.label = line.Mid(q1 + 1, q2 - q1 - 1);

        m_shapes.push_back(s);
    }

    Modify(false);
    UpdateAllViews();
    return true;
}
