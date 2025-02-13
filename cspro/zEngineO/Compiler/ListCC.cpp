#include "stdafx.h"
#include "IncludesCC.h"
#include "List.h"
#include "PreinitializedVariable.h"


LogicList* LogicCompiler::CompileLogicListDeclaration(const LogicList* list_to_copy_attributes/* = nullptr*/)
{
    bool is_numeric_list;

    // for the first list, read the type
    if( list_to_copy_attributes == nullptr )
    {
        if( NextKeywordIf(TOKSTRING) )
        {
            is_numeric_list = false;
        }

        else
        {
            // read the optional numeric token if present
            NextKeywordIf(TOKNUMERIC);
            is_numeric_list = true;
        }
    }

    // otherwise copy the previous type
    else
    {
        is_numeric_list = list_to_copy_attributes->IsNumeric();
    }


    std::wstring list_name = CompileNewSymbolName();

    auto logic_list = std::make_shared<LogicList>(std::move(list_name));
    logic_list->SetNumeric(is_numeric_list);

    m_engineData->AddSymbol(logic_list);

    return logic_list.get();
}


int LogicCompiler::CompileLogicListDeclarations()
{
    ASSERT(Tkn == TOKKWLIST);
    Nodes::SymbolReset* symbol_reset_node = nullptr;
    const LogicList* last_list_compiled = nullptr;

    do
    {
        last_list_compiled = CompileLogicListDeclaration(last_list_compiled);

        int& initialize_value = AddSymbolResetNode(symbol_reset_node, *last_list_compiled);

        NextToken();

        if( Tkn == TOKEQOP )
            initialize_value = CompileLogicListComputeInstruction(last_list_compiled);

    } while( Tkn == TOKCOMMA );

    IssueErrorOnTokenMismatch(TOKSEMICOLON, MGF::expecting_semicolon_30);

    return GetOptionalProgramIndex(symbol_reset_node);
}


int LogicCompiler::CompileLogicListComputeInstruction(const LogicList* list_from_declaration/* = nullptr*/)
{
    // compiling list_name(index) = value;
    //           list_name = rhs_list_name;
    LogicList* logic_list = const_cast<LogicList*>(list_from_declaration);
    bool assigning_to_element = false;

    if( list_from_declaration == nullptr )
    {
        ASSERT(Tkn == TOKLIST);
        logic_list = &GetSymbolLogicList(Tokstindex);

        if( logic_list->IsReadOnly() )
            IssueError(MGF::List_read_only_cannot_be_modified_965, logic_list->GetName().c_str());

        if( IsNextToken(TOKLPAREN) )
        {
            assigning_to_element = true;
        }

        else
        {
            NextToken();
        }
    }


    int program_index = -1;

    auto create_symbol_compute_node = [&](int lhs_symbol_index, SymbolType rhs_symbol_type, int rhs_symbol_index)
    {
        auto& symbol_compute_node = CreateNode<Nodes::SymbolCompute>(FunctionCode::LISTFN_COMPUTE_CODE);

        symbol_compute_node.next_st = -1;
        symbol_compute_node.lhs_symbol_index = lhs_symbol_index;
        symbol_compute_node.rhs_symbol_type = rhs_symbol_type;
        symbol_compute_node.rhs_symbol_index = rhs_symbol_index;

        program_index = GetProgramIndex(symbol_compute_node);
    };


    // assigning something to the entire list
    if( Tkn == TOKEQOP )
    {
        NextTokenHelperResult next_token_helper_result = CheckNextTokenHelper();
        NextToken();

        // assigning a list to a list
        if( next_token_helper_result == NextTokenHelperResult::List )
        {
            // assignment of a list is not allowed in PROC GLOBAL
            if( IsGlobalCompilation() )
                IssueError(MGF::symbol_assignment_not_allowed_in_proc_global_687, ToString(SymbolType::List));

            if( logic_list->GetDataType() != GetSymbolLogicList(Tokstindex).GetDataType() )
                IssueError(MGF::List_mismatched_types_962, ToString(logic_list->GetDataType()), logic_list->GetName().c_str());

            create_symbol_compute_node(logic_list->GetSymbolIndex(), SymbolType::List, Tokstindex);

            NextToken();
        }

        // assigning values to a list
        else
        {
            std::vector<int> list_values;

            while( true )
            {
                list_values.emplace_back(CompileSymbolInitialAssignment(*logic_list));

                if( Tkn == TOKSEMICOLON )
                    break;

                IssueErrorOnTokenMismatch(TOKCOMMA, MGF::function_call_comma_expected_528);

                NextToken();
            }

            // if in PROC GLOBAL, add to the preinitialized variables
            if( IsGlobalCompilation() )
            {
                m_engineData->runtime_events_processor.AddEvent(std::make_unique<PreinitializedVariable>(
                    *m_engineData,
                    *logic_list,
                    std::move(list_values)));
            }

            // in other places, add it as the symbol reset node's initialize value
            else
            {
                create_symbol_compute_node(logic_list->GetSymbolIndex(), SymbolType::Variable, CreateListNode(list_values));
            }
        }
    }

    // assigning a value to an element of the list
    else if( assigning_to_element )
    {
        int list_expression = CompileLogicListReference(logic_list);

        IssueErrorOnTokenMismatch(TOKEQOP, MGF::equals_expected_in_assignment_5);

        NextToken();
        int rhs_expression = CompileExpression(logic_list->GetDataType());

        create_symbol_compute_node(list_expression, SymbolType::None, rhs_expression);
    }

    if( list_from_declaration == nullptr )
        IssueErrorOnTokenMismatch(TOKSEMICOLON, MGF::expecting_semicolon_30);

    return program_index;
}


