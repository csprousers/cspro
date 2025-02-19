#pragma once

#include <zParadataO/zParadataO.h>

template<typename Key, typename Value> class ExpansiveMap;
struct sqlite3;
struct sqlite3_stmt;
class SQLiteStatement;
namespace Paradata { struct ConcatColumn; class Concatenator; struct ConcatTable; }


class ZPARADATAO_API Paradata::Concatenator
{
public:
    Concatenator();
    virtual ~Concatenator();

    void Run(const std::variant<std::string, sqlite3*>& output_file_path_or_database, const std::set<std::string>& paradata_log_file_paths);

protected:
    virtual void OnInputProcessedSuccess(const std::variant<std::string, sqlite3*>& output_file_path_or_database, int64_t events_processed);
    virtual void OnInputProcessedError(const std::string& input_file_path, const char* error_message);

    virtual void OnProgressUpdate(const std::string& operation_message, int operation_percent, const char* total_message, int total_percent);
    virtual bool UserRequestsCancellation();

    int64_t static GetNumberEvents(sqlite3* db);

private:
    template<typename T>
    static T ExecuteSingleQuery(sqlite3* db, cs::string_sz query, std::optional<int> argument = std::nullopt);

    void Cleanup();
    void CloseInputDatabases();

    void CreateOutputFile(const std::string& output_file_path, bool output_is_also_an_input);
    void CreateWorkingDatabase();
    void SetDatabasePragma(sqlite3* db, cs::string_sz pragma, cs::string_sz value, bool add_to_cache);
    void SetDatabasePragmas(sqlite3* db, bool set_journal_mode_off, bool set_synchronous_off);
    void RestoreDatabasePragmas();

    void SetProgressUpdateInFile(const std::variant<std::string, sqlite3*>& output_file_path_or_database, const char* total_message);

    sqlite3* OpenInputFile(const std::string& input_file_path);

    void RunBatch(const std::vector<std::string>& input_file_paths, double total_progress_bar_file_step);

    void AddApplicationInstancesToWorkingDatabase(sqlite3* db);

    void SetupOutputConcatTables();

    sqlite3_stmt* GetInputSelectStatement(ConcatTable& concat_table, const char* where_column_name);
    sqlite3_stmt* GetOutputSelectStatement(ConcatTable& concat_table, int null_values_flag);
    sqlite3_stmt* GetOutputInsertStatement(ConcatTable& concat_table);

    void ManageTransaction(bool begin);

    int64_t ConcatenateEvents(long application_instance_id);

    void BindArguments(ConcatTable& concat_table, sqlite3_stmt* stmt_input, sqlite3_stmt* stmt_output,
                       int output_bind_index, std::set<ConcatTable*>* caller_queried_tables = nullptr);

    void ConcatenateRow(long* id, ConcatTable& concat_table, sqlite3_stmt* stmt_input);

    long ConcatenateAssociatedRow(long input_id, ConcatTable& concat_table, std::set<ConcatTable*>* caller_queried_tables = nullptr);

private:
    sqlite3* m_outputDb;
    bool m_outputDbIsCurrentlyOpenParadataLog;
    std::vector<std::tuple<std::string, std::string>> m_pragmaSettingsToRestore;

    sqlite3* m_workingDb;
    std::unique_ptr<SQLiteStatement> m_stmtInsertWorkingApplicationInstance;
    using WorkingAssociatedRowKey = std::tuple<int, int, long>;
    std::unique_ptr<ExpansiveMap<WorkingAssociatedRowKey, long>> m_workingAssociatedRowKeys;

    std::vector<sqlite3*> m_inputDbs;

    std::vector<ConcatTable*> m_concatTables;
    std::map<int, ConcatTable*> m_eventTypeToConcatTableMap;
    ConcatTable* m_baseEventConcatTable;

    int m_startingFileIndex;
    int m_currentFileIndex;

    double m_operationProgressBarValue;
    double m_totalProgressBarValue;
};
