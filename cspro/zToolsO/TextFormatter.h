#pragma once


template<typename... Args>
CString FormatText(const TCHAR* formatter, Args const&... args)
{
    if constexpr(sizeof...(Args) == 0)
    {
        return formatter;
    }

    else
    {
        CString formatted_text;
	    formatted_text.Format(formatter, args...);
        return formatted_text;
    }
}


template<typename... Args>
std::wstring FormatTextCS2WS(const TCHAR* formatter, Args const&... args)
{
    CString formatted_text = FormatText(formatter, args...);
    return std::wstring(formatted_text.GetString(), static_cast<std::wstring::size_type>(formatted_text.GetLength()));
}
