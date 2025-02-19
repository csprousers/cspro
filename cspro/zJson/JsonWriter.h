#pragma once

#include <zJson/zJson.h>
#include <zJson/JsonNode.h>
#include <zJson/JsonSerializer.h>
#include <zToolsO/SerializerHelper.h>


// --------------------------------------------------------------------------
// JsonWriter
// --------------------------------------------------------------------------

class JsonWriter
{
public:
    JsonWriter();
    virtual ~JsonWriter() { }

    // --------------------------------------------------------------------------
    // key methods
    // --------------------------------------------------------------------------
    virtual JsonWriter& Key(std::string_view key_sv) = 0;


    // --------------------------------------------------------------------------
    // object methods
    // --------------------------------------------------------------------------
    virtual JsonWriter& BeginObject() = 0;
    virtual JsonWriter& EndObject() = 0;

    JsonWriter& BeginObject(std::string_view key_sv);


    // --------------------------------------------------------------------------
    // array methods
    // --------------------------------------------------------------------------
    virtual JsonWriter& BeginArray() = 0;
    virtual JsonWriter& EndArray() = 0;

    JsonWriter& BeginArray(std::string_view key_sv);


    // --------------------------------------------------------------------------
    // writing methods
    // --------------------------------------------------------------------------
    virtual JsonWriter& WriteNull() = 0;

    template<typename ValueType>
    JsonWriter& Write(const ValueType& value);

    virtual JsonWriter& Write(bool value) = 0;

    virtual JsonWriter& Write(int value) = 0;
    virtual JsonWriter& Write(unsigned int value) = 0;

#ifdef WASM
    JsonWriter& Write(unsigned long value);
#endif

    virtual JsonWriter& Write(int64_t value) = 0;
    virtual JsonWriter& Write(uint64_t value) = 0;

    virtual JsonWriter& Write(double value) = 0;

    virtual JsonWriter& Write(std::string_view value_sv) = 0;
    JsonWriter& Write(const std::string& value);
    JsonWriter& Write(const SharableString& value);
    JsonWriter& Write(const char* value);
    JsonWriter& Write(const unsigned char* value);
    JsonWriter& Write(cs::string_sz value);

    ZJSON_API JsonWriter& Write(const std::wstring& value);
    ZJSON_API JsonWriter& Write(const CString& value);

    virtual JsonWriter& Write(const JsonNode& json_node) = 0;

    template<typename ValueType>
    JsonWriter& WriteVariant(const ValueType& variant_value);

    ZJSON_API JsonWriter& WriteEngineValue(double value);
    JsonWriter& WriteEngineValue(const std::string& value);
    JsonWriter& WriteEngineValue(const SharableString& value);
    JsonWriter& WriteEngineValue(const std::variant<double, std::string>& value);
    JsonWriter& WriteEngineValue(const std::variant<double, SharableString>& value);
    ZJSON_API JsonWriter& WriteEngineValue(const std::wstring& value);
    ZJSON_API JsonWriter& WriteEngineValue(const std::variant<double, std::wstring>& value);

    template<typename ValueType>
    JsonWriter& Write(const std::vector<ValueType>& values);

    template<typename ValueType>
    JsonWriter& Write(const std::set<ValueType>& values);

    JsonWriter& WriteNull(std::string_view key_sv);

    template<typename ValueType>
    JsonWriter& Write(std::string_view key_sv, const ValueType& value);

    template<typename ValueType>
    JsonWriter& WriteVariant(std::string_view key_sv, const ValueType& value);

    template<typename ValueType>
    JsonWriter& WriteEngineValue(std::string_view key_sv, const ValueType& value);


    // --------------------------------------------------------------------------
    // writing convenience methods
    // --------------------------------------------------------------------------
    JsonWriter& WriteIfNotBlank(std::string_view key_sv, std::string_view value_sv);

    template<typename ValueType>
    JsonWriter& WriteIfHasValue(std::string_view key_sv, const std::optional<ValueType>& value);

    JsonWriter& WriteIfHasValue(std::string_view key_sv, const SharableString& value);

    template<typename ValueType>
    JsonWriter& WriteIfNot(std::string_view key_sv, const ValueType& value, const ValueType& default_value);

    JsonWriter& WriteIfNotEmpty(std::string_view key_sv, const JsonNode& json_node);

    template<typename ValueType>
    JsonWriter& WriteIfNotEmpty(std::string_view key_sv, const std::vector<ValueType>& values);

