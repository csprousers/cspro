#include "StdAfx.h"
#include "BatchMeterDlg.h"
#include <zToolsO/CancelFlag.h>


BEGIN_MESSAGE_MAP(BatchMeterDlg, CDialog)
    ON_BN_CLICKED(IDCANCEL, OnCancel)
    ON_BN_CLICKED(IDC_DETAILS, OnDetails)
    ON_MESSAGE(UWM::UtilF::UpdateBatchMeterDlg, OnUpdateDlg)
END_MESSAGE_MAP()


namespace
{
    constexpr WPARAM SetTitleAndInitialize         = 1;
    constexpr WPARAM UpdateSourceAndProcessSummary = 2;
    constexpr WPARAM UpdateKeyAndProcessSummary    = 3;
    constexpr WPARAM UpdateProcessSummaryOnly      = 4;

    template<typename T>
    void SetWindowTextToNumber(CWnd* dlg_item, T value)
    {
        dlg_item->SetWindowText(UTF8_TODO::GetCString(IntToString(value)));
    }

    template<typename T>
    void SetWindowTextToTime(CWnd* dlg_item, const T& value)
    {
        dlg_item->SetWindowText(value.Format(L"%H:%M:%S"));
    }
}


BatchMeterDlg::BatchMeterDlg(CWnd* pParent/* = nullptr*/)
    :   CDialog(IDD_BATCHMETER_DIALOG, pParent),
        m_initialized(false),
        m_cancelationPending(false),
        m_cancelFlag(nullptr),
        m_completedFlag(false),
        m_showingDetails(true),
        m_dlgItemDetailsButton(nullptr),
        m_dlgItemSource(nullptr),
        m_dlgItemCaseKey(nullptr),
        m_dlgItemPercentBar(nullptr),
        m_dlgItemPercentRead(nullptr),
        m_dlgItemRecordsRead(nullptr),
        m_dlgItemElapsedTime(nullptr),
        m_dlgItemRemainingTime(nullptr),
        m_dlgItemMessagesUser(nullptr),
        m_dlgItemMessagesWarning(nullptr),
        m_dlgItemMessagesError(nullptr),
        m_dlgItemMessagesTotal(nullptr),
        m_dlgItemAttibutesUnknown(nullptr),
        m_dlgItemAttibutesErased(nullptr),
        m_dlgItemAttibutesIgnored(nullptr),
        m_levelDetails(nullptr)
{
}


BOOL BatchMeterDlg::OnInitDialog()
{
    m_dlgItemDetailsButton = static_cast<CButton*>(GetDlgItem(IDC_DETAILS));

    m_dlgItemSource = GetDlgItem(IDC_STATIC_DFNAME);
    m_dlgItemCaseKey = GetDlgItem(IDC_STATIC_CASEID);

    m_dlgItemPercentBar = static_cast<CProgressCtrl*>(GetDlgItem(IDC_PROGRESS));
    m_dlgItemPercentRead = GetDlgItem(IDC_STATIC_PERC);

    m_dlgItemRecordsRead = GetDlgItem(IDC_STATIC_RECORDSREAD);

    m_startTime = CTime::GetCurrentTime();
    CWnd* dlg_item_start_time = GetDlgItem(IDC_STATIC_TIME_START);
    SetWindowTextToTime(dlg_item_start_time, m_startTime);

    m_dlgItemElapsedTime = GetDlgItem(IDC_STATIC_TIME_ELAPSED);
    m_dlgItemRemainingTime = GetDlgItem(IDC_STATIC_TIME_REMAINING);

    m_dlgItemMessagesUser = GetDlgItem(IDC_STATIC_MSGS_USR);
    m_dlgItemMessagesWarning = GetDlgItem(IDC_STATIC_MSGS_WAR);
    m_dlgItemMessagesError = GetDlgItem(IDC_STATIC_MSGS_ERR);
    m_dlgItemMessagesTotal = GetDlgItem(IDC_STATIC_MSGS_TOT);

    m_dlgItemAttibutesUnknown = GetDlgItem(IDC_STATIC_IGN_UNK);
    m_dlgItemAttibutesErased = GetDlgItem(IDC_STATIC_IGN_ERA);
    m_dlgItemAttibutesIgnored = GetDlgItem(IDC_STATIC_IGN_TOT);

    m_levelDetails = static_cast<CListCtrl*>(GetDlgItem(IDC_LIST_LEVEL));

    constexpr std::tuple<TCHAR*, size_t> ColumnDetails[]
    {
        { L"Level",      50 },
        { L"Input Case", 87 },
        { L"Bad Struct", 87 },
        { L"Level Post", 87 }
    };

    LV_COLUMN lvc { };
    lvc.mask = LVCF_FMT | LVCF_WIDTH | LVCF_TEXT | LVCF_SUBITEM;

    for( size_t column = 0; column < _countof(ColumnDetails); ++column )
    {
        lvc.iSubItem = column;
        lvc.pszText = std::get<0>(ColumnDetails[column]);
        lvc.cx = std::get<1>(ColumnDetails[column]);
        lvc.fmt = LVCFMT_LEFT;
        m_levelDetails->InsertColumn(column, &lvc);
    }

    // by default hide the details
    ToggleDetails(false);

    return TRUE;
}


