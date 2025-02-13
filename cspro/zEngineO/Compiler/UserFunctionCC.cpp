#include "stdafx.h"
#include "IncludesCC.h"
#include "AllSymbols.h"
#include "UserFunctionArgumentChecker.h"
#include "Nodes/UserFunction.h"
#include <zLogicO/LocalSymbolStack.h>
#include <zLogicO/SpecialFunction.h>


// --------------------------------------------------------------------------
// compile the user-defined function definition
// --------------------------------------------------------------------------

namespace
{
    const std::vector<SymbolType> SymbolTypesSupportingRecursion =
    {
        SymbolType::Array,          SymbolType::Audio,          SymbolType::Dictionary,
        SymbolType::Document,       SymbolType::File,           SymbolType::Geometry,
        SymbolType::HashMap,        SymbolType::Image,          SymbolType::List,
        SymbolType::Map,            SymbolType::NamedFrequency, SymbolType::Pff,
        SymbolType::Record,         SymbolType::SystemApp,      SymbolType::ValueSet,
        SymbolType::WorkString,     SymbolType::WorkVariable
    };

    const std::vector<SymbolType> SymbolTypesDisallowedAsOptionalParameters
    {
        SymbolType::Array,
        SymbolType::NamedFrequency,
        SymbolType::Report
    };

    const std::vector<SymbolType> SymbolTypesAllowedAsParameters
    {
        SymbolType::Array,          SymbolType::Audio,          SymbolType::Dictionary,
        SymbolType::Document,       SymbolType::File,           SymbolType::Geometry,
        SymbolType::HashMap,        SymbolType::Image,          SymbolType::List,
        SymbolType::Map,            SymbolType::NamedFrequency, SymbolType::Pff,
        SymbolType::Report,         SymbolType::SystemApp,      SymbolType::UserFunction,
        SymbolType::ValueSet
    };
}


