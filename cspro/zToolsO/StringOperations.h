#pragma once

#include <zToolsO/zToolsO.h>
#include <zToolsO/StringView.h>
#include <zToolsO/NullTerminatedString.h>
#include <cwctype>

class StringNoCase;


// eventually, if we move away from using CString, we can remove these calls
inline CString WS2CS(const std::wstring& text) { return CString(text.c_str(), static_cast<int>(text.size())); }
inline std::wstring CS2WS(const CString& text) { return std::wstring(text.GetString(), static_cast<std::wstring::size_type>(text.GetLength())); }
CLASS_DECL_ZTOOLSO const CString& WS2CS_Reference(const std::wstring& text);
CLASS_DECL_ZTOOLSO const std::wstring& CS2WS_Reference(const CString& text);


class SO // SO = string operations
{
public:
    // --------------------------------------------------------------------------
    // EmptyString: empty string objects that can be used by methods that return
    // const references
    // --------------------------------------------------------------------------

    CLASS_DECL_ZTOOLSO static const std::wstring EmptyString;
    CLASS_DECL_ZTOOLSO static const CString EmptyCString;


    // --------------------------------------------------------------------------
    // Case functions
    // --------------------------------------------------------------------------

    static std::wstring& MakeUpper(std::wstring& text);
    static std::wstring ToUpper(wstring_view text_sv);
    static bool IsUpper(wstring_view text_sv);

    static std::wstring& MakeLower(std::wstring& text);
    static std::wstring ToLower(wstring_view text_sv);
    static bool IsLower(wstring_view text_sv);

    CLASS_DECL_ZTOOLSO static std::wstring ToProperCase(std::wstring text);
    static std::wstring TitleToCamelCase(std::wstring text);


    // --------------------------------------------------------------------------
    // Comparison functions
    // --------------------------------------------------------------------------

    static bool IsBlank(wstring_view text_sv);
    static bool IsWhitespace(wstring_view text_sv);

    static bool Equals(wstring_view sv1, wstring_view sv2) noexcept;
    CLASS_DECL_ZTOOLSO static bool Equals(wstring_view sv1, std::string_view sv2);

    static int CompareNoCase(wstring_view sv1, wstring_view sv2) noexcept;

    template<typename ST1, typename ST2>
    static bool EqualsNoCase(const ST1& sv_or_cstr1, const ST2& sv_or_cstr2);

    template<typename... Args>
    static bool EqualsOneOf(wstring_view source_sv, wstring_view compare1_sv, Args const&... compare2_and_more);

    template<typename... Args>
    static bool EqualsOneOfNoCase(wstring_view source_sv, wstring_view compare1_sv, Args const&... compare2_and_more);

    static bool StartsWith(wstring_view source_sv, wstring_view starts_with_text_sv);

    template<typename ST1, typename ST2>
    static bool StartsWithNoCase(const ST1& source_sv_or_cstr, const ST2& starts_with_text_sv_or_cstr);


    // --------------------------------------------------------------------------
    // Search functions
    // --------------------------------------------------------------------------

    // searches in a case-insensitive manner, the source string, for the substring;
    // if not found, wstring_view::npos (-1) is returned
    static size_t FindNoCase(wstring_view source_sv, wstring_view substring_sv, size_t offset = 0);

    // returns the indices of two characters, with the second character coming after the first;
    // if both are not found, then the second returned value will be equal to npos
    static std::tuple<size_t, size_t> FindCharacters(wstring_view text_sv, TCHAR ch1, TCHAR ch2, size_t offset = 0);

    // returns the index of the first whitespace character in the source string;
    // if not found, wstring_view::npos (-1) is returned
    static size_t FindFirstWhitespace(wstring_view source_sv, size_t offset = 0);

    // returns the substring between the two characters found using the above function;
    // if both are not found, then an empty string view is returned
    static wstring_view GetTextBetweenCharacters(wstring_view text_sv, TCHAR ch1, TCHAR ch2);

    // returns the strings to the left and right of the character;
    // if the character is not found, the left string will be equal to the entire string and the right string will be empty
    static std::tuple<wstring_view, wstring_view> GetTextOnEitherSideOfCharacter(wstring_view text_sv, TCHAR ch);


    // --------------------------------------------------------------------------
    // Newline functions
    // --------------------------------------------------------------------------

    // newline characters handled throughout the code
    static constexpr const TCHAR* NewlineCharacters = _T("\r\n");

