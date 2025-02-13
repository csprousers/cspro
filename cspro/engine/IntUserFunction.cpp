#include "StandardSystemIncludes.h"
#include "INTERPRE.H"
#include "Ctab.h"
#include "EngineExecutor.h"
#include "ParadataDriver.h"
#include "ProgramControl.h"
#include <zEngineO/AllSymbols.h>
#include <zEngineO/UserFunctionArgumentChecker.h>
#include <zEngineO/Versioning.h>
#include <zEngineO/Nodes/UserFunction.h>
#include <zEngineF/TraceHandler.h>
#include <zJson/Json.h>
#include <zParadataO/Logger.h>


// --------------------------------------------------------------------------
// routines for running the actual user-defined functions
// --------------------------------------------------------------------------

namespace
{
#ifdef WIN_DESKTOP
    std::unique_ptr<LogicArray> CreateArrayFromCrosstab(CTAB* pCtab)
    {
        auto logic_array = std::make_unique<LogicArray>(pCtab->GetName());

        std::vector<size_t> dimensions;

        for( int i = 0; i < pCtab->GetNumDim(); ++i )
            dimensions.emplace_back(static_cast<size_t>(pCtab->GetTotDim(i)));

        logic_array->SetDimensions(std::move(dimensions));

        std::function<void(const std::vector<size_t>&)> crosstab_to_array_copier =
            [&](const std::vector<size_t>& indices)
            {
                double value = pCtab->m_pAcum.GetDoubleValue(
                    indices[0],
                    ( indices.size() > 1 ) ? indices[1] : 0,
                    ( indices.size() > 2 ) ? indices[2] : 0);

                logic_array->SetValue(indices, value);
            };

        logic_array->IterateCells(0, crosstab_to_array_copier);

        return logic_array;
    }

    void CopyArrayToCrosstab(CTAB* pCtab, const LogicArray& logic_array)
    {
        std::function<void(const std::vector<size_t>&)> array_to_crosstab_copier =
            [&](const std::vector<size_t>& indices)
            {
                double value = logic_array.GetValue<double>(indices);

                pCtab->m_pAcum.PutDoubleValue(value,
                    indices[0],
                    ( indices.size() > 1 ) ? indices[1] : 0,
                    ( indices.size() > 2 ) ? indices[2] : 0);
            };

        logic_array.IterateCells(0, array_to_crosstab_copier);
    }
#endif
}


