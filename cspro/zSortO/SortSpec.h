#pragma once

#include <zSortO/zSortO.h>

class CDataDict;
class CDictRecord;
namespace JsonSpecFile { class ReaderMessageLogger; }


class ZSORTO_API SortSpec
{
public:
    enum class SortType { Case, CasePlus, Record, RecordUsing };
    enum class SortOrder { Ascending, Descending };

    struct SortItem
    {
        const CDictItem* dict_item;
        SortOrder order;
    };

public:
    SortSpec();
    ~SortSpec();

    // Load and Save throw exceptions
    void Load(const std::wstring& filename, bool silent, std::shared_ptr<const CDataDict> embedded_dictionary = nullptr);
    void Load(const JsonNode<wchar_t>& json_node, bool silent, std::shared_ptr<const CDataDict> embedded_dictionary = nullptr,
              std::shared_ptr<JsonSpecFile::ReaderMessageLogger> message_logger = nullptr);
    void Save(const std::wstring& filename) const;

    SortType GetSortType() const { return m_sortType; }
    bool IsCaseSort() const      { return ( m_sortType == SortType::Case || m_sortType == SortType::CasePlus ); }
    bool IsRecordSort() const    { return !IsCaseSort(); }

    const CDataDict& GetDictionary() const                       { ASSERT(m_dictionary != nullptr); return *m_dictionary; }
    std::shared_ptr<const CDataDict> GetSharedDictionary() const { ASSERT(m_dictionary != nullptr); return m_dictionary; }

    const std::vector<const CDictItem*>& GetPossibleSortableDictItems() { return m_possibleSortableDictItems; }

    const std::vector<SortItem>& GetSortItems() { return m_usedSortItems; }

    bool IsRecordTypeItem(const SortItem& sort_item) const { return ( sort_item.dict_item == m_recordTypeDictItem.get() ); }


    // methods used by CSSort
    void SetDictionary(std::shared_ptr<const CDataDict> dictionary);

    void SetSortType(SortType sort_type, const CDictRecord* record_sort_dict_record = nullptr);

    const CDictRecord* GetRecordSortDictRecord() const { return m_recordSortDictRecord; }

    void SetSortItems(const std::vector<std::tuple<int, SortOrder>>& sort_item_indices_and_orders);


private:
    void ClearSortItems();
    void RefreshPossibleSortItems();

    static std::wstring ConvertPre80SpecFile(const std::wstring& filename);

private:
    std::shared_ptr<const CDataDict> m_dictionary;

    SortType m_sortType;
    const CDictRecord* m_recordSortDictRecord;

    std::unique_ptr<CDictItem> m_recordTypeDictItem;

    std::vector<const CDictItem*> m_possibleSortableDictItems;
    std::vector<SortItem> m_usedSortItems;
};
