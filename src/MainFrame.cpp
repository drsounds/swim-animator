#include "MainFrame.h"
#include "View.h"
#include "App.h"
#include "DrawIds.h"
#include "SettingsDialog.h"
#include "DrawDoc.h"
#include "SpaDoc.h"
#include "SpaView.h"
#include "SmilDoc.h"
#include "SmilView.h"
#include "Palette.h"
#include <wx/aui/aui.h>
#include <wx/panel.h>
#include <wx/menu.h>
#include <wx/statusbr.h>
#include <wx/artprov.h>
#include <wx/msgdlg.h>
#include <wx/settings.h>
#include <wx/dcmemory.h>
#include <wx/filedlg.h>
#include <wx/stdpaths.h>
#include <wx/filename.h>
#include <wx/dialog.h>
#include <wx/sizer.h>
#include <wx/statbmp.h>
#include <wx/button.h>
#include <wx/filefn.h>
#include <wx/clipbrd.h>
#include <algorithm>

// ---------------------------------------------------------------------------
// BeveledToolBarArt – draws a raised 3-D border on every button in its idle
// state; falls back to the default art for hover, pressed and checked states.
// ---------------------------------------------------------------------------

class BeveledToolBarArt : public wxAuiDefaultToolBarArt {
public:
    void DrawButton(wxDC& dc, wxWindow* wnd,
                    const wxAuiToolBarItem& item, const wxRect& rect) override
    {
        int state = item.GetState();
        bool active = (state & wxAUI_BUTTON_STATE_HOVER)   ||
                      (state & wxAUI_BUTTON_STATE_PRESSED)  ||
                      (state & wxAUI_BUTTON_STATE_CHECKED);

        if (active || (state & wxAUI_BUTTON_STATE_DISABLED)) {
            wxAuiDefaultToolBarArt::DrawButton(dc, wnd, item, rect);
            return;
        }

        // Raised 3-D border
        int x1 = rect.x,             y1 = rect.y;
        int x2 = rect.GetRight() - 1, y2 = rect.GetBottom() - 1;

        dc.SetPen(wxPen(wxSystemSettings::GetColour(wxSYS_COLOUR_3DHIGHLIGHT)));
        dc.DrawLine(x1, y1, x2, y1);   // top
        dc.DrawLine(x1, y1, x1, y2);   // left
        dc.SetPen(wxPen(wxSystemSettings::GetColour(wxSYS_COLOUR_3DSHADOW)));
        dc.DrawLine(x2, y1, x2, y2);   // right
        dc.DrawLine(x1, y2, x2, y2);   // bottom

        wxBitmap bmp = item.GetBitmap();
        if (bmp.IsOk()) {
            int bx = rect.x + (rect.width  - bmp.GetWidth())  / 2;
            int by = rect.y + (rect.height - bmp.GetHeight()) / 2;
            dc.DrawBitmap(bmp, bx, by, true);
        }
    }
};

// ---------------------------------------------------------------------------
// Event table
// ---------------------------------------------------------------------------

