#include "SpaDoc.h"
#include "SpaView.h"
#include "SpaSaveAsDialog.h"
#include "SmilDoc.h"
#include <wx/app.h>
#include <wx/cmdproc.h>
#include <wx/wfstream.h>
#include <wx/zipstrm.h>
#include <wx/mstream.h>
#include <wx/xml/xml.h>
#include <wx/filename.h>
#include <wx/filefn.h>
#include <wx/stdpaths.h>
#include <wx/utils.h>
#include <memory>
#include <set>

wxIMPLEMENT_DYNAMIC_CLASS(SpaDoc, wxDocument);

// ---------------------------------------------------------------------------
// Internal helpers
// ---------------------------------------------------------------------------

static wxString MimeFromExt(const wxString& ext) {
    const wxString e = ext.Lower();
    if (e == "png")                  return "image/png";
    if (e == "jpg" || e == "jpeg")   return "image/jpeg";
    if (e == "gif")                  return "image/gif";
    if (e == "svg")                  return "image/svg+xml";
    if (e == "txt")                  return "text/plain";
    if (e == "html" || e == "htm")   return "text/html";
    if (e == "ogg")                  return "audio/ogg";
    if (e == "wav")                  return "audio/wav";
    if (e == "ogv")                  return "video/ogg";
    if (e == "mp4")                  return "video/mp4";
    if (e == "smil")                 return "application/smil+xml";
    if (e == "js")                   return "application/javascript";
    if (e == "sly")                  return "application/x-spacely";
    return "application/octet-stream";
}

// ---------------------------------------------------------------------------
// Lifetime
// ---------------------------------------------------------------------------

SpaDoc::~SpaDoc() {
    delete m_indexSmilDoc;
    if (!m_cacheDir.IsEmpty() && wxFileName::DirExists(m_cacheDir))
        wxFileName::Rmdir(m_cacheDir, wxPATH_RMDIR_RECURSIVE);
}

// ---------------------------------------------------------------------------
// wxDocument overrides
// ---------------------------------------------------------------------------

bool SpaDoc::OnNewDocument() {
    if (!wxDocument::OnNewDocument()) return false;
    delete m_indexSmilDoc;
    m_indexSmilDoc = new SmilDoc();
    m_indexSmilDoc->InitDefaults(24);
    return true;
}

bool SpaDoc::IsModified() const {
    return wxDocument::IsModified() || m_modified;
}

void SpaDoc::Modify(bool modified) {
    wxDocument::Modify(modified);
    m_modified = modified;
}

bool SpaDoc::Save() {
    if (GetFilename().IsEmpty())
        return SaveAs();
    return wxDocument::Save();
}

bool SpaDoc::SaveAs() {
    wxWindow* parent = GetDocumentWindow();
    if (!parent) parent = wxTheApp->GetTopWindow();

    SpaSaveAsDialog dlg(parent, GetFilename(), m_isFolderBundle);
    if (dlg.ShowModal() != wxID_OK) return false;

    m_isFolderBundle = dlg.IsFolderBundle();
    wxString path = dlg.GetPath();
    if (!m_isFolderBundle && !path.EndsWith(".spa"))
        path += ".spa";

    if (!DoSaveDocument(path)) return false;

    SetFilename(path, true);
    SetTitle(wxFileNameFromPath(path));
    GetDocumentManager()->AddFileToHistory(path);
    Modify(false);
    return true;
}

bool SpaDoc::DoOpenDocument(const wxString& filename) {
    m_assets.clear();
    m_sceneOrder.clear();

    m_isFolderBundle = wxFileName::DirExists(filename);
    bool ok = m_isFolderBundle ? LoadFromFolder(filename) : LoadFromZip(filename);
    if (!ok) return false;

    Modify(false);
    UpdateAllViews();
    return true;
}

bool SpaDoc::DoSaveDocument(const wxString& filename) {
    bool ok = m_isFolderBundle ? SaveAsFolder(filename) : SaveAsZip(filename);
    if (!ok) return false;
    Modify(false);
    if (GetCommandProcessor())
        GetCommandProcessor()->MarkAsSaved();
    return true;
}