double CIntDriver::CallUserFunction(UserFunction& user_function, UserFunctionArgumentEvaluator& user_function_argument_evaluator)
{
    // reset the return value
    user_function.Reset();

    auto get_function_return_value = [&]() -> double
    {
        if( user_function.GetReturnType() == SymbolType::WorkVariable )
        {
            return user_function.GetReturnValue<double>();
        }

        else
        {
            return AssignAlphaValue(user_function.GetReturnValue<std::wstring>());
        }
    };

    // only execute functions that have a body
    if( user_function.GetProgramIndex() < 0 )
        return get_function_return_value();

    // get the set of local symbols that will be used for this call
    UserFunctionLocalSymbolsManager local_symbols_manager = user_function.GetLocalSymbolsManager();


    // evaluate each of the arguments
    std::unique_ptr<std::vector<std::tuple<CTAB*, std::shared_ptr<LogicArray>>>> crosstabs_converted_to_arrays;

    for( size_t i = 0; i < user_function.GetNumberParameters(); ++i )
    {
        const int parameter_symbol_index = user_function.GetParameterSymbolIndex(i);        
        Symbol& parameter_symbol = local_symbols_manager.GetSymbol(parameter_symbol_index);

        // numeric
        if( parameter_symbol.IsA(SymbolType::WorkVariable) )
        {
            WorkVariable& work_variable = assert_cast<WorkVariable&>(parameter_symbol);
            work_variable.SetValue(user_function_argument_evaluator.GetNumeric(i));
        }


        // string/alpha
        else if( parameter_symbol.IsA(SymbolType::WorkString) )
        {
            WorkString& work_string = assert_cast<WorkString&>(parameter_symbol);
            work_string.SetString(user_function_argument_evaluator.GetString(i));
        }


        // string/alpha
        else if( parameter_symbol.IsA(SymbolType::Variable) )
        {
            ASSERT(Versioning::PredatesCompiledLogicVersion(Serializer::Iteration_7_6_000_1));

            VART* pVarT = assert_cast<VART*>(&parameter_symbol);
            std::wstring parameter_value = user_function_argument_evaluator.GetString(i);

            // variable length strings
            if( pVarT->GetLogicStringPtr() != nullptr )
            {
                *(pVarT->GetLogicStringPtr()) = WS2CS(parameter_value);
            }

            // fixed length string
            else
            {
                VARX* pVarX = pVarT->GetVarX();
                TCHAR* buffer = (TCHAR*)svaraddr(pVarX);
                SO::MakeExactLength(parameter_value, pVarT->GetLength());
                _tmemcpy(buffer, parameter_value.data(), pVarT->GetLength());
            }
        }


        // other symbols (not constructed in place using the parameter symbol)
        else if( !user_function_argument_evaluator.ConstructSymbolInPlace(i, parameter_symbol) )
        {
            std::shared_ptr<Symbol> argument_symbol;

            try
            {
                argument_symbol = user_function_argument_evaluator.GetSymbol(i);
            }

            catch( const UserFunctionArgumentEvaluator::InvalidSubscript& )
            {
                return AssignInvalidValue(user_function.GetReturnDataType());
            }

            const bool no_argument_provided_to_optional_parameter = ( argument_symbol == nullptr );

            // dictionary (case) objects have special processing
            if( parameter_symbol.IsA(SymbolType::Dictionary) && assert_cast<const EngineDictionary&>(parameter_symbol).IsCaseObject() )
            {
                EngineCase& engine_case = assert_cast<EngineDictionary&>(parameter_symbol).GetEngineCase();

                if( no_argument_provided_to_optional_parameter )
                {
                    engine_case.CreateNewCase();
                }

                else
                {
                    engine_case.ShareCase(assert_cast<EngineDictionary&>(*argument_symbol).GetEngineCase());
                }
            }


            // all other objects are passed by reference
            else
            {
                // if the argument symbol is not set, then this is an optional argument
                // so the function's (local) symbol should be used (after being reset)
                if( no_argument_provided_to_optional_parameter )
                {
                    ResetSymbol(parameter_symbol);
                }

                // otherwise use the symbol substitutor
                else
                {
#ifdef WIN_DESKTOP
                    // crosstabs can be passed as arguments to arrays
                    if( argument_symbol->IsA(SymbolType::Crosstab) )
                    {
                        CTAB* pCtab = assert_cast<CTAB*>(argument_symbol.get());
                        std::shared_ptr<LogicArray> crosstab_converted_to_array = CreateArrayFromCrosstab(pCtab);
                        argument_symbol = crosstab_converted_to_array;

                        if( crosstabs_converted_to_arrays == nullptr )
                            crosstabs_converted_to_arrays = std::make_unique<std::vector<std::tuple<CTAB*, std::shared_ptr<LogicArray>>>>();

                        crosstabs_converted_to_arrays->emplace_back(pCtab, std::move(crosstab_converted_to_array));
                    }
#endif
                    local_symbols_manager.MarkForSymbolSubstitution(parameter_symbol, std::move(argument_symbol));
                }
            }
        }


        m_iStopExec = FALSE;
    }

    // once all arguments have been evaluated, move the local symbols into the symbol table
    local_symbols_manager.RunSymbolSubstitution();


    // execute the function
    bool bRequestIssued = false;

    if( m_traceHandler != nullptr )
    {
        m_traceHandler->Output(FormatTextCS2WS(_T("Entering function %s..."), user_function.GetName().c_str()),
                               TraceHandler::OutputType::SystemText);
    }

    try
    {
        bRequestIssued = ExecuteProgramStatements(user_function.GetProgramIndex()); // TODO what if RequestIssued is true???
    }

    // an exit statement terminates the function
    catch( const ExitProgramControlException& ) { }


    if( m_traceHandler != nullptr )
    {
        m_traceHandler->Output(FormatTextCS2WS(_T("Exiting function %s..."), user_function.GetName().c_str()),
                               TraceHandler::OutputType::SystemText);
    }


#ifdef WIN_DESKTOP
    // convert back any arrays -> crosstabs
    if( crosstabs_converted_to_arrays != nullptr )
    {
        for( const auto& [pCtab, crosstab_converted_to_array] : *crosstabs_converted_to_arrays )
            CopyArrayToCrosstab(pCtab, *crosstab_converted_to_array);
    }
#endif

    m_iStopExec = FALSE;

    return get_function_return_value();
}



