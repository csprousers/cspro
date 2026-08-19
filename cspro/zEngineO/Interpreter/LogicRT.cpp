#include "StandardSystemIncludes.h"
#include "Interpreter.h"
#include "EngineExecutor.h"
#include <zEngineO/UserFunctionArgumentEvaluator.h>
#include <zAppO/Application.h>
#include <zLogicO/BaseCompiler.h>


// --------------------------------------------------------------------------
// DynamicLogicFunctionCompiler
// --------------------------------------------------------------------------

class DynamicLogicFunctionCompiler : public Logic::BaseCompiler
{
public:
    DynamicLogicFunctionCompiler(CIntDriver* interpreter, SharableString logic,
                                 UserFunction*& user_function, std::vector<std::variant<double, SharableString>>& arguments);

    void CompileFunctionCall();

private:
    const LogicSettings& GetLogicSettings() const override;
    std::string GetCurrentProcName() const override;
    void FormatMessageAndProcessParserMessage(Logic::ParserMessage& parser_message, va_list parg) override;

    [[noreturn]] void ThrowCompilationError() const;

    void NextTokenAndCheck(TokenCode token_code);

private:
    CEngineDriver* m_pEngineDriver;
    CIntDriver* m_interpreter;

    SharableString m_logic;
    UserFunction*& m_userFunction;
    std::vector<std::variant<double, SharableString>>& m_arguments;
};


DynamicLogicFunctionCompiler::DynamicLogicFunctionCompiler(CIntDriver* interpreter, SharableString logic,
                                                           UserFunction*& user_function, std::vector<std::variant<double, SharableString>>& arguments)
    :   Logic::BaseCompiler(interpreter->GetSymbolTable()),
        m_pEngineDriver(interpreter->m_pEngineDriver),
        m_interpreter(interpreter),
        m_logic(std::move(logic)),
        m_userFunction(user_function),
        m_arguments(arguments)
{
    ASSERT(m_userFunction == nullptr && m_arguments.empty());

    SetSourceBuffer(std::make_unique<Logic::SourceBuffer>(m_logic));
}


const LogicSettings& DynamicLogicFunctionCompiler::GetLogicSettings() const
{
    return m_pEngineDriver->GetApplication()->GetLogicSettings();
}


std::string DynamicLogicFunctionCompiler::GetCurrentProcName() const
{
    ThrowCompilationError();
}


void DynamicLogicFunctionCompiler::FormatMessageAndProcessParserMessage(Logic::ParserMessage& /*parser_message*/, va_list /*parg*/)
{
    ThrowCompilationError();
}


void DynamicLogicFunctionCompiler::ThrowCompilationError() const
{
    std::string message = *m_logic + "\n\nThere was an error compiling the ";

    if( m_userFunction != nullptr )
        message.append(FormatText("'%s' ", m_userFunction->GetName().c_str()));

    message.append("function call. The function call must use valid CSPro syntax and only "
                   "numeric constant and string literal arguments are allowed.");

    throw CSProException(message);
}


void DynamicLogicFunctionCompiler::NextTokenAndCheck(const TokenCode token_code)
{
    NextToken();

    if( Tkn != token_code )
        ThrowCompilationError();
}


void DynamicLogicFunctionCompiler::CompileFunctionCall()
{
    NextToken();

    if( Tkn != TOKUSERFUNCTION )
        ThrowCompilationError();

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
            ThrowCompilationError();
        }

        NextToken();

        if( Tkn == TOKRPAREN )
            break;

        if( Tkn != TOKCOMMA )
            ThrowCompilationError();

        NextToken();

        if( Tkn == TOKRPAREN )
            ThrowCompilationError();
    }

    NextTokenAndCheck(TOKSEMICOLON);
    NextTokenAndCheck(TOKEOP);

    // at this point, all arguments have been successfully processed, so see
    // if they match the (required) number and type of parameters the function expects
    if( m_arguments.size() < m_userFunction->GetNumberRequiredParameters() ||
        m_arguments.size() > m_userFunction->GetNumberParameters() )
    {
        ThrowCompilationError();
    }

    for( size_t i = 0; i < m_arguments.size(); ++i )
    {
        SymbolType symbol_type = std::holds_alternative<double>(m_arguments[i]) ? SymbolType::WorkVariable :
                                                                                  SymbolType::WorkString;

        if( !m_userFunction->GetParameterSymbol(i).IsA(symbol_type) )
            ThrowCompilationError();
    }
}



// --------------------------------------------------------------------------
// CIntDriver::EvaluateLogic
// --------------------------------------------------------------------------

InterpreterExecuteResult CIntDriver::EvaluateLogic(SharableString logic, CancelFlag& cancel_flag)
{
    // forward any cancelation requests to the interpreter's cancelation flag
    const CancelFlag::ListenerHolder cancel_flag_listener_holder = cancel_flag.AddListener([&]() { m_bStopProc = true; });

    // the only logic currently supported is the ability to call
    // user-defined functions with numeric constants and string literals
    UserFunction* user_function = nullptr;
    std::vector<std::variant<double, SharableString>> arguments;
    DynamicLogicFunctionCompiler function_compiler(this, std::move(logic), user_function, arguments);

    function_compiler.CompileFunctionCall();

    NumericStringValuesOnlyUserFunctionArgumentEvaluator<true> argument_evaluator(std::move(arguments));

    // execute the function
    return Execute([&]() { return CallUserFunction(*user_function, argument_evaluator); });
}