wxBEGIN_EVENT_TABLE(MainFrame, wxDocParentFrame)
    EVT_AUINOTEBOOK_PAGE_CLOSE(  wxID_ANY, MainFrame::OnNotebookPageClose)
    EVT_AUINOTEBOOK_PAGE_CHANGED(wxID_ANY, MainFrame::OnNotebookPageChanged)
    EVT_MENU(wxID_ABOUT,        MainFrame::OnAbout)
    EVT_CLOSE(                  MainFrame::OnClose)
    EVT_MENU(ID_TOOL_SELECT,    MainFrame::OnDrawTool)
    EVT_MENU(ID_TOOL_RECT,      MainFrame::OnDrawTool)
    EVT_MENU(ID_TOOL_CIRCLE,    MainFrame::OnDrawTool)
    EVT_MENU(ID_TOOL_TEXT,      MainFrame::OnDrawTool)
    EVT_MENU(ID_TOOL_BEZIER,    MainFrame::OnDrawTool)
    EVT_UPDATE_UI(ID_TOOL_SELECT, MainFrame::OnUpdateDrawTool)
    EVT_UPDATE_UI(ID_TOOL_RECT,   MainFrame::OnUpdateDrawTool)
    EVT_UPDATE_UI(ID_TOOL_CIRCLE, MainFrame::OnUpdateDrawTool)
    EVT_UPDATE_UI(ID_TOOL_TEXT,   MainFrame::OnUpdateDrawTool)
    EVT_UPDATE_UI(ID_TOOL_BEZIER, MainFrame::OnUpdateDrawTool)
    EVT_MENU(wxID_CUT,            MainFrame::OnCut)
    EVT_MENU(wxID_COPY,           MainFrame::OnCopy)
    EVT_MENU(wxID_PASTE,          MainFrame::OnPaste)
    EVT_MENU(wxID_SELECTALL,      MainFrame::OnSelectAll)
    EVT_UPDATE_UI(wxID_CUT,       MainFrame::OnUpdateCutCopy)
    EVT_UPDATE_UI(wxID_COPY,      MainFrame::OnUpdateCutCopy)
    EVT_UPDATE_UI(wxID_PASTE,     MainFrame::OnUpdatePaste)
    EVT_UPDATE_UI(wxID_SELECTALL, MainFrame::OnUpdateSelectAll)
    EVT_MENU(ID_PALETTE_IMPORT,   MainFrame::OnPaletteImport)
    EVT_MENU(ID_PALETTE_EXPORT,   MainFrame::OnPaletteExport)
    EVT_MENU(ID_SETTINGS_SNAP,    MainFrame::OnSettingsSnap)
    EVT_MENU(ID_SMIL_REC,         MainFrame::OnSmilRec)
    EVT_MENU(ID_SMIL_PLAY_FWD,    MainFrame::OnSmilPlayFwd)
    EVT_MENU(ID_SMIL_PLAY_BWD,    MainFrame::OnSmilPlayBwd)
    EVT_UPDATE_UI(ID_SMIL_REC,      MainFrame::OnUpdateSmilTool)
    EVT_UPDATE_UI(ID_SMIL_PLAY_FWD, MainFrame::OnUpdateSmilTool)
    EVT_UPDATE_UI(ID_SMIL_PLAY_BWD, MainFrame::OnUpdateSmilTool)
wxEND_EVENT_TABLE()

// ---------------------------------------------------------------------------
// Construction / destruction
// ---------------------------------------------------------------------------

MainFrame::MainFrame(wxDocManager* manager, wxFrame* parent, wxWindowID id,
                     const wxString& title, const wxPoint& pos,
                     const wxSize& size)
    : wxDocParentFrame(manager, parent, id, title, pos, size)
{
    m_auiMgr.SetManagedWindow(this);

    CreateMenuBar();
    CreateToolBar();
    CreateDrawToolBar();
    CreateAnimToolBar();
    CreateColorSwatchPane();
    CreateAssetManagerPane();
    CreatePropertiesPane();
    CreateHierarchyPane();
    CreateScenePane();
    CreateStatusBar_();
    CreateAuiPanes();

    // Install the custom art provider that draws collapse arrows, then set up
    // the collapse manager.  Art provider must be set before Update().
    m_collapseManager = new CollapseManager(&m_auiMgr);
    m_auiMgr.SetArtProvider(new SpacelyDockArt(m_collapseManager));

    m_auiMgr.Update();

    // Bind AFTER wxAuiManager has connected its own handlers so ours runs
    // first (Bind is LIFO: last-bound executes first).
    Bind(wxEVT_LEFT_DOWN, &MainFrame::OnAuiCaptionClick, this);

    SetMinSize(wxSize(640, 480));
}

MainFrame::~MainFrame() {
    m_auiMgr.UnInit();
    delete m_collapseManager;
}

// ---------------------------------------------------------------------------
// Layout helpers
// ---------------------------------------------------------------------------

