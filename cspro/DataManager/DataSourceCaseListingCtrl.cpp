#include "StdAfx.h"
#include "DataSourceCaseListingCtrl.h"
#include "DataSourceFrame.h"
#include <zToolsO/WinClipboard.h>
#include <zUtilO/BCMenu.h>


BEGIN_MESSAGE_MAP(DataSourceCaseListingCtrl, CaseListingCtrl)
    ON_COMMAND(ID_COPY_KEY, OnCopyKey)
    ON_COMMAND(ID_VIEW_CASES, OnViewCases)
    ON_COMMAND_RANGE(ID_FILE_SAVE_CASES, ID_FILE_SAVE_CASES, OnRunTaskFromCaseListing)
    ON_COMMAND_RANGE(ID_FILE_EXPORT_CASES, ID_FILE_EXPORT_CASES, OnRunTaskFromCaseListing)
    ON_COMMAND_RANGE(ID_FILE_EXTRACT_NOTES, ID_FILE_EXTRACT_NOTES, OnRunTaskFromCaseListing)
    ON_COMMAND_RANGE(ID_FILE_EXTRACT_BINARY_DATA, ID_FILE_EXTRACT_BINARY_DATA, OnRunTaskFromCaseListing)
    ON_COMMAND_RANGE(ID_DATA_DELETE_CASE, ID_DATA_DELETE_CASE, OnPostCommand)
END_MESSAGE_MAP()


DataSourceFrame& DataSourceCaseListingCtrl::GetDataSourceFrame()
{
    return *assert_cast<DataSourceFrame*>(GetParentFrame());
}


void DataSourceCaseListingCtrl::OnCaseListingCaseSummariesQueried(const std::variant<size_t, const char*> number_cases_or_exception)
{
    if( std::holds_alternative<const char*>(number_cases_or_exception) )
    {
        ErrorMessage::Display(SO::Concatenate("There was an error querying the number of cases: ",
                                              std::get<const char*>(number_cases_or_exception)));
    }

    GetParentFrame()->PostMessage(UWM::DataManager::UpdateStatusBarCaseCount);
}


void DataSourceCaseListingCtrl::OnCaseListingSelectionsChanged()
{
    GetParentFrame()->PostMessage(UWM::DataManager::UpdateStatusBarSelectedCaseKeyPosition);
    GetParentFrame()->PostMessage(UWM::DataManager::ShowSelectedCases, FALSE);
}


void DataSourceCaseListingCtrl::OnCaseListingDoubleClickAndReturn()
{
    GetParentFrame()->PostMessage(UWM::DataManager::ShowSelectedCases, TRUE);
}


