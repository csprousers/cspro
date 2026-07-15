#include "stdafx.h"
#include "IncludesCC.h"
#include "EnginePreprocessor.h"


EnginePreprocessor::EnginePreprocessor(LogicCompiler& compiler, EngineData& engine_data)
    :   Logic::Preprocessor(compiler),
        m_compiler(compiler),
        m_engineData(engine_data)
{
}


const char* EnginePreprocessor::GetAppType()
{
    return ToString(m_compiler.GetEngineAppType());
}


Symbol* EnginePreprocessor::FindSymbol(const std::string_view symbol_name_sv, const bool search_only_base_symbols)
{
    for( Symbol* const symbol : m_engineData.symbol_table.FindSymbols(symbol_name_sv) )
    {
        if( !search_only_base_symbols ||
            SymbolCalculator::IsSymbolCreatedAutomatically(*symbol) )
        {
            return symbol;
        }
    }

    return nullptr;
}


template<>
std::optional<bool> EnginePreprocessor::ParseValue(const std::variant<double, SharableString>& value)
{
    if( std::holds_alternative<double>(value) )
    {
        return ( std::get<double>(value) == 1 ) ? std::make_optional(true) :
               ( std::get<double>(value) == 0 ) ? std::make_optional(false) :
                                                  std::nullopt;
    }

    else
    {
        return ( *std::get<SharableString>(value) == "true" )  ? std::make_optional(true) :
               ( *std::get<SharableString>(value) == "false" ) ? std::make_optional(false) :
                                                                 std::nullopt;
    }
}


void EnginePreprocessor::SetProperty(Symbol* const symbol, const std::string& attribute, const std::variant<double, SharableString>& value)
{
    auto issue_value_error = [&]()
    {
        std::string error_message = FormatText(
            "the value '%s' is invalid for attribute '%s'",
            std::holds_alternative<double>(value) ? DoubleToString(std::get<double>(value)).c_str() : std::get<SharableString>(value)->c_str(),
            attribute.c_str()
        );

        if( symbol != nullptr )
            error_message.append(FormatText(" for symbol type '%s'", ToString(symbol->GetType())));

        IssueError(MGF::preprocessor_function_error_69, error_message.c_str());
    };

    if( symbol != nullptr && symbol->IsA(SymbolType::Pre80Dictionary) && attribute == JK::readOptimization ) // ENGINECR_TODO implement for non-DICT
    {
        const std::optional<bool> use_read_optimization = ParseValue<bool>(value);

        if( !use_read_optimization.has_value() )
            issue_value_error();

        if( !*use_read_optimization )
            m_compiler.SetCaseAccessSetRequiresFullAccess_COMPILER_DLL_TODO(*symbol);

        return;
    }

    std::string error_message = FormatText("the attribute '%s' is invalid", attribute.c_str());

    if( symbol != nullptr )
        error_message.append(FormatText(" for symbol type '%s'", ToString(symbol->GetType())));

    IssueError(MGF::preprocessor_function_error_69, error_message.c_str());
}
