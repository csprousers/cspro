#include "Stdafx.h"
#include "Excel2CSProWorker.h"
#include <zCaseO/NumericCaseItem.h>
#include <zCaseO/StringCaseItem.h>
#include <zDataO/CaseIterator.h>
#include <zDataO/DataRepositoryHelpers.h>
#include <zDataO/DataRepositoryTransaction.h>


namespace
{
    constexpr int MaxDuplicateKeysToDisplay = 50;
    constexpr size_t WritesPerProgressBarUpdate = 50;
    constexpr const TCHAR* InvalidCellMessage = _T("Excel cell type unknown.");
}


CSPro::Data::Excel2CSPro::ConversionCounts::ConversionCounts()
    :   m_added(0),
        m_modified(0),
        m_skipped(0),
        m_deleted(0),
        m_duplicateKeys(gcnew System::Collections::Generic::SortedSet<System::String^>),
        m_writingProgressPercentage(0)
{
}


int CSPro::Data::Excel2CSPro::ConversionCounts::Added::get()           { return m_added; }
void CSPro::Data::Excel2CSPro::ConversionCounts::Added::set(int value) { m_added = value; }

int CSPro::Data::Excel2CSPro::ConversionCounts::Modified::get()           { return m_modified; }
void CSPro::Data::Excel2CSPro::ConversionCounts::Modified::set(int value) { m_modified = value; }

int CSPro::Data::Excel2CSPro::ConversionCounts::Skipped::get()           { return m_skipped; }
void CSPro::Data::Excel2CSPro::ConversionCounts::Skipped::set(int value) { m_skipped = value; }

int CSPro::Data::Excel2CSPro::ConversionCounts::Deleted::get()           { return m_deleted; }
void CSPro::Data::Excel2CSPro::ConversionCounts::Deleted::set(int value) { m_deleted = value; }

int CSPro::Data::Excel2CSPro::ConversionCounts::WritingProgressPercentage::get()           { return m_writingProgressPercentage; }
void CSPro::Data::Excel2CSPro::ConversionCounts::WritingProgressPercentage::set(int value) { m_writingProgressPercentage = value; }



CSPro::Data::Excel2CSPro::ItemConversionInformation::ItemConversionInformation(CSPro::Dictionary::DictionaryItem^ item, int excel_column, size_t occurrence)
    :   m_item(item),
        m_excelColumn(excel_column),
        m_occurrence(occurrence),
        m_caseItem(nullptr)
{
}


const CDictRecord* CSPro::Data::Excel2CSPro::ItemConversionInformation::Initialize(const CaseAccess& case_access)
{
    m_caseItem = case_access.LookupCaseItem(*m_item->GetNativePointer());

    ASSERT(m_caseItem != nullptr &&
           m_occurrence < m_caseItem->GetTotalNumberItemSubitemOccurrences());

    if( m_caseItem->IsTypeBinary() )
    {
        throw CSProException(_T("Converting to binary dictionary items ('%s') is not supported."),
                             m_caseItem->GetDictionaryItem().GetName().GetString());
    }

    ASSERT(m_caseItem->IsTypeFixed());

    return m_caseItem->GetDictionaryItem().GetRecord();
}


namespace
{
    bool ConvertCellToNumber(System::Object^ cell_data, double* value)
    {
        double^ double_value = dynamic_cast<double^>(cell_data);

        if( double_value != nullptr )
        {
            *value = *double_value;
            return true;
        }

        int^ int_value = dynamic_cast<int^>(cell_data);

        if( int_value != nullptr )
        {
            *value = *int_value;
            return true;
        }

        bool^ bool_value = dynamic_cast<bool^>(cell_data);

        if( bool_value != nullptr )
        {
            *value = *bool_value;
            return true;
        }

        return false;
    }
}


