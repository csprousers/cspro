#pragma once

#include <zAppO/zAppO.h>
#include <zAppO/AppFileType.h>


class ZAPPO_API LogicSettings
{
public:
    enum class Version : int { V0, V8_0 };
    static constexpr Version LatestVersion = Version::V8_0;

    enum class ActionInvokerAccessFromExternalCaller { AlwaysAllow, PromptIfNoValidAccessToken, RequireAccessToken };

public:
    LogicSettings(Version version = Version::V0);

    static LogicSettings GetOriginalSettings() { return LogicSettings(Version::V0); }

    static LogicSettings GetUserDefaultSettings();

    bool operator==(const LogicSettings& rhs) const;
    bool operator!=(const LogicSettings& rhs) const { return !operator==(rhs); }

    Version GetVersion() const               { return m_version; }
    void SetVersion(Version version)         { m_version = version; }
    bool MeetsVersion(Version version) const { return ( m_version >= version ); }

    bool EscapeStringLiterals() const      { return MeetsVersion(Version::V8_0); }
    bool UseVerbatimStringLiterals() const { return MeetsVersion(Version::V8_0); }

    bool CaseSensitiveSymbols() const       { return m_caseSensitiveSymbols; }
    void SetCaseSensitiveSymbols(bool flag) { m_caseSensitiveSymbols = flag; }

    const std::string& GetSingleLineComment() const { return m_singleLineComment; }

    const std::string& GetMultilineCommentStart() const { return std::get<0>(MeetsVersion(Version::V8_0) ? m_multilineCommentNew : m_multilineCommentOld); }
    const std::string& GetMultilineCommentEnd() const   { return std::get<1>(MeetsVersion(Version::V8_0) ? m_multilineCommentNew : m_multilineCommentOld); }

    // default line methods
    std::string GetDefaultFirstLineForTextSource(cs::string_sz application_label, AppFileType app_file_type) const;
    std::string GetGeneratedCodeTextForTextSource() const;

    // Action Invoker settings
    ActionInvokerAccessFromExternalCaller GetActionInvokerAccessFromExternalCaller() const      { return m_actionInvokerAccessFromExternalCaller; }
    void SetActionInvokerAccessFromExternalCaller(ActionInvokerAccessFromExternalCaller access) { m_actionInvokerAccessFromExternalCaller = access; }

    const std::vector<std::string>& GetActionInvokerAccessTokens() const      { return m_actionInvokerAccessTokens; }
    void SetActionInvokerAccessTokens(std::vector<std::string> access_tokens) { m_actionInvokerAccessTokens = std::move(access_tokens); }

    bool GetActionInvokerConvertResults() const    { return m_actionInvokerConvertResults; }
    void SetActionInvokerConvertResults(bool flag) { m_actionInvokerConvertResults = flag; }

    // serialization
    static LogicSettings CreateFromJson(const JsonNode& json_node);
    void WriteJson(JsonWriter& json_writer) const;
    void serialize(Serializer& ar);

private:
    Version m_version;
    bool m_caseSensitiveSymbols;

    ActionInvokerAccessFromExternalCaller m_actionInvokerAccessFromExternalCaller;
    std::vector<std::string> m_actionInvokerAccessTokens;
    bool m_actionInvokerConvertResults;

    static const std::string m_singleLineComment;
    static const std::tuple<const std::string, const std::string> m_multilineCommentNew;
    static const std::tuple<const std::string, const std::string> m_multilineCommentOld;
};


namespace CommentStrings
{
    constexpr std::string_view SingleLine_sv        = "//";
    constexpr std::string_view MultilineNewStart_sv = "/*";
    constexpr std::string_view MultilineNewEnd_sv   = "*/";
    constexpr std::string_view MultilineOldStart_sv = "{";
    constexpr std::string_view MultilineOldEnd_sv   = "}";

    constexpr std::tuple<const std::string_view&, const std::string_view&> GetMultilineStartEnd(bool new_version)
    {
        return new_version ? std::tuple<const std::string_view&, const std::string_view&>(MultilineNewStart_sv, MultilineNewEnd_sv) :
                             std::tuple<const std::string_view&, const std::string_view&>(MultilineOldStart_sv, MultilineOldEnd_sv);
    }
}
