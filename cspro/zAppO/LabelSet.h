#pragma once

#include <zAppO/zAppO.h>


// --------------------------------------------------------------------------
// LabelSet
//
// A class for maintaining a set of labels (to support multiple languages).
// --------------------------------------------------------------------------

class ZAPPO_API LabelSet
{
    friend class OccurrenceLabels;

private:
    LabelSet(std::vector<CString> labels);

public:
    LabelSet(CString label = CString());

    static LabelSet DefaultValue;

    size_t GetCurrentLanguageIndex() const               { return m_languageIndex; }
    void SetCurrentLanguage(size_t language_index) const { m_languageIndex = language_index; }

    const CString& GetLabel() const;
    const CString& GetLabel(size_t language_index, bool use_primary_label_if_undefined = true) const;
    const std::vector<CString>& GetLabels() const { return m_labels; }

    void SetLabel(CString label, size_t language_index = SIZE_MAX);
    void SetLabels(std::vector<CString> labels);
    void SetLabels(const LabelSet& label_set);

    void DeleteLabel(size_t language_index);
    void DeleteLabelsBeyond(size_t number_languages);

    // Equality operators check that labels are equal, but do not check the current language index.
    // To be equal, the labels must be specified in the same order.
    bool operator==(const LabelSet& rhs) const;
    bool operator!=(const LabelSet& rhs) const noexcept { return !( *this == rhs ); }

    // serialization
    static LabelSet CreateFromJson(const JsonNode& json_node);
    void WriteJson(JsonWriter& json_writer) const;

    void serialize(Serializer& ar);

private:
    static std::vector<CString> Pre80SerializableLabelToLabels(std::string_view serializable_label_sv);

private:
    std::vector<CString> m_labels;
    mutable size_t m_languageIndex;
};



// --------------------------------------------------------------------------
// inline implementations
// --------------------------------------------------------------------------

inline LabelSet::LabelSet(std::vector<CString> labels)
    :   m_labels(std::move(labels)),
        m_languageIndex(0)
{
}


inline LabelSet::LabelSet(CString label/* = CString()*/)
    :   m_languageIndex(0)
{
    m_labels.emplace_back(std::move(label));
}


inline const CString& LabelSet::GetLabel() const
{
    ASSERT(!m_labels.empty());
    return ( m_languageIndex == 0 ) ? m_labels.front() : GetLabel(m_languageIndex);
}


inline void LabelSet::SetLabels(std::vector<CString> labels)
{
    ASSERT(!labels.empty());
    m_labels = std::move(labels);
}


inline void LabelSet::SetLabels(const LabelSet& label_set)
{
    SetLabels(label_set.m_labels);
}


inline void LabelSet::DeleteLabelsBeyond(const size_t number_languages)
{
    if( m_labels.size() > number_languages )
        m_labels.resize(number_languages);
}


inline bool LabelSet::operator==(const LabelSet& rhs) const
{
    return ( m_labels == rhs.m_labels );
}
