#include "stdafx.h"
#include "Application.h"
#include "Properties/ApplicationProperties.h"
#include <zUtilO/AppLoader.h>
#include <zUtilO/ArrUtil.h>
#include <zUtilO/TextSourceEditable.h>
#include <zUtilO/TextSourceExternal.h>


const char* ToString(const EngineAppType engine_app_type)
{
    return ( engine_app_type == EngineAppType::Entry )      ? "entry" :
           ( engine_app_type == EngineAppType::Tabulation ) ? "tabulation" :
           ( engine_app_type == EngineAppType::Batch )      ? "batch" :
                                                              "invalid";
}


// --------------------------------------------------------------------------
// application properties
// --------------------------------------------------------------------------

Application::Application()
    :   m_version(Versioning::Number),
        m_serializerArchiveVersion(Serializer::GetCurrentVersion()),
        m_engineAppType(EngineAppType::Invalid),
        m_applicationProperties(std::make_unique<ApplicationProperties>()),
        m_askOperatorId(true),
        m_partialSave(false),
        m_autoPartialSaveMinutes(0),
        m_caseTreeType(CaseTreeType::MobileOnly),
        m_useQuestionText(false),
        m_showEndCaseMessage(true),
        m_centerForms(false),
        m_decimalMarkIsComma(false),
        m_createListingFile(true),
        m_createLogFile(true),
        m_editNotePermissions(EditNotePermissions::AllPermissions),
        m_autoAdvanceOnSelection(false),
        m_displayCodesAlongsideLabels(false),
        m_showFieldLabels(true),
        m_showErrorMessageNumbers(true),
        m_comboBoxShowOnlyDiscreteValues(false),
        m_showRefusals(true),
        m_verifyFrequency(1),
        m_verifyStart(1),
        m_hasWriteStatements(false),
        m_hasSaveableFrequencyStatements(false),
        m_hasImputeStatements(false),
        m_hasImputeStatStatements(false),
        m_hasSaveArrays(false),
        m_updateSaveArrayFile(true),
        m_pAppSrcCode(nullptr),
        m_compiled(false),
        m_optimizeFlowTree(false),
        m_pAppLoader(std::make_unique<CAppLoader>())
{
}


Application::Application(Application&& rhs) noexcept = default;


Application::Application(const Application& rhs)
    :   m_version(rhs.m_version),
        m_serializerArchiveVersion(rhs.m_serializerArchiveVersion),
        m_engineAppType(rhs.m_engineAppType),
        m_name(rhs.m_name),
        m_label(rhs.m_label),
        m_applicationFilePath(rhs.m_applicationFilePath),
        m_applicationPropertiesFilePath(rhs.m_applicationPropertiesFilePath),
        m_applicationProperties(std::make_unique<ApplicationProperties>(*rhs.m_applicationProperties)),
        m_questionTextFilePath(rhs.m_questionTextFilePath),
        m_formFilePaths(rhs.m_formFilePaths),
        m_tableSpecFilePaths(rhs.m_tableSpecFilePaths),
        m_externalDictionaryFilePaths(rhs.m_externalDictionaryFilePaths),
        m_codeFiles(rhs.m_codeFiles),
        m_messageFiles(rhs.m_messageFiles),
        m_reportFiles(rhs.m_reportFiles),
        m_resources(rhs.m_resources),
        m_dictionaryDescriptions(rhs.m_dictionaryDescriptions),
        m_askOperatorId(rhs.m_askOperatorId),
        m_partialSave(rhs.m_partialSave),
        m_autoPartialSaveMinutes(rhs.m_autoPartialSaveMinutes),
        m_caseTreeType(rhs.m_caseTreeType),
        m_useQuestionText(rhs.m_useQuestionText),
        m_showEndCaseMessage(rhs.m_showEndCaseMessage),
        m_centerForms(rhs.m_centerForms),
        m_decimalMarkIsComma(rhs.m_decimalMarkIsComma),
        m_createListingFile(rhs.m_createListingFile),
        m_createLogFile(rhs.m_createLogFile),
        m_editNotePermissions(rhs.m_editNotePermissions),
        m_autoAdvanceOnSelection(rhs.m_autoAdvanceOnSelection),
        m_displayCodesAlongsideLabels(rhs.m_displayCodesAlongsideLabels),
        m_showFieldLabels(rhs.m_showFieldLabels),
        m_showErrorMessageNumbers(rhs.m_showErrorMessageNumbers),
        m_comboBoxShowOnlyDiscreteValues(rhs.m_comboBoxShowOnlyDiscreteValues),
        m_showRefusals(rhs.m_showRefusals),
        m_verifyFrequency(rhs.m_verifyFrequency),
        m_verifyStart(rhs.m_verifyStart),
        m_syncParameters(rhs.m_syncParameters),
        m_logicSettings(rhs.m_logicSettings),
        m_mappingOptions(rhs.m_mappingOptions),
        m_hasWriteStatements(rhs.m_hasWriteStatements),
        m_hasSaveableFrequencyStatements(rhs.m_hasSaveableFrequencyStatements),
        m_hasImputeStatements(rhs.m_hasImputeStatements),
        m_hasImputeStatStatements(rhs.m_hasImputeStatStatements),
        m_hasSaveArrays(rhs.m_hasSaveArrays),
        m_updateSaveArrayFile(rhs.m_updateSaveArrayFile),
        m_pAppSrcCode(nullptr),
        m_compiled(false),
        m_optimizeFlowTree(false),
        m_pAppLoader(std::make_unique<CAppLoader>())
{
}


Application::~Application()
{
}


AppFileType Application::GetApplicationAppFileType() const
{
    return ( m_engineAppType == EngineAppType::Entry )      ? AppFileType::ApplicationEntry :
           ( m_engineAppType == EngineAppType::Batch )      ? AppFileType::ApplicationBatch :
           ( m_engineAppType == EngineAppType::Tabulation ) ? AppFileType::ApplicationTabulation :
                                                              ReturnProgrammingError(AppFileType::ApplicationBatch);
}


