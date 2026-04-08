#pragma once
#include <wx/docview.h>
#include <wx/longlong.h>
#include <wx/xml/xml.h>
#include <vector>

// ---------------------------------------------------------------------------
// SpaAsset – metadata for one file stored inside a .spa project bundle.
// Binary content is never loaded eagerly; it is extracted to a cache
// directory on demand via SpaDoc::ExtractAssetToCache().
// ---------------------------------------------------------------------------
struct SpaAsset {
    wxString    logicalPath;    // ZIP entry name / relative folder path, e.g. "assets/hero.png"
    wxString    displayName;    // user-visible label (filename without directory prefix)
    wxString    mimeType;       // "image/png", "audio/ogg", etc.
    wxULongLong sizeBytes{0};
};

// ---------------------------------------------------------------------------
// SpaDoc – wxDocument subclass for .spa "Swim Animator Project" files.
//
// A .spa bundle is either:
//   • a ZIP archive (default)  – the standard single-file distribution format
//   • a plain folder           – useful for version control and direct editing
//
// On open, only assets.xml metadata is parsed into m_assets / m_sceneOrder.
// Binary asset content stays in the bundle until the user double-clicks an
// item, at which point ExtractAssetToCache() copies it to a temp directory.
// ---------------------------------------------------------------------------
class SpaDoc : public wxDocument {
    wxDECLARE_DYNAMIC_CLASS(SpaDoc);
public:
    SpaDoc() = default;
    ~SpaDoc() override;

    // wxDocument overrides
    bool DoSaveDocument(const wxString& filename) override;
    bool DoOpenDocument(const wxString& filename) override;
    bool IsModified() const override;
    void Modify(bool modified) override;
    bool Save() override;
    bool SaveAs() override;

    // Asset CRUD – each method calls Modify(true) + UpdateAllViews() on success
    void     AddAsset(const wxString& srcPath);
    void     RemoveAsset(int idx);
    void     RenameAsset(int idx, const wxString& newName);

    // Returns a local filesystem path to the asset, extracting from the bundle
    // if necessary.  Returns wxEmptyString on failure.
    wxString ExtractAssetToCache(int idx);

    const std::vector<SpaAsset>& GetAssets()     const { return m_assets; }
    const std::vector<wxString>& GetSceneOrder() const { return m_sceneOrder; }
    bool  IsFolderBundle() const  { return m_isFolderBundle; }
    void  SetFolderBundle(bool v) { m_isFolderBundle = v; }

private:
    // I/O helpers
    bool LoadFromZip(const wxString& path);
    bool LoadFromFolder(const wxString& path);
    bool SaveAsZip(const wxString& targetPath);
    bool SaveAsFolder(const wxString& targetPath);

    void           ParseAssetsXml(const wxXmlDocument& doc);
    wxXmlDocument  BuildAssetsXml() const;

    void     EnsureCacheDir();
    wxString CachePath(const SpaAsset& a) const;  // local path in cache dir

    std::vector<SpaAsset>  m_assets;
    std::vector<wxString>  m_sceneOrder;   // logical paths of embedded scene files
    bool                   m_isFolderBundle{false};
    wxString               m_cacheDir;     // OS temp dir created on first use
    bool                   m_modified{false};
};
