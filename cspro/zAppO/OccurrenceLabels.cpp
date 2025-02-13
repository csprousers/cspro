#include "stdafx.h"
#include "OccurrenceLabels.h"


LabelSet& OccurrenceLabels::GetOrCreateLabelSet(size_t occurrence)
{
    if( m_labels.size() <= occurrence )
        m_labels.resize(occurrence + 1);

    return m_labels[occurrence];
}


const LabelSet& OccurrenceLabels::GetLabelSet(size_t occurrence) const
{
    return ( occurrence < m_labels.size() ) ? m_labels[occurrence] :
                                              LabelSet::DefaultValue;
}


OccurrenceLabels OccurrenceLabels::CreateFromJson(const JsonNode<wchar_t>& json_node, size_t max_occurrences/* = SIZE_MAX*/)
{
    ASSERT(max_occurrences != SIZE_MAX);

    OccurrenceLabels occurrence_labels;    

    for( const auto& array_node : json_node.GetArray() )
    {
        LabelSet label_set = array_node.Get<LabelSet>(JK::labels);
        size_t occurrence = array_node.Get<size_t>(JK::occurrence) - 1; // one-based occurrences

        if( occurrence > max_occurrences )
        {
            json_node.LogWarning(_T("The label for occurrence '%d' is greater than the maximum number of occurrences '%d' and is ignored: '%s'"),
                                 (int)occurrence, (int)max_occurrences, label_set.GetLabel().GetString());
        }

        else
        {
            // only add non-blank labels
            if( !label_set.GetLabel(0).IsEmpty() )
                occurrence_labels.GetOrCreateLabelSet(occurrence) = std::move(label_set);
        }
    }

    return occurrence_labels;
}


void OccurrenceLabels::WriteJson(JsonWriter& json_writer, size_t max_occurrences/* = SIZE_MAX*/) const
{
    if( max_occurrences == SIZE_MAX )
    {
        // if occurrence labels are used for large numbers of occurrences
        // (e.g., if the dictionary record's max occurrences value becomes optional),
        // then this class should store occurrences using a map instead of a vector
        ASSERT(false);
        max_occurrences = m_labels.size();
    }

    json_writer.BeginArray();

    for( size_t i = 0; i < max_occurrences; ++i )
    {
        bool label_exists = ( i < m_labels.size() );

        // unless in verbose mode, only save occurrences that have defined labels
        if( label_exists && !json_writer.Verbose() )
        {
            if( m_labels[i].GetLabel().IsEmpty() )
                continue;
        }

        json_writer.BeginObject()
                   .Write(JK::occurrence, i + 1) // one-based occurrences
                   .Write(JK::labels, label_exists ? m_labels[i] : LabelSet::DefaultValue)
                   .EndObject();
    }

    json_writer.EndArray();
}


void OccurrenceLabels::serialize(Serializer& ar)
{
    if( ar.MeetsVersionIteration(Serializer::Iteration_8_0_000_1) )
    {
        ar & m_labels;
    }

    else
    {
        for( const std::wstring& occ_label : ar.Read<std::vector<std::wstring>>() )
            m_labels.emplace_back(LabelSet(LabelSet::Pre80SerializableLabelToLabels(occ_label)));
    }
}
