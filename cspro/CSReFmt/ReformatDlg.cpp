#include "StdAfx.h"
#include "ReformatDlg.h"
#include <zUtilO/imsaDlg.H>
#include <zUtilO/Filedlg.h>
#include <ZBRIDGEO/DataFileDlg.h>
#include <zDataO/DataRepositoryHelpers.h>
#include <zReformatO/ToolReformatter.h>


BEGIN_MESSAGE_MAP(ReformatDlg, CDialog)
    ON_COMMAND(ID_HELP, OnHelp)
    ON_COMMAND(ID_APP_ABOUT, OnAppAbout)
    ON_COMMAND(ID_FILE_OPEN, OnFileOpen)
    ON_COMMAND(ID_FILE_SAVE_AS, OnFileSaveAs)
    ON_COMMAND(ID_FILE_RUN, OnReformatData)
    ON_COMMAND(ID_VIEW_NAMES, OnToggleNames)
    ON_COMMAND(ID_VIEW_SHOW_ONLY_DESTRUCTIVE_CHANGES, OnToggleShowOnlyDestructiveChanges)
    ON_EN_CHANGE(IDC_INPUT_DICT, OnTextChange)
    ON_BN_CLICKED(IDC_INPUT_DICT_BROWSE, OnInputDictionaryBrowse)
    ON_EN_CHANGE(IDC_INPUT_DATA, OnTextChange)
    ON_BN_CLICKED(IDC_INPUT_DATA_BROWSE, OnInputDataBrowse)
    ON_EN_CHANGE(IDC_OUTPUT_DICT, OnTextChange)
    ON_BN_CLICKED(IDC_OUTPUT_DICT_BROWSE, OnOutputDictionaryBrowse)
    ON_EN_CHANGE(IDC_OUTPUT_DATA, OnTextChange)
    ON_BN_CLICKED(IDC_OUTPUT_DATA_BROWSE, OnOutputDataBrowse)    
    ON_BN_CLICKED(IDC_REFORMAT_DATA, OnReformatData)
    ON_MESSAGE(UWM::CSReFmt::UpdateDialogUI, OnUpdateDialogUI)
END_MESSAGE_MAP()


ReformatDlg::ReformatDlg(CWnd* pParent/* = nullptr*/)
    :   CDialog(ReformatDlg::IDD, pParent),
        m_hIcon(AfxGetApp()->LoadIcon(IDR_MAINFRAME)),
        m_showOnlyDestructiveChanges(false)
{
    SetDefaultPffSettings();
}


ReformatDlg::~ReformatDlg()
{
}


void ReformatDlg::DoDataExchange(CDataExchange* pDX)
{
    CDialog::DoDataExchange(pDX);

    DDX_Text(pDX, IDC_INPUT_DICT, m_inputDictionaryFilename);
    DDX_Text(pDX, IDC_INPUT_DATA, m_inputConnectionString);
    DDX_Text(pDX, IDC_OUTPUT_DICT, m_outputDictionaryFilename);
    DDX_Text(pDX, IDC_OUTPUT_DATA, m_outputConnectionString);
    DDX_Control(pDX, IDC_DICT_DIFFERENCES, m_dictionaryChangesHtml);
}


BOOL ReformatDlg::OnInitDialog()
{
    CDialog::OnInitDialog();

    // add the menu
    m_menu.LoadMenu(IDR_REFORMAT_MENU);
    SetMenu(&m_menu);

    // set the icons
    SetIcon(m_hIcon, TRUE);
    SetIcon(m_hIcon, FALSE);

    PostMessage(UWM::CSReFmt::UpdateDialogUI);

    m_dictionaryChangesHtml.SetContextMenuEnabled(false);

    return TRUE;
}


