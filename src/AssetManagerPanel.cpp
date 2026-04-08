#include "AssetManagerPanel.h"
#include "DrawIds.h"
#include <wx/sizer.h>
#include <wx/filedlg.h>
#include <wx/textdlg.h>
#include <wx/msgdlg.h>
#include <wx/utils.h>
#include <wx/filename.h>
#include <map>

// ---------------------------------------------------------------------------
// Helpers
// ---------------------------------------------------------------------------

static wxString CategoryFromMime(const wxString& mime) {
    if (mime.StartsWith("image/"))          return "Images";
    if (mime.StartsWith("audio/"))          return "Audio";
    if (mime.StartsWith("video/"))          return "Video";
    if (mime == "text/plain")              return "Text";
    if (mime == "text/html")               return "Documents";
    if (mime == "application/javascript")  return "Scripts";
    if (mime == "application/smil+xml")    return "Scenes";
    return "Other";
}

// ---------------------------------------------------------------------------
// Construction
// ---------------------------------------------------------------------------

AssetManagerPanel::AssetManagerPanel(wxWindow* parent)
    : wxPanel(parent, wxID_ANY)
{
    auto* outer = new wxBoxSizer(wxVERTICAL);

    // "No project" state
    m_noDocLabel = new wxStaticText(this, wxID_ANY, "No project open",
                                    wxDefaultPosition, wxDefaultSize,
                                    wxALIGN_CENTER_HORIZONTAL);
    outer->Add(m_noDocLabel, 1, wxALIGN_CENTER | wxALL, 10);

    // Content panel (shown when a SpaDoc is active)
    m_contentPanel = new wxPanel(this, wxID_ANY);
    auto* contentSizer = new wxBoxSizer(wxVERTICAL);
    m_contentPanel->SetSizer(contentSizer);

    // Button row
    auto* btnRow = new wxBoxSizer(wxHORIZONTAL);
    m_addBtn    = new wxButton(m_contentPanel, ID_ASSET_ADD,    "+",  wxDefaultPosition, wxSize(28, 24));
    m_removeBtn = new wxButton(m_contentPanel, ID_ASSET_REMOVE, "-",  wxDefaultPosition, wxSize(28, 24));
    m_renameBtn = new wxButton(m_contentPanel, ID_ASSET_RENAME, "R",  wxDefaultPosition, wxSize(28, 24));
    m_addBtn->SetToolTip("Add asset");
    m_removeBtn->SetToolTip("Remove selected asset");
    m_renameBtn->SetToolTip("Rename selected asset");
    btnRow->Add(m_addBtn,    0, wxRIGHT, 2);
    btnRow->Add(m_removeBtn, 0, wxRIGHT, 2);
    btnRow->Add(m_renameBtn, 0);
    contentSizer->Add(btnRow, 0, wxALL, 4);

    // Tab book with two tree views
    m_viewBook = new wxNotebook(m_contentPanel, wxID_ANY,
                                wxDefaultPosition, wxDefaultSize, wxNB_TOP);

    m_storageTree = new wxTreeCtrl(m_viewBook, wxID_ANY,
                                   wxDefaultPosition, wxDefaultSize,
                                   wxTR_HAS_BUTTONS | wxTR_LINES_AT_ROOT |
                                   wxTR_HIDE_ROOT | wxBORDER_NONE);
    m_typeTree    = new wxTreeCtrl(m_viewBook, wxID_ANY,
                                   wxDefaultPosition, wxDefaultSize,
                                   wxTR_HAS_BUTTONS | wxTR_LINES_AT_ROOT |
                                   wxTR_HIDE_ROOT | wxBORDER_NONE);

    m_viewBook->AddPage(m_storageTree, "Storage", true);
    m_viewBook->AddPage(m_typeTree,    "By Type");

    contentSizer->Add(m_viewBook, 1, wxEXPAND);
    outer->Add(m_contentPanel, 1, wxEXPAND);

    SetSizer(outer);

    // Start in "no project" state
    m_noDocLabel->Show(true);
    m_contentPanel->Show(false);

    // Events
    m_addBtn->Bind(wxEVT_BUTTON, &AssetManagerPanel::OnAdd, this);
    m_removeBtn->Bind(wxEVT_BUTTON, &AssetManagerPanel::OnRemove, this);
    m_renameBtn->Bind(wxEVT_BUTTON, &AssetManagerPanel::OnRename, this);
    m_storageTree->Bind(wxEVT_TREE_ITEM_ACTIVATED, &AssetManagerPanel::OnItemActivated, this);
    m_typeTree->Bind(wxEVT_TREE_ITEM_ACTIVATED, &AssetManagerPanel::OnItemActivated, this);
}

