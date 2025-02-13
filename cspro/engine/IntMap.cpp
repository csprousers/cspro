#include "StandardSystemIncludes.h"
#include "Engdrv.h"
#include "INTERPRE.H"
#include <zEngineO/Geometry.h>
#include <zEngineO/Map.h>
#include <zEngineO/UserFunction.h>
#include <zEngineO/Versioning.h>
#include <zPlatformO/PlatformInterface.h>
#include <zUtilO/PortableColor.h>
#include <zMessageO/Messages.h>
#include <ZBRIDGEO/npff.h>
#include <zMapping/DefaultBaseMapEvaluator.h>
#include <zMapping/IMapUI.h>


double CIntDriver::exmapshow(int iExpr)
{
    const auto& symbol_va_node = GetNode<Nodes::SymbolVariableArguments>(iExpr);
    LogicMap& logic_map = GetSymbolLogicMap(symbol_va_node.symbol_index);

    if( logic_map.GetMapUI() == nullptr )
        return 0;

    // if a base map has not been set, then set the default one
    if( !logic_map.GetMapUI()->IsBaseMapDefined() )
        SetBaseMap(logic_map, GetDefaultBaseMapSelection(*m_pEngineDriver->m_pPifFile));

    // Show map (display map window)
    if (!logic_map.GetMapUI()->Show())
        return 0;

    logic_map.SetIsShowing(true);

    // We can't wait for map window to be closed to return from this function since clicks
    // on the map require running user functions in the engine. So we run an event loop
    // here by waiting for events on the map from UI, processing them and then continuing
    // to the next event. We keep processing events until the map is closed either via
    // the user interface or by a call to map.hide() in CSPro logic.
    while (logic_map.IsShowing()) {

        IMapUI::MapEvent event = logic_map.GetMapUI()->WaitForEvent();

        // Save the camera position
        logic_map.GetMapUI()->SetCamera(event.camera_);

        switch (event.code_) {
            case IMapUI::EventCode::MapClosed:
                // User closed map from UI. This will set IsShowing() to false so
                // loop will exit.
                logic_map.SetIsShowing(false);
                break;

            case IMapUI::EventCode::MapClicked:
            {
                logic_map.SetLastOnClick(event.latitude_, event.longitude_);
                int callback_id = logic_map.GetOnClickCallbackId();
                if (callback_id >= 0) {
                    ExecuteCallbackUserFunction(m_iExSymbol, logic_map.GetCallback(callback_id));

                    if (m_iStopExec || m_bStopProc) {
                        logic_map.GetMapUI()->Hide();
                        logic_map.SetIsShowing(false);
                    }
                }
                break;
            }

            case IMapUI::EventCode::MarkerDragged:
                logic_map.GetMapUI()->SetMarkerLocation(event.marker_id_, event.latitude_, event.longitude_);
                [[fallthrough]];
            case IMapUI::EventCode::MarkerClicked:
            case IMapUI::EventCode::MarkerInfoWindowClicked:
            case IMapUI::EventCode::ButtonClicked:
                if (event.callback_id_ >= 0) {
                    ExecuteCallbackUserFunction(m_iExSymbol, logic_map.GetCallback(event.callback_id_));

                    if (m_iStopExec || m_bStopProc) {
                        logic_map.GetMapUI()->Hide();
                        logic_map.SetIsShowing(false);
                    }
                }
                break;
        }
    }

    return 1;
}


double CIntDriver::exmaphide(int iExpr)
{
    const auto& symbol_va_node = GetNode<Nodes::SymbolVariableArguments>(iExpr);
    LogicMap& logic_map = GetSymbolLogicMap(symbol_va_node.symbol_index);

    if( logic_map.GetMapUI() == nullptr )
        return 0;

    logic_map.SetIsShowing(false);

    return ( logic_map.GetMapUI()->Hide() != 0 ) ? 1 : 0;
}


double CIntDriver::exmapaddmarker(int iExpr)
{
    const auto& symbol_va_node = GetNode<Nodes::SymbolVariableArguments>(iExpr);
    LogicMap& logic_map = GetSymbolLogicMap(symbol_va_node.symbol_index);

    if( logic_map.GetMapUI() == nullptr )
        return 0;

    double latitude = evalexpr(symbol_va_node.arguments[0]);
    double longitude = evalexpr(symbol_va_node.arguments[1]);

    return logic_map.GetMapUI()->AddMarker(latitude, longitude);
}


