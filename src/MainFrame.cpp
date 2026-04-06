#include "MainFrame.h"
#include "View.h"
#include "App.h"
#include "DrawIds.h"
#include <wx/aui/aui.h>
#include <wx/panel.h>
#include <wx/menu.h>
#include <wx/statusbr.h>
#include <wx/artprov.h>
#include <wx/msgdlg.h>
#include <wx/settings.h>
#include <wx/dcmemory.h>

// ---------------------------------------------------------------------------
// FlatToolBarArt – removes the button bevel in the normal (idle) state while
// keeping hover and pressed feedback from the default art provider.
// ---------------------------------------------------------------------------

class FlatToolBarArt : public wxAuiDefaultToolBarArt {
public:
    void DrawBackground(wxDC& dc, wxWindow* wnd, const wxRect& rect) override
    {
        // Fill with the plain window colour – no gradient, no bevel.
        dc.SetBrush(wxSystemSettings::GetColour(wxSYS_COLOUR_BTNFACE));
        dc.SetPen(*wxTRANSPARENT_PEN);
        dc.DrawRectangle(rect);
    }
};

// ---------------------------------------------------------------------------
// Event table
// ---------------------------------------------------------------------------

wxBEGIN_EVENT_TABLE(MainFrame, wxDocParentFrame)
    EVT_AUINOTEBOOK_PAGE_CLOSE(  wxID_ANY, MainFrame::OnNotebookPageClose)
    EVT_AUINOTEBOOK_PAGE_CHANGED(wxID_ANY, MainFrame::OnNotebookPageChanged)
    EVT_MENU(wxID_ABOUT, MainFrame::OnAbout)
    EVT_CLOSE(MainFrame::OnClose)
    EVT_UPDATE_UI(ID_TOOL_SELECT, MainFrame::OnUpdateDrawTool)
    EVT_UPDATE_UI(ID_TOOL_RECT,   MainFrame::OnUpdateDrawTool)
    EVT_UPDATE_UI(ID_TOOL_CIRCLE, MainFrame::OnUpdateDrawTool)
    EVT_UPDATE_UI(ID_TOOL_TEXT,   MainFrame::OnUpdateDrawTool)
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
    CreateStatusBar_();
    CreateAuiPanes();

    m_auiMgr.Update();

    SetMinSize(wxSize(640, 480));
}

MainFrame::~MainFrame() {
    m_auiMgr.UnInit();
}

// ---------------------------------------------------------------------------
// Layout helpers
// ---------------------------------------------------------------------------

