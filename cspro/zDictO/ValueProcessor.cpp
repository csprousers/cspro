#include "StdAfx.h"
#include "ValueProcessor.h"
#include "ValueSetResponse.h"
#include <zToolsO/VarFuncs.h>
#include <zUtilO/VectorMap.h>
#include <engine/EngineFloatingPointMath.h>


class ValueProcessorImpl
{
public:
    ValueProcessorImpl(const CDictItem& dict_item, const DictValueSet* dict_value_set)
        :   m_dictItem(dict_item),
            m_dictValueSet(dict_value_set)
    {
    }

    virtual ~ValueProcessorImpl()
    {
    }

    const CDictItem& GetDictItem() const
    {
        return m_dictItem;
    }

    const DictValueSet* GetDictValueSet() const
    {
        return m_dictValueSet;
    }

    virtual double GetMinValue() const
    {
        ASSERT(false);
        return 0;
    }

    virtual double GetMaxValue() const
    {
        ASSERT(false);
        return 0;
    }

    virtual bool IsValid(double /*value*/) const
    {
        ASSERT(false);
        return false;
    }

    virtual bool IsValid(const CString& /*value*/, bool /*pad_value_to_length*/) const
    {
        ASSERT(false);
        return false;
    }

    virtual const DictValue* GetDictValue(double /*value*/) const
    {
        return nullptr;
    }

    virtual const DictValue* GetDictValue(const CString& /*value*/, bool /*pad_value_to_length*/) const
    {
        return nullptr;
    }

    const DictValue* GetDictValueByLabel(const CString& label) const
    {
        if( m_dictValueSet != nullptr )
        {
            for( const DictValue& dict_value : m_dictValueSet->GetValues() )
            {
                if( dict_value.GetLabel().CompareNoCase(label) == 0 )
                    return &dict_value;
            }
        }

        return nullptr;
    }

    virtual std::vector<const DictValue*> GetMatchingDictValues(double /*value*/) const
    {
        return { };
    }

    virtual std::vector<const DictValue*> GetMatchingDictValues(wstring_view /*value_sv*/) const
    {
        return { };
    }

    virtual double GetNumericFromInput(const CString& /*value*/) const
    {
        ASSERT(false);
        return DEFAULT;
    }

    virtual CString GetAlphaFromInput(const CString& value) const
    {
        ASSERT(false);
        return value;
    }

    virtual CString GetOutput(double /*value*/) const
    {
        ASSERT(false);
        return CString();
    }

    virtual CString GetOutput(const CString& value) const
    {
        ASSERT(false);
        return value;
    }

    const DictValue* GetDictValueFromInput(const CString& value) const
    {
        if( m_dictItem.GetContentType() == ContentType::Numeric )
        {
            return GetDictValue(GetNumericFromInput(value));
        }

        else if( m_dictItem.GetContentType() == ContentType::Alpha )
        {
            return GetDictValue(GetAlphaFromInput(value), true);
        }

        else
        {
            CONTENT_TYPE_REFACTOR::LOOK_AT_AND_THROW("what to do about non-numeric + non-alpha fields?");
        }
    }

    const std::vector<std::shared_ptr<const ValueSetResponse>>& GetResponses() const
    {
        return m_responses;
    }

    virtual double ConvertNumberToEngineFormat(double value) const
    {
        ASSERT(m_dictItem.GetContentType() == ContentType::Numeric);
        return value;
    }

    virtual double ConvertNumberFromEngineFormat(double value) const
    {
        ASSERT(m_dictItem.GetContentType() == ContentType::Numeric);
        return value;
    }

    virtual void SetupFullValueProcessor() const
    {
    }

protected:
    const CDictItem& m_dictItem;
    const DictValueSet* m_dictValueSet;
    mutable std::vector<std::shared_ptr<const ValueSetResponse>> m_responses;
};



// --------------------------------------------------------------------------
// AlphaItemValueProcessor
// --------------------------------------------------------------------------

class AlphaItemValueProcessor : public ValueProcessorImpl
{
public:
    AlphaItemValueProcessor(const CDictItem& dict_item, const DictValueSet* dict_value_set)
        :   ValueProcessorImpl(dict_item, dict_value_set)
    {
    }

