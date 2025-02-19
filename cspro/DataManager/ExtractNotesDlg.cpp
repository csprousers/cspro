#include "StdAfx.h"
#include "ExtractNotesDlg.h"
#include "ExtractNotesTask.h"


BEGIN_MESSAGE_MAP(ExtractNotesDlg, ResizableDlg)
    ON_CONTROL_RANGE(BN_CLICKED, IDC_NOTES_FORMAT_CSPRO, IDC_NOTES_FORMAT_EXCEL, OnFormatChange)
    ON_COMMAND(IDC_NOTES_SELECT, OnSelectNotes)
    ON_COMMAND(IDC_DICTIONARY_SELECT, OnSelectDictionary)
    ON_MESSAGE(UWM::DataManager::UpdateDialogControls, OnUpdateDialogControls)
END_MESSAGE_MAP()


ExtractNotesDlg::ExtractNotesDlg(const CaseHoldingDoc& case_holding_doc, ExtractNotesSettings settings,
                                 std::shared_ptr<CaseProvider> case_provider, CWnd* const pParent/* = nullptr*/)
    :   ResizableDlg(IDD_EXTRACT_NOTES, pParent),
        m_caseHoldingDoc(case_holding_doc),
        m_dataDirectory(m_caseHoldingDoc.GetDataDirectory()),
        m_settings(std::move(settings)),
        m_caseProvider(std::move(case_provider)),
        m_outputTypeRadioEnumHelper({ ExtractNotesSettings::OutputType::CSPro,
                                      ExtractNotesSettings::OutputType::CSV,
                                      ExtractNotesSettings::OutputType::Excel }),
        m_notesConnectionString(m_settings.notes_connection_string),
        m_notesDictionaryFilePath(m_settings.notes_dictionary_file_path)
{
    ASSERT(m_caseProvider != nullptr);

    SerializeDialogSize("ExtractNotesDlg");

    // when not specified, provide default values for the notes data and dictionary file path
    if( !m_notesConnectionString.IsDefined() )
    {
        const ConnectionString& data_connection_string = m_caseHoldingDoc.GetConnectionString();

        if( data_connection_string.HasFilePath() )
        {
            std::string notes_filename = Path::GetFilenameWithoutExtension(data_connection_string.GetFilePath()) + " (Notes)";
            PortableFunctions::MakePathAppendFileExtension(notes_filename, PortableFunctions::PathGetFileExtension(data_connection_string.GetFilePath()));

            m_notesConnectionString = ConnectionString(PortableFunctions::PathReplaceFilename(data_connection_string.GetFilePath(), notes_filename));
        }

        m_notesDictionaryFilePath = GetSuggestedNotesDictionaryFilePath();
    }
}


void ExtractNotesDlg::DoDataExchange(CDataExchange* const pDX)
{
    __super::DoDataExchange(pDX);

    DDX_Radio(pDX, IDC_NOTES_FORMAT_CSPRO, m_outputTypeRadioEnumHelper, m_settings.output_type);
    DDX_Text(pDX, IDC_NOTES, m_notesConnectionString);
    DDX_Text(pDX, IDC_DICTIONARY_FILE_PATH, m_notesDictionaryFilePath, true);
}


void ExtractNotesDlg::UpdateSettingsFromData()
{
    UpdateData(TRUE);

    m_settings.notes_connection_string = m_notesConnectionString;
    m_settings.notes_connection_string.AdjustRelativePath(m_dataDirectory);

    m_settings.notes_dictionary_file_path = MakeFullPath(m_dataDirectory, m_notesDictionaryFilePath);
}


BOOL ExtractNotesDlg::OnInitDialog()
{
    __super::OnInitDialog();

    try
    {
        const size_t number_cases = m_caseProvider->GetNumberCases();
        WindowsUtf8::SetText(this, IDC_HEADING, FormatText("Specify how to save the notes from %d case%s:",
                                                           static_cast<int>(number_cases), PluralizeWord(number_cases)));
    }
    catch(...) { ASSERT(false); }

    PostMessage(UWM::DataManager::UpdateDialogControls, FALSE);

    return TRUE;
}


LRESULT ExtractNotesDlg::OnUpdateDialogControls(const WPARAM wParam, LPARAM /*lParam*/)
{
    const int dictionary_show_command = ( m_settings.output_type == ExtractNotesSettings::OutputType::CSPro ) ? SW_SHOW : SW_HIDE;

    for( const int id : { IDC_DICTIONARY_TEXT, IDC_DICTIONARY_FILE_PATH, IDC_DICTIONARY_SELECT } )
        GetDlgItem(id)->ShowWindow(dictionary_show_command);

    // potentially suggest a dictionary file path
    if( wParam == TRUE &&
        m_settings.output_type == ExtractNotesSettings::OutputType::CSPro &&
        m_notesDictionaryFilePath.empty() )
    {
        m_notesDictionaryFilePath = GetSuggestedNotesDictionaryFilePath();
        UpdateData(FALSE);
    }

    return 1;
}


