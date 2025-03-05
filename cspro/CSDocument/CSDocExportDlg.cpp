#include "StdAfx.h"
#include "CSDocExportDlg.h"
#include "CSDocCompiler.h"
#include <zUtilO/DynamicLayoutControlResizer.h>


namespace
{
    // export settings, associated with documents, will be persisted for eight weeks
    constexpr const char* ExportSettingsTableName     = "export_settings";
    constexpr int64_t ExportSettingsExpirationSeconds = DateHelper::SecondsInWeek(8);

    constexpr int SettingsHtml       = 0;
    constexpr int SettingsPdf        = 1;
    constexpr int SettingsFromBuilds = 2;
    constexpr int SettingsCustom     = 3;
}


BEGIN_MESSAGE_MAP(CSDocExportDlg, CDialog)
    ON_WM_SIZE()
    ON_COMMAND(IDC_OUTPUT_FILENAME_BROWSE, OnOutputFilePathBrowse)
    ON_BN_CLICKED(IDC_SETTINGS_HTML, OnSettingsChange)
    ON_BN_CLICKED(IDC_SETTINGS_PDF, OnSettingsChange)
    ON_BN_CLICKED(IDC_SETTINGS_FROM_BUILDS, OnSettingsChange)
    ON_BN_CLICKED(IDC_SETTINGS_CUSTOM, OnSettingsChange)
    ON_CBN_SELCHANGE(IDC_SETTINGS_BUILD, OnBuildChange)
    ON_BN_CLICKED(IDC_SETTINGS_FILE_BROWSE, OnSettingsFileBrowse)
    ON_EN_CHANGE(IDC_SETTINGS_CUSTOM_TEXT, OnSettingsTextChange)
END_MESSAGE_MAP()


CSDocExportDlg::CSDocExportDlg(cs::non_null_shared_or_raw_ptr<DocSetSpec> doc_set_spec, std::string csdoc_file_path, CLogicCtrl& logic_ctrl, CWnd* const pParent/* = nullptr*/)
    :   CDialog(IDD_EXPORT_CSDOC, pParent),
        m_settingsDb(CSProExecutables::Program::CSDocument, ExportSettingsTableName, ExportSettingsExpirationSeconds, SettingsDb::KeyObfuscator::Hash),
        m_docSetSpec(std::move(doc_set_spec)),
        m_csdocFilePath(std::move(csdoc_file_path)),
        m_logicCtrl(logic_ctrl),
        m_docSetCompiler(DocSetCompiler::ThrowErrors { }),
        m_jsonReaderInterface(PortableFunctions::PathGetDirectory(m_csdocFilePath)),
        m_buildSettingsSourceFilePath(m_docSetSpec->GetFilePath()),
        m_settingsButton(SettingsHtml),
        m_settingsTextIsBeingPrefilled(false)
{
    // load the options previously associated with exporting this document
    if( !m_csdocFilePath.empty() )
    {
        const std::optional<std::string> options_json = m_settingsDb.Read<std::string>(m_csdocFilePath);

        if( options_json.has_value() )
        {
            try
            {
                const JsonNode json_node = Json::Parse(*options_json);

                m_outputFilePath = json_node.GetOrDefault(JK::path, m_outputFilePath);

                m_settingsButton = std::max(SettingsHtml, std::min(SettingsCustom, json_node.GetOrDefault(JK::type, m_settingsButton)));

                if( m_settingsButton == SettingsFromBuilds )
                {
                    m_selectedBuildSettingName = json_node.GetOrConstruct<std::string>(JK::name);
                }

                else if( m_settingsButton == SettingsCustom )
                {
                    m_settingsText = json_node.GetOrDefault(JK::settings, m_settingsText);
                }
            }
            catch(...) { }
        }
    }

    // suggest an output filename is none was specified
    if( m_outputFilePath.empty() )
        SyncOutputFilePathWithSettings(false);
}


CSDocExportDlg::~CSDocExportDlg()
{
}


