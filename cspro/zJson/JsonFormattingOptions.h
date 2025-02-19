#pragma once

namespace jsoncons
{
    template<class CharType> class basic_json_options;
    template<class CharType> struct ModifiableOptions;
}


// --------------------------------------------------------------------------
// JsonFormattingOptions
// --------------------------------------------------------------------------

enum class JsonFormattingOptions { Compact = 0, MinimalSpacing, PrettySpacing };

constexpr JsonFormattingOptions DefaultJsonFormattingOptions = DebugMode() ? JsonFormattingOptions::PrettySpacing :
                                                                             JsonFormattingOptions::Compact;

constexpr JsonFormattingOptions DefaultJsonFileWriterFormattingOptions = JsonFormattingOptions::PrettySpacing;

const jsoncons::basic_json_options<char>& GetJsonOptions(JsonFormattingOptions formatting_options);


// --------------------------------------------------------------------------
// temporary formatting options for JsonWriter that apply when using
// JsonFormattingOptions::PrettySpacing
// --------------------------------------------------------------------------

enum class JsonFormattingType
{
    Tight = 0,                    // objects and arrays appear on the same line (rather than being written with line breaks)
    ObjectArraySingleLineSpacing, // the { } and [ ] are written as if the preceding / subsequent values were written on the same line
};

enum class JsonFormattingAction
{
    TopmostObjectLineSplitSameLine,  // values in the object being written will be on the same line
    TopmostObjectLineSplitMultiLine, // values in the object being written will be on multiple lines
};

const jsoncons::ModifiableOptions<char>& GetJsonModifiableOptions(JsonFormattingType formatting_type);
