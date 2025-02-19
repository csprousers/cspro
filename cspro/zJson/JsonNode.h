#pragma once

#include <zJson/zJson.h>
#include <zJson/JsonFormattingOptions.h>
#include <zJson/JsonParseException.h>
#include <zJson/JsonSerializer.h>
#include <zToolsO/SerializerHelper.h>
#include <zToolsO/span.h>

namespace jsoncons
{
    template<class CharT, class ImplementationPolicy, class Allocator> class basic_json;
    class json_exception;
    template<class Json, template<typename, typename> class SequenceContainer> class json_array;
    struct order_preserving_policy;
}

class JsonNodeArray;
class JsonNodeArrayIterator;
class PropertyRetriever;


// --------------------------------------------------------------------------
// JsonReaderInterface
// --------------------------------------------------------------------------

class ZJSON_API JsonReaderInterface
{
public:
    JsonReaderInterface(std::string directory = std::string());
    virtual ~JsonReaderInterface() { }

    const std::string& GetDirectory() const { return m_directory; }

    SerializerHelper& OnGetSerializerHelper() const { return const_cast<SerializerHelper&>(m_serializerHelper); }

    virtual void OnLogWarning(std::string message);

    virtual void OnReportInvalidAccessUsingKey(cs::string_sz key, cs::string_sz node_text, const JsonParseException* exception_to_be_thrown);

protected:
    std::string m_directory;

private:
    SerializerHelper m_serializerHelper;
};



// --------------------------------------------------------------------------
// JsonNode
// --------------------------------------------------------------------------

class ZJSON_API JsonNode
{
    friend JsonNodeArray;

protected:
    using BasicJson = jsoncons::basic_json<char, jsoncons::order_preserving_policy, std::allocator<char>>;

    // --------------------------------------------------------------------------
    // construction
    // --------------------------------------------------------------------------

private:
    // constructs a node from a jsoncons operation
    JsonNode(std::shared_ptr<const BasicJson> parent_owned_json, const BasicJson* json, JsonReaderInterface& json_reader_interface);

    // constructs an empty node
    static JsonNode EmptyNode(JsonReaderInterface& json_reader_interface);

public:
    // constructs a node from a jsoncons operation, assuming ownership of the jsoncons object
    JsonNode(std::shared_ptr<const BasicJson> json, JsonReaderInterface* json_reader_interface = nullptr);

    // constructs a node by parsing text;
    // on error, throws JsonParseException
    JsonNode(std::string_view json_text_sv, JsonReaderInterface* json_reader_interface = nullptr);

    // constructs an empty node
    static JsonNode EmptyNode();

    virtual ~JsonNode() { }


    // --------------------------------------------------------------------------
    // operations
    // --------------------------------------------------------------------------

    // returns a string representation of the node
    [[nodiscard]] std::string GetNodeAsString(JsonFormattingOptions formatting_options = DefaultJsonFormattingOptions) const;
    [[nodiscard]] SharableString GetNodeAsSharableString(JsonFormattingOptions formatting_options = DefaultJsonFormattingOptions) const;

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
    [[nodiscard]] bool Contains(std::string_view key_sv) const;

    // returns the property with the given key name;
    // on error, throws JsonParseException
    [[nodiscard]] JsonNode operator[](std::string_view key_sv) const;

    // returns the node with the given key name;
    // if it does not exist, an empty node is returned
    [[nodiscard]] JsonNode GetOrEmpty(std::string_view key_sv) const;


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
    template<typename ValueType = JsonNode>
    [[nodiscard]] ValueType Get(std::string_view key_sv) const;

    // returns the value of the node with the name key, if it can be interpreted as ValueType;
    // on error, returns std::nullopt
    template<typename ValueType>
    [[nodiscard]] std::optional<ValueType> GetOptional(std::string_view key_sv) const;

    // returns the value of the node with the name key, if it can be interpreted as ValueType;
    // on error, returns default_value
    template<typename ValueType, class = typename std::enable_if<!std::is_lvalue_reference<ValueType>::value>::type>
    [[nodiscard]] auto GetOrDefault(std::string_view key_sv, ValueType&& default_value) const;

