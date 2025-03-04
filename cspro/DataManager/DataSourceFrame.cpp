#include "StdAfx.h"
#include "DataSourceFrame.h"
#include "CaseListingView.h"
#include "DataManager.h"
#include "DataSourceSettings.h"
#include "ExtractCasesTask.h"
#include "SyncHelpers.h"
#include "SynchronizeDlg.h"
#include "TaskRunnerDlg.h"
#include <zDataO/DataRepositoryHelpers.h>
#include <zDataO/DataRepositoryTransaction.h>
#include <zDataO/DataRepositoryUniqueCaseIdentifer.h>


IMPLEMENT_DYNCREATE(DataSourceFrame, CaseHoldingFrame)

BEGIN_MESSAGE_MAP(DataSourceFrame, CaseHoldingFrame)

    ON_WM_MDIACTIVATE()

    // File menu
    ON_COMMAND(ID_FILE_REFRESH, OnFileRefresh)

    ON_COMMAND(ID_FILE_SYNCHRONIZE, OnFileSynchronize)

    ON_COMMAND(ID_FILE_SAVE_DATA, OnFileSaveData)

    // Listing menu
    ON_COMMAND_RANGE(ID_LISTING_SEQUENTIAL, ID_LISTING_INDEXED, OnListingMethod)
    ON_UPDATE_COMMAND_UI_RANGE(ID_LISTING_SEQUENTIAL, ID_LISTING_INDEXED, OnUpdateListingMethod)
    ON_COMMAND(ID_TOGGLE_CASE_ITERATION_METHOD, OnListingToggleMethod)

    ON_COMMAND_RANGE(ID_LISTING_CASES_ALL, ID_LISTING_CASES_DUPLICATE, OnListingStatus)
    ON_UPDATE_COMMAND_UI_RANGE(ID_LISTING_CASES_ALL, ID_LISTING_CASES_DUPLICATE, OnUpdateListingStatus)

    ON_COMMAND_RANGE(ID_LISTING_ASCENDING, ID_LISTING_DESCENDING, OnListingOrder)
    ON_UPDATE_COMMAND_UI_RANGE(ID_LISTING_ASCENDING, ID_LISTING_DESCENDING, OnUpdateListingOrder)

    ON_COMMAND_RANGE(ID_LISTING_CASE_LABEL, ID_LISTING_CASE_KEY, OnListingCaseKeyLabel)
    ON_UPDATE_COMMAND_UI_RANGE(ID_LISTING_CASE_LABEL, ID_LISTING_CASE_KEY, OnUpdateListingCaseKeyLabel)
    ON_COMMAND(ID_TOGGLE_CASE_KEY_LABEL, OnListingToggleCaseKeyLabel)

    // View menu
    ON_COMMAND_RANGE(ID_VIEW_CASE_HTML, ID_VIEW_CASE_QUESTIONNAIRE, OnViewCasePage)

    // View Options menu
    ON_COMMAND_RANGE(ID_VIEW_OPTIONS_LANGUAGE0, ID_VIEW_OPTIONS_LANGUAGE9, OnViewOptionsLanguage)

    // Data menu
    ON_COMMAND(ID_DATA_TOGGLE_READ_ONLY, OnDataToggleReadOnly)
    ON_UPDATE_COMMAND_UI(ID_DATA_TOGGLE_READ_ONLY, OnUpdateDataToggleReadOnly)

    ON_COMMAND(ID_DATA_DELETE_CASE, OnDataDeleteCase)
    ON_UPDATE_COMMAND_UI(ID_DATA_DELETE_CASE, OnUpdateDataSourceIsReadWriteWithCaseSelected)

    // Tools menu
    ON_COMMAND(ID_TOOLS_EXPORT_DATA, OnExportData)
    ON_COMMAND(ID_TOOLS_TABULATE_FREQUENCIES, OnTabulateFrequencies)

    // message handlers
    ON_MESSAGE(UWM::DataManager::UpdateStatusBarCaseCount, OnUpdateStatusBarCaseCount)
    ON_MESSAGE(UWM::DataManager::UpdateStatusBarSelectedCaseKeyPosition, OnUpdateStatusBarSelectedCaseKeyPosition)
    ON_MESSAGE(UWM::DataManager::ShowDefaultPage, OnShowDefaultPage)
    ON_MESSAGE(UWM::DataManager::ProcessConnectionStringParameters, OnProcessConnectionStringParameters)
    ON_MESSAGE(UWM::DataManager::ShowSelectedCases, OnShowSelectedCases)
    ON_MESSAGE(UWM::DataManager::RunTaskFromCaseListing, OnRunTaskFromCaseListing)

