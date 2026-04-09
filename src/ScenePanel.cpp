#include "ScenePanel.h"
#include "SmilView.h"
#include "SmilDoc.h"
#include <wx/sizer.h>
#include <wx/textdlg.h>
#include <wx/msgdlg.h>
#include <wx/artprov.h>

// ---------------------------------------------------------------------------
// Event table
// ---------------------------------------------------------------------------

wxBEGIN_EVENT_TABLE(ScenePanel, wxPanel)
    EVT_LISTBOX(ID_SCENE_LIST,      ScenePanel::OnListSelection)
    EVT_MENU(ID_SCENE_ADD,          ScenePanel::OnAdd)
    EVT_MENU(ID_SCENE_REMOVE,       ScenePanel::OnRemove)
    EVT_MENU(ID_SCENE_RENAME,       ScenePanel::OnRename)
    EVT_MENU(ID_SCENE_MOVE_UP,      ScenePanel::OnMoveUp)
    EVT_MENU(ID_SCENE_MOVE_DOWN,    ScenePanel::OnMoveDown)
wxEND_EVENT_TABLE()

// ---------------------------------------------------------------------------
// Construction
// ---------------------------------------------------------------------------

ScenePanel::ScenePanel(wxWindow* parent)
    : wxPanel(parent, wxID_ANY)
{
    auto* sizer = new wxBoxSizer(wxVERTICAL);

    // Toolbar row.
    auto* tb = new wxToolBar(this, wxID_ANY, wxDefaultPosition, wxDefaultSize,
                              wxTB_HORIZONTAL | wxTB_FLAT | wxTB_NODIVIDER);
    tb->AddTool(ID_SCENE_ADD,      "Add",      wxArtProvider::GetBitmap(wxART_ADD_BOOKMARK,    wxART_TOOLBAR, wxSize(16,16)), "Add scene");
    tb->AddTool(ID_SCENE_REMOVE,   "Remove",   wxArtProvider::GetBitmap(wxART_DEL_BOOKMARK,    wxART_TOOLBAR, wxSize(16,16)), "Remove scene");
    tb->AddTool(ID_SCENE_RENAME,   "Rename",   wxArtProvider::GetBitmap(wxART_EDIT,            wxART_TOOLBAR, wxSize(16,16)), "Rename scene");
    tb->AddSeparator();
    tb->AddTool(ID_SCENE_MOVE_UP,  "Up",       wxArtProvider::GetBitmap(wxART_GO_UP,           wxART_TOOLBAR, wxSize(16,16)), "Move scene up");
    tb->AddTool(ID_SCENE_MOVE_DOWN,"Down",     wxArtProvider::GetBitmap(wxART_GO_DOWN,         wxART_TOOLBAR, wxSize(16,16)), "Move scene down");
    tb->Realize();
    sizer->Add(tb, 0, wxEXPAND);

    // Scene list.
    m_list = new wxListBox(this, ID_SCENE_LIST, wxDefaultPosition, wxDefaultSize,
                           0, nullptr, wxLB_SINGLE);
    sizer->Add(m_list, 1, wxEXPAND);

    SetSizer(sizer);
}

// ---------------------------------------------------------------------------
// Public API
// ---------------------------------------------------------------------------

void ScenePanel::Refresh(SmilView* view) {
    m_view = view;
    RebuildList();
}

// ---------------------------------------------------------------------------
// Helpers
// ---------------------------------------------------------------------------

wxString ScenePanel::SceneLabel(int idx) const {
    SmilDoc* doc = m_view ? wxDynamicCast(m_view->GetDocument(), SmilDoc) : nullptr;
    if (!doc || idx < 0 || idx >= (int)doc->GetScenes().size())
        return wxString();
    const SmilScene& sc = doc->GetScenes()[idx];
    int fps = doc->GetFps();
    double durSec = fps > 0 ? (double)sc.durationFrames / fps : 0.0;
    return wxString::Format("%s  (%.1fs)", sc.name, durSec);
}

