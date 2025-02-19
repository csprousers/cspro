#include "StdAfx.h"
#include "GenerateDlg.h"


namespace
{
    constexpr double HideProgressBarCode = std::numeric_limits<double>::lowest();
}


BEGIN_MESSAGE_MAP(GenerateDlg, CDialog)
    ON_BN_CLICKED(IDC_CLOSE, OnClose)
    ON_NOTIFY(NM_CLICK, IDC_OUTPUTS_SYSLINK, OnOutputsClick)
    ON_NOTIFY(NM_RETURN, IDC_OUTPUTS_SYSLINK, OnOutputsClick)
    ON_MESSAGE(UWM::CSDocument::GenerateDlgUpdateText, OnUpdateText)
    ON_MESSAGE(UWM::CSDocument::GenerateDlgTaskComplete, OnGenerateTaskComplete)
END_MESSAGE_MAP()


GenerateDlg::GenerateDlg(GenerateTask& generate_task, CWnd* const pParent/* = nullptr*/)
    :   CDialog(IDD_GENERATE, pParent),
        m_globalSettings(assert_cast<CMainFrame*>(AfxGetMainWnd())->GetGlobalSettings()),
        m_generateTask(generate_task),
        m_closeDialogOnCompletion(m_globalSettings.close_generate_dialog_on_completion)
{
}


void GenerateDlg::DoDataExchange(CDataExchange* const pDX)
{
    __super::DoDataExchange(pDX);

    DDX_Control(pDX, IDC_LOG, m_loggingListBox);
    DDX_Control(pDX, IDC_PROGRESS, m_progressCtrl);
    DDX_Check(pDX, IDC_CLOSE_DIALOG_ON_COMPLETION, m_closeDialogOnCompletion);
}


BOOL GenerateDlg::OnInitDialog()
{
    __super::OnInitDialog();

    WindowHelpers::RemoveDialogSystemIcon(*this);

    m_generateTask.SetInterface(*this);
    m_generateTask.Run();

    return TRUE;
}


void GenerateDlg::OnCancel()
{
    if( !m_generateTask.IsRunning() )
    {
        ASSERT(GetDlgItem(IDC_CLOSE)->IsWindowEnabled());
        OnClose();
    }

    else if( MessageBox(L"Are you sure you want to cancel the process?", L"Cancel Process?", MB_YESNO | MB_DEFBUTTON2) == IDYES )
    {
        m_generateTask.Cancel();
    }
}


void GenerateDlg::OnClose()
{
    UpdateData(TRUE);

    m_globalSettings.close_generate_dialog_on_completion = m_closeDialogOnCompletion;

    OnOK();
}


void GenerateDlg::OnOutputsClick(NMHDR* const pNMHDR, LRESULT* const pResult)
{
    const bool control_pressed = ( GetKeyState(VK_CONTROL) < 0 );

    const NMLINK* const pNMLink = reinterpret_cast<const NMLINK*>(pNMHDR);
    ASSERT(static_cast<size_t>(pNMLink->item.iLink) < m_finalOutputs.size());

    const std::string& path = std::get<1>(m_finalOutputs[pNMLink->item.iLink]);

    if( control_pressed )
    {
        OpenContainingFolder(path);
    }

    else
    {
        ShellExecute(nullptr, L"open", TC::ToWide(EscapeCommandLineArgument(path)).c_str(), nullptr, nullptr, SW_SHOW);
    }

    *pResult = 0;
}


void GenerateDlg::PostTextForUpdate(CWnd* const pWnd, const std::string_view text_sv)
{
    ASSERT(pWnd->GetSafeHwnd() != nullptr);

    const std::wstring* text_ptr;

    // lock
    {
        std::lock_guard<std::mutex> lock(m_postedTextUpdatesMutex);
        text_ptr = m_postedTextUpdates.emplace_back(std::make_unique<std::wstring>(TC::ToWide(text_sv))).get();
    }

    PostMessage(UWM::CSDocument::GenerateDlgUpdateText, reinterpret_cast<WPARAM>(pWnd->m_hWnd), reinterpret_cast<LPARAM>(text_ptr));
}


