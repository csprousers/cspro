#pragma once

#include <zToolsO/zToolsO.h>
#include <zToolsO/CallbackFunctionProcessor.h>
#include <zToolsO/StringView.h>
#include <zToolsO/NullTerminatedString.h>
#include <zToolsO/string_sz.h>
#include <cwctype>

class InterfaceString;
class SharableString;


#ifdef USING_CSTRING
// eventually, if we move away from using CString, we can remove these calls
inline CString WS2CS(const std::wstring& text) { return CString(text.c_str(), static_cast<int>(text.size())); }
inline std::wstring CS2WS(const CString& text) { return std::wstring(text.GetString(), static_cast<std::wstring::size_type>(text.GetLength())); }
CLASS_DECL_ZTOOLSO const CString& WS2CS_Reference(const std::wstring& text);
CLASS_DECL_ZTOOLSO const std::wstring& CS2WS_Reference(const CString& text);
#endif


class CLASS_DECL_ZTOOLSO UTF8_TODO
{
public:
    template<typename T>
    static const T& Create_Reference(std::string_view text_sv);

    static const wchar_t* Create_wide_c_str(std::string_view text_sv) { return Create_Reference<std::wstring>(text_sv).c_str(); }

    static const std::string& Create_Reference(wstring_view text_sv);
    static const SharableString& Create_SharableStringReference(wstring_view text_sv);

    static std::wstring GetWide(const std::string& text);
    static std::wstring GetWide(std::string_view text_sv);
    static std::wstring GetWide(cs::string_view_sz file_path);
    static std::wstring GetWide(const char* text);

    static std::wstring EnsureWide(const std::string& text)         { return GetWide(text); }
    static std::wstring EnsureWide(std::string_view text_sv)        { return GetWide(text_sv); }
    static std::wstring EnsureWide(const char* const& text)         { return GetWide(text); }
#ifdef USING_CSTRING
    static std::wstring EnsureWide(const CString& text)             { return CS2WS(text); }
#endif
    static const std::wstring& EnsureWide(const std::wstring& text) { return text; }
    static const wchar_t* EnsureWide(const wchar_t* const text)     { return text; }
    static std::wstring EnsureWide(InterfaceString text);

    static std::string EnsureUtf8(const std::wstring& text)       { return GetUtf8(text); }
    static std::string EnsureUtf8(const wchar_t* const& text)     { return GetUtf8(text); }
#ifdef USING_CSTRING
    static std::string EnsureUtf8(const CString& text)            { return GetUtf8(text); }
#endif
    static const std::string& EnsureUtf8(const std::string& text) { return text; }
    static std::string_view EnsureUtf8(std::string_view text_sv)  { return text_sv; }
    static const char* EnsureUtf8(const char* const text)         { return text; }

#ifdef USING_CSTRING
    template<typename T>
    static CString GetCString(const T& text_or_sv);
#endif

    static std::string GetUtf8(wstring_view text_sv);

    static std::optional<std::wstring> GetOptionalWide(const std::optional<std::string>& optional_text);
    static std::optional<std::string> GetOptionalUtf8(const std::optional<std::wstring>& optional_text);

    static std::vector<std::string> GetUtf8(const std::vector<std::wstring>& texts);
    static std::vector<std::wstring> GetWide(const std::vector<std::string>& texts);
    static std::vector<std::wstring> GetWide(const std::vector<SharableString>& texts);
#ifdef USING_CSTRING
    static std::vector<CString> GetCString(const std::vector<std::string>& texts);
#endif
};


class SO // SO = string operations
{
    friend SharableString;

public:
    // --------------------------------------------------------------------------
    // Empty string objects that can be used by methods that return const
    // references
    // --------------------------------------------------------------------------

    CLASS_DECL_ZTOOLSO static const std::string Empty_string;
    CLASS_DECL_ZTOOLSO static const std::shared_ptr<const std::string> Empty_shared_string;
    CLASS_DECL_ZTOOLSO static const std::wstring Empty_wstring;
#ifdef USING_CSTRING
    CLASS_DECL_ZTOOLSO static const CString Empty_CString;
#endif


    // --------------------------------------------------------------------------
    // Definitional functions
    // --------------------------------------------------------------------------

    // returns true if the character has the same value in UTF-8 and wide formats
    template<typename CT>
    static constexpr bool CharacterIsSameInWideAndUtf8(CT ch) noexcept;

    // returns true if the string type uses wide characters
    template<typename ST>
    static constexpr bool StringIsWide() noexcept;


    // --------------------------------------------------------------------------
    // Access functions
    // --------------------------------------------------------------------------

    // returns the object's null-terminated string
    template<typename ST>
    static const char* GetNullTerminatedString(const ST& text);


    // --------------------------------------------------------------------------
    // Length functions
    // --------------------------------------------------------------------------

    // returns the length of all of the arguments
    template<typename T, typename... Args>
    static size_t GetLength(const T& arg1, Args const&... args);


    // --------------------------------------------------------------------------
    // Wide character routines that work on UTF-8 text
    // --------------------------------------------------------------------------

    // returns the length of the string in wide characters
    template<typename ST>
    static size_t WideLength(const ST& text_or_sv);

    // returns the offset into the string, measured in wide characters;
    // the offset does not need to be valid, and if not found, std::string_view::npos is returned
    CLASS_DECL_ZTOOLSO static size_t WideGetOffset(std::string_view text_sv, size_t wide_offset);

    // returns the offset into the string, measured in wide characters;
    // the offset must be valid;
    // can also return const char* / char*
    template<typename RT = size_t, typename CT>
    CLASS_DECL_ZTOOLSO static RT WideGetOffset(CT* text, size_t wide_offset);

    // returns a substring with the offset and count measured in wide characters;
    // note that a std::string_view, not a std::string, is returned
    CLASS_DECL_ZTOOLSO static std::string_view WideSubstring(std::string_view text_sv, size_t wide_offset, size_t wide_count = std::string_view::npos);

    // sets the character at the wide offset
    CLASS_DECL_ZTOOLSO static void WideSetChar(std::string& text, size_t wide_offset, wchar_t ch);

    // sets the length of the string to the desired length in wide characters, padding with spaces if necessary
    template<bool insert_spaces_at_end = true>
    static std::string& WideMakeExactLength(std::string& text, size_t wide_length);

    // sets the length of the string to the desired length in wide characters, centering the string (when padding is necessary)
    CLASS_DECL_ZTOOLSO static std::string& WideCenterExactLength(std::string& text, size_t wide_length);

    // iterates over each wide character in the string
    CLASS_DECL_ZTOOLSO static void ForeachWideChar(std::string_view text_sv, const std::function<void(wchar_t)>& callback_function);


    // --------------------------------------------------------------------------
    // Case functions
    // --------------------------------------------------------------------------

    // transforms the wide character to uppercase/lowercase
    template<bool ToUpper>
    static wchar_t WideCharToCase(wchar_t ch);

    static wchar_t WideCharToUpper(wchar_t ch) { return WideCharToCase<true>(ch); }
    static wchar_t WideCharToLower(wchar_t ch) { return WideCharToCase<false>(ch); }

