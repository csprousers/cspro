#include "StdAfx.h"
#include "LanguageSettings.h"
#include "LanguageSettingsPersister.h"
#include <zToolsO/DirectoryLister.h>
#include <zJson/JsonSpecFile.h>
#include <zAppO/Application.h>


namespace
{
    bool UseLogicVersionV8(const LogicSettings& logic_settings)
    {
        return ( logic_settings.GetVersion() >= LogicSettings::Version::V8_0 );
    }


    LanguageSettingsPersister& GetLanguageSettingsPersister()
    {
        return assert_cast<CMainFrame*>(AfxGetMainWnd())->GetLanguageSettingsPersister();
    }
}



// --------------------------------------------------------------------------
// LanguageJsonSpecFile
// --------------------------------------------------------------------------

const std::vector<std::tuple<unsigned, std::wstring>> LanguageJsonSpecFile::m_submenuOptions = // JSON_TODO make sure that all spec files all listed here
{
    { ID_RUN_CSPRO_SPEC_FILE_APP,     _T("Application (.ent / .bch / .xtb)")  },
    { ID_RUN_CSPRO_SPEC_FILE_CSPROPS, _T("Application Properties (.csprops)") },
    { ID_RUN_CSPRO_SPEC_FILE_CMP,     _T("Compare Data (.cmp)")               },
    { ID_RUN_CSPRO_SPEC_FILE_CSDS,    _T("Deploy Application (.csds)")        },
    { ID_RUN_CSPRO_SPEC_FILE_DCF,     _T("Dictionary (.dcf)")                 },
    { ID_RUN_CSPRO_SPEC_FILE_XL2CS,   _T("Excel to CSPro (.xl2cs)")           },
    { ID_RUN_CSPRO_SPEC_FILE_EXF,     _T("Export Data (.exf)")                },
    { ID_RUN_CSPRO_SPEC_FILE_CSPACK,  _T("Pack Application (.cspack)")        },
    { ID_RUN_CSPRO_SPEC_FILE_SSF,     _T("Sort Data (.ssf)")                  },
    { ID_RUN_CSPRO_SPEC_FILE_FQF,     _T("Tabulate Frequencies (.fqf)")       },
};


std::optional<unsigned> LanguageJsonSpecFile::GetIndexFromFilename(const std::wstring& filename)
{
    static const std::vector<std::tuple<unsigned, std::vector<const TCHAR*>>> SpecFileExtensions =
    {
        { ID_RUN_CSPRO_SPEC_FILE_APP,     { FileExtensions::EntryApplication, FileExtensions::BatchApplication, FileExtensions::TabulationApplication } },
        { ID_RUN_CSPRO_SPEC_FILE_CSPROPS, { FileExtensions::ApplicationProperties                                                                     } },
        { ID_RUN_CSPRO_SPEC_FILE_CMP,     { FileExtensions::CompareSpec                                                                               } },
        { ID_RUN_CSPRO_SPEC_FILE_CSDS,    { FileExtensions::DeploySpec                                                                                } },
        { ID_RUN_CSPRO_SPEC_FILE_DCF,     { FileExtensions::Dictionary                                                                                } },
        { ID_RUN_CSPRO_SPEC_FILE_XL2CS,   { FileExtensions::ExcelToCSProSpec                                                                          } },
        { ID_RUN_CSPRO_SPEC_FILE_EXF,     { FileExtensions::ExportSpec                                                                                } },
        { ID_RUN_CSPRO_SPEC_FILE_CSPACK,  { FileExtensions::PackSpec                                                                                  } },
        { ID_RUN_CSPRO_SPEC_FILE_SSF,     { FileExtensions::SortSpec                                                                                  } },
        { ID_RUN_CSPRO_SPEC_FILE_FQF,     { FileExtensions::FrequencySpec                                                                             } },
    };

    const std::wstring extension = PortableFunctions::PathGetFileExtension(filename);

    for( const auto& [index, these_extensions] : SpecFileExtensions )
    {
        for( const TCHAR* this_extension : these_extensions )
        {
            if( SO::EqualsNoCase(extension, this_extension) )
                return index;
        }
    }

    return std::nullopt;
}


const std::wstring& LanguageJsonSpecFile::GetDescriptionFromIndex(const unsigned index)
{
    const auto& lookup = std::find_if(m_submenuOptions.cbegin(), m_submenuOptions.cend(),
                                      [&](const auto& id_and_description) { return ( std::get<0>(id_and_description) == index ); });
    ASSERT(lookup != m_submenuOptions.cend());

    return std::get<1>(*lookup);
}



