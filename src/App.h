#pragma once
#include <wx/app.h>
#include <wx/docview.h>

class MainFrame;

class App : public wxApp {
public:
    bool OnInit() override;
    int  OnExit() override;

    wxDocManager* GetDocManager() { return m_docManager; }

private:
    wxDocManager* m_docManager{nullptr};
};

wxDECLARE_APP(App);
