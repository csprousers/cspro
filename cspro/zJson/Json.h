#pragma once

#include <zJson/zJson.h>
#include <zJson/JsonKeys.h>
#include <zJson/JsonNode.h>
#include <zJson/JsonObjectCreator.h>
#include <zJson/JsonWriter.h>
#include <sstream>


namespace Json
{
    // --------------------------------------------------------------------------
    // JsonWriter creation functions
    // --------------------------------------------------------------------------

    // create a string writer, using the string provided
    ZJSON_API std::unique_ptr<JsonStringWriter<char>> CreateStringWriter(std::string& text,
                                                                         JsonFormattingOptions formatting_options = DefaultJsonFormattingOptions);

    // create a string writer, using the wide-character string provided
    ZJSON_API std::unique_ptr<JsonStringWriter<wchar_t>> CreateStringWriter(std::wstring& text,
                                                                            JsonFormattingOptions formatting_options = DefaultJsonFormattingOptions);

    // create a string writer, using a string owned by the object
    template<typename CharType = wchar_t>
    ZJSON_API std::unique_ptr<JsonStringWriter<CharType>> CreateStringWriter(JsonFormattingOptions formatting_options = DefaultJsonFormattingOptions);

    // create a string writer where relative paths will be written based on the filename, using a string owned by the object;
    // note that the default formatting options, unlike the other string writers, match those of the file writers
    ZJSON_API std::unique_ptr<JsonStringWriter<wchar_t>> CreateStringWriterWithRelativePaths(std::wstring filename,
                                                                                             JsonFormattingOptions formatting_options = DefaultJsonFileWriterFormattingOptions);


    // create a stream writer, using the stream provided
    ZJSON_API std::unique_ptr<JsonStreamWriter<std::ostream>> CreateStreamWriter(std::ostream& stream,
                                                                                 JsonFormattingOptions formatting_options = DefaultJsonFormattingOptions);

    // create a stream writer, using the wide-character stream provided
    ZJSON_API std::unique_ptr<JsonStreamWriter<std::wostream>> CreateStreamWriter(std::wostream& stream,
                                                                                  JsonFormattingOptions formatting_options = DefaultJsonFormattingOptions);


    // create a file (stream) writer, writing to the filename specified:
    // - directories will be created to store the file as needed
    // - if the file exists, it will be overwriten
    // - if there is an error writing to the file, a FileIO::Exception exception will be thrown
    ZJSON_API std::unique_ptr<JsonFileWriter> CreateFileWriter(NullTerminatedString filename,
                                                               JsonFormattingOptions formatting_options = DefaultJsonFileWriterFormattingOptions);



    // --------------------------------------------------------------------------
    // JSON parsing
    // --------------------------------------------------------------------------

    // parse a string:
    // - errors in parsing, or interacting with JSON nodes, will result in JsonParseException exceptions
    // - the text in the string view is only used during the parsing operation
    inline JsonNode<char> Parse(std::string_view json_text, JsonReaderInterface* json_reader_interface = nullptr)
    {
        return JsonNode<char>(json_text, json_reader_interface);
    }

    inline JsonNode<wchar_t> Parse(wstring_view json_text, JsonReaderInterface* json_reader_interface = nullptr)
    {
        return JsonNode<wchar_t>(json_text, json_reader_interface);
    }

    // read and parse the contents of the file:
    // - if there is an error reading the file, a FileIO::Exception exception will be thrown
    // - errors in parsing, or interacting with JSON nodes, will result in JsonParseException exceptions
    template<typename CharType = wchar_t>
    ZJSON_API JsonNode<CharType> ParseFile(NullTerminatedString filename);



    // --------------------------------------------------------------------------
    // CreateObject + CreateObjectString: ways to easily construct a JSON node or
    // the JSON string defining a single object
    // --------------------------------------------------------------------------

    ZJSON_API JsonNode<wchar_t> CreateObject(std::initializer_list<std::tuple<wstring_view, JsonObjectCreatorWrapper>> keys_and_values);

    ZJSON_API std::wstring CreateObjectString(std::initializer_list<std::tuple<wstring_view, JsonObjectCreatorWrapper>> keys_and_values);

    // the JsonObjectCreator class also exists for creating objects
    using ObjectCreator = JsonObjectCreator<wchar_t>;

    // the JsonNodeCreator class also exists for creating nodes representing values (without keys)
    using NodeCreator = JsonNodeCreator<wchar_t>;



    // --------------------------------------------------------------------------
    // Predefined JSON strings
    // --------------------------------------------------------------------------

    namespace Text
    {
        constexpr const TCHAR* Null        = _T("null");
        constexpr const TCHAR* EmptyArray  = _T("[]");
        constexpr const TCHAR* EmptyObject = _T("{}");

        constexpr const TCHAR* Bool(bool value) { return value ? _T("true") : _T("false"); };
    }
}
