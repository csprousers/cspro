#pragma once

#include <zParadataO/zParadataO.h>
#include <zParadataO/TableDefinitions.h>
#include <zToolsO/DateTime.h>
#include <zEngineO/ProcType.h>

namespace Paradata { class Event; class Log; class NamedObject; }


class ZPARADATAO_API Paradata::Event
{
    friend class Log;

public:
    Event();
    virtual ~Event() { }

    virtual ParadataTable GetType() const = 0;

    double GetTimestamp() const         { return m_timestamp; }
    void SetTimestamp(double timestamp) { m_timestamp = timestamp; }

    void SetInstanceGeneratingObject(const void* instance_generating_object) { m_instanceGeneratingObject = instance_generating_object; }

    void SetProcInformation(std::shared_ptr<NamedObject> proc, ProcType proc_type);

private:
    virtual bool PreSave(Log& log) const;
    virtual void Save(Log& log, long base_event_id) const = 0;

protected:
    double m_timestamp;
    const void* m_instanceGeneratingObject;
    std::shared_ptr<NamedObject> m_proc;
    ProcType m_procType;
};


#define DECLARE_PARADATA_EVENT(class_name)                                       \
    friend class Log;                                                            \
protected:                                                                       \
    ParadataTable GetType() const override { return ParadataTable::class_name; } \
    static void SetupTables(Log& log);                                           \
    void Save(Log& log, long base_event_id) const override;


#define DECLARE_PARADATA_SHARED_PTR_INSTANCE() \
    friend class Log;                          \
protected:                                     \
    static void SetupTables(Log& log);         \
public:                                        \
    long Save(Log& log);



// --------------------------------------------------------------------------
// inline implementations
// --------------------------------------------------------------------------

inline Paradata::Event::Event()
    :   m_timestamp(::GetTimestamp<double>()),
        m_instanceGeneratingObject(nullptr),
        m_procType(ProcType::None)
{
}


inline void Paradata::Event::SetProcInformation(std::shared_ptr<NamedObject> proc, const ProcType proc_type)
{
    ASSERT(proc != nullptr && proc_type != ProcType::None);

    m_proc = std::move(proc);
    m_procType = proc_type;
}


inline bool Paradata::Event::PreSave(Log& /*log*/) const
{
    return true;
}
