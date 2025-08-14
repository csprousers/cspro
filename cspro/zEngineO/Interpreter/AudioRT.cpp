#include "stdafx.h"
#include "IncludesRT.h"
#include "Audio.h"
#include "Document.h"


double LogicInterpreter::ex_Audio_compute(const int program_index)
{
    const auto& symbol_compute_with_subscript_node = GetOrConvertPre80SymbolComputeWithSubscriptNode(program_index);
    const SymbolReference<Symbol*> lhs_symbol_reference = EvaluateSymbolReference<Symbol*>(symbol_compute_with_subscript_node.lhs_symbol_index, symbol_compute_with_subscript_node.lhs_subscript_compilation);
    const Symbol* const rhs_symbol = GetFromSymbolOrEngineItem<Symbol*>(symbol_compute_with_subscript_node.rhs_symbol_index, symbol_compute_with_subscript_node.rhs_subscript_compilation);

    if( rhs_symbol == nullptr )
        return 0;

    LogicAudio* const lhs_logic_audio = GetFromSymbolOrEngineItem<LogicAudio*>(lhs_symbol_reference);

    if( lhs_logic_audio == nullptr )
        return 0;

    try
    {
        if( rhs_symbol->IsA(SymbolType::Audio) )
        {
            *lhs_logic_audio = assert_cast<const LogicAudio&>(*rhs_symbol);
        }

        else if( rhs_symbol->IsA(SymbolType::Document) )
        {
            *lhs_logic_audio = assert_cast<const LogicDocument&>(*rhs_symbol);
        }

        else
        {
            ASSERT(false);
        }
    }

    catch( const CSProException& exception )
    {
        IssueMessage(MessageType::Error, MGF::Audio_assignment_error_100302,
                                         rhs_symbol->GetName().c_str(), lhs_logic_audio->GetName().c_str(),
                                         exception.what());
    }

    return 0;
}


double LogicInterpreter::ex_Audio_clear(const int program_index)
{
    const auto& symbol_va_with_subscript_node = GetOrConvertPre80SymbolVariableArgumentsWithSubscriptNode(program_index);
    LogicAudio* const logic_audio = GetFromSymbolOrEngineItem<LogicAudio*>(symbol_va_with_subscript_node.symbol_index, symbol_va_with_subscript_node.subscript_compilation);

    if( logic_audio == nullptr )
        return 0;

    logic_audio->Reset();

    return 1;
}


double LogicInterpreter::ex_Audio_concat(const int program_index)
{
    const auto& symbol_va_with_subscript_node = GetOrConvertPre80SymbolVariableArgumentsWithSubscriptNode(program_index);
    LogicAudio* rhs_logic_audio = nullptr;

    if( symbol_va_with_subscript_node.arguments[0] < 0 )
    {
        rhs_logic_audio = GetFromSymbolOrEngineItem<LogicAudio*>(-1 * symbol_va_with_subscript_node.arguments[0],
                                                                 m_engineData->MeetsCompiledLogicVersion(Serializer::Iteration_8_0_000_1) ? symbol_va_with_subscript_node.arguments[1] : -1);

        if( rhs_logic_audio == nullptr )
            return 0;
    }

    LogicAudio* const lhs_logic_audio = GetFromSymbolOrEngineItem<LogicAudio*>(symbol_va_with_subscript_node.symbol_index, symbol_va_with_subscript_node.subscript_compilation);

    if( lhs_logic_audio == nullptr )
        return 0;

    try
    {
        if( rhs_logic_audio != nullptr )
        {
            lhs_logic_audio->Concat(*rhs_logic_audio);
        }

        else
        {
            std::string file_path = EvaluatePath(symbol_va_with_subscript_node.arguments[0]);
            lhs_logic_audio->Concat(std::move(file_path));
        }

        return 1;
    }

    catch( const CSProException& exception )
    {
        IssueMessage(MessageType::Error, MGF::Audio_concat_error_100303, exception.what());
        return 0;
    }
}


double LogicInterpreter::ex_Audio_length(const int program_index)
{
    const auto& symbol_va_with_subscript_node = GetOrConvertPre80SymbolVariableArgumentsWithSubscriptNode(program_index);
    const LogicAudio* const logic_audio = GetFromSymbolOrEngineItem<LogicAudio*>(symbol_va_with_subscript_node.symbol_index, symbol_va_with_subscript_node.subscript_compilation);

    if( logic_audio == nullptr )
        return DEFAULT;

    return logic_audio->GetLength();
}