void BatchMeterDlg::Initialize(InterfaceString title, std::shared_ptr<ProcessSummary> process_summary, CancelFlag* const cancel_flag)
{
    m_processSummary = std::move(process_summary);
    m_cancelFlag = cancel_flag;

    ASSERT(m_processSummary != nullptr && m_cancelFlag != nullptr);

    if( m_cancelationPending )
    {
        *m_cancelFlag = true;
    }

    else
    {
        m_dialogTitle = title.Release();
        PostMessage(UWM::UtilF::UpdateBatchMeterDlg, SetTitleAndInitialize);
    }
}


void BatchMeterDlg::SetSource(InterfaceString source_text)
{
    UpdateText(m_sourceText, source_text.Release(), UpdateSourceAndProcessSummary);
}


void BatchMeterDlg::SetKey(const std::string& case_key)
{
    UpdateText(m_caseKey, TC::ToWide(case_key), UpdateKeyAndProcessSummary);
}


void BatchMeterDlg::UpdateText(std::wstring& destination_text, std::wstring source_text, const WPARAM update_type)
{
    ASSERT(IsWindow(GetSafeHwnd()));

    std::scoped_lock<std::mutex> lock(m_memberAccessMutex);
    destination_text = std::move(source_text);

    PostMessage(UWM::UtilF::UpdateBatchMeterDlg, update_type);
}


void BatchMeterDlg::OnCancel()
{
    if( m_completedFlag )
    {
        CDialog::OnCancel();
    }

    else if( m_initialized && *m_cancelFlag )
    {
        // if the cancel flag is already set, wait until the completed flag is set before closing the dialog
    }

    else if( !m_cancelationPending &&
             MessageBox(L"Are you sure you want to cancel the process?", L"Cancel Process?", MB_YESNO | MB_DEFBUTTON2) == IDYES )
    {
        m_cancelationPending = true;

        // don't set the cancel flag if the task has already completed running (while the message box was up)
        if( m_initialized && !m_completedFlag )
            *m_cancelFlag = true;
    }
}


void BatchMeterDlg::OnDetails()
{
    ToggleDetails(!m_showingDetails);

    PostMessage(UWM::UtilF::UpdateBatchMeterDlg, UpdateProcessSummaryOnly);
}


void BatchMeterDlg::ToggleDetails(const bool show_details)
{
    ASSERT(show_details != m_showingDetails);

    // if not already calculated, figure out the proper dimensions of the dialog
    if( !m_dialogWidthAndHeightDetailsNoDetails.has_value() )
    {
        ASSERT(m_showingDetails);

        RECT full_dialog_rect;
        GetWindowRect(&full_dialog_rect);
        const int full_width = full_dialog_rect.right - full_dialog_rect.left;
        const int full_height = full_dialog_rect.bottom - full_dialog_rect.top;

        // see how much padding there is at the bottom of the dialog
        RECT level_details_rect;
        m_levelDetails->GetWindowRect(&level_details_rect);
        const int padding = full_dialog_rect.bottom - level_details_rect.bottom;

        // add the padding to the bottom of the progress bar
        RECT progress_bar_rect;
        m_dlgItemPercentBar->GetWindowRect(&progress_bar_rect);

        const int no_details_height = progress_bar_rect.bottom + padding - full_dialog_rect.top;

        m_dialogWidthAndHeightDetailsNoDetails = std::make_tuple(full_width, full_height, no_details_height);
    }

    m_showingDetails = show_details;

    SetWindowPos(&wndTop, 0, 0,
                 std::get<0>(*m_dialogWidthAndHeightDetailsNoDetails),
                 m_showingDetails ? std::get<1>(*m_dialogWidthAndHeightDetailsNoDetails) : std::get<2>(*m_dialogWidthAndHeightDetailsNoDetails),
                 SWP_NOMOVE);

    m_dlgItemDetailsButton->SetWindowText(FormatText(L"Details %s", m_showingDetails ? L"<<" : L">>").c_str());
}


