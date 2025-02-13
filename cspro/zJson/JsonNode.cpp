#include "stdafx.h"
#include "JsonNode.h"
#include "Json.h"
#include "JsonConsExceptionRethrower.h"
#include <zToolsO/Special.h>


// --------------------------------------------------------------------------
// NullJsonReaderInterface
// --------------------------------------------------------------------------

class NullJsonReaderInterface : public JsonReaderInterface
{
public:
    static NullJsonReaderInterface* GetInstance()
    {
        static NullJsonReaderInterface null_json_reader_interface;
        return &null_json_reader_interface;
    }
};



// --------------------------------------------------------------------------
// JsonNode
// --------------------------------------------------------------------------

template<typename CharType>
JsonNode<CharType>::JsonNode(std::basic_string_view<CharType> json_text, JsonReaderInterface* json_reader_interface/* = nullptr*/)
    :   m_jsonReaderInterface(json_reader_interface)
{
    try
    {
        m_ownedJson = std::make_shared<BasicJson>(BasicJson::parse(json_text));
    }

    catch( const jsoncons::json_exception& exception )
    {
        RethrowJsonConsException(exception);
    }

    m_json = m_ownedJson.get();

    if( m_jsonReaderInterface == nullptr )
        m_jsonReaderInterface = NullJsonReaderInterface::GetInstance();
}


template<typename CharType>
JsonNode<CharType>::JsonNode(std::shared_ptr<const BasicJson> json, JsonReaderInterface* json_reader_interface/* = nullptr*/)
    :   m_ownedJson(std::move(json)),
        m_json(m_ownedJson.get()),
        m_jsonReaderInterface(json_reader_interface)
{
    if( m_jsonReaderInterface == nullptr )
        m_jsonReaderInterface = NullJsonReaderInterface::GetInstance();
}


template<typename CharType>
JsonNode<CharType>::JsonNode(const BasicJson* json, JsonReaderInterface& json_reader_interface)
    :   m_json(json),
        m_jsonReaderInterface(&json_reader_interface)
{
}


template<typename CharType>
JsonNode<CharType>::~JsonNode()
{
}


template<typename CharType>
std::basic_string<CharType> JsonNode<CharType>::GetNodeAsString(const JsonFormattingOptions formatting_options/* = DefaultJsonFormattingOptions*/) const
{
    std::unique_ptr<JsonStringWriter<CharType>> json_writer = Json::CreateStringWriter<CharType>(formatting_options);

    json_writer->Write(*this);

    return json_writer->GetString();
}


template<typename CharType>
bool JsonNode<CharType>::IsEmpty() const
{
    return m_json->empty();
}


template<typename CharType>
bool JsonNode<CharType>::IsNull() const
{
    return m_json->is_null();
}


template<typename CharType>
bool JsonNode<CharType>::IsBoolean() const
{
    return m_json->is_bool();
}


template<typename CharType>
bool JsonNode<CharType>::IsNumber() const
{
    return m_json->is_number();
}


template<typename CharType>
bool JsonNode<CharType>::IsDouble() const
{
    return m_json->is_double();
}


template<typename CharType>
bool JsonNode<CharType>::IsString() const
{
    return m_json->is_string();
}


template<typename CharType>
bool JsonNode<CharType>::IsArray() const
{
    return m_json->is_array();
}


template<typename CharType>
bool JsonNode<CharType>::IsObject() const
{
    return m_json->is_object();
}


template<typename CharType>
bool JsonNode<CharType>::Contains(const StringView key_sv) const
{
    return m_json->contains(key_sv);
}


template<typename CharType>
JsonNode<CharType> JsonNode<CharType>::operator[](const StringView key_sv) const
{
    try
    {
        return JsonNode<CharType>(&m_json->at(key_sv), *m_jsonReaderInterface);
    }

    catch( const jsoncons::json_exception& exception )
    {
        RethrowJsonConsException(exception);
    }
}