void MainFrame::CreateMenuBar() {
    auto* docMgr = wxGetApp().GetDocManager();

    // File menu – wxDocParentFrame / wxDocManager provides the standard items
    // when we pass the doc manager; we just need to add our own extras.
    auto* fileMenu = new wxMenu();
    fileMenu->Append(wxID_NEW);
    fileMenu->Append(wxID_OPEN);
    fileMenu->Append(wxID_CLOSE);
    fileMenu->AppendSeparator();
    fileMenu->Append(wxID_SAVE);
    fileMenu->Append(wxID_SAVEAS);
    fileMenu->AppendSeparator();
    // Recent files will be inserted here by wxDocManager automatically.
    fileMenu->Append(wxID_EXIT);

    auto* editMenu = new wxMenu();
    editMenu->Append(wxID_UNDO);
    editMenu->Append(wxID_REDO);
    editMenu->AppendSeparator();
    editMenu->Append(wxID_CUT);
    editMenu->Append(wxID_COPY);
    editMenu->Append(wxID_PASTE);
    editMenu->Append(wxID_SELECTALL);

    auto* viewMenu = new wxMenu();
    viewMenu->AppendCheckItem(wxID_ANY, "&Toolbar\tCtrl+Shift+T", "Show or hide the toolbar");

    auto* windowMenu = new wxMenu();
    // wxDocManager will populate this with the open document list.

    auto* helpMenu = new wxMenu();
    helpMenu->Append(wxID_ABOUT);

    auto* menuBar = new wxMenuBar();
    menuBar->Append(fileMenu,   "&File");
    menuBar->Append(editMenu,   "&Edit");
    menuBar->Append(viewMenu,   "&View");
    menuBar->Append(windowMenu, "&Window");
    menuBar->Append(helpMenu,   "&Help");

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
    m_toolbar = new wxAuiToolBar(this, wxID_ANY, wxDefaultPosition, wxDefaultSize,
                                 wxAUI_TB_DEFAULT_STYLE | wxAUI_TB_HORZ_LAYOUT | wxAUI_TB_PLAIN_BACKGROUND);

    m_toolbar->AddTool(wxID_NEW,  "New",  wxArtProvider::GetBitmap(wxART_NEW,        wxART_TOOLBAR));
    m_toolbar->AddTool(wxID_OPEN, "Open", wxArtProvider::GetBitmap(wxART_FILE_OPEN,  wxART_TOOLBAR));
    m_toolbar->AddTool(wxID_SAVE, "Save", wxArtProvider::GetBitmap(wxART_FILE_SAVE,  wxART_TOOLBAR));
    m_toolbar->AddSeparator();
    m_toolbar->AddTool(wxID_UNDO, "Undo", wxArtProvider::GetBitmap(wxART_UNDO,       wxART_TOOLBAR));
    m_toolbar->AddTool(wxID_REDO, "Redo", wxArtProvider::GetBitmap(wxART_REDO,       wxART_TOOLBAR));
    m_toolbar->AddSeparator();
    m_toolbar->AddTool(wxID_CUT,  "Cut",  wxArtProvider::GetBitmap(wxART_CUT,        wxART_TOOLBAR));
    m_toolbar->AddTool(wxID_COPY, "Copy", wxArtProvider::GetBitmap(wxART_COPY,       wxART_TOOLBAR));
    m_toolbar->AddTool(wxID_PASTE,"Paste",wxArtProvider::GetBitmap(wxART_PASTE,      wxART_TOOLBAR));
    m_toolbar->AddSeparator();

    auto selectBmp = MakeBmp([](wxMemoryDC& dc) {
        dc.SetPen(*wxBLACK_PEN);
        dc.SetBrush(*wxWHITE_BRUSH);
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

    m_toolbar->AddTool(ID_TOOL_SELECT, "Select", selectBmp, "Select (click shapes to select)", wxITEM_CHECK);
    m_toolbar->AddTool(ID_TOOL_RECT,   "Rect",   rectBmp,   "Draw Rectangle (drag to create)", wxITEM_CHECK);
    m_toolbar->AddTool(ID_TOOL_CIRCLE, "Circle", circleBmp, "Draw Circle (drag to create)",    wxITEM_CHECK);
    m_toolbar->AddTool(ID_TOOL_TEXT,   "Text",   textBmp,   "Draw Text (drag to create)",      wxITEM_CHECK);

    m_toolbar->SetArtProvider(new FlatToolBarArt());
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

    // --- Left pane: placeholder (e.g. project explorer / outline) ---
    auto* sidePanel = new wxPanel(this, wxID_ANY);
    sidePanel->SetBackgroundColour(wxSystemSettings::GetColour(wxSYS_COLOUR_BTNFACE));

    m_auiMgr.AddPane(sidePanel,
        wxAuiPaneInfo()
            .Left()
            .Name("SidePanel")
            .Caption("Explorer")
            .BestSize(wxSize(200, -1))
            .MinSize(wxSize(120, -1))
            .Floatable(false)
            .CloseButton(true)
            .Layer(1));

    // --- Bottom pane: placeholder (e.g. output / log / console) ---
    auto* outputPanel = new wxPanel(this, wxID_ANY);
    outputPanel->SetBackgroundColour(wxSystemSettings::GetColour(wxSYS_COLOUR_BTNFACE));

    m_auiMgr.AddPane(outputPanel,
        wxAuiPaneInfo()
            .Bottom()
            .Name("OutputPanel")
            .Caption("Output")
            .BestSize(wxSize(-1, 150))
            .MinSize(wxSize(-1, 80))
            .Floatable(false)
            .CloseButton(true)
            .Layer(1));
}

// ---------------------------------------------------------------------------
// Event handlers
// ---------------------------------------------------------------------------

void MainFrame::OnUpdateDrawTool(wxUpdateUIEvent& e) {
    // Default: disabled. An active DrawView overrides this via PushEventHandler.
    e.Enable(false);
    e.Check(false);
}

void MainFrame::OnNotebookPageClose(wxAuiNotebookEvent& event) {
    // Find the View that owns the page being closed and close its document.
    wxWindow* page = m_notebook->GetPage(event.GetSelection());
    auto* canvas = wxDynamicCast(page, ViewCanvas);
    if (!canvas) {
        event.Skip();
        return;
    }

    View* view = canvas->GetView();
    if (!view) {
        event.Skip();
        return;
    }

    // Ask the document if it is safe to close (prompts save-changes dialog).
    wxDocument* doc = view->GetDocument();
    if (doc && !doc->Close()) {
        // User cancelled – veto the notebook close.
        event.Veto();
        return;
    }

    // Prevent OnClose from trying to delete the page again (we are already
    // inside the notebook's close sequence).
    // The document close above will invoke View::OnClose(deleteWindow=true),
    // but by this point the notebook is already removing the page, so we
    // simply let it proceed.
    event.Skip();
}

void MainFrame::OnNotebookPageChanged(wxAuiNotebookEvent& event) {
    int sel = event.GetSelection();
    if (sel == wxNOT_FOUND) {
        event.Skip();
        return;
    }

    wxWindow* page = m_notebook->GetPage(sel);
    auto* canvas = wxDynamicCast(page, ViewCanvas);
    if (canvas && canvas->GetView())
        canvas->GetView()->Activate(true);

    event.Skip();
}

void MainFrame::OnAbout(wxCommandEvent& /*event*/) {
    wxMessageBox(
        wxGetApp().GetAppName() + "\n\nA wxWidgets Document/View + wxAUI application.",
        "About " + wxGetApp().GetAppName(),
        wxOK | wxICON_INFORMATION,
        this);
}

void MainFrame::OnClose(wxCloseEvent& event) {
    // Ask the doc manager to close all open documents (prompts for unsaved changes).
    if (!wxGetApp().GetDocManager()->Clear(!event.CanVeto())) {
        event.Veto();
        return;
    }
    event.Skip(); // allow default destruction
}
