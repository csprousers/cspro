#include "stdafx.h"
#include "IndexDlg.h"
#include "ToolIndexer.h"
#include <zUtilO/FileDlg.h>
#include <zUtilO/imsaDlg.H>
#include <zUtilO/PathHelpers.h>
#include <zBridgeO/DataFileDlg.h>
#include <iterator>


namespace
{
    constexpr int ActionOutput     = 0;
    constexpr int ActionView       = 1;
    constexpr int ActionPrompt     = 2;
    constexpr int ActionAutoDelete = 3;

    constexpr const wchar_t* OutputConnectionStringOptionsLinkCtrlText =
        L"If Output Data is a proper filename, then all input files will be concatenated into that single file. Alternatively, if "
        L"Output Data includes " _T(IndexerFilenameWildcard) L", each input file will be output to a separate file with a new name based on the "
        L"input filename. For example, " _T(IndexerFilenameWildcard) L"-fixed, would append -fixed after each filename (in front of the extension).";
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


IndexDlg::IndexDlg(std::string initial_dictionary_file_path, CWnd* const pParent/* = nullptr*/)
    :   CDialog(IndexDlg::IDD, pParent),
        m_hIcon(AfxGetApp()->LoadIcon(IDR_MAINFRAME)),
        m_dictionaryFilePath(std::move(initial_dictionary_file_path)),
        m_action(ActionOutput),
        m_autoDeleteIdentical(TRUE),
        m_suggestOutputConnectionString(true),
        m_outputConnectionStringOptionsLinkCtrl(OutputConnectionStringOptionsLinkCtrlText)
{
    SetDefaultPffSettings();
}


void IndexDlg::DoDataExchange(CDataExchange* const pDX)
{
    __super::DoDataExchange(pDX);

    DDX_Text(pDX, IDC_DICTIONARY_FILE, m_dictionaryFilePath);
    DDX_Control(pDX, IDC_FILE_LIST, m_fileList);
    DDX_Radio(pDX, IDC_ACTION_OUTPUT, m_action);
    DDX_Check(pDX, IDC_ACTION_AUTO_DELETE_IDENTICAL, m_autoDeleteIdentical);
    DDX_Text(pDX, IDC_OUTPUT_FILE, m_outputConnectionString);
    DDX_Control(pDX, IDC_OUTPUT_FILE_OPTIONS_LINK, m_outputConnectionStringOptionsLinkCtrl);
}


BOOL IndexDlg::OnInitDialog()
{
    __super::OnInitDialog();

    // add the menu
    m_menu.LoadMenu(IDR_CSINDEX_MENU);
    SetMenu(&m_menu);

    // set the icons
    SetIcon(m_hIcon, TRUE);
    SetIcon(m_hIcon, FALSE);

    m_fileList.SetExtendedStyle(LVS_EX_FULLROWSELECT);
    m_fileList.SetHeadings(L"Name,250;Directory,450");
    m_fileList.LoadColumnInfo();

    // set up the callback to allow the dragging of files onto the list of data to index
    m_fileList.InitializeDropFiles(DropFilesListCtrl::DirectoryHandling::RecurseInto,
        [&](const std::vector<std::string>& paths)
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
        m_outputConnectionString = ConnectionString(std::string_view(IndexerFilenameWildcard "-fixed"));
        UpdateData(FALSE);
    }

    GetDlgItem(IDC_ACTION_AUTO_DELETE_IDENTICAL)->EnableWindow(( m_action == ActionPrompt ));

    GetDlgItem(IDC_OUTPUT_FILE)->EnableWindow(enable_output_file);
    GetDlgItem(IDC_OUTPUT_BROWSE)->EnableWindow(enable_output_file);

    // handle the run button
    bool enable_run = false;

    if( !SO::IsWhitespace(m_dictionaryFilePath) && m_fileList.GetItemCount() > 0 )
        enable_run = ( !enable_output_file || m_outputConnectionString.IsDefined() );

    m_menu.EnableMenuItem(ID_FILE_RUN, enable_run ? MF_ENABLED : MF_DISABLED);
    GetDlgItem(IDC_RUN)->EnableWindow(enable_run);

    // update the number of files
    WindowsUtf8::SetText(this, IDC_NUMBER_FILES,
                         FormatText("%d file%s", m_fileList.GetItemCount(), PluralizeWord(m_fileList.GetItemCount())));

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
    OpenFileDlg open_file_dlg(0, FileExtensions::Pff, nullptr, FileFilters::Pff, this);
    open_file_dlg.SetTitle(L"Select Input PFF");

    if( open_file_dlg.DoModal() != IDOK )
        return;

    m_pff.ResetContents();
    m_pff.SetPifFileName(UTF8_TODO::GetCString(open_file_dlg.GetFilePath()));

    if( !m_pff.LoadPifFile() || m_pff.GetAppType() != INDEX_TYPE )
    {
        AfxMessageBox(L"The PFF could not be read or was not a Index Data PFF.");
        SetDefaultPffSettings();
    }

    m_dictionaryFilePath = UTF8_TODO::GetUtf8(m_pff.GetInputDictFName());

    m_fileList.DeleteAllItems();
    AddConnectionStrings(m_pff.GetInputDataConnectionStrings());

    m_action = ( m_pff.GetDuplicateCase() == DuplicateCase::List )      ? ActionOutput :
               ( m_pff.GetDuplicateCase() == DuplicateCase::View )      ? ActionView :
               ( m_pff.GetDuplicateCase() == DuplicateCase::KeepFirst ) ? ActionAutoDelete :
                                                                          ActionPrompt;

    m_autoDeleteIdentical = ( m_pff.GetDuplicateCase() == DuplicateCase::PromptIfDifferent ) ? TRUE : FALSE;

    m_outputConnectionString = m_pff.GetSingleOutputDataConnectionString();

    // don't show the full path if using the wildcard
    if( m_outputConnectionString.HasFilePath() )
    {
        const std::string filename = PortableFunctions::PathGetFilename(m_outputConnectionString.GetFilePath());

        if( filename.find(IndexerFilenameWildcard) != std::string::npos )
            m_outputConnectionString = ConnectionString(filename);
    }

    m_suggestOutputConnectionString = false;

    UpdateData(FALSE);
    PostMessage(UWM::CSIndex::UpdateDialogUI);
}


void IndexDlg::UIToPff()
{
    UpdateData(TRUE);

    m_pff.SetInputDictFName(UTF8_TODO::GetCString(m_dictionaryFilePath));

    m_pff.ClearInputDataConnectionStrings();

    for( int i = 0; i < m_fileList.GetItemCount(); ++i )
    {
        const size_t connection_string_index = m_fileList.GetItemData(i);

        if( connection_string_index < m_fileListConnectionStrings.size() )
        {
            m_pff.AddInputDataConnectionString(m_fileListConnectionStrings[connection_string_index]);
        }

        else
        {
            ASSERT(false);
        }
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
    SaveFileDlg save_file_dlg(0, FileExtensions::Pff, m_pff.GetPifFileName(), FileFilters::Pff, this);
    save_file_dlg.SetTitle(L"Select Output PFF");

    if( save_file_dlg.DoModal() != IDOK )
        return;

    m_pff.SetPifFileName(UTF8_TODO::GetCString(save_file_dlg.GetFilePath()));

    UIToPff();

    // base the listing filename on the PFF filename
    if( m_pff.GetListingFName().IsEmpty() )
        m_pff.SetListingFName(UTF8_TODO::GetCString(PortableFunctions::PathReplaceFileExtension(UTF8_TODO::GetUtf8(m_pff.GetPifFileName()), FileExtensions::Listing)));

    m_pff.Save();
}


void IndexDlg::OnDictionaryBrowse()
{
    UpdateData(TRUE);

    OpenFileDlg open_file_dlg(0, FileExtensions::Dictionary, m_dictionaryFilePath, FileFilters::Dictionary, this);
    open_file_dlg.SetTitle(L"Select Dictionary");

    if( open_file_dlg.DoModal() != IDOK )
        return;

    m_dictionaryFilePath = open_file_dlg.GetFilePath();

    UpdateData(FALSE);
    PostMessage(UWM::CSIndex::UpdateDialogUI);
}


void IndexDlg::AddConnectionStrings(const std::vector<ConnectionString>& connection_strings)
{
    for( const ConnectionString& connection_string : connection_strings )
    {
        for( ConnectionString& expanded_connection_string : PathHelpers::ExpandConnectionStringWildcards(connection_string) )
        {
            auto add = [&](const std::string_view name_sv, const wchar_t* const directory)
            {
                const int pos = m_fileList.AddItem(TC::ToWide(name_sv).c_str(), directory);
                m_fileList.SetItemData(pos, m_fileListConnectionStrings.size());
                m_fileListConnectionStrings.emplace_back(std::move(expanded_connection_string));
            };

            if( expanded_connection_string.HasFilePath() )
            {
                add(PortableFunctions::PathGetFilename(expanded_connection_string.GetFilePath()),
                    TC::ToWide(PortableFunctions::PathGetDirectory(expanded_connection_string.GetFilePath())).c_str());
            }

            else if( expanded_connection_string.HasUrl() )
            {
                add(expanded_connection_string.ToDisplayString(), L"");
            }
        }
    }
}


void IndexDlg::OnAddFiles()
{
    DataFileDlg data_file_dlg(DataFileDlg::Type::OpenExisting, true);
    data_file_dlg.SetTitle(L"Select Data Sources To Index")
                 .SetDictionaryFilePath(m_dictionaryFilePath)
                 .AllowMultipleSelections();

    if( data_file_dlg.DoModal() != IDOK )
        return;

    AddConnectionStrings(data_file_dlg.GetConnectionStrings());

    PostMessage(UWM::CSIndex::UpdateDialogUI);
}


void IndexDlg::OnDropFiles(const std::vector<std::string>& paths)
{
    std::vector<ConnectionString> connection_strings;

    std::transform(paths.cbegin(), paths.cend(),
                   std::back_inserter(connection_strings), [](const std::string& path) { return ConnectionString(path); });

    const int initial_number_files = m_fileList.GetItemCount();

    AddConnectionStrings(connection_strings);

    m_fileList.EnsureVisible(initial_number_files - 1, FALSE);

    PostMessage(UWM::CSIndex::UpdateDialogUI);
}


void IndexDlg::OnRemoveFiles()
{
    if( m_fileList.GetItemCount() == 0 )
    {
        AfxMessageBox(L"No files to remove.");
        return;
    }

    POSITION file_list_pos = m_fileList.GetFirstSelectedItemPosition();

    if( file_list_pos == nullptr )
    {
       AfxMessageBox(L"No files selected.");
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

    const std::string prompt = FormatText("Are you sure that you want to clear %d file%s?",
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
    data_file_dlg.SetDictionaryFilePath(m_dictionaryFilePath)
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
    const bool use_temporary_listing_file = m_pff.GetListingFName().IsEmpty();

    if( use_temporary_listing_file )
    {
        m_pff.SetListingFName(UTF8_TODO::GetCString(m_pff.GetPifFileName().IsEmpty() ? Path::Combine(GetTempDirectory(), "CSIndex.lst") :
                                                                                       PortableFunctions::PathReplaceFileExtension(UTF8_TODO::GetUtf8(m_pff.GetPifFileName()), FileExtensions::Listing)));
    }

    try
    {
        ToolIndexer().Run(m_pff, false);
    }

    catch( const CSProException& exception )
    {
        ErrorMessage::Display(exception);
    }

    if( use_temporary_listing_file )
        m_pff.SetListingFName(CString());
}
