#include "StandardSystemIncludes.h"
#include "INTERPRE.H"
#include "Engine.h"
#include <zEngineO/Audio.h>
#include <zEngineO/Document.h>
#include <zEngineO/Versioning.h>
#include <zPlatformO/PlatformInterface.h>
#include <zAppO/Application.h>


double CIntDriver::exAudio_compute(int program_index)
{
    const auto& symbol_compute_with_subscript_node = GetOrConvertPre80SymbolComputeWithSubscriptNode(program_index);
    const SymbolReference<Symbol*> lhs_symbol_reference = EvaluateSymbolReference<Symbol*>(symbol_compute_with_subscript_node.lhs_symbol_index, symbol_compute_with_subscript_node.lhs_subscript_compilation);
    const Symbol* rhs_symbol = GetFromSymbolOrEngineItem<Symbol*>(symbol_compute_with_subscript_node.rhs_symbol_index, symbol_compute_with_subscript_node.rhs_subscript_compilation);

    if( rhs_symbol == nullptr )
        return 0;

    LogicAudio* lhs_logic_audio = GetFromSymbolOrEngineItem<LogicAudio*>(lhs_symbol_reference);

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
        issaerror(MessageType::Error, 100302, rhs_symbol->GetName().c_str(), lhs_logic_audio->GetName().c_str(), exception.GetErrorMessage().c_str());
    }

    return 0;
}


double CIntDriver::exAudio_clear(int program_index)
{
    const auto& symbol_va_with_subscript_node = GetOrConvertPre80SymbolVariableArgumentsWithSubscriptNode(program_index);
    LogicAudio* logic_audio = GetFromSymbolOrEngineItem<LogicAudio*>(symbol_va_with_subscript_node.symbol_index, symbol_va_with_subscript_node.subscript_compilation);

    if( logic_audio == nullptr )
        return 0;

    logic_audio->Reset();

    return 1;
}


double CIntDriver::exAudio_concat(int program_index)
{
    const auto& symbol_va_with_subscript_node = GetOrConvertPre80SymbolVariableArgumentsWithSubscriptNode(program_index);
    LogicAudio* rhs_logic_audio = nullptr;

    if( symbol_va_with_subscript_node.arguments[0] < 0 )
    {
        rhs_logic_audio = GetFromSymbolOrEngineItem<LogicAudio*>(-1 * symbol_va_with_subscript_node.arguments[0],
                                                                 Versioning::MeetsCompiledLogicVersion(Serializer::Iteration_8_0_000_1) ? symbol_va_with_subscript_node.arguments[1] : -1);

        if( rhs_logic_audio == nullptr )
            return 0;
    }

    LogicAudio* lhs_logic_audio = GetFromSymbolOrEngineItem<LogicAudio*>(symbol_va_with_subscript_node.symbol_index, symbol_va_with_subscript_node.subscript_compilation);

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
            std::wstring filename = EvalFullPathFileName(symbol_va_with_subscript_node.arguments[0]);
            lhs_logic_audio->Concat(std::move(filename));
        }

        return 1;
    }

    catch( const CSProException& exception )
    {
        issaerror(MessageType::Error, 100303, exception.GetErrorMessage().c_str());
        return 0;
    }
}


double CIntDriver::exAudio_length(int program_index)
{
    const auto& symbol_va_with_subscript_node = GetOrConvertPre80SymbolVariableArgumentsWithSubscriptNode(program_index);
    const LogicAudio* logic_audio = GetFromSymbolOrEngineItem<LogicAudio*>(symbol_va_with_subscript_node.symbol_index, symbol_va_with_subscript_node.subscript_compilation);

    if( logic_audio == nullptr )
        return DEFAULT;

    return logic_audio->GetLength();
}


double CIntDriver::exAudio_load(int program_index)
{
    const auto& symbol_va_with_subscript_node = GetOrConvertPre80SymbolVariableArgumentsWithSubscriptNode(program_index);
    const std::wstring filename = EvalFullPathFileName(symbol_va_with_subscript_node.arguments[0]);
    LogicAudio* logic_audio = GetFromSymbolOrEngineItem<LogicAudio*>(symbol_va_with_subscript_node.symbol_index, symbol_va_with_subscript_node.subscript_compilation);

    if( logic_audio == nullptr )
        return 0;

    try
    {
        logic_audio->Load(filename);
        return 1;
    }

    catch( const CSProException& exception )
    {
        issaerror(MessageType::Error, 100301, filename.c_str(), exception.GetErrorMessage().c_str());
        return 0;
    }
}


