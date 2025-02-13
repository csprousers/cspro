#pragma once

class Serializer;


namespace FrequencySetting
{
    constexpr bool IncludeAllRecordOccurrencesWhenDisjoint = true;
}


struct FrequencyEntry
{
    struct OccurrenceDetails
    {
        // occurrence numbers are 0-based
        size_t min_item_subitem_occurrence;
        size_t max_item_subitem_occurrence;

        bool combine_record_occurrences;
        bool disjoint_record_occurrences;
        std::set<size_t> record_occurrences_to_explicitly_display;
        std::set<size_t> record_occurrences_to_explicitly_exclude;

        void serialize(Serializer& ar);
    };

    int symbol_index;
    std::vector<OccurrenceDetails> occurrence_details;
    std::optional<int> breakdown;

    void serialize(Serializer& ar);
};