    // transforms the string to uppercase characters
    static std::string& MakeUpper(std::string& text);

    template<typename ST>
    static std::string ToUpper(ST&& text_or_sv);

    static bool IsUpper(std::string_view text_sv);

    static std::wstring& MakeUpper(std::wstring& text);
    static std::wstring ToUpper(wstring_view text_sv);
    static bool IsUpper(wstring_view text_sv);

    // transforms the string to lowercase characters
    static std::string& MakeLower(std::string& text);

    template<typename ST>
    static std::string ToLower(ST&& text_or_sv);

    static bool IsLower(std::string_view text_sv);

    CLASS_DECL_ZTOOLSO static std::wstring& MakeLower(std::wstring& text);

    static std::wstring ToLower(wstring_view text_sv);
    static bool IsLower(wstring_view text_sv);

    CLASS_DECL_ZTOOLSO static std::string ToProperCase(std::string text);
    CLASS_DECL_ZTOOLSO static std::string TitleToCamelCase(std::string text);


    // --------------------------------------------------------------------------
    // Comparison functions
    // --------------------------------------------------------------------------

    // returns whether the string is empty or only contains space (' ') characters
    template<typename ST>
    static bool IsBlank(const ST& text);

    // returns whether the character is a whitespace character (as determined by std::iswspace)
    template<typename CT>
    static constexpr bool IsWhitespaceChar(CT ch);

    // returns whether the string is empty or only contains whitespace characters (as determined by std::iswspace)
    template<typename ST>
    static bool IsWhitespace(const ST& text);

    // common whitespace characters
    static constexpr const char* WhitespaceChars = " \n\r\t\v\f";
    static constexpr std::string_view WhitespaceChars_sv = WhitespaceChars;

    static bool Equals(wstring_view sv1, wstring_view sv2) noexcept;
    CLASS_DECL_ZTOOLSO static bool Equals(std::string_view sv1, wstring_view sv2);
    static bool Equals(wstring_view sv1, std::string_view sv2) { return Equals(sv2, sv1); }

    // std::string_view's operator== promotes null-terminated strings to string views,
    // which is wasteful, so these Equals methods provide an alternative
    static bool Equals(std::string_view sv1, const char* text2) noexcept;
    static bool Equals(const char* text1, std::string_view sv2) noexcept { return Equals(sv2, text1); }

    CLASS_DECL_ZTOOLSO static int CompareNoCase(std::string_view sv1, std::string_view sv2);
    CLASS_DECL_ZTOOLSO static int CompareNoCase(wstring_view sv1, wstring_view sv2);

    // returns whether the two strings are equal in a case-insensitive manner
    template<typename ST1, typename ST2>
    static bool EqualsNoCase(const ST1& text1, const ST2& text2);

    template<typename... Args>
    static bool EqualsOneOf(std::string_view source_sv, std::string_view compare1_sv, Args const&... compare2_and_more);

    template<typename... Args>
    static bool EqualsOneOfNoCase(std::string_view source_sv, std::string_view compare1_sv, Args const&... compare2_and_more);

    template<typename... Args>
    static bool EqualsOneOfNoCase(wstring_view source_sv, wstring_view compare1_sv, Args const&... compare2_and_more);

    // returns whether the source string starts with the entire second string
    template<typename ST1, typename ST2>
    static bool StartsWith(const ST1& source, const ST2& starts_with_text);

    // returns whether the source string starts with the entire second string
    // in a case-insensitive manner
    template<typename ST1, typename ST2>
    static bool StartsWithNoCase(const ST1& source, const ST2& starts_with_text);


    // --------------------------------------------------------------------------
    // Search functions
    // --------------------------------------------------------------------------

    // searches in a case-insensitive manner, the source string, for the substring;
    // if not found, std::string_view::npos (-1) is returned
    CLASS_DECL_ZTOOLSO static size_t FindNoCase(std::string_view source_sv, std::string_view substring_sv, size_t offset = 0);

    // returns the indices of two characters, with the second character coming after the first;
    // if both are not found, then the second returned value will be equal to npos
    static std::tuple<size_t, size_t> FindCharacters(std::string_view text_sv, char ch1, char ch2, size_t offset = 0);
    CLASS_DECL_ZTOOLSO static std::tuple<size_t, size_t> FindCharacters(wstring_view text_sv, wchar_t ch1, wchar_t ch2, size_t offset = 0);

    // returns the index of the first whitespace character in the source string;
    // if not found, std::string_view::npos (-1) is returned
    CLASS_DECL_ZTOOLSO static size_t FindFirstWhitespace(std::string_view source_sv, size_t offset = 0);

    // returns the substring between the two characters found using the above function;
    // if both are not found, then an empty string view is returned
    CLASS_DECL_ZTOOLSO static std::string_view GetTextBetweenCharacters(std::string_view text_sv, char ch1, char ch2);

    // returns the strings to the left and right of the character;
    // if the character is not found, the left string will be equal to the entire string and the right string will be empty
    CLASS_DECL_ZTOOLSO static std::tuple<std::string_view, std::string_view> GetTextOnEitherSideOfCharacter(std::string_view text_sv, char ch);


    // --------------------------------------------------------------------------
    // Newline functions
    // --------------------------------------------------------------------------

    // newline characters handled throughout the code
    static constexpr std::string_view Newline_crlf_sv = "\r\n";
    static constexpr std::string_view Newline_lf_sv   = "\n";

    // indicates whether a '\n' newline character exists in the string
    template<typename ST>
    static bool ContainsNewlineCharacter(const ST& text);

    // makes sure that newlines are "\r\n", not just "\n"
    static void MakeNewlineCRLF(std::string& text);
    static std::string ToNewlineCRLF(std::string text);

    // makes sure that newlines are only "\n", not "\r\n"
    static void MakeNewlineLF(std::string& text);
    static std::string ToNewlineLF(std::string text);
    CLASS_DECL_ZTOOLSO static void MakeNewlineLF(std::wstring& text);
    static std::wstring ToNewlineLF(std::wstring text);

    // takes a vector of strings and, if any strings contain newlines, those strings will be split into separate vector entries
    CLASS_DECL_ZTOOLSO static void MakeSplitVectorStringsOnNewlines(std::vector<std::string>& strings);


    // --------------------------------------------------------------------------
    // Manipulation functions
    // --------------------------------------------------------------------------

    // trims whitespace characters from the left or right side of the string, returning a string_view
    template<typename ST>
    static auto TrimLeft(const ST& text_or_sv);

    template<typename ST>
    static auto TrimRight(const ST& text_or_sv);

    // trims a specific character from the left or right side of the string, returning a string_view
    template<typename ST>
    static auto TrimLeft(const ST& text_or_sv, char trim_char) { return SO::TrimLeftRightCharWorker<true, ST, char>(text_or_sv, trim_char); }

    template<typename ST>
    static auto TrimLeft(const ST& text_or_sv, wchar_t trim_char) { return SO::TrimLeftRightCharWorker<true, ST, wchar_t>(text_or_sv, trim_char); }