bool CSPro::Data::Excel2CSPro::ItemConversionInformation::ConvertCell(CaseItemIndex& index, size_t processing_row_index)
{
    // if only one row of the data, the lower bound is 0; otherwise it is 1
    const int lower_bound = ExcelData->GetLowerBound(0);

    System::Object^ cell_data = ExcelData[processing_row_index + lower_bound, lower_bound];
    bool cell_is_blank = ( cell_data == nullptr );

    index.SetItemSubitemOccurrence(*m_caseItem, m_occurrence);

    // convert cells to numbers
    if( m_caseItem->IsTypeNumeric() )
    {
        double value = NOTAPPL;

        if( !cell_is_blank )
        {
            if( !ConvertCellToNumber(cell_data, &value) )
            {
                System::String^ string_value = dynamic_cast<System::String^>(cell_data);

                if( string_value != nullptr )
                {
                    if( System::String::IsNullOrWhiteSpace(string_value) )
                    {
                        cell_is_blank = true;
                    }

                    // if the value can't be represented as a double, make it default
                    else if( !System::Double::TryParse(string_value, value) )
                    {
                        value = DEFAULT;
                    }
                }

                else
                {
                    throw CSProException(InvalidCellMessage);
                }
            }
        }

        static_cast<const NumericCaseItem*>(m_caseItem)->SetValue(index, value);
    }

    // convert cells to strings
    else
    {
        ASSERT(m_caseItem->IsTypeString());
        CString value;

        if( !cell_is_blank )
        {
            System::String^ string_value = dynamic_cast<System::String^>(cell_data);

            if( string_value == nullptr )
            {
                double numeric_value;

                // format the double showing the maximum number of decimal digits that Excel supports (15)
                if( ConvertCellToNumber(cell_data, &numeric_value) )
                {
                    string_value = System::String::Format("{0:0.###############}", numeric_value);
                }

                else
                {
                    throw CSProException(InvalidCellMessage);
                }
            }

            value = string_value;
        }

        static_cast<const StringCaseItem*>(m_caseItem)->SetValue(index, value);
    }

    return !cell_is_blank;
}


CSPro::Data::Excel2CSPro::RecordConversionInformation::RecordConversionInformation(CSPro::Dictionary::DictionaryRecord^ record, System::Object^ excel_worksheet)
    :   m_record(record),
        m_items(gcnew System::Collections::Generic::List<ItemConversionInformation^>),
        m_itemsOnIdRecord(gcnew System::Collections::Generic::List<ItemConversionInformation^>),
        m_itemsOnRecord(gcnew System::Collections::Generic::List<ItemConversionInformation^>),
        m_caseRecordMetadata(nullptr),
        m_caseForKeyConstruction(new std::unique_ptr<Case>),
        m_idCaseRecord(nullptr),
        m_idIndexForKeyConstruction(nullptr),
        m_recordIndexForKeyConstruction(nullptr),
        m_rowsToProcess(0),
        m_rowIndex(0),
        m_keyHasBeenConstructed(false)
{
    ExcelWorksheet = excel_worksheet;
    NextRow = 0;
    LastRow = 0;
}


CSPro::Data::Excel2CSPro::RecordConversionInformation::!RecordConversionInformation()
{
    delete m_caseForKeyConstruction;
    delete m_idIndexForKeyConstruction;
    delete m_recordIndexForKeyConstruction;
}


void CSPro::Data::Excel2CSPro::RecordConversionInformation::AddItem(ItemConversionInformation^ item_conversion_information)
{
    m_items->Add(item_conversion_information);
}


void CSPro::Data::Excel2CSPro::RecordConversionInformation::SetRowsRead(int rows_read)
{
    ASSERT(m_rowIndex == m_rowsToProcess);
    m_rowsToProcess = rows_read;
    m_rowIndex = 0;
}


void CSPro::Data::Excel2CSPro::RecordConversionInformation::Initialize(const CaseAccess& case_access)
{
    // link the record
    for( const CaseRecordMetadata* case_record_metadata : case_access.GetCaseMetadata().GetCaseLevelsMetadata()[0]->GetCaseRecordsMetadata() )
    {
        if( &case_record_metadata->GetDictionaryRecord() == m_record->GetNativePointer() )
        {
            m_caseRecordMetadata = case_record_metadata;
            break;
        }
    }

    ASSERT(m_caseRecordMetadata != nullptr);

    // initialize the items
    for each( ItemConversionInformation^ item in m_items )
    {
        const CDictRecord* parent_record = item->Initialize(case_access);

        if( parent_record == m_record->GetNativePointer() )
        {
            m_itemsOnRecord->Add(item);
        }

        else
        {
            ASSERT(&m_caseRecordMetadata->GetCaseLevelMetadata().GetIdCaseRecordMetadata()->GetDictionaryRecord() == parent_record);
            m_itemsOnIdRecord->Add(item);
        }
    }

    // create a case that will be used to construct keys for this record
    *m_caseForKeyConstruction = case_access.CreateCase();

    const CaseLevel& root_case_level = (*m_caseForKeyConstruction)->GetRootCaseLevel();
    m_idCaseRecord = &root_case_level.GetIdCaseRecord();
    m_idIndexForKeyConstruction = new CaseItemIndex(m_idCaseRecord->GetCaseItemIndex());

    CaseRecord& case_record = (*m_caseForKeyConstruction)->GetRootCaseLevel().GetCaseRecord(m_caseRecordMetadata->GetRecordIndex());
    case_record.SetNumberOccurrences(1);

    m_recordIndexForKeyConstruction = new CaseItemIndex(case_record.GetCaseItemIndex());
}


