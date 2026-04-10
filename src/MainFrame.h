#pragma once
#include <wx/docview.h>
#include <wx/aui/aui.h>
#include <wx/aui/auibook.h>
#include <wx/aui/auibar.h>
#include <wx/toolbar.h>
#include "DrawView.h"
#include "PropPanel.h"
#include "ColorSwatchPanel.h"
#include "AssetManagerPanel.h"
#include "HierarchyPanel.h"
#include "ScenePanel.h"
#include "CollapseManager.h"
#include <vector>

class DrawDoc;
class SpaDoc;
class SmilView;

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
    void CreateAnimToolBar();
    void CreateColorSwatchPane();
    void CreateAssetManagerPane();
    void CreatePropertiesPane();
    void CreateHierarchyPane();
    void CreateScenePane();
    void CreateKeyframePane();
    void CreateStatusBar_();
    void CreateAuiPanes();

    // Event handlers
    void OnNotebookPageClose(wxAuiNotebookEvent& event);
    void OnNotebookPageChanged(wxAuiNotebookEvent& event);
    void OnAbout(wxCommandEvent& event);
    void OnFileNew(wxCommandEvent& event);
    void OnFileOpen(wxCommandEvent& event);
    void OnFileClose(wxCommandEvent& event);
    void OnFileSave(wxCommandEvent& event);
    void OnFileSaveAs(wxCommandEvent& event);
    void OnFileExit(wxCommandEvent& event);
    void OnClose(wxCloseEvent& event);

    void OnDrawTool(wxCommandEvent& event);
    void OnUpdateDrawTool(wxUpdateUIEvent& event);

    void OnEditUndo(wxCommandEvent& event);
    void OnEditRedo(wxCommandEvent& event);
    void OnCut(wxCommandEvent& event);
    void OnCopy(wxCommandEvent& event);
    void OnPaste(wxCommandEvent& event);
    void OnSelectAll(wxCommandEvent& event);
    void OnUpdateCutCopy(wxUpdateUIEvent& event);
    void OnUpdatePaste(wxUpdateUIEvent& event);
    void OnUpdateSelectAll(wxUpdateUIEvent& event);

    void OnPaletteImport(wxCommandEvent& event);
    void OnPaletteExport(wxCommandEvent& event);

    void OnSettingsSnap(wxCommandEvent& event);

    void OnSmilRec(wxCommandEvent& event);
    void OnSmilPlayFwd(wxCommandEvent& event);
    void OnSmilPlayBwd(wxCommandEvent& event);
    void OnUpdateSmilTool(wxUpdateUIEvent& event);

public:
    // Draw-tool routing – called by DrawView::OnActivateView.
    void SetActiveDrawView(DrawView* view);

    // Properties + hierarchy panes – called by DrawView when selection changes.
    void OnSelectionChanged(DrawDoc* doc, const std::vector<int>& selection);

    // Called by HierarchyPanel when the user selects shapes in the tree.
    void OnHierarchySelectionChanged(const std::vector<int>& indices);

    // Asset manager pane – called by SpaView when the active project changes.
    void SetActiveSpaDoc(SpaDoc* doc);

    // SMIL animation – called by SmilView on activation.
    void SetActiveSmilView(SmilView* view);
    SmilView* GetActiveSmilView() { return m_activeSmilView; }

    // Called by SmilProxyView when the selection inside a SmilCanvas changes.
    void OnSmilSelectionChanged(SmilView* view, const std::vector<int>& sel);

private:
    wxAuiManager      m_auiMgr;
    wxAuiNotebook*    m_notebook{nullptr};
    wxToolBar*        m_toolbar{nullptr};
    wxToolBar*        m_animToolbar{nullptr};
    ColorSwatchPanel*  m_swatchPanel     {nullptr};
    AssetManagerPanel* m_assetPanel     {nullptr};
    PropPanel*         m_propPanel      {nullptr};
    HierarchyPanel*    m_hierarchyPanel {nullptr};
    ScenePanel*        m_scenePanel     {nullptr};
    DrawView*          m_activeDrawView {nullptr};
    SmilView*          m_activeSmilView {nullptr};
    CollapseManager*   m_collapseManager{nullptr};

    void OnAuiCaptionClick(wxMouseEvent& e);

    wxDECLARE_EVENT_TABLE();
};
