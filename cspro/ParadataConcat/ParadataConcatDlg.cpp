#include "stdafx.h"
#include "ParadataConcatDlg.h"
#include <zUtilO/FileDlg.h>
#include <zUtilO/imsaDlg.H>
#include <zUtilO/WindowsWS.h>
#include <zParadataO/GuiConcatenator.h>


BEGIN_MESSAGE_MAP(ParadataConcatDlg, CDialog)
    ON_COMMAND(ID_FILE_OPEN, OnFileOpen)
    ON_COMMAND(ID_FILE_SAVE_AS, OnFileSaveAs)
    ON_COMMAND(ID_FILE_RUN, OnFileRun)
    ON_COMMAND(ID_HELP, OnHelp)
    ON_COMMAND(ID_APP_ABOUT, OnAppAbout)
    ON_COMMAND(IDC_BROWSE_OUTPUT, OnBrowseOutput)
    ON_BN_CLICKED(IDC_ADD_LOGS, OnAddLogs)
    ON_BN_CLICKED(IDC_REMOVE_LOGS, OnRemoveLogs)
    ON_BN_CLICKED(IDC_CLEAR_LOGS, OnClearLogs)
    ON_BN_CLICKED(IDC_RUN, OnFileRun)
END_MESSAGE_MAP()


namespace
{
    constexpr const wchar_t* RegistryKey       = L"Settings";
    constexpr const wchar_t* RegistryValueName = L"Last Data Folder";
}


ParadataConcatDlg::ParadataConcatDlg(CWnd* const pParent /*= nullptr*/)
    :   CDialog(ParadataConcatDlg::IDD, pParent),
        m_hIcon(AfxGetApp()->LoadIcon(IDR_MAINFRAME))
{
}


void ParadataConcatDlg::DoDataExchange(CDataExchange* const pDX)
{
    CDialog::DoDataExchange(pDX);

    DDX_Text(pDX, IDC_OUTPUT_FILENAME, m_outputFilePath, true);
    DDX_Control(pDX, IDC_LIST_LOGS, m_paradataLogList);
}


BOOL ParadataConcatDlg::OnInitDialog()
{
    CDialog::OnInitDialog();

    // add the menu
    CMenu menu;
    menu.LoadMenu(IDR_PARADATACONCAT_MENU);
    SetMenu(&menu);

    // set the icons
    SetIcon(m_hIcon, TRUE);
    SetIcon(m_hIcon, FALSE);

    // set up the list control
    m_paradataLogList.SetExtendedStyle(LVS_EX_FULLROWSELECT);
    m_paradataLogList.SetHeadings(L"Name,180;Directory,220;Date,120;Events,90;Size,90");
    m_paradataLogList.LoadColumnInfo();

    // set up the callback to allow the dragging of files onto the list of logs
    m_paradataLogList.InitializeDropFiles(DropFilesListCtrl::DirectoryHandling::RecurseInto, FileExtensions::CreateWildcard(FileExtensions::Paradata),
        [&](std::vector<std::string> paths)
        {
            OnDropFiles(std::move(paths));
        });

    return TRUE;
}


void ParadataConcatDlg::OnAppAbout()
{
    CIMSAAboutDlg about_dlg(WindowsWS::LoadString(AFX_IDS_APP_TITLE), m_hIcon);
    about_dlg.DoModal();
}


void ParadataConcatDlg::OnBrowseOutput()
{
    UpdateData(TRUE);

    const std::string paradata_log_wildcard = FileExtensions::CreateWildcard(FileExtensions::Paradata);
    const std::string single_paradata_log_filter = FormatText("Paradata Log (%s)|%s||", paradata_log_wildcard.c_str(), paradata_log_wildcard.c_str());

    SaveFileDlg save_file_dlg(0, FileExtensions::Paradata, m_outputFilePath, single_paradata_log_filter, this);
    save_file_dlg.SetTitle(L"Select Output Paradata Log");

    if( save_file_dlg.DoModal() != IDOK )
        return;

    std::string output_file_path = save_file_dlg.GetFilePath();
    bool update_file_path = true;

    if( m_paradataLogFilePaths.find(output_file_path) == m_paradataLogFilePaths.end() &&
        Paradata::GuiConcatenator::GetNumberEvents(output_file_path) > 0 )
    {
        const int result = MessageBox(L"The output file has events in it. Do you want to include these as inputs?",
                                      L"Include events?", MB_YESNOCANCEL | MB_ICONEXCLAMATION);

        if( result == IDYES )
        {
            AddLogs({ output_file_path });
        }

        else if( result == IDCANCEL )
        {
            update_file_path = false;
        }
    }

    if( update_file_path )
    {
        m_outputFilePath = std::move(output_file_path);
        UpdateData(FALSE);
    }
}


