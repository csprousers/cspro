#include "stdafx.h"
#include "IncludesRT.h"
#include "Video.h"


double LogicInterpreter::ex_Video_compute(const int program_index)
{
    return DEFAULT; // TODO
}


double LogicInterpreter::ex_Video_clear(const int program_index)
{
    const auto& symbol_va_with_subscript_node = GetOrConvertPre80SymbolVariableArgumentsWithSubscriptNode(program_index);
    LogicVideo* const logic_video = GetFromSymbolOrEngineItem<LogicVideo*>(symbol_va_with_subscript_node.symbol_index, symbol_va_with_subscript_node.subscript_compilation);

    if( logic_video == nullptr )
        return 0;

    logic_video->Reset();

    return 1;
}


double LogicInterpreter::ex_Video_load(const int program_index)
{
    const auto& symbol_va_with_subscript_node = GetOrConvertPre80SymbolVariableArgumentsWithSubscriptNode(program_index);
    const std::string file_path = EvaluatePath(symbol_va_with_subscript_node.arguments[0]);
    LogicVideo* const logic_video = GetFromSymbolOrEngineItem<LogicVideo*>(symbol_va_with_subscript_node.symbol_index, symbol_va_with_subscript_node.subscript_compilation);

    if( logic_video == nullptr )
        return 0;

    try
    {
        logic_video->Load(file_path);
        return 1;
    }

    catch( const CSProException& exception )
    {
        IssueMessage(MessageType::Error, MGF::Video_load_error_48152,
                                         file_path.c_str(), exception.what());
        return 0;
    }
}


double LogicInterpreter::ex_Video_save(const int program_index)
{
    const auto& symbol_va_with_subscript_node = GetOrConvertPre80SymbolVariableArgumentsWithSubscriptNode(program_index);
    const std::string file_path = EvaluatePath(symbol_va_with_subscript_node.arguments[0]);
    LogicVideo* const logic_video = GetFromSymbolOrEngineItem<LogicVideo*>(symbol_va_with_subscript_node.symbol_index, symbol_va_with_subscript_node.subscript_compilation);

    if( logic_video == nullptr )
        return 0;

    try
    {
        logic_video->Save(file_path);
        return 1;
    }

    catch( const CSProException& exception )
    {
        IssueMessage(MessageType::Error, MGF::Video_save_error_48153,
                                         file_path.c_str(), exception.what());
        return 0;
    }
}
