#include "stdafx.h"
#include "Case.h"
#include "BinaryCaseItem.h"
#include "CaseConstructionHelpers.h"


// --------------------------------------------------------------------------
// CaseMetadata
// --------------------------------------------------------------------------

CaseMetadata::CaseMetadata(const CDataDict& dictionary, const CaseAccess& case_access)
    :   m_dictionary(dictionary),
        m_totalNumberRecords(0),
        m_totalNumberCaseItems(0),
        m_totalNumberBinaryCaseItems(0)
{
    // the tuple is: total record count, total case item count, total binary case item count
    std::tuple<size_t&, size_t&, size_t&> attribute_counter(m_totalNumberRecords, m_totalNumberCaseItems, m_totalNumberBinaryCaseItems);

    for( const DictLevel& dict_level : dictionary.GetLevels() )
        m_caseLevelsMetadata.emplace_back(CaseLevelMetadata(*this, dict_level, case_access, attribute_counter));

    ASSERT(!m_caseLevelsMetadata.empty());

    // update the pointers
    for( CaseLevelMetadata& case_level_metadata : m_caseLevelsMetadata )
    {
        ASSERT(case_level_metadata.m_caseMetadata == this);

        case_level_metadata.ForeachCaseRecordMetadata(
            [&](CaseRecordMetadata& case_record_metadata)
            {
                case_record_metadata.m_caseLevelMetadata = &case_level_metadata;
            });
    }
}


const CaseLevelMetadata* CaseMetadata::FindCaseLevelMetadata(const std::string_view level_name_sv) const
{
    for( const CaseLevelMetadata& case_level_metadata : m_caseLevelsMetadata )
    {
        if( case_level_metadata.GetDictLevel().GetName() == level_name_sv )
            return &case_level_metadata;
    }

    return nullptr;
}


const CaseRecordMetadata* CaseMetadata::FindCaseRecordMetadata(const std::string_view record_name_sv) const
{
    for( const CaseLevelMetadata& case_level_metadata : m_caseLevelsMetadata )
    {
        const CaseRecordMetadata* case_record_metadata = case_level_metadata.FindCaseRecordMetadata(record_name_sv);

        if( case_record_metadata != nullptr )
            return case_record_metadata;
    }

    return nullptr;
}


const CaseItem* CaseMetadata::FindCaseItem(const std::string_view item_name_sv) const
{
    const CaseItem* found_case_item = nullptr;

    for( const CaseLevelMetadata& case_level_metadata : m_caseLevelsMetadata )
    {
        case_level_metadata.ForeachCaseRecordMetadata(
            [&](const CaseRecordMetadata& case_record_metadata)
            {
                for( const CaseItem* const case_item : case_record_metadata.GetCaseItems() )
                {
                    if( case_item->GetDictItem().GetName() == item_name_sv )
                    {
                        found_case_item = case_item;
                        return false;
                    }
                }

                return true;
            });
    }

    return found_case_item;
}



// --------------------------------------------------------------------------
// CaseKey
// --------------------------------------------------------------------------

std::string CaseKey::GetSingleLineKey() const
{
    // turn \n -> ␤
    return NewlineSubstitutor::NewlineToUnicodeNL(GetKey());
}



// --------------------------------------------------------------------------
// CaseSummary
// --------------------------------------------------------------------------

std::string CaseSummary::GetSingleLineCaseLabel() const
{
    // turn \n -> ␤
    return NewlineSubstitutor::NewlineToUnicodeNL(GetCaseLabel());
}



// --------------------------------------------------------------------------
// Case
// --------------------------------------------------------------------------

Case::Case(const CaseMetadata& case_metadata)
    :   m_caseMetadata(case_metadata),
        m_rootCaseLevel(*this, m_caseMetadata.m_caseLevelsMetadata.front(), nullptr)
{
    // have the root case level start in a proper state after construction
    Reset();
}


Case::~Case() // CR_TODO can remove once the Pre74_Case-related unique_ptrs are gone
{
}


