#include "StdAfx.h"
#include "StringOperations.h"
#include "Utf8Convert.h"
#include <locale>
#include <mutex>


const std::locale SO::m_localeForCaseConversions("");


// --------------------------------------------------------------------------
// std::wstring <-> CString temporary operations
// --------------------------------------------------------------------------

const CString& WS2CS_Reference(const std::wstring& text)
{
    static std::map<std::wstring, CString> cstring_values;
    const auto& lookup = cstring_values.find(text);
    return ( lookup != cstring_values.cend() ) ? lookup->second :
                                                 cstring_values.try_emplace(text, WS2CS(text)).first->second;
}


const std::wstring& CS2WS_Reference(const CString& text)
{
    static std::map<CString, std::wstring> string_values;
    const auto& lookup = string_values.find(text);
    return ( lookup != string_values.cend() ) ? lookup->second :
                                                string_values.try_emplace(text, CS2WS(text)).first->second;
}



// --------------------------------------------------------------------------
// UTF8_TODO
// --------------------------------------------------------------------------

template<typename T>
const T& UTF8_TODO::Create_Reference(const std::string_view text_sv)
{
    static std::map<std::string, std::unique_ptr<T>> converted_values;

    std::string text = std::string(text_sv);
    auto lookup = converted_values.find(text);

    if( lookup == converted_values.cend() )
        lookup = converted_values.try_emplace(std::move(text), std::make_unique<T>(UTF8Convert::UTF8ToWide<T>(text_sv))).first;

    return *lookup->second;
}

template CLASS_DECL_ZTOOLSO const std::wstring& UTF8_TODO::Create_Reference(std::string_view text_sv);
template CLASS_DECL_ZTOOLSO const CString& UTF8_TODO::Create_Reference(std::string_view text_sv);


const std::string& UTF8_TODO::Create_Reference(const wstring_view text_sv)
{
    return Create_SharableStringReference(text_sv).GetString();
}


const SharableString& UTF8_TODO::Create_SharableStringReference(const wstring_view text_sv)
{
    static std::map<size_t, std::unique_ptr<SharableString>> converted_values;

    const size_t text_hash = text_sv.hash_code();
    auto lookup = converted_values.find(text_hash);

    if( lookup == converted_values.cend() )
        lookup = converted_values.try_emplace(text_hash, std::make_unique<SharableString>(UTF8Convert::WideToUTF8(text_sv))).first;

    return *lookup->second;
}


std::wstring UTF8_TODO::GetWide(const std::string& text)          { return UTF8Convert::UTF8ToWide(text); }
std::wstring UTF8_TODO::GetWide(const std::string_view text_sv)   { return UTF8Convert::UTF8ToWide(text_sv); }
std::wstring UTF8_TODO::GetWide(const cs::string_view_sz text_sv) { return UTF8Convert::UTF8ToWide(text_sv); }
std::wstring UTF8_TODO::GetWide(const char* const text)           { return UTF8Convert::UTF8ToWide(text); }

std::wstring UTF8_TODO::EnsureWide(InterfaceString text)
{
#ifdef WIN32
    return text.Release();
#else
    return UTF8_TODO::GetWide(text.GetStringView());
#endif
}


template<typename T>
CString UTF8_TODO::GetCString(const T& text_or_sv)
{
    return UTF8Convert::UTF8ToWide<CString>(text_or_sv);
}

template CLASS_DECL_ZTOOLSO CString UTF8_TODO::GetCString(const char* const& text_or_sv);
template CLASS_DECL_ZTOOLSO CString UTF8_TODO::GetCString(const std::string& text_or_sv);
template CLASS_DECL_ZTOOLSO CString UTF8_TODO::GetCString(const std::string_view& text_or_sv);
template CLASS_DECL_ZTOOLSO CString UTF8_TODO::GetCString(const cs::string_sz& text_or_sv);
template CLASS_DECL_ZTOOLSO CString UTF8_TODO::GetCString(const cs::string_view_sz& text_or_sv);


std::string UTF8_TODO::GetUtf8(const wstring_view text_sv)
{
    return UTF8Convert::WideToUTF8(text_sv);
}


std::optional<std::wstring> UTF8_TODO::GetOptionalWide(const std::optional<std::string>& optional_text)
{
    if( optional_text.has_value() )
        return GetWide(*optional_text);

    return std::nullopt;
}


std::optional<std::string> UTF8_TODO::GetOptionalUtf8(const std::optional<std::wstring>& optional_text)
{
    if( optional_text.has_value() )
        return GetUtf8(*optional_text);

    return std::nullopt;
}


std::vector<std::string> UTF8_TODO::GetUtf8(const std::vector<std::wstring>& texts)
{
    std::vector<std::string> utf8_texts;
    utf8_texts.reserve(texts.size());

    for( const std::wstring& text : texts )
        utf8_texts.emplace_back(GetUtf8(text));

    return utf8_texts;
}


std::vector<std::wstring> UTF8_TODO::GetWide(const std::vector<std::string>& texts)
{
    std::vector<std::wstring> wide_texts;
    wide_texts.reserve(texts.size());

    for( const std::string& text : texts )
        wide_texts.emplace_back(GetWide(text));

    return wide_texts;
}