// ---------------------------------------------------------------------------
// ZIP I/O
// ---------------------------------------------------------------------------

bool SpaDoc::LoadFromZip(const wxString& path) {
    wxFFileInputStream fileIn(path);
    if (!fileIn.IsOk()) return false;
    wxZipInputStream zipIn(fileIn);
    if (!zipIn.IsOk()) return false;

    std::unique_ptr<wxZipEntry> entry;
    while (entry.reset(zipIn.GetNextEntry()), entry != nullptr) {
        if (entry->GetName() == "assets.xml") {
            wxMemoryOutputStream buf;
            zipIn.Read(buf);
            wxMemoryInputStream memIn(buf);
            wxXmlDocument xmlDoc;
            if (xmlDoc.Load(memIn))
                ParseAssetsXml(xmlDoc);
        } else if (entry->GetName() == "index.smil") {
            wxString tmpPath = wxFileName::CreateTempFileName("spa_index");
            {
                wxMemoryOutputStream buf;
                zipIn.Read(buf);
                wxMemoryInputStream memIn(buf);
                wxFFileOutputStream tmpOut(tmpPath);
                if (tmpOut.IsOk())
                    memIn.Read(tmpOut);
            }
            delete m_indexSmilDoc;
            m_indexSmilDoc = new SmilDoc();
            if (!m_indexSmilDoc->DoOpenDocument(tmpPath)) {
                delete m_indexSmilDoc;
                m_indexSmilDoc = nullptr;
            }
            wxRemoveFile(tmpPath);
        }
    }

    if (!m_indexSmilDoc) {
        m_indexSmilDoc = new SmilDoc();
        m_indexSmilDoc->InitDefaults(24);
    }
    return true;
}

bool SpaDoc::SaveAsZip(const wxString& targetPath) {
    // Collect logical paths whose data lives in the cache dir.
    std::set<wxString> cached;
    if (!m_cacheDir.IsEmpty()) {
        for (const auto& a : m_assets) {
            if (wxFileExists(CachePath(a)))
                cached.insert(a.logicalPath);
        }
    }

    // Logical paths we still need to copy from the original ZIP.
    std::set<wxString> needFromZip;
    for (const auto& a : m_assets) {
        if (!cached.count(a.logicalPath))
            needFromZip.insert(a.logicalPath);
    }

    wxString tmpPath = targetPath + ".tmp";
    {
        wxFFileOutputStream fileOut(tmpPath);
        if (!fileOut.IsOk()) return false;
        wxZipOutputStream zipOut(fileOut);

        // Write manifest.
        {
            wxXmlDocument xmlDoc = BuildAssetsXml();
            wxMemoryOutputStream xmlBuf;
            if (!xmlDoc.Save(xmlBuf)) { zipOut.Close(); wxRemoveFile(tmpPath); return false; }
            zipOut.PutNextEntry(new wxZipEntry("assets.xml"));
            wxMemoryInputStream xmlIn(xmlBuf);
            xmlIn.Read(zipOut);
        }

        // Write index.smil.
        if (m_indexSmilDoc) {
            wxString tmpSmil = wxFileName::CreateTempFileName("spa_index");
            if (m_indexSmilDoc->DoSaveDocument(tmpSmil)) {
                wxFFileInputStream smilIn(tmpSmil);
                if (smilIn.IsOk()) {
                    zipOut.PutNextEntry(new wxZipEntry("index.smil"));
                    smilIn.Read(zipOut);
                }
                wxRemoveFile(tmpSmil);
            }
        }

        // Write cached entries.
        for (const auto& lp : cached) {
            // Find the asset struct to get the full cache path.
            for (const auto& a : m_assets) {
                if (a.logicalPath == lp) {
                    wxFFileInputStream src(CachePath(a));
                    if (src.IsOk()) {
                        zipOut.PutNextEntry(new wxZipEntry(lp));
                        src.Read(zipOut);
                    }
                    break;
                }
            }
        }

        // Copy remaining entries from the original ZIP.
        wxString origPath = GetFilename();
        if (!needFromZip.empty() && !origPath.IsEmpty() && wxFileExists(origPath)) {
            wxFFileInputStream origFile(origPath);
            if (origFile.IsOk()) {
                wxZipInputStream origZip(origFile);
                std::unique_ptr<wxZipEntry> entry;
                while (entry.reset(origZip.GetNextEntry()), entry != nullptr) {
                    if (needFromZip.count(entry->GetName())) {
                        zipOut.PutNextEntry(new wxZipEntry(entry->GetName()));
                        origZip.Read(zipOut);
                    }
                }
            }
        }

        if (!zipOut.Close()) { wxRemoveFile(tmpPath); return false; }
    }

    if (wxFileExists(targetPath) && !wxRemoveFile(targetPath)) {
        wxRemoveFile(tmpPath);
        return false;
    }
    return wxRenameFile(tmpPath, targetPath);
}

