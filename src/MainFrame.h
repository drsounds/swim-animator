#pragma once
#include <wx/docview.h>
#include <wx/aui/aui.h>
#include <wx/aui/auibook.h>
#include <wx/aui/auibar.h>
#include <wx/toolbar.h>
#include "DrawView.h"
#include "PropPanel.h"
#include "ColorSwatchPanel.h"

class DrawDoc;

// ---------------------------------------------------------------------------
// MainFrame
//
// Top-level application window.  Hosts a wxAuiManager that arranges:
//   - Central pane   : wxAuiNotebook  – one tab per open View
//   - Left pane      : placeholder side panel (project tree, etc.)
//   - Right pane (1) : ColorSwatchPanel – colour palette + FG/BG indicator
//   - Right pane (2) : PropPanel       – editable properties for selection
//   - Bottom pane    : placeholder output / log panel
//
// Derives from wxDocParentFrame so that the Doc/View machinery wires up
// the standard File menu commands automatically.
// ---------------------------------------------------------------------------
class MainFrame : public wxDocParentFrame {
public:
    MainFrame(wxDocManager* manager, wxFrame* parent, wxWindowID id,
              const wxString& title,
              const wxPoint&  pos  = wxDefaultPosition,
              const wxSize&   size = wxDefaultSize);
    ~MainFrame();

    wxAuiNotebook*    GetNotebook()        { return m_notebook; }
    DrawView*         GetActiveDrawView()  { return m_activeDrawView; }

private:
    // Layout helpers
    void CreateMenuBar();
    void CreateToolBar();
    void CreateDrawToolBar();
    void CreateColorSwatchPane();
    void CreatePropertiesPane();
    void CreateStatusBar_();
    void CreateAuiPanes();

    // Event handlers
    void OnNotebookPageClose(wxAuiNotebookEvent& event);
    void OnNotebookPageChanged(wxAuiNotebookEvent& event);
    void OnAbout(wxCommandEvent& event);
    void OnClose(wxCloseEvent& event);

    void OnDrawTool(wxCommandEvent& event);
    void OnUpdateDrawTool(wxUpdateUIEvent& event);

    void OnPaletteImport(wxCommandEvent& event);
    void OnPaletteExport(wxCommandEvent& event);

public:
    // Draw-tool routing – called by DrawView::OnActivateView.
    void SetActiveDrawView(DrawView* view);

    // Properties pane – called by DrawView when selection changes.
    void OnSelectionChanged(DrawDoc* doc, int idx);

private:
    wxAuiManager      m_auiMgr;
    wxAuiNotebook*    m_notebook{nullptr};
    wxToolBar*        m_toolbar{nullptr};
    wxToolBar*        m_drawToolbar{nullptr};
    ColorSwatchPanel* m_swatchPanel{nullptr};
    PropPanel*        m_propPanel{nullptr};
    DrawView*         m_activeDrawView{nullptr};

    wxDECLARE_EVENT_TABLE();
};
