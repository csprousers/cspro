#include "stdafx.h"
#include "Application.h"
#include "Properties/ApplicationProperties.h"
#include <zUtilO/AppLdr.h>
#include <zUtilO/ArrUtil.h>
#include <zUtilO/TextSourceEditable.h>
#include <zUtilO/TextSourceExternal.h>


const TCHAR* const ToString(EngineAppType engine_app_type)
{
    return ( engine_app_type == EngineAppType::Entry )      ? _T("entry") :
           ( engine_app_type == EngineAppType::Tabulation ) ? _T("tabulation") :
           ( engine_app_type == EngineAppType::Batch )      ? _T("batch") :
                                                              _T("invalid");
}


// --------------------------------------------------------------------------
// application properties
// --------------------------------------------------------------------------

Application::Application()
    :   m_version(CSPRO_VERSION_NUMBER),
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


Application::~Application()
{
}


void Application::SetApplicationProperties(ApplicationProperties application_properties)
{
    *m_applicationProperties = std::move(application_properties);
}


// --------------------------------------------------------------------------
// form files
// --------------------------------------------------------------------------

void Application::AddFormFilename(const CString& form_filename)
{
    ASSERT(GetEngineAppType() == EngineAppType::Entry || GetEngineAppType() == EngineAppType::Batch);
    m_formFilenames.emplace_back(form_filename);
}


void Application::AddFormFilename(std::wstring form_filename)
{
    AddFormFilename(WS2CS(form_filename));
}


void Application::DropFormFilename(wstring_view form_filename)
{
    for( auto itr = m_formFilenames.cbegin(); itr != m_formFilenames.cend(); ++itr )
    {
        if( SO::EqualsNoCase(form_filename, *itr) )
        {
            m_formFilenames.erase(itr);
            return;
        }
    }
}


void Application::RenameFormFilename(wstring_view original_form_filename, const CString& new_form_filename)
{
    for( CString& form_filename : m_formFilenames )
    {
        if( SO::EqualsNoCase(original_form_filename, form_filename) )
        {
            form_filename = new_form_filename;
            return;
        }
    }
}


// --------------------------------------------------------------------------
// tab specs
// --------------------------------------------------------------------------

void Application::AddTabSpecFilename(const CString& tab_spec_filename)
{
    ASSERT(GetEngineAppType() == EngineAppType::Tabulation);
    m_tabSpecFilenames.emplace_back(tab_spec_filename);
}


void Application::RenameTabSpecFilename(wstring_view original_tab_spec_filename, const CString& new_tab_spec_filename)
{
   for( CString& tab_spec_filename : m_tabSpecFilenames )
    {
        if( SO::EqualsNoCase(original_tab_spec_filename, tab_spec_filename) )
        {
            tab_spec_filename = new_tab_spec_filename;
            return;
        }
    }
}


// --------------------------------------------------------------------------
// external dictionaries
// --------------------------------------------------------------------------

void Application::AddExternalDictionaryFilename(const CString& dictionary_filename)
{
    m_externalDictionaryFilenames.emplace_back(dictionary_filename);
}


void Application::DropExternalDictionaryFilename(wstring_view dictionary_filename)
{
    for( auto itr = m_externalDictionaryFilenames.cbegin(); itr != m_externalDictionaryFilenames.cend(); ++itr )
    {
        if( SO::EqualsNoCase(dictionary_filename, *itr) )
        {
            m_externalDictionaryFilenames.erase(itr);
            return;
        }
    }
}


void Application::RenameExternalDictionaryFilename(wstring_view original_dictionary_filename, std::wstring new_dictionary_filename)
{
    for( CString& dictionary_filename : m_externalDictionaryFilenames )
    {
        if( SO::EqualsNoCase(original_dictionary_filename, dictionary_filename) )
        {
            dictionary_filename = WS2CS(new_dictionary_filename);
            break;
        }
    }

    for( DictionaryDescription& dictionary_description : m_dictionaryDescriptions )
    {
        if( SO::EqualsNoCase(original_dictionary_filename, dictionary_description.GetDictionaryFilename()) )
        {
            dictionary_description.SetDictionaryFilename(std::move(new_dictionary_filename));
            return;
        }
    }
}


// --------------------------------------------------------------------------
// code files
// --------------------------------------------------------------------------

