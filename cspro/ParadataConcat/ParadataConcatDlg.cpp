#include "stdafx.h"
#include "ParadataConcatDlg.h"
#include <zUtilO/imsaDlg.H>
#include <zUtilO/Filedlg.h>
#include <zUtilO/WindowsWS.h>
#include <zParadataO/GuiConcatenator.h>


BEGIN_MESSAGE_MAP(CParadataConcatDlg,CDialog)
    ON_COMMAND(ID_FILE_OPEN,OnFileOpen)
    ON_COMMAND(ID_FILE_SAVE_AS,OnFileSaveAs)
    ON_COMMAND(ID_FILE_RUN,OnFileRun)
    ON_COMMAND(ID_HELP,OnHelp)
    ON_COMMAND(ID_APP_ABOUT,OnAppAbout)
    ON_COMMAND(IDC_BROWSE_OUTPUT,OnBrowseOutput)
    ON_BN_CLICKED(IDC_ADD_LOGS,OnAddLogs)
    ON_BN_CLICKED(IDC_REMOVE_LOGS,OnRemoveLogs)
    ON_BN_CLICKED(IDC_CLEAR_LOGS,OnClearLogs)
    ON_BN_CLICKED(IDC_RUN,OnFileRun)
END_MESSAGE_MAP()


namespace
{
    constexpr const TCHAR* RegistryKey       = _T("Settings");
    constexpr const TCHAR* RegistryValueName = _T("Last Data Folder");
}


CParadataConcatDlg::CParadataConcatDlg(CWnd* pParent /*= nullptr*/)
    :   CDialog(CParadataConcatDlg::IDD, pParent)
{
    m_hIcon = AfxGetApp()->LoadIcon(IDR_MAINFRAME);
}


void CParadataConcatDlg::DoDataExchange(CDataExchange* pDX)
{
    CDialog::DoDataExchange(pDX);

    DDX_Text(pDX, IDC_OUTPUT_FILENAME, m_outputFilename, true);
    DDX_Control(pDX, IDC_LIST_LOGS, m_ParadataLogList);
}


BOOL CParadataConcatDlg::OnInitDialog()
{
    CDialog::OnInitDialog();

    // add the menu
    CMenu menu;
    menu.LoadMenu(IDR_PARADATACONCAT_MENU);
    SetMenu(&menu);

    // set the icons
    SetIcon(m_hIcon,TRUE);
    SetIcon(m_hIcon,FALSE);

    // set up the list control
    m_ParadataLogList.SetExtendedStyle(LVS_EX_FULLROWSELECT);
    m_ParadataLogList.SetHeadings(_T("Name,180;Path,220;Date,120;Events,90;Size,90"));
    m_ParadataLogList.LoadColumnInfo();

    // set up the callback to allow the dragging of files onto the list of logs
    m_ParadataLogList.InitializeDropFiles(DropFilesListCtrl::DirectoryHandling::RecurseInto, FileExtensions::Wildcard::Paradata,
        [&](std::vector<std::wstring> paths)
        {
            OnDropFiles(paths);
        });

    return TRUE;
}


void CParadataConcatDlg::OnAppAbout()
{
    CString csWindowTitle;
    GetWindowText(csWindowTitle);

    CIMSAAboutDlg dlg;
    dlg.m_hIcon = m_hIcon;
    dlg.m_csModuleName = csWindowTitle;
    dlg.DoModal();
}


void CParadataConcatDlg::OnBrowseOutput()
{
    UpdateData(TRUE);

    CString csSingleParadataLogFilter;
    csSingleParadataLogFilter.Format(_T("Paradata Log (%s)|%s||"), FileExtensions::Wildcard::Paradata, FileExtensions::Wildcard::Paradata);

    CIMSAFileDialog saveDlg(FALSE, FileExtensions::Paradata, m_outputFilename.c_str(), OFN_HIDEREADONLY, csSingleParadataLogFilter);
    saveDlg.m_ofn.lpstrTitle = _T("Select Output Paradata Log");

    if( saveDlg.DoModal() != IDOK )
        return;

    std::wstring output_filename = saveDlg.GetPathName();
    bool bUpdateFilename = true;

    if( m_paradataLogFilenames.find(output_filename) == m_paradataLogFilenames.end()  &&
        Paradata::GuiConcatenator::GetNumberEvents(output_filename) > 0 )
    {
        int result = MessageBox(_T("The output file has events in it. Do you want to include these as inputs?"),
                                _T("Include events?"), MB_YESNOCANCEL | MB_ICONEXCLAMATION);

        if( result == IDYES )
        {
            AddLogs({ output_filename });
        }

        else if( result == IDCANCEL )
        {
            bUpdateFilename = false;
        }
    }

    if( bUpdateFilename )
    {
        m_outputFilename = WS2CS(output_filename);
        UpdateData(FALSE);
    }
}