void CSDocExportDlg::DoDataExchange(CDataExchange* const pDX)
{
    __super::DoDataExchange(pDX);

    DDX_Text(pDX, IDC_OUTPUT_FILENAME, m_outputFilePath);
    DDX_Radio(pDX, IDC_SETTINGS_HTML, m_settingsButton);
    DDX_Control(pDX, IDC_SETTINGS_BUILD, m_docSetBuildsComboBox);
    DDX_Control(pDX, IDC_SETTINGS_CUSTOM_TEXT, m_settingsLogicCtrl);
}


BOOL CSDocExportDlg::OnInitDialog()
{
    __super::OnInitDialog();

    m_settingsLogicCtrl.ReplaceCEdit(this, false, false, SCLEX_JSON);

    UpdateNamedBuildSettings(m_docSetSpec->GetSettings());
    SetSettingsText();

    return TRUE;
}


void CSDocExportDlg::OnSize(const UINT nType, const int cx, const int cy)
{
    // the Scintilla control doesn't seem to respond to Dynamic Layout settings
    if( m_dynamicLayoutControlResizer == nullptr )
        m_dynamicLayoutControlResizer = std::make_unique<DynamicLayoutControlResizer>(*this, std::initializer_list<CWnd*>{ &m_settingsLogicCtrl });

    __super::OnSize(nType, cx, cy);

    m_dynamicLayoutControlResizer->OnSize(cx, cy);
}


void CSDocExportDlg::SetSettingsText()
{
    auto get_json_for_build_settings = [](const DocBuildSettings& build_settings)
    {
        const std::unique_ptr<JsonStringWriter> json_writer = Json::CreateStringWriter(JsonFormattingOptions::PrettySpacing);
        json_writer->Write(build_settings);
        return json_writer->ReleaseString();
    };

    if( m_settingsButton == SettingsHtml )
    {
        m_settingsText = get_json_for_build_settings(DocBuildSettings::DefaultSettingsForCSDocBuildToHtml());
    }

    else if( m_settingsButton == SettingsPdf )
    {
        m_settingsText = get_json_for_build_settings(DocBuildSettings::DefaultSettingsForCSDocBuildToPdf());
    }

    else if( m_settingsButton == SettingsFromBuilds )
    {
        if( static_cast<size_t>(m_docSetBuildsComboBox.GetCurSel()) < m_evaluatedBuildSettings.size() )
        {
            std::variant<DocBuildSettings, std::string>& build_settings_or_json_text = std::get<1>(m_evaluatedBuildSettings[m_docSetBuildsComboBox.GetCurSel()]);

            if( std::holds_alternative<DocBuildSettings>(build_settings_or_json_text) )
                build_settings_or_json_text = get_json_for_build_settings(std::get<DocBuildSettings>(build_settings_or_json_text));

            m_settingsText = std::get<std::string>(build_settings_or_json_text);
        }

        else
        {
            m_settingsText.clear();
        }
    }

    m_settingsTextIsBeingPrefilled = true;
    m_settingsLogicCtrl.SetText(m_settingsText);
    m_settingsTextIsBeingPrefilled = false;
}


std::optional<DocBuildSettings> CSDocExportDlg::GetDocBuildSettingsFromSettingsText(const bool use_logic_ctrl_text, const bool throw_exceptions)
{
    // short-circuit parsing the text for the default options
    if( m_settingsButton == SettingsHtml )
    {
        return DocBuildSettings::DefaultSettingsForCSDocBuildToHtml();
    }

    else if( m_settingsButton == SettingsPdf )
    {
        return DocBuildSettings::DefaultSettingsForCSDocBuildToPdf();
    }

    try
    {
        if( use_logic_ctrl_text )
            m_settingsText = m_settingsLogicCtrl.GetText();

        const JsonNode json_node = Json::Parse(m_settingsText, &m_jsonReaderInterface);

        DocBuildSettings build_settings;
        build_settings.Compile(m_docSetCompiler, json_node);

        return build_settings;
    }

    catch( const CSProException& exception )
    {
        if( throw_exceptions )
            throw CSProException("There was an error processing the build settings: %s", exception.what());

        return std::nullopt;
    }
}


