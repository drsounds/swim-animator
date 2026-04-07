#include "MainFrame.h"
#include "View.h"
#include "App.h"
#include "DrawIds.h"
#include "DrawDoc.h"
#include <wx/aui/aui.h>
#include <wx/panel.h>
#include <wx/menu.h>
#include <wx/statusbr.h>
#include <wx/artprov.h>
#include <wx/msgdlg.h>
#include <wx/settings.h>
#include <wx/dcmemory.h>

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
    EVT_MENU(wxID_ABOUT, MainFrame::OnAbout)
    EVT_CLOSE(MainFrame::OnClose)
    EVT_MENU(ID_TOOL_SELECT, MainFrame::OnDrawTool)
    EVT_MENU(ID_TOOL_RECT,   MainFrame::OnDrawTool)
    EVT_MENU(ID_TOOL_CIRCLE, MainFrame::OnDrawTool)
    EVT_MENU(ID_TOOL_TEXT,   MainFrame::OnDrawTool)
    EVT_MENU(ID_TOOL_BEZIER, MainFrame::OnDrawTool)
    EVT_UPDATE_UI(ID_TOOL_SELECT, MainFrame::OnUpdateDrawTool)
    EVT_UPDATE_UI(ID_TOOL_RECT,   MainFrame::OnUpdateDrawTool)
    EVT_UPDATE_UI(ID_TOOL_CIRCLE, MainFrame::OnUpdateDrawTool)
    EVT_UPDATE_UI(ID_TOOL_TEXT,   MainFrame::OnUpdateDrawTool)
    EVT_UPDATE_UI(ID_TOOL_BEZIER, MainFrame::OnUpdateDrawTool)
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
    CreatePropertiesPane();
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
    m_toolbar = new wxToolBar(this, wxID_ANY, wxDefaultPosition, wxDefaultSize, wxTB_HORIZONTAL);
    m_auiMgr.GetArtProvider()->SetMetric(wxAUI_DOCKART_PANE_BORDER_SIZE, 0);
    m_auiMgr.GetArtProvider()->SetColor(wxAUI_DOCKART_BACKGROUND_COLOUR,
                                     wxSystemSettings::GetColour(wxSYS_COLOUR_FRAMEBK));
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


    m_drawToolbar->Realize();

    m_auiMgr.AddPane(m_drawToolbar,
        wxAuiPaneInfo()
            .Name("DrawToolbar")
            .Caption("Draw")
            .ToolbarPane()
            .Left()
            .Row(0)
            .Gripper(true));
}

void MainFrame::CreatePropertiesPane() {
    m_propPanel = new PropPanel(this);
    m_auiMgr.AddPane(m_propPanel,
        wxAuiPaneInfo()
            .Name("Properties")
            .Caption("Properties")
            .Right()
            .BestSize(200, -1)
            .MinSize(160, -1)
            .CloseButton(true)
            .Floatable(true)
            .Show(true));
}

void MainFrame::SetActiveDrawView(DrawView* view) {
    m_activeDrawView = view;
    if (m_propPanel)
        m_propPanel->ShowShape(nullptr, -1);
}

void MainFrame::OnSelectionChanged(DrawDoc* doc, int idx) {
    if (m_propPanel)
        m_propPanel->ShowShape(doc, idx);
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

void MainFrame::OnNotebookPageClose(wxAuiNotebookEvent& event) {
    wxWindow* page = m_notebook->GetPage(event.GetSelection());

    // Determine which view owns this page.
    wxView* view = nullptr;
    if (auto* c = wxDynamicCast(page, ViewCanvas))
        view = c->GetView();
    else if (auto* c = wxDynamicCast(page, DrawCanvas))
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
    }

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