    template<typename MapKeyType, typename MapValueType>
    JsonWriter& WriteMap(std::string_view key_sv, const std::map<MapKeyType, MapValueType>& values);

    // writes the date in RFC 3339 format
    ZJSON_API JsonWriter& WriteDate(std::string_view key_sv, int64_t date);

    // writes a path with forward slashes
    ZJSON_API JsonWriter& WritePath(std::string path);
    JsonWriter& WritePath(std::string_view key_sv, std::string path);

    // writes a relative path with forward slashes, evaluated relative to the spec file (if available)
    ZJSON_API JsonWriter& WriteRelativePath(const std::string& path);
    JsonWriter& WriteRelativePath(std::string_view key_sv, const std::string& path);

    ZJSON_API JsonWriter& WriteRelativePathWithDirectorySupport(const std::string& path);
    JsonWriter& WriteRelativePathWithDirectorySupport(std::string_view key_sv, const std::string& path);


    // --------------------------------------------------------------------------
    // writing methods using callbacks
    // --------------------------------------------------------------------------
    template<typename WriterCallback>
    JsonWriter& WriteObject(const WriterCallback& writer_callback);

    template<typename ObjectType, typename WriterCallback>
    JsonWriter& WriteObject(const ObjectType& value, const WriterCallback& writer_callback);

    template<typename ObjectType, typename WriterCallback>
    JsonWriter& WriteObject(std::string_view key_sv, const ObjectType& value, const WriterCallback& writer_callback);

    template<typename ObjectType, typename WriterCallback>
    JsonWriter& WriteObjects(const std::vector<ObjectType>& values, const WriterCallback& writer_callback);

    template<typename ObjectType, typename WriterCallback>
    JsonWriter& WriteObjects(std::string_view key_sv, const std::vector<ObjectType>& values, const WriterCallback& writer_callback);

    template<typename ValueType, typename WriterCallback>
    JsonWriter& WriteArray(std::string_view key_sv, const std::vector<ValueType>& values, const WriterCallback& writer_callback);

    template<typename ValueType, typename WriterCallback>
    JsonWriter& WriteArrayIfNotEmpty(std::string_view key_sv, const std::vector<ValueType>& values, const WriterCallback& writer_callback);


    // --------------------------------------------------------------------------
    // formatting methods (described in JsonFormattingOptions.h)
    // --------------------------------------------------------------------------
    class FormattingHolder;
    virtual FormattingHolder SetFormattingType(JsonFormattingType formatting_type) = 0;
    virtual void SetFormattingAction(JsonFormattingAction formatting_action) = 0;


    // --------------------------------------------------------------------------
    // other methods
    // --------------------------------------------------------------------------
    SerializerHelper& GetSerializerHelper() { return m_serializerHelper; }

    bool Verbose() const { return m_verbose; }
    void SetVerbose()    { m_verbose = true; }


protected:
    virtual std::string GetRelativePath(const std::string& path) const { return path; }

private:
    virtual void RemoveTopmostFormattingType() = 0;

private:
    SerializerHelper m_serializerHelper;
    bool m_verbose;
};



// --------------------------------------------------------------------------
// JsonStringWriter
// --------------------------------------------------------------------------

class JsonStringWriter : virtual public JsonWriter
{
public:
    virtual const std::string& GetString() const = 0;
    virtual std::string ReleaseString() = 0;
    virtual SharableString ReleaseSharableString() = 0;
};



// --------------------------------------------------------------------------
// JsonStreamWriter
// --------------------------------------------------------------------------

template<typename StreamType>
class JsonStreamWriter : virtual public JsonWriter
{
public:
    virtual StreamType& GetStream() = 0;

    virtual void Flush() = 0;
};



// --------------------------------------------------------------------------
// JsonFileWriter
// --------------------------------------------------------------------------

class JsonFileWriter : virtual public JsonWriter
{
public:
    virtual void Close() = 0;
};



// --------------------------------------------------------------------------
// JsonWriter::FormattingHolder
// --------------------------------------------------------------------------

class JsonWriter::FormattingHolder
{
public:
    FormattingHolder(JsonWriter* json_writer);
    FormattingHolder(const FormattingHolder& rhs) = delete;
    FormattingHolder(FormattingHolder&& rhs) noexcept;
    ~FormattingHolder();

private:
    JsonWriter* m_jsonWriter;
};



// --------------------------------------------------------------------------
// JsonWriter inline implementations
// --------------------------------------------------------------------------

inline JsonWriter::JsonWriter()
    :   m_verbose(false)
{
}


