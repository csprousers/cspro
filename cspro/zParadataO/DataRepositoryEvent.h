#pragma once
#include "Event.h"

namespace Paradata
{
    class ZPARADATAO_API DataRepositoryEvent : public Event
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

    protected:
        Action m_action;
        std::shared_ptr<NamedObject> m_dictionary;

    private:
        CString m_caseUuid;
        CString m_caseKey;
        bool m_partialSave;

    public:
        DataRepositoryEvent(Action action, std::shared_ptr<NamedObject> dictionary,
            const CString& case_uuid = CString(), const CString& case_key = CString(), bool partial_save = false);
    };

    class ZPARADATAO_API DataRepositoryOpenEvent : public DataRepositoryEvent
    {
    private:
        CString m_filename;
        int m_dataRepositoryType;
        int m_dataRepositoryAccess;
        int m_dataRepositoryOpenFlag;

    public:
        DataRepositoryOpenEvent(std::shared_ptr<NamedObject> dictionary, const CString& filename,
            int data_repository_type, int data_repository_access, int data_repository_open_flag);

        void Save(Log& log, long base_event_id) const override;
    };
}
