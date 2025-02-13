#pragma once

#include <zJson/zJson.h>
#include <zJson/JsonFormattingOptions.h>
#include <zJson/JsonSerializer.h>
#include <zToolsO/SerializerHelper.h>

namespace jsoncons
{
    template<class CharT, class ImplementationPolicy, class Allocator> class basic_json;
    class json_exception;
    template<class Json, template<typename, typename> class SequenceContainer> class json_array;
    struct order_preserving_policy;
}

template<typename CharType> class JsonNodeArray;
template<typename CharType> class JsonNodeArrayIterator;



// --------------------------------------------------------------------------
// JsonParseException
// --------------------------------------------------------------------------

class JsonParseException : public CSProException
{
public:
    using CSProException::CSProException;

    JsonParseException(int line_number, const std::string& message)
        :   CSProException(message.c_str()),
            m_lineNumber(line_number)
    {
        // json_exception exceptions will be rethrown as JsonParseException
        // exceptions with the line number of the parse error
    }

    int GetLineNumber() const { return m_lineNumber; }

private:
    const int m_lineNumber = -1;
};



// --------------------------------------------------------------------------
// JsonReaderInterface
// --------------------------------------------------------------------------

class JsonReaderInterface
{
public:
    JsonReaderInterface(std::wstring directory = std::wstring())
        :   m_directory(std::move(directory))
    {
    }

    virtual ~JsonReaderInterface() { }

    const std::wstring& GetDirectory() { return m_directory; }

    SerializerHelper& OnGetSerializerHelper() const { return const_cast<SerializerHelper&>(m_serializerHelper); }

    virtual void OnLogWarning(std::wstring /*message*/) { }

    virtual void OnReportInvalidAccessUsingKey(const TCHAR* key, const TCHAR* node_text,
                                               const JsonParseException* exception_to_be_thrown)
    {
        if( exception_to_be_thrown == nullptr )
        {
            OnLogWarning(FormatTextCS2WS(_T("The value of '%s' was ignored because it contained an invalid entry: %s"), key, node_text));
        }

        else
        {
            OnLogWarning(FormatTextCS2WS(_T("The value of '%s' was invalid and resulted in an error ('%s'): %s"),
                                         key, exception_to_be_thrown->GetErrorMessage().c_str(), node_text));
        }
    }

protected:
    std::wstring m_directory;

private:
    SerializerHelper m_serializerHelper;
};



// --------------------------------------------------------------------------
// JsonNode
// --------------------------------------------------------------------------

template<typename CharType>
class ZJSON_API JsonNode
{
    friend JsonNodeArray<CharType>;

protected:
    using BasicJson = jsoncons::basic_json<CharType, jsoncons::order_preserving_policy, std::allocator<char>>;
    using StringView = typename std::conditional_t<std::is_same_v<CharType, char>, std::string_view, wstring_view>;

    // --------------------------------------------------------------------------
    // construction
    // --------------------------------------------------------------------------

private:
    // constructs a node from a jsoncons operation
    JsonNode(const BasicJson* json, JsonReaderInterface& json_reader_interface);

public:
    // constructs a node from a jsoncons operation, assuming ownership of the jsoncons object
    JsonNode(std::shared_ptr<const BasicJson> json, JsonReaderInterface* json_reader_interface = nullptr);

    // constructs a node by parsing text;
    // on error, throws JsonParseException
    JsonNode(std::basic_string_view<CharType> json_text, JsonReaderInterface* json_reader_interface = nullptr);

    virtual ~JsonNode();


    // --------------------------------------------------------------------------
    // operations
    // --------------------------------------------------------------------------

    // returns a string representation of the node
    [[nodiscard]] std::basic_string<CharType> GetNodeAsString(JsonFormattingOptions formatting_options = DefaultJsonFormattingOptions) const;

    // returns whether or not the node is empty
    [[nodiscard]] bool IsEmpty() const;

