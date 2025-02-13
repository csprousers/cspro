#include "stdafx.h"
#include "IndexDlg.h"
#include "ToolIndexer.h"
#include <zUtilO/Filedlg.h>
#include <zUtilO/imsaDlg.H>
#include <zUtilO/PathHelpers.h>
#include <ZBRIDGEO/DataFileDlg.h>
#include <iterator>


namespace
{
    constexpr int ActionOutput = 0;
    constexpr int ActionView = 1;
    constexpr int ActionPrompt = 2;
    constexpr int ActionAutoDelete = 3;

    constexpr const TCHAR* OutputConnectionStringOptionsLinkCtrlText =
        _T("If Output Data is a proper filename, then all input files will be concatenated into that single file. Alternatively, if ")
        _T("Output Data includes ") IndexerFilenameWildcard _T(", each input file will be output to a separate file with a new name based on the ")
        _T("input filename. For example, ") IndexerFilenameWildcard _T("-fixed, would append -fixed after each filename (in front of the extension).");
}


BEGIN_MESSAGE_MAP(IndexDlg, CDialog)
    ON_MESSAGE(UWM::CSIndex::UpdateDialogUI, OnUpdateDialogUI)
    ON_COMMAND(ID_HELP, OnHelp)
    ON_COMMAND(ID_APP_ABOUT, OnAppAbout)
    ON_COMMAND(ID_FILE_OPEN, OnFileOpen)
    ON_COMMAND(ID_FILE_SAVE_AS, OnFileSaveAs)
    ON_EN_CHANGE(IDC_DICTIONARY_FILE, OnChangeUpdateDialogUI)
    ON_BN_CLICKED(IDC_DICTIONARY_BROWSE, OnDictionaryBrowse)
    ON_BN_CLICKED(IDC_ADD_FILES, OnAddFiles)
    ON_BN_CLICKED(IDC_REMOVE_FILES, OnRemoveFiles)
    ON_BN_CLICKED(IDC_CLEAR_FILES, OnClearFiles)
    ON_BN_CLICKED(IDC_ACTION_OUTPUT, OnChangeUpdateDialogUI)
    ON_BN_CLICKED(IDC_ACTION_VIEW, OnChangeUpdateDialogUI)
    ON_BN_CLICKED(IDC_ACTION_PROMPT, OnChangeUpdateDialogUI)
    ON_BN_CLICKED(IDC_ACTION_AUTO_DELETE, OnChangeUpdateDialogUI)
    ON_EN_CHANGE(IDC_OUTPUT_FILE, OnOutputEdit)
    ON_BN_CLICKED(IDC_OUTPUT_BROWSE, OnOutputBrowse)
    ON_COMMAND(ID_FILE_RUN, OnRun)
    ON_BN_CLICKED(IDC_RUN, OnRun)
END_MESSAGE_MAP()


IndexDlg::IndexDlg(std::wstring initial_dictionary_filename, CWnd* pParent/* = nullptr*/)
    :   CDialog(IndexDlg::IDD, pParent),
        m_hIcon(AfxGetApp()->LoadIcon(IDR_MAINFRAME)),
        m_dictionaryFilename(std::move(initial_dictionary_filename)),
        m_action(ActionOutput),
        m_autoDeleteIdentical(TRUE),
        m_suggestOutputConnectionString(true),
        m_outputConnectionStringOptionsLinkCtrl(OutputConnectionStringOptionsLinkCtrlText)
{
    SetDefaultPffSettings();
}


void IndexDlg::DoDataExchange(CDataExchange* pDX)
{
    CDialog::DoDataExchange(pDX);

    DDX_Text(pDX, IDC_DICTIONARY_FILE, m_dictionaryFilename);
    DDX_Control(pDX, IDC_FILE_LIST, m_fileList);
    DDX_Radio(pDX, IDC_ACTION_OUTPUT, m_action);
    DDX_Check(pDX, IDC_ACTION_AUTO_DELETE_IDENTICAL, m_autoDeleteIdentical);
    DDX_Text(pDX, IDC_OUTPUT_FILE, m_outputConnectionString);
    DDX_Control(pDX, IDC_OUTPUT_FILE_OPTIONS_LINK, m_outputConnectionStringOptionsLinkCtrl);
}


