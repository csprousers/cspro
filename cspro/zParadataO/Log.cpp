#include "stdafx.h"
#include "Log.h"
#include "Event.h"
#include "EventList.h"
#include <zUtilO/Versioning.h>
#include <zEngineO/ProcType.h>

using namespace Paradata;


#ifdef _DEBUG
// if the paradata file is changed using the paradata(open, ...) function, then the instances will not be valid
#define PARADATA_CHECK_INSTANCES
#endif


namespace
{
    constexpr size_t EventsPerTransaction = 5000;
    constexpr int MillisecondsToWaitForTransaction = 10000;
}


Log::Log(const std::string& file_path)
    :   m_db(nullptr),
        m_closeDatabaseOnDestruction(true)
{
    const bool file_initially_existed = PortableFunctions::FileExists(file_path);

    if( !PortableFunctions::PathMakeDirectories(PortableFunctions::PathGetDirectory(file_path)) ||
        sqlite3_open(file_path.c_str(), &m_db) != SQLITE_OK )
    {
        throw Exception(Exception::Type::OpenDatabase);
    }

    // initialize the tables, creating the tables and prepared statements
    BeginTransaction(true);

    SetupSharedTables();
    SetupFieldTables();
    SetupBaseEventTable();
    SetupEventTables();

    // check if the the paradata file is using an old schema
    const bool check_for_column_completeness = ( file_initially_existed && IsDatabaseVersionOld(m_db) );

    for( Table& table : VI_V(m_tables) )
        table.CreateTable(check_for_column_completeness);

    for( Table& table : VI_V(m_tables) )
    {
        table.AddMetadata(GetTable(ParadataTable::MetadataTableInfo),
                          GetTable(ParadataTable::MetadataColumnInfo),
                          GetTable(ParadataTable::MetadataCodeInfo));
    }

    if( !file_initially_existed || check_for_column_completeness )
        SetDatabaseVersion(m_db);

    EndTransaction();
}


void Log::SetupSharedTables()
{
    CreateTable(ParadataTable::MetadataTableInfo)
            .AddColumn("table", Table::ColumnType::Text)
            .AddColumn("code", Table::ColumnType::Integer)
            .AddColumn("insert_type", Table::ColumnType::Text)
            .AddIndex("metadata_table_info_index", { 0 });
        ;

    CreateTable(ParadataTable::MetadataColumnInfo)
            .AddColumn("metadata_table_info", Table::ColumnType::Long)
            .AddColumn("column", Table::ColumnType::Text)
            .AddColumn("type", Table::ColumnType::Text)
            .AddColumn("nullable", Table::ColumnType::Boolean)
            .AddIndex("metadata_column_info_index", { 0, 1 });
        ;

    CreateTable(ParadataTable::MetadataCodeInfo)
            .AddColumn("metadata_column_info", Table::ColumnType::Long)
            .AddColumn("code", Table::ColumnType::Integer)
            .AddColumn("value", Table::ColumnType::Text)
            .AddIndex("metadata_code_info_index", { 0, 1 });
        ;

    CreateTable(ParadataTable::Name)
            .AddColumn("type", Table::ColumnType::Integer)
                    .AddCode(NamedObject::Type::Other, "other")
                    .AddCode(NamedObject::Type::Flow, "flow")
                    .AddCode(NamedObject::Type::Dictionary, "dictionary")
                    .AddCode(NamedObject::Type::Item, "item")
                    .AddCode(NamedObject::Type::ValueSet, "valueset")
                    .AddCode(NamedObject::Type::Language, "language")
                    .AddCode(NamedObject::Type::Level, "level")
                    .AddCode(NamedObject::Type::Record, "record")
                    .AddCode(NamedObject::Type::Group, "group")
                    .AddCode(NamedObject::Type::Block, "block")
            .AddColumn("name", Table::ColumnType::Text)
            .AddColumn("parent_name", Table::ColumnType::Long, true)
            .AddIndex("name_index", { 1, 0 });
        ;

    CreateTable(ParadataTable::Text)
            .AddColumn("text", Table::ColumnType::Text)
            .AddIndex("text_index", { 0 });
        ;

    CreateTable(ParadataTable::CaseInfo)
            .AddColumn("dictionary_name", Table::ColumnType::Long)
            .AddColumn("uuid", Table::ColumnType::Text)
            .AddIndex("case_info_index", { 1 });
        ;

    CreateTable(ParadataTable::CaseKeyInfo)
            .AddColumn("dictionary_name", Table::ColumnType::Long)
            .AddColumn("case_info", Table::ColumnType::Long, true)
            .AddColumn("key", Table::ColumnType::Text)
            .AddIndex("case_key_info_index", { 2 });
        ;
}