    bool IsValid(const CString& value, bool pad_value_to_length) const override
    {
        // if the value is too long, it is not valid; if it is too short, it is only valid if the
        // padding setting is true
        int length_difference = (int)m_dictItem.GetLen() - value.GetLength();
        return ( length_difference == 0 || ( pad_value_to_length && length_difference > 0 ) );
    }

    CString GetAlphaFromInput(const CString& value) const override
    {
        return CIMSAString::MakeExactLength(value, m_dictItem.GetLen());
    }

    CString GetOutput(const CString& value) const override
    {
        return CIMSAString::MakeExactLength(value, m_dictItem.GetLen());
    }
};



// --------------------------------------------------------------------------
// AlphaValueSetValueProcessor
// --------------------------------------------------------------------------

class AlphaValueSetValueProcessor : public AlphaItemValueProcessor
{
public:
    AlphaValueSetValueProcessor(const CDictItem& dict_item, const DictValueSet* dict_value_set)
        :   AlphaItemValueProcessor(dict_item, dict_value_set)
    {
    }

    bool IsValid(const CString& value, bool pad_value_to_length) const override
    {
        return ( GetDictValue(value, pad_value_to_length) != nullptr );
    }

    const DictValue* GetDictValue(const CString& value, bool pad_value_to_length) const override
    {
        // first check the length of the string
        if( AlphaItemValueProcessor::IsValid(value, pad_value_to_length) )
        {
            // and then check if the string is in the value set
            const size_t* alphas_index = m_alphas.Find(value);

            // for CSPro 8.0, the codes are not necessarily right-padded, so if not found,
            // search ignoring whitespace at the end ... ENGINECR_TODO: revisit this ... this check was put in for field validation in 8.0, but wasn't necessary in 7.7
            if( alphas_index == nullptr )
            {
                const wstring_view trimmed_value_sv = SO::TrimRightSpace(value);

                for( const auto& [code, index] : m_alphas.GetVector() )
                {
                    if( SO::StartsWith(code, trimmed_value_sv) && SO::TrimRightSpace(code).length() == trimmed_value_sv.length() )
                    {
                        alphas_index = &index;
                        break;
                    }
                }
            }

            if( alphas_index != nullptr )
                return m_responses[*alphas_index]->GetDictValue();
        }

        return nullptr;
    }

    std::vector<const DictValue*> GetMatchingDictValues(wstring_view value_sv) const override
    {
        std::vector<const DictValue*> dict_values;

        // right-trim the value (to match the storage of the response codes)
        value_sv = SO::TrimRightSpace(value_sv);

        for( const ValueSetResponse& response : VI_V(m_responses) )
        {
            if( SO::Equals(response.GetCode(), value_sv) )
                dict_values.emplace_back(response.GetDictValue());
        }

        return dict_values;
    }

    void SetupFullValueProcessor() const override
    {
        DictionaryIterator::ValueSetIterator::IterateValueSetValuePairs(*m_dictValueSet,
            [&](const DictValue& dict_value, const DictValuePair& dict_value_pair)
            {
                size_t response_index = m_responses.size();
                m_responses.emplace_back(std::make_shared<ValueSetResponse>(m_dictItem, dict_value, dict_value_pair));
                m_alphas.Insert(dict_value_pair.GetFrom(), response_index);
            });
    }

private:
    mutable VectorMap<CString, size_t> m_alphas;
};



// --------------------------------------------------------------------------
// NumericItemValueProcessor
// --------------------------------------------------------------------------

class NumericItemValueProcessor : public ValueProcessorImpl
{
public:
    NumericItemValueProcessor(const CDictItem& dict_item, const DictValueSet* dict_value_set)
        :   ValueProcessorImpl(dict_item, dict_value_set),
            m_minValue(std::numeric_limits<double>::max()),
            m_maxValue(std::numeric_limits<double>::lowest())
    {
    }

    double GetMinValue() const override
    {
        return m_minValue;
    }

    double GetMaxValue() const override
    {
        return m_maxValue;
    }

