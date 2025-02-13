#include "stdafx.h"
#include "IncludesCC.h"
#include "Map.h"


LogicMap* LogicCompiler::CompileLogicMapDeclaration()
{
    std::wstring map_name = CompileNewSymbolName();

    auto logic_map = std::make_shared<LogicMap>(std::move(map_name));

    m_engineData->AddSymbol(logic_map);

    return logic_map.get();
}


int LogicCompiler::CompileLogicMapDeclarations()
{
    ASSERT(Tkn == TOKKWMAP);
    Nodes::SymbolReset* symbol_reset_node = nullptr;

    do
    {
        const LogicMap* logic_map = CompileLogicMapDeclaration();

        AddSymbolResetNode(symbol_reset_node, *logic_map);

        NextToken();

    } while( Tkn == TOKCOMMA );

    IssueErrorOnTokenMismatch(TOKSEMICOLON, MGF::expecting_semicolon_30);

    return GetOptionalProgramIndex(symbol_reset_node);
}


int LogicCompiler::CompileLogicMapFunctions()
{
    FunctionCode function_code = CurrentToken.function_details->code;
    std::wstring function_name = Tokstr;
    const LogicMap& logic_map = assert_cast<const LogicMap&>(*CurrentToken.symbol);
    Nodes::SymbolVariableArguments& symbol_va_node = CreateSymbolVariableArgumentsNode(function_code, logic_map,
                                                                                       CurrentToken.function_details->number_arguments, -1);

    NextToken();
    IssueErrorOnTokenMismatch(TOKLPAREN, MGF::left_parenthesis_expected_in_function_call_14);

    if( function_code != FunctionCode::MAPFN_SET_BASE_MAP_CODE )
        NextToken();

    // map_name.show()
    // map_name.hide()
    // map_name.clear()
    // map_name.clearMarkers()
    // map_name.clearButtons()
    // map_name.clearGeometry()
    // map_name.getLastClickLatitude()
    // map_name.getLastClickLongitude()
    if( function_code == FunctionCode::MAPFN_SHOW_CODE ||
        function_code == FunctionCode::MAPFN_HIDE_CODE ||
        function_code == FunctionCode::MAPFN_CLEAR_CODE ||
        function_code == FunctionCode::MAPFN_CLEAR_MARKERS_CODE ||
        function_code == FunctionCode::MAPFN_CLEAR_BUTTONS_CODE ||
        function_code == FunctionCode::MAPFN_CLEAR_GEOMETRY_CODE ||
        function_code == FunctionCode::MAPFN_GET_LAST_CLICK_LATITUDE_CODE ||
        function_code == FunctionCode::MAPFN_GET_LAST_CLICK_LONGITUDE_CODE )
    {
    }

    // map_name.addMarker(latitude, longitude)
    else if( function_code == FunctionCode::MAPFN_ADD_MARKER_CODE )
    {
        symbol_va_node.arguments[0] = exprlog();

        IssueErrorOnTokenMismatch(TOKCOMMA, MGF::function_call_comma_expected_528);
        NextToken();

        symbol_va_node.arguments[1] = exprlog();
    }

    // map_name.setMarkerImage(markerId, pathOfImageFile)
    // map_name.setMarkerDescription(markerId, description)
    else if( function_code == FunctionCode::MAPFN_SET_MARKER_IMAGE_CODE ||
             function_code == FunctionCode::MAPFN_SET_MARKER_DESCRIPTION_CODE)
    {
        symbol_va_node.arguments[0] = exprlog();

        IssueErrorOnTokenMismatch(TOKCOMMA, MGF::function_call_comma_expected_528);
        NextToken();

        symbol_va_node.arguments[1] = CompileStringExpression();
    }

    // map_name.setMarkerText(markerId, text, [backgroundColor], [textColor])
    else if( function_code == FunctionCode::MAPFN_SET_MARKER_TEXT_CODE )
    {
        symbol_va_node.arguments[0] = exprlog();

        IssueErrorOnTokenMismatch(TOKCOMMA, MGF::function_call_comma_expected_528);
        NextToken();

        symbol_va_node.arguments[1] = CompileStringExpression();

        if( Tkn == TOKCOMMA )
        {
            NextToken();
            symbol_va_node.arguments[2] = CompilePortableColorText();
        }

        if( Tkn == TOKCOMMA )
        {
            NextToken();
            symbol_va_node.arguments[3] = CompilePortableColorText();
        }
    }

    // map_name.setMarkerOnClick(markerId, onClickFunction)
    // map_name.setMarkerOnDrag(markerId, onDragFunction)
    // map_name.setMarkerOnClickInfoWindow(markerId, onClickFunction)
    else if( function_code == FunctionCode::MAPFN_SET_MARKER_ON_CLICK_CODE ||
             function_code == FunctionCode::MAPFN_SET_MARKER_ON_CLICK_INFO_CODE ||
             function_code == FunctionCode::MAPFN_SET_MARKER_ON_DRAG_CODE )
    {
        symbol_va_node.arguments[0] = exprlog();

        IssueErrorOnTokenMismatch(TOKCOMMA, MGF::function_call_comma_expected_528);

        NextToken();

        if( Tkn != TOKUSERFUNCTION )
            IssueError(MGF::Map_argument_must_be_function_94200, function_name.c_str());

        symbol_va_node.arguments[1] = CompileUserFunctionCall(true);
    }

    // map_name.setOnClick(onClickFunction)
    else if( function_code == FunctionCode::MAPFN_SET_ON_CLICK_CODE )
    {
        if( Tkn != TOKUSERFUNCTION )
            IssueError(MGF::Map_argument_must_be_function_94200, function_name.c_str());

        symbol_va_node.arguments[0] = CompileUserFunctionCall(true);
    }

    // map_name.setMarkerLocation(markerId, latitude, longitude)
    else if( function_code == FunctionCode::MAPFN_SET_MARKER_LOCATION_CODE )
    {
        symbol_va_node.arguments[0] = exprlog();

        IssueErrorOnTokenMismatch(TOKCOMMA, MGF::function_call_comma_expected_528);
        NextToken();

        symbol_va_node.arguments[1] = exprlog();

        IssueErrorOnTokenMismatch(TOKCOMMA, MGF::function_call_comma_expected_528);
        NextToken();

        symbol_va_node.arguments[2] = exprlog();
    }

    // map_name.getMarkerLatitude(markerId)
    // map_name.getMarkerLongitude(markerId)
    // map_name.removeMarker(markerId)
    // map_name.removeButton(markerId)
    // map_name.removeGeometry(markerId)
    else if( function_code == FunctionCode::MAPFN_GET_MARKER_LATITUDE_CODE ||
             function_code == FunctionCode::MAPFN_GET_MARKER_LONGITUDE_CODE ||
             function_code == FunctionCode::MAPFN_REMOVE_MARKER_CODE ||
             function_code == FunctionCode::MAPFN_REMOVE_BUTTON_CODE ||
             function_code == FunctionCode::MAPFN_REMOVE_GEOMETRY_CODE )
    {
        symbol_va_node.arguments[0] = exprlog();
    }

    // map_name.addTextButton(label, onClickFunction)
    // map_name.addImageButton(pathToImageFile, onClickFunction)
    else if( function_code == FunctionCode::MAPFN_ADD_IMAGE_BUTTON_CODE ||
             function_code == FunctionCode::MAPFN_ADD_TEXT_BUTTON_CODE )
    {
        symbol_va_node.arguments[0] = CompileStringExpression();

        IssueErrorOnTokenMismatch(TOKCOMMA, MGF::function_call_comma_expected_528);

        NextToken();

        if( Tkn != TOKUSERFUNCTION )
            IssueError(MGF::Map_argument_must_be_function_94200, function_name.c_str());

        symbol_va_node.arguments[1] = CompileUserFunctionCall(true);
    }

    // map_name.showCurrentLocation(show_condition)
    else if( function_code == FunctionCode::MAPFN_SHOW_CURRENT_LOCATION_CODE )
    {
        symbol_va_node.arguments[0] = exprlog();
    }

    // map_name.setTitle(title)
    // map_name.saveSnapshot(filename)
    else if( function_code == FunctionCode::MAPFN_SET_TITLE_CODE ||
             function_code == FunctionCode::MAPFN_SAVESNAPSHOT_CODE )
    {
        symbol_va_node.arguments[0] = CompileStringExpression();
    }

    // map_name.setBasemap(Normal|Hybrid|Satellite|Terrain|None | pathToFile)
    else if( function_code == FunctionCode::MAPFN_SET_BASE_MAP_CODE )
    {
        symbol_va_node.arguments[0] = static_cast<int>(NextKeyword(GetBaseMapStrings()));

        NextToken();

        // Must be a path to a file or text such as "Terrain"
        if( symbol_va_node.arguments[0] == 0 )
            symbol_va_node.arguments[1] = CompileStringExpression();
    }

    // map_name.zoomTo(lat, lon[, zoom])
    // map_name.zoomTo(minLat, minLong, maxLat, maxLong[, padding])
    else if( function_code == FunctionCode::MAPFN_ZOOM_TO_CODE )
    {
        size_t arg_num = 0;

        do
        {
            if( arg_num > 0 )
                NextToken();

            symbol_va_node.arguments[arg_num] = exprlog();
            ++arg_num;

        } while( Tkn == TOKCOMMA && arg_num < 5 );

        if( arg_num < 2 || Tkn != TOKRPAREN )
            IssueError(MGF::Map_zoomTo_invalid_arguments_94201);
    }

    // map_name.addGeometry(geometry)
    else if( function_code == FunctionCode::MAPFN_ADD_GEOMETRY_CODE )
    {
        if( Tkn != TOKGEOMETRY )
            IssueError(MGF::Map_argument_must_be_Geometry_94202, function_name.c_str());

        symbol_va_node.arguments[0] = Tokstindex;
        symbol_va_node.arguments[1] = CurrentToken.symbol_subscript_compilation;
        NextToken();
    }

    IssueErrorOnTokenMismatch(TOKRPAREN, MGF::right_parenthesis_expected_in_function_call_17);

    NextToken();

    return GetProgramIndex(symbol_va_node);
}