void MainFrame::CreateMenuBar() {
    auto* docMgr = wxGetApp().GetDocManager();

    // File menu – wxDocParentFrame / wxDocManager provides the standard items
    // when we pass the doc manager; we just need to add our own extras.
    auto* fileMenu = new wxMenu();
    fileMenu->Append(wxID_NEW,    "&New",      "Create a new document");
    fileMenu->Append(wxID_OPEN,   "&Open",     "Open a document");
    fileMenu->Append(ID_FILE_CLOSE, "&Close",    "Close the current document");
    fileMenu->AppendSeparator();
    fileMenu->Append(wxID_SAVE,   "&Save",     "Save the current document");
    fileMenu->Append(wxID_SAVEAS, "Save &As",  "Save the current document with a new name");
    fileMenu->AppendSeparator();
    fileMenu->Append(ID_PALETTE_IMPORT, "Import Palette...", "Load a GIMP palette (.gpl) file");
    fileMenu->Append(ID_PALETTE_EXPORT, "Export Palette...", "Save the current palette as a GIMP palette (.gpl) file");
    fileMenu->AppendSeparator();
    // Recent files will be inserted here by wxDocManager automatically.
    fileMenu->Append(wxID_EXIT,   "E&xit",     "Exit the application");

    auto* editMenu = new wxMenu();
    editMenu->Append(wxID_UNDO,     "&Undo",       "Undo the last action");
    editMenu->Append(wxID_REDO,     "&Redo",       "Redo the last action");
    editMenu->AppendSeparator();
    editMenu->Append(wxID_CUT,      "Cu&t",        "Cut the selection");
    editMenu->Append(wxID_COPY,     "&Copy",       "Copy the selection");
    editMenu->Append(wxID_PASTE,    "&Paste",      "Paste from clipboard");
    editMenu->Append(ID_EDIT_SELECTALL,"Select &All", "Select all items");

    auto* viewMenu = new wxMenu();
    viewMenu->AppendCheckItem(ID_VIEW_TOOLBAR, "&Toolbar\tCtrl+Shift+T", "Show or hide the toolbar");

    auto* settingsMenu = new wxMenu();
    settingsMenu->Append(ID_SETTINGS_SNAP, "&Snapping...\tCtrl+Shift+,",
                         "Configure snap and alignment settings");

    auto* windowMenu = new wxMenu();
    // wxDocManager will populate this with the open document list.

    auto* helpMenu = new wxMenu();
    helpMenu->Append(ID_HELP_ABOUT, "&About", "About this application");

    auto* menuBar = new wxMenuBar();
    menuBar->Append(fileMenu,     "&File");
    menuBar->Append(editMenu,     "&Edit");
    menuBar->Append(viewMenu,     "&View");
    menuBar->Append(settingsMenu, "&Settings");
    menuBar->Append(windowMenu,   "&Window");
    menuBar->Append(helpMenu,     "&Help");

    SetMenuBar(menuBar);

    // Let the doc manager append recent files into the File menu.
    docMgr->FileHistoryUseMenu(fileMenu);
}

static wxBitmap MakeBmp(void (*draw)(wxMemoryDC&)) {
    wxBitmap bmp(16, 16);
    wxMemoryDC dc(bmp);
    dc.SetBackground(*wxWHITE_BRUSH);
    dc.Clear();
    draw(dc); 
    return bmp;
}

void MainFrame::CreateToolBar() {
    m_toolbar = new wxToolBar(this, wxID_ANY, wxDefaultPosition, wxDefaultSize, wxTB_HORIZONTAL);
    m_auiMgr.GetArtProvider()->SetMetric(wxAUI_DOCKART_PANE_BORDER_SIZE, 0);
    m_auiMgr.GetArtProvider()->SetColor(wxAUI_DOCKART_BACKGROUND_COLOUR,
                                     wxSystemSettings::GetColour(wxSYS_COLOUR_FRAMEBK));
    
    m_toolbar->SetToolBitmapSize(wxSize(16,16));

    m_toolbar->AddTool(wxID_NEW,  "New",  wxArtProvider::GetBitmap(wxART_NEW,        wxART_TOOLBAR, wxSize(16, 16)));
    m_toolbar->AddTool(wxID_OPEN, "Open", wxArtProvider::GetBitmap(wxART_FILE_OPEN,  wxART_TOOLBAR, wxSize(16, 16)));
    m_toolbar->AddTool(wxID_SAVE, "Save", wxArtProvider::GetBitmap(wxART_FILE_SAVE,  wxART_TOOLBAR, wxSize(16, 16)));
    m_toolbar->AddSeparator();
    m_toolbar->AddTool(wxID_UNDO, "Undo", wxArtProvider::GetBitmap(wxART_UNDO,       wxART_TOOLBAR, wxSize(16, 16)));
    m_toolbar->AddTool(wxID_REDO, "Redo", wxArtProvider::GetBitmap(wxART_REDO,       wxART_TOOLBAR, wxSize(16, 16)));
    m_toolbar->AddSeparator();
    m_toolbar->AddTool(wxID_CUT,  "Cut",  wxArtProvider::GetBitmap(wxART_CUT,        wxART_TOOLBAR, wxSize(16, 16)));
    m_toolbar->AddTool(wxID_COPY, "Copy", wxArtProvider::GetBitmap(wxART_COPY,       wxART_TOOLBAR, wxSize(16, 16)));
    m_toolbar->AddTool(wxID_PASTE,"Paste",wxArtProvider::GetBitmap(wxART_PASTE,      wxART_TOOLBAR, wxSize(16, 16)));


    m_toolbar->Realize();

    m_auiMgr.AddPane(m_toolbar,
        wxAuiPaneInfo()
            .Name("Toolbar")
            .Caption("Toolbar")
            .ToolbarPane()
            .Top()
            .LeftDockable(true)
            .RightDockable(true)
            .Floatable(true)
            .Gripper(true));
}

