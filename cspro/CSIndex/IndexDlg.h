#pragma once

#include <zUtilF/PopupInfoLinkCtrl.h>
#include <zUtilF/SrtLstCt.h>


class IndexDlg : public CDialog
{
public:
    IndexDlg(std::wstring initial_dictionary_filename, CWnd* pParent = nullptr);

    enum { IDD = IDD_CSINDEX_DIALOG };

protected:
    DECLARE_MESSAGE_MAP()

    void DoDataExchange(CDataExchange* pDX) override;
    BOOL OnInitDialog() override;

    LRESULT OnUpdateDialogUI(WPARAM wParam, LPARAM lParam);
    afx_msg void OnChangeUpdateDialogUI();

    afx_msg void OnAppAbout();
    afx_msg void OnFileOpen();
    afx_msg void OnFileSaveAs();
    afx_msg void OnDictionaryBrowse();
    afx_msg void OnAddFiles();
    afx_msg void OnRemoveFiles();
    afx_msg void OnClearFiles();
    afx_msg void OnOutputEdit();
    afx_msg void OnOutputBrowse();
    afx_msg void OnRun();

private:
    void SetDefaultPffSettings();
    void UIToPff();
    void AddConnectionStrings(const std::vector<ConnectionString>& connection_strings);
    void OnDropFiles(const std::vector<std::wstring>& filenames);

private:
    const HICON m_hIcon;
    CMenu m_menu;

    PFF m_pff;

    std::wstring m_dictionaryFilename;
    CSortListCtrl m_fileList;
    int m_action;
    int m_autoDeleteIdentical;
    ConnectionString m_outputConnectionString;

    bool m_suggestOutputConnectionString;
    PopupInfoLinkCtrl m_outputConnectionStringOptionsLinkCtrl;
};