template<typename CharType>
JsonNode<CharType> JsonNode<CharType>::GetOrEmpty(const StringView key_sv) const
{
    try
    {
        if( Contains(key_sv) )
            return JsonNode<CharType>(&m_json->at(key_sv), *m_jsonReaderInterface);
    }

    catch( const jsoncons::json_exception& )
    {
        // an exception will be thrown if the node is not an object;
        // in this case, return an empty node
        ReportInvalidAccessUsingKey(key_sv, nullptr);
    }

    static BasicJson empty_node;
    return JsonNode<CharType>(&empty_node, *m_jsonReaderInterface);
}


template<typename CharType>
template<typename ValueType>
ValueType JsonNode<CharType>::GetWorker() const
{
    try
    {
        if constexpr(std::is_same_v<ValueType, JsonNode<CharType>>)
        {
            return *this;
        }

        else if constexpr(std::is_same_v<ValueType, std::variant<double, std::wstring>>)
        {
            if( IsNumber() )
                return m_json->template as<double>();

            return m_json->template as<std::wstring>();
        }

        else if constexpr(std::is_same_v<ValueType, CString>)
        {
            if( IsString() )
            {
                const std::wstring_view sv = m_json->template as<std::wstring_view>();
                return CString(sv.data(), sv.length());
            }

            else
            {
                return WS2CS(GetWorker<std::wstring>());
            }
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
#define INSTANTIATE_GET(CharType, ValueType) template ZJSON_API ValueType JsonNode<CharType>::GetWorker<ValueType>() const;

#define INSTANTIATE_GET_BOTH(ValueType) INSTANTIATE_GET(char, ValueType)    \
                                        INSTANTIATE_GET(wchar_t, ValueType)

INSTANTIATE_GET_BOTH(bool)
INSTANTIATE_GET_BOTH(int)
INSTANTIATE_GET_BOTH(unsigned int)
#ifdef WASM
INSTANTIATE_GET_BOTH(unsigned long)
#endif
INSTANTIATE_GET_BOTH(int64_t)
INSTANTIATE_GET_BOTH(uint64_t)
INSTANTIATE_GET_BOTH(float)
INSTANTIATE_GET_BOTH(double)
INSTANTIATE_GET_BOTH(std::vector<double>)

INSTANTIATE_GET(char, std::string_view)
INSTANTIATE_GET(char, std::string)

INSTANTIATE_GET(wchar_t, std::wstring_view)
INSTANTIATE_GET(wchar_t, wstring_view)
INSTANTIATE_GET(wchar_t, std::wstring)
INSTANTIATE_GET(wchar_t, std::string)
INSTANTIATE_GET(wchar_t, CString)
INSTANTIATE_GET(wchar_t, std::vector<std::wstring>)
INSTANTIATE_GET(wchar_t, std::vector<CString>)

INSTANTIATE_GET(char, JsonNode<char>)
INSTANTIATE_GET(wchar_t, JsonNode<wchar_t>)

template ZJSON_API std::variant<double, std::wstring> JsonNode<wchar_t>::GetWorker<std::variant<double, std::wstring>>() const;



template<typename CharType>
inline time_t JsonNode<CharType>::GetDate() const
{
    return PortableFunctions::ParseRFC3339DateTime(Get<std::wstring>());
}


template<typename CharType>
std::wstring JsonNode<CharType>::GetOnlyString() const
{
    if( !IsString() )
    {
        if( IsNull() )
            return std::wstring();

        if( IsArray() || IsObject() )
            throw JsonParseException("Not a string");
    }

    return Get<std::wstring>();
}


template<typename CharType>
double JsonNode<CharType>::GetDouble() const
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


template<typename CharType>
template<typename T>
bool JsonNode<CharType>::IsEngineValue() const
{
    if constexpr(std::is_same_v<T, double>)
    {
        return ( IsNumber() ||
                 IsBoolean() || 
                 ( IsString() && SpecialValues::StringIsSpecial(m_json->template as<std::wstring_view>()) ) ||
                 IsNull() );
    }

    else
    {
        return ( IsString() ||
                 ( !IsArray() && !IsObject() ) );
    }
}

template ZJSON_API bool JsonNode<wchar_t>::IsEngineValue<double>() const;
template ZJSON_API bool JsonNode<wchar_t>::IsEngineValue<std::wstring>() const;


template<typename CharType>
template<typename T>
T JsonNode<CharType>::GetEngineValue() const
{
    if constexpr(std::is_same_v<T, double>)
    {
        // check for special values
        if( IsString() )
        {
            const double* special_value = SpecialValues::StringIsSpecial<const double* >(m_json->template as<std::wstring_view>());

            if( special_value != nullptr )
                return *special_value;
        }

        else if( IsNull() )
        {
            return NOTAPPL;
        }

        return GetDouble();
    }

    else if constexpr(std::is_same_v<T, std::wstring>)
    {
        return GetOnlyString();
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
            const double* special_value = SpecialValues::StringIsSpecial<const double* >(m_json->template as<std::wstring_view>());

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

template ZJSON_API double JsonNode<wchar_t>::GetEngineValue() const;
template ZJSON_API std::wstring JsonNode<wchar_t>::GetEngineValue() const;
template ZJSON_API std::variant<double, std::wstring> JsonNode<wchar_t>::GetEngineValue() const;


template<typename CharType>
JsonNodeArray<CharType> JsonNode<CharType>::GetEmptyArray() const
{
    static typename JsonNodeArray<CharType>::JsonArray empty_array;
    return JsonNodeArray<CharType>(&empty_array, *m_jsonReaderInterface);
}


template<typename CharType>
JsonNodeArray<CharType> JsonNode<CharType>::GetArray() const
{
    try
    {
        return JsonNodeArray<CharType>(&m_json->array_value(), *m_jsonReaderInterface);
    }

    catch( const jsoncons::json_exception& exception )
    {
        RethrowJsonConsException(exception);
    }
}


template<typename CharType>
JsonNodeArray<CharType> JsonNode<CharType>::GetArray(const StringView key_sv) const
{
    return Get(key_sv).GetArray();
}


template<typename CharType>
JsonNodeArray<CharType> JsonNode<CharType>::GetArrayOrEmpty() const
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


template<typename CharType>
JsonNodeArray<CharType> JsonNode<CharType>::GetArrayOrEmpty(const StringView key_sv) const
{
    try
    {
        if( Contains(key_sv) )
            return (*this)[key_sv].JsonNode<CharType>::GetArray();
    }

    catch( const JsonParseException& )
    {
        ReportInvalidAccessUsingKey(key_sv, nullptr);
    }

    return GetEmptyArray();
}


template<typename CharType>
std::wstring JsonNode<CharType>::GetAbsolutePath() const
{
    StringView path_sv = Get<StringView>();

    if constexpr(std::is_same_v<CharType, wchar_t>)
    {
        std::wstring absolute_path = MakeFullPath(m_jsonReaderInterface->GetDirectory(), path_sv);
        ASSERT(absolute_path == PortableFunctions::PathToNativeSlash(absolute_path));
        return absolute_path;
    }

    else
    {
        // currently this is only implemented for wide characters
        return ReturnProgrammingError(UTF8Convert::UTF8ToWide(path_sv));
    }
}


template<typename CharType>
std::vector<std::basic_string<CharType>> JsonNode<CharType>::GetKeys() const
{
    std::vector<std::basic_string<CharType>> keys;

    if( m_json->is_object() )
    {
        for( const auto& member : m_json->object_range() )
            keys.emplace_back(member.key());
    }

    return keys;
}


template<typename CharType>
void JsonNode<CharType>::ForeachNode(const std::function<void(StringView, const JsonNode<CharType>&)>& callback_function) const
{
    if( m_json->is_object() )
    {
        for( const auto& member : m_json->object_range() )
            callback_function(member.key(), JsonNode<CharType>(&member.value(), *m_jsonReaderInterface));
    }
}


template<typename CharType>
void JsonNode<CharType>::ReportInvalidAccessUsingKey(const StringView key_sv, const JsonParseException* exception_to_be_thrown) const
{
    if( !Contains(key_sv) )
        return;

    std::basic_string<CharType> node_text;

    try
    {
        std::unique_ptr<JsonStringWriter<CharType>> json_writer = Json::CreateStringWriter<CharType>();
        json_writer->Write((*this)[key_sv]);
        node_text = json_writer->GetString();
    }

    catch(...)
    {
        // ignore errors creating the node text
        ASSERT(false);
    }

    if constexpr(std::is_same_v<CharType, wchar_t>)
    {
        m_jsonReaderInterface->OnReportInvalidAccessUsingKey(std::wstring(key_sv).c_str(), node_text.c_str(), exception_to_be_thrown);
    }

    else
    {
        // the reporter is only implemented using wide characters
        ASSERT(false);
    }
}



// --------------------------------------------------------------------------
// JsonNodeArray
// --------------------------------------------------------------------------

template<typename CharType>
JsonNodeArray<CharType>::JsonNodeArray(const JsonArray* json_array, JsonReaderInterface& json_reader_interface)
    :   m_jsonArray(json_array),
        m_jsonReaderInterface(json_reader_interface)
{
}


template<typename CharType>
bool JsonNodeArray<CharType>::empty() const
{
    return m_jsonArray->empty();
}


template<typename CharType>
size_t JsonNodeArray<CharType>::size() const
{
    return m_jsonArray->size();
}


template<typename CharType>
JsonNode<CharType> JsonNodeArray<CharType>::operator[](const size_t index) const
{
    try
    {
        ASSERT(index < size());
        return JsonNode<CharType>(&((*m_jsonArray)[index]), m_jsonReaderInterface);
    }

    catch( const jsoncons::json_exception& exception )
    {
        RethrowJsonConsException(exception);
    }
}


template<typename CharType>
JsonNodeArrayIterator<CharType> JsonNodeArray<CharType>::begin() const
{
    return JsonNodeArrayIterator<CharType>(this, 0);
}


template<typename CharType>
JsonNodeArrayIterator<CharType> JsonNodeArray<CharType>::end() const
{
    return JsonNodeArrayIterator<CharType>(this, m_jsonArray->size());
}



// --------------------------------------------------------------------------
// JsonNodeArrayIterator
// --------------------------------------------------------------------------

template<typename CharType>
JsonNodeArrayIterator<CharType>::JsonNodeArrayIterator(const JsonNodeArray<CharType>* json_node_array, const size_t index)
    :   m_jsonNodeArray(json_node_array),
        m_index(index),
        m_currentJsonNode(nullptr)
{
}


template<typename CharType>
JsonNodeArrayIterator<CharType>& JsonNodeArrayIterator<CharType>::operator++()
{
    ++m_index;
    m_currentJsonNode = nullptr;
    return *this;
}

template<typename CharType>
const JsonNode<CharType>* JsonNodeArrayIterator<CharType>::operator->() const
{
    if( m_currentJsonNode == nullptr )
        m_currentJsonNode = std::make_shared<const JsonNode<CharType>>((*m_jsonNodeArray)[m_index]);

    return m_currentJsonNode.get();
}



// --------------------------------------------------------------------------
// instantiate the classes for single and wide characters
// --------------------------------------------------------------------------

template class JsonNode<char>;
template class JsonNode<wchar_t>;
template class JsonNodeArray<char>;
template class JsonNodeArray<wchar_t>;
template class JsonNodeArrayIterator<char>;
template class JsonNodeArrayIterator<wchar_t>;