void MainFrame::CreateDrawToolBar() {
    m_drawToolbar = new wxToolBar(this, wxID_ANY, wxDefaultPosition, wxDefaultSize, wxTB_VERTICAL);

    auto selectBmp = MakeBmp([](wxMemoryDC& dc) {
        dc.SetPen(*wxBLACK_PEN);
        dc.SetBrush(*wxTRANSPARENT_BRUSH);
        wxPoint arrow[] = {{3,1},{3,11},{6,8},{8,13},{10,12},{8,7},{12,7}};
        dc.DrawPolygon(7, arrow);
    });
    auto rectBmp = MakeBmp([](wxMemoryDC& dc) {
        dc.SetPen(*wxBLACK_PEN);
        dc.SetBrush(*wxTRANSPARENT_BRUSH);
        dc.DrawRectangle(2, 3, 12, 9);
    });
    auto circleBmp = MakeBmp([](wxMemoryDC& dc) {
        dc.SetPen(*wxBLACK_PEN);
        dc.SetBrush(*wxTRANSPARENT_BRUSH);
        dc.DrawEllipse(2, 2, 12, 12);
    });
    auto textBmp = MakeBmp([](wxMemoryDC& dc) {
        dc.SetFont(wxFont(10, wxFONTFAMILY_DEFAULT,
                          wxFONTSTYLE_NORMAL, wxFONTWEIGHT_BOLD));
        dc.SetTextForeground(*wxBLACK);
        dc.DrawText("T", 4, 1);
    });

    auto bezierBmp = MakeBmp([](wxMemoryDC& dc) {
        // S-shaped cubic Bezier icon
        dc.SetPen(*wxBLACK_PEN);
        wxPoint p[4] = {{2,13},{3,2},{13,14},{14,3}};
        wxPoint prev = p[0];
        for (int i = 1; i <= 24; i++) {
            float t = i/24.0f, mt = 1.0f-t;
            int x = (int)(mt*mt*mt*p[0].x + 3*mt*mt*t*p[1].x + 3*mt*t*t*p[2].x + t*t*t*p[3].x);
            int y = (int)(mt*mt*mt*p[0].y + 3*mt*mt*t*p[1].y + 3*mt*t*t*p[2].y + t*t*t*p[3].y);
            dc.DrawLine(prev, wxPoint(x, y));
            prev = wxPoint(x, y);
        }
        // Anchor dots
        dc.SetBrush(*wxBLACK_BRUSH);
        dc.DrawCircle(p[0], 2);
        dc.DrawCircle(p[3], 2);
    });

    m_drawToolbar->AddTool(ID_TOOL_SELECT, "Select", selectBmp, "Select",       wxITEM_CHECK);
    m_drawToolbar->AddTool(ID_TOOL_RECT,   "Rect",   rectBmp,   "Rectangle",    wxITEM_CHECK);
    m_drawToolbar->AddTool(ID_TOOL_CIRCLE, "Circle", circleBmp, "Circle",       wxITEM_CHECK);
    m_drawToolbar->AddTool(ID_TOOL_TEXT,   "Text",   textBmp,   "Text",         wxITEM_CHECK);
    m_drawToolbar->AddTool(ID_TOOL_BEZIER, "Bezier", bezierBmp, "Bezier Curve", wxITEM_CHECK);

    m_drawToolbar->SetRows(3);
    m_drawToolbar->Realize();

    m_auiMgr.AddPane(m_drawToolbar,
        wxAuiPaneInfo()
            .Name("DrawToolbar")
            .Caption("Draw")
            .Floatable()
            .Left()
            .Layer(2)
            .Row(0));
}

void MainFrame::CreateColorSwatchPane() {
    m_swatchPanel = new ColorSwatchPanel(this, this);
    m_auiMgr.AddPane(m_swatchPanel,
        wxAuiPaneInfo()
            .Name("ColorSwatches")
            .Caption("Color Swatches")
            .Right()
            .Layer(2)
            .BestSize(200, 180)
            .MinSize(160, 80)
            .CloseButton(true)
            .Floatable(true)
            .Show(true));
}