END_MESSAGE_MAP()


DataSourceFrame::DataSourceFrame()
    :   m_caseListingView(nullptr),
        m_htmlView(nullptr)
{
}


CaseHoldingDoc& DataSourceFrame::GetCaseHoldingDoc()
{
    return GetDataSourceDoc();
}


HtmlView& DataSourceFrame::GetHtmlView()
{
    ASSERT(m_htmlView != nullptr);
    return *m_htmlView;
}


std::unique_ptr<CaseProvider> DataSourceFrame::CreateCaseProvider()
{
    DataSourceDoc& data_source_doc = GetDataSourceDoc();
    return std::make_unique<DataRepositoryCaseProvider>(data_source_doc.GetSharedDataRepository());
}


std::unique_ptr<CaseProvider> DataSourceFrame::CreateCaseProvider(const std::vector<std::shared_ptr<const CaseSummary>>& selected_case_summaries)
{
    DataSourceDoc& data_source_doc = GetDataSourceDoc();

    // if a single case is selected, we can use the super efficient SingleCaseProvider
    if( selected_case_summaries.size() == 1 )
    {
        ASSERT(data_source_doc.GetCurrentCase() != nullptr &&
               data_source_doc.GetCurrentCase()->GetKey() == selected_case_summaries.front()->GetKey());

        return std::make_unique<SingleCaseProvider>(data_source_doc.GetSharedCurrentCase());
    }

    // otherwise, when no cases are selected (meaning process all cases),
    // or if the number of selected case summaries matches the total number of cases in the case listing,
    // we can use a DataRepositoryCaseProvider, which is more efficient than reading cases one-by-one based on the case summaries
    if( selected_case_summaries.empty() ||
        selected_case_summaries.size() == m_caseListingView->GetCaseListingCtrl().GetNumberCases() )
    {
        return std::make_unique<DataRepositoryCaseProvider>(data_source_doc.GetSharedDataRepository());
    }

    // otherwise, use a SelectiveDataRepositoryCaseProvider
    else
    {
        return std::make_unique<SelectiveDataRepositoryCaseProvider>(data_source_doc.GetSharedDataRepository(), selected_case_summaries);
    }
}


BOOL DataSourceFrame::OnCreateClient(LPCREATESTRUCT /*lpcs*/, CCreateContext* const pContext)
{
    ASSERT(pContext->m_pNewViewClass == RUNTIME_CLASS(HtmlView));

    if( !m_splitterWnd.CreateStatic(this, 1, 2) ||
        !m_splitterWnd.CreateView(0, 0, RUNTIME_CLASS(CaseListingView), CSize(1, 1), pContext) ||
        !m_splitterWnd.CreateView(0, 1, RUNTIME_CLASS(HtmlView), CSize(1, 1), pContext) )
    {
        return FALSE;
    }

    m_caseListingView = assert_cast<CaseListingView*>(m_splitterWnd.GetPane(0, 0));
    m_htmlView = assert_cast<HtmlView*>(m_splitterWnd.GetPane(0, 1));

    return TRUE;
}


void DataSourceFrame::ActivateFrame(int nCmdShow)
{
    // maximize the frame if this is the only document open
    if( nCmdShow != SW_SHOWMAXIMIZED )
    {
        size_t documents_open = 0;

        ForeachDoc(
            [&](CDocument& /*doc*/)
            {
                ++documents_open;
                return ( documents_open == 1 );
            });

        ASSERT(documents_open == 1 || documents_open == 2);

        if( documents_open == 1 )
            nCmdShow = SW_SHOWMAXIMIZED;
    }

    __super::ActivateFrame(nCmdShow);
}


void DataSourceFrame::OnMDIActivate(const BOOL bActivate, CWnd* const pActivateWnd, CWnd* const pDeactivateWnd)
{
    __super::OnMDIActivate(bActivate, pActivateWnd, pDeactivateWnd);

    CMainFrame* const main_frame = assert_cast<CMainFrame*>(AfxGetMainWnd());
    DataSourceDoc& data_source_doc = GetDataSourceDoc();

    // if closing the last window, clear all status text
    if( pActivateWnd == nullptr )
    {
        main_frame->ClearStatusBarPaneText();
    }

    // otherwise update the status text for the data source info...
    else
    {
        const std::string text = SO::CreateParentheticalExpression(data_source_doc.GetDictionary().GetName(),
                                                                   ToString(data_source_doc.GetConnectionString().GetType()));
        main_frame->SetStatusBarPaneText(ID_STATUS_PANE_DATA_SOURCE_INFO, TC::ToWide(text).c_str());

        // ...case count, and selected case key/position
        OnUpdateStatusBarCaseCount(0, 0);
        OnUpdateStatusBarSelectedCaseKeyPosition(0, 0);
    }
}


