#pragma once

#include <zAppO/zAppO.h>
#include <zAppO/AppFileType.h>

template<typename CharType> class JsonNode;
class JsonWriter;
class Serializer;


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

    const std::wstring& GetSingleLineComment() const { return m_singleLineComment; }

    const std::wstring& GetMultilineCommentStart() const { return std::get<0>(MeetsVersion(Version::V8_0) ? m_multilineCommentNew : m_multilineCommentOld); }
    const std::wstring& GetMultilineCommentEnd() const   { return std::get<1>(MeetsVersion(Version::V8_0) ? m_multilineCommentNew : m_multilineCommentOld); }

    // default line methods
    std::wstring GetDefaultFirstLineForTextSource(NullTerminatedString application_label, AppFileType app_file_type) const;
    std::wstring GetGeneratedCodeTextForTextSource() const;

    // Action Invoker settings
    ActionInvokerAccessFromExternalCaller GetActionInvokerAccessFromExternalCaller() const      { return m_actionInvokerAccessFromExternalCaller; }
    void SetActionInvokerAccessFromExternalCaller(ActionInvokerAccessFromExternalCaller access) { m_actionInvokerAccessFromExternalCaller = access; }

    const std::vector<std::wstring>& GetActionInvokerAccessTokens() const      { return m_actionInvokerAccessTokens; }
    void SetActionInvokerAccessTokens(std::vector<std::wstring> access_tokens) { m_actionInvokerAccessTokens = std::move(access_tokens); }

    bool GetActionInvokerConvertResults() const    { return m_actionInvokerConvertResults; }
    void SetActionInvokerConvertResults(bool flag) { m_actionInvokerConvertResults = flag; }

    // serialization
    static LogicSettings CreateFromJson(const JsonNode<wchar_t>& json_node);
    void WriteJson(JsonWriter& json_writer) const;
    void serialize(Serializer& ar);

private:
    Version m_version;
    bool m_caseSensitiveSymbols;

    ActionInvokerAccessFromExternalCaller m_actionInvokerAccessFromExternalCaller;
    std::vector<std::wstring> m_actionInvokerAccessTokens;
    bool m_actionInvokerConvertResults;

    static const std::wstring m_singleLineComment;
    static const std::tuple<const std::wstring, const std::wstring> m_multilineCommentNew;
    static const std::tuple<const std::wstring, const std::wstring> m_multilineCommentOld;
};


namespace CommentStrings
{
    constexpr std::wstring_view SingleLine        = _T("//");
    constexpr std::wstring_view MultilineNewStart = _T("/*");
    constexpr std::wstring_view MultilineNewEnd   = _T("*/");
    constexpr std::wstring_view MultilineOldStart = _T("{");
    constexpr std::wstring_view MultilineOldEnd   = _T("}");

    constexpr std::tuple<const std::wstring_view&, const std::wstring_view&> GetMultilineStartEnd(bool new_version)
    {
        return new_version ? std::tuple<const std::wstring_view&, const std::wstring_view&>(MultilineNewStart, MultilineNewEnd) :
                             std::tuple<const std::wstring_view&, const std::wstring_view&>(MultilineOldStart, MultilineOldEnd);
    }
}
