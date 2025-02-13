#pragma once

#include <zAppO/zAppO.h>

class CDataDict;
template<typename CharType> class JsonNode;
class JsonWriter;
class Serializer;


enum class DictionaryType : int { Unknown, Input, External, Output, Working };

ZAPPO_API const TCHAR* ToString(DictionaryType dictionary_type);


class ZAPPO_API DictionaryDescription
{
public:
    DictionaryDescription(std::wstring dictionary_filename, std::wstring parent_filename, DictionaryType dictionary_type)
        :   m_dictionaryFilename(std::move(dictionary_filename)),
            m_parentFilename(std::move(parent_filename)),
            m_dictionaryType(dictionary_type),
            m_dictionary(nullptr)
    {
    }

    DictionaryDescription(std::wstring dictionary_filename = std::wstring(), DictionaryType dictionary_type = DictionaryType::Unknown)
        :   DictionaryDescription(std::move(dictionary_filename), std::wstring(), dictionary_type)
    {
    }

    const std::wstring& GetDictionaryFilename() const            { return m_dictionaryFilename; }
    void SetDictionaryFilename(std::wstring dictionary_filename) { m_dictionaryFilename = std::move(dictionary_filename); }

    DictionaryType GetDictionaryType() const               { return m_dictionaryType; }
    void SetDictionaryType(DictionaryType dictionary_type) { m_dictionaryType = dictionary_type; }

    const std::wstring& GetParentFilename() const        { return m_parentFilename; }
    void SetParentFilename(std::wstring parent_filename) { m_parentFilename = std::move(parent_filename); }

    const CDataDict* GetDictionary() const          { return m_dictionary; }
    void SetDictionary(const CDataDict* dictionary) { m_dictionary = dictionary; }

    // serialization
    static DictionaryDescription CreateFromJson(const JsonNode<wchar_t>& json_node);
    void WriteJson(JsonWriter& json_writer) const;
    void serialize(Serializer& ar);

private:
    std::wstring m_dictionaryFilename;
    DictionaryType m_dictionaryType;
    std::wstring m_parentFilename;
    const CDataDict* m_dictionary;
};