    // returns whether or not the node is a null value
    [[nodiscard]] bool IsNull() const;

    // returns whether or not the node is a boolean
    [[nodiscard]] bool IsBoolean() const;

    // returns whether or not the node is a number
    [[nodiscard]] bool IsNumber() const;

    // returns whether or not the node is a double
    [[nodiscard]] bool IsDouble() const;

    // returns whether or not the node is a string
    [[nodiscard]] bool IsString() const;

    // returns whether or not the node is an array
    [[nodiscard]] bool IsArray() const;

    // returns whether or not the node is an object
    [[nodiscard]] bool IsObject() const;

    // indicates whether the node contains a property with the given key name
    [[nodiscard]] bool Contains(StringView key_sv) const;

    // returns the property with the given key name;
    // on error, throws JsonParseException
    [[nodiscard]] JsonNode<CharType> operator[](StringView key_sv) const;

    // returns the node with the given key name;
    // if it does not exist, an empty node is returned
    [[nodiscard]] JsonNode<CharType> GetOrEmpty(StringView key_sv) const;


    // --------------------------------------------------------------------------
    // General Get operations (Without a key)
    // --------------------------------------------------------------------------

    // returns the value of this node interpreted as ValueType;
    // on error, throws JsonParseException
    template<typename ValueType>
    [[nodiscard]] ValueType Get() const;

    // returns the value of this node if it can be interpreted as ValueType;
    // on error, returns std::nullopt
    template<typename ValueType>
    [[nodiscard]] std::optional<ValueType> GetOptional() const;

    // returns the value of this node if it can be interpreted as ValueType;
    // on error, returns default_value
    template<typename ValueType, class = typename std::enable_if<!std::is_lvalue_reference<ValueType>::value>::type>
    [[nodiscard]] auto GetOrDefault(ValueType&& default_value) const;

    template<typename ValueType>
    [[nodiscard]] auto GetOrDefault(const ValueType& default_value) const;


    // --------------------------------------------------------------------------
    // General Get operations (with a key)
    // --------------------------------------------------------------------------

    // returns the value of the node with the name key, interpreted as ValueType;
    // on error, throws JsonParseException
    template<typename ValueType = JsonNode<CharType>>
    [[nodiscard]] ValueType Get(StringView key_sv) const;

    // returns the value of the node with the name key, if it can be interpreted as ValueType;
    // on error, returns std::nullopt
    template<typename ValueType>
    [[nodiscard]] std::optional<ValueType> GetOptional(StringView key_sv) const;

    // returns the value of the node with the name key, if it can be interpreted as ValueType;
    // on error, returns default_value
    template<typename ValueType, class = typename std::enable_if<!std::is_lvalue_reference<ValueType>::value>::type>
    [[nodiscard]] auto GetOrDefault(StringView key_sv, ValueType&& default_value) const;

    template<typename ValueType>
    [[nodiscard]] auto GetOrDefault(StringView key_sv, const ValueType& default_value) const;


    // --------------------------------------------------------------------------
    // Specialized Get operations (with and without a key)
    // --------------------------------------------------------------------------

    // returns the value of the node interpreted as a date in RFC 3339 format;
    // on error, throws JsonParseException
    [[nodiscard]] time_t GetDate() const;
    [[nodiscard]] time_t GetDate(StringView key_sv) const { return JsonNode<CharType>::Get(key_sv).GetDate(); }

    // returns the value of the node interpreted as a string (failing if the node is an object or an array);
    // on error, throws JsonParseException
    [[nodiscard]] std::wstring GetOnlyString() const;
    [[nodiscard]] std::wstring GetOnlyString(StringView key_sv) const { return JsonNode<CharType>::Get(key_sv).GetOnlyString(); }

