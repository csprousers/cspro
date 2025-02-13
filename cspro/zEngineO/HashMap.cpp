#include "stdafx.h"
#include "HashMap.h"


// --------------------------------------------------------------------------
// LogicHashMap
// --------------------------------------------------------------------------

struct LogicHashMap::DimensionValue
{
    std::variant<Data, std::map<Data, std::unique_ptr<DimensionValue>>> data;
};


LogicHashMap::LogicHashMap(std::wstring hashmap_name)
    :   Symbol(std::move(hashmap_name), SymbolType::HashMap),
        m_valueType(DataType::Numeric)
{
}


LogicHashMap::LogicHashMap(const LogicHashMap& hashmap)
    :   Symbol(hashmap),
        m_valueType(hashmap.m_valueType),
        m_dimensionTypes(hashmap.m_dimensionTypes),
        m_defaultValue(hashmap.m_defaultValue)
{
}


LogicHashMap::~LogicHashMap()
{
}


std::unique_ptr<Symbol> LogicHashMap::CloneInInitialState() const
{
    return std::unique_ptr<LogicHashMap>(new LogicHashMap(*this));
}


bool LogicHashMap::IsHashMapAssignable(const LogicHashMap& rhs_hashmap, bool allow_implicit_dimension_type_conversion) const
{
    if( m_valueType != rhs_hashmap.m_valueType || m_dimensionTypes.size() != rhs_hashmap.m_dimensionTypes.size() )
        return false;

    for( size_t i = 0; i < m_dimensionTypes.size(); ++i )
    {
        const std::optional<DataType>& lhs_dimension_type = GetDimensionType(i);
        const std::optional<DataType>& rhs_dimension_type = rhs_hashmap.GetDimensionType(i);

        if( ( lhs_dimension_type.has_value() || !allow_implicit_dimension_type_conversion ) && lhs_dimension_type != rhs_dimension_type )
            return false;
    }

    return true;
}


LogicHashMap& LogicHashMap::operator=(const LogicHashMap& rhs_hashmap)
{
    ASSERT(IsHashMapAssignable(rhs_hashmap, true));

    Reset();

    std::function<void(std::map<Data, std::unique_ptr<DimensionValue>>&, const std::map<Data, std::unique_ptr<DimensionValue>>&)> data_copier =
        [&](std::map<Data, std::unique_ptr<DimensionValue>>& lhs_map, const std::map<Data, std::unique_ptr<DimensionValue>>& rhs_map)
        {
            for( const auto& [rhs_key, rhs_value] : rhs_map )
            {
                std::unique_ptr<DimensionValue> lhs_value;

                if( std::holds_alternative<Data>(rhs_value->data) )
                {
                    lhs_value.reset(new DimensionValue { std::get<Data>(rhs_value->data) });
                }

                else
                {
                    lhs_value.reset(new DimensionValue { std::map<Data, std::unique_ptr<DimensionValue>>() });
                    data_copier(std::get<std::map<Data, std::unique_ptr<DimensionValue>>>(lhs_value->data),
                                std::get<std::map<Data, std::unique_ptr<DimensionValue>>>(rhs_value->data));
                }

                lhs_map[rhs_key] = std::move(lhs_value);
            }
        };

    data_copier(m_data, rhs_hashmap.m_data);

    return *this;
}


void LogicHashMap::Reset()
{
    m_data.clear();
}


const std::map<LogicHashMap::Data, std::unique_ptr<LogicHashMap::DimensionValue>>* LogicHashMap::TraverseData(
    const std::vector<Data>& dimension_values, size_t dimensions_to_traverse) const
{
    ASSERT(dimensions_to_traverse <= dimension_values.size() && dimensions_to_traverse < m_dimensionTypes.size());

    auto* data_traverser = &m_data;

    for( size_t i = 0; i < dimensions_to_traverse; ++i )
    {
        const auto& data_lookup = data_traverser->find(dimension_values[i]);

        if( data_lookup == data_traverser->cend() )
            return nullptr;

        ASSERT(!std::holds_alternative<Data>(data_lookup->second->data));
        data_traverser = &std::get<std::map<Data, std::unique_ptr<DimensionValue>>>(data_lookup->second->data);
    }

    return data_traverser;
}


