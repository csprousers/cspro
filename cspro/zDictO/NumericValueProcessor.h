#pragma once

#include <zDictO/zDictO.h>
#include <zDictO/ValueProcessor.h>


// --------------------------------------------------------------------------
// NumericValueProcessor
//
// The base class for the value processor of numeric values.
// --------------------------------------------------------------------------

class CLASS_DECL_ZDICTO NumericValueProcessor : public ValueProcessor
{
protected:
    using ValueProcessor::ValueProcessor;

public:
    // Returns the minimum valid value.
    virtual double GetMinValue() const = 0;

    // Returns the maximum valid value.
    virtual double GetMaxValue() const = 0;

    // Converts a number to its engine format.
    // For example, if -99 is mapped to MISSING, an input of -99 would return MISSING.
    virtual double ConvertNumberToEngineFormat(double value) const = 0;

    // Converts a number from its engine format.
    // For example, if -99 is mapped to MISSING, an input of MISSING would return -99.
    virtual double ConvertNumberFromEngineFormat(double value) const = 0;

    // ValueProcessor overrides
    using ValueProcessor::GetDictValue;

    const DictValue* GetDictValueFromInput(std::string_view value_sv) const override final;

    using ValueProcessor::GetOutput;

private:
    bool IsValid(std::string_view value_sv, bool pad_value_to_length = true) const override final;

    const DictValue* GetDictValue(std::string_view value_sv, bool pad_value_to_length = true) const override final;

    std::vector<const DictValue*> GetMatchingDictValues(std::string_view value_sv) const override final;

    std::string GetAlphaFromInput(std::string value) const override final;

    std::string GetOutput(std::string value) const override final;
};



// --------------------------------------------------------------------------
// NumericItemValueProcessor
//
// A value processor for numeric items that does not use information from a
// value set.
// --------------------------------------------------------------------------

class CLASS_DECL_ZDICTO NumericItemValueProcessor : public NumericValueProcessor
{
protected:
    NumericItemValueProcessor(const CDictItem* dict_item, const DictValueSet* dict_value_set);

public:
    NumericItemValueProcessor(const CDictItem& dict_item);

    // NumericValueProcessor overrides
    double GetMinValue() const override;
    double GetMaxValue() const override;

    double ConvertNumberToEngineFormat(double value) const override;
    double ConvertNumberFromEngineFormat(double value) const override;

    // ValueProcessor overrides
    bool IsValid(double value) const override;

    const DictValue* GetDictValue(double value) const override;

    std::vector<const DictValue*> GetMatchingDictValues(double value) const override;

    double GetNumericFromInput(std::string_view value_sv) const override;

    std::string GetOutput(double value) const override;

    const std::vector<std::shared_ptr<const ValueSetResponse>>& GetResponses() const override;

protected:
    double m_minValue;
    double m_maxValue;
    std::vector<std::shared_ptr<const ValueSetResponse>> m_responses;
};


// --------------------------------------------------------------------------
// NumericValueSetValueProcessor
//
// A value processor for numeric value sets that uses information from a
// value set.
// --------------------------------------------------------------------------

class CLASS_DECL_ZDICTO NumericValueSetValueProcessor : public NumericItemValueProcessor
{
public:
    NumericValueSetValueProcessor(const CDictItem& dict_item, const DictValueSet& dict_value_set);
    ~NumericValueSetValueProcessor();

    // NumericValueProcessor overrides
    double GetMinValue() const override;
    double GetMaxValue() const override;

    double ConvertNumberToEngineFormat(double value) const override;
    double ConvertNumberFromEngineFormat(double value) const override;

    // ValueProcessor overrides
    bool IsValid(double value) const override;

    const DictValue* GetDictValue(double value) const override;

    std::vector<const DictValue*> GetMatchingDictValues(double value) const override;

    double GetNumericFromInput(std::string_view value_sv) const override;

    std::string GetOutput(double value) const override;

    const std::vector<std::shared_ptr<const ValueSetResponse>>& GetResponses() const override;

private:
    void CalculateData() const;
    void CreateData();

    template<typename SpecialsMapT>
    void SetUpSpecialValue(SpecialsMapT& specials_map, const ValueSetResponse& response, const DictValuePair& dict_value_pair) const;

    template<typename RT, typename VT, typename SpecialsMapObjectT>
    const RT* LookupSpecialMapping(const VT& value, SpecialsMapObjectT map_ptr) const;

    const DictValue* RangeSearch(size_t right, double value) const;
    const DictValue* GetDictValueWithFuzzyNoiseMatch(double value) const;

private:
    struct Data;
    std::unique_ptr<Data> m_data;
};