    // interprets the node as a string and returns the 0-based index of the node in the options;
    // on error, or if not in the options, throws JsonParseException
    template<typename T>
    [[nodiscard]] size_t GetFromStringOptions(const T& option_strings) const;
    template<typename T>
    [[nodiscard]] size_t GetFromStringOptions(StringView key_sv, const T& option_strings) const { return JsonNode<CharType>::Get(key_sv).template GetFromStringOptions<T>(option_strings); }

    // returns the value of the node interpreted as a double (casting booleans to numbers);
    // on error, throws JsonParseException
    [[nodiscard]] double GetDouble() const;
    [[nodiscard]] double GetDouble(StringView key_sv) const { return JsonNode<CharType>::Get(key_sv).GetDouble(); }

    // returns whether or not the node is valid for the engine (i.e., GetEngineValue will succeed)
    template<typename T>
    [[nodiscard]] bool IsEngineValue() const;
    template<typename T>
    [[nodiscard]] bool IsEngineValue(StringView key_sv) const { return JsonNode<CharType>::Get(key_sv).template IsEngineValue<T>(); }

    // returns the value of the node interpreted for the engine;
    // for numerics: it processes string values specified as text, and calls GetDouble otherwise
    // for strings: it calls GetOnlyString
    template<typename T>
    [[nodiscard]] T GetEngineValue() const;
    template<typename T>
    [[nodiscard]] T GetEngineValue(StringView key_sv) const { return JsonNode<CharType>::Get(key_sv).template GetEngineValue<T>(); }


    // --------------------------------------------------------------------------
    // Array operations (with and without a key)
    // --------------------------------------------------------------------------

    // returns a JsonNodeArray wrapper around the node (when it is an array)
    // on error, throws JsonParseException
    [[nodiscard]] JsonNodeArray<CharType> GetArray() const;
    [[nodiscard]] JsonNodeArray<CharType> GetArray(StringView key_sv) const;

    // returns a JsonNodeArray wrapper around the node (when it is an array)
    // on error, returns an empty array
    [[nodiscard]] JsonNodeArray<CharType> GetArrayOrEmpty() const;
    [[nodiscard]] JsonNodeArray<CharType> GetArrayOrEmpty(StringView key_sv) const;


    // --------------------------------------------------------------------------
    // Miscellaneous operations
    // --------------------------------------------------------------------------

    // returns a list of all the keys that are part of an object (returning an empty list if not an object)
    std::vector<std::basic_string<CharType>> GetKeys() const;

    // executes the callback function for all child nodes, passing the key and child node
    void ForeachNode(const std::function<void(StringView, const JsonNode<CharType>&)>& callback_function) const;

    // returns the underlying jsonscons object representing this node
    [[nodiscard]] const BasicJson& GetBasicJson() const { return *m_json; }


    // --------------------------------------------------------------------------
    // JsonReader calls, which have null default behavior if the JsonNode object
    // was not constructed from a JsonReader
    // --------------------------------------------------------------------------

    // logs a warning
    template<typename... Args>
    void LogWarning(const TCHAR* warning_or_formatter, Args const&... args) const
    {
        m_jsonReaderInterface->OnLogWarning(FormatTextCS2WS(warning_or_formatter, args...));
    }

    // returns an absolute path with native slashes, evaluated relative to the spec file (if available)
    std::wstring GetAbsolutePath() const;
    std::wstring GetAbsolutePath(StringView key_sv) const { return Get(key_sv).GetAbsolutePath(); }

    // returns the serializer helper
    SerializerHelper& GetSerializerHelper() const { return m_jsonReaderInterface->OnGetSerializerHelper(); }


private:
    template<typename ValueType>
    [[nodiscard]] ValueType GetWorker() const;

    [[nodiscard]] JsonNodeArray<CharType> GetEmptyArray() const;

    void ReportInvalidAccessUsingKey(StringView key_sv, const JsonParseException* exception_to_be_thrown) const;

private:
    std::shared_ptr<const BasicJson> m_ownedJson;
    const BasicJson* m_json;
    JsonReaderInterface* m_jsonReaderInterface;
};



