#pragma once

#include <zParadataO/zParadataO.h>

struct sqlite3;
struct sqlite3_stmt;


namespace Paradata
{
    struct ConcatTable;

    class ZPARADATAO_API Concatenator
    {
    public:
        Concatenator();
        virtual ~Concatenator();

        void Run(std::variant<std::wstring, sqlite3*> output_filename_or_database, const std::set<std::wstring>& paradata_log_filenames);

    protected:
        virtual void OnInputProcessedSuccess(const std::variant<std::wstring, sqlite3*>& filename_or_database, int64_t iEventsProcessed);
        virtual void OnInputProcessedError(NullTerminatedString input_filename, const std::wstring& error_message);

        virtual void OnProgressUpdate(const CString& csOperationMessage, int iOperationPercent, const CString& csTotalMessage, int iTotalPercent);
        virtual bool UserRequestsCancellation();

        int64_t static GetNumberEvents(sqlite3* db);

    private:
        template<typename T>
        static T ExecuteSingleQuery(sqlite3* db, const char* query, std::optional<int> argument = std::nullopt);

        void Cleanup();
        void CloseInputDatabases();

        void CreateOutputFile(NullTerminatedString output_filename, bool bOutputIsAlsoAnInput);
        void CreateWorkingDatabase();
        void SetDatabasePragma(sqlite3* db, const TCHAR* pragma, const TCHAR* value, bool add_to_cache);
        void SetDatabasePragmas(sqlite3* db, bool set_journal_mode_off, bool set_synchronous_off);
        void RestoreDatabasePragmas();

        void SetProgressUpdateInFile(const std::variant<std::wstring, sqlite3*>& filename_or_database, CString csTotalMessage);

        sqlite3* OpenInputFile(NullTerminatedString input_filename);

        void RunBatch(const std::vector<std::wstring>& input_filenames, double dTotalProgressBarFileStep);

        void AddApplicationInstancesToWorkingDatabase(sqlite3* db);

        void SetupOutputConcatTables();

        sqlite3_stmt* GetInputSelectStatement(ConcatTable* pConcatTable,LPCTSTR lpszWhereColumnName);
        sqlite3_stmt* GetOutputSelectStatement(ConcatTable* pConcatTable,int iNullValuesFlag);
        sqlite3_stmt* GetOutputInsertStatement(ConcatTable* pConcatTable);

        ///<summary>The pointer piEventsUntilNextTransaction should be set to nullptr to end the transaction.</summary>
        void ManageTransaction(int* piEventsUntilNextTransaction);

        int64_t ConcatenateEvents(long lApplicationInstanceId);

        void BindArguments(ConcatTable* pConcatTable,sqlite3_stmt* stmtInput,sqlite3_stmt* stmtOutput,int iOutputBindIndex,std::set<ConcatTable*>* pSetCallerQueriedTables = nullptr);

        void ConcatenateRow(long *plId,ConcatTable* pConcatTable,sqlite3_stmt* stmtInput);

        long ConcatenateAssociatedRow(long lInputId,ConcatTable* pConcatTable,std::set<ConcatTable*>* pSetCallerQueriedTables = nullptr);

    private:
        sqlite3* m_OutputDb;
        bool m_bOutputDbIsCurrentlyOpenParadataLog;
        std::vector<std::tuple<CString, CString>> m_pragmaSettingsToRestore;

        sqlite3* m_WorkingDb;
        sqlite3_stmt* m_stmtInsertWorkingApplicationInstance;
        sqlite3_stmt* m_stmtInsertWorkingAssociatedRow;
        sqlite3_stmt* m_stmtQueryWorkingAssociatedRow;

        std::vector<sqlite3*> m_inputDbs;

        std::vector<ConcatTable*> m_apConcatTables;
        std::map<int,ConcatTable*> m_mapEventTypeToConcatTable;
        ConcatTable* m_pBaseEventConcatTable;

        int m_iStartingFileIndex;
        int m_iCurrentFileIndex;

        double m_dOperationProgressBarValue;
        double m_dTotalProgressBarValue;
    };
}