Case& Case::operator=(const Case& rhs)
{
    ASSERT(&m_caseMetadata == &rhs.m_caseMetadata);

    Reset();

    // copy the case data using the binary representation of the data
    const std::function<void(CaseRecord&, const CaseRecord&)> copy_case_record =
        [](CaseRecord& lhs_case_record, const CaseRecord& rhs_case_record)
        {
            lhs_case_record.SetNumberOccurrences(rhs_case_record.GetNumberOccurrences());

            for( size_t record_occurrence = 0; record_occurrence < rhs_case_record.GetNumberOccurrences(); ++record_occurrence )
                lhs_case_record.CopyValues(rhs_case_record, record_occurrence);
        };

    const std::function<void(CaseLevel&, const CaseLevel&)> copy_case_level =
        [&copy_case_record, &copy_case_level](CaseLevel& lhs_case_level, const CaseLevel& rhs_case_level)
        {
            copy_case_record(lhs_case_level.GetIdCaseRecord(), rhs_case_level.GetIdCaseRecord());

            for( size_t record_number = 0; record_number < rhs_case_level.GetNumberCaseRecords(); ++record_number )
                copy_case_record(lhs_case_level.GetCaseRecord(record_number), rhs_case_level.GetCaseRecord(record_number));

            for( size_t level_index = 0; level_index < rhs_case_level.GetNumberChildCaseLevels(); ++level_index )
                copy_case_level(lhs_case_level.AddChildCaseLevel(), rhs_case_level.GetChildCaseLevel(level_index));
        };

    copy_case_level(m_rootCaseLevel, rhs.m_rootCaseLevel);

    // copy over any other attributes
    m_positionInRepository = rhs.m_positionInRepository;
    m_uuid = rhs.m_uuid;
    m_caseLabel = rhs.m_caseLabel;
    m_deleted = rhs.m_deleted;
    m_verified = rhs.m_verified;
    m_partialSaveMode = rhs.m_partialSaveMode;
    m_partialSaveCaseItemReference = rhs.m_partialSaveCaseItemReference;
    m_notes = rhs.m_notes;
    m_vectorClock = rhs.m_vectorClock;

    return *this;
}


bool Case::Equals(const Case& rhs, const bool compare_vector_clock/* = true*/) const
{
    if( m_uuid != rhs.m_uuid ||
        m_caseLabel != rhs.m_caseLabel ||
        m_deleted != rhs.m_deleted ||
        m_verified != rhs.m_verified ||
        m_partialSaveMode != rhs.m_partialSaveMode ||
        !NamedReference::AreEqual(m_partialSaveCaseItemReference.get(), rhs.m_partialSaveCaseItemReference.get()) ||
        m_notes.size() != rhs.m_notes.size() ||
        ( compare_vector_clock && m_vectorClock != rhs.m_vectorClock ) ||
        m_rootCaseLevel != rhs.m_rootCaseLevel )
    {
        return false;
    }

    // notes do not need to be in the same order
    for( const Note& note : m_notes )
    {
        if( std::find(rhs.m_notes.cbegin(), rhs.m_notes.cend(), note) == rhs.m_notes.cend() )
            return false;
    }

    return true;
}


void Case::Reset()
{
    if( m_pre74Case != nullptr )
        m_pre74Case->Reset();

    m_rootCaseLevel.Reset();

    m_positionInRepository = -1;
    m_uuid.clear();
    m_caseLabel.clear();
    m_deleted = false;
    m_verified = false;
    m_partialSaveMode = PartialSaveMode::None;
    m_partialSaveCaseItemReference.reset();
    m_notes.clear();
    m_vectorClock.clear();
}


void Case::SetKey(std::string /*key*/)
{
    // the key must be set by modifying case items directly
    throw ProgrammingErrorException();
}


template<typename T>
void Case::GetAllCaseLevelsWorker(std::vector<T*>& case_levels, T& case_level) const
{
    case_levels.emplace_back(&case_level);

    for( size_t level_index = 0; level_index < case_level.GetNumberChildCaseLevels(); ++level_index )
        GetAllCaseLevelsWorker(case_levels, case_level.GetChildCaseLevel(level_index));
}


std::vector<const CaseLevel*> Case::GetAllCaseLevels() const
{
    std::vector<const CaseLevel*> case_levels;
    GetAllCaseLevelsWorker<const CaseLevel>(case_levels, m_rootCaseLevel);
    return case_levels;
}