CString CSPro::Data::Excel2CSPro::RecordConversionInformation::ConstructKey()
{
    // use a previously constructed key if this record wasn't already added to a case
    if( m_keyHasBeenConstructed )
        return (*m_caseForKeyConstruction)->GetKey();

    // find a non-blank row to construct the key
    for( ; m_rowIndex < m_rowsToProcess; ++m_rowIndex )
    {
        bool row_has_entries = false;

        for each( ItemConversionInformation^ id_item in m_itemsOnIdRecord )
            row_has_entries |= id_item->ConvertCell(*m_idIndexForKeyConstruction, m_rowIndex);

        // if none of the IDs were filled, see if any of the record items are filled
        if( !row_has_entries )
        {
            for each( ItemConversionInformation^ record_item in m_itemsOnRecord )
            {
                if( record_item->ConvertCell(*m_recordIndexForKeyConstruction, m_rowIndex) )
                {
                    row_has_entries = true;
                    break;
                }
            }
        }

        if( row_has_entries )
        {
            m_keyHasBeenConstructed = true;
            return (*m_caseForKeyConstruction)->GetKey();
        }
    }

    // no more rows to process
    return CString();
}


void CSPro::Data::Excel2CSPro::RecordConversionInformation::CopyIdItems(CaseRecord& case_record)
{
    case_record.CopyValues(*m_idCaseRecord, 0);
}


void CSPro::Data::Excel2CSPro::RecordConversionInformation::ConstructRecord(CaseLevel& case_level)
{
    ASSERT(m_rowIndex < m_rowsToProcess);

    // add an occurrence if possible
    CaseRecord& case_record = case_level.GetCaseRecord(m_caseRecordMetadata->GetRecordIndex());
    size_t occurrence = case_record.GetNumberOccurrences();

    if( occurrence < m_caseRecordMetadata->GetDictionaryRecord().GetMaxRecs() )
    {
        case_record.SetNumberOccurrences(occurrence + 1);
        CaseItemIndex index = case_record.GetCaseItemIndex(occurrence);

        for each( ItemConversionInformation^ record_item in m_itemsOnRecord )
            record_item->ConvertCell(index, m_rowIndex);
    }

    ++m_rowIndex;
    m_keyHasBeenConstructed = false;
}


int CSPro::Data::Excel2CSPro::RecordConversionInformation::GetPercentProcessed()
{
    return ( m_rowsToProcess == 0 ) ? 0 : ( 100 * m_rowIndex / m_rowsToProcess);
}


CSPro::Data::Excel2CSPro::Worker::Worker()
    :   m_counts(gcnew ConversionCounts),
        m_records(nullptr),
        m_caseAccess(new std::shared_ptr<CaseAccess>),
        m_repository(nullptr),
        m_transaction(new std::unique_ptr<DataRepositoryTransaction>),
        m_modifyCaseMode(true),
        m_initialKeys(nullptr),
        m_case(new std::unique_ptr<Case>),
        m_caseKey(new CString),
        m_compareCaseBeforeWriting(false),
        m_initialCaseForModificationCheck(new std::unique_ptr<Case>),
        m_writesUntilNextNotification(WritesPerProgressBarUpdate)
{
}


CSPro::Data::Excel2CSPro::Worker::!Worker()
{
    delete m_initialCaseForModificationCheck;
    delete m_caseKey;
    delete m_case;
    delete m_initialKeys;
    delete m_transaction;
    delete m_repository;
    delete m_caseAccess;
}


