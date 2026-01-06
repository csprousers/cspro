#pragma once

#include <zUtilF/LoggingListBox.h>


class EditorConfigApplierView : public CFormView
{
    DECLARE_DYNCREATE(EditorConfigApplierView)

protected:
    EditorConfigApplierView();

public:
    ~EditorConfigApplierView();

protected:
    DECLARE_MESSAGE_MAP()

    void DoDataExchange(CDataExchange* pDX) override;

    LRESULT OnThreadComplete(WPARAM wParam, LPARAM lParam);

    void OnCancel();

    void OnDirectorySelect();

    void OnCreateListOfApplicableRules();
    void OnCreateListOfGitIgnoredFiles();
    void OnApplyRules();

private:
    void RunInThread(std::function<void(bool& cancel_flag)> thread_function);

    void SetUpButtonsForThread(bool starting_thread);

    void CreateDataForDirectory();

    void GetPathsInGitIndex(GitRepository& repo, std::vector<std::string>& file_paths) const;
    void GetPathsModifiedSinceLastGitRemoteCommit(GitRepository& repo, std::vector<std::string>& file_paths) const;

    void ParseFilesUsingEditorConfig();

private:
    SettingsDb m_settingsDb;
    std::string m_directory;
    int m_processFilesOption;
    bool m_useDefaultEditorConfig;
    LoggingListBox m_loggingListBox;

    struct Data;
    std::unique_ptr<Data> m_data;
};
