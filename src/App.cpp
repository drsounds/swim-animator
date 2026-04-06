#include "App.h"
#include "MainFrame.h"
#include "Document.h"
#include "View.h"

wxIMPLEMENT_APP(App);

bool App::OnInit() {
    if (!wxApp::OnInit())
        return false;

    SetAppName("Spacely");
    SetVendorName("Example");

    m_docManager = new wxDocManager();

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

    // Limit to one view per document; change to 0 for unlimited.
    m_docManager->SetMaxDocsOpen(10);

    auto* frame = new MainFrame(m_docManager, nullptr, wxID_ANY,
                                GetAppName(),
                                wxDefaultPosition, wxSize(1200, 800));
    frame->Show();
    SetTopWindow(frame);

    return true;
}

int App::OnExit() {
    delete m_docManager;
    return wxApp::OnExit();
}
