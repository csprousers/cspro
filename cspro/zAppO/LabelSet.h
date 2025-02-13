#pragma once

#include <zAppO/zAppO.h>

template<typename CharType> class JsonNode;
class JsonWriter;
class Serializer;


// a class for maintaining a set of labels (to support multiple languages)

class ZAPPO_API LabelSet
{
    friend class OccurrenceLabels;

private:
    LabelSet(std::vector<CString> labels)
        :   m_labels(std::move(labels)),
            m_languageIndex(0)
    {
    }

public:
    LabelSet(CString label = CString())
        :   LabelSet(std::vector<CString> { std::move(label) })
    {
    }

    static LabelSet DefaultValue;


    size_t GetCurrentLanguageIndex() const               { return m_languageIndex; }
    void SetCurrentLanguage(size_t language_index) const { m_languageIndex = language_index; }


    const CString& GetLabel() const
    {
        ASSERT(!m_labels.empty());
        return ( m_languageIndex == 0 ) ? m_labels.front() : GetLabel(m_languageIndex);
    }

    const CString& GetLabel(size_t language_index, bool use_primary_label_if_undefined = true) const;

    const std::vector<CString>& GetLabels() const
    {
        return m_labels;
    }


    void SetLabel(CString label, size_t language_index = SIZE_MAX);

    void SetLabels(std::vector<CString> labels)
    {
        ASSERT(!labels.empty());
        m_labels = std::move(labels);
    }

    void SetLabels(const LabelSet& label_set)
    {
        SetLabels(label_set.m_labels);
    }


    void DeleteLabel(size_t language_index);

    void DeleteLabelsBeyond(size_t number_languages)
    {
        if( m_labels.size() > number_languages )
            m_labels.resize(number_languages);
    }

    bool operator==(const LabelSet& rhs) const // DD_STD_REFACTOR_TODO is this needed?
    {
        return ( m_labels == rhs.m_labels );
    }

    // serialization
    static LabelSet CreateFromJson(const JsonNode<wchar_t>& json_node);
    void WriteJson(JsonWriter& json_writer) const;

    void serialize(Serializer& ar);

private:
    static std::vector<CString> Pre80SerializableLabelToLabels(wstring_view serializable_label);

private:
    std::vector<CString> m_labels;
    mutable size_t m_languageIndex;
};