double CIntDriver::exmapsetmarkerimage(int iExpr)
{
    const auto& symbol_va_node = GetNode<Nodes::SymbolVariableArguments>(iExpr);
    LogicMap& logic_map = GetSymbolLogicMap(symbol_va_node.symbol_index);

    if( logic_map.GetMapUI() == nullptr )
        return 0;

    int marker_id = evalexpr<int>(symbol_va_node.arguments[0]);

    std::wstring image_path = EvalFullPathFileName(symbol_va_node.arguments[1]);

    // Try to catch invalid image file here since it is a pain to handle on the Java side
    if (!PortableFunctions::FileIsRegular(image_path))
    {
        issaerror(MessageType::Error, 2001, image_path.c_str());
        return 0;
    }

    return logic_map.GetMapUI()->SetMarkerImage(marker_id, std::move(image_path));
}


double CIntDriver::exmapsetmarkertext(int iExpr)
{
    const auto& symbol_va_node = GetNode<Nodes::SymbolVariableArguments>(iExpr);
    LogicMap& logic_map = GetSymbolLogicMap(symbol_va_node.symbol_index);

    if( logic_map.GetMapUI() == nullptr )
        return 0;

    int marker_id = evalexpr<int>(symbol_va_node.arguments[0]);
    std::wstring text = EvalAlphaExpr(symbol_va_node.arguments[1]);

    auto evaluate_and_get_color = [&](int argument_index, int default_color)
    {
        if( symbol_va_node.arguments[argument_index] >= 0 )
        {
            std::wstring color_string = EvalAlphaExpr(symbol_va_node.arguments[argument_index]);
            std::optional<PortableColor> portable_color = PortableColor::FromString(color_string);

            if( portable_color.has_value() )
                return portable_color->ToColorInt();

            issaerror(MessageType::Error, 2036, color_string.c_str());
        }

        return default_color;
    };

    int background_color = evaluate_and_get_color(2, 0xFFFFFFFF); // default background color is white
    int text_color = evaluate_and_get_color(3, 0xFF000000); // default text color is black

    return logic_map.GetMapUI()->SetMarkerText(marker_id, std::move(text), background_color, text_color);
}


double CIntDriver::exmapsetmarkeronclickonclickinfo(int iExpr)
{
    const auto& symbol_va_node = GetNode<Nodes::SymbolVariableArguments>(iExpr);
    LogicMap& logic_map = GetSymbolLogicMap(symbol_va_node.symbol_index);

    if( logic_map.GetMapUI() == nullptr )
        return 0;

    int marker_id = evalexpr<int>(symbol_va_node.arguments[0]);

    int callback_index = logic_map.AddCallback(
        EvaluateArgumentsForCallbackUserFunction(symbol_va_node.arguments[1], FunctionCode::MAPFN_SHOW_CODE));

    return ( symbol_va_node.function_code == FunctionCode::MAPFN_SET_MARKER_ON_CLICK_CODE ) ?
        logic_map.GetMapUI()->SetMarkerOnClick(marker_id, callback_index) :
        logic_map.GetMapUI()->SetMarkerOnClickInfoWindow(marker_id, callback_index);
}


double CIntDriver::exmapsetmarkerdescription(int iExpr)
{
    const auto& symbol_va_node = GetNode<Nodes::SymbolVariableArguments>(iExpr);
    LogicMap& logic_map = GetSymbolLogicMap(symbol_va_node.symbol_index);

    if( logic_map.GetMapUI() == nullptr )
        return 0;

    int marker_id = evalexpr<int>(symbol_va_node.arguments[0]);
    std::wstring description = EvalAlphaExpr(symbol_va_node.arguments[1]);

    return logic_map.GetMapUI()->SetMarkerDescription(marker_id, std::move(description));
}


double CIntDriver::exmapsetmarkerondrag(int iExpr)
{
    const auto& symbol_va_node = GetNode<Nodes::SymbolVariableArguments>(iExpr);
    LogicMap& logic_map = GetSymbolLogicMap(symbol_va_node.symbol_index);

    if( logic_map.GetMapUI() == nullptr )
        return 0;

    int marker_id = evalexpr<int>(symbol_va_node.arguments[0]);

    int callback_index = logic_map.AddCallback(
        EvaluateArgumentsForCallbackUserFunction(symbol_va_node.arguments[1], FunctionCode::MAPFN_SHOW_CODE));

    return logic_map.GetMapUI()->SetMarkerOnDrag(marker_id, callback_index);
}