// --------------------------------------------------------------------------
// LanguageSettings
// --------------------------------------------------------------------------

LanguageSettings::LanguageSettings(const std::wstring& filename/* = std::wstring()*/)
{
    std::optional<LanguageType> language_type;

    // if a filename is specified...
    if( !filename.empty() )
    {
        // ...see if the type has been manually specified at some point
        language_type = GetLanguageSettingsPersister().GetLanguageType(filename);

        // if not, determine the language type based on the filename -> lexer language routine
        if( !language_type.has_value() )
        {
            const int lexer_language = Lexers::GetLexerFromFilename(filename);

            if( lexer_language == SCLEX_HTML )
            {
                // for HTML files, if the file is in the HTML dialogs directory, treat it as a HTML dialog
                language_type = SO::StartsWithNoCase(filename, Html::GetDirectory(Html::Subdirectory::Dialogs)) ? LanguageType::CSProHtmlDialog :
                                                                                                                  LanguageType::Html;
            }

            else
            {
                language_type = ( lexer_language == SCLEX_CSPRO_LOGIC_V8_0 )       ? std::make_optional(LanguageType::CSProLogic) :
                                ( lexer_language == SCLEX_CSPRO_MESSAGE_V8_0 )     ? std::make_optional(LanguageType::CSProMessages) :
                                ( lexer_language == SCLEX_CSPRO_DOCUMENT )         ? std::make_optional(LanguageType::CSProDocument) :
                                ( lexer_language == SCLEX_JAVASCRIPT )             ? std::make_optional(LanguageType::JavaScript) :
                                ( lexer_language == SCLEX_JSON )                   ? std::make_optional(LanguageType::Json) :
                                ( lexer_language == SCLEX_SQL )                    ? std::make_optional(LanguageType::Sql) :
                                ( lexer_language == SCLEX_YAML )                   ? std::make_optional(LanguageType::Yaml) :
                                ( lexer_language == SCLEX_NULL )                   ? std::nullopt :
                                                                                     ReturnProgrammingError(std::nullopt);
            }

            // if the language type is still unknown, check the extension to see if this is a CSPro specification file
            if( !language_type.has_value() && LanguageJsonSpecFile::GetIndexFromFilename(filename).has_value() )
            {
                language_type = JsonSpecFile::IsPre80SpecFile(filename) ? LanguageType::CSProSpecFileIni :
                                                                          LanguageType::CSProSpecFileJson;
            }
        }
    }

    // if the language type is still unknown, default to text
    m_languageType = language_type.value_or(LanguageType::Text);

    SyncPropertiesFollowingLanguageChange(filename);
}


LanguageSettings::LanguageSettings(const LanguageType language_type)
    :   m_languageType(language_type),
        m_lexerLanguage(std::get<0>(GetLexerLanguageAndLogicSettings(m_languageType, SO::EmptyString)))
{
}


void LanguageSettings::SyncPropertiesFollowingLanguageChange(const std::wstring& filename)
{
    // get the lexer language and logic settings based on the language type
    std::tie(m_lexerLanguage, m_logicSettings) = GetLexerLanguageAndLogicSettings(m_languageType, filename);


    // sync the Action Invoker settings
    if( m_languageType == LanguageType::CSProActionInvoker )
    {
        m_actionInvokerJsonResultsAndExceptionFlags = GetLanguageSettingsPersister().GetActionInvokerJsonResultsAndExceptionFlags(filename);
    }

    else
    {
        m_actionInvokerJsonResultsAndExceptionFlags.reset();
    }


    // sync the spec file index, defaulting to a dictionary if the type cannot be determined by the filename
    if( m_languageType == LanguageType::CSProSpecFileJson )
    {
        if( !m_jsonSpecFileIndex.has_value() )
            m_jsonSpecFileIndex = LanguageJsonSpecFile::GetIndexFromFilename(filename).value_or(ID_RUN_CSPRO_SPEC_FILE_DCF);
    }

    else
    {
        m_jsonSpecFileIndex.reset();
    }


    // sync the JavaScript module type, defaulting to autodetect
    if( m_languageType == LanguageType::JavaScript )
    {
        // CODE_TODO for JavaScript files, can look at the application file to see if the file is a module
        if( !m_javascriptModuleType.has_value() )
        {
            m_javascriptModuleType = GetLanguageSettingsPersister().GetJavaScriptModuleType(filename);

            if( !m_javascriptModuleType.has_value() )
            {
                if( SO::EqualsNoCase(PortableFunctions::PathGetFileExtension(filename), FileExtensions::JavaScriptModule) )
                {
                    m_javascriptModuleType = ID_RUN_JAVASCRIPT_MODULE_MODULE;
                }

                else
                {
                    m_javascriptModuleType = ID_RUN_JAVASCRIPT_MODULE_AUTODETECT;
                }
            }
        }
    }

    else
    {
        m_javascriptModuleType.reset();
    }
}