LRESULT DataSourceFrame::OnUpdateStatusBarCaseCount(WPARAM /*wParam*/, LPARAM /*lParam*/)
{
    CMainFrame* const main_frame = assert_cast<CMainFrame*>(AfxGetMainWnd());

    const std::optional<size_t>& number_cases = m_caseListingView->GetCaseListingCtrl().GetNumberCases();

    if( number_cases.has_value() )
    {
        const CaseIterationCaseStatus status = GetSettings<ViewableCaseIteratorSettings>().GetStatus();
        const wchar_t* const status_text = ( status == CaseIterationCaseStatus::All )            ? L"All" :
                                           ( status == CaseIterationCaseStatus::NotDeletedOnly ) ? L"Not Deleted" :
                                           ( status == CaseIterationCaseStatus::PartialsOnly )   ? L"Partial" :
                                         /*( status == CaseIterationCaseStatus::DuplicatesOnly )*/ L"Duplicate";

        main_frame->SetStatusBarPaneText(ID_STATUS_PANE_CASE_COUNT,
                                         FormatTextCS2WS(L"%s Cases: %d", status_text, static_cast<int>(*number_cases)).c_str());
    }

    else
    {
        main_frame->SetStatusBarPaneText(ID_STATUS_PANE_CASE_COUNT, nullptr);
    }

    return 1;
}


LRESULT DataSourceFrame::OnUpdateStatusBarSelectedCaseKeyPosition(WPARAM /*wParam*/, LPARAM /*lParam*/)
{
    CMainFrame* const main_frame = assert_cast<CMainFrame*>(AfxGetMainWnd());

    const std::vector<std::shared_ptr<const CaseSummary>>& selected_case_summaries = m_caseListingView->GetCaseListingCtrl().GetSelectedCaseSummaries();

    if( selected_case_summaries.size() == 1 )
    {
        std::wstring text = L"Key: " + TC::ToWide(selected_case_summaries.front()->GetSingleLineKey());
        main_frame->SetStatusBarPaneText(ID_STATUS_PANE_CASE_KEY, text.c_str());

        text = L"Position: " + TC::ToWide(DoubleToString(selected_case_summaries.front()->GetPositionInRepository()));
        main_frame->SetStatusBarPaneText(ID_STATUS_PANE_CASE_POSITION, text.c_str());
    }

    else
    {
        main_frame->SetStatusBarPaneText(ID_STATUS_PANE_CASE_KEY, nullptr);
        main_frame->SetStatusBarPaneText(ID_STATUS_PANE_CASE_POSITION, nullptr);
    }

    return 1;
}


std::shared_ptr<Case> DataSourceFrame::GetCaseForReading()
{
    // prune the cache if necessary
    if( m_cachedCases.size() == CachedCasesMaxSize )
        m_cachedCases.erase(m_cachedCases.begin());

    // create the case
    DataSourceDoc& data_source_doc = GetDataSourceDoc();

    std::shared_ptr<Case> data_case = data_source_doc.GetCaseAccess().CreateCase();
    m_cachedCases.emplace_back(data_case);

    data_case->SetCaseConstructionReporter(std::make_unique<StringVectorCaseConstructionReporter>());

    return data_case;
}


std::shared_ptr<const Case> DataSourceFrame::LoadCase(const double position_in_repository)
{
    // if possible, load the case from the cache, searching in reverse
    // because those will be the cases that were most recently accessed
    const auto& lookup = std::find_if(m_cachedCases.crbegin(), m_cachedCases.crend(),
        [&](const std::shared_ptr<const Case>& data_case)
        {
            return ( position_in_repository == data_case->GetPositionInRepository() );
        });

    if( lookup != m_cachedCases.crend() )
        return *lookup;

    // read the case
    DataSourceDoc& data_source_doc = GetDataSourceDoc();
    std::shared_ptr<Case> data_case = GetCaseForReading();

    data_source_doc.GetDataRepository().ReadCase(*data_case, position_in_repository);

    return data_case;
}


std::shared_ptr<const Case> DataSourceFrame::LoadCaseByKey(const std::string& key)
{
    DataSourceDoc& data_source_doc = GetDataSourceDoc();
    std::shared_ptr<Case> data_case = GetCaseForReading();

    data_source_doc.GetDataRepository().ReadCase(*data_case, key);

    return data_case;
}


