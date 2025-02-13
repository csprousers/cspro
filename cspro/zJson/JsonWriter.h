#pragma once

#include <zJson/JsonNode.h>
#include <zJson/JsonSerializer.h>
#include <zToolsO/PortableFunctions.h>
#include <zToolsO/SerializerHelper.h>


// --------------------------------------------------------------------------
// JsonWriter
// --------------------------------------------------------------------------

class JsonWriter
{
public:
    JsonWriter()
        :   m_verbose(false)
    {
    }

    virtual ~JsonWriter() { }

    // --------------------------------------------------------------------------
    // key methods
    // --------------------------------------------------------------------------
    virtual JsonWriter& Key(std::string_view key_sv) = 0;
    virtual JsonWriter& Key(wstring_view key_sv) = 0;


    // --------------------------------------------------------------------------
    // object methods
    // --------------------------------------------------------------------------
    virtual JsonWriter& BeginObject() = 0;
    virtual JsonWriter& EndObject() = 0;

    template<typename KeyType>
    JsonWriter& BeginObject(KeyType key)
    {
        return Key(key).BeginObject();
    }


    // --------------------------------------------------------------------------
    // array methods
    // --------------------------------------------------------------------------
    virtual JsonWriter& BeginArray() = 0;
    virtual JsonWriter& EndArray() = 0;

    template<typename KeyType>
    JsonWriter& BeginArray(KeyType key)
    {
        return Key(key).BeginArray();
    }


    // --------------------------------------------------------------------------
    // writing methods
    // --------------------------------------------------------------------------
    virtual JsonWriter& WriteNull() = 0;

    template<typename ValueType>
    JsonWriter& Write(const ValueType& value)
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

    virtual JsonWriter& Write(bool value) = 0;

    virtual JsonWriter& Write(int value) = 0;
    virtual JsonWriter& Write(unsigned int value) = 0;

#ifdef WASM
    JsonWriter& Write(unsigned long value)
    {
        static_assert(sizeof(unsigned long) == sizeof(unsigned int));
        return Write(static_cast<unsigned int>(value));
    }
#endif

    virtual JsonWriter& Write(int64_t value) = 0;
    virtual JsonWriter& Write(uint64_t value) = 0;

    virtual JsonWriter& Write(double value) = 0;

    virtual JsonWriter& Write(std::string_view value_sv) = 0;
    virtual JsonWriter& Write(wstring_view value_sv) = 0;

    virtual JsonWriter& Write(const JsonNode<char>& json_node) = 0;
    virtual JsonWriter& Write(const JsonNode<wchar_t>& json_node) = 0;

    JsonWriter& Write(const char* value)
    {
        // to prevent Write(bool) from being called
        return Write(std::string_view(value));
    }

    JsonWriter& Write(const unsigned char* value)
    {
        // to prevent Write(bool) from being called
        return Write(std::string_view(reinterpret_cast<const char*>(value)));
    }

    JsonWriter& Write(const wchar_t* value)
    {
        // to prevent Write(bool) from being called
        return Write(wstring_view(value));
    }

    JsonWriter& Write(const CString& value)
    {
        // necessary to allow CString writing when used in templates (like writing a vector of CStrings)
        return Write(wstring_view(value));
    }

    JsonWriter& Write(const std::wstring& value)
    {
        // necessary to allow std::wstring writing on Clang
        return Write(wstring_view(value));
    }

    JsonWriter& Write(NullTerminatedStringView value)
    {
        return Write(wstring_view(value));
    }

    template<typename ValueType>
    JsonWriter& WriteVariant(const ValueType& variant_value)
    {
        std::visit([&](const auto& value) { Write(value); }, variant_value);
        return *this;
    }

    template<typename ValueType>
    JsonWriter& WriteEngineValue(const ValueType& value)
    {
        if constexpr(std::is_same_v<ValueType, double>)
        {
            return WriteEngineValueDouble(value);
        }

        else if constexpr(std::is_same_v<ValueType, std::variant<double, std::wstring>>)
        {
            return std::holds_alternative<double>(value) ? WriteEngineValueDouble(std::get<double>(value)) :
                                                           Write(wstring_view((std::get<std::wstring>(value))));
        }

        else
        {
            return Write(wstring_view(value));
        }
    }

    template<typename ValueType>
    JsonWriter& Write(const std::vector<ValueType>& values)
    {
        BeginArray();

        for( const ValueType& value : values )
            Write(value);

        return EndArray();
    }

    template<typename ValueType>
    JsonWriter& Write(const std::set<ValueType>& values)
    {
        BeginArray();

        for( const ValueType& value : values )
            Write(value);

        return EndArray();
    }

    template<typename KeyType>
    JsonWriter& WriteNull(KeyType key)
    {
        return Key(key).WriteNull();
    }

    template<typename KeyType, typename ValueType>
    JsonWriter& Write(KeyType key, const ValueType& value)
    {
        return Key(key).Write(value);
    }

    template<typename KeyType, typename ValueType>
    JsonWriter& WriteVariant(KeyType key, const ValueType& value)
    {
        return Key(key).WriteVariant(value);
    }

    template<typename KeyType, typename ValueType>
    JsonWriter& WriteEngineValue(KeyType key, const ValueType& value)
    {
        return Key(key).WriteEngineValue(value);
    }


    // --------------------------------------------------------------------------
    // writing convenience methods
    // --------------------------------------------------------------------------
    template<typename KeyType, typename ValueType>
    JsonWriter& WriteIfNotBlank(KeyType key, const ValueType& value)
    {
        if( !SO::IsWhitespace(value) )
            Key(key).Write(SO::TrimRight(value));

        return *this;
    }