// --------------------------------------------------------------------------
// routines for running the user-defined functions called in logic
// --------------------------------------------------------------------------

namespace
{
    class LogicUserFunctionArgumentEvaluator : public UserFunctionArgumentEvaluator
    {
    public:
        LogicUserFunctionArgumentEvaluator(CIntDriver& interpreter, const Nodes::UserFunction& user_function_node)
            :   m_interpreter(interpreter),
                m_argumentExpressions(user_function_node.argument_expressions),
                m_pre80SupportMultiplier(Versioning::MeetsCompiledLogicVersion(Serializer::Iteration_8_0_000_1) ? 2 : 1)
        {
            static_assert(Serializer::GetEarliestSupportedVersion() < Serializer::Iteration_8_0_000_1, "remove m_pre80SupportMultiplier and replace with 2");
        }

        double GetNumeric(int parameter_number) override
        {
            ASSERT(Versioning::PredatesCompiledLogicVersion(Serializer::Iteration_8_0_000_1) || m_argumentExpressions[2 * parameter_number + 1] == -1);

            return m_interpreter.evalexpr(m_argumentExpressions[m_pre80SupportMultiplier * parameter_number]);
        }

        std::wstring GetString(int parameter_number) override
        {
            ASSERT(Versioning::PredatesCompiledLogicVersion(Serializer::Iteration_8_0_000_1) || m_argumentExpressions[2 * parameter_number + 1] == -1);

            return m_interpreter.EvalAlphaExpr(m_argumentExpressions[m_pre80SupportMultiplier * parameter_number]);
        }

        std::shared_ptr<Symbol> GetSymbol(int parameter_number) override
        {
            const int& symbol_index = m_argumentExpressions[m_pre80SupportMultiplier * parameter_number];

            if( symbol_index == -1 )
                return nullptr;

            const int subscript_compilation = ( m_pre80SupportMultiplier == 2 ) ? m_argumentExpressions[m_pre80SupportMultiplier * parameter_number + 1] : -1;

            std::shared_ptr<Symbol> symbol = m_interpreter.GetFromSymbolOrEngineItem<std::shared_ptr<Symbol>>(symbol_index, subscript_compilation);                                                                                                              

            if( symbol == nullptr )
                throw UserFunctionArgumentEvaluator::InvalidSubscript();

            return symbol;
        }

    private:
        CIntDriver& m_interpreter;
        const int* m_argumentExpressions;
        const size_t m_pre80SupportMultiplier;
    };
}


