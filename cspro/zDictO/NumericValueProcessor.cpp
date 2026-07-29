#include "StdAfx.h"
#include "NumericValueProcessor.h"
#include "ValueSetResponse.h"
#include <zToolsO/FloatingPointMath.h>
#include <zToolsO/VarFuncs.h>
#include <zUtilO/VectorMap.h>


// --------------------------------------------------------------------------
// NumericValueProcessor
// --------------------------------------------------------------------------

bool NumericValueProcessor::IsValid(std::string_view /*value_sv*/, bool /*pad_value_to_length = true*/) const
{
    return ReturnProgrammingError(false);
}


const DictValue* NumericValueProcessor::GetDictValue(std::string_view /*value_sv*/, bool /*pad_value_to_length = true*/) const
{
    return ReturnProgrammingError(nullptr);
}


std::vector<const DictValue*> NumericValueProcessor::GetMatchingDictValues(std::string_view /*value_sv*/) const
{
    return ReturnProgrammingError(std::vector<const DictValue*>());
}


std::string NumericValueProcessor::GetAlphaFromInput(std::string /*value*/) const
{
    return ReturnProgrammingError(std::string());
}


const DictValue* NumericValueProcessor::GetDictValueFromInput(const std::string_view value_sv) const
{
    return GetDictValue(GetNumericFromInput(value_sv));
}


std::string NumericValueProcessor::GetOutput(std::string value) const
{
    return ReturnProgrammingError(std::move(value));
}



// --------------------------------------------------------------------------
// NumericItemValueProcessor
// --------------------------------------------------------------------------

NumericItemValueProcessor::NumericItemValueProcessor(const CDictItem* const dict_item, const DictValueSet* const dict_value_set)
    :   NumericValueProcessor(dict_item, dict_value_set),
        m_minValue(DEFAULT),
        m_maxValue(DEFAULT)
{
    ASSERT(m_dictItem != nullptr && m_dictItem->GetContentType() == ContentType::Numeric);
}


NumericItemValueProcessor::NumericItemValueProcessor(const CDictItem& dict_item)
    :   NumericValueProcessor(&dict_item, nullptr)
{
    VERIFY(hard_bounds(m_dictItem->GetCompleteLen(), m_dictItem->GetDecimal(), &m_minValue, &m_maxValue));
}


double NumericItemValueProcessor::GetMinValue() const
{
    return m_minValue;
}


double NumericItemValueProcessor::GetMaxValue() const
{
    return m_maxValue;
}


double NumericItemValueProcessor::ConvertNumberToEngineFormat(const double value) const
{
    return value;
}


double NumericItemValueProcessor::ConvertNumberFromEngineFormat(const double value) const
{
    return value;
}


bool NumericItemValueProcessor::IsValid(const double value) const
{
    return ( ( value >= m_minValue ) &&
             ( value <= m_maxValue ) );
}


const DictValue* NumericItemValueProcessor::GetDictValue(double /*value*/) const
{
    return nullptr;
}


std::vector<const DictValue*> NumericItemValueProcessor::GetMatchingDictValues(double /*value*/) const
{
    return { };
}


double NumericItemValueProcessor::GetNumericFromInput(const std::string_view value_sv) const
{
    return chartodval(UTF8_TODO::GetCString(value_sv), m_dictItem->GetLen(), m_dictItem->GetDecimal());
}