std::vector<std::wstring> UTF8_TODO::GetWide(const std::vector<SharableString>& texts)
{
    std::vector<std::wstring> wide_texts;
    wide_texts.reserve(texts.size());

    for( const SharableString& text : texts )
        wide_texts.emplace_back(GetWide(*text));

    return wide_texts;
}


std::vector<CString> UTF8_TODO::GetCString(const std::vector<std::string>& texts)
{
    std::vector<CString> wide_texts;
    wide_texts.reserve(texts.size());

    for( const std::string& text : texts )
        wide_texts.emplace_back(GetCString(text));

    return wide_texts;
}



// --------------------------------------------------------------------------
// Empty string objects
// --------------------------------------------------------------------------

const std::string SO::Empty_string;
const std::shared_ptr<const std::string> SO::Empty_shared_string(std::make_shared<std::string>());
const std::wstring SO::Empty_wstring;
const CString SO::Empty_CString;



// --------------------------------------------------------------------------
// Wide character routines that work on UTF-8 text
// --------------------------------------------------------------------------

size_t SO::WideLengthWorker(const char* text_itr, const char* const text_end)
{
#ifdef _DEBUG
    const std::wstring wide_text_check = TC::ToWide(std::string_view(text_itr, text_end - text_itr));
#endif

    size_t wide_length = 0;

    while( text_itr < text_end )
    {
        ++wide_length;
        text_itr += TC::Utf8BytesFromFirstByte(*text_itr);
    }

    ASSERT(text_itr == text_end);

    // this assert can fail when using characters like 🐼 because, at least on Windows, std::wstring will report
    // that single character as length 2 (a UTF-16 surrogate pair), whereas this routine counts it as length 1;
    // even other programs reports these differently: Notepad++ and Windows count it as 1, but Notepad counts it as 2
    ASSERT81(wide_length == wide_text_check.length());

    return wide_length;
}


size_t SO::WideLengthWorker(const char* text_itr)
{
#ifdef _DEBUG
    const std::wstring wide_text_check = TC::ToWide(text_itr);
#endif

    size_t wide_length = 0;
    char ch;

    while( ( ch = *text_itr ) != '\0' )
    {
        ++wide_length;
        text_itr += TC::Utf8BytesFromFirstByte(ch);
    }

    ASSERT81(wide_length == wide_text_check.length());

    return wide_length;
}


size_t SO::WideGetOffset(const std::string_view text_sv, size_t wide_offset)
{
    if( wide_offset == 0 )
        return 0;

    const char* text_itr = text_sv.data();
    const char* const text_end = text_itr + text_sv.length();

    while( text_itr < text_end )
    {
        ASSERT(wide_offset > 0);

        text_itr += TC::Utf8BytesFromFirstByte(*text_itr);

        if( --wide_offset == 0 )
            return ( text_itr - text_sv.data() );
    }

    ASSERT(text_itr == text_end);

    return std::string_view::npos;
}


template<typename RT/* = size_t*/, typename CT>
RT SO::WideGetOffset(CT* const text, size_t wide_offset)
{
    if( wide_offset == 0 )
    {
        if constexpr(std::is_same_v<RT, size_t>)
        {
            return 0;
        }

        else
        {
            return text;
        }
    }

    CT* text_itr = text;

    do
    {
        text_itr += TC::Utf8BytesFromFirstByte(*text_itr);

    } while( --wide_offset != 0 );

    if constexpr(std::is_same_v<RT, size_t>)
    {
        return ( text_itr - text );
    }

    else
    {
        return text_itr;
    }
}

template CLASS_DECL_ZTOOLSO size_t SO::WideGetOffset(const char* text, size_t wide_offset);
template CLASS_DECL_ZTOOLSO size_t SO::WideGetOffset(char* text, size_t wide_offset);
template CLASS_DECL_ZTOOLSO const char* SO::WideGetOffset(const char* text, size_t wide_offset);
template CLASS_DECL_ZTOOLSO char* SO::WideGetOffset(char* text, size_t wide_offset);


size_t SO::WideGetOffsetFromEnd(const std::string_view text_sv, size_t wide_offset)
{
    ASSERT(wide_offset <= text_sv.length());

    if( wide_offset == 0 )
        return text_sv.length();

    const char* const text_sv_front = &text_sv.front();

    for( const char* text_sv_itr = &text_sv.back(); text_sv_itr > text_sv_front; --text_sv_itr )
    {
        if( TC::InMiddleOfUtf8Sequence(*text_sv_itr) )
            continue;

        if( --wide_offset == 0 )
            return text_sv_itr - text_sv_front;
    }

    return 0;
}