    // indicates whether a '\n' newline character exists in the string
    template<typename T>
    static bool ContainsNewlineCharacter(const T& text);

    // makes sure that newlines are "\r\n", not just "\n"
    static void MakeNewlineCRLF(std::wstring& text);
    static std::wstring ToNewlineCRLF(std::wstring text);

    // makes sure that newlines are only "\n", not "\r\n"
    static void MakeNewlineLF(std::wstring& text);
    static std::wstring ToNewlineLF(std::wstring text);

    // takes a vector of strings and, if any strings contain newlines, those strings will be split into separate vector entries
    CLASS_DECL_ZTOOLSO static void MakeSplitVectorStringsOnNewlines(std::vector<std::wstring>& strings);
    static std::vector<std::wstring> SplitVectorStringsOnNewlines(std::vector<std::wstring> strings);


    // --------------------------------------------------------------------------
    // Manipulation functions
    // --------------------------------------------------------------------------

    CLASS_DECL_ZTOOLSO static wstring_view TrimLeft(wstring_view text_sv);
    CLASS_DECL_ZTOOLSO static wstring_view TrimRight(wstring_view text_sv);

    CLASS_DECL_ZTOOLSO static wstring_view TrimLeft(wstring_view text_sv, TCHAR trim_char);
    CLASS_DECL_ZTOOLSO static wstring_view TrimLeft(wstring_view text_sv, wstring_view trim_chars_sv);
    CLASS_DECL_ZTOOLSO static wstring_view TrimRight(wstring_view text_sv, TCHAR trim_char);
    CLASS_DECL_ZTOOLSO static wstring_view TrimRight(wstring_view text_sv, wstring_view trim_chars_sv);

    template<typename... Args>
    static wstring_view Trim(wstring_view text_sv, Args const&... args);

    template<typename... Args>
    static std::wstring& MakeTrim(std::wstring& text, Args const&... args);

    template<typename... Args>
    static std::wstring& MakeTrimLeft(std::wstring& text, Args const&... args);

    template<typename... Args>
    static std::wstring& MakeTrimRight(std::wstring& text, Args const&... args);

    static wstring_view TrimRightSpace(wstring_view text_sv);
    static std::wstring& MakeTrimRightSpace(std::wstring& text);
    static CString& MakeTrimRightSpace(CString& text);

    // replaces all instances of the old character with the new character
    CLASS_DECL_ZTOOLSO static std::wstring& Replace(std::wstring& text, TCHAR old_char, TCHAR new_char, std::wstring::size_type ch_pos = 0);

    // replaces all instances of the old text with the new text
    CLASS_DECL_ZTOOLSO static std::wstring& Replace(std::wstring& text, wstring_view old_text_sv, wstring_view new_text_sv, std::wstring::size_type ch_pos = 0);
    CLASS_DECL_ZTOOLSO static std::wstring& ReplaceNoCase(std::wstring& text, wstring_view old_text_sv, wstring_view new_text_sv, std::wstring::size_type ch_pos = 0);

    // replaces all instances of the old text with the new text, calling the Replace function repeatedly
    // until no more instances of the text are replaced;
    // this function can be called if the new text contains text that may also be part of the old text
    CLASS_DECL_ZTOOLSO static std::wstring& RecursiveReplace(std::wstring& text, wstring_view old_text_sv, wstring_view new_text_sv);

    // removes all instances of the character
    CLASS_DECL_ZTOOLSO static std::wstring& Remove(std::wstring& text, TCHAR ch);

    // sets the length of the string to the desired length, padding with spaces if necessary
    template<bool insert_spaces_at_end = true, typename T>
    static T& MakeExactLength(T& text, size_t length);

    // sets the length of the string to the desired length, centering the string (when padding is necessary)
    CLASS_DECL_ZTOOLSO static std::wstring& CenterExactLength(std::wstring& text, size_t length);

    // appends text, or formatted text, to the destination string
    template<typename T, typename... Args>
    static T& Append(T& destination, wstring_view text1_sv, Args... textN);

    template<typename T>
    static T& Append(T& destination, wstring_view text_sv);

    template<typename T, typename... Args>
    static T& AppendFormat(T& destination, const TCHAR* formatter, Args const&... args);

    // appends text with the separator added if the destination string is not blank
    static std::wstring& AppendWithSeparator(std::wstring& destination, wstring_view text_sv, wchar_t separator);

    template<typename T>
    static T& AppendWithSeparator(T& destination, wstring_view text_sv, wstring_view separator_sv);