bool CSDocExportDlg::SyncOutputFilePathWithSettings(const bool use_logic_ctrl_text)
{
    // when a user has manually modified the filename, stop suggesting other filenames
    if( m_lastSuggestedOutputFilePath.has_value() && m_outputFilePath != *m_lastSuggestedOutputFilePath )
        return false;

    // the suggested directory will be...
    std::string suggested_directory;

    // ...the output directory specified in the build settings
    const std::optional<DocBuildSettings> build_settings = GetDocBuildSettingsFromSettingsText(use_logic_ctrl_text, false);

    if( build_settings.has_value() && PortableFunctions::FileIsDirectory(build_settings->GetOutputDirectory()) )
    {
        suggested_directory = build_settings->GetOutputDirectory();
    }

    // ...the current directory specified
    else if( std::string current_directory = PortableFunctions::PathGetDirectory(m_outputFilePath);
             PortableFunctions::FileIsDirectory(current_directory) )
    {
        suggested_directory = std::move(current_directory);
    }

    // ...or the directory of the CSPro document
    else
    {
        suggested_directory = PortableFunctions::PathGetDirectory(m_csdocFilePath);
    }


    // the suggested filename will be the current one...
    std::string suggested_filename = Path::GetFilenameWithoutExtension(m_outputFilePath);

    // ...or if empty, based on the CSPro Document
    if( suggested_filename.empty() )
    {
        suggested_filename = m_csdocFilePath.empty() ? "CSPro Document" :
                                                       Path::GetFilenameWithoutExtension(m_csdocFilePath);
    }


    // the extension comes from the build type, defaulting to HTML if not specified
    const char* suggested_extension = FileExtensions::HTML;

    if( build_settings.has_value() && build_settings->GetBuildType() == DocBuildSettings::BuildType::Pdf )
        suggested_extension = FileExtensions::PDF;

    ASSERT(suggested_extension == FileExtensions::PDF || m_settingsButton != SettingsPdf);


    // construct the full suggested filename
    m_lastSuggestedOutputFilePath = PortableFunctions::CreateFilePath(suggested_directory, suggested_filename, suggested_extension);

    if( *m_lastSuggestedOutputFilePath == m_outputFilePath )
        return false;

    m_outputFilePath = *m_lastSuggestedOutputFilePath;

    return true;
}


void CSDocExportDlg::OnOutputFilePathBrowse()
{
    UpdateData(TRUE);

    SaveFileDlg save_file_dlg(0, nullptr, m_outputFilePath, L"HTML and PDF Files (*.html;*.pdf)|*.html;*.pdf|All Files (*.*)|*.*||", this);

    if( save_file_dlg.DoModal() != IDOK )
        return;

    m_outputFilePath = save_file_dlg.GetFilePath();

    UpdateData(FALSE);
}


void CSDocExportDlg::OnSettingsChange()
{
    UpdateData(TRUE);

    SetSettingsText();

    // by resetting m_lastSuggestedOutputFilePath, SyncOutputFilePathWithSettings will
    // provide a new filename suggestion, even if the user provided one manually
    m_lastSuggestedOutputFilePath.reset();

    if( SyncOutputFilePathWithSettings(true) )
        UpdateData(FALSE);
}


void CSDocExportDlg::OnBuildChange()
{
    if( m_settingsButton != SettingsFromBuilds )
    {
         m_settingsButton = SettingsFromBuilds;
         UpdateData(FALSE);
    }

    const size_t current_selection = static_cast<size_t>(m_docSetBuildsComboBox.GetCurSel());

    if( current_selection < m_evaluatedBuildSettings.size() )
    {
        m_selectedBuildSettingName = std::get<0>(m_evaluatedBuildSettings[current_selection]);
    }

    else
    {
        m_selectedBuildSettingName.clear();
    }

    OnSettingsChange();
}


