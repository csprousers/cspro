#pragma once

#include <zParadataO/zParadataO.h>
#include <zParadataO/TableDefinitions.h>
#include <zUtilO/VectorMap.h>

namespace Paradata { class Event; class Log; class NamedObject; class Table; }
struct sqlite3;


// this is not accessible outside of this DLL

class Paradata::Log
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
    Log(const std::string& file_path);
    ~Log();

    std::string GetFilePath() const;

    Table& CreateTable(ParadataTable type);
    Table& GetTable(ParadataTable type);

    const std::optional<long>& GetInstance(Instance instance_type) const;
    void StartInstance(Instance instance_type, long id);
    void StopInstance(Instance instance_type);

    std::optional<long> GetInstance(const Event& event) const;
    void StartInstance(const Event& event, long id);
    void StopInstance(const Event& event);

    void LogEvent(std::shared_ptr<Event> event, const void* instance_object = nullptr);

    std::optional<long> AddNullableNamedObject(NamedObject* named_object);
    long AddNamedObject(NamedObject* named_object);

    long AddText(cs::string_sz text);
    long AddText(NullTerminatedString text);

    template<typename T>
    std::optional<long> AddNullableText(const T& optional_or_shared_ptr_text);

    std::optional<long> AddNullableText(const SharableString& sharable_string);

    long AddOperatorIdInfo(const std::string& operator_id);
    long AddCaseInfo(NamedObject* dictionary, const std::string& case_uuid);
    long AddCaseKeyInfo(NamedObject* dictionary, const std::string& case_key, const std::optional<long>& case_info_id);

    static ZPARADATAO_API sqlite3* GetDatabaseForTool(const std::string& file_path, bool create_new_log);

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



// --------------------------------------------------------------------------
// inline implementations
// --------------------------------------------------------------------------

template<typename T>
std::optional<long> Paradata::Log::AddNullableText(const T& optional_or_shared_ptr_text)
{
    if constexpr(cs::is_optional<T>::value)
    {
        if( !optional_or_shared_ptr_text.has_value() )
            return std::nullopt;
    }

    else
    {
        if( optional_or_shared_ptr_text == nullptr )
            return std::nullopt;
    }

    return AddText(*optional_or_shared_ptr_text);
}


inline std::optional<long> Paradata::Log::AddNullableText(const SharableString& sharable_string)
{
    if( sharable_string.IsSet() )
        return AddText(*sharable_string);

    return std::nullopt;
}