void MainFrame::CreateAssetManagerPane() {
    m_assetPanel = new AssetManagerPanel(this);
    m_auiMgr.AddPane(m_assetPanel,
        wxAuiPaneInfo()
            .Name("AssetManager")
            .Caption("Asset Manager")
            .Right()
            .Layer(2)
            .BestSize(200, 260)
            .MinSize(160, 100)
            .CloseButton(true)
            .Floatable(true)
            .Show(true));
}

void MainFrame::SetActiveSpaDoc(SpaDoc* doc) {
    if (m_assetPanel)
        m_assetPanel->Refresh(doc);
}

void MainFrame::CreatePropertiesPane() {
    m_propPanel = new PropPanel(this);
    m_auiMgr.AddPane(m_propPanel,
        wxAuiPaneInfo()
            .Name("Properties")
            .Caption("Properties")
            .Bottom()
            .Layer(1)
            .BestSize(220, 300)
            .MinSize(160, 100)
            .CloseButton(true)
            .Floatable(true)
            .Show(true));
}

void MainFrame::CreateHierarchyPane() {
    m_hierarchyPanel = new HierarchyPanel(this);
    m_auiMgr.AddPane(m_hierarchyPanel,
        wxAuiPaneInfo()
            .Name("Hierarchy")
            .Caption("Object Hierarchy")
            .Right()
            .Layer(2)
            .BestSize(-1, 160)
            .MinSize(-1, 80)
            .CloseButton(true)
            .Floatable(true)
            .Show(true));
}

void MainFrame::SetActiveDrawView(DrawView* view) {
    m_activeDrawView = view;
    DrawDoc* doc = view ? wxDynamicCast(view->GetDocument(), DrawDoc) : nullptr;
    const std::vector<int> emptySelection;
    if (m_propPanel)
        m_propPanel->ShowShape(doc, -1);
    if (m_hierarchyPanel)
        m_hierarchyPanel->RefreshTree(doc, emptySelection);
    if (m_swatchPanel)
        m_swatchPanel->UpdateColors();
}

void MainFrame::OnSelectionChanged(DrawDoc* doc, const std::vector<int>& selection) {
    int propIdx = (selection.size() == 1) ? selection[0] : -1;
    if (m_propPanel)
        m_propPanel->ShowShape(doc, propIdx);
    if (m_hierarchyPanel)
        m_hierarchyPanel->RefreshTree(doc, selection);
    if (m_swatchPanel)
        m_swatchPanel->UpdateColors();
}

void MainFrame::OnHierarchySelectionChanged(const std::vector<int>& indices) {
    if (!m_activeDrawView || !m_activeDrawView->GetCanvas()) return;
    DrawCanvas* canvas = m_activeDrawView->GetCanvas();
    canvas->SetSelection(indices);

    DrawDoc* doc = wxDynamicCast(m_activeDrawView->GetDocument(), DrawDoc);
    int propIdx = (indices.size() == 1) ? indices[0] : -1;
    if (m_propPanel) m_propPanel->ShowShape(doc, propIdx);
}

void MainFrame::CreateAnimToolBar() {
    m_animToolbar = new wxToolBar(this, wxID_ANY, wxDefaultPosition, wxDefaultSize,
                                  wxTB_HORIZONTAL);

    auto bwdBmp = MakeBmp([](wxMemoryDC& dc) {
        dc.SetPen(*wxBLACK_PEN);
        dc.SetBrush(*wxBLACK_BRUSH);
        // Double-left-arrow (play backward)
        wxPoint a1[] = {{10,3},{3,8},{10,13}};
        wxPoint a2[] = {{15,3},{8,8},{15,13}};
        dc.DrawPolygon(3, a1);
        dc.DrawPolygon(3, a2);
    });
    auto recBmp = MakeBmp([](wxMemoryDC& dc) {
        dc.SetBrush(wxBrush(wxColour(200, 20, 20)));
        dc.SetPen(*wxTRANSPARENT_PEN);
        dc.DrawCircle(8, 8, 5);
    });
    auto fwdBmp = MakeBmp([](wxMemoryDC& dc) {
        dc.SetPen(*wxBLACK_PEN);
        dc.SetBrush(*wxBLACK_BRUSH);
        // Double-right-arrow (play forward)
        wxPoint a1[] = {{6,3},{13,8},{6,13}};
        wxPoint a2[] = {{1,3},{8,8},{1,13}};
        dc.DrawPolygon(3, a1);
        dc.DrawPolygon(3, a2);
    });

    m_animToolbar->AddTool(ID_SMIL_PLAY_BWD, "Play Backward", bwdBmp,
                           "Play Backward (not yet implemented)", wxITEM_NORMAL);
    m_animToolbar->AddTool(ID_SMIL_REC,      "Record",        recBmp,
                           "Toggle keyframe recording", wxITEM_CHECK);
    m_animToolbar->AddTool(ID_SMIL_PLAY_FWD, "Play Forward",  fwdBmp,
                           "Play Forward (not yet implemented)", wxITEM_NORMAL);
    m_animToolbar->Realize();

    m_auiMgr.AddPane(m_animToolbar,
        wxAuiPaneInfo()
            .Name("AnimToolbar")
            .Caption("Animation")
            .ToolbarPane()
            .Top()
            .LeftDockable(false)
            .RightDockable(false)
            .Floatable(false));
}