UserFunction* LogicCompiler::CompileUserFunction(bool compiling_function_pointer/* = false*/)
{
    ASSERT(Tkn == TOKKWFUNCTION);
    std::shared_ptr<UserFunction> user_function;

    try
    {
        // check if the function is a SQL callback function
        bool sql_callback_function = NextKeywordIf(TOKSQL);

        if( sql_callback_function && compiling_function_pointer )
            IssueError(MGF::UserFunction_function_pointer_invalid_50001, _T("you cannot define the function as a SQL callback"));

        // read an optional return type
        SymbolType return_type;
        int return_string_length = 0;

        if( NextKeywordIf(TOKSTRING) || NextKeywordIf(TOKALPHA) )
        {
            return_type = SymbolType::WorkString;

            if( Tkn == TOKALPHA )
                return_string_length = CompileAlphaLength();
        }

        else
        {
            // read the optional numeric token if present
            NextKeywordIf(TOKNUMERIC);
            return_type = SymbolType::WorkVariable;
        }


        // read the function name
        std::wstring function_name = CompileNewSymbolName();

        // when creating a special function, check the case against the expected case
        if( GetLogicSettings().CaseSensitiveSymbols() )
        {
            for( size_t i = 0; i < _countof(SpecialFunctionNames); ++i )
            {
                if( SO::EqualsNoCase(function_name, SpecialFunctionNames[i]) )
                {
                    if( !SO::Equals(function_name, SpecialFunctionNames[i]) )
                        IssueError(MGF::SpecialFunction_invalid_case_9112, SpecialFunctionNames[i]);

                    break;
                }
            }
        }

        // create the symbol
        user_function = std::make_shared<UserFunction>(std::move(function_name), *m_engineData);

        user_function->SetSqlCallbackFunction(sql_callback_function);
        user_function->SetReturnType(return_type);
        user_function->SetReturnPaddingStringLength(return_string_length);

        m_engineData->AddSymbol(user_function);


        // create a local symbol stack for this function's parameters and local symobls
        Logic::LocalSymbolStack local_symbol_stack = m_symbolTable.CreateLocalSymbolStack();

        // when compiling a function pointer, we need to use a new symbol name compiler so that
        // symbol names are not read when compiling any symbols that are part of the function pointer's signature
        std::unique_ptr<RAII::PushOnStackAndPopOnDestruction<std::function<std::wstring()>>> suppress_reading_symbol_names_compiler;

        if( compiling_function_pointer )
        {
            suppress_reading_symbol_names_compiler =
                std::make_unique<RAII::PushOnStackAndPopOnDestruction<std::function<std::wstring()>>>(
                    m_symbolCompilerModifier.name_compiler,
                        [user_function]()
                        {
                            return FormatTextCS2WS(_T("_%s_p%d"), user_function->GetName().c_str(),
                                                                  static_cast<int>(user_function->GetNumberParameters()) + 1);
                        });
        }

        // read in each of the parameters
        NextToken();
        IssueErrorOnTokenMismatch(TOKLPAREN, MGF::left_parenthesis_expected_in_function_call_14);

        Symbol* first_optional_parameter = nullptr;
        bool previous_token_was_optional = false;

        while( true )
        {
            Symbol* symbol = nullptr;
            std::unique_ptr<Symbol> created_symbol;

            bool parameter_is_optional = previous_token_was_optional;
            previous_token_was_optional = false;

            NextTokenOrNewSymbolName();

            if( Tkn == TOKRPAREN )
            {
                break;
            }

            else if( Tkn == TOKOPTIONAL && !parameter_is_optional )
            {
                if( compiling_function_pointer )
                    IssueError(MGF::UserFunction_function_pointer_invalid_50001, _T("you cannot specify parameters as optional"));

                previous_token_was_optional = true;
                continue;
            }

            // for backwards compatibility, numeric parameters don't have to include 'numeric'
            else if( Tkn == TOKNEWSYMBOL && !compiling_function_pointer )
            {
                created_symbol = std::make_unique<WorkVariable>(Tokstr);
            }

            // SQL callback functions can only have numeric/alpha/string parameters
            else if( sql_callback_function && !( Tkn == TOKNUMERIC || Tkn == TOKALPHA || Tkn == TOKSTRING ) )
            {
                IssueError(MGF::UserFunction_sql_callback_invalid_50002);
            }

            else if( Tkn == TOKNUMERIC )
            {
                symbol = CompileWorkVariableDeclaration();
            }

            else if( Tkn == TOKALPHA || Tkn == TOKSTRING )
            {
                symbol = CompileLogicStringDeclaration(Tkn);
            }

            else if( Tkn == TOKKWARRAY )
            {
                symbol = CompileLogicArrayDeclarationOnly(true);
            }

            else if( Tkn == TOKKWAUDIO )
            {
                symbol = CompileLogicAudioDeclaration();
            }

            else if( Tkn == TOKKWCASE )
            {
                symbol = CompileEngineCaseDeclaration();
            }

            else if( Tkn == TOKKWDATASOURCE )
            {
                symbol = CompileEngineDataRepositoryDeclaration();
            }

            else if( Tkn == TOKKWDOCUMENT )
            {
                symbol = CompileLogicDocumentDeclaration();
            }

            else if( Tkn == TOKKWFILE )
            {
                symbol = CompileLogicFileDeclaration(true);
            }

            else if( Tkn == TOKKWGEOMETRY )
            {
                symbol = CompileLogicGeometryDeclaration();
            }

            else if( Tkn == TOKKWHASHMAP )
            {
                symbol = CompileLogicHashMapDeclaration();
            }

            else if( Tkn == TOKKWIMAGE )
            {
                symbol = CompileLogicImageDeclaration();
            }

            else if( Tkn == TOKKWLIST )
            {
                symbol = CompileLogicListDeclaration();
            }

            else if( Tkn == TOKKWMAP )
            {
                symbol = CompileLogicMapDeclaration();
            }

            else if( Tkn == TOKKWPFF )
            {
                symbol = CompileLogicPffDeclaration();
            }

            else if( Tkn == TOKKWSYSTEMAPP )
            {
                symbol = CompileSystemAppDeclaration();
            }

            else if( Tkn == TOKKWVALUESET )
            {
                symbol = CompileDynamicValueSetDeclaration();
            }

            // compile named frequency parameters
            else if( Tkn == TOKKWFREQ )
            {
                created_symbol = std::make_unique<NamedFrequency>(CompileNewSymbolName());
            }

            // compile report parameters
            else if( Tkn == TOKFUNCTION && CurrentToken.function_details->code == FNPRE77_REPORT_CODE )
            {
                created_symbol = std::make_unique<Report>(CompileNewSymbolName(), std::wstring());
            }

            // compile function pointers
            else if( Tkn == TOKKWFUNCTION )
            {
                // do not allow nested function pointers
                if( compiling_function_pointer )
                    IssueError(MGF::UserFunction_function_pointer_invalid_50001, _T("you cannot declare a function pointer within another function pointer"));

                symbol = CompileUserFunction(true);
            }

            // invalid parameter type
            else
            {
                IssueError(MGF::argument_invalid_560);
            }

            // if the symbol was created here, add it to the symbol table
            if( created_symbol != nullptr )
            {
                symbol = created_symbol.get();
                m_engineData->AddSymbol(std::move(created_symbol));
            }

            ASSERT(symbol != nullptr);

            user_function->AddParameterSymbol(*symbol);

            m_functionParameterSymbols.emplace_back(symbol);

            // arrays, named frequencies, and reports cannot be optional parameters
            if( parameter_is_optional && symbol->IsOneOf(SymbolTypesDisallowedAsOptionalParameters) )
                IssueError(MGF::UserFunction_type_invalid_as_optional_50004, ToString(symbol->GetType()));

            // a comma must separate each parameter
            if( !symbol->IsA(SymbolType::Array) )
                NextToken();

            // numeric and string parameters can have default values
            std::optional<std::variant<double, std::wstring>> default_value;

            if( Tkn == TOKEQOP && !compiling_function_pointer &&
                symbol->IsOneOf(SymbolType::WorkVariable, SymbolType::WorkString) )
            {
                parameter_is_optional = true;

                NextToken();

                int conserver_index = CompileSymbolInitialAssignment(*symbol);

                if( symbol->IsA(SymbolType::WorkVariable) )
                {
                    default_value = GetNumericConstant(conserver_index);
                }

                else
                {
                    default_value = GetStringLiteral(conserver_index);
                }
            }

            if( parameter_is_optional )
            {
                ASSERT(!compiling_function_pointer);

                int default_value_expression;

                if( symbol->IsA(SymbolType::WorkVariable) )
                {
                    default_value_expression = CreateNumericConstantNode(default_value.has_value() ? std::get<double>(*default_value) : NOTAPPL);
                }

                else if( symbol->IsA(SymbolType::WorkString) )
                {
                    default_value_expression = CreateStringLiteralNode(default_value.has_value() ? std::get<std::wstring>(*default_value) : std::wstring());
                }

                else
                {
                    default_value_expression = -1;
                }

                user_function->AddParameterDefaultValue(default_value_expression);

                if( first_optional_parameter == nullptr )
                    first_optional_parameter = symbol;
            }

            // once an optional parameter is specified, all subsequent parameters must be optional
            else if( first_optional_parameter != nullptr )
            {
                IssueError(MGF::UserFunction_parameter_must_be_optional_50003, first_optional_parameter->GetName().c_str());
            }


            if( Tkn == TOKRPAREN )
                break;

            IssueErrorOnTokenMismatch(TOKCOMMA, MGF::function_call_comma_expected_528);
        }

        IssueErrorOnTokenMismatch(TOKRPAREN, MGF::right_parenthesis_expected_in_function_call_17);

        // if compiling a function pointer, we are done now that the function declaration
        // has been read, but some symbols may need to be modified because we do not know
        // exactly what will be assigned to them
        if( compiling_function_pointer )
        {
            UserFunctionArgumentChecker::MarkParametersAsUsedInFunctionPointerUse(*user_function);
        }

        // otherwise we must read the contents of the function
        else
        {
            // modify the current compilation symbol
            RAII::SetValueAndRestoreOnDestruction<const Symbol*> compilation_symbol_conserver(m_compilationSymbol, user_function.get());
            RAII::SetValueAndRestoreOnDestruction<int> compilation_index_conserver(get_COMPILER_DLL_TODO_InCompIdx(), user_function->GetSymbolIndex());

            // compile the function's body, keeping track of any symbols added
            std::vector<int> function_body_symbols;

            local_symbol_stack.SetAddSymbolListener(std::make_unique<std::function<void(int)>>(
                [&](int symbol_index)
                {
                    function_body_symbols.emplace_back(symbol_index);
                }));

            NextToken();

            int function_body = instruc_COMPILER_DLL_TODO(false);

#ifdef _DEBUG
            // ensure that all locally-declared variables support recursion
            for( int symbol_index : function_body_symbols )
            {
                const Symbol& symbol = NPT_Ref(symbol_index);

                // implicit relations can get created as part of loops
                ASSERT(( symbol.IsOneOf(SymbolTypesSupportingRecursion) ) ||
                       ( symbol.IsA(SymbolType::Relation) && symbol.GetName().front() == '_' ));
            }
#endif

            user_function->SetFunctionBodySymbols(std::move(function_body_symbols));

            // set the program index only if there was a function body
            if( function_body != static_cast<int>(m_engineData->logic_byte_code.GetSize()) )
            {
                function_body = WrapNodeAroundScopeChange(local_symbol_stack, function_body, true);

                user_function->SetProgramIndex(function_body);
            }

            // compile the end of the function
            IssueErrorOnTokenMismatch(TOKEND, MGF::UserFunction_must_terminate_with_end_50005);

            NextToken();

            // the function name can be repeated after 'end'
            if( Tkn == TOKUSERFUNCTION && Tokstindex == user_function->GetSymbolIndex() )
                NextToken();
        }
    }

    catch( const Logic::ParserError& )
    {
        // if there is an error, skip until the end token
        if( SkipBasicTokensUntil(TokenCode::TOKEND) )
            NextToken();

        throw;
    }

    return user_function.get();
}



