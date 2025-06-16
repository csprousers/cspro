#include "stdafx.h"
#include "Json.h"
#include "JsonFileWriter.h"
#include "JsonStreamWriter.h"
#include "JsonStringWriter.h"


// --------------------------------------------------------------------------
// JsonStringWriter creation functions
// --------------------------------------------------------------------------

std::unique_ptr<JsonStringWriter> Json::CreateStringWriter(std::string& text,
                                                           const JsonFormattingOptions formatting_options/* = DefaultJsonFormattingOptions*/)
{
    if( formatting_options == JsonFormattingOptions::Compact )
    {
        return std::make_unique<JsonStringWriterImpl<jsoncons::compact_json_string_encoder>>(text, formatting_options);
    }

    else
    {
        return std::make_unique<JsonStringWriterImpl<jsoncons::json_string_encoder>>(text, formatting_options);
    }
}


std::unique_ptr<JsonStringWriter> Json::CreateStringWriter(const JsonFormattingOptions formatting_options/* = DefaultJsonFormattingOptions*/)
{
    if( formatting_options == JsonFormattingOptions::Compact )
    {
        return std::make_unique<JsonStringWriterOwningTextBufferImpl<jsoncons::compact_json_string_encoder>>(formatting_options);
    }

    else
    {
        return std::make_unique<JsonStringWriterOwningTextBufferImpl<jsoncons::json_string_encoder>>(formatting_options);
    }
}


std::unique_ptr<JsonStringWriter> Json::CreateStringWriterWithRelativePaths(std::string file_path,
                                                                            const JsonFormattingOptions formatting_options/* = DefaultJsonFileWriterFormattingOptions*/)
{
    if( formatting_options == JsonFormattingOptions::Compact )
    {
        return std::make_unique<JsonStringWriterOwningTextBufferWithRelativePathsImpl<jsoncons::compact_json_string_encoder>>(std::move(file_path), formatting_options);
    }

    else
    {
        return std::make_unique<JsonStringWriterOwningTextBufferWithRelativePathsImpl<jsoncons::json_string_encoder>>(std::move(file_path), formatting_options);
    }
}



// --------------------------------------------------------------------------
// JsonStreamWriter creation functions
// --------------------------------------------------------------------------

std::unique_ptr<JsonStreamWriter<std::ostream>> Json::CreateStreamWriter(std::ostream& stream,
                                                                         const JsonFormattingOptions formatting_options/* = DefaultJsonFormattingOptions*/)
{
    if( formatting_options == JsonFormattingOptions::Compact )
    {
        return std::make_unique<JsonStreamWriterImpl<std::ostream, jsoncons::compact_json_stream_encoder>>(stream, formatting_options);
    }

    else
    {
        return std::make_unique<JsonStreamWriterImpl<std::ostream, jsoncons::json_stream_encoder>>(stream, formatting_options);
    }
}



// --------------------------------------------------------------------------
// JsonFileWriter creation functions
// --------------------------------------------------------------------------

std::unique_ptr<JsonFileWriter> Json::CreateFileWriter(const InterfaceString file_path,
                                                       const JsonFormattingOptions formatting_options/* = DefaultJsonFileWriterFormattingOptions*/)
{
    if( formatting_options == JsonFormattingOptions::Compact )
    {
        return std::make_unique<JsonFileWriterImpl<jsoncons::compact_json_stream_encoder>>(file_path.GetString<std::string>(), formatting_options);
    }

    else
    {
        return std::make_unique<JsonFileWriterImpl<jsoncons::json_stream_encoder>>(file_path.GetString<std::string>(), formatting_options);
    }
}



// --------------------------------------------------------------------------
// JSON parsing
// --------------------------------------------------------------------------

JsonNode Json::ParseFile(InterfaceString file_path, JsonReaderInterface* const json_reader_interface/* = nullptr*/)
{
    const std::string json_text = FileIO::ReadText(std::move(file_path));
    return JsonNode(json_text, json_reader_interface);
}