double CIntDriver::exuserfunctioncall(int program_index)
{
    const auto& user_function_node = GetNode<Nodes::UserFunction>(program_index);
    UserFunction& user_function = GetSymbolUserFunction(user_function_node.user_function_symbol_index);

    // execute the user-defined function
    LogicUserFunctionArgumentEvaluator argument_evaluator(*this, user_function_node);
    double return_value = CallUserFunction(user_function, argument_evaluator);

    // if any arguments were passed by reference, assign the values to the destination variables
    const auto& reference_destinations_list_node = GetListNode(user_function_node.reference_destinations_list);

    for( int i = 0; i < reference_destinations_list_node.number_elements; i += 2 )
    {
        const Symbol& parameter_symbol = NPT_Ref(reference_destinations_list_node.elements[i]);
        const auto& symbol_value_node = GetNode<Nodes::SymbolValue>(reference_destinations_list_node.elements[i + 1]);

        if( parameter_symbol.IsA(SymbolType::WorkVariable) )
        {
            const WorkVariable& work_variable = assert_cast<const WorkVariable&>(parameter_symbol);
            AssignValueToSymbol(symbol_value_node, work_variable.GetValue());
        }

        else if( parameter_symbol.IsA(SymbolType::WorkString) )
        {
            const WorkString& work_string = assert_cast<const WorkString&>(parameter_symbol);
            AssignValueToSymbol(symbol_value_node, work_string.GetString());
        }

        else if( parameter_symbol.IsA(SymbolType::Variable) )
        {
            ASSERT(Versioning::PredatesCompiledLogicVersion(Serializer::Iteration_7_6_000_1));

            const VART* pVarT = assert_cast<const VART*>(&parameter_symbol);
            CString value;

            if( pVarT->GetLogicStringPtr() != nullptr )
            {
                value = *(pVarT->GetLogicStringPtr());
            }

            else
            {
                VARX* pVarX = pVarT->GetVarX();
                _tmemcpy(value.GetBufferSetLength(pVarT->GetLength()), (LPCTSTR)svaraddr(pVarX), pVarT->GetLength());
                value.ReleaseBuffer();
            }

            AssignValueToSymbol(symbol_value_node, CS2WS(value));
        }

        else
        {
            ASSERT(false);
        }
    }

    return return_value;
}



// --------------------------------------------------------------------------
// routines for running user-defined functions as callbacks
// --------------------------------------------------------------------------

namespace
{
    // callback arguments are evaluated immediately and then stored for later use
    class LogicCallbackUserFunctionArgumentEvaluator : public UserFunctionArgumentEvaluator
    {
    public:
        LogicCallbackUserFunctionArgumentEvaluator(CIntDriver& interpreter, const Nodes::UserFunction& user_function_node, FunctionCode function_code)
            :   m_interpreter(interpreter),
                m_userFunctionNode(user_function_node),
                m_functionCode(function_code)
        {
            const size_t m_pre80SupportMultiplier = Versioning::MeetsCompiledLogicVersion(Serializer::Iteration_8_0_000_1) ? 2 : 1;
            static_assert(Serializer::GetEarliestSupportedVersion() < Serializer::Iteration_8_0_000_1, "remove m_pre80SupportMultiplier and replace with 2");

            // evaluate the numeric and string arguments
            const UserFunction& user_function = GetSymbolUserFunction(user_function_node.user_function_symbol_index);

            for( size_t i = 0; i < user_function.GetNumberParameters(); ++i )
            {
                const Symbol& parameter_symbol = user_function.GetParameterSymbol(i);

                // numeric
                if( parameter_symbol.IsA(SymbolType::WorkVariable) )
                {
                    m_numericValues.try_emplace(i, m_interpreter.evalexpr(user_function_node.argument_expressions[m_pre80SupportMultiplier * i]));
                }

                // string/alpha
                else if( parameter_symbol.IsA(SymbolType::WorkString) )
                {
                    m_stringValues.try_emplace(i, m_interpreter.EvalAlphaExpr(user_function_node.argument_expressions[m_pre80SupportMultiplier * i]));
                }

                else if( parameter_symbol.IsA(SymbolType::Variable) )
                {
                    ASSERT(Versioning::PredatesCompiledLogicVersion(Serializer::Iteration_7_6_000_1));
                    m_stringValues.try_emplace(i, m_interpreter.EvalAlphaExpr(user_function_node.argument_expressions[m_pre80SupportMultiplier * i]));
                }

                // symbols
                else
                {
                    const int& symbol_index = user_function_node.argument_expressions[m_pre80SupportMultiplier * i];

                    if( symbol_index != -1 )
                    {
                        const int subscript_compilation = ( m_pre80SupportMultiplier == 2 ) ? user_function_node.argument_expressions[m_pre80SupportMultiplier * i + 1] : -1;
                        m_evaluatedSymbolReferences.try_emplace(i, m_interpreter.EvaluateSymbolReference<std::shared_ptr<Symbol>>(symbol_index, subscript_compilation));
                    }
                }
            }
        }