void MainFrame::CreateScenePane() {
    m_scenePanel = new ScenePanel(this);
    m_auiMgr.AddPane(m_scenePanel,
        wxAuiPaneInfo()
            .Name("Scenes")
            .Caption("Scenes")
            .Right()
            .Layer(2)
            .BestSize(200, 200)
            .MinSize(140, 80)
            .CloseButton(true)
            .Floatable(true)
            .Show(true));
}

void MainFrame::CreateKeyframePane() {
    // KeyframePanel is now embedded inside SmilCanvas, not a separate AUI pane.
}

void MainFrame::SetActiveSmilView(SmilView* view) {
    m_activeSmilView = view;

    if (m_scenePanel) m_scenePanel->Refresh(view);

    // Also wire the draw-tool toolbar to the SmilCanvas's DrawCanvas.
    if (view && view->GetCanvas() && view->GetCanvas()->GetDrawCanvas()) {
        // Route draw-tool events through a proxy DrawView.
        // The SmilCanvas's proxy view is set as the active draw view so
        // the existing draw-tool dispatch in OnDrawTool works naturally.
        DrawCanvas* dc = view->GetCanvas()->GetDrawCanvas();
        // We cast to DrawView* because SmilProxyView IS-A DrawView.
        DrawView* proxy = dynamic_cast<DrawView*>(dc->GetView());
        m_activeDrawView = proxy;
    } else if (view == nullptr) {
        // Clear draw view if no SMIL doc active (unless a DrawView is active).
        if (m_activeSmilView == nullptr && m_activeDrawView != nullptr) {
            // Only clear if the active draw view was a proxy.
            // A real DrawView will clear itself via OnActivateView.
        }
    }
}

void MainFrame::OnSmilSelectionChanged(SmilView* /*view*/, const std::vector<int>& /*sel*/) {
    // Keyframe panel is now embedded in SmilCanvas and refreshes itself.
}

void MainFrame::OnAuiCaptionClick(wxMouseEvent& e) {
    if (m_collapseManager) {
        wxString name = m_collapseManager->HitTestArrow(e.GetPosition());
        if (!name.IsEmpty()) {
            m_collapseManager->Toggle(name);
            return;   // swallow event: prevent wxAUI from starting a drag
        }
    }
    e.Skip();
}

void MainFrame::CreateStatusBar_() {
    auto* sb = wxFrame::CreateStatusBar(3);
    int widths[] = {-1, 120, 80};
    sb->SetStatusWidths(3, widths);
    sb->SetStatusText("Ready");
}

void MainFrame::CreateAuiPanes() {
    // --- Central pane: notebook that holds document views ---
    m_notebook = new wxAuiNotebook(this, wxID_ANY,
                                   wxDefaultPosition, wxDefaultSize,
                                   wxAUI_NB_DEFAULT_STYLE | wxAUI_NB_WINDOWLIST_BUTTON);

    m_auiMgr.AddPane(m_notebook,
        wxAuiPaneInfo()
            .CenterPane()
            .Name("DocumentNotebook")
            .Caption("Documents")
            .PaneBorder(false));


}

// ---------------------------------------------------------------------------
// Event handlers
// ---------------------------------------------------------------------------

void MainFrame::OnDrawTool(wxCommandEvent& e) {
    if (!m_activeDrawView || !m_activeDrawView->GetCanvas()) return;
    DrawCanvas* c = m_activeDrawView->GetCanvas();
    switch (e.GetId()) {
        case ID_TOOL_SELECT: c->SetTool(DrawTool::Select); break;
        case ID_TOOL_RECT:   c->SetTool(DrawTool::Rect);   break;
        case ID_TOOL_CIRCLE: c->SetTool(DrawTool::Circle); break;
        case ID_TOOL_TEXT:   c->SetTool(DrawTool::Text);   break;
        case ID_TOOL_BEZIER: c->SetTool(DrawTool::Bezier); break;
    }
}