LRESULT GenerateDlg::OnUpdateText(const WPARAM wParam, const LPARAM lParam)
{
    HWND hWnd = reinterpret_cast<HWND>(wParam);
    const std::wstring* const text_ptr = reinterpret_cast<const std::wstring*>(lParam);

    // update the text
    WindowsWS::SetWindowText(hWnd, *text_ptr);

    // prevent the m_postedTextUpdatesMutex vector from using too much memory
    constexpr size_t PostedTextUpdatesMaxSize = 1000;

    if( m_postedTextUpdates.size() > PostedTextUpdatesMaxSize )
    {
        std::lock_guard<std::mutex> lock(m_postedTextUpdatesMutex);

        const auto& lookup = std::find_if(m_postedTextUpdates.cbegin(), m_postedTextUpdates.cend(),
                                          [&](const std::unique_ptr<std::wstring>& this_text) { return ( text_ptr == this_text.get() ); });
        ASSERT(lookup != m_postedTextUpdates.cend());

        m_postedTextUpdates.erase(m_postedTextUpdates.cbegin(), lookup);
    }

    return 1;
}


LRESULT GenerateDlg::OnGenerateTaskComplete(const WPARAM wParam, LPARAM /*lParam*/)
{
    const GenerateTask::Status status = static_cast<GenerateTask::Status>(wParam);

    if( status == GenerateTask::Status::Canceled )
    {
        __super::OnCancel();
    }

    else
    {
        // hide the Cancel button and show the Close button
        GetDlgItem(IDCANCEL)->ShowWindow(SW_HIDE);

        CWnd* const close_button = GetDlgItem(IDC_CLOSE);
        close_button->EnableWindow();
        close_button->SetFocus();

        if( status == GenerateTask::Status::Complete )
        {
            UpdateData(TRUE);

            if( m_closeDialogOnCompletion )
                OnClose();
        }
    }

    return 0;
}


void GenerateDlg::SetTitle(const std::string& title)
{
    PostTextForUpdate(this, title);
}


void GenerateDlg::LogText(SharableString text)
{
    m_loggingListBox.AddText(std::move(text));
}


void GenerateDlg::UpdateProgress(const double percent)
{
    if( percent == HideProgressBarCode )
    {
        m_progressCtrl.ShowWindow(SW_HIDE);
    }

    else
    {
        m_progressCtrl.PostMessage(PBM_SETPOS, static_cast<int>(percent));
    }
}


void GenerateDlg::SetOutputText(const std::string& text)
{
    PostTextForUpdate(GetDlgItem(IDC_OUTPUTS_SYSLINK), Encoders::ToHtml(text));
}


void GenerateDlg::OnCreatedOutput(std::string output_title, std::string path)
{
    m_finalOutputs.emplace_back(std::move(output_title), std::move(path));

    // update the Outputs string
    std::string outputs_html = "Outputs:";

    for( const auto& [this_output_title, this_path] : m_finalOutputs )
    {
        outputs_html.append("  <a>")
                    .append(Encoders::ToHtml(this_output_title))
                    .append("</a>");
    }

    PostTextForUpdate(GetDlgItem(IDC_OUTPUTS_SYSLINK), outputs_html);
}


void GenerateDlg::OnException(const CSProException& exception)
{
    Interface::LogText(u8"\n⚠ %s", exception.what());

    SetOutputText("Error!");
    UpdateProgress(HideProgressBarCode);

    MessageBeep(MB_ICONERROR);
}


void GenerateDlg::OnCompletion(const GenerateTask::Status status)
{
    PostMessage(UWM::CSDocument::GenerateDlgTaskComplete, static_cast<WPARAM>(status));
}


const GlobalSettings& GenerateDlg::GetGlobalSettings()
{
    return m_globalSettings;
}
