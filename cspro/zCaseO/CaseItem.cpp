#include "stdafx.h"
#include "CaseItem.h"
#include "BinaryCaseItem.h"
#include "FixedWidthNumericCaseItem.h"
#include "FixedWidthNumericWithStringBufferCaseItem.h"
#include "FixedWidthStringCaseItem.h"
#include "NumericCaseItem.h"
#include "StringCaseItem.h"


CaseItem::CaseItem(const CDictItem& dict_item, Type type)
    :   m_dictItem(dict_item),
        m_itemIndexHelper(m_dictItem),
        m_type(type),
        m_recordDataOffset(SIZE_MAX),
        m_memorySize(SIZE_MAX),
        m_totalCaseItemIndex(SIZE_MAX),
        m_totalNumberOccurrences(m_dictItem.GetItemSubitemOccurs()),
        m_itemOccurrenceMultiplier(m_dictItem.GetOccurs()),
        m_parentDictionaryItem(m_dictItem.GetParentItem())
{
    m_typeString =  ( m_type == Type::String ||
                      m_type == Type::FixedWidthString );

    m_typeNumeric = ( m_type == Type::Numeric ||
                      m_type == Type::FixedWidthNumeric || 
                      m_type == Type::FixedWidthNumericWithStringBuffer );

    m_typeFixed =   ( m_type == Type::FixedWidthString ||
                      m_type == Type::FixedWidthNumeric || 
                      m_type == Type::FixedWidthNumericWithStringBuffer );

    // m_recordDataOffset, m_memorySize, and m_totalCaseItemIndex will be set in the CaseRecordMetadata constructor

    m_hasMultipleOccurrences = ( m_totalNumberOccurrences > 1 );
}


CaseItem* CaseItem::Create(const CDictItem& dict_item, Type type)
{
    switch( type )
    {
        case Type::String:                            return new StringCaseItem(dict_item);
        case Type::FixedWidthString:                  return new FixedWidthStringCaseItem(dict_item);
        case Type::Numeric:                           return new NumericCaseItem(dict_item);
        case Type::FixedWidthNumeric:                 return new FixedWidthNumericCaseItem(dict_item);
        case Type::FixedWidthNumericWithStringBuffer: return new FixedWidthNumericWithStringBufferCaseItem(dict_item);
        case Type::Binary:                            return new BinaryCaseItem(dict_item);
        default:                                      return ReturnProgrammingError(nullptr);
    }
}


CaseItem* CaseItem::Create(const CDictItem& dict_item)
{
    switch( dict_item.GetContentType() )
    {
        case ContentType::Numeric:
            return Create(dict_item, Type::FixedWidthNumeric);

        case ContentType::Alpha:
            return Create(dict_item, Type::FixedWidthString);

        case ContentType::Audio:
        case ContentType::Document:
        case ContentType::Geometry:
        case ContentType::Image:
            return Create(dict_item, Type::Binary);

        default:
            return ReturnProgrammingError(nullptr);
    }
}


const void* CaseItem::GetDataBuffer(const CaseItemIndex& index) const
{
    // if the last calculated data buffer cannot be reused, calculate the requested data buffer
    if( index.m_lastCalculatedDataBuffer == nullptr || index.m_lastCalculatedCaseItem != this )
    {
        ASSERT(m_dictItem.GetRecord() == &index.GetCaseRecord().GetCaseRecordMetadata().GetDictionaryRecord());
        ASSERT(index.GetRecordOccurrence() < index.GetCaseRecord().GetNumberOccurrences());

        index.m_lastCalculatedDataBuffer = index.m_caseRecord.m_recordData[index.GetRecordOccurrence()] + m_recordDataOffset;
        index.m_lastCalculatedCaseItem = this;

        if( !m_hasMultipleOccurrences )
        {
            ASSERT(index.GetItemOccurrence() == 0 && index.GetSubitemOccurrence() == 0);
        }

        else if( m_parentDictionaryItem == nullptr ) 
        {
            ASSERT(index.GetItemOccurrence() < m_itemOccurrenceMultiplier && index.GetSubitemOccurrence() == 0);
            index.m_lastCalculatedDataBuffer += ( index.GetItemOccurrence() * m_memorySize );
        }

        else
        {
            ASSERT(index.GetItemOccurrence() < m_parentDictionaryItem->GetOccurs() && index.GetSubitemOccurrence() < m_itemOccurrenceMultiplier);
            index.m_lastCalculatedDataBuffer += ( ( ( index.GetItemOccurrence() * m_itemOccurrenceMultiplier ) + index.GetSubitemOccurrence() ) * m_memorySize );
        }
    }

    return index.m_lastCalculatedDataBuffer;
}


void CaseItem::RunPostSetValueTask(CaseItemIndex& index, const std::vector<std::shared_ptr<IPostSetValueTask>>::const_iterator& task_iterator) const
{
    if( task_iterator == m_postSetValueTasks.end() )
        return;

    (*task_iterator)->Do(*this, index);

    RunPostSetValueTask(index, task_iterator + 1);
}
