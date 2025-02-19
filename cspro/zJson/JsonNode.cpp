#include "stdafx.h"
#include "JsonNode.h"
#include "Json.h"
#include "JsonConsExceptionRethrower.h"
#include <zToolsO/Encoders.h>
#include <zToolsO/PropertyRetriever.h>
#include <zToolsO/Special.h>


// --------------------------------------------------------------------------
// JsonReaderInterface
// --------------------------------------------------------------------------

void JsonReaderInterface::OnLogWarning(std::string /*message*/)
{
}


void JsonReaderInterface::OnReportInvalidAccessUsingKey(const cs::string_sz key, const cs::string_sz node_text,
                                                        const JsonParseException* const exception_to_be_thrown)
{
    if( exception_to_be_thrown == nullptr )
    {
        OnLogWarning(FormatText("The value of '%s' was ignored because it contained an invalid entry: %s",
                                key.c_str(), node_text.c_str()));
    }

    else
    {
        OnLogWarning(FormatText("The value of '%s' was invalid and resulted in an error ('%s'): %s",
                                key.c_str(), exception_to_be_thrown->what(), node_text.c_str()));
    }
}



// --------------------------------------------------------------------------
// NullJsonReaderInterface
// --------------------------------------------------------------------------

class NullJsonReaderInterface : public JsonReaderInterface
{
public:
    static NullJsonReaderInterface& GetInstance()
    {
        static NullJsonReaderInterface null_json_reader_interface;
        return null_json_reader_interface;
    }
};



// --------------------------------------------------------------------------
// JsonNode
// --------------------------------------------------------------------------

JsonNode::JsonNode(const std::string_view json_text_sv, JsonReaderInterface* const json_reader_interface/* = nullptr*/)
    :   m_jsonReaderInterface(json_reader_interface)
{
    try
    {
        m_ownedJson = std::make_unique<BasicJson>(BasicJson::parse(json_text_sv));
    }

    catch( const jsoncons::json_exception& exception )
    {
        RethrowJsonConsException(exception);
    }

    m_json = m_ownedJson.get();

    if( m_jsonReaderInterface == nullptr )
        m_jsonReaderInterface = &NullJsonReaderInterface::GetInstance();
}


JsonNode::JsonNode(std::shared_ptr<const BasicJson> json, JsonReaderInterface* const json_reader_interface/* = nullptr*/)
    :   m_ownedJson(std::move(json)),
        m_json(m_ownedJson.get()),
        m_jsonReaderInterface(json_reader_interface)
{
    if( m_jsonReaderInterface == nullptr )
        m_jsonReaderInterface = &NullJsonReaderInterface::GetInstance();
}


JsonNode::JsonNode(std::shared_ptr<const BasicJson> parent_owned_json, const BasicJson* const json, JsonReaderInterface& json_reader_interface)
    :   m_ownedJson(std::move(parent_owned_json)),
        m_json(json),
        m_jsonReaderInterface(&json_reader_interface)
{
}


JsonNode JsonNode::EmptyNode(JsonReaderInterface& json_reader_interface)
{
    static BasicJson empty_node;
    return JsonNode(nullptr, &empty_node, json_reader_interface);
}


JsonNode JsonNode::EmptyNode()
{
    return EmptyNode(NullJsonReaderInterface::GetInstance());
}


std::string JsonNode::GetNodeAsString(const JsonFormattingOptions formatting_options/* = DefaultJsonFormattingOptions*/) const
{
    return GetNodeAsSharableString(formatting_options).Release();
}


SharableString JsonNode::GetNodeAsSharableString(const JsonFormattingOptions formatting_options/* = DefaultJsonFormattingOptions*/) const
{
    if( IsString() )
    {
        return Encoders::ToJsonString(Get<std::string_view>());
    }

    else
    {
        const std::unique_ptr<JsonStringWriter> json_writer = Json::CreateStringWriter(formatting_options);
        json_writer->Write(*this);
        return json_writer->ReleaseSharableString();
    }
}


bool JsonNode::IsEmpty() const
{
    return m_json->empty();
}


bool JsonNode::IsNull() const
{
    return m_json->is_null();
}


bool JsonNode::IsBoolean() const
{
    return m_json->is_bool();
}


bool JsonNode::IsNumber() const
{
    return m_json->is_number();
}


bool JsonNode::IsDouble() const
{
    return m_json->is_double();
}


bool JsonNode::IsString() const
{
    return m_json->is_string();
}


bool JsonNode::IsArray() const
{
    return m_json->is_array();
}


bool JsonNode::IsObject() const
{
    return m_json->is_object();
}