// --------------------------------------------------------------------------
// JsonNodeArray +
// JsonNodeArrayIterator
// --------------------------------------------------------------------------

template<typename CharType>
class ZJSON_API JsonNodeArray
{
    friend JsonNode<CharType>;

    using JsonArray = jsoncons::json_array<jsoncons::basic_json<CharType, jsoncons::order_preserving_policy, std::allocator<char>>, std::vector>;

private:
    // constructs a node from a jsoncons operation
    JsonNodeArray(const JsonArray* json_array, JsonReaderInterface& json_reader_interface);

public:
    // returns whether or not the array is empty
    [[nodiscard]] bool empty() const;

    // returns the number of elements in the array
    [[nodiscard]] size_t size() const;

    // returns the node at the given index;
    // on error, throws JsonParseException
    [[nodiscard]] JsonNode<CharType> operator[](size_t index) const;

    // returns an interator to the elements of the array
    [[nodiscard]] JsonNodeArrayIterator<CharType> begin() const;
    [[nodiscard]] JsonNodeArrayIterator<CharType> end() const;

    // returns a vector of the contents of the array
    template<typename ValueType>
    [[nodiscard]] std::vector<ValueType> GetVector() const;

    // returns a vector of the contents of the array with a callback to handle exceptions;
    // when the exception handler is invoked, the object will not be added to the vector,
    // but unless the exception handler rethrows the exception, it will be eaten
    template<typename ValueType, typename ExceptionHandler>
    [[nodiscard]] std::vector<ValueType> GetVector(ExceptionHandler exception_handler) const;

    // returns a set of the contents of the array
    template<typename ValueType>
    [[nodiscard]] std::set<ValueType> GetSet() const;

private:
    const JsonArray* m_jsonArray;
    JsonReaderInterface& m_jsonReaderInterface;
};


template<typename CharType>
class ZJSON_API JsonNodeArrayIterator
{
    friend JsonNodeArray<CharType>;

private:
    JsonNodeArrayIterator(const JsonNodeArray<CharType>* json_node_array, size_t index);

public:
    [[nodiscard]] bool operator!=(const JsonNodeArrayIterator<CharType>& rhs) const { return ( m_index != rhs.m_index ); }

    JsonNodeArrayIterator& operator++();

    [[nodiscard]] const JsonNode<CharType>* operator->() const;
    [[nodiscard]] const JsonNode<CharType>& operator*() const { return *operator->(); }

private:
    const JsonNodeArray<CharType>* m_jsonNodeArray;
    size_t m_index;
    mutable std::shared_ptr<const JsonNode<CharType>> m_currentJsonNode;
};



// --------------------------------------------------------------------------
// JsonNode inline implementations
// --------------------------------------------------------------------------

template<typename CharType>
template<typename ValueType>
ValueType JsonNode<CharType>::Get() const
{
    if constexpr(JsonSerializerTester<ValueType>::HasCreateFromJson())
    {
        return ValueType::CreateFromJson(*this);
    }

    else if constexpr(JsonSerializerTester<JsonSerializer<ValueType>>::HasCreateFromJson())
    {
        return JsonSerializer<ValueType>::CreateFromJson(*this);
    }

    else
    {
        return GetWorker<ValueType>();
    }
}


template<typename CharType>
template<typename ValueType>
std::optional<ValueType> JsonNode<CharType>::GetOptional() const
{
    try
    {
        return Get<ValueType>();
    }

    catch( const JsonParseException& )
    {
        return std::nullopt;
    }
}


template<typename CharType>
template<typename ValueType, class/* = typename std::enable_if<!std::is_lvalue_reference<ValueType>::value>::type*/>
auto JsonNode<CharType>::GetOrDefault(ValueType&& default_value) const
{
    try
    {
        return Get<ValueType>();
    }

    catch( const JsonParseException& )
    {
        return std::forward<ValueType>(default_value);
    }
}


