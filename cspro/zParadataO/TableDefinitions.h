#pragma once

#include <zParadataO/zParadataO.h>


namespace Paradata
{
    enum class ParadataTable
    {
        MetadataTableInfo,
        MetadataColumnInfo,
        MetadataCodeInfo,

        Name,
        Text,

        CaseInfo,
        CaseKeyInfo,

        FieldOccurrenceInfo,
        FieldInfo,
        FieldValueInfo,
        FieldValidationInfo,
        FieldEntryInstance,

        BaseEvent,

        ApplicationInfo,
        DiagnosticsInfo,
        DeviceInfo,
        ApplicationInstance,
        ApplicationEvent,

        OperatorIdInfo,
        SessionInfo,
        SessionInstance,
        SessionEvent,

        KeyingInstance,
        CaseInstance,
        CaseEvent,

        DataRepositoryInfo,
        DataRepositoryInstance,
        DataRepositoryEvent,

        MessageEvent,

        PropertyInfo,
        PropertyEvent,

        OperatorSelectionEvent,

        LanguageInfo,
        LanguageChangeEvent,

        ExternalApplicationEvent,

        DeviceStateEvent,

        FieldMovementTypeInfo,
        FieldMovementInstance,
        FieldMovementEvent,

        FieldEntryEvent,

        FieldValidationEvent,

        NoteEvent,

        GpsInstance,
        GpsReadingInstance,
        GpsReadRequestInstance,
        GpsEvent,

        ImputeEvent,

        // when adding tables, make sure to update ParadataTable_NumberTables below
    };

    constexpr size_t ParadataTable_FirstNonMetadataTableIndex = static_cast<size_t>(ParadataTable::MetadataCodeInfo) + 1;
    constexpr size_t ParadataTable_NumberTables = static_cast<size_t>(ParadataTable::ImputeEvent) + 1;

    enum class InsertType
    {
        AutoIncrement,
        AutoIncrementIfUnique,
        WithId,
        WithIdIfNotExist,
    };

    struct TableDefinition
    {
        ParadataTable type;
        int table_code;
        const TCHAR* name;
        InsertType insert_type;
    };

    const ZPARADATAO_API TableDefinition& GetTableDefinition(ParadataTable type);
}
