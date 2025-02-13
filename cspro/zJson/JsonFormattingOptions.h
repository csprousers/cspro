#pragma once

namespace jsoncons
{
    template <class CharType> class basic_json_options;
    template <class CharType> struct ModifiableOptions;
}


// --------------------------------------------------------------------------
// JsonFormattingOptions
// --------------------------------------------------------------------------

enum class JsonFormattingOptions { Compact = 0, MinimalSpacing, PrettySpacing };

constexpr JsonFormattingOptions DefaultJsonFormattingOptions = DebugMode() ? JsonFormattingOptions::PrettySpacing :
                                                                             JsonFormattingOptions::Compact;

constexpr JsonFormattingOptions DefaultJsonFileWriterFormattingOptions = JsonFormattingOptions::PrettySpacing;

template<typename CharType>
const jsoncons::basic_json_options<CharType>& GetJsonOptions(JsonFormattingOptions formatting_options);


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

template<typename CharType>
const jsoncons::ModifiableOptions<CharType>& GetJsonModifiableOptions(JsonFormattingType formatting_type);
