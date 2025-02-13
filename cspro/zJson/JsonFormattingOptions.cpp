#include "stdafx.h"
#include "JsonFormattingOptions.h"


namespace
{
    template<typename CharType>
    jsoncons::basic_json_options<CharType> CreateJsonOptions(JsonFormattingOptions formatting_options)
    {
        jsoncons::basic_json_options<CharType> json_options;

        switch( formatting_options )
        {
            case JsonFormattingOptions::MinimalSpacing:
            {
                json_options.line_length_limit(SIZE_MAX);
                json_options.indent_size(2);
                json_options.spaces_around_colon(jsoncons::spaces_option::no_spaces);
                json_options.spaces_around_comma(jsoncons::spaces_option::no_spaces);
                json_options.object_object_line_splits(jsoncons::line_split_kind::same_line);
                json_options.array_object_line_splits(jsoncons::line_split_kind::same_line);
                json_options.object_array_line_splits(jsoncons::line_split_kind::same_line);
                json_options.array_array_line_splits(jsoncons::line_split_kind::same_line);
                break;
            }

            case JsonFormattingOptions::PrettySpacing:
            {
                json_options.line_length_limit(SIZE_MAX);
                json_options.indent_size(2);
                json_options.spaces_around_colon(jsoncons::spaces_option::space_after);
                json_options.spaces_around_comma(jsoncons::spaces_option::space_after);
                json_options.object_object_line_splits(jsoncons::line_split_kind::multi_line);
                json_options.array_object_line_splits(jsoncons::line_split_kind::multi_line);
                json_options.object_array_line_splits(jsoncons::line_split_kind::multi_line);
                json_options.array_array_line_splits(jsoncons::line_split_kind::multi_line);
                break;
            }
        }

        return json_options;
    }
}


template<typename CharType>
const jsoncons::basic_json_options<CharType>& GetJsonOptions(JsonFormattingOptions formatting_options)
{
    static const jsoncons::basic_json_options<CharType> json_options[] =
    {
        CreateJsonOptions<CharType>(JsonFormattingOptions::Compact),
        CreateJsonOptions<CharType>(JsonFormattingOptions::MinimalSpacing),
        CreateJsonOptions<CharType>(JsonFormattingOptions::PrettySpacing)
    };

    ASSERT(static_cast<size_t>(formatting_options) < _countof(json_options));
    return json_options[static_cast<size_t>(formatting_options)];
}

template const jsoncons::basic_json_options<char>& GetJsonOptions(JsonFormattingOptions formatting_options);
template const jsoncons::basic_json_options<wchar_t>& GetJsonOptions(JsonFormattingOptions formatting_options);



namespace
{
    template<typename CharType>
    jsoncons::ModifiableOptions<CharType> CreateJsonModifiableOptions(JsonFormattingType formatting_type)
    {
        jsoncons::basic_json_options<CharType> json_options = GetJsonOptions<CharType>(JsonFormattingOptions::PrettySpacing);

        if( formatting_type == JsonFormattingType::Tight )
        {
            json_options.object_object_line_splits(jsoncons::line_split_kind::same_line);
            json_options.array_object_line_splits(jsoncons::line_split_kind::same_line);
            json_options.object_array_line_splits(jsoncons::line_split_kind::same_line);
            json_options.array_array_line_splits(jsoncons::line_split_kind::same_line);
        }

        else
        {
            ASSERT(formatting_type == JsonFormattingType::ObjectArraySingleLineSpacing);

            // no changes needed
        }

        // ideally we could call these methods...

        // json_options.pad_inside_object_braces(true);
        // json_options.pad_inside_array_brackets(true);

        // ...but the settings are only evaluated in the encoder's constructor, so we must construct the strings themselves

        return jsoncons::ModifiableOptions<CharType>
            {
                std::move(json_options),
                std::basic_string<CharType>({ '{', ' ' }),
                std::basic_string<CharType>({ ' ', '}' }),
                std::basic_string<CharType>({ '[', ' ' }),
                std::basic_string<CharType>({ ' ', ']' })
            };
    }
}


template<typename CharType>
const jsoncons::ModifiableOptions<CharType>& GetJsonModifiableOptions(JsonFormattingType formatting_type)
{
    static const jsoncons::ModifiableOptions<CharType> modifiable_options[] =
    {
        CreateJsonModifiableOptions<CharType>(JsonFormattingType::Tight),
        CreateJsonModifiableOptions<CharType>(JsonFormattingType::ObjectArraySingleLineSpacing)
    };

    ASSERT(static_cast<size_t>(formatting_type) < _countof(modifiable_options));
    return modifiable_options[static_cast<size_t>(formatting_type)];
}

template const jsoncons::ModifiableOptions<char>& GetJsonModifiableOptions(JsonFormattingType formatting_type);
template const jsoncons::ModifiableOptions<wchar_t>& GetJsonModifiableOptions(JsonFormattingType formatting_type);
