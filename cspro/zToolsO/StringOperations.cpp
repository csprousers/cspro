#include "StdAfx.h"
#include "StringOperations.h"
#include <mutex>


// ------------------------------------------------------
// std::wstring <-> CString temporary operations
// ------------------------------------------------------

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



// ------------------------------------------------------
// EmptyString
// ------------------------------------------------------

const std::wstring SO::EmptyString;
const CString SO::EmptyCString;



// ------------------------------------------------------
// Case functions
// ------------------------------------------------------

std::wstring SO::ToProperCase(std::wstring text)
{
    bool next_char_should_be_upper_case = true;

    for( TCHAR* itr = text.data(); *itr != 0; ++itr )
    {
        if( *itr == ' ' )
        {
            next_char_should_be_upper_case = true;
        }

        else if( next_char_should_be_upper_case )
        {
            *itr = std::towupper(*itr);
            next_char_should_be_upper_case = false;
        }

        else
        {
            // if this function is ever used with characters like (), figure out what characters
            // should result in next_char_should_be_upper_case being set to true
            ASSERT(is_alpha(*itr));

            *itr = std::towlower(*itr);
        }
    }

    return text;
}



// ------------------------------------------------------
// Comparison functions
// ------------------------------------------------------

bool SO::Equals(wstring_view sv1, std::string_view sv2)
{
    return ( UTF8Convert::WideToUTF8(sv1) == sv2 );
}



// ------------------------------------------------------
// Newline functions
// ------------------------------------------------------

void SO::MakeSplitVectorStringsOnNewlines(std::vector<std::wstring>& strings)
{
    for( auto strings_itr = strings.begin(); strings_itr != strings.cend(); )
    {
        if( !SO::ContainsNewlineCharacter(*strings_itr) )
        {
            ++strings_itr;
            continue;
        }

        const std::wstring this_string = *strings_itr;
        bool first_string = true;

        SO::ForeachLine(this_string, true,
            [&](wstring_view text_sv)
            {
                if( first_string )
                {
                    *strings_itr = text_sv;
                    first_string = false;
                }

                else
                {
                    strings_itr = strings.insert(strings_itr, text_sv);
                }

                ++strings_itr;
                    
                return true;
            });

        ASSERT(!first_string);
    }
}



// ------------------------------------------------------
// Manipulation functions
// ------------------------------------------------------

namespace
{
    template<typename Predicate>
    wstring_view TrimLeft(wstring_view text_sv, Predicate predicate)
    {
        size_t length = text_sv.length();

        // short-circuit the most common outcome
        if( length == 0 || !predicate(text_sv[0]) )
            return text_sv;

        const wchar_t* text_itr = text_sv.data() + 1;
        --length;

        while( length > 0 && predicate(*text_itr) )
        {
            ++text_itr;
            --length;
        }

        return wstring_view(text_itr, length);
    }

    template<typename Predicate>
    wstring_view TrimRight(wstring_view text_sv, Predicate predicate)
    {
        if( text_sv.empty() )
            return text_sv;

        size_t length = text_sv.length();
        const wchar_t* text_itr = text_sv.data() + length - 1;

        while( length > 0 && predicate(*text_itr) )
        {
            --text_itr;
            --length;
        }

        return wstring_view(text_sv.data(), length);
    }
}

wstring_view SO::TrimLeft(wstring_view text_sv)
{
    return ::TrimLeft(text_sv, [](TCHAR ch) { return std::iswspace(ch); });
}

wstring_view SO::TrimRight(wstring_view text_sv)
{
    return ::TrimRight(text_sv, [](TCHAR ch) { return std::iswspace(ch); });
}

wstring_view SO::TrimLeft(wstring_view text_sv, TCHAR trim_char)
{
    return ::TrimLeft(text_sv, [trim_char](TCHAR ch) { return ( ch == trim_char ); });
}

wstring_view SO::TrimLeft(wstring_view text_sv, wstring_view trim_chars_sv)
{
    return ::TrimLeft(text_sv, [trim_chars_sv](TCHAR ch) { return ( trim_chars_sv.find(ch) != wstring_view::npos ); });
}

wstring_view SO::TrimRight(wstring_view text_sv, TCHAR trim_char)
{
    return ::TrimRight(text_sv, [trim_char](TCHAR ch) { return ( ch == trim_char ); });
}

wstring_view SO::TrimRight(wstring_view text_sv, wstring_view trim_chars_sv)
{
    return ::TrimRight(text_sv, [trim_chars_sv](TCHAR ch) { return ( trim_chars_sv.find(ch) != wstring_view::npos ); });
}


std::wstring& SO::Replace(std::wstring& text, TCHAR old_char, TCHAR new_char, std::wstring::size_type ch_pos/* = 0*/)
{
    while( ( ch_pos = text.find(old_char, ch_pos) ) != std::wstring::npos )
    {
        text.replace(ch_pos, 1, 1, new_char);
        ++ch_pos;
    }

    return text;
}


std::wstring& SO::Replace(std::wstring& text, wstring_view old_text_sv, wstring_view new_text_sv, std::wstring::size_type ch_pos/* = 0*/)
{
    while( ( ch_pos = text.find(old_text_sv, ch_pos) ) != std::wstring::npos )
    {
        text.replace(ch_pos, old_text_sv.length(), new_text_sv.data(), new_text_sv.length());
        ch_pos += new_text_sv.length();
    }

    return text;
}