std::shared_ptr<const Case> DataSourceFrame::LoadCaseByUuid(const std::string& uuid)
{
    DataSourceDoc& data_source_doc = GetDataSourceDoc();
    std::shared_ptr<Case> data_case = GetCaseForReading();

    data_source_doc.GetDataRepository().ReadCaseByUuid(*data_case, uuid);

    return data_case;
}


std::shared_ptr<const Case> DataSourceFrame::LoadCaseByUuidOrKey(const std::string& uuid, const std::string& key)
{
    DataSourceDoc& data_source_doc = GetDataSourceDoc();
    std::shared_ptr<Case> data_case = GetCaseForReading();

    if( !uuid.empty() )
    {
        try
        {
            data_source_doc.GetDataRepository().ReadCaseByUuid(*data_case, uuid);
            return data_case;
        }
        catch( const DataRepositoryException::CaseNotFound& ) { }
    }

    data_source_doc.GetDataRepository().ReadCase(*data_case, key);
    return data_case;
}


void DataSourceFrame::ShowCase(std::shared_ptr<const Case> data_case, const bool select_in_case_listing)
{
    ASSERT(data_case != nullptr);

    DataSourceDoc& data_source_doc = GetDataSourceDoc();
    data_source_doc.SetCurrentCase(std::move(data_case));

    // if on no page, or on the data summary page, switch to the case view
    if( IsShowingPageOrNothing(ID_VIEW_DATA_SUMMARY) )
    {
        PostMessage(WM_COMMAND, GetSettings<DataSourceSettings>().GetDefaultCasePageCommandId());
    }

    // otherwise update any page shown
    else
    {
        PostMessage(UWM::DataManager::UpdateContentOnCaseListingSelectionsChange);
    }

    if( select_in_case_listing ) // DATA_TODO highlight case in case listing?
    {
    }
}


LRESULT DataSourceFrame::OnShowDefaultPage(const WPARAM wParam, const LPARAM lParam)
{
    // if a specific case to show has been specified via the connection string,
    // it will be handled on OnProcessConnectionStringParameters
    if( OnProcessConnectionStringParameters(wParam, lParam) == 1 )
    {
        return 1;
    }

    // otherwise show the default page
    else
    {
        return __super::OnShowDefaultPage(wParam, lParam);
    }
}


LRESULT DataSourceFrame::OnProcessConnectionStringParameters(const WPARAM wParam, LPARAM /*lParam*/)
{
    DataSourceDoc& data_source_doc = GetDataSourceDoc();
    cs::shared_or_raw_ptr<const ConnectionString> connection_string;

    // the connection string comes from this object (in OnShowDefaultPage) or from
    // DocSetComponentDocTemplate (when trying to open the data source another time, say from the URI handler)
    if( wParam == 0 )
    {
        connection_string = &data_source_doc.GetConnectionString();
    }

    else
    {
        const SharableString connection_string_text = WindowsDesktopMessage::GetPostedObject<SharableString>(wParam);

        if( !connection_string_text.IsSet() )
            return 0;

        connection_string = std::make_unique<ConnectionString>(*connection_string_text);
    }

    // handle connection string properties to select a specific case
    try
    {
        const std::string* uuid_or_key_property = connection_string->GetProperty(CSProperty::uuid);
        std::shared_ptr<const Case> data_case;

        if( uuid_or_key_property != nullptr )
        {
            data_case = LoadCaseByUuid(*uuid_or_key_property);
        }

        else
        {
            uuid_or_key_property = connection_string->GetProperty(CSProperty::key);

            if( uuid_or_key_property == nullptr )
                return 0;

            data_case = LoadCaseByKey(*uuid_or_key_property);
        }

        ShowCase(std::move(data_case), true);

        return 1;
    }

    catch( const CSProException& exception )
    {
        ErrorMessage::Display(exception);
        return 0;
    }
}


