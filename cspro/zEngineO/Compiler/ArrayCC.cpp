#include "stdafx.h"
#include "IncludesCC.h"
#include "Array.h"
#include "PreinitializedVariable.h"
#include "ValueSet.h"
#include "WorkVariable.h"


LogicArray* LogicCompiler::CompileLogicArrayDeclarationOnly(bool use_function_parameter_syntax)
{
    ASSERT(Tkn == TOKKWARRAY);
    constexpr size_t MaximumArraySize = 128 * 1024 * 1024;

    bool is_numeric_array = true;
    int string_length = 0;

    // check if an old, unsupported type is specified
    {
        static const std::vector<const TCHAR*> DeprecatedTypes = { _T("SINT"), _T("LINT"), _T("FLOAT") };

        if( size_t deprecated_type_index = NextKeyword(DeprecatedTypes); deprecated_type_index != 0 )
            IssueError(MGF::deprecation_removed_Array_with_old_type_95019, DeprecatedTypes[deprecated_type_index - 1]);
    }

    // check if a type is specified
    if( IsNextToken({ TOKNUMERIC, TOKALPHA, TOKSTRING }) )
    {
        NextToken();

        if( Tkn != TOKNUMERIC )
        {
            is_numeric_array = false;

            if( Tkn == TOKALPHA )
                string_length = CompileAlphaLength();
        }
    }

    // get the array name
    std::wstring array_name = CompileNewSymbolName();

    // read in the dimensions and attributes
    std::vector<size_t> dimension_sizes;
    std::vector<int> dimension_symbols;
    bool save_array = false;

    NextToken();

    // function parameters either specify no dimensions or specify them like (,,)
    if( use_function_parameter_syntax )
    {
        dimension_sizes.emplace_back(SIZE_MAX);

        if( Tkn == TOKLPAREN )
        {
            while( true )
            {
                NextToken();

                if( Tkn == TOKRPAREN )
                {
                    break;
                }

                else if( Tkn == TOKCOMMA )
                {
                    dimension_sizes.emplace_back(SIZE_MAX);
                }

                else
                {
                    IssueError(MGF::right_parenthesis_expected_19);
                }
            }

            NextToken();
        }

        dimension_symbols = std::vector<int>(dimension_sizes.size(), 0);
    }

    else
    {
        IssueErrorOnTokenMismatch(TOKLPAREN, MGF::left_parenthesis_expected_in_element_reference_22);

        // a dimension can be a numeric constant, a preinitialized work variable, or a value set (for DeckArrays)
        while( true )
        {
            double dimension_size = 0;
            int dimension_symbol = 0;

            NextToken();

            if( Tkn == TOKCTE )
            {
                dimension_size = Tokvalue;
            }

            else if( Tkn == TOKVAR )
            {
                const Symbol& symbol = NPT_Ref(Tokstindex);

                if( symbol.IsA(SymbolType::WorkVariable) )
                {
                    const WorkVariable& work_variable = assert_cast<const WorkVariable&>(symbol);
                    dimension_size = work_variable.GetValue();
                }
            }

            else if( Tkn == TOKVALUESET )
            {
                const ValueSet& value_set = GetSymbolValueSet(Tokstindex);

                if( !is_numeric_array || value_set.IsDynamic() )
                    IssueError(MGF::DeckArray_only_numeric_arrays_47501);

                dimension_size = value_set.GetDictValueSet().GetNumValues();
                dimension_symbol = Tokstindex;

                if( dimension_size == 0 )
                    IssueError(MGF::DeckArray_value_set_size_invalid_47502);

                // DeckArrays can have a spillover row specified by (+)
                NextToken();

                if( Tkn == TOKLPAREN )
                {
                    NextToken();

                    if( Tkn == TOKADDOP )
                    {
                        NextToken();

                        if( Tkn == TOKRPAREN )
                        {
                            NextToken();

                            // indicate the spillover row by negating the symbol index
                            dimension_symbol *= -1;
                            ++dimension_size;
                        }
                    }

                    // if the symbol index isn't negated, then there was a problem with the above compilation
                    if( dimension_symbol > 0 )
                        IssueError(MGF::DeckArray_leftover_cell_error_47503);
                }
            }

            // only accept the dimension if it is a positive integer
            int dimension_int_size = static_cast<int>(dimension_size);

            if( dimension_int_size <= 0 || dimension_int_size != dimension_size )
                IssueError(MGF::integer_constant_expected_82);

            // because of the legacy of arrays being zero-indexed, the dimension will be 1
            // greater than the value specified
            dimension_sizes.emplace_back(static_cast<size_t>(dimension_int_size + 1));
            dimension_symbols.emplace_back(dimension_symbol);

            // the next token was already read with valuesets (to check for spillover rows)
            if( dimension_symbol == 0 )
                NextToken();

            if( Tkn == TOKRPAREN )
                break;

            IssueErrorOnTokenMismatch(TOKCOMMA, MGF::right_parenthesis_expected_in_element_reference_24);
        }

        if( dimension_sizes.empty() )
            IssueError(MGF::integer_constant_expected_82);

        // prohibit the user from creating ridiculously large arrays (but continue the compilation)
        size_t total_number_cells = 1;
        size_t total_number_cells_for_initialization = 1;

        for( size_t size : dimension_sizes )
        {
            total_number_cells *= size;
            total_number_cells_for_initialization *= ( size - 1 );
        }

        if( total_number_cells > MaximumArraySize )
            IssueError(MGF::Array_too_many_cells_8225, MaximumArraySize);

        NextToken();

        // save arrays are only allowed for global arrays
        if( Tkn == TOKSAVE )
        {
            if( !IsGlobalCompilation() )
                IssueError(MGF::SaveArray_only_allowed_in_proc_global_19004);

            save_array = true;

            NextToken();
        }
    }

    // add the array
    auto logic_array = std::make_shared<LogicArray>(std::move(array_name));

    logic_array->SetNumeric(is_numeric_array);
    logic_array->SetPaddingStringLength(string_length);
    logic_array->SetDimensions(std::move(dimension_sizes), std::move(dimension_symbols));

    if( save_array )
    {
        logic_array->SetUsingSaveArray();

        if( m_engineData->application != nullptr )
            m_engineData->application->SetHasSaveArrays();
    }

    m_engineData->AddSymbol(logic_array);

    return logic_array.get();
}


