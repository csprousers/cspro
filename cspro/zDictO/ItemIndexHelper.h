#pragma once

#include <zDictO/zDictO.h>
#include <zDictO/ItemIndex.h>

class CDictItem;


class CLASS_DECL_ZDICTO ItemIndexHelper
{
public:
    ItemIndexHelper(const CDictItem& dict_item);

    bool HasOccurrences() const { return ( m_numberMinimalOccurrences != 0 ); }

    bool HasRecordOccurrences() const      { return m_hasRecordOccurrences; }
    size_t GetMaxRecordOccurrences() const { return m_maxRecordOccurrences; }

    bool IsSubitem() const { return m_isSubitem; }

    bool HasItemSubitemOccurrences() const      { return m_hasItemSubitemOccurrences; }
    bool HasItemOccurrences() const             { return m_hasItemOccurrences; }
    bool HasSubitemOccurrences() const          { return m_hasSubitemOccurrences; }
    size_t GetMaxItemSubitemOccurrences() const { return m_maxItemSubitemOccurrences; }

    // Gets one-based occurrences for serializing to a format suitable to CSPro users.
    std::vector<size_t> GetOneBasedOccurrences(const ItemIndex& item_index) const
    {
        return
        {
            item_index.GetRecordOccurrence() + 1,
            item_index.GetItemOccurrence() + 1,
            HasSubitemOccurrences() ? ( item_index.GetSubitemOccurrence() + 1 ) : 0
        };
    }

    // Returns whether the occurrences are valid.
    bool IsValid(const ItemIndex& item_index) const;

    // Gets text representing the full set of occurrences.
    std::string GetFullOccurrencesText(const ItemIndex& item_index) const;

    // Gets text representing the minimal set of occurrences.
    std::string GetMinimalOccurrencesText(const ItemIndex& item_index) const;

    // Parses the occurrence text (which can include parentheses) and returns the number of minimally specified occurrences.
    // The occurrences are not validated against the dictionary item limits. The return value:
    // - 0: the occurrence text is not valid;
    // - 1: a record occurrence was specified, or an item/subitem occurrence if the item/subitem repeats;
    // - 2: record and item/subitem occurrences were specified
    unsigned ParseOccurrencesFromText(std::string_view occurrences_text_sv, size_t occurrences[ItemIndex::NumberDimensions]) const;

    // Set the occurrences from the supplied text (which can include parentheses).
    // This routine works with either full or minimal occurrences and returns true if the occurrences are valid.
    bool SetOccurrencesFromText(ItemIndex& item_index, std::string_view occurrences_text_sv) const;

    // Writes the occurrences in JSON format.
    enum class WriteJsonMode { FullOccurrences, MinimalOccurrences, ApplicableOccurrences };
    void WriteJson(JsonWriter& json_writer, const ItemIndex& item_index, WriteJsonMode mode) const;

private:
    const size_t m_maxRecordOccurrences;
    const bool m_hasRecordOccurrences;

    const size_t m_maxItemSubitemOccurrences;
    const bool m_hasItemSubitemOccurrences;

    const bool m_isSubitem;
    const bool m_hasItemOccurrences;
    const bool m_hasSubitemOccurrences;

    const size_t m_numberMinimalOccurrences;
};