    // concatenates the strings, returning the combined string
    template<typename... Args>
    static std::wstring Concatenate(std::wstring text1, Args... textN);

    // removes any text following the character; the character is optionally removed
    CLASS_DECL_ZTOOLSO static wstring_view RemoveTextFollowingCharacter(wstring_view text_sv, char ch, bool remove_ch);

    // the default number of spaces per tab and a space string that can be used for the tabs
    static constexpr int DefaultSpacesPerTab = 4;
    static constexpr const TCHAR* SingleTabAsSpaces = _T("    ");

    // converts tabs to spaces, using DefaultSpacesPerTab as the number of spaces per tab
    CLASS_DECL_ZTOOLSO static int ConvertTabsToSpaces(std::wstring& text, int position_in_line = 0);

    // converts tabs to spaces and right-trims each line
    CLASS_DECL_ZTOOLSO static void ConvertTabsToSpacesAndTrimRightEachLine(std::wstring& text);


    // --------------------------------------------------------------------------
    // Parsing functions
    // --------------------------------------------------------------------------

    // the callback function is passed each section; sections are determined by splitting the text
    // using one of the separators; the callback function should return true to keep processing
    template<typename ST, typename CF>
    static void ForeachSection(wstring_view text_sv, ST separators, CF callback_function);

    // the callback function should return true to keep processing
    template<typename CF>
    static void ForeachLine(wstring_view text_sv, bool process_whitespace_lines, CF callback_function);


    // --------------------------------------------------------------------------
    // Other functions
    // --------------------------------------------------------------------------

    // splits the string at any provided separators; unlike functions like std::wcstok, multiple separators in a row will
    // be treated as separating entities (e.g., "a;;b;c" would result in 4 [not 3] entities with ; as the separator);
    // the include_empty_entities flag can be set to false to receive results like std::wcstok
    template<typename T = std::wstring>
    CLASS_DECL_ZTOOLSO static std::vector<T> SplitString(wstring_view text_sv, const TCHAR* separators, bool trim_all = true, bool include_empty_entities = true);

    template<typename T = std::wstring>
    static std::vector<T> SplitString(wstring_view text_sv, TCHAR single_separator, bool trim_all = true, bool include_empty_entities = true);

    // combines the strings into a single string, separating each with the separator
    template<bool right_trim_each_string = true, typename T>
    static std::wstring CreateSingleString(const T& strings, wstring_view separator_sv = _T(", "));

    // combines the objects into a single string by calling the callback on each object, separating each with the separator
    template<typename T, typename CF>
    static std::wstring CreateSingleStringUsingCallback(const T& objects, CF callback_function, wstring_view separator_sv = _T(", "));

    // if text2_sv is not empty, it is appended to text1, separated by a colon
    static std::wstring CreateColonSeparatedString(std::wstring text1, wstring_view text2_sv);

    // if text2_sv is not empty, it is appended to text1, wrapped in parentheses
    static std::wstring CreateParentheticalExpression(std::wstring text1, wstring_view text2_sv);

    // wraps the text at newline characters and width boundaries, adding hyphens as necessary
    CLASS_DECL_ZTOOLSO static std::vector<std::wstring> WrapText(wstring_view text_sv_sv, size_t maximum_line_width);

    // returns a C string that contains the specified number of repeating characters
    CLASS_DECL_ZTOOLSO static const TCHAR* GetRepeatingCharacterString(TCHAR ch, size_t length);

    static const TCHAR* GetDashedLine(size_t length) { return GetRepeatingCharacterString('-', length); }


    // --------------------------------------------------------------------------
    // Worker functions
    // --------------------------------------------------------------------------
private:
    template<typename ST>
    static constexpr bool StringTypeHasPrecalculatedLength() noexcept;

    template<typename ST>
    static const wchar_t* GetStringData(const ST& sv_or_cstr);

    template<typename ST1, typename ST2, bool EqualsMode>
    static bool EqualsStartsNoCaseWorker(const ST1& source_sv_or_cstr, const ST2& starts_with_text_sv_or_cstr);
};



// --------------------------------------------------------------------------
// Case functions
// --------------------------------------------------------------------------

inline std::wstring& SO::MakeUpper(std::wstring& text)
{
    std::transform(text.begin(), text.end(), text.begin(), std::towupper);
    return text;
}


inline std::wstring SO::ToUpper(wstring_view text_sv)
{
    std::wstring str(text_sv.length(), 0);
    std::transform(text_sv.cbegin(), text_sv.cend(), str.begin(), std::towupper);
    return str;
}


