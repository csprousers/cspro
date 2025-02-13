#include "stdafx.h"
#include "Reformatter.h"
#include "BinaryCaseItemConverter.h"
#include "NumericCaseItemConverter.h"
#include "StringCaseItemConverter.h"
#include <zCaseO/CaseConstructionReporter.h>
#include <zCaseO/CaseItemReference.h>
#include <zDataO/DataRepositoryHelpers.h>


Reformatter::Reformatter(std::shared_ptr<const CDataDict> initial_dictionary, std::shared_ptr<const CDataDict> final_dictionary)
    :   DictionaryComparer(*initial_dictionary, *final_dictionary),
        m_initialDictionary(std::move(initial_dictionary)),
        m_finalDictionary(std::move(final_dictionary))
{
}


bool Reformatter::RequiresReformat(const DataRepositoryType data_repository_type) const
{
    std::unique_ptr<std::vector<DataStorageCharacteristic>> data_storage_characteristics;

    switch( data_repository_type )
    {
        case DataRepositoryType::Null:
        case DataRepositoryType::Memory:
            return false;

        case DataRepositoryType::Text:
            data_storage_characteristics = std::make_unique<std::vector<DataStorageCharacteristic>>();
            data_storage_characteristics->emplace_back(DataStorageCharacteristic::UsesFixedWidthAndPositions);
            data_storage_characteristics->emplace_back(DataStorageCharacteristic::UsesRecordType);
            break;

        case DataRepositoryType::SQLite:
        case DataRepositoryType::EncryptedSQLite:
        case DataRepositoryType::Json:
            break;

        default:
            ASSERT(DataRepositoryHelpers::IsTypeExportWriter(data_repository_type));
            return false;
    }

    return DictionaryComparer::RequiresReformat(data_storage_characteristics.get());
}


std::tuple<std::unique_ptr<CaseAccess>, std::shared_ptr<CaseAccess>> Reformatter::Initialize()
{
    auto initial_case_access = std::make_unique<CaseAccess>(*m_initialDictionary);
    initial_case_access->SetUsesAllCaseAttributes();

    // only the dictionary items that are in the final need to be read in
    for( const ItemPair& item_pair : m_matchedItemPairs )
        initial_case_access->SetUseDictionaryItem(*item_pair.initial_dict_item);

    initial_case_access->Initialize();

    m_finalCaseAccess = CaseAccess::CreateAndInitializeFullCaseAccess(*m_finalDictionary);

    // setup the case item pairs
    m_caseItemPairsByLevel.clear();

    for( const ItemPair& item_pair : m_matchedItemPairs )
    {
        const size_t level_number = item_pair.initial_dict_item->GetRecord()->GetLevel()->GetLevelNumber();

        while( level_number >= m_caseItemPairsByLevel.size() )
            m_caseItemPairsByLevel.emplace_back();

        m_caseItemPairsByLevel[level_number].emplace_back(new CaseItemPair
            {
                *initial_case_access->LookupCaseItem(*item_pair.initial_dict_item),
                item_pair.initial_dict_item->GetRecord()->GetRecordNumber(),
                *m_finalCaseAccess->LookupCaseItem(*item_pair.final_dict_item),
                item_pair.final_dict_item->GetRecord()->GetRecordNumber()
            });
    }

    if( m_caseItemPairsByLevel.empty() )
        throw CSProException("You cannot reformat data from two dictionaries that have no items in common.");

    return std::make_tuple(std::move(initial_case_access), m_finalCaseAccess);
}


