#pragma once

#include <zAppO/zAppO.h>
#include <zUtilO/TextSource.h>

template<typename CharType> class JsonNode;
class JsonWriter;
class Serializer;


// --------------------------------------------------------------------------
// CodeType
// --------------------------------------------------------------------------

enum class CodeType : int { LogicMain, LogicExternal };

ZAPPO_API const TCHAR* ToString(CodeType code_type);


// --------------------------------------------------------------------------
// CodeFile
// --------------------------------------------------------------------------

class ZAPPO_API CodeFile
{
public:
    CodeFile(CodeType code_type, std::shared_ptr<TextSource> text_source, std::wstring namespace_name = std::wstring());
    CodeFile();

    CodeType GetCodeType() const { return m_codeType; }
    bool IsLogicMain() const     { return ( m_codeType == CodeType::LogicMain ); }

    const TextSource& GetTextSource() const           { ASSERT(m_textSource != nullptr); return *m_textSource; }
    TextSource& GetTextSource()                       { ASSERT(m_textSource != nullptr); return *m_textSource; }
    std::shared_ptr<TextSource> GetSharedTextSource() { return m_textSource; }

    const std::wstring& GetFilename() const { ASSERT(m_textSource != nullptr); return m_textSource->GetFilename(); }

    const std::wstring& GetNamespaceName() const { return m_namespaceName; }

    // serialization
    // --------------------------------------------------------------------------
    static CodeFile CreateFromJson(const JsonNode<wchar_t>& json_node,
                                   const std::function<std::shared_ptr<TextSource>(const std::wstring& filename)>& text_source_creator = { });
    void WriteJson(JsonWriter& json_writer) const;

    void serialize(Serializer& ar);

private:
    CodeType m_codeType;
    std::shared_ptr<TextSource> m_textSource;
    std::wstring m_namespaceName;
};