    template<typename ST>
    static auto TrimRight(const ST& text_or_sv, char trim_char) { return SO::TrimLeftRightCharWorker<false, ST, char>(text_or_sv, trim_char); }

    template<typename ST>
    static auto TrimRight(const ST& text_or_sv, wchar_t trim_char) { return SO::TrimLeftRightCharWorker<false, ST, wchar_t>(text_or_sv, trim_char); }

    // trims space (' ') characters from the left or right side of the string, returning a string_view
    template<typename ST>
    static auto TrimLeftSpace(const ST& text_or_sv) { return SO::TrimLeft(text_or_sv, ' '); }

    template<typename ST>
    static auto TrimRightSpace(const ST& text_or_sv) { return SO::TrimRight(text_or_sv, ' '); }

    // trims any of the specified trim characters from the left side of the string, returning a string_view
    template<typename ST>
    static auto TrimLeft(const ST& text_or_sv, const std::string_view& trim_chars_sv) { return SO::TrimLeftRightStringViewWorker<true, ST>(text_or_sv, trim_chars_sv); }

    template<typename ST>
    static auto TrimLeft(const ST& text_or_sv, const wstring_view& trim_chars_sv) { return SO::TrimLeftRightStringViewWorker<true, ST>(text_or_sv, trim_chars_sv); }

    template<typename ST>
    static auto TrimRight(const ST& text_or_sv, const std::string_view& trim_chars_sv) { return SO::TrimLeftRightStringViewWorker<false, ST>(text_or_sv, trim_chars_sv); }

    template<typename ST>
    static auto TrimRight(const ST& text_or_sv, const wstring_view& trim_chars_sv) { return SO::TrimLeftRightStringViewWorker<false, ST>(text_or_sv, trim_chars_sv); }

    // left- and right-side trimming versions of the above
    template<typename ST, typename... Args>
    static auto Trim(const ST& text_or_sv, Args const&... args);

    // left- and right-side trimming versions of the above that bind the changes to the modifiable string argument
    template<typename ST, typename... Args>
    static ST& MakeTrim(ST& text, Args const&... args) { return MakeTrimWorker<false, ST>(text, SO::Trim(text, args...)); }

    template<typename ST, typename... Args>
    static ST& MakeTrimLeft(ST& text, Args const&... args) { return MakeTrimWorker<false, ST>(text, SO::TrimLeft(text, args...)); }

    template<typename ST, typename... Args>
    static ST& MakeTrimRight(ST& text, Args const&... args) { return MakeTrimWorker<true, ST>(text, SO::TrimRight(text, args...)); }

    template<typename ST>
    static ST& MakeTrimRightSpace(ST& text) { return MakeTrimWorker<true, ST>(text, SO::TrimRightSpace(text)); }

    // replaces all instances of the old character with the new character
    CLASS_DECL_ZTOOLSO static std::string& Replace(std::string& text, char old_char, char new_char, std::string::size_type ch_pos = 0);
    CLASS_DECL_ZTOOLSO static std::wstring& Replace(std::wstring& text, wchar_t old_char, wchar_t new_char, std::wstring::size_type ch_pos = 0);

    // replaces all instances of the old text with the new text
    CLASS_DECL_ZTOOLSO static std::string& Replace(std::string& text, std::string_view old_text_sv, std::string_view new_text_sv, std::string::size_type ch_pos = 0);
    CLASS_DECL_ZTOOLSO static std::wstring& Replace(std::wstring& text, wstring_view old_text_sv, wstring_view new_text_sv, std::wstring::size_type ch_pos = 0);

    CLASS_DECL_ZTOOLSO static std::string& ReplaceNoCase(std::string& text, std::string_view old_text_sv, std::string_view new_text_sv, std::string::size_type ch_pos = 0);

    // replaces all instances of the old text with the new text, calling the Replace function repeatedly
    // until no more instances of the text are replaced;
    // this function can be called if the new text contains text that may also be part of the old text
    CLASS_DECL_ZTOOLSO static std::string& RecursiveReplace(std::string& text, std::string_view old_text_sv, std::string_view new_text_sv);

    // removes all instances of the character
    CLASS_DECL_ZTOOLSO static std::string& Remove(std::string& text, char ch);
    CLASS_DECL_ZTOOLSO static std::wstring& Remove(std::wstring& text, wchar_t ch);

    // removes all whitespace characters from the text
    CLASS_DECL_ZTOOLSO static std::string RemoveWhitespace(std::string_view text_sv);

    // sets the length of the string to the desired length, padding with spaces if necessary
    template<bool insert_spaces_at_end = true, typename ST>
    static ST& MakeExactLength(ST& text, size_t length);

    // appends text, or formatted text, to the destination string
    template<typename T, typename... Args>
    static T& Append(T& destination, wstring_view text1_sv, Args... textN);

    template<typename T>
    static T& Append(T& destination, wstring_view text_sv);

    template<typename T, typename... Args>
    static T& AppendFormat(T& destination, const wchar_t* formatter, Args const&... args);

    // appends text with the separator added if the destination string is not blank
    template<typename AppendT, typename SeparatorT>
    static std::string& AppendWithSeparator(std::string& destination, AppendT&& text_to_append, SeparatorT&& separator);

    CLASS_DECL_ZTOOLSO static std::wstring& AppendWithSeparator(std::wstring& destination, wstring_view text_sv, wchar_t separator);

    template<typename T>
    static T& AppendWithSeparator(T& destination, wstring_view text_sv, wstring_view separator_sv);

    // concatenates the strings, returning the combined string
    template<typename T, typename... Args>
    static std::string Concatenate(T&& arg1, Args const&... args);

    template<typename... Args>
    static std::wstring ConcatenateWS(std::wstring text1, Args... textN);

    // removes any text following the character; the character is optionally removed
    CLASS_DECL_ZTOOLSO static std::string_view RemoveTextFollowingCharacter(std::string_view text_sv, char ch, bool remove_ch);

    // the default number of spaces per tab and a space string that can be used for the tabs
    static constexpr int DefaultSpacesPerTab = 4;
    static constexpr const char* SingleTabAsSpaces = "    ";

    // converts tabs to spaces, using DefaultSpacesPerTab as the number of spaces per tab
    CLASS_DECL_ZTOOLSO static int ConvertTabsToSpaces(std::string& text, int position_in_line = 0);

    // converts tabs to spaces and right-trims each line
    CLASS_DECL_ZTOOLSO static void ConvertTabsToSpacesAndTrimRightEachLine(std::string& text);


    // --------------------------------------------------------------------------
    // Copying functions
    // --------------------------------------------------------------------------

    // Copies the source string to the destination buffer, ensuring that the destination string is null-terminated.
    // If the source string is longer than the destination buffer, it will be truncated.
    static void CopyToFixedBuffer(char* destination, std::string_view source_sv, size_t destination_size);
    static void CopyToFixedBuffer(wchar_t* destination, std::wstring_view source_sv, size_t destination_size);

