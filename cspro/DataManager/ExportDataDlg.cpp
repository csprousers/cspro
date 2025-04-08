#include "StdAfx.h"
#include "ExportDataDlg.h"
#include <zExportO/CSProExportWriter.h>
#include <zExportO/ExportDefinitions.h>


BEGIN_MESSAGE_MAP(ExportDataDlg, ResizableDlg)
    ON_CONTROL_RANGE(BN_CLICKED, IDC_EXPORT_CSV, IDC_EXPORT_STATA, OnFormatChange)
    ON_EN_CHANGE(IDC_EXPORT_BASE_FILE_PATH, OnChangeBaseFilePath)
    ON_COMMAND(IDC_EXPORT_BASE_FILE_PATH_SELECT, OnSelectBaseFilePath)
    ON_BN_CLICKED(IDC_EXPORT_ONE_FILE_PER_RECORD, OnOneFilePerRecordChange)
    ON_MESSAGE(UWM::DataManager::UpdateDialogControls, OnUpdateDialogControls)
END_MESSAGE_MAP()


ExportDataDlg::ExportDataDlg(const CaseHoldingDoc& case_holding_doc, ExportDataSettings settings,
                             std::shared_ptr<CaseProvider> case_provider, CWnd* const pParent/* = nullptr*/)
    :   ResizableDlg(IDD_EXPORT_DATA, pParent),
        m_caseHoldingDoc(case_holding_doc),
        m_settings(std::move(settings)),
        m_caseProvider(std::move(case_provider)),
        m_baseFilePathWnd(nullptr),
        m_outputsListBox(nullptr),
        m_okWnd(nullptr)
{
    ASSERT(m_caseProvider != nullptr);

    SerializeDialogSize("ExportDataDlg");

    // when not specified, suggest a base file path using...
    if( m_settings.base_file_path.empty() )
    {
        const ConnectionString& connection_string = m_caseHoldingDoc.GetConnectionString();

        m_settings.base_file_path = m_caseHoldingDoc.GetDataDirectory();

        // ...the case key (for exports while viewing a single case)
        const Case* const single_operation_case = m_caseProvider->GetCaseIfSingleCaseOperation();

        if( single_operation_case != nullptr )
        {
            Path::MakeCombine(m_settings.base_file_path, Path::CreateValidFilename(single_operation_case->GetSingleLineKey()));
        }

        // ...the extension-less filename
        else if( connection_string.HasFilePath() )
        {
            Path::MakeCombine(m_settings.base_file_path, Path::GetFilenameWithoutExtension(connection_string.GetFilePath()));
        }

        // ...or the dictionary name
        else
        {
            Path::MakeCombine(m_settings.base_file_path, m_caseHoldingDoc.GetDictionary().GetName());
        }
    }

    // populate the record names
    const CDataDict& dictionary = m_caseHoldingDoc.GetDictionary();

    for( const DictLevel& dict_level : dictionary.GetLevels() )
    {
        for( int r = 0; r < dict_level.GetNumRecords(); ++r )
            m_recordNames.emplace_back(dict_level.GetRecord(r)->GetName());

        if( !CSProExportWriter::MultipleLevelExportSupported )
            break;
    }
}


template<typename OT, typename IT>
OT ExportDataDlg::ConvertResourceIdDataRepositoryType(const IT input)
{
    constexpr std::tuple<unsigned int, DataRepositoryType> Mapping[]
    {
        { IDC_EXPORT_CSV,   DataRepositoryType::CommaDelimited },
        { IDC_EXPORT_EXCEL, DataRepositoryType::Excel },
        { IDC_EXPORT_JSON,  DataRepositoryType::Json },
        { IDC_EXPORT_R,     DataRepositoryType::R },
        { IDC_EXPORT_SAS,   DataRepositoryType::SAS },
        { IDC_EXPORT_SPSS,  DataRepositoryType::SPSS },
        { IDC_EXPORT_STATA, DataRepositoryType::Stata }
    };

    const std::tuple<unsigned int, DataRepositoryType>* mapping_itr = Mapping;
    const std::tuple<unsigned int, DataRepositoryType>* const mapping_end = Mapping + _countof(Mapping);

    for( ; mapping_itr != mapping_end; ++mapping_itr )
    {
        if( input == std::get<IT>(*mapping_itr) )
            return std::get<OT>(*mapping_itr);
    }

    throw ProgrammingErrorException();
}


