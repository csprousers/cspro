#include "stdafx.h"
#include "FieldEntryEvent.h"
#include <zFormO/FormFile.h>

using namespace Paradata;


Table& FieldEntryEvent::AddCaptureType(const char* const column_name, Table& table)
{
    static_assert(CaptureType::Audio == CaptureType::LastDefined);

    return table
            .AddColumn(column_name, Table::ColumnType::Integer)
                    .AddCode(CaptureType::Unspecified, "unspecified")
                    .AddCode(CaptureType::TextBox, "textbox")
                    .AddCode(CaptureType::RadioButton, "radio_button")
                    .AddCode(CaptureType::CheckBox, "checkbox")
                    .AddCode(CaptureType::DropDown, "drop_down")
                    .AddCode(CaptureType::ComboBox, "combo_box")
                    .AddCode(CaptureType::Date, "date")
                    .AddCode(CaptureType::NumberPad, "number_pad")
                    .AddCode(CaptureType::Barcode, "barcode")
                    .AddCode(CaptureType::Slider, "slider")
                    .AddCode(CaptureType::ToggleButton, "toggle_button")
                    .AddCode(CaptureType::Photo, "photo")
                    .AddCode(CaptureType::Signature, "signature")
                    .AddCode(CaptureType::Audio, "audio")
        ;
}


void FieldEntryEvent::SetupTables(Log& log)
{
    Table& field_entry_event_table =
    log.CreateTable(ParadataTable::FieldEntryEvent)
            .AddColumn("arrival_field_movement_instance", Table::ColumnType::Long)
            .AddColumn("field_entry_instance", Table::ColumnType::Long)
            .AddColumn("field_validation_info", Table::ColumnType::Long);
                AddCaptureType("requested_capture_type", field_entry_event_table);
                AddCaptureType("actual_capture_type", field_entry_event_table)
            .AddColumn("display_duration", Table::ColumnType::Double)
        ;
}


FieldEntryEvent::FieldEntryEvent(std::shared_ptr<FieldMovementInstance> arrival_field_movement_instance,
                                 std::shared_ptr<FieldValidationInfo> field_validation_info,
                                 const int requested_capture_type, const int actual_capture_type)
    :   m_arrivalFieldMovementInstance(std::move(arrival_field_movement_instance)),
        m_fieldValidationInfo(std::move(field_validation_info)),
        m_requestedCaptureType(requested_capture_type),
        m_actualCaptureType(actual_capture_type),
        m_displayDuration(0)
{
}


void FieldEntryEvent::SetPostEntryValues()
{
    m_displayDuration = ::GetTimestamp() - this->GetTimestamp();
}


void FieldEntryEvent::Save(Log& log, long base_event_id) const
{
    Table& field_entry_event_table = log.GetTable(ParadataTable::FieldEntryEvent);
    field_entry_event_table.Insert(&base_event_id,
        m_arrivalFieldMovementInstance->Save(log),
        m_arrivalFieldMovementInstance->GetToFieldEntryInstance()->Save(log),
        m_fieldValidationInfo->Save(log),
        m_requestedCaptureType,
        m_actualCaptureType,
        m_displayDuration
    );
}