        double GetNumeric(int parameter_number) override
        {
            return m_numericValues[parameter_number];
        }

        std::wstring GetString(int parameter_number) override
        {
            return m_stringValues[parameter_number];
        }

        std::shared_ptr<Symbol> GetSymbol(int parameter_number) override
        {
            const auto& lookup = m_evaluatedSymbolReferences.find(parameter_number);

            if( lookup == m_evaluatedSymbolReferences.cend() )
                return nullptr;

            std::shared_ptr<Symbol> symbol = m_interpreter.GetFromSymbolOrEngineItem<std::shared_ptr<Symbol>>(lookup->second);

            if( symbol == nullptr )
                throw UserFunctionArgumentEvaluator::InvalidSubscript();

            return symbol;
        }

        const Nodes::UserFunction& GetUserFunctionNode() const { return m_userFunctionNode; }

        FunctionCode GetFunctionCode() const { return m_functionCode; }

    private:
        const Logic::SymbolTable& GetSymbolTable() const { return m_interpreter.GetSymbolTable(); }

    private:
        CIntDriver& m_interpreter;
        const Nodes::UserFunction& m_userFunctionNode;
        const FunctionCode m_functionCode;
        std::map<int, double> m_numericValues;
        std::map<int, std::wstring> m_stringValues;
        std::map<int, SymbolReference<std::shared_ptr<Symbol>>> m_evaluatedSymbolReferences;
    };
}


std::unique_ptr<UserFunctionArgumentEvaluator> CIntDriver::EvaluateArgumentsForCallbackUserFunction(int program_index, FunctionCode function_code)
{
    const auto& user_function_node = GetNode<Nodes::UserFunction>(program_index);
    return std::make_unique<LogicCallbackUserFunctionArgumentEvaluator>(*this, user_function_node, function_code);
}


void CIntDriver::ExecuteCallbackUserFunction(int field_symbol_index, UserFunctionArgumentEvaluator& user_function_argument_evaluator)
{
    // these statements clear any preexisting stuff that might have been going on
    m_iSkipStmt = FALSE;
    m_iStopExec = m_bStopProc;
    SetRequestIssued(false);

    m_FieldSymbol = field_symbol_index;
    m_iExSymbol = field_symbol_index;
    m_iExLevel = SymbolCalculator::GetLevelNumber_base1(NPT_Ref(field_symbol_index));

    LogicCallbackUserFunctionArgumentEvaluator* argument_evaluator = assert_cast<LogicCallbackUserFunctionArgumentEvaluator*>(&user_function_argument_evaluator);

    const auto& user_function_node = argument_evaluator->GetUserFunctionNode();
    UserFunction& user_function = GetSymbolUserFunction(user_function_node.user_function_symbol_index);

    if( Paradata::Logger::IsOpen() )
    {
        Paradata::OperatorSelectionEvent::Source source =
            ( argument_evaluator->GetFunctionCode() == FNUSERBAR_CODE ) ?
            Paradata::OperatorSelectionEvent::Source::Userbar : Paradata::OperatorSelectionEvent::Source::MapShow;

        auto operator_selection_event = std::make_unique<Paradata::OperatorSelectionEvent>(source);
        operator_selection_event->SetPostSelectionValues(std::nullopt, user_function.GetName(), false);
        m_pParadataDriver->RegisterAndLogEvent(std::move(operator_selection_event));
    }

    CallUserFunction(user_function, *argument_evaluator);

    m_FieldSymbol = 0;

    if( m_bStopProc )
        WindowsDesktopMessage::Post(WM_IMSA_USERBAR_UPDATE, 0, -1); // stop the program
}



