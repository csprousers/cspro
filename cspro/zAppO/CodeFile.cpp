#include "stdafx.h"
#include "CodeFile.h"


// --------------------------------------------------------------------------
// CodeType
// --------------------------------------------------------------------------

const TCHAR* ToString(CodeType code_type)
{
    return ( code_type == CodeType::LogicMain )     ?   _T("Logic") :
         /*( code_type == CodeType::LogicExternal ) ? */_T("Logic (External)");
}

CREATE_ENUM_JSON_SERIALIZER(CodeType,
    { CodeType::LogicMain,     _T("main") },
    { CodeType::LogicExternal, _T("external") })



// --------------------------------------------------------------------------
// CodeFile
// --------------------------------------------------------------------------

CodeFile::CodeFile(CodeType code_type, std::shared_ptr<TextSource> text_source, std::wstring namespace_name/* = std::wstring()*/)
    :   m_codeType(code_type),
        m_textSource(std::move(text_source)),
        m_namespaceName(std::move(namespace_name))
{
    ASSERT(m_textSource != nullptr);
}


CodeFile::CodeFile()
    :   m_codeType(CodeType::LogicExternal)
{
    // this should never be called explicitly but allows serialization routines to work properly
}


CodeFile CodeFile::CreateFromJson(const JsonNode<wchar_t>& json_node,
                                  const std::function<std::shared_ptr<TextSource>(const std::wstring& filename)>& text_source_creator/* = { }*/)
{
    std::wstring filename = json_node.GetAbsolutePath(json_node.Contains(JK::filename) ? JK::filename : JK::path);
    std::shared_ptr<TextSource> text_source;

    if( text_source_creator )
    {
        text_source = text_source_creator(filename);
        ASSERT(text_source != nullptr);
    }

    else
    {
        text_source = std::make_shared<TextSource>(filename);
    }

    return CodeFile(json_node.GetOrDefault(JK::type, CodeType::LogicMain),
                    std::move(text_source),
                    json_node.GetOrDefault(JK::namespace_, SO::EmptyString));
}


void CodeFile::WriteJson(JsonWriter& json_writer) const
{
    ASSERT(m_textSource != nullptr);

    json_writer.BeginObject()
               .Write(JK::type, m_codeType)
               .WriteRelativePath(JK::path, m_textSource->GetFilename())
               .WriteIfNotBlank(JK::namespace_, m_namespaceName)
               .EndObject();
}


void CodeFile::serialize(Serializer& ar)
{
    if( ar.IsLoading() )
        m_textSource = std::make_shared<TextSource>();

    ASSERT(m_textSource != nullptr);

    if( ar.MeetsVersionIteration(Serializer::Iteration_8_0_000_1) )
        ar.SerializeEnum(m_codeType);

    ar & *m_textSource;

    if( ar.MeetsVersionIteration(Serializer::Iteration_8_0_000_1) )
       ar & m_namespaceName;
}
