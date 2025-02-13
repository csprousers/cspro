#pragma once

namespace Paradata
{
    template<typename T>
    const T* GetOptionalValueOrNull(const std::optional<T>& optional_value)
    {
        return optional_value.has_value() ? &(*optional_value) : nullptr;
    }

    inline const TCHAR* GetOptionalTextValueOrNull(const std::optional<CString>& optional_value)
    {
        return optional_value.has_value() ? (LPCTSTR)(*optional_value) : nullptr;
    }
}
