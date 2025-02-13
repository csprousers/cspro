#include "stdafx.h"
#include "FieldEntryEvent.h"
#include <zFormO/FormFile.h>


namespace Paradata
{
    Table& AddCaptureType(const TCHAR* column_name, Table& table)
    {
        static_assert(CaptureType::Audio == CaptureType::LastDefined);

        return table
                .AddColumn(column_name, Table::ColumnType::Integer)
                        .AddCode((int)CaptureType::Unspecified, _T("unspecified"))
                        .AddCode((int)CaptureType::TextBox, _T("textbox"))
                        .AddCode((int)CaptureType::RadioButton, _T("radio_button"))
                        .AddCode((int)CaptureType::CheckBox, _T("checkbox"))
                        .AddCode((int)CaptureType::DropDown, _T("drop_down"))
                        .AddCode((int)CaptureType::ComboBox, _T("combo_box"))
                        .AddCode((int)CaptureType::Date, _T("date"))
                        .AddCode((int)CaptureType::NumberPad, _T("number_pad"))
                        .AddCode((int)CaptureType::Barcode, _T("barcode"))
                        .AddCode((int)CaptureType::Slider, _T("slider"))
                        .AddCode((int)CaptureType::ToggleButton, _T("toggle_button"))
                        .AddCode((int)CaptureType::Photo, _T("photo"))
                        .AddCode((int)CaptureType::Signature, _T("signature"))
                        .AddCode((int)CaptureType::Audio, _T("audio"))
            ;
    }

    void FieldEntryEvent::SetupTables(Log& log)
    {
        Table& field_entry_event_table =
        log.CreateTable(ParadataTable::FieldEntryEvent)
                .AddColumn(_T("arrival_field_movement_instance"), Table::ColumnType::Long)
                .AddColumn(_T("field_entry_instance"), Table::ColumnType::Long)
                .AddColumn(_T("field_validation_info"), Table::ColumnType::Long);
                 AddCaptureType(_T("requested_capture_type"), field_entry_event_table);
                 AddCaptureType(_T("actual_capture_type"), field_entry_event_table)
                .AddColumn(_T("display_duration"), Table::ColumnType::Double)
            ;
    }

    FieldEntryEvent::FieldEntryEvent(std::shared_ptr<FieldMovementInstance> arrival_field_movement_instance,
        std::shared_ptr<FieldValidationInfo> field_validation_info, int requested_capture_type, int actual_capture_type)
        :   m_arrivalFieldMovementInstance(arrival_field_movement_instance),
            m_fieldValidationInfo(field_validation_info),
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
}
