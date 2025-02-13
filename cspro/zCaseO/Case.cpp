#include "stdafx.h"
#include "Case.h"
#include "BinaryCaseItem.h"
#include "CaseConstructionHelpers.h"


CaseMetadata::CaseMetadata(const CDataDict& dictionary, const CaseAccess& case_access)
    :   m_dictionary(dictionary)
{
    // the tuple is: record count (for this level), total record count, total case item count, total binary case item count
    std::tuple<size_t, size_t, size_t, size_t> attribute_counter(0, 0, 0, 0);

    for( const DictLevel& dict_level : dictionary.GetLevels() )
        m_caseLevelsMetadata.emplace_back(new CaseLevelMetadata(*this, dict_level, case_access, attribute_counter));

    ASSERT(!m_caseLevelsMetadata.empty());

    m_totalNumberRecords = std::get<1>(attribute_counter);
    m_totalNumberCaseItems = std::get<2>(attribute_counter);
    m_totalNumberBinaryCaseItems = std::get<3>(attribute_counter);
}


CaseMetadata::~CaseMetadata()
{
    safe_delete_vector_contents(m_caseLevelsMetadata);
}


const CaseLevelMetadata* CaseMetadata::FindCaseLevelMetadata(wstring_view level_name) const
{
    const auto case_level_metadata_search = std::find_if(m_caseLevelsMetadata.cbegin(),
        m_caseLevelsMetadata.cend(), [&](const auto& case_level_metadata)
        { return SO::Equals(case_level_metadata->GetDictLevel().GetName(), level_name); });

    return ( case_level_metadata_search == m_caseLevelsMetadata.cend() ) ? nullptr : *case_level_metadata_search;
}


const CaseRecordMetadata* CaseMetadata::FindCaseRecordMetadata(wstring_view record_name) const
{
    for( const CaseLevelMetadata* case_level_metadata : m_caseLevelsMetadata )
    {
        const CaseRecordMetadata* case_record_metadata = case_level_metadata->FindCaseRecordMetadata(record_name);

        if( case_record_metadata != nullptr )
            return case_record_metadata;
    }

    return nullptr;
}


const CaseItem* CaseMetadata::FindCaseItem(wstring_view item_name) const
{
    const CaseItem* found_case_item;

    auto search_for_case_item = [&](const CaseRecordMetadata* case_record_metadata)
    {
        for( const CaseItem* case_item : case_record_metadata->GetCaseItems() )
        {
            if( SO::Equals(case_item->GetDictionaryItem().GetName(), item_name) )
            {
                found_case_item = case_item;
                return true;
            }
        }

        return false;
    };

    for( const CaseLevelMetadata* case_level_metadata : m_caseLevelsMetadata )
    {
        if( search_for_case_item(case_level_metadata->GetIdCaseRecordMetadata()) )
            return found_case_item;

        for( const CaseRecordMetadata* case_record_metadata : case_level_metadata->GetCaseRecordsMetadata() )
        {
            if( search_for_case_item(case_record_metadata) )
                return found_case_item;
        }
    }

    return nullptr;
}



Case::Case(const CaseMetadata& case_metadata)
    :   m_caseMetadata(case_metadata),
        m_rootCaseLevel(new CaseLevel(*this, *case_metadata.m_caseLevelsMetadata.front(), nullptr))
{
    // have the root case level start in a proper state after construction
    Reset();
}


Case::~Case()
{
    delete m_rootCaseLevel;
}


void Case::Reset()
{
    if( m_pre74Case != nullptr )
        m_pre74Case->Reset();

    m_rootCaseLevel->Reset();

    m_positionInRepository = -1;
    m_uuid.Empty();
    m_caseLabel.Empty();
    m_deleted = false;
    m_verified = false;
    m_partialSaveMode = PartialSaveMode::None;
    m_partialSaveCaseItemReference.reset();
    m_notes.clear();
    m_vectorClock.clear();
}