BOOL IndexDlg::OnInitDialog()
{
    CDialog::OnInitDialog();

    // add the menu
    m_menu.LoadMenu(IDR_CSINDEX_MENU);
    SetMenu(&m_menu);

    // set the icons
    SetIcon(m_hIcon, TRUE);
    SetIcon(m_hIcon, FALSE);

    m_fileList.SetExtendedStyle(LVS_EX_FULLROWSELECT);
    m_fileList.SetHeadings(_T("Name,250;Directory,450"));
    m_fileList.LoadColumnInfo();

    // set up the callback to allow the dragging of files onto the list of data to index
    m_fileList.InitializeDropFiles(DropFilesListCtrl::DirectoryHandling::RecurseInto,
        [&](const std::vector<std::wstring>& paths)
        {
            OnDropFiles(paths);
        });

    PostMessage(UWM::CSIndex::UpdateDialogUI);

    return TRUE;
}


LRESULT IndexDlg::OnUpdateDialogUI(WPARAM /*wParam*/, LPARAM /*lParam*/)
{
    // process the action options
    const bool enable_output_file = ( m_action == ActionPrompt || m_action == ActionAutoDelete );

    // fill in a default output filename
    if( enable_output_file && m_suggestOutputConnectionString )
    {
        m_outputConnectionString = ConnectionString(wstring_view(IndexerFilenameWildcard _T("-fixed")));
        UpdateData(FALSE);
    }

    GetDlgItem(IDC_ACTION_AUTO_DELETE_IDENTICAL)->EnableWindow(( m_action == ActionPrompt ));

    GetDlgItem(IDC_OUTPUT_FILE)->EnableWindow(enable_output_file);
    GetDlgItem(IDC_OUTPUT_BROWSE)->EnableWindow(enable_output_file);

    // handle the run button
    bool enable_run = false;

    if( !SO::IsBlank(m_dictionaryFilename) && m_fileList.GetItemCount() > 0 )
        enable_run = ( !enable_output_file || m_outputConnectionString.IsDefined() );

    m_menu.EnableMenuItem(ID_FILE_RUN, enable_run ? MF_ENABLED : MF_DISABLED);
    GetDlgItem(IDC_RUN)->EnableWindow(enable_run);

    // update the number of files
    WindowsWS::SetDlgItemText(this, IDC_NUMBER_FILES,
                              FormatTextCS2WS(_T("%d file%s"), m_fileList.GetItemCount(), PluralizeWord(m_fileList.GetItemCount())));

    return 0;
}


void IndexDlg::OnChangeUpdateDialogUI()
{
    UpdateData(TRUE);
    PostMessage(UWM::CSIndex::UpdateDialogUI);
}


void IndexDlg::OnAppAbout()
{
    CIMSAAboutDlg about_dlg(WindowsWS::LoadString(AFX_IDS_APP_TITLE), m_hIcon);
    about_dlg.DoModal();
}


void IndexDlg::SetDefaultPffSettings()
{
    m_pff.ResetContents();
    m_pff.SetAppType(APPTYPE::INDEX_TYPE);
    m_pff.SetDuplicateCase(DuplicateCase::List);
    m_pff.SetViewListing(VIEWLISTING::ALWAYS);
}


void IndexDlg::OnFileOpen()
{
    CIMSAFileDialog file_dlg(TRUE, FileExtensions::Pff, nullptr, OFN_HIDEREADONLY, FileFilters::Pff);
    file_dlg.m_ofn.lpstrTitle = _T("Select Input PFF");

    if( file_dlg.DoModal() != IDOK )
        return;

    m_pff.ResetContents();
    m_pff.SetPifFileName(file_dlg.GetPathName());

    if( !m_pff.LoadPifFile() || m_pff.GetAppType() != INDEX_TYPE )
    {
        AfxMessageBox(_T("The PFF could not be read or was not a Index Data PFF."));
        SetDefaultPffSettings();
    }

    m_dictionaryFilename = m_pff.GetInputDictFName();

    m_fileList.DeleteAllItems();
    AddConnectionStrings(m_pff.GetInputDataConnectionStrings());

    m_action = ( m_pff.GetDuplicateCase() == DuplicateCase::List )      ? ActionOutput :
               ( m_pff.GetDuplicateCase() == DuplicateCase::View )      ? ActionView :
               ( m_pff.GetDuplicateCase() == DuplicateCase::KeepFirst ) ? ActionAutoDelete :
                                                                          ActionPrompt;

    m_autoDeleteIdentical = ( m_pff.GetDuplicateCase() == DuplicateCase::PromptIfDifferent ) ? TRUE : FALSE;

    m_outputConnectionString = m_pff.GetSingleOutputDataConnectionString();

    // don't show the full path if using the wildcard
    if( m_outputConnectionString.IsFilenamePresent() )
    {
        const std::wstring filename = PortableFunctions::PathGetFilename(m_outputConnectionString.GetFilename());

        if( filename.find(IndexerFilenameWildcard) != std::wstring::npos )
            m_outputConnectionString = ConnectionString(filename);
    }

    m_suggestOutputConnectionString = false;

    UpdateData(FALSE);
    PostMessage(UWM::CSIndex::UpdateDialogUI);
}


