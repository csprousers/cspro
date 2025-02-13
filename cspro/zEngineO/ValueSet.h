#pragma once

#include <zEngineO/zEngineO.h>
#include <zEngineO/List.h>
#include <zLogicO/Symbol.h>
#include <zDictO/DDClass.h>

class CSymbolVar;
struct EngineData;
class ResponseProcessor;


// --------------------------------------------------------------------------
// ValueSet
// --------------------------------------------------------------------------

class ZENGINEO_API ValueSet : public Symbol
{
    friend class ValueSetListWrapper;

protected:
    ValueSet(const ValueSet& value_set);
    ValueSet(std::wstring value_set_name, CSymbolVar* pVarT, const DictValueSet* dict_value_set, EngineData& engine_data);

public:
    ValueSet(const DictValueSet& dict_value_set, CSymbolVar* pVarT, EngineData& engine_data);
    ~ValueSet();

    bool IsDynamic() const { return ( GetSubType() == SymbolSubType::DynamicValueSet ); }

    CSymbolVar* GetVarT() const                         { return m_pVarT; }
    virtual const DictValueSet& GetDictValueSet() const { return *m_dictValueSet; }

    virtual bool IsNumeric() const;
    bool IsString() const        { return !IsNumeric(); }
    DataType GetDataType() const { return IsNumeric() ? DataType::Numeric : DataType::String; }

    virtual const CString& GetLabel() const;

    virtual size_t GetLength() const;

    struct ForeachValueInfo
    {
        const CString& label;
        const CString& image_filename;
        const PortableColor& text_color;
    };

    virtual void ForeachValue(const std::function<void(const ForeachValueInfo&, double, const std::optional<double>&)>& numeric_callback_function) const;
    virtual void ForeachValue(const std::function<void(const ForeachValueInfo&, const CString&)>& string_callback_function) const;

    virtual void Randomize(const std::vector<double>& numeric_exclusions, const std::vector<CString>& string_exclusions);

    virtual void Sort(bool ascending, bool sort_by_label);

    std::shared_ptr<const ValueProcessor> GetSharedValueProcessor() const;
    const ValueProcessor& GetValueProcessor() const;

    virtual ResponseProcessor* GetResponseProcessor() const;

    // Symbol overrides
    Symbol* FindChildSymbol(const std::wstring& symbol_name) const override;

    void serialize_subclass(Serializer& ar) override;

protected:
    void WriteJsonMetadata_subclass(JsonWriter& json_writer) const override;

public:
    void WriteValueToJson(JsonWriter& json_writer) const override;

    // protected ValueSet overrides
protected:
    virtual void CreateValueProcessor() const;

protected:
    EngineData& m_engineData;
    mutable std::shared_ptr<const ValueProcessor> m_valueProcessor;
    mutable std::unique_ptr<ResponseProcessor> m_responseProcessor;

private:
    CSymbolVar* m_pVarT;
    const DictValueSet* m_dictValueSet;
    mutable std::unique_ptr<std::vector<int>> m_valueSetListWrapperIndices;
};


// --------------------------------------------------------------------------
// DynamicValueSet
// --------------------------------------------------------------------------

struct DynamicValueSetEntry;


class ZENGINEO_API DynamicValueSet : public ValueSet
{
    friend class ValueSetListWrapper;

private:
    DynamicValueSet(const DynamicValueSet& value_set);

public:
    DynamicValueSet(std::wstring value_set_name, EngineData& engine_data);
    ~DynamicValueSet();
 
    void SetNumeric(bool numeric)     { m_numeric = numeric; }
    bool IsNumeric() const override   { return m_numeric; }

    const CString& GetLabel() const override { return WS2CS_Reference(GetName()); }

    size_t GetLength() const override;

    void ValidateNumericFromTo(double from_value, std::optional<double>& to_value) const;

    void AddValue(std::wstring label, std::wstring image_filename, PortableColor text_color, double from_value, std::optional<double> to_value);
    void AddValue(std::wstring label, std::wstring image_filename, PortableColor text_color, std::wstring value);
    size_t AddValues(const ValueSet& value_set);

    size_t RemoveValue(double value);
    size_t RemoveValue(wstring_view value_sv);

    virtual void ForeachValue(const std::function<void(const ForeachValueInfo&, double, const std::optional<double>&)>& numeric_callback_function) const override;
    virtual void ForeachValue(const std::function<void(const ForeachValueInfo&, const CString&)>& string_callback_function) const override;

    void Randomize(const std::vector<double>& numeric_exclusions, const std::vector<CString>& string_exclusions) override;

    void Sort(bool ascending, bool sort_by_label) override;

    std::unique_ptr<ValueSet> CreateValueSet(CSymbolVar* pVarT, bool& value_does_not_fit_in_value_set_warning) const;

    // these methods should never be called for dynamic value sets
    const DictValueSet& GetDictValueSet() const override     { throw ProgrammingErrorException(); }
    ResponseProcessor* GetResponseProcessor() const override { throw ProgrammingErrorException(); }

    // Symbol overrides
    std::unique_ptr<Symbol> CloneInInitialState() const override;

    void Reset() override;

    void serialize_subclass(Serializer& ar) override;

    void WriteValueToJson(JsonWriter& json_writer) const override;
    void UpdateValueFromJson(const JsonNode<wchar_t>& json_node) override;

    // protected ValueSet overrides
protected:
    void CreateValueProcessor() const override;

private:
    template<typename T>
    const T& GetEntry(size_t index) const { return assert_cast<const T&>(*m_entries[index]); }

    std::tuple<std::unique_ptr<DictValueSet>, bool> CreateDictValueSet(size_t complete_length, size_t length, size_t decimals) const;

private:
    bool m_numeric;
    std::vector<std::unique_ptr<const DynamicValueSetEntry>> m_entries;
};


// --------------------------------------------------------------------------
// ValueSetListWrapper (list wrapper of the value set codes/labels)
// --------------------------------------------------------------------------

class ZENGINEO_API ValueSetListWrapper : public LogicList
{
    friend class ValueSet;

private:
    ValueSetListWrapper(std::wstring value_set_list_wrapper_name, int value_set_symbol_index, bool codes_wrapper, const EngineData& engine_data);

public:
    ValueSetListWrapper(std::wstring value_set_list_wrapper_name, const EngineData& engine_data);

    // Symbol overrides
    void serialize_subclass(Serializer& ar) override;

    void WriteValueToJson(JsonWriter& json_writer) const override;
    void UpdateValueFromJson(const JsonNode<wchar_t>& json_node) override;

    // LogicList overrides
    size_t GetCount() const override;
    double GetValue(size_t index) const override;
    const std::wstring& GetString(size_t index) const override;

private:
    const Logic::SymbolTable& GetSymbolTable() const;

private:
    const EngineData& m_engineData;
    int m_valueSetSymbolIndex;
    bool m_codesWrapper;
};
