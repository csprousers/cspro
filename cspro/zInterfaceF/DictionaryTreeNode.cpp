#include "StdAfx.h"
#include "DictionaryTreeNode.h"
#include <zToolsO/Tools.h>


namespace
{
    constexpr int IconResources[] =
    {
        IDI_DICTIONARY,
        IDI_DICTIONARY_LEVEL,
        IDI_DICTIONARY_ID_RECORD,
        IDI_DICTIONARY_RECORD,
        IDI_DICTIONARY_ITEM,  
        IDI_DICTIONARY_SUBITEM,
        IDI_DICTIONARY_VALUESET
    };
}


template<typename T>
DictionaryTreeNode::DictionaryTreeNode(int icon_resource, const T& dict_base)
    :   m_iconResource(icon_resource),
        m_dictBase(dict_base),
        m_name(CS2WS(dict_base.GetName()))
{
}


DictionaryTreeNode::DictionaryTreeNode(const CDataDict& dictionary)
    :   DictionaryTreeNode(IDI_DICTIONARY, dictionary)
{
}


DictionaryTreeNode::DictionaryTreeNode(const DictLevel& dict_level)
    :   DictionaryTreeNode(IDI_DICTIONARY_LEVEL, dict_level)
{
}


DictionaryTreeNode::DictionaryTreeNode(const CDictRecord& dict_record, const bool id_record)
    :   DictionaryTreeNode(id_record ? IDI_DICTIONARY_ID_RECORD : IDI_DICTIONARY_RECORD, dict_record)
{
}


DictionaryTreeNode::DictionaryTreeNode(const CDictItem& dict_item, const std::optional<size_t>& occurrence)
    :   DictionaryTreeNode(( dict_item.GetItemType() == ItemType::Item ) ? IDI_DICTIONARY_ITEM : IDI_DICTIONARY_SUBITEM, dict_item)
{
    m_itemOccurrenceInfo.emplace(&dict_item, occurrence);
}


DictionaryTreeNode::DictionaryTreeNode(const DictValueSet& dict_value_set, const CDictItem& parent_dict_item,
                                       const std::optional<size_t>& occurrence)
    :   DictionaryTreeNode(IDI_DICTIONARY_VALUESET, dict_value_set)
{
    m_itemOccurrenceInfo.emplace(&parent_dict_item, occurrence);
}


std::optional<AppFileType> DictionaryTreeNode::GetAppFileType() const
{
    if( GetDictElementType() == DictElementType::Dictionary )
        return AppFileType::Dictionary;

    return std::nullopt;
}


std::wstring DictionaryTreeNode::GetNameOrLabelWithOccurrenceInfo(const NameOrLabelType name_or_label_type) const
{
    std::wstring text = ( name_or_label_type == NameOrLabelType::Label ) ? CS2WS(m_dictBase.GetLabel()) :
                                                                           m_name;

    // value sets should use their parent item's name for logic
    if( name_or_label_type == NameOrLabelType::LogicName && GetDictElementType() == DictElementType::ValueSet )
        text = CS2WS(std::get<0>(*m_itemOccurrenceInfo)->GetName());

    // add the occurrence information
    if( m_itemOccurrenceInfo.has_value() && std::get<1>(*m_itemOccurrenceInfo).has_value() )
    {
        const size_t occurrence = *std::get<1>(*m_itemOccurrenceInfo);
        std::wstring occurrence_label;

        if( name_or_label_type == NameOrLabelType::Label )
        {
            const CDictItem* dict_item_with_occurrence_labels = std::get<0>(*m_itemOccurrenceInfo);

            if( dict_item_with_occurrence_labels->GetOccurs() == 1 )
                dict_item_with_occurrence_labels = dict_item_with_occurrence_labels->GetParentItem();
        
            occurrence_label = CS2WS(dict_item_with_occurrence_labels->GetOccurrenceLabels().GetLabel(occurrence));
        }

        if( occurrence_label.empty() )
            occurrence_label = CS2WS(IntToString(occurrence + 1));

        if( name_or_label_type == NameOrLabelType::Label )
            text.push_back(' ');

        SO::AppendFormat(text, _T("(%s)"), occurrence_label.c_str());
    }

    return text;
}


template<typename T>
const T& DictionaryTreeNode::GetElement() const
{
    const T* element = dynamic_cast<const T*>(&m_dictBase);

    if( element == nullptr )
        throw ProgrammingErrorException();

    return *element;
}

template CLASS_DECL_ZINTERFACEF const CDictItem& DictionaryTreeNode::GetElement<CDictItem>() const;
template CLASS_DECL_ZINTERFACEF const DictValueSet& DictionaryTreeNode::GetElement<DictValueSet>() const;


const CDictItem* DictionaryTreeNode::GetAssociatedDictItem() const
{
    return m_itemOccurrenceInfo.has_value() ? std::get<0>(*m_itemOccurrenceInfo) :
                                              nullptr;
}


const std::tuple<const CDictItem*, std::optional<size_t>>& DictionaryTreeNode::GetDictItemOccurrenceInfo() const
{
    if( m_itemOccurrenceInfo.has_value() )
        return *m_itemOccurrenceInfo;

    throw ProgrammingErrorException();
}


std::unique_ptr<CImageList> DictionaryTreeNode::CreateImageList()
{
    auto image_list = std::make_unique<CImageList>();

    image_list->Create(16, 16, ILC_COLOR32, 0, 2);
    image_list->SetBkColor(GetSysColor(COLOR_WINDOW));

    for( int i = 0; i < _countof(IconResources); ++i )
    {
        HICON icon = AfxGetApp()->LoadIcon(IconResources[i]);
        image_list->Add(icon);
    }

    return image_list;
}


int DictionaryTreeNode::GetImageListIndex() const
{
    for( int i = 0; i < _countof(IconResources); ++i )
    {
        if( m_iconResource == IconResources[i] )
            return i;
    }

    throw ProgrammingErrorException();
}
