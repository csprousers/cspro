#pragma once

#include <zToolsO/Utf8.h>

class clr_helpers
{
public:
    static std::wstring to_wstring(System::String^ text);
    static std::string to_string(System::String^ text);

    template<typename T>
    static System::String^ to_SystemString(const T& text);

    template<typename... Args>
    static System::String^ to_FormattedSystemString(const char* formatter, Args const&... args);

    static std::vector<std::wstring> to_wstring_vector(array<System::String^>^ array_values);
};


// --------------------------------------------------------------------------
// inline implementations
// --------------------------------------------------------------------------

inline std::wstring clr_helpers::to_wstring(System::String^ text)
{
    if( text != nullptr )
    {
        pin_ptr<const wchar_t> ptr = PtrToStringChars(text);
        ASSERT(wcslen(ptr) == static_cast<size_t>(text->Length));
        return std::wstring(ptr, text->Length);
    }

    return std::wstring();
}


inline std::string clr_helpers::to_string(System::String^ text)
{
    if( text != nullptr )
    {
        pin_ptr<const wchar_t> ptr = PtrToStringChars(text);
        ASSERT(wcslen(ptr) == static_cast<size_t>(text->Length));
        return TC::ToUtf8(ptr, text->Length);
    }

    return std::string();
}


template<typename T>
System::String^ clr_helpers::to_SystemString(const T& text)
{
    return gcnew System::String(text.data(), 0, int32_cast(text.length()), System::Text::Encoding::UTF8);
}


template<>
inline System::String^ clr_helpers::to_SystemString(const char* const& text)
{
    return to_SystemString(std::string_view(text));
}


template<typename... Args>
static System::String^ clr_helpers::to_FormattedSystemString(const char* const formatter, Args const&... args)
{
    return to_SystemString(FormatText(formatter, args...));
}


inline std::vector<std::wstring> clr_helpers::to_wstring_vector(array<System::String^>^ array_values)
{
    std::vector<std::wstring> values;
    values.reserve(array_values->Length);

    for( int i = 0; i < array_values->Length; ++i )
        values.emplace_back(to_wstring(array_values[i]));

    return values;
}
