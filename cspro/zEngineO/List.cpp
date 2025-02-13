#include "stdafx.h"
#include "List.h"
#include <zToolsO/VectorHelpers.h>


// --------------------------------------------------------------------------
// LogicList
// --------------------------------------------------------------------------

LogicList::LogicList(std::wstring list_name)
    :   Symbol(std::move(list_name), SymbolType::List),
        m_numeric(true),
        m_consecutiveIndexOfCalls(0)
{
}


LogicList::LogicList(const LogicList& logic_list)
    :   Symbol(logic_list),
        m_numeric(logic_list.m_numeric),
        m_consecutiveIndexOfCalls(0)
{
}


std::unique_ptr<Symbol> LogicList::CloneInInitialState() const
{
    if( IsReadOnly() )
        return nullptr;

    return std::unique_ptr<LogicList>(new LogicList(*this));
}


void LogicList::Reset()
{
    if( m_numeric )
    {
        m_doubleValues.clear();
    }

    else
    {
        m_stringValues.clear();
    }

    SetModified();
}


void LogicList::Remove(const size_t index)
{
    ASSERT(IsValidIndex(index));

    if( m_numeric )
    {
        m_doubleValues.erase(m_doubleValues.begin() + ( index - 1 ));
    }

    else
    {
        m_stringValues.erase(m_stringValues.begin() + ( index - 1 ));
    }

    SetModified();
}


void LogicList::AddStrings(std::vector<std::wstring> string_values)
{
    VectorHelpers::Append(m_stringValues, std::move(string_values));

    SetModified();
}


void LogicList::InsertList(const size_t index, const LogicList& logic_list_to_insert)
{
    // using one-based array indices
    ASSERT(IsValidIndex(index) || index == ( GetCount() + 1 ));
    ASSERT(GetDataType() == logic_list_to_insert.GetDataType());
    ASSERT(this != &logic_list_to_insert);

    if( logic_list_to_insert.GetSubType() == SymbolSubType::ValueSetListWrapper )
    {
        size_t insert_count = logic_list_to_insert.GetCount();

        if( m_numeric )
        {
            for( size_t i = 1; i <= insert_count; ++i )
                m_doubleValues.insert(m_doubleValues.begin() + ( index + i - 2 ), logic_list_to_insert.GetValue(i));
        }

        else
        {
            for( size_t i = 1; i <= insert_count; ++i )
                m_stringValues.insert(m_stringValues.begin() + ( index + i - 2 ), logic_list_to_insert.GetString(i));
        }
    }

    else
    {
        if( m_numeric )
        {
            m_doubleValues.insert(m_doubleValues.begin() + ( index - 1 ), logic_list_to_insert.m_doubleValues.begin(), logic_list_to_insert.m_doubleValues.end());
        }

        else
        {
            m_stringValues.insert(m_stringValues.begin() + ( index - 1 ), logic_list_to_insert.m_stringValues.begin(), logic_list_to_insert.m_stringValues.end());
        }
    }

    SetModified();
}


void LogicList::Sort(const bool ascending)
{
    auto do_sort = [](auto& values, auto&& sorter)
    {
        std::sort(values.begin(), values.end(), std::move(sorter));
    };

    m_numeric ? ascending ? do_sort(m_doubleValues, std::less<double>()) :
                            do_sort(m_doubleValues, std::greater<double>()) :
                ascending ? do_sort(m_stringValues, std::less<std::wstring>()) :
                            do_sort(m_stringValues, std::greater<std::wstring>());

    SetModified();
}


size_t LogicList::RemoveDuplicates()
{
    size_t duplicates_removed = 0;

    auto do_removal = [&](auto& values)
    {
        if( values.size() < 2 )
            return;

        for( auto itr = values.end() - 1; itr > values.begin(); --itr )
        {
            if( std::find(values.begin(), itr, *itr) < itr )
            {
                itr = values.erase(itr);
                ++duplicates_removed;
            }
        }
    };

    m_numeric ? do_removal(m_doubleValues) :
                do_removal(m_stringValues);

    if( duplicates_removed > 0 )
        SetModified();

    return duplicates_removed;
}


