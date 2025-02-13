#include "stdafx.h"
#include "IncludesCC.h"
#include "List.h"
#include "Pff.h"
#include "PffExecutor.h"


LogicPff* LogicCompiler::CompileLogicPffDeclaration()
{
    std::wstring pff_name = CompileNewSymbolName();

    auto logic_pff = std::make_shared<LogicPff>(std::move(pff_name));

    m_engineData->AddSymbol(logic_pff);

    return logic_pff.get();
}


int LogicCompiler::CompileLogicPffDeclarations()
{
    ASSERT(Tkn == TOKKWPFF);
    Nodes::SymbolReset* symbol_reset_node = nullptr;

    do
    {
        LogicPff* logic_pff = CompileLogicPffDeclaration();

        int& initialize_value = AddSymbolResetNode(symbol_reset_node, *logic_pff);

        NextToken();

        if( Tkn == TOKEQOP )
            initialize_value = CompileLogicPffComputeInstruction(logic_pff);

    } while( Tkn == TOKCOMMA );

    IssueErrorOnTokenMismatch(TOKSEMICOLON, MGF::expecting_semicolon_30);

    return GetOptionalProgramIndex(symbol_reset_node);
}


int LogicCompiler::CompileLogicPffComputeInstruction(const LogicPff* pff_from_declaration/* = nullptr*/)
{
    const LogicPff* logic_pff = pff_from_declaration;

    if( IsGlobalCompilation() )
        IssueError(MGF::symbol_assignment_not_allowed_in_proc_global_687, ToString(SymbolType::Pff));

    if( logic_pff == nullptr )
    {
        ASSERT(Tkn == TOKPFF);
        logic_pff = &GetSymbolLogicPff(Tokstindex);

        NextToken();

        IssueErrorOnTokenMismatch(TOKEQOP, MGF::Pff_invalid_assignment_47193);
    }

    ASSERT(Tkn == TOKEQOP);

    NextToken();
    IssueErrorOnTokenMismatch(TOKPFF, MGF::Pff_invalid_assignment_47193);

    auto& symbol_compute_node = CreateNode<Nodes::SymbolCompute>(FunctionCode::PFFFN_COMPUTE_CODE);

    symbol_compute_node.next_st = -1;
    symbol_compute_node.lhs_symbol_index = logic_pff->GetSymbolIndex();
    symbol_compute_node.rhs_symbol_type = SymbolType::Pff;
    symbol_compute_node.rhs_symbol_index = Tokstindex;

    NextToken();

    return GetProgramIndex(symbol_compute_node);
}


int LogicCompiler::CompileLogicPffFunctions()
{
    // compiling pff_name.exec()
    //           pff_name.getProperty(property_name[, value_list])
    //           pff_name.load(filename | form_file_name)
    //           pff_name.save(filename)
    //           pff_name.setProperty(property_name, value_string | value_number | value_list | value_dictionary)
    FunctionCode function_code = CurrentToken.function_details->code;
    const LogicPff& logic_pff = assert_cast<const LogicPff&>(*CurrentToken.symbol);
    int number_arguments = CurrentToken.function_details->number_arguments;
    std::vector<int> arguments;

    NextToken();
    IssueErrorOnTokenMismatch(TOKLPAREN, MGF::left_parenthesis_expected_in_function_call_14);

    std::optional<std::wstring> set_property_string_literal_argument;

    if( function_code == FunctionCode::PFFFN_SETPROPERTY_CODE && CheckNextTokenHelper() == NextTokenHelperResult::StringLiteral )
    {
        NextToken();
        set_property_string_literal_argument = Tokstr;
    }

    else
    {
        NextTokenWithPreference(SymbolType::Pre80Flow); // FLOW_TODO support pff.load()
    }

    // load can take a string argument or a form file name
    if( function_code == FunctionCode::PFFFN_LOAD_CODE && !IsCurrentTokenString() )
    {
        if( Tkn != TOKFLOW_PRE80 || NPT_Ref(Tokstindex).GetSubType() != SymbolSubType::Primary ) // FLOW_TODO support pff.load()
            IssueError(MGF::Pff_load_argument_invalid_47190);

        arguments.emplace_back(-1 * Tokstindex);

        NextToken();
    }

    // the other functions take string (or string list or dictionary) arguments;
    // setProperty's value can also be expressed as a numeric value
    else
    {
        for( int i = 0; i < number_arguments; ++i )
        {
            // getProperty's second argument is optional
            if( i == 1 && function_code == FunctionCode::PFFFN_GETPROPERTY_CODE && Tkn == TOKRPAREN )
            {
                arguments.emplace_back(-1);
                break;
            }

            bool is_string_list = false;

            if( i == 1 )
            {
                IssueErrorOnTokenMismatch(TOKCOMMA, MGF::function_call_comma_expected_528);

                NextTokenHelperResult next_token_helper_result = CheckNextTokenHelper();
                NextToken();

                is_string_list = ( next_token_helper_result == NextTokenHelperResult::List && IsCurrentTokenString() );
            }

            if( is_string_list )
            {
                if( function_code == FunctionCode::PFFFN_GETPROPERTY_CODE )
                {
                    const LogicList& logic_list = GetSymbolLogicList(Tokstindex);

                    if( logic_list.IsReadOnly() )
                        IssueError(MGF::List_read_only_cannot_be_modified_965, logic_list.GetName().c_str());

                    arguments.emplace_back(logic_list.GetSymbolIndex());
                }

                else
                {
                    ASSERT(function_code == FunctionCode::PFFFN_SETPROPERTY_CODE);
                    arguments.emplace_back(-1 * Tokstindex);
                }

                NextToken();
            }

            else if( i == 1 && function_code == FunctionCode::PFFFN_SETPROPERTY_CODE )
            {
                if( Tkn == TOKDICT || Tkn == TOKDICT_PRE80 )
                {
                    if( Tkn == TOKDICT )
                        VerifyDictionaryObject();

                    // check that this property name supports a dictionary argument
                    if( set_property_string_literal_argument.has_value() &&
                        !PffExecutor::IsValidEmbeddedProperty(*set_property_string_literal_argument) )
                    {
                        IssueError(MGF::Pff_property_invalid_with_dictionary_47194, set_property_string_literal_argument->c_str());
                    }

                    arguments.emplace_back(-1 * Tokstindex);
                    NextToken();
                }

                else
                {
                    DataType value_data_type = GetCurrentTokenDataType();
                    arguments.emplace_back(static_cast<int>(value_data_type));
                    arguments.emplace_back(CompileExpression(value_data_type));
                }
            }

            else
            {
                arguments.emplace_back(CompileStringExpression());
            }
        }
    }

    IssueErrorOnTokenMismatch(TOKRPAREN, MGF::right_parenthesis_expected_in_function_call_17);

    NextToken();

    ASSERT(( arguments.size() == static_cast<size_t>(number_arguments) ) ||
           ( function_code == FunctionCode::PFFFN_SETPROPERTY_CODE && arguments.size() == 3 ));

    return CreateSymbolVariableArgumentsNode(function_code, logic_pff, arguments);
}