void CSDocExportDlg::OnSettingsFileBrowse()
{
    const std::string filter = FormatText("CSPro Document Sets (*.%s)|*.%s|All Files (*.*)|*.*||",
                                          FileExtensions::CSDocumentSet, FileExtensions::CSDocumentSet);

    OpenFileDlg open_file_dlg(0, nullptr, m_buildSettingsSourceFilePath, filter, this);

    if( open_file_dlg.DoModal() != IDOK )
        return;

    try
    {
        std::string file_path = open_file_dlg.GetFilePath();
        const DocSetSettings doc_set_settings = DocSetCompiler::GetSettingsFromSpecOrSettingsFile(file_path);

        UpdateNamedBuildSettings(doc_set_settings);
        OnBuildChange();

        if( m_evaluatedBuildSettings.empty() )
            ErrorMessage::Display("There are no build settings in the file: " + file_path);

        m_buildSettingsSourceFilePath = std::move(file_path);
    }

    catch( const CSProException& exception )
    {
        ErrorMessage::Display(FormatText("There were problems reading the build settings: %s", exception.what()));
    }
}


void CSDocExportDlg::OnSettingsTextChange()
{
    // when the user modifies the settings JSON, automatically select the Custom radio button
    if( m_settingsTextIsBeingPrefilled || m_settingsButton == SettingsCustom )
        return;

    m_settingsButton = SettingsCustom;

    UpdateData(FALSE);
}


void CSDocExportDlg::UpdateNamedBuildSettings(const DocSetSettings& doc_set_settings)
{
    m_docSetBuildsComboBox.ResetContent();
    m_evaluatedBuildSettings.clear();

    int add_index = 0;
    std::optional<int> matched_index;

    auto add = [&](std::string name, DocBuildSettings build_settings)
    {
        if( name == m_selectedBuildSettingName )
            matched_index = add_index;

        m_docSetBuildsComboBox.AddString(TC::ToWide(name).c_str());
        m_evaluatedBuildSettings.emplace_back(std::move(name), std::move(build_settings));

        ++add_index;
    };

    if( doc_set_settings.GetDefaultBuildSettings().has_value() )
        add("<Default Settings>", *doc_set_settings.GetDefaultBuildSettings());

    for( const auto& [name, build_settings] : doc_set_settings.GetNamedBuildSettings() )
        add(name, doc_set_settings.GetEvaluatedBuildSettings(build_settings));

    // if no matched index, select the first option
    if( matched_index.has_value() || add_index != 0 )
        m_docSetBuildsComboBox.SetCurSel(matched_index.value_or(0));
}


void CSDocExportDlg::OnOK()
{
    UpdateData(TRUE);

    try
    {
        if( SO::IsWhitespace(m_outputFilePath) )
            throw CSProException("You must specify a output filename.");

        if( m_settingsButton == SettingsFromBuilds && m_docSetBuildsComboBox.GetCurSel() < 0 )
            throw CSProException("You must specify a build target.");

        // validate the build settings
        std::optional<DocBuildSettings> build_settings = GetDocBuildSettingsFromSettingsText(true, true);
        ASSERT(build_settings.has_value());

        // create the task
        m_generateTask = std::make_unique<CSDocCompilerBuildToFileGenerateTask>(m_docSetSpec, std::move(*build_settings),
                                                                                m_csdocFilePath, m_logicCtrl.GetText(), m_outputFilePath);
        m_generateTask->ValidateInputs();
    }

    catch( const CSProException& exception )
    {
        m_generateTask.reset();
        ErrorMessage::Display(exception);
        return;
    }

    // save these the options so they can be restored the next time this document is exported
    if( !m_csdocFilePath.empty() )
    {
        try
        {
            const std::unique_ptr<JsonStringWriter> json_writer = Json::CreateStringWriter();

            json_writer->BeginObject();

            // only write the output filename if it is different from the suggestion
            if( m_outputFilePath != m_lastSuggestedOutputFilePath )
                json_writer->Write(JK::path, m_outputFilePath);

            json_writer->Write(JK::type, m_settingsButton);

            if( m_settingsButton == SettingsFromBuilds )
            {
                json_writer->Write(JK::name, m_selectedBuildSettingName);
            }

            else if( m_settingsButton == SettingsCustom )
            {
                json_writer->Write(JK::settings, m_settingsText);
            }

            json_writer->EndObject();

            m_settingsDb.Write(m_csdocFilePath, json_writer->GetString());
        }
        catch(...) { }
    }

    __super::OnOK();
}
