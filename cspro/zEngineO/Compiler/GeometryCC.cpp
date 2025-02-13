#include "stdafx.h"
#include "IncludesCC.h"
#include "Geometry.h"


LogicGeometry* LogicCompiler::CompileLogicGeometryDeclaration()
{
    std::wstring geometry_name = CompileNewSymbolName();

    auto logic_geometry = std::make_shared<LogicGeometry>(std::move(geometry_name));

    m_engineData->AddSymbol(logic_geometry);

    return logic_geometry.get();
}


int LogicCompiler::CompileLogicGeometryDeclarations()
{
    ASSERT(Tkn == TOKKWGEOMETRY);
    Nodes::SymbolReset* symbol_reset_node = nullptr;

    do
    {
        LogicGeometry* logic_geometry = CompileLogicGeometryDeclaration();

        int& initialize_value = AddSymbolResetNode(symbol_reset_node, *logic_geometry);

        NextToken();

        // allow assignments as part of the declaration
        if( Tkn == TOKEQOP )
            initialize_value = CompileLogicGeometryComputeInstruction(logic_geometry);

    } while( Tkn == TOKCOMMA );

    IssueErrorOnTokenMismatch(TOKSEMICOLON, MGF::expecting_semicolon_30);

    return GetOptionalProgramIndex(symbol_reset_node);
}


int LogicCompiler::CompileLogicGeometryComputeInstruction(const LogicGeometry* logic_geometry_from_declaration/* = nullptr*/)
{
    const Symbol* lhs_symbol = logic_geometry_from_declaration;
    const int lhs_subscript_compilation = CurrentToken.symbol_subscript_compilation;

    if( IsGlobalCompilation() )
        IssueError(MGF::symbol_assignment_not_allowed_in_proc_global_687, ToString(SymbolType::Geometry));

    if( lhs_symbol == nullptr )
    {
        ASSERT(Tkn == TOKGEOMETRY);
        lhs_symbol = &NPT_Ref(Tokstindex);
        ASSERT(lhs_symbol->IsA(SymbolType::Geometry) || ( lhs_symbol->IsA(SymbolType::Item) && lhs_subscript_compilation != -1 ));

        NextToken();

        IssueErrorOnTokenMismatch(TOKEQOP, MGF::Geometry_invalid_assignment_47321);
    }

    ASSERT(Tkn == TOKEQOP);

    NextToken();

    // a Geometry can be assigned another Geometry or a Document object
    if( Tkn != TOKGEOMETRY && Tkn != TOKDOCUMENT )
        IssueError(MGF::Geometry_invalid_assignment_47321);

    auto& symbol_compute_with_subscript_node = CreateNode<Nodes::SymbolComputeWithSubscript>(FunctionCode::GEOMETRYFN_COMPUTE_CODE);

    symbol_compute_with_subscript_node.next_st = -1;
    symbol_compute_with_subscript_node.lhs_symbol_index = lhs_symbol->GetSymbolIndex();
    symbol_compute_with_subscript_node.lhs_subscript_compilation = lhs_subscript_compilation;
    symbol_compute_with_subscript_node.rhs_symbol_index = Tokstindex;
    symbol_compute_with_subscript_node.rhs_subscript_compilation = CurrentToken.symbol_subscript_compilation;

    NextToken();

    if( logic_geometry_from_declaration == nullptr )
        IssueErrorOnTokenMismatch(TOKSEMICOLON, MGF::expecting_semicolon_30);

    return GetProgramIndex(symbol_compute_with_subscript_node);
}


int LogicCompiler::CompileLogicGeometryFunctions()
{
    // compiling geometry_name.clear();
    //           geometry_name.load(filename);
    //           geometry_name.save(filename);
    //           geometry_name.tracePolygon([map]);
    //           geometry_name.walkPolygon([map]);
    //           geometry_name.area();
    //           geometry_name.perimeter();
    //           geometry_name.minLatitude();
    //           geometry_name.maxLatitude();
    //           geometry_name.minLongitude();
    //           geometry_name.maxLongitude();
    //           geometry_name.getProperty(name);
    //           geometry_name.setProperty(name, value);
    const FunctionCode function_code = CurrentToken.function_details->code;

    ASSERT(CurrentToken.symbol != nullptr && CurrentToken.symbol->IsOneOf(SymbolType::Geometry, SymbolType::Item));
    const Symbol& symbol = *CurrentToken.symbol;

    Nodes::SymbolVariableArgumentsWithSubscript& symbol_va_with_subscript_node =
        CreateSymbolVariableArgumentsWithSubscriptNode(function_code, symbol, CurrentToken.symbol_subscript_compilation,
                                                       CurrentToken.function_details->number_arguments, -1);

    NextToken();
    IssueErrorOnTokenMismatch(TOKLPAREN, MGF::left_parenthesis_expected_in_function_call_14);

    NextToken();

    // load and save take a string argument
    if( function_code == FunctionCode::GEOMETRYFN_LOAD_CODE ||
        function_code == FunctionCode::GEOMETRYFN_SAVE_CODE )
    {
        symbol_va_with_subscript_node.arguments[0] = CompileStringExpression();
    }

    else if( function_code == FunctionCode::GEOMETRYFN_TRACE_POLYGON_CODE ||
             function_code == FunctionCode::GEOMETRYFN_WALK_POLYGON_CODE )
    {
        // optional map argument
        if( Tkn != TOKRPAREN )
        {
            if( Tkn != TOKMAP )
                IssueError(MGF::Geometry_Map_argument_expected_47322, Logic::FunctionTable::GetFunctionName(symbol_va_with_subscript_node.function_code), Tokstr.c_str());

            symbol_va_with_subscript_node.arguments[0] = Tokstindex;
            NextToken();
        }
    }

    else if( function_code == FunctionCode::GEOMETRYFN_GET_PROPERTY_CODE ||
             function_code == FunctionCode::GEOMETRYFN_SET_PROPERTY_CODE )
    {
        // the property name
        symbol_va_with_subscript_node.arguments[0] = CompileStringExpression();

        // for setProperty, the property value can be numeric/string
        if( function_code == FunctionCode::GEOMETRYFN_SET_PROPERTY_CODE )
        {
            IssueErrorOnTokenMismatch(TOKCOMMA, MGF::function_call_comma_expected_528);

            NextToken();

            const DataType value_data_type = GetCurrentTokenDataType();
            symbol_va_with_subscript_node.arguments[1] = static_cast<int>(value_data_type);
            symbol_va_with_subscript_node.arguments[2] = CompileExpression(value_data_type);
        }
    }

    IssueErrorOnTokenMismatch(TOKRPAREN, MGF::right_parenthesis_expected_in_function_call_17);

    NextToken();

    return GetProgramIndex(symbol_va_with_subscript_node);
}
