#include "stdafx.h"
#include "List.h"
#include <zToolsO/VectorHelpers.h>
#include <zJavaScript/Executor.h>


// --------------------------------------------------------------------------
// LogicList
// --------------------------------------------------------------------------

LogicList::LogicList(std::string list_name)
    :   Symbol(std::move(list_name), SymbolType::List),
        m_values(std::vector<double>()),
        m_consecutiveIndexOfCalls(0)
{
}


LogicList::LogicList(const LogicList& logic_list)
    :   Symbol(logic_list),
        m_consecutiveIndexOfCalls(0)
{
    SetNumeric(logic_list.IsNumeric());
}


void LogicList::CompareDeclarationAttributes(const Symbol& symbol) const
{
    const LogicList& logic_list = assert_cast<const LogicList&>(symbol);

    if( IsNumeric() != logic_list.IsNumeric() )
    {
        throw CompareDeclarationAttributesException("data type: %s vs. %s", ToString(GetDataType()),
                                                                            ToString(logic_list.GetDataType()));
    }
}


std::unique_ptr<Symbol> LogicList::CloneInInitialState() const
{
    if( IsReadOnly() )
        return nullptr;

    return std::unique_ptr<LogicList>(new LogicList(*this));
}


void LogicList::SetNumeric(const bool numeric)
{
    if( numeric )
    {
        if( !IsNumeric() )
            m_values.emplace<std::vector<double>>();
    }

    else if( !IsString() )
    {
        m_values.emplace<std::vector<SharableString>>();
    }
}


double LogicList::GetValueNumeric(const size_t index) const
{
    ASSERT(IsValidIndex(index));

    return GetValues<double>()[index - 1];
}


const SharableString& LogicList::GetValueString(const size_t index) const
{
    ASSERT(IsValidIndex(index));

    return GetValues<SharableString>()[index - 1];
}


size_t LogicList::GetCount() const
{
    return IsNumeric() ? GetValues<double>().size() :
                         GetValues<SharableString>().size();
}


void LogicList::Reset()
{
    IsNumeric() ? GetValues<double>().clear() :
                  GetValues<SharableString>().clear();

    SetModified();
}


void LogicList::Remove(const size_t index)
{
    ASSERT(IsValidIndex(index));

    std::visit([&](auto& values) { values.erase(values.begin() + ( index - 1 )); },
               m_values);

    SetModified();
}


template<typename T>
void LogicList::AddValues(std::vector<T> values)
{
    auto& list_values = GetValues<T>();

    list_values.insert(list_values.end(), std::make_move_iterator(values.begin()),
                                          std::make_move_iterator(values.end()));

    SetModified();
}

template ZENGINEO_API void LogicList::AddValues<SharableString>(std::vector<SharableString> values);
template ZENGINEO_API void LogicList::AddValues<std::string>(std::vector<std::string> values);


void LogicList::InsertList(const size_t index, const LogicList& logic_list_to_insert)
{
    IsNumeric() ? InsertListWorker<double>(index, logic_list_to_insert) :
                  InsertListWorker<SharableString>(index, logic_list_to_insert);
}


template<typename T>
void LogicList::InsertListWorker(const size_t index, const LogicList& logic_list_to_insert)
{
    // using one-based array indices
    ASSERT(IsValidIndex(index) || index == ( GetCount() + 1 ));
    ASSERT(GetDataType() == logic_list_to_insert.GetDataType());
    ASSERT(this != &logic_list_to_insert);

    auto& values = GetValues<T>();

    if( logic_list_to_insert.GetSubType() == SymbolSubType::ValueSetListWrapper )
    {
        const size_t insert_count = logic_list_to_insert.GetCount();

        for( size_t i = 1; i <= insert_count; ++i )
            values.insert(values.begin() + ( index + i - 2 ), logic_list_to_insert.GetValue<T>(i));
    }

    else
    {
        const auto& insertion_values = logic_list_to_insert.GetValues<T>();
        values.insert(values.begin() + ( index - 1 ), insertion_values.begin(), insertion_values.end());
    }

    SetModified();
}


void LogicList::Sort(const bool ascending)
{
    IsNumeric() ? SortWorker<double>(ascending) :
                  SortWorker<SharableString>(ascending);
}


template<typename T>
void LogicList::SortWorker(const bool ascending)
{
    auto& values = GetValues<T>();

    ascending ? std::sort(values.begin(), values.end(), std::less<T>()) :
                std::sort(values.begin(), values.end(), std::greater<T>());

    SetModified();
}


size_t LogicList::RemoveDuplicates()
{
    size_t duplicates_removed = 0;

    std::visit(
        [&](auto& values)
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

        }, m_values);

    if( duplicates_removed > 0 )
        SetModified();

    return duplicates_removed;
}


