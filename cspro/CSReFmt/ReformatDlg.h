#pragma once

#include <zUtilO/BasicLogger.h>
#include <zHtml/HtmlViewCtrl.h>
#include <zDictO/DDClass.h>
#include <zDataO/DataRepository.h>
#include <zReformatO/Reformatter.h>


class ReformatDlg : public CDialog
{
public:
    ReformatDlg(CWnd* pParent = nullptr);
    ~ReformatDlg();

    enum { IDD = IDD_REFORMAT };

protected:
    DECLARE_MESSAGE_MAP()

    void DoDataExchange(CDataExchange* pDX) override;
    BOOL OnInitDialog() override;

    LRESULT OnUpdateDialogUI(WPARAM wParam, LPARAM lParam);

    afx_msg void OnAppAbout();
    afx_msg void OnFileOpen();
    afx_msg void OnFileSaveAs();

    afx_msg void OnToggleNames();
    afx_msg void OnToggleShowOnlyDestructiveChanges();
    
    afx_msg void OnTextChange();
    afx_msg void OnInputDictionaryBrowse();
    afx_msg void OnInputDataBrowse();
    afx_msg void OnOutputDictionaryBrowse();
    afx_msg void OnOutputDataBrowse();

    afx_msg void OnReformatData();

private:
    void SetDefaultPffSettings();
    void UIToPff();

    void OnDictionaryBrowse(std::wstring& dictionary_filename, const TCHAR* title_text);
    void OnDataBrowse(ConnectionString& connection_string, bool open_existing, const std::wstring& dictionary_filename, const ConnectionString& other_connection_string);

    const std::shared_ptr<const CDataDict> GetUsableInputDictionary() { return ( m_inputDictionary != nullptr ) ? m_inputDictionary :
                                                                                                                  m_embeddedDictionaryFromInputRepository; }

private:
    HICON m_hIcon;
    CMenu m_menu;
    HtmlViewCtrl m_dictionaryChangesHtml;

    PFF m_pff;
    bool m_showOnlyDestructiveChanges;

    std::wstring m_inputDictionaryFilename;
    ConnectionString m_inputConnectionString;

    std::wstring m_outputDictionaryFilename;
    ConnectionString m_outputConnectionString;

    std::shared_ptr<const CDataDict> m_inputDictionary;
    std::wstring m_lastLoadedInputDictionaryFilename;

    std::shared_ptr<const CDataDict> m_outputDictionary;
    std::wstring m_lastLoadedOutputDictionaryFilename;

    std::shared_ptr<const CDataDict> m_embeddedDictionaryFromInputRepository;
    ConnectionString m_lastLoadedInputConnectionString;

    std::unique_ptr<Reformatter> m_reformatter;
};