void Reformatter::ReformatCaseItem(const CaseItem& initial_case_item, const CaseItemIndex& initial_index,
                                  const CaseItem& final_case_item, CaseItemIndex& final_index)
{
    // no need to reformat blank data
    if( initial_case_item.IsBlank(initial_index) )
        return;

    // create the converter
    std::unique_ptr<CaseItemConverter> converter;

    if( initial_case_item.IsTypeNumeric() )
    {
        converter = std::make_unique<NumericCaseItemConverter>(assert_cast<const NumericCaseItem&>(initial_case_item), initial_index);
    }

    else if( initial_case_item.IsTypeString() )
    {
        converter = std::make_unique<StringCaseItemConverter>(assert_cast<const StringCaseItem&>(initial_case_item), initial_index);
    }

    else if( initial_case_item.IsTypeBinary() )
    {
        converter = std::make_unique<BinaryCaseItemConverter>(assert_cast<const BinaryCaseItem&>(initial_case_item), initial_index);
    }

    else
    {
        throw ProgrammingErrorException();
    }

    // convert the data
    bool success;

    if( final_case_item.IsTypeNumeric() )
    {
        success = converter->ToNumber(assert_cast<const NumericCaseItem&>(final_case_item), final_index);
    }

    else if( final_case_item.IsTypeString() )
    {
        success = converter->ToString(assert_cast<const StringCaseItem&>(final_case_item), final_index);
    }

    else if( final_case_item.IsTypeBinary() )
    {
        success = converter->ToBinary(assert_cast<const BinaryCaseItem&>(final_case_item), final_index);
    }

    else
    {
        throw ProgrammingErrorException();
    }

    if( !success )
    {
        const Case& initial_case = initial_index.GetCase();

        if( initial_case.GetCaseConstructionReporter() != nullptr )
        {
            // pass the initial case so that, when the key is printed, it is clear
            // which case this is coming from
            initial_case.GetCaseConstructionReporter()->ContentTypeConversionError(initial_case,
                                                                                   initial_case_item.GetDictionaryItem().GetName(),
                                                                                   initial_case_item.GetDictionaryItem().GetContentType(),
                                                                                   final_case_item.GetDictionaryItem().GetContentType());
        }
    }
}