    bool IsValid(double value) const override
    {
        return ( ( value >= m_minValue ) && ( value <= m_maxValue ) );
    }

    void SetupFullValueProcessor() const override
    {
        hard_bounds(m_dictItem.GetCompleteLen(), m_dictItem.GetDecimal(), &m_minValue, &m_maxValue);
    }

    double GetNumericFromInput(const CString& value) const override
    {
        return chartodval(value, m_dictItem.GetLen(), m_dictItem.GetDecimal());
    }

    CString GetOutput(double value) const override
    {
        CString string_value;
        TCHAR* buffer = string_value.GetBufferSetLength(m_dictItem.GetLen());

        // BLOCK_TODO ... look at all the places NOTAPPL->MASKBLK and MASKBLK->NOTAPPL;
        // can these be minimized, or can MASKBLK be removed?

        if( value == NOTAPPL )
            value = MASKBLK;

        dvaltochar(value, buffer, m_dictItem.GetLen(), m_dictItem.GetDecimal(),
            m_dictItem.GetZeroFill(), m_dictItem.GetDecChar());

        string_value.ReleaseBuffer();

        return string_value;
    }

protected:
    mutable double m_minValue;
    mutable double m_maxValue;
};



// --------------------------------------------------------------------------
// NumericValueSetValueProcessor
// --------------------------------------------------------------------------

struct NumericValueSetValueProcessorRange
{
    double from;
    double to;
    size_t response_index;

    NumericValueSetValueProcessorRange(const ValueSetResponse& response, size_t response_index_)
        :   from(response.GetMinimumValue()),
            to(response.GetMaximumValue()),
            response_index(response_index_)
    {
    }
};


class NumericValueSetValueProcessor : public NumericItemValueProcessor
{
public:
    NumericValueSetValueProcessor(const CDictItem& dict_item, const DictValueSet* dict_value_set)
        :   NumericItemValueProcessor(dict_item, dict_value_set)
    {
    }

    bool IsValid(double value) const override
    {
        return ( GetDictValue(value) != nullptr );
    }

    const DictValue* GetDictValue(double value) const override
    {
        const DictValue* dict_value = nullptr;

        // check discrete values
        size_t* discrete_index = m_discretes.Find(value);

        if( discrete_index != nullptr )
        {
            dict_value = m_responses[*discrete_index]->GetDictValue();
        }

        // check ranges
        else if( !m_ranges.empty() && ( value >= m_minValue && value <= m_maxValue ) )
        {
            dict_value = RangeSearch(m_ranges.size() - 1, value);
        }

        // if not found, check the discretes with some fuzzy noise matching
        if( dict_value == nullptr && !m_discretes.GetVector().empty() && !IsSpecial(value) )
            dict_value = GetDictValueWithFuzzyNoiseMatch(value);

        return dict_value;                                           
    }

    std::vector<const DictValue*> GetMatchingDictValues(double value) const override
    {
        std::vector<const DictValue*> dict_values;

        for( const ValueSetResponse& response : VI_V(m_responses) )
        {
            bool matches;

            if( response.IsDiscrete() )
            {
                matches = ( value == response.GetMinimumValue() );
            }

            else
            {
                matches = ( value >= response.GetMinimumValue() && value <= response.GetMaximumValue() );
            }

            if( matches )
                dict_values.emplace_back(response.GetDictValue());
        }

        return dict_values;
    }