inline bool SO::IsUpper(wstring_view text_sv)
{
    return ( std::find_if(text_sv.cbegin(), text_sv.cend(),
                          [](wchar_t ch) { return ( ch != std::towupper(ch) ); }) == text_sv.cend() );
}


inline std::wstring& SO::MakeLower(std::wstring& text)
{
    std::transform(text.begin(), text.end(), text.begin(), std::towlower);
    return text;
}


inline std::wstring SO::ToLower(wstring_view text_sv)
{
    std::wstring str(text_sv.length(), 0);
    std::transform(text_sv.cbegin(), text_sv.cend(), str.begin(), std::towlower);
    return str;
}


inline bool SO::IsLower(wstring_view text_sv)
{
    return ( std::find_if(text_sv.cbegin(), text_sv.cend(),
                          [](wchar_t ch) { return ( ch != std::towlower(ch) ); }) == text_sv.cend() );
}


inline std::wstring SO::TitleToCamelCase(std::wstring text)
{
    if( !text.empty() )
    {
        TCHAR& ch = text[0];
        ch = std::towlower(ch);
    }

    return text;
}



// --------------------------------------------------------------------------
// Comparison functions
// --------------------------------------------------------------------------

inline bool SO::IsBlank(wstring_view text_sv)
{
    return ( text_sv.empty() || std::find_if(text_sv.cbegin(), text_sv.cend(),
                                             [](wchar_t ch) { return ( ch != ' ' ); }) == text_sv.cend() );

}


inline bool SO::IsWhitespace(wstring_view text_sv)
{
    return ( text_sv.empty() || std::find_if(text_sv.cbegin(), text_sv.cend(),
                                             [](wchar_t ch) { return !std::iswspace(ch); }) == text_sv.cend() );
}


inline bool SO::Equals(wstring_view sv1, wstring_view sv2) noexcept
{
    return ( sv1 == sv2 );
}


inline int SO::CompareNoCase(wstring_view sv1, wstring_view sv2) noexcept
{
    if( sv1.empty() )
        return sv2.empty() ? 0 : -1;

    if( sv2.empty() )
        return 1;

    const TCHAR* sv1_itr = sv1.data();
    const TCHAR* sv2_itr = sv2.data();
    const bool sv1_is_longer = ( sv1.length() > sv2.length() );
    const TCHAR* const sv1_end = sv1.data() + ( sv1_is_longer ? sv2.length() : sv1.length() );

    for( ; sv1_itr < sv1_end; ++sv1_itr, ++sv2_itr )
    {
        auto ch_diff = std::towupper(*sv1_itr) - std::towupper(*sv2_itr);

        if( ch_diff != 0 )
            return ch_diff;
    }

    return sv1_is_longer                    ?  1 :
           ( sv1.length() == sv2.length() ) ?  0 :
                                              -1;
}


template<typename ST1, typename ST2>
bool SO::EqualsNoCase(const ST1& sv_or_cstr1, const ST2& sv_or_cstr2)
{
    return EqualsStartsNoCaseWorker<ST1, ST2, true>(sv_or_cstr1, sv_or_cstr2);
}


template<typename... Args>
bool SO::EqualsOneOf(wstring_view source_sv, wstring_view compare1_sv, Args const&... compare2_and_more)
{
    static_assert(sizeof...(compare2_and_more) > 0, "Use SO::Equals if only comparing against one string");

    for( const wstring_view compare_sv : std::initializer_list<wstring_view> { compare1_sv, compare2_and_more... } )
    {
        if( Equals(source_sv, compare_sv) )
            return true;
    }

    return false;
}


template<typename... Args>
bool SO::EqualsOneOfNoCase(wstring_view source_sv, wstring_view compare1_sv, Args const&... compare2_and_more)
{
    static_assert(sizeof...(compare2_and_more) > 0, "Use SO::EqualsNoCase if only comparing against one string");

    for( const wstring_view compare_sv : std::initializer_list<wstring_view> { compare1_sv, compare2_and_more... } )
    {
        if( EqualsNoCase(source_sv, compare_sv) )
            return true;
    }

    return false;
}


inline bool SO::StartsWith(wstring_view source_sv, wstring_view starts_with_text_sv)
{
    // the string_view in C++20 has a starts_with operator, but in the meantime:
    return ( starts_with_text_sv.length() <= source_sv.length() &&
             source_sv.substr(0, starts_with_text_sv.length()).compare(starts_with_text_sv) == 0 );
}