LRESULT ReformatDlg::OnUpdateDialogUI(WPARAM wParam, LPARAM /*lParam*/)
{
    BasicLogger message;
    bool recalculate_differences = ( wParam == 1 );

    // load the input dictionary
    const bool input_dictionary_changed = ( m_inputDictionaryFilename != m_lastLoadedInputDictionaryFilename );

    if( input_dictionary_changed )
    {
        m_reformatter.reset();
        m_lastLoadedInputDictionaryFilename.clear();

        if( PortableFunctions::FileIsRegular(m_inputDictionaryFilename) )
        {
            try
            {
                m_inputDictionary = CDataDict::InstantiateAndOpen(m_inputDictionaryFilename, true);
                m_lastLoadedInputDictionaryFilename = m_inputDictionaryFilename;
            }

            catch( const CSProException& )
            {
                message.AppendFormatLine(BasicLogger::Color::DarkBlue, _T("There was an error opening the input dictionary: %s"),
                                                                       PortableFunctions::PathGetFilename(m_inputDictionaryFilename));
            }
        }

        if( m_lastLoadedInputDictionaryFilename.empty() )
            m_inputDictionary.reset();
    }

    // load the input data in case it has an embedded dictionary and no input dictionary is specified
    if( input_dictionary_changed || !m_lastLoadedInputConnectionString.Equals(m_inputConnectionString) )
    {
        m_reformatter.reset();

        if( m_inputDictionaryFilename.empty() )
        {
            m_lastLoadedInputConnectionString = m_inputConnectionString;
            m_embeddedDictionaryFromInputRepository = DataRepositoryHelpers::GetEmbeddedDictionary(m_inputConnectionString);
        }

        else
        {
            m_lastLoadedInputConnectionString.Clear();
            m_embeddedDictionaryFromInputRepository.reset();
        }
    }


    // load the output dictionary
    if( m_outputDictionaryFilename != m_lastLoadedOutputDictionaryFilename )
    {
        m_reformatter.reset();
        m_lastLoadedOutputDictionaryFilename.clear();

        if( PortableFunctions::FileIsRegular(m_outputDictionaryFilename) )
        {
            try
            {
                m_outputDictionary = CDataDict::InstantiateAndOpen(m_outputDictionaryFilename, true);
                m_lastLoadedOutputDictionaryFilename = m_outputDictionaryFilename;
            }

            catch( const CSProException& )
            {
                message.AppendFormatLine(BasicLogger::Color::DarkBlue, _T("There was an error opening the output dictionary: %s"),
                                                                       PortableFunctions::PathGetFilename(m_outputDictionaryFilename));
            }
        }

        if( m_lastLoadedOutputDictionaryFilename.empty() )
            m_outputDictionary.reset();
    }

    // do the dictionary comparison (or warn about what files need to be supplied)
    bool enable_reformat = false;
    const bool input_dictionary_defined = ( GetUsableInputDictionary() != nullptr );

    if( !input_dictionary_defined )
        message.AppendLine(BasicLogger::Color::DarkBlue, _T("Select an input dictionary or data file."));

    if( m_outputDictionary == nullptr )
    {
        message.AppendLine(BasicLogger::Color::DarkBlue, _T("Select an output dictionary."));
    }

    else if( input_dictionary_defined )
    {
        if( m_reformatter == nullptr )
        {
            m_reformatter = std::make_unique<Reformatter>(GetUsableInputDictionary(), m_outputDictionary);
            recalculate_differences = true;
        }

        if( recalculate_differences )
        {
            if( m_inputConnectionString.IsDefined() )
            {
                const bool requires_reformat = m_reformatter->RequiresReformat(m_inputConnectionString.GetType());
                const BasicLogger::Color color = requires_reformat ? BasicLogger::Color::Red :
                                                                     BasicLogger::Color::DarkBlue;

                message.AppendFormatLine(color, _T("The differences %srequire reformatting data stored as '%s'."),
                                                requires_reformat ? _T("") : _T("do not "),
                                                ToString(m_inputConnectionString.GetType()));
                message.AppendLine();
            }

            ToolReformatter::GetDictionaryDifferences(message, *m_reformatter, m_pff.GetDisplayNames(), m_showOnlyDestructiveChanges);
        }

        enable_reformat = ( m_inputConnectionString.IsDefined() && m_outputConnectionString.IsDefined() );
    }

    // update the UI
    m_menu.EnableMenuItem(ID_FILE_RUN, enable_reformat ? MF_ENABLED : MF_DISABLED);
    GetDlgItem(IDC_REFORMAT_DATA)->EnableWindow(enable_reformat);

    m_menu.CheckMenuItem(ID_VIEW_NAMES, m_pff.GetDisplayNames() ? MF_CHECKED : MF_UNCHECKED);
    m_menu.CheckMenuItem(ID_VIEW_SHOW_ONLY_DESTRUCTIVE_CHANGES, m_showOnlyDestructiveChanges ? MF_CHECKED : MF_UNCHECKED);

    if( !message.IsEmpty() )
        m_dictionaryChangesHtml.SetHtml(message.ToHtml());

    return 0;
}


void ReformatDlg::OnAppAbout()
{
    CIMSAAboutDlg about_dlg(WindowsWS::LoadString(AFX_IDS_APP_TITLE), m_hIcon);
    about_dlg.DoModal();
}


void ReformatDlg::SetDefaultPffSettings()
{
    m_pff.ResetContents();

    m_pff.SetAppType(APPTYPE::REFORMAT_TYPE);
    m_pff.SetDisplayNames(true);
    m_pff.SetViewListing(VIEWLISTING::ALWAYS);
    m_pff.SetViewResultsFlag(false);
}