double LogicInterpreter::ex_Audio_load(const int program_index)
{
    const auto& symbol_va_with_subscript_node = GetOrConvertPre80SymbolVariableArgumentsWithSubscriptNode(program_index);
    const std::string file_path = EvaluatePath(symbol_va_with_subscript_node.arguments[0]);
    LogicAudio* const logic_audio = GetFromSymbolOrEngineItem<LogicAudio*>(symbol_va_with_subscript_node.symbol_index, symbol_va_with_subscript_node.subscript_compilation);

    if( logic_audio == nullptr )
        return 0;

    try
    {
        logic_audio->Load(file_path);
        return 1;
    }

    catch( const CSProException& exception )
    {
        IssueMessage(MessageType::Error, MGF::Audio_load_error_100301,
                                         file_path.c_str(), exception.what());
        return 0;
    }
}


double LogicInterpreter::ex_Audio_play(const int program_index)
{
    const auto& symbol_va_with_subscript_node = GetOrConvertPre80SymbolVariableArgumentsWithSubscriptNode(program_index);
    const SharableString message = EvaluateNullableSharableString(symbol_va_with_subscript_node.arguments[0]);
    LogicAudio* const logic_audio = GetFromSymbolOrEngineItem<LogicAudio*>(symbol_va_with_subscript_node.symbol_index, symbol_va_with_subscript_node.subscript_compilation);

    if( logic_audio == nullptr )
        return 0;

    try
    {
        logic_audio->Play(message);
        return 1;
    }

    catch( const CSProException& exception )
    {
        IssueMessage(MessageType::Error, MGF::Audio_play_error_100304, exception.what());
        return 0;
    }
}


double LogicInterpreter::ex_Audio_save(const int program_index)
{
    const auto& symbol_va_with_subscript_node = GetOrConvertPre80SymbolVariableArgumentsWithSubscriptNode(program_index);
    const std::string file_path = EvaluatePath(symbol_va_with_subscript_node.arguments[0]);
    LogicAudio* const logic_audio = GetFromSymbolOrEngineItem<LogicAudio*>(symbol_va_with_subscript_node.symbol_index, symbol_va_with_subscript_node.subscript_compilation);

    if( logic_audio == nullptr )
        return 0;

    try
    {
        std::string application_name = ( m_engineData->application != nullptr ) ? m_engineData->application->GetLabel() :
                                                                                  "CSPro";
        logic_audio->Save(file_path, std::move(application_name));

        return 1;
    }

    catch( const CSProException& exception )
    {
        IssueMessage(MessageType::Error, MGF::Audio_save_error_100300,
                                         file_path.c_str(), exception.what());
        return 0;
    }
}


double LogicInterpreter::ex_Audio_stop(const int program_index)
{
    const auto& symbol_va_with_subscript_node = GetOrConvertPre80SymbolVariableArgumentsWithSubscriptNode(program_index);
    LogicAudio* const logic_audio = GetFromSymbolOrEngineItem<LogicAudio*>(symbol_va_with_subscript_node.symbol_index, symbol_va_with_subscript_node.subscript_compilation);

    if( logic_audio == nullptr )
        return 0;

    try
    {
        return logic_audio->Stop();
    }

    catch( const CSProException& exception )
    {
        IssueMessage(MessageType::Error, MGF::Audio_record_error_100305, exception.what());
        return 0;
    }
}


double LogicInterpreter::ex_Audio_record(const int program_index)
{
    const auto& symbol_va_with_subscript_node = GetOrConvertPre80SymbolVariableArgumentsWithSubscriptNode(program_index);
    const std::optional<double> seconds = EvaluateOptional(symbol_va_with_subscript_node.arguments[0]);
    LogicAudio* const logic_audio = GetFromSymbolOrEngineItem<LogicAudio*>(symbol_va_with_subscript_node.symbol_index, symbol_va_with_subscript_node.subscript_compilation);

    if( logic_audio == nullptr )
        return 0;

    try
    {
        logic_audio->Record(seconds);
        return 1;
    }

    catch( const CSProException& exception )
    {
        IssueMessage(MessageType::Error, MGF::Audio_record_error_100305, exception.what());
        return 0;
    }
}


double LogicInterpreter::ex_Audio_recordInteractive(const int program_index)
{
    const auto& symbol_va_with_subscript_node = GetOrConvertPre80SymbolVariableArgumentsWithSubscriptNode(program_index);
    const SharableString message = EvaluateNullableSharableString(symbol_va_with_subscript_node.arguments[0]);
    LogicAudio* const logic_audio = GetFromSymbolOrEngineItem<LogicAudio*>(symbol_va_with_subscript_node.symbol_index, symbol_va_with_subscript_node.subscript_compilation);

    if( logic_audio == nullptr )
        return DEFAULT;

    try
    {
        return logic_audio->RecordInteractive(message);
    }

    catch( const CSProException& exception )
    {
        IssueMessage(MessageType::Error, MGF::Audio_record_error_100305, exception.what());
        return DEFAULT;
    }
}