template<typename ST1, typename ST2>
bool SO::StartsWithNoCase(const ST1& source_sv_or_cstr, const ST2& starts_with_text_sv_or_cstr)
{
    return SO::EqualsStartsNoCaseWorker<ST1, ST2, false>(source_sv_or_cstr, starts_with_text_sv_or_cstr);
}


template<typename ST1, typename ST2, bool EqualsMode>
bool SO::EqualsStartsNoCaseWorker(const ST1& source_sv_or_cstr, const ST2& starts_with_text_sv_or_cstr)
{
    // handle CString arguments as string views
    if constexpr(std::is_same_v<ST1, CString>)
    {
        return SO::EqualsStartsNoCaseWorker<wstring_view, ST2, EqualsMode>(source_sv_or_cstr, starts_with_text_sv_or_cstr);
    }

    else if constexpr(std::is_same_v<ST2, CString>)
    {
        return SO::EqualsStartsNoCaseWorker<ST1, wstring_view, EqualsMode>(source_sv_or_cstr, starts_with_text_sv_or_cstr);
    }

    // process either string views or null-terminated strings
    else
    {
        size_t length_remaining;

        if constexpr(StringTypeHasPrecalculatedLength<ST2>())
        {
            length_remaining = starts_with_text_sv_or_cstr.length();

            if constexpr(StringTypeHasPrecalculatedLength<ST1>())
            {
                if constexpr(EqualsMode)
                {
                    if( length_remaining != source_sv_or_cstr.length() )
                        return false;
                }

                else
                {
                    if( length_remaining > source_sv_or_cstr.length() )
                        return false;
                }
            }
        }

        else if constexpr(StringTypeHasPrecalculatedLength<ST1>())
        {
            length_remaining = source_sv_or_cstr.length();
        }

        const wchar_t* itr1 = GetStringData<ST1>(source_sv_or_cstr);
        const wchar_t* itr2 = GetStringData<ST2>(starts_with_text_sv_or_cstr);

        while( true )
        {
            // get the characters for both strings
            wchar_t ch1;

            if constexpr(StringTypeHasPrecalculatedLength<ST1>())
            {
                ch1 = ( length_remaining > 0 ) ? *itr1 : 0;
            }

            else
            {
                ch1 = *itr1;
            }

            wchar_t ch2;

            if constexpr(StringTypeHasPrecalculatedLength<ST2>())
            {
                ch2 = ( length_remaining > 0 ) ? *itr2 : 0;
            }

            else
            {
                ch2 = *itr2;
            }

            // return if at the end of one or both strings
            if( ch2 == 0 )
            {
                return ( !EqualsMode || ch1 == 0 );
            }

            else if( ch1 == 0 )
            {
                return false;
            }

            // compare the characters
            if( std::towupper(ch1) != std::towupper(ch2) )
                return false;

            // advance the iterators
            ++itr1;
            ++itr2;

            if constexpr(StringTypeHasPrecalculatedLength<ST1>() || StringTypeHasPrecalculatedLength<ST2>())
                --length_remaining;
        }
    }
}



// --------------------------------------------------------------------------
// Search functions
// --------------------------------------------------------------------------

inline size_t SO::FindNoCase(wstring_view source_sv, wstring_view substring_sv, size_t offset/* = 0*/)
{
    if( offset >= source_sv.length() )
        return wstring_view::npos;

    const size_t max_start_pos = source_sv.size() - substring_sv.length();

    if( max_start_pos >= source_sv.size() || max_start_pos < offset )
        return wstring_view::npos;

    ASSERT(!source_sv.empty() && !substring_sv.empty());

    const wchar_t substring_first_ch_lower = std::towlower(substring_sv.front());
    const wstring_view substring_second_ch_on_sv = substring_sv.substr(1);

    const TCHAR* source_sv_itr = source_sv.data() + offset;
    const TCHAR* source_sv_last_pos_to_check = source_sv.data() + max_start_pos;

    for( ; source_sv_itr <= source_sv_last_pos_to_check; ++source_sv_itr )
    {
        if( std::towlower(*source_sv_itr) == substring_first_ch_lower )
        {
            size_t pos = source_sv_itr - source_sv.data();

            if( SO::StartsWithNoCase(source_sv.substr(pos + 1), substring_second_ch_on_sv) )
                return pos;
        }
    }

    return wstring_view::npos;
}


