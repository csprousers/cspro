#include "stdafx.h"
#include "CaseLevel.h"
#include "CaseItemReference.h"
#include "FixedWidthCaseItem.h"


// --------------------------------------------------------------------------
// CaseLevelMetadata
// --------------------------------------------------------------------------

CaseLevelMetadata::CaseLevelMetadata(const CaseMetadata& case_metadata, const DictLevel& dict_level,
                                     const CaseAccess& case_access, std::tuple<size_t&, size_t&, size_t&>& attribute_counter)
    :   m_caseMetadata(&case_metadata),
        m_dictLevel(dict_level),
        m_levelKeyLength(0),
        m_idCaseRecordMetadata(*this, *m_dictLevel.GetIdItemsRec(), case_access, SIZE_MAX, attribute_counter)
{
    for( int record_counter = 0; record_counter < m_dictLevel.GetNumRecords(); ++record_counter )
    {
        const CDictRecord& dict_record = *(m_dictLevel.GetRecord(record_counter));
        m_caseRecordsMetadata.emplace_back(CaseRecordMetadata(*this, dict_record, case_access, record_counter, attribute_counter));
    }

    // calculate the key length
    const CDictRecord& id_dict_record = *m_dictLevel.GetIdItemsRec();

    for( int i = 0; i < id_dict_record.GetNumItems(); ++i )
        m_levelKeyLength += id_dict_record.GetItem(i)->GetLen();
}


CaseLevelMetadata::~CaseLevelMetadata()
{
}


const CaseRecordMetadata* CaseLevelMetadata::FindCaseRecordMetadata(const std::string_view record_name_sv) const
{
    const CaseRecordMetadata* found_case_record_metadata = nullptr;

    ForeachCaseRecordMetadata(
        [&](const CaseRecordMetadata& case_record_metadata)
        {
            if( case_record_metadata.GetDictRecord().GetName() == record_name_sv )
            {
                found_case_record_metadata = &case_record_metadata;
                return false;
            }

            return true;

        });

    return found_case_record_metadata;
}


const CaseLevelMetadata* CaseLevelMetadata::GetChildCaseLevelMetadata() const
{
    const std::vector<CaseLevelMetadata>& case_levels_metadata = m_caseMetadata->GetCaseLevelsMetadata();
    const size_t child_level_number = m_dictLevel.GetLevelNumber() + 1;

    if( child_level_number < case_levels_metadata.size() )
        return &case_levels_metadata[child_level_number];

    return nullptr;
}



// --------------------------------------------------------------------------
// CaseLevel
// --------------------------------------------------------------------------

CaseLevel::CaseLevel(Case& data_case, const CaseLevelMetadata& case_level_metadata, CaseLevel* const parent_case_level)
    :   m_case(data_case),
        m_caseLevelMetadata(case_level_metadata),
        m_parentCaseLevel(parent_case_level),
        m_numberChildCaseLevels(0),
        m_idCaseRecord(*this, m_caseLevelMetadata.GetIdCaseRecordMetadata())
{
    for( const CaseRecordMetadata& case_record_metadata : m_caseLevelMetadata.GetCaseRecordsMetadata() )
        m_caseRecords.emplace_back(*this, case_record_metadata);
}


CaseLevel::~CaseLevel()
{
}