LRESULT DataSourceFrame::OnShowSelectedCases(const WPARAM wParam, LPARAM /*lParam*/)
{
    const bool open_in_new_window = ( wParam == TRUE );
    const std::vector<std::shared_ptr<const CaseSummary>>& selected_case_summaries = m_caseListingView->GetCaseListingCtrl().GetSelectedCaseSummaries();

    DataSourceDoc& data_source_doc = GetDataSourceDoc();

    // only process valid selections;
    // when there are multiple cases selected, the only valid action is to open the cases in new windows
    if( ( selected_case_summaries.empty() ) ||
        ( !open_in_new_window && selected_case_summaries.size() > 1 ) )
    {
        data_source_doc.SetCurrentCase(nullptr);
        PostMessage(UWM::DataManager::UpdateContentOnCaseListingSelectionsChange);

        return 0;
    }

    std::string case_load_errors;

    for( const std::shared_ptr<const CaseSummary>& case_summary : selected_case_summaries )
    {
        try
        {
            std::shared_ptr<const Case> data_case = LoadCase(case_summary->GetPositionInRepository());

            if( open_in_new_window )
            {
                assert_cast<DataManagerApp*>(AfxGetApp())->OpenCaseDoc(std::make_tuple(
                    data_source_doc.GetSharedDictionary(),
                    data_source_doc.GetSharedCaseAccess(),
                    data_source_doc.GetConnectionString(),
                    std::move(data_case),
                    GetSettings<DataSourceSettings>().GetDefaultCasePageCommandId()));
            }

            else
            {
                ShowCase(std::move(data_case), false);
            }
        }

        catch( const CSProException& exception )
        {
            SO::AppendWithSeparator(case_load_errors, exception.what(), "\n\n");
        }
    }

    if( !case_load_errors.empty() )
        ErrorMessage::Display(case_load_errors);

    return 1;
}


LRESULT DataSourceFrame::OnRunTaskFromCaseListing(const WPARAM wParam, LPARAM /*lParam*/)
{
    std::unique_ptr<CaseProvider> case_provider = CreateCaseProvider(m_caseListingView->GetCaseListingCtrl().GetSelectedCaseSummaries());

    switch( wParam )
    {
        case ID_FILE_SAVE_CASES:
            OnFileSaveCases(std::move(case_provider));
            return 1;

        case ID_FILE_EXPORT_CASES:
            OnFileExportCases(std::move(case_provider));
            return 1;

        case ID_FILE_EXTRACT_NOTES:
            OnFileExtractNotes(std::move(case_provider));
            return 1;

        case ID_FILE_EXTRACT_BINARY_DATA:
            OnFileExtractBinaryData(std::move(case_provider));
            return 1;

        default:
            return ReturnProgrammingError(0);
    }
}


void DataSourceFrame::OnFileRefresh()
{
    Refresh(false);
}


void DataSourceFrame::Refresh(const bool show_data_summary)
{
    DataSourceDoc& data_source_doc = GetDataSourceDoc();
    const std::shared_ptr<const Case> current_case = !show_data_summary ? data_source_doc.GetSharedCurrentCase() :
                                                                          nullptr;

    // clear the cached cases because any of the cases may have changed
    m_cachedCases.clear();

    // update the case listing
    UpdateCaseListing(false);

    // if necessary, reload the currently shown case and refresh the page,
    // falling back to showing the data summary if the case no longer exists
    if( current_case != nullptr  )
    {
        try
        {
            ShowCase(LoadCaseByUuidOrKey(current_case->GetUuid(), current_case->GetKey()), true);
            return;
        }

        catch(...)
        {
            ErrorMessage::PostMessageForDisplay(FormatText("The previously shown case with key '%s' no longer exists.",
                                                           current_case->GetKey().c_str()));
        }
    }

    OnViewDataSummaryPage();
}


void DataSourceFrame::OnFileSynchronize()
{
    DataSourceDoc& data_source_doc = GetDataSourceDoc();
    bool need_to_restore_read_only_mode = false;
    bool need_to_refresh_cases = false;

    try
    {
        SyncHelpers::ValidateDataRepositoryTypeValidForSync(data_source_doc.GetConnectionString());

        SynchronizeDlg synchronize_dlg(data_source_doc.GetSharedDataRepository(), this);

        if( synchronize_dlg.DoModal() != IDOK )
            return;

        // make sure the data source is in read-write mode
        if( IsDataSourceReadOnly() )
        {
            if( !ToggleReadOnly() )
                return;

            need_to_restore_read_only_mode = true;
        }

        need_to_refresh_cases = true;

        TaskRunnerDlg task_runner_dlg(synchronize_dlg.ReleaseSyncTask(), this);
        task_runner_dlg.SetCloseDialogOnSuccess();

        task_runner_dlg.DoModal();
    }

    catch( const CSProException& exception )
    {
        ErrorMessage::Display(exception);
    }

    // potentially restore read-only mode
    if( need_to_restore_read_only_mode && !ToggleReadOnly() )
        return;

    // because cases may have changed, refresh the case tree and then show
    // show the data summary, which will contain information on the last sync time
    if( need_to_refresh_cases )
        Refresh(true);
}


