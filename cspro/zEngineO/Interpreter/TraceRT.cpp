#include "stdafx.h"
#include "IncludesRT.h"
#include "Nodes/Trace.h"
#include <zEngineF/TraceHandler.h>


double CIntDriver::ex_trace(const int program_index)
{
    const auto& trace_node = GetNode<Nodes::Trace>(program_index);

    auto get_trace_hander = [&]() -> TraceHandler&
    {
        // we need to make sure that a trace handler exists
        if( m_traceHandler == nullptr )
            m_traceHandler = TraceHandler::CreateTraceHandler();

        return *m_traceHandler;
    };

    // turn trace off
    if( trace_node.action == Nodes::Trace::Action::TurnOff  )
    {
        m_traceHandler.reset();
        return 1;
    }

    // trace with a window
    if( trace_node.action == Nodes::Trace::Action::WindowOn )
    {
        return get_trace_hander().TurnOnWindowTrace();
    }

    // trace with a file
    else if( const bool append = ( trace_node.action == Nodes::Trace::Action::FileOn ); append || trace_node.action == Nodes::Trace::Action::FileOnClear )
    {
        const std::string file_path = EvaluatePath(trace_node.argument);
        return get_trace_hander().TurnOnFileTrace(file_path, append);
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

    return 1;
}