void ParadataConcatDlg::UpdateNumberLogsText()
{
    WindowsUtf8::SetText(this, IDC_NUMBER_LOGS, FormatText("%d log%s", static_cast<int>(m_paradataLogFilePaths.size()),
                                                                       PluralizeWord(m_paradataLogFilePaths.size())));
}


void ParadataConcatDlg::AddLogs(std::vector<std::string> file_paths)
{
    for( std::string& file_path : file_paths )
    {
        if( m_paradataLogFilePaths.find(file_path) != m_paradataLogFilePaths.end() )
            continue; // no reason to include the same paradata log file twice

        const int64_t events = Paradata::GuiConcatenator::GetNumberEvents(file_path);

        // this was not a valid paradata log file
        if( events < 0 )
            continue;

        const auto [file_size, modified_time] = PortableFunctions::FileSizeAndModifiedTime(file_path);

        m_paradataLogList.AddItem(TC::ToWide(PortableFunctions::PathGetFilename(file_path)).c_str(),
                                  TC::ToWide(PortableFunctions::PathGetDirectory(file_path)).c_str(),
                                  TC::ToWide(FormatTimestamp(static_cast<double>(modified_time))).c_str(),
                                  TC::ToWide(IntToString(events)).c_str(),
                                  TC::ToWide(IntToString(file_size)).c_str());

        m_paradataLogFilePaths.insert(std::move(file_path));
    }

    UpdateNumberLogsText();
}


void ParadataConcatDlg::OnAddLogs()
{
    const std::string paradata_log_wildcard = FileExtensions::CreateWildcard(FileExtensions::Paradata);
    const std::string multiple_paradata_logs_filter = FormatText("Paradata Log Files (%s)|%s|All Files (*.*)|*.*||", paradata_log_wildcard.c_str(), paradata_log_wildcard.c_str());

    const CString initial_directory = AfxGetApp()->GetProfileString(RegistryKey, RegistryValueName);

    OpenFileDlg open_file_dlg(0, nullptr, initial_directory.GetString(), multiple_paradata_logs_filter, this);
    open_file_dlg.SetTitle(L"Select Paradata Logs to Concatenate")
                 .SetMultiSelectBuffer();

    if( open_file_dlg.DoModal() != IDOK )
        return;

    std::vector<std::string> file_paths = open_file_dlg.GetFilePaths();
    ASSERT(!file_paths.empty());

    AfxGetApp()->WriteProfileString(RegistryKey, RegistryValueName, TC::ToWide(PortableFunctions::PathGetDirectory(file_paths.front())).c_str());

    AddLogs(std::move(file_paths));
}


void ParadataConcatDlg::OnDropFiles(std::vector<std::string> file_paths)
{
    const int starting_number_logs = m_paradataLogList.GetItemCount();

    AddLogs(std::move(file_paths));

    m_paradataLogList.EnsureVisible(starting_number_logs - 1, FALSE);
}


void ParadataConcatDlg::OnRemoveLogs()
{
    if( m_paradataLogFilePaths.empty() )
    {
        AfxMessageBox(L"There are no paradata logs to remove.");
        return;
    }

    int first_selection = m_paradataLogList.GetSelectionMark();
    POSITION pos = m_paradataLogList.GetFirstSelectedItemPosition();
    std::vector<int> positions_to_rmeove;

    while( pos != nullptr )
        positions_to_rmeove.emplace_back(m_paradataLogList.GetNextSelectedItem(pos));

    for( auto itr = positions_to_rmeove.rbegin(); itr != positions_to_rmeove.rend(); ++itr )
    {
        const std::string file_path = Path::Combine(TC::ToUtf8(m_paradataLogList.GetItemText(*itr, 1)),
                                                    TC::ToUtf8(m_paradataLogList.GetItemText(*itr, 0)));
        ASSERT(m_paradataLogFilePaths.find(file_path) != m_paradataLogFilePaths.end());

        m_paradataLogFilePaths.erase(file_path);

        m_paradataLogList.DeleteItem(*itr);
    }

    if( first_selection >= m_paradataLogList.GetItemCount() )
        first_selection = m_paradataLogList.GetItemCount() - 1;

    m_paradataLogList.SetItemState(first_selection, LVIS_SELECTED | LVIS_FOCUSED,LVIS_SELECTED | LVIS_FOCUSED);
    m_paradataLogList.SetFocus();

    UpdateNumberLogsText();
}


