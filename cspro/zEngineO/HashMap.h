#pragma once

#include <zEngineO/zEngineO.h>
#include <zUtilO/DataTypes.h>
#include <zLogicO/Symbol.h>


class ZENGINEO_API LogicHashMap : public Symbol
{
private:
    LogicHashMap(const LogicHashMap& hashmap);

public:
    using Data = std::variant<double, std::wstring>;
    struct DimensionValue;

    LogicHashMap(std::wstring hashmap_name);
    ~LogicHashMap();

    void SetValueType(DataType data_type) { m_valueType = data_type; }
    DataType GetValueType() const         { return m_valueType; }

    bool IsValueTypeNumeric() const       { return IsNumeric(m_valueType); }
    bool IsValueTypeString() const        { return IsString(m_valueType); }

    void SetDimensionTypes(std::vector<std::optional<DataType>> dimension_types) { m_dimensionTypes = std::move(dimension_types); }
    size_t GetNumberDimensions() const                                           { return m_dimensionTypes.size(); }
    const std::optional<DataType>& GetDimensionType(size_t index) const          { return m_dimensionTypes[index]; }

    bool DimensionTypeHandles(size_t index, DataType data_type) const { return ( data_type == m_dimensionTypes[index].value_or(data_type) ); }

    bool HasDefaultValue() const                       { return m_defaultValue.has_value(); }
    const std::optional<Data>& GetDefaultValue() const { return m_defaultValue; }
    void SetDefaultValue(Data value)                   { m_defaultValue = std::move(value); }

    bool IsHashMapAssignable(const LogicHashMap& rhs_hashmap, bool allow_implicit_dimension_type_conversion) const;

    LogicHashMap& operator=(const LogicHashMap& rhs_hashmap);

    std::optional<Data> GetValue(const std::vector<Data>& dimension_values) const;
    void SetValue(const std::vector<Data>& dimension_values, Data value);

    bool Contains(const std::vector<Data>& dimension_values) const;

    size_t GetLength(const std::vector<Data>& dimension_values) const;

    bool Remove(const std::vector<Data>& dimension_values);

    std::vector<const Data*> GetKeys(const std::vector<Data>& dimension_values) const;

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
    static void SetValue(std::map<Data, std::unique_ptr<DimensionValue>>& data, const std::vector<Data>& dimension_values, Data value);

    const std::map<Data, std::unique_ptr<DimensionValue>>* TraverseData(const std::vector<Data>& dimension_values, size_t dimensions_to_traverse) const;

private:
    DataType m_valueType;
    std::vector<std::optional<DataType>> m_dimensionTypes;

    std::optional<Data> m_defaultValue;

    std::map<Data, std::unique_ptr<DimensionValue>> m_data;
};
