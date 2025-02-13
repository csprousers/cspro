#pragma once

#include <zEngineO/zEngineO.h>
#include <zUtilO/DataTypes.h>
#include <zLogicO/Symbol.h>


class ZENGINEO_API LogicList : public Symbol
{
protected:
    LogicList(const LogicList& logic_list);

public:
    LogicList(std::wstring list_name);

    bool IsReadOnly() const               { return ( GetSubType() == SymbolSubType::ValueSetListWrapper ); }
                                          
    void SetNumeric(bool numeric)         { m_numeric = numeric; }
    bool IsNumeric() const                { return m_numeric; }
    bool IsString() const                 { return !m_numeric; }
    DataType GetDataType() const          { return m_numeric ? DataType::Numeric : DataType::String; }

    virtual size_t GetCount() const       { return m_numeric ? m_doubleValues.size() : m_stringValues.size(); }
    bool IsValidIndex(size_t index) const { return ( index >= 1 && index <= GetCount() ); }

    void Remove(size_t index);

    void AddValue(double double_value);
    void SetValue(size_t index, double double_value);
    void InsertValue(size_t index, double double_value);
    virtual double GetValue(size_t index) const;

    void AddString(std::wstring string_value);
    void AddStrings(std::vector<std::wstring> string_values);
    void SetString(size_t index, std::wstring string_value);
    void InsertString(size_t index, std::wstring string_value);
    virtual const std::wstring& GetString(size_t index) const;

    void InsertList(size_t index, const LogicList& logic_list_to_insert);

    void Sort(bool ascending);

    size_t RemoveDuplicates();

    size_t IndexOf(double double_value) const;
    size_t IndexOf(const std::wstring& string_value) const;

    template<typename T>
    bool Contains(const T& value) const { return ( IndexOf(value) > 0 ); }

    // Symbol overrides
    std::unique_ptr<Symbol> CloneInInitialState() const override;

    void Reset() override;

    void serialize_subclass(Serializer& ar) override;

protected:
    void WriteJsonMetadata_subclass(JsonWriter& json_writer) const override;

public:
    void WriteValueToJson(JsonWriter& json_writer) const override;
    void UpdateValueFromJson(const JsonNode<wchar_t>& json_node) override;

private:
    void SetModified() { m_consecutiveIndexOfCalls = 0; }

    template<typename T>
    size_t IndexOfWorker(const std::vector<T>& values, const T& value) const;

    template<typename T>
    static void WriteValueToJsonWorker(JsonWriter& json_writer, const std::vector<T>& values);

    template<typename T>
    std::vector<T> UpdateValueFromJsonWorker(const JsonNode<wchar_t>& json_node);

private:
    bool m_numeric;
    std::vector<double> m_doubleValues;
    std::vector<std::wstring> m_stringValues;

    mutable size_t m_consecutiveIndexOfCalls;
    mutable std::map<size_t, size_t> m_indexOfMap;
};



// --------------------------------------------------------------------------
// inline implementations
// --------------------------------------------------------------------------

inline void LogicList::AddValue(double double_value)
{
    m_doubleValues.emplace_back(double_value);

    SetModified();
}


inline void LogicList::SetValue(size_t index, double double_value)
{
    ASSERT(IsValidIndex(index) || index == ( GetCount() + 1 ));

    if( index > m_doubleValues.size() )
    {
        m_doubleValues.emplace_back(double_value);
    }

    else
    {
        m_doubleValues[index - 1] = double_value;
    }

    SetModified();
}


inline void LogicList::InsertValue(size_t index, double double_value)
{
    // using one-based array indices
    ASSERT(IsValidIndex(index) || index == ( GetCount() + 1 ));

    m_doubleValues.insert(m_doubleValues.begin() + ( index - 1 ), double_value);

    SetModified();
}


inline double LogicList::GetValue(size_t index) const
{
    ASSERT(IsValidIndex(index));

    return m_doubleValues[index - 1];
}


inline void LogicList::AddString(std::wstring string_value)
{
    m_stringValues.emplace_back(std::move(string_value));

    SetModified();
}


inline void LogicList::SetString(size_t index, std::wstring string_value)
{
    ASSERT(IsValidIndex(index) || index == ( GetCount() + 1 ));

    if( index > m_stringValues.size() )
    {
        m_stringValues.emplace_back(std::move(string_value));
    }

    else
    {
        m_stringValues[index - 1] = std::move(string_value);
    }

    SetModified();
}


inline void LogicList::InsertString(size_t index, std::wstring string_value)
{
    // using one-based array indices
    ASSERT(IsValidIndex(index) || index == ( GetCount() + 1 ));

    m_stringValues.insert(m_stringValues.begin() + ( index - 1 ), std::move(string_value));

    SetModified();
}


inline const std::wstring& LogicList::GetString(size_t index) const
{
    ASSERT(IsValidIndex(index));

    return m_stringValues[index - 1];
}
