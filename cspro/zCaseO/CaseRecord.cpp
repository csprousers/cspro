#include "stdafx.h"
#include "CaseRecord.h"
#include "ItemSubitemSyncTask.h"
#include "KeyChangeTask.h"


// --------------------------------------------------------------------------
// CaseRecordMetadata
// --------------------------------------------------------------------------

CaseRecordMetadata::CaseRecordMetadata(const CaseLevelMetadata& case_level_metadata, const CDictRecord& dict_record, const CaseAccess& case_access,
                                       const size_t record_index, std::tuple<size_t&, size_t&, size_t&>& attribute_counter)
    :   m_caseLevelMetadata(&case_level_metadata),
        m_dictRecord(dict_record),
        m_recordIndex(record_index),
        m_totalRecordIndex(std::get<0>(attribute_counter)++),
        m_recordSizeForMemoryAllocation(0)
{
    // create CaseItem objects for each used item
    const bool adding_ids = ( case_level_metadata.GetDictLevel().GetIdItemsRec() == &dict_record  );

    CaseItem* last_non_subitem_case_item = nullptr;
    std::shared_ptr<ItemSubitemSyncTask> last_item_subitem_sync_task;

    for( int item_counter = 0; item_counter < dict_record.GetNumItems(); ++item_counter )
    {
        const CDictItem& dict_item = *(dict_record.GetItem(item_counter));

        if( !case_access.GetUseDictionaryItem(dict_item) )
            continue;

        std::unique_ptr<CaseItem> case_item;

        if( dict_item.GetContentType() == ContentType::Numeric )
        {
            // force using a fixed width numeric with a string buffer when getbuffer is used in logic or if this is a subitem or an item with subitems
            if( case_access.GetUsesGetBuffer() ||
                dict_item.GetItemType() == ItemType::Subitem ||
                dict_item.HasSubitems() )
            {
                case_item = CaseItem::Create(dict_item, CaseItem::Type::FixedWidthNumericWithStringBuffer);
            }
        }

        if( case_item == nullptr )
        {
            case_item = CaseItem::Create(dict_item);
            ASSERT(case_item != nullptr);
        }

        case_item->m_recordDataOffset = m_recordSizeForMemoryAllocation;
        case_item->m_memorySize = case_item->GetSizeForMemoryAllocation();
        m_recordSizeForMemoryAllocation += case_item->m_memorySize * case_item->m_totalNumberOccurrences;

        case_item->m_totalCaseItemIndex = std::get<1>(attribute_counter)++;

        if( IsBinary(case_item->GetDataType()) )
            ++std::get<2>(attribute_counter);

        // if the item is an ID item, add a key change task
        if( adding_ids )
            case_item->AddPostSetValueTask(std::make_unique<KeyChangeTask>());

        // if the item is a subitem, make sure that it is kept in sync with its parent
        if( dict_item.GetItemType() == ItemType::Item )
        {
            last_non_subitem_case_item = case_item.get();
            last_item_subitem_sync_task.reset();
        }

        else
        {
            ASSERT(dict_item.GetParentItem() == &last_non_subitem_case_item->m_dictItem);

            // set up the item/subitem sync task
            if( last_item_subitem_sync_task == nullptr )
            {
                last_item_subitem_sync_task = std::make_shared<ItemSubitemSyncTask>(*last_non_subitem_case_item);
                last_non_subitem_case_item->AddPostSetValueTask(last_item_subitem_sync_task);
            }

            last_item_subitem_sync_task->AddSubitem(*case_item);

            // add the task to the subitem
            case_item->AddPostSetValueTask(last_item_subitem_sync_task);
        }

        m_caseItems.emplace_back(case_item.get());
        m_caseItemsStorage.emplace_back(std::move(case_item));
    }
}


CaseRecordMetadata::~CaseRecordMetadata()
{
}



// --------------------------------------------------------------------------
// CaseRecord
// --------------------------------------------------------------------------

CaseRecord::CaseRecord(CaseLevel& case_level, const CaseRecordMetadata& case_record_metadata)
    :   m_caseLevel(case_level),
        m_caseRecordMetadata(case_record_metadata),
        m_numberOccurrences(0)
{
}


CaseRecord::~CaseRecord()
{
    for( std::unique_ptr<std::byte[]>& record_data : m_recordData )
    {
        std::byte* data_buffer = record_data.get();

        for( const CaseItem* const case_item : m_caseRecordMetadata.GetCaseItems() )
        {
            for( size_t occurrence = 0; occurrence < case_item->m_totalNumberOccurrences; ++occurrence )
            {
                case_item->DeallocateMemory(data_buffer);
                data_buffer += case_item->m_memorySize;
            }
        }
    }
}


