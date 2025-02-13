#include "stdafx.h"
#include "IncludesCC.h"
#include "List.h"
#include "HashMap.h"


LogicHashMap* LogicCompiler::CompileLogicHashMapDeclaration(const LogicHashMap* hashmap_to_copy_attributes/* = nullptr*/)
{
    DataType value_type;

    // for the first hashmap, read the value type
    if( hashmap_to_copy_attributes == nullptr )
    {
        if( NextKeywordIf(TOKSTRING) )
        {
            value_type = DataType::String;
        }

        else
        {
            // read the optional numeric token if present
            NextKeywordIf(TOKNUMERIC);
            value_type = DataType::Numeric;
        }
    }

    // otherwise copy the previous type
    else
    {
        value_type = hashmap_to_copy_attributes->GetValueType();
    }

    std::wstring hashmap_name = CompileNewSymbolName();

    auto hashmap = std::make_shared<LogicHashMap>(std::move(hashmap_name));

    // read the optional dimension types
    std::vector<std::optional<DataType>> dimension_types;

    if( NextKeywordIf(TOKLPAREN) )
    {
        while( true )
        {
            NextToken();

            if( Tkn == TOKNUMERIC )
            {
                dimension_types.emplace_back(DataType::Numeric);
            }

            else if( Tkn == TOKSTRING )
            {
                dimension_types.emplace_back(DataType::String);
            }

            else if( Tkn == TOKALL )
            {
                dimension_types.emplace_back();
            }

            else
            {
                IssueError(MGF::HashMap_invalid_dimension_47232);
            }

            NextToken();

            if( Tkn == TOKRPAREN )
            {
                break;
            }

            else if( Tkn != TOKCOMMA )
            {
                IssueError(MGF::right_parenthesis_expected_19);
            }
        }
    }

    // if no dimensions are specified, default to a one-dimensional map
    else
    {
        dimension_types.emplace_back();
    }

    ASSERT(!dimension_types.empty());

    hashmap->SetValueType(value_type);
    hashmap->SetDimensionTypes(std::move(dimension_types));

    m_engineData->AddSymbol(hashmap);

    return hashmap.get();
}


int LogicCompiler::CompileLogicHashMapDeclarations()
{
    ASSERT(Tkn == TOKKWHASHMAP);
    Nodes::SymbolReset* symbol_reset_node = nullptr;
    LogicHashMap* last_hashmap_compiled = nullptr;

    do
    {
        last_hashmap_compiled = CompileLogicHashMapDeclaration(last_hashmap_compiled);

        int& initialize_value = AddSymbolResetNode(symbol_reset_node, *last_hashmap_compiled);

        NextToken();

        // allow the specification of a default value
        if( Tkn == TOKCTE && SO::EqualsNoCase(Tokstr, _T("DEFAULT")) )
        {
            NextToken();

            if( Tkn == TOKLPAREN )
            {
                NextToken();

                bool negative = ( Tkn == TOKMINUS );

                if( negative )
                    NextToken();

                if( Tkn == TOKCTE && last_hashmap_compiled->IsValueTypeNumeric() )
                {
                    last_hashmap_compiled->SetDefaultValue(( negative ? -1 : 1 ) * Tokvalue);
                }

                else if( Tkn == TOKSCTE && last_hashmap_compiled->IsValueTypeString() )
                {
                    last_hashmap_compiled->SetDefaultValue(Tokstr);
                }

                if( last_hashmap_compiled->HasDefaultValue() )
                    NextToken();
            }

            if( !last_hashmap_compiled->HasDefaultValue() || Tkn != TOKRPAREN )
            {
                IssueError(MGF::HashMap_invalid_default_value_47236, last_hashmap_compiled->GetName().c_str(),
                                                                     ToString(last_hashmap_compiled->GetValueType()));
            }

            NextToken();
        }

        // allow assignments as part of the declaration
        if( Tkn == TOKEQOP )
            initialize_value = CompileLogicHashMapComputeInstruction(last_hashmap_compiled);

    } while( Tkn == TOKCOMMA );

    IssueErrorOnTokenMismatch(TOKSEMICOLON, MGF::expecting_semicolon_30);

    return GetOptionalProgramIndex(symbol_reset_node);
}


