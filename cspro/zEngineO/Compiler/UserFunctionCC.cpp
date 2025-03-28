#include "stdafx.h"
#include "IncludesCC.h"
#include "AllSymbols.h"
#include "JavaScriptProcessor.h"
#include "UserFunctionArgumentChecker.h"
#include "Nodes/UserFunction.h"
#include <zLogicO/LocalSymbolStack.h>
#include <zLogicO/SpecialFunction.h>


// --------------------------------------------------------------------------
// compile the user-defined function definition
// --------------------------------------------------------------------------

namespace
{
    constexpr SymbolType SymbolTypesSupportingRecursion[] =
    {
        SymbolType::Array,          SymbolType::Audio,          SymbolType::Dictionary,
        SymbolType::Document,       SymbolType::File,           SymbolType::Geometry,
        SymbolType::HashMap,        SymbolType::Image,          SymbolType::List,
        SymbolType::Map,            SymbolType::NamedFrequency, SymbolType::Pff,
        SymbolType::Record,         SymbolType::SystemApp,      SymbolType::ValueSet,
        SymbolType::WorkString,     SymbolType::WorkVariable
    };

    constexpr SymbolType SymbolTypesDisallowedAsOptionalParameters[]
    {
        SymbolType::Array,
        SymbolType::NamedFrequency,
        SymbolType::Report
    };

    constexpr SymbolType SymbolTypesAllowedAsParameters[]
    {
        SymbolType::Array,          SymbolType::Audio,          SymbolType::Dictionary,
        SymbolType::Document,       SymbolType::File,           SymbolType::Geometry,
        SymbolType::HashMap,        SymbolType::Image,          SymbolType::List,
        SymbolType::Map,            SymbolType::NamedFrequency, SymbolType::Pff,
        SymbolType::Report,         SymbolType::SystemApp,      SymbolType::UserFunction,
        SymbolType::ValueSet
    };
}


enum class LogicCompiler::UserFunctionParametersType { FunctionPointer, JavaScript, All };


