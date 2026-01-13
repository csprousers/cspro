#include "stdafx.h"
#include "JsonFormattingOptions.h"


namespace
{
    jsoncons::basic_json_options<char> CreateJsonOptions(const JsonFormattingOptions formatting_options)
    {
        jsoncons::basic_json_options<char> json_options;

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
                json_options.spaces_around_comma(jsoncons::spaces_option::no_spaces);
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


const jsoncons::basic_json_options<char>& GetJsonOptions(const JsonFormattingOptions formatting_options)
{
    static const jsoncons::basic_json_options<char> json_options[] =
    {
        CreateJsonOptions(JsonFormattingOptions::Compact),
        CreateJsonOptions(JsonFormattingOptions::MinimalSpacing),
        CreateJsonOptions(JsonFormattingOptions::PrettySpacing)
    };

    ASSERT(static_cast<size_t>(formatting_options) < _countof(json_options));
    return json_options[static_cast<size_t>(formatting_options)];
}


namespace
{
    jsoncons::ModifiableOptions<char> CreateJsonModifiableOptions(const JsonFormattingType formatting_type)
    {
        jsoncons::basic_json_options<char> json_options = GetJsonOptions(JsonFormattingOptions::PrettySpacing);

        if( formatting_type == JsonFormattingType::Tight )
        {
            json_options.root_line_splits(jsoncons::line_split_kind::same_line);
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

        // json_options.spaces_around_comma(jsoncons::spaces_option::space_after);
        // json_options.pad_inside_object_braces(true);
        // json_options.pad_inside_array_brackets(true);

        // ...but the settings are only evaluated in the encoder's constructor, so we must construct the strings themselves

        return jsoncons::ModifiableOptions<char>
            {
                std::move(json_options),
                true,
                "{ ",
                " }",
                "[ ",
                " ]"
            };
    }
}


const jsoncons::ModifiableOptions<char>& GetJsonModifiableOptions(const JsonFormattingType formatting_type)
{
    static const jsoncons::ModifiableOptions<char> modifiable_options[] =
    {
        CreateJsonModifiableOptions(JsonFormattingType::Tight),
        CreateJsonModifiableOptions(JsonFormattingType::ObjectArraySingleLineSpacing)
    };

    ASSERT(static_cast<size_t>(formatting_type) < _countof(modifiable_options));
    return modifiable_options[static_cast<size_t>(formatting_type)];
}
