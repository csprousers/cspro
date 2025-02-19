#pragma once

#include <zParadataO/Event.h>

namespace Paradata { class CaseEvent; class KeyingInstance; }


class ZPARADATAO_API Paradata::CaseEvent : public Event
{
    DECLARE_PARADATA_EVENT(CaseEvent)

private:
    CaseEvent(bool start, std::shared_ptr<NamedObject> dictionary = nullptr);

public:
    ~CaseEvent();

    static std::unique_ptr<CaseEvent> CreateStartEvent(std::shared_ptr<NamedObject> dictionary, std::string case_uuid);
    static std::unique_ptr<CaseEvent> CreateStopEvent();

private:
    bool PreSave(Log& log) const override;

private:
    const bool m_start;
    const std::shared_ptr<NamedObject> m_dictionary;
    std::string m_caseUuid;
    std::unique_ptr<KeyingInstance> m_keyingInstance;
};
