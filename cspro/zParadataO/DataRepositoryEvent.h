#pragma once

#include <zParadataO/Event.h>
#include <zDataO/DataRepositoryDefines.h>

namespace Paradata { class DataRepositoryEvent; class DataRepositoryOpenEvent; }


// --------------------------------------------------------------------------
// DataRepositoryEvent
// --------------------------------------------------------------------------

class ZPARADATAO_API Paradata::DataRepositoryEvent : public Event
{
    DECLARE_PARADATA_EVENT(DataRepositoryEvent)

public:
    enum class Action
    {
        Close,
        Open,
        ReadCase,
        WriteCase,
        DeleteCase,
        CaseNotFound,
        UndeleteCase
    };

public:
    DataRepositoryEvent(Action action, std::shared_ptr<NamedObject> dictionary,
                        std::string case_uuid = std::string(), std::string case_key = std::string(),
                        bool partial_save = false);

private:
    static constexpr int DataRepositoryTypeToParadataInt(DataRepositoryType data_repository_type);

protected:
    const Action m_action;
    const std::shared_ptr<NamedObject> m_dictionary;

private:
    const std::string m_caseUuid;
    const std::string m_caseKey;
    const bool m_partialSave;
};


// --------------------------------------------------------------------------
// DataRepositoryOpenEvent
// --------------------------------------------------------------------------

class ZPARADATAO_API Paradata::DataRepositoryOpenEvent : public DataRepositoryEvent
{
public:
    DataRepositoryOpenEvent(std::shared_ptr<NamedObject> dictionary, std::string repository_name,
                            DataRepositoryType type, DataRepositoryAccess access_type, DataRepositoryOpenFlag open_flag);

    void Save(Log& log, long base_event_id) const override;

private:
    std::string m_repositoryName;
    DataRepositoryType m_dataRepositoryType;
    DataRepositoryAccess m_dataRepositoryAccess;
    DataRepositoryOpenFlag m_dataRepositoryOpenFlag;
};