void Application::SetApplicationProperties(ApplicationProperties application_properties)
{
    *m_applicationProperties = std::move(application_properties);
}


// --------------------------------------------------------------------------
// miscellaneous functionality
// --------------------------------------------------------------------------

void Application::ClearApplicationFiles()
{
    m_questionTextFilePath.clear();

    m_formFilePaths.clear();
    m_tableSpecFilePaths.clear();

    m_externalDictionaryFilePaths.clear();
    m_dictionaryDescriptions.clear();

    m_codeFiles.clear();
    m_messageFiles.clear();
    m_reportFiles.clear();
    m_resources.clear();
}


// --------------------------------------------------------------------------
// form files
// --------------------------------------------------------------------------

const std::string* Application::GetForm(const std::string& form_file_path)
{
    const auto& lookup = std::find_if(m_formFilePaths.cbegin(), m_formFilePaths.cend(),
                                      [&](const std::string& this_form_file_path) { return SO::EqualsNoCase(form_file_path, this_form_file_path); });

    return ( lookup != m_formFilePaths.cend() ) ? &(*lookup) :
                                                  nullptr;

}

void Application::AddForm(std::string form_file_path)
{
    ASSERT(GetEngineAppType() == EngineAppType::Entry || GetEngineAppType() == EngineAppType::Batch);
    ASSERT(GetForm(form_file_path) == nullptr);

    m_formFilePaths.emplace_back(std::move(form_file_path));
}


void Application::DropForm(const std::string& form_file_path)
{
    const auto& lookup = std::find_if(m_formFilePaths.cbegin(), m_formFilePaths.cend(),
                                      [&](const std::string& this_form_file_path) { return SO::EqualsNoCase(form_file_path, this_form_file_path); });
    ASSERT(lookup != m_formFilePaths.cend());

    if( lookup != m_formFilePaths.cend() )
        m_formFilePaths.erase(lookup);
}


void Application::RenameFormFilePath(const std::string& original_form_file_path, std::string new_form_file_path)
{
    for( std::string& form_file_path: m_formFilePaths )
    {
        if( SO::EqualsNoCase(original_form_file_path, form_file_path) )
        {
            form_file_path = std::move(new_form_file_path);
            return;
        }
    }
}



// --------------------------------------------------------------------------
// table specs
// --------------------------------------------------------------------------

void Application::AddTableSpec(std::string table_spec_file_path)
{
    ASSERT(GetEngineAppType() == EngineAppType::Tabulation);
    ASSERT(m_tableSpecFilePaths.empty());

    m_tableSpecFilePaths.emplace_back(std::move(table_spec_file_path));
}


void Application::RenameTableSpecFilePath(const std::string& original_table_spec_file_path, std::string new_table_spec_file_path)
{
   for( std::string& table_spec_file_path : m_tableSpecFilePaths )
    {
        if( SO::EqualsNoCase(original_table_spec_file_path, table_spec_file_path) )
        {
            table_spec_file_path = std::move(new_table_spec_file_path);
            return;
        }
    }
}



// --------------------------------------------------------------------------
// external dictionaries
// --------------------------------------------------------------------------

void Application::AddExternalDictionary(std::string dictionary_file_path)
{
    m_externalDictionaryFilePaths.emplace_back(std::move(dictionary_file_path));
}


void Application::DropExternalDictionary(const std::string& dictionary_file_path)
{
    for( auto itr = m_externalDictionaryFilePaths.cbegin(); itr != m_externalDictionaryFilePaths.cend(); ++itr )
    {
        if( SO::EqualsNoCase(dictionary_file_path, *itr) )
        {
            m_externalDictionaryFilePaths.erase(itr);
            return;
        }
    }
}


void Application::RenameExternalDictionaryFilePath(const std::string& original_dictionary_file_path, std::string new_dictionary_file_path)
{
    for( std::string& dictionary_file_path : m_externalDictionaryFilePaths )
    {
        if( SO::EqualsNoCase(original_dictionary_file_path, dictionary_file_path) )
        {
            dictionary_file_path = new_dictionary_file_path;
            break;
        }
    }

    for( DictionaryDescription& dictionary_description : m_dictionaryDescriptions )
    {
        if( SO::EqualsNoCase(original_dictionary_file_path, dictionary_description.GetDictionaryFilePath()) )
        {
            dictionary_description.SetDictionaryFilePath(std::move(new_dictionary_file_path));
            return;
        }
    }
}



// --------------------------------------------------------------------------
// code files
// --------------------------------------------------------------------------

const CodeFile* Application::GetCodeFile(const std::string& file_path) const
{
    const auto& lookup = std::find_if(m_codeFiles.cbegin(), m_codeFiles.cend(),
                                      [&](const CodeFile& code_file) { return SO::EqualsNoCase(file_path, code_file.GetFilePath()); });

    return ( lookup != m_codeFiles.cend() ) ? &(*lookup) :
                                              nullptr;
}


const CodeFile* Application::GetLogicMainCodeFile() const
{
    const auto& lookup = std::find_if(m_codeFiles.cbegin(), m_codeFiles.cend(),
                                      [&](const CodeFile& code_file) { return code_file.IsLogicMain(); });

    return ( lookup != m_codeFiles.cend() ) ? &(*lookup) :
                                              nullptr;
}


CodeFile* Application::GetLogicMainCodeFile()
{
    return const_cast<CodeFile*>(const_cast<const Application*>(this)->GetLogicMainCodeFile());
}


void Application::AddCodeFile(CodeFile code_file)
{
    ASSERT(!IsFilePathInUse(m_codeFiles, code_file.GetFilePath()));
    ASSERT(!code_file.IsLogicMain() || GetLogicMainCodeFile() == nullptr);

    m_codeFiles.emplace_back(std::move(code_file));
}


void Application::DropCodeFile(const std::string& file_path)
{
    const auto& lookup = std::find_if(m_codeFiles.cbegin(), m_codeFiles.cend(),
                                      [&](const CodeFile& code_file) { return SO::EqualsNoCase(file_path, code_file.GetFilePath()); });
    ASSERT(lookup != m_codeFiles.cend());

    if( lookup != m_codeFiles.cend() )
        m_codeFiles.erase(lookup);
}



