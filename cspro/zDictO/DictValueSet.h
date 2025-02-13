#pragma once

#include <zDictO/zDictO.h>
#include <zDictO/DictNamedBase.h>
#include <zDictO/DictValue.h>

class ValueProcessor;


class CLASS_DECL_ZDICTO DictValueSet : public DictNamedBase
{
public:
    DictValueSet();

    std::unique_ptr<DictNamedBase> Clone() const override { return std::make_unique<DictValueSet>(*this); }

    DictElementType GetElementType() const override { return DictElementType::ValueSet; }

    int GetSymbolIndex() const            { return m_symbolIndex; }
    void SetSymbolIndex(int symbol_index) { m_symbolIndex = symbol_index; }


    // value set values
    // --------------------------------------------------------------------------
    size_t GetNumValues() const { return m_dictValues.size(); }
    bool HasValues() const      { return !m_dictValues.empty(); }

    const std::vector<DictValue>& GetValues() const { return m_dictValues; }
    std::vector<DictValue>& GetValues()             { return m_dictValues; }

    const DictValue& GetValue(size_t index) const   { ASSERT(index < m_dictValues.size()); return m_dictValues[index]; }
    DictValue& GetValue(size_t index)               { ASSERT(index < m_dictValues.size()); return m_dictValues[index]; }

    void SetValues(std::vector<DictValue> dict_values);

    void AddValue(DictValue dict_value);
    void InsertValue(size_t index, DictValue dict_value);
    void RemoveValue(size_t index);
    void RemoveAllValues();


    // linked value set management
    // --------------------------------------------------------------------------
    bool IsLinkedValueSet() const                     { return !m_linkedValueSetCode.empty(); }
    const std::wstring& GetLinkedValueSetCode() const { return m_linkedValueSetCode; }

    // links both the source value set and this value set to each other
    void LinkValueSet(DictValueSet& source_dict_value_set);

    // sets the linked value set code, creating a serialized link to other
    // value sets that also have their code set to this value
    void LinkValueSetByCode(std::wstring code);

    void UnlinkValueSet();


    // value processor holder
    // --------------------------------------------------------------------------
    std::shared_ptr<const ValueProcessor> GetSharedValueProcessor() const         { return m_valueProcessor; }
    void SetValueProcessor(std::shared_ptr<const ValueProcessor> value_processor) { m_valueProcessor = std::move(value_processor); }


    // convenience methods
    // --------------------------------------------------------------------------

    // returns the number of value pairs that have defined "to" values
    size_t GetNumToValues() const;

    // returns the minimum and maximum values in a numeric value set, ignoring special values
    std::tuple<double, double> GetMinMax() const;


    // serialization
    // --------------------------------------------------------------------------
    static DictValueSet CreateFromJson(const JsonNode<wchar_t>& json_node);
    void WriteJson(JsonWriter& json_writer) const;

    void serialize(Serializer& ar);

private:
    std::vector<DictValue> m_dictValues;

    std::wstring m_linkedValueSetCode;

    int m_symbolIndex; // the symbol table index to the ValueSet class

    std::shared_ptr<const ValueProcessor> m_valueProcessor;
};
