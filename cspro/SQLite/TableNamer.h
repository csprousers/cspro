#pragma once

#include <zToolsO/Hash.h>


// SQLite allows a variety of table names if preceeded with a special character like [
// but this routine creates a name with these rules:
//     - "Names can contain only alphanumeric characters and must begin with an alphabetic character or an underscore (_)."
//     - name cannot end in _HC
//     - if any characters exist that do not follow those rules, they will be stored and a hash of the characters will be
//       appended to the table name, followed by _HC

inline std::string CreateSQLiteValidTableName(std::string table_name)
{
    // note that SQLite table names are case-insensitive
    constexpr std::string_view HashedCharactersSuffix_sv = "_HC";

    std::optional<size_t> invalid_characters_hash;

    auto add_character_to_hash = [&](const size_t pos)
    {
        if( !invalid_characters_hash.has_value() )
            invalid_characters_hash = 0;

        Hash::Combine(*invalid_characters_hash, table_name[pos]);

        // also hash the position of the character so that different table names are returned for "a b" and "ab "
        Hash::Combine(*invalid_characters_hash, pos);

        table_name.erase(table_name.begin() + pos);
    };

    for( size_t i = 0; i < table_name.size(); )
    {
        if( is_tokch(table_name[i]) )
        {
            ++i;
        }

        else
        {
            add_character_to_hash(i);
        }
    }

    // make sure the first character is not a number
    while( !table_name.empty() && is_digit(table_name.front()) )
        add_character_to_hash(0);

    // do not allow the table to end with _HC
    if( table_name.length() >= HashedCharactersSuffix_sv.length() )
    {
        const size_t invalid_suffix_pos = table_name.length() - HashedCharactersSuffix_sv.length();
        
        if( SO::EqualsNoCase(UTF8Convert::UTF8ToWide(HashedCharactersSuffix_sv), UTF8Convert::UTF8ToWide(table_name.data() + invalid_suffix_pos)) )
        {
            for( size_t i = HashedCharactersSuffix_sv.length(); i > 0; --i )
                add_character_to_hash(invalid_suffix_pos);
        }
    }

    // if any invalid characters were used, append the hash code and suffix to the table name
    if( invalid_characters_hash.has_value() )
    {
        table_name.append(UTF8Convert::WideToUTF8(IntToString(*invalid_characters_hash)));
        table_name.append(HashedCharactersSuffix_sv);
    }

    return table_name;
}
