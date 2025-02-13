#pragma once

#include <zDictO/zDictO.h>
#include <zDictO/DDClass.h>
#include <zDataO/DataRepositoryDefines.h>


struct CLASS_DECL_ZDICTO DictionaryDifference
{
    enum class Type
    {
        LevelRemoved,
        LevelAdded,
        RecordRemoved,
        RequiredRecordAdded,
        NonRequiredRecordAdded,
        RecordTypeChanged,
        RecordOccurrencesDecreased,
        RecordOccurrencesIncreased,
        RequiredRecordOccurrenceAdded,
        ItemRemoved,
        ItemAdded,
        ItemLengthChanged,
        ItemValueTruncated,
        ItemContentTypeChangedValidConversionUnnecessary,
        ItemContentTypeChangedValidConversionNeeded,
        ItemContentTypeChangedInvalidSometimes,
        ItemContentTypeChangedInvalidAlways,
        ItemFormattingChanged, // zero fill and decimal character
        ItemMovedOnRecord,
        ItemMovedToDifferentRecord,
        ItemMovedToDifferentRecordOccurrencesDecreased,
        ItemItemSubitemOccurrencesDeceased,
        ItemItemSubitemOccurrencesIncreased,
    };

    Type type;
    const DictNamedBase* initial_dict_element;
    const DictNamedBase* final_dict_element;
    bool key_related;

    bool IsDestructive() const;

    const DictNamedBase* GetDefinedDictElement() const;

    bool IsLevelDifference() const;
    bool IsRecordDifference() const;
    bool IsItemDifference() const;

    static std::wstring GetDisplayName(const DictNamedBase* dict_element, bool show_names = true, size_t display_name_length = SIZE_MAX);
    std::wstring GetDisplayName(bool show_names = true, size_t display_name_length = SIZE_MAX) const;
};


class CLASS_DECL_ZDICTO DictionaryComparer
{
public:
    DictionaryComparer(const CDataDict& initial_dictionary, const CDataDict& final_dictionary);
    virtual ~DictionaryComparer() { }

    const std::vector<DictionaryDifference>& GetDifferences() const { return m_differences; }

    enum class DataStorageCharacteristic { UsesRecordType, UsesFixedWidthAndPositions };

    bool RequiresReformat(const std::vector<DataStorageCharacteristic>* data_storage_characteristics) const;

    std::vector<DictionaryDifference> GetDataRepositorySpecificDifferences(DataRepositoryType data_repository_type);

protected:
    struct ItemPair
    {
        const CDictItem* initial_dict_item;
        const CDictItem* final_dict_item;
        bool item_is_an_id;
    };

private:
    void CompareDictionary();
    void CompareLevel(const DictLevel& initial_dict_level, const DictLevel& final_dict_level);
    void CompareRecord(const CDictRecord* initial_dict_record, const CDictRecord* final_dict_record);
    void CompareItem(const ItemPair& item_pair);
    static std::optional<DictionaryDifference::Type> GetItemContentTypeDifference(ContentType initial_content_type, ContentType final_content_type);

    void AddDifference(DictionaryDifference::Type type, const DictNamedBase* initial_dict_element, const DictNamedBase* final_dict_element);
    void AddDifference(DictionaryDifference::Type type, const ItemPair& item_pair);

protected:
    const CDataDict& m_initialDictionary;
    const CDataDict& m_finalDictionary;
    std::vector<DictionaryDifference> m_differences;
    std::vector<ItemPair> m_matchedItemPairs;
};