    template<typename ValueType>
    [[nodiscard]] auto GetOrDefault(std::string_view key_sv, const ValueType& default_value) const;

    // returns the value of the node with the name key, if it can be interpreted as ValueType;
    // on error, returns ValueType()
    template<typename ValueType>
    [[nodiscard]] auto GetOrConstruct(std::string_view key_sv) const;


    // --------------------------------------------------------------------------
    // Specialized Get operations (with and without a key)
    // --------------------------------------------------------------------------

    // returns the value of the node interpreted as a date in RFC 3339 format;
    // on error, throws JsonParseException
    [[nodiscard]] int64_t GetDate() const;
    [[nodiscard]] int64_t GetDate(std::string_view key_sv) const { return Get(key_sv).GetDate(); }

    // returns the value of the node interpreted as a string (failing if the node is an object or an array);
    // on error, throws JsonParseException
    [[nodiscard]] std::string GetOnlyString() const;
    [[nodiscard]] std::string GetOnlyString(std::string_view key_sv) const { return Get(key_sv).GetOnlyString(); }

    // interprets the node as a string and returns the 0-based index of the node in the options;
    // on error, or if not in the options, throws JsonParseException
    template<typename T>
    [[nodiscard]] size_t GetFromStringOptions(const T& option_strings) const;
    [[nodiscard]] size_t GetFromStringOptions(std::initializer_list<const char*> option_strings) const { return GetFromStringOptions(cs::span<const char* const>(option_strings)); }

    template<typename T>
    [[nodiscard]] size_t GetFromStringOptions(std::string_view key_sv, const T& option_strings) const                           { return Get(key_sv).GetFromStringOptions(option_strings); }
    [[nodiscard]] size_t GetFromStringOptions(std::string_view key_sv, std::initializer_list<const char*> option_strings) const { return Get(key_sv).GetFromStringOptions(option_strings); }

    // returns the value of the node interpreted as a double (casting booleans to numbers);
    // on error, throws JsonParseException
    [[nodiscard]] double GetDouble() const;
    [[nodiscard]] double GetDouble(std::string_view key_sv) const { return Get(key_sv).GetDouble(); }

    // returns whether or not the node is valid for the engine (i.e., GetEngineValue will succeed)
    template<typename T>
    [[nodiscard]] bool IsEngineValue() const;
    template<typename T>
    [[nodiscard]] bool IsEngineValue(std::string_view key_sv) const { return Get(key_sv).template IsEngineValue<T>(); }

    // returns the value of the node interpreted for the engine;
    // for numerics: it processes string values specified as text, and calls GetDouble otherwise
    // for strings: it calls GetOnlyString
    template<typename T>
    [[nodiscard]] T GetEngineValue() const;
    template<typename T>
    [[nodiscard]] T GetEngineValue(std::string_view key_sv) const { return Get(key_sv).template GetEngineValue<T>(); }


    // --------------------------------------------------------------------------
    // Array operations (with and without a key)
    // --------------------------------------------------------------------------

    // returns a JsonNodeArray wrapper around the node (when it is an array)
    // on error, throws JsonParseException
    [[nodiscard]] JsonNodeArray GetArray() const;
    [[nodiscard]] JsonNodeArray GetArray(std::string_view key_sv) const;

    // returns a JsonNodeArray wrapper around the node (when it is an array)
    // on error, returns an empty array
    [[nodiscard]] JsonNodeArray GetArrayOrEmpty() const;
    [[nodiscard]] JsonNodeArray GetArrayOrEmpty(std::string_view key_sv) const;


    // --------------------------------------------------------------------------
    // Miscellaneous operations
    // --------------------------------------------------------------------------

    // returns a list of all the keys that are part of an object (returning an empty list if not an object)
    std::vector<std::string> GetKeys() const;