std::vector<CaseLevel*> Case::GetAllCaseLevels()
{
    std::vector<CaseLevel*> case_levels;
    GetAllCaseLevelsWorker<CaseLevel>(case_levels, m_rootCaseLevel);
    return case_levels;
}


void Case::AddRequiredRecords(bool report_additions_using_case_construction_reporter,
                              std::vector<std::string>* const added_record_names/* = nullptr*/)
{
    if( report_additions_using_case_construction_reporter && m_caseConstructionReporter == nullptr )
        report_additions_using_case_construction_reporter = false;

    const std::function<void(CaseLevel&)> add_required_records =
        [&](CaseLevel& case_level)
        {
            for( size_t record_number = 0; record_number < case_level.GetNumberCaseRecords(); ++record_number )
            {
                CaseRecord& case_record = case_level.GetCaseRecord(record_number);

                if( case_record.GetNumberOccurrences() == 0 )
                {
                    const CDictRecord& dict_record = case_record.GetCaseRecordMetadata().GetDictRecord();

                    if( dict_record.GetRequired() )
                    {
                        if( report_additions_using_case_construction_reporter )
                            m_caseConstructionReporter->BlankRecordAdded(GetKey(), dict_record.GetName());

                        if( added_record_names != nullptr )
                            added_record_names->emplace_back(dict_record.GetName());

                        case_record.SetNumberOccurrences(1);
                    }
                }

                for( size_t level_index = 0; level_index < case_level.GetNumberChildCaseLevels(); ++level_index )
                    add_required_records(case_level.GetChildCaseLevel(level_index));
            }
        };

    add_required_records(m_rootCaseLevel);
}


const std::string& Case::GetOrCreateUuid()
{
    if( m_uuid.empty() )
        m_uuid = CreateUuid();

    return m_uuid;
}


void Case::SetPartialSaveStatus(const PartialSaveMode mode, std::shared_ptr<CaseItemReference> case_item_reference/* = nullptr*/)
{
    ASSERT(( mode != PartialSaveMode::None ) == ( case_item_reference != nullptr ));

    SetPartialSaveMode(mode);
    m_partialSaveCaseItemReference = std::move(case_item_reference);
}


const std::string& Case::GetCaseNote() const
{
    const std::string* const case_note = CaseConstructionHelpers::LookupCaseNote(m_caseMetadata.GetDictionary().GetName(), &m_notes);

    if( case_note != nullptr )
        return *case_note;

    return SO::Empty_string;
}


void Case::SetCaseNote(std::string /*case_note*/)
{
    // the case note must be set by modifying the notes directly
    throw ProgrammingErrorException();
}


void Case::ResetCaseNote()
{
    // the case note must be set by modifying the notes directly
    throw ProgrammingErrorException();
}


template<typename CF>
void Case::ForeachDefinedBinaryCaseItemWorker(const CF& callback_function) const
{
    if( !m_caseMetadata.UsesBinaryData() )
        return;

    ForeachCaseLevel(
        [&](const CaseLevel& case_level)
        {
            // the ID record cannot have binary data so we do not have to process it

            for( size_t record_number = 0; record_number < case_level.GetNumberCaseRecords(); ++record_number )
            {
                const CaseRecord& case_record = case_level.GetCaseRecord(record_number);

                for( size_t record_occurrence = 0; record_occurrence < case_record.GetNumberOccurrences(); ++record_occurrence )
                {
                    for( const CaseItem* const case_item : case_record.GetCaseItems() )
                    {
                        if( !IsBinary(case_item->GetDataType()) )
                            continue;

                        const BinaryCaseItem& binary_case_item = assert_cast<const BinaryCaseItem&>(*case_item);

                        CaseItemIndex index = case_record.GetCaseItemIndex(record_occurrence);

                        for( index.SetItemSubitemOccurrence(*case_item, 0);
                             index.GetItemSubitemOccurrence(*case_item) < case_item->GetTotalNumberItemSubitemOccurrences();
                             index.IncrementItemSubitemOccurrence(*case_item) )
                        {
                            if( !binary_case_item.IsBlank(index) )
                            {
                                if( !CallbackFunctionProcessor::KeepProcessing(callback_function, binary_case_item, index) )
                                    return false;
                            }
                        }
                    }
                }
            }

            return true;
        });
}