std::tuple<int, std::optional<LogicSettings>> LanguageSettings::GetLexerLanguageAndLogicSettings(const LanguageType language_type, const std::wstring& filename)
{
    std::optional<LogicSettings> logic_settings = GetLanguageSettingsPersister().GetLogicSettings(filename);

    auto use_logic_version_v8 = [&]()
    {
        if( logic_settings.has_value() )
            return UseLogicVersionV8(*logic_settings);

        // if never defined, search for application files in the directory to see if there are any applications that use this file
        if( !filename.empty() )
        {
            logic_settings = SearchApplicationsForLogicSettings(filename);

            // if defined, save this setting for future use
            if( logic_settings.has_value() )
                GetLanguageSettingsPersister().Remember(filename, &language_type, nullptr, &(*logic_settings), nullptr);
        }

        // if not found in any file, use the default settings
        if( !logic_settings.has_value() )
            logic_settings = LogicSettings::GetUserDefaultSettings();

        return UseLogicVersionV8(*logic_settings);
    };

    const int lexer_language =
        ( language_type == LanguageType::CSProLogic )         ? ( use_logic_version_v8() ? SCLEX_CSPRO_LOGIC_V8_0 : SCLEX_CSPRO_LOGIC_V0 ) :
        ( language_type == LanguageType::CSProReportHtml)     ? ( use_logic_version_v8() ? SCLEX_CSPRO_REPORT_HTML_V8_0 : SCLEX_CSPRO_REPORT_HTML_V0 ) :
        ( language_type == LanguageType::CSProReport )        ? ( use_logic_version_v8() ? SCLEX_CSPRO_REPORT_V8_0 : SCLEX_CSPRO_REPORT_V0 ) :
        ( language_type == LanguageType::CSProMessages )      ? ( use_logic_version_v8() ? SCLEX_CSPRO_MESSAGE_V8_0 : SCLEX_CSPRO_MESSAGE_V0 ) :
        ( language_type == LanguageType::CSProActionInvoker ) ? SCLEX_JSON :
        ( language_type == LanguageType::CSProHtmlDialog )    ? SCLEX_HTML :
        ( language_type == LanguageType::CSProDocument )      ? SCLEX_CSPRO_DOCUMENT :
        ( language_type == LanguageType::CSProSpecFileJson )  ? SCLEX_JSON :
        ( language_type == LanguageType::CSProSpecFileIni )   ? SCLEX_CSPRO_PRE80_SPEC_FILE :
        ( language_type == LanguageType::Html )               ? SCLEX_HTML :
        ( language_type == LanguageType::JavaScript )         ? SCLEX_JAVASCRIPT :
        ( language_type == LanguageType::Json )               ? SCLEX_JSON :
        ( language_type == LanguageType::Sql )                ? SCLEX_SQL :
        ( language_type == LanguageType::Yaml )               ? SCLEX_YAML :
        ( language_type == LanguageType::Text )               ? SCLEX_NULL :
                                                                ReturnProgrammingError(SCLEX_NULL);

    return { lexer_language, std::move(logic_settings) };
}


std::optional<LogicSettings> LanguageSettings::SearchApplicationsForLogicSettings(const std::wstring& filename)
{
    ASSERT(!filename.empty());

    const std::wstring directory = PortableFunctions::PathGetDirectory(filename);
    ASSERT(PortableFunctions::PathEnsureTrailingSlash(directory) == directory);

    std::vector<std::wstring> application_filenames;

    for( const TCHAR* wildcard : { FileExtensions::Wildcard::EntryApplication,
                                   FileExtensions::Wildcard::BatchApplication,
                                   FileExtensions::Wildcard::TabulationApplication } )
    {
        DirectoryLister().AddFilenamesWithPossibleWildcard(application_filenames, directory + wildcard, false);
    }

    for( const std::wstring& application_filename : application_filenames )
    {
        try
        {
            Application application;
            application.Open(application_filename, true, false);

            // search code files
            for( const CodeFile& code_file : application.GetCodeFiles() )
            {
                if( SO::EqualsNoCase(filename, code_file.GetFilename()) )
                    return application.GetLogicSettings();
            }

            // search reports
            for( const NamedTextSource& report_named_text_sources : VI_V(application.GetReportNamedTextSources()) )
            {
                if( SO::EqualsNoCase(filename, report_named_text_sources.text_source->GetFilename()) )
                    return application.GetLogicSettings();
            }
        }
        catch(...) { }
    }

    return std::nullopt;
}


