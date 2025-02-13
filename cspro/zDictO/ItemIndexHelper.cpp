#include "StdAfx.h"
#include "ItemIndexHelper.h"


ItemIndexHelper::ItemIndexHelper(const CDictItem& dict_item)
{
    m_maxRecordOccurrences = dict_item.GetRecord()->GetMaxRecs();
    m_hasRecordOccurrences = ( m_maxRecordOccurrences > 1 );

    m_maxItemSubitemOccurrences = dict_item.GetItemSubitemOccurs();
    m_hasItemSubitemOccurrences = ( m_maxItemSubitemOccurrences > 1 );

    m_isSubitem = ( dict_item.GetItemType() == ItemType::Subitem );
    m_hasItemOccurrences = ( !m_isSubitem && dict_item.GetOccurs() > 1 );
    m_hasSubitemOccurrences = ( m_isSubitem && dict_item.GetOccurs() > 1 );

    m_numberMinimalOccurrences = ( m_hasRecordOccurrences ? 1 : 0 ) +
                                 ( m_hasItemSubitemOccurrences ? 1 : 0 );
}


bool ItemIndexHelper::IsValid(const ItemIndex& item_index) const
{
    if( item_index.GetRecordOccurrence() >= m_maxRecordOccurrences )
        return false;

    if( m_isSubitem )
    {
        if( item_index.GetItemOccurrence() != 0 || item_index.GetSubitemOccurrence() >= m_maxItemSubitemOccurrences )
            return false;
    }

    else
    {
        if( item_index.GetItemOccurrence() >= m_maxItemSubitemOccurrences || item_index.GetSubitemOccurrence() != 0 )
            return false;
    }

    return true;
}


CString ItemIndexHelper::GetFullOccurrencesText(const ItemIndex& item_index) const
{
    const std::vector<size_t>& one_based_occurrences = GetOneBasedOccurrences(item_index);
    return FormatText(_T("(%d,%d,%d)"), static_cast<int>(one_based_occurrences[0]),
                                        static_cast<int>(one_based_occurrences[1]),
                                        static_cast<int>(one_based_occurrences[2]));
}


CString ItemIndexHelper::GetMinimalOccurrencesText(const ItemIndex& item_index) const
{
    if( m_numberMinimalOccurrences == 0 )
        return CString();

    const TCHAR* formatter = ( m_numberMinimalOccurrences == 1 ) ? _T("(%d)") : _T("(%d,%d)");
    int formatted_occurrences[2];
    int* formatted_occurrences_itr = formatted_occurrences;

    // add a record occurrence for multiply occurring records
    if( m_hasRecordOccurrences )
        *(formatted_occurrences_itr++) = static_cast<int>(item_index.GetRecordOccurrence()) + 1;

    // for multiply occurring items, add an item or subitem occurrence
    if( m_hasItemSubitemOccurrences )
        *formatted_occurrences_itr = static_cast<int>(m_hasSubitemOccurrences ? item_index.GetSubitemOccurrence() : item_index.GetItemOccurrence()) + 1;

    return FormatText(formatter, formatted_occurrences[0], formatted_occurrences[1]);
}


unsigned ItemIndexHelper::ParseOccurrencesFromText(const TCHAR* occurrences_text, size_t occurrences[ItemIndex::NumberDimensions]) const
{
    memset(occurrences, 0, sizeof(occurrences[0]) * ItemIndex::NumberDimensions);

    size_t* current_occurrence = occurrences;
    bool allow_left_parenthesis = true;
    bool read_left_parenthesis = false;

    auto finalize_occurrence = [&]
    {
        // convert to a zero-based occurrence
        if( *current_occurrence > 0 )
            --(*current_occurrence);

        ++current_occurrence;
    };

    for( ; *occurrences_text != 0 && *occurrences_text != ')'; ++occurrences_text )
    {
        TCHAR digit_value = ( *occurrences_text - '0' );

        if( digit_value <= 9 )
        {
            *current_occurrence = *current_occurrence * 10 + digit_value;
        }

        // skip over spaces
        else if( *occurrences_text == ' ' )
        {
            continue;
        }

        // if a comma, advance to the next occurrence
        else if( *occurrences_text == ',' )
        {
            finalize_occurrence();

            // check for too many occurrences
            if( ( current_occurrence - occurrences ) == ItemIndex::NumberDimensions )
                return 0;
        }

        // allow the string to start with a left parenthesis
        else if( *occurrences_text == '(' && allow_left_parenthesis )
        {
            read_left_parenthesis = true;
        }

        // an invalid character
        else
        {
            return 0;
        }

        allow_left_parenthesis = false;
    }

    // match the left and right parenthesis
    if( read_left_parenthesis && *occurrences_text != ')' )
        return 0;

    finalize_occurrence();

    // when not all occurrences are defined, process the minimal occurrences for the item
    const unsigned occurrences_defined = current_occurrence - occurrences;

    if( occurrences_defined != ItemIndex::NumberDimensions )
    {
        if( occurrences_defined < m_numberMinimalOccurrences )
            return 0;

        // the occurrences potentially have to be shifted
        if( m_hasItemSubitemOccurrences )
        {
            // shift the record occurrence to the item/subitem occurrence
            if( occurrences_defined == 1 )
            {
                occurrences[m_hasSubitemOccurrences ? 2 : 1] = occurrences[0];
                occurrences[0] = 0;
            }
                
            // or shift the item occurrence to the subitem occurrence
            else if( occurrences_defined == 2 && m_hasSubitemOccurrences )
            {
                occurrences[2] = occurrences[1];
                occurrences[1] = 0;
            }
        }
    }

    return std::min<unsigned>(occurrences_defined, 2);
}


bool ItemIndexHelper::SetOccurrencesFromText(ItemIndex& item_index, const TCHAR* occurrences_text) const
{
    size_t occurrences[ItemIndex::NumberDimensions];

    if( ParseOccurrencesFromText(occurrences_text, occurrences) == 0 )
        return false;

    // set the index and return whether it is valid
    item_index.SetOccurrences(occurrences);

    return IsValid(item_index);
}


void ItemIndexHelper::WriteJson(JsonWriter& json_writer, const ItemIndex& item_index, WriteJsonMode mode) const
{
    json_writer.BeginObject();

    if( mode == WriteJsonMode::FullOccurrences || mode == WriteJsonMode::ApplicableOccurrences )
    {
        json_writer.Write(JK::record, item_index.GetRecordOccurrence() + 1)
                   .Write(JK::item, item_index.GetItemOccurrence() + 1);

        if( HasSubitemOccurrences() )
        {
            json_writer.Write(JK::subitem, item_index.GetSubitemOccurrence() + 1);
        }

        else if( mode == WriteJsonMode::FullOccurrences )
        {
            json_writer.Write(JK::subitem, 0);
        }
    }

    else 
    {
        ASSERT(mode == WriteJsonMode::MinimalOccurrences);

        if( HasRecordOccurrences() )
            json_writer.Write(JK::record, item_index.GetRecordOccurrence() + 1);

        if( HasItemOccurrences() )
            json_writer.Write(JK::item, item_index.GetItemOccurrence() + 1);

        if( HasSubitemOccurrences() )
            json_writer.Write(JK::subitem, item_index.GetSubitemOccurrence() + 1);
    }

    json_writer.EndObject();
}