Case& Case::operator=(const Case& rhs)
{
    Reset();

    // copy the case data using the binary representation of the data
    std::function<void(CaseRecord&, const CaseRecord&)> copy_case_record =
        [](CaseRecord& lhs_case_record, const CaseRecord& rhs_case_record)
    {
        lhs_case_record.SetNumberOccurrences(rhs_case_record.GetNumberOccurrences());

        for( size_t record_occurrence = 0; record_occurrence < rhs_case_record.GetNumberOccurrences(); ++record_occurrence )
            lhs_case_record.CopyValues(rhs_case_record, record_occurrence);
    };

    std::function<void(CaseLevel&, const CaseLevel&)> copy_case_level =
        [&copy_case_record, &copy_case_level](CaseLevel& lhs_case_level, const CaseLevel& rhs_case_level)
    {
        copy_case_record(lhs_case_level.GetIdCaseRecord(), rhs_case_level.GetIdCaseRecord());

        for( size_t record_number = 0; record_number < rhs_case_level.GetNumberCaseRecords(); ++record_number )
            copy_case_record(lhs_case_level.GetCaseRecord(record_number), rhs_case_level.GetCaseRecord(record_number));

        for( size_t level_index = 0; level_index < rhs_case_level.GetNumberChildCaseLevels(); ++level_index )
            copy_case_level(lhs_case_level.AddChildCaseLevel(), rhs_case_level.GetChildCaseLevel(level_index));
    };

    copy_case_level(*m_rootCaseLevel, *rhs.m_rootCaseLevel);

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


void Case::AddRequiredRecords(bool report_additions_using_case_construction_reporter/* = false*/)
{
    report_additions_using_case_construction_reporter |= ( m_caseConstructionReporter != nullptr );

    std::function<void(CaseLevel&)> add_required_records = [&](CaseLevel& case_level)
    {
        for( size_t record_number = 0; record_number < case_level.GetNumberCaseRecords(); ++record_number )
        {
            CaseRecord& case_record = case_level.GetCaseRecord(record_number);

            if( case_record.GetNumberOccurrences() == 0 && case_record.GetCaseRecordMetadata().GetDictionaryRecord().GetRequired() )
            {
                if( report_additions_using_case_construction_reporter )
                    m_caseConstructionReporter->BlankRecordAdded(GetKey(), case_record.GetCaseRecordMetadata().GetDictionaryRecord().GetName());

                case_record.SetNumberOccurrences(1);
            }

            for( size_t level_index = 0; level_index < case_level.GetNumberChildCaseLevels(); ++level_index )
                add_required_records(case_level.GetChildCaseLevel(level_index));
        }
    };

    add_required_records(*m_rootCaseLevel);
}


const CString& Case::GetOrCreateUuid()
{
    if( m_uuid.IsEmpty() )
        m_uuid = WS2CS(CreateUuid());

    return m_uuid;
}


const CString& Case::GetCaseNote() const
{
    return m_notes.empty() ? SO::EmptyCString :
                             CaseConstructionHelpers::LookupCaseNote(m_caseMetadata.GetDictionary().GetName(), m_notes);
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
                    for( const CaseItem* case_item : case_record.GetCaseItems() )
                    {
                        if( !case_item->IsTypeBinary() )
                            continue;

                        const BinaryCaseItem& binary_case_item = assert_cast<const BinaryCaseItem&>(*case_item);

                        CaseItemIndex index = case_record.GetCaseItemIndex(record_occurrence);

                        for( index.SetItemSubitemOccurrence(*case_item, 0);
                             index.GetItemSubitemOccurrence(*case_item) < case_item->GetTotalNumberItemSubitemOccurrences();
                             index.IncrementItemSubitemOccurrence(*case_item) )
                        {
                            if( !binary_case_item.IsBlank(index) )
                                callback_function(binary_case_item, index);
                        }
                    }
                }
            }
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


void Case::LoadAllBinaryData()
{
    if( !m_caseMetadata.UsesBinaryData() )
        return;

    ForeachDefinedBinaryCaseItem(
        [](const BinaryCaseItem& binary_case_item, CaseItemIndex& index)
        {
            BinaryDataAccessor& binary_data_accessor = binary_case_item.GetBinaryDataAccessor(index);
            ASSERT(binary_data_accessor.IsDefined());

            try
            {
                binary_data_accessor.GetBinaryData();
            }

            catch(...)
            {
                // if the data could not be loaded, clear the content
                binary_data_accessor.Clear();
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

        for( size_t i = 0; i < m_rootCaseLevel->GetNumberCaseRecords(); ++i )
        {
            if( m_rootCaseLevel->GetCaseRecord(i).HasOccurrences() )
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
            ASSERT(&dict_record == &case_record.GetCaseRecordMetadata().GetDictionaryRecord());

            for( CaseItemIndex index = case_record.GetCaseItemIndex();
                 index.GetRecordOccurrence() < static_cast<size_t>(pCaseRecord->GetNumRecordOccs());
                 index.IncrementRecordOccurrence() )
            {
                const std::wstring line(pCaseRecord->GetRecordBuffer(index.GetRecordOccurrence()), pCaseRecord->GetRecordLength());

                for( const CDictItem* binary_dict_item : *binary_dict_items )
                {
                    const BinaryCaseItem* binary_case_item = assert_nullable_cast<const BinaryCaseItem*>(m_caseMetadata.FindCaseItem(binary_dict_item->GetName()));

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

                        const BinaryStorageFor80& binary_storage = *pre74_case_level->m_binaryStorageFor80->at(binary_storage_index);
                        const std::variant<const BinaryData*, std::shared_ptr<BinaryDataReader>> binary_data_or_reader = binary_storage.GetBinaryDataOrReader_noexcept(*this);

                        if( std::holds_alternative<std::shared_ptr<BinaryDataReader>>(binary_data_or_reader) )
                        {
                            auto binary_data_reader = std::get<std::shared_ptr<BinaryDataReader>>(binary_data_or_reader);
                            ASSERT(binary_data_reader != nullptr);
                            binary_case_item->GetBinaryDataAccessor(index).SetBinaryDataReader(std::move(binary_data_reader));
                        }

                        else
                        {
                            const BinaryData* binary_data = std::get<const BinaryData*>(binary_data_or_reader);

                            if( binary_data != nullptr )
                            {
                                binary_case_item->GetBinaryDataAccessor(index).SetBinaryData(*binary_data);
                            }

                            else
                            {
                                ASSERT(!binary_storage.binary_data_accessor.IsDefined());
                            }
                        }
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
