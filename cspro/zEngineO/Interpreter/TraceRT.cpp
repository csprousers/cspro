#include "stdafx.h"
#include "IncludesRT.h"
#include "Nodes/Trace.h"
#include <zEngineF/TraceHandler.h>


void LogicInterpreter::DoWithTraceHandler(const std::function<void(TraceHandler&)>& callback_function)
{
    ASSERT(callback_function);

    if( m_traceHandler != nullptr )
        callback_function(*m_traceHandler);
}


Engine::Value LogicInterpreter::ex_trace(const int program_index)
{
    const auto& trace_node = GetNode<Nodes::Trace>(program_index);

    auto get_trace_hander = [&]() -> TraceHandler&
    {
        // we need to make sure that a trace handler exists
        if( m_traceHandler == nullptr )
        {
            SendEngineUIMessage(EngineUI::Type::CreateTraceHandler, m_traceHandler);
            ASSERT(m_traceHandler != nullptr);
        }

        return *m_traceHandler;
    };

    // turn trace off
    if( trace_node.action == Nodes::Trace::Action::TurnOff  )
    {
        m_traceHandler.reset();
        return Engine::Value::Bool(true);
    }

    // trace with a window
    if( trace_node.action == Nodes::Trace::Action::WindowOn )
    {
        return Engine::Value::Bool(
            get_trace_hander().TurnOnWindowTrace()
        );
    }

    // trace with a file
    else if( const bool append = ( trace_node.action == Nodes::Trace::Action::FileOn );
             append || trace_node.action == Nodes::Trace::Action::FileOnClear )
    {
        const std::string file_path = EvaluatePath(trace_node.argument);

        return Engine::Value::Bool(
            get_trace_hander().TurnOnFileTrace(file_path, append)
        );
    }

    // trace some text
    else if( m_traceHandler != nullptr )
    {
        ASSERT(trace_node.action == Nodes::Trace::Action::UserText ||
               trace_node.action == Nodes::Trace::Action::LogicText);

        const TraceHandler::OutputType output_type =
            ( trace_node.action == Nodes::Trace::Action::UserText ) ? TraceHandler::OutputType::UserText :
                                                                      TraceHandler::OutputType::LogicText;

        m_traceHandler->Output(Evaluate<SharableString>(trace_node.argument), output_type);
    }

    return Engine::Value::Bool(true);
}