int LogicCompiler::CompileLogicArrayDeclaration()
{
    ASSERT(Tkn == TOKKWARRAY);
    Nodes::SymbolReset* symbol_reset_node = nullptr;

    LogicArray* logic_array = CompileLogicArrayDeclarationOnly(false);
    int& initialize_value = AddSymbolResetNode(symbol_reset_node, *logic_array);

    // allow the array to have some number of initial values, which can repeat if ... is found
    if( Tkn == TOKEQOP )
    {
        size_t total_number_cells_for_initialization = 1;

        for( size_t size : logic_array->GetDimensions() )
            total_number_cells_for_initialization *= ( size - 1 );

        std::vector<int> array_values;
        bool repeat_values = false;

        NextToken();

        while( true )
        {
            // read the value
            array_values.emplace_back(CompileSymbolInitialAssignment(*logic_array));

            if( Tkn == TOKSEMICOLON )
                break;

            if( Tkn == TOKCOMMA )
            {
                // check if they wany the value to repeat
                if( !repeat_values )
                {
                    MarkInputBufferToRestartLater();

                    int read_periods = 0;

                    for( ; read_periods < 3; ++read_periods )
                    {
                        if( NextToken().code != TOKPERIOD )
                            break;
                    }

                    if( read_periods == 3 )
                    {
                        ClearMarkedInputBuffer();
                        repeat_values = true;
                        NextToken();
                        break;
                    }

                    else if( read_periods != 0 )
                    {
                        IssueError(MGF::Array_repeating_cells_error_8222);
                    }

                    else
                    {
                        RestartFromMarkedInputBuffer();
                    }
                }

                NextToken();
            }
        }

        // check to see if the number of items entered matches the number expected
        if( array_values.size() > total_number_cells_for_initialization )
        {
            IssueWarning(MGF::Array_too_many_initialization_values_8214);
            array_values.resize(total_number_cells_for_initialization);
        }

        else if( array_values.size() < total_number_cells_for_initialization )
        {
            if( repeat_values )
            {
                if( ( total_number_cells_for_initialization % array_values.size() ) != 0 )
                    IssueWarning(MGF::Array_repeating_cells_not_evently_repeated_8224);
            }

            else
            {
                IssueWarning(MGF::Array_too_few_initialization_values_8213);
            }
        }

        // if in PROC GLOBAL, add to the preinitialized variables
        if( IsGlobalCompilation() )
        {
            m_engineData->runtime_events_processor.AddEvent(std::make_unique<PreinitializedVariable>(
                *m_engineData,
                *logic_array,
                std::move(array_values),
                repeat_values ? PreinitializedVariable::SpecialProcessing::RepeatingArray :
                                PreinitializedVariable::SpecialProcessing::None));
        }

        // in other places, add it as the symbol reset node's initialize value
        else
        {
            // add repeat_value to the values
            array_values.insert(array_values.begin(), repeat_values ? 1 : 0);
            initialize_value = CreateListNode(array_values);
        }
    }

    // only one array declaration is allowed per statement
    IssueErrorOnTokenMismatch(TOKSEMICOLON, MGF::expecting_semicolon_30);

    return GetOptionalProgramIndex(symbol_reset_node);
}