    template<typename KeyType, typename ValueType>
    JsonWriter& WriteIfHasValue(KeyType key, const std::optional<ValueType>& value)
    {
        if( value.has_value() )
            Key(key).Write(*value);

        return *this;
    }

    template<typename KeyType, typename ValueType>
    JsonWriter& WriteIfNot(KeyType key, const ValueType& value, const ValueType& default_value)
    {
        if( value != default_value )
            Key(key).Write(value);

        return *this;
    }

    template<typename KeyType, typename ValueType>
    JsonWriter& WriteIfNotEmpty(KeyType key, const std::vector<ValueType>& values)
    {
        if( !values.empty() )
            Key(key).Write(values);

        return *this;
    }

    // writes the date in RFC 3339 format
    template<typename KeyType>
    JsonWriter& WriteDate(KeyType key, time_t date)
    {
        return Key(key).Write(PortableFunctions::TimeToRFC3339String(date));
    }


    // writes a path with forward slashes
    JsonWriter& WritePath(std::wstring path)
    {
        return Write(PortableFunctions::PathToForwardSlash(std::move(path)));
    }

    template<typename KeyType>
    JsonWriter& WritePath(KeyType key, std::wstring path)
    {
        return Key(key).WritePath(std::move(path));
    }

    // writes a relative path with forward slashes, evaluated relative to the spec file (if available)
    JsonWriter& WriteRelativePath(const std::wstring& path)
    {
        return WritePath(GetRelativePath(path));
    }

    JsonWriter& WriteRelativePathWithDirectorySupport(const std::wstring& path)
    {
        // the relative path calculation functions don't work properly with a directory if it does not end in a slash
        if( !path.empty() && path.back() != PATH_CHAR && PortableFunctions::FileIsDirectory(path) )
        {
            std::wstring modified_path = GetRelativePath(PortableFunctions::PathEnsureTrailingSlash(path));

            if( modified_path.empty() )
                return Write(_T("."));

            ASSERT(modified_path.back() == PATH_CHAR);
            return WritePath(modified_path.substr(0, modified_path.length() - 1));
        }

        else
        {
            return WriteRelativePath(path);
        }
    }

    template<typename KeyType>
    JsonWriter& WriteRelativePath(KeyType key, const std::wstring& path)
    {
        return Key(key).WriteRelativePath(path);
    }

    template<typename KeyType>
    JsonWriter& WriteRelativePathWithDirectorySupport(KeyType key, const std::wstring& path)
    {
        return Key(key).WriteRelativePathWithDirectorySupport(path);
    }


    // --------------------------------------------------------------------------
    // writing methods using callbacks
    // --------------------------------------------------------------------------
    template<typename WriterCallback>
    JsonWriter& WriteObject(WriterCallback writer_callback)
    {
        BeginObject();
        writer_callback();
        return EndObject();
    }

    template<typename ObjectType, typename WriterCallback>
    JsonWriter& WriteObject(const ObjectType& value, WriterCallback writer_callback)
    {
        BeginObject();
        writer_callback(value);
        return EndObject();
    }

    template<typename KeyType, typename ObjectType, typename WriterCallback>
    JsonWriter& WriteObject(KeyType key, const ObjectType& value, WriterCallback writer_callback)
    {
        return Key(key).WriteObject(value, writer_callback);
    }

    template<typename KeyType, typename ObjectType, typename WriterCallback>
    JsonWriter& WriteObjects(KeyType key, const std::vector<ObjectType>& values, WriterCallback writer_callback)
    {
        BeginArray(key);

        for( const ObjectType& value : values )
            WriteObject(value, writer_callback);

        return EndArray();
    }

    template<typename KeyType, typename ValueType, typename WriterCallback>
    JsonWriter& WriteArray(KeyType key, const std::vector<ValueType>& values, WriterCallback writer_callback)
    {
        BeginArray(key);

        for( const ValueType& value : values )
            writer_callback(value);

        return EndArray();
    }

    template<typename KeyType, typename ValueType, typename WriterCallback>
    JsonWriter& WriteArrayIfNotEmpty(KeyType&& key, const std::vector<ValueType>& values, WriterCallback&& writer_callback)
    {
        return values.empty() ? *this :
                                WriteArray(std::forward<KeyType>(key), values, std::forward<WriterCallback>(writer_callback));
    }


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
    virtual JsonWriter& WriteEngineValueDouble(double value) = 0;

    virtual std::wstring GetRelativePath(const std::wstring& path) const { return path; }

private:
    virtual void RemoveTopmostFormattingType() = 0;

private:
    SerializerHelper m_serializerHelper;
    bool m_verbose;
};



// --------------------------------------------------------------------------
// JsonStringWriter
// --------------------------------------------------------------------------

template<typename CharType>
class JsonStringWriter : virtual public JsonWriter
{
public:
    virtual const std::basic_string<CharType>& GetString() = 0;

    const CharType* c_str()
    {
        return GetString().c_str();
    }
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
    FormattingHolder(JsonWriter* json_writer)
        :   m_jsonWriter(json_writer)
    {
    }

    FormattingHolder(const FormattingHolder& rhs) = delete;

    FormattingHolder(FormattingHolder&& rhs) noexcept
        :   m_jsonWriter(rhs.m_jsonWriter)
    {
        rhs.m_jsonWriter = nullptr;
    }

    ~FormattingHolder()
    {
        if( m_jsonWriter != nullptr )
            m_jsonWriter->RemoveTopmostFormattingType();
    }

private:
    JsonWriter* m_jsonWriter;
};