// --------------------------------------------------------------------------
// routines for the invoke function
// --------------------------------------------------------------------------

namespace
{
    class InvokeArgumentsProvidedUsingJsonArgumentEvaluator : public UserFunctionArgumentEvaluator
    {
    public:
        InvokeArgumentsProvidedUsingJsonArgumentEvaluator(CIntDriver& interpreter, const UserFunction& user_function,
                                                          const JsonNode<wchar_t>& json_arguments)
            :   m_interpreter(interpreter),
                m_userFunction(user_function),
                m_jsonArguments(json_arguments),
                m_parameterSymbol(nullptr)
        {
        }

        const Symbol* GetLastEvaluatedParameterSymbol() const { return m_parameterSymbol; }

        double GetNumeric(int parameter_number) override
        {
            if( UseDefaultValue(parameter_number) )
                return m_interpreter.evalexpr(m_userFunction.GetParameterDefaultValue(parameter_number));

            const auto& parameter_node = m_jsonArguments.Get(m_parameterSymbol->GetName());

            return parameter_node.IsBoolean() ? ( parameter_node.Get<bool>() ? 1 : 0 ) :
                                                parameter_node.Get<double>();
        }

        std::wstring GetString(int parameter_number) override
        {
            if( UseDefaultValue(parameter_number) )
                return m_interpreter.EvalAlphaExpr(m_userFunction.GetParameterDefaultValue(parameter_number));

            return m_jsonArguments.Get<std::wstring>(m_parameterSymbol->GetName());
        }

        bool ConstructSymbolInPlace(int parameter_number, Symbol& parameter_symbol) override
        {
            if( UseDefaultValue(parameter_number) )
                return false;

            try
            {
                parameter_symbol.UpdateValueFromJson(m_jsonArguments[m_parameterSymbol->GetName()]);
                return true;
            }

            catch( const Symbol::NoUpdateValueFromJsonRoutine& )
            {
                throw CSProException(_T("Arguments of type '%s' are currently not supported when supplied via JSON"),
                                     ToString(m_parameterSymbol->GetType()));
            }

            catch( const CSProException& exception )
            {
                throw CSProException(_T("Error processing argument '%s': %s"),
                                     m_parameterSymbol->GetName().c_str(),
                                     exception.GetErrorMessage().c_str());
            }
        }

        std::shared_ptr<Symbol> GetSymbol(int parameter_number) override
        {
            if( UseDefaultValue(parameter_number) )
                return nullptr;

            return ReturnProgrammingError(nullptr);
        }

    private:
        // sets the parameter symbol and return whether or not the default value should be used
        bool UseDefaultValue(int parameter_number)
        {
            m_parameterSymbol = &m_userFunction.GetParameterSymbol(parameter_number);

            // if an argument is provided, use it
            if( m_jsonArguments.Contains(m_parameterSymbol->GetName()) )
            {
                return false;
            }

            // if a default value exists, use it
            else if( static_cast<size_t>(parameter_number) >= m_userFunction.GetNumberRequiredParameters() )
            {
                return true;
            }

            // otherwise an error
            else
            {
                throw CSProException(_T("No argument for '%s' provided"), m_parameterSymbol->GetName().c_str());
            }
        }

    private:
        CIntDriver& m_interpreter;
        const UserFunction& m_userFunction;
        const JsonNode<wchar_t>& m_jsonArguments;
        const Symbol* m_parameterSymbol;
    };


    class InvokeArgumentsProvidedDirectlyArgumentEvaluator : public UserFunctionArgumentEvaluator
    {
    public:
        InvokeArgumentsProvidedDirectlyArgumentEvaluator(CIntDriver& interpreter, const std::vector<std::tuple<int, int>>& arguments)
            :   m_interpreter(interpreter),
                m_arguments(arguments)
        {
        }