// --------------------------------------------------------------------------
// message files
// --------------------------------------------------------------------------

void Application::AddMessageFile(AppMessageFile app_message_file)
{
    ASSERT(!IsFilePathInUse(m_messageFiles, app_message_file.GetFilePath()));
    m_messageFiles.emplace_back(std::move(app_message_file));
}


void Application::DropMessageFile(const std::string& file_path)
{
    const auto& lookup = std::find_if(m_messageFiles.cbegin(), m_messageFiles.cend(),
                                      [&](const AppMessageFile& app_message_file) { return SO::EqualsNoCase(file_path, app_message_file.GetFilePath()); });
    ASSERT(lookup != m_messageFiles.cend());

    if( lookup != m_messageFiles.cend() )
        m_messageFiles.erase(lookup);
}



// --------------------------------------------------------------------------
// reports
// --------------------------------------------------------------------------

const ReportFile* Application::GetReportFile(const std::string_view name_or_file_path_sv, const bool search_by_name) const
{
    const auto& lookup = std::find_if(m_reportFiles.cbegin(), m_reportFiles.cend(),
        [&](const ReportFile& report_file)
        {
            return SO::EqualsNoCase(name_or_file_path_sv, search_by_name ? report_file.GetName() :
                                                                           report_file.GetFilePath());
        });

    return ( lookup != m_reportFiles.cend() ) ? &(*lookup) :
                                                nullptr;
}


void Application::AddReport(ReportFile report_file)
{
    ASSERT(GetReportFile(report_file.GetName(), true) == nullptr);
    m_reportFiles.emplace_back(std::move(report_file));
}


void Application::DropReport(const std::string& file_path)
{
    const auto& lookup = std::find_if(m_reportFiles.cbegin(), m_reportFiles.cend(),
                                      [&](const ReportFile& report_file) { return SO::EqualsNoCase(file_path, report_file.GetFilePath()); });
    ASSERT(lookup != m_reportFiles.cend());

    if( lookup != m_reportFiles.cend() )
        m_reportFiles.erase(lookup);
}



// --------------------------------------------------------------------------
// resources
// --------------------------------------------------------------------------

const AppResource* Application::GetResource(const std::string& path) const
{
    const auto& lookup = std::find_if(m_resources.cbegin(), m_resources.cend(),
                                      [&](const AppResource& resource) { return SO::EqualsNoCase(path, resource.GetPath()); });

    return ( lookup != m_resources.cend() ) ? &(*lookup) :
                                              nullptr;
}


const AppResource& Application::AddResource(AppResource resource)
{
    ASSERT(GetResource(resource.GetPath()) == nullptr);
    return m_resources.emplace_back(std::move(resource));
}


void Application::DropResource(const std::string& path)
{
    const auto& lookup = std::find_if(m_resources.cbegin(), m_resources.cend(),
                                      [&](const AppResource& resource) { return SO::EqualsNoCase(path, resource.GetPath()); });
    ASSERT(lookup != m_resources.cend());

    if( lookup != m_resources.cend() )
        m_resources.erase(lookup);
}



// --------------------------------------------------------------------------
// dictionary descriptions
// --------------------------------------------------------------------------

DictionaryType Application::GetDictionaryType(const CDataDict& dictionary) const
{
    const DictionaryDescription* const dictionary_description = GetDictionaryDescription(dictionary);

    return ( dictionary_description == nullptr ) ? DictionaryType::Unknown :
                                                   dictionary_description->GetDictionaryType();
}


const DictionaryDescription* Application::GetDictionaryDescription(const CDataDict& dictionary) const
{
    const auto& lookup = std::find_if(m_dictionaryDescriptions.cbegin(), m_dictionaryDescriptions.cend(),
        [&](const DictionaryDescription& dictionary_description)
        {
            return ( dictionary_description.GetDictionary() == &dictionary );
        });

    return ( lookup == m_dictionaryDescriptions.cend() ) ? nullptr :
                                                           &(*lookup);
}


const DictionaryDescription* Application::GetDictionaryDescription(const std::string& dictionary_file_path,
                                                                   const std::string& parent_file_path/* = SO::Empty_string*/,
                                                                   const bool ignore_parent/* = false*/) const
{
    const auto& lookup = std::find_if(m_dictionaryDescriptions.cbegin(), m_dictionaryDescriptions.cend(),
        [&](const DictionaryDescription& dictionary_description)
        {
            return ( ( SO::EqualsNoCase(dictionary_description.GetDictionaryFilePath(), dictionary_file_path) ) &&
                     ( ignore_parent || SO::EqualsNoCase(dictionary_description.GetParentFilePath(), parent_file_path) ) );
        });

    return ( lookup == m_dictionaryDescriptions.cend() ) ? nullptr :
                                                           &(*lookup);
}


DictionaryDescription* Application::GetDictionaryDescription(const std::string& dictionary_file_path,
                                                             const std::string& parent_file_path/* = SO::Empty_string*/,
                                                             const bool ignore_parent/* = false*/)
{
    return const_cast<DictionaryDescription*>(const_cast<const Application*>(this)->GetDictionaryDescription(dictionary_file_path, parent_file_path, ignore_parent));
}


const std::string& Application::GetFirstDictionaryFilePathOfType(const DictionaryType dictionary_type) const
{
    const auto& lookup = std::find_if(m_dictionaryDescriptions.cbegin(), m_dictionaryDescriptions.cend(),
        [&](const DictionaryDescription& dictionary_description)
        {
            return ( dictionary_description.GetDictionaryType() == dictionary_type );
        });

    return ( lookup == m_dictionaryDescriptions.cend() ) ? SO::Empty_string :
                                                           lookup->GetDictionaryFilePath();
}