void CParadataConcatDlg::UpdateNumberLogsText()
{
    GetDlgItem(IDC_NUMBER_LOGS)->SetWindowText(FormatText(_T("%d log%s"),
                                                          (int)m_paradataLogFilenames.size(),
                                                          PluralizeWord(m_paradataLogFilenames.size())));
}


void CParadataConcatDlg::AddLogs(const std::vector<std::wstring>& filenames)
{
    for( const std::wstring& filename : filenames )
    {
        CFileStatus fStatus;
        CFile::GetStatus(filename.c_str(), fStatus);

        if( m_paradataLogFilenames.find(fStatus.m_szFullName) != m_paradataLogFilenames.end() )
            continue; // no reason to include the same paradata log file twice

        int64_t iEvents = Paradata::GuiConcatenator::GetNumberEvents(filename);

        if( iEvents < 0 )
            continue; // this was not a valid paradata log file

        CString csEvents = IntToString(iEvents);
        CString csDate = fStatus.m_mtime.Format(_T("%c"));
        const std::wstring size = PortableFunctions::FileSizeString(fStatus.m_size);

        m_ParadataLogList.AddItem(PortableFunctions::PathGetFilename(fStatus.m_szFullName),
                                  PortableFunctions::PathGetDirectory(fStatus.m_szFullName).c_str(),
                                  csDate.GetString(),
                                  csEvents.GetString(),
                                  size.c_str());

        m_paradataLogFilenames.insert(fStatus.m_szFullName);
    }

    UpdateNumberLogsText();
}


void CParadataConcatDlg::OnAddLogs()
{
    CString csMultipleParadataLogsFilter;
    csMultipleParadataLogsFilter.Format(_T("Paradata Log Files (%s)|%s|All Files (*.*)|*.*||"), FileExtensions::Wildcard::Paradata, FileExtensions::Wildcard::Paradata);

    CString csInitialDirectory = AfxGetApp()->GetProfileString(RegistryKey,RegistryValueName);

    CIMSAFileDialog openDlg(TRUE, nullptr, nullptr, OFN_HIDEREADONLY | OFN_ALLOWMULTISELECT, csMultipleParadataLogsFilter, AfxGetApp()->GetMainWnd(), CFD_NO_DIR);
    openDlg.m_ofn.lpstrInitialDir = csInitialDirectory;
    openDlg.m_ofn.lpstrTitle = _T("Select Paradata Logs to Concatenate");
    openDlg.SetMultiSelectBuffer();

    if( openDlg.DoModal() != IDOK )
        return;

    std::vector<std::wstring> filenames;

    for( int i = 0; i < openDlg.m_aFileName.GetSize(); ++i )
    {
        filenames.emplace_back(openDlg.m_aFileName[i]);

        if( i == 0 )
            AfxGetApp()->WriteProfileString(RegistryKey, RegistryValueName, PortableFunctions::PathGetDirectory(openDlg.m_aFileName[0]).c_str());
    }

    AddLogs(filenames);
}


void CParadataConcatDlg::OnDropFiles(const std::vector<std::wstring>& filenames)
{
    int iStartingNumberLogs = m_ParadataLogList.GetItemCount();

    AddLogs(filenames);

    m_ParadataLogList.EnsureVisible(iStartingNumberLogs - 1, FALSE);
}


void CParadataConcatDlg::OnRemoveLogs()
{
    if( m_paradataLogFilenames.empty() )
    {
        AfxMessageBox(_T("There are no paradata logs to remove."));
        return;
    }

    int iFirstSelection = m_ParadataLogList.GetSelectionMark();
    POSITION pos = m_ParadataLogList.GetFirstSelectedItemPosition();
    std::vector<int> aPositionsToRemove;

    while( pos != nullptr )
        aPositionsToRemove.emplace_back(m_ParadataLogList.GetNextSelectedItem(pos));

    for( auto itr = aPositionsToRemove.rbegin(); itr != aPositionsToRemove.rend(); itr++ )
    {
        CString csFilenameOnly = m_ParadataLogList.GetItemText(*itr,0);
        CString csDirectory = m_ParadataLogList.GetItemText(*itr,1);
        std::wstring filename = CS2WS(PortableFunctions::PathAppendToPath(csDirectory, csFilenameOnly));

        ASSERT(m_paradataLogFilenames.find(filename) != m_paradataLogFilenames.end());

        m_paradataLogFilenames.erase(filename);

        m_ParadataLogList.DeleteItem(*itr);
    }

    if( iFirstSelection >= m_ParadataLogList.GetItemCount() )
        iFirstSelection = m_ParadataLogList.GetItemCount() - 1;

    m_ParadataLogList.SetItemState(iFirstSelection,LVIS_SELECTED | LVIS_FOCUSED,LVIS_SELECTED | LVIS_FOCUSED);
    m_ParadataLogList.SetFocus();

    UpdateNumberLogsText();
}


