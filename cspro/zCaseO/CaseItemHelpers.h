#pragma once

#include <zCaseO/NumericCaseItem.h>
#include <zCaseO/StringCaseItem.h>


namespace CaseItemHelpers
{
    template<typename T>
    void SetValue(const CaseItem& destination_case_item, CaseItemIndex& destination_index, T&& value);


    void CopyValue(const CaseItem& source_case_item, const CaseItemIndex& source_index,
                   const CaseItem& destination_case_item, CaseItemIndex& destination_index);
}



// --------------------------------------------------------------------------
// inline implementations
// --------------------------------------------------------------------------

template<typename T>
void CaseItemHelpers::SetValue(const CaseItem& destination_case_item, CaseItemIndex& destination_index, T&& value)
{
    if constexpr(std::is_same_v<std::remove_cvref_t<T>, CString> ||
                 std::is_same_v<std::remove_cvref_t<T>, std::wstring> ||
                 std::is_same_v<std::remove_cvref_t<T>, wstring_view>)
    {
        ASSERT(IsString(destination_case_item.GetDataType()));
        assert_cast<const StringCaseItem&>(destination_case_item).SetValue(destination_index, UTF8_TODO::GetUtf8(value));
    }

    else if constexpr(std::is_same_v<std::remove_cvref_t<T>, std::string> ||
                      std::is_same_v<std::remove_cvref_t<T>, SharableString>)
    {
        ASSERT(IsString(destination_case_item.GetDataType()));
        assert_cast<const StringCaseItem&>(destination_case_item).SetValue(destination_index, std::forward<T>(value));
    }

    else
    {
        ASSERT(IsNumeric(destination_case_item.GetDataType()));
        assert_cast<const NumericCaseItem&>(destination_case_item).SetValue(destination_index, value);
    }
}


inline void CaseItemHelpers::CopyValue(const CaseItem& source_case_item, const CaseItemIndex& source_index,
                                       const CaseItem& destination_case_item, CaseItemIndex& destination_index)
{
    ASSERT(source_case_item.GetDataType() == destination_case_item.GetDataType());

    switch( source_case_item.GetDataType() )
    {
        case DataType::Numeric:
            SetValue(destination_case_item, destination_index, assert_cast<const NumericCaseItem&>(source_case_item).GetValue(source_index));
            break;

        case DataType::String:
            SetValue(destination_case_item, destination_index, assert_cast<const StringCaseItem&>(source_case_item).GetValue(source_index));
            break;

        default:
            ASSERT(false);
            break;
    }
}
