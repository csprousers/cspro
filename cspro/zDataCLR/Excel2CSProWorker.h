#pragma once

#include <zDataCLR/Excel2CSProSpec.h>

class CaseAccess;
class DataRepository;
class DataRepositoryTransaction;


namespace CSPro
{
    namespace Data
    {
        namespace Excel2CSPro
        {
            public ref class ConversionCounts sealed
            {
            public:
                ConversionCounts();

                property int Added     { int get(); internal: void set(int); }
                property int Modified  { int get(); internal: void set(int); }
                property int Skipped   { int get(); internal: void set(int); }
                property int Deleted   { int get(); internal: void set(int); }

                property int Converted { int get() { return Added + Modified + Skipped; } }

                property System::Collections::Generic::SortedSet<System::String^>^ DuplicateKeys
                { 
                    System::Collections::Generic::SortedSet<System::String^>^ get() { return m_duplicateKeys; }
                }

                property int WritingProgressPercentage { int get(); internal: void set(int); }

            private:
                int m_added;
                int m_modified;
                int m_skipped;
                int m_deleted;
                System::Collections::Generic::SortedSet<System::String^>^ m_duplicateKeys;
                int m_writingProgressPercentage;
            };


            public ref class ItemConversionInformation sealed
            {
            public:
                ItemConversionInformation(CSPro::Dictionary::DictionaryItem^ item, int excel_column, size_t occurrence);

                property int ExcelColumn { int get() { return m_excelColumn; } } // one-based
                property array<System::Object^, 2>^ ExcelData;

            internal:
                const CDictRecord* Initialize(const CaseAccess& case_access);

                bool ConvertCell(CaseItemIndex& index, size_t processing_row_index);

            private:
                CSPro::Dictionary::DictionaryItem^ m_item;
                int m_excelColumn;
                size_t m_occurrence;
                const CaseItem* m_caseItem;
            };


            public ref class RecordConversionInformation sealed
            {
            public:
                RecordConversionInformation(CSPro::Dictionary::DictionaryRecord^ record, System::Object^ excel_worksheet);

                !RecordConversionInformation();
                ~RecordConversionInformation() { this->!RecordConversionInformation(); }                

                property System::Object^ ExcelWorksheet;
                property int NextRow;
                property int LastRow;

                property bool ReadMoreRows { bool get() { return ( m_rowIndex == m_rowsToProcess ); } }

                property System::Collections::Generic::List<ItemConversionInformation^>^ Items
                {
                    System::Collections::Generic::List<ItemConversionInformation^>^ get() { return m_items; }
                }

                void AddItem(ItemConversionInformation^ item_conversion_information);

                void SetRowsRead(int rows_read);

            internal:
                property bool MoreDataExistsToRead { bool get() { return ( NextRow < LastRow ); } }

                void Initialize(const CaseAccess& case_access);

                CString ConstructKey();

                void CopyIdItems(CaseRecord& case_record);

                void ConstructRecord(CaseLevel& case_level);

                int GetPercentProcessed();

            private:
                CSPro::Dictionary::DictionaryRecord^ m_record;
                System::Collections::Generic::List<ItemConversionInformation^>^ m_items;
                System::Collections::Generic::List<ItemConversionInformation^>^ m_itemsOnIdRecord;
                System::Collections::Generic::List<ItemConversionInformation^>^ m_itemsOnRecord;
                const CaseRecordMetadata* m_caseRecordMetadata;
                std::unique_ptr<Case>* m_caseForKeyConstruction;
                const CaseRecord* m_idCaseRecord;
                CaseItemIndex* m_idIndexForKeyConstruction;
                CaseItemIndex* m_recordIndexForKeyConstruction;
                size_t m_rowsToProcess;
                size_t m_rowIndex;
                bool m_keyHasBeenConstructed;                
            };


            public ref class Worker sealed
            {
            public:
                Worker();

                !Worker();
                ~Worker() { this->!Worker(); }

                property ConversionCounts^ Counts { ConversionCounts^ get() { return m_counts; } }

                void Initialize(CSPro::Dictionary::DataDictionary^ dictionary,
                                System::Collections::Generic::List<RecordConversionInformation^>^ record_conversion_information_list,
                                Spec^ spec);

                void ConstructCases(System::ComponentModel::BackgroundWorker^ background_worker);

                void FinishConversion(System::ComponentModel::BackgroundWorker^ background_worker);

            private:
                ConversionCounts^ m_counts;
                System::Collections::Generic::List<RecordConversionInformation^>^ m_records;
                std::shared_ptr<CaseAccess>* m_caseAccess;
                DataRepository* m_repository;
                std::unique_ptr<DataRepositoryTransaction>* m_transaction;
                bool m_modifyCaseMode;
                std::set<CString>* m_initialKeys;
                std::unique_ptr<Case>* m_case;
                CString* m_caseKey;
                bool m_compareCaseBeforeWriting;
                std::unique_ptr<Case>* m_initialCaseForModificationCheck;
                size_t m_writesUntilNextNotification;
            };
        }
    }
}
