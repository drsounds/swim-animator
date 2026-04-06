#pragma once
#include <wx/docview.h>
#include <wx/aui/aui.h>
#include <wx/aui/auibook.h>
#include <wx/aui/auibar.h>
#include <wx/toolbar.h>

// ---------------------------------------------------------------------------
// MainFrame
//
// Top-level application window.  Hosts a wxAuiManager that arranges:
//   - Central pane   : wxAuiNotebook  – one tab per open View
//   - Left pane      : placeholder side panel (project tree, etc.)
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

    wxAuiNotebook* GetNotebook() { return m_notebook; }

private:
    // Layout helpers
    void CreateMenuBar();
    void CreateToolBar();
    void CreateStatusBar_();
    void CreateAuiPanes();

    // Event handlers
    void OnNotebookPageClose(wxAuiNotebookEvent& event);
    void OnNotebookPageChanged(wxAuiNotebookEvent& event);
    void OnAbout(wxCommandEvent& event);
    void OnClose(wxCloseEvent& event);
    void OnUpdateDrawTool(wxUpdateUIEvent& event);

    wxAuiManager   m_auiMgr;
    wxAuiNotebook* m_notebook{nullptr};
    wxAuiToolBar*  m_toolbar{nullptr};

    wxDECLARE_EVENT_TABLE();
};
