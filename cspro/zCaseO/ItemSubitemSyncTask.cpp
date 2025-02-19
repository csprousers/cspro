#include "stdafx.h"
#include "ItemSubitemSyncTask.h"
#include "FixedWidthNumericWithStringBufferCaseItem.h"
#include "FixedWidthStringCaseItem.h"
#include <mutex>


ItemSubitemSyncTask::ItemSubitemSyncTask(CaseItem& parent_case_item)
    :   m_parentCaseItem(parent_case_item)
{
    ASSERT(parent_case_item.GetDictItem().GetItemType() == ItemType::Item);
    ASSERT(parent_case_item.GetType() == CaseItem::Type::FixedWidthNumericWithStringBuffer ||
           parent_case_item.GetType() == CaseItem::Type::FixedWidthString);
}


void ItemSubitemSyncTask::AddSubitem(CaseItem& subitem_case_item)
{
    ASSERT(subitem_case_item.GetDictItem().GetItemType() == ItemType::Subitem);
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
    const CaseRecord& case_record = index.GetCaseRecord();

    if( std::find(case_records_being_processed.cbegin(), case_records_being_processed.cend(), &case_record) != case_records_being_processed.cend() )
        return;

    const std::lock_guard<std::mutex> lock(case_records_being_processed_mutex);

    case_records_being_processed.emplace_back(&case_record);

    // get the parent's text value
    const CDictItem& parent_dict_item = m_parentCaseItem.GetDictItem();
    const bool modified_case_item_is_parent = ( &modified_case_item == &m_parentCaseItem );

    CaseItemIndex parent_index = index;

    if( !modified_case_item_is_parent )
        parent_index.SetSubitemOccurrence(0);

    SharableString parent_text_value = ( m_parentCaseItem.GetType() == CaseItem::Type::FixedWidthString ) ?
        assert_cast<const FixedWidthStringCaseItem&>(m_parentCaseItem).GetString(parent_index) :
        assert_cast<const FixedWidthNumericWithStringBufferCaseItem&>(m_parentCaseItem).GetBufferString(parent_index);
    
    ASSERT(parent_text_value.WideLength() == parent_dict_item.GetLen());


    // if a subitem was modified, modify the parent item's text value and then have the parent item
    // modify all subitems using its new text value
    if( !modified_case_item_is_parent )
    {
        const CDictItem& subitem_dict_item = modified_case_item.GetDictItem();
        const FixedWidthCaseItem& fixed_width_subitem_case_item = dynamic_cast<const FixedWidthCaseItem&>(modified_case_item);

        // get the subitem's offset in the parent item
        size_t subitem_offset = subitem_dict_item.GetStart() - parent_dict_item.GetStart();
        subitem_offset += index.GetSubitemOccurrence() * subitem_dict_item.GetLen();

        // adjust the parent's text value...
        std::string& modifiable_parent_text_value = parent_text_value.MakeModifiable();
        const size_t subitem_start_pos = SO::WideGetOffset(modifiable_parent_text_value, subitem_offset);
        char* subitem_text_start = modifiable_parent_text_value.data() + subitem_start_pos;
        const size_t current_subitem_text_length = SO::WideGetOffset(subitem_text_start, subitem_dict_item.GetLen());
        const size_t max_subitem_text_length = fixed_width_subitem_case_item.GetMaxUtf8FixedValueWidth();

        // ... either in place, when enough room exists
        if( current_subitem_text_length == max_subitem_text_length )
        {
            const size_t output_length = fixed_width_subitem_case_item.OutputFixedValue(index, subitem_text_start);
            const ptrdiff_t length_difference = current_subitem_text_length - output_length;

            if( length_difference != 0 )
            {
                ASSERT(length_difference > 0);
                modifiable_parent_text_value.erase(subitem_start_pos + output_length, length_difference);
            }
        }

        // ...or using a buffer
        else
        {
            ASSERT(current_subitem_text_length < max_subitem_text_length);
            auto buffer = std::make_unique_for_overwrite<char[]>(max_subitem_text_length);
            const size_t output_length = fixed_width_subitem_case_item.OutputFixedValue(index, buffer.get());

            modifiable_parent_text_value.replace(subitem_start_pos, current_subitem_text_length, buffer.get(), output_length);
        }

        // update the parent item's value
        if( m_parentCaseItem.GetType() == CaseItem::Type::FixedWidthString )
        {
            assert_cast<const FixedWidthStringCaseItem&>(m_parentCaseItem).SetValue(parent_index, parent_text_value);
        }

        else
        {
            assert_cast<const FixedWidthNumericWithStringBufferCaseItem&>(m_parentCaseItem).SetValueFromTextInput(parent_index, UTF8_TODO::GetWide(*parent_text_value).c_str());
        }
    }


    // now adjust all of the subitems (but the one modified, if applicable), based on the new parent item's text value
    CaseItemIndex subitem_index = index;

    for( CaseItem* const subitem_case_item_pointer : m_subitemCaseItems )
    {
        // don't process the subitem when that is the value that was initially changed
        if( subitem_case_item_pointer == &modified_case_item )
            continue;

        CaseItem& subitem_case_item = *subitem_case_item_pointer;
        const CDictItem& subitem_dict_item = subitem_case_item.GetDictItem();

        // get the subitem's offset in the parent item
        size_t subitem_offset = subitem_dict_item.GetStart() - m_parentCaseItem.GetDictItem().GetStart();

        for( subitem_index.ResetSubitemOccurrence();
             subitem_index.GetSubitemOccurrence() < subitem_dict_item.GetOccurs();
             subitem_index.IncrementSubitemOccurrence() )
        {
            const std::string_view subitem_text_value_sv = SO::WideSubstring(*parent_text_value, subitem_offset, subitem_dict_item.GetLen());

            if( subitem_case_item.GetType() == CaseItem::Type::FixedWidthString )
            {
                assert_cast<const FixedWidthStringCaseItem&>(subitem_case_item).SetValue(subitem_index, std::string(subitem_text_value_sv));
            }

            else
            {
                assert_cast<const FixedWidthNumericWithStringBufferCaseItem&>(subitem_case_item).SetValueFromTextInput(subitem_index, UTF8_TODO::GetWide(subitem_text_value_sv).c_str());
            }

            subitem_offset += subitem_dict_item.GetLen();
        }
    }

    case_records_being_processed.erase(std::find(case_records_being_processed.begin(), case_records_being_processed.end(), &case_record));
}