void Reformatter::ReformatCase(const Case& initial_case, Case& final_case)
{
    final_case.Reset();
    final_case.SetUuid(CS2WS(initial_case.GetUuid()));
    final_case.SetCaseLabel(initial_case.GetCaseLabel());
    final_case.SetDeleted(initial_case.GetDeleted());
    final_case.SetVerified(initial_case.GetVerified());
    final_case.SetVectorClock(initial_case.GetVectorClock());

    std::vector<std::tuple<std::wstring, std::wstring>> level_key_pairs;

    // reformat each level
    std::function<void(const CaseLevel&, CaseLevel&, size_t)> reformat_level =
        [&](const CaseLevel& initial_case_level, CaseLevel& final_case_level, size_t level_number)
    {
        // process each item on the level
        for( const auto& case_item_pair : m_caseItemPairsByLevel[level_number] )
        {
            const CaseRecord& initial_case_record = initial_case_level.GetCaseRecord(case_item_pair->initial_record_number);
            CaseRecord& final_case_record = final_case_level.GetCaseRecord(case_item_pair->final_record_number);

            const size_t record_occurrences_to_output = std::min(initial_case_record.GetNumberOccurrences(),
                                                                 static_cast<size_t>(final_case_record.GetCaseRecordMetadata().GetDictionaryRecord().GetMaxRecs()));

            if( record_occurrences_to_output == 0 )
                continue;

            if( final_case_record.GetNumberOccurrences() < record_occurrences_to_output )
                final_case_record.SetNumberOccurrences(record_occurrences_to_output);

            const size_t item_subitem_occurrences_to_output = std::min(case_item_pair->initial_case_item.GetDictionaryItem().GetItemSubitemOccurs(),
                                                                       case_item_pair->final_case_item.GetDictionaryItem().GetItemSubitemOccurs());

            for( size_t record_occurrence = 0; record_occurrence < record_occurrences_to_output; ++record_occurrence )
            {
                CaseItemIndex initial_index = initial_case_record.GetCaseItemIndex(record_occurrence);
                CaseItemIndex final_index = final_case_record.GetCaseItemIndex(record_occurrence);

                while( initial_index.GetItemSubitemOccurrence(case_item_pair->initial_case_item) < item_subitem_occurrences_to_output )
                {
                    ReformatCaseItem(case_item_pair->initial_case_item, initial_index, case_item_pair->final_case_item, final_index);

                    initial_index.IncrementItemSubitemOccurrence(case_item_pair->initial_case_item);
                    final_index.IncrementItemSubitemOccurrence(case_item_pair->final_case_item);
                }
            }
        }

        if( level_number > 0 )
            level_key_pairs.emplace_back(CS2WS(initial_case_level.GetLevelKey()), CS2WS(final_case_level.GetLevelKey()));

        // if multiple levels are matched, process them
        if( ( level_number + 1 ) < m_caseItemPairsByLevel.size() )
        {
            for( size_t level_index = 0; level_index < initial_case_level.GetNumberChildCaseLevels(); ++level_index )
                reformat_level(initial_case_level.GetChildCaseLevel(level_index), final_case_level.AddChildCaseLevel(), level_number + 1);
        }
    };

    reformat_level(initial_case.GetRootCaseLevel(), final_case.GetRootCaseLevel(), 0);

    // add any required records not added above
    final_case.AddRequiredRecords();


    // a routine to create a new case item reference
    auto create_case_item_reference = [&](const CaseItemReference& initial_case_item_reference) -> std::unique_ptr<CaseItemReference>
    {
        const CaseItem* final_case_item = m_finalCaseAccess->LookupCaseItem(initial_case_item_reference.GetName());

        // check that case item and occurrence are still valid
        if( final_case_item == nullptr || !final_case_item->GetItemIndexHelper().IsValid(initial_case_item_reference) )
            return nullptr;

        const std::wstring* matched_final_level_key = nullptr;

        if( !initial_case_item_reference.GetLevelKey().IsEmpty() )
        {
            for( const auto& [initial_level_key, final_level_key] : level_key_pairs )
            {
                if( SO::Equals(initial_level_key, initial_case_item_reference.GetLevelKey()) )
                {
                    matched_final_level_key = &final_level_key;
                    break;
                }
            }

            // can't create a reference if the level hasn't been processed
            if( matched_final_level_key == nullptr )
                return nullptr;
        }

        return std::make_unique<CaseItemReference>(*final_case_item,
                                                   WS2CS(( matched_final_level_key != nullptr ) ? *matched_final_level_key : std::wstring()),
                                                   initial_case_item_reference.GetOccurrences());
    };


    // adjust the partial save status
    if( initial_case.IsPartial() )
    {
        std::unique_ptr<CaseItemReference> partial_save_case_item_reference;

        if( initial_case.GetPartialSaveCaseItemReference() != nullptr )
            partial_save_case_item_reference = create_case_item_reference(*initial_case.GetPartialSaveCaseItemReference());

        final_case.SetPartialSaveStatus(initial_case.GetPartialSaveMode(), std::move(partial_save_case_item_reference));
    }


    // adjust the notes
    for( const Note& initial_note : initial_case.GetNotes() )
    {
        const NamedReference& named_reference = initial_note.GetNamedReference();
        std::unique_ptr<NamedReference> final_named_reference;

        const CaseItemReference* initial_case_item_reference = dynamic_cast<const CaseItemReference*>(&named_reference);

        if( initial_case_item_reference == nullptr )
        {
            // the only valid options here are dictionary, level, or record names, which should not have level keys set
            if( named_reference.GetLevelKey().IsEmpty() )
            {
                bool valid_name = ( m_finalDictionary->GetName() == named_reference.GetName() );

                if( !valid_name )
                {
                    int level;
                    int record;
                    int item;
                    int vset;
                    valid_name = ( m_finalDictionary->LookupName(named_reference.GetName(), &level, &record, &item, &vset) && item == NONE );
                }

                if( valid_name )
                    final_named_reference = std::make_unique<NamedReference>(named_reference.GetName(), CString());
            }
        }

        else
        {
            final_named_reference = create_case_item_reference(*initial_case_item_reference);
        }

        if( final_named_reference != nullptr )
        {
            final_case.GetNotes().emplace_back(initial_note.GetContent(), std::move(final_named_reference),
                                               initial_note.GetOperatorId(), initial_note.GetModifiedDateTime());
        }
    }
}
