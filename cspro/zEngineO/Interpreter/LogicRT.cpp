#include "stdafx.h"
#include "IncludesRT.h"
#include "UserFunction.h"
#include "UserFunctionArgumentEvaluator.h"
#include "Compiler/DynamicLogicCompiler.h"
#include <engine/InterpreterAccessor.h>


// --------------------------------------------------------------------------
// DynamicLogicFunctionCompiler
// --------------------------------------------------------------------------

class DynamicLogicFunctionCompiler : public DynamicLogicCompiler_COMPILER_DLL_TODO
{
public:
    DynamicLogicFunctionCompiler(LogicInterpreter& interpreter, SharableString logic,
                                 std::vector<std::variant<double, SharableString>>& arguments);

    UserFunction& CompileFunctionCall();

private:
    CSProException CreateCompilationException(const char* missing_method = nullptr) const override;

    void NextTokenAndCheck(TokenCode token_code);

private:
    LogicInterpreter& m_interpreter;
    SharableString m_logic;
    UserFunction* m_userFunction;
    std::vector<std::variant<double, SharableString>>& m_arguments;
};


DynamicLogicFunctionCompiler::DynamicLogicFunctionCompiler(LogicInterpreter& interpreter, SharableString logic,
                                                           std::vector<std::variant<double, SharableString>>& arguments)
    :   DynamicLogicCompiler_COMPILER_DLL_TODO(&interpreter.GetEngineData()),
        m_interpreter(interpreter),
        m_logic(std::move(logic)),
        m_userFunction(nullptr),
        m_arguments(arguments)
{
    ASSERT(m_arguments.empty());

    SetSourceBuffer(std::make_unique<Logic::SourceBuffer>(m_logic));
}


CSProException DynamicLogicFunctionCompiler::CreateCompilationException(const char* const missing_method/* = nullptr*/) const
{
    std::string message = *m_logic + "\n\nThere was an error compiling the ";

    if( m_userFunction != nullptr )
        message.append(FormatText("'%s' ", m_userFunction->GetName().c_str()));

    message.append("function call. The function call must use valid CSPro syntax and only "
                   "numeric constant and string literal arguments are allowed.");

    if( missing_method != nullptr )
        message.append("\n\nMissing method: ").append(missing_method);

    return CSProException(message);
}


void DynamicLogicFunctionCompiler::NextTokenAndCheck(const TokenCode token_code)
{
    NextToken();

    if( Tkn != token_code )
        throw CreateCompilationException();
}


UserFunction& DynamicLogicFunctionCompiler::CompileFunctionCall()
{
    NextToken();

    if( Tkn != TOKUSERFUNCTION )
        throw CreateCompilationException();

    m_userFunction = assert_cast<UserFunction*>(CurrentToken.symbol);

    NextTokenAndCheck(TOKLPAREN);

    // read any arguments...
    NextToken();

    while( Tkn != TOKRPAREN )
    {
        // ...numeric
        if( Tkn == TOKCTE )
        {
            m_arguments.emplace_back(Tokvalue);
        }

        // ...negative numeric
        else if( Tkn == TOKMINUS )
        {
            NextTokenAndCheck(TOKCTE);
            m_arguments.emplace_back(-1 * Tokvalue);
        }

        // ...string
        else if( Tkn == TOKSCTE )
        {
            m_arguments.emplace_back(Tokstr);
        }

        // ...anything else
        else
        {
            throw CreateCompilationException();
        }

        NextToken();

        if( Tkn == TOKRPAREN )
            break;

        if( Tkn != TOKCOMMA )
            throw CreateCompilationException();

        NextToken();

        if( Tkn == TOKRPAREN )
            throw CreateCompilationException();
    }

    NextTokenAndCheck(TOKSEMICOLON);
    NextTokenAndCheck(TOKEOP);

    // at this point, all arguments have been successfully processed, so see
    // if they match the (required) number and type of parameters the function expects
    if( m_arguments.size() < m_userFunction->GetNumberRequiredParameters() ||
        m_arguments.size() > m_userFunction->GetNumberParameters() )
    {
        throw CreateCompilationException();
    }

    for( size_t i = 0; i < m_arguments.size(); ++i )
    {
        const SymbolType symbol_type = std::holds_alternative<double>(m_arguments[i])
            ? SymbolType::WorkVariable
            : SymbolType::WorkString;

        if( !m_userFunction->GetParameterSymbol(i).IsA(symbol_type) )
            throw CreateCompilationException();
    }

    return *m_userFunction;
}



// --------------------------------------------------------------------------
// LogicInterpreter::EvaluateLogic
// --------------------------------------------------------------------------

InterpreterExecuteResult LogicInterpreter::EvaluateLogic(SharableString logic, CancelFlag& cancel_flag)
{
    // forward any cancelation requests to the interpreter's cancelation flag
    const CancelFlag::ListenerHolder cancel_flag_listener_holder = cancel_flag.AddListener([&]() { m_bStopProc = true; });

    // the only logic currently supported is the ability to call
    // user-defined functions with numeric constants and string literals
    std::vector<std::variant<double, SharableString>> arguments;
    DynamicLogicFunctionCompiler function_compiler(*this, std::move(logic), arguments);

    UserFunction& user_function = function_compiler.CompileFunctionCall();

    NumericStringValuesOnlyUserFunctionArgumentEvaluator<true> argument_evaluator(std::move(arguments));

    // execute the function
    return Execute([&]() { return CallUserFunction(user_function, argument_evaluator); });
}
