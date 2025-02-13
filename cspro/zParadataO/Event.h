#pragma once
#include "zParadataO.h"
#include "TableDefinitions.h"
#include <zToolsO/DateTime.h>

namespace Paradata
{
    class NamedObject;
    class Log;

    class ZPARADATAO_API Event
    {
        friend class Log;

    protected:
        double m_timestamp;
        const void* m_instanceGeneratingObject;
        std::shared_ptr<NamedObject> m_proc;
        std::optional<int> m_procType;

    private:
        virtual bool PreSave(Log& /*log*/) const { return true; }
        virtual void Save(Log& log, long base_event_id) const = 0;

    public:
        Event()
            :   m_timestamp(::GetTimestamp()),
                m_instanceGeneratingObject(nullptr)
        {
        }

        virtual ~Event()
        {
        }

        virtual ParadataTable GetType() const = 0;

        double GetTimestamp() const
        {
            return m_timestamp;
        }

        void SetTimestamp(double timestamp)
        {
            m_timestamp = timestamp;
        }

        void SetInstanceGeneratingObject(const void* instance_generating_object)
        {
            m_instanceGeneratingObject = instance_generating_object;
        }

        void SetProcInformation(std::shared_ptr<NamedObject> proc, int proc_type)
        {
            m_proc = proc;
            m_procType = proc_type;
        }
    };

    #define DECLARE_PARADATA_EVENT(class_name) \
        friend class Log; \
        protected: \
        ParadataTable GetType() const override { return ParadataTable::class_name; } \
        static void SetupTables(Log& log); \
        void Save(Log& log, long base_event_id) const override;

    #define DECLARE_PARADATA_SHARED_PTR_INSTANCE() \
        friend class Log; \
        protected: \
        static void SetupTables(Log& log); \
        public: \
        long Save(Log& log);
}