bool JsonNode::Contains(const std::string_view key_sv) const
{
    return m_json->contains(key_sv);
}


JsonNode JsonNode::operator[](const std::string_view key_sv) const
{
    try
    {
        return JsonNode(m_ownedJson, &m_json->at(key_sv), *m_jsonReaderInterface);
    }

    catch( const jsoncons::json_exception& exception )
    {
        RethrowJsonConsException(exception);
    }
}


JsonNode JsonNode::GetOrEmpty(const std::string_view key_sv) const
{
    try
    {
        if( Contains(key_sv) )
            return JsonNode(m_ownedJson, &m_json->at(key_sv), *m_jsonReaderInterface);
    }

    catch( const jsoncons::json_exception& )
    {
        // an exception will be thrown if the node is not an object;
        // in this case, return an empty node
        ReportInvalidAccessUsingKey(key_sv, nullptr);
    }

    return EmptyNode(*m_jsonReaderInterface);
}


template<typename ValueType>
ValueType JsonNode::GetWorker() const
{
    try
    {
        if constexpr(std::is_same_v<ValueType, JsonNode>)
        {
            return *this;
        }

        else if constexpr(std::is_same_v<ValueType, SharableString>)
        {
            return m_json->template as<std::string_view>();
        }

        else if constexpr(std::is_same_v<ValueType, std::variant<double, std::string>>)
        {
            if( IsNumber() )
                return m_json->template as<double>();

            return m_json->template as<std::string>();
        }

        else if constexpr(std::is_same_v<ValueType, std::variant<double, std::wstring>>)
        {
            if( IsNumber() )
                return m_json->template as<double>();

            return GetWorker<std::wstring>();
        }

        else if constexpr(std::is_same_v<ValueType, std::wstring>)
        {
            return IsString() ? UTF8_TODO::GetWide(m_json->template as<std::string_view>()) :
                                UTF8_TODO::GetWide(GetWorker<std::string>());
        }

        else if constexpr(std::is_same_v<ValueType, CString>)
        {
            return IsString() ? UTF8_TODO::GetCString(m_json->template as<std::string_view>()) :
                                UTF8_TODO::GetCString(GetWorker<std::string>());
        }

        else if constexpr(std::is_same_v<ValueType, std::vector<CString>>)
        {
            return GetArray().template GetVector<CString>();
        }

        else
        {
            return m_json->template as<ValueType>();
        }
    }

    catch( const jsoncons::json_exception& exception )
    {
        RethrowJsonConsException(exception);
    }
}

// instantiate Get for common types
template ZJSON_API JsonNode JsonNode::GetWorker<JsonNode>() const;

template ZJSON_API bool JsonNode::GetWorker<bool>() const;
template ZJSON_API int JsonNode::GetWorker<int>() const;
template ZJSON_API unsigned int JsonNode::GetWorker<unsigned int>() const;
#ifdef WASM
template ZJSON_API unsigned long JsonNode::GetWorker<unsigned long>() const;
#endif
template ZJSON_API int64_t JsonNode::GetWorker<int64_t>() const;
template ZJSON_API uint64_t JsonNode::GetWorker<uint64_t>() const;
template ZJSON_API float JsonNode::GetWorker<float>() const;
template ZJSON_API double JsonNode::GetWorker<double>() const;

template ZJSON_API std::string_view JsonNode::GetWorker<std::string_view>() const;
template ZJSON_API std::string JsonNode::GetWorker<std::string>() const;
template ZJSON_API SharableString JsonNode::GetWorker<SharableString>() const;

template ZJSON_API std::wstring JsonNode::GetWorker<std::wstring>() const;
template ZJSON_API CString JsonNode::GetWorker<CString>() const;

template ZJSON_API std::vector<double> JsonNode::GetWorker<std::vector<double>>() const;
template ZJSON_API std::vector<std::string> JsonNode::GetWorker<std::vector<std::string>>() const;
template ZJSON_API std::vector<std::wstring> JsonNode::GetWorker<std::vector<std::wstring>>() const;
template ZJSON_API std::vector<CString> JsonNode::GetWorker<std::vector<CString>>() const;

template ZJSON_API std::variant<double, std::string> JsonNode::GetWorker<std::variant<double, std::string>>() const;
template ZJSON_API std::variant<double, std::wstring> JsonNode::GetWorker<std::variant<double, std::wstring>>() const;


int64_t JsonNode::GetDate() const
{
    return PortableFunctions::ParseRFC3339DateTime(Get<std::string>());
}


std::string JsonNode::GetOnlyString() const
{
    if( !IsString() )
    {
        if( IsNull() )
            return std::string();

        if( IsArray() || IsObject() )
            throw JsonParseException("Not a string");
    }

    return Get<std::string>();
}