void CParadataConcatDlg::OnClearLogs()
{
    if( m_ParadataLogList.GetItemCount() == 0 )
        return;

    const std::wstring prompt = FormatTextCS2WS(_T("Are you sure that you want to clear %d log%s?"),
                                                m_ParadataLogList.GetItemCount(), PluralizeWord(m_ParadataLogList.GetItemCount()));

    if( AfxMessageBox(prompt, MB_YESNOCANCEL) != IDYES )
        return;

    m_ParadataLogList.DeleteAllItems();
    m_paradataLogFilenames.clear();

    UpdateNumberLogsText();
}


void CParadataConcatDlg::OnFileOpen()
{
    CIMSAFileDialog openDlg(TRUE,FileExtensions::Pff,nullptr,OFN_HIDEREADONLY,FileFilters::Pff);
    openDlg.m_ofn.lpstrTitle = _T("Select Input PFF");

    if( openDlg.DoModal() != IDOK )
        return;

    PFF pff(openDlg.GetPathName());

    if( !pff.LoadPifFile() || pff.GetAppType() != PARADATA_CONCAT_TYPE )
    {
        AfxMessageBox(_T("The PFF could not be read or was not a Paradata Concatenator PFF"));
        return;
    }

    m_outputFilename = pff.GetOutputParadataFilename();

    OnClearLogs();

    std::vector<std::wstring> filenames;

    for( const CString& filename : pff.GetInputParadataFilenames() )
        filenames.emplace_back(filename);

    AddLogs(filenames);

    UpdateData(FALSE);
}


bool CParadataConcatDlg::ValidateGuiParameters()
{
    bool bValidated = false;

    UpdateData(TRUE);

    if( SO::IsBlank(m_outputFilename) )
    {
        AfxMessageBox(_T("You must specify an output filename"));
    }

    else if( m_paradataLogFilenames.empty() )
    {
        AfxMessageBox(_T("You must specify at least one input paradata log"));
    }

    else
    {
        bValidated = true;
    }

    return bValidated;
}


std::unique_ptr<PFF> CParadataConcatDlg::CreatePffFromGuiParameters(NullTerminatedString pff_filename)
{
    auto pff = std::make_unique<PFF>();

    pff->SetAppType(PARADATA_CONCAT_TYPE);
    pff->SetPifFileName(pff_filename.c_str());
    pff->SetOutputParadataFilename(WS2CS(m_outputFilename));

    for( const std::wstring& filename : m_paradataLogFilenames )
        pff->AddInputParadataFilenames(filename.c_str());

    pff->SetListingFName(PortableFunctions::PathRemoveFileExtension<CString>(pff_filename) + FileExtensions::WithDot::Listing);

    return pff;
}


void CParadataConcatDlg::OnFileSaveAs()
{
    if( !ValidateGuiParameters() )
        return;

    CIMSAFileDialog saveDlg(FALSE,FileExtensions::Pff,nullptr,OFN_HIDEREADONLY,FileFilters::Pff);
    saveDlg.m_ofn.lpstrTitle = _T("Select Output PFF");

    if( saveDlg.DoModal() != IDOK )
        return;

    std::unique_ptr<PFF> pff = CreatePffFromGuiParameters(saveDlg.GetPathName());
    pff->Save();
}


void CParadataConcatDlg::OnFileRun()
{
    if( !ValidateGuiParameters() )
        return;

    std::unique_ptr<PFF> pff = CreatePffFromGuiParameters(GetTempDirectory() + _T("ParadataConcat.pff"));

    try
    {
        Paradata::GuiConcatenatorPffWrapper pff_wrapper(*pff);
        Paradata::GuiConcatenator::Run(pff_wrapper);
    }

    catch( const CSProException& exception )
    {
        ErrorMessage::Display(exception);
    }
}