void Application::DropDictionaryDescription(const std::string& file_path, const bool file_path_is_parent)
{
    const auto& lookup = std::find_if(m_dictionaryDescriptions.cbegin(), m_dictionaryDescriptions.cend(),
        [&](const DictionaryDescription& dictionary_description)
        {
            return SO::EqualsNoCase(file_path, file_path_is_parent ? dictionary_description.GetParentFilePath() :
                                                                     dictionary_description.GetDictionaryFilePath());
        });

    ASSERT(lookup != m_dictionaryDescriptions.cend());

    if( lookup != m_dictionaryDescriptions.cend() )
        m_dictionaryDescriptions.erase(lookup);
}



// --------------------------------------------------------------------------
// flags
// --------------------------------------------------------------------------

bool Application::GetShowCaseTree() const
{
    return ( ( m_caseTreeType == CaseTreeType::Always ) ||
             ( OnWindowsDesktop() && m_caseTreeType == CaseTreeType::DesktopOnly ) ||
             ( !OnWindowsDesktop() && m_caseTreeType == CaseTreeType::MobileOnly ) );
}


void Application::SetEditNotePermissions(int permission, bool flag)
{
    if( flag )
    {
        m_editNotePermissions |= permission;
    }

    else
    {
        // if an operator cannot delete a note, then they also cannot edit the note (since this would allow the note to be deleted indirectly)
        if( ( permission & EditNotePermissions::DeleteOtherOperators ) != 0 )
            permission |= EditNotePermissions::EditOtherOperators;

        m_editNotePermissions &= ~permission;

    }
}



//--------------------------------------------------------------------------
// other methods
// --------------------------------------------------------------------------

bool Application::IsNameUnique(const std::string_view name_sv) const
{
    // search reports
    if( GetReportFile(name_sv, true) != nullptr )
        return false;

    return true;
}



// --------------------------------------------------------------------------
// serialization
// --------------------------------------------------------------------------

CREATE_JSON_VALUE(application)
CREATE_JSON_VALUE(map)
CREATE_JSON_VALUE(random)

CREATE_ENUM_JSON_SERIALIZER(EngineAppType,
    { EngineAppType::Entry,      ToString(EngineAppType::Entry) },
    { EngineAppType::Tabulation, ToString(EngineAppType::Tabulation) },
    { EngineAppType::Batch,      ToString(EngineAppType::Batch) })

CREATE_ENUM_JSON_SERIALIZER(CaseTreeType,
    { CaseTreeType::Never,       "off" },
    { CaseTreeType::MobileOnly,  "mobileOnly" },
    { CaseTreeType::DesktopOnly, "desktopOnly" },
    { CaseTreeType::Always,      "on" })

// ideally these enums would be used by the class instead of bools, but
// that refactoring can be done at a later point
enum class DecimalMark { Dot, Comma };
enum class NotePermission { Operator, All };

CREATE_ENUM_JSON_SERIALIZER(DecimalMark,
    { DecimalMark::Dot,   "dot" },
    { DecimalMark::Comma, "comma" })

CREATE_ENUM_JSON_SERIALIZER(NotePermission,
    { NotePermission::Operator, "operator" },
    { NotePermission::All,      "all" })


void Application::Open(const InterfaceString file_path, const bool silent/* = false*/, const bool load_text_sources_and_external_application_properties/* = true*/)
{
    const std::unique_ptr<JsonSpecFile::Reader> json_reader = JsonSpecFile::CreateReader(file_path, nullptr, [&]() { return ConvertPre80SpecFile(file_path); });

    try
    {
        m_version = json_reader->CheckVersion();
        json_reader->CheckFileType(JV::application);

        CreateFromJsonWorker(*json_reader, load_text_sources_and_external_application_properties, silent, json_reader->GetSharedMessageLogger());

        m_applicationFilePath = file_path.GetString<std::string>();
    }

    catch( const CSProException& exception )
    {
        json_reader->GetMessageLogger().RethrowException(file_path, exception);
    }

    // report any warnings
    json_reader->GetMessageLogger().DisplayWarnings(silent);
}


void Application::Save(InterfaceString file_path, const bool continue_using_file_path/* = true*/) const
{
    const std::unique_ptr<JsonFileWriter> json_writer = JsonSpecFile::CreateWriter(file_path, JV::application);

    WriteJson(*json_writer, false);

    json_writer->EndObject();

    json_writer->Close();

    if( continue_using_file_path )
        const_cast<Application*>(this)->m_applicationFilePath = file_path.Release<std::string>();
}


Application Application::CreateFromJson(const JsonNode& json_node)
{
    Application application;
    application.CreateFromJsonWorker(json_node, false, true, nullptr);
    return application;
}