double JsonNode::GetDouble() const
{
    try
    {
        // if the node cannot be read as a double...
        return m_json->as_double();
    }

    catch( const jsoncons::json_exception& exception )
    {
        // ... try to read it as integer (as boolean values will be cast properly)
        try
        {
            return m_json->template as_integer<int>();
        }
        catch(...) { }

        RethrowJsonConsException(exception);
    }
}


template<typename T>
bool JsonNode::IsEngineValue() const
{
    if constexpr(std::is_same_v<T, double>)
    {
        return ( IsNumber() ||
                 IsBoolean() ||
                 ( IsString() && SpecialValues::StringIsSpecial(m_json->template as<std::string_view>()) ) ||
                 IsNull() );
    }

    else
    {
        return ( IsString() ||
                 ( !IsArray() && !IsObject() ) );
    }
}

template ZJSON_API bool JsonNode::IsEngineValue<double>() const;
template ZJSON_API bool JsonNode::IsEngineValue<std::string>() const;
template ZJSON_API bool JsonNode::IsEngineValue<SharableString>() const;
template ZJSON_API bool JsonNode::IsEngineValue<std::wstring>() const;


template<typename T>
T JsonNode::GetEngineValue() const
{
    if constexpr(std::is_same_v<T, double>)
    {
        // check for special values
        if( IsString() )
        {
            const double* const special_value = SpecialValues::StringIsSpecial<const double*>(m_json->template as<std::string_view>());

            if( special_value != nullptr )
                return *special_value;
        }

        else if( IsNull() )
        {
            return NOTAPPL;
        }

        return GetDouble();
    }

    else if constexpr(std::is_same_v<T, std::string> ||
                      std::is_same_v<T, SharableString>)
    {
        return GetOnlyString();
    }

    else if constexpr(std::is_same_v<T, std::wstring>)
    {
        return UTF8_TODO::GetWide(GetOnlyString());
    }

    else if constexpr(std::is_same_v<T, std::variant<double, SharableString>>)
    {
        if( IsEngineValue<double>() && !IsNull() )
        {
            return GetEngineValue<double>();
        }

        else
        {
            // when reading variant values, treat special value strings as numbers
            const double* const special_value = SpecialValues::StringIsSpecial<const double*>(m_json->template as<std::string_view>());

            if( special_value != nullptr )
                return *special_value;

            return GetEngineValue<SharableString>();
        }
    }

    else if constexpr(std::is_same_v<T, std::variant<double, std::wstring>>)
    {
        if( IsEngineValue<double>() && !IsNull() )
        {
            return GetEngineValue<double>();
        }

        else
        {
            // when reading variant values, treat special value strings as numbers
            const double* const special_value = SpecialValues::StringIsSpecial<const double*>(m_json->template as<std::string_view>());

            if( special_value != nullptr )
                return *special_value;

            return GetEngineValue<std::wstring>();
        }
    }

    else
    {
        static_assert_false();
    }
}

template ZJSON_API double JsonNode::GetEngineValue() const;
template ZJSON_API std::string JsonNode::GetEngineValue() const;
template ZJSON_API SharableString JsonNode::GetEngineValue() const;
template ZJSON_API std::wstring JsonNode::GetEngineValue() const;
template ZJSON_API std::variant<double, SharableString> JsonNode::GetEngineValue() const;
template ZJSON_API std::variant<double, std::wstring> JsonNode::GetEngineValue() const;


JsonNodeArray JsonNode::GetEmptyArray() const
{
    static typename JsonNodeArray::JsonArray empty_array;
    return JsonNodeArray(nullptr, &empty_array, *m_jsonReaderInterface);
}


JsonNodeArray JsonNode::GetArray() const
{
    try
    {
        return JsonNodeArray(m_ownedJson, &m_json->array_value(), *m_jsonReaderInterface);
    }

    catch( const jsoncons::json_exception& exception )
    {
        RethrowJsonConsException(exception);
    }
}


JsonNodeArray JsonNode::GetArray(const std::string_view key_sv) const
{
    return Get(key_sv).GetArray();
}


JsonNodeArray JsonNode::GetArrayOrEmpty() const
{
    try
    {
        return GetArray();
    }

    catch( const JsonParseException& )
    {
        return GetEmptyArray();
    }
}


JsonNodeArray JsonNode::GetArrayOrEmpty(const std::string_view key_sv) const
{
    try
    {
        if( Contains(key_sv) )
            return (*this)[key_sv].JsonNode::GetArray();
    }

    catch( const JsonParseException& )
    {
        ReportInvalidAccessUsingKey(key_sv, nullptr);
    }

    return GetEmptyArray();
}