void CSPro::Data::Excel2CSPro::Worker::Initialize(CSPro::Dictionary::DataDictionary^ dictionary,
                                                 System::Collections::Generic::List<RecordConversionInformation^>^ record_conversion_information_list,
                                                 Spec^ spec)
{
    try
    {
        // create the case access
        CDataDict* native_dictionary = dictionary->GetNativePointer();

        m_caseAccess = new std::shared_ptr<CaseAccess>(CaseAccess::CreateAndInitializeFullCaseAccess(*native_dictionary));
        CaseAccess& case_access = *(*m_caseAccess);

        *m_case = case_access.CreateCase();
        *m_initialCaseForModificationCheck = case_access.CreateCase();

        // initialize the records
        m_records = record_conversion_information_list;

        for each( RecordConversionInformation^ record in m_records )
            record->Initialize(case_access);

        // open the repository
        const ::ConnectionString& native_connection_string = spec->OutputConnectionString->GetNativeConnectionString();
        DataRepositoryAccess access_type = DataRepositoryAccess::ReadWrite;
        DataRepositoryOpenFlag open_flag = DataRepositoryOpenFlag::OpenMustExist;

        CaseManagement case_management = spec->CaseManagement;

        if( case_management != CaseManagement::CreateNewFile &&
            native_connection_string.IsFilenamePresent() &&
            !PortableFunctions::FileIsRegular(native_connection_string.GetFilename()) )
        {
            case_management = CaseManagement::CreateNewFile;
        }

        if( case_management == CaseManagement::CreateNewFile )
        {
            if( DataRepositoryHelpers::TypeSupportsIndexedQueriesInBatchOutput(native_connection_string.GetType()) )
                access_type = DataRepositoryAccess::BatchOutput;

            open_flag = DataRepositoryOpenFlag::CreateNew;
            m_modifyCaseMode = false;
        }

        m_repository = DataRepository::CreateAndOpen(*m_caseAccess, native_connection_string, access_type, open_flag).release();

        *m_transaction = std::make_unique<DataRepositoryTransaction>(*m_repository);

        // if deleting cases, keep track of the cases that were initially in the file
        if( case_management == CaseManagement::ModifyAddDeleteCases )
        {
            m_initialKeys = new std::set<CString>;

            CaseKey case_key;
            auto case_key_iterator = m_repository->CreateCaseKeyIterator(CaseIterationMethod::SequentialOrder, CaseIterationOrder::Ascending);

            while( case_key_iterator->NextCaseKey(case_key) )
                m_initialKeys->insert(case_key.GetKey());
        }
    }

    catch( const CSProException& exception )
    {
        throw gcnew System::Exception(gcnew System::String(exception.GetErrorMessage().c_str()));
    }
}