const CodeFile* Application::GetLogicMainCodeFile() const
{
    const auto& search = std::find_if(m_codeFiles.cbegin(), m_codeFiles.cend(),
                                      [&](const CodeFile& code_file) { return code_file.IsLogicMain(); });

    return ( search != m_codeFiles.cend() ) ? &*search : nullptr;
}


CodeFile* Application::GetLogicMainCodeFile()
{
    return const_cast<CodeFile*>(const_cast<const Application*>(this)->GetLogicMainCodeFile());
}


void Application::AddCodeFile(CodeFile code_file)
{
    ASSERT(!IsFilenameInUse(m_codeFiles, code_file.GetFilename()));
    ASSERT(!code_file.IsLogicMain() || GetLogicMainCodeFile() == nullptr);

    m_codeFiles.emplace_back(std::move(code_file));
}


void Application::DropCodeFile(size_t index)
{
    ASSERT(index < m_codeFiles.size());
    m_codeFiles.erase(m_codeFiles.begin() + index);
}



// --------------------------------------------------------------------------
// message files
// --------------------------------------------------------------------------

void Application::AddMessageFile(std::shared_ptr<TextSource> message_text_source)
{
    ASSERT(message_text_source != nullptr && !IsFilenameInUse(m_messageTextSources, message_text_source->GetFilename()));
    m_messageTextSources.emplace_back(std::move(message_text_source));
}


void Application::DropMessageFile(size_t index)
{
    ASSERT(index < m_messageTextSources.size());
    m_messageTextSources.erase(m_messageTextSources.begin() + index);
}



// --------------------------------------------------------------------------
// reports
// --------------------------------------------------------------------------

const NamedTextSource* Application::GetReportNamedTextSource(wstring_view name_or_filename, bool search_by_name) const
{
    const auto& search = std::find_if(m_reportNamedTextSources.cbegin(), m_reportNamedTextSources.cend(),
        [&](const auto& rnts)
        {
            return search_by_name ? SO::EqualsNoCase(name_or_filename, rnts->name) :
                                    SO::EqualsNoCase(name_or_filename, rnts->text_source->GetFilename());
        });

    return ( search != m_reportNamedTextSources.cend() ) ? search->get() : nullptr;
}


void Application::AddReport(std::wstring name, std::shared_ptr<TextSource> report_text_source)
{
    ASSERT(report_text_source != nullptr && GetReportNamedTextSource(name, true) == nullptr);
    m_reportNamedTextSources.emplace_back(std::make_shared<NamedTextSource>(NamedTextSource { std::move(name), std::move(report_text_source) }));
}


void Application::AddReport(std::wstring name, std::wstring filename)
{
    AddReport(std::move(name), TextSourceEditable::FindOpenOrCreate(std::move(filename)));
}


void Application::DropReport(size_t index)
{
    ASSERT(index < m_reportNamedTextSources.size());
    m_reportNamedTextSources.erase(m_reportNamedTextSources.begin() + index);
}


// --------------------------------------------------------------------------
// resource folders
// --------------------------------------------------------------------------

void Application::AddResourceFolder(const CString& folder)
{
    if( !ContainsStringInVectorNoCase(m_resourceFolders, folder) )
        m_resourceFolders.emplace_back(folder);
}


void Application::DropResourceFolder(size_t index)
{
    ASSERT(index < m_resourceFolders.size());
    m_resourceFolders.erase(m_resourceFolders.begin() + index);
}


// --------------------------------------------------------------------------
// dictionary descriptions
// --------------------------------------------------------------------------

DictionaryType Application::GetDictionaryType(const CDataDict& dictionary) const
{
    const auto& dictionary_description_search = std::find_if(m_dictionaryDescriptions.cbegin(), m_dictionaryDescriptions.cend(),
        [&](const DictionaryDescription& dictionary_description)
        {
            return ( dictionary_description.GetDictionary() == &dictionary );
        });

    return ( dictionary_description_search == m_dictionaryDescriptions.cend() ) ? DictionaryType::Unknown :
                                                                                  (*dictionary_description_search).GetDictionaryType();
}