    void SetupFullValueProcessor() const override
    {
        DictionaryIterator::ValueSetIterator::IterateValueSetValuePairs(*m_dictValueSet,
            [&](const DictValue& dict_value, const DictValuePair& dict_value_pair)
            {
                size_t response_index = m_responses.size();
                const ValueSetResponse& response = *m_responses.emplace_back(std::make_shared<const ValueSetResponse>(m_dictItem, dict_value, dict_value_pair));

                auto update_min_max = [&](double value)
                {
                    // special values won't be counted as min/max values
                    if( !IsSpecial(value) )
                    {
                        if( value < m_minValue )
                            m_minValue = value;

                        if( value > m_maxValue )
                            m_maxValue = value;
                    }
                };

                update_min_max(response.GetMinimumValue());

                // process discrete values
                if( response.IsDiscrete() )
                {
                    m_discretes.Insert(response.GetMinimumValue(), response_index);
                }

                // process ranges
                else
                {
                    m_ranges.emplace_back(response, response_index);
                    update_min_max(response.GetMaximumValue());
                }

                // process special values
                if( dict_value.IsSpecial() )
                    SetupSpecialValue(response, dict_value_pair);
            });

        // sort the ranges
        std::sort(m_ranges.begin(), m_ranges.end(), [](const auto& lhs, const auto& rhs) { return ( lhs.from < rhs.from ); });

        // if the min and max values weren't modified (because the value set only had special values),
        // make the values DEFAULT
        if( m_minValue == std::numeric_limits<double>::max() )
        {
            m_minValue = DEFAULT;
            m_maxValue = DEFAULT;
        }
    }

    double ConvertNumberToEngineFormat(double value) const override
    {
        const double* converted_value;

        if( m_specialsMap.has_value() && ( converted_value = m_specialsMap->number_to_engine.Find(value) ) != nullptr )
            return *converted_value;

        return value;
    }

    double ConvertNumberFromEngineFormat(double value) const override
    {
        const double* converted_value;

        if( m_specialsMap.has_value() && ( converted_value = m_specialsMap->number_from_engine.Find(value) ) != nullptr )
            return *converted_value;

        return value;
    }

    double GetNumericFromInput(const CString& value) const override
    {
        const double* converted_value;

        if( m_specialsMap.has_value() && ( converted_value = m_specialsMap->special_to_number.Find(value) ) != nullptr )
            return *converted_value;

        return NumericItemValueProcessor::GetNumericFromInput(value);
    }

    CString GetOutput(double value) const override
    {
        const CString* converted_value;

        if( m_specialsMap.has_value() && ( converted_value = m_specialsMap->number_to_special.Find(value) ) != nullptr )
            return *converted_value;

        return NumericItemValueProcessor::GetOutput(value);
    }

private:
    const DictValue* RangeSearch(size_t right, double value) const
    {
        // given that the ranges are sorted only on the lower value, we first do a
        // modified binary search to get a good starting position to begin the search
        if( right != 0 )
        {
            size_t middle = ( right - 1 ) / 2;
            const NumericValueSetValueProcessorRange& middle_range = m_ranges.at(middle);

            // if the value is less than the lower value in the range, then keep searching to the left
            if( value < middle_range.from )
            {
                if( middle != 0 )
                    return RangeSearch(middle - 1, value);
            }
        }

        // otherwise, search linearly for the value
        for( const NumericValueSetValueProcessorRange& this_range : m_ranges )
        {
            // if the value is less than the lower value, the value doesn't exist in the ranges
            if( value < this_range.from )
            {
                break;
            }

            // check if the value is in this range
            else if( value >= this_range.from && value <= this_range.to )
            {
                return m_responses[this_range.response_index]->GetDictValue();
            }
        }

        return nullptr;
    }

    void SetupSpecialValue(const ValueSetResponse& response, const DictValuePair& dict_value_pair) const
    {
        if( !m_specialsMap.has_value() )
            m_specialsMap = SpecialsMap();

        CString from_value = dict_value_pair.GetFrom();
        double engine_value = response.GetMinimumValue();

        CString trimmed_from_value = SO::Trim(from_value);

        // add numeric values to the double <-> double maps
        if( CIMSAString::IsNumeric(trimmed_from_value, false) )
        {
            double display_value = atod(trimmed_from_value);

            m_specialsMap->number_to_engine.Insert(display_value, engine_value);
            m_specialsMap->number_from_engine.Insert(engine_value, display_value);

            // format the value because some values (e.g., numerics with implied decimals)
            // are not stored in "saving" format by the dictionary editor
            from_value = NumericItemValueProcessor::GetOutput(display_value);
        }

        ASSERT(from_value.GetLength() == (int)m_dictItem.GetLen());

        // add all special values to the double <-> text maps
        m_specialsMap->special_to_number.Insert(from_value, engine_value);
        m_specialsMap->number_to_special.Insert(engine_value, from_value);
    }

