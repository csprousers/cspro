#pragma once

#include <zUtilF/SrtLstCt.h>


class ParadataConcatDlg : public CDialog
{
public:
    ParadataConcatDlg(CWnd* pParent = nullptr);

    enum { IDD = IDD_PARADATACONCAT };

protected:
    DECLARE_MESSAGE_MAP()

    void DoDataExchange(CDataExchange* pDX) override;
    BOOL OnInitDialog() override;

    void OnFileOpen();
    void OnFileSaveAs();
    void OnFileRun();
    void OnAppAbout();
    void OnBrowseOutput();
    void OnAddLogs();
    void OnRemoveLogs();
    void OnClearLogs();

private:
    void UpdateNumberLogsText();
    void AddLogs(std::vector<std::string> file_paths);
    void OnDropFiles(std::vector<std::string> file_paths);

    bool ValidateGuiParameters();
    std::unique_ptr<PFF> CreatePffFromGuiParameters(const std::string& pff_file_path);

private:
    HICON m_hIcon;
    std::string m_outputFilePath;
    CSortListCtrl m_paradataLogList;

    std::set<std::string> m_paradataLogFilePaths;
};
