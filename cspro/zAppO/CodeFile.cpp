#include "stdafx.h"
#include "CodeFile.h"


// --------------------------------------------------------------------------
// CodeType
// --------------------------------------------------------------------------

constexpr const char* ToString(const CodeType code_type)
{
    return ( code_type == CodeType::LogicMain )       ?      "Logic" :
           ( code_type == CodeType::LogicExternal )   ?      "Logic (External)" :
           ( code_type == CodeType::JavaScriptAutodetect ) ? "JavaScript (Autodetect)" :
           ( code_type == CodeType::JavaScriptGlobal ) ?     "JavaScript (Global)" :
         /*( code_type == CodeType::JavaScriptModule ) ? */  "JavaScript (Module)";
}

CREATE_ENUM_JSON_SERIALIZER(CodeType,
    { CodeType::LogicMain,            "main" },
    { CodeType::LogicExternal,        "external" },
    { CodeType::JavaScriptAutodetect, "JavaScript" },
    { CodeType::JavaScriptGlobal,     "JavaScript:global" },
    { CodeType::JavaScriptModule,     "JavaScript:module" })



// --------------------------------------------------------------------------
// CodeFile
// --------------------------------------------------------------------------

CodeFile::CodeFile(const CodeType code_type, std::shared_ptr<TextSource> text_source)
    :   m_codeType(code_type),
        m_textSource(std::move(text_source))
{
    ASSERT(m_textSource != nullptr);
}


CodeFile::CodeFile()
    :   m_codeType(CodeType::LogicExternal)
{
    // this should never be called explicitly but allows serialization routines to work properly
}


CodeFile CodeFile::CreateFromJson(const JsonNode& json_node,
                                  const std::function<std::shared_ptr<TextSource>(const std::string& file_path)>& text_source_creator/* = { }*/)
{
    std::string file_path = json_node.GetAbsolutePath(json_node.Contains(JK::filename) ? JK::filename : JK::path);
    std::shared_ptr<TextSource> text_source;

    if( text_source_creator )
    {
        text_source = text_source_creator(file_path);
        ASSERT(text_source != nullptr);
    }

    else
    {
        text_source = std::make_unique<TextSource>(std::move(file_path));
    }

    return CodeFile(json_node.GetOrDefault(JK::type, CodeType::LogicMain),
                    std::move(text_source));
}


void CodeFile::WriteJson(JsonWriter& json_writer) const
{
    ASSERT(m_textSource != nullptr);

    json_writer.BeginObject()
               .Write(JK::type, m_codeType)
               .WriteRelativePath(JK::path, m_textSource->GetFilePath())
               .EndObject();
}


void CodeFile::serialize(Serializer& ar)
{
    ASSERT(ar.IsLoading() == ( m_textSource == nullptr ));

    if( ar.IsLoading() )
        m_textSource = std::make_unique<TextSource>();

    if( ar.MeetsVersionIteration(Serializer::Iteration_8_0_000_1) )
        ar.SerializeEnum(m_codeType);

    ar & *m_textSource;

    if( ar.MeetsVersionIteration(Serializer::Iteration_8_0_000_1) &&
        ar.PredatesVersionIteration(Serializer::Iteration_8_1_000_1) )
    {
        ar.Read<std::string>(); // m_namespaceName;
    }
}