LRESULT BatchMeterDlg::OnUpdateDlg(const WPARAM wParam, LPARAM /*lParam*/)
{
    if( wParam == SetTitleAndInitialize )
    {
        WindowsWS::SetWindowText(this, m_dialogTitle);

        // initialize the level details
        if( m_processSummary->GetNumberLevels() == 0 )
        {
            m_levelDetails->ShowWindow(SW_HIDE);
        }

        else
        {
            m_levelDetails->ShowWindow(SW_SHOW);
            m_levelDetails->DeleteAllItems();

            for( size_t level_number = 0; level_number < m_processSummary->GetNumberLevels(); ++level_number )
                m_levelDetails->InsertItem(level_number, UTF8_TODO::GetCString(IntToString(level_number + 1)));
        }

        m_initialized = true;
    }

    else
    {
        if( wParam != UpdateProcessSummaryOnly )
        {
            std::scoped_lock<std::mutex> lock(m_memberAccessMutex);

            if( wParam == UpdateKeyAndProcessSummary )
            {
                WindowsWS::SetWindowText(m_dlgItemCaseKey, m_caseKey);
            }

            else if( wParam == UpdateSourceAndProcessSummary )
            {
                WindowsWS::SetWindowText(m_dlgItemSource, m_sourceText);
            }
        }

        UpdateProcessSummary();
    }

    return 0;
}


void BatchMeterDlg::UpdateProcessSummary()
{
    if( !m_initialized )
        return;

    const int percent_read = static_cast<int>(m_processSummary->GetPercentSourceRead());

    m_dlgItemPercentBar->SetPos(percent_read);
    SetWindowTextToNumber(m_dlgItemPercentRead, percent_read);

    SetWindowTextToNumber(m_dlgItemRecordsRead, m_processSummary->GetAttributesRead());

    if( !m_showingDetails )
        return;

    const CTimeSpan elapsed_time = CTime::GetCurrentTime() - m_startTime;
    SetWindowTextToTime(m_dlgItemElapsedTime, elapsed_time);

    if( percent_read != 0 )
    {
        const CTimeSpan remaining_time = static_cast<__time64_t>(( 100.0 - percent_read ) / percent_read * elapsed_time.GetTimeSpan());
        SetWindowTextToTime(m_dlgItemRemainingTime, remaining_time);
    }

    SetWindowTextToNumber(m_dlgItemMessagesUser, m_processSummary->GetUserMessages());
    SetWindowTextToNumber(m_dlgItemMessagesWarning, m_processSummary->GetWarningMessages());
    SetWindowTextToNumber(m_dlgItemMessagesError, m_processSummary->GetErrorMessages());
    SetWindowTextToNumber(m_dlgItemMessagesTotal, m_processSummary->GetTotalMessages());

    if( m_processSummary->GetAttributesIgnored() != 0 )
    {
        SetWindowTextToNumber(m_dlgItemAttibutesUnknown, m_processSummary->GetAttributesUnknown());
        SetWindowTextToNumber(m_dlgItemAttibutesErased, m_processSummary->GetAttributesErased());
        SetWindowTextToNumber(m_dlgItemAttibutesIgnored, m_processSummary->GetAttributesIgnored());
    }

    for( size_t level_number = 0; level_number < m_processSummary->GetNumberLevels(); ++level_number )
    {
        m_levelDetails->SetItemText(level_number, 1, TC::ToWide(IntToString(m_processSummary->GetCaseLevelsRead(level_number))).c_str());
        m_levelDetails->SetItemText(level_number, 2, TC::ToWide(IntToString(m_processSummary->GetBadCaseLevelStructures(level_number))).c_str());
        m_levelDetails->SetItemText(level_number, 3, TC::ToWide(IntToString(m_processSummary->GetLevelPostProcsExecuted(level_number))).c_str());
    }
}
