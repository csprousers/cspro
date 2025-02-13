#include "stdafx.h"
#include "CaseLevel.h"
#include "CaseItemReference.h"


CaseLevelMetadata::CaseLevelMetadata(const CaseMetadata& case_metadata, const DictLevel& dict_level,
    const CaseAccess& case_access, std::tuple<size_t, size_t, size_t, size_t>& attribute_counter)
    :   m_caseMetadata(case_metadata),
        m_dictLevel(dict_level),
        m_levelKeyLength(0)
{
    std::get<0>(attribute_counter) = SIZE_MAX;

    const CDictRecord& id_dictionary_record = *m_dictLevel.GetIdItemsRec();
    m_idCaseRecordMetadata = new CaseRecordMetadata(*this, id_dictionary_record, case_access, attribute_counter);    

    for( int record_counter = 0; record_counter < m_dictLevel.GetNumRecords(); ++record_counter )
    {
        std::get<0>(attribute_counter) = record_counter;

        const CDictRecord& dictionary_record = *(m_dictLevel.GetRecord(record_counter));
        m_caseRecordsMetadata.emplace_back(new CaseRecordMetadata(*this, dictionary_record, case_access, attribute_counter));
    }

    // calculate the key length
    for( auto i = 0; i < id_dictionary_record.GetNumItems(); ++i )
        m_levelKeyLength += id_dictionary_record.GetItem(i)->GetLen();
}


CaseLevelMetadata::~CaseLevelMetadata()
{
    delete m_idCaseRecordMetadata;
    safe_delete_vector_contents(m_caseRecordsMetadata);
}


const CaseRecordMetadata* CaseLevelMetadata::FindCaseRecordMetadata(const CString& record_name) const
{
    if( m_idCaseRecordMetadata->GetDictionaryRecord().GetName().Compare(record_name) == 0 )
        return m_idCaseRecordMetadata;

    const auto case_record_metadata_search = std::find_if(m_caseRecordsMetadata.cbegin(),
        m_caseRecordsMetadata.cend(), [&](const auto& case_record_metadata)
        { return ( case_record_metadata->GetDictionaryRecord().GetName().Compare(record_name) == 0 ); });

    return ( case_record_metadata_search == m_caseRecordsMetadata.cend() ) ? nullptr : *case_record_metadata_search;
}


const CaseLevelMetadata* CaseLevelMetadata::GetChildCaseLevelMetadata() const
{
    const auto& case_levels_metadata = m_caseMetadata.GetCaseLevelsMetadata();
    size_t child_level_number = m_dictLevel.GetLevelNumber() + 1;
    return ( child_level_number < case_levels_metadata.size() ) ? case_levels_metadata[child_level_number] : nullptr;
}



CaseLevel::CaseLevel(Case& data_case, const CaseLevelMetadata& case_level_metadata, CaseLevel* parent_case_level)
    :   m_case(data_case),
        m_caseLevelMetadata(case_level_metadata),
        m_parentCaseLevel(parent_case_level),
        m_numberChildCaseLevels(0)
{
    m_idCaseRecord = new CaseRecord(*this, *m_caseLevelMetadata.m_idCaseRecordMetadata);

    for( const CaseRecordMetadata* case_record_metadata : m_caseLevelMetadata.m_caseRecordsMetadata )
        m_caseRecords.emplace_back(new CaseRecord(*this, *case_record_metadata));
}


CaseLevel::~CaseLevel()
{
    safe_delete_vector_contents(m_childCaseLevels);
    delete m_idCaseRecord;
    safe_delete_vector_contents(m_caseRecords);
}