// ---------------------------------------------------------------------------
// Public interface
// ---------------------------------------------------------------------------

void AssetManagerPanel::Refresh(SpaDoc* doc) {
    m_doc = doc;
    const bool hasDoc = (doc != nullptr);

    GetSizer()->Show(m_noDocLabel,   !hasDoc);
    GetSizer()->Show(m_contentPanel,  hasDoc);

    if (hasDoc) {
        PopulateStorageTree();
        PopulateTypeTree();
    }

    Layout();
    wxPanel::Refresh();
}

// ---------------------------------------------------------------------------
// Tree population
// ---------------------------------------------------------------------------

void AssetManagerPanel::PopulateStorageTree() {
    m_storageTree->DeleteAllItems();
    wxTreeItemId root = m_storageTree->AddRoot("Project");

    if (!m_doc) return;

    // Scenes sub-tree
    if (!m_doc->GetSceneOrder().empty()) {
        wxTreeItemId scenesNode = m_storageTree->AppendItem(root, "scenes/");
        for (const auto& p : m_doc->GetSceneOrder()) {
            wxString name = wxFileName(p, wxPATH_UNIX).GetFullName();
            m_storageTree->AppendItem(scenesNode, name, -1, -1,
                new AssetItemData(/*isScene=*/true, p));
        }
        m_storageTree->Expand(scenesNode);
    }

    // Assets – group by directory prefix inside the bundle
    std::map<wxString, wxTreeItemId> dirNodes;
    const auto& assets = m_doc->GetAssets();
    for (int i = 0; i < (int)assets.size(); ++i) {
        const auto& a = assets[i];
        wxString dir = wxFileName(a.logicalPath, wxPATH_UNIX).GetPath();
        if (dir.IsEmpty()) dir = "(root)";

        if (!dirNodes.count(dir)) {
            dirNodes[dir] = m_storageTree->AppendItem(root, dir + "/");
        }
        m_storageTree->AppendItem(dirNodes[dir], a.displayName, -1, -1,
            new AssetItemData(i, a.logicalPath));
    }

    for (auto& [dir, id] : dirNodes)
        m_storageTree->Expand(id);
}

void AssetManagerPanel::PopulateTypeTree() {
    m_typeTree->DeleteAllItems();
    wxTreeItemId root = m_typeTree->AddRoot("Project");

    if (!m_doc) return;

    // Scenes
    if (!m_doc->GetSceneOrder().empty()) {
        wxTreeItemId scenesNode = m_typeTree->AppendItem(root, "Scenes");
        for (const auto& p : m_doc->GetSceneOrder()) {
            wxString name = wxFileName(p, wxPATH_UNIX).GetFullName();
            m_typeTree->AppendItem(scenesNode, name, -1, -1,
                new AssetItemData(/*isScene=*/true, p));
        }
        m_typeTree->Expand(scenesNode);
    }

    // Assets grouped by category
    std::map<wxString, wxTreeItemId> catNodes;
    const auto& assets = m_doc->GetAssets();
    for (int i = 0; i < (int)assets.size(); ++i) {
        const auto& a = assets[i];
        wxString cat = CategoryFromMime(a.mimeType);
        if (!catNodes.count(cat))
            catNodes[cat] = m_typeTree->AppendItem(root, cat);
        m_typeTree->AppendItem(catNodes[cat], a.displayName, -1, -1,
            new AssetItemData(i, a.logicalPath));
    }

    for (auto& [cat, id] : catNodes)
        m_typeTree->Expand(id);
}

