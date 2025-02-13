#pragma once

#include <zDictO/zDictO.h>

class CDictItem;
class DictValue;
class DictValueSet;
class ValueSetResponse;
class ValueProcessorImpl;


class CLASS_DECL_ZDICTO ValueProcessor
{
protected:
    ValueProcessor(const CDictItem& dict_item, const DictValueSet* dict_value_set);
    ValueProcessor();

public:
    static std::shared_ptr<const ValueProcessor> CreateValueProcessor(const CDictItem& dict_item, const DictValueSet* dict_value_set = nullptr);

    virtual ~ValueProcessor();

    const CDictItem& GetDictItem() const;
    const DictValueSet* GetDictValueSet() const;

    // validation routines
    virtual bool IsValid(double value) const;
    virtual bool IsValid(const CString& value, bool pad_value_to_length = true) const;

    virtual const DictValue* GetDictValue(double value) const;
    virtual const DictValue* GetDictValue(const CString& value, bool pad_value_to_length = true) const;

    const DictValue* GetDictValue(const std::wstring& value, bool pad_value_to_length = true) const { return GetDictValue(WS2CS(value), pad_value_to_length); }

    virtual const DictValue* GetDictValueByLabel(const CString& label) const;

    // frequency routines
    virtual std::vector<const DictValue*> GetMatchingDictValues(double value) const;
    virtual std::vector<const DictValue*> GetMatchingDictValues(wstring_view value) const;

    // formatting routines
    double GetNumericFromInput(const CString& value) const;
    CString GetAlphaFromInput(const CString& value) const;

    CString GetOutput(double value) const;
    CString GetOutput(const CString& value) const;

    const DictValue* GetDictValueFromInput(const CString& value) const;

    const std::vector<std::shared_ptr<const ValueSetResponse>>& GetResponses() const;

protected:
    const ValueProcessorImpl& GetFullValueProcessor() const;

private:
    std::unique_ptr<ValueProcessorImpl> m_valueProcessor;
    mutable bool m_valueProcessorIsFullySetup;
};


class CLASS_DECL_ZDICTO NumericValueProcessor : public ValueProcessor
{
    friend class ValueProcessor;

private:
    NumericValueProcessor(const CDictItem& dict_item, const DictValueSet* dict_value_set);

protected:
    NumericValueProcessor();

public:
    virtual double GetMinValue() const;
    virtual double GetMaxValue() const;

    double ConvertNumberToEngineFormat(double value) const;
    double ConvertNumberFromEngineFormat(double value) const;
};