void Case::ForeachDefinedBinaryCaseItem(const std::function<void(const BinaryCaseItem&, const CaseItemIndex&)>& callback_function) const
{
    ForeachDefinedBinaryCaseItemWorker(callback_function);
}


void Case::ForeachDefinedBinaryCaseItem(const std::function<void(const BinaryCaseItem&, CaseItemIndex&)>& callback_function)
{
    ForeachDefinedBinaryCaseItemWorker(callback_function);
}


bool Case::HasDefinedBinaryData() const
{
    if( !m_caseMetadata.UsesBinaryData() )
        return false;

    bool keep_processing_to_find_binary_data = true;

    ForeachDefinedBinaryCaseItemWorker(
        [&](const BinaryCaseItem& /*binary_case_item*/, const CaseItemIndex& /*index*/)
        {
            keep_processing_to_find_binary_data = false;
            return keep_processing_to_find_binary_data;
        });

    return !keep_processing_to_find_binary_data;
}


void Case::LoadAllBinaryData()
{
    if( !m_caseMetadata.UsesBinaryData() )
        return;

    ForeachDefinedBinaryCaseItem(
        [](const BinaryCaseItem& binary_case_item, CaseItemIndex& index)
        {
            const BinaryDataAccessor& binary_data_accessor = binary_case_item.GetBinaryDataAccessor(index);
            ASSERT(binary_data_accessor.IsDefined());

            try
            {
                binary_data_accessor.GetBinaryData();
                ASSERT(binary_data_accessor.GetBinaryContentReader() == nullptr);
            }

            catch(...)
            {
                // if the data could not be loaded, clear the content
                binary_case_item.Clear(index);
            }
        });
}



// CR_TODO remove below
#include "TextToCaseConverter.h"
#include <engine/BinaryStorageFor80.h>


Pre74_Case* Case::GetPre74_Case()
{
    if( m_pre74Case == nullptr )
        m_pre74Case = std::make_unique<Pre74_Case>(&m_caseMetadata.GetDictionary());

    if( m_recalculatePre74Case )
    {
        if( m_textToCaseConverter == nullptr )
            m_textToCaseConverter = std::make_unique<TextToCaseConverter>(m_caseMetadata);

        for( size_t i = 0; i < m_rootCaseLevel.GetNumberCaseRecords(); ++i )
        {
            if( m_rootCaseLevel.GetCaseRecord(i).HasOccurrences() )
            {
                CString case_text = m_textToCaseConverter->CaseToTextWide(*this);

                // convert from one buffer to lines
                std::vector<CString> lines;
                int last_pos = 0;
                int nl_pos;

                while( ( nl_pos = case_text.Find(L'\r', last_pos) ) >= 0 )
                {
                    lines.emplace_back(case_text.Mid(last_pos, nl_pos - last_pos));
                    last_pos = nl_pos + 2; // past \r\n
                }

                m_pre74Case->Construct(std::move(lines));
                goto done;
            }
        }

        m_pre74Case->Reset(); // reset if no occurrences

done:
        m_recalculatePre74Case = false;
    }

    return m_pre74Case.get();
}


void Case::ApplyPre74_Case(const Pre74_Case* pre74_case)
{
    std::vector<CString> lines;
    pre74_case->GetCaseLines(lines);

    int buffer_length = 0;

    for( const CString& line : lines )
        buffer_length += line.GetLength() + 1;

    auto buffer = std::make_unique_for_overwrite<TCHAR[]>(buffer_length);
    TCHAR* buffer_itr = buffer.get();

    for( const CString& line : lines )
    {
        _tmemcpy(buffer_itr, line.GetString(), line.GetLength());
        buffer_itr += line.GetLength();

        *buffer_itr = '\n';
        ++buffer_itr;
    }

    if( m_textToCaseConverter == nullptr )
        m_textToCaseConverter = std::make_unique<TextToCaseConverter>(m_caseMetadata);

    m_textToCaseConverter->suppress_using_case_construction_reporter = true;
    m_textToCaseConverter->TextWideToCase(*this, buffer.get(), buffer_length);
    m_textToCaseConverter->suppress_using_case_construction_reporter.reset();

    if( m_caseMetadata.UsesBinaryData() )
        ApplyBinaryDataFor80(pre74_case->GetRootLevel(), GetRootCaseLevel());
}