void Application::CreateFromJsonWorker(const JsonNode& json_node, const bool load_text_sources_and_external_application_properties,
                                       const bool silent, std::shared_ptr<JsonSpecFile::ReaderMessageLogger> message_logger)
{
    m_engineAppType = json_node.Get<EngineAppType>(JK::type);
    const bool entry_app = ( m_engineAppType == EngineAppType::Entry );

    m_name = SO::ToUpper(json_node.Get<std::string>(JK::name));
    m_label = json_node.GetOrConstruct<std::string>(JK::label);

    // reading routines
    // --------------------------------------------------------------------------
    auto read_path = [&](const JsonNode& path_node, const bool throw_exception_if_file_is_not_regular)
    {
        std::string path = path_node.GetAbsolutePath();

        if( throw_exception_if_file_is_not_regular && !PortableFunctions::FileIsRegular(path) )
            throw ApplicationFileNotFoundException(path);

        return path;
    };

    auto read_paths = [&](const JsonNodeArray& paths_array_node, const bool throw_exception_if_file_is_not_regular)
    {
        std::vector<std::string> paths;

        for( const JsonNode& path_node : paths_array_node )
            paths.emplace_back(read_path(path_node, throw_exception_if_file_is_not_regular));

        return paths;
    };

    auto check_file_is_regular_and_warn_if_not_exist = [&](const char* const file_type, const std::string& file_path)
    {
        if( PortableFunctions::FileIsRegular(file_path) )
            return true;

        json_node.LogWarning("The %s file was not found and will be dropped: %s",
                             file_type, file_path.c_str());
        return false;
    };

    auto load_text_source = [&](std::string file_path, const bool text_source_is_editable)
    {
        std::shared_ptr<TextSource> text_source;

        if( text_source_is_editable && load_text_sources_and_external_application_properties )
        {
            text_source = TextSourceEditable::FindOpenOrCreate(std::move(file_path));
        }

        else
        {
            text_source = std::make_unique<TextSourceExternal>(std::move(file_path));
        }

        return text_source;
    };

    auto check_name = [&](const char* const file_type, const std::string& name, const std::string& file_path)
    {
        if( !CIMSAString::IsName(name) || CIMSAString::IsReservedWord(name) )
        {
            json_node.LogWarning("The %s name '%s' is not valid and the %s will not be loaded.",
                                 file_type, name.c_str(), file_type);
            return false;
        }

        if( !IsNameUnique(name) )
        {
            json_node.LogWarning("The %s name '%s' is already in use so the %s '%s' will not be loaded.",
                                 file_type, name.c_str(), file_type, PortableFunctions::PathGetFilename(file_path).c_str());
            return false;
        }

        return true;
    };


    // forms / orders / table specs
    // --------------------------------------------------------------------------
    if( entry_app || m_engineAppType == EngineAppType::Batch )
    {
        m_formFilePaths = read_paths(json_node.GetArrayOrEmpty(entry_app ? JK::forms : JK::order), true);
    }

    else if( m_engineAppType == EngineAppType::Tabulation )
    {
        m_tableSpecFilePaths = read_paths(json_node.GetArrayOrEmpty(JK::tableSpecs), true);
    }

    const std::vector<std::string>& form_or_table_spec_file_paths = ( m_engineAppType == EngineAppType::Tabulation ) ? m_tableSpecFilePaths :
                                                                                                                       m_formFilePaths;

    if( form_or_table_spec_file_paths.empty() )
    {
        throw CSProException("A '%s' application must have at least one %s",    ToString(m_engineAppType),
                             entry_app                                        ? "form" :
                             ( m_engineAppType == EngineAppType::Tabulation ) ? "tab spec" :
                                                                                "order");
    }


    // dictionaries
    // --------------------------------------------------------------------------
    for( const JsonNode& dictionary_node : json_node.GetArrayOrEmpty(JK::dictionaries) )
    {
        DictionaryDescription dictionary_description = dictionary_node.Get<DictionaryDescription>();

        if( !check_file_is_regular_and_warn_if_not_exist("dictionary", dictionary_description.GetDictionaryFilePath()) )
            continue;

        if( !dictionary_description.GetParentFilePath().empty() &&
            !ContainsStringInVectorNoCase(form_or_table_spec_file_paths, dictionary_description.GetParentFilePath()) )
        {
            json_node.LogWarning("The dictionary's parent '%s' was not valid and will be reset: %s",
                                 GetRelativePathForDisplay(dictionary_description.GetDictionaryFilePath(), dictionary_description.GetParentFilePath()).c_str(),
                                 dictionary_description.GetDictionaryFilePath().c_str());

            dictionary_description.SetParentFilePath(std::string());
        }

        // if this dictionary description does not have a parent, add it as an external dictionary
        if( dictionary_description.GetParentFilePath().empty() )
            m_externalDictionaryFilePaths.emplace_back(dictionary_description.GetDictionaryFilePath());

        m_dictionaryDescriptions.emplace_back(std::move(dictionary_description));
    }


    // question text
    // --------------------------------------------------------------------------
    if( entry_app )
    {
        std::vector<std::string> question_text_file_paths = read_paths(json_node.GetArrayOrEmpty(JK::questionText), true);

        if( question_text_file_paths.size() != 1 )
            throw CSProException("A '%s' application must have one question text file", ToString(m_engineAppType));

        m_questionTextFilePath = std::move(question_text_file_paths.front());
    }


    // code files
    // --------------------------------------------------------------------------
    for( const JsonNode& code_file_node : json_node.GetArrayOrEmpty(JK::code) )
    {
        try
        {
            CodeFile code_file = CodeFile::CreateFromJson(code_file_node,
                [&](const std::string& file_path)
                {
                    // all code files are editable
                    return load_text_source(file_path, true);
                });

            // don't add duplicate code files
            if( IsFilePathInUse(m_codeFiles, code_file.GetFilePath()) )
                continue;

            // make sure that there is only one main logic file
            if( code_file.IsLogicMain() )
            {
                const CodeFile* const logic_main_code_file = GetLogicMainCodeFile();

                if( logic_main_code_file != nullptr )
                {
                    const CodeType new_code_type = CodeFile::GetSuggestedCodeTypeForExternalCode(code_file.GetFilePath());

                    json_node.LogWarning("The application already has a main logic file, '%s', so '%s' is being set to: '%s'",
                                         Path::GetFilename(logic_main_code_file->GetFilePath()).c_str(),
                                         Path::GetFilename(code_file.GetFilePath()).c_str(),
                                         ToString(new_code_type));

                    code_file.SetCodeType(new_code_type);
                }
            }

            AddCodeFile(std::move(code_file));
        }

        catch( const CSProException& exception )
        {
            // don't abort on errors reading code files
            json_node.LogWarning("A code file could not be loaded and will be dropped: %s",
                                 exception.what());
        }
    }


    // message files
    // --------------------------------------------------------------------------
    for( const JsonNode& app_message_node : json_node.GetArrayOrEmpty(JK::messages) )
    {
        try
        {
            AppMessageFile app_message_file = AppMessageFile::CreateFromJson(app_message_node,
                [&](const std::string& file_path)
                {
                    // only the main message file is editable
                    const bool main_message_file = m_messageFiles.empty();
                    return load_text_source(file_path, main_message_file);
                });

            // don't add duplicate message files
            if( IsFilePathInUse(m_messageFiles, app_message_file.GetFilePath()) )
                continue;

            AddMessageFile(std::move(app_message_file));
        }

        catch( const CSProException& exception )
        {
            // don't abort on errors reading message files
            json_node.LogWarning("A message file could not be loaded and will be dropped: %s",
                                 exception.what());
        }
    }


    // reports
    // --------------------------------------------------------------------------
    for( const JsonNode& report_node : json_node.GetArrayOrEmpty(JK::reports) )
    {
        try
        {
            ReportFile report_file = ReportFile::CreateFromJson(report_node,
                [&](const std::string& file_path)
                {
                    // all reports are editable
                    return load_text_source(file_path, true);
                });

            // make sure the report name is valid and unique
            if( !check_name("report", report_file.GetName(), report_file.GetFilePath()) )
                continue;

            AddReport(std::move(report_file));
        }

        catch( const CSProException& exception )
        {
            // don't abort on errors reading report files
            json_node.LogWarning("A report file could not be loaded and will be dropped: %s",
                                 exception.what());
        }
    }


    // resources
    // --------------------------------------------------------------------------
    for( const JsonNode& resource_node : json_node.GetArrayOrEmpty(JK::resources) )
    {
        AppResource resource = resource_node.Get<AppResource>();

        // don't add duplicate resources
        if( GetResource(resource.GetPath()) != nullptr )
            continue;

        if( PortableFunctions::FileExists(resource.GetPath()) )
        {
            AddResource(std::move(resource));
        }

        else
        {
            // don't fail if the resource is not present, just don't add it
            json_node.LogWarning("The resource path was not found and will be dropped: %s",
                                 resource.GetPath().c_str());
        }
    }


    // logic settings
    // --------------------------------------------------------------------------
    if( json_node.Contains(JK::logicSettings) )
        m_logicSettings = json_node.Get<LogicSettings>(JK::logicSettings);


    // the properties node
    // --------------------------------------------------------------------------
    const JsonNode& properties_node = json_node.Get(JK::properties);

    if( entry_app )
    {
        m_askOperatorId = properties_node.GetOrDefault(JK::askOperatorId, m_askOperatorId);
        m_autoAdvanceOnSelection = properties_node.GetOrDefault(JK::autoAdvanceOnSelection, m_autoAdvanceOnSelection);

        ASSERT(!m_mappingOptions.IsDefined());
        if( properties_node.Contains(JK::caseListing) )
        {
            const JsonNode& case_listing_node = properties_node.Get(JK::caseListing);

            if( case_listing_node.Get<std::string_view>(JK::type) == JV::map )
            {
                m_mappingOptions = case_listing_node.Get<AppMappingOptions>();
            }

            else
            {
                case_listing_node.LogWarning("Case listings of type '%s' are not supported", case_listing_node.Get<std::string>(JK::type).c_str());
            }
        }

        m_caseTreeType = properties_node.GetOrDefault(JK::caseTree, m_caseTreeType);
        m_centerForms = properties_node.GetOrDefault(JK::centerForms, m_centerForms);
        m_createListingFile = properties_node.GetOrDefault(JK::createListing, m_createListingFile);
        m_createLogFile = properties_node.GetOrDefault(JK::createLog, m_createLogFile);
        m_decimalMarkIsComma = ( properties_node.GetOrDefault(JK::decimalMark, DecimalMark::Dot) == DecimalMark::Comma );
        m_displayCodesAlongsideLabels = properties_node.GetOrDefault(JK::displayCodesAlongsideLabels, m_displayCodesAlongsideLabels);

        // notes
        {
            const JsonNode& notes_node = properties_node.GetOrEmpty(JK::notes);
            // the edit permission is evaluated before delete because when delete is false, edit must be false
            SetEditNotePermissions(EditNotePermissions::EditOtherOperators, ( notes_node.GetOrDefault(JK::edit, NotePermission::All) == NotePermission::All ));
            SetEditNotePermissions(EditNotePermissions::DeleteOtherOperators, ( notes_node.GetOrDefault(JK::delete_, NotePermission::All) == NotePermission::All ));
        }

        // partialSave
        {
            const JsonNode& partial_save_node = properties_node.GetOrEmpty(JK::partialSave);
            m_partialSave = partial_save_node.GetOrDefault(JK::operatorEnabled, m_partialSave);
            SetAutoPartialSaveMinutes(partial_save_node.GetOrDefault(JK::autoSaveMinutes, m_autoPartialSaveMinutes));
        }

        m_showEndCaseMessage = properties_node.GetOrDefault(JK::showEndCaseMessage, m_showEndCaseMessage);
        m_comboBoxShowOnlyDiscreteValues = properties_node.GetOrDefault(JK::showOnlyDiscreteValuesInComboBoxes, m_comboBoxShowOnlyDiscreteValues);
        m_showFieldLabels = properties_node.GetOrDefault(JK::showFieldLabels, m_showFieldLabels);
        m_showErrorMessageNumbers = properties_node.GetOrDefault(JK::showErrorMessageNumbers, m_showErrorMessageNumbers);
        m_useQuestionText = properties_node.GetOrDefault(JK::showQuestionText, m_useQuestionText);
        m_showRefusals = properties_node.GetOrDefault(JK::showRefusals, m_showRefusals);

        if( properties_node.Contains(JK::sync) )
            m_syncParameters = properties_node.Get<AppSyncParameters>(JK::sync);

        // verify
        {
            const JsonNode& verify_node = properties_node.GetOrEmpty(JK::verify);
            m_verifyFrequency = verify_node.GetOrDefault(JK::frequency, m_verifyFrequency);

            if( m_verifyFrequency < 1 || m_verifyFrequency > GetVerifyFreqMax() )
            {
                verify_node.LogWarning("Verification frequencies must be between 1 and %d so '%d' is invalid", GetVerifyFreqMax(), m_verifyFrequency);
                m_verifyFrequency = 1;
            }

            const JsonNode& verify_start_node = verify_node.GetOrEmpty(JK::start);

            if( !verify_start_node.IsEmpty() )
            {
                if( verify_start_node.IsString() && verify_start_node.Get<std::string_view>() == JV::random )
                {
                    m_verifyStart = -1;
                }

                else
                {
                    m_verifyStart = verify_start_node.Get<int>();

                    if( m_verifyStart < 1 )
                    {
                        verify_start_node.LogWarning("The verification start position must be 1 or greater so '%d' is invalid", m_verifyFrequency);
                        m_verifyStart = 1;
                    }
                }
            }
        }
    }


    // additional properties, which can come from external files...
    // --------------------------------------------------------------------------
    if( properties_node.Contains(JK::import) )
    {
        std::vector<std::string> application_properties_file_paths = read_paths(properties_node.GetArrayOrEmpty(JK::import), false);

        for( std::string& application_properties_file_path : application_properties_file_paths )
        {
            if( !m_applicationPropertiesFilePath.empty() )
            {
                properties_node.LogWarning("Defining multiple application property files is not currently supported and these properties will be dropped: %s",
                                           application_properties_file_path.c_str());
                continue;
            }

            if( !check_file_is_regular_and_warn_if_not_exist("application properties", application_properties_file_path) )
                continue;

            m_applicationPropertiesFilePath = std::move(application_properties_file_path);

            if( load_text_sources_and_external_application_properties )
                m_applicationProperties->Open(m_applicationPropertiesFilePath, silent, message_logger);
        }
    }

    // ...and/or directly in the properties node
    m_applicationProperties->CreateFromJsonWorker(properties_node);
}