    template<typename CT, size_t destination_size, typename ST>
    static void CopyToFixedBuffer(CT (&destination)[destination_size], ST&& source);


    // --------------------------------------------------------------------------
    // Parsing functions
    // --------------------------------------------------------------------------

    // the callback function is passed each section; sections are determined by splitting the text
    // using one of the separators; the callback function should return true to keep processing
    template<typename ST, typename SeparatorT, typename CF>
    static void ForeachSection(const ST& text_or_sv, const SeparatorT& separators, const CF& callback_function);

    // the callback function should return true to keep processing;
    // the callback function parameter type can also be std::string
    template<typename CFPT = std::string_view, typename CF>
    static void ForeachLine(std::string_view text_sv, bool process_whitespace_lines, const CF& callback_function);
    template<typename CF>
    static void ForeachLine(wstring_view text_sv, bool process_whitespace_lines, const CF& callback_function);


    // --------------------------------------------------------------------------
    // Other functions
    // --------------------------------------------------------------------------

    // splits the string at any provided separators; unlike functions like std::wcstok, multiple separators in a row will
    // be treated as separating entities (e.g., "a;;b;c" would result in 4 [not 3] entities with ; as the separator);
    // the include_empty_entities flag can be set to false to receive results like std::wcstok
    template<typename RT = std::string, typename SeparatorT>
    static std::vector<RT> SplitString(std::string_view text_sv, const SeparatorT& separators, bool trim_all = true, bool include_empty_entities = true)
    {
        return SplitStringWorker<RT, SeparatorT>(text_sv, separators, trim_all, include_empty_entities);
    }

    template<typename RT = std::wstring, typename SeparatorT>
    static std::vector<RT> SplitString(wstring_view text_sv, const SeparatorT& separators, bool trim_all = true, bool include_empty_entities = true)
    {
        return SplitStringWorker<RT, SeparatorT>(text_sv, separators, trim_all, include_empty_entities);
    }

    // combines the strings into a single string, separating each with the separator
    template<bool right_trim_each_string = true, typename T>
    static std::string CreateSingleString(const T& strings, std::string_view separator_sv = ", ");

    // combines the objects into a single string by calling the callback on each object, separating each with the separator
    template<typename T, typename CF>
    static std::string CreateSingleStringUsingCallback(const T& objects, const CF& callback_function, std::string_view separator_sv = ", ");

    // if text2_sv is not empty, it is appended to text1, separated by a colon
    CLASS_DECL_ZTOOLSO static std::string CreateColonSeparatedString(std::string text1, std::string_view text2_sv);

    // if text2_sv is not empty, it is appended to text1, wrapped in parentheses
    CLASS_DECL_ZTOOLSO static std::string CreateParentheticalExpression(std::string text1, std::string_view text2_sv);

    // wraps the text at newline characters and width boundaries, adding hyphens as necessary
    CLASS_DECL_ZTOOLSO static std::vector<std::wstring> WrapText(wstring_view text_sv, size_t maximum_line_width);

    // returns a C string that contains the specified number of repeating characters
    CLASS_DECL_ZTOOLSO static const char* GetRepeatingCharacterString(wchar_t ch, size_t length);

    static const char* GetDashedLine(size_t length) { return GetRepeatingCharacterString('-', length); }

    // returns a vector of bytes with the text and an optional null terminator
    CLASS_DECL_ZTOOLSO static std::vector<std::byte> CreateByteVector(std::string_view text_sv, bool include_null_terminator = false);


    // --------------------------------------------------------------------------
    // Worker functions
    // --------------------------------------------------------------------------
private:
    template<typename ST>
    static constexpr bool StringTypeHasPrecalculatedLength() noexcept;

    template<typename ST>
    static const auto* GetStringData(const ST& sv_or_cstr);

    CLASS_DECL_ZTOOLSO static size_t WideLengthWorker(const char* text_start, const char* text_end);
    CLASS_DECL_ZTOOLSO static size_t WideLengthWorker(const char* text_start);

    // returns the offset from the end of a UTF-8 string in wide characters;
    // e.g., WideGetOffsetFromEnd(u8"abcdéf", 2) == 4
    CLASS_DECL_ZTOOLSO static size_t WideGetOffsetFromEnd(std::string_view text_sv, size_t wide_offset);

    template<bool insert_spaces_at_end = true>
    CLASS_DECL_ZTOOLSO static void WideMakeExactLengthAdjuster(std::string& text, const ptrdiff_t non_zero_length_difference);

    template<typename ST1, typename ST2, bool EqualsMode, bool NoCase>
    static bool EqualsStartsWithWorker(const ST1& source, const ST2& starts_with_text);

    template<typename CT, typename Predicate>
    static auto TrimLeftWorker(const CT* text, size_t length, const Predicate& predicate);

    template<typename CT, typename Predicate>
    static auto TrimRightWorker(const CT* text, size_t length, const Predicate& predicate);

    template<bool trim_left, typename ST, typename CT>
    static auto TrimLeftRightCharWorker(const ST& text_or_sv, CT trim_char);

    template<bool trim_left, typename ST, typename SVT>
    static auto TrimLeftRightStringViewWorker(const ST& text_or_sv, const SVT& trim_chars_sv);

    template<bool is_from_trim_right, typename ST, typename SVT>
    static ST& MakeTrimWorker(ST& text, SVT trimmed_text_sv);

    template<bool ToUpper>
    CLASS_DECL_ZTOOLSO static wchar_t WideCharToCaseWithLocale(wchar_t ch);

    template<bool ToUpper>
    CLASS_DECL_ZTOOLSO static size_t FindFirstNonMatchingWideCase(std::string_view text_sv);

    // out_text should be null (in which case a new string is created with the modified case (if necessary)
    // or should be set to in_text, in which case the modified case string replaces the input text
    template<bool ToUpper>
    CLASS_DECL_ZTOOLSO static void MakeWideCaseWorker(const std::string& in_text, std::string*& out_text);

    template<typename CT>
    CLASS_DECL_ZTOOLSO static void CopyToFixedBufferWorker(CT* destination, size_t destination_size, const CT* source, size_t source_length);

    template<bool trim_right_each_line>
    static int ConvertTabsToSpacesWorker(std::string& text, int position_in_line);

    template<typename RT, typename SeparatorT, typename SVT>
    static std::vector<RT> SplitStringWorker(SVT text_sv, const SeparatorT& separators, bool trim_all, bool include_empty_entities);

private:
    static const std::locale m_localeForCaseConversions;
};



// --------------------------------------------------------------------------
// Definitional functions
// --------------------------------------------------------------------------

template<typename CT>
constexpr bool SO::CharacterIsSameInWideAndUtf8(const CT ch) noexcept
{
    return ( ch <= static_cast<CT>(0x7F) );
}


template<typename ST>
constexpr bool SO::StringIsWide() noexcept
{
    if constexpr(std::is_convertible_v<ST, std::string> ||
                 std::is_convertible_v<ST, std::string_view> ||
                 std::is_convertible_v<ST, cs::string_sz> ||
                 std::is_convertible_v<ST, const char*>)
    {
        return false;
    }

    else
    {
        return true;
    }
}



