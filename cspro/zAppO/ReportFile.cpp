#include "stdafx.h"
#include "ReportFile.h"
#include <zUtilO/FileExtensions.h>


DEFINE_ENUM_JSON_SERIALIZER_CLASS(ReportFile::Encoding,
    { ReportFile::Encoding::None,     "none" },
    { ReportFile::Encoding::Html,     "HTML" },
    { ReportFile::Encoding::Markdown, "Markdown" },
    { ReportFile::Encoding::Csv,      "CSV" })


ReportFile::ReportFile(std::string name, const Encoding encoding, std::shared_ptr<TextSource> text_source)
    :   m_name(std::move(name)),
        m_encoding(encoding),
        m_textSource(std::move(text_source))
{
    ASSERT(m_textSource != nullptr);
}


ReportFile::ReportFile()
    :   m_encoding(Encoding::None)
{
    // this should never be called explicitly but allows serialization routines to work properly
}


ReportFile::Encoding ReportFile::GetDefaultEncodingFromFilename(const std::string& file_path, const bool match_against_all_encodings)
{
    const std::string extension = PortableFunctions::PathGetFileExtension(file_path);

    if( FileExtensions::IsExtensionHtml(extension) )
        return Encoding::Html;

    // prior to CSPro 8.1, the only encoding supported was HTML
    return !match_against_all_encodings                          ? Encoding::None :
           SO::EqualsNoCase(extension, FileExtensions::Markdown) ? Encoding::Markdown:
           SO::EqualsNoCase(extension, FileExtensions::CSV)      ? Encoding::Csv :
                                                                   Encoding::None;
}


ReportFile ReportFile::CreateFromJson(const JsonNode& json_node,
                                      const std::function<std::shared_ptr<TextSource>(const std::string& file_path)>& text_source_creator/* = { }*/)
{
    std::string name = SO::ToUpper(json_node.Get<std::string_view>(JK::name));

    std::string file_path = json_node.Contains(JK::filename) ? json_node.GetAbsolutePath(JK::filename) :
                                                               json_node.GetAbsolutePath(JK::path);

    const Encoding encoding = json_node.Contains(JK::encoding) ? json_node.Get<Encoding>(JK::encoding) :
                              json_node.Contains("escapeType") ? json_node.Get<Encoding>("escapeType") : // used in CSPro 8.0 only
                                                                 GetDefaultEncodingFromFilename(file_path, false);

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

    return ReportFile(std::move(name), encoding, std::move(text_source));
}


void ReportFile::WriteJson(JsonWriter& json_writer) const
{
    ASSERT(m_textSource != nullptr);

    json_writer.BeginObject()
               .Write(JK::name, m_name)
               .Write(JK::encoding, m_encoding)
               .WriteRelativePath(JK::path, m_textSource->GetFilePath())
               .EndObject();
}


void ReportFile::serialize(Serializer& ar)
{
    ASSERT(ar.IsLoading() == ( m_textSource == nullptr ));

    if( ar.IsLoading() )
        m_textSource = std::make_unique<TextSource>();

    ar & m_name;

    if( ar.MeetsVersionIteration(Serializer::Iteration_8_1_000_1) )
    {
        ar.SerializeEnum(m_encoding);
        ar & *m_textSource;
    }

    else
    {
        ar & *m_textSource;
        m_encoding = GetDefaultEncodingFromFilename(m_textSource->GetFilePath(), false);
    }
}
