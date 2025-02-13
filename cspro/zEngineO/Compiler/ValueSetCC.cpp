#include "stdafx.h"
#include "IncludesCC.h"
#include "ValueSet.h"
#include <engine/VarT.h>


DynamicValueSet* LogicCompiler::CompileDynamicValueSetDeclaration(const DynamicValueSet* value_set_to_copy_attributes/* = nullptr*/)
{
    bool is_numeric_value_set;

    // for the first value set, read the type
    if( value_set_to_copy_attributes == nullptr )
    {
        if( NextKeywordIf(TOKSTRING) )
        {
            is_numeric_value_set = false;
        }

        else
        {
            // read the optional numeric token if present
            NextKeywordIf(TOKNUMERIC);
            is_numeric_value_set = true;
        }
    }

    else
    {
        is_numeric_value_set = value_set_to_copy_attributes->IsNumeric();
    }

    std::wstring value_set_name = CompileNewSymbolName();

    auto value_set = std::make_shared<DynamicValueSet>(std::move(value_set_name), *m_engineData);
    value_set->SetNumeric(is_numeric_value_set);

    m_engineData->AddSymbol(value_set);

    return value_set.get();
}


int LogicCompiler::CompileDynamicValueSetDeclarations()
{
    ASSERT(Tkn == TOKKWVALUESET);
    Nodes::SymbolReset* symbol_reset_node = nullptr;
    const DynamicValueSet* last_value_set_compiled = nullptr;

    do
    {
        last_value_set_compiled = CompileDynamicValueSetDeclaration(last_value_set_compiled);

        int& initialize_value = AddSymbolResetNode(symbol_reset_node, *last_value_set_compiled);

        NextToken();

        if( Tkn == TOKEQOP )
        {
            // assignment not allowed in PROC GLOBAL
            if( IsGlobalCompilation() )
                IssueError(MGF::symbol_assignment_not_allowed_in_proc_global_687, ToString(SymbolType::ValueSet));

            initialize_value = CompileDynamicValueSetComputeInstruction(last_value_set_compiled);
        }

    } while( Tkn == TOKCOMMA );

    IssueErrorOnTokenMismatch(TOKSEMICOLON, MGF::expecting_semicolon_30);

    return GetOptionalProgramIndex(symbol_reset_node);
}


int LogicCompiler::CompileDynamicValueSetComputeInstruction(const DynamicValueSet* value_set_from_declaration/* = nullptr*/)
{
    // compiling valueset_name = rhs_list_name;
    const ValueSet* value_set = value_set_from_declaration;

    if( value_set == nullptr )
    {
        ASSERT(Tkn == TOKVALUESET);
        value_set = &GetSymbolValueSet(Tokstindex);

        NextToken();
    }

    IssueErrorOnTokenMismatch(TOKEQOP, MGF::equals_expected_in_assignment_5);

    if( !value_set->IsDynamic() )
        IssueError(MGF::ValueSet_invalid_operation_for_dict_value_set_47170, _T("="), value_set->GetName().c_str());

    NextToken();

    if( Tkn != TOKVALUESET || value_set->GetDataType() != GetSymbolValueSet(Tokstindex).GetDataType() )
        IssueError(MGF::ValueSet_add_not_correct_data_type_47171, ToString(value_set->GetDataType()), value_set->GetName().c_str());

    auto& symbol_compute_node = CreateNode<Nodes::SymbolCompute>(FunctionCode::VALUESETFN_COMPUTE_CODE);

    symbol_compute_node.next_st = -1;
    symbol_compute_node.lhs_symbol_index = value_set->GetSymbolIndex();
    symbol_compute_node.rhs_symbol_type = SymbolType::ValueSet;
    symbol_compute_node.rhs_symbol_index = Tokstindex;

    NextToken();

    if( value_set_from_declaration == nullptr )
        IssueErrorOnTokenMismatch(TOKSEMICOLON, MGF::expecting_semicolon_30);

    return GetProgramIndex(symbol_compute_node);
}


