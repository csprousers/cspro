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

    void OnAppAbout();
    void OnFileOpen();
    void OnFileSaveAs();

    void OnToggleNames();
    void OnToggleShowOnlyDestructiveChanges();

    void OnTextChange();
    void OnInputDictionaryBrowse();
    void OnInputDataBrowse();
    void OnOutputDictionaryBrowse();
    void OnOutputDataBrowse();

    void OnReformatData();

private:
    void SetDefaultPffSettings();
    void UIToPff();

    void OnDictionaryBrowse(std::string& dictionary_file_path, const wchar_t* title_text);

    void OnDataBrowse(ConnectionString& connection_string, bool open_existing,
                      const std::string& dictionary_file_path, const ConnectionString& other_connection_string);

    const std::shared_ptr<const CDataDict> GetUsableInputDictionary() { return ( m_inputDictionary != nullptr ) ? m_inputDictionary :
                                                                                                                  m_embeddedDictionaryFromInputRepository; }

private:
    HICON m_hIcon;
    CMenu m_menu;
    HtmlViewCtrl m_dictionaryChangesHtml;

    PFF m_pff;
    bool m_showOnlyDestructiveChanges;

    std::string m_inputDictionaryFilePath;
    ConnectionString m_inputConnectionString;

    std::string m_outputDictionaryFilePath;
    ConnectionString m_outputConnectionString;

    std::shared_ptr<const CDataDict> m_inputDictionary;
    std::string m_lastLoadedInputDictionaryFilePath;

    std::shared_ptr<const CDataDict> m_outputDictionary;
    std::string m_lastLoadedOutputDictionaryFilePath;

    std::shared_ptr<const CDataDict> m_embeddedDictionaryFromInputRepository;
    ConnectionString m_lastLoadedInputConnectionString;

    std::unique_ptr<Reformatter> m_reformatter;
};