double CIntDriver::exmapsetmarkerlocation(int iExpr)
{
    const auto& symbol_va_node = GetNode<Nodes::SymbolVariableArguments>(iExpr);
    LogicMap& logic_map = GetSymbolLogicMap(symbol_va_node.symbol_index);

    if( logic_map.GetMapUI() == nullptr )
        return 0;

    int marker_id = evalexpr<int>(symbol_va_node.arguments[0]);
    double latitude = evalexpr(symbol_va_node.arguments[1]);
    double longitude = evalexpr(symbol_va_node.arguments[2]);

    return logic_map.GetMapUI()->SetMarkerLocation(marker_id, latitude, longitude);
}


double CIntDriver::exmapgetmarkerlatitudelongitude(int iExpr)
{
    const auto& symbol_va_node = GetNode<Nodes::SymbolVariableArguments>(iExpr);
    LogicMap& logic_map = GetSymbolLogicMap(symbol_va_node.symbol_index);

    if( logic_map.GetMapUI() == nullptr )
        return 0;

    int marker_id = evalexpr<int>(symbol_va_node.arguments[0]);

    double latitude;
    double longitude;

    return ( logic_map.GetMapUI()->GetMarkerLocation(marker_id, latitude, longitude) == 0 ) ? DEFAULT :
           ( symbol_va_node.function_code == FunctionCode::MAPFN_GET_MARKER_LATITUDE_CODE ) ? latitude :
                                                                                              longitude;
}


double CIntDriver::exmapremovemarker(int iExpr)
{
    const auto& symbol_va_node = GetNode<Nodes::SymbolVariableArguments>(iExpr);
    LogicMap& logic_map = GetSymbolLogicMap(symbol_va_node.symbol_index);

    if( logic_map.GetMapUI() == nullptr )
        return 0;

    int marker_id = evalexpr<int>(symbol_va_node.arguments[0]);

    return logic_map.GetMapUI()->RemoveMarker(marker_id);
}


double CIntDriver::exmapshowcurrentlocation(int iExpr)
{
    const auto& symbol_va_node = GetNode<Nodes::SymbolVariableArguments>(iExpr);
    LogicMap& logic_map = GetSymbolLogicMap(symbol_va_node.symbol_index);

    if( logic_map.GetMapUI() == nullptr )
        return 0;

    bool show = ConditionalValueIsTrue(evalexpr(symbol_va_node.arguments[0]));

    return logic_map.GetMapUI()->SetShowCurrentLocation(show);
}


double CIntDriver::exmapaddtextbutton(int iExpr)
{
    const auto& symbol_va_node = GetNode<Nodes::SymbolVariableArguments>(iExpr);
    LogicMap& logic_map = GetSymbolLogicMap(symbol_va_node.symbol_index);

    if( logic_map.GetMapUI() == nullptr )
        return 0;

    std::wstring label = EvalAlphaExpr(symbol_va_node.arguments[0]);

    int callback_index = logic_map.AddCallback(
        EvaluateArgumentsForCallbackUserFunction(symbol_va_node.arguments[1], FunctionCode::MAPFN_SHOW_CODE));

    return logic_map.GetMapUI()->AddTextButton(std::move(label), callback_index);
}


double CIntDriver::exmapaddimagebutton(int iExpr)
{
    const auto& symbol_va_node = GetNode<Nodes::SymbolVariableArguments>(iExpr);
    LogicMap& logic_map = GetSymbolLogicMap(symbol_va_node.symbol_index);

    if( logic_map.GetMapUI() == nullptr )
        return 0;

    std::wstring image_path = EvalFullPathFileName(symbol_va_node.arguments[0]);

    // Try to catch invalid image file here since it is a pain to handle on the Java side
    if (!PortableFunctions::FileIsRegular(image_path))
    {
        issaerror(MessageType::Error, 2001, image_path.c_str());
        return 0;
    }

    int callback_index = logic_map.AddCallback(
        EvaluateArgumentsForCallbackUserFunction(symbol_va_node.arguments[1], FunctionCode::MAPFN_SHOW_CODE));

    return logic_map.GetMapUI()->AddImageButton(std::move(image_path), callback_index);
}


