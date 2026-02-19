#pragma once

#include "ReleaseCreator.h"
#include <zUtilO/ResizableDlg.h>
#include <zUtilO/TemporaryFile.h>
#include <zEditO/LogicCtrl.h>


class CreateReleaseDlg : public DynamicLayoutResizableDlg
{
public:
    CreateReleaseDlg(std::unique_ptr<ReleaseCreator> release_creator, CWnd* pParent = nullptr);

protected:
    DECLARE_MESSAGE_MAP()

    void DoDataExchange(CDataExchange* pDX) override;
    BOOL OnInitDialog() override;

    std::vector<std::tuple<CWnd*, SizingDirection>> GetDynamicLayoutControls() override;

    void OnViewReadme();

    void OnSelectInstaller32();

    void OnPreviewReleaseNotes();

    void OnOK() override;

private:
    void UpdateAndValidateInputs(bool validate_files);
    void UpdateAndValidateFileInputs();

    void LoadAndSetAsset(const char* type, std::string filename, const std::string& path, bool is_repo_path);

private:
    std::unique_ptr<ReleaseCreator> m_releaseCreator;

    SharableString m_tagName;
    SharableString m_librariesTag;
    SharableString m_versionText;

    std::string m_readmeRepoPath;
    std::string m_installer32BitFilePath;

    std::string m_releaseTitle;
    int m_releaseType;
    CLogicCtrl m_releaseNotesLogicCtrl;

    std::optional<TemporaryFile> m_readmeTemporaryFile;

    std::map<std::string, std::shared_ptr<const BinaryBlock>> m_loadedFiles;
};
