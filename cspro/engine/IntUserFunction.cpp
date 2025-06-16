#include "StandardSystemIncludes.h"
#include "INTERPRE.H"
#include "Ctab.h"
#include "EngineExecutor.h"
#include "ParadataDriver.h"
#include "ProgramControl.h"
#include <zEngineO/AllSymbols.h>
#include <zEngineO/UserFunctionArgumentChecker.h>
#include <zEngineO/UserFunctionArgumentEvaluator.h>
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
    std::unique_ptr<LogicArray> CreateArrayFromCrosstab(CTAB* const pCtab)
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

    void CopyArrayToCrosstab(CTAB* const pCtab, const LogicArray& logic_array)
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


double CIntDriver::CallUserFunction(UserFunction& user_function, UserFunctionArgumentEvaluator& argument_evaluator)
{
    // reset the return value
    user_function.Reset();

    // get the set of local symbols that will be used for this call
    UserFunctionLocalSymbolsManager local_symbols_manager = user_function.GetLocalSymbolsManager();

    // evaluate each of the arguments
    std::unique_ptr<std::vector<std::tuple<CTAB*, std::shared_ptr<LogicArray>>>> crosstabs_converted_to_arrays;

    const std::optional<size_t> number_defined_arguments = argument_evaluator.GetNumberArguments();

    for( size_t i = 0; i < user_function.GetNumberParameters(); ++i )
    {
        const int parameter_symbol_index = user_function.GetParameterSymbolIndex(i);
        Symbol& parameter_symbol = local_symbols_manager.GetSymbol(parameter_symbol_index);

        const bool use_default_argument = number_defined_arguments.has_value() ? ( i >= *number_defined_arguments ) :
                                                                                 !argument_evaluator.ArgumentExists(i);
        ASSERT(!use_default_argument || i >= user_function.GetNumberRequiredParameters());

        // numeric
        if( parameter_symbol.IsA(SymbolType::WorkVariable) )
        {
            WorkVariable& work_variable = assert_cast<WorkVariable&>(parameter_symbol);
            work_variable.SetValue(use_default_argument ? Evaluate(user_function.GetParameterDefaultValue(i)) :
                                                          argument_evaluator.GetNumeric(i));
        }


        // string/alpha
        else if( parameter_symbol.IsA(SymbolType::WorkString) )
        {
            WorkString& work_string = assert_cast<WorkString&>(parameter_symbol);
            work_string.SetString(use_default_argument ? EvaluateSharableString(user_function.GetParameterDefaultValue(i)) :
                                                         argument_evaluator.GetString(i));
        }


        // other symbols (not constructed in place using the parameter symbol)
        else if( use_default_argument || !argument_evaluator.ConstructSymbolInPlace(i, parameter_symbol) )
        {
            std::shared_ptr<Symbol> argument_symbol;

            if( !use_default_argument )
            {
                try
                {
                    argument_symbol = argument_evaluator.GetSymbol(i);
                }

                catch( const UserFunctionArgumentEvaluator::InvalidSubscript& )
                {
                    return AssignInvalidValue(user_function.GetReturnDataType());
                }
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
                    // Report argument -> StringWriter parameter
                    if( argument_symbol->IsA(SymbolType::Report) && parameter_symbol.IsA(SymbolType::StringWriter) )
                    {
                        argument_symbol = std::make_unique<StringWriter>(argument_symbol->GetName(), *argument_symbol, *m_engineData);
                    }
#ifdef WIN_DESKTOP
                    // Crosstab argument -> Array parameter
                    else if( argument_symbol->IsA(SymbolType::Crosstab) )
                    {
                        CTAB* const pCtab = assert_cast<CTAB*>(argument_symbol.get());
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


        m_bStopExec = false;
    }

    // once all arguments have been evaluated, move the local symbols into the symbol table
    local_symbols_manager.RunSymbolSubstitution();


    // execute the function
    if( m_traceHandler != nullptr )
    {
        m_traceHandler->Output(FormatText("Entering function %s...", user_function.GetName().c_str()),
                               TraceHandler::OutputType::SystemText);
    }

    try
    {
        // only execute functions that have a body
        if( user_function.GetProgramIndex() >= 0 )
        {
            const bool bRequestIssued = ExecuteProgramStatements(user_function.GetProgramIndex());
            bRequestIssued; // TODO what if RequestIssued is true???
        }
    }

    // an exit statement terminates the function
    catch( const ExitProgramControlException& ) { }

    if( m_traceHandler != nullptr )
    {
        m_traceHandler->Output(FormatText("Exiting function %s...", user_function.GetName().c_str()),
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

    m_bStopExec = false;

    return AssignVariantValue(user_function.GetReturnValue());
}



// --------------------------------------------------------------------------
// routines for running the user-defined functions called in logic
// --------------------------------------------------------------------------

class LogicUserFunctionArgumentEvaluator : public UserFunctionArgumentEvaluator
{
public:
    LogicUserFunctionArgumentEvaluator(CIntDriver& interpreter, const UserFunction& user_function,
                                       const Nodes::UserFunction& user_function_node);

protected:
    std::optional<size_t> GetNumberArguments() override { return m_numberArguments; }

    double GetNumeric(size_t parameter_number) override;
    SharableString GetString(size_t parameter_number) override;
    std::shared_ptr<Symbol> GetSymbol(size_t parameter_number) override;

private:
    CIntDriver& m_interpreter;
    size_t m_numberArguments;
    const int* m_argumentExpressions;
    size_t m_pre80SupportMultiplier;
};


LogicUserFunctionArgumentEvaluator::LogicUserFunctionArgumentEvaluator(CIntDriver& interpreter, const UserFunction& user_function,
                                                                       const Nodes::UserFunction& user_function_node)
    :   m_interpreter(interpreter),
        m_numberArguments(user_function.GetNumberParameters()),
        m_argumentExpressions(user_function_node.argument_expressions),
        m_pre80SupportMultiplier(m_interpreter.GetEngineData().MeetsCompiledLogicVersion(Serializer::Iteration_8_0_000_1) ? 2 : 1)
{
    static_assert(Serializer::GetEarliestSupportedVersion() < Serializer::Iteration_8_0_000_1, "remove m_pre80SupportMultiplier and replace with 2");
    ASSERT(m_argumentExpressions != nullptr);
}


double LogicUserFunctionArgumentEvaluator::GetNumeric(const size_t parameter_number)
{
    ASSERT(parameter_number < m_numberArguments);
    ASSERT(m_interpreter.GetEngineData().PredatesCompiledLogicVersion(Serializer::Iteration_8_0_000_1) || m_argumentExpressions[2 * parameter_number + 1] == -1);

    return m_interpreter.Evaluate(m_argumentExpressions[m_pre80SupportMultiplier * parameter_number]);
}


SharableString LogicUserFunctionArgumentEvaluator::GetString(const size_t parameter_number)
{
    ASSERT(parameter_number < m_numberArguments);
    ASSERT(m_interpreter.GetEngineData().PredatesCompiledLogicVersion(Serializer::Iteration_8_0_000_1) || m_argumentExpressions[2 * parameter_number + 1] == -1);

    return m_interpreter.EvaluateSharableString(m_argumentExpressions[m_pre80SupportMultiplier * parameter_number]);
}


std::shared_ptr<Symbol> LogicUserFunctionArgumentEvaluator::GetSymbol(const size_t parameter_number)
{
    ASSERT(parameter_number < m_numberArguments);
    const int& symbol_index = m_argumentExpressions[m_pre80SupportMultiplier * parameter_number];

    if( symbol_index == -1 )
        return nullptr;

    const int subscript_compilation = ( m_pre80SupportMultiplier == 2 ) ? m_argumentExpressions[m_pre80SupportMultiplier * parameter_number + 1] :
                                                                          -1;

    std::shared_ptr<Symbol> symbol = m_interpreter.GetFromSymbolOrEngineItem<std::shared_ptr<Symbol>>(symbol_index, subscript_compilation);

    if( symbol == nullptr )
        throw UserFunctionArgumentEvaluator::InvalidSubscript();

    return symbol;
}


double CIntDriver::exuserfunctioncall(const int program_index)
{
    const auto& user_function_node = GetNode<Nodes::UserFunction>(program_index);
    UserFunction& user_function = GetSymbolUserFunction(user_function_node.user_function_symbol_index);

    // execute the user-defined function
    LogicUserFunctionArgumentEvaluator argument_evaluator(*this, user_function, user_function_node);
    const double return_value = CallUserFunction(user_function, argument_evaluator);

    // if any arguments were passed by reference, assign the values to the destination variables
    const Nodes::List& reference_destinations_list_node = GetListNode(user_function_node.reference_destinations_list);

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
            AssignValueToSymbol(symbol_value_node, work_string.GetSharableString());
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

class LogicCallbackUserFunctionArgumentEvaluator : public UserFunctionArgumentEvaluator
{
public:
    // callback arguments are evaluated immediately and then stored for later use
    LogicCallbackUserFunctionArgumentEvaluator(CIntDriver& interpreter, FunctionCode function_code,
                                               const UserFunction& user_function, const Nodes::UserFunction& user_function_node);

    FunctionCode GetFunctionCode() const { return m_functionCode; }

    const Nodes::UserFunction& GetUserFunctionNode() const { return m_userFunctionNode; }

protected:
    std::optional<size_t> GetNumberArguments() override { return m_evaluatedArguments.size(); }

    double GetNumeric(size_t parameter_number) override        { return GetArgument<double>(parameter_number); }
    SharableString GetString(size_t parameter_number) override { return GetArgument<SharableString>(parameter_number); }
    std::shared_ptr<Symbol> GetSymbol(size_t parameter_number) override;

private:
    const Logic::SymbolTable& GetSymbolTable() const { return m_interpreter.GetSymbolTable(); }

    template<typename T>
    T GetArgument(size_t parameter_number) const;

private:
    CIntDriver& m_interpreter;
    FunctionCode m_functionCode;
    const Nodes::UserFunction& m_userFunctionNode;

    using EvaluatedArgument = std::variant<double,
                                           SharableString,
                                           std::shared_ptr<std::unique_ptr<SymbolReference<std::shared_ptr<Symbol>>>>>;
    std::vector<EvaluatedArgument> m_evaluatedArguments;
};


LogicCallbackUserFunctionArgumentEvaluator::LogicCallbackUserFunctionArgumentEvaluator(CIntDriver& interpreter, const FunctionCode function_code,
                                                                                       const UserFunction& user_function,
                                                                                       const Nodes::UserFunction& user_function_node)
    :   m_interpreter(interpreter),
        m_functionCode(function_code),
        m_userFunctionNode(user_function_node)
{
    const size_t m_pre80SupportMultiplier = m_interpreter.GetEngineData().MeetsCompiledLogicVersion(Serializer::Iteration_8_0_000_1) ? 2 : 1;
    static_assert(Serializer::GetEarliestSupportedVersion() < Serializer::Iteration_8_0_000_1, "remove m_pre80SupportMultiplier and replace with 2");

    // evaluate the numeric and string arguments
    for( size_t i = 0; i < user_function.GetNumberParameters(); ++i )
    {
        const Symbol& parameter_symbol = user_function.GetParameterSymbol(i);

        // numeric
        if( parameter_symbol.IsA(SymbolType::WorkVariable) )
        {
            m_evaluatedArguments.emplace_back(m_interpreter.Evaluate(user_function_node.argument_expressions[m_pre80SupportMultiplier * i]));
        }

        // string/alpha
        else if( parameter_symbol.IsA(SymbolType::WorkString) )
        {
            m_evaluatedArguments.emplace_back(m_interpreter.EvaluateSharableString(user_function_node.argument_expressions[m_pre80SupportMultiplier * i]));
        }

        // symbols
        else
        {
            const int& symbol_index = user_function_node.argument_expressions[m_pre80SupportMultiplier * i];

            if( symbol_index != -1 )
            {
                const int subscript_compilation = ( m_pre80SupportMultiplier == 2 ) ? user_function_node.argument_expressions[m_pre80SupportMultiplier * i + 1] :
                                                                                      -1;

                // because LogicCallbackUserFunctionArgumentEvaluator may be held by a symbol (e.g., a Map) and this
                // evaluated symbol reference may reference itself, we store this with the engine data that will be cleared
                // prior to the destruction of the symbol table, ensuring that we avoid a memory leak
                auto pointer_to_evaluated_symbol_reference = std::make_shared<std::unique_ptr<SymbolReference<std::shared_ptr<Symbol>>>>(
                                                             std::make_unique<SymbolReference<std::shared_ptr<Symbol>>>(
                                                             m_interpreter.EvaluateSymbolReference<std::shared_ptr<Symbol>>(symbol_index, subscript_compilation)));

                m_interpreter.GetEngineData().evaluated_symbol_references.emplace_back(pointer_to_evaluated_symbol_reference);
                m_evaluatedArguments.emplace_back(std::move(pointer_to_evaluated_symbol_reference));
            }
        }
    }
}


template<typename T>
T LogicCallbackUserFunctionArgumentEvaluator::GetArgument(const size_t parameter_number) const
{
    ASSERT(parameter_number < m_evaluatedArguments.size());

    // handle numerics and strings
    ASSERT(std::holds_alternative<T>(m_evaluatedArguments[parameter_number]));
    return std::get<T>(m_evaluatedArguments[parameter_number]);
}


std::shared_ptr<Symbol> LogicCallbackUserFunctionArgumentEvaluator::GetSymbol(const size_t parameter_number)
{
    ASSERT(parameter_number < m_evaluatedArguments.size());

    // handle symbols
    const std::shared_ptr<std::unique_ptr<SymbolReference<std::shared_ptr<Symbol>>>>& symbol_reference = std::get<2>(m_evaluatedArguments[parameter_number]);
    ASSERT(symbol_reference != nullptr && (*symbol_reference) != nullptr);

    std::shared_ptr<Symbol> symbol = m_interpreter.GetFromSymbolOrEngineItem<std::shared_ptr<Symbol>>(*(*symbol_reference));

    if( symbol == nullptr )
        throw UserFunctionArgumentEvaluator::InvalidSubscript();

    return symbol;
}


std::unique_ptr<UserFunctionArgumentEvaluator> CIntDriver::EvaluateArgumentsForCallbackUserFunction(const int program_index, const FunctionCode function_code)
{
    const auto& user_function_node = GetNode<Nodes::UserFunction>(program_index);
    const UserFunction& user_function = GetSymbolUserFunction(user_function_node.user_function_symbol_index);

    return std::make_unique<LogicCallbackUserFunctionArgumentEvaluator>(*this, function_code, user_function, user_function_node);
}


void CIntDriver::ExecuteCallbackUserFunction(const int field_symbol_index, UserFunctionArgumentEvaluator& argument_evaluator)
{
    // these statements clear any preexisting stuff that might have been going on
    m_bSkipStmt = false;
    m_bStopExec = m_bStopProc;
    SetRequestIssued(false);

    m_FieldSymbol = field_symbol_index;
    m_iExSymbol = field_symbol_index;
    m_iExLevel = SymbolCalculator::GetLevelNumber_base1(NPT_Ref(field_symbol_index));

    LogicCallbackUserFunctionArgumentEvaluator* const actual_argument_evaluator = assert_cast<LogicCallbackUserFunctionArgumentEvaluator*>(&argument_evaluator);

    const Nodes::UserFunction& user_function_node = actual_argument_evaluator->GetUserFunctionNode();
    UserFunction& user_function = GetSymbolUserFunction(user_function_node.user_function_symbol_index);

    if( Paradata::Logger::IsOpen() )
    {
        const Paradata::OperatorSelectionEvent::Source source =
            ( actual_argument_evaluator->GetFunctionCode() == FNUSERBAR_CODE ) ? Paradata::OperatorSelectionEvent::Source::Userbar :
                                                                                 Paradata::OperatorSelectionEvent::Source::MapShow;

        auto operator_selection_event = std::make_unique<Paradata::OperatorSelectionEvent>(source);
        operator_selection_event->SetPostSelectionValues(std::nullopt, user_function.GetName(), false);
        m_paradataDriver->RegisterAndLogEvent(std::move(operator_selection_event));
    }

    CallUserFunction(user_function, argument_evaluator);

    m_FieldSymbol = 0;

    if( m_bStopProc )
        WindowsDesktopMessage::Post(WM_IMSA_USERBAR_UPDATE, 0, -1); // stop the program
}



// --------------------------------------------------------------------------
// routines for the invoke function
// --------------------------------------------------------------------------

class InvokeArgumentsProvidedUsingJsonArgumentEvaluator : public UserFunctionArgumentEvaluator
{
public:
    InvokeArgumentsProvidedUsingJsonArgumentEvaluator(CIntDriver& interpreter, const UserFunction& user_function, const JsonNode& json_arguments);

    const Symbol* GetLastEvaluatedParameterSymbol() const { return m_parameterSymbol; }

protected:
    std::optional<size_t> GetNumberArguments() override { return std::nullopt; }
    bool ArgumentExists(size_t parameter_number) override;

    double GetNumeric(size_t parameter_number) override;
    SharableString GetString(size_t parameter_number) override;
    bool ConstructSymbolInPlace(size_t parameter_number, Symbol& parameter_symbol) override;

private:
    CIntDriver& m_interpreter;
    const UserFunction& m_userFunction;
    const JsonNode& m_jsonArguments;
    const Symbol* m_parameterSymbol;
};


InvokeArgumentsProvidedUsingJsonArgumentEvaluator::InvokeArgumentsProvidedUsingJsonArgumentEvaluator(CIntDriver& interpreter, const UserFunction& user_function,
                                                                                                     const JsonNode& json_arguments)
    :   m_interpreter(interpreter),
        m_userFunction(user_function),
        m_jsonArguments(json_arguments),
        m_parameterSymbol(nullptr)
{
}


bool InvokeArgumentsProvidedUsingJsonArgumentEvaluator::ArgumentExists(const size_t parameter_number)
{
    m_parameterSymbol = &m_userFunction.GetParameterSymbol(parameter_number);

    return ( m_jsonArguments.Contains(m_parameterSymbol->GetName()) )           ? true :
           ( parameter_number >= m_userFunction.GetNumberRequiredParameters() ) ? false :
           throw CSProException("No argument for '%s' provided", m_parameterSymbol->GetName().c_str());
}


double InvokeArgumentsProvidedUsingJsonArgumentEvaluator::GetNumeric(const size_t parameter_number)
{
    const JsonNode argument_node = m_jsonArguments.Get(m_parameterSymbol->GetName());
    return argument_node.GetEngineValue<double>();
}


SharableString InvokeArgumentsProvidedUsingJsonArgumentEvaluator::GetString(const size_t parameter_number)
{
    const JsonNode argument_node = m_jsonArguments.Get(m_parameterSymbol->GetName());
    return argument_node.GetEngineValue<SharableString>();
}


bool InvokeArgumentsProvidedUsingJsonArgumentEvaluator::ConstructSymbolInPlace(const size_t parameter_number, Symbol& parameter_symbol)
{
    try
    {
        parameter_symbol.SetValueFromJson(m_jsonArguments[m_parameterSymbol->GetName()]);
        return true;
    }

    catch( const Symbol::NoSetValueFromJsonRoutine& )
    {
        throw CSProException("Arguments of type '%s' are currently not supported when supplied via JSON",
                             ToString(m_parameterSymbol->GetType()));
    }

    catch( const CSProException& exception )
    {
        throw CSProException("Error processing argument '%s': %s",
                             m_parameterSymbol->GetName().c_str(),
                             exception.what());
    }
}



class InvokeArgumentsProvidedDirectlyArgumentEvaluator : public UserFunctionArgumentEvaluator
{
public:
    InvokeArgumentsProvidedDirectlyArgumentEvaluator(CIntDriver& interpreter, const std::vector<std::tuple<int, int>>& arguments);

protected:
    std::optional<size_t> GetNumberArguments() override { return m_arguments.size(); }

    double GetNumeric(size_t parameter_number) override;
    SharableString GetString(size_t parameter_number) override;
    std::shared_ptr<Symbol> GetSymbol(size_t parameter_number) override;

private:
    CIntDriver& m_interpreter;
    const std::vector<std::tuple<int, int>>& m_arguments;
};


InvokeArgumentsProvidedDirectlyArgumentEvaluator::InvokeArgumentsProvidedDirectlyArgumentEvaluator(CIntDriver& interpreter,
                                                                                                   const std::vector<std::tuple<int, int>>& arguments)
    :   m_interpreter(interpreter),
        m_arguments(arguments)
{
}


double InvokeArgumentsProvidedDirectlyArgumentEvaluator::GetNumeric(const size_t parameter_number)
{
    ASSERT(( std::get<0>(m_arguments[parameter_number]) == ( -1 * static_cast<int>(SymbolType::WorkVariable)) ) ||
           ( std::get<0>(m_arguments[parameter_number]) == -1 ));

    return m_interpreter.Evaluate(std::get<1>(m_arguments[parameter_number]));
}


SharableString InvokeArgumentsProvidedDirectlyArgumentEvaluator::GetString(const size_t parameter_number)
{
    ASSERT(( std::get<0>(m_arguments[parameter_number]) == ( -1 * static_cast<int>(SymbolType::WorkString)) ) ||
           ( std::get<0>(m_arguments[parameter_number]) == -1 ));

    return m_interpreter.EvaluateSharableString(std::get<1>(m_arguments[parameter_number]));
}


std::shared_ptr<Symbol> InvokeArgumentsProvidedDirectlyArgumentEvaluator::GetSymbol(const size_t parameter_number)
{
    const int& symbol_index = std::get<0>(m_arguments[parameter_number]);

    if( symbol_index == -1 )
        return nullptr;

    std::shared_ptr<Symbol> symbol = m_interpreter.GetFromSymbolOrEngineItem<std::shared_ptr<Symbol>>(symbol_index, std::get<1>(m_arguments[parameter_number]));

    if( symbol == nullptr )
        throw UserFunctionArgumentEvaluator::InvalidSubscript();

    return symbol;
}


template<typename T>
InterpreterExecuteResult CIntDriver::RunInvoke(const std::string_view function_name_sv, const T& variable_arguments, CancelFlag* const cancel_flag)
{
    UserFunction* user_function = nullptr;

    try
    {
        Symbol& symbol = GetSymbolFromSymbolName(function_name_sv, SymbolType::UserFunction);

        if( symbol.IsA(SymbolType::UserFunction) )
            user_function = assert_cast<UserFunction*>(&symbol);
    }
    catch(...) { }

    if( user_function == nullptr )
        throw CSProException("no user-defined function with the name exists.");


    // arguments provided as JSON text
    if constexpr(std::is_same_v<T, std::string> || std::is_same_v<T, JsonNode>)
    {
        std::unique_ptr<InvokeArgumentsProvidedUsingJsonArgumentEvaluator> argument_evaluator;

        try
        {
            cs::shared_or_raw_ptr<const JsonNode> json_arguments;

            if constexpr(std::is_same_v<T, std::string>)
            {
                const std::string& json_arguments_text = variable_arguments;
                json_arguments = std::make_unique<JsonNode>(Json::Parse(json_arguments_text, GetEngineJsonReaderInterface()));
            }

            else
            {
                json_arguments = &variable_arguments;
            }

            // forward any cancelation requests to the interpreter's cancelation flag
            std::optional<CancelFlag::ListenerHolder> cancel_flag_listener_holder;

            if( cancel_flag != nullptr )
                cancel_flag_listener_holder.emplace(cancel_flag->AddListener([&]() { m_bStopProc = true; }));

            // execute the function
            ASSERT(json_arguments != nullptr);
            argument_evaluator = std::make_unique<InvokeArgumentsProvidedUsingJsonArgumentEvaluator>(*this, *user_function, *json_arguments);
            return Execute(user_function->GetReturnDataType(), [&]() { return CallUserFunction(*user_function, *argument_evaluator); });
        }

        catch( const JsonParseException& exception )
        {
            std::string message = exception.what();

            if( argument_evaluator != nullptr && argument_evaluator->GetLastEvaluatedParameterSymbol() != nullptr )
                message.append(FormatText(" (for argument '%s'", argument_evaluator->GetLastEvaluatedParameterSymbol()->GetName().c_str()));

            throw CSProException(message);
        }
    }


    // arguments provided directly in positional order
    else
    {
        const Nodes::List& arguments_list = variable_arguments;

        try
        {
            UserFunctionArgumentChecker argument_checker(nullptr, *user_function);

            const size_t number_arguments = arguments_list.number_elements / 2;

            argument_checker.CheckNumberArguments(number_arguments);

            // evaluate each of the arguments and determine if they are valid
            std::vector<std::tuple<int, int>> arguments;
            size_t argument_index = 0;

            for( ; argument_index < number_arguments; ++argument_index )
            {
                auto& [symbol_index, subscript_compilation] = arguments.emplace_back(arguments_list.elements[2 * argument_index],
                                                                                     arguments_list.elements[2 * argument_index + 1]);

                if( m_engineData->PredatesCompiledLogicVersion(Serializer::Iteration_8_0_000_1) )
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

            // execute the function
            InvokeArgumentsProvidedDirectlyArgumentEvaluator argument_evaluator(*this, arguments);
            return Execute(user_function->GetReturnDataType(), [&]() { return CallUserFunction(*user_function, argument_evaluator); });
        }

        catch( const UserFunctionArgumentChecker::CheckError& error )
        {
            // rethrow the error in the message style that would be issues during compile-time
            throw CSProException("the user-defined function expects %s", error.what());
        }
    }
}

template InterpreterExecuteResult CIntDriver::RunInvoke(std::string_view function_name_sv, const JsonNode& variable_arguments, CancelFlag* cancel_flag);


double CIntDriver::ex_invoke(const int program_index)
{
    const auto& invoke_node = GetNode<Nodes::Invoke>(program_index);
    const SharableString function_name = EvaluateSharableString(invoke_node.function_name_expression);

    try
    {
        InterpreterExecuteResult execute_result;

        if( invoke_node.arguments_expression != -1 )
        {
            const SharableString json_arguments_text = EvaluateSharableString(invoke_node.arguments_expression);
            execute_result = RunInvoke(*function_name, *json_arguments_text, nullptr);
        }

        else
        {
            const Nodes::List& arguments_list = GetListNode(invoke_node.arguments_list);
            execute_result = RunInvoke(*function_name, arguments_list, nullptr);
        }

        if( execute_result.program_control_executed )
            RethrowProgramControlExceptions();

        return AssignString(std::holds_alternative<SharableString>(execute_result.result) ? std::move(std::get<SharableString>(execute_result.result)) :
                                                                                            DoubleToString(std::get<double>(execute_result.result)));
    }

    catch( const CSProException& exception )
    {
        issaerror(MessageType::Error, 50051, Logic::FunctionTable::GetFunctionName(invoke_node.function_code),
                                             function_name->c_str(), exception.what());
        return AssignStringNull();
    }
}
