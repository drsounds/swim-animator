#include "App.h"
#include "MainFrame.h"
#include "Document.h"
#include "View.h"
#include "DrawDoc.h"
#include "DrawView.h"
#include "SpaDoc.h"
#include "SpaView.h"
#include "SmilDoc.h"
#include "SmilView.h"
#include "Palette.h"
#include <wx/filefn.h>
#include <wx/filename.h>
#include <wx/stdpaths.h>
#include <wx/splash.h>
#include <wx/utils.h>

wxIMPLEMENT_APP(App);

bool App::Initialize(int& argc, wxChar** argv) {
    // GTK2 is initialised inside wxApp::Initialize(), so we must set
    // GTK2_RC_FILES before calling the base class.
    //
    // Only apply the Trinity rc when the desktop session has not already set
    // the variable (i.e. when the app is launched outside a live TDE session,
    // such as from a terminal or from KDevelop).
    wxString rcFiles;

    return wxApp::Initialize(argc, argv);
}

bool App::OnInit() {
    if (!wxApp::OnInit())
        return false;

    SetAppName("Swim Animator");
    SetVendorName("Alexander Forselius");

    // Load palette: start with the built-in default, then try the saved one.
    m_palette = DefaultPalette();
    {
        wxString dir  = wxStandardPaths::Get().GetUserDataDir();
        wxString path = dir + wxFILE_SEP_PATH + "palette.gpl";
        Palette loaded;
        if (LoadGplPalette(path, loaded))
            m_palette = std::move(loaded);
    }

    // Load snap settings (silently uses defaults if the file is absent).
    m_snapSettings.LoadFromXml(SnapSettings::DefaultPath());

    m_docManager = new wxDocManager();

    new wxDocTemplate(
        m_docManager,
        "Swim Animator Project",
        "*.spa",
        "",
        "spa",
        "SpaDoc",
        "SpaView",
        wxCLASSINFO(SpaDoc),
        wxCLASSINFO(SpaView)
    );

    // Register the document/view template.
    // Extend the wildcard and extension list as the project grows.
    new wxDocTemplate(
        m_docManager,
        "Spacely Document",     // human-readable type name
        "*.sly",                // file wildcard
        "",                     // default dir
        "sly",                  // default extension
        "SpacelyDoc",           // document class name (must match DYNAMIC_CLASS)
        "SpacelyView",          // view class name
        wxCLASSINFO(Document),
        wxCLASSINFO(View)
    );

    new wxDocTemplate(
        m_docManager,
        "Drawing",
        "*.svg",
        "",
        "svg",
        "DrawDoc",
        "DrawView",
        wxCLASSINFO(DrawDoc),
        wxCLASSINFO(DrawView)
    );

    new wxDocTemplate(
        m_docManager,
        "SMIL Animation",
        "*.smil",
        "",
        "smil",
        "SmilDoc",
        "SmilView",
        wxCLASSINFO(SmilDoc),
        wxCLASSINFO(SmilView)
    );

    // Limit to one view per document; change to 0 for unlimited.
    m_docManager->SetMaxDocsOpen(10);

    // Show a splash screen if Splash.png exists next to the executable
    // or in the current working directory.
    {
        wxString exeDir = wxFileName(wxStandardPaths::Get().GetExecutablePath()).GetPath();
        wxString splashPath = exeDir + wxFILE_SEP_PATH + "Splash.png";
        if (!wxFileExists(splashPath))
            splashPath = wxGetCwd() + wxFILE_SEP_PATH + "Splash.png";

        if (wxFileExists(splashPath)) {
            wxBitmap bmp(splashPath, wxBITMAP_TYPE_PNG);
            if (bmp.IsOk()) {
                new wxSplashScreen(bmp,
                    wxSPLASH_CENTRE_ON_SCREEN | wxSPLASH_TIMEOUT,
                    3000, nullptr, wxID_ANY);
                wxYield();
            }
        }
    }

    auto* frame = new MainFrame(m_docManager, nullptr, wxID_ANY,
                                GetAppName(),
                                wxDefaultPosition, wxSize(1200, 800));
    frame->Centre();
    frame->Show();
    SetTopWindow(frame);

    return true;
}

int App::OnExit() {
    SavePaletteToConfig();
    delete m_docManager;
    return wxApp::OnExit();
}

void App::SavePaletteToConfig() {
    wxString dir = wxStandardPaths::Get().GetUserDataDir();
    wxFileName::Mkdir(dir, 0755, wxPATH_MKDIR_FULL);
    SaveGplPalette(dir + wxFILE_SEP_PATH + "palette.gpl", m_palette);
}