std::optional<LogicHashMap::Data> LogicHashMap::GetValue(const std::vector<Data>& dimension_values) const
{
    ASSERT(dimension_values.size() == m_dimensionTypes.size());

    auto* data_traverser = TraverseData(dimension_values, dimension_values.size() - 1);

    if( data_traverser != nullptr )
    {
        const auto& data_lookup = data_traverser->find(dimension_values.back());

        if( data_lookup != data_traverser->cend() )
        {
            ASSERT(std::holds_alternative<Data>(data_lookup->second->data));
            return std::get<Data>(data_lookup->second->data);
        }
    }

    return m_defaultValue;
}


void LogicHashMap::SetValue(std::map<Data, std::unique_ptr<DimensionValue>>& data, const std::vector<Data>& dimension_values, Data value)
{
    auto* data_traverser = &data;

    for( size_t i = 0; i < dimension_values.size(); ++i )
    {
        const auto& data_lookup = data_traverser->find(dimension_values[i]);

        if( ( i + 1 ) < dimension_values.size() )
        {
            if( data_lookup != data_traverser->cend() )
            {
                ASSERT(!std::holds_alternative<Data>(data_lookup->second->data));
                data_traverser = &std::get<std::map<Data, std::unique_ptr<DimensionValue>>>(data_lookup->second->data);
            }

            else
            {
                std::unique_ptr<DimensionValue>& new_dimension_value = (*data_traverser)[dimension_values[i]] =
                                                                       std::unique_ptr<DimensionValue>(new DimensionValue { std::map<Data, std::unique_ptr<DimensionValue>>() });
                data_traverser = &std::get<std::map<Data, std::unique_ptr<DimensionValue>>>(new_dimension_value->data);
            }
        }

        else
        {
            if( data_lookup != data_traverser->cend() )
            {
                data_lookup->second->data = value;
            }

            else
            {
                (*data_traverser)[dimension_values[i]] = std::unique_ptr<DimensionValue>(new DimensionValue { value });
            }
        }
    }
}


void LogicHashMap::SetValue(const std::vector<Data>& dimension_values, Data value)
{
    ASSERT(dimension_values.size() == m_dimensionTypes.size());

    SetValue(m_data, dimension_values, value);
}


bool LogicHashMap::Contains(const std::vector<Data>& dimension_values) const
{
    ASSERT(!dimension_values.empty() && dimension_values.size() <= m_dimensionTypes.size());

    bool checking_if_contains_value = ( dimension_values.size() == m_dimensionTypes.size() );

    auto* data_traverser = TraverseData(dimension_values, checking_if_contains_value ? ( dimension_values.size() - 1 ) : dimension_values.size());

    if( data_traverser != nullptr )
    {
        if( !checking_if_contains_value )
            return true;

        const auto& data_lookup = data_traverser->find(dimension_values.back());
        return ( data_lookup != data_traverser->cend() );
    }

    return false;
}


size_t LogicHashMap::GetLength(const std::vector<Data>& dimension_values) const
{
    ASSERT(dimension_values.size() < m_dimensionTypes.size());

    auto* data_traverser = TraverseData(dimension_values, dimension_values.size());

    return ( data_traverser == nullptr ) ? 0 : data_traverser->size();
}


bool LogicHashMap::Remove(const std::vector<Data>& dimension_values)
{
    ASSERT(!dimension_values.empty() && dimension_values.size() <= m_dimensionTypes.size());

    auto* data_traverser = const_cast<std::map<LogicHashMap::Data, std::unique_ptr<DimensionValue>>*>(
        TraverseData(dimension_values, dimension_values.size() - 1));

    if( data_traverser != nullptr )
        return ( data_traverser->erase(dimension_values.back()) > 0 );

    return false;
}


std::vector<const LogicHashMap::Data*> LogicHashMap::GetKeys(const std::vector<Data>& dimension_values) const
{
    ASSERT(dimension_values.size() < m_dimensionTypes.size());

    std::vector<const Data*> keys;

    auto* data_traverser = TraverseData(dimension_values, dimension_values.size());

    if( data_traverser != nullptr )
    {
        for( const auto& [key, value] : *data_traverser )
            keys.emplace_back(&key);
    }

    return keys;
}


