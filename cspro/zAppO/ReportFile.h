#pragma once

#include <zAppO/zAppO.h>
#include <zUtilO/TextSource.h>


// --------------------------------------------------------------------------
// ReportFile
// --------------------------------------------------------------------------

class ZAPPO_API ReportFile
{
public:
    enum class EscapeType : int { None, Html, Csv };

    ReportFile(std::string name, EscapeType escape_type, std::shared_ptr<TextSource> text_source);
    ReportFile();

    const std::string& GetName() const { return m_name; }
    void SetName(std::string name)     { m_name = std::move(name); }

    EscapeType GetEscapeType() const           { return m_escapeType; }
    void SetEscapeType(EscapeType escape_type) { m_escapeType = escape_type; }

    const TextSource& GetTextSource() const           { ASSERT(m_textSource != nullptr); return *m_textSource; }
    TextSource& GetTextSource()                       { ASSERT(m_textSource != nullptr); return *m_textSource; }
    std::shared_ptr<TextSource> GetSharedTextSource() { return m_textSource; }

    const std::string& GetFilePath() const { ASSERT(m_textSource != nullptr); return m_textSource->GetFilePath(); }

    static EscapeType GetDefaultEscapeTypeFromFilename(const std::string& file_path, bool match_against_all_escape_types);

    // serialization
    // --------------------------------------------------------------------------
    static ReportFile CreateFromJson(const JsonNode& json_node,
                                    const std::function<std::shared_ptr<TextSource>(const std::string& file_path)>& text_source_creator = { });
    void WriteJson(JsonWriter& json_writer) const;

    void serialize(Serializer& ar);

private:
    std::string m_name;
    EscapeType m_escapeType;
    std::shared_ptr<TextSource> m_textSource;
};



DECLARE_ENUM_JSON_SERIALIZER_CLASS(ReportFile::EscapeType, ZAPPO_API)
