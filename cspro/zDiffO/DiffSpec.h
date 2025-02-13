#pragma once

#include <zDiffO/zDiffO.h>

class CDataDict;
class CDictItem;
namespace JsonSpecFile { class ReaderMessageLogger; }


class ZDIFFO_API DiffSpec
{
public:
    enum class DiffMethod { OneWay, BothWays };
    enum class DiffOrder { Indexed, Sequential };
    enum class ItemDisplay { Labels, Names };
    enum class ItemSerialization { Included, Excluded };

public:
    DiffSpec();

    // Load and Save throw exceptions
    void Load(const std::wstring& filename, bool silent, std::shared_ptr<const CDataDict> embedded_dictionary = nullptr);
    void Load(const JsonNode<wchar_t>& json_node, bool silent, std::shared_ptr<const CDataDict> embedded_dictionary = nullptr,
              std::shared_ptr<JsonSpecFile::ReaderMessageLogger> message_logger = nullptr);
    void Save(const std::wstring& filename) const;

    bool IsDictionaryDefined() const                             { return ( m_dictionary != nullptr ); }
    const CDataDict& GetDictionary() const                       { ASSERT(IsDictionaryDefined()); return *m_dictionary; }
    std::shared_ptr<const CDataDict> GetSharedDictionary() const { return m_dictionary; }

    DiffMethod GetDiffMethod() const  { return m_diffMethod; }
    DiffOrder GetDiffOrder() const    { return m_diffOrder; }
    bool GetShowLabels() const        { return ( m_itemDisplay == ItemDisplay::Labels ); }
    bool GetSaveExcludedItems() const { return ( m_itemSerialization == ItemSerialization::Excluded ); }

    const std::vector<std::tuple<const CDictItem*, std::optional<size_t>>>& GetDiffItems() const { return m_selectedItemsAndOccurrences; }

    // methods used by CSDiff
    void SetDictionary(std::shared_ptr<const CDataDict> dictionary);

    void SetDiffMethod(DiffMethod diff_method)          { m_diffMethod = diff_method; }
    void SetDiffOrder(DiffOrder diff_order)             { m_diffOrder = diff_order; }
    void SetShowLabels(bool show_labels)                { m_itemDisplay = show_labels ? ItemDisplay::Labels : ItemDisplay::Names; }
    void SetSaveExcludedItems(bool save_excluded_items) { m_itemSerialization = save_excluded_items ? ItemSerialization::Excluded : ItemSerialization::Included; }

    bool IsItemSelected(const std::tuple<const CDictItem*, std::optional<size_t>>& item_and_occurrence) const;
    void SetItemSelection(const std::tuple<const CDictItem*, std::optional<size_t>>& item_and_occurrence, bool selection);

private:
    std::vector<std::tuple<const CDictItem*, std::optional<size_t>>> GetSortedItemsAndOccurrences() const;

    static std::wstring ConvertPre80SpecFile(const std::wstring& filename);

private:
    std::shared_ptr<const CDataDict> m_dictionary;

    DiffMethod m_diffMethod;
    DiffOrder m_diffOrder;
    ItemDisplay m_itemDisplay;
    ItemSerialization m_itemSerialization;

    std::vector<std::tuple<const CDictItem*, std::optional<size_t>>> m_selectedItemsAndOccurrences;
};