// --------------------------------------------------------------------------
// Worker functions
// --------------------------------------------------------------------------

template<typename ST>
constexpr bool SO::StringTypeHasPrecalculatedLength() noexcept
{
    if constexpr(std::is_same_v<ST, std::string> ||
                 std::is_same_v<ST, std::string_view> ||
                 std::is_same_v<ST, cs::string_view_sz> ||
                 std::is_same_v<ST, std::wstring> ||
                 std::is_same_v<ST, std::wstring_view> ||
                 std::is_same_v<ST, wstring_view>)
    {
        return true;
    }

#ifdef USING_CSTRING
    else if constexpr(std::is_same_v<ST, CString>)
    {
        return true;
    }
#endif

    else
    {
        return false;
    }
}


template<typename ST>
const auto* SO::GetStringData(const ST& sv_or_cstr)
{
    if constexpr(
#ifdef USING_CSTRING
                 ( !std::is_same_v<ST, CString> && StringTypeHasPrecalculatedLength<ST>() ) ||
#else
                 ( StringTypeHasPrecalculatedLength<ST>() ) ||
#endif
                 ( std::is_same_v<ST, cs::string_sz> ) ||
                 ( std::is_same_v<ST, NullTerminatedString> ))
    {
        return sv_or_cstr.data();
    }

    else if constexpr(StringIsWide<ST>())
    {
        return static_cast<const wchar_t*>(sv_or_cstr);
    }

    else
    {
        return static_cast<const char*>(sv_or_cstr);
    }
}



// --------------------------------------------------------------------------
// Access functions
// --------------------------------------------------------------------------

template<typename ST>
const char* SO::GetNullTerminatedString(const ST& text)
{
         if constexpr(std::is_same_v<ST, SharableString>) return text->c_str();
    else if constexpr(std::is_same_v<ST, std::string>)    return text.c_str();
    else                                                  return text;
}



// --------------------------------------------------------------------------
// Length functions
// --------------------------------------------------------------------------

template<typename T, typename... Args>
size_t SO::GetLength(const T& arg1, Args const&... args)
{
    size_t length;

    if constexpr(std::is_same_v<T, const char*> ||
                 std::is_array_v<T>) // for char[]
    {
        length = strlen(arg1);
    }

    else
    {
        length = arg1.length();
    }

    if constexpr(sizeof...(Args) != 0)
    {
        length += GetLength(args...);
    }

    return length;
}



// --------------------------------------------------------------------------
// Wide character routines that work on UTF-8 text
// --------------------------------------------------------------------------

template<typename ST>
size_t SO::WideLength(const ST& text_or_sv)
{
    if constexpr(StringTypeHasPrecalculatedLength<ST>())
    {
        return WideLengthWorker(text_or_sv.data(), text_or_sv.data() + text_or_sv.length());
    }

    else
    {
        return WideLengthWorker(GetStringData(text_or_sv));
    }
}


template<bool insert_spaces_at_end/* = true*/>
std::string& SO::WideMakeExactLength(std::string& text, const size_t wide_length)
{
#ifdef _DEBUG
    std::wstring wide_text_check = UTF8_TODO::GetWide(text);
    SO::MakeExactLength<insert_spaces_at_end>(wide_text_check, wide_length);
#endif

    const ptrdiff_t length_difference = wide_length - WideLength(text);

    if( length_difference != 0 )
        WideMakeExactLengthAdjuster<insert_spaces_at_end>(text, length_difference);

    ASSERT81(text == UTF8_TODO::GetUtf8(wide_text_check));
    return text;
}



// --------------------------------------------------------------------------
// Case functions
// --------------------------------------------------------------------------

template<bool ToUpper>
wchar_t SO::WideCharToCase(const wchar_t ch)
{
    if( CharacterIsSameInWideAndUtf8(ch) )
    {
        if constexpr(ToUpper)
        {
            ASSERT81(std::towupper(ch) == std::toupper(ch));
            return static_cast<wchar_t>(std::toupper(ch));
        }

        else
        {
            ASSERT81(std::towlower(ch) == std::tolower(ch));
            return static_cast<wchar_t>(std::tolower(ch));
        }
    }

    else
    {
        return WideCharToCaseWithLocale<ToUpper>(ch);
    }
}


inline std::string& SO::MakeUpper(std::string& text)
{
    std::string* modified_case_string = &text;
    SO::MakeWideCaseWorker<true>(text, modified_case_string);
    return text;
}


template<typename ST>
std::string SO::ToUpper(ST&& text_or_sv)
{
    std::string text(std::forward<ST>(text_or_sv));
    SO::MakeUpper(text);
    return text;
}


inline bool SO::IsUpper(const std::string_view text_sv)
{
    return ( FindFirstNonMatchingWideCase<true>(text_sv) == text_sv.length() );
}


inline std::string& SO::MakeLower(std::string& text)
{
    std::string* modified_case_string = &text;
    SO::MakeWideCaseWorker<false>(text, modified_case_string);
    return text;
}


template<typename ST>
std::string SO::ToLower(ST&& text_or_sv)
{
    std::string text(std::forward<ST>(text_or_sv));
    SO::MakeLower(text);
    return text;
}


inline bool SO::IsLower(const std::string_view text_sv)
{
    return ( FindFirstNonMatchingWideCase<false>(text_sv) == text_sv.length() );
}


inline std::wstring& SO::MakeUpper(std::wstring& text)
{
    std::transform(text.begin(), text.end(), text.begin(), std::towupper);
    return text;
}


inline std::wstring SO::ToUpper(const wstring_view text_sv)
{
    std::wstring str(text_sv.length(), 0);
    std::transform(text_sv.cbegin(), text_sv.cend(), str.begin(), std::towupper);
    return str;
}


inline bool SO::IsUpper(const wstring_view text_sv)
{
    return ( std::find_if(text_sv.cbegin(), text_sv.cend(),
                          [](const wchar_t ch) { return ( ch != std::towupper(ch) ); }) == text_sv.cend() );
}


inline std::wstring SO::ToLower(const wstring_view text_sv)
{
    std::wstring str(text_sv.length(), 0);
    std::transform(text_sv.cbegin(), text_sv.cend(), str.begin(), std::towlower);
    return str;
}


inline bool SO::IsLower(const wstring_view text_sv)
{
    return ( std::find_if(text_sv.cbegin(), text_sv.cend(),
                          [](const wchar_t ch) { return ( ch != std::towlower(ch) ); }) == text_sv.cend() );
}



// --------------------------------------------------------------------------
// Comparison functions
// --------------------------------------------------------------------------

template<typename ST>
bool SO::IsBlank(const ST& text)
{
    return ( text.empty() || std::find_if(text.cbegin(), text.cend(),
                                          [](const typename ST::value_type ch) { return ( ch != ' ' ); }) == text.cend() );

}

