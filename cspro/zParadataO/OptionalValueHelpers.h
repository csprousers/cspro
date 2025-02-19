#pragma once


namespace Paradata
{
    template<typename T>
    const T* GetOptionalValueOrNull(const std::optional<T>& optional_value)
    {
        return optional_value.has_value() ? &(*optional_value) :
                                            nullptr;
    }


    inline const char* GetOptionalTextValueOrNull(const std::optional<std::string>& optional_value)
    {
        return optional_value.has_value() ? optional_value->c_str() :
                                            nullptr;
    }
}