inline std::tuple<size_t, size_t> SO::FindCharacters(wstring_view text_sv, TCHAR ch1, TCHAR ch2, size_t offset/* = 0*/)
{
    const size_t ch1_pos = text_sv.find(ch1, offset);
    return std::make_tuple(ch1_pos, ( ch1_pos != wstring_view::npos ) ? text_sv.find(ch2, ch1_pos + 1) :
                                                                        wstring_view::npos);
}


inline size_t SO::FindFirstWhitespace(wstring_view source_sv, size_t offset/* = 0*/)
{
    ASSERT(offset <= source_sv.length());
    const auto& lookup = std::find_if(source_sv.cbegin() + offset, source_sv.cend(),
                                      [](wchar_t ch) { return std::iswspace(ch); });

    return ( lookup != source_sv.cend() ) ? std::distance(source_sv.cbegin(), lookup) :
                                            wstring_view::npos;
}


inline wstring_view SO::GetTextBetweenCharacters(wstring_view text_sv, TCHAR ch1, TCHAR ch2)
{
    const auto [ch1_pos, ch2_pos] = SO::FindCharacters(text_sv, ch1, ch2);
    return ( ch2_pos != wstring_view::npos ) ? text_sv.substr(ch1_pos + 1, ch2_pos - ch1_pos - 1) :
                                               wstring_view();
}


inline std::tuple<wstring_view, wstring_view> SO::GetTextOnEitherSideOfCharacter(wstring_view text_sv, TCHAR ch)
{
    const size_t ch_pos = text_sv.find(ch);
    return ( ch_pos != wstring_view::npos ) ? std::make_tuple(text_sv.substr(0, ch_pos), text_sv.substr(ch_pos + 1) ) :
                                              std::make_tuple(text_sv, wstring_view());
}



// --------------------------------------------------------------------------
// Newline functions
// --------------------------------------------------------------------------

template<typename T>
bool SO::ContainsNewlineCharacter(const T& text)
{
    if constexpr(std::is_same_v<T, CString>)
    {
        return ( text.Find('\n') >= 0 );
    }

    else
    {
        return ( text.find('\n') != T::npos );
    }
}


inline void SO::MakeNewlineCRLF(std::wstring& text)
{
    auto text_end = text.cend();

    for( auto text_itr = text.begin(); text_itr < text_end; ++text_itr )
    {
        if( ( *text_itr == '\n' ) &&
            ( text_itr == text.begin() || *( text_itr - 1 ) != '\r' ) )
        {
            text_itr = text.insert(text_itr, '\r') + 1;
            text_end = text.cend();
        }
    }

    // this routine assumes that newlines come in as either \r\n or only as \n, not only \r
    ASSERT(std::count(text.cbegin(), text.cend(), '\r') == std::count(text.cbegin(), text.cend(), '\n'));
}


inline std::wstring SO::ToNewlineCRLF(std::wstring text)
{
    SO::MakeNewlineCRLF(text);
    return text;
}


inline void SO::MakeNewlineLF(std::wstring& text)
{
    // this routine assumes that newlines come in as either \r\n or only as \n, not only \r
    ASSERT(( std::count(text.cbegin(), text.cend(), '\r') == std::count(text.cbegin(), text.cend(), '\n') ) ||
           ( std::count(text.cbegin(), text.cend(), '\r') == 0 ));

#ifdef _DEBUG
    std::wstring replace_test = text;
#endif

    SO::Remove(text, '\r');

#ifdef _DEBUG
    SO::Replace(replace_test, _T("\r\n"), _T("\n"));
    SO::Replace(replace_test, _T("\r"), _T("\n"));
    ASSERT(replace_test == text);
#endif
}


inline std::wstring SO::ToNewlineLF(std::wstring text)
{
    SO::MakeNewlineLF(text);
    return text;
}


inline std::vector<std::wstring> SO::SplitVectorStringsOnNewlines(std::vector<std::wstring> strings)
{
    MakeSplitVectorStringsOnNewlines(strings);
    return strings;
}



// --------------------------------------------------------------------------
// Manipulation functions
// --------------------------------------------------------------------------

template<typename... Args>
wstring_view SO::Trim(wstring_view text_sv, Args const&... args)
{
    return ( text_sv.empty() )                                   ? text_sv :
           ( text_sv = SO::TrimLeft(text_sv, args...) ).empty()  ? text_sv :
                                                                   SO::TrimRight(text_sv, args...);
}