bool CaseLevel::operator==(const CaseLevel& rhs_case_level) const
{
    auto compare_record = [](const CaseRecord& case_record1, const CaseRecord& case_record2) -> bool
    {
        if( case_record1.GetNumberOccurrences() != case_record2.GetNumberOccurrences() )
            return false;

        CaseItemIndex index2 = case_record2.GetCaseItemIndex();

        for( CaseItemIndex index1 = case_record1.GetCaseItemIndex(); index1.GetRecordOccurrence() < case_record1.GetNumberOccurrences(); index1.IncrementRecordOccurrence(), index2.IncrementRecordOccurrence() )
        {
            auto case_items2_itr = case_record2.GetCaseItems().cbegin();

            for( const CaseItem* case_item1 : case_record1.GetCaseItems() )
            {
                const CaseItem* case_item2 = *case_items2_itr;

                for( index1.SetItemSubitemOccurrence(*case_item1, 0), index2.SetItemSubitemOccurrence(*case_item2, 0);
                     index1.GetItemSubitemOccurrence(*case_item1) < case_item1->GetTotalNumberItemSubitemOccurrences();
                     index1.IncrementItemSubitemOccurrence(*case_item1), index2.IncrementItemSubitemOccurrence(*case_item2) )
                {
                    if( case_item1->CompareValues(index1, index2) != 0 )
                        return false;
                }

                ++case_items2_itr;
            }
        }

        return true;
    };

    std::function<bool(const CaseLevel&, const CaseLevel&)> compare_level =
        [&](const CaseLevel& case_level1, const CaseLevel& case_level2) -> bool
    {
        if( !compare_record(case_level1.GetIdCaseRecord(), case_level2.GetIdCaseRecord()) )
            return false;

        for( size_t record_number = 0; record_number < case_level1.GetNumberCaseRecords(); ++record_number )
        {
            if( !compare_record(case_level1.GetCaseRecord(record_number), case_level2.GetCaseRecord(record_number)) )
                return false;
        }

        if( case_level1.GetNumberChildCaseLevels() != case_level2.GetNumberChildCaseLevels() )
            return false;

        for( size_t level_number = 0; level_number < case_level1.GetNumberChildCaseLevels(); ++level_number )
        {
            if( !compare_level(case_level1.GetChildCaseLevel(level_number), case_level2.GetChildCaseLevel(level_number)) )
                return false;
        }

        return true;
    };

    ASSERT(&m_caseLevelMetadata == &rhs_case_level.m_caseLevelMetadata);

    return compare_level(*this, rhs_case_level);
}


void CaseLevel::Reset()
{
    m_numberChildCaseLevels = 0;
    m_levelIdentifier.Empty();

    // the ID record always exists
    m_idCaseRecord->Reset();
    m_idCaseRecord->SetNumberOccurrences(1);

    for( CaseRecord* case_record : m_caseRecords )
        case_record->Reset();
}


const CaseLevel& CaseLevel::GetParentCaseLevel() const
{
    if( m_parentCaseLevel == nullptr )
        throw CSProException("The root level does not have a parent");

    return *m_parentCaseLevel;
}


CaseLevel& CaseLevel::AddChildCaseLevel()
{
    if( m_numberChildCaseLevels >= m_childCaseLevels.size() )
    {
        const CaseLevelMetadata* case_level_metadata = m_caseLevelMetadata.GetChildCaseLevelMetadata();
        ASSERT(case_level_metadata != nullptr);

        m_childCaseLevels.emplace_back(new CaseLevel(m_case, *case_level_metadata, this));
    }

    CaseLevel* child_case_level = m_childCaseLevels[m_numberChildCaseLevels];
    child_case_level->Reset();

    ++m_numberChildCaseLevels;

    return *child_case_level;
}


void CaseLevel::RemoveChildCaseLevel(CaseLevel& child_case_level)
{
    for( size_t level_number = 0; level_number < m_numberChildCaseLevels; ++level_number )
    {
        if( m_childCaseLevels[level_number] == &child_case_level )
        {
            // shift all of the subsequent levels
            for( size_t i = level_number + 1; i < m_numberChildCaseLevels; ++i )
                m_childCaseLevels[i - 1] = m_childCaseLevels[i];

            --m_numberChildCaseLevels;

            // store the old level at the new end so that it can be reused
            m_childCaseLevels[m_numberChildCaseLevels] = &child_case_level;


            // if a case was partially saved on this level or a child level, remove that reference
            Case& data_case = GetCase();

            const auto& partial_save_case_item_reference = data_case.GetPartialSaveCaseItemReference();

            if( partial_save_case_item_reference != nullptr && partial_save_case_item_reference->GetLevelKey().Find(child_case_level.GetLevelKey()) == 0 )
                data_case.SetPartialSaveStatus(data_case.GetPartialSaveMode());

            // remove any notes from this level or child levels
            auto& notes = data_case.GetNotes();

            for( auto note_itr = notes.cbegin(); note_itr != notes.cend(); )
            {
                if( note_itr->GetNamedReference().GetLevelKey().Find(child_case_level.GetLevelKey()) == 0 )
                {
                    note_itr = notes.erase(note_itr);
                }

                else
                {
                    ++note_itr;
                }
            }

            return;
        }
    }

    throw CSProException("The child case level was not found");
}