std::wstring& SO::ReplaceNoCase(std::wstring& text, wstring_view old_text_sv, wstring_view new_text_sv, std::wstring::size_type ch_pos/* = 0*/)
{
    while( ( ch_pos = FindNoCase(text, old_text_sv, ch_pos) ) != wstring_view::npos )
    {
        text.replace(ch_pos, old_text_sv.length(), new_text_sv.data(), new_text_sv.length());
        ch_pos += new_text_sv.length();
    }

    return text;
}


std::wstring& SO::RecursiveReplace(std::wstring& text, wstring_view old_text_sv, wstring_view new_text_sv)
{
    size_t initial_length;

    do
    {
        initial_length = text.length();
        SO::Replace(text, old_text_sv, new_text_sv);

    } while( initial_length != text.length() );

    return text;
}


std::wstring& SO::Remove(std::wstring& text, TCHAR ch)
{
    std::wstring::size_type ch_pos = 0;

    while( ( ch_pos = text.find(ch, ch_pos) ) != std::wstring::npos )
        text.erase(ch_pos, 1);

    return text;
}


std::wstring& SO::CenterExactLength(std::wstring& text, size_t length)
{
    ASSERT(!SO::ContainsNewlineCharacter(text));

    if( text.length() < length )
    {
        // insert left spaces; the right spaces will be added by MakeExactLength
        const size_t left_spaces_needed = ( length - text.length() ) / 2;
        text.insert(0, left_spaces_needed, ' ');
    }

    return SO::MakeExactLength(text, length);
}


wstring_view SO::RemoveTextFollowingCharacter(wstring_view text_sv, char ch, bool remove_ch)
{
    const size_t ch_pos = text_sv.find(ch);

    return ( ch_pos != wstring_view::npos ) ? text_sv.substr(0, ch_pos + ( remove_ch ? 0 : 1 )) :
                                              text_sv;
}


namespace
{
    template<bool trim_right_each_line>
    int ConvertTabsToSpacesWorker(std::wstring& text, int position_in_line)
    {
        // when trim_right_each_line is true, position_in_line should not be considered accurate,
        // which is fine because SO::ConvertTabsToSpacesAndTrimRightEachLine does not return that value
        for( size_t i = 0; i < text.length(); ++i )
        {
            const TCHAR ch = text[i];

            if( is_crlf(ch) )
            {
                position_in_line = 0;

                if constexpr(trim_right_each_line)
                {
                    if( i > 0 )
                    {
                        const wstring_view text_up_to_this_crlf_sv = wstring_view(text).substr(0, i);
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
                    const int spaces_to_insert = SO::DefaultSpacesPerTab - ( position_in_line % SO::DefaultSpacesPerTab ) - 1;

                    if( spaces_to_insert != 0 )
                    {
                        text.insert(i + 1, SO::GetRepeatingCharacterString(' ', spaces_to_insert));
                        position_in_line += spaces_to_insert;
                        i += spaces_to_insert;
                    }

                    ASSERT(( position_in_line + 1 ) % SO::DefaultSpacesPerTab == 0);
                }

                ++position_in_line;
            }
        }

        if constexpr(trim_right_each_line)
        {
            SO::MakeTrimRightSpace(text);
        }

        return position_in_line;
    }
}


int SO::ConvertTabsToSpaces(std::wstring& text, const int position_in_line/* = 0*/)
{
    return ConvertTabsToSpacesWorker<false>(text, position_in_line);
}


void SO::ConvertTabsToSpacesAndTrimRightEachLine(std::wstring& text)
{
    ConvertTabsToSpacesWorker<true>(text, 0);
}



// ------------------------------------------------------
// Other functions
// ------------------------------------------------------

template<typename T/* = std::wstring*/>
std::vector<T> SO::SplitString(wstring_view source_sv, const TCHAR* separators, bool trim_all/* = true*/, bool include_empty_entities/* = true*/)
{
    std::vector<T> entities;

    SO::ForeachSection(source_sv, separators,
        [&](wstring_view entity_sv)
        {
            if( include_empty_entities || !SO::IsWhitespace(entity_sv) )
            {
                entities.emplace_back(trim_all ? SO::Trim(entity_sv) :
                                                 entity_sv);
            }

            return true;
        });

    return entities;
}

template CLASS_DECL_ZTOOLSO std::vector<std::wstring> SO::SplitString(wstring_view source_sv, const TCHAR* separators, bool trim_all/* = true*/, bool include_empty_entities/* = true*/);
template CLASS_DECL_ZTOOLSO std::vector<wstring_view> SO::SplitString(wstring_view source_sv, const TCHAR* separators, bool trim_all/* = true*/, bool include_empty_entities/* = true*/);


std::vector<std::wstring> SO::WrapText(wstring_view text_sv, size_t maximum_line_width)
{
    constexpr const TCHAR* SpaceCharacters = _T(" \t\r\n");
    constexpr wstring_view SpaceCharactersWithoutNewline_sv(SpaceCharacters, 3);
    constexpr TCHAR HyphenCharacter = '-';

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


const TCHAR* SO::GetRepeatingCharacterString(TCHAR ch, size_t length)
{
    // keep the strings on hand for repeated use
    static std::vector<std::unique_ptr<std::wstring>> strings;
    static std::mutex mtx;

    std::lock_guard<std::mutex> lock(mtx);

    // use an existing line, potentially offsetting the index
    for( const std::unique_ptr<std::wstring>& string : strings )
    {
        if( string->front() == ch && string->length() >= length )
        {
            size_t length_difference = string->length() - length;
            return string->c_str() + length_difference;
        }
    }

    // or create a new one
    if( length == 0 )
        return EmptyString.c_str();

    return strings.emplace_back(std::make_unique<std::wstring>(length, ch))->c_str();
}