void IndexDlg::UIToPff()
{
    UpdateData(TRUE);

    m_pff.SetInputDictFName(WS2CS(m_dictionaryFilename));

    m_pff.ClearInputDataConnectionStrings();

    for( int i = 0; i < m_fileList.GetItemCount(); ++i )
    {
        const std::wstring name = m_fileList.GetItemText(i, 0);
        const std::wstring directory = m_fileList.GetItemText(i, 1);

        m_pff.AddInputDataConnectionString(WS2CS(!directory.empty() ? MakeFullPath(directory, name) :
                                                                      name));
    }

    m_pff.SetDuplicateCase(( m_action == ActionOutput )     ? DuplicateCase::List :
                           ( m_action == ActionView )       ? DuplicateCase::View :
                           ( m_action == ActionAutoDelete ) ? DuplicateCase::KeepFirst :
                           m_autoDeleteIdentical            ? DuplicateCase::PromptIfDifferent :
                                                              DuplicateCase::Prompt);

    m_pff.SetSingleOutputDataConnectionString(m_outputConnectionString);
}


void IndexDlg::OnFileSaveAs()
{
    CIMSAFileDialog file_dlg(FALSE, FileExtensions::Pff, m_pff.GetPifFileName(), OFN_HIDEREADONLY, FileFilters::Pff);
    file_dlg.m_ofn.lpstrTitle = _T("Select Output PFF");

    if( file_dlg.DoModal() != IDOK )
        return;

    m_pff.SetPifFileName(file_dlg.GetPathName());

    UIToPff();

    // base the listing filename on the PFF filename
    if( m_pff.GetListingFName().IsEmpty() )
        m_pff.SetListingFName(WS2CS(PortableFunctions::PathReplaceFileExtension(m_pff.GetPifFileName(), FileExtensions::WithDot::Listing)));

    m_pff.Save();
}


void IndexDlg::OnDictionaryBrowse()
{
    UpdateData(TRUE);

    CIMSAFileDialog file_dlg(TRUE, FileExtensions::Dictionary, m_dictionaryFilename.c_str(), 0,
                             _T("Data Dictionary Files (*.dcf)|*.dcf|All Files (*.*)|*.*||"));
    file_dlg.m_ofn.lpstrTitle =  _T("Select Dictionary File");

    if( file_dlg.DoModal() != IDOK )
        return;

    m_dictionaryFilename = file_dlg.GetPathName();

    UpdateData(FALSE);
    PostMessage(UWM::CSIndex::UpdateDialogUI);
}


void IndexDlg::AddConnectionStrings(const std::vector<ConnectionString>& connection_strings)
{
    for( const ConnectionString& connection_string : connection_strings )
    {
        for( const ConnectionString& expanded_connection_string : PathHelpers::ExpandConnectionStringWildcards(connection_string) )
        {
            std::wstring name = expanded_connection_string.ToStringWithoutDirectory();
            std::wstring directory;

            if( expanded_connection_string.IsFilenamePresent() )
                directory = PortableFunctions::PathGetDirectory(expanded_connection_string.GetFilename());

            m_fileList.AddItem(name.c_str(), directory.c_str());
        }
    }
}


void IndexDlg::OnAddFiles()
{
    DataFileDlg data_file_dlg(DataFileDlg::Type::OpenExisting, true);
    data_file_dlg.SetTitle(_T("Select Files To Index"))
                 .SetDictionaryFilename(WS2CS(m_dictionaryFilename))
                 .AllowMultipleSelections();

    if( data_file_dlg.DoModal() != IDOK )
        return;

    AddConnectionStrings(data_file_dlg.GetConnectionStrings());

    PostMessage(UWM::CSIndex::UpdateDialogUI);
}