namespace
{
    inline size_t HashValue(const std::wstring& value) { return std::hash<std::wstring>{}(std::wstring(value)); }
    inline size_t HashValue(const double& value)  { return std::hash<double>{}(value); }
}


template<typename T>
size_t LogicList::IndexOfWorker(const std::vector<T>& values, const T& value) const
{
    // if not using the values vectors, search using GetValue
    if( GetSubType() == SymbolSubType::ValueSetListWrapper )
    {
        size_t list_count = GetCount();

        for( size_t i = 1; i <= list_count; ++i )
        {
            if constexpr(std::is_same_v<T, std::wstring>)
            {
                if( value == GetString(i) )
                    return i;
            }

            else
            {
                if( value == GetValue(i) )
                    return i;
            }
        }

        return 0;
    }

    // otherwise we can search using std::find; however, if the list is being frequently
    // searched, we will convert the values into a map that can speed up lookups
    constexpr size_t NumberConsecutiveCallsToCreateIndexOfMap = 10;

    if( ++m_consecutiveIndexOfCalls < NumberConsecutiveCallsToCreateIndexOfMap )
    {
        const auto& value_search = std::find(values.cbegin(), values.cend(), value);

        if( value_search == values.cend() )
            return 0;

        return std::distance(values.cbegin(), value_search) + 1;
    }

    else
    {
        // create the map
        if( m_consecutiveIndexOfCalls == NumberConsecutiveCallsToCreateIndexOfMap )
        {
            m_indexOfMap.clear();

            for( size_t i = 0; i < values.size(); ++i )
                m_indexOfMap.try_emplace(HashValue(values[i]), i + 1);
        }

        const auto& map_search = m_indexOfMap.find(HashValue(value));

        return ( map_search == m_indexOfMap.cend() ) ? 0 : map_search->second;
    }
}


size_t LogicList::IndexOf(const double double_value) const
{
    return IndexOfWorker(m_doubleValues, double_value);
}


size_t LogicList::IndexOf(const std::wstring& string_value) const
{
    return IndexOfWorker(m_stringValues, string_value);
}


void LogicList::serialize_subclass(Serializer& ar)
{
    ar & m_numeric;
}


void LogicList::WriteJsonMetadata_subclass(JsonWriter& json_writer) const
{
    json_writer.Write(JK::contentType, GetDataType());
}


void LogicList::WriteValueToJson(JsonWriter& json_writer) const
{
    m_numeric ? WriteValueToJsonWorker(json_writer, m_doubleValues) :
                WriteValueToJsonWorker(json_writer, m_stringValues);
}


template<typename T>
void LogicList::WriteValueToJsonWorker(JsonWriter& json_writer, const std::vector<T>& values)
{
    json_writer.BeginArray();

    for( const T& value : values )
        json_writer.WriteEngineValue(value);

    json_writer.EndArray();
}


void LogicList::UpdateValueFromJson(const JsonNode<wchar_t>& json_node)
{
    if( m_numeric )
    {
        m_doubleValues = UpdateValueFromJsonWorker<double>(json_node);
    }

    else
    {
        m_stringValues = UpdateValueFromJsonWorker<std::wstring>(json_node);
    }
}


template<typename T>
std::vector<T> LogicList::UpdateValueFromJsonWorker(const JsonNode<wchar_t>& json_node)
{
    if( !json_node.IsArray() )
        throw CSProException("A List must be specified as an array.");

    // allow the specification of Lists in two forms:
    // - [ 1, 2, 3 ]
    // - [ [1], [2], [3] ]

    const JsonNodeArray<wchar_t> array_node = json_node.GetArray();

    std::vector<T> values;
    values.reserve(array_node.size());

    for( const auto& value_node : array_node )
    {
        if( value_node.IsArray() )
        {
            const JsonNodeArray<wchar_t> value_as_array_node = value_node.GetArray();

            if( value_as_array_node.size() != 1 )
                throw CSProException("A List cannot be created from a multi-dimensional array.");

            values.emplace_back(value_as_array_node[0].GetEngineValue<T>());
        }

        else
        {
            values.emplace_back(value_node.GetEngineValue<T>());
        }
    }

    return values;
}