        double GetNumeric(int parameter_number) override
        {
            ASSERT(( std::get<0>(m_arguments[parameter_number]) == ( -1 * static_cast<int>(SymbolType::WorkVariable)) ) ||
                   ( std::get<0>(m_arguments[parameter_number]) == -1 ));
                
            return m_interpreter.evalexpr(std::get<1>(m_arguments[parameter_number]));
        }

        std::wstring GetString(int parameter_number) override
        {
            ASSERT(( std::get<0>(m_arguments[parameter_number]) == ( -1 * static_cast<int>(SymbolType::WorkString)) ) ||
                   ( std::get<0>(m_arguments[parameter_number]) == -1 ));

            return m_interpreter.EvalAlphaExpr(std::get<1>(m_arguments[parameter_number]));
        }

        std::shared_ptr<Symbol> GetSymbol(int parameter_number) override
        {
            const int& symbol_index = std::get<0>(m_arguments[parameter_number]);

            if( symbol_index == -1 )
                return nullptr;

            std::shared_ptr<Symbol> symbol = m_interpreter.GetFromSymbolOrEngineItem<std::shared_ptr<Symbol>>(symbol_index, std::get<1>(m_arguments[parameter_number]));

            if( symbol == nullptr )
                throw UserFunctionArgumentEvaluator::InvalidSubscript();

            return symbol;
        }

    private:
        CIntDriver& m_interpreter;
        const std::vector<std::tuple<int, int>>& m_arguments;
    };
}