void DataSourceCaseListingCtrl::OnCaseListingContextMenu(CPoint point)
{
    auto build_and_show_menu = [&](const wchar_t* const copy_key_text,
                                   const wchar_t* const view_case_text,
                                   const wchar_t* const save_case_text,
                                   const wchar_t* const export_case_text,
                                   const wchar_t* const extract_notes_text,
                                   const wchar_t* const extract_binary_data_text,
                                   const wchar_t* const delete_case_text)
    {
        ASSERT(( copy_key_text != nullptr ) == ( view_case_text != nullptr ));
        ASSERT(save_case_text != nullptr);
        ASSERT(export_case_text != nullptr);
        ASSERT(extract_notes_text != nullptr);
        ASSERT(extract_binary_data_text != nullptr);

        constexpr UINT grayed_flags = MF_STRING | MF_GRAYED;

        DataSourceFrame& data_source_frame = GetDataSourceFrame();
        const DataSourceDoc& data_source_doc = data_source_frame.GetDataSourceDoc();

        BCMenu popup_menu;
        popup_menu.CreatePopupMenu();

        if( copy_key_text != nullptr )
        {
            popup_menu.AppendMenu(MF_STRING, ID_COPY_KEY, copy_key_text);

            popup_menu.AppendMenu(MF_SEPARATOR);
            popup_menu.AppendMenu(MF_STRING, ID_VIEW_CASES, view_case_text);

            popup_menu.AppendMenu(MF_SEPARATOR);
        }

        popup_menu.AppendMenu(MF_STRING, ID_FILE_SAVE_CASES, save_case_text);
        popup_menu.AppendMenu(MF_STRING, ID_FILE_EXPORT_CASES, export_case_text);

        popup_menu.AppendMenu(MF_SEPARATOR);
        popup_menu.AppendMenu(MF_STRING, ID_FILE_EXTRACT_NOTES, extract_notes_text);

        popup_menu.AppendMenu(data_source_doc.DictionaryUsesBinaryData() ? MF_STRING : grayed_flags, ID_FILE_EXTRACT_BINARY_DATA, extract_binary_data_text);

        if( data_source_frame.IsDataSourceReadWrite() )
        {
            if( delete_case_text != nullptr )
            {
                popup_menu.AppendMenu(MF_SEPARATOR);
                popup_menu.AppendMenu(MF_STRING, ID_DATA_DELETE_CASE, delete_case_text);
            }
        }

        ClientToScreen(&point);
        popup_menu.TrackPopupMenu(TPM_RIGHTBUTTON, point.x, point.y, this);
    };

    const std::vector<std::shared_ptr<const CaseSummary>>& selected_case_summaries = GetSelectedCaseSummaries();

    if( selected_case_summaries.size() == 1 )
    {
        const CaseSummary& case_summary = *selected_case_summaries.front();

        build_and_show_menu(( L"Copy Key: " + TC::ToWide(case_summary.GetSingleLineKey()) ).c_str(),
                            L"View Case in New Window",
                            L"Save Case",
                            L"Export Case",
                            L"Extract Notes from Case",
                            L"Extract Binary Data from Case",
                            case_summary.GetDeleted() ? L"Undelete Case" : L"Delete Case");
    }

    else if( selected_case_summaries.size() > 1 )
    {
        build_and_show_menu(FormatText(L"Copy Keys of Selected Cases (%d)", static_cast<int>(selected_case_summaries.size())).c_str(),
                            L"View Selected Cases in New Windows",
                            L"Save Selected Cases",
                            L"Export Selected Cases",
                            L"Extract Notes from Selected Cases",
                            L"Extract Binary Data from Selected Cases",
                            L"Delete Cases");
    }

    else if( GetNumberCases() > size_t(0) )
    {
        build_and_show_menu(nullptr,
                            nullptr,
                            FormatText(L"Save Filtered Cases (%d)", static_cast<int>(*GetNumberCases())).c_str(),
                            L"Export Filtered Cases",
                            L"Extract Notes from Filtered Cases",
                            L"Extract Binary Data from Filtered Cases",
                            nullptr);

    }
}


bool DataSourceCaseListingCtrl::OnCaseListingDeleteKey()
{
    GetParentFrame()->PostMessage(WM_COMMAND, ID_DATA_DELETE_CASE);
    return true;
}


void DataSourceCaseListingCtrl::OnCopyKey()
{
    const std::string keys = SO::CreateSingleStringUsingCallback(GetSelectedCaseSummaries(),
                                                                 [](const std::shared_ptr<const CaseSummary>& case_summary) { return case_summary->GetKey(); },
                                                                 SO::Newline_lf_sv);
    ASSERT(!keys.empty());

    WinClipboard::PutText(this, keys);
}


void DataSourceCaseListingCtrl::OnViewCases()
{
    GetParentFrame()->PostMessage(UWM::DataManager::ShowSelectedCases, TRUE);
}


void DataSourceCaseListingCtrl::DataSourceCaseListingCtrl::OnRunTaskFromCaseListing(const UINT nID)
{
    GetParentFrame()->PostMessage(UWM::DataManager::RunTaskFromCaseListing, nID);
}


void DataSourceCaseListingCtrl::DataSourceCaseListingCtrl::OnPostCommand(const UINT nID)
{
    GetParentFrame()->PostMessage(WM_COMMAND, nID);
}
