#include "StdAfx.h"
#include "DictionaryReconcileDlg.h"
#include <zToolsO/WinClipboard.h>
#include <zDictO/DictionaryComparer.h>
#include <zDataO/DictionarySource.h>


BEGIN_MESSAGE_MAP(DictionaryReconcileDlg, CDialog)
    ON_BN_CLICKED(IDC_COPY_TO_CLIPBOARD, OnCopyToClipboard)
END_MESSAGE_MAP()


DictionaryReconcileDlg::DictionaryReconcileDlg(std::string dictionary_name, const ConnectionString& connection_string,
                                               std::vector<DictionaryDifference> differences, CWnd* const pParent/* = nullptr*/)
    :   CDialog(IDD_DICTIONARY_RECONCILE, pParent),
        m_dictionaryName(std::move(dictionary_name)),
        m_dataSourceDisplayText(connection_string.ToDisplayString(true)),
        m_differences(std::move(differences))
{
    ASSERT(!m_differences.empty());
}


void DictionaryReconcileDlg::DoDataExchange(CDataExchange* const pDX)
{
    __super::DoDataExchange(pDX);

    DDX_Control(pDX, IDC_DIFFERENCES_VIEW, m_dictionaryChangesHtml);
}


BOOL DictionaryReconcileDlg::OnInitDialog()
{
    __super::OnInitDialog();

    LogDifferences();

    m_dictionaryChangesHtml.SetContextMenuEnabled(false);
    m_dictionaryChangesHtml.SetHtml(m_differenceLog.ToHtml());

    return TRUE;
}


void DictionaryReconcileDlg::OnCopyToClipboard()
{
    WinClipboard::PutHtml(m_differenceLog.ToHtml(), true);
    WinClipboard::PutText(this, m_differenceLog.ToString(), false);
}


void DictionaryReconcileDlg::LogDifferences()
{
    // group differences by type
    std::map<DictionaryDifference::Type, std::vector<const DictionaryDifference*>> differences_by_type;

    for( const DictionaryDifference& difference : m_differences )
        differences_by_type[difference.type].emplace_back(&difference);


    m_differenceLog.AppendFormatLine("The dictionary '%s' does not match the dictionary used to create the data file '%s'. "
                                     "This could be because you have chosen the wrong data file or because the dictionary was modified.",
                                     m_dictionaryName.c_str(), m_dataSourceDisplayText.c_str());
    m_differenceLog.AppendLine();

    m_differenceLog.AppendLine("If you choose to continue, the following potentially destructive modifications will be "
                               "made to the data file to make it match the dictionary: ");
    m_differenceLog.AppendLine();


    auto log_differences = [&](const DictionaryDifference::Type type, const char* const text1, const char* const text2)
    {
        const auto& difference_lookup = differences_by_type.find(type);

        if( difference_lookup == differences_by_type.cend() )
            return;

        m_differenceLog.AppendLine(BasicLogger::Color::DarkBlue, text1);
        m_differenceLog.AppendLine(BasicLogger::Color::SlateBlue, text2);

        for( const DictionaryDifference* difference : difference_lookup->second )
            m_differenceLog.AppendLine(BasicLogger::Color::Red, "    " + difference->GetDisplayName());

        m_differenceLog.AppendLine();
    };


    log_differences(DictionaryDifference::Type::ItemRemoved,
        "The following items will be removed:",
        "Existing data in these items will be lost");

    log_differences(DictionaryDifference::Type::ItemMovedToDifferentRecord,
        "The following items have moved to a different record and will be removed:",
        "Existing data in these items will be lost");

    log_differences(DictionaryDifference::Type::ItemContentTypeChangedInvalidSometimes,
        "The following items will be changed from alpha to numeric:",
        "Existing alpha values in these items will no longer be readable");

    log_differences(DictionaryDifference::Type::ItemContentTypeChangedInvalidAlways,
        "The following items will have their data type changed to an incompatible type:",
        "Existing data in these items will be lost");

    log_differences(DictionaryDifference::Type::RecordOccurrencesDecreased,
        "Max occurrences of the following records will be decreased:",
        "Occurrences greater than the new max will be lost");

    log_differences(DictionaryDifference::Type::ItemItemSubitemOccurrencesDeceased,
        "The number of occcurences of the following items will be decreased:",
        "Occurrences greater than the new max will be removed");
}


bool DictionaryReconcileDlg::DictionaryChangesIfAnyAreOk(const ConnectionString& connection_string,
                                                         const std::variant<std::reference_wrapper<const CDataDict>,
                                                                            std::reference_wrapper<const std::string>> dictionary_or_dictionary_file_path)
{
    // no need to compare if the data file can't have an embedded dictionary
    if( !DictionarySource::HasEmbeddedDictionary(connection_string) )
        return true;

    cs::shared_or_raw_ptr<const CDataDict> dictionary;

    if( dictionary_or_dictionary_file_path.index() == 0 )
    {
        dictionary = &std::get<0>(dictionary_or_dictionary_file_path).get();
    }

    else
    {
        const std::string& dictionary_file_path = std::get<1>(dictionary_or_dictionary_file_path).get();

        // don't compare if the dictionary doesn't exist (such as when running from a .pen file),
        // or when the dictionary cannot be opened
        if( !PortableFunctions::FileIsRegular(dictionary_file_path) )
            return true;

        try
        {
            dictionary = CDataDict::InstantiateAndOpen(dictionary_file_path);
        }
        catch(...) { return true; }
    }

    ASSERT(dictionary != nullptr);

    // load the embedded dictionary for comparison
    std::unique_ptr<const CDataDict> embedded_dictionary;

    try
    {
        embedded_dictionary = DictionarySource::GetEmbeddedDictionary(connection_string);

        if( embedded_dictionary == nullptr )
            return true;
    }
    catch(...) { return true; }

    // don't compare if the structure of the dictionary hasn't changed;
    // an efficiency would be to first query the dictionary structure
    // from the data file rather than get the entire dictionary
    if( embedded_dictionary->GetStructureMd5() == dictionary->GetStructureMd5() )
        return true;

    // compare the dictionaries
    DictionaryComparer comparer(*embedded_dictionary, *dictionary);
    std::vector<DictionaryDifference> differences = comparer.GetDataRepositorySpecificDifferences(connection_string.GetType());

    if( differences.empty() )
        return true;

    DictionaryReconcileDlg dlg(dictionary->GetName(), connection_string, std::move(differences));

    return ( dlg.DoModal() == IDOK );
}