#ifdef USING_CSTRING
template<> inline bool SO::IsBlank(const CString& text) { return IsBlank<wstring_view>(text); }
#endif


template<typename CT>
constexpr bool SO::IsWhitespaceChar(const CT ch)
{
    return std::iswspace(static_cast<wint_t>(ch));
}


template<typename ST>
bool SO::IsWhitespace(const ST& text)
{
    return ( text.empty() ||
             std::find_if_not(text.cbegin(), text.cend(), SO::IsWhitespaceChar<typename ST::value_type>) == text.cend() );
}

#ifdef USING_CSTRING
template<> inline bool SO::IsWhitespace(const CString& text) { return SO::IsWhitespace<wstring_view>(text); }
#endif


inline bool SO::Equals(const wstring_view sv1, const wstring_view sv2) noexcept
{
    return ( sv1 == sv2 );
}


inline bool SO::Equals(const std::string_view sv1, const char* const text2) noexcept
{
    return EqualsStartsWithWorker<std::string_view, const char*, true, false>(sv1, text2);
}


template<typename ST1, typename ST2>
bool SO::EqualsNoCase(const ST1& text1, const ST2& text2)
{
    return EqualsStartsWithWorker<ST1, ST2, true, true>(text1, text2);
}


template<typename... Args>
bool SO::EqualsOneOf(const std::string_view source_sv, const std::string_view compare1_sv, Args const&... compare2_and_more)
{
    static_assert(sizeof...(compare2_and_more) > 0, "Use SO::Equals if only comparing against one string");

    for( const std::string_view compare_sv : std::initializer_list<std::string_view> { compare1_sv, compare2_and_more... } )
    {
        if( source_sv == compare_sv )
            return true;
    }

    return false;
}


template<typename... Args>
bool SO::EqualsOneOfNoCase(std::string_view source_sv, const std::string_view compare1_sv, Args const&... compare2_and_more)
{
    static_assert(sizeof...(compare2_and_more) > 0, "Use SO::EqualsNoCase if only comparing against one string");

    for( const std::string_view compare_sv : std::initializer_list<std::string_view> { compare1_sv, compare2_and_more... } )
    {
        if( EqualsNoCase(source_sv, compare_sv) )
            return true;
    }

    return false;
}


template<typename... Args>
bool SO::EqualsOneOfNoCase(const wstring_view source_sv, const wstring_view compare1_sv, Args const&... compare2_and_more)
{
    static_assert(sizeof...(compare2_and_more) > 0, "Use SO::EqualsNoCase if only comparing against one string");

    for( const wstring_view compare_sv : std::initializer_list<wstring_view> { compare1_sv, compare2_and_more... } )
    {
        if( EqualsNoCase(source_sv, compare_sv) )
            return true;
    }

    return false;
}


template<typename ST1, typename ST2>
bool SO::StartsWith(const ST1& source, const ST2& starts_with_text)
{
    // C++20 has a starts_with operator, but in the meantime:
    return EqualsStartsWithWorker<ST1, ST2, false, false>(source, starts_with_text);
}


template<typename ST1, typename ST2>
bool SO::StartsWithNoCase(const ST1& source, const ST2& starts_with_text)
{
    return SO::EqualsStartsWithWorker<ST1, ST2, false, true>(source, starts_with_text);
}


template<typename ST1, typename ST2, bool EqualsMode, bool NoCase>
bool SO::EqualsStartsWithWorker(const ST1& source, const ST2& starts_with_text)
{
    // temporarily handle strings of different types
    if constexpr(StringIsWide<ST1>() != StringIsWide<ST2>())
    {
        return EqualsStartsWithWorker<std::string_view, std::string_view, EqualsMode, NoCase>(UTF8_TODO::EnsureUtf8(source), UTF8_TODO::EnsureUtf8(starts_with_text));
    }

#ifdef USING_CSTRING
    // handle CString arguments as string views
    else if constexpr(std::is_same_v<ST1, CString>)
    {
        return EqualsStartsWithWorker<wstring_view, ST2, EqualsMode, NoCase>(source, starts_with_text);
    }

    else if constexpr(std::is_same_v<ST2, CString>)
    {
        return EqualsStartsWithWorker<ST1, wstring_view, EqualsMode, NoCase>(source, starts_with_text);
    }
#endif

    // process either strings, string_views, or null-terminated strings
    else
    {
        size_t length_remaining;

        if constexpr(StringTypeHasPrecalculatedLength<ST2>())
        {
            length_remaining = starts_with_text.length();

            if constexpr(StringTypeHasPrecalculatedLength<ST1>())
            {
                if constexpr(EqualsMode)
                {
                    if( length_remaining != source.length() )
                        return false;
                }

                else
                {
                    if( length_remaining > source.length() )
                        return false;
                }
            }
        }

        else if constexpr(StringTypeHasPrecalculatedLength<ST1>())
        {
            length_remaining = source.length();
        }

        // case-insensitive comparisons on UTF-8 strings will be handled using CompareNoCase
        if constexpr(NoCase && !StringIsWide<ST1>())
        {
            if constexpr(EqualsMode)
            {
                return ( CompareNoCase(source, starts_with_text) == 0 );
            }

            else
            {
                const std::string_view& source_sv = source;
                const std::string_view& starts_with_text_sv = starts_with_text;
                return ( CompareNoCase(source_sv.substr(0, starts_with_text_sv.length()), starts_with_text_sv) == 0 );
            }
        }

        else
        {
            using char_type = typename std::conditional<StringIsWide<ST1>(), wchar_t, char>::type;
            const char_type* itr1 = GetStringData<ST1>(source);
            const char_type* itr2 = GetStringData<ST2>(starts_with_text);

            while( true )
            {
                // get the characters for both strings
                char_type ch1;

                if constexpr(StringTypeHasPrecalculatedLength<ST1>())
                {
                    ch1 = ( length_remaining > 0 ) ? *itr1 : 0;
                }

                else
                {
                    ch1 = *itr1;
                }

                char_type ch2;

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
                if constexpr(NoCase)
                {
                    static_assert(StringIsWide<ST1>());

                    if( std::towupper(ch1) != std::towupper(ch2) )
                        return false;
                }

                else
                {
                    if( ch1 != ch2 )
                        return false;
                }

                // advance the iterators
                ++itr1;
                ++itr2;

                if constexpr(StringTypeHasPrecalculatedLength<ST1>() || StringTypeHasPrecalculatedLength<ST2>())
                    --length_remaining;
            }
        }
    }
}



// --------------------------------------------------------------------------
// Search functions
// --------------------------------------------------------------------------

inline std::tuple<size_t, size_t> SO::FindCharacters(const std::string_view text_sv, const char ch1, const char ch2, const size_t offset/* = 0*/)
{
    const size_t ch1_pos = text_sv.find(ch1, offset);
    return std::make_tuple(ch1_pos, ( ch1_pos != std::string_view::npos ) ? text_sv.find(ch2, ch1_pos + 1) :
                                                                            std::string_view::npos);
}



// --------------------------------------------------------------------------
// Newline functions
// --------------------------------------------------------------------------

