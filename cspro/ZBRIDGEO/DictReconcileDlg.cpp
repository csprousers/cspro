#include "StdAfx.h"
#include "DictReconcileDlg.h"
#include <zToolsO/WinClipboard.h>


BEGIN_MESSAGE_MAP(DictReconcileDlg, CDialog)
    ON_BN_CLICKED(IDC_COPY_TO_CLIPBOARD, OnCopyToClipboard)
END_MESSAGE_MAP()


DictReconcileDlg::DictReconcileDlg(std::wstring dictionary_name, const ConnectionString& connection_string,
                                   std::vector<DictionaryDifference> differences, CWnd* pParent/* = nullptr*/)
    :   CDialog(DictReconcileDlg::IDD, pParent),
        m_dictionaryName(std::move(dictionary_name)),
        m_filename(connection_string.GetFilename()),
        m_differences(std::move(differences))
{
    ASSERT(!m_differences.empty());
}


void DictReconcileDlg::DoDataExchange(CDataExchange* pDX)
{
    CDialog::DoDataExchange(pDX);

    DDX_Control(pDX, IDC_DIFFERENCES_VIEW, m_dictionaryChangesHtml);
}


BOOL DictReconcileDlg::OnInitDialog()
{
    CDialog::OnInitDialog();

    LogDifferences();

    m_dictionaryChangesHtml.SetContextMenuEnabled(false);
    m_dictionaryChangesHtml.SetHtml(m_differenceLog.ToHtml());

    return TRUE;
}


void DictReconcileDlg::OnCopyToClipboard()
{
    WinClipboard::PutHtml(m_differenceLog.ToHtml(), true);
    WinClipboard::PutText(this, m_differenceLog.ToString(), false);
}


void DictReconcileDlg::LogDifferences()
{
    // group differences by type
    std::map<DictionaryDifference::Type, std::vector<const DictionaryDifference*>> differences_by_type;

    for( const DictionaryDifference& difference : m_differences )
        differences_by_type[difference.type].emplace_back(&difference);


    m_differenceLog.AppendFormatLine(_T("The dictionary '%s' does not match the dictionary used to create the data file '%s'. ")
                                     _T("This could be because you have chosen the wrong data file or because the dictionary was modified."),
                                     m_dictionaryName.c_str(), PortableFunctions::PathGetFilename(m_filename));
    m_differenceLog.AppendLine();

    m_differenceLog.AppendLine(_T("If you choose to continue, the following potentially destructive modifications will be ")
                               _T("made to the data file to make it match the dictionary: "));
    m_differenceLog.AppendLine();


    auto log_differences = [&](const DictionaryDifference::Type type, const TCHAR* text1, const TCHAR* text2)
    {
        const auto& difference_lookup = differences_by_type.find(type);

        if( difference_lookup == differences_by_type.cend() )
            return;

        m_differenceLog.AppendLine(BasicLogger::Color::DarkBlue, text1);
        m_differenceLog.AppendLine(BasicLogger::Color::SlateBlue, text2);

        for( const DictionaryDifference* difference : difference_lookup->second )
            m_differenceLog.AppendFormatLine(BasicLogger::Color::Red, _T("    %s"), difference->GetDisplayName().c_str());

        m_differenceLog.AppendLine();
    };


    log_differences(DictionaryDifference::Type::ItemRemoved,
        _T("The following items will be removed:"),
        _T("Existing data in these items will be lost"));

    log_differences(DictionaryDifference::Type::ItemMovedToDifferentRecord,
        _T("The following items have moved to a different record and will be removed:"),
        _T("Existing data in these items will be lost"));

    log_differences(DictionaryDifference::Type::ItemContentTypeChangedInvalidSometimes,
        _T("The following items will be changed from alpha to numeric:"),
        _T("Existing alpha values in these items will no longer be readable"));

    log_differences(DictionaryDifference::Type::ItemContentTypeChangedInvalidAlways,
        _T("The following items will have their data type changed to an incompatible type:"),
        _T("Existing data in these items will be lost"));

    log_differences(DictionaryDifference::Type::RecordOccurrencesDecreased,
        _T("Max occurrences of the following records will be decreased:"),
        _T("Occurrences greater than the new max will be lost"));

    log_differences(DictionaryDifference::Type::ItemItemSubitemOccurrencesDeceased,
        _T("The number of occcurences of the following items will be decreased:"),
        _T("Occurrences greater than the new max will be removed"));
}