BOOL ExportDataDlg::OnInitDialog()
{
    __super::OnInitDialog();

    m_baseFilePathWnd = GetDlgItem(IDC_EXPORT_BASE_FILE_PATH);
    m_outputsListBox = static_cast<CListBox*>(GetDlgItem(IDC_EXPORT_FILE_PATHS));
    m_okWnd = GetDlgItem(IDOK);

    try
    {
        const size_t number_cases = m_caseProvider->GetNumberCases();
        WindowsUtf8::SetText(this, IDC_HEADING, FormatText("Specify how to export %d case%s. "
                                                           "For more options, use the Export Data tool, available from the Tools menu.",
                                                            static_cast<int>(number_cases), PluralizeWord(number_cases)));
    }
    catch(...) { ASSERT(false); }

    // set the initial options based on the settings
    auto check = [&](const int resource_id)
    {
        static_cast<CButton*>(GetDlgItem(resource_id))->SetCheck(BST_CHECKED);
    };

    for( const DataRepositoryType data_repository_type : m_settings.export_formats )
        check(ConvertResourceIdDataRepositoryType<unsigned int>(data_repository_type));

    WindowsUtf8::SetText(m_baseFilePathWnd, m_settings.base_file_path);

    if( m_settings.one_file_per_record )
        check(IDC_EXPORT_ONE_FILE_PER_RECORD);

    PostMessage(UWM::DataManager::UpdateDialogControls);

    return TRUE;
}


void ExportDataDlg::OnOK()
{
    ASSERT(!m_exportConnectionStrings.empty());

    std::string existent_files;

    for( const ConnectionString& connection_string : m_exportConnectionStrings )
    {
        if( PortableFunctions::FileIsRegular(connection_string.GetFilePath()) )
            SO::AppendWithSeparator(existent_files, connection_string.ToDisplayString(), SO::Newline_lf_sv);
    }

    if( !existent_files.empty() )
    {
        const std::string message = FormatText("Proceeding will overwrite these files:\n\n%s\n\nDo you want to continue?",
                                               existent_files.c_str());

        if( AfxMessageBox(message, MB_YESNOCANCEL) != IDYES )
            return;
    }

    __super::OnOK();
}


void ExportDataDlg::OnFormatChange(const UINT nID)
{
    const DataRepositoryType data_repository_type = ConvertResourceIdDataRepositoryType<DataRepositoryType>(nID);
    const auto& lookup = m_settings.export_formats.find(data_repository_type);

    // toggle the export format
    if( lookup == m_settings.export_formats.cend() )
    {
        m_settings.export_formats.insert(data_repository_type);
    }

    else
    {
        m_settings.export_formats.erase(lookup);
    }

    PostMessage(UWM::DataManager::UpdateDialogControls);
}


void ExportDataDlg::OnChangeBaseFilePath()
{
    ASSERT(m_baseFilePathWnd != nullptr);

    std::string new_base_file_path = WindowsUtf8::GetText(m_baseFilePathWnd);
    SO::MakeTrim(new_base_file_path);

    if( new_base_file_path != m_settings.base_file_path )
    {
        m_settings.base_file_path = std::move(new_base_file_path);
        PostMessage(UWM::DataManager::UpdateDialogControls);
    }
}


void ExportDataDlg::OnSelectBaseFilePath()
{
    ASSERT(m_baseFilePathWnd != nullptr);

    SaveFileDlg save_file_dlg(OFN_HIDEREADONLY, std::monostate(), m_settings.base_file_path, std::monostate(), this);
    save_file_dlg.SetTitle(L"Select Base Filename for Export");

    if( save_file_dlg.DoModal() != IDOK )
        return;

    m_settings.base_file_path = save_file_dlg.GetFilePath();
    WindowsUtf8::SetText(m_baseFilePathWnd, m_settings.base_file_path);

    PostMessage(UWM::DataManager::UpdateDialogControls);
}


void ExportDataDlg::OnOneFilePerRecordChange()
{
    m_settings.one_file_per_record = !m_settings.one_file_per_record;
    PostMessage(UWM::DataManager::UpdateDialogControls);
}