template<typename ST>
bool SO::ContainsNewlineCharacter(const ST& text)
{
    if constexpr(std::is_same_v<ST, CString>)
    {
        return ( text.Find('\n') >= 0 );
    }

    else
    {
        return ( text.find('\n') != ST::npos );
    }
}


inline void SO::MakeNewlineCRLF(std::string& text)
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


inline std::string SO::ToNewlineCRLF(std::string text)
{
    SO::MakeNewlineCRLF(text);
    return text;
}


inline void SO::MakeNewlineLF(std::string& text)
{
    // this routine assumes that newlines come in as either \r\n or only as \n, not only \r
    ASSERT(( std::count(text.cbegin(), text.cend(), '\r') == std::count(text.cbegin(), text.cend(), '\n') ) ||
           ( std::count(text.cbegin(), text.cend(), '\r') == 0 ));

#ifdef _DEBUG
    std::string replace_test = text;
#endif

    SO::Remove(text, '\r');

#ifdef _DEBUG
    SO::Replace(replace_test, "\r\n", "\n");
    SO::Replace(replace_test, "\r", "\n");
    ASSERT(replace_test == text);
#endif
}


inline std::string SO::ToNewlineLF(std::string text)
{
    SO::MakeNewlineLF(text);
    return text;
}


inline std::wstring SO::ToNewlineLF(std::wstring text)
{
    SO::MakeNewlineLF(text);
    return text;
}



// --------------------------------------------------------------------------
// Manipulation functions
// --------------------------------------------------------------------------

template<typename CT, typename Predicate>
auto SO::TrimLeftWorker(const CT* const text, const size_t length, const Predicate& predicate)
{
    using string_view_type = typename std::conditional<std::is_same_v<CT, wchar_t>, wstring_view, std::string_view>::type;

    if( length == 0 )
        return string_view_type();

    const CT* text_itr = text;
    const CT* const text_end = text + length;

    while( predicate(*text_itr) && ++text_itr != text_end )
    {
    }

    return string_view_type(text_itr, text_end - text_itr);
}


template<typename CT, typename Predicate>
auto SO::TrimRightWorker(const CT* const text, size_t length, const Predicate& predicate)
{
    using string_view_type = typename std::conditional<std::is_same_v<CT, wchar_t>, wstring_view, std::string_view>::type;

    if( length == 0 )
        return string_view_type();

    const CT* text_itr = text + length - 1;

    while( length > 0 && predicate(*text_itr) )
    {
        --text_itr;
        --length;
    }

    return string_view_type(text, length);
}


template<typename ST>
auto SO::TrimLeft(const ST& text_or_sv)
{
#ifdef USING_CSTRING
    if constexpr(std::is_same_v<ST, CString>)
    {
        return TrimLeft<wstring_view>(text_or_sv);
    }

    else
#endif
    {
        return SO::TrimLeftWorker(text_or_sv.data(), text_or_sv.length(), SO::IsWhitespaceChar<typename ST::value_type>);
    }
}


template<typename ST>
auto SO::TrimRight(const ST& text_or_sv)
{
#ifdef USING_CSTRING
    if constexpr(std::is_same_v<ST, CString>)
    {
        return TrimRight<wstring_view>(text_or_sv);
    }

    else
#endif
    {
        return SO::TrimRightWorker(text_or_sv.data(), text_or_sv.length(), SO::IsWhitespaceChar<typename ST::value_type>);
    }
}


template<bool trim_left, typename ST, typename CT>
auto SO::TrimLeftRightCharWorker(const ST& text_or_sv, const CT trim_char)
{
#ifdef USING_CSTRING
    if constexpr(std::is_same_v<ST, CString>)
    {
        return TrimLeftRightCharWorker<trim_left, wstring_view, CT>(text_or_sv, trim_char);
    }

    else
#endif
    {
        // if this asserts, we need to add a version of trimming that properly handles trimming
        // wide characters in UTF-8 strings
        ASSERT(SO::CharacterIsSameInWideAndUtf8(trim_char));

        const auto predicate =
            [cast_trim_char = static_cast<typename ST::value_type>(trim_char)](const typename ST::value_type ch)
            {
                return ( ch == cast_trim_char);
            };

        if constexpr(trim_left)
        {
            return SO::TrimLeftWorker(text_or_sv.data(), text_or_sv.length(), predicate);
        }

        else
        {
            return SO::TrimRightWorker(text_or_sv.data(), text_or_sv.length(), predicate);
        }
    }
}


template<bool trim_left, typename ST, typename SVT>
auto SO::TrimLeftRightStringViewWorker(const ST& text_or_sv, const SVT& trim_chars_sv)
{
    if constexpr(std::is_same_v<ST, CString>)
    {
        return TrimLeftRightStringViewWorker<trim_left, wstring_view, SVT>(text_or_sv, trim_chars_sv);
    }

    else
    {
        // if this asserts, we need to add a version of trimming that properly handles trimming
        // wide characters in UTF-8 strings
        ASSERT(std::find_if_not(trim_chars_sv.cbegin(), trim_chars_sv.cend(),
                                SO::CharacterIsSameInWideAndUtf8<typename ST::value_type>) == trim_chars_sv.cend());

        const auto predicate =
            [&trim_chars_sv](const typename ST::value_type ch)
            {
                return ( trim_chars_sv.find(ch) != SVT::npos );
            };

        if constexpr(trim_left)
        {
            return SO::TrimLeftWorker(text_or_sv.data(), text_or_sv.length(), predicate);
        }

        else
        {
            return SO::TrimRightWorker(text_or_sv.data(), text_or_sv.length(), predicate);
        }
    }
}


template<typename ST, typename... Args>
auto SO::Trim(const ST& text_or_sv, Args const&... args)
{
#ifdef USING_CSTRING
    if constexpr(std::is_same_v<ST, CString>)
    {
        return Trim<wstring_view>(text_or_sv, args...);
    }

    else
#endif
    {
        using string_view_type = typename std::conditional<StringIsWide<ST>(), wstring_view, std::string_view>::type;

        if( text_or_sv.empty() )
            return string_view_type();

        const string_view_type left_trimmed_sv = SO::TrimLeft(text_or_sv, args...);

        return left_trimmed_sv.empty() ? string_view_type() :
                                         SO::TrimRight(left_trimmed_sv, args...);
    }
}


template<bool is_from_trim_right, typename ST, typename SVT>
ST& SO::MakeTrimWorker(ST& text, const SVT trimmed_text_sv)
{
#ifdef USING_CSTRING
    if constexpr(std::is_same_v<ST, CString>)
    {
        if( static_cast<size_t>(text.GetLength()) != trimmed_text_sv.length() )
            text = CString(trimmed_text_sv);
    }

    else
#endif
    {
        if( text.length() != trimmed_text_sv.length() )
        {
            if constexpr(is_from_trim_right)
            {
                text.resize(trimmed_text_sv.length());
            }

            else
            {
                text = trimmed_text_sv;
            }
        }
    }

    return text;
}


