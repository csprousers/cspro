#include "StdAfx.h"
#include "TextConverter.h"


namespace AnsiConverter
{
    constexpr char UnconvertableCharacter = '?';

    // returns an array of 256 characters with the wide character associated with each ANSI character
    const wchar_t* GetAnsiToWideMap();

    // returns a mapping of wide characters mapped to their ANSI equivalents (when falling outside of the joint range)
    const std::map<wchar_t, char>& GetWideToAnsiMap();

    // all characters from 0 - 127, 129, and 160 - 255 are the same
    template<typename CT>
    constexpr bool CharacterIsInJointRange(const CT ch)
    {
        return ( ch < 0 )                 ? false :
               ( ch <= 127 || ch == 129 ) ? true :
                                            ( ch >= 160 && ch <= 255 );
    }

    constexpr std::tuple<int, int> AnsiToWideMappingOutsideOfJointRange[] =
    {
        { 128, 8364 }, // '€'
        { 130, 8218 }, // '‚'
        { 131, 402  }, // 'ƒ'
        { 132, 8222 }, // '„'
        { 133, 8230 }, // '…'
        { 134, 8224 }, // '†'
        { 135, 8225 }, // '‡'
        { 136, 710  }, // 'ˆ'
        { 137, 8240 }, // '‰'
        { 138, 352  }, // 'Š'
        { 139, 8249 }, // '‹'
        { 140, 338  }, // 'Œ'
        { 142, 381  }, // 'Ž'
        { 145, 8216 }, // '‘'
        { 146, 8217 }, // '’'
        { 147, 8220 }, // '“'
        { 148, 8221 }, // '”'
        { 149, 8226 }, // '•'
        { 150, 8211 }, // '–'
        { 151, 8212 }, // '—'
        { 152, 732  }, // '˜'
        { 153, 8482 }, // '™'
        { 154, 353  }, // 'š'
        { 155, 8250 }, // '›'
        { 156, 339  }, // 'œ'
        { 158, 382  }, // 'ž'
        { 159, 376  }, // 'Ÿ'
    };
}


const wchar_t* AnsiConverter::GetAnsiToWideMap()
{
    static const std::unique_ptr<wchar_t[]> ansi_to_wide_map = []()
    {
        auto map = std::make_unique_for_overwrite<wchar_t[]>(256);

        // assign all of the default values
        for( wchar_t i = 0; i < 256; ++i )
            map[i] = i;

        // assign all of the specially mapped values
        for( const auto& [ansi_code, wide_code] : AnsiToWideMappingOutsideOfJointRange )
        {
            ASSERT(!CharacterIsInJointRange(ansi_code));
            map[ansi_code] = static_cast<wchar_t>(wide_code);
        }

        return map;
    }();

    return ansi_to_wide_map.get();
}


const std::map<wchar_t, char>& AnsiConverter::GetWideToAnsiMap()
{
    static const std::map<wchar_t, char> wide_to_ansi_map = []()
    {
        std::map<wchar_t, char> map;

        // add the specially mapped codes
        for( const auto& [ansi_code, wide_code] : AnsiToWideMappingOutsideOfJointRange )
        {
            ASSERT(!CharacterIsInJointRange(ansi_code));
            map.try_emplace(static_cast<wchar_t>(wide_code), static_cast<char>(ansi_code));
        }

        return map;
    }();

    return wide_to_ansi_map;
}


std::wstring TextConverter::WindowsAnsiToWideWorker(const char* const non_null_ansi_string, const size_t ansi_length)
{
    ASSERT(non_null_ansi_string != nullptr && ansi_length >= 0);

    std::wstring wide_string(ansi_length, '\0');

    WindowsAnsiToWideBufferWorker(non_null_ansi_string, wide_string.data(), ansi_length);

    return wide_string;
}


