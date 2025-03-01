#include "StdAfx.h"
#include "ExtractBinaryDataDlg.h"
#include "ExtractBinaryDataTask.h"
#include <zUtilO/FileUtil.h>


BEGIN_MESSAGE_MAP(ExtractBinaryDataDlg, ResizableDlg)
    ON_CONTROL_RANGE(BN_CLICKED, IDC_BINARY_DATA_FORMAT_FILENAME, IDC_BINARY_DATA_FORMAT_KEY_SIGNATURE, OnSettingsChange)
    ON_BN_CLICKED(IDC_RIGHT_TRIM_CASE_KEYS, OnSettingsChange)
    ON_EN_CHANGE(IDC_INVALID_CHAR_REPLACEMENT, OnSettingsChange)
    ON_COMMAND(IDC_BINARY_DATA_DIRECTORY_SELECT, OnSelectOutputDirectory)
    ON_MESSAGE(UWM::DataManager::UpdateDialogControls, OnUpdateDialogControls)
END_MESSAGE_MAP()


ExtractBinaryDataDlg::ExtractBinaryDataDlg(const CaseHoldingDoc& case_holding_doc, ExtractBinaryDataSettings settings,
                                          std::shared_ptr<CaseProvider> case_provider, CWnd* const pParent/* = nullptr*/)
    :   ResizableDlg(IDD_EXTRACT_BINARY_DATA, pParent),
        m_caseHoldingDoc(case_holding_doc),
        m_settings(std::move(settings)),
        m_caseProvider(std::move(case_provider)),
        m_filenameFormatRadioEnumHelper({ ExtractBinaryDataSettings::FilenameFormat::Filename,
                                          ExtractBinaryDataSettings::FilenameFormat::KeyFilename,
                                          ExtractBinaryDataSettings::FilenameFormat::Signature,
                                          ExtractBinaryDataSettings::FilenameFormat::KeySignature })
{
    ASSERT(m_caseProvider != nullptr);

    SerializeDialogSize("ExtractBinaryDataDlg");

    SetUpFakeCaseKeysForSampleFilenames();

    // when not specified, provide a suggested directory for the binary data
    if( m_settings.output_directory.empty() )
    {
        const ConnectionString& data_connection_string = m_caseHoldingDoc.GetConnectionString();

        if( data_connection_string.HasFilePath() )
            m_settings.output_directory = PortableFunctions::PathRemoveFileExtension(data_connection_string.GetFilePath()) + " (binary data)";
    }
}


void ExtractBinaryDataDlg::DoDataExchange(CDataExchange* const pDX)
{
    __super::DoDataExchange(pDX);

    DDX_Radio(pDX, IDC_BINARY_DATA_FORMAT_FILENAME, m_filenameFormatRadioEnumHelper, m_settings.filename_format);
    DDX_Check(pDX, IDC_RIGHT_TRIM_CASE_KEYS, m_settings.right_trim_keys);
    DDX_Text(pDX, IDC_INVALID_CHAR_REPLACEMENT, m_settings.invalid_character_replacement, true);
    DDX_Text(pDX, IDC_BINARY_DATA_DIRECTORY, m_settings.output_directory, true);
}


BOOL ExtractBinaryDataDlg::OnInitDialog()
{
    __super::OnInitDialog();

    try
    {
        const size_t number_cases = m_caseProvider->GetNumberCases();
        WindowsUtf8::SetText(this, IDC_HEADING, FormatText("Specify how to extract the binary data contained in %d case%s:",
                                                           static_cast<int>(number_cases), PluralizeWord(number_cases)));
    }
    catch(...) { ASSERT(false); }

    PostMessage(UWM::DataManager::UpdateDialogControls);

    return TRUE;
}