// ---------------------------------------------------------------------------
// Folder I/O
// ---------------------------------------------------------------------------

bool SpaDoc::LoadFromFolder(const wxString& path) {
    wxString xmlPath = path + wxFILE_SEP_PATH + "assets.xml";
    if (wxFileExists(xmlPath)) {
        wxXmlDocument xmlDoc;
        if (!xmlDoc.Load(xmlPath)) return false;
        ParseAssetsXml(xmlDoc);
    }

    wxString indexPath = path + wxFILE_SEP_PATH + "index.smil";
    delete m_indexSmilDoc;
    m_indexSmilDoc = new SmilDoc();
    if (wxFileExists(indexPath)) {
        if (!m_indexSmilDoc->DoOpenDocument(indexPath)) {
            delete m_indexSmilDoc;
            m_indexSmilDoc = nullptr;
        }
    }
    if (!m_indexSmilDoc) {
        m_indexSmilDoc = new SmilDoc();
        m_indexSmilDoc->InitDefaults(24);
    }
    return true;
}

bool SpaDoc::SaveAsFolder(const wxString& targetPath) {
    if (!wxFileName::DirExists(targetPath))
        wxFileName::Mkdir(targetPath, 0755, wxPATH_MKDIR_FULL);

    // Write assets.xml.
    wxXmlDocument xmlDoc = BuildAssetsXml();
    if (!xmlDoc.Save(targetPath + wxFILE_SEP_PATH + "assets.xml"))
        return false;

    // Write index.smil.
    if (m_indexSmilDoc)
        m_indexSmilDoc->DoSaveDocument(targetPath + wxFILE_SEP_PATH + "index.smil");

    // Copy asset files into the folder.
    for (const auto& a : m_assets) {
        wxFileName dstFn(wxFileName::DirName(targetPath));
        dstFn.AppendDir(wxFileName(a.logicalPath, wxPATH_UNIX).GetPath());
        dstFn.SetFullName(wxFileName(a.logicalPath, wxPATH_UNIX).GetFullName());
        wxFileName::Mkdir(dstFn.GetPath(), 0755, wxPATH_MKDIR_FULL);
        wxString dst = dstFn.GetFullPath();

        wxString src = CachePath(a);
        if (wxFileExists(src)) {
            wxCopyFile(src, dst, true);
        } else if (!m_isFolderBundle) {
            // Extract from original ZIP on-the-fly.
            wxString extracted = ExtractAssetToCache(
                (int)(&a - m_assets.data()));
            if (!extracted.IsEmpty() && extracted != dst)
                wxCopyFile(extracted, dst, true);
        }
        // If source was already a folder bundle, the file is already in place.
    }
    return true;
}

// ---------------------------------------------------------------------------
// XML manifest
// ---------------------------------------------------------------------------