template<typename T>
InterpreterExecuteResult CIntDriver::RunInvoke(const StringNoCase& function_name, const T& variable_arguments)
{
    UserFunction* user_function = nullptr;

    try
    {
        Symbol& symbol = GetSymbolFromSymbolName(function_name, SymbolType::UserFunction);

        if( symbol.IsA(SymbolType::UserFunction) )
            user_function = assert_cast<UserFunction*>(&symbol);
    }
    catch(...) { }

    if( user_function == nullptr )
        throw CSProException("no user-defined function with the name exists.");


    // arguments provided as JSON text
    if constexpr(std::is_same_v<T, std::wstring> || std::is_same_v<T, JsonNode<wchar_t>>)
    {
        std::unique_ptr<InvokeArgumentsProvidedUsingJsonArgumentEvaluator> argument_evaluator;

        try
        {
            auto run_execute = [&](const JsonNode<wchar_t>& json_arguments)
            {
                argument_evaluator = std::make_unique<InvokeArgumentsProvidedUsingJsonArgumentEvaluator>(*this, *user_function, json_arguments);
                return Execute(user_function->GetReturnDataType(), [&]() { return CallUserFunction(*user_function, *argument_evaluator); });
            };

            if constexpr(std::is_same_v<T, std::wstring>)
            {
                const std::wstring& json_arguments_text = variable_arguments;
                const JsonNode<wchar_t> json_arguments = Json::Parse(json_arguments_text, GetEngineJsonReaderInterface());
                return run_execute(json_arguments);
            }

            else
            {
                const JsonNode<wchar_t>& json_arguments = variable_arguments;
                return run_execute(json_arguments);
            }
        }

        catch( const JsonParseException& exception )
        {
            std::wstring message = exception.GetErrorMessage();

            if( argument_evaluator != nullptr && argument_evaluator->GetLastEvaluatedParameterSymbol() != nullptr )
                SO::AppendFormat(message, _T(" (for argument '%s')"), argument_evaluator->GetLastEvaluatedParameterSymbol()->GetName().c_str());

            throw CSProException(message);
        }
    }


    // arguments provided directly in positional order
    else
    {
        const Nodes::List& arguments_list = variable_arguments;

        try
        {
            UserFunctionArgumentChecker argument_checker(*user_function);

            const size_t number_arguments = arguments_list.number_elements / 2;

            argument_checker.CheckNumberArguments(number_arguments);

            // evaluate each of the arguments and determine if they are valid
            std::vector<std::tuple<int, int>> arguments;
            size_t argument_index = 0;

            for( ; argument_index < number_arguments; ++argument_index )
            {
                auto& [symbol_index, subscript_compilation] = arguments.emplace_back(arguments_list.elements[2 * argument_index],
                                                                                     arguments_list.elements[2 * argument_index + 1]);
                
                if( Versioning::PredatesCompiledLogicVersion(Serializer::Iteration_8_0_000_1) )
                {
                    if( symbol_index == static_cast<int>(SymbolType::WorkString) ||
                        symbol_index == static_cast<int>(SymbolType::WorkVariable) )
                    {
                        symbol_index = -1 * symbol_index;
                    }

                    else
                    {
                        ASSERT(symbol_index == static_cast<int>(NPT_Ref(subscript_compilation).GetType()));
                        symbol_index = subscript_compilation;
                        subscript_compilation = -1;
                    }
                }

                Symbol* symbol;
                SymbolType argument_symbol_type;

                if( symbol_index < 0 )
                {
                    symbol = nullptr;
                    argument_symbol_type = static_cast<SymbolType>(-1 * symbol_index);
                }

                else
                {
                    symbol = &NPT_Ref(symbol_index);
                    argument_symbol_type = symbol->GetType();
                }

                // expressions
                if( argument_checker.ArgumentShouldBeExpression(argument_index) )
                {
                    argument_checker.CheckExpressionArgument(argument_index, argument_symbol_type);
                }

                // symbols
                else
                {
                    argument_checker.CheckSymbolArgument(argument_index, symbol);
                }
            }

            // add the default values for any optional parameters that did not have corresponding arguments
            for( ; argument_index < user_function->GetNumberParameters(); ++argument_index )
                arguments.emplace_back(-1, user_function->GetParameterDefaultValue(argument_index));

            // run the function
            InvokeArgumentsProvidedDirectlyArgumentEvaluator argument_evaluator(*this, arguments);

            return Execute(user_function->GetReturnDataType(), [&]() { return CallUserFunction(*user_function, argument_evaluator); });
        }

        catch( const UserFunctionArgumentChecker::CheckError& error )
        {
            // rethrow the error in the message style that would be issues during compile-time
            throw CSProException(_T("the user-defined function expects %s"), error.GetErrorMessage().c_str());
        }
    }
}

template InterpreterExecuteResult CIntDriver::RunInvoke(const StringNoCase& function_name, const JsonNode<wchar_t>& variable_arguments);


double CIntDriver::exinvoke(int program_index)
{
    const auto& invoke_node = GetNode<Nodes::Invoke>(program_index);
    StringNoCase function_name = EvalAlphaExpr<StringNoCase>(invoke_node.function_name_expression);

    try
    {
        auto process_result = [&](InterpreterExecuteResult execute_result)
        {
            if( execute_result.program_control_executed )
                RethrowProgramControlExceptions();

            return AssignAlphaValue(std::holds_alternative<std::wstring>(execute_result.result) ? std::move(std::get<std::wstring>(execute_result.result)) :
                                                                                                  DoubleToString(std::get<double>(execute_result.result)));
        };

        if( invoke_node.arguments_expression != -1 )
        {
            std::wstring json_arguments_text = EvalAlphaExpr(invoke_node.arguments_expression);
            return process_result(RunInvoke(function_name, json_arguments_text));
        }

        else
        {
            const auto& arguments_list = GetListNode(invoke_node.arguments_list);
            return process_result(RunInvoke(function_name, arguments_list));
        }
    }

    catch( const CSProException& exception )
    {
        issaerror(MessageType::Error, 50051, Logic::FunctionTable::GetFunctionName(invoke_node.function_code),
                                             function_name.c_str(), exception.GetErrorMessage().c_str());
        return AssignBlankAlphaValue();
    }
}