    const DictValue* GetDictValueWithFuzzyNoiseMatch(double value) const
    {
        // floating point precision issues may result in a value not being matched, so check for 
        // values with some imprecision; this was reported as a bug for discrete values, so this
        // algorithm only checks discretes, but could later be extended to do the same for ranges
        const std::vector<std::tuple<double, size_t>>& discretes = m_discretes.GetVector();
        ASSERT(!discretes.empty() && !IsSpecial(value));

        const auto& upper_bound_itr = std::upper_bound(discretes.cbegin(), discretes.cend(), value,
            [](double value, const auto& key_value_pair)
            {
                return ( value < std::get<0>(key_value_pair) );
            });

        const std::tuple<double, size_t>* lower_than_entry;

        // if a value greater than this value exists, check if it is equal
        if( upper_bound_itr != discretes.cend() )
        {
            if( FloatingPointMath::Equals(value, std::get<0>(*upper_bound_itr)) )
                return m_responses[std::get<1>(*upper_bound_itr)]->GetDictValue();

            if( upper_bound_itr != discretes.cbegin() )
            {
                lower_than_entry = &(*upper_bound_itr) - 1;
            }

            else
            {
                return nullptr;
            }
        }

        else
        {
            lower_than_entry = &discretes.back();
        }

        // otherwise check the value less than this value
        if( FloatingPointMath::Equals(value, std::get<0>(*lower_than_entry)) )
            return m_responses[std::get<1>(*lower_than_entry)]->GetDictValue();

        return nullptr;
    }

private:
    mutable VectorMap<double, size_t> m_discretes;
    mutable std::vector<NumericValueSetValueProcessorRange> m_ranges;

    struct SpecialsMap
    {
        VectorMap<double, double> number_to_engine;
        VectorMap<double, double> number_from_engine;
        VectorMap<CString, double> special_to_number;
        VectorMap<double, CString> number_to_special;
    };

    mutable std::optional<SpecialsMap> m_specialsMap;
};



// --------------------------------------------------------------------------
// ValueProcessor
// --------------------------------------------------------------------------

ValueProcessor::ValueProcessor(const CDictItem& dict_item, const DictValueSet* dict_value_set)
    :   m_valueProcessorIsFullySetup(false)
{
    if( dict_item.GetContentType() == ContentType::Numeric )
    {
        m_valueProcessor.reset(( dict_value_set == nullptr ) ?
            static_cast<ValueProcessorImpl*>(new NumericItemValueProcessor(dict_item, dict_value_set)) :
            static_cast<ValueProcessorImpl*>(new NumericValueSetValueProcessor(dict_item, dict_value_set)));
    }

    else if( dict_item.GetContentType() == ContentType::Alpha )
    {
        m_valueProcessor.reset(( dict_value_set == nullptr ) ?
            static_cast<ValueProcessorImpl*>(new AlphaItemValueProcessor(dict_item, dict_value_set)) :
            static_cast<ValueProcessorImpl*>(new AlphaValueSetValueProcessor(dict_item, dict_value_set)));
    }

    else
    {
        CONTENT_TYPE_REFACTOR::LOOK_AT("what to do about non-numeric + non-alpha fields?");
    }
}


ValueProcessor::ValueProcessor()
    :   m_valueProcessorIsFullySetup(true)
{
    // this is used by the DynamicValueSet
}


std::shared_ptr<const ValueProcessor> ValueProcessor::CreateValueProcessor(const CDictItem& dict_item, const DictValueSet* dict_value_set/* = nullptr*/)
{
    std::shared_ptr<const ValueProcessor> value_processor;

    // if the value processor has already been setup for a value set, reuse it
    if( dict_value_set != nullptr )
        value_processor = dict_value_set->GetSharedValueProcessor();

    if( value_processor == nullptr )
    {
        if( dict_item.GetContentType() == ContentType::Numeric )
        {
            value_processor.reset(new NumericValueProcessor(dict_item, dict_value_set));
        }

        else if( dict_item.GetContentType() == ContentType::Alpha )
        {
            value_processor.reset(new ValueProcessor(dict_item, dict_value_set));
        }

        else
        {
            CONTENT_TYPE_REFACTOR::LOOK_AT("what to do about non-numeric + non-alpha fields?");
        }

        if( dict_value_set != nullptr )
            const_cast<DictValueSet*>(dict_value_set)->SetValueProcessor(value_processor);
    }

    return value_processor;
}


