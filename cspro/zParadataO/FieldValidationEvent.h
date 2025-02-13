#pragma once
#include "Event.h"
#include "FieldInfo.h"

namespace Paradata
{
    class ZPARADATAO_API FieldValidationEvent : public Event
    {
        DECLARE_PARADATA_EVENT(FieldValidationEvent)

    private:
        std::shared_ptr<FieldInfo> m_fieldInfo;
        std::shared_ptr<FieldValidationInfo> m_fieldValidationInfo;
        std::shared_ptr<FieldValueInfo> m_fieldValueInfo;
        std::shared_ptr<FieldEntryInstance> m_fieldEntryInstance;
        bool m_inValueSet;
        std::optional<double> m_onRefusedResult;
        std::optional<bool> m_operatorConfirmed;
        bool m_validated;

    public:
        FieldValidationEvent(std::shared_ptr<FieldInfo> field_info, std::shared_ptr<FieldValidationInfo> field_validation_info,
            std::shared_ptr<FieldValueInfo> field_value_info, std::shared_ptr<FieldEntryInstance> field_entry_instance);

        void SetInValueSet(bool in_value_set);
        void SetOnRefusedResult(double on_refused_result);
        void SetOperatorConfirmed(bool operator_confirmed);
    };
}
