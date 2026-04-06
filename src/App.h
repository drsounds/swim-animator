#pragma once
#include <wx/app.h>
#include <wx/docview.h>

class MainFrame;

class App : public wxApp {
public:
    // Called before GTK is initialised — used to inject the Trinity GTK2 rc.
    bool Initialize(int& argc, wxChar** argv) override;

    bool OnInit() override;
    int  OnExit() override;

    wxDocManager* GetDocManager() { return m_docManager; }

private:
    wxDocManager* m_docManager{nullptr};
};

wxDECLARE_APP(App);
