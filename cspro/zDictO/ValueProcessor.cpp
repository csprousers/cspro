#include "StdAfx.h"
#include "ValueProcessor.h"
#include "NumericValueProcessor.h"
#include "StringValueProcessor.h"


ValueProcessor::ValueProcessor(const CDictItem* const dict_item, const DictValueSet* const dict_value_set) noexcept
    :   m_dictItem(dict_item),
        m_dictValueSet(dict_value_set)
{
    ASSERT(m_dictItem == nullptr ||
           m_dictItem->GetContentType() == ContentType::Numeric ||
           m_dictItem->GetContentType() == ContentType::Alpha);
}


std::shared_ptr<const ValueProcessor> ValueProcessor::CreateValueProcessor(const CDictItem& dict_item, const DictValueSet* const dict_value_set/* = nullptr*/)
{
    std::shared_ptr<const ValueProcessor> value_processor;

    // if the value processor has already been set up for a value set, reuse it
    if( dict_value_set != nullptr )
        value_processor = dict_value_set->GetSharedValueProcessor();

    if( value_processor != nullptr )
        return value_processor;

    switch( dict_item.GetContentType() )
    {
        case ContentType::Numeric:
        {
            if( dict_value_set == nullptr )
            {
                value_processor = std::make_unique<NumericItemValueProcessor>(dict_item);
            }

            else
            {
                value_processor = std::make_unique<NumericValueSetValueProcessor>(dict_item, *dict_value_set);
            }

            break;
        }

        case ContentType::Alpha:
        {
            if( dict_value_set == nullptr )
            {
                value_processor = std::make_unique<StringItemValueProcessor>(dict_item);
            }

            else
            {
                value_processor = std::make_unique<StringValueSetValueProcessor>(dict_item, *dict_value_set);
            }

            break;
        }

        default:
        {
            CONTENT_TYPE_REFACTOR::LOOK_AT("what to do about non-numeric + non-alpha fields?");
            throw ProgrammingErrorException();
        }
    }

    if( dict_value_set != nullptr )
        const_cast<DictValueSet*>(dict_value_set)->SetValueProcessor(value_processor);

    return value_processor;
}


const DictValue* ValueProcessor::GetDictValueByLabel(const std::string_view label_sv) const
{
    if( m_dictValueSet != nullptr )
    {
        for( const DictValue& dict_value : m_dictValueSet->GetValues() )
        {
            if( SO::EqualsNoCase(label_sv, dict_value.GetLabel()) )
                return &dict_value;
        }
    }

    return nullptr;
}
