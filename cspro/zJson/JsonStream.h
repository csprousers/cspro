#pragma once

#include <zJson/zJson.h>
#include <zJson/JsonKeys.h>
#include <zJson/JsonNode.h>

struct JsonStreamData;
class JsonStreamObjectArrayIterator;


// --------------------------------------------------------------------------
// JsonStream
// --------------------------------------------------------------------------

class ZJSON_API JsonStream
{
    // --------------------------------------------------------------------------
    // construction
    // --------------------------------------------------------------------------

private:
    JsonStream(std::unique_ptr<JsonStreamData> data);

public:
    JsonStream(const JsonStream& rhs) = delete;
    JsonStream(JsonStream&& rhs) noexcept;
    ~JsonStream();

public:
    static JsonStream FromStream(std::unique_ptr<std::istream> stream, std::optional<std::streampos> stream_size = std::nullopt);

    static JsonStream FromString(const std::string& text);
    static JsonStream FromString(std::string&& text);

    // If there is an error reading the file, a FileIO::Exception exception will be thrown.
    static JsonStream FromFile(InterfaceString file_path);

    // If there is an error reading the file, a FileIO::Exception exception will be thrown.
    static JsonStream FromSpecFile(InterfaceString file_path, const std::function<std::string()>& pre_80_spec_file_converter);


    // --------------------------------------------------------------------------
    // operations
    // --------------------------------------------------------------------------

    // Resets the stream to the beginning (when reading from a file) and restarts the stream cursor.
    // On error, the stream is not restarted.
    bool RestartStream();

    // Reads until the key is found, returning the value associated with the key.
    // The flag dictates whether objects and arrays will be traversed while looking for the key.
    // If the key is not found, a JsonParseException will be thrown.
    JsonNode ReadUntilKey(std::string_view key_sv, bool parse_keys_only_at_this_level = true);

    // Creates an iterator over an array of objects.
    // The next event in the stream must be the beginning of an array.
    // The class IncrementalJsonObjectArrayParser provides another way to iterator over an array of objects.
    JsonStreamObjectArrayIterator CreateObjectArrayIterator();


    // --------------------------------------------------------------------------
    // convenience methods
    // --------------------------------------------------------------------------

    // Opens a spec file and gets the value located at the root object.
    // - If there is an error reading the file, a FileIO::Exception exception will be thrown.
    // - If the key is not found, a JsonParseException will be thrown.
    template<typename ValueType, typename SpecFileType>
    [[nodiscard]] static ValueType GetValueFromSpecFile(const std::string_view key_sv, const InterfaceString file_path)
    {
        JsonStream json_stream = FromSpecFile(file_path, [&]() { return SpecFileType::ConvertPre80SpecFile(file_path); });
        return json_stream.ReadUntilKey(key_sv).Get<ValueType>();
    }


private:
    std::unique_ptr<JsonStreamData> m_data;
};



// --------------------------------------------------------------------------
// JsonStreamObjectArrayIterator
// --------------------------------------------------------------------------

class ZJSON_API JsonStreamObjectArrayIterator
{
public:
    JsonStreamObjectArrayIterator(JsonStreamData& data);

    // returns the next object in the array
    std::optional<JsonNode> Next();

    // returns the percent of the stream read (when reading from a file)
    int GetPercentRead() const;

    // following the reading of the objects in the array, returns true
    // if there are no remaining events in the stream
    bool AtEndOfStream() const;

private:
    JsonStreamData& m_data;
};