void DataSourceFrame::OnFileSaveData()
{
    DataSourceDoc& data_source_doc = GetDataSourceDoc();

    if( !data_source_doc.DictionaryAllowsExport("saving data") )
        return;

    DataFileDlg data_file_dlg(DataFileDlg::Type::CreateNew, false);
    data_file_dlg.SetTitle(FormatText("Save '%s' Data As", data_source_doc.GetDictionary().GetName().c_str()))
                 .SuggestMatchingDataRepositoryType(data_source_doc.GetConnectionString());

    if( data_file_dlg.DoModal() != IDOK )
        return;

    // cases will be saved in sequential order
    RunCaseTask(std::make_unique<DataRepositoryCaseProvider>(data_source_doc.GetSharedDataRepository()),
                std::make_unique<ExtractCasesTask>(data_file_dlg.GetConnectionString()));
}


namespace
{
    constexpr CaseIterationMethod FromCaseIterationMethodId(const UINT nID)
    {
        return ( nID == ID_LISTING_SEQUENTIAL ) ? CaseIterationMethod::SequentialOrder :
             /*( nID == ID_LISTING_INDEXED )*/    CaseIterationMethod::KeyOrder;
    }


    constexpr CaseIterationCaseStatus FromCaseIterationCaseStatusId(const UINT nID)
    {
        return ( nID == ID_LISTING_CASES_ALL )         ? CaseIterationCaseStatus::All :
               ( nID == ID_LISTING_CASES_NOT_DELETED ) ? CaseIterationCaseStatus::NotDeletedOnly :
               ( nID == ID_LISTING_CASES_PARTIAL )     ? CaseIterationCaseStatus::PartialsOnly :
             /*( nID == ID_LISTING_CASES_DUPLICATE )*/   CaseIterationCaseStatus::DuplicatesOnly;
    }


    constexpr CaseIterationOrder FromCaseIterationOrderId(const UINT nID)
    {
        return ( nID == ID_LISTING_ASCENDING )    ? CaseIterationOrder::Ascending :
             /*( nID == ID_LISTING_DESCENDING )*/   CaseIterationOrder::Descending;
    }


    constexpr bool IsViewCaseLabelId(const UINT nID)
    {
        return ( nID == ID_LISTING_CASE_LABEL ) ? true :
             /*( nID == ID_LISTING_CASE_KEY )*/   false;
    }
}


template<typename T>
T& DataSourceFrame::GetSettings()
{
    std::shared_ptr<T>& settings = std::get<std::shared_ptr<T>>(m_settings);

    if( settings == nullptr )
    {
        DataSourceDoc& data_source_doc = GetDataSourceDoc();
        settings = data_source_doc.GetSettings<T>();
    }

    return *settings;
}


void DataSourceFrame::OnListingMethod(const UINT nID)
{
    GetSettings<ViewableCaseIteratorSettings>().SetMethod(FromCaseIterationMethodId(nID));
    UpdateCaseListing(false);
}


void DataSourceFrame::OnUpdateListingMethod(CCmdUI* const pCmdUI)
{
    pCmdUI->SetCheck(( GetSettings<ViewableCaseIteratorSettings>().GetEvaluatedMethod() == FromCaseIterationMethodId(pCmdUI->m_nID) ));
}


void DataSourceFrame::OnListingToggleMethod()
{
    GetSettings<ViewableCaseIteratorSettings>().ToggleMethod();
    UpdateCaseListing(false);
}


void DataSourceFrame::OnListingStatus(const UINT nID)
{
    GetSettings<ViewableCaseIteratorSettings>().SetStatus(FromCaseIterationCaseStatusId(nID));

    // make sure the case status is properly updated (when filters are showing)
    m_caseListingView->UpdateCaseStatusComboBox();

    UpdateCaseListing(false);
}


void DataSourceFrame::OnUpdateListingStatus(CCmdUI* const pCmdUI)
{
    pCmdUI->SetCheck(( GetSettings<ViewableCaseIteratorSettings>().GetStatus() == FromCaseIterationCaseStatusId(pCmdUI->m_nID) ));
}


void DataSourceFrame::OnListingOrder(const UINT nID)
{
    GetSettings<ViewableCaseIteratorSettings>().SetOrder(FromCaseIterationOrderId(nID));
    UpdateCaseListing(false);
}


void DataSourceFrame::OnUpdateListingOrder(CCmdUI* const pCmdUI)
{
    pCmdUI->SetCheck(( GetSettings<ViewableCaseIteratorSettings>().GetEvaluatedOrder() == FromCaseIterationOrderId(pCmdUI->m_nID) ));
}