void MainFrame::OnUpdateDrawTool(wxUpdateUIEvent& e) {
    if (!m_activeDrawView || !m_activeDrawView->GetCanvas()) {
        e.Enable(false);
        e.Check(false);
        return;
    }
    e.Enable(true);
    DrawTool cur = m_activeDrawView->GetCanvas()->GetTool();
    e.Check((e.GetId() == ID_TOOL_SELECT && cur == DrawTool::Select) ||
            (e.GetId() == ID_TOOL_RECT   && cur == DrawTool::Rect)   ||
            (e.GetId() == ID_TOOL_CIRCLE && cur == DrawTool::Circle) ||
            (e.GetId() == ID_TOOL_TEXT   && cur == DrawTool::Text)   ||
            (e.GetId() == ID_TOOL_BEZIER && cur == DrawTool::Bezier));
}

void MainFrame::OnCut(wxCommandEvent&) {
    if (!m_activeDrawView || !m_activeDrawView->GetCanvas()) return;
    m_activeDrawView->GetCanvas()->CutShapes();
}

void MainFrame::OnCopy(wxCommandEvent&) {
    if (!m_activeDrawView || !m_activeDrawView->GetCanvas()) return;
    m_activeDrawView->GetCanvas()->CopyShapes();
}

void MainFrame::OnPaste(wxCommandEvent&) {
    if (!m_activeDrawView || !m_activeDrawView->GetCanvas()) return;
    m_activeDrawView->GetCanvas()->PasteShapes();
}

void MainFrame::OnSelectAll(wxCommandEvent&) {
    if (!m_activeDrawView || !m_activeDrawView->GetCanvas()) return;
    m_activeDrawView->GetCanvas()->SelectAll();
}

void MainFrame::OnUpdateCutCopy(wxUpdateUIEvent& e) {
    if (!m_activeDrawView || !m_activeDrawView->GetCanvas()) {
        e.Enable(false); return;
    }
    e.Enable(!m_activeDrawView->GetCanvas()->GetSelection().empty());
}

void MainFrame::OnUpdatePaste(wxUpdateUIEvent& e) {
    if (!m_activeDrawView || !m_activeDrawView->GetCanvas()) {
        e.Enable(false); return;
    }
    static wxDataFormat fmt("application/x-spacely-shapes");
    bool hasData = false;
    if (wxClipboard::Get()->Open()) {
        hasData = wxClipboard::Get()->IsSupported(fmt);
        wxClipboard::Get()->Close();
    }
    e.Enable(hasData);
}

void MainFrame::OnUpdateSelectAll(wxUpdateUIEvent& e) {
    e.Enable(m_activeDrawView != nullptr && m_activeDrawView->GetCanvas() != nullptr);
}

void MainFrame::OnNotebookPageClose(wxAuiNotebookEvent& event) {
    wxWindow* page = m_notebook->GetPage(event.GetSelection());

    // Determine which view owns this page.
    wxView* view = nullptr;
    if (auto* c = wxDynamicCast(page, ViewCanvas))
        view = c->GetView();
    else if (auto* c = wxDynamicCast(page, DrawCanvas))
        view = c->GetView();
    else if (auto* c = dynamic_cast<SpaCanvas*>(page))
        view = c->GetView();
    else if (auto* c = dynamic_cast<SmilCanvas*>(page))
        view = c->GetView();

    if (!view) { event.Skip(); return; }

    // Ask the document if it is safe to close (prompts save-changes dialog).
    wxDocument* doc = view->GetDocument();
    if (doc && !doc->Close()) {
        event.Veto();
        return;
    }
    event.Skip();
}

void MainFrame::OnNotebookPageChanged(wxAuiNotebookEvent& event) {
    int sel = event.GetSelection();
    if (sel == wxNOT_FOUND) { event.Skip(); return; }

    wxWindow* page = m_notebook->GetPage(sel);

    if (auto* c = wxDynamicCast(page, ViewCanvas)) {
        if (c->GetView()) c->GetView()->Activate(true);
    } else if (auto* c = wxDynamicCast(page, DrawCanvas)) {
        if (c->GetView()) c->GetView()->Activate(true);
    } else if (auto* c = dynamic_cast<SpaCanvas*>(page)) {
        if (c->GetView()) c->GetView()->Activate(true);
    } else if (auto* c = dynamic_cast<SmilCanvas*>(page)) {
        if (c->GetView()) c->GetView()->Activate(true);
    }

    event.Skip();
}