std::wstring LanguageSettings::GetFileTypeDescription() const
{
    std::wstring description = ( m_languageType == LanguageType::CSProActionInvoker ) ? _T("CSPro Action Invoker") :
                               ( m_languageType == LanguageType::CSProHtmlDialog )    ? _T("CSPro HTML Dialog") :
                                                                                        Lexers::GetLexerName(m_lexerLanguage);

    if( m_jsonSpecFileIndex.has_value() )
    {
        ASSERT(m_languageType == LanguageType::CSProSpecFileJson);
        SO::Append(description, _T(": "), LanguageJsonSpecFile::GetDescriptionFromIndex(*m_jsonSpecFileIndex));
    }

    else if( m_javascriptModuleType.has_value() )
    {
        ASSERT(m_languageType == LanguageType::JavaScript);
        SO::AppendFormat(description, _T(" (%s)"), ( *m_javascriptModuleType == ID_RUN_JAVASCRIPT_MODULE_AUTODETECT ) ? _T("Autodetect") :
                                                   ( *m_javascriptModuleType == ID_RUN_JAVASCRIPT_MODULE_GLOBAL )     ? _T("Global") :
                                                                                                                        _T("Module"));
    }

    return description;
}


bool LanguageSettings::GetActionInvokerDisplayResultsAsJson() const
{
    return m_actionInvokerJsonResultsAndExceptionFlags.has_value() ? std::get<0>(*m_actionInvokerJsonResultsAndExceptionFlags) :
                                                                     false;
}


bool LanguageSettings::GetActionInvokerAbortOnException() const
{
    return m_actionInvokerJsonResultsAndExceptionFlags.has_value() ? std::get<1>(*m_actionInvokerJsonResultsAndExceptionFlags) :
                                                                     true;
}


const LogicSettings& LanguageSettings::GetOrCreateLogicSettings()
{
    if( !m_logicSettings.has_value() )
        m_logicSettings.emplace(Lexers::IsNotV0(m_lexerLanguage) ? LogicSettings::Version::V8_0 : LogicSettings::Version::V0);

    return *m_logicSettings;
}


void LanguageSettings::SetLanguageType(const LanguageType language_type, const std::wstring& filename)
{
    m_languageType = language_type;

    // save the override for future use
    GetLanguageSettingsPersister().Remember(filename, &m_languageType, nullptr, nullptr, nullptr);

    SyncPropertiesFollowingLanguageChange(filename);
}


void LanguageSettings::SetActionInvokerDisplayResultsAsJson(const bool display_results_as_json, const std::wstring& filename)
{
    m_actionInvokerJsonResultsAndExceptionFlags.emplace(display_results_as_json, GetActionInvokerAbortOnException());

    // save the override for future use
    GetLanguageSettingsPersister().Remember(filename, nullptr, &(*m_actionInvokerJsonResultsAndExceptionFlags), nullptr, nullptr);
}


void LanguageSettings::SetActionInvokerAbortOnException(const bool abort_on_exception, const std::wstring& filename)
{
    m_actionInvokerJsonResultsAndExceptionFlags.emplace(GetActionInvokerDisplayResultsAsJson(), abort_on_exception);

    // save the override for future use
    GetLanguageSettingsPersister().Remember(filename, nullptr, &(*m_actionInvokerJsonResultsAndExceptionFlags), nullptr, nullptr);
}


void LanguageSettings::SetLogicVersion(const LogicSettings::Version version, const std::wstring& filename)
{
    if( !m_logicSettings.has_value() )
        GetOrCreateLogicSettings();

    m_logicSettings->SetVersion(version);

    // save the override for future use
    GetLanguageSettingsPersister().Remember(filename, nullptr, nullptr, &(*m_logicSettings), nullptr);

    // get the new lexer language
    std::tie(m_lexerLanguage, std::ignore) = GetLexerLanguageAndLogicSettings(m_languageType, filename);
}


void LanguageSettings::SetJsonSpecFileIndex(const unsigned index)
{
    ASSERT(m_languageType == LanguageType::CSProSpecFileJson);

    m_jsonSpecFileIndex = index;
}


void LanguageSettings::SetJavaScriptModuleType(const unsigned index, const std::wstring& filename)
{
    ASSERT(m_languageType == LanguageType::JavaScript);

    m_javascriptModuleType = index;

    // save the override for future use
    GetLanguageSettingsPersister().Remember(filename, nullptr, nullptr, nullptr, &(*m_javascriptModuleType));
}
