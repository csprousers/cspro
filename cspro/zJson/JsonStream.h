#pragma once

#include <zJson/zJson.h>
#include <zJson/JsonKeys.h>
#include <zJson/JsonNode.h>

struct JsonStreamData;
class JsonStreamObjectArrayIterator;
class Utf8InputFileStream;


// --------------------------------------------------
// JsonStream
// --------------------------------------------------

class ZJSON_API JsonStream
{
    // --------------------------------------------------
    // construction
    // --------------------------------------------------

private:
    JsonStream(std::unique_ptr<JsonStreamData> data);

public:
    JsonStream(const JsonStream& rhs) = delete;
    JsonStream(JsonStream&& rhs) noexcept;
    ~JsonStream();

public:
    static JsonStream FromStream(std::unique_ptr<std::wistream> stream);

    static JsonStream FromString(std::wstring text);

    // if there is an error reading the file, a FileIO::Exception exception will be thrown
    static JsonStream FromFile(NullTerminatedString filename);

    // if there is an error reading the file, a FileIO::Exception exception will be thrown
    static JsonStream FromSpecFile(NullTerminatedString filename, const std::function<std::wstring()>& pre_80_spec_file_converter);


    // --------------------------------------------------
    // operations
    // --------------------------------------------------

    // resets the stream to the beginning (when reading from a file) and restarts the stream cursor;
    // on error, the stream is not restarted
    bool RestartStream();

    // reads until the key is found, returning the value associated with the key;
    // the flag dictates whether objects and arrays will be traversed while looking for the key;
    // if the key is not found, a JsonParseException will be thrown
    JsonNode<wchar_t> ReadUntilKey(wstring_view key, bool parse_keys_only_at_this_level = true);

    // creates an iterator over an array of objects; the next event in the stream must be the beginning of an array
    JsonStreamObjectArrayIterator CreateObjectArrayIterator();


    // --------------------------------------------------
    // convenience methods
    // --------------------------------------------------

    // opens a spec file and gets the value located at the root object;
    // - if there is an error reading the file, a FileIO::Exception exception will be thrown;
    // - if the key is not found, a JsonParseException will be thrown
    template<typename ValueType, typename SpecFileType>
    [[nodiscard]] static ValueType GetValueFromSpecFile(wstring_view key, NullTerminatedString filename)
    {
        JsonStream json_stream = FromSpecFile(filename, [&]() { return SpecFileType::ConvertPre80SpecFile(filename); });
        return json_stream.ReadUntilKey(key).Get<ValueType>();
    }


private:
    std::unique_ptr<JsonStreamData> m_data;
};



// --------------------------------------------------
// JsonStreamObjectArrayIterator
// --------------------------------------------------

class ZJSON_API JsonStreamObjectArrayIterator
{
public:
    JsonStreamObjectArrayIterator(JsonStreamData& data);

    // returns the next object in the array
    std::optional<JsonNode<wchar_t>> Next();

    // returns the percent of the stream read (when reading from a file)
    int GetPercentRead() const;

    // following the reading of the objects in the array, returns true
    // if there are no remaining events in the stream
    bool AtEndOfStream() const;

private:
    JsonStreamData& m_data;
    Utf8InputFileStream* m_streamAsUtf8InputFileStream;
};