void Case::ApplyBinaryDataFor80(const Pre74_CaseLevel* pre74_case_level, const CaseLevel& case_level)
{
    // BINARY_TYPES_TO_ENGINE_TODO temporary processing for 8.0
    ASSERT(m_caseMetadata.UsesBinaryData() && pre74_case_level != nullptr);

    if( pre74_case_level->m_binaryStorageFor80 != nullptr && !pre74_case_level->m_binaryStorageFor80->empty() )
    {
        for( int iRecType = 0; iRecType < pre74_case_level->m_iNumRecords; ++iRecType )
        {
            Pre74_CaseRecord* pCaseRecord = const_cast<Pre74_CaseLevel*>(pre74_case_level)->GetRecord(iRecType);

            // check if this record has binary data
            const CDictRecord& dict_record = *pCaseRecord->GetDictRecord();
            std::unique_ptr<std::vector<const CDictItem*>> binary_dict_items;

            for( int i = 0; i < dict_record.GetNumItems(); ++i )
            {
                const CDictItem& dict_item = *dict_record.GetItem(i);

                if( IsBinary(dict_item) )
                {
                    if( binary_dict_items == nullptr )
                        binary_dict_items = std::make_unique<std::vector<const CDictItem*>>();

                    binary_dict_items->emplace_back(&dict_item);
                }
            }

            if( binary_dict_items == nullptr )
                continue;

            // process defined binary data
            const CaseRecord& case_record = case_level.GetCaseRecord(iRecType);
            ASSERT(&dict_record == &case_record.GetCaseRecordMetadata().GetDictRecord());

            for( CaseItemIndex index = case_record.GetCaseItemIndex();
                 index.GetRecordOccurrence() < static_cast<size_t>(pCaseRecord->GetNumRecordOccs());
                 index.IncrementRecordOccurrence() )
            {
                const std::wstring line(pCaseRecord->GetRecordBuffer(index.GetRecordOccurrence()), pCaseRecord->GetRecordLength());

                for( const CDictItem* const binary_dict_item : *binary_dict_items )
                {
                    const BinaryCaseItem* const binary_case_item = assert_nullable_cast<const BinaryCaseItem*>(m_caseMetadata.FindCaseItem(binary_dict_item->GetName()));

                    if( binary_case_item == nullptr )
                        continue;

                    ASSERT(!binary_dict_item->IsSubitem() && index.GetSubitemOccurrence() == 0);

                    for( index.SetItemOccurrence(0);
                         index.GetItemOccurrence() < binary_dict_item->GetOccurs();
                         index.IncrementItemOccurrence() )
                    {
                        ASSERT(binary_dict_item->GetLen() == 1);
                        const size_t data_end = binary_dict_item->GetStart() + index.GetItemOccurrence(); // GetStart is one-based
                        ASSERT(data_end >= 1);
                        const TCHAR data_ch = ( data_end <= line.length() ) ? line[data_end - 1] : 0;

                        if( data_ch < BinaryStorageFor80::BinaryCaseItemCharacterOffset )
                            continue;

                        const size_t binary_storage_index = data_ch - BinaryStorageFor80::BinaryCaseItemCharacterOffset;
                        ASSERT(binary_storage_index < pre74_case_level->m_binaryStorageFor80->size() &&
                               pre74_case_level->m_binaryStorageFor80->at(binary_storage_index) != nullptr);

                        BinaryStorageFor80& binary_storage = *pre74_case_level->m_binaryStorageFor80->at(binary_storage_index);
                        binary_case_item->SetValue(index, binary_storage.binary_data_accessor);
                    }
                }
            }
        }
    }

    // process children levels
    ASSERT(static_cast<size_t>(pre74_case_level->GetNumChildLevels()) == case_level.GetNumberChildCaseLevels());

    for( int iChildLevel = 0; iChildLevel < pre74_case_level->GetNumChildLevels(); ++iChildLevel )
        ApplyBinaryDataFor80(pre74_case_level->GetChildLevel(iChildLevel), case_level.GetChildCaseLevel(iChildLevel));
}