void LogicHashMap::serialize_subclass(Serializer& ar)
{
    ar.SerializeEnum(m_valueType);

    serialize_optional_enum_vector(ar, m_dimensionTypes);

    ::serialize(ar, m_defaultValue,
        [&](Data& default_value)
        {
            if( IsNumeric(m_valueType) )
            {
                if( ar.IsLoading() )
                {
                    default_value = ar.Read<double>();
                }

                else
                {
                    ar << std::get<double>(default_value);
                }
            }

            else
            {
                ASSERT(IsString(m_valueType));

                if( ar.IsLoading() )
                {
                    default_value = ar.Read<std::wstring>();
                }

                else
                {
                    ar << std::get<std::wstring>(default_value);
                }
            }
        });
}


void LogicHashMap::WriteJsonMetadata_subclass(JsonWriter& json_writer) const
{
    json_writer.Write(JK::contentType, m_valueType);

    json_writer.WriteArray(JK::dimensions, m_dimensionTypes,
        [&](const std::optional<DataType>& dimension_type)
        {
            json_writer.BeginArray();

            if( dimension_type.has_value() )
            {
                json_writer.Write(*dimension_type);
            }

            else
            {
                json_writer.Write(DataType::Numeric);
                json_writer.Write(DataType::String);
            }

            json_writer.EndArray();
        });

    if( m_defaultValue.has_value() )
        json_writer.WriteEngineValue(JK::defaultValue, *m_defaultValue);
}


void LogicHashMap::WriteValueToJson(JsonWriter& json_writer) const
{
    // the HashMap can be written out as an array or an object
    const SymbolSerializerHelper* symbol_serializer_helper = json_writer.GetSerializerHelper().Get<SymbolSerializerHelper>();
    const JsonProperties::HashMapFormat hash_map_format =
        ( symbol_serializer_helper != nullptr ) ? symbol_serializer_helper->GetJsonProperties().GetHashMapFormat() :
                                                  JsonProperties::DefaultHashMapFormat;

    // write in array-style
    if( hash_map_format == JsonProperties::HashMapFormat::Array )
    {
        std::function<void(JsonWriter&, const std::map<LogicHashMap::Data, std::unique_ptr<DimensionValue>>&)> write_map_as_array =
            [&](JsonWriter& json_writer, const std::map<LogicHashMap::Data, std::unique_ptr<DimensionValue>>& data)
            {
                json_writer.BeginArray();

                for( const auto& [key, value] : data )
                {
                    json_writer.BeginObject();

                    json_writer.WriteEngineValue(JK::key, key);

                    json_writer.Key(JK::value);

                    if( std::holds_alternative<LogicHashMap::Data>(value->data) )
                    {
                        json_writer.WriteEngineValue(std::get<LogicHashMap::Data>(value->data));
                    }

                    else
                    {
                        write_map_as_array(json_writer, std::get<std::map<LogicHashMap::Data, std::unique_ptr<DimensionValue>>>(value->data));
                    }

                    json_writer.EndObject();
                }

                json_writer.EndArray();
            };

        write_map_as_array(json_writer, m_data);
    }


    // write in object-style (with numeric keys converted to strings)
    else
    {
        ASSERT(hash_map_format == JsonProperties::HashMapFormat::Object);

        std::function<void(JsonWriter&, const std::map<LogicHashMap::Data, std::unique_ptr<DimensionValue>>&)> write_map_as_object =
            [&](JsonWriter& json_writer, const std::map<LogicHashMap::Data, std::unique_ptr<DimensionValue>>& data)
            {
                json_writer.BeginObject();

                for( const auto& [key, value] : data )
                {
                    // write numeric keys as strings
                    if( std::holds_alternative<double>(key) )
                    {
                        json_writer.Key(DoubleToString(std::get<double>(key)));
                    }

                    else
                    {
                        json_writer.Key(std::get<std::wstring>(key));
                    }

                    if( std::holds_alternative<LogicHashMap::Data>(value->data) )
                    {
                        json_writer.WriteEngineValue(std::get<LogicHashMap::Data>(value->data));
                    }

                    else
                    {
                        write_map_as_object(json_writer, std::get<std::map<LogicHashMap::Data, std::unique_ptr<DimensionValue>>>(value->data));
                    }
                }

                json_writer.EndObject();
            };

        write_map_as_object(json_writer, m_data);
    }
}