    // executes the callback function for all child nodes, passing the key and child node
    void ForeachNode(const std::function<void(std::string_view, const JsonNode&)>& callback_function) const;

    // returns the underlying jsonscons object representing this node
    [[nodiscard]] const BasicJson& GetBasicJson() const { return *m_json; }

    // returns a PropertyRetriever object that can retrieve properties from the node;
    // the object throws a JsonParseException when processing a property with an invalid value
    std::unique_ptr<PropertyRetriever> CreatePropertyRetriever() const;


    // --------------------------------------------------------------------------
    // JsonReader calls, which have null default behavior if the JsonNode object
    // was not constructed from a JsonReader
    // --------------------------------------------------------------------------

    // logs a warning
    template<typename... Args>
    void LogWarning(const char* warning_or_formatter, Args const&... args) const
    {
        m_jsonReaderInterface->OnLogWarning(FormatText(warning_or_formatter, args...));
    }

    // returns an absolute path with native slashes, evaluated relative to the spec file (if available)
    std::string GetAbsolutePath() const;
    std::string GetAbsolutePath(std::string_view key_sv) const { return Get(key_sv).GetAbsolutePath(); }

    // returns the JsonNode's JsonReaderInterface
    const JsonReaderInterface& GetJsonReaderInterface() const { return *m_jsonReaderInterface; }

    // returns the serializer helper
    SerializerHelper& GetSerializerHelper() const { return m_jsonReaderInterface->OnGetSerializerHelper(); }


private:
    template<typename ValueType>
    [[nodiscard]] ValueType GetWorker() const;

    [[nodiscard]] JsonNodeArray GetEmptyArray() const;

    void ReportInvalidAccessUsingKey(std::string_view key_sv, const JsonParseException* exception_to_be_thrown) const;

private:
    std::shared_ptr<const BasicJson> m_ownedJson;
    const BasicJson* m_json;
    JsonReaderInterface* m_jsonReaderInterface;
};



// --------------------------------------------------------------------------
// JsonNodeArray +
// JsonNodeArrayIterator
// --------------------------------------------------------------------------

class ZJSON_API JsonNodeArray
{
    friend JsonNode;

    using BasicJson = jsoncons::basic_json<char, jsoncons::order_preserving_policy, std::allocator<char>>;
    using JsonArray = jsoncons::json_array<BasicJson, std::vector>;

private:
    // constructs a node from a jsoncons operation
    JsonNodeArray(std::shared_ptr<const BasicJson> parent_owned_json, const JsonArray* json_array, JsonReaderInterface& json_reader_interface);

public:
    // returns whether or not the array is empty
    [[nodiscard]] bool empty() const;

    // returns the number of elements in the array
    [[nodiscard]] size_t size() const;

    // returns the node at the given index;
    // on error, throws JsonParseException
    [[nodiscard]] JsonNode operator[](size_t index) const;

    // returns an interator to the elements of the array
    [[nodiscard]] JsonNodeArrayIterator begin() const;
    [[nodiscard]] JsonNodeArrayIterator end() const;

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
    std::shared_ptr<const BasicJson> m_ownedJson;
    const JsonArray* m_jsonArray;
    JsonReaderInterface& m_jsonReaderInterface;
};


class ZJSON_API JsonNodeArrayIterator
{
    friend JsonNodeArray;

private:
    JsonNodeArrayIterator(const JsonNodeArray* json_node_array, size_t index);

public:
    [[nodiscard]] bool operator!=(const JsonNodeArrayIterator& rhs) const { return ( m_index != rhs.m_index ); }

    JsonNodeArrayIterator& operator++();
    JsonNodeArrayIterator& operator++(int);

    [[nodiscard]] const JsonNode* operator->() const;
    [[nodiscard]] const JsonNode& operator*() const { return *operator->(); }

private:
    const JsonNodeArray* m_jsonNodeArray;
    size_t m_index;
    mutable std::optional<const JsonNode> m_currentJsonNode;
};



// --------------------------------------------------------------------------
// JsonReaderInterfaceinline implementations
// --------------------------------------------------------------------------

