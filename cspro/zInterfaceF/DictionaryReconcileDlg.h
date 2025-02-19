#pragma once

#include <zInterfaceF/zInterfaceF.h>
#include <zUtilO/BasicLogger.h>
#include <zUtilO/ConnectionString.h>
#include <zHtml/HtmlViewCtrl.h>
#include <zDictO/DictionaryComparer.h>


class CLASS_DECL_ZINTERFACEF DictionaryReconcileDlg : public CDialog
{
public:
    DictionaryReconcileDlg(std::string dictionary_name, const ConnectionString& connection_string,
                           std::vector<DictionaryDifference> differences, CWnd* pParent = nullptr);

    // If the data source has an embedded dictionary and there are changes compared with the dictionary,
    // this dialog is shown to confirm that the user is fine with the changes.
    static bool DictionaryChangesIfAnyAreOk(const ConnectionString& connection_string,
                                            std::variant<std::reference_wrapper<const CDataDict>,
                                                         std::reference_wrapper<const std::string>> dictionary_or_dictionary_file_path);

protected:
    DECLARE_MESSAGE_MAP()

    void DoDataExchange(CDataExchange* pDX) override;
    BOOL OnInitDialog() override;

    void OnCopyToClipboard();

private:
    void LogDifferences();

private:
    std::string m_dictionaryName;
    std::string m_dataSourceDisplayText;
    std::vector<DictionaryDifference> m_differences;

    HtmlViewCtrl m_dictionaryChangesHtml;
    BasicLogger m_differenceLog;
};
