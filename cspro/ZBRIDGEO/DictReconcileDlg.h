#pragma once

#include <zUtilO/BasicLogger.h>
#include <zHtml/HtmlViewCtrl.h>
#include <zDictO/DictionaryComparer.h>


class DictReconcileDlg : public CDialog
{
public:
    DictReconcileDlg(std::wstring dictionary_name, const ConnectionString& connection_string,
                     std::vector<DictionaryDifference> differences, CWnd* pParent = nullptr);

    enum { IDD = IDD_DICT_RECONCILE };

protected:
    DECLARE_MESSAGE_MAP()

    void DoDataExchange(CDataExchange* pDX) override;
    BOOL OnInitDialog() override;

    void OnCopyToClipboard();

private:
    void LogDifferences();

private:
    const std::wstring m_dictionaryName;
    const std::wstring m_filename;
    const std::vector<DictionaryDifference> m_differences;

    HtmlViewCtrl m_dictionaryChangesHtml;
    BasicLogger m_differenceLog;
};
