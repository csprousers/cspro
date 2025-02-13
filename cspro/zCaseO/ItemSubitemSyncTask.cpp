#include "stdafx.h"
#include "ItemSubitemSyncTask.h"
#include "FixedWidthNumericWithStringBufferCaseItem.h"
#include "FixedWidthStringCaseItem.h"
#include <mutex>


ItemSubitemSyncTask::ItemSubitemSyncTask(CaseItem& parent_case_item)
    :   m_parentCaseItem(parent_case_item)
{
    ASSERT(parent_case_item.GetDictionaryItem().GetItemType() == ItemType::Item);
    ASSERT(parent_case_item.GetType() == CaseItem::Type::FixedWidthNumericWithStringBuffer ||
           parent_case_item.GetType() == CaseItem::Type::FixedWidthString);
}


void ItemSubitemSyncTask::AddSubitem(CaseItem& subitem_case_item)
{
    ASSERT(subitem_case_item.GetDictionaryItem().GetItemType() == ItemType::Subitem);
    ASSERT(subitem_case_item.GetType() == CaseItem::Type::FixedWidthNumericWithStringBuffer ||
           subitem_case_item.GetType() == CaseItem::Type::FixedWidthString);

    m_subitemCaseItems.emplace_back(&subitem_case_item);
}


void ItemSubitemSyncTask::Do(const CaseItem& modified_case_item, CaseItemIndex& index)
{
    // because items and subitems have this task, information in this vector keeps
    // the setting routines from causing endless recursion
    static std::vector<const CaseRecord*> case_records_being_processed;
    static std::mutex case_records_being_processed_mutex; 
    const CaseRecord* case_record = &(index.GetCaseRecord());

    if( std::find(case_records_being_processed.begin(), case_records_being_processed.end(), case_record) != case_records_being_processed.end() )
        return;

    std::lock_guard<std::mutex> lock(case_records_being_processed_mutex);

    case_records_being_processed.emplace_back(case_record);

    // get the parent's text value
    const CDictItem& parent_dictionary_item = m_parentCaseItem.GetDictionaryItem();
    bool modified_case_item_is_parent = ( &modified_case_item == &m_parentCaseItem );

    CaseItemIndex parent_index = index;

    if( !modified_case_item_is_parent )
        parent_index.SetSubitemOccurrence(0);

    CString parent_text_value = ( m_parentCaseItem.GetType() == CaseItem::Type::FixedWidthString ) ?
        assert_cast<const FixedWidthStringCaseItem&>(m_parentCaseItem).GetValue(parent_index) :
        assert_cast<const FixedWidthNumericWithStringBufferCaseItem&>(m_parentCaseItem).GetStringBufferValue(parent_index);


    // if a subitem was modified, modify the parent item's text value and then have the parent item
    // modify all subitems using its new text value
    if( !modified_case_item_is_parent )
    {
        const CDictItem& subitem_dictionary_item = modified_case_item.GetDictionaryItem();

        // get the subitem's offset in the parent item
        size_t subitem_offset = subitem_dictionary_item.GetStart() - parent_dictionary_item.GetStart();
        subitem_offset += index.GetSubitemOccurrence() * subitem_dictionary_item.GetLen();

        // adjust the parent's text value
        TCHAR* parent_text_value_buffer = parent_text_value.GetBuffer() + subitem_offset;
        dynamic_cast<const FixedWidthCaseItem&>(modified_case_item).OutputFixedValue(index, parent_text_value_buffer);
        parent_text_value.ReleaseBuffer(parent_text_value.GetLength());

        // update the parent item's value
        if( m_parentCaseItem.GetType() == CaseItem::Type::FixedWidthString )
        {
            assert_cast<const FixedWidthStringCaseItem&>(m_parentCaseItem).SetValue(parent_index, parent_text_value);
        }

        else
        {
            assert_cast<const FixedWidthNumericWithStringBufferCaseItem&>(m_parentCaseItem).SetValueFromTextInput(parent_index, parent_text_value);
        }
    }


    // now adjust all of the subitems (but the one modified, if applicable), based on the new parent item's text value
    CaseItemIndex subitem_index = index;

    for( CaseItem* subitem_case_item_pointer : m_subitemCaseItems )
    {
        // don't process the subitem when that is the value that was initially changed
        if( subitem_case_item_pointer == &modified_case_item )
            continue;

        CaseItem& subitem_case_item = *subitem_case_item_pointer;
        const CDictItem& subitem_dictionary_item = subitem_case_item.GetDictionaryItem();

        // get the subitem's offset in the parent item
        size_t subitem_offset = subitem_dictionary_item.GetStart() - m_parentCaseItem.GetDictionaryItem().GetStart();

        for( subitem_index.ResetSubitemOccurrence(); subitem_index.GetSubitemOccurrence() < subitem_dictionary_item.GetOccurs(); subitem_index.IncrementSubitemOccurrence() )
        {
            CString subitem_text_value = parent_text_value.Mid(subitem_offset, subitem_dictionary_item.GetLen());

            if( subitem_case_item.GetType() == CaseItem::Type::FixedWidthString )
            {
                assert_cast<const FixedWidthStringCaseItem&>(subitem_case_item).SetValue(subitem_index, subitem_text_value);
            }

            else
            {
                assert_cast<const FixedWidthNumericWithStringBufferCaseItem&>(subitem_case_item).SetValueFromTextInput(subitem_index, subitem_text_value);
            }

            subitem_offset += subitem_dictionary_item.GetLen();
        }
    }

    case_records_being_processed.erase(std::find(case_records_being_processed.begin(), case_records_being_processed.end(), case_record));
}