int TextConverter::WindowsAnsiToWideBufferWorker(const char* non_null_ansi_string, wchar_t* non_null_wide_buffer, const size_t length)
{
    ASSERT(non_null_ansi_string != nullptr && non_null_wide_buffer != nullptr);
    ASSERT(length != SIZE_MAX);

    const wchar_t* const ansi_to_wide_map = AnsiConverter::GetAnsiToWideMap();

    const unsigned char* non_null_ansi_string_itr = reinterpret_cast<const unsigned char*>(non_null_ansi_string);
    const unsigned char* const non_null_ansi_string_end = non_null_ansi_string_itr + length;

    for( ; non_null_ansi_string_itr != non_null_ansi_string_end; ++non_null_ansi_string_itr, ++non_null_wide_buffer )
        *non_null_wide_buffer = ansi_to_wide_map[*non_null_ansi_string_itr];

    return length;
}


std::string TextConverter::WideToWindowsAnsiWorker(const wchar_t* non_null_wide_string, size_t wide_length)
{
    ASSERT(non_null_wide_string != nullptr && wide_length >= 0);

    const std::map<wchar_t, char>& wide_to_ansi_map = AnsiConverter::GetWideToAnsiMap();

    const wchar_t*& non_null_wide_string_itr = non_null_wide_string;
    const wchar_t* const non_null_ansi_string_end = non_null_wide_string_itr + wide_length;

    std::string multi_byte_string(wide_length, '\0');
    char* multi_byte_string_itr = multi_byte_string.data();

    for( ; non_null_wide_string_itr != non_null_ansi_string_end; ++non_null_wide_string_itr, ++multi_byte_string_itr )
    {
        const wchar_t wide_ch = *non_null_wide_string_itr;

        if( AnsiConverter::CharacterIsInJointRange(wide_ch) )
        {
            *multi_byte_string_itr = static_cast<char>(wide_ch);
        }

        else
        {
            const auto& lookup = wide_to_ansi_map.find(wide_ch);
            *multi_byte_string_itr = ( lookup != wide_to_ansi_map.cend() ) ? lookup->second :
                                                                             AnsiConverter::UnconvertableCharacter;
        }
    }

    return multi_byte_string;
}


std::string TextConverter::AnsiToUtf8(const std::string_view ansi_text_sv)
{
    const wchar_t* const ansi_to_wide_map = AnsiConverter::GetAnsiToWideMap();

    std::string utf8_text;
    utf8_text.reserve(ansi_text_sv.size());

    for( const unsigned char ch : ansi_text_sv )
    {
        if( TC::IsUtf8SingleByte(ch) )
        {
            ASSERT(AnsiConverter::CharacterIsInJointRange(ch));
            utf8_text.push_back(static_cast<char>(ch));
        }

        else
        {
            utf8_text.append(TC::GetUtf8ForWideChar(ansi_to_wide_map[ch]));
        }
    }

    ASSERT81(ansi_text_sv == Utf8ToAnsi(utf8_text));

    return utf8_text;
}


std::string TextConverter::Utf8ToAnsi(const std::string_view utf8_text_sv)
{
    const std::map<wchar_t, char>& wide_to_ansi_map = AnsiConverter::GetWideToAnsiMap();

    std::string ansi_text;
    ansi_text.reserve(utf8_text_sv.size());

    const char* text_itr = utf8_text_sv.data();
    const char* const text_end = text_itr + utf8_text_sv.length();

    while( text_itr < text_end )
    {
        const unsigned char ch = *text_itr;
        const size_t ch_length = TC::Utf8BytesFromFirstByte(ch);

        if( ch_length == 1 )
        {
            ASSERT(AnsiConverter::CharacterIsInJointRange(ch));
            ansi_text.push_back(static_cast<char>(ch));
            ++text_itr;
        }

        else
        {
            const wchar_t wide_ch = TC::GetWideCharFromUtf8Sequence(text_itr, ch_length);
            text_itr += ch_length;

            if( AnsiConverter::CharacterIsInJointRange(wide_ch) )
            {
                ansi_text.push_back(static_cast<char>(wide_ch));
            }

            else
            {
                const auto& lookup = wide_to_ansi_map.find(wide_ch);
                ansi_text.push_back(( lookup != wide_to_ansi_map.cend() ) ? lookup->second :
                                                                            AnsiConverter::UnconvertableCharacter);
            }
        }
    }

    return ansi_text;
}
