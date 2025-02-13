#pragma once

#include <zEngineO/zEngineO.h>

struct EngineData;
class Serializer;


// --------------------------------------------------------------------------
// RuntimeEvent
// --------------------------------------------------------------------------

class RuntimeEvent
{
    friend class RuntimeEventsProcessor;

public:
    virtual ~RuntimeEvent() { }

protected:
    enum class Type: int { PreinitializedVariable, PersistentVariableProcessor };

    virtual Type GetType() const = 0;

    virtual void serialize(Serializer& ar) = 0;

    virtual void OnStart() { }
    virtual void OnStop()  { }
};



// --------------------------------------------------------------------------
// RuntimeEventsProcessor
// --------------------------------------------------------------------------

class ZENGINEO_API RuntimeEventsProcessor
{
public:
    RuntimeEventsProcessor(EngineData& engine_data);

    void AddEvent(std::shared_ptr<RuntimeEvent> event) { m_events.emplace_back(std::move(event)); }

    template<typename T>
    T& GetOrCreateEvent();

    void serialize(Serializer& ar);

    void RunEventsOnStart();
    void RunEventsOnStop();

private:
    EngineData& m_engineData;
    std::vector<std::shared_ptr<RuntimeEvent>> m_events;
};
