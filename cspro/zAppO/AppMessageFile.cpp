#include "stdafx.h"
#include "AppMessageFile.h"


CREATE_ENUM_JSON_SERIALIZER(AppMessageFile::Type,
    { AppMessageFile::Type::User,   JK::user },
    { AppMessageFile::Type::System, "system" })


AppMessageFile::AppMessageFile(const Type type, std::shared_ptr<TextSource> text_source)
    :   m_type(type),
        m_textSource(std::move(text_source))
{
    ASSERT(m_textSource != nullptr);
}


AppMessageFile::AppMessageFile()
    :   m_type(Type::User)
{
    // this should never be called explicitly but allows serialization routines to work properly
}


AppMessageFile::Type AppMessageFile::GetTypeFromFilename(const std::string& file_path)
{
    // prior to CSPro 8.1, system messages had to be specified with the CSProRuntime prefix
    return SO::StartsWithNoCase(PortableFunctions::PathGetFilename(file_path), "CSProRuntime") ? Type::System :
                                                                                                 Type::User;
}


AppMessageFile AppMessageFile::CreateFromJson(const JsonNode& json_node,
                                              const std::function<std::shared_ptr<TextSource>(const std::string& file_path)>& text_source_creator/* = { }*/)
{
    const bool specified_in_8_0_format = json_node.IsString();

    std::string file_path = specified_in_8_0_format ? json_node.GetAbsolutePath() :
                                                      json_node.GetAbsolutePath(JK::path);

    const Type type = specified_in_8_0_format ? GetTypeFromFilename(file_path) :
                                                json_node.Get<Type>(JK::type);

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

    return AppMessageFile(type, std::move(text_source));
}


void AppMessageFile::WriteJson(JsonWriter& json_writer) const
{
    ASSERT(m_textSource != nullptr);

    json_writer.BeginObject()
               .Write(JK::type, m_type)
               .WriteRelativePath(JK::path, m_textSource->GetFilePath())
               .EndObject();
}


void AppMessageFile::serialize(Serializer& ar)
{
    ASSERT(ar.IsLoading() == ( m_textSource == nullptr ));

    if( ar.IsLoading() )
        m_textSource = std::make_unique<TextSource>();

    if( ar.MeetsVersionIteration(Serializer::Iteration_8_1_000_1) )
    {
        ar.SerializeEnum(m_type);
        ar & *m_textSource;
    }

    else
    {
        ar & *m_textSource;
        m_type = GetTypeFromFilename(m_textSource->GetFilePath());
    }
}
