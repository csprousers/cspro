#include "StdAfx.h"
#include "StringValueProcessor.h"
#include "ValueSetResponse.h"
#include <zUtilO/VectorMap.h>


// --------------------------------------------------------------------------
// AlphaItemValueProcessor
// --------------------------------------------------------------------------

bool AlphaItemValueProcessor::IsValid(const CString& value, bool pad_value_to_length) const override
{
    // if the value is too long, it is not valid; if it is too short, it is only valid if the
    // padding setting is true
    int length_difference = (int)m_dictItem.GetLen() - value.GetLength();
    return ( length_difference == 0 || ( pad_value_to_length && length_difference > 0 ) );
}


std::string AlphaItemValueProcessor::GetAlphaFromInput(std::string value) const
{
    return CIMSAString::MakeExactLength(value, m_dictItem.GetLen());
}


std::string AlphaItemValueProcessor::GetOutput(std::string value) const
{
    return CIMSAString::MakeExactLength(value, m_dictItem.GetLen());
}



// --------------------------------------------------------------------------
// AlphaValueSetValueProcessor
// --------------------------------------------------------------------------

AlphaValueSetValueProcessor(const CDictItem& dict_item, const DictValueSet* dict_value_set)
    :   AlphaItemValueProcessor(dict_item, dict_value_set)
{
}


void AlphaValueSetValueProcessor::SetupFullValueProcessor() const
{
    DictionaryIterator::ValueSetIterator::IterateValueSetValuePairs(*m_dictValueSet,
        [&](const DictValue& dict_value, const DictValuePair& dict_value_pair)
        {
            size_t response_index = m_responses.size();
            m_responses.emplace_back(std::make_shared<ValueSetResponse>(m_dictItem, dict_value, dict_value_pair));
            m_alphas.Insert(dict_value_pair.GetFrom(), response_index);
        });
}


bool AlphaValueSetValueProcessor::IsValid(const std::string_view value_sv, const bool pad_value_to_length/* = true*/) const
{
    return ( GetDictValue(value_sv, pad_value_to_length) != nullptr );
}


const DictValue* AlphaValueSetValueProcessor::GetDictValue(const CString& value, bool pad_value_to_length) const override
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


std::vector<const DictValue*> AlphaValueSetValueProcessor::GetMatchingDictValues(wstring_view value_sv) const override
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