int LogicCompiler::CompileLogicArrayReference()
{
    const LogicArray& logic_array = GetSymbolLogicArray(Tokstindex);

    NextToken();
    IssueErrorOnTokenMismatch(TOKLPAREN, MGF::left_parenthesis_expected_in_element_reference_22);

    // read in each index
    std::vector<size_t> index_expressions;

    while( true )
    {
        NextTokenHelperResult next_token_helper_result = CheckNextTokenHelper();
        int index_expression = 0;
        size_t max_dimension_size = ( index_expressions.size() < logic_array.GetNumberDimensions() ) ?
            ( logic_array.GetDimension(index_expressions.size()) - 1 ) : SIZE_MAX;
        bool index_out_of_range = false;

        NextToken();

        // check numeric constants to make sure that they are in the proper range
        if( next_token_helper_result == NextTokenHelperResult::NumericConstantNonNegative )
        {
            if( Tokvalue < 0 || Tokvalue > max_dimension_size )
            {
                index_out_of_range = true;
            }

            else
            {
                index_expression = CreateNumericConstantNode(Tokvalue);
                NextToken();
            }
        }

        // disallow negative or special value indices
        else if( Tkn == TOKMINUS || Tkn == TOKDEFAULT || Tkn == TOKNOTAPPL || Tkn == TOKMISSING || Tkn == TOKREFUSED )
        {
            index_out_of_range = true;
        }

        // otherwise compile the number
        else
        {
            index_expression = exprlog();
        }

        if( index_out_of_range )
            IssueError(MGF::Array_invalid_subscript_608, static_cast<int>(max_dimension_size));

        index_expressions.emplace_back(index_expression);

        // commas will separate each index
        if( Tkn == TOKRPAREN )
            break;

        if( Tkn != TOKCOMMA )
        {
            // clearing the index expressions will trigger an error below
            index_expressions.clear();
            break;
        }
    }

    if( index_expressions.size() != logic_array.GetNumberDimensions() )
        IssueError(MGF::Array_subscript_count_mismatch_609, static_cast<int>(logic_array.GetNumberDimensions()));

    IssueErrorOnTokenMismatch(TOKRPAREN, MGF::right_parenthesis_expected_in_element_reference_24);

    NextToken();

    auto& element_reference_node = CreateVariableSizeNode<Nodes::ElementReference>(FunctionCode::ARRAYVAR_CODE, index_expressions.size());

    element_reference_node.symbol_index = logic_array.GetSymbolIndex();
    memcpy(element_reference_node.element_expressions, index_expressions.data(), index_expressions.size() * sizeof(index_expressions[0]));

    return GetProgramIndex(element_reference_node);
}


int LogicCompiler::CompileLogicArrayFunctions()
{
    // compiling array_name.clear()
    //           array_name.length()
    FunctionCode function_code = CurrentToken.function_details->code;
    const LogicArray* logic_array = assert_cast<const LogicArray*>(CurrentToken.symbol);
    int number_arguments = ( function_code == FunctionCode::ARRAYFN_LENGTH_CODE ) ? 1 : 0;
    Nodes::SymbolVariableArguments& symbol_va_node = CreateSymbolVariableArgumentsNode(function_code, *logic_array, number_arguments);

    NextToken();
    IssueErrorOnTokenMismatch(TOKLPAREN, MGF::left_parenthesis_expected_in_function_call_14);

    NextToken();

    if( function_code == FunctionCode::ARRAYFN_LENGTH_CODE )
    {
        int dimension_expression = 0;

        // if more than one dimension, they must specify the dimension
        if( Tkn == TOKRPAREN )
        {
            if( logic_array->GetNumberDimensions() > 1  )
                IssueError(MGF::Array_specify_dimension_8226);

            // if not specified, default to the first dimension
            dimension_expression = CreateNumericConstantNode(1);
        }

        else
        {
            dimension_expression = exprlog();
        }

        symbol_va_node.arguments[0] = dimension_expression;
    }

    IssueErrorOnTokenMismatch(TOKRPAREN, MGF::right_parenthesis_expected_in_function_call_17);

    NextToken();

    return GetProgramIndex(symbol_va_node);
}
