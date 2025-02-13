#pragma once

#include <zInterfaceF/zInterfaceF.h>
#include <zInterfaceF/TreeNode.h>
#include <zDictO/DDClass.h>


class CLASS_DECL_ZINTERFACEF DictionaryTreeNode : public TreeNode
{
private:
    template<typename T>
    DictionaryTreeNode(int icon_resource, const T& dict_base);

public:
    DictionaryTreeNode(const CDataDict& dictionary);

    DictionaryTreeNode(const DictLevel& dict_level);

    DictionaryTreeNode(const CDictRecord& dict_record, bool id_record);

    DictionaryTreeNode(const CDictItem& dict_item, const std::optional<size_t>& occurrence);

    DictionaryTreeNode(const DictValueSet& dict_value_set, const CDictItem& parent_dict_item,
                       const std::optional<size_t>& occurrence);

    // TreeNode overrides
    std::optional<AppFileType> GetAppFileType() const override;

    std::wstring GetName() const override  { return GetNameOrLabelWithOccurrenceInfo(NameOrLabelType::Name); }
    std::wstring GetLabel() const override { return GetNameOrLabelWithOccurrenceInfo(NameOrLabelType::Label); }

    // other methods
    DictElementType GetDictElementType() const { return m_dictBase.GetElementType(); }

    std::wstring GetLogicName() const { return GetNameOrLabelWithOccurrenceInfo(NameOrLabelType::LogicName); }

    // a helper method for getting the dictionary object from a dictionary tree element;
    // throws an exception if the cast is invalid
    template<typename T>
    const T& GetElement() const;

    // returns null if this is not an item
    const CDictItem* GetAssociatedDictItem() const;

    // throws an exception if this isn't an item or value set
    const std::tuple<const CDictItem*, std::optional<size_t>>& GetDictItemOccurrenceInfo() const;

    static std::unique_ptr<CImageList> CreateImageList();

    int GetImageListIndex() const;

private:
    enum class NameOrLabelType { Name, LogicName, Label };
    std::wstring GetNameOrLabelWithOccurrenceInfo(NameOrLabelType name_or_label_type) const;

private:
    int m_iconResource;
    const DictBase& m_dictBase;
    std::wstring m_name;
    std::optional<std::tuple<const CDictItem*, std::optional<size_t>>> m_itemOccurrenceInfo;
};