template<typename... Args>
std::wstring& SO::MakeTrim(std::wstring& text, Args const&... args)
{
    const wstring_view text_sv = SO::Trim(text, args...);

    if( text_sv.length() != text.length() )
        text = text_sv;

    return text;
}


template<typename... Args>
std::wstring& SO::MakeTrimLeft(std::wstring& text, Args const&... args)
{
    const wstring_view text_sv = SO::TrimLeft(text, args...);

    if( text_sv.length() != text.length() )
        text = text_sv;

    return text;
}


template<typename... Args>
std::wstring& SO::MakeTrimRight(std::wstring& text, Args const&... args)
{
    const wstring_view text_sv = SO::TrimRight(text, args...);

    if( text_sv.length() != text.length() )
        text.resize(text_sv.length());

    return text;
}


inline wstring_view SO::TrimRightSpace(wstring_view text_sv)
{
    return SO::TrimRight(text_sv, ' ');
}


inline std::wstring& SO::MakeTrimRightSpace(std::wstring& text)
{
    const wstring_view text_sv = SO::TrimRightSpace(text);

    if( text_sv.length() != text.length() )
        text.resize(text_sv.length());

    return text;
}


inline CString& SO::MakeTrimRightSpace(CString& text)
{
    const int trimmed_length = static_cast<int>(SO::TrimRightSpace(text).length());
    const int length_difference = text.GetLength() - trimmed_length;

    if( length_difference > 0 )
        text.Delete(trimmed_length, length_difference);

    return text;
}


template<bool insert_spaces_at_end/* = true*/, typename T>
T& SO::MakeExactLength(T& text, size_t length)
{
    if constexpr(std::is_same_v<T, std::wstring>)
    {
        if( text.length() != length )
        {
            if( insert_spaces_at_end || text.length() > length )
            {
                text.resize(length, ' ');
            }

            else
            {
                text.insert(0, length - text.length(), ' ');
            }
        }
    }

    else
    {
        static_assert(insert_spaces_at_end);

        int spaces_needed = static_cast<int>(length) - text.GetLength();

        if( spaces_needed > 0 )
        {
            text.Append(CString(' ', spaces_needed));
        }

        else if( spaces_needed < 0 )
        {
            text.Truncate(static_cast<int>(length));
        }
    }

    return text;
}


template<typename T, typename... Args>
T& SO::Append(T& destination, wstring_view text1_sv, Args... textN)
{
    SO::Append(destination, text1_sv);
    SO::Append(destination, textN...);
    return destination;
}


template<typename T>
T& SO::Append(T& destination, wstring_view text_sv)
{
    if constexpr(std::is_same_v<T, std::wstring>)
    {
        destination.append(text_sv.data(), text_sv.length());
    }

    else
    {
        destination.Append(text_sv.data(), text_sv.length());
    }

    return destination;
}


template<typename T, typename... Args>
T& SO::AppendFormat(T& destination, const TCHAR* formatter, Args const&... args)
{
    if constexpr(std::is_same_v<T, std::wstring>)
    {
        SO::Append(destination, ::FormatText(formatter, args...));
    }

    else
    {
        destination.AppendFormat(formatter, args...);
    }

    return destination;
}


inline std::wstring& SO::AppendWithSeparator(std::wstring& destination, wstring_view text_sv, wchar_t separator)
{
    if( !destination.empty() )
        destination.push_back(separator);

    return SO::Append(destination, text_sv);
}


template<typename T>
T& SO::AppendWithSeparator(T& destination, wstring_view text_sv, wstring_view separator_sv)
{
    if( !destination.empty() )
        SO::Append(destination, separator_sv);

    return SO::Append(destination, text_sv);
}


template<typename... Args>
std::wstring SO::Concatenate(std::wstring text1, Args... textN)
{
    SO::Append(text1, textN...);
    return text1;
}



// --------------------------------------------------------------------------
// Parsing functions
// --------------------------------------------------------------------------

template<typename ST, typename CF>
void SO::ForeachSection(wstring_view text_sv, ST separators, CF callback_function)
{
    while( !text_sv.empty() )
    {
        const std::wstring_view::size_type separator_pos = text_sv.find_first_of(separators);

        if( !callback_function(text_sv.substr(0, separator_pos)) )
            return;

        if( separator_pos == std::wstring_view::npos )
            return;

        text_sv = text_sv.substr(separator_pos + 1);
    }
}


