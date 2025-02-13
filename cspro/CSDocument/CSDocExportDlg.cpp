#include "StdAfx.h"
#include "CSDocExportDlg.h"
#include "CSDocCompiler.h"
#include <zUtilF/DynamicLayoutControlResizer.h>


namespace
{
    // export settings, associated with documents, will be persisted for eight weeks
    constexpr const TCHAR* ExportSettingsTableName    = _T("export_settings");
    constexpr int64_t ExportSettingsExpirationSeconds = DateHelper::SecondsInWeek(8);

    constexpr int SettingsHtml       = 0;
    constexpr int SettingsPdf        = 1;
    constexpr int SettingsFromBuilds = 2;
    constexpr int SettingsCustom     = 3;
}


BEGIN_MESSAGE_MAP(CSDocExportDlg, CDialog)
    ON_WM_SIZE()
    ON_COMMAND(IDC_OUTPUT_FILENAME_BROWSE, OnOutputFilenameBrowse)
    ON_BN_CLICKED(IDC_SETTINGS_HTML, OnSettingsChange)
    ON_BN_CLICKED(IDC_SETTINGS_PDF, OnSettingsChange)
    ON_BN_CLICKED(IDC_SETTINGS_FROM_BUILDS, OnSettingsChange)
    ON_BN_CLICKED(IDC_SETTINGS_CUSTOM, OnSettingsChange)
    ON_CBN_SELCHANGE(IDC_SETTINGS_BUILD, OnBuildChange)
    ON_BN_CLICKED(IDC_SETTINGS_FILE_BROWSE, OnSettingsFileBrowse)
    ON_EN_CHANGE(IDC_SETTINGS_CUSTOM_TEXT, OnSettingsTextChange)
END_MESSAGE_MAP()