// --------------------------------------------------------------------------
// compile calls to user-defined functions
// --------------------------------------------------------------------------

int LogicCompiler::CompileUserFunctionCall(bool allow_function_name_without_parentheses/* = false*/)
{
    ASSERT(Tkn == TOKUSERFUNCTION);
    UserFunction& user_function = GetSymbolUserFunction(Tokstindex);

    auto& user_function_node = CreateVariableSizeNode<Nodes::UserFunction>(FunctionCode::USERFUNCTIONCALL_CODE, 2 * user_function.GetNumberParameters());
    std::vector<int> reference_destinations;

    auto finalize_and_return_user_function_node = [&]()
    {
        user_function_node.user_function_symbol_index = user_function.GetSymbolIndex();
        user_function_node.reference_destinations_list = CreateListNode(reference_destinations);

        return GetProgramIndex(user_function_node);
    };


    NextToken();

    auto assign_default_arguments = [&](size_t argument_index)
    {
        for( ; argument_index < user_function.GetNumberParameters(); ++argument_index )
        {
            user_function_node.argument_expressions[2 * argument_index] = user_function.GetParameterDefaultValue(argument_index);
            user_function_node.argument_expressions[2 * argument_index + 1] = -1;
        }
    };

    if( Tkn != TOKLPAREN )
    {
        if( allow_function_name_without_parentheses )
        {
            // make sure that the function has no parameters (or that they all have default values)
            if( user_function.GetNumberRequiredParameters() == 0 )
            {
                assign_default_arguments(0);
                return finalize_and_return_user_function_node();
            }
        }

        IssueError(MGF::left_parenthesis_expected_in_function_call_14);
    }

    NextToken();

    // evaluate each of the provided arguments
    try
    {
        UserFunctionArgumentChecker argument_checker(user_function);

        for( size_t argument_index = 0; argument_index < user_function.GetNumberParameters(); ++argument_index )
        {
            if( Tkn == TOKRPAREN )
            {
                // issue an error if there are not enough arguments
                argument_checker.CheckNumberArguments(argument_index);

                // if fine, set the rest of the arguments to their default values
                assign_default_arguments(argument_index);

                break;
            }

            if( argument_index > 0 )
            {
                // all arguments must be separated by a comma
                IssueErrorOnTokenMismatch(TOKCOMMA, MGF::function_call_comma_expected_528);

                NextToken();
            }

            const Symbol& parameter_symbol = user_function.GetParameterSymbol(argument_index);
            int& argument_expression = user_function_node.argument_expressions[2 * argument_index] = -1;
            int& argument_subscript_compilation = user_function_node.argument_expressions[2 * argument_index + 1] = -1;
            bool pass_by_reference = false;

            if( Tkn == TOKREF )
            {
                pass_by_reference = true;
                MarkInputBufferToRestartLater();

                NextToken();
            }

            // numeric/string/alpha parameters map to numeric and string expressions
            const std::optional<DataType> expression_data_type = parameter_symbol.IsA(SymbolType::WorkVariable) ? std::make_optional(DataType::Numeric) :
                                                                 parameter_symbol.IsA(SymbolType::WorkString)   ? std::make_optional(DataType::String) :
                                                                                                                  std::nullopt;

            if( expression_data_type.has_value() )
            {
                argument_checker.CheckExpressionArgument(argument_index, !IsCurrentTokenString());
                argument_expression = CompileExpression(*expression_data_type);
            }

            // other parameters map to symbols
            else
            {
                // ensure that items are coming as their wrapped type
                ASSERT(Tkn != TOKITEM);

                Symbol* argument_symbol = ( Tkn == TOKARRAY ||
                                            Tkn == TOKAUDIO ||
                                            Tkn == TOKCROSSTAB ||
                                            Tkn == TOKDICT ||
                                            Tkn == TOKDOCUMENT ||
                                            Tkn == TOKFILE ||
                                            Tkn == TOKFREQ ||
                                            Tkn == TOKGEOMETRY ||
                                            Tkn == TOKHASHMAP ||
                                            Tkn == TOKIMAGE ||
                                            Tkn == TOKLIST ||
                                            Tkn == TOKMAP ||
                                            Tkn == TOKPFF ||
                                            Tkn == TOKREPORT ||
                                            Tkn == TOKSYSTEMAPP ||
                                            Tkn == TOKUSERFUNCTION ||
                                            Tkn == TOKVALUESET ) ? &NPT_Ref(Tokstindex) : nullptr;

                argument_subscript_compilation = CurrentToken.symbol_subscript_compilation;

                argument_checker.CheckSymbolArgument(argument_index, argument_symbol);
                ASSERT(argument_symbol != nullptr);

                argument_expression = argument_symbol->GetSymbolIndex();

                NextToken();
            }

            // if passing by reference, recompile the function parameter as a destination variable
            if( pass_by_reference )
            {
                if( expression_data_type.has_value() )
                {
                    RestartFromMarkedInputBuffer();
                    NextToken();

                    reference_destinations.emplace_back(parameter_symbol.GetSymbolIndex());
                    reference_destinations.emplace_back(CompileDestinationVariable(*expression_data_type));
                }

                // other symbols are automatically passed by reference
                else
                {
                    ClearMarkedInputBuffer();
                }
            }
        }
    }

    catch( const UserFunctionArgumentChecker::CheckError& error )
    {
        // issue an error when there was a problem with the supplied argument
        IssueError(MGF::UserFunction_expects_argument_50000, user_function.GetName().c_str(), error.GetErrorMessage().c_str());
    }

    IssueErrorOnTokenMismatch(TOKRPAREN, MGF::right_parenthesis_expected_in_function_call_17);

    NextToken();

    return finalize_and_return_user_function_node();
}



