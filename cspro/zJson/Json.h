#pragma once

#include <zJson/zJson.h>
#include <zJson/JsonKeys.h>
#include <zJson/JsonNode.h>
#include <zJson/JsonObjectCreator.h>
#include <zJson/JsonWriter.h>
#include <iosfwd>


namespace Json
{
    // --------------------------------------------------------------------------
    // JsonWriter creation functions
    // --------------------------------------------------------------------------

    // Create a string writer, using the string provided.
    ZJSON_API std::unique_ptr<JsonStringWriter> CreateStringWriter(std::string& text,
                                                                   JsonFormattingOptions formatting_options = DefaultJsonFormattingOptions);

    // Create a string writer, using a string owned by the object.
    ZJSON_API std::unique_ptr<JsonStringWriter> CreateStringWriter(JsonFormattingOptions formatting_options = DefaultJsonFormattingOptions);

    // Create a string writer where relative paths will be written based on the file path, using a string owned by the object.
    // Note that the default formatting options, unlike the other string writers, match those of the file writers
    ZJSON_API std::unique_ptr<JsonStringWriter> CreateStringWriterWithRelativePaths(std::string file_path,
                                                                                    JsonFormattingOptions formatting_options = DefaultJsonFileWriterFormattingOptions);


    // Create a stream writer, using the stream provided.
    ZJSON_API std::unique_ptr<JsonStreamWriter<std::ostream>> CreateStreamWriter(std::ostream& stream,
                                                                                 JsonFormattingOptions formatting_options = DefaultJsonFormattingOptions);

    // Create a file (stream) writer, writing to the file path specified:
    // - Directories will be created to store the file as needed.
    // - If the file exists, it will be overwriten.
    // - If there is an error writing to the file, a FileIO::Exception exception will be thrown.
    ZJSON_API std::unique_ptr<JsonFileWriter> CreateFileWriter(InterfaceString file_path,
                                                               JsonFormattingOptions formatting_options = DefaultJsonFileWriterFormattingOptions);



    // --------------------------------------------------------------------------
    // JSON parsing
    // --------------------------------------------------------------------------

    // Parse a string:
    // - Errors in parsing, or interacting with JSON nodes, will result in JsonParseException exceptions.
    // - The text in the string view is only used during the parsing operation.
    JsonNode Parse(std::string_view json_text_sv, JsonReaderInterface* json_reader_interface = nullptr);

    // Read and parse the contents of the file:
    // - If there is an error reading the file, a FileIO::Exception exception will be thrown.
    // - Errors in parsing, or interacting with JSON nodes, will result in JsonParseException exceptions/
    ZJSON_API JsonNode ParseFile(InterfaceString file_path);



    // --------------------------------------------------------------------------
    // CreateObject + CreateObjectString: ways to easily construct a JSON node or
    // the JSON string defining a single object; when creating a string, it will
    // be created using compact JSON (on a single line without line breaks)
    // --------------------------------------------------------------------------

    ZJSON_API JsonNode CreateObject(std::initializer_list<std::tuple<std::string_view, JsonObjectCreatorWrapper>> keys_and_values);

    ZJSON_API std::string CreateObjectString(std::initializer_list<std::tuple<std::string_view, JsonObjectCreatorWrapper>> keys_and_values);

    // the JsonObjectCreator class also exists for creating objects
    using ObjectCreator = JsonObjectCreator;

    // the JsonNodeCreator class also exists for creating nodes representing values (without keys)
    using NodeCreator = JsonNodeCreator;



    // --------------------------------------------------------------------------
    // ToJson + FromJson: ways to easily get the JSON string for a single value,
    // or to construct a value from a JSON string
    // --------------------------------------------------------------------------

    template<typename T>
    std::string ToJson(const T& value, JsonFormattingOptions formatting_options = DefaultJsonFormattingOptions);

    template<typename T>
    T FromJson(std::string_view json_text_sv);



    // --------------------------------------------------------------------------
    // Predefined JSON strings
    // --------------------------------------------------------------------------

    namespace Text
    {
        constexpr std::string_view Null_sv        = "null";
        constexpr std::string_view EmptyArray_sv  = "[]";
        constexpr std::string_view EmptyObject_sv = "{}";
        constexpr std::string_view BlankString_sv = "\"\"";

        constexpr const char* Bool(bool value) { return value ? "true" : "false"; };
    }
}



// --------------------------------------------------------------------------
// inline implementations
// --------------------------------------------------------------------------

inline JsonNode Json::Parse(const std::string_view json_text_sv, JsonReaderInterface* const json_reader_interface/* = nullptr*/)
{
    return JsonNode(json_text_sv, json_reader_interface);
}


template<typename T>
std::string Json::ToJson(const T& value, const JsonFormattingOptions formatting_options/* = DefaultJsonFormattingOptions*/)
{
    const std::unique_ptr<JsonStringWriter> json_writer = Json::CreateStringWriter(formatting_options);
    json_writer->Write(value);
    return json_writer->ReleaseString();
}


template<>
inline std::string Json::ToJson(const bool& value, const JsonFormattingOptions /*formatting_options = DefaultJsonFormattingOptions*/)
{
    return Json::Text::Bool(value);
}


template<typename T>
T Json::FromJson(const std::string_view json_text_sv)
{
    const JsonNode json_node = Json::Parse(json_text_sv);
    return json_node.Get<T>();
}
