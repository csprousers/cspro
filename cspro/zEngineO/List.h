#pragma once

#include <zEngineO/zEngineO.h>
#include <zUtilO/DataTypes.h>
#include <zLogicO/Symbol.h>


class ZENGINEO_API LogicList : public Symbol
{
protected:
    LogicList(const LogicList& logic_list);

public:
    LogicList(std::string list_name);

    bool IsReadOnly() const               { return ( GetSubType() == SymbolSubType::ValueSetListWrapper ); }

    void SetNumeric(bool numeric);
    bool IsNumeric() const                { return std::holds_alternative<std::vector<double>>(m_values); }
    bool IsString() const                 { return !IsNumeric(); }
    DataType GetDataType() const          { return IsNumeric() ? DataType::Numeric : DataType::String; }

    virtual size_t GetCount() const;
    bool IsValidIndex(size_t index) const { return ( index >= 1 && index <= GetCount() ); }

    void Remove(size_t index);

    template<typename T>
    void AddValue(T value);

    template<typename T>
    void AddValues(std::vector<T> values);

    template<typename T>
    void SetValue(size_t index, T value);

    template<typename T>
    void InsertValue(size_t index, T value);

    template<typename T>
    decltype(auto) GetValue(size_t index) const;

    void InsertList(size_t index, const LogicList& logic_list_to_insert);

    void Sort(bool ascending);

    size_t RemoveDuplicates();

    template<typename T>
    size_t IndexOf(const T& value) const;

    template<typename T>
    bool Contains(const T& value) const { return ( IndexOf(value) > 0 ); }

    // Symbol overrides
    void CompareDeclarationAttributes(const Symbol& symbol) const override;

    std::unique_ptr<Symbol> CloneInInitialState() const override;

    void Reset() override;

    void serialize_subclass(Serializer& ar) override;

    void WriteJsonMetadata_subclass(JsonWriter& json_writer) const override;
    void WriteValueToJson(JsonWriter& json_writer) const override;
    void SetValueFromJson(const JsonNode& json_node) override;

    JavaScript::Value GetJavaScriptValue(JavaScript::Executor& executor) const override;
    void SetValueFromJavaScript(JavaScript::Executor& executor, const JavaScript::Value& js_value) override;

protected:
    virtual double GetValueNumeric(size_t index) const;
    virtual const SharableString& GetValueString(size_t index) const;

private:
    void SetModified() { m_consecutiveIndexOfCalls = 0; }

    template<typename T>
    const auto& GetValues() const;

    template<typename T>
    auto& GetValues();

    template<typename T>
    void InsertListWorker(size_t index, const LogicList& logic_list_to_insert);

    template<typename T>
    void SortWorker(bool ascending);

    template<typename T>
    void SetValueFromJsonWorker(const JsonNode& json_node);

    template<typename T>
    void SetValueFromJavaScriptWorker(JavaScript::Executor& executor, const JavaScript::Value& js_value);

private:
    std::variant<std::vector<double>, std::vector<SharableString>> m_values;

    mutable size_t m_consecutiveIndexOfCalls;
    mutable std::map<size_t, size_t> m_indexOfMap;
};



// --------------------------------------------------------------------------
// inline implementations
// --------------------------------------------------------------------------

template<typename T>
const auto& LogicList::GetValues() const
{
    using ActualT = typename std::conditional_t<std::is_same_v<T, double>, T, SharableString>;
    ASSERT(std::holds_alternative<std::vector<ActualT>>(m_values));
    return std::get<std::vector<ActualT>>(m_values);
}


template<typename T>
auto& LogicList::GetValues()
{
    using ActualT = typename std::conditional_t<std::is_same_v<T, double>, T, SharableString>;
    ASSERT(std::holds_alternative<std::vector<ActualT>>(m_values));
    return std::get<std::vector<ActualT>>(m_values);
}


template<typename T>
void LogicList::AddValue(T value)
{
    GetValues<T>().emplace_back(std::move(value));

    SetModified();
}


template<typename T>
void LogicList::SetValue(const size_t index, T value)
{
    ASSERT(IsValidIndex(index) || index == ( GetCount() + 1 ));
    auto& values = GetValues<T>();

    if( index > values.size() )
    {
        values.emplace_back(std::move(value));
    }

    else
    {
        values[index - 1] = std::move(value);
    }

    SetModified();
}


template<typename T>
void LogicList::InsertValue(const size_t index, T value)
{
    // using one-based array indices
    ASSERT(IsValidIndex(index) || index == ( GetCount() + 1 ));
    auto& values = GetValues<T>();

    values.insert(values.begin() + ( index - 1 ), std::move(value));

    SetModified();
}


template<typename T>
decltype(auto) LogicList::GetValue(const size_t index) const
{
    ASSERT(IsValidIndex(index));

    if constexpr(std::is_same_v<T, double>)
    {
        return GetValueNumeric(index);
    }

    else
    {
        return GetValueString(index);
    }
}
