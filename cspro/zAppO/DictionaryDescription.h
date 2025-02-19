#pragma once

#include <zAppO/zAppO.h>

class CDataDict;


enum class DictionaryType : int { Unknown, Input, External, Output, Working };

ZAPPO_API const char* ToString(DictionaryType dictionary_type);


class ZAPPO_API DictionaryDescription
{
public:
    DictionaryDescription(std::string dictionary_file_path, std::string parent_file_path, DictionaryType dictionary_type,
                          std::optional<bool> include_in_simple_synchronization = std::nullopt,
                          bool include_value_set_images_in_compiled_application = false);
    DictionaryDescription(std::string dictionary_file_path = std::string(), DictionaryType dictionary_type = DictionaryType::Unknown);

    const std::string& GetDictionaryFilePath() const             { return m_dictionaryFilePath; }
    void SetDictionaryFilePath(std::string dictionary_file_path) { m_dictionaryFilePath = std::move(dictionary_file_path); }

    DictionaryType GetDictionaryType() const               { return m_dictionaryType; }
    void SetDictionaryType(DictionaryType dictionary_type) { m_dictionaryType = dictionary_type; }

    const std::string& GetParentFilePath() const         { return m_parentFilePath; }
    void SetParentFilePath(std::string parent_file_path) { m_parentFilePath = std::move(parent_file_path); }

    bool GetIncludeInSimpleSynchronization() const    { return m_includeInSimpleSynchronization; }
    void SetIncludeInSimpleSynchronization(bool flag) { m_includeInSimpleSynchronization = flag; }

    bool GetIncludeValueSetImagesInCompiledApplication() const    { return m_includeValueSetImagesInCompiledApplication; }
    void SetIncludeValueSetImagesInCompiledApplication(bool flag) { m_includeValueSetImagesInCompiledApplication = flag; }

    const CDataDict* GetDictionary() const          { return m_dictionary; }
    void SetDictionary(const CDataDict* dictionary) { m_dictionary = dictionary; }

    // serialization
    static DictionaryDescription CreateFromJson(const JsonNode& json_node);
    void WriteJson(JsonWriter& json_writer) const;
    void serialize(Serializer& ar);

private:
    std::string m_dictionaryFilePath;
    DictionaryType m_dictionaryType;
    std::string m_parentFilePath;
    bool m_includeInSimpleSynchronization;
    bool m_includeValueSetImagesInCompiledApplication;
    const CDataDict* m_dictionary;
};
