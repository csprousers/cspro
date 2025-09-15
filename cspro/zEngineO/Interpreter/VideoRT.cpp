#include "stdafx.h"
#include "IncludesRT.h"
#include "Video.h"


namespace VideoRT
{
    bool EnsureVideoExists(LogicInterpreter& interpreter, const LogicVideo& logic_video, const char* action_for_displayed_error_message);
    bool EnsureVideoExistsAndIsValid(LogicInterpreter& interpreter, const LogicVideo& logic_video, const char* action_for_displayed_error_message);
}


bool VideoRT::EnsureVideoExists(LogicInterpreter& interpreter, const LogicVideo& logic_video, const char* const action_for_displayed_error_message)
{
    if( !logic_video.HasContent() )
    {
        if( action_for_displayed_error_message != nullptr )
        {
            interpreter.IssueMessage(MessageType::Error, MGF::Video_no_video_for_action_48152,
                                                         logic_video.GetName().c_str(), action_for_displayed_error_message);
        }

        return false;
    }

    return true;
}


bool VideoRT::EnsureVideoExistsAndIsValid(LogicInterpreter& interpreter, const LogicVideo& logic_video, const char* const action_for_displayed_error_message)
{
    if( !EnsureVideoExists(interpreter, logic_video, action_for_displayed_error_message) )
    {
        return false;
    }

    else if( !logic_video.HasValidContent() )
    {
        if( action_for_displayed_error_message != nullptr )
        {
            interpreter.IssueMessage(MessageType::Error, MGF::Video_invalid_content_error_48153,
                                                         logic_video.GetName().c_str(), action_for_displayed_error_message);
        }

        return false;
    }

    return true;
}


double LogicInterpreter::ex_Video_compute(const int program_index)
{
    return DEFAULT; // TODO
}


double LogicInterpreter::ex_Video_clear(const int program_index)
{
    const auto& symbol_va_with_subscript_node = GetNode<Nodes::SymbolVariableArgumentsWithSubscript>(program_index);
    LogicVideo* const logic_video = GetFromSymbolOrEngineItem<LogicVideo*>(symbol_va_with_subscript_node.symbol_index, symbol_va_with_subscript_node.subscript_compilation);

    if( logic_video == nullptr )
        return 0;

    logic_video->Reset();

    return 1;
}


double LogicInterpreter::ex_Video_length(const int program_index)
{
    const auto& symbol_va_with_subscript_node = GetNode<Nodes::SymbolVariableArgumentsWithSubscript>(program_index);
    LogicVideo* const logic_video = GetFromSymbolOrEngineItem<LogicVideo*>(symbol_va_with_subscript_node.symbol_index, symbol_va_with_subscript_node.subscript_compilation);

    if( logic_video == nullptr )
        return DEFAULT;

    return logic_video->GetLength();
}


double LogicInterpreter::ex_Video_load(const int program_index)
{
    const auto& symbol_va_with_subscript_node = GetNode<Nodes::SymbolVariableArgumentsWithSubscript>(program_index);
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
        IssueMessage(MessageType::Error, MGF::Video_load_error_48154,
                                         file_path.c_str(), exception.what());
        return 0;
    }
}


double LogicInterpreter::ex_Video_save(const int program_index)
{
    const auto& symbol_va_with_subscript_node = GetNode<Nodes::SymbolVariableArgumentsWithSubscript>(program_index);
    const std::string file_path = EvaluatePath(symbol_va_with_subscript_node.arguments[0]);
    LogicVideo* const logic_video = GetFromSymbolOrEngineItem<LogicVideo*>(symbol_va_with_subscript_node.symbol_index, symbol_va_with_subscript_node.subscript_compilation);

    if( logic_video == nullptr || !VideoRT::EnsureVideoExists(*this, *logic_video, "save the video") )
        return DEFAULT;

    try
    {
        logic_video->Save(file_path);
        return 1;
    }

    catch( const CSProException& exception )
    {
        IssueMessage(MessageType::Error, MGF::Video_save_error_48155,
                                         file_path.c_str(), exception.what());
        return 0;
    }
}


double LogicInterpreter::ex_Video_width_height(const int program_index)
{
    const auto& symbol_va_with_subscript_node = GetNode<Nodes::SymbolVariableArgumentsWithSubscript>(program_index);
    LogicVideo* const logic_video = GetFromSymbolOrEngineItem<LogicVideo*>(symbol_va_with_subscript_node.symbol_index, symbol_va_with_subscript_node.subscript_compilation);

    if( logic_video == nullptr || !VideoRT::EnsureVideoExistsAndIsValid(*this, *logic_video, nullptr) )
        return DEFAULT;

    try
    {
        const std::tuple<long long, long long>& width_height = logic_video->GetWidthHeight();
        const bool requesting_width = ( symbol_va_with_subscript_node.function_code == FunctionCode::VIDEOFN_WIDTH_CODE );
        return static_cast<double>(requesting_width ? std::get<0>(width_height) :
                                                      std::get<1>(width_height));
    }

    catch(...)
    {
        return DEFAULT;
    }
}
