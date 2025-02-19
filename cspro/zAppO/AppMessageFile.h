#pragma once

#include <zAppO/zAppO.h>
#include <zUtilO/TextSource.h>


// --------------------------------------------------------------------------
// AppMessageFile
// --------------------------------------------------------------------------

class ZAPPO_API AppMessageFile
{
    friend class Application;

public:
    enum class Type { User, System };

    AppMessageFile(Type type, std::shared_ptr<TextSource> text_source);
    AppMessageFile();

    Type GetType() const    { return m_type; }
    void SetType(Type type) { m_type = type; }

    const TextSource& GetTextSource() const                       { ASSERT(m_textSource != nullptr); return *m_textSource; }
    TextSource& GetTextSource()                                   { ASSERT(m_textSource != nullptr); return *m_textSource; }
    std::shared_ptr<const TextSource> GetSharedTextSource() const { return m_textSource; }
    std::shared_ptr<TextSource> GetSharedTextSource()             { return m_textSource; }

    const std::string& GetFilePath() const { ASSERT(m_textSource != nullptr); return m_textSource->GetFilePath(); }

    // serialization
    // --------------------------------------------------------------------------
    static AppMessageFile CreateFromJson(const JsonNode& json_node,
                                         const std::function<std::shared_ptr<TextSource>(const std::string& file_path)>& text_source_creator = { });
    void WriteJson(JsonWriter& json_writer) const;

    void serialize(Serializer& ar);

private:
    static Type GetTypeFromFilename(const std::string& file_path);

private:
    Type m_type;
    std::shared_ptr<TextSource> m_textSource;
};
