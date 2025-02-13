#pragma once

#include <zParadataO/zParadataO.h>
#include <zParadataO/TableDefinitions.h>
#include <zUtilO/VectorMap.h>


namespace Paradata
{
    class Event;
    class NamedObject;
    class Table;


    // this is not accessible outside of this DLL
    class Log
    {
        friend class Logger;
        friend class Syncer;

    public:
        static constexpr int NumberInstances = 5;

        enum class Instance
        {
            Application,
            Session,
            Case,
            Gps,
            BackgroundGps
        };

    public:
        Log(const std::wstring& filename);
        ~Log();

        Table& CreateTable(ParadataTable type);
        Table& GetTable(ParadataTable type);

        const std::optional<long>& GetInstance(Instance instance_type) const;
        void StartInstance(Instance instance_type, long id);
        void StopInstance(Instance instance_type);

        std::optional<long> GetInstance(const Event& event) const;
        void StartInstance(const Event& event, long id);
        void StopInstance(const Event& event);

        void LogEvent(std::shared_ptr<Event> event, const void* instance_object = nullptr);

        std::optional<long> AddNullableNamedObject(const std::shared_ptr<NamedObject>& named_object);
        long AddNamedObject(const std::shared_ptr<NamedObject>& named_object);

        long AddText(NullTerminatedString text);
        std::optional<long> AddNullableText(const std::optional<CString>& text);
        std::optional<long> AddNullableText(const std::optional<std::wstring>& text);

        long AddOperatorIdInfo(const CString& operator_id);
        long AddCaseInfo(const std::shared_ptr<NamedObject>& dictionary, const CString& case_uuid);
        long AddCaseKeyInfo(const std::shared_ptr<NamedObject>& dictionary, const CString& case_key,
                            const std::optional<long>& case_info_id);

        static ZPARADATAO_API sqlite3* GetDatabaseForTool(NullTerminatedString filename, bool create_new_log);

    private:
        void SetupSharedTables();
        void SetupFieldTables();
        void SetupBaseEventTable();
        void SetupEventTables();

        bool BeginTransaction(bool wait_for_transaction);
        void EndTransaction();

        void WriteEvents(bool wait_for_transaction);

        static bool IsDatabaseVersionOld(sqlite3* db);
        static void SetDatabaseVersion(sqlite3* db);

    private:
        sqlite3* m_db;
        bool m_closeDatabaseOnDestruction;

        std::vector<std::shared_ptr<Table>> m_tables;

        std::optional<long> m_instances[NumberInstances]; // the case instances
        VectorMap<const void*, long> m_eventInstances;    // event-created instances

        std::optional<long> m_firstWrittenEventId;

        std::vector<std::shared_ptr<Event>> m_cachedEvents;
    };
}
