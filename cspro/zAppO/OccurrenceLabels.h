#pragma once

#include <zAppO/zAppO.h>
#include <zAppO/LabelSet.h>

class CSpecFile;


// a class for maintaining occurrence labels (occurrence are zero-based)

class ZAPPO_API OccurrenceLabels
{
public:
    const std::vector<LabelSet>& GetLabels() const { return m_labels; }
    std::vector<LabelSet>& GetLabels()             { return m_labels; }

    const CString& GetLabel(size_t occurrence) const
    {
        return ( occurrence < m_labels.size() ) ? m_labels[occurrence].GetLabel() :
                                                  SO::EmptyCString;
    }

    const CString& GetLabel(size_t occurrence, size_t language_index) const
    {
        return ( occurrence < m_labels.size() ) ? m_labels[occurrence].GetLabel(language_index) :
                                                  SO::EmptyCString;
    }

    const LabelSet& GetLabelSet(size_t occurrence) const;


    void SetLabel(size_t occurrence, CString label, size_t language_index = SIZE_MAX)
    {
        GetOrCreateLabelSet(occurrence).SetLabel(std::move(label), language_index);
    }

    void SetLabels(size_t occurrence, const LabelSet& label_set)
    {
        GetOrCreateLabelSet(occurrence).SetLabels(label_set);
    }


    void DeleteOccurrencesBeyond(size_t occurrences)
    {
        if( m_labels.size() > occurrences )
            m_labels.resize(occurrences);
    }


    // serialization
    static OccurrenceLabels CreateFromJson(const JsonNode<wchar_t>& json_node, size_t max_occurrences = SIZE_MAX);
    void WriteJson(JsonWriter& json_writer, size_t max_occurrences = SIZE_MAX) const;

    void serialize(Serializer& ar);

private:
    LabelSet& GetOrCreateLabelSet(size_t occurrence);

private:
    std::vector<LabelSet> m_labels;
};
