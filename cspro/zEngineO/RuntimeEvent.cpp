#include "stdafx.h"
#include "RuntimeEvent.h"
#include "PersistentVariableProcessor.h"
#include "PreinitializedVariable.h"


RuntimeEventsProcessor::RuntimeEventsProcessor(EngineData& engine_data)
    :   m_engineData(engine_data)
{
}


template<typename T>
T& RuntimeEventsProcessor::GetOrCreateEvent()
{
    RuntimeEvent::Type type;

    if constexpr(std::is_same_v<T, PersistentVariableProcessor>)
    {
        type = RuntimeEvent::Type::PersistentVariableProcessor;
    }

    else
    {
        static_assert_false();
    }

    auto event_lookup = std::find_if(m_events.begin(), m_events.end(),
                                     [&](const auto& event) { return ( event->GetType() == type ); });

    std::shared_ptr<T> event;

    if( event_lookup != m_events.end() )
    {
        event = std::dynamic_pointer_cast<T, RuntimeEvent>(*event_lookup);
    }

    else
    {
        if constexpr(std::is_same_v<T, PersistentVariableProcessor>)
        {
            event.reset(new PersistentVariableProcessor(m_engineData));
            m_events.emplace_back(event);
        }

        else
        {
            static_assert_false();
        }
    }

    return *event;
}

template ZENGINEO_API PersistentVariableProcessor& RuntimeEventsProcessor::GetOrCreateEvent();


void RuntimeEventsProcessor::serialize(Serializer& ar)
{
    vector_serialize(ar, m_events,
        [&](std::shared_ptr<RuntimeEvent>& event)
        {
            ASSERT(ar.IsLoading() == ( event == nullptr ));
            RuntimeEvent::Type type;

            if( ar.IsSaving() )
                type = event->GetType();

            if( ar.MeetsVersionIteration(Serializer::Iteration_8_0_000_1) )
            {
                ar.SerializeEnum(type);
            }

            else
            {
                type = RuntimeEvent::Type::PreinitializedVariable;
            }

            if( ar.IsLoading() )
            {
                switch( type )
                {
                    case RuntimeEvent::Type::PreinitializedVariable:
                        event.reset(new PreinitializedVariable(m_engineData));
                        break;

                    case RuntimeEvent::Type::PersistentVariableProcessor:
                        event.reset(new PersistentVariableProcessor(m_engineData));
                        break;

                    default:
                        throw SerializationException(_T("Invalid RuntimeEvent::Type"));
                }
            }

            event->serialize(ar);
        });
}


void RuntimeEventsProcessor::RunEventsOnStart()
{
    // sort the events so that the PersistentVariableProcessor event is last
    std::sort(m_events.begin(), m_events.end(),
        [&](const auto& event1, const auto& event2)
        {
            return ( ( ( event1->GetType() == RuntimeEvent::Type::PersistentVariableProcessor ) ? 1 : 0 ) <
                     ( ( event2->GetType() == RuntimeEvent::Type::PersistentVariableProcessor ) ? 1 : 0 ) );
        });

    for( RuntimeEvent& event : VI_V(m_events) )
        event.OnStart();
}


void RuntimeEventsProcessor::RunEventsOnStop()
{
    for( RuntimeEvent& event : VI_V(m_events) )
        event.OnStop();
}