void Log::SetupFieldTables()
{
    FieldOccurrenceInfo::SetupTables(*this);
    FieldInfo::SetupTables(*this);
    FieldValueInfo::SetupTables(*this);
    FieldValidationInfo::SetupTables(*this);
    FieldEntryInstance::SetupTables(*this);
}


void Log::SetupBaseEventTable()
{
    Table& base_event_table = CreateTable(ParadataTable::BaseEvent);

    base_event_table
            .AddColumn("application_instance", Table::ColumnType::Long, true)
            .AddColumn("session_instance", Table::ColumnType::Long, true)
            .AddColumn("case_instance", Table::ColumnType::Long, true)
            .AddColumn("type", Table::ColumnType::Long);

    // add the table type codes
    for( size_t i = 0; i < ParadataTable_NumberTables; ++i )
    {
        const TableDefinition& table_definition = GetTableDefinition(static_cast<ParadataTable>(i));

        if( table_definition.table_code > 0 )
            base_event_table.AddCode(table_definition.table_code, table_definition.name);
    }

    base_event_table
            .AddColumn("time", Table::ColumnType::Double)
            .AddColumn("proc_name", Table::ColumnType::Long, true)
            .AddColumn("proc_type", Table::ColumnType::Integer, true)
                    .AddCode(ProcType::PreProc, "preproc")
                    .AddCode(ProcType::OnFocus, "onfocus")
                    .AddCode(ProcType::KillFocus, "killfocus")
                    .AddCode(ProcType::PostProc, "postproc")
                    .AddCode(ProcType::OnOccChange, "onoccchange")
        ;
}


void Log::SetupEventTables()
{
    ApplicationEvent::SetupTables(*this);
    SessionEvent::SetupTables(*this);
    CaseEvent::SetupTables(*this);
    DataRepositoryEvent::SetupTables(*this);
    MessageEvent::SetupTables(*this);
    PropertyEvent::SetupTables(*this);
    OperatorSelectionEvent::SetupTables(*this);
    LanguageChangeEvent::SetupTables(*this);
    ExternalApplicationEvent::SetupTables(*this);
    DeviceStateEvent::SetupTables(*this);
    FieldMovementEvent::SetupTables(*this);
    FieldEntryEvent::SetupTables(*this);
    FieldValidationEvent::SetupTables(*this);
    NoteEvent::SetupTables(*this);
    GpsEvent::SetupTables(*this);
    ImputeEvent::SetupTables(*this);
    SyncConnectionEvent::SetupTables(*this);
    SyncMessageEvent::SetupTables(*this);
    SyncParadataEvent::SetupTables(*this);
}


Log::~Log()
{
    try
    {
        WriteEvents(true);
    }

    catch( const Exception& exception ) // ignore any write errors
    {
#if defined(_DEBUG) && defined(WIN_DESKTOP)
        ErrorMessage::Display(exception);
#endif
    }

    // finalize the prepared statements stored in the table objects
    m_tables.clear();

    if( m_closeDatabaseOnDestruction && m_db != nullptr )
        sqlite3_close(m_db);
}


std::string Log::GetFilePath() const
{
    return ( m_db != nullptr) ? Path::ToNativeSlash(sqlite3_db_filename(m_db, nullptr)) :
                                ReturnProgrammingError(std::string());
}


Table& Log::CreateTable(const ParadataTable type)
{
    ASSERT(m_tables.size() == static_cast<size_t>(type));
    m_tables.emplace_back(new Table(m_db, type));
    return *m_tables.back();
}


