#pragma once


#ifdef _DEBUG

template<typename... Args>
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
                          std::is_convertible_v<type, const char*> ||   // %s
                          std::is_same_v<type, char> ||                 // %c
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
        ValidateFormatTextArgumentTypes(args...);
#endif

        std::string formatted_text;
        formatted_text.resize(std::snprintf(nullptr, 0, formatter, args...));

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


#ifdef _DEBUG

template<typename... Args>
void ValidateWideFormatTextArgumentTypes(Args const&... args)
{
    (
        [&]
        {
            using type = std::remove_cvref_t<decltype(args)>;

            static_assert(std::is_same_v<type, int> ||                   // %d
                          std::is_same_v<type, long> ||                  // %ld
                          std::is_same_v<type, unsigned int> ||          // %u / %x
                          std::is_same_v<type, unsigned long> ||         // %lu
                          std::is_same_v<type, int64_t> ||               // %lld
                          std::is_same_v<type, uint64_t> ||              // %llu
                          std::is_same_v<type, double> ||                // %f
                          std::is_convertible_v<type, const wchar_t*> || // %s
                          std::is_same_v<type, wchar_t> ||               // %c
                          std::is_same_v<type, const void*>,             // %p
                          "argument to FormatText is invalid");
        }
    (), ...);
}

#endif


template<typename... Args>
CString FormatText(const TCHAR* const formatter, Args const&... args)
{
    if constexpr(sizeof...(Args) == 0)
    {
        return formatter;
    }

    else
    {
#ifdef _DEBUG
        ValidateWideFormatTextArgumentTypes(args...);
#endif

        CString formatted_text;
	    formatted_text.Format(formatter, args...);
        return formatted_text;
    }
}


template<typename... Args>
std::wstring FormatTextCS2WS(const TCHAR* const formatter, Args const&... args)
{
    const CString formatted_text = FormatText(formatter, args...);
    return std::wstring(formatted_text.GetString(), static_cast<size_t>(formatted_text.GetLength()));
}