// ---------------------------------------------------------------------------
// Event handlers
// ---------------------------------------------------------------------------

void AssetManagerPanel::OnAdd(wxCommandEvent&) {
    if (!m_doc) return;

    wxFileDialog dlg(this, "Add assets to project", "", "",
        "All supported files|*.png;*.jpg;*.jpeg;*.gif;*.svg;*.txt;*.html;*.ogg;*.wav;*.ogv;*.mp4;*.smil;*.js;*.sly|"
        "Images (*.png;*.jpg;*.jpeg;*.gif)|*.png;*.jpg;*.jpeg;*.gif|"
        "Vector (*.svg)|*.svg|"
        "Audio (*.ogg;*.wav)|*.ogg;*.wav|"
        "Video (*.ogv;*.mp4)|*.ogv;*.mp4|"
        "Text / HTML (*.txt;*.html)|*.txt;*.html|"
        "Scripts (*.js)|*.js|"
        "Scenes (*.smil)|*.smil|"
        "All files (*)|*",
        wxFD_OPEN | wxFD_MULTIPLE);

    if (dlg.ShowModal() != wxID_OK) return;

    wxArrayString paths;
    dlg.GetPaths(paths);
    for (const auto& p : paths)
        m_doc->AddAsset(p);

    Refresh(m_doc);
}

void AssetManagerPanel::OnRemove(wxCommandEvent&) {
    if (!m_doc) return;

    // Determine selection from whichever tree page is active.
    wxTreeCtrl* tree = m_viewBook->GetSelection() == 0 ? m_storageTree : m_typeTree;
    wxTreeItemId sel = tree->GetSelection();
    if (!sel.IsOk()) return;

    auto* data = dynamic_cast<AssetItemData*>(tree->GetItemData(sel));
    if (!data || data->assetIdx < 0) return;

    m_doc->RemoveAsset(data->assetIdx);
    Refresh(m_doc);
}

void AssetManagerPanel::OnRename(wxCommandEvent&) {
    if (!m_doc) return;

    wxTreeCtrl* tree = m_viewBook->GetSelection() == 0 ? m_storageTree : m_typeTree;
    wxTreeItemId sel = tree->GetSelection();
    if (!sel.IsOk()) return;

    auto* data = dynamic_cast<AssetItemData*>(tree->GetItemData(sel));
    if (!data || data->assetIdx < 0) return;

    const SpaAsset& asset = m_doc->GetAssets()[data->assetIdx];
    wxTextEntryDialog dlg(this, "New filename:", "Rename Asset", asset.displayName);
    if (dlg.ShowModal() != wxID_OK) return;

    wxString newName = dlg.GetValue().Trim(true).Trim(false);
    if (newName.IsEmpty() || newName == asset.displayName) return;

    m_doc->RenameAsset(data->assetIdx, newName);
    Refresh(m_doc);
}

void AssetManagerPanel::OnItemActivated(wxTreeEvent& event) {
    if (!m_doc) return;

    wxTreeCtrl* tree = wxDynamicCast(event.GetEventObject(), wxTreeCtrl);
    if (!tree) return;

    auto* data = dynamic_cast<AssetItemData*>(tree->GetItemData(event.GetItem()));
    if (!data) return;

    wxString localPath;
    if (data->isScene) {
        if (m_doc->IsFolderBundle()) {
            localPath = wxFileName::DirName(m_doc->GetFilename()).GetFullPath() +
                        wxFILE_SEP_PATH +
                        wxFileName(data->logicalPath, wxPATH_UNIX).GetFullPath();
        } else {
            // Would need to add scene extraction – use the doc manager for now.
            wxMessageBox("Scene editing from the asset manager will be supported in a future version.",
                         "Open Scene", wxOK | wxICON_INFORMATION, this);
            return;
        }
    } else {
        localPath = m_doc->ExtractAssetToCache(data->assetIdx);
    }

    if (localPath.IsEmpty() || !wxFileExists(localPath)) {
        wxMessageBox("Could not locate the asset file.", "Open Asset",
                     wxOK | wxICON_WARNING, this);
        return;
    }
    wxLaunchDefaultApplication(localPath);
}