int LogicCompiler::CompileValueSetFunctions()
{
    // compiling valueset_name.add(valueset_name[, from_code][, to_code])
    //           valueset_name.add(label, from_code[, to_code | special_code][, image := image_filename][, textColor := html_color_or_hex_code])"),
    //           valueset_name.clear()
    //           valueset_name.length()
    //           valueset_name.remove(code)
    //           valueset_name.show([heading])
    //           valueset_name.sort([ascending | descending] [by code | label])
    FunctionCode function_code = CurrentToken.function_details->code;
    const ValueSet& value_set = assert_cast<const ValueSet&>(*CurrentToken.symbol);
    int number_arguments = CurrentToken.function_details->number_arguments;

    Nodes::SymbolVariableArguments& symbol_va_node = CreateSymbolVariableArgumentsNode(function_code, value_set, number_arguments, -1);

    // some of the functions only apply to dynamic value sets
    if( !value_set.IsDynamic() && ( function_code == FunctionCode::VALUESETFN_ADD_CODE    ||
                                    function_code == FunctionCode::VALUESETFN_CLEAR_CODE  ||
                                    function_code == FunctionCode::VALUESETFN_REMOVE_CODE ) )
    {
        IssueError(MGF::ValueSet_invalid_operation_for_dict_value_set_47170, CurrentToken.function_details->name, value_set.GetName().c_str());
    }

    NextToken();
    IssueErrorOnTokenMismatch(TOKLPAREN, MGF::left_parenthesis_expected_in_function_call_14);

    // some functions have no arguments
    if( number_arguments == 0 )
    {
        NextToken();
    }


    // add
    else if( function_code == FunctionCode::VALUESETFN_ADD_CODE )
    {
        int& valueset_symbol_index = symbol_va_node.arguments[0];
        int& label_expression = symbol_va_node.arguments[1];
        int& image_filename_expression = symbol_va_node.arguments[2];
        int& from_code_expression = symbol_va_node.arguments[3];
        int& to_code_expression = symbol_va_node.arguments[4];
        int& text_color_expression = symbol_va_node.arguments[5];

        NextToken();

        // add a complete value set or a value from a value set
        if( Tkn == TOKVALUESET )
        {
            const ValueSet& rhs_value_set = assert_cast<const ValueSet&>(*CurrentToken.symbol);

            // check that the type is correct
            if( rhs_value_set.GetDataType() != value_set.GetDataType() )
                IssueError(MGF::ValueSet_add_not_correct_data_type_47171, ToString(value_set.GetDataType()), value_set.GetName().c_str());

            if( &rhs_value_set == &value_set )
                IssueError(MGF::ValueSet_add_cannot_add_self_47172, value_set.GetName().c_str());

            valueset_symbol_index = rhs_value_set.GetSymbolIndex();

            NextToken();

            // instead of the complete value set, a specific value can be added
            if( Tkn == TOKCOMMA )
            {
                NextToken();
                from_code_expression = CompileExpression(value_set.GetDataType());

                // for numeric value sets, a range can be specified
                if( Tkn == TOKCOMMA && value_set.IsNumeric() )
                {
                    NextToken();
                    to_code_expression = exprlog();
                }
            }
        }

        // or add a single value
        else
        {
            // this is not the ideal use of this class--though it's the first use of this class--because, to support
            // the initial way of defining the optional image filename, we have to put compilation calls in
            // more than one location (rather than just after the required arguments)
            OptionalNamedArgumentsCompiler optional_named_arguments_compiler(*this);

            optional_named_arguments_compiler.AddArgument(_T("image"), image_filename_expression, DataType::String);
            optional_named_arguments_compiler.AddArgumentPortableColorText(_T("textColor"), text_color_expression);

            // add the label
            label_expression = CompileStringExpression();

            IssueErrorOnTokenMismatch(TOKCOMMA, MGF::function_call_comma_expected_528);

            // potentially add an image filename
            bool image_filename_specified_using_deprecated_way = false;

            NextToken();

            if( value_set.IsString() || IsCurrentTokenString() )
            {
                image_filename_expression = CompileStringExpression();
                image_filename_specified_using_deprecated_way = true;

                if( value_set.IsNumeric() )
                {
                    IssueErrorOnTokenMismatch(TOKCOMMA, MGF::function_call_comma_expected_528);
                    NextToken();
                }
            }

            // add the string from code, or use the image filename expression as the string from code
            if( value_set.IsString() )
            {
                if( Tkn == TOKRPAREN )
                {
                    from_code_expression = image_filename_expression;
                    image_filename_specified_using_deprecated_way = false;
                    image_filename_expression = -1;
                }

                else
                {
                    IssueErrorOnTokenMismatch(TOKCOMMA, MGF::function_call_comma_expected_528);

                    int saved_image_filename_expression = image_filename_expression;
                    image_filename_expression = -1;

                    if( optional_named_arguments_compiler.Compile() > 0 )
                    {
                        from_code_expression = saved_image_filename_expression;
                        image_filename_specified_using_deprecated_way = false;
                    }

                    else
                    {
                        image_filename_expression = saved_image_filename_expression;

                        NextToken();
                        from_code_expression = CompileStringExpression();
                    }
                }
            }

            // add the numeric from code and then potentially a to code
            else
            {
                from_code_expression = exprlog();

                if( Tkn != TOKRPAREN )
                {
                    IssueErrorOnTokenMismatch(TOKCOMMA, MGF::function_call_comma_expected_528);

                    if( optional_named_arguments_compiler.Compile() == 0 )
                    {
                        NextToken();
                        to_code_expression = exprlog();
                    }
                }
            }

            if( image_filename_specified_using_deprecated_way )
                IssueWarning(Logic::ParserMessage::Type::DeprecationMajor, MGF::deprecation_ValueSet_image_filename_95028);

            optional_named_arguments_compiler.Compile();
        }
    }


    // remove
    else if( function_code == FunctionCode::VALUESETFN_REMOVE_CODE )
    {
        // add the code
        NextToken();
        symbol_va_node.arguments[0] = CompileExpression(value_set.GetDataType());
    }


    // show
    else if( function_code == FunctionCode::VALUESETFN_SHOW_CODE )
    {
        // add the optional heading
        NextToken();

        if( Tkn != TOKRPAREN )
            symbol_va_node.arguments[0] = CompileStringExpression();
    }

    // sort
    else if( function_code == FunctionCode::VALUESETFN_SORT_CODE )
    {
        std::optional<bool> ascending;
        std::optional<bool> sort_by_label;

        NextToken();

        while( Tkn != TOKRPAREN )
        {
            if( bool specified_ascending = ( Tkn == TOKASCENDING ); ( specified_ascending || Tkn == TOKDESCENDING ) && !ascending.has_value() )
            {
                ascending = specified_ascending;
            }

            else if( Tkn == TOKBY && !sort_by_label.has_value() )
            {
                size_t by_type = NextKeywordOrError({ _T("code"), _T("label") });
                sort_by_label = ( by_type == 2 );
            }

            else
            {
                IssueError(MGF::right_parenthesis_expected_in_function_call_17);
            }

            NextToken();
        }

        symbol_va_node.arguments[0] = ascending.value_or(true) ? 0 : 1;
        symbol_va_node.arguments[1] = sort_by_label.value_or(true) ? 0 : 1;
    }


    IssueErrorOnTokenMismatch(TOKRPAREN, MGF::right_parenthesis_expected_in_function_call_17);

    NextToken();

    return GetProgramIndex(symbol_va_node);
}