void Application::WriteJson(JsonWriter& json_writer, const bool write_to_new_json_object/* = true*/) const
{
    if( write_to_new_json_object )
        json_writer.BeginObject();

    json_writer.Write(JK::type, m_engineAppType)
               .Write(JK::name, m_name)
               .Write(JK::label, m_label);

    // writing routines
    const bool entry_app = ( m_engineAppType == EngineAppType::Entry );

    auto write_paths = [&](const char* const key, const std::vector<std::string>& paths)
    {
        if( !json_writer.Verbose() && paths.empty() )
            return;

        json_writer.BeginArray(key);

        for( const std::string& path : paths )
            json_writer.WriteRelativePath(path);

        json_writer.EndArray();
    };

    auto write_path_as_array = [&](const char* const key, const std::string& path)
    {
        if( !json_writer.Verbose() && path.empty() )
            return;

        json_writer.BeginArray(key);

        if( !path.empty() )
            json_writer.WriteRelativePath(path);

        json_writer.EndArray();
    };


    // dictionaries (first the input dictionary, then those with parent filenames, and then the rest)
    json_writer.BeginArray(JK::dictionaries);

#ifdef _DEBUG
    size_t dictionaries_written = 0;
#endif

    for( int i = 0; i < 3; ++i )
    {
        for( const DictionaryDescription& dictionary_description : m_dictionaryDescriptions )
        {
            const bool process =
                ( i == 0 ) ? ( dictionary_description.GetDictionaryType() == DictionaryType::Input ) :
                ( i == 1 ) ? ( dictionary_description.GetDictionaryType() != DictionaryType::Input && !dictionary_description.GetParentFilePath().empty() ) :
                             ( dictionary_description.GetDictionaryType() != DictionaryType::Input && dictionary_description.GetParentFilePath().empty() );

            if( process )
            {
                json_writer.Write(dictionary_description);
#ifdef _DEBUG
                ++dictionaries_written;
#endif
            }
        }
    }

    ASSERT(dictionaries_written == ( ( ( m_engineAppType == EngineAppType::Tabulation ) ? m_tableSpecFilePaths.size() : m_formFilePaths.size() ) + m_externalDictionaryFilePaths.size() ));

    json_writer.EndArray();


    // forms / orders / table specs
    if( entry_app || m_engineAppType == EngineAppType::Batch )
    {
        write_paths(entry_app ? JK::forms : JK::order, m_formFilePaths);
    }

    else if( m_engineAppType == EngineAppType::Tabulation )
    {
        write_paths(JK::tableSpecs, m_tableSpecFilePaths);
    }


    // write the question text file path as an array to match how other file paths are written
    if( entry_app )
        write_path_as_array(JK::questionText, m_questionTextFilePath);


    // code files
    if( json_writer.Verbose() || !m_codeFiles.empty() )
        json_writer.Write(JK::code, m_codeFiles);


    // message files
    if( json_writer.Verbose() || !m_messageFiles.empty() )
    {
        static_assert(Versioning::Number <= 8.1, "remove the conditional way of writing out message files in CSPro 8.2+");

        // CSPro 8.0 will choke on 8.1+ files with messages written as objects instead of strings,
        // so to prevent 8.1 files from being unreadable in 8.0, we'll temporarily write out messages
        // in the 8.0 format unless the user is using 8.1+ only features
        const auto& system_messages_lookup = std::find_if(m_messageFiles.cbegin(), m_messageFiles.cend(),
            [&](const AppMessageFile& app_message_file) { return ( app_message_file.GetType() == AppMessageFile::Type::System ); });

        if( system_messages_lookup == m_messageFiles.cend() )
        {
            // 8.0 format
            json_writer.BeginArray(JK::messages);

            for( const AppMessageFile& app_message_file : m_messageFiles )
                json_writer.WriteRelativePath(app_message_file.GetFilePath());

            json_writer.EndArray();
        }

        else
        {
            // 8.1+ format
            json_writer.Write(JK::messages, m_messageFiles);
        }
    }


    // reports
    if( json_writer.Verbose() || !m_reportFiles.empty() )
        json_writer.Write(JK::reports, m_reportFiles);


    // resources
    if( json_writer.Verbose() || !m_resources.empty() )
        json_writer.Write(JK::resources, m_resources);


    // logic settings
    json_writer.Write(JK::logicSettings, m_logicSettings);


    // the properties node
    json_writer.BeginObject(JK::properties);
    {
        // flags and other objects (written in alphabetical order)
        if( entry_app )
        {
            json_writer.Write(JK::askOperatorId, m_askOperatorId);
            json_writer.Write(JK::autoAdvanceOnSelection, m_autoAdvanceOnSelection);

            if( m_mappingOptions.IsDefined() )
            {
                json_writer.Key(JK::caseListing).WriteObject(
                    [&]()
                    {
                        json_writer.Write(JK::type, JV::map);
                        m_mappingOptions.WriteJson(json_writer, false);
                    });
            }

            json_writer.Write(JK::caseTree, m_caseTreeType);
            json_writer.Write(JK::centerForms, m_centerForms);
            json_writer.Write(JK::createListing, m_createListingFile);
            json_writer.Write(JK::createLog, m_createLogFile);
            json_writer.Write(JK::decimalMark, m_decimalMarkIsComma ? DecimalMark::Comma : DecimalMark::Dot);
            json_writer.Write(JK::displayCodesAlongsideLabels, m_displayCodesAlongsideLabels);

            json_writer.Key(JK::notes).WriteObject(
                [&]()
                {
                    json_writer.Write(JK::delete_, GetEditNotePermissions(EditNotePermissions::DeleteOtherOperators) ? NotePermission::All : NotePermission::Operator)
                               .Write(JK::edit, GetEditNotePermissions(EditNotePermissions::EditOtherOperators) ? NotePermission::All : NotePermission::Operator);
                });

            json_writer.Key(JK::partialSave).WriteObject(
                [&]()
                {
                    json_writer.Write(JK::operatorEnabled, m_partialSave);

                    if( GetAutoPartialSave() )
                        json_writer.Write(JK::autoSaveMinutes, m_autoPartialSaveMinutes);
                });

            json_writer.Write(JK::showEndCaseMessage, m_showEndCaseMessage);
            json_writer.Write(JK::showOnlyDiscreteValuesInComboBoxes, m_comboBoxShowOnlyDiscreteValues);
            json_writer.Write(JK::showFieldLabels, m_showFieldLabels);
            json_writer.Write(JK::showErrorMessageNumbers, m_showErrorMessageNumbers);
            json_writer.Write(JK::showQuestionText, m_useQuestionText);
            json_writer.Write(JK::showRefusals, m_showRefusals);

            if( json_writer.Verbose() || m_syncParameters.sync_connection_string.IsDefined() )
                json_writer.Write(JK::sync, m_syncParameters);

            json_writer.Key(JK::verify).WriteObject(
                [&]()
                {
                    json_writer.Write(JK::frequency, m_verifyFrequency);

                    ( m_verifyStart == -1 ) ? json_writer.Write(JK::start, JV::random) :
                                              json_writer.Write(JK::start, m_verifyStart);
                });
        }


        // additional properties
        // if no application properties file is specified, write the properties directly
        if( m_applicationPropertiesFilePath.empty() )
        {
            m_applicationProperties->WriteJson(json_writer, false, json_writer.Verbose());
        }

        // otherwise write the properties file as an array (to support a future scenario where multiple property files can be associated with an application)
        else
        {
            write_path_as_array(JK::import, m_applicationPropertiesFilePath);
        }
    }
    json_writer.EndObject();


    if( write_to_new_json_object )
        json_writer.EndObject();
}