void CSPro::Data::Excel2CSPro::Worker::ConstructCases(System::ComponentModel::BackgroundWorker^ background_worker)
{
    try
    {
        Case& data_case = *(*m_case);
        CaseLevel& root_case_level = data_case.GetRootCaseLevel();

        std::vector<bool> include_record(m_records->Count, false);

        while( !background_worker->CancellationPending )
        {
            // potentially use the key from records that were part of the last read
            const bool use_existing_key = !m_caseKey->IsEmpty();
            size_t last_record_to_use_index = SIZE_MAX;

            for( size_t i = 0; i < include_record.size(); ++i )
            {
                CString key = m_records[i]->ConstructKey();
                bool include_this_record = true;

                if( key.IsEmpty() )
                {
                    // if more data exists, read it before continuing the processing
                    if( m_records[i]->MoreDataExistsToRead )
                        goto stop_processing;

                    include_this_record = false;
                }

                else if( use_existing_key )
                {
                    include_this_record = ( key.Compare(*m_caseKey) == 0 );
                }

                // if the first record with a key, use it
                else if( m_caseKey->IsEmpty() )
                {
                    *m_caseKey = key;
                }

                // otherwise see if this record's key comes first
                else
                {
                    const int key_comparison = key.Compare(*m_caseKey);

                    // if less than, use this key
                    if( key_comparison < 0 )
                    {
                        *m_caseKey = key;

                        // ignore previously processed records
                        for( size_t j = 0; j < i; ++j )
                            include_record[j] = false;
                    }

                    // if greater than, ignore the record
                    if( key_comparison > 0 )
                        include_this_record = false;
                }

                include_record[i] = include_this_record;

                if( include_this_record )
                    last_record_to_use_index = i;
            }

            // if searching for a new key...
            if( !use_existing_key )
            {
                // if no data remains, quit out
                if( last_record_to_use_index == SIZE_MAX )
                {
                    ASSERT(m_caseKey->IsEmpty());
                    goto stop_processing;
                }

                ASSERT(!m_caseKey->IsEmpty());

                // load the existing case while in modify mode so that the UUID, notes, and other attributes are maintained
                if( m_modifyCaseMode && m_repository->ContainsCase(*m_caseKey) )
                {
                    if( m_initialKeys != nullptr )
                        m_initialKeys->erase(*m_caseKey);

                    m_repository->ReadCase(data_case, *m_caseKey);

                    m_compareCaseBeforeWriting = true;
                    *(*m_initialCaseForModificationCheck) = data_case;

                    // clear any existing records
                    for( size_t record_number = 0; record_number < root_case_level.GetNumberCaseRecords(); ++record_number )
                        root_case_level.GetCaseRecord(record_number).SetNumberOccurrences(0);
                }

                // otherwise, copy over the IDs
                else
                {
                    m_compareCaseBeforeWriting = false;
                    m_records[last_record_to_use_index]->CopyIdItems(root_case_level.GetIdCaseRecord());
                }
            }


            // construct the records
            if( last_record_to_use_index != SIZE_MAX )
            {
                for( size_t i = 0; i < include_record.size(); ++i )
                {
                    if( include_record[i] )
                        m_records[i]->ConstructRecord(root_case_level);
                }
            }

            // if none of these records belong to the existing key, write out the case
            else
            {
                if( use_existing_key )
                {
                    if( !m_modifyCaseMode && m_repository->ContainsCase(*m_caseKey) )
                    {
                        if( Counts->DuplicateKeys->Count < MaxDuplicateKeysToDisplay )
                            Counts->DuplicateKeys->Add(gcnew System::String(*m_caseKey));
                    }

                    else
                    {
                        // add any required records
                        data_case.AddRequiredRecords();

                        // check if the case has changed from what was initially in the file
                        if( m_compareCaseBeforeWriting && data_case.GetRootCaseLevel() == (*m_initialCaseForModificationCheck)->GetRootCaseLevel() )
                        {
                            ++Counts->Skipped;
                        }

                        // write the case
                        else
                        {
                            m_repository->WriteCase(data_case);

                            if( m_compareCaseBeforeWriting )
                            {
                                ++Counts->Modified;
                            }

                            else
                            {
                                ++Counts->Added;
                            }
                        }
                    }
                }

                m_caseKey->Empty();
                data_case.Reset();

                // update the progress bar
                if( --m_writesUntilNextNotification == 0 )
                {
                    // use the number of processed rows for the first record as a rough estimate of the percent processed
                    Counts->WritingProgressPercentage = m_records[0]->GetPercentProcessed();
                    background_worker->ReportProgress(System::Int32::MinValue, Counts);
                    m_writesUntilNextNotification = WritesPerProgressBarUpdate;
                }
            }
        }

stop_processing:
        Counts->WritingProgressPercentage = 0;
        background_worker->ReportProgress(System::Int32::MinValue, Counts);
    }

    catch( const CSProException& exception )
    {
        throw gcnew System::Exception(gcnew System::String(exception.GetErrorMessage().c_str()));
    }
}


void CSPro::Data::Excel2CSPro::Worker::FinishConversion(System::ComponentModel::BackgroundWorker^ background_worker)
{
    try
    {
        // delete cases that weren't in the Excel file (if applicable)
        if( m_initialKeys != nullptr )
        {
            for( const CString& key : *m_initialKeys )
            {
                if( background_worker->CancellationPending )
                    return;

                try
                {
                    m_repository->DeleteCase(key);
                    ++Counts->Deleted;
                }

                // ignore deletion problems
                catch( const DataRepositoryException::CaseNotFound& ) { }
            }
        }

        m_transaction->reset();

        m_repository->Close();
    }

    catch( const CSProException& exception )
    {
        throw gcnew System::Exception(gcnew System::String(exception.GetErrorMessage().c_str()));
    }
}