namespace
{
    inline size_t HashValue(double value)                { return std::hash<double>{}(value); }
    inline size_t HashValue(const SharableString& value) { return std::hash<std::string>{}(*value); }
}


template<typename T>
size_t LogicList::IndexOf(const T& value) const
{
    // if not using the values vectors, search using GetValue
    if( GetSubType() == SymbolSubType::ValueSetListWrapper )
    {
        const size_t list_count = GetCount();

        for( size_t i = 1; i <= list_count; ++i )
        {
            if( GetValue<T>(i) == value )
                return i;
        }

        return 0;
    }

    // otherwise we can search using std::find; however, if the list is being frequently
    // searched, we will convert the values into a map that can speed up lookups
    constexpr size_t NumberConsecutiveCallsToCreateIndexOfMap = 10;

    const auto& values = GetValues<T>();

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

template ZENGINEO_API size_t LogicList::IndexOf<double>(const double& value) const;
template ZENGINEO_API size_t LogicList::IndexOf<std::string>(const std::string& value) const;
template ZENGINEO_API size_t LogicList::IndexOf<SharableString>(const SharableString& value) const;


void LogicList::serialize_subclass(Serializer& ar)
{
    if( ar.IsSaving() )
    {
        ar.Write<bool>(IsNumeric());
    }

    else
    {
        SetNumeric(ar.Read<bool>());
    }
}


void LogicList::WriteJsonMetadata_subclass(JsonWriter& json_writer) const
{
    json_writer.Write(JK::contentType, GetDataType());
}


void LogicList::WriteValueToJson(JsonWriter& json_writer) const
{
    std::visit(
        [&](const auto& values)
        {
            json_writer.BeginArray();

            for( const auto& value : values )
                json_writer.WriteEngineValue(value);

            json_writer.EndArray();

        }, m_values);
}


void LogicList::SetValueFromJson(const JsonNode& json_node)
{
    IsNumeric() ? SetValueFromJsonWorker<double>(json_node) :
                  SetValueFromJsonWorker<SharableString>(json_node);
}


template<typename T>
void LogicList::SetValueFromJsonWorker(const JsonNode& json_node)
{
    ASSERT(!IsReadOnly());

    if( !json_node.IsArray() )
        throw CSProException("A List must be specified as an array.");

    // allow the specification of Lists in two forms:
    // - [ 1, 2, 3 ]
    // - [ [1], [2], [3] ]

    const JsonNodeArray array_node = json_node.GetArray();

    std::vector<T> values;
    values.reserve(array_node.size());

    for( const JsonNode& value_node : array_node )
    {
        if( value_node.IsArray() )
        {
            const JsonNodeArray value_as_array_node = value_node.GetArray();

            if( value_as_array_node.size() != 1 )
                throw CSProException("A List cannot be created from a multi-dimensional array.");

            values.emplace_back(value_as_array_node[0].GetEngineValue<T>());
        }

        else
        {
            values.emplace_back(value_node.GetEngineValue<T>());
        }
    }

    m_values = std::move(values);

    SetModified();
}


JavaScript::Value LogicList::GetJavaScriptValue(JavaScript::Executor& executor) const
{
    return std::visit(
        [&](const auto& values)
        {
            auto js_array_values = std::make_unique_for_overwrite<JavaScript::Value[]>(values.size());
            JavaScript::Value* js_array_values_itr = js_array_values.get();

            for( const auto& value : values )
            {
                new (js_array_values_itr) JavaScript::Value(executor.CreateEngineValue(value));
                ++js_array_values_itr;
            }

            return executor.CreateArray(values.size(), js_array_values.get());

        }, m_values);
}


void LogicList::SetValueFromJavaScript(JavaScript::Executor& executor, const JavaScript::Value& js_value)
{
    IsNumeric() ? SetValueFromJavaScriptWorker<double>(executor, js_value) :
                  SetValueFromJavaScriptWorker<SharableString>(executor, js_value);
}


template<typename T>
void LogicList::SetValueFromJavaScriptWorker(JavaScript::Executor& executor, const JavaScript::Value& js_value)
{
    ASSERT(!IsReadOnly());

    if( !js_value.IsArray() )
        throw CSProException("A List must be specified as an array.");

    const uint32_t array_size = executor.GetArrayLength(js_value);

    std::vector<T> values;
    values.reserve(array_size);

    for( uint32_t i = 0; i < array_size; ++i )
    {
        const JavaScript::Value js_element = executor.GetArrayElement(js_value, i);

        if( js_element.IsArray() )
        {
            if( executor.GetArrayLength(js_element) != 1 )
                throw CSProException("A List cannot be created from a multi-dimensional array.");

            values.emplace_back(executor.ConvertEngineValue<T>(executor.GetArrayElement(js_element, 0)));
        }

        else
        {
            values.emplace_back(executor.ConvertEngineValue<T>(js_element));
        }
    }

    m_values = std::move(values);

    SetModified();
}