void ScenePanel::RebuildList() {
    m_list->Clear();
    SmilDoc* doc = m_view ? wxDynamicCast(m_view->GetDocument(), SmilDoc) : nullptr;
    if (!doc) return;

    for (int i = 0; i < (int)doc->GetScenes().size(); ++i)
        m_list->Append(SceneLabel(i));

    int cur = doc->GetCurrentSceneIndex();
    if (cur >= 0 && cur < (int)m_list->GetCount())
        m_list->SetSelection(cur);
}

// ---------------------------------------------------------------------------
// Event handlers
// ---------------------------------------------------------------------------

void ScenePanel::OnListSelection(wxCommandEvent& e) {
    SmilDoc* doc = m_view ? wxDynamicCast(m_view->GetDocument(), SmilDoc) : nullptr;
    if (!doc) return;
    int sel = e.GetSelection();
    if (sel >= 0 && sel < (int)doc->GetScenes().size())
        doc->SetCurrentScene(sel);
}

void ScenePanel::OnAdd(wxCommandEvent&) {
    SmilDoc* doc = m_view ? wxDynamicCast(m_view->GetDocument(), SmilDoc) : nullptr;
    if (!doc) return;

    wxString name = wxGetTextFromUser("Scene name:", "Add Scene",
                                      wxString::Format("Scene %d", (int)doc->GetScenes().size() + 1),
                                      this);
    if (name.IsEmpty()) return;

    SmilScene s;
    s.name = name;
    s.id   = wxString::Format("scene_%d", (int)doc->GetScenes().size() + 1);
    // Start after the last existing scene.
    if (!doc->GetScenes().empty()) {
        const SmilScene& last = doc->GetScenes().back();
        s.startFrame = last.startFrame + last.durationFrames;
    }
    s.durationFrames = doc->GetFps() * 10;

    int idx = doc->AddScene(s);
    doc->SetCurrentScene(idx);
    RebuildList();
}

void ScenePanel::OnRemove(wxCommandEvent&) {
    SmilDoc* doc = m_view ? wxDynamicCast(m_view->GetDocument(), SmilDoc) : nullptr;
    if (!doc || doc->GetScenes().size() <= 1) return;

    int sel = m_list->GetSelection();
    if (sel == wxNOT_FOUND) return;

    if (wxMessageBox("Remove this scene?", "Remove Scene",
                     wxYES_NO | wxICON_QUESTION, this) != wxYES) return;

    doc->RemoveScene(sel);
    RebuildList();
}

void ScenePanel::OnRename(wxCommandEvent&) {
    SmilDoc* doc = m_view ? wxDynamicCast(m_view->GetDocument(), SmilDoc) : nullptr;
    if (!doc) return;
    int sel = m_list->GetSelection();
    if (sel == wxNOT_FOUND || sel >= (int)doc->GetScenes().size()) return;

    wxString cur = doc->GetScenes()[sel].name;
    wxString name = wxGetTextFromUser("New scene name:", "Rename Scene", cur, this);
    if (name.IsEmpty()) return;

    SmilScene s = doc->GetScenes()[sel];
    s.name = name;
    doc->UpdateScene(sel, s);
    RebuildList();
}

void ScenePanel::OnMoveUp(wxCommandEvent&) {
    SmilDoc* doc = m_view ? wxDynamicCast(m_view->GetDocument(), SmilDoc) : nullptr;
    if (!doc) return;
    int sel = m_list->GetSelection();
    if (sel <= 0) return;
    doc->MoveScene(sel, sel - 1);
    RebuildList();
    m_list->SetSelection(sel - 1);
}

void ScenePanel::OnMoveDown(wxCommandEvent&) {
    SmilDoc* doc = m_view ? wxDynamicCast(m_view->GetDocument(), SmilDoc) : nullptr;
    if (!doc) return;
    int sel = m_list->GetSelection();
    if (sel < 0 || sel + 1 >= (int)doc->GetScenes().size()) return;
    doc->MoveScene(sel, sel + 1);
    RebuildList();
    m_list->SetSelection(sel + 1);
}