bool CaseRecord::operator==(const CaseRecord& rhs) const
{
    if( m_numberOccurrences != rhs.m_numberOccurrences )
        return false;

    CaseItemIndex lhs_index = GetCaseItemIndex();
    CaseItemIndex rhs_index = rhs.GetCaseItemIndex();

    while( lhs_index.GetRecordOccurrence() < m_numberOccurrences )
    {
        auto rhs_case_items_itr = rhs.GetCaseItems().cbegin();

        for( const CaseItem* const lhs_case_item : GetCaseItems() )
        {
            const CaseItem* const rhs_case_item = *rhs_case_items_itr;

            lhs_index.SetItemSubitemOccurrence(*lhs_case_item, 0);
            rhs_index.SetItemSubitemOccurrence(*rhs_case_item, 0);

            while( lhs_index.GetItemSubitemOccurrence(*lhs_case_item) < lhs_case_item->GetTotalNumberItemSubitemOccurrences() )
            {
                if( lhs_case_item->CompareValues(lhs_index, rhs_index) != 0 )
                    return false;

                lhs_index.IncrementItemSubitemOccurrence(*lhs_case_item);
                rhs_index.IncrementItemSubitemOccurrence(*rhs_case_item);
            }

            ++rhs_case_items_itr;
        }

        lhs_index.IncrementRecordOccurrence();
        rhs_index.IncrementRecordOccurrence();
    }

    return true;
}


void CaseRecord::SetNumberOccurrences(const size_t number_occurrences)
{
    ASSERT(number_occurrences <= m_caseRecordMetadata.m_dictRecord.GetMaxRecs());

    for( size_t record_occurrence = m_numberOccurrences; record_occurrence < number_occurrences; ++record_occurrence )
    {
        const bool allocate_new_record_occurrence = ( record_occurrence == m_recordData.size() );

        if( allocate_new_record_occurrence )
            m_recordData.emplace_back(std::make_unique_for_overwrite<std::byte[]>(m_caseRecordMetadata.m_recordSizeForMemoryAllocation));

        // potentially allocate memory and reset the data for all the items
        std::byte* data_buffer = m_recordData[record_occurrence].get();

        for( const CaseItem* const case_item : m_caseRecordMetadata.GetCaseItems() )
        {
            // the for loop below could handle items with only one occurrence,
            // but because that is very common optimize for that case
            if( case_item->m_hasMultipleOccurrences )
            {
                for( size_t occurrence = 0; occurrence < case_item->m_totalNumberOccurrences; ++occurrence )
                {
                    if( allocate_new_record_occurrence )
                        case_item->AllocateMemory(data_buffer);

                    case_item->ResetValue(data_buffer);

                    data_buffer += case_item->m_memorySize;
                }
            }

            else
            {
                if( allocate_new_record_occurrence )
                    case_item->AllocateMemory(data_buffer);

                case_item->ResetValue(data_buffer);

                data_buffer += case_item->m_memorySize;
            }
        }
    }

    m_numberOccurrences = number_occurrences;
}


void CaseRecord::CopyValues(const CaseRecord& copy_case_record, const size_t record_occurrence)
{
    std::byte* data_buffer = m_recordData[record_occurrence].get();
    const std::byte* copy_data_buffer = copy_case_record.m_recordData[record_occurrence].get();

    for( const CaseItem* const case_item : m_caseRecordMetadata.GetCaseItems() )
    {
        for( size_t occurrence = 0; occurrence < case_item->m_totalNumberOccurrences; ++occurrence )
        {
            case_item->CopyValue(data_buffer, copy_data_buffer);
            data_buffer += case_item->m_memorySize;
            copy_data_buffer += case_item->m_memorySize;
        }
    }
}


std::vector<std::byte> CaseRecord::GetBinaryValues(const size_t record_occurrence) const
{
    std::byte* binary_buffer_data = nullptr;

    auto iterate_over_case_items = [&](auto& value_to_increment)
    {
        const std::byte* data_buffer = m_recordData[record_occurrence].get();

        for( const CaseItem* const case_item : m_caseRecordMetadata.GetCaseItems() )
        {
            for( size_t occurrence = 0; occurrence < case_item->m_totalNumberOccurrences; ++occurrence )
            {
                const size_t binary_buffer_size = case_item->StoreBinaryValue(data_buffer, binary_buffer_data);

                value_to_increment += binary_buffer_size;

                data_buffer += case_item->m_memorySize;
            }
        }
    };

    // on the first pass, get the size needed for the binary buffer
    size_t binary_buffer_total_size = 0;

    iterate_over_case_items(binary_buffer_total_size);

    // allocate the buffer for the second pass and then store the data
    std::vector<std::byte> binary_buffer(binary_buffer_total_size);
    binary_buffer_data = binary_buffer.data();

    iterate_over_case_items(binary_buffer_data);

    return binary_buffer;
}


void CaseRecord::SetBinaryValues(const size_t record_occurrence, const std::byte* binary_buffer)
{
    std::byte* data_buffer = m_recordData[record_occurrence].get();

    for( const CaseItem* const case_item : m_caseRecordMetadata.GetCaseItems() )
    {
        for( size_t occurrence = 0; occurrence < case_item->m_totalNumberOccurrences; ++occurrence )
        {
            case_item->RetrieveBinaryValue(data_buffer, binary_buffer);
            data_buffer += case_item->m_memorySize;
        }
    }
}
