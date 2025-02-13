#pragma once

std::wstring ToWS(System::String^ text);
System::String^ FromWS(wstring_view text_sv);
std::vector<std::wstring> ToVectorWS(array<System::String^>^ array_values);


inline std::wstring ToWS(System::String^ text)
{
    if( text != nullptr )
    {
        pin_ptr<const wchar_t> ptr = PtrToStringChars(text);
        return ptr;
    }

    return std::wstring();
}


inline System::String^ FromWS(const wstring_view text_sv)
{
    return gcnew System::String(text_sv.data(), 0, text_sv.length());
}


inline std::vector<std::wstring> ToVectorWS(array<System::String^>^ array_values)
{
    std::vector<std::wstring> values;
    values.reserve(array_values->Length);

    for( int i = 0; i < array_values->Length; ++i )
        values.emplace_back(ToWS(array_values[i]));

    return values;
}