int LogicCompiler::CompileLogicHashMapReference(const LogicHashMap* hashmap/* = nullptr*/)
{
    if( hashmap == nullptr )
    {
        hashmap = &GetSymbolLogicHashMap(Tokstindex);
        NextToken();
    }

    IssueErrorOnTokenMismatch(TOKLPAREN, MGF::left_parenthesis_expected_in_element_reference_22);

    NextToken();

    std::vector<int> arguments;

    for( size_t i = 0; i < hashmap->GetNumberDimensions(); ++i )
    {
        DataType value_data_type = GetCurrentTokenDataType();

        if( !hashmap->DimensionTypeHandles(i, value_data_type) )
        {
            IssueError(MGF::HashMap_invalid_value_for_dimension_47235, hashmap->GetName().c_str(),
                                                                       ToString(value_data_type),
                                                                       static_cast<int>(i) + 1);
        }

        arguments.emplace_back(static_cast<int>(value_data_type));
        arguments.emplace_back(CompileExpression(value_data_type));

        if( ( i + 1 ) < hashmap->GetNumberDimensions() )
        {
            IssueErrorOnTokenMismatch(TOKCOMMA, MGF::right_parenthesis_expected_in_element_reference_24);
            NextToken();
        }
    }

    IssueErrorOnTokenMismatch(TOKRPAREN, MGF::right_parenthesis_expected_in_element_reference_24);

    NextToken();

    auto& element_reference_node = CreateNode<Nodes::ElementReference>(FunctionCode::HASHMAPVAR_CODE);

    element_reference_node.symbol_index = hashmap->GetSymbolIndex();
    element_reference_node.element_expressions[0] = CreateListNode(arguments);

    return GetProgramIndex(element_reference_node);
}


int LogicCompiler::CompileLogicHashMapComputeInstruction(const LogicHashMap* hashmap_from_declaration/* = nullptr*/)
{
    // compiling hashmap_name(key_expression[, key_expression, ...]) = value
    //           hashmap_name = rhs_hashmap_name;
    const LogicHashMap* lhs_hashmap = hashmap_from_declaration;
    bool assigning_to_element = false;

    if( IsGlobalCompilation() )
        IssueError(MGF::symbol_assignment_not_allowed_in_proc_global_687, ToString(SymbolType::HashMap));

    if( lhs_hashmap == nullptr )
    {
        ASSERT(Tkn == TOKHASHMAP);
        lhs_hashmap = &GetSymbolLogicHashMap(Tokstindex);

        NextToken();

        if( Tkn == TOKLPAREN )
        {
            assigning_to_element = true;
        }

        else
        {
            IssueErrorOnTokenMismatch(TOKEQOP, MGF::equals_expected_in_assignment_5);
        }
    }

    auto& symbol_compute_node = CreateNode<Nodes::SymbolCompute>(FunctionCode::HASHMAPFN_COMPUTE_CODE);

    symbol_compute_node.next_st = -1;

    // assigning to an element
    if( assigning_to_element )
    {
        symbol_compute_node.lhs_symbol_index = CompileLogicHashMapReference(lhs_hashmap);

        IssueErrorOnTokenMismatch(TOKEQOP, MGF::equals_expected_in_assignment_5);

        NextToken();

        symbol_compute_node.rhs_symbol_type = SymbolType::None;
        symbol_compute_node.rhs_symbol_index = CompileExpression(lhs_hashmap->GetValueType());
    }

    // assigning a hashmap to a hashmap
    else
    {
        ASSERT(Tkn == TOKEQOP);

        NextToken();
        IssueErrorOnTokenMismatch(TOKHASHMAP, MGF::HashMap_invalid_assignment_47231);

        // confirm that the types match
        const LogicHashMap& rhs_hashmap = GetSymbolLogicHashMap(Tokstindex);

        if( lhs_hashmap->GetValueType() != rhs_hashmap.GetValueType() )
        {
            IssueError(MGF::HashMap_invalid_assignment_HashMap_value_47233, ToString(lhs_hashmap->GetValueType()),
                                                                            lhs_hashmap->GetName().c_str());
        }

        if( !lhs_hashmap->IsHashMapAssignable(rhs_hashmap, true) )
        {
            IssueError(MGF::HashMap_invalid_assignment_HashMap_dimensions_47234, rhs_hashmap.GetName().c_str(),
                                                                                 lhs_hashmap->GetName().c_str());
        }

        symbol_compute_node.lhs_symbol_index = lhs_hashmap->GetSymbolIndex();
        symbol_compute_node.rhs_symbol_type = SymbolType::HashMap;
        symbol_compute_node.rhs_symbol_index = rhs_hashmap.GetSymbolIndex();

        NextToken();
    }

    if( hashmap_from_declaration == nullptr )
        IssueErrorOnTokenMismatch(TOKSEMICOLON, MGF::expecting_semicolon_30);

    return GetProgramIndex(symbol_compute_node);
}