std::string_view SO::WideSubstring(const std::string_view text_sv, const size_t wide_offset, size_t wide_count/* = std::string_view::npos*/)
{
#ifdef _DEBUG
    const std::wstring wide_text_check = TC::ToWide(text_sv);
    ASSERT81(wide_offset <= wide_text_check.length());
    const std::wstring_view wide_text_substr_sv = std::wstring_view(wide_text_check).substr(wide_offset, wide_count);
#endif

    ASSERT(wide_offset <= SO::WideLength(text_sv));

    const size_t start_offset = WideGetOffset(text_sv.data(), wide_offset);

    if( wide_count == std::string_view::npos )
    {
        ASSERT81(text_sv.substr(start_offset) == TC::ToUtf8(wide_text_substr_sv));
        return text_sv.substr(start_offset);
    }

    // when specifying a count, calculate the end offset
    else
    {
        size_t end_offset = start_offset;

        for( ; wide_count > 0 && end_offset < text_sv.length(); --wide_count )
            end_offset += TC::Utf8BytesFromFirstByte(text_sv[end_offset]);

        ASSERT81(text_sv.substr(start_offset, end_offset - start_offset) == TC::ToUtf8(wide_text_substr_sv));
        return text_sv.substr(start_offset, end_offset - start_offset);
    }
}


void SO::WideSetChar(std::string& text, const size_t wide_offset, const wchar_t ch)
{
#ifdef _DEBUG
    std::wstring wide_text_check = TC::ToWide(text);
    ASSERT81(wide_offset < wide_text_check.length());
    wide_text_check[wide_offset] = ch;
#endif

    ASSERT(wide_offset < SO::WideLength(text));

    char* destination = WideGetOffset<char*>(text.data(), wide_offset);
    ASSERT(destination <= &text.back());

    const size_t current_char_length = TC::Utf8BytesFromFirstByte(*destination);

    // the most frequent case is modifying characters that are represented as a single byte
    if( current_char_length == 1 && TC::IsUtf8SingleByte(ch) )
    {
        *destination = static_cast<char>(ch);
    }

    // otherwise get the multibyte UTF-8 representation
    else
    {
        const std::string& utf8_representation = TC::GetUtf8ForWideChar(ch);

        if( current_char_length == utf8_representation.length() )
        {
            memcpy(destination, utf8_representation.data(), current_char_length);
        }

        else
        {
            text.replace(destination - text.data(), current_char_length, utf8_representation);
        }
    }

    ASSERT81(text == TC::ToUtf8(wide_text_check));
}


template<bool insert_spaces_at_end/* = true*/>
void SO::WideMakeExactLengthAdjuster(std::string& text, const ptrdiff_t non_zero_length_difference)
{
    ASSERT(non_zero_length_difference != 0);

    // or add spaces
    if( non_zero_length_difference > 0 )
    {
        if constexpr(insert_spaces_at_end)
        {
            text.resize(text.length() + non_zero_length_difference, ' ');
        }

        else
        {
            text.insert(text.begin(), non_zero_length_difference, ' ');
        }
    }

    // ... or truncate the string
    else
    {
        text.resize(SO::WideGetOffsetFromEnd(text, -1 * non_zero_length_difference));
    }
}

template CLASS_DECL_ZTOOLSO void SO::WideMakeExactLengthAdjuster<true>(std::string& text, ptrdiff_t non_zero_length_difference);
template CLASS_DECL_ZTOOLSO void SO::WideMakeExactLengthAdjuster<false>(std::string& text, ptrdiff_t non_zero_length_difference);


std::string& SO::WideCenterExactLength(std::string& text, const size_t wide_length)
{
    ASSERT(!SO::ContainsNewlineCharacter(text));

    const size_t current_wide_length = SO:: WideLength(text);

    if( current_wide_length < wide_length )
    {
        // insert left spaces; the right spaces will be added by MakeExactLength
        const size_t left_spaces_needed = ( wide_length - current_wide_length ) / 2;
        text.insert(0, left_spaces_needed, ' ');
    }

    return SO::MakeExactLength(text, wide_length);
}


void SO::ForeachWideChar(const std::string_view text_sv, const std::function<void(wchar_t)>& callback_function)
{
    const char* text_itr = text_sv.data();
    const char* const text_end = text_itr + text_sv.length();

    while( text_itr < text_end )
    {
        const size_t ch_utf8_length = TC::Utf8BytesFromFirstByte(*text_itr);

        callback_function(TC::GetWideCharFromUtf8Sequence(text_itr, ch_utf8_length));

        text_itr += ch_utf8_length;
    }

    ASSERT(text_itr == text_end);

}



// --------------------------------------------------------------------------
// Case functions
// --------------------------------------------------------------------------

template<bool ToUpper>
wchar_t SO::WideCharToCaseWithLocale(const wchar_t ch)
{
    if constexpr(ToUpper)
    {
        return std::toupper(ch, m_localeForCaseConversions);
    }

    else
    {
        return std::tolower(ch, m_localeForCaseConversions);
    }
}


template<bool ToUpper>
size_t SO::FindFirstNonMatchingWideCase(const std::string_view text_sv)
{
    // find the first character that does not match the specified case
    const char* text_itr = text_sv.data();
    const char* const text_end = text_itr + text_sv.length();

    while( text_itr < text_end )
    {
        const size_t ch_utf8_length = TC::Utf8BytesFromFirstByte(*text_itr);
        const wchar_t wide_ch = TC::GetWideCharFromUtf8Sequence(text_itr, ch_utf8_length);
        const wchar_t adjusted_case_ch = SO::WideCharToCase<ToUpper>(wide_ch);

        if( wide_ch != adjusted_case_ch )
            break;

        text_itr += ch_utf8_length;
    }

    return ( text_itr - text_sv.data() );
}