void ReformatDlg::OnFileOpen()
{
    CIMSAFileDialog file_dlg(TRUE, FileExtensions::Pff, nullptr, OFN_HIDEREADONLY, FileFilters::Pff);
    file_dlg.m_ofn.lpstrTitle = _T("Select Input PFF");

    if( file_dlg.DoModal() != IDOK )
        return;

    m_pff.ResetContents();
    m_pff.SetPifFileName(file_dlg.GetPathName());

    if( !m_pff.LoadPifFile() || m_pff.GetAppType() != REFORMAT_TYPE )
    {
        AfxMessageBox(_T("The PFF could not be read or was not a Reformat Data PFF."));
        SetDefaultPffSettings();
    }

    m_inputDictionaryFilename = m_pff.GetInputDictFName();
    m_inputConnectionString = m_pff.GetSingleInputDataConnectionString();
    m_outputDictionaryFilename = m_pff.GetOutputDictFName();
    m_outputConnectionString = m_pff.GetSingleOutputDataConnectionString();

    UpdateData(FALSE);
    PostMessage(UWM::CSReFmt::UpdateDialogUI);
}


void ReformatDlg::UIToPff()
{
    UpdateData(TRUE);

    m_pff.SetInputDictFName(WS2CS(m_inputDictionaryFilename));
    m_pff.SetSingleInputDataConnectionString(m_inputConnectionString);
    m_pff.SetOutputDictFName(WS2CS(m_outputDictionaryFilename));
    m_pff.SetSingleOutputDataConnectionString(m_outputConnectionString);
}


void ReformatDlg::OnFileSaveAs()
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


void ReformatDlg::OnToggleNames()
{
    m_pff.SetDisplayNames(!m_pff.GetDisplayNames());
    PostMessage(UWM::CSReFmt::UpdateDialogUI, 1);
}


void ReformatDlg::OnToggleShowOnlyDestructiveChanges()
{
    m_showOnlyDestructiveChanges = !m_showOnlyDestructiveChanges;
    PostMessage(UWM::CSReFmt::UpdateDialogUI, 1);
}


void ReformatDlg::OnTextChange()
{
    UpdateData(TRUE);
    PostMessage(UWM::CSReFmt::UpdateDialogUI);
}


void ReformatDlg::OnDictionaryBrowse(std::wstring& dictionary_filename, const TCHAR* title_text)
{
    UpdateData(TRUE);

    const std::wstring dialog_title = FormatTextCS2WS(_T("Select %s Dictionary"), title_text);

    CIMSAFileDialog file_dlg(TRUE, FileExtensions::Dictionary, dictionary_filename.c_str(), 0,
                             _T("Data Dictionary Files (*.dcf)|*.dcf|All Files (*.*)|*.*||"));
    file_dlg.m_ofn.lpstrTitle = dialog_title.c_str();

    if( file_dlg.DoModal() != IDOK )
        return;

    dictionary_filename = file_dlg.GetPathName();

    UpdateData(FALSE);
    PostMessage(UWM::CSReFmt::UpdateDialogUI);
}


void ReformatDlg::OnDataBrowse(ConnectionString& connection_string, const bool open_existing,
                               const std::wstring& dictionary_filename, const ConnectionString& other_connection_string)
{
    UpdateData(TRUE);

    DataFileDlg data_file_dlg(open_existing ? DataFileDlg::Type::OpenExisting : DataFileDlg::Type::CreateNew,
                              open_existing, connection_string);
    data_file_dlg.SetDictionaryFilename(WS2CS(dictionary_filename))
                 .SuggestMatchingDataRepositoryType(other_connection_string);

    if( data_file_dlg.DoModal() != IDOK )
        return;

    connection_string = data_file_dlg.GetConnectionString();

    UpdateData(FALSE);
    PostMessage(UWM::CSReFmt::UpdateDialogUI);
}


void ReformatDlg::OnInputDictionaryBrowse()
{
    OnDictionaryBrowse(m_inputDictionaryFilename, _T("Input"));
}


void ReformatDlg::OnInputDataBrowse()
{
    OnDataBrowse(m_inputConnectionString, true, m_inputDictionaryFilename, m_outputConnectionString);
}


void ReformatDlg::OnOutputDictionaryBrowse()
{
    OnDictionaryBrowse(m_outputDictionaryFilename, _T("Output"));
}


void ReformatDlg::OnOutputDataBrowse()
{
    OnDataBrowse(m_outputConnectionString, false, m_outputDictionaryFilename, m_inputConnectionString);
}


void ReformatDlg::OnReformatData()
{
    ASSERT(m_reformatter != nullptr);

    UIToPff();

    // if the listing file hasn't been defined, put it in the same folder as the PFF, or in the temporary folder if the PFF hasn't been saved
    const bool use_temporary_listing_filename = m_pff.GetListingFName().IsEmpty();

    if( use_temporary_listing_filename )
    {
        m_pff.SetListingFName(WS2CS(m_pff.GetPifFileName().IsEmpty() ? PortableFunctions::PathAppendToPath(GetTempDirectory(), _T("CSRefmt.lst")) :
                                                                       PortableFunctions::PathReplaceFileExtension(m_pff.GetPifFileName(), FileExtensions::Listing)));
    }

    try
    {
        ToolReformatter().Run(m_pff, false, *m_reformatter);
    }

    catch( const CSProException& exception )
    {
        ErrorMessage::Display(exception);
    }

    if( use_temporary_listing_filename )
        m_pff.SetListingFName(CString());
}
