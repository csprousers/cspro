#include "stdafx.h"
#include "TableDefinitions.h"

namespace Paradata
{
    const TableDefinition TableDefinitions[] =
    {
        { ParadataTable::MetadataTableInfo, 		   -1,	_T("metadata_table_info"),  		InsertType::AutoIncrementIfUnique },
        { ParadataTable::MetadataColumnInfo,		   -1,	_T("metadata_column_info"), 		InsertType::AutoIncrementIfUnique },
        { ParadataTable::MetadataCodeInfo,  		   -1,	_T("metadata_code_info"),   		InsertType::AutoIncrementIfUnique },

        { ParadataTable::Name,						   -1,  _T("name"), 						InsertType::AutoIncrementIfUnique },
        { ParadataTable::Text,						   -1,  _T("text"), 						InsertType::AutoIncrementIfUnique },

        { ParadataTable::CaseInfo,    				   -1, 	_T("case_info"),     				InsertType::AutoIncrementIfUnique },
        { ParadataTable::CaseKeyInfo, 				   -1, 	_T("case_key_info"), 				InsertType::AutoIncrementIfUnique },

        { ParadataTable::FieldOccurrenceInfo,   	   -1,	_T("field_occurrence_info"), 		InsertType::AutoIncrementIfUnique },
        { ParadataTable::FieldInfo,             	   -1,	_T("field_info"),            		InsertType::AutoIncrementIfUnique },
        { ParadataTable::FieldValueInfo,        	   -1,	_T("field_value_info"),      		InsertType::AutoIncrementIfUnique },
        { ParadataTable::FieldValidationInfo,   	   -1,	_T("field_validation_info"), 		InsertType::AutoIncrementIfUnique },
        { ParadataTable::FieldEntryInstance,    	   -1,	_T("field_entry_instance"),  		InsertType::AutoIncrement },
	
        { ParadataTable::BaseEvent, 				   -1,	_T("event"), 						InsertType::AutoIncrement },
	
        { ParadataTable::ApplicationInfo,     		   -1,	_T("application_info"),     		InsertType::AutoIncrementIfUnique },
        { ParadataTable::DiagnosticsInfo,     		   -1,	_T("diagnostics_info"),     		InsertType::AutoIncrementIfUnique },
        { ParadataTable::DeviceInfo,          		   -1,	_T("device_info"),          		InsertType::AutoIncrementIfUnique },
        { ParadataTable::ApplicationInstance, 		   -1,	_T("application_instance"), 		InsertType::AutoIncrement },
        { ParadataTable::ApplicationEvent,    		 1001,	_T("application_event"),    		InsertType::WithId },

        { ParadataTable::OperatorIdInfo,  			   -1,	_T("operatorid_info"), 				InsertType::AutoIncrementIfUnique },
        { ParadataTable::SessionInfo,     			   -1,	_T("session_info"), 				InsertType::AutoIncrementIfUnique },
        { ParadataTable::SessionInstance, 			   -1,	_T("session_instance"), 			InsertType::AutoIncrement },
        { ParadataTable::SessionEvent,    			 2001,	_T("session_event"), 				InsertType::WithId },

        { ParadataTable::KeyingInstance,               -1,	_T("keying_instance"), 				InsertType::AutoIncrement },
        { ParadataTable::CaseInstance,                 -1,	_T("case_instance"), 				InsertType::AutoIncrement },
        { ParadataTable::CaseEvent,                  3001,  _T("case_event"), 					InsertType::WithId },

        { ParadataTable::DataRepositoryInfo,           -1,  _T("data_source_info"), 			InsertType::AutoIncrementIfUnique },
        { ParadataTable::DataRepositoryInstance,       -1,  _T("data_source_instance"), 		InsertType::AutoIncrement },
        { ParadataTable::DataRepositoryEvent,        4001,  _T("data_source_event"), 			InsertType::WithId },

        { ParadataTable::MessageEvent,               5001,  _T("message_event"), 				InsertType::WithId },

        { ParadataTable::PropertyInfo,                 -1,  _T("property_info"), 				InsertType::AutoIncrementIfUnique },
        { ParadataTable::PropertyEvent,              6001,  _T("property_event"), 				InsertType::WithId },

        { ParadataTable::OperatorSelectionEvent,     7001,  _T("operator_selection_event"), 	InsertType::WithId },

        { ParadataTable::LanguageInfo,                 -1,  _T("language_info"), 				InsertType::AutoIncrementIfUnique },
        { ParadataTable::LanguageChangeEvent,        8001,  _T("language_change_event"), 		InsertType::WithId },

        { ParadataTable::ExternalApplicationEvent,   9001,  _T("external_application_event"), 	InsertType::WithId },

        { ParadataTable::DeviceStateEvent,          10001,  _T("device_state_event"), 			InsertType::WithId },

        { ParadataTable::FieldMovementTypeInfo,        -1,  _T("field_movement_type_info"), 	InsertType::AutoIncrementIfUnique },
        { ParadataTable::FieldMovementInstance,        -1,  _T("field_movement_instance"), 		InsertType::AutoIncrement },
        { ParadataTable::FieldMovementEvent,        11001,  _T("field_movement_event"), 		InsertType::WithId },
	
        { ParadataTable::FieldEntryEvent,           11101,  _T("field_entry_event"), 			InsertType::WithId },
	
        { ParadataTable::FieldValidationEvent,      11201,  _T("field_validation_event"), 		InsertType::WithId },
	
        { ParadataTable::NoteEvent,                 12001,  _T("note_event"), 					InsertType::WithId },

        { ParadataTable::GpsInstance,                  -1,  _T("gps_instance"), 				InsertType::AutoIncrement },
        { ParadataTable::GpsReadingInstance,           -1,  _T("gps_reading_instance"), 		InsertType::AutoIncrement },
        { ParadataTable::GpsReadRequestInstance,       -1,  _T("gps_read_request_instance"),	InsertType::AutoIncrement },
        { ParadataTable::GpsEvent,                  13001,  _T("gps_event"), 					InsertType::WithId },

        { ParadataTable::ImputeEvent,               14001,  _T("impute_event"),                 InsertType::WithId },
    };

    const TableDefinition& GetTableDefinition(ParadataTable type)
    {
        return TableDefinitions[(int)type];
    }
}
