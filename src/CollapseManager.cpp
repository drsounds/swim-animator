#include "CollapseManager.h"
#include <wx/dc.h>
#include <wx/settings.h>

// ---------------------------------------------------------------------------
// SpacelyDockArt::DrawCaption
// ---------------------------------------------------------------------------

void SpacelyDockArt::DrawCaption(wxDC& dc, wxWindow* window, const wxString& text,
                                  const wxRect& rect, wxAuiPaneInfo& pane) {
    // Let the default art render the full caption (background, gradient, buttons).
    wxAuiDefaultDockArt::DrawCaption(dc, window, text, rect, pane);

    // Determine caption colours from the art provider to match the background.
    bool active = (pane.state & wxAuiPaneInfo::optionActive) != 0;
    wxColour bgCol = GetColour(active ? wxAUI_DOCKART_ACTIVE_CAPTION_COLOUR
                                      : wxAUI_DOCKART_INACTIVE_CAPTION_COLOUR);
    wxColour fgCol = GetColour(active ? wxAUI_DOCKART_ACTIVE_CAPTION_TEXT_COLOUR
                                      : wxAUI_DOCKART_INACTIVE_CAPTION_TEXT_COLOUR);

    // Overdraw the left kArrowArea pixels with the plain caption colour so the
    // gradient / text that the default art drew there is erased cleanly.
    dc.SetPen(*wxTRANSPARENT_PEN);
    dc.SetBrush(wxBrush(bgCol));
    dc.DrawRectangle(rect.x, rect.y, kArrowArea, rect.height);

    // Draw the collapse/expand triangle.
    bool collapsed = m_mgr && m_mgr->IsCollapsed(pane.name);
    const int cx = rect.x + kArrowArea / 2;
    const int cy = rect.y + rect.height / 2;
    const int R  = 4;
    wxPoint tri[3];
    if (collapsed) {
        // Right-pointing triangle: click to expand.
        tri[0] = {cx - R + 2, cy - R};
        tri[1] = {cx - R + 2, cy + R};
        tri[2] = {cx + R - 1, cy   };
    } else {
        // Down-pointing triangle: click to collapse.
        tri[0] = {cx - R,     cy - R + 2};
        tri[1] = {cx + R,     cy - R + 2};
        tri[2] = {cx,         cy + R - 1};
    }
    dc.SetBrush(wxBrush(fgCol));
    dc.SetPen(wxPen(fgCol));
    dc.DrawPolygon(3, tri);

    // Redraw the caption text shifted right so it clears the arrow.
    dc.SetFont(wxSystemSettings::GetFont(wxSYS_DEFAULT_GUI_FONT));
    dc.SetTextForeground(fgCol);
    wxRect textClip(rect.x + kArrowArea, rect.y, rect.width - kArrowArea, rect.height);
    dc.SetClippingRegion(textClip);
    dc.DrawText(text,
                rect.x + kArrowArea + 2,
                rect.y + (rect.height - dc.GetCharHeight()) / 2);
    dc.DestroyClippingRegion();
}

// ---------------------------------------------------------------------------
// CollapseManager
// ---------------------------------------------------------------------------

CollapseManager::CollapseManager(wxAuiManager* auiMgr)
    : m_auiMgr(auiMgr)
{}

bool CollapseManager::IsCollapsed(const wxString& paneName) const {
    auto it = m_collapsed.find(paneName);
    return (it != m_collapsed.end()) && it->second;
}

void CollapseManager::Toggle(const wxString& paneName) {
    wxAuiPaneInfo& pane = m_auiMgr->GetPane(paneName);
    if (!pane.IsOk() || pane.IsFloating() || pane.IsToolbar()) return;

    const bool nowCollapsed = !IsCollapsed(paneName);
    m_collapsed[paneName] = nowCollapsed;

    const int captionH = m_auiMgr->GetArtProvider()
                             ->GetMetric(wxAUI_DOCKART_CAPTION_SIZE);

    const bool horizontal = (pane.dock_direction == wxAUI_DOCK_TOP ||
                              pane.dock_direction == wxAUI_DOCK_BOTTOM);

    if (nowCollapsed) {
        m_savedBestSize[paneName] = pane.best_size;
        m_savedMinSize[paneName]  = pane.min_size;

        if (horizontal) {
            pane.best_size.y = captionH;
            pane.min_size.y  = captionH;
        } else {
            pane.best_size.x = captionH;
            pane.min_size.x  = captionH;
        }
        if (pane.window) pane.window->Show(false);
    } else {
        auto itBest = m_savedBestSize.find(paneName);
        auto itMin  = m_savedMinSize.find(paneName);
        if (itBest != m_savedBestSize.end()) pane.best_size = itBest->second;
        if (itMin  != m_savedMinSize.end())  pane.min_size  = itMin->second;
        if (pane.window) pane.window->Show(true);
    }

    m_auiMgr->Update();
}

wxString CollapseManager::HitTestArrow(const wxPoint& pt) const {
    const int captionH = m_auiMgr->GetArtProvider()
                             ->GetMetric(wxAUI_DOCKART_CAPTION_SIZE);
    const wxAuiPaneInfoArray& panes = m_auiMgr->GetAllPanes();

    for (size_t i = 0; i < panes.GetCount(); ++i) {
        const wxAuiPaneInfo& p = panes.Item(i);
        if (!p.IsShown() || p.IsFloating() || p.IsToolbar()) continue;
        if (!p.HasCaption()) continue;
        if (p.rect.IsEmpty()) continue;

        // pane.rect covers caption + content; caption is at the top.
        wxRect arrowRect(p.rect.x, p.rect.y,
                         SpacelyDockArt::kArrowArea, captionH);
        if (arrowRect.Contains(pt))
            return p.name;
    }
    return wxEmptyString;
}