double CIntDriver::exAudio_play(int program_index)
{
    const auto& symbol_va_with_subscript_node = GetOrConvertPre80SymbolVariableArgumentsWithSubscriptNode(program_index);
    const std::wstring message = ValueOrDefault(EvaluateOptionalStringExpression(symbol_va_with_subscript_node.arguments[0]));
    LogicAudio* logic_audio = GetFromSymbolOrEngineItem<LogicAudio*>(symbol_va_with_subscript_node.symbol_index, symbol_va_with_subscript_node.subscript_compilation);

    if( logic_audio == nullptr )
        return 0;

    try
    {
        logic_audio->Play(message);
        return 1;
    }

    catch( const CSProException& exception )
    {
        issaerror(MessageType::Error, 100304, exception.GetErrorMessage().c_str());
        return 0;
    }
}


double CIntDriver::exAudio_save(int program_index)
{
    const auto& symbol_va_with_subscript_node = GetOrConvertPre80SymbolVariableArgumentsWithSubscriptNode(program_index);
    const std::wstring filename = EvalFullPathFileName(symbol_va_with_subscript_node.arguments[0]);
    LogicAudio* logic_audio = GetFromSymbolOrEngineItem<LogicAudio*>(symbol_va_with_subscript_node.symbol_index, symbol_va_with_subscript_node.subscript_compilation);

    if( logic_audio == nullptr )
        return 0;

    try
    {
        const CString& application_name = m_pEngineDriver->GetApplication()->GetLabel();

        logic_audio->Save(filename, CS2WS(application_name));

        return 1;
    }

    catch( const CSProException& exception )
    {
        issaerror(MessageType::Error, 100300, filename.c_str(), exception.GetErrorMessage().c_str());
        return 0;
    }
}


double CIntDriver::exAudio_stop(int program_index)
{
    const auto& symbol_va_with_subscript_node = GetOrConvertPre80SymbolVariableArgumentsWithSubscriptNode(program_index);
    LogicAudio* logic_audio = GetFromSymbolOrEngineItem<LogicAudio*>(symbol_va_with_subscript_node.symbol_index, symbol_va_with_subscript_node.subscript_compilation);

    if( logic_audio == nullptr )
        return 0;

    try
    {
        return logic_audio->Stop();
    }

    catch( const CSProException& exception )
    {
        issaerror(MessageType::Error, 100305, exception.GetErrorMessage().c_str());
        return 0;
    }
}


double CIntDriver::exAudio_record(int program_index)
{
    const auto& symbol_va_with_subscript_node = GetOrConvertPre80SymbolVariableArgumentsWithSubscriptNode(program_index);
    const std::optional<double> seconds = EvaluateOptionalNumericExpression(symbol_va_with_subscript_node.arguments[0]);
    LogicAudio* logic_audio = GetFromSymbolOrEngineItem<LogicAudio*>(symbol_va_with_subscript_node.symbol_index, symbol_va_with_subscript_node.subscript_compilation);

    if( logic_audio == nullptr )
        return 0;

    try
    {
        logic_audio->Record(seconds);
        return 1;
    }

    catch( const CSProException& exception )
    {
        issaerror(MessageType::Error, 100305, exception.GetErrorMessage().c_str());
        return 0;
    }
}


double CIntDriver::exAudio_recordInteractive(int program_index)
{
    const auto& symbol_va_with_subscript_node = GetOrConvertPre80SymbolVariableArgumentsWithSubscriptNode(program_index);
    const std::wstring message = ValueOrDefault(EvaluateOptionalStringExpression(symbol_va_with_subscript_node.arguments[0]));
    LogicAudio* logic_audio = GetFromSymbolOrEngineItem<LogicAudio*>(symbol_va_with_subscript_node.symbol_index, symbol_va_with_subscript_node.subscript_compilation);

    if( logic_audio == nullptr )
        return DEFAULT;

    try
    {
        return logic_audio->RecordInteractive(message);
    }

    catch( const CSProException& exception )
    {
        issaerror(MessageType::Error, 100305, exception.GetErrorMessage().c_str());
        return DEFAULT;
    }
}