UserFunction* LogicCompiler::CompileUserFunction(const bool compiling_function_pointer)
{
    ASSERT(Tkn == TOKKWFUNCTION);
    std::shared_ptr<UserFunction> user_function;
    std::optional<UserFunctionParametersType> parameters_type = compiling_function_pointer ? std::make_optional(UserFunctionParametersType::FunctionPointer) :
                                                                                             std::nullopt;

    try
    {
        // check if the function is a SQL callback function...
        const bool sql_callback_function = NextKeywordIf(TOKSQL);

        if( sql_callback_function )
        {
            if( compiling_function_pointer )
                IssueError(MGF::UserFunction_function_pointer_invalid_50001, "you cannot define the function as a SQL callback");
        }

        // ...or a JavaScript function
        const bool javascript_function_declaration = NextKeywordIf("JS");

        if( javascript_function_declaration )
        {
            if( compiling_function_pointer )
                IssueError(MGF::UserFunction_function_pointer_invalid_50001, "you cannot define the function as a JavaScript function");

            if( !m_symbolCompilerModifier.declare_variable )
                IssueError(MGF::UserFunction_JavaScript_function_requires_declare_50010);

            ASSERT(!parameters_type.has_value());
            parameters_type = UserFunctionParametersType::JavaScript;
        }


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
        std::string function_name = CompileNewSymbolName(compiling_function_pointer ? std::nullopt :
                                                                                      std::make_optional(TOKUSERFUNCTION));

        // when creating a special function, check the case against the expected case
        if( GetLogicSettings().CaseSensitiveSymbols() )
        {
            for( const char* const special_function_name : SpecialFunctionNames )
            {
                if( SO::EqualsNoCase(function_name, special_function_name) )
                {
                    if( function_name != special_function_name )
                        IssueError(MGF::SpecialFunction_invalid_case_9112, special_function_name);

                    break;
                }
            }
        }

        // if the function name is not new, the function must have been previously declared
        const bool function_was_previously_declared = ( Tkn == TOKUSERFUNCTION );

        if( function_was_previously_declared )
        {
            const auto& lookup = m_declaredSymbolIndices.find(Tokstindex);

            if( lookup == m_declaredSymbolIndices.cend() )
                IssueError(MGF::symbol_name_in_use_102, function_name.c_str());

            user_function = std::dynamic_pointer_cast<UserFunction, Symbol>(GetSharedSymbol(*lookup));
            ASSERT(user_function != nullptr);

            // make sure the declared function matches what is being defined
            try
            {
                user_function->CompareDeclarationAttributes(return_type, return_string_length, sql_callback_function);
            }

            catch( const CSProException& exception )
            {
                ASSERT(dynamic_cast<const Symbol::CompareDeclarationAttributesException*>(&exception) != nullptr);

                IssueError(MGF::UserFunction_declaration_definition_mismatch_50006,
                           user_function->GetName().c_str(), exception.what());
            }
        }

        // otherwise we will create the symbol
        else
        {
            user_function = std::make_unique<UserFunction>(std::move(function_name), *m_engineData);

            user_function->SetSqlCallbackFunction(sql_callback_function);
            user_function->SetReturnType(return_type);
            user_function->SetReturnPaddingStringLength(return_string_length);

            m_engineData->AddSymbol(user_function);
        }


        // create a local symbol stack for this function's parameters and local symobls
        Logic::LocalSymbolStack local_symbol_stack = m_symbolTable.CreateLocalSymbolStack();

        CompileUserFunctionParameters(*user_function, function_was_previously_declared,
                                      parameters_type.value_or(UserFunctionParametersType::All));


        // if compiling a function pointer, we are done now that the function declaration
        // has been read, but some symbols may need to be modified because we do not know
        // exactly what will be assigned to them
        if( compiling_function_pointer )
        {
            UserFunctionArgumentChecker::MarkParametersAsUsedInFunctionPointerUse(*user_function);
            return user_function.get();
        }

        NextToken();

        // if a JavaScript function, set the function body to a node that will execute the function in the JavaScript environment
        if( javascript_function_declaration )
        {
            auto& javascript_function_call_node = CreateNode<Nodes::FunctionCall>(FunctionCode::JSFN_USERFUNCTIONWRAPPER_CODE);
            javascript_function_call_node.next_st = -1;
            javascript_function_call_node.expression = user_function->GetSymbolIndex();
            user_function->SetProgramIndex(GetProgramIndex(javascript_function_call_node));
        }

        // if a function declaration, skip compiling the function and mark this as needing to be defined later
        else if( m_symbolCompilerModifier.declare_variable )
        {
            m_declaredSymbolIndices.insert(user_function->GetSymbolIndex());
        }

        // otherwise we must read the contents of the function
        else
        {
            // modify the current compilation symbol
            const RAII::SetValueAndRestoreOnDestruction<const Symbol*> compilation_symbol_conserver(m_compilationSymbol, user_function.get());
            const RAII::SetValueAndRestoreOnDestruction<int> compilation_index_conserver(get_COMPILER_DLL_TODO_InCompIdx(), user_function->GetSymbolIndex());

            // compile the function's body, keeping track of any symbols added
            std::vector<int> function_body_symbols;

            local_symbol_stack.SetAddSymbolListener(std::make_unique<std::function<void(int)>>(
                [&](const int symbol_index)
                {
                    function_body_symbols.emplace_back(symbol_index);
                }));

            int function_body = instruc_COMPILER_DLL_TODO(false);

#ifdef _DEBUG
            // ensure that all locally-declared variables support recursion
            for( const int symbol_index : function_body_symbols )
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

            // if this was a previously-declared function, mark it as defined
            if( function_was_previously_declared )
                m_declaredSymbolIndices.erase(user_function->GetSymbolIndex());
        }

        // compile the end of the function
        ASSERT(!compiling_function_pointer);

        if( m_symbolCompilerModifier.declare_variable )
        {
            IssueErrorOnTokenMismatch(TOKSEMICOLON, MGF::UserFunction_declaration_cannot_have_body_50007);
        }

        else if( !compiling_function_pointer )
        {
            IssueErrorOnTokenMismatch(TOKEND, MGF::UserFunction_must_terminate_with_end_50005);

            NextToken();

            // the function name can be repeated after 'end'
            if( Tkn == TOKUSERFUNCTION && Tokstindex == user_function->GetSymbolIndex() )
                NextToken();
        }

        return user_function.get();
    }

    catch( const Logic::ParserError& )
    {
        // if there is an error, skip until the end token
        if( SkipBasicTokensUntil(TokenCode::TOKEND) )
            NextToken();

        throw;
    }
}


