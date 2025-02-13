#include "stdafx.h"
#include "Frequency.h"
#include <zToolsO/Serializer.h>


void Frequency::AddFrequencyEntry(const FrequencyEntry& frequency_entry)
{
    m_frequencyEntries.emplace_back(frequency_entry);

    // calculate if this frequency will only result in a single frequency variable
    m_symbolIndexOfSingleFrequencyVariable.reset();

    if( m_frequencyEntries.size() == 1 )
    {
        // one frequency counter will exist is there are no occurrence details or
        // if the single occurrence detail has combined record occurrences or only a single record occurrence
        bool has_single_frequency = frequency_entry.occurrence_details.empty();

        if( frequency_entry.occurrence_details.size() == 1 )
        {
            const auto& occurrence_details = frequency_entry.occurrence_details.front();
            has_single_frequency =
                occurrence_details.combine_record_occurrences ||
                ( !occurrence_details.disjoint_record_occurrences && occurrence_details.record_occurrences_to_explicitly_display.size() == 1 );
        }

        if( has_single_frequency )
            m_symbolIndexOfSingleFrequencyVariable = frequency_entry.symbol_index;
    }
}


void Frequency::serialize(Serializer& ar)
{
    ar & m_namedFrequencySymbolIndex
       & m_frequencyEntries
       & m_frequencyPrinterOptions
       & m_symbolIndexOfSingleFrequencyVariable;
}

void FrequencyEntry::serialize(Serializer& ar)
{
    ar & symbol_index
       & breakdown
       & occurrence_details;
}

void FrequencyEntry::OccurrenceDetails::serialize(Serializer& ar)
{
    ar & min_item_subitem_occurrence
       & max_item_subitem_occurrence
       & combine_record_occurrences
       & disjoint_record_occurrences
       & record_occurrences_to_explicitly_display
       & record_occurrences_to_explicitly_exclude;
}