inline JsonWriter& JsonWriter::BeginObject(const std::string_view key_sv)
{
    return Key(key_sv).BeginObject();
}


inline JsonWriter& JsonWriter::BeginArray(const std::string_view key_sv)
{
    return Key(key_sv).BeginArray();
}


template<typename ValueType>
JsonWriter& JsonWriter::Write(const ValueType& value)
{
    if constexpr(JsonSerializerTester<ValueType>::HasWriteJson())
    {
        value.WriteJson(*this);
    }

    else if constexpr(JsonSerializerTester<JsonSerializer<ValueType>>::HasWriteJson())
    {
        JsonSerializer<ValueType>::WriteJson(*this, value);
    }

    else
    {
#if defined(WIN32) && !defined(_CONSOLE)
        // this fails on Clang
        static_assert(false, "create a JsonSerializer for ValueType");
#else
        static_assert_false();
#endif
    }

    return *this;
}


#ifdef WASM
inline JsonWriter& JsonWriter::Write(const unsigned long value)
{
    static_assert(sizeof(unsigned long) == sizeof(unsigned int));
    return Write(static_cast<unsigned int>(value));
}
#endif


inline JsonWriter& JsonWriter::Write(const std::string& value)
{
    // necessary to allow std::string writing on Clang
    return Write(std::string_view(value));
}


inline JsonWriter& JsonWriter::Write(const SharableString& value)
{
    return Write(std::string_view(*value));
}


inline JsonWriter& JsonWriter::Write(const char* const value)
{
    // to prevent Write(bool) from being called
    return Write(std::string_view(value));
}


inline JsonWriter& JsonWriter::Write(const unsigned char* const value)
{
    // to prevent Write(bool) from being called
    return Write(std::string_view(reinterpret_cast<const char*>(value)));
}


inline JsonWriter& JsonWriter::Write(const cs::string_sz value)
{
    return Write(value.c_str());
}


template<typename ValueType>
JsonWriter& JsonWriter::WriteVariant(const ValueType& variant_value)
{
    std::visit([&](const auto& value) { Write(value); }, variant_value);
    return *this;
}


inline JsonWriter& JsonWriter::WriteEngineValue(const std::string& value)
{
    return Write(value);
}


inline JsonWriter& JsonWriter::WriteEngineValue(const SharableString& value)
{
    return Write(value);
}


inline JsonWriter& JsonWriter::WriteEngineValue(const std::variant<double, std::string>& value)
{
    return std::holds_alternative<double>(value) ? WriteEngineValue(std::get<double>(value)) :
                                                   Write(std::get<std::string>(value));
}


inline JsonWriter& JsonWriter::WriteEngineValue(const std::variant<double, SharableString>& value)
{
    return std::holds_alternative<double>(value) ? WriteEngineValue(std::get<double>(value)) :
                                                   Write(std::get<SharableString>(value));
}


template<typename ValueType>
JsonWriter& JsonWriter::Write(const std::vector<ValueType>& values)
{
    BeginArray();

    for( const ValueType& value : values )
        Write(value);

    return EndArray();
}


template<typename ValueType>
JsonWriter& JsonWriter::Write(const std::set<ValueType>& values)
{
    BeginArray();

    for( const ValueType& value : values )
        Write(value);

    return EndArray();
}


inline JsonWriter& JsonWriter::WriteNull(const std::string_view key_sv)
{
    return Key(key_sv).WriteNull();
}


template<typename ValueType>
JsonWriter& JsonWriter::Write(const std::string_view key_sv, const ValueType& value)
{
    return Key(key_sv).Write(value);
}


template<typename ValueType>
JsonWriter& JsonWriter::WriteVariant(const std::string_view key_sv, const ValueType& value)
{
    return Key(key_sv).WriteVariant(value);
}


template<typename ValueType>
JsonWriter& JsonWriter::WriteEngineValue(const std::string_view key_sv, const ValueType& value)
{
    return Key(key_sv).WriteEngineValue(value);
}


inline JsonWriter& JsonWriter::WriteIfNotBlank(const std::string_view key_sv, const std::string_view value_sv)
{
    if( !SO::IsWhitespace(value_sv) )
        Key(key_sv).Write(SO::TrimRight(value_sv));

    return *this;
}


template<typename MapKeyType, typename MapValueType>
JsonWriter& JsonWriter::WriteMap(const std::string_view key_sv, const std::map<MapKeyType, MapValueType>& values)
{
    BeginObject(key_sv);

    for( const auto& [key, value] : values )
    {
        if constexpr(std::is_same_v<MapKeyType, int>)
        {
            Key(IntToString(key));
        }

        else
        {
            Key(key);
        }

        Write(value);
    }

    return EndObject();
}