CaseRecord& CaseLevel::GetCaseRecord(const CaseRecordMetadata& case_record_metadata)
{
    return ( case_record_metadata.GetRecordIndex() == SIZE_MAX ) ? *m_idCaseRecord :
                                                                   GetCaseRecord(case_record_metadata.GetRecordIndex());
}


const CString& CaseLevel::GetLevelKey() const
{
    return ( m_parentCaseLevel == nullptr ) ? SO::EmptyCString :
                                              GetLevelIdentifier();
}


const CString& CaseLevel::GetLevelIdentifier() const
{
    // for the root level, the level identifier is the case key
    // for other levels, the level identifier is the complete level key (not including the case key)

    // if the key is empty, calculate it
    if( m_levelIdentifier.IsEmpty() )
    {
        // get the key of any parent that isn't the root level
        if( m_caseLevelMetadata.GetDictLevel().GetLevelNumber() >= 2 )
            m_levelIdentifier = m_parentCaseLevel->GetLevelIdentifier();

        size_t parent_level_key_length = m_levelIdentifier.GetLength();
        size_t full_key_length = parent_level_key_length + m_caseLevelMetadata.m_levelKeyLength;
        TCHAR* level_key_iterator = m_levelIdentifier.GetBufferSetLength(full_key_length) + parent_level_key_length;

        // add this level's key
        CaseItemIndex index = m_idCaseRecord->GetCaseItemIndex();

        for( const CaseItem* case_item : m_idCaseRecord->GetCaseItems() )
        {
            ASSERT(case_item->IsTypeFixed());
            dynamic_cast<const FixedWidthCaseItem*>(case_item)->OutputFixedValue(index, level_key_iterator);
            level_key_iterator += case_item->GetDictionaryItem().GetLen();
        }

        m_levelIdentifier.ReleaseBuffer(full_key_length);
    }

    return m_levelIdentifier;
}


void CaseLevel::RecalculateLevelIdentifier(bool adjust_level_keys/* = true*/)
{
    // for the root level, mark the level identifier as empty (to be computed on demand)
    if( m_caseLevelMetadata.GetDictLevel().GetLevelNumber() == 0 )
    {
        m_levelIdentifier.Empty();
        return;
    }

    // for other levels, because partial save statuses and notes are linked by the level key,
    // we can't just compute the level key on demand; we need to always compute it and then
    // adjust the level keys of statuses and notes based on the changed key
    CString previous_level_key = m_levelIdentifier;

    m_levelIdentifier.Empty();

    // mark that all child levels also need to recalculate their identifiers
    for( size_t level_number = 0; level_number < m_numberChildCaseLevels; ++level_number )
        m_childCaseLevels[level_number]->RecalculateLevelIdentifier(false);

    if( !adjust_level_keys )
        return;

    const CString& new_level_key = GetLevelIdentifier();

    if( !previous_level_key.IsEmpty() )
    {
        auto calculate_new_level_key = [&](CString level_key) -> CString
        {
            ASSERT(new_level_key.GetLength() <= level_key.GetLength());
            _tcsncpy(level_key.GetBuffer(), (LPCTSTR)new_level_key, new_level_key.GetLength());
            level_key.ReleaseBuffer();
            return level_key;
        };

        // if a case was partially saved on this level or a child level, change that reference
        Case& data_case = GetCase();

        CaseItemReference* partial_save_case_item_reference = data_case.GetPartialSaveCaseItemReference();

        if( partial_save_case_item_reference != nullptr && partial_save_case_item_reference->GetLevelKey().Find(previous_level_key) == 0 )
            partial_save_case_item_reference->SetLevelKey(calculate_new_level_key(partial_save_case_item_reference->GetLevelKey()));

        // change the references in notes
        for( Note& note : data_case.GetNotes() )
        {
            NamedReference& named_reference = note.GetNamedReference();

            if( named_reference.GetLevelKey().Find(previous_level_key) == 0 )
                named_reference.SetLevelKey(calculate_new_level_key(named_reference.GetLevelKey()));
        }
    }
}
