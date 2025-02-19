#pragma once

#include <zParadataO/Event.h>
#include <zParadataO/FieldInfo.h>

namespace Paradata { class ImputeEvent; }


class ZPARADATAO_API Paradata::ImputeEvent : public Event
{
    DECLARE_PARADATA_EVENT(ImputeEvent)

public:
    ImputeEvent(std::shared_ptr<FieldInfo> field_info, double initial_value, double imputed_value);

private:
    std::shared_ptr<FieldInfo> m_fieldInfo;
    double m_initialValue;
    double m_imputedValue;
};