// --------------------------------------------------------------------------
// compile calls to the invoke function
// --------------------------------------------------------------------------

int LogicCompiler::CompileInvokeFunction()
{
    ASSERT(CurrentToken.function_details->code == FunctionCode::FNINVOKE_CODE);

    auto& invoke_node = CreateNode<Nodes::Invoke>(FunctionCode::FNINVOKE_CODE);

    NextToken();
    IssueErrorOnTokenMismatch(TOKLPAREN, MGF::left_parenthesis_expected_in_function_call_14);

    NextToken();
    invoke_node.function_name_expression = CompileSymbolNameText(SymbolType::UserFunction, false);

    // arguments can be supplied using arguments := with JSON text...
    invoke_node.arguments_expression = -1;
    invoke_node.arguments_list = -1;

    OptionalNamedArgumentsCompiler optional_named_arguments_compiler(*this);
    optional_named_arguments_compiler.AddArgumentJsonText(_T("arguments"), invoke_node.arguments_expression);

    // ...or by supplying each argument
    if( optional_named_arguments_compiler.Compile() == 0 )
    {
        std::vector<int> arguments;

        while( Tkn == TOKCOMMA )
        {
            // add a symbol...
            std::optional<SymbolType> next_token_symbol_type = GetNextTokenSymbolType();

            if( next_token_symbol_type.has_value() && std::find(SymbolTypesAllowedAsParameters.cbegin(), SymbolTypesAllowedAsParameters.cend(),
                                                                *next_token_symbol_type) != SymbolTypesAllowedAsParameters.cend() )
            {
                NextToken();

                // make sure the symbol has permissions to be used as part of a function call
                Symbol& symbol = NPT_Ref(Tokstindex);
                UserFunctionArgumentChecker::MarkSymbolAsDynamicallyBoundToFunctionParameter(symbol);

                arguments.emplace_back(symbol.GetSymbolIndex());
                arguments.emplace_back(CurrentToken.symbol_subscript_compilation);

                NextToken();
            }

            // ...or an expression
            else
            {
                NextToken();

                const DataType value_data_type = GetCurrentTokenDataType();
                arguments.emplace_back(-1 * static_cast<int>(IsNumeric(value_data_type) ? SymbolType::WorkVariable : SymbolType::WorkString));
                arguments.emplace_back(CompileExpression(value_data_type));
            }
        }

        ASSERT(arguments.size() % 2 == 0);

        invoke_node.arguments_list = CreateListNode(arguments);
    }

    IssueErrorOnTokenMismatch(TOKRPAREN, MGF::right_parenthesis_expected_in_function_call_17);

    NextToken();

    return GetProgramIndex(invoke_node);
}
