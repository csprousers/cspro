#pragma once

#include <zDictO/zDictO.h>
#include <zDictO/ValueProcessor.h>


// --------------------------------------------------------------------------
// StringValueProcessor
//
// The base class for the value processor of string values.
// --------------------------------------------------------------------------

class CLASS_DECL_ZDICTO StringValueProcessor : public ValueProcessor
{
protected:
    using ValueProcessor::ValueProcessor;

    // ValueProcessor overrides
public:
    using ValueProcessor::IsValid;
    using ValueProcessor::GetDictValue;

    const DictValue* GetDictValueFromInput(std::string_view value_sv) const override final;

private:
    bool IsValid(double value) const override final;

    const DictValue* GetDictValue(double value) const override final;

    std::vector<const DictValue*> GetMatchingDictValues(double value) const override final;

    double GetNumericFromInput(std::string_view value_sv) const override final;

    std::string GetOutput(double value) const override final;
};



// --------------------------------------------------------------------------
// StringItemValueProcessor
//
// A value processor for alphanumeric items that does not use information
// from a value set.
// --------------------------------------------------------------------------

class CLASS_DECL_ZDICTO StringItemValueProcessor : public StringValueProcessor
{
protected:
    StringItemValueProcessor(const CDictItem& dict_item, const DictValueSet* dict_value_set) noexcept;

public:
    StringItemValueProcessor(const CDictItem& dict_item) noexcept;

    // ValueProcessor overrides
    bool IsValid(std::string_view value_sv, bool pad_value_to_length = true) const override;

    const DictValue* GetDictValue(std::string_view value_sv, bool pad_value_to_length = true) const override;

    std::vector<const DictValue*> GetMatchingDictValues(std::string_view value_sv) const override;

    std::string GetAlphaFromInput(std::string value) const override;

    std::string GetOutput(std::string value) const override;

    const std::vector<std::shared_ptr<const ValueSetResponse>>& GetResponses() const override;

protected:
    std::vector<std::shared_ptr<const ValueSetResponse>> m_responses;
};


// --------------------------------------------------------------------------
// StringValueSetValueProcessor
//
// A value processor for alphanumeric items that uses information from a
// value set.
// --------------------------------------------------------------------------

class CLASS_DECL_ZDICTO StringValueSetValueProcessor : public StringItemValueProcessor
{
public:
    StringValueSetValueProcessor(const CDictItem& dict_item, const DictValueSet& dict_value_set);
    ~StringValueSetValueProcessor();

    // ValueProcessor overrides
    bool IsValid(std::string_view value_sv, bool pad_value_to_length = true) const override;

    const DictValue* GetDictValue(std::string_view value_sv, bool pad_value_to_length = true) const override;

    std::vector<const DictValue*> GetMatchingDictValues(std::string_view value_sv) const override;

    const std::vector<std::shared_ptr<const ValueSetResponse>>& GetResponses() const override;

private:
    void CalculateData() const;
    void CreateData();

private:
    struct Data;
    std::unique_ptr<Data> m_data;
};
