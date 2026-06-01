#include "stdafx.h"
#include "TableDefinitions.h"

using namespace Paradata;


namespace
{
    constexpr TableDefinition TableDefinitions[] =
    {
        { ParadataTable::MetadataTableInfo,            -1,  "metadata_table_info",          InsertType::AutoIncrementIfUnique },
        { ParadataTable::MetadataColumnInfo,           -1,  "metadata_column_info",         InsertType::AutoIncrementIfUnique },
        { ParadataTable::MetadataCodeInfo,             -1,  "metadata_code_info",           InsertType::AutoIncrementIfUnique },

        { ParadataTable::Name,                         -1,  "name",                         InsertType::AutoIncrementIfUnique },
        { ParadataTable::Text,                         -1,  "text",                         InsertType::AutoIncrementIfUnique },

        { ParadataTable::CaseInfo,                     -1,  "case_info",                    InsertType::AutoIncrementIfUnique },
        { ParadataTable::CaseKeyInfo,                  -1,  "case_key_info",                InsertType::AutoIncrementIfUnique },

        { ParadataTable::FieldOccurrenceInfo,          -1,  "field_occurrence_info",        InsertType::AutoIncrementIfUnique },
        { ParadataTable::FieldInfo,                    -1,  "field_info",                   InsertType::AutoIncrementIfUnique },
        { ParadataTable::FieldValueInfo,               -1,  "field_value_info",             InsertType::AutoIncrementIfUnique },
        { ParadataTable::FieldValidationInfo,          -1,  "field_validation_info",        InsertType::AutoIncrementIfUnique },
        { ParadataTable::FieldEntryInstance,           -1,  "field_entry_instance",         InsertType::AutoIncrement },

        { ParadataTable::BaseEvent,                    -1,  "event",                        InsertType::AutoIncrement },

        { ParadataTable::ApplicationInfo,              -1,  "application_info",             InsertType::AutoIncrementIfUnique },
        { ParadataTable::DiagnosticsInfo,              -1,  "diagnostics_info",             InsertType::AutoIncrementIfUnique },
        { ParadataTable::DeviceInfo,                   -1,  "device_info",                  InsertType::AutoIncrementIfUnique },
        { ParadataTable::ApplicationInstance,          -1,  "application_instance",         InsertType::AutoIncrement },
        { ParadataTable::ApplicationEvent,           1001,  "application_event",            InsertType::WithId },

        { ParadataTable::OperatorIdInfo,               -1,  "operatorid_info",              InsertType::AutoIncrementIfUnique },
        { ParadataTable::SessionInfo,                  -1,  "session_info",                 InsertType::AutoIncrementIfUnique },
        { ParadataTable::SessionInstance,              -1,  "session_instance",             InsertType::AutoIncrement },
        { ParadataTable::SessionEvent,               2001,  "session_event",                InsertType::WithId },

        { ParadataTable::KeyingInstance,               -1,  "keying_instance",              InsertType::AutoIncrement },
        { ParadataTable::CaseInstance,                 -1,  "case_instance",                InsertType::AutoIncrement },
        { ParadataTable::CaseEvent,                  3001,  "case_event",                   InsertType::WithId },

        { ParadataTable::DataRepositoryInfo,           -1,  "data_source_info",             InsertType::AutoIncrementIfUnique },
        { ParadataTable::DataRepositoryInstance,       -1,  "data_source_instance",         InsertType::AutoIncrement },
        { ParadataTable::DataRepositoryEvent,        4001,  "data_source_event",            InsertType::WithId },

        { ParadataTable::MessageEvent,               5001,  "message_event",                InsertType::WithId },

        { ParadataTable::PropertyInfo,                 -1,  "property_info",                InsertType::AutoIncrementIfUnique },
        { ParadataTable::PropertyEvent,              6001,  "property_event",               InsertType::WithId },

        { ParadataTable::OperatorSelectionEvent,     7001,  "operator_selection_event",     InsertType::WithId },

        { ParadataTable::LanguageInfo,                 -1,  "language_info",                InsertType::AutoIncrementIfUnique },
        { ParadataTable::LanguageChangeEvent,        8001,  "language_change_event",        InsertType::WithId },

        { ParadataTable::ExternalApplicationEvent,   9001,  "external_application_event",   InsertType::WithId },

        { ParadataTable::DeviceStateEvent,          10001,  "device_state_event",           InsertType::WithId },

        { ParadataTable::FieldMovementTypeInfo,        -1,  "field_movement_type_info",     InsertType::AutoIncrementIfUnique },
        { ParadataTable::FieldMovementInstance,        -1,  "field_movement_instance",      InsertType::AutoIncrement },
        { ParadataTable::FieldMovementEvent,        11001,  "field_movement_event",         InsertType::WithId },

        { ParadataTable::FieldEntryEvent,           11101,  "field_entry_event",            InsertType::WithId },

        { ParadataTable::FieldValidationEvent,      11201,  "field_validation_event",       InsertType::WithId },

        { ParadataTable::NoteEvent,                 12001,  "note_event",                   InsertType::WithId },

        { ParadataTable::GpsInstance,                  -1,  "gps_instance",                 InsertType::AutoIncrement },
        { ParadataTable::GpsReadingInstance,           -1,  "gps_reading_instance",         InsertType::AutoIncrement },
        { ParadataTable::GpsReadRequestInstance,       -1,  "gps_read_request_instance",    InsertType::AutoIncrement },
        { ParadataTable::GpsEvent,                  13001,  "gps_event",                    InsertType::WithId },

        { ParadataTable::ImputeEvent,               14001,  "impute_event",                 InsertType::WithId },

        { ParadataTable::SyncServiceInfo,              -1,  "sync_service_info",            InsertType::AutoIncrementIfUnique },
        { ParadataTable::SyncServiceInstance,          -1,  "sync_service_instance",        InsertType::AutoIncrement },
        { ParadataTable::SyncConnectionEvent,       15001,  "sync_connection_event",        InsertType::WithId },
        { ParadataTable::SyncDataEvent,             15031,  "sync_data_event",              InsertType::WithId },
        { ParadataTable::SyncMessageEvent,          15011,  "sync_message_event",           InsertType::WithId },
        { ParadataTable::SyncParadataEvent,         15021,  "sync_paradata_event",          InsertType::WithId },
    };

    static_assert(_countof(TableDefinitions) == ParadataTable_NumberTables);
}


const TableDefinition& Paradata::GetTableDefinition(const ParadataTable type)
{
    ASSERT(static_cast<size_t>(type) < _countof(TableDefinitions));

    return TableDefinitions[static_cast<size_t>(type)];
}