bool CaseLevel::operator==(const CaseLevel& rhs) const
{
    const std::function<bool(const CaseLevel&, const CaseLevel&)> compare_level =
        [&](const CaseLevel& case_level1, const CaseLevel& case_level2)
        {
            if( case_level1.GetIdCaseRecord() != case_level2.GetIdCaseRecord() )
                return false;

            for( size_t record_number = 0; record_number < case_level1.GetNumberCaseRecords(); ++record_number )
            {
                if( case_level1.GetCaseRecord(record_number) != case_level2.GetCaseRecord(record_number) )
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

    ASSERT(&m_caseLevelMetadata == &rhs.m_caseLevelMetadata);

    return compare_level(*this, rhs);
}


void CaseLevel::Reset()
{
    m_numberChildCaseLevels = 0;
    m_levelIdentifier.Empty();

    // the ID record always exists
    m_idCaseRecord.Reset();
    m_idCaseRecord.SetNumberOccurrences(1);

    for( CaseRecord& case_record : m_caseRecords )
        case_record.Reset();
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

        m_childCaseLevels.emplace_back(std::make_unique<CaseLevel>(m_case, *case_level_metadata, this));
    }

    CaseLevel& child_case_level = *m_childCaseLevels[m_numberChildCaseLevels];
    child_case_level.Reset();

    ++m_numberChildCaseLevels;

    return child_case_level;
}


void CaseLevel::RemoveChildCaseLevel(CaseLevel& child_case_level)
{
    for( size_t level_number = 0; level_number < m_numberChildCaseLevels; ++level_number )
    {
        if( m_childCaseLevels[level_number].get() == &child_case_level )
        {
            // shift all of the subsequent levels
            for( size_t i = level_number + 1; i < m_numberChildCaseLevels; ++i )
                std::swap(m_childCaseLevels[i - 1], m_childCaseLevels[i]);

            --m_numberChildCaseLevels;

            // the old level is at the end and can be resued
            ASSERT(m_childCaseLevels[m_numberChildCaseLevels].get() == &child_case_level);


            // if a case was partially saved on this level or a child level, remove that reference
            Case& data_case = GetCase();

            const CaseItemReference* const partial_save_case_item_reference = data_case.GetPartialSaveCaseItemReference();

            if( partial_save_case_item_reference != nullptr && SO::StartsWith(partial_save_case_item_reference->GetLevelKey(), child_case_level.GetLevelKey()) )
                data_case.SetPartialSaveStatus(data_case.GetPartialSaveMode());

            // remove any notes from this level or child levels
            std::vector<Note>& notes = data_case.GetNotes();

            for( auto note_itr = notes.cbegin(); note_itr != notes.cend(); )
            {
                if( SO::StartsWith(note_itr->GetNamedReference().GetLevelKey(), child_case_level.GetLevelKey()) )
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
    return case_record_metadata.IsIdRecord() ? m_idCaseRecord :
                                               GetCaseRecord(case_record_metadata.GetRecordIndex());
}


const CString& CaseLevel::GetLevelKey() const
{
    return ( m_parentCaseLevel == nullptr ) ? SO::Empty_CString :
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

        const size_t parent_level_key_length = m_levelIdentifier.GetLength();
        const size_t full_key_length = parent_level_key_length + m_caseLevelMetadata.m_levelKeyLength;
        wchar_t* level_key_iterator = m_levelIdentifier.GetBufferSetLength(full_key_length) + parent_level_key_length;

        // add this level's key
        CaseItemIndex index = m_idCaseRecord.GetCaseItemIndex();

        for( const CaseItem* const case_item : m_idCaseRecord.GetCaseItems() )
        {
            ASSERT(case_item->IsFixedWidth());
            dynamic_cast<const FixedWidthCaseItem*>(case_item)->OutputFixedValue(index, level_key_iterator);
            level_key_iterator += case_item->GetDictItem().GetLen();
        }

        m_levelIdentifier.ReleaseBuffer(full_key_length);
    }

    return m_levelIdentifier;
}


void CaseLevel::RecalculateLevelIdentifier(const bool adjust_level_keys/* = true*/)
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
        auto calculate_new_level_key = [&](CString level_key)
        {
            ASSERT(new_level_key.GetLength() <= level_key.GetLength());
            _tcsncpy(level_key.GetBuffer(), new_level_key.GetString(), new_level_key.GetLength());
            level_key.ReleaseBuffer();
            return UTF8_TODO::GetUtf8(level_key);
        };

        // if a case was partially saved on this level or a child level, change that reference
        Case& data_case = GetCase();

        CaseItemReference* partial_save_case_item_reference = data_case.GetPartialSaveCaseItemReference();

        if( partial_save_case_item_reference != nullptr && SO::StartsWith(partial_save_case_item_reference->GetLevelKey(), previous_level_key) )
            partial_save_case_item_reference->SetLevelKey(calculate_new_level_key(UTF8_TODO::GetCString(partial_save_case_item_reference->GetLevelKey())));

        // change the references in notes
        for( Note& note : data_case.GetNotes() )
        {
            NamedReference& named_reference = note.GetNamedReference();

            if( SO::StartsWith(named_reference.GetLevelKey(), previous_level_key) )
                named_reference.SetLevelKey(calculate_new_level_key(UTF8_TODO::GetCString(named_reference.GetLevelKey())));
        }
    }
}
