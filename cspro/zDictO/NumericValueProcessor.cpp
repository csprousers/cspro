#include "StdAfx.h"
#include "NumericValueProcessor.h"
#include "ValueSetResponse.h"
#include <zToolsO/FloatingPointMath.h>
#include <zToolsO/VarFuncs.h>
#include <zUtilO/VectorMap.h>


// --------------------------------------------------------------------------
// NumericItemValueProcessor
// --------------------------------------------------------------------------

NumericItemValueProcessor::NumericItemValueProcessor(const CDictItem& dict_item, const DictValueSet* dict_value_set)
    :   ValueProcessorImpl(dict_item, dict_value_set),
        m_minValue(std::numeric_limits<double>::max()),
        m_maxValue(std::numeric_limits<double>::lowest())
{
}


void NumericItemValueProcessor::SetupFullValueProcessor() const
{
    hard_bounds(m_dictItem.GetCompleteLen(), m_dictItem.GetDecimal(), &m_minValue, &m_maxValue);
}


double NumericItemValueProcessor::GetMinValue() const
{
    return m_minValue;
}


double NumericItemValueProcessor::GetMaxValue() const
{
    return m_maxValue;
}


bool NumericItemValueProcessor::IsValid(const double value) const
{
    return ( ( value >= m_minValue ) && ( value <= m_maxValue ) );
}


double NumericItemValueProcessor::GetNumericFromInput(const std::string_view value_sv) const
{
    return chartodval(value, m_dictItem.GetLen(), m_dictItem.GetDecimal());
}


CString NumericItemValueProcessor::GetOutput(double value) const
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


const std::vector<std::shared_ptr<const ValueSetResponse>>& NumericItemValueProcessor::GetResponses() const
{
    ASSERT(m_responses.empty());
    return m_responses;
}



// --------------------------------------------------------------------------
// NumericValueSetValueProcessor
// --------------------------------------------------------------------------

struct NumericValueSetValueProcessor_Range
{
    double from;
    double to;
    size_t response_index;

    NumericValueSetValueProcessor_Range(const ValueSetResponse& response, const size_t response_index_)
        :   from(response.GetMinimumValue()),
            to(response.GetMaximumValue()),
            response_index(response_index_)
    {
    }
};


NumericValueSetValueProcessor::NumericValueSetValueProcessor(const CDictItem& dict_item, const DictValueSet& dict_value_set)
    :   NumericItemValueProcessor(&dict_item, &dict_value_set)
{
}


void NumericValueSetValueProcessor::SetupFullValueProcessor() const
{
    DictionaryIterator::ValueSetIterator::IterateValueSetValuePairs(*m_dictValueSet,
        [&](const DictValue& dict_value, const DictValuePair& dict_value_pair)
        {
            size_t response_index = m_responses.size();
            const ValueSetResponse& response = *m_responses.emplace_back(std::make_shared<const ValueSetResponse>(m_dictItem, dict_value, dict_value_pair));

            auto update_min_max = [&](double value)
            {
                // special values are not counted as min/max values
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


void NumericValueSetValueProcessor::SetupSpecialValue(const ValueSetResponse& response, const DictValuePair& dict_value_pair) const
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


double NumericValueSetValueProcessor::ConvertNumberToEngineFormat(const double value) const
{
    const double* converted_value;

    if( m_specialsMap.has_value() && ( converted_value = m_specialsMap->number_to_engine.Find(value) ) != nullptr )
        return *converted_value;

    return value;
}


double NumericValueSetValueProcessor::ConvertNumberFromEngineFormat(const double value) const
{
    const double* converted_value;

    if( m_specialsMap.has_value() && ( converted_value = m_specialsMap->number_from_engine.Find(value) ) != nullptr )
        return *converted_value;

    return value;
}


bool NumericValueSetValueProcessor::IsValid(const double value) const
{
    return ( GetDictValue(value) != nullptr );
}


const DictValue* NumericValueSetValueProcessor::GetDictValue(const double value) const
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


const DictValue* NumericValueSetValueProcessor::RangeSearch(const size_t right, const double value) const
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


const DictValue* NumericValueSetValueProcessor::GetDictValueWithFuzzyNoiseMatch(const double value) const
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


std::vector<const DictValue*> NumericValueSetValueProcessor::GetMatchingDictValues(const double value) const
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


double NumericValueSetValueProcessor::GetNumericFromInput(const CString& value) const
{
        const double* converted_value;

        if( m_specialsMap.has_value() && ( converted_value = m_specialsMap->special_to_number.Find(value) ) != nullptr )
            return *converted_value;

        return NumericItemValueProcessor::GetNumericFromInput(value);
}


CString NumericValueSetValueProcessor::GetOutput(const double value) const
{
    const CString* converted_value;

    if( m_specialsMap.has_value() && ( converted_value = m_specialsMap->number_to_special.Find(value) ) != nullptr )
        return *converted_value;

    return NumericItemValueProcessor::GetOutput(value);
}