template<typename CharType>
template<typename ValueType>
auto JsonNode<CharType>::GetOrDefault(const ValueType& default_value) const
{
    try
    {
        return Get<ValueType>();
    }

    catch( const JsonParseException& )
    {
        return default_value;
    }
}


template<typename CharType>
template<typename ValueType/* = JsonNode<CharType>*/>
ValueType JsonNode<CharType>::Get(const StringView key_sv) const
{
    try
    {
        return (*this)[key_sv].JsonNode<CharType>::Get<ValueType>();
    }

    catch( const JsonParseException& exception )
    {
        ReportInvalidAccessUsingKey(key_sv, &exception);
        throw;
    }
}


template<typename CharType>
template<typename ValueType>
std::optional<ValueType> JsonNode<CharType>::GetOptional(const StringView key_sv) const
{
    try
    {
        if( Contains(key_sv) )
            return (*this)[key_sv].JsonNode<CharType>::Get<ValueType>();
    }

    catch( const JsonParseException& )
    {
        ReportInvalidAccessUsingKey(key_sv, nullptr);
    }

    return std::nullopt;
}


template<typename CharType>
template<typename ValueType, class/* = typename std::enable_if<!std::is_lvalue_reference<ValueType>::value>::type*/>
auto JsonNode<CharType>::GetOrDefault(const StringView key_sv, ValueType&& default_value) const
{
    try
    {
        if( Contains(key_sv) )
            return (*this)[key_sv].JsonNode<CharType>::Get<ValueType>();
    }

    catch( const JsonParseException& )
    {
        ReportInvalidAccessUsingKey(key_sv, nullptr);
    }

    return std::forward<ValueType>(default_value);
}


template<typename CharType>
template<typename ValueType>
auto JsonNode<CharType>::GetOrDefault(const StringView key_sv, const ValueType& default_value) const
{
    try
    {
        if( Contains(key_sv) )
            return (*this)[key_sv].JsonNode<CharType>::Get<ValueType>();
    }

    catch( const JsonParseException& )
    {
        ReportInvalidAccessUsingKey(key_sv, nullptr);
    }

    return default_value;
}


template<typename CharType>
template<typename T>
size_t JsonNode<CharType>::GetFromStringOptions(const T& option_strings) const
{
    const wstring_view text_sv = Get<wstring_view>();
    size_t index = 0;

    for( const auto& option_string : option_strings )
    {
        if( SO::Equals(text_sv, option_string) )
            return index;

        ++index;
    }

    ASSERT(index > 0);
    throw JsonParseException(_T("'%s' is not a valid option"), std::wstring(text_sv).c_str());
}



// --------------------------------------------------------------------------
// JsonNodeArray inline implementations
// --------------------------------------------------------------------------

template<typename CharType>
template<typename ValueType>
std::vector<ValueType> JsonNodeArray<CharType>::GetVector() const
{
    std::vector<ValueType> values;
    values.reserve(size());

    for( const auto& element : *this )
        values.emplace_back(element.template Get<ValueType>());

    return values;
}


template<typename CharType>
template<typename ValueType, typename ExceptionHandler>
std::vector<ValueType> JsonNodeArray<CharType>::GetVector(ExceptionHandler exception_handler) const
{
    std::vector<ValueType> values;
    values.reserve(size());

    for( const auto& element : *this )
    {
        try
        {
            values.emplace_back(element.template Get<ValueType>());
        }

        catch( const JsonParseException& exception )
        {
            exception_handler(exception);
        }
    }

    return values;
}


template<typename CharType>
template<typename ValueType>
std::set<ValueType> JsonNodeArray<CharType>::GetSet() const
{
    std::set<ValueType> values;

    for( const auto& element : *this )
        values.insert(element.template Get<ValueType>());

    return values;
}
