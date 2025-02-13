#pragma once
#include "Event.h"
#include "FieldInfo.h"

namespace Paradata
{
    class ZPARADATAO_API ImputeEvent : public Event
    {
        DECLARE_PARADATA_EVENT(ImputeEvent)

    private:
        std::shared_ptr<FieldInfo> m_fieldInfo;
        double m_initialValue;
        double m_imputedValue;

    public:
        ImputeEvent(std::shared_ptr<FieldInfo> field_info, double initial_value, double imputed_value);
    };
}