inline JsonReaderInterface::JsonReaderInterface(std::string directory/* = std::string()*/)
    :   m_directory(std::move(directory))
{
}



// --------------------------------------------------------------------------
// JsonNode inline implementations
// --------------------------------------------------------------------------

template<typename ValueType>
ValueType JsonNode::Get() const
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


template<typename ValueType>
std::optional<ValueType> JsonNode::GetOptional() const
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


template<typename ValueType, class/* = typename std::enable_if<!std::is_lvalue_reference<ValueType>::value>::type*/>
auto JsonNode::GetOrDefault(ValueType&& default_value) const
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


template<typename ValueType>
auto JsonNode::GetOrDefault(const ValueType& default_value) const
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


template<typename ValueType/* = JsonNode*/>
ValueType JsonNode::Get(const std::string_view key_sv) const
{
    try
    {
        return (*this)[key_sv].JsonNode::Get<ValueType>();
    }

    catch( const JsonParseException& exception )
    {
        ReportInvalidAccessUsingKey(key_sv, &exception);
        throw;
    }
}


template<typename ValueType>
std::optional<ValueType> JsonNode::GetOptional(const std::string_view key_sv) const
{
    try
    {
        if( Contains(key_sv) )
            return (*this)[key_sv].JsonNode::Get<ValueType>();
    }

    catch( const JsonParseException& )
    {
        ReportInvalidAccessUsingKey(key_sv, nullptr);
    }

    return std::nullopt;
}


template<typename ValueType, class/* = typename std::enable_if<!std::is_lvalue_reference<ValueType>::value>::type*/>
auto JsonNode::GetOrDefault(const std::string_view key_sv, ValueType&& default_value) const
{
    try
    {
        if( Contains(key_sv) )
            return (*this)[key_sv].JsonNode::Get<ValueType>();
    }

    catch( const JsonParseException& )
    {
        ReportInvalidAccessUsingKey(key_sv, nullptr);
    }

    return std::forward<ValueType>(default_value);
}


template<typename ValueType>
auto JsonNode::GetOrDefault(const std::string_view key_sv, const ValueType& default_value) const
{
    try
    {
        if( Contains(key_sv) )
            return (*this)[key_sv].JsonNode::Get<ValueType>();
    }

    catch( const JsonParseException& )
    {
        ReportInvalidAccessUsingKey(key_sv, nullptr);
    }

    return default_value;
}


template<typename ValueType>
auto JsonNode::GetOrConstruct(const std::string_view key_sv) const
{
    try
    {
        if( Contains(key_sv) )
            return (*this)[key_sv].JsonNode::Get<ValueType>();
    }

    catch( const JsonParseException& )
    {
        ReportInvalidAccessUsingKey(key_sv, nullptr);
    }

    return ValueType();
}


template<typename T>
size_t JsonNode::GetFromStringOptions(const T& option_strings) const
{
    const std::string_view text_sv = Get<std::string_view>();
    size_t index = 0;

    for( const auto& option_string : option_strings )
    {
        if( SO::Equals(text_sv, option_string) )
            return index;

        ++index;
    }

    ASSERT(index > 0);
    throw JsonParseException("'%s' is not a valid option", std::string(text_sv).c_str());
}



// --------------------------------------------------------------------------
// JsonNodeArray inline implementations
// --------------------------------------------------------------------------

template<typename ValueType>
std::vector<ValueType> JsonNodeArray::GetVector() const
{
    std::vector<ValueType> values;
    values.reserve(size());

    for( const auto& element : *this )
        values.emplace_back(element.template Get<ValueType>());

    return values;
}


template<typename ValueType, typename ExceptionHandler>
std::vector<ValueType> JsonNodeArray::GetVector(ExceptionHandler exception_handler) const
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


template<typename ValueType>
std::set<ValueType> JsonNodeArray::GetSet() const
{
    std::set<ValueType> values;

    for( const auto& element : *this )
        values.insert(element.template Get<ValueType>());

    return values;
}