std::string JsonNode::GetAbsolutePath() const
{
    std::string absolute_path = MakeFullPath(m_jsonReaderInterface->GetDirectory(),
                                             Get<std::string>());

    ASSERT(absolute_path == PortableFunctions::PathToNativeSlash(absolute_path));

    return absolute_path;
}


std::vector<std::string> JsonNode::GetKeys() const
{
    std::vector<std::string> keys;

    if( m_json->is_object() )
    {
        for( const auto& member : m_json->object_range() )
            keys.emplace_back(member.key());
    }

    return keys;
}


void JsonNode::ForeachNode(const std::function<void(std::string_view, const JsonNode&)>& callback_function) const
{
    if( m_json->is_object() )
    {
        for( const auto& member : m_json->object_range() )
            callback_function(member.key(), JsonNode(m_ownedJson, &member.value(), *m_jsonReaderInterface));
    }
}


void JsonNode::ReportInvalidAccessUsingKey(const std::string_view key_sv, const JsonParseException* const exception_to_be_thrown) const
{
    if( !Contains(key_sv) )
        return;

    std::string node_text;

    try
    {
        node_text = Json::ToJson(Get(key_sv));
    }

    catch(...)
    {
        // ignore errors creating the node text
        ASSERT(false);
    }

    m_jsonReaderInterface->OnReportInvalidAccessUsingKey(std::string(key_sv), node_text, exception_to_be_thrown);
}



// --------------------------------------------------------------------------
// JsonNodePropertyRetriever
// --------------------------------------------------------------------------

class JsonNodePropertyRetriever : public PropertyRetriever
{
public:
    JsonNodePropertyRetriever(const JsonNode& json_node)
        :   m_jsonNode(json_node)
    {
    }

    std::optional<std::string> GetProperty(const std::string_view attribute_sv) override
    {
        return m_jsonNode.template GetOptional<std::string>(attribute_sv);
    }

    void OnInvalidPropertyValue(const std::string_view attribute_sv, const std::string& value) override
    {
        throw JsonParseException("'%s' is not a valid %s", value.c_str(), std::string(attribute_sv).c_str());
    }

private:
    const JsonNode& m_jsonNode;
};


std::unique_ptr<PropertyRetriever> JsonNode::CreatePropertyRetriever() const
{
    return std::make_unique<JsonNodePropertyRetriever>(*this);
}



// --------------------------------------------------------------------------
// JsonNodeArray
// --------------------------------------------------------------------------

JsonNodeArray::JsonNodeArray(std::shared_ptr<const BasicJson> parent_owned_json, const JsonArray* const json_array, JsonReaderInterface& json_reader_interface)
    :   m_ownedJson(std::move(parent_owned_json)),
        m_jsonArray(json_array),
        m_jsonReaderInterface(json_reader_interface)
{
}


bool JsonNodeArray::empty() const
{
    return m_jsonArray->empty();
}


size_t JsonNodeArray::size() const
{
    return m_jsonArray->size();
}


JsonNode JsonNodeArray::operator[](const size_t index) const
{
    try
    {
        ASSERT(index < size());
        return JsonNode(m_ownedJson, &((*m_jsonArray)[index]), m_jsonReaderInterface);
    }

    catch( const jsoncons::json_exception& exception )
    {
        RethrowJsonConsException(exception);
    }
}


JsonNodeArrayIterator JsonNodeArray::begin() const
{
    return JsonNodeArrayIterator(this, 0);
}


JsonNodeArrayIterator JsonNodeArray::end() const
{
    return JsonNodeArrayIterator(this, m_jsonArray->size());
}



// --------------------------------------------------------------------------
// JsonNodeArrayIterator
// --------------------------------------------------------------------------

JsonNodeArrayIterator::JsonNodeArrayIterator(const JsonNodeArray* const json_node_array, const size_t index)
    :   m_jsonNodeArray(json_node_array),
        m_index(index)
{
}


JsonNodeArrayIterator& JsonNodeArrayIterator::operator++()
{
    ++m_index;
    m_currentJsonNode.reset();
    return *this;
}


JsonNodeArrayIterator& JsonNodeArrayIterator::operator++(int)
{
    m_currentJsonNode.emplace((*m_jsonNodeArray)[m_index]);
    ++m_index;
    return *this;
}


const JsonNode* JsonNodeArrayIterator::operator->() const
{
    if( !m_currentJsonNode.has_value() )
        m_currentJsonNode.emplace((*m_jsonNodeArray)[m_index]);

    return &(*m_currentJsonNode);
}