Table& Log::GetTable(const ParadataTable type)
{
    return *m_tables[static_cast<size_t>(type)];
}


const std::optional<long>& Log::GetInstance(const Instance instance_type) const
{
    const std::optional<long>& instance = m_instances[static_cast<size_t>(instance_type)];

#ifdef PARADATA_CHECK_INSTANCES
    ASSERT(instance.has_value());
#endif

    return instance;
}


void Log::StartInstance(const Instance instance_type, const long id)
{
    m_instances[static_cast<size_t>(instance_type)] = id;
}


void Log::StopInstance(const Instance instance_type)
{
    m_instances[static_cast<size_t>(instance_type)].reset();
}


std::optional<long> Log::GetInstance(const Event& event) const
{
    const long* instance_lookup = m_eventInstances.Find(event.m_instanceGeneratingObject);

#ifdef PARADATA_CHECK_INSTANCES
    ASSERT(instance_lookup != nullptr);
#endif

    return ( instance_lookup != nullptr ) ? (std::optional<long>)*instance_lookup : std::nullopt;
}


void Log::StartInstance(const Event& event, const long id)
{
#ifdef PARADATA_CHECK_INSTANCES
    ASSERT(m_eventInstances.Find(event.m_instanceGeneratingObject) == nullptr);
#endif
    m_eventInstances.Insert(event.m_instanceGeneratingObject, id);
}


void Log::StopInstance(const Event& event)
{
#ifdef PARADATA_CHECK_INSTANCES
    ASSERT(GetInstance(event).has_value());
#endif
    m_eventInstances.Remove(event.m_instanceGeneratingObject);
}


void Log::LogEvent(std::shared_ptr<Event> event, const void* const instance_object/* = nullptr*/)
{
    if( instance_object != nullptr )
        event->SetInstanceGeneratingObject(instance_object);

    m_cachedEvents.emplace_back(std::move(event));

    if( m_cachedEvents.size() >= EventsPerTransaction )
        WriteEvents(false);
}


bool Log::BeginTransaction(const bool wait_for_transaction)
{
    sqlite3_busy_timeout(m_db, wait_for_transaction ? MillisecondsToWaitForTransaction : 0);

    if( sqlite3_exec(m_db, "BEGIN IMMEDIATE;", nullptr, nullptr, nullptr) == SQLITE_OK )
    {
        return true;
    }

    else if( wait_for_transaction )
    {
        throw Exception(Exception::Type::BeginTransaction);
    }

    return false;
}


void Log::EndTransaction()
{
    if( sqlite3_exec(m_db, "COMMIT;", nullptr, nullptr, nullptr) != SQLITE_OK )
        throw Exception(Exception::Type::EndTransaction);
}


void Log::WriteEvents(const bool wait_for_transaction)
{
    if( !BeginTransaction(wait_for_transaction) )
        return;

    for( Event& event : VI_V(m_cachedEvents) )
    {
        if( event.PreSave(*this) )
        {
            Table& table = GetTable(event.GetType());

            // save the base event
            long base_event_id = 0;
            GetTable(ParadataTable::BaseEvent).Insert(&base_event_id,
                GetOptionalValueOrNull(m_instances[0]),
                GetOptionalValueOrNull(m_instances[1]),
                GetOptionalValueOrNull(m_instances[2]),
                table.m_tableDefinition.table_code,
                event.m_timestamp,
                GetOptionalValueOrNull(AddNullableNamedObject(event.m_proc.get())),
                GetOptionalValueOrNull(event.m_procType)
            );

            event.Save(*this, base_event_id);

            if( !m_firstWrittenEventId.has_value() )
                m_firstWrittenEventId = base_event_id;
        }
    }

    EndTransaction();

    m_cachedEvents.clear();
}