void ExtractNotesDlg::OnOK()
{
    UpdateSettingsFromData();

    try
    {
        if( !m_settings.notes_connection_string.IsDefined() )
            throw CSProException("Specify the file where notes should be saved.");

        if( m_settings.output_type == ExtractNotesSettings::OutputType::CSPro &&
            SO::IsWhitespace(m_settings.notes_dictionary_file_path) &&
            !DictionarySource::HasEmbeddedDictionaryWhenCreated(m_settings.notes_connection_string) )
        {
            throw CSProException("Saving data to type '%s' requires that you specify a dictionary to describe the data format.",
                                 ToString(m_settings.notes_connection_string.GetType()));
        }

        if( m_settings.output_type != ExtractNotesSettings::OutputType::CSPro )
            m_settings.notes_dictionary_file_path.clear();

        __super::OnOK();
    }

    catch( const CSProException& exception )
    {
        ErrorMessage::Display(exception);
    }
}


void ExtractNotesDlg::OnFormatChange(const UINT nID)
{
    UpdateData(TRUE);

    // if a filename is already given, modify the extension to match the change
    if( m_notesConnectionString.HasFilePath() )
    {
        const char* const extension = ( nID == IDC_NOTES_FORMAT_CSPRO )  ? FileExtensions::Data::CSProDB :
                                      ( nID == IDC_NOTES_FORMAT_CSV )    ? FileExtensions::CSV :
                                    /*( nID == IDC_NOTES_FORMAT_EXCEL ) */ FileExtensions::Excel;

        m_notesConnectionString = ConnectionString(PortableFunctions::PathReplaceFileExtension(m_notesConnectionString.GetFilePath(), extension));
        UpdateData(FALSE);
    }

    PostMessage(UWM::DataManager::UpdateDialogControls, TRUE);
}


void ExtractNotesDlg::OnSelectNotes()
{
    UpdateSettingsFromData();

    DataFileDlg data_file_dlg(DataFileDlg::Type::CreateNew, false, m_settings.notes_connection_string);
    data_file_dlg.SetTitle(FormatText("Save '%s' Notes As", m_caseHoldingDoc.GetDictionary().GetName().c_str()));

    if( data_file_dlg.DoModal() != IDOK )
        return;

    m_notesConnectionString = data_file_dlg.GetConnectionString();

    bool update_dialog_controls = false;

    if( m_settings.output_type == ExtractNotesSettings::OutputType::CSPro )
    {
        // suggest a dictionary file path if one hasn't been set
        if( m_notesDictionaryFilePath.empty() )
            update_dialog_controls = true;
    }

    // if the output type is CSV/Excel but the extension doesn't match what is expected, modify the output type
    else if( m_notesConnectionString.HasFilePath() )
    {
        const std::string extension = PortableFunctions::PathGetFileExtension(m_notesConnectionString.GetFilePath());
        std::optional<ExtractNotesSettings::OutputType> output_type;

        if( SO::EqualsNoCase(extension, FileExtensions::CSV) )
        {
            if( m_settings.output_type != ExtractNotesSettings::OutputType::CSV )
                output_type = ExtractNotesSettings::OutputType::CSV;
        }

        else if( SO::EqualsNoCase(extension, FileExtensions::Excel) )
        {
            if( m_settings.output_type != ExtractNotesSettings::OutputType::Excel )
                output_type = ExtractNotesSettings::OutputType::Excel;
        }

        else
        {
            output_type = ExtractNotesSettings::OutputType::CSPro;
        }

        if( output_type.has_value() )
        {
            m_settings.output_type = *output_type;
            update_dialog_controls = true;
        }
    }

    UpdateData(FALSE);

    if( update_dialog_controls )
        PostMessage(UWM::DataManager::UpdateDialogControls, TRUE);
}


std::string ExtractNotesDlg::GetSuggestedNotesDictionaryFilePath() const
{
    const CDataDict& dictionary = m_caseHoldingDoc.GetDictionary();
    std::string dictionary_filename = Path::GetFilenameWithoutExtension(dictionary.GetFilePath());

    if( dictionary_filename.empty() )
        dictionary_filename = m_caseHoldingDoc.GetDictionary().GetName();

    return PortableFunctions::CreateFilePath(m_dataDirectory,
                                             dictionary_filename + " (Notes)",
                                             FileExtensions::Dictionary);
}


void ExtractNotesDlg::OnSelectDictionary()
{
    UpdateSettingsFromData();

    std::string suggested_file_path = m_settings.notes_dictionary_file_path;

    if( suggested_file_path.empty() )
        suggested_file_path = GetSuggestedNotesDictionaryFilePath();

    SaveFileDlg save_file_dlg(0, FileExtensions::Dictionary, suggested_file_path, FileFilters::Dictionary, this);

    save_file_dlg.SetTitle(FormatText("Save Dictionary '%s%s' As", ExtractNotesTask::NamePrefix,
                                                                   m_caseHoldingDoc.GetDictionary().GetName().c_str()));

    if( save_file_dlg.DoModal() != IDOK )
        return;

    m_notesDictionaryFilePath = save_file_dlg.GetFilePath();

    UpdateData(FALSE);
}