double CIntDriver::exmapremovebutton(int iExpr)
{
    const auto& symbol_va_node = GetNode<Nodes::SymbolVariableArguments>(iExpr);
    LogicMap& logic_map = GetSymbolLogicMap(symbol_va_node.symbol_index);

    if( logic_map.GetMapUI() == nullptr )
        return 0;

    int button_id = evalexpr<int>(symbol_va_node.arguments[0]);

    return logic_map.GetMapUI()->RemoveButton(button_id);
}


template<typename T>
int CIntDriver::SetBaseMap(LogicMap& logic_map, const T& base_map_selection)
{
    try
    {
        // if a filename, try to catch an invalid map file here since it is a pain to handle on the Java side
        if( std::holds_alternative<std::wstring>(base_map_selection) &&
            !PortableFunctions::FileIsRegular(std::get<std::wstring>(base_map_selection)) )
        {
            throw CSProException(MGF::GetMessageText(2001).c_str(), std::get<std::wstring>(base_map_selection).c_str());
        }

        return logic_map.GetMapUI()->SetBaseMap(base_map_selection);
    }

    catch( const CSProException& exception )
    {
        issaerror(MessageType::Error, 94205, logic_map.GetName().c_str(), exception.GetErrorMessage().c_str());
        return 0;
    }
}


double CIntDriver::exmapsetbasemap(int iExpr)
{
    const auto& symbol_va_node = GetNode<Nodes::SymbolVariableArguments>(iExpr);
    LogicMap& logic_map = GetSymbolLogicMap(symbol_va_node.symbol_index);
    const int& base_map_type = symbol_va_node.arguments[0];
    const int& custom_source_expression = symbol_va_node.arguments[1];

    if( logic_map.GetMapUI() == nullptr )
        return 0;

    BaseMapSelection base_map_selection;

    if( base_map_type > 0 )
    {
        base_map_selection = (BaseMap)base_map_type;
    }

    else
    {
        std::wstring base_map_text = EvalAlphaExpr(custom_source_expression);
        base_map_selection = FromString(base_map_text, m_pEngineDriver->m_pPifFile->GetAppFName());
    }

    return SetBaseMap(logic_map, base_map_selection);
}


double CIntDriver::exmapsettitle(int iExpr)
{
    const auto& symbol_va_node = GetNode<Nodes::SymbolVariableArguments>(iExpr);
    LogicMap& logic_map = GetSymbolLogicMap(symbol_va_node.symbol_index);

    if( logic_map.GetMapUI() == nullptr )
        return 0;

    std::wstring title = EvalAlphaExpr(symbol_va_node.arguments[0]);

    return logic_map.GetMapUI()->SetTitle(std::move(title));
}


double CIntDriver::exmapzoomto(int iExpr)
{
    // zoomTo(ml, ml, ml, ml, padding)
    const auto& symbol_va_node = GetNode<Nodes::SymbolVariableArguments>(iExpr);
    LogicMap& logic_map = GetSymbolLogicMap(symbol_va_node.symbol_index);

    if (symbol_va_node.arguments[3] < 0)
    {
        // lat/long/[zoom]
        double lat = evalexpr(symbol_va_node.arguments[0]);
        double lon = evalexpr(symbol_va_node.arguments[1]);
        double zoom = ( symbol_va_node.arguments[2] >= 0 ) ? evalexpr(symbol_va_node.arguments[2]) : -1;
        return logic_map.GetMapUI()->ZoomTo(lat, lon, zoom);
    }

    else
    {
        // minlat, minlong,maxlat,maxlong,[padding]
        double minlat = evalexpr(symbol_va_node.arguments[0]);
        double minlon = evalexpr(symbol_va_node.arguments[1]);
        double maxlat = evalexpr(symbol_va_node.arguments[2]);
        double maxlon = evalexpr(symbol_va_node.arguments[3]);
        double padding = ( symbol_va_node.arguments[4] >= 0 ) ? ( evalexpr(symbol_va_node.arguments[4]) / 100 ) : 0;
        return logic_map.GetMapUI()->ZoomTo(minlat, minlon, maxlat, maxlon, padding);
    }
}