std::optional<long> Log::AddNullableNamedObject(NamedObject* const named_object)
{
    if( named_object == nullptr )
        return std::nullopt;

    // if the named object already knows the ID, use it; otherwise, fill the name table
    if( !named_object->m_id.has_value() )
    {
        Table& named_object_table = GetTable(ParadataTable::Name);

        long id = 0;
        named_object_table.Insert(&id,
            static_cast<int>(named_object->GetType()),
            named_object->GetName().c_str(),
            GetOptionalValueOrNull(AddNullableNamedObject(named_object->GetParent()))
        );

        named_object->m_id = id;
    }

    return named_object->m_id;
}


long Log::AddNamedObject(NamedObject* const named_object)
{
    ASSERT(named_object != nullptr);
    return *AddNullableNamedObject(named_object);
}


long Log::AddText(const cs::string_sz text)
{
    // fill the text table
    Table& text_table = GetTable(ParadataTable::Text);
    long text_id = 0;
    text_table.Insert(&text_id,
        text.c_str()
    );

    return text_id;
}


long Log::AddText(const NullTerminatedString text)
{
    // fill the text table
    Table& text_table = GetTable(ParadataTable::Text);
    long text_id = 0;
    text_table.Insert(&text_id,
        UTF8_TODO::GetUtf8(text).c_str()
    );

    return text_id;
}


long Log::AddOperatorIdInfo(const std::string& operator_id)
{
    // fill the operator ID info table
    Table& operator_id_info_table = GetTable(ParadataTable::OperatorIdInfo);
    long operator_id_info_id = 0;
    operator_id_info_table.Insert(&operator_id_info_id,
        operator_id.c_str()
    );

    return operator_id_info_id;
}


long Log::AddCaseInfo(NamedObject* const dictionary, const std::string& case_uuid)
{
    // fill the case info table
    Table& case_info_table = GetTable(ParadataTable::CaseInfo);
    long case_info_id = 0;
    case_info_table.Insert(&case_info_id,
        AddNamedObject(dictionary),
        case_uuid.c_str()
    );

    return case_info_id;
}


long Log::AddCaseKeyInfo(NamedObject* const dictionary, const std::string& case_key, const std::optional<long>& case_info_id)
{
    // fill the case key info table
    Table& case_key_info_table = GetTable(ParadataTable::CaseKeyInfo);
    long case_key_info_id = 0;
    case_key_info_table.Insert(&case_key_info_id,
        AddNamedObject(dictionary),
        GetOptionalValueOrNull(case_info_id),
        case_key.c_str()
    );

    return case_key_info_id;
}


bool Log::IsDatabaseVersionOld(sqlite3* const db)
{
    sqlite3_stmt* stmt;

    if( sqlite3_prepare_v2(db, "PRAGMA user_version;", -1, &stmt, nullptr) != SQLITE_OK ||
        sqlite3_step(stmt) != SQLITE_ROW )
    {
        throw Exception(Exception::Type::Version);
    }

    const int version = sqlite3_column_int(stmt, 0);

    sqlite3_finalize(stmt);

    return ( version < Versioning::GetReleaseDate() );
}


void Log::SetDatabaseVersion(sqlite3* const db)
{
    const std::string sql = FormatText("PRAGMA user_version = %d;", Versioning::GetReleaseDate());

    if( sqlite3_exec(db, sql.c_str(), nullptr, nullptr, nullptr) != SQLITE_OK )
        throw Exception(Exception::Type::Version);
}


sqlite3* Log::GetDatabaseForTool(const std::string& file_path, const bool create_new_log)
{
    sqlite3* db = nullptr;

    // create_new_log is false when a log is opened by the Paradata Viewer or if the log is an
    // input to the Paradata Concatenator, so the database can be opened in read only format
    if( !create_new_log && sqlite3_open_v2(file_path.c_str(), &db, SQLITE_OPEN_READONLY, nullptr) != SQLITE_OK )
        throw Exception(Exception::Type::OpenDatabase);

    // if using an old paradata log, close it and open it with the Log, which should
    // reconcile any table or column differences
    if( create_new_log || Log::IsDatabaseVersionOld(db) )
    {
        if( !create_new_log )
            sqlite3_close(db);

        Log log(file_path);
        log.m_closeDatabaseOnDestruction = false;
        db = log.m_db;
    }

    return db;
}