ValueProcessor::~ValueProcessor()
{
}


const CDictItem& ValueProcessor::GetDictItem() const
{
    return m_valueProcessor->GetDictItem();
}


const DictValueSet* ValueProcessor::GetDictValueSet() const
{
    return m_valueProcessor->GetDictValueSet();
}


const ValueProcessorImpl& ValueProcessor::GetFullValueProcessor() const
{
    if( !m_valueProcessorIsFullySetup )
    {
        m_valueProcessor->SetupFullValueProcessor();
        m_valueProcessorIsFullySetup = true;
    }

    return *m_valueProcessor;
}


bool ValueProcessor::IsValid(double value) const
{
    return GetFullValueProcessor().IsValid(value);
}


bool ValueProcessor::IsValid(const CString& value, bool pad_value_to_length/* = true*/) const
{
    return GetFullValueProcessor().IsValid(value, pad_value_to_length);
}


const DictValue* ValueProcessor::GetDictValue(double value) const
{
    return GetFullValueProcessor().GetDictValue(value);
}


const DictValue* ValueProcessor::GetDictValue(const CString& value, bool pad_value_to_length/* = true*/) const
{
    return GetFullValueProcessor().GetDictValue(value, pad_value_to_length);
}


const DictValue* ValueProcessor::GetDictValueByLabel(const CString& label) const
{
    return GetFullValueProcessor().GetDictValueByLabel(label);
}


std::vector<const DictValue*> ValueProcessor::GetMatchingDictValues(double value) const
{
    return GetFullValueProcessor().GetMatchingDictValues(value);
}


std::vector<const DictValue*> ValueProcessor::GetMatchingDictValues(wstring_view value_sv) const
{
    return GetFullValueProcessor().GetMatchingDictValues(value_sv);
}


double ValueProcessor::GetNumericFromInput(const CString& value) const
{
    return GetFullValueProcessor().GetNumericFromInput(value);
}


CString ValueProcessor::GetAlphaFromInput(const CString& value) const
{
    return GetFullValueProcessor().GetAlphaFromInput(value);
}


CString ValueProcessor::GetOutput(double value) const
{
    return GetFullValueProcessor().GetOutput(value);
}


CString ValueProcessor::GetOutput(const CString& value) const
{
    return GetFullValueProcessor().GetOutput(value);
}


const DictValue* ValueProcessor::GetDictValueFromInput(const CString& value) const
{
    return GetFullValueProcessor().GetDictValueFromInput(value);
}


const std::vector<std::shared_ptr<const ValueSetResponse>>& ValueProcessor::GetResponses() const
{
    return GetFullValueProcessor().GetResponses();
}



// --------------------------------------------------------------------------
// NumericValueProcessor
// --------------------------------------------------------------------------

NumericValueProcessor::NumericValueProcessor(const CDictItem& dict_item, const DictValueSet* dict_value_set)
    :   ValueProcessor(dict_item, dict_value_set)
{
}


NumericValueProcessor::NumericValueProcessor()
{
    // this is used by the DynamicValueSet
}


double NumericValueProcessor::GetMinValue() const
{
    return GetFullValueProcessor().GetMinValue();
}


double NumericValueProcessor::GetMaxValue() const
{
    return GetFullValueProcessor().GetMaxValue();
}


double NumericValueProcessor::ConvertNumberToEngineFormat(double value) const
{
    return GetFullValueProcessor().ConvertNumberToEngineFormat(value);
}


double NumericValueProcessor::ConvertNumberFromEngineFormat(double value) const
{
    return GetFullValueProcessor().ConvertNumberFromEngineFormat(value);
}