LRESULT ExportDataDlg::OnUpdateDialogControls(WPARAM /*wParam*/, LPARAM /*lParam*/)
{
    ASSERT(m_outputsListBox != nullptr);

    m_exportConnectionStrings.clear();
    m_outputsListBox->ResetContent();

    try
    {
        if( m_settings.export_formats.empty() )
            throw CSProException("Specify at least one export format.");

        if( m_settings.base_file_path != m_lastValidCheckedBaseFilePath )
        {
            if( m_settings.base_file_path.empty() )
                throw CSProException("Specify a base filename.");

            if( PortableFunctions::FileIsDirectory(m_settings.base_file_path) )
                throw CSProException("Specify a base filename within the directory: " + m_settings.base_file_path);

            if( !PortableFunctions::FileIsDirectory(PortableFunctions::PathGetDirectory(m_settings.base_file_path)) )
                throw CSProException("Specify a valid directory.");

            m_lastValidCheckedBaseFilePath = m_settings.base_file_path;
        }

        ConstructExportConnectionStrings();

        if( m_exportConnectionStrings.empty() )
            throw CSProException("No files can be created because they would overwrite the input data source.");
    }

    catch( const CSProException& exception )
    {
        m_outputsListBox->AddString(TC::ToWide(FormatText("<< %s >> ", exception.what())).c_str());
    }

    m_okWnd->EnableWindow(!m_exportConnectionStrings.empty());

    return 1;
}


void ExportDataDlg::ConstructExportConnectionStrings()
{
    // add the connection strings in the order of the checkboxes
    std::vector<DataRepositoryType> sorted_export_formats(m_settings.export_formats.cbegin(), m_settings.export_formats.cend());
    std::sort(sorted_export_formats.begin(), sorted_export_formats.end(),
        [&](const DataRepositoryType t1, const DataRepositoryType t2)
        {
            return ( ConvertResourceIdDataRepositoryType<unsigned int>(t1) < ConvertResourceIdDataRepositoryType<unsigned int>(t2) );
        });

    size_t added_connection_strings = m_exportConnectionStrings.size();
    ASSERT(added_connection_strings == 0);

    for( const DataRepositoryType data_repository_type : sorted_export_formats )
    {
        bool one_file_per_record = m_settings.one_file_per_record;
        const char* export_extension;
        std::string data_repository_type_override;

        if( DataRepositoryHelpers::IsTypeExportWriter(data_repository_type) )
        {
            export_extension = ExportTypeDefaultExtension(data_repository_type);

            // if the export type doesn't support multiple records, then we must use one file per record
            if( !one_file_per_record && !ExportTypeSupportsMultipleRecords(data_repository_type) )
                one_file_per_record = true;
        }

        // if writing to a non-export writer, make sure that we write it as using CSProExportWriter
        else
        {
            data_repository_type_override = FormatText("%s=%s", ConnectionStringDataRepositoryPropertyType,
                                                                DataRepositoryTypeNames[static_cast<size_t>(DataRepositoryType::CSProExport)]);

            export_extension = DataRepositoryTypeDefaultExtensions[static_cast<size_t>(data_repository_type)];
        }

        // when outputting multiple records, the record name is added to the filename
        if( one_file_per_record && m_recordNames.size() > 1 )
        {
            if( !data_repository_type_override.empty() )
                data_repository_type_override.insert(0, 1, PropertyString::PropertySeparatorAdditional);

            for( const std::string& record_name : m_recordNames )
            {
                AddExportConnectionString(FormatText("%s-%s.%s%c%s=%s%s",
                                                     m_settings.base_file_path.c_str(), record_name.c_str(), export_extension,
                                                     PropertyString::PropertySeparatorInitial,
                                                     CSProperty::record, record_name.c_str(),
                                                     data_repository_type_override.c_str()));
            }
        }

        else
        {
            if( !data_repository_type_override.empty() )
                data_repository_type_override.insert(0, 1, PropertyString::PropertySeparatorInitial);

            AddExportConnectionString(FormatText("%s.%s%s",
                                                 m_settings.base_file_path.c_str(), export_extension,
                                                 data_repository_type_override.c_str()));
        }

        // add a space between each format's connection strings
        if( added_connection_strings != 0 )
            m_outputsListBox->AddString(L"");

        for( ; added_connection_strings < m_exportConnectionStrings.size(); ++added_connection_strings )
            m_outputsListBox->AddString(TC::ToWide(m_exportConnectionStrings[added_connection_strings].ToDisplayString()).c_str());
    }
}


void ExportDataDlg::AddExportConnectionString(const std::string_view connection_string_text_sv)
{
    const ConnectionString& connection_string = m_exportConnectionStrings.emplace_back(connection_string_text_sv);

    // don't export on top of the existing data source (which could occur with JSON data sources)
    if( m_caseHoldingDoc.GetConnectionString().SharesResource(connection_string) )
        m_exportConnectionStrings.pop_back();
}