int LogicCompiler::CompileLogicHashMapFunctions()
{
    // compiling hashmap_name.clear();
    //           hashmap_name.contains(key_expression[, key_expression, ...]);
    //           hashmap_name.getKeys(list, [key_expression, ...]);
    //           hashmap_name.length([key_expression, ...]);
    //           hashmap_name.remove(key_expression[, key_expression, ...]);

    FunctionCode function_code = CurrentToken.function_details->code;
    const LogicHashMap* hashmap = assert_cast<const LogicHashMap*>(CurrentToken.symbol);

    std::vector<int> arguments;
    size_t dimensions_specified = 0;

    size_t min_dimensions_allowed =
        ( function_code == HASHMAPFN_CLEAR_CODE ||
          function_code == HASHMAPFN_GETKEYS_CODE ||
          function_code == HASHMAPFN_LENGTH_CODE )   ? 0 : 1;

    size_t max_dimensions_allowed =
        ( function_code == HASHMAPFN_CLEAR_CODE )    ? 0 :
        ( function_code == HASHMAPFN_GETKEYS_CODE ||
          function_code == HASHMAPFN_LENGTH_CODE )   ? ( hashmap->GetNumberDimensions() - 1 ) :
                                                       hashmap->GetNumberDimensions();

    NextToken();
    IssueErrorOnTokenMismatch(TOKLPAREN, MGF::left_parenthesis_expected_in_function_call_14);

    NextToken();

    // read the list for getKeys
    const LogicList* getkeys_list = nullptr;

    if( function_code == HASHMAPFN_GETKEYS_CODE )
    {
        if( Tkn != TOKLIST )
            IssueError(MGF::HashMap_getKeys_requires_List_47237, hashmap->GetName().c_str(), _T(""));

        getkeys_list = &GetSymbolLogicList(Tokstindex);

        if( getkeys_list->IsReadOnly() )
            IssueError(MGF::List_read_only_cannot_be_modified_965, getkeys_list->GetName().c_str());

        NextToken();

        if( Tkn == TOKCOMMA )
        {
            NextToken();

            if( Tkn == TOKRPAREN )
                IssueError(MGF::function_call_missing_arguments_49);
        }
    }

    // read the dimensions
    while( Tkn != TOKRPAREN && dimensions_specified < hashmap->GetNumberDimensions() )
    {
        DataType value_data_type = GetCurrentTokenDataType();

        if( !hashmap->DimensionTypeHandles(dimensions_specified, value_data_type) )
        {
            IssueError(MGF::HashMap_invalid_value_for_dimension_47235, hashmap->GetName().c_str(),
                                                                       ToString(value_data_type),
                                                                       static_cast<int>(dimensions_specified) + 1);
        }

        arguments.emplace_back(static_cast<int>(value_data_type));
        arguments.emplace_back(CompileExpression(value_data_type));

        ++dimensions_specified;

        if( Tkn == TOKCOMMA )
        {
            NextToken();
        }

        else
        {
            break;
        }
    }

    if( dimensions_specified < min_dimensions_allowed || dimensions_specified > max_dimensions_allowed )
        IssueError(MGF::function_call_arguments_count_mismatch_50);

    // check that the list type matches the dimension
    if( getkeys_list != nullptr )
    {
        bool string_list = hashmap->DimensionTypeHandles(dimensions_specified, DataType::String);

        if( getkeys_list->IsString() != string_list )
        {
            std::wstring list_type_text = FormatTextCS2WS(_T(" of type '%s'"), ToString(string_list ? DataType::String : DataType::Numeric));
            IssueError(MGF::HashMap_getKeys_requires_List_47237, hashmap->GetName().c_str(), list_type_text.c_str());
        }

        arguments.emplace_back(getkeys_list->GetSymbolIndex());
    }

    IssueErrorOnTokenMismatch(TOKRPAREN, MGF::right_parenthesis_expected_in_function_call_17);

    NextToken();

    return CreateSymbolVariableArgumentsNode(function_code, *hashmap, std::vector<int>{ CreateListNode(arguments) });
}