void LogicHashMap::UpdateValueFromJson(const JsonNode<wchar_t>& json_node)
{
    std::map<Data, std::unique_ptr<DimensionValue>> new_data;
    std::vector<Data> dimension_values;

    // read in array-style
    if( json_node.IsArray() )
    {
        std::function<void(const JsonNodeArray<wchar_t>&)> process_array_nodes =
            [&](const JsonNodeArray<wchar_t>& json_node_array)
            {
                for( const JsonNode<wchar_t>& array_node : json_node_array )
                {
                    ASSERT(dimension_values.size() < m_dimensionTypes.size());
                    const std::optional<DataType>& dimension_type = m_dimensionTypes[dimension_values.size()];

                    const JsonNode<wchar_t>& key_node = array_node.Get(JK::key);
                    const JsonNode<wchar_t>& value_node = array_node.Get(JK::value);

                    // process the key
                    try
                    {
                        dimension_values.emplace_back(( !dimension_type.has_value() )          ? Data(key_node.GetEngineValue<std::variant<double, std::wstring>>()) :
                                                      ( *dimension_type == DataType::Numeric ) ? Data(key_node.GetEngineValue<double>()) :
                                                                                                 Data(key_node.GetEngineValue<std::wstring>()));
                    }

                    catch(...)
                    {
                        throw CSProException(_T("Dimension '%d' of HashMap '%s' cannot be '%s'"),
                                                static_cast<int>(dimension_values.size()) + 1,
                                                GetName().c_str(),
                                                key_node.GetNodeAsString().c_str());
                    }

                    // if we have read in the correct number of dimensions, store the value
                    if( dimension_values.size() == m_dimensionTypes.size() )
                    {
                        SetValue(new_data, dimension_values, IsValueTypeNumeric() ? Data(value_node.GetEngineValue<double>()) :
                                                                                    Data(value_node.GetEngineValue<std::wstring>()));
                    }

                    // otherwise read more dimensions
                    else
                    {
                        process_array_nodes(value_node.GetArray());
                    }

                    dimension_values.pop_back();
                }
            };

        process_array_nodes(json_node.GetArray());
    }


    // read in object-style
    else
    {
        std::function<void(std::wstring_view, const JsonNode<wchar_t>&)> process_object_node =
            [&](std::wstring_view key_sv, const JsonNode<wchar_t>& value_node)
            {
                ASSERT(dimension_values.size() < m_dimensionTypes.size());
                const std::optional<DataType>& dimension_type = m_dimensionTypes[dimension_values.size()];

                // process the key
                bool add_key_as_string;

                if( dimension_type == DataType::String )
                {
                    add_key_as_string = true;
                }

                else
                {
                    add_key_as_string = !CIMSAString::IsNumericOrSpecial(key_sv);

                    if( add_key_as_string && dimension_type == DataType::Numeric )
                    {
                        throw CSProException(_T("Dimension '%d' of HashMap '%s' cannot be '%s'"),
                                             static_cast<int>(dimension_values.size()) + 1,
                                             GetName().c_str(),
                                             std::wstring(key_sv).c_str());
                    }
                }

                dimension_values.emplace_back(add_key_as_string ? Data(std::wstring(key_sv)) :
                                                                  Data(StringToNumber(key_sv)));

                // if we have read in the correct number of dimensions, store the value
                if( dimension_values.size() == m_dimensionTypes.size() )
                {
                    SetValue(new_data, dimension_values, IsValueTypeNumeric() ? Data(value_node.GetEngineValue<double>()) :
                                                                                Data(value_node.GetEngineValue<std::wstring>()));
                }

                // otherwise read more dimensions
                else
                {
                    value_node.ForeachNode(process_object_node);
                }

                dimension_values.pop_back();
            };

        json_node.ForeachNode(process_object_node);
    }

    m_data = std::move(new_data);
}