template<bool insert_spaces_at_end/* = true*/, typename ST>
ST& SO::MakeExactLength(ST& text, const size_t length)
{
    if constexpr(std::is_same_v<ST, std::string> ||
                 std::is_same_v<ST, std::wstring>)
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
T& SO::Append(T& destination, const wstring_view text1_sv, Args... textN)
{
    SO::Append(destination, text1_sv);
    SO::Append(destination, textN...);
    return destination;
}


template<typename T>
T& SO::Append(T& destination, const wstring_view text_sv)
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
T& SO::AppendFormat(T& destination, const wchar_t* const formatter, Args const&... args)
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


template<typename AppendT, typename SeparatorT>
std::string& SO::AppendWithSeparator(std::string& destination, AppendT&& text_to_append, SeparatorT&& separator)
{
    if( !destination.empty() )
        destination += std::forward<SeparatorT>(separator);

    return destination.append(std::forward<AppendT>(text_to_append));
}


template<typename T>
T& SO::AppendWithSeparator(T& destination, const wstring_view text_sv, const wstring_view separator_sv)
{
    if( !destination.empty() )
        SO::Append(destination, separator_sv);

    return SO::Append(destination, text_sv);
}


template<typename T, typename... Args>
std::string SO::Concatenate(T&& arg1, Args const&... args)
{
    std::string text = std::string(std::forward<T>(arg1));

    if constexpr(sizeof...(Args) != 0)
    {
        // reserve the entire length needed
        text.reserve(text.length() + GetLength(args...));

        (
            [&]
            {
                text.append(args);
            }
        (), ...);
    }

    return text;
}


template<typename... Args>
std::wstring SO::ConcatenateWS(std::wstring text1, Args... textN)
{
    SO::Append(text1, textN...);
    return text1;
}



// --------------------------------------------------------------------------
// Copying functions
// --------------------------------------------------------------------------

inline void SO::CopyToFixedBuffer(char* const destination, const std::string_view source_sv, const size_t destination_size)
{
    CopyToFixedBufferWorker(destination, destination_size, source_sv.data(), source_sv.length());
}


inline void SO::CopyToFixedBuffer(wchar_t* const destination, const std::wstring_view source_sv, const size_t destination_size)
{
    CopyToFixedBufferWorker(destination, destination_size, source_sv.data(), source_sv.length());
}


template<typename CT, size_t destination_size, typename ST>
void SO::CopyToFixedBuffer(CT (&destination)[destination_size], ST&& source)
{
    CopyToFixedBuffer(destination, std::forward<ST>(source), destination_size);
}



// --------------------------------------------------------------------------
// Parsing functions
// --------------------------------------------------------------------------

template<typename ST, typename SeparatorT, typename CF>
void SO::ForeachSection(const ST& text_or_sv, const SeparatorT& separators, const CF& callback_function)
{
    // UTF8_TODO when the wide version of this is removed, the first argument can just be: std::string_view text_sv
    using string_view_type = std::basic_string_view<typename ST::value_type>;
    string_view_type text_sv = text_or_sv;

    while( !text_sv.empty() )
    {
        const typename string_view_type::size_type separator_pos = text_sv.find_first_of(separators);

        if( !CallbackFunctionProcessor::KeepProcessing(callback_function, ST(text_sv.substr(0, separator_pos))) )
            return;

        if( separator_pos == string_view_type::npos )
            return;

        text_sv.remove_prefix(separator_pos + 1);
    }
}


template<typename CFPT/* = std::string_view*/, typename CF>
void SO::ForeachLine(std::string_view text_sv, const bool process_whitespace_lines, const CF& callback_function)
{
    if( text_sv.empty() )
        return;

    while( true )
    {
        const std::string_view::size_type newline_pos = text_sv.find_first_of(Newline_crlf_sv);
        const std::string_view this_line_sv = text_sv.substr(0, newline_pos);

        if( process_whitespace_lines || !SO::IsWhitespace(this_line_sv) )
        {
            if( !CallbackFunctionProcessor::KeepProcessing(callback_function, CFPT(this_line_sv)) )
                return;
        }

        if( newline_pos == std::string_view::npos )
            return;

        std::string_view::size_type after_newline_pos = newline_pos + 1;

        // skip past the \n following a \r
        if( text_sv[newline_pos] == '\r' && after_newline_pos < text_sv.length() && text_sv[after_newline_pos] == '\n' )
            ++after_newline_pos;

        if( after_newline_pos == text_sv.length() )
        {
            // if processing whitespace, handle the blank line that occurs when the newline is at the end of the string; e.g.,: Line 1\nLine 2\n
            if( process_whitespace_lines )
                callback_function(CFPT());

            return;
        }

        text_sv = text_sv.substr(after_newline_pos);
    }
}


template<typename CF>
void SO::ForeachLine(wstring_view text_sv, const bool process_whitespace_lines, const CF& callback_function)
{
    if( text_sv.empty() )
        return;

    while( true )
    {
        const std::wstring_view::size_type newline_pos = text_sv.find_first_of(L"\r\n");
        const wstring_view this_line_sv = text_sv.substr(0, newline_pos);

        if( process_whitespace_lines || !SO::IsWhitespace(this_line_sv) )
        {
            if( !CallbackFunctionProcessor::KeepProcessing(callback_function, this_line_sv) )
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

template<typename RT, typename SeparatorT, typename SVT>
std::vector<RT> SO::SplitStringWorker(SVT text_sv, const SeparatorT& separators, bool trim_all, bool include_empty_entities)
{
    std::vector<RT> entities;

    SO::ForeachSection(text_sv, separators,
        [&](const SVT entity_sv)
        {
            if( include_empty_entities || !SO::IsWhitespace(entity_sv) )
            {
                entities.emplace_back(trim_all ? SO::Trim(entity_sv) :
                                                 entity_sv);
            }
        });

    return entities;
}


template<bool right_trim_each_string/* = true*/, typename T>
std::string SO::CreateSingleString(const T& strings, const std::string_view separator_sv/* = ", "*/)
{
    auto strings_itr = strings.begin();
    const auto& strings_end = strings.end();

    if( strings_itr == strings_end )
        return std::string();

    auto get_string = [](const std::string_view text_sv)
    {
        if constexpr(right_trim_each_string)
        {
            return std::string(SO::TrimRight(text_sv));
        }

        else
        {
            return std::string(text_sv);
        }
    };

    std::string result = get_string(*strings_itr);

    while( ++strings_itr != strings_end )
        SO::AppendWithSeparator(result, get_string(*strings_itr), separator_sv);

    return result;
}


template<typename T, typename CF>
std::string SO::CreateSingleStringUsingCallback(const T& objects, const CF& callback_function, const std::string_view separator_sv/* = ", "*/)
{
    if( objects.empty() )
        return std::string();

    auto objects_itr = objects.cbegin();
    const auto& objects_end = objects.cend();

    std::string result = callback_function(*objects_itr);

    while( ++objects_itr != objects_end )
        SO::AppendWithSeparator(result, callback_function(*objects_itr), separator_sv);

    return result;
}
