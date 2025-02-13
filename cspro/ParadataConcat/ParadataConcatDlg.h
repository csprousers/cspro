#pragma once

#include <zUtilF/SrtLstCt.h>


class CParadataConcatDlg : public CDialog
{
public:
    CParadataConcatDlg(CWnd* pParent = nullptr);

    enum { IDD = IDD_PARADATACONCAT };

protected:
    DECLARE_MESSAGE_MAP()

    void DoDataExchange(CDataExchange* pDX) override;
    BOOL OnInitDialog() override;

    afx_msg void OnFileOpen();
    afx_msg void OnFileSaveAs();
    afx_msg void OnFileRun();
    afx_msg void OnAppAbout();
    afx_msg void OnBrowseOutput();
    afx_msg void OnAddLogs();
    afx_msg void OnRemoveLogs();
    afx_msg void OnClearLogs();

private:
    void UpdateNumberLogsText();
    void AddLogs(const std::vector<std::wstring>& filenames);
    void OnDropFiles(const std::vector<std::wstring>& filenames);

    bool ValidateGuiParameters();
    std::unique_ptr<PFF> CreatePffFromGuiParameters(NullTerminatedString pff_filename);

private:
    HICON m_hIcon;
    std::wstring m_outputFilename;
    CSortListCtrl m_ParadataLogList;

    std::set<std::wstring> m_paradataLogFilenames;
};