double CIntDriver::exmapsetonclick(int iExpr)
{
    const auto& symbol_va_node = GetNode<Nodes::SymbolVariableArguments>(iExpr);
    LogicMap& logic_map = GetSymbolLogicMap(symbol_va_node.symbol_index);

    int callback_index = logic_map.AddCallback(
        EvaluateArgumentsForCallbackUserFunction(symbol_va_node.arguments[0], FunctionCode::MAPFN_SHOW_CODE));

    logic_map.SetOnClickCallbackId(callback_index);

    return 1;
}


double CIntDriver::exmap_clear_clearButtons_clearGeometry_clearMarkers(int program_index)
{
    const auto& symbol_va_node = GetNode<Nodes::SymbolVariableArguments>(program_index);
    LogicMap& logic_map = GetSymbolLogicMap(symbol_va_node.symbol_index);

    if( logic_map.GetMapUI() == nullptr )
        return 0;

    switch( symbol_va_node.function_code )
    {
        case FunctionCode::MAPFN_CLEAR_CODE:
            logic_map.GetMapUI()->Clear();
            break;

        case FunctionCode::MAPFN_CLEAR_BUTTONS_CODE:
            logic_map.GetMapUI()->ClearButtons();
            break;

        case FunctionCode::MAPFN_CLEAR_GEOMETRY_CODE:
            logic_map.GetMapUI()->ClearGeometry();
            break;

        default:
            ASSERT(symbol_va_node.function_code == FunctionCode::MAPFN_CLEAR_MARKERS_CODE);
            logic_map.GetMapUI()->ClearMarkers();
            break;
    }

    return 1;
}


double CIntDriver::exmapgetlastclicklatitudelongitude(int iExpr)
{
    const auto& symbol_va_node = GetNode<Nodes::SymbolVariableArguments>(iExpr);
    const LogicMap& logic_map = GetSymbolLogicMap(symbol_va_node.symbol_index);

    return ( symbol_va_node.function_code == FunctionCode::MAPFN_GET_LAST_CLICK_LATITUDE_CODE ) ?
        logic_map.GetLastClickLatitude() : logic_map.GetLastClickLongitude();
}


double CIntDriver::exmapaddgeometry(int iExpr)
{
    const auto& symbol_va_node = GetNode<Nodes::SymbolVariableArguments>(iExpr);
    LogicMap& logic_map = GetSymbolLogicMap(symbol_va_node.symbol_index);
    const LogicGeometry* logic_geometry = GetFromSymbolOrEngineItem<LogicGeometry*>(symbol_va_node.arguments[0],
                                                                                    Versioning::MeetsCompiledLogicVersion(Serializer::Iteration_8_0_000_1) ? symbol_va_node.arguments[1] : -1);

    if( logic_geometry == nullptr || !EnsureGeometryExistsAndHasValidContent(*logic_geometry, _T("add it to a map")) )
        return 0;

    return logic_map.GetMapUI()->AddGeometry(logic_geometry->GetSharedFeatures(), logic_geometry->GetSharedBoundingBox());
}


double CIntDriver::exmapremovegeometry(int iExpr)
{
    const auto& symbol_va_node = GetNode<Nodes::SymbolVariableArguments>(iExpr);
    LogicMap& logic_map = GetSymbolLogicMap(symbol_va_node.symbol_index);

    if( logic_map.GetMapUI() == nullptr )
        return 0;

    int geometry_id = evalexpr<int>(symbol_va_node.arguments[0]);

    return logic_map.GetMapUI()->RemoveGeometry(geometry_id);
}


double CIntDriver::exmapsavesnapshot(int iExpr)
{
    const auto& symbol_va_node = GetNode<Nodes::SymbolVariableArguments>(iExpr);
    LogicMap& logic_map = GetSymbolLogicMap(symbol_va_node.symbol_index);

    if( logic_map.GetMapUI() == nullptr )
        return 0;

    try
    {
        if( !logic_map.IsShowing() )
            throw CSProException("the map must be showing before you can save a snapshot");

        std::wstring snapshot_filename = EvalFullPathFileName(symbol_va_node.arguments[0]);

        return logic_map.GetMapUI()->SaveSnapshot(snapshot_filename);
    }

    catch( const CSProException& exception )
    {
        issaerror(MessageType::Error, 94206, logic_map.GetName().c_str(), exception.GetErrorMessage().c_str());
        return 0;
    }
}