int LogicCompiler::CompileLogicListReference(const LogicList* logic_list/* = nullptr*/)
{
    if( logic_list == nullptr )
        logic_list = &GetSymbolLogicList(Tokstindex);

    NextToken();
    IssueErrorOnTokenMismatch(TOKLPAREN, MGF::left_parenthesis_expected_in_element_reference_22);

    NextToken();

    int subscript_expression = exprlog();

    IssueErrorOnTokenMismatch(TOKRPAREN, MGF::right_parenthesis_expected_in_element_reference_24);

    NextToken();

    auto& element_reference_node = CreateNode<Nodes::ElementReference>(FunctionCode::LISTVAR_CODE);

    element_reference_node.symbol_index = logic_list->GetSymbolIndex();
    element_reference_node.element_expressions[0] = subscript_expression;

    return GetProgramIndex(element_reference_node);
}


int LogicCompiler::CompileLogicListFunctions()
{
    // compiling list_name.add(item | list)
    //           list_name.clear()
    //           list_name.insert(index, item | list)
    //           list_name.length()
    //           list_name.remove(index)
    //           list_name.removeDuplicates()
    //           list_name.removeIn(in_list)
    //           list_name.seek(value)
    //           list_name.show([heading])
    //           list_name.sort([ascending | descending])

    FunctionCode function_code = CurrentToken.function_details->code;
    const LogicList& logic_list = assert_cast<const LogicList&>(*CurrentToken.symbol);
    std::vector<int> arguments;

    if( logic_list.IsReadOnly() && function_code != FunctionCode::LISTFN_LENGTH_CODE &&
                                   function_code != FunctionCode::LISTFN_SEEK_CODE &&
                                   function_code != FunctionCode::LISTFN_SHOW_CODE )
    {
        IssueError(MGF::List_read_only_cannot_be_modified_965, logic_list.GetName().c_str());
    }

    NextToken();
    IssueErrorOnTokenMismatch(TOKLPAREN, MGF::left_parenthesis_expected_in_function_call_14);

    // clear, length, and removeDuplicates have no arguments
    if( function_code == FunctionCode::LISTFN_CLEAR_CODE ||
        function_code == FunctionCode::LISTFN_LENGTH_CODE ||
        function_code == FunctionCode::LISTFN_REMOVEDUPLICATES_CODE )
    {
        NextToken();
    }

    // remove and insert expect an index
    if( function_code == FunctionCode::LISTFN_REMOVE_CODE ||
        function_code == FunctionCode::LISTFN_INSERT_CODE )
    {
        NextToken();

        int index_expression = exprlog();
        arguments.emplace_back(index_expression);

        if( function_code == FunctionCode::LISTFN_INSERT_CODE )
            IssueErrorOnTokenMismatch(TOKCOMMA, MGF::function_call_comma_expected_528);
    }

    // add and insert expect either an item or a list
    if( function_code == FunctionCode::LISTFN_ADD_CODE ||
        function_code == FunctionCode::LISTFN_INSERT_CODE )
    {
        NextTokenHelperResult next_token_helper_result = CheckNextTokenHelper();
        int object_type = 0;
        int object_expression = 0;

        NextToken();

        if( next_token_helper_result == NextTokenHelperResult::List )
        {
            ASSERT(Tkn == TOKLIST);
            object_type = 1; // list
            object_expression = Tokstindex;

            const LogicList& logic_list_object = GetSymbolLogicList(object_expression);

            // you must add or insert a list of the same type
            if( logic_list_object.IsNumeric() != logic_list.IsNumeric() )
                IssueError(MGF::List_mismatched_types_962, ToString(logic_list.GetDataType()), logic_list.GetName().c_str());

            // you cannot add or insert a list into itself
            if( &logic_list_object == &logic_list )
                IssueError(MGF::List_cannot_assign_to_itself_963, logic_list.GetName().c_str());

            NextToken();
        }

        else
        {
            object_expression = CompileExpression(logic_list.GetDataType());
        }

        arguments.emplace_back(object_type);
        arguments.emplace_back(object_expression);
    }

    // seek expects a value
    if( function_code == FunctionCode::LISTFN_SEEK_CODE )
    {
        NextToken();

        int value_expression = CompileExpression(logic_list.GetDataType());
        int index_expression = -1;
        int nth_expression = -1;

        if( Tkn == TOKCOMMA )
        {
            NextToken();

            int* expression_used = &index_expression;

            if( Tkn == TOKATOP )
            {
                NextToken();
                expression_used = &nth_expression;
            }

            *expression_used = exprlog();
        }

        arguments.emplace_back(value_expression);
        arguments.emplace_back(index_expression);
        arguments.emplace_back(nth_expression);
    }

    // show expects an optional title
    if( function_code == FunctionCode::LISTFN_SHOW_CODE )
    {
        NextToken();

        int title_expression = -1;

        if( Tkn != TOKRPAREN )
            title_expression = CompileStringExpression();

        arguments.emplace_back(title_expression);
    }

    // sort can specify the sort type
    if( function_code == FunctionCode::LISTFN_SORT_CODE )
    {
        NextToken();

        bool ascending = true;

        if( bool specified_ascending = ( Tkn == TOKASCENDING ); specified_ascending || Tkn == TOKDESCENDING )
        {
            ascending = specified_ascending;
            NextToken();
        }

        arguments.emplace_back(ascending ? 0 : 1);
    }

    // removeIn expects an in list
    if( function_code == FunctionCode::LISTFN_REMOVEIN_CODE )
    {
        NextToken();

        int in_node_expression = CompileInNodes(logic_list.GetDataType());

        arguments.emplace_back(in_node_expression);
    }

    IssueErrorOnTokenMismatch(TOKRPAREN, MGF::right_parenthesis_expected_in_function_call_17);

    NextToken();

    return CreateSymbolVariableArgumentsNode(function_code, logic_list, arguments);
}