CSDocExportDlg::CSDocExportDlg(cs::non_null_shared_or_raw_ptr<DocSetSpec> doc_set_spec, std::wstring csdoc_filename, CLogicCtrl& logic_ctrl, CWnd* pParent/* = nullptr*/)
    :   CDialog(IDD_EXPORT_CSDOC, pParent),
        m_settingsDb(CSProExecutables::Program::CSDocument, ExportSettingsTableName, ExportSettingsExpirationSeconds, SettingsDb::KeyObfuscator::Hash),
        m_docSetSpec(std::move(doc_set_spec)),
        m_csdocFilename(std::move(csdoc_filename)),
        m_logicCtrl(logic_ctrl),
        m_docSetCompiler(DocSetCompiler::ThrowErrors { }),
        m_jsonReaderInterface(PortableFunctions::PathGetDirectory(m_csdocFilename)),
        m_buildSettingsSourceFilename(m_docSetSpec->GetFilename()),
        m_settingsButton(SettingsHtml),
        m_settingsTextIsBeingPrefilled(false)
{
    // load the options previously associated with exporting this document
    if( !m_csdocFilename.empty() )
    {
        const std::optional<std::wstring> options_json = m_settingsDb.Read<std::wstring>(m_csdocFilename);

        if( options_json.has_value() )
        {
            try
            {
                const auto json_node = Json::Parse(*options_json);

                m_outputFilename = json_node.GetOrDefault(JK::path, m_outputFilename);

                m_settingsButton = std::max(SettingsHtml, std::min(SettingsCustom, json_node.GetOrDefault(JK::type, m_settingsButton)));

                if( m_settingsButton == SettingsFromBuilds )
                {
                    m_selectedBuildSettingName = json_node.GetOrDefault(JK::name, SO::EmptyString);
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
    if( m_outputFilename.empty() )
        SyncOutputFilenameWithSettings(false);
}


CSDocExportDlg::~CSDocExportDlg()
{
}


void CSDocExportDlg::DoDataExchange(CDataExchange* pDX)
{
    __super::DoDataExchange(pDX);

    DDX_Text(pDX, IDC_OUTPUT_FILENAME, m_outputFilename);
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


void CSDocExportDlg::OnSize(UINT nType, int cx, int cy)
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
        auto json_writer = Json::CreateStringWriter(JsonFormattingOptions::PrettySpacing);
        json_writer->Write(build_settings);
        return json_writer->GetString();
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
            std::variant<DocBuildSettings, std::wstring>& build_settings_or_json_text = std::get<1>(m_evaluatedBuildSettings[m_docSetBuildsComboBox.GetCurSel()]);

            if( std::holds_alternative<DocBuildSettings>(build_settings_or_json_text) )
                build_settings_or_json_text = get_json_for_build_settings(std::get<DocBuildSettings>(build_settings_or_json_text));

            m_settingsText = std::get<std::wstring>(build_settings_or_json_text);
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


std::optional<DocBuildSettings> CSDocExportDlg::GetDocBuildSettingsFromSettingsText(bool use_logic_ctrl_text, bool throw_exceptions)
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

        const auto json_node = Json::Parse(m_settingsText, &m_jsonReaderInterface);

        DocBuildSettings build_settings;
        build_settings.Compile(m_docSetCompiler, json_node);

        return build_settings;
    }

    catch( const CSProException& exception )
    {
        if( throw_exceptions )
            throw CSProException(_T("There was an error processing the build settings: ") + exception.GetErrorMessage());

        return std::nullopt;
    }
}


bool CSDocExportDlg::SyncOutputFilenameWithSettings(bool use_logic_ctrl_text)
{
    // when a user has manually modified the filename, stop suggesting other filenames
    if( m_lastSuggestedOutputFilename.has_value() && m_outputFilename != *m_lastSuggestedOutputFilename )
        return false;

    // the suggested directory will be...
    std::wstring suggested_directory;

    // ...the output directory specified in the build settings
    const std::optional<DocBuildSettings> build_settings = GetDocBuildSettingsFromSettingsText(use_logic_ctrl_text, false);

    if( build_settings.has_value() && PortableFunctions::FileIsDirectory(build_settings->GetOutputDirectory()) )
    {
        suggested_directory = build_settings->GetOutputDirectory();
    }

    // ...the current directory specified
    else if( std::wstring current_directory = PortableFunctions::PathGetDirectory(m_outputFilename);
             PortableFunctions::FileIsDirectory(current_directory) )
    {
        suggested_directory = std::move(current_directory);
    }

    // ...or the directory of the CSPro document
    else
    {
        suggested_directory = PortableFunctions::PathGetDirectory(m_csdocFilename);
    }


    // the suggested filename will be the current one...
    std::wstring suggested_filename = PortableFunctions::PathGetFilenameWithoutExtension(m_outputFilename);

    // ...or if empty, based on the CSPro Document
    if( suggested_filename.empty() )
    {
        suggested_filename = m_csdocFilename.empty() ? _T("CSPro Document") :
                                                       PortableFunctions::PathGetFilenameWithoutExtension(m_csdocFilename);
    }


    // the extension comes from the build type, defaulting to HTML if not specified
    const TCHAR* suggested_extension = FileExtensions::WithDot::HTML;

    if( build_settings.has_value() && build_settings->GetBuildType() == DocBuildSettings::BuildType::Pdf )
        suggested_extension = FileExtensions::WithDot::PDF;

    ASSERT(suggested_extension == FileExtensions::WithDot::PDF || m_settingsButton != SettingsPdf);


    // construct the full suggested filename
    m_lastSuggestedOutputFilename = PortableFunctions::PathAppendToPath(suggested_directory, suggested_filename + suggested_extension);

    if( *m_lastSuggestedOutputFilename == m_outputFilename )
        return false;

    m_outputFilename = *m_lastSuggestedOutputFilename;

    return true;
}


void CSDocExportDlg::OnOutputFilenameBrowse()
{
    UpdateData(TRUE);

    CIMSAFileDialog file_dlg(FALSE, nullptr, m_outputFilename.c_str(), OFN_HIDEREADONLY | OFN_OVERWRITEPROMPT,
                             _T("HTML and PDF Files (*.html;*.pdf)|*.html;*.pdf||All Files (*.*)|*.*||"), nullptr, CFD_NO_DIR);

    if( file_dlg.DoModal() != IDOK )
        return;

    m_outputFilename = CS2WS(file_dlg.GetPathName());

    UpdateData(FALSE);
}


void CSDocExportDlg::OnSettingsChange()
{
    UpdateData(TRUE);

    SetSettingsText();

    // by resetting m_lastSuggestedOutputFilename, SyncOutputFilenameWithSettings will
    // provide a new filename suggestion, even if the user provided one manually
    m_lastSuggestedOutputFilename.reset();

    if( SyncOutputFilenameWithSettings(true) )
        UpdateData(FALSE);
}


void CSDocExportDlg::OnBuildChange()
{
    if( m_settingsButton != SettingsFromBuilds )
    {
         m_settingsButton = SettingsFromBuilds;
         UpdateData(FALSE);
    }

    size_t current_selection = static_cast<size_t>(m_docSetBuildsComboBox.GetCurSel());

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
    const std::wstring filter = FormatTextCS2WS(_T("CSPro Document Sets (%s)|%s|All Files (*.*)|*.*||"),
                                                FileExtensions::Wildcard::CSDocumentSet, FileExtensions::Wildcard::CSDocumentSet);
    
    CIMSAFileDialog file_dlg(TRUE, nullptr, m_buildSettingsSourceFilename.c_str(), OFN_HIDEREADONLY, filter.c_str(), nullptr, CFD_NO_DIR);

    if( file_dlg.DoModal() != IDOK )
        return;

    try
    {
        std::wstring filename = CS2WS(file_dlg.GetPathName());
        const DocSetSettings doc_set_settings = DocSetCompiler::GetSettingsFromSpecOrSettingsFile(filename);

        UpdateNamedBuildSettings(doc_set_settings);
        OnBuildChange();

        if( m_evaluatedBuildSettings.empty() )
            ErrorMessage::Display(_T("There are no build settings in the file: ") + filename);

        m_buildSettingsSourceFilename = std::move(filename);
    }

    catch( const CSProException& exception )
    {
        ErrorMessage::Display(_T("There were problems reading the build settings: ") + exception.GetErrorMessage());
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

    auto add = [&](std::wstring name, DocBuildSettings build_settings)
    {
        if( name == m_selectedBuildSettingName )
            matched_index = add_index;

        m_docSetBuildsComboBox.AddString(name.c_str());
        m_evaluatedBuildSettings.emplace_back(std::move(name), std::move(build_settings));

        ++add_index;
    };   

    if( doc_set_settings.GetDefaultBuildSettings().has_value() )
        add(_T("<Default Settings>"), *doc_set_settings.GetDefaultBuildSettings());

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
        if( SO::IsWhitespace(m_outputFilename) )
            throw CSProException("You must specify a output filename.");

        if( m_settingsButton == SettingsFromBuilds && m_docSetBuildsComboBox.GetCurSel() < 0 )
            throw CSProException("You must specify a build target.");

        // validate the build settings
        std::optional<DocBuildSettings> build_settings = GetDocBuildSettingsFromSettingsText(true, true);
        ASSERT(build_settings.has_value());

        // create the task
        m_generateTask = std::make_unique<CSDocCompilerBuildToFileGenerateTask>(m_docSetSpec, std::move(*build_settings),
                                                                                m_csdocFilename, m_logicCtrl.GetText(), m_outputFilename);
        m_generateTask->ValidateInputs();
    }

    catch( const CSProException& exception )
    {
        m_generateTask.reset();
        ErrorMessage::Display(exception);
        return;
    }

    // save these the options so they can be restored the next time this document is exported
    if( !m_csdocFilename.empty() )
    {
        try
        {
            auto json_writer = Json::CreateStringWriter();

            json_writer->BeginObject();

            // only write the output filename if it is different from the suggestion
            if( m_outputFilename != m_lastSuggestedOutputFilename )
                json_writer->Write(JK::path, m_outputFilename);

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

            m_settingsDb.Write(m_csdocFilename, json_writer->GetString());
        }
        catch(...) { }
    }

    __super::OnOK(); 
}