void IndexDlg::OnDropFiles(const std::vector<std::wstring>& filenames)
{
    std::vector<ConnectionString> connection_strings;

    std::transform(filenames.cbegin(), filenames.cend(),
                   std::back_inserter(connection_strings), [](const std::wstring& filename) { return ConnectionString(filename); });

    const int initial_number_files = m_fileList.GetItemCount();

    AddConnectionStrings(connection_strings);

    m_fileList.EnsureVisible(initial_number_files - 1, FALSE);

    PostMessage(UWM::CSIndex::UpdateDialogUI);
}


void IndexDlg::OnRemoveFiles()
{
    if( m_fileList.GetItemCount() == 0 )
    {
        AfxMessageBox(_T("No files to remove."));
        return;
    }

    POSITION file_list_pos = m_fileList.GetFirstSelectedItemPosition();

    if( file_list_pos == nullptr )
    {
       AfxMessageBox(_T("No files selected."));
       return;
    }

    std::vector<int> indices_to_remove;

    while( file_list_pos != nullptr )
        indices_to_remove.emplace_back(m_fileList.GetNextSelectedItem(file_list_pos));

    for( auto index_to_remove = indices_to_remove.crbegin();
         index_to_remove != indices_to_remove.crend();
         ++index_to_remove )
    {
        m_fileList.DeleteItem(*index_to_remove);
    }

    const int new_index_to_select = std::min(indices_to_remove.front(), m_fileList.GetItemCount() - 1);

    m_fileList.SetItemState(new_index_to_select, LVIS_SELECTED | LVIS_FOCUSED, LVIS_SELECTED | LVIS_FOCUSED);
    m_fileList.SetFocus();

    PostMessage(UWM::CSIndex::UpdateDialogUI);
}


void IndexDlg::OnClearFiles()
{
    if( m_fileList.GetItemCount() == 0 )
        return;

    const std::wstring prompt = FormatTextCS2WS(_T("Are you sure that you want to clear %d file%s?"),
                                                m_fileList.GetItemCount(), PluralizeWord(m_fileList.GetItemCount()));

    if( AfxMessageBox(prompt, MB_YESNOCANCEL) != IDYES )
        return;

    m_fileList.DeleteAllItems();
    PostMessage(UWM::CSIndex::UpdateDialogUI);
}


void IndexDlg::OnOutputEdit()
{
    m_suggestOutputConnectionString = false;
    OnChangeUpdateDialogUI();
}


void IndexDlg::OnOutputBrowse()
{
    // update the PFF so that the connection strings can be passed in to SuggestMatchingDataRepositoryType
    UIToPff();

    DataFileDlg data_file_dlg(DataFileDlg::Type::CreateNew, false, m_outputConnectionString);
    data_file_dlg.SetDictionaryFilename(WS2CS(m_dictionaryFilename))
                 .SuggestMatchingDataRepositoryType(m_pff.GetInputDataConnectionStrings())
                 .WarnIfDifferentDataRepositoryType();

    if( data_file_dlg.DoModal() != IDOK )
        return;

    m_outputConnectionString = data_file_dlg.GetConnectionString();
    m_suggestOutputConnectionString = false;

    UpdateData(FALSE);
}


void IndexDlg::OnRun()
{
    UIToPff();

    // if the listing file hasn't been defined, put it in the same folder as the PFF, or in the temporary folder if the PFF hasn't been saved
    const bool use_temporary_listing_filename = m_pff.GetListingFName().IsEmpty();

    if( use_temporary_listing_filename )
    {
        m_pff.SetListingFName(WS2CS(m_pff.GetPifFileName().IsEmpty() ? PortableFunctions::PathAppendToPath(GetTempDirectory(), _T("CSIndex.lst")) :
                                                                       PortableFunctions::PathReplaceFileExtension(m_pff.GetPifFileName(), FileExtensions::Listing)));
    }

    try
    {
        ToolIndexer().Run(m_pff, false);
    }

    catch( const CSProException& exception )
    {
        ErrorMessage::Display(exception);
    }

    if( use_temporary_listing_filename )
        m_pff.SetListingFName(CString());
}
