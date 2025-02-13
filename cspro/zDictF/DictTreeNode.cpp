#include "StdAfx.h"
#include "DictTreeNode.h"


// --------------------------------------------------------------------------
// DictTreeNode
// --------------------------------------------------------------------------

DictTreeNode::DictTreeNode(DictElementType dict_element_type)
    :   m_dictElementType(dict_element_type),
        m_relationIndex(NONE),
        m_levelIndex(NONE), 
        m_recordIndex(NONE),
        m_itemIndex(NONE),
        m_valueSetIndex(NONE),
        m_itemOccurs(NONE)
{
}


std::optional<AppFileType> DictTreeNode::GetAppFileType() const
{
    if( GetDictElementType() == DictElementType::Dictionary )
        return AppFileType::Dictionary;

    return std::nullopt;
}


std::wstring DictTreeNode::GetNameOrLabel(bool name) const
{
    ASSERT(GetDDDoc() != nullptr);
    const CDataDict& dictionary = GetDDDoc()->GetDictionary();

    auto get =
        [&](const DictNamedBase& dict_element)
        {
            return CS2WS(name ? dict_element.GetName() :
                                dict_element.GetLabel());
        };

    if( m_dictElementType == DictElementType::Dictionary )
        return get(dictionary);

    if( m_dictElementType == DictElementType::Relation )
        return CS2WS(dictionary.GetRelation(m_relationIndex).GetName());

    const DictLevel& dict_level = dictionary.GetLevel(m_levelIndex);
    if( m_dictElementType == DictElementType::Level )
        return get(dict_level);

    const CDictRecord& dict_record = *dict_level.GetRecord(m_recordIndex);
    if( m_dictElementType == DictElementType::Record )
        return get(dict_record);

    const CDictItem& dict_item = *dict_record.GetItem(m_itemIndex);
    if( m_dictElementType == DictElementType::Item )
        return get(dict_item);

    if( m_dictElementType == DictElementType::ValueSet )
        return get(dict_item.GetValueSet(m_valueSetIndex));

    throw ProgrammingErrorException();
}


bool DictTreeNode::IsSubitem() const
{
    if( m_levelIndex == NONE || m_recordIndex == NONE || m_valueSetIndex != NONE )
        return false;

    const CDictItem* dict_item = GetDDDoc()->GetDictionary().GetLevel(m_levelIndex).GetRecord(m_recordIndex)->GetItem(m_itemIndex);
    return ( dict_item->GetItemType() == ItemType::Subitem );
}



// --------------------------------------------------------------------------
// DictionaryDictTreeNode
// --------------------------------------------------------------------------

DictionaryDictTreeNode::DictionaryDictTreeNode(std::wstring dictionary_filename, std::wstring label)
    :   DictTreeNode(DictElementType::Dictionary),
        m_dictionaryFilename(std::move(dictionary_filename)),
        m_label(std::move(label)),
        m_refCount(0)
{
}
