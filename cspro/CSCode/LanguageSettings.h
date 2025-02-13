#pragma once

#include <CSCode/resource.h>
#include <zToolsO/EnumHelpers.h>
#include <zAppO/LogicSettings.h>
#include <zEdit2O/Lexers.h>


// --------------------------------------------------------------------------
// LanguageType
// --------------------------------------------------------------------------

enum class LanguageType
{
    CSProLogic,
    CSProReportHtml,
    CSProReport,
    CSProMessages,
    CSProActionInvoker,
    CSProHtmlDialog,
    CSProDocument,
    CSProSpecFileJson,
    CSProSpecFileIni,
    Html,
    JavaScript,
    Json,
    Sql,
    Yaml,
    Text
};

template<> constexpr LanguageType FirstInEnum<LanguageType>() { return LanguageType::CSProLogic; }
template<> constexpr LanguageType LastInEnum<LanguageType>()  { return LanguageType::Text;       }


constexpr LanguageType GetLanguageTypeFromId(UINT nID)
{
    static_assert(( ID_LANGUAGE_NONE - ID_LANGUAGE_CSPRO_LOGIC ) == static_cast<unsigned>(LastInEnum<LanguageType>()));

    return AddToEnum(LanguageType::CSProLogic, nID - ID_LANGUAGE_CSPRO_LOGIC);
}



// --------------------------------------------------------------------------
// LanguageJsonSpecFile
// --------------------------------------------------------------------------

class LanguageJsonSpecFile
{
public:
    static const std::vector<std::tuple<unsigned, std::wstring>>& GetSubmenuOptions() { return m_submenuOptions; }

    static std::optional<unsigned> GetIndexFromFilename(const std::wstring& filename);

    static const std::wstring& GetDescriptionFromIndex(unsigned index);

private:
    static const std::vector<std::tuple<unsigned, std::wstring>> m_submenuOptions;
};



// --------------------------------------------------------------------------
// LanguageSettings
// --------------------------------------------------------------------------

class LanguageSettings
{
public:
    LanguageSettings(const std::wstring& filename = std::wstring());
    LanguageSettings(LanguageType language_type);

    // getters
    LanguageType GetLanguageType() const { return m_languageType; }

    int GetLexerLanguage() const { return m_lexerLanguage; }

    bool GetActionInvokerDisplayResultsAsJson() const;
    bool GetActionInvokerAbortOnException() const;

    const LogicSettings& GetOrCreateLogicSettings();

    const std::optional<unsigned>& GetJsonSpecFileIndex() const { return m_jsonSpecFileIndex; }

    const std::optional<unsigned>& GetJavaScriptModuleType() const { return m_javascriptModuleType; }

    std::wstring GetFileTypeDescription() const;

    // setters
    void SetLanguageType(LanguageType language_type, const std::wstring& filename);

    void SetActionInvokerDisplayResultsAsJson(bool display_results_as_json, const std::wstring& filename);
    void SetActionInvokerAbortOnException(bool abort_on_exception, const std::wstring& filename);

    void SetLogicVersion(LogicSettings::Version version, const std::wstring& filename);

    void SetJsonSpecFileIndex(unsigned index);

    void SetJavaScriptModuleType(unsigned index, const std::wstring& filename);

    // menu helpers
    bool CanCompileCode() const;
    bool CanValidateCode() const;
    bool CanCompileOrValidateCode() const;
    bool CanRunCode() const;
    bool CanStopCode() const;
    bool CanViewReportPreview() const;

    // other helpers
    bool UsesTwoCodeViews() const;

private:
    void SyncPropertiesFollowingLanguageChange(const std::wstring& filename);

    static std::tuple<int, std::optional<LogicSettings>> GetLexerLanguageAndLogicSettings(LanguageType language_type, const std::wstring& filename);

    static std::optional<LogicSettings> SearchApplicationsForLogicSettings(const std::wstring& filename);

private:
    LanguageType m_languageType;
    int m_lexerLanguage;
    std::optional<std::tuple<bool, bool>> m_actionInvokerJsonResultsAndExceptionFlags;
    std::optional<LogicSettings> m_logicSettings;
    std::optional<unsigned> m_jsonSpecFileIndex;
    std::optional<unsigned> m_javascriptModuleType;
};


inline bool LanguageSettings::CanCompileCode() const
{
    return ( // CODE_TODO restore when CSPro logic can be compiled Lexers::UsesCSProLogic(m_lexerLanguage) ||
             m_lexerLanguage == SCLEX_CSPRO_MESSAGE_V8_0 || 
             m_lexerLanguage == SCLEX_JAVASCRIPT );
}


inline bool LanguageSettings::CanValidateCode() const
{
    return ( m_lexerLanguage == SCLEX_JSON );
}


inline bool LanguageSettings::CanCompileOrValidateCode() const
{
    return ( CanCompileCode() || CanValidateCode() );
}


inline bool LanguageSettings::CanRunCode() const
{
    return ( // CODE_TODO restore when CSPro logic can be compiled Lexers::UsesCSProLogic(m_lexerLanguage) ||
             m_languageType == LanguageType::CSProActionInvoker ||
             m_lexerLanguage == SCLEX_HTML ||
             m_lexerLanguage == SCLEX_JAVASCRIPT );
}


inline bool LanguageSettings::CanStopCode() const
{
    return ( m_languageType == LanguageType::CSProActionInvoker ||
             m_lexerLanguage == SCLEX_JAVASCRIPT );
}


inline bool LanguageSettings::CanViewReportPreview() const
{
    return Lexers::IsCSProReportHtml(m_lexerLanguage);
}


inline bool LanguageSettings::UsesTwoCodeViews() const
{
    return ( m_languageType == LanguageType::CSProHtmlDialog );
}