template CLASS_DECL_ZTOOLSO size_t SO::FindFirstNonMatchingWideCase<true>(std::string_view text_sv);
template CLASS_DECL_ZTOOLSO size_t SO::FindFirstNonMatchingWideCase<false>(std::string_view text_sv);


template<bool ToUpper>
void SO::MakeWideCaseWorker(const std::string& in_text, std::string*& out_text)
{
#ifdef _DEBUG
    // this code is similar to the pre-8.0 toupper/tolower logic functions,
    // used now to check that the results are the same
    CString cstring_check = UTF8_TODO::GetCString(in_text);

    {
        const std::locale old_locale = std::locale::global(std::locale(""));

        if constexpr(ToUpper)
        {
            cstring_check.MakeUpper();
        }

        else
        {
            cstring_check.MakeLower();
        }

        std::locale::global(old_locale);
    }
#endif

    ASSERT(out_text == nullptr || out_text == &in_text);

    const size_t non_matching_wide_case_pos = SO::FindFirstNonMatchingWideCase<ToUpper>(in_text);

    // quit if the text is already in the correct case
    if( non_matching_wide_case_pos == in_text.length() )
    {
        ASSERT81(in_text == TC::ToUtf8(cstring_check));
        return;
    }

    // allocate space for the text, if necessary
    if( out_text == nullptr )
        out_text = new std::string(in_text);

    // convert all characters from the initial one that did not match
    ptrdiff_t remaining_characters = out_text->length() - non_matching_wide_case_pos;
    char* out_text_itr = out_text->data() + non_matching_wide_case_pos;

    while( remaining_characters > 0 )
    {
        const size_t current_char_length = TC::Utf8BytesFromFirstByte(*out_text_itr);
        const wchar_t wide_ch = TC::GetWideCharFromUtf8Sequence(out_text_itr, current_char_length);
        const wchar_t adjusted_case_ch = SO::WideCharToCase<ToUpper>(wide_ch);

        if( wide_ch == adjusted_case_ch )
        {
            out_text_itr += current_char_length;
        }

        // update the character as necessary
        else
        {
            // update the character(s) directly when the UTF-8 length is the same
            if( current_char_length == TC::Utf8BytesFromFirstByte(adjusted_case_ch) )
            {
                if( current_char_length == 1 )
                {
                    *out_text_itr = static_cast<char>(adjusted_case_ch);
                }

                else
                {
                    const std::string& utf8_representation = TC::GetUtf8ForWideChar(adjusted_case_ch);
                    memcpy(out_text_itr, utf8_representation.data(), current_char_length);
                }

                out_text_itr += current_char_length;
            }

            // or replace the character with a different number of characters
            else
            {
                const std::string& utf8_representation = TC::GetUtf8ForWideChar(adjusted_case_ch);

                const size_t current_pos = out_text_itr - out_text->data();
                out_text->replace(current_pos, current_char_length, utf8_representation);

                out_text_itr = out_text->data() + current_pos + utf8_representation.length();
            }
        }

        remaining_characters -= current_char_length;
    }

    ASSERT(remaining_characters == 0);
    ASSERT81(*out_text == TC::ToUtf8(cstring_check));
}

template CLASS_DECL_ZTOOLSO void SO::MakeWideCaseWorker<true>(const std::string& in_text, std::string*& out_text);
template CLASS_DECL_ZTOOLSO void SO::MakeWideCaseWorker<false>(const std::string& in_text, std::string*& out_text);


std::string SO::ToProperCase(std::string text)
{
    const char* text_itr = text.data();
    const char* text_end = text_itr + text.length();
    bool next_char_should_be_upper_case = true;

    while( text_itr != text_end )
    {
        if( *text_itr == ' ' )
        {
            next_char_should_be_upper_case = true;
            ++text_itr;
        }

        else
        {
            const size_t ch_utf8_length = TC::Utf8BytesFromFirstByte(*text_itr);
            const wchar_t wide_ch = TC::GetWideCharFromUtf8Sequence(text_itr, ch_utf8_length);
            wchar_t converted_wide_ch;

            if( next_char_should_be_upper_case )
            {
                converted_wide_ch = WideCharToUpper(wide_ch);
                next_char_should_be_upper_case = false;
            }

            else
            {
                // if this function is ever used with characters like (), figure out what characters
                // should result in next_char_should_be_upper_case being set to true
                ASSERT(is_alpha(wide_ch));

                converted_wide_ch = WideCharToLower(wide_ch);
            }

            if( wide_ch == converted_wide_ch )
            {
                text_itr += ch_utf8_length;
            }

            else
            {
                const std::string& utf8_representation = TC::GetUtf8ForWideChar(converted_wide_ch);

                const size_t current_pos = text_itr - text.data();
                text.replace(current_pos, ch_utf8_length, utf8_representation);

                text_itr = text.data() + current_pos + utf8_representation.length();
                text_end = text.data() + text.length();
            }
        }
    }

    return text;
}