void SpaDoc::ParseAssetsXml(const wxXmlDocument& doc) {
    wxXmlNode* root = doc.GetRoot();
    if (!root) return;

    for (wxXmlNode* child = root->GetChildren(); child; child = child->GetNext()) {
        if (child->GetType() != wxXML_ELEMENT_NODE) continue;

        if (child->GetName() == "scenes") {
            for (wxXmlNode* s = child->GetChildren(); s; s = s->GetNext()) {
                if (s->GetType() == wxXML_ELEMENT_NODE && s->GetName() == "scene") {
                    wxString p = s->GetAttribute("path");
                    if (!p.IsEmpty())
                        m_sceneOrder.push_back(p);
                }
            }
        } else if (child->GetName() == "assets") {
            for (wxXmlNode* a = child->GetChildren(); a; a = a->GetNext()) {
                if (a->GetType() != wxXML_ELEMENT_NODE || a->GetName() != "asset")
                    continue;
                SpaAsset asset;
                asset.logicalPath  = a->GetAttribute("path");
                asset.mimeType     = a->GetAttribute("mime", "application/octet-stream");
                asset.displayName  = wxFileName(asset.logicalPath, wxPATH_UNIX).GetFullName();
                wxString szStr = a->GetAttribute("size", "0");
                unsigned long sz = 0;
                szStr.ToULong(&sz);
                asset.sizeBytes = sz;
                if (!asset.logicalPath.IsEmpty())
                    m_assets.push_back(asset);
            }
        }
    }
}

wxXmlDocument SpaDoc::BuildAssetsXml() const {
    wxXmlDocument doc;
    wxXmlNode* root = new wxXmlNode(nullptr, wxXML_ELEMENT_NODE, "spa-project");
    root->AddAttribute("xmlns",       "https://spacify.se/spa/1.0");
    root->AddAttribute("version",     "1");
    root->AddAttribute("bundle-mode", m_isFolderBundle ? "folder" : "zip");
    doc.SetRoot(root);

    // Use AddChild (append) rather than the parent-pointer constructor (prepend)
    // to preserve insertion order on save/load round-trips.

    if (!m_sceneOrder.empty()) {
        wxXmlNode* scenes = new wxXmlNode(nullptr, wxXML_ELEMENT_NODE, "scenes");
        for (const auto& p : m_sceneOrder) {
            wxXmlNode* s = new wxXmlNode(nullptr, wxXML_ELEMENT_NODE, "scene");
            s->AddAttribute("path", p);
            scenes->AddChild(s);
        }
        root->AddChild(scenes);
    }

    if (!m_assets.empty()) {
        wxXmlNode* assets = new wxXmlNode(nullptr, wxXML_ELEMENT_NODE, "assets");
        for (const auto& a : m_assets) {
            wxXmlNode* node = new wxXmlNode(nullptr, wxXML_ELEMENT_NODE, "asset");
            node->AddAttribute("path", a.logicalPath);
            node->AddAttribute("mime", a.mimeType);
            node->AddAttribute("size", wxString::Format("%llu", a.sizeBytes.GetValue()));
            assets->AddChild(node);
        }
        root->AddChild(assets);
    }

    return doc;
}

// ---------------------------------------------------------------------------
// Asset CRUD
// ---------------------------------------------------------------------------

void SpaDoc::AddAsset(const wxString& srcPath) {
    EnsureCacheDir();

    wxFileName srcFn(srcPath);
    wxString name = srcFn.GetFullName();
    wxString mime = MimeFromExt(srcFn.GetExt());

    // Deduplicate: if an asset with this name already exists, add a suffix.
    wxString lp = "assets/" + name;
    wxString base = srcFn.GetName(), ext = srcFn.GetExt();
    int n = 2;
    while (true) {
        bool clash = false;
        for (const auto& a : m_assets)
            if (a.logicalPath == lp) { clash = true; break; }
        if (!clash) break;
        lp = wxString::Format("assets/%s_%d.%s", base, n++, ext);
    }

    // Copy into cache.
    wxString cacheDst = m_cacheDir + wxFILE_SEP_PATH + "assets";
    wxFileName::Mkdir(cacheDst, 0755, wxPATH_MKDIR_FULL);
    cacheDst += wxFILE_SEP_PATH + wxFileName(lp, wxPATH_UNIX).GetFullName();
    if (!wxCopyFile(srcPath, cacheDst, true)) return;

    SpaAsset a;
    a.logicalPath  = lp;
    a.displayName  = wxFileName(lp, wxPATH_UNIX).GetFullName();
    a.mimeType     = mime;
    a.sizeBytes    = wxFileName::GetSize(srcPath);
    m_assets.push_back(a);

    Modify(true);
    UpdateAllViews();
}

