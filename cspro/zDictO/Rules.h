#pragma once

#include <zDictO/DDClass.h>


namespace DictionaryRules
{
    // length
    constexpr bool CanModifyLength(ContentType content_type)
    {
        return !IsBinary(content_type);
    }

    constexpr bool CanModifyLength(const CDictItem& dict_item)
    {
        return CanModifyLength(dict_item.GetContentType());
    }


    // value sets
    constexpr bool CanHaveValueSet(const CDictItem& dict_item)
    {
        return !IsBinary(dict_item.GetContentType());
    }


    // ID items
    constexpr bool CanBeIdItem(ContentType content_type)
    {
        return !IsBinary(content_type);
    }

    inline bool CanBeIdItem(const CDictItem& dict_item)
    {
        // if the dictionary ever supports non-fixed width items, check for that here
        return ( !dict_item.IsSubitem() && CanBeIdItem(dict_item.GetContentType()) );
    }


    // subitems
    constexpr bool CanBeSubitem(ContentType content_type)
    {
        return !IsBinary(content_type);
    }

    inline bool CanBeSubitem(const CDictRecord& dict_record, ContentType content_type)
    {
        return ( !dict_record.IsIdRecord() && CanBeSubitem(content_type) );
    }

    inline bool CanBeSubitem(const CDictRecord& dict_record, const CDictItem& dict_item)
    {
        return CanBeSubitem(dict_record, dict_item.GetContentType());
    }

    constexpr bool CanHaveSubitems(ContentType content_type)
    {
        return !IsBinary(content_type);
    }

    inline bool CanHaveSubitems(const CDictRecord& dict_record, const CDictItem& dict_item)
    {
        return ( !dict_record.IsIdRecord() && CanHaveSubitems(dict_item.GetContentType()) );
    }


    // multiple occurrences
    inline bool CanItemHaveMultipleOccurrences(const CDictRecord& dict_record)
    {
        return !dict_record.IsIdRecord();
    }


    // numeric decimals
    inline bool CanHaveDecimals(const CDictRecord& dict_record, ContentType content_type)
    {
        return ( !dict_record.IsIdRecord() && IsNumeric(content_type) );
    }


    // numeric zero fill
    inline bool CanHaveZeroFill(ContentType content_type)
    {
        // if the dictionary ever supports non-fixed width items, potentially check for that here
        return IsNumeric(content_type);
    }
}
