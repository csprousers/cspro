#include "StdAfx.h"
#include "StringValueProcessor.h"
#include "ValueSetResponse.h"
#include <zUtilO/VectorMap.h>


// --------------------------------------------------------------------------
// StringValueProcessor
// --------------------------------------------------------------------------

bool StringValueProcessor::IsValid(double /*value*/) const
{
    return ReturnProgrammingError(false);
}


const DictValue* StringValueProcessor::GetDictValue(double /*value*/) const
{
    return ReturnProgrammingError(nullptr);
}


std::vector<const DictValue*> StringValueProcessor::GetMatchingDictValues(double /*value*/) const
{
    return ReturnProgrammingError(std::vector<const DictValue*>());
}


double StringValueProcessor::GetNumericFromInput(const std::string_view /*value_sv*/) const
{
    return ReturnProgrammingError(DEFAULT);
}


const DictValue* StringValueProcessor::GetDictValueFromInput(const std::string_view value_sv) const
{
    return GetDictValue(GetAlphaFromInput(std::string(value_sv)));
}


std::string StringValueProcessor::GetOutput(const double value) const
{
    return ReturnProgrammingError(DoubleToString(value));
}



// --------------------------------------------------------------------------
// StringItemValueProcessor
// --------------------------------------------------------------------------

StringItemValueProcessor::StringItemValueProcessor(const CDictItem& dict_item, const DictValueSet* const dict_value_set) noexcept
    :   StringValueProcessor(&dict_item, dict_value_set)
{
    ASSERT(m_dictItem != nullptr && m_dictItem->GetContentType() == ContentType::Alpha);
}


StringItemValueProcessor::StringItemValueProcessor(const CDictItem& dict_item) noexcept
    :   StringItemValueProcessor(dict_item, nullptr)
{
}


bool StringItemValueProcessor::IsValid(const std::string_view value_sv, const bool pad_value_to_length/* = true*/) const
{
    ASSERT(m_dictItem != nullptr);
    const int length_difference = int32_cast(SO::WideLength(value_sv)) - m_dictItem->GetLen();

    if( length_difference == 0 )
        return true;

    // if the value is too long, it is not valid
    if( length_difference > 0 )
        return false;

    // if it is too short, it is only valid if the padding setting is true
    return pad_value_to_length;
}


const DictValue* StringItemValueProcessor::GetDictValue(std::string_view /*value_sv*/, bool /*pad_value_to_length = true*/) const
{
    return nullptr;
}


std::vector<const DictValue*> StringItemValueProcessor::GetMatchingDictValues(std::string_view /*value_sv*/) const
{
    return { };
}


std::string StringItemValueProcessor::GetAlphaFromInput(std::string value) const
{
    return SO::WideMakeExactLength(value, m_dictItem->GetLen());
}


std::string StringItemValueProcessor::GetOutput(std::string value) const
{
    return SO::WideMakeExactLength(value, m_dictItem->GetLen());
}


const std::vector<std::shared_ptr<const ValueSetResponse>>& StringItemValueProcessor::GetResponses() const
{
    ASSERT(m_responses.empty());
    return m_responses;
}



// --------------------------------------------------------------------------
// StringValueSetValueProcessor
// --------------------------------------------------------------------------

struct StringValueSetValueProcessor::Data
{
    VectorMap<std::string, size_t> alphas;
};


StringValueSetValueProcessor::StringValueSetValueProcessor(const CDictItem& dict_item, const DictValueSet& dict_value_set)
    :   StringItemValueProcessor(dict_item, &dict_value_set)
{
}


StringValueSetValueProcessor::~StringValueSetValueProcessor()
{
}


inline void StringValueSetValueProcessor::CalculateData() const
{
    if( m_data == nullptr )
        const_cast<StringValueSetValueProcessor*>(this)->CreateData();
}


void StringValueSetValueProcessor::CreateData()
{
    ASSERT(m_data == nullptr && m_responses.empty());

    m_data = std::make_unique<Data>();

    DictionaryIterator::ValueSetIterator::IterateValueSetValuePairs(*m_dictValueSet,
        [&](const DictValue& dict_value, const DictValuePair& dict_value_pair)
        {
            const size_t response_index = m_responses.size();
            m_responses.emplace_back(std::make_unique<const ValueSetResponse>(*m_dictItem, dict_value, dict_value_pair));
            m_data->alphas.Insert(dict_value_pair.GetFrom(), response_index);
        });
}


bool StringValueSetValueProcessor::IsValid(const std::string_view value_sv, const bool pad_value_to_length/* = true*/) const
{
    return ( GetDictValue(value_sv, pad_value_to_length) != nullptr );
}


const DictValue* StringValueSetValueProcessor::GetDictValue(std::string_view value_sv, const bool pad_value_to_length/* = true*/) const
{
    CalculateData();

    // first check the length of the string
    if( StringItemValueProcessor::IsValid(value_sv, pad_value_to_length) )
    {
        // and then check if the string is in the value set
        const size_t* alphas_index = m_data->alphas.Find(value_sv);

        // for CSPro 8.0, the codes are not necessarily right-padded,
        // so if not found, search ignoring whitespace at the end
        // // ENGINECR_TODO: revisit this ... this check was put in for field validation in 8.0, but wasn't necessary in 7.7
        if( alphas_index == nullptr )
        {
            value_sv = SO::TrimRightSpace(value_sv);

            for( const auto& [code, index] : m_data->alphas.GetVector() )
            {
                if( SO::StartsWith(code, value_sv) &&
                    SO::TrimRightSpace(code).length() == value_sv.length() )
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


std::vector<const DictValue*> StringValueSetValueProcessor::GetMatchingDictValues(std::string_view value_sv) const
{
    CalculateData();

    std::vector<const DictValue*> dict_values;

    // right-trim the value (to match the storage of the response codes)
    value_sv = SO::TrimRightSpace(value_sv);

    for( const ValueSetResponse& response : VI_V(m_responses) )
    {
        if( value_sv == response.GetCode() )
            dict_values.emplace_back(response.GetDictValue());
    }

    return dict_values;
}


const std::vector<std::shared_ptr<const ValueSetResponse>>& StringValueSetValueProcessor::GetResponses() const
{
    CalculateData();

    return m_responses;
}