int LogicCompiler::CompileSetValueSetFunction()
{
    int item_symbol_index;
    int value_set_symbol_index;
    std::optional<DataType> value_data_type;

    NextToken();
    IssueErrorOnTokenMismatch(TOKLPAREN, MGF::left_parenthesis_expected_in_function_call_14);

    NextToken();

    // @ specified before a string allows the specification of the item at runtime
    if( Tkn == TOKATOP )
    {
        NextToken();

        if( Tkn == TOKSCTE )
            IssueWarning(Logic::ParserMessage::Type::DeprecationMajor, MGF::deprecation_setvalueset_with_at_string_literal_95032);

        item_symbol_index = -1 * CompileStringExpression();
    }

    else
    {
        IssueErrorOnTokenMismatch(TOKVAR, MGF::item_expected_33109);

        item_symbol_index = Tokstindex;
        value_data_type = VPT(item_symbol_index)->GetDataType();

        NextToken();
    }

    IssueErrorOnTokenMismatch(TOKCOMMA, MGF::function_call_comma_expected_528);

    NextToken();

    // creating value sets with arrays is no longer supported
    if( Tkn == TOKARRAY )
        IssueError(MGF::deprecation_setvalueset_with_Array_95020);

    // the value set name can be specified as a string
    if( IsCurrentTokenString() )
    {
        value_set_symbol_index = -1 * CompileStringExpression();
    }

    else if( Tkn == TOKVALUESET )
    {
        value_set_symbol_index = Tokstindex;

        if( value_data_type.has_value() && *value_data_type != GetSymbolValueSet(value_set_symbol_index).GetDataType() )
            IssueError(MGF::ValueSet_not_correct_data_type_941, ToString(*value_data_type));

        NextToken();
    }

    else if( value_data_type.has_value() )
    {
        IssueError(MGF::ValueSet_not_correct_data_type_941, ToString(*value_data_type));
    }

    else
    {
        IssueError(MGF::object_of_type_expected_33116, ToString(SymbolType::ValueSet));
    }

    IssueErrorOnTokenMismatch(TOKRPAREN, MGF::right_parenthesis_expected_in_function_call_17);

    NextToken();

    return CreateVariableArgumentsNode(FunctionCode::FNSETVALUESET_CODE, { item_symbol_index, value_set_symbol_index });
}
