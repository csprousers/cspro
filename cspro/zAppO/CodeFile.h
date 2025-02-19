#pragma once

#include <zAppO/zAppO.h>
#include <zUtilO/TextSource.h>


// --------------------------------------------------------------------------
// CodeType
// --------------------------------------------------------------------------

enum class CodeType : int { LogicMain, LogicExternal, JavaScriptAutodetect, JavaScriptGlobal, JavaScriptModule };

ZAPPO_API constexpr const char* ToString(CodeType code_type);

constexpr bool IsLogic(CodeType code_type);
constexpr bool IsJavaScript(CodeType code_type);



// --------------------------------------------------------------------------
// CodeFile
// --------------------------------------------------------------------------

class ZAPPO_API CodeFile
{
public:
    CodeFile(CodeType code_type, std::shared_ptr<TextSource> text_source);
    CodeFile();

    CodeType GetCodeType() const         { return m_codeType; }
    void SetCodeType(CodeType code_type) { m_codeType = code_type; }

    bool IsLogicMain() const             { return ( m_codeType == CodeType::LogicMain ); }
    bool IsLogic() const                 { return ::IsLogic(m_codeType); }
    bool IsJavaScript() const            { return ::IsJavaScript(m_codeType); }

    const TextSource& GetTextSource() const           { ASSERT(m_textSource != nullptr); return *m_textSource; }
    TextSource& GetTextSource()                       { ASSERT(m_textSource != nullptr); return *m_textSource; }
    std::shared_ptr<TextSource> GetSharedTextSource() { return m_textSource; }

    const std::string& GetFilePath() const { ASSERT(m_textSource != nullptr); return m_textSource->GetFilePath(); }

    // serialization
    // --------------------------------------------------------------------------
    static CodeFile CreateFromJson(const JsonNode& json_node,
                                   const std::function<std::shared_ptr<TextSource>(const std::string& file_path)>& text_source_creator = { });
    void WriteJson(JsonWriter& json_writer) const;

    void serialize(Serializer& ar);

private:
    CodeType m_codeType;
    std::shared_ptr<TextSource> m_textSource;
};



// --------------------------------------------------------------------------
// inline implementations
// --------------------------------------------------------------------------

constexpr bool IsLogic(const CodeType code_type)
{
    return ( code_type <= CodeType::LogicExternal );
}


constexpr bool IsJavaScript(const CodeType code_type)
{
    return ( code_type >= CodeType::JavaScriptAutodetect );
}