std::string NumericItemValueProcessor::GetOutput(double value) const
{
    // BLOCK_TODO ... look at all the places NOTAPPL->MASKBLK and MASKBLK->NOTAPPL;
    // can these be minimized, or can MASKBLK be removed?

    if( value == NOTAPPL )
        value = MASKBLK;

    return dvaltochar<std::string>(value, m_dictItem->GetLen(), m_dictItem->GetDecimal(),
                                   m_dictItem->GetZeroFill(), m_dictItem->GetDecChar());
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


struct NumericValueSetValueProcessor_SpecialsMap
{
    VectorMap<double, double> number_to_engine;
    VectorMap<double, double> number_from_engine;
    VectorMap<std::string, double> special_to_number;
    VectorMap<double, std::string> number_to_special;
};


struct NumericValueSetValueProcessor::Data
{
    VectorMap<double, size_t> discretes;
    std::vector<NumericValueSetValueProcessor_Range> ranges;

    std::unique_ptr<NumericValueSetValueProcessor_SpecialsMap> specials_map;
};


NumericValueSetValueProcessor::NumericValueSetValueProcessor(const CDictItem& dict_item, const DictValueSet& dict_value_set)
    :   NumericItemValueProcessor(&dict_item, &dict_value_set)
{
}


NumericValueSetValueProcessor::~NumericValueSetValueProcessor()
{
}


inline void NumericValueSetValueProcessor::CalculateData() const
{
    if( m_data == nullptr )
        const_cast<NumericValueSetValueProcessor*>(this)->CreateData();
}


void NumericValueSetValueProcessor::CreateData()
{
    ASSERT(m_data == nullptr && m_responses.empty());
    ASSERT(m_minValue == DEFAULT && m_maxValue == DEFAULT);

    m_data = std::make_unique<Data>();

    bool first_min_max_update = true;

    auto update_min_max = [&](double value)
    {
        // special values are not counted as min/max values
        if( !IsSpecial(value) )
        {
            if( first_min_max_update || value < m_minValue )
                m_minValue = value;

            if( first_min_max_update || value > m_maxValue )
                m_maxValue = value;

            first_min_max_update = false;
        }
    };

    DictionaryIterator::ValueSetIterator::IterateValueSetValuePairs(*m_dictValueSet,
        [&](const DictValue& dict_value, const DictValuePair& dict_value_pair)
        {
            const size_t response_index = m_responses.size();

            const ValueSetResponse& response = *m_responses.emplace_back(
                std::make_unique<const ValueSetResponse>(*m_dictItem, dict_value, dict_value_pair)
            );

            update_min_max(response.GetMinimumValue());

            // process discrete values
            if( response.IsDiscrete() )
            {
                m_data->discretes.Insert(response.GetMinimumValue(), response_index);
            }

            // process ranges
            else
            {
                m_data->ranges.emplace_back(response, response_index);
                update_min_max(response.GetMaximumValue());
            }

            // process special values
            if( dict_value.IsSpecial() )
            {
                if( m_data->specials_map == nullptr )
                    m_data->specials_map = std::make_unique<NumericValueSetValueProcessor_SpecialsMap>();

                SetUpSpecialValue(*m_data->specials_map, response, dict_value_pair);
            }
        });

    // sort the ranges
    std::sort(m_data->ranges.begin(), m_data->ranges.end(),
              [](const auto& lhs, const auto& rhs) { return ( lhs.from < rhs.from ); });
}


template<typename SpecialsMapT>
void NumericValueSetValueProcessor::SetUpSpecialValue(SpecialsMapT& specials_map, const ValueSetResponse& response,
                                                      const DictValuePair& dict_value_pair) const
{
    std::string from_value = dict_value_pair.GetFrom();
    const double engine_value = response.GetMinimumValue();

    std::string_view trimmed_from_value_sv = SO::Trim(from_value);

    // add numeric values to the double <-> double maps
    if( CIMSAString::IsNumeric(trimmed_from_value_sv, false) )
    {
        const double display_value = atod(trimmed_from_value_sv);

        specials_map.number_to_engine.Insert(display_value, engine_value);
        specials_map.number_from_engine.Insert(engine_value, display_value);

        // format the value because some values (e.g., numerics with implied decimals)
        // are not stored in "saving" format by the dictionary editor
        from_value = NumericItemValueProcessor::GetOutput(display_value);
    }

    ASSERT(from_value.length() == m_dictItem->GetLen());

    // add all special values to the double <-> text maps
    specials_map.special_to_number.Insert(from_value, engine_value);
    specials_map.number_to_special.Insert(engine_value, from_value);
}


double NumericValueSetValueProcessor::GetMinValue() const
{
    CalculateData();

    return m_minValue;
}


double NumericValueSetValueProcessor::GetMaxValue() const
{
    CalculateData();

    return m_maxValue;
}


template<typename RT, typename VT, typename SpecialsMapObjectT>
const RT* NumericValueSetValueProcessor::LookupSpecialMapping(const VT& value, const SpecialsMapObjectT map_ptr) const
{
    CalculateData();

    if( m_data->specials_map != nullptr )
    {
        const auto& map = (*m_data->specials_map).*map_ptr;
        return map.Find(value);
    }

    return nullptr;
}


double NumericValueSetValueProcessor::ConvertNumberToEngineFormat(const double value) const
{
    const double* const converted_value = LookupSpecialMapping<double>(value, &NumericValueSetValueProcessor_SpecialsMap::number_to_engine);

    return ( converted_value != nullptr ) ? *converted_value :
                                            value;
}


double NumericValueSetValueProcessor::ConvertNumberFromEngineFormat(const double value) const
{
    const double* const converted_value = LookupSpecialMapping<double>(value, &NumericValueSetValueProcessor_SpecialsMap::number_from_engine);

    return ( converted_value != nullptr ) ? *converted_value :
                                            value;
}


bool NumericValueSetValueProcessor::IsValid(const double value) const
{
    return ( GetDictValue(value) != nullptr );
}


const DictValue* NumericValueSetValueProcessor::GetDictValue(const double value) const
{
    CalculateData();

    const DictValue* dict_value;

    // check discrete values
    const size_t* const discrete_index = m_data->discretes.Find(value);

    if( discrete_index != nullptr )
    {
        dict_value = m_responses[*discrete_index]->GetDictValue();
    }

    // check ranges
    else if( !m_data->ranges.empty() && ( value >= m_minValue && value <= m_maxValue ) )
    {
        dict_value = RangeSearch(m_data->ranges.size() - 1, value);
    }

    else
    {
        dict_value = nullptr;
    }

    // if not found, check the discretes with some fuzzy noise matching
    if( dict_value == nullptr && !m_data->discretes.GetVector().empty() && !IsSpecial(value) )
        dict_value = GetDictValueWithFuzzyNoiseMatch(value);

    return dict_value;
}


const DictValue* NumericValueSetValueProcessor::RangeSearch(const size_t right, const double value) const
{
    ASSERT(m_data != nullptr);

    // given that the ranges are sorted only on the lower value, we first do a
    // modified binary search to get a good starting position to begin the search
    if( right != 0 )
    {
        const size_t middle = ( right - 1 ) / 2;
        const NumericValueSetValueProcessor_Range& middle_range = m_data->ranges.at(middle);

        // if the value is less than the lower value in the range, then keep searching to the left
        if( value < middle_range.from )
        {
            if( middle != 0 )
                return RangeSearch(middle - 1, value);
        }
    }

    // otherwise, search linearly for the value
    for( const NumericValueSetValueProcessor_Range& this_range : m_data->ranges )
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
    ASSERT(m_data != nullptr);

    // floating point precision issues may result in a value not being matched, so check for
    // values with some imprecision; this was reported as a bug for discrete values, so this
    // algorithm only checks discretes, but could later be extended to do the same for ranges
    const std::vector<std::tuple<double, size_t>>& discretes = m_data->discretes.GetVector();
    ASSERT(!discretes.empty() && !IsSpecial(value));

    const auto& upper_bound_itr = std::upper_bound(discretes.cbegin(), discretes.cend(), value,
        [](const double value, const auto& key_value_pair)
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
            matches = ( value >= response.GetMinimumValue() &&
                        value <= response.GetMaximumValue() );
        }

        if( matches )
            dict_values.emplace_back(response.GetDictValue());
    }

    return dict_values;
}


double NumericValueSetValueProcessor::GetNumericFromInput(const std::string_view value_sv) const
{
    const double* const converted_value = LookupSpecialMapping<double>(value_sv, &NumericValueSetValueProcessor_SpecialsMap::special_to_number);

    if( converted_value != nullptr )
        return *converted_value;

    return NumericItemValueProcessor::GetNumericFromInput(value_sv);
}


std::string NumericValueSetValueProcessor::GetOutput(const double value) const
{
    const std::string* const converted_value = LookupSpecialMapping<std::string>(value, &NumericValueSetValueProcessor_SpecialsMap::number_to_special);

    if( converted_value != nullptr )
        return *converted_value;

    return NumericItemValueProcessor::GetOutput(value);
}


const std::vector<std::shared_ptr<const ValueSetResponse>>& NumericValueSetValueProcessor::GetResponses() const
{
    CalculateData();

    return m_responses;
}