void LogicCompiler::CompileUserFunctionParameters(UserFunction& user_function, const bool function_was_previously_declared,
                                                  const UserFunctionParametersType parameters_type)
{
    ASSERT(user_function.GetNumberParameters() == 0 || function_was_previously_declared);

    const bool optional_parameters_previously_defined = ( function_was_previously_declared &&
                                                          user_function.GetNumberRequiredParameters() < user_function.GetNumberParameters() );
    std::vector<int> parameter_symbol_indices;
    std::vector<int> parameter_default_values;

    // when compiling a function pointer, or when declaring a function, we will use a different
    // symbol name compiler so that symbol names are optional (because they won't be used)
    std::optional<RAII::PushOnStackAndPopOnDestruction<std::function<std::string()>>> suppress_reading_symbol_names_compiler;

    if( parameters_type == UserFunctionParametersType::FunctionPointer ||
        m_symbolCompilerModifier.declare_variable )
    {
        suppress_reading_symbol_names_compiler.emplace(m_symbolCompilerModifier.name_compiler,
            [&]()
            {
                MarkInputBufferToRestartLater();

                NextTokenOrNewSymbolName();

                if( Tkn == TOKNEWSYMBOL )
                {
                    ClearMarkedInputBuffer();
                    return Tokstr;
                }

                else
                {
                    RestartFromMarkedInputBuffer();

                    // create a dummy name
                    return FormatText("_%s_p%d", user_function.GetName().c_str(),
                                                 static_cast<int>(parameter_symbol_indices.size() + 1));
                }
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
            previous_token_was_optional = true;
            continue;
        }

        // for backwards compatibility, numeric parameters don't have to include 'numeric'
        else if( Tkn == TOKNEWSYMBOL && parameters_type != UserFunctionParametersType::FunctionPointer )
        {
            created_symbol = std::make_unique<WorkVariable>(Tokstr);
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
            created_symbol = Report::CreateReportFunctionParamter(CompileNewSymbolName());
        }

        // compile function pointers
        else if( Tkn == TOKKWFUNCTION )
        {
            // do not allow nested function pointers
            if( parameters_type == UserFunctionParametersType::FunctionPointer )
                IssueError(MGF::UserFunction_function_pointer_invalid_50001, "you cannot declare a function pointer within another function pointer");

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

        parameter_symbol_indices.emplace_back(symbol->GetSymbolIndex());

        m_functionParameterSymbols.emplace_back(symbol);


        // check that the symbol is valid for...

        // ...SQL callback functions, which can only have numeric/alpha/string parameters
        if( user_function.IsSqlCallbackFunction() &&
            !symbol->IsOneOf(SymbolType::WorkVariable, SymbolType::WorkString) )
        {
            IssueError(MGF::UserFunction_sql_callback_invalid_50002);
        }

        // ...and JavaScript functions
        if( parameters_type == UserFunctionParametersType::JavaScript &&
            !EngineJavaScriptProcessor::IsSymbolTypeAllowedAsArgument(symbol->GetType()) )
        {
            IssueError(MGF::JavaScript_function_symbol_conversion_invalid_100466,
                       ToString(symbol->GetType()), symbol->GetName().c_str(), user_function.GetName().c_str());
        }


        // arrays, named frequencies, and reports cannot be optional parameters
        if( parameter_is_optional && symbol->IsOneOf(SymbolTypesDisallowedAsOptionalParameters) )
            IssueError(MGF::UserFunction_type_invalid_as_optional_50004, ToString(symbol->GetType()));

        // a comma must separate each parameter
        if( !symbol->IsA(SymbolType::Array) )
            NextToken();

        // numeric and string parameters can have default values
        std::optional<std::variant<double, std::string>> default_value;

        if( Tkn == TOKEQOP &&
            parameters_type != UserFunctionParametersType::FunctionPointer &&
            symbol->IsOneOf(SymbolType::WorkVariable, SymbolType::WorkString) )
        {
            parameter_is_optional = true;

            NextToken();

            const int conserver_index = CompileSymbolInitialAssignment(*symbol);

            if( symbol->IsA(SymbolType::WorkVariable) )
            {
                default_value = GetNumericConstant(conserver_index);
            }

            else
            {
                default_value = m_engineData->string_literals[conserver_index].GetString();
            }
        }

        if( parameter_is_optional )
        {
            if( parameters_type == UserFunctionParametersType::FunctionPointer )
                IssueError(MGF::UserFunction_function_pointer_invalid_50001, "you cannot specify parameters as optional");

            if( optional_parameters_previously_defined )
                IssueError(MGF::UserFunction_optional_parameters_redefinition_50008);

            int default_value_expression;

            if( symbol->IsA(SymbolType::WorkVariable) )
            {
                default_value_expression = CreateNumericConstantNode(default_value.has_value() ? std::get<double>(*default_value) : NOTAPPL);
            }

            else if( symbol->IsA(SymbolType::WorkString) )
            {
                default_value_expression = CreateStringLiteralNode(default_value.has_value() ? std::move(std::get<std::string>(*default_value)) :
                                                                                               std::string());
            }

            else
            {
                default_value_expression = -1;
            }

            parameter_default_values.emplace_back(default_value_expression);

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


    // if the function was previously declared, make sure that the parameters match
    if( function_was_previously_declared )
    {
        ASSERT(!optional_parameters_previously_defined || parameter_default_values.empty());

        if( parameter_symbol_indices.size() != user_function.GetNumberParameters() )
        {
            IssueError(MGF::UserFunction_declaration_definition_mismatch_50006,
                       user_function.GetName().c_str(),
                       FormatText("%d parameter%s", static_cast<int>(user_function.GetNumberParameters()),
                                                    PluralizeWord(user_function.GetNumberParameters())).c_str());
        }

        for( size_t i = 0; i < parameter_symbol_indices.size(); ++i )
        {
            Symbol& symbol = NPT_Ref(parameter_symbol_indices[i]);
            const Symbol& previous_symbol = NPT_Ref(user_function.GetParameterSymbolIndex(i));

            try
            {
                if( symbol.GetType() != previous_symbol.GetType() )
                {
                    throw Symbol::CompareDeclarationAttributesException("symbol type: %s vs. %s", ToDisplayString(symbol.GetType()),
                                                                                                  ToDisplayString(previous_symbol.GetType()));
                }

                symbol.CompareDeclarationAttributes(previous_symbol);

                ASSERT(symbol.GetSubType() == previous_symbol.GetSubType());

                symbol.CopyCompileTimeAttributes(previous_symbol);
            }

            catch( const CSProException& exception )
            {
                ASSERT(dynamic_cast<const Symbol::CompareDeclarationAttributesException*>(&exception) != nullptr);

                // add the symbol name after the parameter number when it is valid
                const std::string symbol_name = ( symbol.GetName().front() != '_' ) ? FormatText(", '%s'", symbol.GetName().c_str()) :
                                                                                      std::string();

                IssueError(MGF::UserFunction_declaration_definition_mismatch_50006,
                           user_function.GetName().c_str(),
                           FormatText("%s (parameter #%d%s)", exception.what(), static_cast<int>(i + 1), symbol_name.c_str()).c_str());
            }
        }
    }

    user_function.SetParameters(std::move(parameter_symbol_indices), std::move(parameter_default_values));
}


int LogicCompiler::CompileUserFunctionDeclarations()
{
    ASSERT(Tkn == TOKKWFUNCTION && IsGlobalCompilation());

    CompileUserFunction(false);

    return -1;
}



// --------------------------------------------------------------------------
// compile calls to user-defined functions
// --------------------------------------------------------------------------

int LogicCompiler::CompileUserFunctionCall(const bool allow_function_name_without_parentheses/* = false*/)
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

                Symbol* const argument_symbol = ( Tkn == TOKARRAY ||
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
        IssueError(MGF::UserFunction_expects_argument_50000, user_function.GetName().c_str(), error.what());
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
    optional_named_arguments_compiler.AddArgumentJsonText("arguments", invoke_node.arguments_expression);

    // ...or by supplying each argument
    if( optional_named_arguments_compiler.Compile() == 0 )
    {
        std::vector<int> arguments;

        while( Tkn == TOKCOMMA )
        {
            // add a symbol...
            const std::optional<SymbolType> next_token_symbol_type = GetNextTokenSymbolType();

            if( next_token_symbol_type.has_value() && std::find(std::begin(SymbolTypesAllowedAsParameters), std::end(SymbolTypesAllowedAsParameters),
                                                                *next_token_symbol_type) != std::end(SymbolTypesAllowedAsParameters) )
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
