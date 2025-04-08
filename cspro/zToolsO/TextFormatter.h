#pragma once


// Returns a formatted string as a std::string.
template<typename... Args>
std::string FormatText(const char* formatter, Args const&... args);

// Returns a formatted wide-character string as a std::wstring or a CString, defaulting to std::wstring.
template<typename RT = std::wstring, typename... Args>
RT FormatText(const wchar_t* formatter, Args const&... args);



// --------------------------------------------------------------------------
// inline implementations
// --------------------------------------------------------------------------

#ifdef _DEBUG

template<typename CT, typename... Args>
void ValidateFormatTextArgumentTypes(Args const&... args)
{
    (
        [&]
        {
            using type = std::remove_cvref_t<decltype(args)>;

            static_assert(std::is_same_v<type, int> ||                  // %d
                          std::is_same_v<type, long> ||                 // %ld
                          std::is_same_v<type, unsigned int> ||         // %u / %x
                          std::is_same_v<type, unsigned long> ||        // %lu
                          std::is_same_v<type, int64_t> ||              // %lld
                          std::is_same_v<type, uint64_t> ||             // %llu
                          std::is_same_v<type, double> ||               // %f
                          std::is_convertible_v<type, const CT*> ||     // %s
                          std::is_same_v<type, CT> ||                   // %c
                          std::is_same_v<type, const void*>,            // %p
                          "argument to FormatText is invalid");
        }
    (), ...);
}

#endif


template<typename... Args>
std::string FormatText(const char* const formatter, Args const&... args)
{
    if constexpr(sizeof...(Args) == 0)
    {
        return formatter;
    }

    else
    {
#ifdef _DEBUG
        ValidateFormatTextArgumentTypes<char>(args...);
#endif

        std::string formatted_text(std::snprintf(nullptr, 0, formatter, args...), '\0');

#pragma warning(push)
#pragma warning(disable:4996)
        std::sprintf(formatted_text.data(), formatter, args...);
#pragma warning(pop)

#if defined(_DEBUG) && defined(WIN_DESKTOP)
        CStringA cstring_formatted_text;
        cstring_formatted_text.Format(formatter, args...);
        ASSERT81(formatted_text == std::string(cstring_formatted_text.GetString()));
#endif

        return formatted_text;
    }
}


template<typename RT/* = std::wstring*/, typename... Args>
RT FormatText(const wchar_t* const formatter, Args const&... args)
{
    if constexpr(sizeof...(Args) == 0)
    {
        return formatter;
    }

    else
    {
#ifdef _DEBUG
        ValidateFormatTextArgumentTypes<wchar_t>(args...);
#endif

        if constexpr(std::is_same_v<RT, CString>)
        {
            CString formatted_text;
            formatted_text.Format(formatter, args...);
            return formatted_text;
        }

        else
        {
#ifdef WIN32
#pragma warning(push)
#pragma warning(disable:4996)
            std::wstring formatted_text(_snwprintf(nullptr, 0, formatter, args...), '\0');
           _swprintf(formatted_text.data(), formatter, args...);
#pragma warning(pop)

            ASSERT81(wcscmp(formatted_text.c_str(), FormatText<CString>(formatter, args...).GetString()) == 0);

            return formatted_text;
#else
            const CString formatted_text = FormatText<CString>(formatter, args...);
            return std::wstring(formatted_text.GetString(), static_cast<size_t>(formatted_text.GetLength()));
#endif
        }
    }
}