void ParadataConcatDlg::OnClearLogs()
{
    if( m_paradataLogList.GetItemCount() == 0 )
        return;

    const std::string prompt = FormatText("Are you sure that you want to clear %d log%s?",
                                          m_paradataLogList.GetItemCount(), PluralizeWord(m_paradataLogList.GetItemCount()));

    if( AfxMessageBox(prompt, MB_YESNOCANCEL) != IDYES )
        return;

    m_paradataLogList.DeleteAllItems();
    m_paradataLogFilePaths.clear();

    UpdateNumberLogsText();
}


void ParadataConcatDlg::OnFileOpen()
{
    OpenFileDlg open_file_dlg(0, FileExtensions::Pff, nullptr, FileFilters::Pff, this);
    open_file_dlg.SetTitle(L"Select Input PFF");

    if( open_file_dlg.DoModal() != IDOK )
        return;

    PFF pff(UTF8_TODO::GetCString(open_file_dlg.GetFilePath()));

    if( !pff.LoadPifFile() || pff.GetAppType() != PARADATA_CONCAT_TYPE )
    {
        AfxMessageBox(L"The PFF could not be read or was not a Paradata Concatenator PFF");
        return;
    }

    m_outputFilePath = UTF8_TODO::GetUtf8(pff.GetOutputParadataFilename());

    OnClearLogs();

    std::vector<std::string> file_paths;

    for( const CString& file_path : pff.GetInputParadataFilenames() )
        file_paths.emplace_back(UTF8_TODO::GetUtf8(file_path));

    AddLogs(std::move(file_paths));

    UpdateData(FALSE);
}


bool ParadataConcatDlg::ValidateGuiParameters()
{
    UpdateData(TRUE);

    try
    {
        if( SO::IsBlank(m_outputFilePath) )
            throw CSProException("You must specify an output filename");

        if( m_paradataLogFilePaths.empty() )
            throw CSProException("You must specify at least one input paradata log");

        return true;
    }

    catch( const CSProException& exception )
    {
        ErrorMessage::Display(exception);
        return false;
    }
}


std::unique_ptr<PFF> ParadataConcatDlg::CreatePffFromGuiParameters(const std::string& pff_file_path)
{
    auto pff = std::make_unique<PFF>();

    pff->SetAppType(PARADATA_CONCAT_TYPE);
    pff->SetPifFileName(UTF8_TODO::GetCString(pff_file_path));
    pff->SetOutputParadataFilename(UTF8_TODO::GetCString(m_outputFilePath));

    for( const std::string& file_path : m_paradataLogFilePaths )
        pff->AddInputParadataFilenames(UTF8_TODO::GetCString(file_path));

    pff->SetListingFName(UTF8_TODO::GetCString(PortableFunctions::PathReplaceFileExtension(pff_file_path, FileExtensions::Listing)));

    return pff;
}


void ParadataConcatDlg::OnFileSaveAs()
{
    if( !ValidateGuiParameters() )
        return;

    SaveFileDlg save_file_dlg(0, FileExtensions::Pff, nullptr, FileFilters::Pff, this);
    save_file_dlg.SetTitle(L"Select Output PFF");

    if( save_file_dlg.DoModal() != IDOK )
        return;

    std::unique_ptr<PFF> pff = CreatePffFromGuiParameters(save_file_dlg.GetFilePath());
    pff->Save();
}


void ParadataConcatDlg::OnFileRun()
{
    if( !ValidateGuiParameters() )
        return;

    const std::unique_ptr<const PFF> pff = CreatePffFromGuiParameters(Path::Combine(GetTempDirectory(), "ParadataConcat.pff"));

    try
    {
        Paradata::GuiConcatenator::Run(*pff);
    }

    catch( const CSProException& exception )
    {
        ErrorMessage::Display(exception);
    }
}