LRESULT ExtractBinaryDataDlg::OnUpdateDialogControls(WPARAM /*wParam*/, LPARAM /*lParam*/)
{
    const bool filename_uses_key = ( m_settings.filename_format == ExtractBinaryDataSettings::FilenameFormat::KeyFilename ||
                                     m_settings.filename_format == ExtractBinaryDataSettings::FilenameFormat::KeySignature );

    const bool filename_uses_signature = ( m_settings.filename_format == ExtractBinaryDataSettings::FilenameFormat::Signature ||
                                           m_settings.filename_format == ExtractBinaryDataSettings::FilenameFormat::KeySignature );

    // right-trim case keys is only valid if the key is part of the filename
    GetDlgItem(IDC_RIGHT_TRIM_CASE_KEYS)->EnableWindow(filename_uses_key);

    // show sample filenames based on the current selections
    const std::string filename = filename_uses_signature ? "5f5f0f6990e35b87d3d95e028026e598.jpg" :
                                                           "roof.jpg";

    auto create_valid_filename = [&](const int nIDDlgItem, const size_t index)
    {
        std::string dlg_filename = ExtractBinaryDataTask::CreateValidFilename(m_settings, m_fakeCaseKeysForSampleFilenames[index], filename);
        SO::Replace(dlg_filename, "&", "&&");
        WindowsUtf8::SetText(this, nIDDlgItem, dlg_filename);
    };

    create_valid_filename(IDC_FILENAME_KEY_VALID, 0);
    create_valid_filename(IDC_FILENAME_KEY_INVALID, 1);
    create_valid_filename(IDC_FILENAME_KEY_RIGHT_SPACES, 2);

    return 1;
}


void ExtractBinaryDataDlg::OnOK()
{
    UpdateData(TRUE);

    try
    {
        ExtractBinaryDataTask::ValidateSettings(m_settings);

        __super::OnOK();
    }

    catch( const CSProException& exception )
    {
        ErrorMessage::Display(exception);
    }
}


void ExtractBinaryDataDlg::OnSettingsChange()
{
    UpdateData(TRUE);

    PostMessage(UWM::DataManager::UpdateDialogControls);
}


void ExtractBinaryDataDlg::OnSelectOutputDirectory()
{
    UpdateData(TRUE);

    std::string suggested_directory = m_settings.output_directory;

    if( suggested_directory.empty() )
        suggested_directory = m_caseHoldingDoc.GetDataDirectory();

    std::optional<std::string> directory = SelectFolderDialog(m_hWnd,
                                                              FormatText("Save '%s' Binary Data in Directory", m_caseHoldingDoc.GetDictionary().GetName().c_str()),
                                                              suggested_directory);

    if( !directory.has_value() )
        return;

    m_settings.output_directory = std::move(*directory);

    UpdateData(FALSE);
}


void ExtractBinaryDataDlg::SetUpFakeCaseKeysForSampleFilenames()
{
    constexpr std::string_view NumericValues_sv = "123";
    constexpr std::string_view AlphaValues_sv = "abc";

    const std::vector<const CDictItem*> id_items = m_caseHoldingDoc.GetDictionary().GetIdItems();

    // first set up the base (valid) key
    for( const CDictItem* const id_item : id_items )
    {
        const std::string_view& values = ( id_item->GetDataType() == DataType::Numeric ) ? NumericValues_sv :
                                         ( id_item->GetDataType() == DataType::String )  ? AlphaValues_sv :
                                                                                           ReturnProgrammingError(AlphaValues_sv);

        for( unsigned i = 0; i < id_item->GetLen(); ++i )
            m_fakeCaseKeysForSampleFilenames[0].push_back(values[i % values.length()]);
    }

    ASSERT(m_fakeCaseKeysForSampleFilenames[0].length() == SO::WideLength(m_fakeCaseKeysForSampleFilenames[0]));

    // create a key with invalid alpha characters
    m_fakeCaseKeysForSampleFilenames[1] = m_fakeCaseKeysForSampleFilenames[0];
    unsigned key_offset = 0;

    for( const CDictItem* const id_item : id_items )
    {
        if( id_item->GetDataType() == DataType::String )
            m_fakeCaseKeysForSampleFilenames[1].replace(key_offset + ( id_item->GetLen() / 2 ), 1, 1, '*');

        key_offset += id_item->GetLen();
    }

    // create a key with right spaces
    m_fakeCaseKeysForSampleFilenames[2] = m_fakeCaseKeysForSampleFilenames[0];
    unsigned potential_right_spaces = 0;

    for( auto id_items_itr = id_items.crbegin();
         id_items_itr != id_items.crend() && (*id_items_itr)->GetDataType() == DataType::String;
         ++id_items_itr )
    {
        potential_right_spaces += (*id_items_itr)->GetLen();
    }

    if( potential_right_spaces != 0 )
    {
        m_fakeCaseKeysForSampleFilenames[2].replace(m_fakeCaseKeysForSampleFilenames[2].length() - potential_right_spaces, potential_right_spaces,
                                                    potential_right_spaces, ' ' );
    }
}
