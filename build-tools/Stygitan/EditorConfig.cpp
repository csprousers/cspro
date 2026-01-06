#include "StdAfx.h"
#include "EditorConfig.h"
#include <external/editorconfig/editorconfig.h>


// --------------------------------------------------------------------------
// EditorConfig::OptionStrings
// --------------------------------------------------------------------------

const char* EditorConfig::OptionStrings::Indent[2] = { "space", "tab" };

const char* EditorConfig::OptionStrings::EndOfLine[3] = { "lf", "cr", "crlf" };

const char* EditorConfig::OptionStrings::Charset[5] = { "latin1", "utf-8", "utf-8-bom", "utf-16be", "utf-16le" };



// --------------------------------------------------------------------------
// EditorConfig::Options
// --------------------------------------------------------------------------

bool EditorConfig::Options::operator<(const Options& rhs) const noexcept
{
    std::optional<bool> lt;

    // assume optional values without a value are less than
    auto optional_lt = [&](const auto& lhs_value, const auto& rhs_value)
    {
        if( lhs_value.has_value() && rhs_value.has_value() )
        {
            if( *lhs_value != *rhs_value )
                lt = ( *lhs_value < *rhs_value );
        }

        else if( lhs_value.has_value() )
        {
            lt = false;
        }

        else if( rhs_value.has_value() )
        {
            lt = true;
        }

        return !lt.has_value();
    };

    optional_lt(indent_style, rhs.indent_style) &&
    optional_lt(indent_size, rhs.indent_size) &&
    optional_lt(end_of_line, rhs.end_of_line) &&
    optional_lt(charset, rhs.charset) &&
    optional_lt(trim_trailing_whitespace, rhs.trim_trailing_whitespace) &&
    optional_lt(insert_final_newline, rhs.insert_final_newline) &&
    optional_lt(stygitan_trim_final_newlines, rhs.stygitan_trim_final_newlines);

    return lt.value_or(false);
}


bool EditorConfig::Options::IsDefined() const noexcept
{
    return ( indent_style.has_value() ||
             indent_size.has_value() ||
             end_of_line.has_value() ||
             charset.has_value() ||
             trim_trailing_whitespace.has_value() ||
             insert_final_newline.has_value() ||
             stygitan_trim_final_newlines.has_value() );
}


std::string EditorConfig::Options::GetShortDescription() const noexcept
{
    ASSERT(IsDefined());

    std::string description;

    auto add_separator = [&]() -> std::string&
    {
        if( !description.empty() )
            description.push_back('-');

        return description;
    };

    if( indent_style.has_value() )
        description.append("indent_style(").append(OptionStrings::Indent[static_cast<size_t>(*indent_style)]).push_back(')');

    if( indent_size.has_value() )
        add_separator().append("indent_size(").append(IntToString(*indent_size)).push_back(')');

    if( end_of_line.has_value() )
        add_separator().append("end_of_line(").append(OptionStrings::EndOfLine[static_cast<size_t>(*end_of_line)]).push_back(')');

    if( charset.has_value() )
        add_separator().append("charset(").append(OptionStrings::Charset[static_cast<size_t>(*charset)]).push_back(')');

    if( trim_trailing_whitespace )
        add_separator().append("trim_trailing_whitespace");

    if( insert_final_newline  )
        add_separator().append("insert_final_newline");

    if( stygitan_trim_final_newlines )
        add_separator().append("stygitan_trim_final_newlines");

    return description;
}


std::string EditorConfig::Options::GetLongDescription() const noexcept
{
    constexpr const char* BoolDescriptions[2] = { "false\n", "true\n" };

    ASSERT(IsDefined());

    std::string description;

    if( indent_style.has_value() )
        description.append("indent_style: ").append(OptionStrings::Indent[static_cast<size_t>(*indent_style)]).push_back('\n');

    if( indent_size.has_value() )
        description.append("indent_size: ").append(IntToString(*indent_size)).push_back('\n');

    if( end_of_line.has_value() )
        description.append("end_of_line: ").append(OptionStrings::EndOfLine[static_cast<size_t>(*end_of_line)]).push_back('\n');

    if( charset.has_value() )
        description.append("charset: ").append(OptionStrings::Charset[static_cast<size_t>(*charset)]).push_back('\n');

    if( trim_trailing_whitespace.has_value() )
        description.append("trim_trailing_whitespace: ").append(BoolDescriptions[*trim_trailing_whitespace]);

    if( insert_final_newline  )
        description.append("insert_final_newline: ").append(BoolDescriptions[*insert_final_newline]);

    if( stygitan_trim_final_newlines )
        description.append("stygitan_trim_final_newlines: ").append(BoolDescriptions[*stygitan_trim_final_newlines]);

    return description;
}



// --------------------------------------------------------------------------
// EditorConfig::Evaluator
// --------------------------------------------------------------------------

