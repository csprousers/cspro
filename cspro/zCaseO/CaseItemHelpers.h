#pragma once

#include <zCaseO/NumericCaseItem.h>
#include <zCaseO/StringCaseItem.h>


namespace CaseItemHelpers
{
    template<typename T>
    inline void SetValue(const CaseItem& destination_case_item, CaseItemIndex& destination_index, T value)
    {
        if constexpr(std::is_same_v<T, CString>)
        {
            ASSERT(destination_case_item.IsTypeString());
            assert_cast<const StringCaseItem&>(destination_case_item).SetValue(destination_index, value);
        }

        else if constexpr(std::is_same_v<T, std::wstring>)
        {
            ASSERT(destination_case_item.IsTypeString());
            assert_cast<const StringCaseItem&>(destination_case_item).SetValue(destination_index, WS2CS(std::move(value)));
        }

        else
        {
            ASSERT(destination_case_item.IsTypeNumeric());
            assert_cast<const NumericCaseItem&>(destination_case_item).SetValue(destination_index, value);
        }
    }

    inline void CopyValue(const CaseItem& source_case_item, const CaseItemIndex& source_index,
                          const CaseItem& destination_case_item, CaseItemIndex& destination_index)
    {
        ASSERT(source_case_item.IsTypeNumeric() == destination_case_item.IsTypeNumeric());

        if( source_case_item.IsTypeNumeric() )
        {
            SetValue(destination_case_item, destination_index, assert_cast<const NumericCaseItem&>(source_case_item).GetValue(source_index));
        }

        else if( source_case_item.IsTypeString() )
        {
            SetValue(destination_case_item, destination_index, assert_cast<const StringCaseItem&>(source_case_item).GetValue(source_index));
        }

        else
        {
            ASSERT(false);
        }
    }
}