void Application::serialize(Serializer& ar)
{
    constexpr int PENHeaderID = 20121109 + 19820605 + 19790404 + 19490117 + 19400129;
    int header_test = PENHeaderID;

    ar & header_test;
    ASSERT(header_test == PENHeaderID);

    if( ar.IsSaving() )
    {
        ar.Write(Versioning::Number);
    }

    else
    {
        m_version = ar.Read<double>();

        if( m_version > Versioning::Number )
            throw CSProException("This application was created using version %0.1f. You cannot run this file on this older version of CSPro (%0.1f).", m_version, Versioning::Number);

        if( ar.GetArchiveVersion() < Serializer::GetEarliestSupportedVersion() )
            throw CSProException("CSEntry %0.1f can no longer run applications created using old versions of CSPro (%0.1f).", Versioning::Number, m_version);

        m_serializerArchiveVersion = ar.GetArchiveVersion();
    }

    ar.IgnoreUnusedVariable<std::string>(Serializer::Iteration_8_0_000_1); // m_csVersion

    ar & m_label
       & m_name;

    if( ar.MeetsVersionIteration(Serializer::Iteration_8_0_000_1) )
    {
        ar.SerializeEnum(m_engineAppType);
    }

    else
    {
        const std::string app_type_string = ar.Read<std::string>();
        ASSERT(app_type_string == "DataEntry");
        m_engineAppType = EngineAppType::Entry;
    }

    ar & m_askOperatorId
       & m_showEndCaseMessage

       & m_partialSave
       & m_autoPartialSaveMinutes
       & m_useQuestionText;

    ar.SerializeEnum(m_caseTreeType);

    ar & m_verifyStart
       & m_verifyFrequency;

    ar.IgnoreUnusedVariable<std::string>(Serializer::Iteration_8_0_000_1); // m_sNote;

    ar & m_applicationProperties->UseHtmlDialogs
       & *m_applicationProperties;

    ar.SerializePaths(m_externalDictionaryFilePaths);

    ar & m_codeFiles
       & m_messageFiles;

    if( ar.MeetsVersionIteration(Serializer::Iteration_7_7_000_2) )
        ar & m_reportFiles;

    if( ar.MeetsVersionIteration(Serializer::Iteration_8_1_000_1) )
        ar & m_resources;

    ar.SerializePath(m_questionTextFilePath)
      .SerializePaths(m_formFilePaths);

    ar & m_dictionaryDescriptions
       & m_centerForms
       & m_decimalMarkIsComma
       & m_createListingFile
       & m_createLogFile
       & m_editNotePermissions
       & m_syncParameters
       & m_autoAdvanceOnSelection
       & m_displayCodesAlongsideLabels
       & m_showFieldLabels
       & m_showErrorMessageNumbers
       & m_hasWriteStatements
       & m_comboBoxShowOnlyDiscreteValues
       & m_showRefusals
       & m_mappingOptions;

    // APP_LOAD_TODO ... this value (as well as m_hasWriteStatements above) isn't
    // known until after the code is compiled, so this should only be serialized
    // after the whole application has been processed
    ar & m_hasSaveableFrequencyStatements
       & m_hasImputeStatements
       & m_hasImputeStatStatements
       & m_hasSaveArrays;

    if( ar.MeetsVersionIteration(Serializer::Iteration_8_0_000_1) )
        ar & m_logicSettings;
}
