#include "StdAfx.h"
#include "EditorConfig.h"
#include <external/editorconfig/editorconfig.h>


// --------------------------------------------------------------------------
// EditorConfig::Options
// --------------------------------------------------------------------------

bool EditorConfig::Options::IsDefined() const
{
    return ( indent_style.has_value() ||
             indent_size.has_value() ||
             end_of_line.has_value() ||
             charset.has_value() ||
             trim_trailing_whitespace.has_value() ||
             insert_final_newline.has_value() ||
             trim_final_newlines.has_value() );
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


EditorConfig::Options EditorConfig::Evaluator::Parse(const cs::string_view_sz file_path_sv, const bool using_default_editorconfig)
{
    editorconfig_handle handle = editorconfig_handle_init();

    if( handle == nullptr )
        throw CSProException("Could not initialize the EditorConfig library.");

    const RAII::RunOnDestruction free_handle([&]() { editorconfig_handle_destroy(handle); });

    const int parse_result = editorconfig_parse(file_path_sv.c_str(), handle);

    if( parse_result != 0 )
        throw CSProException(editorconfig_get_error_msg(parse_result));

    const int count_properties = editorconfig_handle_get_name_value_count(handle);

    // when no properties are defined, use the default .editorconfig file
    if( count_properties == 0 && !using_default_editorconfig )
    {
        const std::string fake_file_path = Path::Combine(GetTempDirectoryForDefaultProcessing(), Path::GetFilename(file_path_sv));
        return Parse(fake_file_path, true);
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

        if( strcmp(name, "indent_style") == 0 )
        {
            options.indent_style =
                ( _stricmp(value, "space") == 0 ) ? Indent::Space :
                ( _stricmp(value, "tab") == 0 )   ? Indent::Tab :
                                                    throw CSProException(InvalidValueFormatter, name, value);
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
            options.end_of_line =
                ( _stricmp(value, "lf") == 0 )   ? EndOfLine::LF :
                ( _stricmp(value, "cr") == 0 )   ? EndOfLine::CR :
                ( _stricmp(value, "crlf") == 0 ) ? EndOfLine::CRLF :
                                                   throw CSProException(InvalidValueFormatter, name, value);
        }

        else if( strcmp(name, "charset") == 0 )
        {
            options.charset =
                ( _stricmp(value, "latin1") == 0 )    ? Charset::Latin1 :
                ( _stricmp(value, "utf-8") == 0 )     ? Charset::Utf8 :
                ( _stricmp(value, "utf-8-bom") == 0 ) ? Charset::Utf8Bom :
                ( _stricmp(value, "utf-16be") == 0 )  ? Charset::Utf16BE :
                ( _stricmp(value, "utf-16le") == 0 )  ? Charset::Utf16LE :
                                                        throw CSProException(InvalidValueFormatter, name, value);
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
            options.trim_final_newlines = evaluate_bool();
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
