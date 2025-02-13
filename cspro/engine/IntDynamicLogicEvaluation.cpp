#include "StandardSystemIncludes.h"
#include "INTERPRE.H"
#include "DefaultParametersOnlyUserFunctionArgumentEvaluator.h"
#include "EngineExecutor.h"
#include <zEngineO/UserFunction.h>
#include <zAppO/Application.h>
#include <zLogicO/BaseCompiler.h>


// --------------------------------------------------------------------------
// declarations of:
// - DynamicLogicFunctionCompiler
// - DynamicLogicFunctionArgumentEvaluator
// --------------------------------------------------------------------------

class DynamicLogicFunctionCompiler : public Logic::BaseCompiler
{
public:
    DynamicLogicFunctionCompiler(CIntDriver* pIntDriver, const std::wstring& logic,
                                 UserFunction*& user_function, std::vector<std::variant<double, std::wstring>>& arguments);

    void CompileFunctionCall();

private:
    const LogicSettings& GetLogicSettings() const override;
    const std::wstring& GetCurrentProcName() const override;
    void FormatMessageAndProcessParserMessage(Logic::ParserMessage& parser_message, va_list parg) override;

    [[noreturn]] void ThrowCompilationError() const;

    void NextTokenAndCheck(TokenCode token_code);

private:
    CEngineDriver* m_pEngineDriver;
    CIntDriver* m_pIntDriver;

    const std::wstring& m_logic;
    UserFunction*& m_userFunction;
    std::vector<std::variant<double, std::wstring>>& m_arguments;
};


class DynamicLogicFunctionArgumentEvaluator : public DefaultParametersOnlyUserFunctionArgumentEvaluator
{
public:
    DynamicLogicFunctionArgumentEvaluator(CIntDriver* pIntDriver, const UserFunction& user_function,
                                          const std::vector<std::variant<double, std::wstring>>& arguments);

    double GetNumeric(int parameter_number) override;
    std::wstring GetString(int parameter_number) override;

private:
    const std::vector<std::variant<double, std::wstring>>& m_arguments;
};



// --------------------------------------------------------------------------
// DynamicLogicFunctionCompiler
// --------------------------------------------------------------------------

DynamicLogicFunctionCompiler::DynamicLogicFunctionCompiler(CIntDriver* pIntDriver, const std::wstring& logic,
                                                           UserFunction*& user_function, std::vector<std::variant<double, std::wstring>>& arguments)
    :   Logic::BaseCompiler(pIntDriver->GetSymbolTable()),
        m_pEngineDriver(pIntDriver->m_pEngineDriver),
        m_pIntDriver(pIntDriver),
        m_logic(logic),
        m_userFunction(user_function),
        m_arguments(arguments)
{
    ASSERT(m_userFunction == nullptr && m_arguments.empty());

    SetSourceBuffer(std::make_shared<Logic::SourceBuffer>(m_logic.c_str(), false));
}


const LogicSettings& DynamicLogicFunctionCompiler::GetLogicSettings() const
{
    return m_pEngineDriver->GetApplication()->GetLogicSettings();
}


const std::wstring& DynamicLogicFunctionCompiler::GetCurrentProcName() const
{
    ThrowCompilationError();
}


void DynamicLogicFunctionCompiler::FormatMessageAndProcessParserMessage(Logic::ParserMessage& /*parser_message*/, va_list /*parg*/)
{
    ThrowCompilationError();
}


void DynamicLogicFunctionCompiler::ThrowCompilationError() const
{
    std::wstring message = m_logic + _T("\n\nThere was an error compiling the ");

    if( m_userFunction != nullptr )
        SO::AppendFormat(message, _T("\"%s\" "), m_userFunction->GetName().c_str());

    message.append(_T("function call. The function call must use valid CSPro syntax and only ")
                    _T("numeric constant and string literal arguments are allowed."));


    throw CSProException(message);
}


void DynamicLogicFunctionCompiler::NextTokenAndCheck(TokenCode token_code)
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
// DynamicLogicFunctionArgumentEvaluator
// --------------------------------------------------------------------------

DynamicLogicFunctionArgumentEvaluator::DynamicLogicFunctionArgumentEvaluator(CIntDriver* pIntDriver, const UserFunction& user_function,
                                                                             const std::vector<std::variant<double, std::wstring>>& arguments)
    :   DefaultParametersOnlyUserFunctionArgumentEvaluator(pIntDriver, user_function),
        m_arguments(arguments)
{
}


double DynamicLogicFunctionArgumentEvaluator::GetNumeric(int parameter_number)
{
    if( static_cast<size_t>(parameter_number) < m_arguments.size() )
        return std::get<double>(m_arguments[parameter_number]);

    return DefaultParametersOnlyUserFunctionArgumentEvaluator::GetNumeric(parameter_number);
}


std::wstring DynamicLogicFunctionArgumentEvaluator::GetString(int parameter_number)
{
    if( static_cast<size_t>(parameter_number) < m_arguments.size() )
        return std::get<std::wstring>(m_arguments[parameter_number]);

    return DefaultParametersOnlyUserFunctionArgumentEvaluator::GetString(parameter_number);
}



// --------------------------------------------------------------------------
// CIntDriver::EvaluateLogic
// --------------------------------------------------------------------------

InterpreterExecuteResult CIntDriver::EvaluateLogic(const std::wstring& logic)
{
    // the only logic currently supported is the ability to call
    // user-defined functions with numeric constants and string literals
    UserFunction* user_function = nullptr;
    std::vector<std::variant<double, std::wstring>> arguments;
    DynamicLogicFunctionCompiler function_compiler(this, logic, user_function, arguments);

    function_compiler.CompileFunctionCall();

    // run the function
    return Execute(user_function->GetReturnDataType(),
        [&]()
        {
            DynamicLogicFunctionArgumentEvaluator argument_evaluator(this, *user_function, arguments);
            return CallUserFunction(*user_function, argument_evaluator);
        });
}
