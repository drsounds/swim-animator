#pragma once
#include <wx/aui/aui.h>
#include <map>

class CollapseManager;

// ---------------------------------------------------------------------------
// SpacelyDockArt
//
// Extends wxAuiDefaultDockArt to draw a small collapse/expand arrow on the
// left side of every docked pane caption bar.  The arrow is drawn after the
// standard caption rendering so that gradient backgrounds and button placement
// from the default art are preserved; only the leftmost kArrowArea pixels are
// overdrawn.
// ---------------------------------------------------------------------------
class SpacelyDockArt : public wxAuiDefaultDockArt {
public:
    explicit SpacelyDockArt(CollapseManager* mgr) : m_mgr(mgr) {}

    void DrawCaption(wxDC& dc, wxWindow* window, const wxString& text,
                     const wxRect& rect, wxAuiPaneInfo& pane) override;

    // Pixels reserved at the left edge of each caption for the arrow.
    static constexpr int kArrowArea = 16;

private:
    CollapseManager* m_mgr;
};

// ---------------------------------------------------------------------------
// CollapseManager
//
// Tracks collapsed / expanded state for named AUI panes and handles the
// size toggle logic.  Provides HitTestArrow() so MainFrame can intercept
// left-clicks on the arrow region before wxAuiManager processes them.
// ---------------------------------------------------------------------------
class CollapseManager {
public:
    explicit CollapseManager(wxAuiManager* auiMgr);

    // Returns true when the named pane is currently collapsed.
    bool IsCollapsed(const wxString& paneName) const;

    // Toggle collapse / expand for the named pane and call Update().
    void Toggle(const wxString& paneName);

    // Given a mouse position in managed-window client coordinates, return the
    // name of the pane whose caption arrow was clicked, or wxEmptyString.
    wxString HitTestArrow(const wxPoint& pt) const;

private:
    wxAuiManager*              m_auiMgr;
    std::map<wxString, bool>   m_collapsed;
    std::map<wxString, wxSize> m_savedBestSize;
    std::map<wxString, wxSize> m_savedMinSize;
};