EditorConfig::Evaluator::~Evaluator() noexcept
{
    if( !m_tempDirectoryForDefaultProcessing.empty() )
        PortableFunctions::DirectoryDelete(m_tempDirectoryForDefaultProcessing, true);
}


const std::string& EditorConfig::Evaluator::GetTempDirectoryForDefaultProcessing()
{
    if( !m_tempDirectoryForDefaultProcessing.empty() )
        return m_tempDirectoryForDefaultProcessing;

    // create a new temporary directory and save the default .editorconfig file in it
    m_tempDirectoryForDefaultProcessing = Path::Combine(GetTempDirectory(), "EditorConfigEvaluator-" + IntToString(GetTimestamp<int64_t>()));

    HRSRC resource;
    HGLOBAL loaded_resource;
    LPVOID resource_data;

    if( ( resource = FindResource(nullptr, MAKEINTRESOURCE(IDR_DEFAULT_EDITOR_CONFIG), RT_RCDATA) ) == nullptr ||
        ( loaded_resource = LoadResource(nullptr, resource) ) == nullptr ||
        ( resource_data = LockResource(loaded_resource) ) == nullptr )
    {
        throw CSProException("There was an error accessing resource data.");
    }

    // save the .editorconfig file
    FileIO::Write(Path::Combine(m_tempDirectoryForDefaultProcessing, ".editorconfig"),
                  resource_data,
                  SizeofResource(nullptr, resource));

    return m_tempDirectoryForDefaultProcessing;
}


EditorConfig::Options EditorConfig::Evaluator::Parse(const cs::string_view_sz file_path_sv, const bool fallback_to_cspro_default_editorconfig/* = true*/)
{
    editorconfig_handle handle = editorconfig_handle_init();

    if( handle == nullptr )
        throw CSProException("Could not initialize the EditorConfig library.");

    const RAII::RunOnDestruction free_handle([&]() { editorconfig_handle_destroy(handle); });

    const int parse_result = editorconfig_parse(file_path_sv.c_str(), handle);

    if( parse_result != 0 )
        throw CSProException(editorconfig_get_error_msg(parse_result));

    const int count_properties = editorconfig_handle_get_name_value_count(handle);

    // when no properties are defined, optionally use CSPro's default .editorconfig file
    if( count_properties == 0 && fallback_to_cspro_default_editorconfig )
    {
        const std::string fake_file_path = Path::Combine(GetTempDirectoryForDefaultProcessing(), Path::GetFilename(file_path_sv));
        return Parse(fake_file_path, false);
    }

    Options options;
    std::optional<int> tab_width;

    for( int i = 0; i < count_properties; ++i )
    {
        constexpr const char* InvalidValueFormatter = "Unknown %s: %s";
        const char* name;
        const char* value;
        editorconfig_handle_get_name_value(handle, i, &name, &value);

        auto evaluate_bool = [&]()
        {
            return ( _stricmp(value, "true") == 0 )  ? true :
                   ( _stricmp(value, "false") == 0 ) ? false :
                                                       throw CSProException(InvalidValueFormatter, name, value);
        };

        auto evaluate_enum = [&](const char* const options[], const int num_options) -> int
        {
            for( int i = 0; i < num_options; ++i )
            {
                if( _stricmp(value, options[i]) == 0 )
                    return i;
            }

            throw CSProException(InvalidValueFormatter, name, value);
        };

        if( strcmp(name, "indent_style") == 0 )
        {
            options.indent_style = static_cast<Indent>(evaluate_enum(OptionStrings::Indent, _countof(OptionStrings::Indent)));
        }

        else if( strcmp(name, "indent_size") == 0 )
        {
            options.indent_size = static_cast<int>(StringToNumber(value));
        }

        else if( strcmp(name, "tab_width") == 0 )
        {
            tab_width = static_cast<int>(StringToNumber(value));
        }

        else if( strcmp(name, "end_of_line") == 0 )
        {
            options.end_of_line = static_cast<EndOfLine>(evaluate_enum(OptionStrings::EndOfLine, _countof(OptionStrings::EndOfLine)));
        }

        else if( strcmp(name, "charset") == 0 )
        {
            options.charset = static_cast<Charset>(evaluate_enum(OptionStrings::Charset, _countof(OptionStrings::Charset)));
        }

        else if( strcmp(name, "trim_trailing_whitespace") == 0 )
        {
            options.trim_trailing_whitespace = evaluate_bool();
        }

        else if( strcmp(name, "insert_final_newline") == 0 )
        {
            options.insert_final_newline = evaluate_bool();
        }

        else if( strcmp(name, "root") == 0 )
        {
            // ignore
        }

        else if( strcmp(name, "stygitan_trim_final_newlines") == 0 )
        {
            options.stygitan_trim_final_newlines = evaluate_bool();
        }

        else
        {
            throw CSProException(InvalidValueFormatter, name, value);
        }
    }

    if( options.indent_size != tab_width )
        throw CSProException("The values for indent_size and tab_width must be the same.");

    return options;
}