void DataSourceFrame::OnListingCaseKeyLabel(const UINT nID)
{
    GetSettings<ViewableCaseIteratorSettings>().SetViewCaseKeyLabel(IsViewCaseLabelId(nID));
    UpdateCaseListing(true);
}


void DataSourceFrame::OnUpdateListingCaseKeyLabel(CCmdUI* const pCmdUI)
{
    pCmdUI->SetCheck(( GetSettings<ViewableCaseIteratorSettings>().GetViewCaseLabel() == IsViewCaseLabelId(pCmdUI->m_nID) ));
}


void DataSourceFrame::OnListingToggleCaseKeyLabel()
{
    GetSettings<ViewableCaseIteratorSettings>().ToggleViewCaseKeyLabel();
    UpdateCaseListing(true);
}


void DataSourceFrame::UpdateCaseListing(const bool change_is_only_visual)
{
    m_caseListingView->GetCaseListingCtrl().UpdateCaseListing(change_is_only_visual);

    if( !change_is_only_visual )
        PostMessage(UWM::DataManager::UpdateContentOnCaseListingSettingsChange);
}


void DataSourceFrame::OnViewCasePage(const UINT nID)
{
    CaseHoldingFrame::OnViewCasePage(nID);

    // use this view as the default case page for this data source
    GetSettings<DataSourceSettings>().SetDefaultCasePageCommandId(nID);
}


void DataSourceFrame::OnViewOptionsLanguage(const UINT nID)
{
    CaseHoldingFrame::OnViewOptionsLanguage(nID);

    // remember this language for future use
    const CDataDict& dictionary = GetDataSourceDoc().GetDictionary();
    GetSettings<DataSourceSettings>().SetLanguageName(dictionary.GetCurrentLanguage().GetName());
}


bool DataSourceFrame::IsDataSourceReadOnly()
{
    const DataRepository& data_repository = GetDataSourceDoc().GetDataRepository();
    return ( data_repository.GetRepositoryAccess() == DataRepositoryAccess::ReadOnly );
}


bool DataSourceFrame::ToggleReadOnly()
{
    DataRepository& data_repository = GetDataSourceDoc().GetDataRepository();
    const bool initially_read_only = IsDataSourceReadOnly();

    try
    {
        data_repository.ToggleReadWriteMode();
        return true;
    }

    catch( const CSProException& exception )
    {
        ErrorMessage::Display(FormatText("There was an error opening the data source in %s mode:\n\n%s\n\nTry opening the data source again.",
                                         initially_read_only ? "read-only" : "read-write", exception.what()));
        SendMessage(WM_CLOSE);
        return false;
    }
}


void DataSourceFrame::OnDataToggleReadOnly()
{
    // make sure this is allowed
    if( IsDataSourceReadOnly() )
    {
        const CDataDict& dictionary = GetDataSourceDoc().GetDictionary();

        if( !dictionary.GetAllowDataManagerModifications() )
        {
            ErrorMessage::Display(FormatText("The security restrictions for the dictionary '%s' prohibit the modification of data.",
                                             dictionary.GetName().c_str()));
            return;
        }
    }

    ToggleReadOnly();
}


void DataSourceFrame::OnUpdateDataToggleReadOnly(CCmdUI* const pCmdUI)
{
    pCmdUI->SetCheck(IsDataSourceReadOnly());
}


void DataSourceFrame::OnUpdateDataSourceIsReadWriteWithCaseSelected(CCmdUI* const pCmdUI)
{
    pCmdUI->Enable(( IsDataSourceReadWrite() &&
                     m_caseListingView->GetCaseListingCtrl().GetSelectedCount() >= 1 ));
}


std::vector<DataRepositoryUniqueCaseIdentifer> DataSourceFrame::GetUniqueCaseIdentifiers(const std::vector<std::shared_ptr<const CaseSummary>>& selected_case_summaries,
                                                                                         const bool use_position_in_repository_for_first_case_summary)
{
    std::vector<DataRepositoryUniqueCaseIdentifer> unique_case_identifiers;

    if( !selected_case_summaries.empty() )
    {
        DataRepository& data_repository = GetDataSourceDoc().GetDataRepository();

        auto selected_case_summaries_itr = selected_case_summaries.cbegin();
        const auto& selected_case_summaries_end = selected_case_summaries.cend();

        if( use_position_in_repository_for_first_case_summary )
        {
            unique_case_identifiers.emplace_back((*selected_case_summaries_itr)->GetPositionInRepository());
            ++selected_case_summaries_itr;
        }

        for( ; selected_case_summaries_itr != selected_case_summaries_end; ++selected_case_summaries_itr )
            unique_case_identifiers.emplace_back(data_repository.GetUniqueCaseIdentifer(*(*selected_case_summaries_itr)));
    }

    return unique_case_identifiers;
}