std::string SO::TitleToCamelCase(std::string text)
{
    if( !text.empty() )
    {
        char& first_ch = text.front();
        const size_t first_ch_utf8_length = TC::Utf8BytesFromFirstByte(first_ch);

        if( first_ch_utf8_length == 1 )
        {
            first_ch = static_cast<char>(std::tolower(first_ch));
        }

        else
        {
            const wchar_t first_wide_ch = TC::GetWideCharFromUtf8Sequence(text.c_str());
            const wchar_t lowercase_first_wide_ch = WideCharToLower(first_wide_ch);

            if( first_wide_ch != lowercase_first_wide_ch )
                SO::WideSetChar(text, 0, lowercase_first_wide_ch);
        }
    }

    return text;
}



// --------------------------------------------------------------------------
// Comparison functions
// --------------------------------------------------------------------------

bool SO::Equals(const std::string_view sv1, const wstring_view sv2)
{
    // UTF8_TODO can improve SO::Equals to go character by character in the wide and non-wide strings and compare accordingly ... though will have to think about how to handle surrogate pairs
    return ( sv1 == UTF8Convert::WideToUTF8(sv2) );
}


int SO::CompareNoCase(const std::string_view sv1, const std::string_view sv2)
{
    if( sv1.empty() )
        return sv2.empty() ? 0 : -1;

    if( sv2.empty() )
        return 1;

    const char* sv1_itr = sv1.data();
    const char* sv2_itr = sv2.data();
    const bool sv1_is_longer = ( sv1.length() > sv2.length() );
    const char* const sv1_end = sv1.data() + ( sv1_is_longer ? sv2.length() : sv1.length() );

    while( sv1_itr < sv1_end )
    {
        const size_t ch1_utf8_length = TC::Utf8BytesFromFirstByte(*sv1_itr);
        const wchar_t ch1 = TC::GetWideCharFromUtf8Sequence(sv1_itr, ch1_utf8_length);

        const size_t ch2_utf8_length = TC::Utf8BytesFromFirstByte(*sv2_itr);
        const wchar_t ch2 = TC::GetWideCharFromUtf8Sequence(sv2_itr, ch2_utf8_length);

        auto ch_diff = SO::WideCharToUpper(ch1) - SO::WideCharToUpper(ch2);

        if( ch_diff != 0 )
            return ch_diff;

        sv1_itr += ch1_utf8_length;
        sv2_itr += ch2_utf8_length;
    }

    return sv1_is_longer                    ?  1 :
           ( sv1.length() == sv2.length() ) ?  0 :
                                              -1;
}