template<typename CF>
void SO::ForeachLine(wstring_view text_sv, bool process_whitespace_lines, CF callback_function)
{
    if( text_sv.empty() )
        return;

    while( true )
    {
        const std::wstring_view::size_type newline_pos = text_sv.find_first_of(NewlineCharacters);
        const wstring_view this_line_sv = text_sv.substr(0, newline_pos);

        if( process_whitespace_lines || !SO::IsWhitespace(this_line_sv) )
        {
            if( !callback_function(this_line_sv) )
                return;
        }

        if( newline_pos == std::wstring_view::npos )
            return;

        std::wstring_view::size_type after_newline_pos = newline_pos + 1;

        // skip past the \n following a \r
        if( text_sv[newline_pos] == '\r' && after_newline_pos < text_sv.length() && text_sv[after_newline_pos] == '\n' )
            ++after_newline_pos;

        if( after_newline_pos == text_sv.length() )
        {
            // if processing whitespace, handle the blank line that occurs when the newline is at the end of the string; e.g.,: Line 1\nLine 2\n
            if( process_whitespace_lines )
                callback_function(wstring_view());

            return;
        }

        text_sv = text_sv.substr(after_newline_pos);
    }
}



// --------------------------------------------------------------------------
// Other functions
// --------------------------------------------------------------------------

template<typename T/* = std::wstring*/>
std::vector<T> SO::SplitString(wstring_view text_sv, TCHAR single_separator, bool trim_all/* = true*/, bool include_empty_entities/* = true*/)
{
    const TCHAR separators[] = { single_separator, 0 };
    return SO::SplitString<T>(text_sv, separators, trim_all, include_empty_entities);
}


template<bool right_trim_each_string/* = true*/, typename T>
std::wstring SO::CreateSingleString(const T& strings, wstring_view separator_sv/* = _T(", ")*/)
{
    if( strings.empty() )
        return std::wstring();

    auto strings_itr = strings.cbegin();
    const auto& strings_end = strings.cend();

    auto get_string = [](wstring_view text_sv)
    {
        if constexpr(right_trim_each_string)
        {
            return SO::TrimRight(text_sv);
        }

        else
        {
            return text_sv;
        }
    };

    std::wstring result = get_string(*strings_itr);

    while( ++strings_itr != strings_end )
        SO::AppendWithSeparator(result, get_string(*strings_itr), separator_sv);

    return result;
}


template<typename T, typename CF>
std::wstring SO::CreateSingleStringUsingCallback(const T& objects, CF callback_function, wstring_view separator_sv/* = _T(", ")*/)
{
    if( objects.empty() )
        return std::wstring();

    auto objects_itr = objects.cbegin();
    const auto& objects_end = objects.cend();

    std::wstring result = callback_function(*objects_itr);

    while( ++objects_itr != objects_end )
        SO::AppendWithSeparator(result, callback_function(*objects_itr), separator_sv);

    return result;
}


inline std::wstring SO::CreateColonSeparatedString(std::wstring text1, wstring_view text2_sv)
{
    if( text1.empty() )
        return text2_sv;

    if( !text2_sv.empty() )
    {
        text1.append(_T(": "));
        text1.append(text2_sv);
    }

    return text1;
}


inline std::wstring SO::CreateParentheticalExpression(std::wstring text1, wstring_view text2_sv)
{
    if( text1.empty() )
        return text2_sv;

    if( !text2_sv.empty() )
    {
        text1.append(_T(" ("));
        text1.append(text2_sv);
        text1.append(_T(")"));
    }

    return text1;
}



// --------------------------------------------------------------------------
// Worker functions
// --------------------------------------------------------------------------

template<typename ST>
constexpr bool SO::StringTypeHasPrecalculatedLength() noexcept
{
    if constexpr(std::is_same_v<ST, std::wstring> ||
                 std::is_same_v<ST, std::wstring_view> ||
                 std::is_same_v<ST, wstring_view> ||
                 std::is_same_v<ST, NullTerminatedStringView> ||
                 std::is_same_v<ST, StringNoCase> ||
                 std::is_same_v<ST, CString>)
    {
        return true;
    }

    else
    {
        return false;
    }
}


template<typename ST>
const wchar_t* SO::GetStringData(const ST& sv_or_cstr)
{
    if constexpr(( !std::is_same_v<ST, CString> && StringTypeHasPrecalculatedLength<ST>() ) ||
                 ( std::is_same_v<ST, NullTerminatedString> ))
    {
        return sv_or_cstr.data();
    }

    else
    {
        return static_cast<const wchar_t*>(sv_or_cstr);
    }
}