void DataSourceFrame::OnDataDeleteCase()
{
    DataSourceCaseListingCtrl& case_listing_ctrl = m_caseListingView->GetCaseListingCtrl();
    std::vector<std::shared_ptr<const CaseSummary>> selected_case_summaries = case_listing_ctrl.GetSelectedCaseSummaries();

    if( selected_case_summaries.empty() )
    {
        ASSERT(false);
        return;
    }

    const bool single_case_deletion = ( selected_case_summaries.size() == 1 );

    DataSourceDoc& data_source_doc = GetDataSourceDoc();
    const DataRepositoryType data_repository_type = data_source_doc.GetConnectionString().GetType();

    // confirm the operation
    std::string delete_prompt;

    if( single_case_deletion )
    {
        const CaseSummary& case_summary = *selected_case_summaries.front();

        delete_prompt = FormatText("Are you sure you want to %sdelete the case with key '%s'?",
                                   case_summary.GetDeleted() ? "un" : "",
                                   case_summary.GetSingleLineKey().c_str());
    }

    else
    {
        delete_prompt = FormatText("Are you sure you want to delete %d cases?",
                                   static_cast<int>(selected_case_summaries.size()));
    }

    if( !DataRepositoryHelpers::TypeSupportsUndeletes(data_repository_type) )
    {
        delete_prompt.append(FormatText("\n\nCases in a '%s' data source cannot be undeleted so this operation is permanent.",
                                        ToString(data_repository_type)));
    }

    if( AfxMessageBox(delete_prompt, MB_YESNO | MB_DEFBUTTON2 | MB_ICONQUESTION) == IDNO )
        return;

    // for text-based repositories, sort by reverse file position to minimize the amount of data that to be rewritten
    if( !single_case_deletion && DataRepositoryHelpers::TypeWritesToText(data_repository_type) )
    {
        std::sort(selected_case_summaries.begin(), selected_case_summaries.end(),
                  [&](const auto& cs1, const auto& cs2) { return ( cs1->GetPositionInRepository() > cs2->GetPositionInRepository() ); });
    }

    DataRepository& data_repository = data_source_doc.GetDataRepository();

    try
    {
        // delete a single case...
        if( single_case_deletion )
        {
            data_repository.DeleteCase(selected_case_summaries.front()->GetPositionInRepository());
        }

        // ...or multiple cases
        else
        {
            // the positions may change, so store unique identifiers to use while deleting
            const std::vector<DataRepositoryUniqueCaseIdentifer> unique_case_identifiers = GetUniqueCaseIdentifiers(selected_case_summaries, true);

            // wrap the deletes in a transaction
            const DataRepositoryTransaction data_repository_transaction(data_repository);

            for( const DataRepositoryUniqueCaseIdentifer& unique_case_identifier : unique_case_identifiers )
                data_repository.DeleteCase(unique_case_identifier.GetPosition(data_repository));
        }
    }

    catch( const CSProException& exception )
    {
        ErrorMessage::Display(SO::Concatenate("There was an error deleting cases:\n\n", exception.what()));
    }

    // TODO need to refresh with case following the deleted one if the deleted one is no longer shown
}


void DataSourceFrame::OnExportData()
{
    if( !GetDataSourceDoc().DictionaryAllowsExport("exporting data") )
        return;

    OpenDataInTool(CSProExecutables::Program::CSExport);
}


void DataSourceFrame::OnTabulateFrequencies()
{
    OpenDataInTool(CSProExecutables::Program::CSFreq);
}


void DataSourceFrame::OpenDataInTool(const CSProExecutables::Program program)
{
    try
    {
        if( IsDataSourceReadWrite() )
            throw CSProException("You cannot open a data source in another program while it is opened in read-write mode.");

        const DataSourceDoc& data_source_doc = GetDataSourceDoc();
        const ConnectionString& connection_string = data_source_doc.GetConnectionString();

        const std::string argument = DictionarySource::HasEmbeddedDictionary(connection_string) ?
            connection_string.ToString() :
            data_source_doc.GetDictionary().GetFilePath();

        CSProExecutables::RunProgramOpeningFile(program, argument, true);
    }

    catch( const CSProException& exception )
    {
        ErrorMessage::Display(exception);
    }
}
