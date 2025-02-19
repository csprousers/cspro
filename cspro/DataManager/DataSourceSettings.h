#pragma once


class DataSourceSettings
{
public:
    DataSourceSettings();

    bool HasUsableDictionaryFilePath() const;
    const std::string& GetDictionaryFilePath() const { return m_dictionaryFilePath; }
    void SetDictionaryFilePath(std::string dictionary_file_path);

    const std::string& GetLanguageName() const      { return m_languageName; }
    void SetLanguageName(std::string language_name) { m_languageName = std::move(language_name); }

    std::optional<int> GetCaseListingWidth() const { return m_caseListingWidth; }
    void SetCaseListingWidth(int width)            { m_caseListingWidth = width; }

    UINT GetDefaultCasePageCommandId() const { return m_defaultCasePageCommandId; }
    void SetDefaultCasePageCommandId(int id) { m_defaultCasePageCommandId = id; }

    static DataSourceSettings CreateFromJson(const JsonNode& json_node);
    void WriteJson(JsonWriter& json_writer) const;

private:
    std::string m_dictionaryFilePath;
    int64_t m_dictionaryFileModifiedTime;

    std::string m_languageName;

    std::optional<int> m_caseListingWidth;

    UINT m_defaultCasePageCommandId;
};