int SO::CompareNoCase(const wstring_view sv1, const wstring_view sv2)
{
    if( sv1.empty() )
        return sv2.empty() ? 0 : -1;

    if( sv2.empty() )
        return 1;

    const wchar_t* sv1_itr = sv1.data();
    const wchar_t* sv2_itr = sv2.data();
    const bool sv1_is_longer = ( sv1.length() > sv2.length() );
    const wchar_t* const sv1_end = sv1.data() + ( sv1_is_longer ? sv2.length() : sv1.length() );

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



// --------------------------------------------------------------------------
// Search functions
// --------------------------------------------------------------------------

size_t SO::FindNoCase(const std::string_view source_sv, const std::string_view substring_sv, const size_t offset/* = 0*/)
{
    if( offset >= source_sv.length() )
        return std::string_view::npos;

    const size_t max_start_pos = source_sv.size() - substring_sv.length();

    if( max_start_pos >= source_sv.size() || max_start_pos < offset )
        return std::string_view::npos;

    ASSERT(!source_sv.empty() && !substring_sv.empty());

    const int substring_first_ch_lower = std::tolower(substring_sv.front());
    const std::string_view substring_second_ch_on_sv = substring_sv.substr(1);

    const char* source_sv_itr = source_sv.data() + offset;
    const char* const source_sv_last_pos_to_check = source_sv.data() + max_start_pos;

    for( ; source_sv_itr <= source_sv_last_pos_to_check; ++source_sv_itr )
    {
        if( std::tolower(*source_sv_itr) == substring_first_ch_lower )
        {
            const size_t pos = source_sv_itr - source_sv.data();

            if( SO::StartsWithNoCase(source_sv.substr(pos + 1), substring_second_ch_on_sv) )
                return pos;
        }
    }

    return std::string_view::npos;
}


size_t SO::FindFirstWhitespace(const std::string_view source_sv, const size_t offset/* = 0*/)
{
    ASSERT(offset <= source_sv.length());
    const auto& lookup = std::find_if(source_sv.cbegin() + offset, source_sv.cend(),
                                      [](const char ch) { return SO::IsWhitespaceChar(ch); });

    return ( lookup != source_sv.cend() ) ? std::distance(source_sv.cbegin(), lookup) :
                                            std::string_view::npos;
}


std::string_view SO::GetTextBetweenCharacters(const std::string_view text_sv, const char ch1, const char ch2)
{
    const auto [ch1_pos, ch2_pos] = SO::FindCharacters(text_sv, ch1, ch2);
    return ( ch2_pos != wstring_view::npos ) ? text_sv.substr(ch1_pos + 1, ch2_pos - ch1_pos - 1) :
                                               std::string_view();
}


std::tuple<std::string_view, std::string_view> SO::GetTextOnEitherSideOfCharacter(const std::string_view text_sv, const char ch)
{
    const size_t ch_pos = text_sv.find(ch);
    return ( ch_pos != std::string_view::npos ) ? std::make_tuple(text_sv.substr(0, ch_pos), text_sv.substr(ch_pos + 1) ) :
                                                  std::make_tuple(text_sv, std::string_view());
}



// --------------------------------------------------------------------------
// Newline functions
// --------------------------------------------------------------------------

void SO::MakeSplitVectorStringsOnNewlines(std::vector<std::string>& strings)
{
    for( auto strings_itr = strings.begin(); strings_itr != strings.cend(); )
    {
        if( !SO::ContainsNewlineCharacter(*strings_itr) )
        {
            ++strings_itr;
            continue;
        }

        const std::string this_string = *strings_itr;
        bool first_string = true;

        SO::ForeachLine<std::string>(this_string, true,
            [&](std::string text)
            {
                if( first_string )
                {
                    *strings_itr = std::move(text);
                    first_string = false;
                }

                else
                {
                    strings_itr = strings.insert(strings_itr, std::move(text));
                }

                ++strings_itr;
            });

        ASSERT(!first_string);
    }
}



// --------------------------------------------------------------------------
// Manipulation functions
// --------------------------------------------------------------------------

std::string& SO::Replace(std::string& text, const char old_char, const char new_char, std::string::size_type ch_pos/* = 0*/)
{
    while( ( ch_pos = text.find(old_char, ch_pos) ) != std::string::npos )
    {
        text.replace(ch_pos, 1, 1, new_char);
        ++ch_pos;
    }

    return text;
}


std::wstring& SO::Replace(std::wstring& text, const wchar_t old_char, const wchar_t new_char, std::wstring::size_type ch_pos/* = 0*/)
{
    while( ( ch_pos = text.find(old_char, ch_pos) ) != std::wstring::npos )
    {
        text.replace(ch_pos, 1, 1, new_char);
        ++ch_pos;
    }

    return text;
}


std::string& SO::Replace(std::string& text, const std::string_view old_text_sv, const std::string_view new_text_sv, std::string::size_type ch_pos/* = 0*/)
{
    while( ( ch_pos = text.find(old_text_sv, ch_pos) ) != std::string::npos )
    {
        text.replace(ch_pos, old_text_sv.length(), new_text_sv.data(), new_text_sv.length());
        ch_pos += new_text_sv.length();
    }

    return text;
}


std::wstring& SO::Replace(std::wstring& text, const wstring_view old_text_sv, const wstring_view new_text_sv, std::wstring::size_type ch_pos/* = 0*/)
{
    while( ( ch_pos = text.find(old_text_sv, ch_pos) ) != std::wstring::npos )
    {
        text.replace(ch_pos, old_text_sv.length(), new_text_sv.data(), new_text_sv.length());
        ch_pos += new_text_sv.length();
    }

    return text;
}


std::string& SO::ReplaceNoCase(std::string& text, const std::string_view old_text_sv, const std::string_view new_text_sv, std::string::size_type ch_pos/* = 0*/)
{
    while( ( ch_pos = FindNoCase(text, old_text_sv, ch_pos) ) != std::string_view::npos )
    {
        text.replace(ch_pos, old_text_sv.length(), new_text_sv.data(), new_text_sv.length());
        ch_pos += new_text_sv.length();
    }

    return text;
}


std::string& SO::RecursiveReplace(std::string& text, const std::string_view old_text_sv, const std::string_view new_text_sv)
{
    size_t initial_length;

    do
    {
        initial_length = text.length();
        SO::Replace(text, old_text_sv, new_text_sv);

    } while( initial_length != text.length() );

    return text;
}


std::string& SO::Remove(std::string& text, const char ch)
{
    std::string::size_type ch_pos = 0;

    while( ( ch_pos = text.find(ch, ch_pos) ) != std::string::npos )
        text.erase(ch_pos, 1);

    return text;
}


std::wstring& SO::Remove(std::wstring& text, const wchar_t ch)
{
    std::wstring::size_type ch_pos = 0;

    while( ( ch_pos = text.find(ch, ch_pos) ) != std::wstring::npos )
        text.erase(ch_pos, 1);

    return text;
}


std::string SO::RemoveWhitespace(const std::string_view text_sv)
{
    std::string result;
    result.reserve(text_sv.size());

    for( const char ch : text_sv )
    {
        if( !SO::IsWhitespaceChar(ch) )
            result.push_back(ch);
    }

    return result;
}


std::string_view SO::RemoveTextFollowingCharacter(const std::string_view text_sv, const char ch, const bool remove_ch)
{
    const size_t ch_pos = text_sv.find(ch);

    return ( ch_pos != std::string_view::npos ) ? text_sv.substr(0, ch_pos + ( remove_ch ? 0 : 1 )) :
                                                  text_sv;
}


template<bool trim_right_each_line>
int SO::ConvertTabsToSpacesWorker(std::string& text, int position_in_line, const int spaces_per_tab)
{
    ASSERT(spaces_per_tab >= 1);

    // when trim_right_each_line is true, position_in_line should not be considered accurate,
    // which is fine because SO::ConvertTabsToSpacesAndTrimRightEachLine does not return that value
    for( size_t i = 0; i < text.length(); )
    {
        const char ch = text[i];

        if( is_crlf(ch) )
        {
            position_in_line = 0;

            if constexpr(trim_right_each_line)
            {
                if( i > 0 )
                {
                    const std::string_view text_up_to_this_crlf_sv(text.data(), i);
                    const size_t spaces_at_end = text_up_to_this_crlf_sv.length() - SO::TrimRightSpace(text_up_to_this_crlf_sv).length();

                    if( spaces_at_end > 0 )
                    {
                        text.erase(i - spaces_at_end, spaces_at_end);
                        i -= spaces_at_end;
                    }
                }
            }
        }

        else
        {
            if( ch == '\t' )
            {
                // replace the tab
                text[i] = ' ';

                // insert new spaces
                const int spaces_to_insert = spaces_per_tab - ( position_in_line % spaces_per_tab ) - 1;

                if( spaces_to_insert != 0 )
                {
                    text.insert(i + 1, SO::GetRepeatingCharacterString(' ', spaces_to_insert));
                    position_in_line += spaces_to_insert;
                    i += spaces_to_insert;
                }

                ASSERT(( position_in_line + 1 ) % spaces_per_tab == 0);
            }

            ++position_in_line;
        }

        i += TC::Utf8BytesFromFirstByte(ch);
    }

    if constexpr(trim_right_each_line)
    {
        SO::MakeTrimRightSpace(text);
    }

    return position_in_line;
}


int SO::ConvertTabsToSpaces(std::string& text, const int position_in_line/* = 0*/, const int spaces_per_tab/* = DefaultSpacesPerTab*/)
{
    return ConvertTabsToSpacesWorker<false>(text, position_in_line, spaces_per_tab);
}


void SO::ConvertTabsToSpacesAndTrimRightEachLine(std::string& text)
{
    ConvertTabsToSpacesWorker<true>(text, 0, DefaultSpacesPerTab);
}



// --------------------------------------------------------------------------
// Copying functions
// --------------------------------------------------------------------------

template<typename CT>
void SO::CopyToFixedBufferWorker(CT* const destination, const size_t destination_size, const CT* const source, size_t source_length)
{
    ASSERT(destination_size != 0);

    if( source_length >= destination_size )
        source_length = destination_size - 1;

    memcpy(destination, source, source_length * sizeof(CT));
    destination[source_length] = 0;
}

template CLASS_DECL_ZTOOLSO void SO::CopyToFixedBufferWorker(char* destination, size_t destination_size, const char* source, size_t source_length);
template CLASS_DECL_ZTOOLSO void SO::CopyToFixedBufferWorker(wchar_t* destination, size_t destination_size, const wchar_t* source, size_t source_length);



// --------------------------------------------------------------------------
// Other functions
// --------------------------------------------------------------------------

std::vector<std::wstring> SO::WrapText(wstring_view text_sv, const size_t maximum_line_width)
{
    constexpr const wchar_t* SpaceCharacters = L" \t\r\n";
    constexpr wstring_view SpaceCharactersWithoutNewline_sv(SpaceCharacters, 3);
    constexpr wchar_t HyphenCharacter = '-';

    // short circuit cases where the entire text will fit on one line
    text_sv = SO::TrimRight(text_sv, SpaceCharactersWithoutNewline_sv);

    if( text_sv.length() <= maximum_line_width && !SO::ContainsNewlineCharacter(text_sv) )
        return { text_sv };

    // split the string into trimmed lines
    std::vector<std::wstring> lines;

    while( !text_sv.empty() )
    {
        ASSERT(text_sv == SO::TrimRight(text_sv, SpaceCharactersWithoutNewline_sv));

        const size_t newline_pos = text_sv.substr(0, maximum_line_width).find('\n');

        // if the entire line will fit and doesn't contain a newline, add this as the final line
        if( text_sv.length() <= maximum_line_width && newline_pos == wstring_view::npos )
        {
            lines.emplace_back(text_sv);
            break;
        }

        // otherwise see if the line needs to be hyphenated
        size_t last_space_char_in_block = text_sv.find_last_of(SpaceCharacters, maximum_line_width);

        // if there were no spaces in this block, then the line must be hyphenated
        if( last_space_char_in_block == wstring_view::npos )
        {
            lines.emplace_back(text_sv.substr(0, maximum_line_width - 1)).push_back(HyphenCharacter);
            text_sv = text_sv.substr(maximum_line_width - 1);
        }

        // otherwise, the line up to the space (or newline) can be added
        else
        {
            // if there is a newline prior to this space character, break the line at the newline
            const bool newline_preceeded_last_space = ( newline_pos < last_space_char_in_block );

            if( newline_preceeded_last_space )
                last_space_char_in_block = newline_pos;

            const std::wstring& trimmed_text = lines.emplace_back(SO::TrimRight(text_sv.substr(0, last_space_char_in_block)));

            // left trim all space characters other than newlines
            size_t trimmed_substr_pos = trimmed_text.length() + 1;

            for( ; trimmed_substr_pos <= last_space_char_in_block; ++trimmed_substr_pos )
            {
                ASSERT(text_sv.find_last_of(SpaceCharacters, trimmed_substr_pos) == trimmed_substr_pos);

                if( text_sv[trimmed_substr_pos] == '\n' )
                {
                    if( trimmed_substr_pos == last_space_char_in_block )
                        ++trimmed_substr_pos;

                    break;
                }
            }

            text_sv = text_sv.substr(trimmed_substr_pos);

            // if the last character in the entire text was a newline, add a blank line to account for it
            if( !newline_preceeded_last_space && text_sv.empty() )
            {
                lines.emplace_back();
                break;
            }
        }
    }

    ASSERT(!lines.empty());
    return lines;
}


std::string SO::CreateColonSeparatedString(std::string text1, const std::string_view text2_sv)
{
    if( text1.empty() )
        return std::string(text2_sv);

    if( !text2_sv.empty() )
    {
        text1.reserve(text2_sv.length() + 2);

        text1.append(": ")
             .append(text2_sv);
    }

    return text1;
}


std::string SO::CreateParentheticalExpression(std::string text1, const std::string_view text2_sv)
{
    if( text1.empty() )
        return std::string(text2_sv);

    if( !text2_sv.empty() )
    {
        text1.append(" (")
             .append(text2_sv)
             .append(")");
    }

    return text1;
}


const char* SO::GetRepeatingCharacterString(const wchar_t ch, const size_t length)
{
    // keep the strings on hand for repeated use
    static std::vector<std::tuple<wchar_t, std::unique_ptr<std::string>>> strings;
    static std::mutex mtx;

    const size_t ch_utf8_length = TC::Utf8BytesNeededForWideChar(ch);
    const size_t string_utf8_length = ch_utf8_length * length;

    std::lock_guard<std::mutex> lock(mtx);

    // use an existing line, potentially offsetting the index
    for( const auto& [wide_ch, utf8_string] : strings )
    {
        if( ch == wide_ch && utf8_string->length() >= string_utf8_length )
        {
            const size_t utf8_length_difference = utf8_string->length() - string_utf8_length;
            return utf8_string->c_str() + utf8_length_difference;
        }
    }

    // the created string will always include one entry more than necessary so that a string is created even if length is 0
    std::unique_ptr<std::string> utf8_string;

    if( ch_utf8_length == 1 )
    {
        ASSERT(ch == static_cast<char>(ch));
        utf8_string = std::make_unique<std::string>(length + 1, static_cast<char>(ch));
    }

    else
    {
        utf8_string = std::make_unique<std::string>();
        utf8_string->reserve(string_utf8_length + ch_utf8_length);

        const std::string& utf8_representation = TC::GetUtf8ForWideChar(ch);

        for( size_t i = length + 1; i != 0; --i )
            utf8_string->append(utf8_representation);
    }

    return std::get<1>(strings.emplace_back(ch, std::move(utf8_string)))->c_str() + ch_utf8_length;
}


std::vector<std::byte> SO::CreateByteVector(const std::string_view text_sv, const bool include_null_terminator/* = false*/)
{
    static_assert(sizeof(char) == sizeof(std::byte));

    const std::byte* const text_bytes = reinterpret_cast<const std::byte*>(text_sv.data());

    std::vector bytes(text_bytes, text_bytes + text_sv.length());

    if( include_null_terminator )
        bytes.emplace_back(std::byte(0));

    return bytes;
}



// --------------------------------------------------------------------------
// formally inline implementations that are being kept around
// until the UTF8_TODO project is complete
// --------------------------------------------------------------------------

std::wstring& SO::MakeLower(std::wstring& text)
{
    std::transform(text.begin(), text.end(), text.begin(), std::towlower);
    return text;
}


void SO::MakeNewlineLF(std::wstring& text)
{
    std::string utf8_text = UTF8_TODO::GetUtf8(text);
    MakeNewlineLF(utf8_text);
    text = UTF8_TODO::GetWide(utf8_text);
}


std::tuple<size_t, size_t> SO::FindCharacters(const wstring_view text_sv, const wchar_t ch1, const wchar_t ch2, const size_t offset/* = 0*/)
{
    const size_t ch1_pos = text_sv.find(ch1, offset);
    return std::make_tuple(ch1_pos, ( ch1_pos != wstring_view::npos ) ? text_sv.find(ch2, ch1_pos + 1) :
                                                                        wstring_view::npos);
}


std::wstring& SO::AppendWithSeparator(std::wstring& destination, const wstring_view text_sv, const wchar_t separator)
{
    if( !destination.empty() )
        destination.push_back(separator);

    return SO::Append(destination, text_sv);
}
