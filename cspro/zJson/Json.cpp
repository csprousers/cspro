#include "stdafx.h"
#include "Json.h"
#include "JsonFileWriter.h"
#include "JsonStreamWriter.h"
#include "JsonStringWriter.h"


// --------------------------------------------------------------------------
// JsonStringWriter creation functions
// --------------------------------------------------------------------------

std::unique_ptr<JsonStringWriter<char>> Json::CreateStringWriter(std::string& text,
    JsonFormattingOptions formatting_options/* = DefaultJsonFormattingOptions*/)
{
    if( formatting_options == JsonFormattingOptions::Compact )
    {
        return std::make_unique<JsonStringWriterImpl<char, jsoncons::compact_json_string_encoder>>(text, formatting_options);
    }

    else
    {
        return std::make_unique<JsonStringWriterImpl<char, jsoncons::json_string_encoder>>(text, formatting_options);
    }
}

std::unique_ptr<JsonStringWriter<wchar_t>> Json::CreateStringWriter(std::wstring& text,
    JsonFormattingOptions formatting_options/* = DefaultJsonFormattingOptions*/)
{
    if( formatting_options == JsonFormattingOptions::Compact )
    {
        return std::make_unique<JsonStringWriterImpl<wchar_t, jsoncons::compact_wjson_string_encoder>>(text, formatting_options);
    }

    else
    {
        return std::make_unique<JsonStringWriterImpl<wchar_t, jsoncons::wjson_string_encoder>>(text, formatting_options);
    }
}


template<> ZJSON_API std::unique_ptr<JsonStringWriter<char>> Json::CreateStringWriter(
    JsonFormattingOptions formatting_options/* = DefaultJsonFormattingOptions*/)
{
    if( formatting_options == JsonFormattingOptions::Compact )
    {
        return std::make_unique<JsonStringWriterOwningTextBufferImpl<char, jsoncons::compact_json_string_encoder>>(formatting_options);
    }

    else
    {
        return std::make_unique<JsonStringWriterOwningTextBufferImpl<char, jsoncons::json_string_encoder>>(formatting_options);
    }
}

template<> ZJSON_API std::unique_ptr<JsonStringWriter<wchar_t>> Json::CreateStringWriter(
    JsonFormattingOptions formatting_options/* = DefaultJsonFormattingOptions*/)
{
    if( formatting_options == JsonFormattingOptions::Compact )
    {
        return std::make_unique<JsonStringWriterOwningTextBufferImpl<wchar_t, jsoncons::compact_wjson_string_encoder>>(formatting_options);
    }

    else
    {
        return std::make_unique<JsonStringWriterOwningTextBufferImpl<wchar_t, jsoncons::wjson_string_encoder>>(formatting_options);
    }
}


std::unique_ptr<JsonStringWriter<wchar_t>> Json::CreateStringWriterWithRelativePaths(std::wstring filename,
    JsonFormattingOptions formatting_options/* = DefaultJsonFileWriterFormattingOptions*/)
{
    if( formatting_options == JsonFormattingOptions::Compact )
    {
        return std::make_unique<JsonStringWriterOwningTextBufferWithRelativePathsImpl<jsoncons::compact_wjson_string_encoder>>(std::move(filename), formatting_options);
    }

    else
    {
        return std::make_unique<JsonStringWriterOwningTextBufferWithRelativePathsImpl<jsoncons::wjson_string_encoder>>(std::move(filename), formatting_options);
    }
}



// --------------------------------------------------------------------------
// JsonStreamWriter creation functions
// --------------------------------------------------------------------------

std::unique_ptr<JsonStreamWriter<std::ostream>> Json::CreateStreamWriter(std::ostream& stream,
    JsonFormattingOptions formatting_options/* = DefaultJsonFormattingOptions*/)
{
    if( formatting_options == JsonFormattingOptions::Compact )
    {
        return std::make_unique<JsonStreamWriterImpl<char, std::ostream, jsoncons::compact_json_stream_encoder>>(stream, formatting_options);
    }

    else
    {
        return std::make_unique<JsonStreamWriterImpl<char, std::ostream, jsoncons::json_stream_encoder>>(stream, formatting_options);
    }
}

std::unique_ptr<JsonStreamWriter<std::wostream>> Json::CreateStreamWriter(std::wostream& stream,
    JsonFormattingOptions formatting_options/* = DefaultJsonFormattingOptions*/)
{
    if( formatting_options == JsonFormattingOptions::Compact )
    {
        return std::make_unique<JsonStreamWriterImpl<wchar_t, std::wostream, jsoncons::compact_wjson_stream_encoder>>(stream, formatting_options);
    }

    else
    {
        return std::make_unique<JsonStreamWriterImpl<wchar_t, std::wostream, jsoncons::wjson_stream_encoder>>(stream, formatting_options);
    }
}



// --------------------------------------------------------------------------
// JsonFileWriter creation functions
// --------------------------------------------------------------------------

std::unique_ptr<JsonFileWriter> Json::CreateFileWriter(NullTerminatedString filename,
    JsonFormattingOptions formatting_options/* = DefaultJsonFileWriterFormattingOptions*/)
{
    if( formatting_options == JsonFormattingOptions::Compact )
    {
        return std::make_unique<JsonFileWriterImpl<jsoncons::compact_json_stream_encoder>>(filename, formatting_options);
    }

    else
    {
        return std::make_unique<JsonFileWriterImpl<jsoncons::json_stream_encoder>>(filename, formatting_options);
    }
}



// --------------------------------------------------------------------------
// JSON parsing
// --------------------------------------------------------------------------

template<typename CharType>
JsonNode<CharType> Json::ParseFile(NullTerminatedString filename)
{
    return JsonNode<CharType>(std::basic_string_view<CharType>(FileIO::ReadText<std::basic_string<CharType>>(filename)));
}

template ZJSON_API JsonNode<char> Json::ParseFile(NullTerminatedString filename);
template ZJSON_API JsonNode<wchar_t> Json::ParseFile(NullTerminatedString filename);