void SpaDoc::RemoveAsset(int idx) {
    if (idx < 0 || idx >= (int)m_assets.size()) return;
    m_assets.erase(m_assets.begin() + idx);
    Modify(true);
    UpdateAllViews();
}

void SpaDoc::RenameAsset(int idx, const wxString& newName) {
    if (idx < 0 || idx >= (int)m_assets.size()) return;
    if (newName.IsEmpty()) return;

    SpaAsset& a = m_assets[idx];
    wxString dir = wxFileName(a.logicalPath, wxPATH_UNIX).GetPath();
    wxString newLp = dir.IsEmpty() ? newName : dir + "/" + newName;

    // Collision check.
    for (int i = 0; i < (int)m_assets.size(); ++i) {
        if (i != idx && m_assets[i].logicalPath == newLp) return;
    }

    // Rename in cache if present.
    wxString oldCache = CachePath(a);
    if (wxFileExists(oldCache)) {
        wxString newCache = m_cacheDir + wxFILE_SEP_PATH +
                            wxFileName(newLp, wxPATH_UNIX).GetFullPath();
        wxFileName::Mkdir(wxFileName(newCache).GetPath(), 0755, wxPATH_MKDIR_FULL);
        if (!wxRenameFile(oldCache, newCache)) return;  // bail — don't corrupt logicalPath
    }

    a.logicalPath = newLp;
    a.displayName = newName;
    Modify(true);
    UpdateAllViews();
}

wxString SpaDoc::ExtractAssetToCache(int idx) {
    if (idx < 0 || idx >= (int)m_assets.size()) return {};
    EnsureCacheDir();

    const SpaAsset& a = m_assets[idx];
    wxString cachePath = CachePath(a);

    if (wxFileExists(cachePath)) return cachePath;

    // Folder bundle: source is already on disk.
    if (m_isFolderBundle) {
        wxString src = GetFilename() + wxFILE_SEP_PATH +
                       wxFileName(a.logicalPath, wxPATH_UNIX).GetFullPath();
        if (wxFileExists(src)) return src;
        return wxEmptyString;
    }

    // ZIP bundle: extract the entry.
    wxString origPath = GetFilename();
    if (origPath.IsEmpty() || !wxFileExists(origPath)) return {};

    wxFFileInputStream fileIn(origPath);
    if (!fileIn.IsOk()) return {};
    wxZipInputStream zipIn(fileIn);

    std::unique_ptr<wxZipEntry> entry;
    while (entry.reset(zipIn.GetNextEntry()), entry != nullptr) {
        if (entry->GetName() == a.logicalPath) {
            wxFileName::Mkdir(wxFileName(cachePath).GetPath(), 0755, wxPATH_MKDIR_FULL);
            wxFFileOutputStream out(cachePath);
            if (!out.IsOk()) return {};
            zipIn.Read(out);
            return cachePath;
        }
    }
    return {};
}

// ---------------------------------------------------------------------------
// Internal helpers
// ---------------------------------------------------------------------------

void SpaDoc::EnsureCacheDir() {
    if (!m_cacheDir.IsEmpty() && wxFileName::DirExists(m_cacheDir)) return;

    wxString base = wxStandardPaths::Get().GetTempDir();
    m_cacheDir = wxString::Format("%s%cspa_%lu",
                                   base, wxFILE_SEP_PATH,
                                   (unsigned long)wxGetProcessId());
    int n = 0;
    while (wxFileName::DirExists(m_cacheDir))
        m_cacheDir = wxString::Format("%s%cspa_%lu_%d",
                                       base, wxFILE_SEP_PATH,
                                       (unsigned long)wxGetProcessId(), ++n);
    wxFileName::Mkdir(m_cacheDir, 0700, wxPATH_MKDIR_FULL);
}

wxString SpaDoc::CachePath(const SpaAsset& a) const {
    if (m_cacheDir.IsEmpty()) return {};
    wxFileName fn(a.logicalPath, wxPATH_UNIX);
    return m_cacheDir + wxFILE_SEP_PATH + fn.GetFullPath();
}