template<typename ValueType>
JsonWriter& JsonWriter::WriteIfHasValue(const std::string_view key_sv, const std::optional<ValueType>& value)
{
    if( value.has_value() )
        Key(key_sv).Write(*value);

    return *this;
}


inline JsonWriter& JsonWriter::WriteIfHasValue(const std::string_view key_sv, const SharableString& value)
{
    if( value.IsSet() )
        Key(key_sv).Write(value);

    return *this;
}


template<typename ValueType>
JsonWriter& JsonWriter::WriteIfNot(const std::string_view key_sv, const ValueType& value, const ValueType& default_value)
{
    if( value != default_value )
        Key(key_sv).Write(value);

    return *this;
}


inline JsonWriter& JsonWriter::WriteIfNotEmpty(const std::string_view key_sv, const JsonNode& json_node)
{
    if( !json_node.IsEmpty() )
        Key(key_sv).Write(json_node);

    return *this;
}


template<typename ValueType>
JsonWriter& JsonWriter::WriteIfNotEmpty(const std::string_view key_sv, const std::vector<ValueType>& values)
{
    if( !values.empty() )
        Key(key_sv).Write(values);

    return *this;
}


inline JsonWriter& JsonWriter::WritePath(const std::string_view key_sv, std::string path)
{
    return Key(key_sv).WritePath(std::move(path));
}


inline JsonWriter& JsonWriter::WriteRelativePath(const std::string_view key_sv, const std::string& path)
{
    return Key(key_sv).WriteRelativePath(path);
}


inline JsonWriter& JsonWriter::WriteRelativePathWithDirectorySupport(const std::string_view key_sv, const std::string& path)
{
    return Key(key_sv).WriteRelativePathWithDirectorySupport(path);
}


template<typename WriterCallback>
JsonWriter& JsonWriter::WriteObject(const WriterCallback& writer_callback)
{
    BeginObject();
    writer_callback();
    return EndObject();
}


template<typename ObjectType, typename WriterCallback>
JsonWriter& JsonWriter::WriteObject(const ObjectType& value, const WriterCallback& writer_callback)
{
    BeginObject();
    writer_callback(value);
    return EndObject();
}


template<typename ObjectType, typename WriterCallback>
JsonWriter& JsonWriter::WriteObject(const std::string_view key_sv, const ObjectType& value, const WriterCallback& writer_callback)
{
    return Key(key_sv).WriteObject(value, writer_callback);
}


template<typename ObjectType, typename WriterCallback>
JsonWriter& JsonWriter::WriteObjects(const std::vector<ObjectType>& values, const WriterCallback& writer_callback)
{
    BeginArray();

    for( const ObjectType& value : values )
        WriteObject(value, writer_callback);

    return EndArray();
}


template<typename ObjectType, typename WriterCallback>
JsonWriter& JsonWriter::WriteObjects(const std::string_view key_sv, const std::vector<ObjectType>& values, const WriterCallback& writer_callback)
{
    return Key(key_sv).WriteObjects(values, writer_callback);
}


template<typename ValueType, typename WriterCallback>
JsonWriter& JsonWriter::WriteArray(const std::string_view key_sv, const std::vector<ValueType>& values, const WriterCallback& writer_callback)
{
    BeginArray(key_sv);

    for( const ValueType& value : values )
        writer_callback(value);

    return EndArray();
}


template<typename ValueType, typename WriterCallback>
JsonWriter& JsonWriter::WriteArrayIfNotEmpty(const std::string_view key_sv, const std::vector<ValueType>& values, const WriterCallback& writer_callback)
{
    return values.empty() ? *this :
                            WriteArray(key_sv, values, writer_callback);
}



// --------------------------------------------------------------------------
// JsonWriter::FormattingHolder inline implementations
// --------------------------------------------------------------------------

inline JsonWriter::FormattingHolder::FormattingHolder(JsonWriter* const json_writer)
    :   m_jsonWriter(json_writer)
{
}


inline JsonWriter::FormattingHolder::FormattingHolder(FormattingHolder&& rhs) noexcept
    :   m_jsonWriter(rhs.m_jsonWriter)
{
    rhs.m_jsonWriter = nullptr;
}


inline JsonWriter::FormattingHolder::~FormattingHolder()
{
    if( m_jsonWriter != nullptr )
        m_jsonWriter->RemoveTopmostFormattingType();
}