const DictionaryDescription* Application::GetDictionaryDescription(wstring_view dictionary_filename, wstring_view parent_filename/* = wstring_view()*/) const
{
    const auto& dictionary_description_search = std::find_if(m_dictionaryDescriptions.cbegin(), m_dictionaryDescriptions.cend(),
        [&](const DictionaryDescription& dictionary_description)
        {
            return ( SO::EqualsNoCase(dictionary_description.GetDictionaryFilename(), dictionary_filename) &&
                     SO::EqualsNoCase(dictionary_description.GetParentFilename(), parent_filename) );
        });

    return ( dictionary_description_search == m_dictionaryDescriptions.cend() ) ? nullptr :
                                                                                  &(*dictionary_description_search);
}


DictionaryDescription* Application::GetDictionaryDescription(wstring_view dictionary_filename, wstring_view parent_filename/* = wstring_view()*/)
{
    return const_cast<DictionaryDescription*>(const_cast<const Application*>(this)->GetDictionaryDescription(dictionary_filename, parent_filename));
}


const std::wstring& Application::GetFirstDictionaryFilenameOfType(DictionaryType dictionary_type) const
{
    const auto& dictionary_description_search = std::find_if(m_dictionaryDescriptions.cbegin(), m_dictionaryDescriptions.cend(),
        [&](const DictionaryDescription& dictionary_description)
        {
            return ( dictionary_description.GetDictionaryType() == dictionary_type );
        });

    return ( dictionary_description_search == m_dictionaryDescriptions.cend() ) ? SO::EmptyString :
                                                                                  dictionary_description_search->GetDictionaryFilename();
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

bool Application::IsNameUnique(const std::wstring& name) const
{
    // search reports
    if( GetReportNamedTextSource(name, true) != nullptr )
        return false;

    // search code namespaces
    for( const CodeFile& code_file : m_codeFiles )
    {
        if( SO::EqualsNoCase(name, code_file.GetNamespaceName()) )
            return false;
    }

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
    { CaseTreeType::Never,       _T("off") },
    { CaseTreeType::MobileOnly,  _T("mobileOnly") },
    { CaseTreeType::DesktopOnly, _T("desktopOnly") },
    { CaseTreeType::Always,      _T("on") })

// ideally these enums would be used by the class instead of bools, but
// that refactoring can be done at a later point
enum class DecimalMark { Dot, Comma };
enum class NotePermission { Operator, All };

CREATE_ENUM_JSON_SERIALIZER(DecimalMark,
    { DecimalMark::Dot,   _T("dot") },
    { DecimalMark::Comma, _T("comma") })

CREATE_ENUM_JSON_SERIALIZER(NotePermission,
    { NotePermission::Operator, _T("operator") },
    { NotePermission::All,      _T("all") })


void Application::Open(NullTerminatedString filename, bool silent/* = false*/, bool load_text_sources_and_external_application_properties/* = true*/)
{
    auto json_reader = JsonSpecFile::CreateReader(filename, nullptr, [&]() { return ConvertPre80SpecFile(filename); });

    try
    {
        m_version = json_reader->CheckVersion();
        json_reader->CheckFileType(JV::application);

        CreateFromJsonWorker(*json_reader, load_text_sources_and_external_application_properties, silent, json_reader->GetSharedMessageLogger());

        m_applicationFilename = CString(filename.c_str());
    }

    catch( const CSProException& exception )
    {
        json_reader->GetMessageLogger().RethrowException(filename, exception);
    }

    // report any warnings
    json_reader->GetMessageLogger().DisplayWarnings(silent);
}


void Application::Save(NullTerminatedString filename, bool continue_using_filename/* = true*/) const
{
    auto json_writer = JsonSpecFile::CreateWriter(filename, JV::application);

    WriteJson(*json_writer, false);

    json_writer->EndObject();

    json_writer->Close();

    if( continue_using_filename )
        const_cast<Application*>(this)->m_applicationFilename = CString(filename.c_str());
}


Application Application::CreateFromJson(const JsonNode<wchar_t>& json_node)
{
    Application application;
    application.CreateFromJsonWorker(json_node, false, true, nullptr);
    return application;
}


void Application::CreateFromJsonWorker(const JsonNode<wchar_t>& json_node, bool load_text_sources_and_external_application_properties,
                                       bool silent, std::shared_ptr<JsonSpecFile::ReaderMessageLogger> message_logger)
{
    m_engineAppType = json_node.Get<EngineAppType>(JK::type);
    bool entry_app = ( m_engineAppType == EngineAppType::Entry );

    m_name = json_node.Get<CString>(JK::name);
    m_name.MakeUpper();

    m_label = json_node.GetOrDefault(JK::label, SO::EmptyCString);

    // reading routines
    // --------------------------------------------------------------------------
    auto read_path = [&](const auto& path_node, bool throw_exception_if_file_is_not_regular)
    {
        std::wstring path = path_node.GetAbsolutePath();

        if( throw_exception_if_file_is_not_regular && !PortableFunctions::FileIsRegular(path) )
            throw ApplicationFileNotFoundException(path);

        return WS2CS(path);
    };

    auto check_file_is_regular_and_warn_if_not_exist = [&](const TCHAR* file_type, NullTerminatedString filename)
    {
        if( PortableFunctions::FileIsRegular(filename) )
            return true;

        json_node.LogWarning(_T("The %s file was not found and will be dropped: %s"), file_type, filename.c_str());
        return false;
    };

    auto read_filename_array = [&](const auto& filenames_array_node, std::vector<CString>& filenames, bool throw_exception_if_file_is_not_regular)
    {
        for( const auto& filename_node : filenames_array_node )
            filenames.emplace_back(read_path(filename_node, throw_exception_if_file_is_not_regular));
    };

    auto load_text_source = [&](std::wstring filename, bool text_source_is_editable)
    {
        std::shared_ptr<TextSource> text_source;

        if( text_source_is_editable && load_text_sources_and_external_application_properties )
        {
            text_source = TextSourceEditable::FindOpenOrCreate(std::move(filename));
        }

        else
        {
            text_source = std::make_shared<TextSourceExternal>(std::move(filename));
        }

        return text_source;
    };

    auto check_name = [&](const TCHAR* file_type, const std::wstring& name, const std::wstring& filename)
    {
        if( !CIMSAString::IsName(name) || CIMSAString::IsReservedWord(name) )
        {
            json_node.LogWarning(_T("The %s name '%s' is not valid and the %s will not be loaded."), file_type, name.c_str(), file_type);
            return false;
        }

        if( !IsNameUnique(name) )
        {
            json_node.LogWarning(_T("The %s name '%s' is already in use so the %s '%s' will not be loaded."),
                                 file_type, name.c_str(), file_type, PortableFunctions::PathGetFilename(filename));
            return false;
        }

        return true;
    };


    // forms/orders/tab specs
    // --------------------------------------------------------------------------
    if( entry_app || m_engineAppType == EngineAppType::Batch )
    {
        read_filename_array(json_node.GetArrayOrEmpty(entry_app ? JK::forms : JK::order), m_formFilenames, true);
    }

    else if( m_engineAppType == EngineAppType::Tabulation )
    {
        read_filename_array(json_node.GetArrayOrEmpty(JK::tableSpecs), m_tabSpecFilenames, true);
    }

    const std::vector<CString>& form_or_tab_spec_filenames = ( m_engineAppType == EngineAppType::Tabulation ) ? m_tabSpecFilenames :
                                                                                                                m_formFilenames;

    if( form_or_tab_spec_filenames.empty() )
    {
        throw CSProException(_T("A '%s' application must have at least one %s"), ToString(m_engineAppType),
                             entry_app                                        ? _T("form") :
                             ( m_engineAppType == EngineAppType::Tabulation ) ? _T("tab spec") :
                                                                                _T("order"));
    }


    // dictionaries
    // --------------------------------------------------------------------------
    for( const auto& dictionary_node : json_node.GetArrayOrEmpty(JK::dictionaries) )
    {
        DictionaryDescription dictionary_description = dictionary_node.Get<DictionaryDescription>();

        if( !check_file_is_regular_and_warn_if_not_exist(_T("dictionary"), dictionary_description.GetDictionaryFilename()) )
            continue;

        if( !dictionary_description.GetParentFilename().empty() && !ContainsStringInVectorNoCase(form_or_tab_spec_filenames, dictionary_description.GetParentFilename()) )
        {
            json_node.LogWarning(_T("The dictionary's parent '%s' was not valid and will be reset: %s"),
                                 GetRelativeFNameForDisplay(dictionary_description.GetDictionaryFilename(), dictionary_description.GetParentFilename()).c_str(),
                                 dictionary_description.GetDictionaryFilename().c_str());

            dictionary_description.SetParentFilename(std::wstring());
        }

        // if this dictionary description does not have a parent, add it as an external dictionary
        if( dictionary_description.GetParentFilename().empty() )
            m_externalDictionaryFilenames.emplace_back(WS2CS(dictionary_description.GetDictionaryFilename()));

        m_dictionaryDescriptions.emplace_back(std::move(dictionary_description));
    }


    // question text
    // --------------------------------------------------------------------------
    if( entry_app )
    {
        std::vector<CString> question_text_filenames;
        read_filename_array(json_node.GetArrayOrEmpty(JK::questionText), question_text_filenames, true);

        if( question_text_filenames.size() != 1 )
            throw CSProException(_T("A '%s' application must have one question text file"), ToString(m_engineAppType));

        m_questionTextFilename = question_text_filenames.front();
    }


    // code files
    // --------------------------------------------------------------------------
    for( const auto& code_file_node : json_node.GetArrayOrEmpty(JK::code) )
    {
        try
        {
            CodeFile code_file = CodeFile::CreateFromJson(code_file_node,
                [&](const std::wstring& filename)
                {
                    // all code files are editable
                    return load_text_source(filename, true);
                });

            // make sure the namespace name is valid and unique (is defined)
            if( !code_file.GetNamespaceName().empty() && !check_name(_T("logic"), code_file.GetNamespaceName(), code_file.GetFilename()) )
                continue;

            // don't add duplicate code files (even if the namespace name differs)
            if( IsFilenameInUse(m_codeFiles, code_file.GetFilename()) )
                continue;

            m_codeFiles.emplace_back(std::move(code_file));
        }

        catch( const CSProException& exception )
        {
            // don't abort on errors reading code files
            json_node.LogWarning(exception.GetErrorMessage().c_str());
        }
    }
        

    // message files
    // --------------------------------------------------------------------------
    for( const auto& filename_node : json_node.GetArrayOrEmpty(JK::messages) )
    {
        // only require that the first message file exists
        bool main_message_file = m_messageTextSources.empty();
        std::wstring filename = CS2WS(read_path(filename_node, main_message_file));

        // don't add duplicate message files
        if( IsFilenameInUse(m_messageTextSources, filename) )
            continue;

        if( !main_message_file && !check_file_is_regular_and_warn_if_not_exist(_T("message"), filename) )
            continue;

        try
        {
            // only the first message file is editable
            m_messageTextSources.emplace_back(load_text_source(std::move(filename), main_message_file));
        }

        catch( const CSProException& exception )
        {
            // don't abort on errors reading message files
            json_node.LogWarning(exception.GetErrorMessage().c_str());
        }
    }


    // reports
    // --------------------------------------------------------------------------
    for( const auto& report_node : json_node.GetArrayOrEmpty(JK::reports) )
    {
        const std::wstring name = SO::ToUpper(report_node.Get<wstring_view>(JK::name));
        std::wstring filename = CS2WS(read_path(report_node.Get(report_node.Contains(JK::filename) ? JK::filename : JK::path), false));

        // make sure the report name is valid and unique
        if( !check_name(_T("report"), name, filename) )
            continue;

        try
        {
            AddReport(name, load_text_source(std::move(filename), true));
        }

        catch( const CSProException& exception )
        {
            json_node.LogWarning(_T("The report '%s' could not be loaded and will be dropped: %s"),
                                 name.c_str(), exception.GetErrorMessage().c_str());
        }
    }


    // resource folders
    // --------------------------------------------------------------------------
    for( const auto& resource_node : json_node.GetArrayOrEmpty(JK::resources) )
    {
        CString resource_folder = read_path(resource_node, false);
        resource_folder.TrimRight(PATH_CHAR);

        if( PortableFunctions::FileIsDirectory(resource_folder) )
        {
            AddResourceFolder(resource_folder);
        }

        else
        {
            // don't fail if the resource folder is not present, just don't add it
            json_node.LogWarning(_T("The resource folder was not found and will be dropped: %s"), resource_folder.GetString());
        }
    }


    // logic settings
    // --------------------------------------------------------------------------
    if( json_node.Contains(JK::logicSettings) )
        m_logicSettings = json_node.Get<LogicSettings>(JK::logicSettings);


    // the properties node
    // --------------------------------------------------------------------------
    const auto& properties_node = json_node.Get(JK::properties);

    if( entry_app )
    {
        m_askOperatorId = properties_node.GetOrDefault(JK::askOperatorId, m_askOperatorId);
        m_autoAdvanceOnSelection = properties_node.GetOrDefault(JK::autoAdvanceOnSelection, m_autoAdvanceOnSelection);

        ASSERT(!m_mappingOptions.IsDefined());
        if( properties_node.Contains(JK::caseListing) )
        {
            const auto& case_listing_node = properties_node.Get(JK::caseListing);

            if( case_listing_node.Get<std::wstring_view>(JK::type) == JV::map )
            {
                m_mappingOptions = case_listing_node.Get<AppMappingOptions>();
            }

            else
            {
                case_listing_node.LogWarning(_T("Case listings of type '%s' are not supported"), case_listing_node.Get<std::wstring>(JK::type).c_str());
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
            const auto& notes_node = properties_node.GetOrEmpty(JK::notes);
            // the edit permission is evaluated before delete because when delete is false, edit must be false
            SetEditNotePermissions(EditNotePermissions::EditOtherOperators, ( notes_node.GetOrDefault(JK::edit, NotePermission::All) == NotePermission::All ));
            SetEditNotePermissions(EditNotePermissions::DeleteOtherOperators, ( notes_node.GetOrDefault(JK::delete_, NotePermission::All) == NotePermission::All ));
        }

        // partialSave
        {
            const auto& partial_save_node = properties_node.GetOrEmpty(JK::partialSave);
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
            const auto& verify_node = properties_node.GetOrEmpty(JK::verify);
            m_verifyFrequency = verify_node.GetOrDefault(JK::frequency, m_verifyFrequency);

            if( m_verifyFrequency < 1 || m_verifyFrequency > GetVerifyFreqMax() )
            {
                verify_node.LogWarning(_T("Verification frequencies must be between 1 and %d so '%d' is invalid"), GetVerifyFreqMax(), m_verifyFrequency);
                m_verifyFrequency = 1;
            }

            const auto& verify_start_node = verify_node.GetOrEmpty(JK::start);

            if( !verify_start_node.IsEmpty() )
            {
                if( verify_start_node.IsString() && verify_start_node.Get<std::wstring_view>() == JV::random )
                {
                    m_verifyStart = -1;
                }

                else
                {
                    m_verifyStart = verify_start_node.Get<int>();

                    if( m_verifyStart < 1 )
                    {
                        verify_start_node.LogWarning(_T("The verification start position must be 1 or greater so '%d' is invalid"), m_verifyFrequency);
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
        std::vector<CString> application_properties_filenames;
        read_filename_array(properties_node.GetArrayOrEmpty(JK::import), application_properties_filenames, false);

        for( const CString& application_properties_filename : application_properties_filenames )
        {
            if( !m_applicationPropertiesFilename.empty() )
            {
                properties_node.LogWarning(_T("Defining multiple application property files is not currently supported and these properties will be dropped: %s"),
                                           application_properties_filename.GetString());
                continue;
            }

            if( !check_file_is_regular_and_warn_if_not_exist(_T("application properties"), application_properties_filename) )
                continue;

            m_applicationPropertiesFilename = CS2WS(application_properties_filename);

            if( load_text_sources_and_external_application_properties )
                m_applicationProperties->Open(m_applicationPropertiesFilename, silent, message_logger);
        }
    }

    // ...and/or directly in the properties node
    m_applicationProperties->CreateFromJsonWorker(properties_node);
}


void Application::WriteJson(JsonWriter& json_writer, bool write_to_new_json_object/* = true*/) const
{
    if( write_to_new_json_object )
        json_writer.BeginObject();

    json_writer.Write(JK::type, m_engineAppType)
               .Write(JK::name, m_name)
               .Write(JK::label, m_label);

    // writing routines
    bool entry_app = ( m_engineAppType == EngineAppType::Entry );

    auto write_filename_array = [&](const TCHAR* key, const std::vector<CString>& filenames)
    {
        if( !json_writer.Verbose() && filenames.empty() )
            return;

        json_writer.BeginArray(key);

        for( const CString& filename : filenames )
            json_writer.WriteRelativePath(CS2WS(filename));

        json_writer.EndArray();
    };

    auto write_filename_as_array = [&](const TCHAR* key, const CString& filename)
    {
        if( !json_writer.Verbose() && filename.IsEmpty() )
            return;

        json_writer.BeginArray(key);

        if( !filename.IsEmpty() )
            json_writer.WriteRelativePath(CS2WS(filename));

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
            bool process = ( i == 0 ) ? ( dictionary_description.GetDictionaryType() == DictionaryType::Input ) :
                           ( i == 1 ) ? ( dictionary_description.GetDictionaryType() != DictionaryType::Input && !dictionary_description.GetParentFilename().empty() ) :
                                        ( dictionary_description.GetDictionaryType() != DictionaryType::Input && dictionary_description.GetParentFilename().empty() );

            if( process )
            {
                json_writer.Write(dictionary_description);
#ifdef _DEBUG
                ++dictionaries_written;
#endif
            }
        }
    }

    ASSERT(dictionaries_written == ( ( ( m_engineAppType == EngineAppType::Tabulation ) ? m_tabSpecFilenames.size() : m_formFilenames.size() ) + m_externalDictionaryFilenames.size() ));

    json_writer.EndArray();


    // forms/orders/tab specs
    if( entry_app || m_engineAppType == EngineAppType::Batch )
    {
        write_filename_array(entry_app ? JK::forms : JK::order, m_formFilenames);
    }

    else if( m_engineAppType == EngineAppType::Tabulation )
    {
        write_filename_array(JK::tableSpecs, m_tabSpecFilenames);
    }


    // write the question text filename as an array to match how other filenames are written
    if( entry_app )
        write_filename_as_array(JK::questionText, m_questionTextFilename);


    // code files
    if( json_writer.Verbose() || !m_codeFiles.empty() )
        json_writer.Write(JK::code, m_codeFiles);


    // message files
    if( json_writer.Verbose() || !m_messageTextSources.empty() )
    {
        json_writer.BeginArray(JK::messages);

        for( const TextSource& text_source : VI_V(m_messageTextSources) )
            json_writer.WriteRelativePath(text_source.GetFilename());

        json_writer.EndArray();
    }


    // reports
    if( json_writer.Verbose() || !m_reportNamedTextSources.empty() )
    {
        json_writer.WriteObjects(JK::reports, m_reportNamedTextSources,
            [&](const auto& report_named_text_source)
            {
                json_writer.Write(JK::name, report_named_text_source->name)
                           .WriteRelativePath(JK::path, report_named_text_source->text_source->GetFilename());
            });
    }


    // resource folders
    write_filename_array(JK::resources, m_resourceFolders);


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

            if( !m_syncParameters.server.empty() )
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
        // if no application properties filename is specified, write the properties directly
        if( m_applicationPropertiesFilename.empty() )
        {
            m_applicationProperties->WriteJson(json_writer, false, json_writer.Verbose());
        }

        // otherwise write the properties filename as an array (to support a future scenario where multiple property files can be associated with an application)
        else
        {
            write_filename_as_array(JK::import, WS2CS(m_applicationPropertiesFilename));
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
        ar.Write(CSPRO_VERSION_NUMBER);
    }

    else
    {
        m_version = ar.Read<double>();

        if( m_version > CSPRO_VERSION_NUMBER )
            throw CSProException(_T("This application was created using version %0.1f. You cannot run this file on this older version of CSPro (%0.1f)."), m_version, CSPRO_VERSION_NUMBER);

        if( ar.GetArchiveVersion() < Serializer::GetEarliestSupportedVersion() )
            throw CSProException(_T("CSEntry %0.1f can no longer run applications created using old versions of CSPro (%0.1f)."), CSPRO_VERSION_NUMBER, m_version);

        m_serializerArchiveVersion = ar.GetArchiveVersion();
    }

    ar.IgnoreUnusedVariable<CString>(Serializer::Iteration_8_0_000_1); // m_csVersion

    ar & m_label
       & m_name;

    if( ar.MeetsVersionIteration(Serializer::Iteration_8_0_000_1) )
    {
        ar.SerializeEnum(m_engineAppType);
    }

    else
    {
        CString app_type_string = ar.Read<CString>();
        ASSERT(app_type_string == _T("DataEntry"));
        m_engineAppType = EngineAppType::Entry;
    }

    ar & m_askOperatorId
       & m_showEndCaseMessage

       & m_partialSave
       & m_autoPartialSaveMinutes
       & m_useQuestionText;

    ar.IgnoreUnusedVariable<CString>(Serializer::Iteration_7_6_000_1); // Capi fonts
    ar.IgnoreUnusedVariable<CString>(Serializer::Iteration_7_6_000_1);
    ar.IgnoreUnusedVariable<COLORREF>(Serializer::Iteration_7_6_000_1);
    ar.IgnoreUnusedVariable<COLORREF>(Serializer::Iteration_7_6_000_1);

    if( ar.MeetsVersionIteration(Serializer::Iteration_7_7_000_1) )
    {
        ar.SerializeEnum(m_caseTreeType);
    }

    else
    {
        m_caseTreeType = (CaseTreeType)ar.Read<char>();
    }

    ar & m_verifyStart
       & m_verifyFrequency;

    ar.IgnoreUnusedVariable<CString>(Serializer::Iteration_8_0_000_1); // m_sNote;

    if( ar.MeetsVersionIteration(Serializer::Iteration_7_7_000_1) )
    {
        ar & m_applicationProperties->UseHtmlDialogs;
        ar & *m_applicationProperties;
    }

    else
    {
        m_applicationProperties->UseHtmlDialogs = false;
    }

    ar.SerializeFilenameArray(m_externalDictionaryFilenames);

    if( ar.PredatesVersionIteration(Serializer::Iteration_7_6_000_1) )
    {
        std::wstring filename;

        ar.SerializeFilename(filename);
        m_codeFiles.emplace_back(CodeType::LogicMain, std::make_shared<TextSource>(filename));

        ar.SerializeFilename(filename);
        m_messageTextSources.emplace_back(std::make_shared<TextSource>(filename));
    }

    else
    {
        ar & m_codeFiles
           & m_messageTextSources;
    }

    if( ar.MeetsVersionIteration(Serializer::Iteration_7_7_000_2) )
        ar & m_reportNamedTextSources;

    ar.SerializeFilename(m_questionTextFilename)
      .SerializeFilenameArray(m_formFilenames);

    ar & m_dictionaryDescriptions
       & m_centerForms
       & m_decimalMarkIsComma

       & m_createListingFile
       & m_createLogFile
       & m_editNotePermissions;

    ar & m_syncParameters;

    ar & m_autoAdvanceOnSelection
       & m_displayCodesAlongsideLabels

       & m_showFieldLabels
       & m_showErrorMessageNumbers;

    if( ar.PredatesVersionIteration(Serializer::Iteration_7_7_000_1) )
    {
        auto& paradata_properties = m_applicationProperties->GetParadataProperties();

        ParadataProperties::CollectionType collection_type;
        ar.SerializeEnum(collection_type);
        paradata_properties.SetCollectionType(collection_type);

        paradata_properties.SetRecordIteratorLoadCases(ar.Read<bool>());
        paradata_properties.SetRecordValues(ar.Read<bool>());
        paradata_properties.SetDeviceStateIntervalMinutes(ar.Read<int>());
        paradata_properties.SetEventNames(ar.Read<std::set<std::wstring>>());
        paradata_properties.SetRecordCoordinates(ar.Read<bool>());
        paradata_properties.SetGpsLocationIntervalMinutes(ar.Read<int>());
        paradata_properties.SetRecordInitialPropertyValues(ar.Read<bool>());
    }

    ar & m_hasWriteStatements
       & m_comboBoxShowOnlyDiscreteValues;

    ar & m_showRefusals;

    ar & m_mappingOptions;

    if( ar.MeetsVersionIteration(Serializer::Iteration_7_6_000_1) )
    {
        // APP_LOAD_TODO ... this value (as well as m_hasWriteStatements above) isn't
        // known until after the code is compiled, so this should only be serialized
        // after the whole application has been processed
        ar & m_hasSaveableFrequencyStatements
           & m_hasImputeStatements
           & m_hasImputeStatStatements
           & m_hasSaveArrays;
    }

    if( ar.MeetsVersionIteration(Serializer::Iteration_8_0_000_1) )
        ar & m_logicSettings;
}