void MainFrame::OnAbout(wxCommandEvent& /*event*/) {
    wxString exeDir = wxFileName(wxStandardPaths::Get().GetExecutablePath()).GetPath();
    wxString splashPath = exeDir + wxFILE_SEP_PATH + "Splash.png";
    if (!wxFileExists(splashPath))
        splashPath = wxGetCwd() + wxFILE_SEP_PATH + "Splash.png";

    wxDialog dlg(this, wxID_ANY, "About " + wxGetApp().GetAppName(),
                 wxDefaultPosition, wxDefaultSize,
                 wxDEFAULT_DIALOG_STYLE | wxRESIZE_BORDER);

    auto* sizer = new wxBoxSizer(wxVERTICAL);

    if (wxFileExists(splashPath)) {
        wxBitmap bmp(splashPath, wxBITMAP_TYPE_PNG);
        if (bmp.IsOk()) {
            auto* img = new wxStaticBitmap(&dlg, wxID_ANY, bmp);
            sizer->Add(img, 0, wxEXPAND);
        }
    }

    sizer->Add(dlg.CreateButtonSizer(wxOK), 0, wxEXPAND | wxALL, 8);
    dlg.SetSizerAndFit(sizer);
    dlg.Centre();
    dlg.ShowModal();
}

void MainFrame::OnClose(wxCloseEvent& event) {
    // Ask the doc manager to close all open documents (prompts for unsaved changes).
    if (!wxGetApp().GetDocManager()->Clear(!event.CanVeto())) {
        event.Veto();
        return;
    }
    event.Skip(); // allow default destruction
}

void MainFrame::OnPaletteImport(wxCommandEvent& /*event*/) {
    wxFileDialog dlg(this, "Import Palette", "", "",
                     "GIMP Palette (*.gpl)|*.gpl|All files (*.*)|*.*",
                     wxFD_OPEN | wxFD_FILE_MUST_EXIST);
    if (dlg.ShowModal() != wxID_OK) return;

    Palette loaded;
    if (!LoadGplPalette(dlg.GetPath(), loaded)) {
        wxMessageBox("Failed to load palette file.", "Import Palette",
                     wxOK | wxICON_ERROR, this);
        return;
    }
    wxGetApp().GetPalette() = std::move(loaded);
    wxGetApp().SavePaletteToConfig();
    if (m_swatchPanel) m_swatchPanel->RefreshPalette();
}

void MainFrame::OnPaletteExport(wxCommandEvent& /*event*/) {
    wxFileDialog dlg(this, "Export Palette", "", "palette.gpl",
                     "GIMP Palette (*.gpl)|*.gpl|All files (*.*)|*.*",
                     wxFD_SAVE | wxFD_OVERWRITE_PROMPT);
    if (dlg.ShowModal() != wxID_OK) return;

    if (!SaveGplPalette(dlg.GetPath(), wxGetApp().GetPalette())) {
        wxMessageBox("Failed to save palette file.", "Export Palette",
                     wxOK | wxICON_ERROR, this);
    }
}

void MainFrame::OnSmilRec(wxCommandEvent& /*event*/) {
    if (!m_activeSmilView) return;
    SmilDoc* doc = wxDynamicCast(m_activeSmilView->GetDocument(), SmilDoc);
    if (!doc) return;
    doc->SetRecMode(!doc->GetRecMode());
}

void MainFrame::OnSmilPlayFwd(wxCommandEvent& /*event*/) {
    // Playback not yet implemented; button is present as a placeholder.
    wxMessageBox("Playback will be implemented in a future version.", "Play Forward",
                 wxOK | wxICON_INFORMATION, this);
}

void MainFrame::OnSmilPlayBwd(wxCommandEvent& /*event*/) {
    wxMessageBox("Playback will be implemented in a future version.", "Play Backward",
                 wxOK | wxICON_INFORMATION, this);
}

void MainFrame::OnUpdateSmilTool(wxUpdateUIEvent& e) {
    SmilDoc* doc = m_activeSmilView
        ? wxDynamicCast(m_activeSmilView->GetDocument(), SmilDoc) : nullptr;
    e.Enable(doc != nullptr);
    if (e.GetId() == ID_SMIL_REC && doc)
        e.Check(doc->GetRecMode());
}

void MainFrame::OnSettingsSnap(wxCommandEvent& /*event*/) {
    SettingsDialog dlg(this, wxGetApp().GetSnapSettings());
    if (dlg.ShowModal() == wxID_OK) {
        wxGetApp().GetSnapSettings() = dlg.GetSettings();
        wxGetApp().GetSnapSettings().SaveToXml(SnapSettings::DefaultPath());
    }
}
