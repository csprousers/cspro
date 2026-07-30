#include "stdafx.h"
#include "IncludesRT.h"
#include "Geometry.h"
#include "Map.h"
#include "UserFunctionArgumentEvaluator.h"
#include <zMessageO/Messages.h>
#include <zMapping/DefaultBaseMapEvaluator.h>
#include <zMapping/IMapUI.h>

#pragma warning(push)
#pragma warning(disable: 4068 4239)
#include <mapbox/geometry.hpp>
#pragma warning(pop)


double LogicInterpreter::ex_Map_show(const int program_index)
{
    const auto& symbol_va_node = GetNode<Nodes::SymbolVariableArguments>(program_index);
    LogicMap& logic_map = GetSymbolLogicMap(symbol_va_node.symbol_index);
    IMapUI* const map_ui = logic_map.GetMapUI();

    if( map_ui == nullptr )
        return 0;

    // if a base map has not been set, then set the default one
    if( !map_ui->IsBaseMapDefined() && m_engineData->pff != nullptr )
        SetBaseMap(logic_map, GetDefaultBaseMapSelection(*m_engineData->pff));

    // show the map
    if( !map_ui->Show() )
        return 0;

    logic_map.SetIsShowing(true);

    // We can't wait for map window to be closed to return from this function since clicks
    // on the map require running user functions in the engine. So we run an event loop
    // here by waiting for events on the map from UI, processing them and then continuing
    // to the next event. We keep processing events until the map is closed either via
    // the user interface or by a call to map.hide() in CSPro logic.
    while( logic_map.IsShowing() )
    {
        const IMapUI::MapEvent event = map_ui->WaitForEvent();

        // save the camera position
        map_ui->SetCamera(event.camera);

        switch( event.code )
        {
            case IMapUI::EventCode::MapClosed:
            {
                // user closed map from UI; this will set IsShowing() to false so loop will exit
                logic_map.SetIsShowing(false);
                break;
            }

            case IMapUI::EventCode::MapClicked:
            {
                logic_map.SetLastOnClick(event.latitude, event.longitude);

                const int callback_id = logic_map.GetOnClickCallbackId();

                if( callback_id >= 0 )
                {
                    ExecuteCallbackUserFunction(Get_m_iExSymbol_INTERPRETER_DLL_TODO(), logic_map.GetCallback(callback_id));

                    if( IsExecutionInterrupted() )
                    {
                        map_ui->Hide();
                        logic_map.SetIsShowing(false);
                    }
                }

                break;
            }

            case IMapUI::EventCode::MarkerDragged:
            {
                map_ui->SetMarkerLocation(event.marker_id, event.latitude, event.longitude);
                [[fallthrough]];
            }

            case IMapUI::EventCode::MarkerClicked:
            case IMapUI::EventCode::MarkerInfoWindowClicked:
            case IMapUI::EventCode::ButtonClicked:
            {
                if( event.callback_id >= 0 )
                {
                    ExecuteCallbackUserFunction(Get_m_iExSymbol_INTERPRETER_DLL_TODO(), logic_map.GetCallback(event.callback_id));

                    if( IsExecutionInterrupted() )
                    {
                        map_ui->Hide();
                        logic_map.SetIsShowing(false);
                    }
                }

                break;
            }
        }
    }

    return 1;
}


double LogicInterpreter::ex_Map_hide(const int program_index)
{
    const auto& symbol_va_node = GetNode<Nodes::SymbolVariableArguments>(program_index);
    LogicMap& logic_map = GetSymbolLogicMap(symbol_va_node.symbol_index);
    IMapUI* const map_ui = logic_map.GetMapUI();

    if( map_ui == nullptr )
        return 0;

    logic_map.SetIsShowing(false);

    return map_ui->Hide();
}


double LogicInterpreter::ex_Map_addMarker(const int program_index)
{
    const auto& symbol_va_node = GetNode<Nodes::SymbolVariableArguments>(program_index);
    LogicMap& logic_map = GetSymbolLogicMap(symbol_va_node.symbol_index);
    IMapUI* const map_ui = logic_map.GetMapUI();

    if( map_ui == nullptr )
        return 0;

    const double latitude = Evaluate<double>(symbol_va_node.arguments[0]);
    const double longitude = Evaluate<double>(symbol_va_node.arguments[1]);

    return map_ui->AddMarker(latitude, longitude);
}


double LogicInterpreter::ex_Map_setMarkerImage(const int program_index)
{
    const auto& symbol_va_node = GetNode<Nodes::SymbolVariableArguments>(program_index);
    LogicMap& logic_map = GetSymbolLogicMap(symbol_va_node.symbol_index);
    IMapUI* const map_ui = logic_map.GetMapUI();

    if( map_ui == nullptr )
        return 0;

    const int marker_id = Evaluate<int>(symbol_va_node.arguments[0]);

    const SharableString image_url_or_file_path = EvaluatePathOrUrl(symbol_va_node.arguments[1]);

    // Try to catch invalid image file here since it is a pain to handle on the Java side
    if( !Encoders::IsDataOrHttpUrl(*image_url_or_file_path) &&
        !PortableFunctions::FileIsRegular(*image_url_or_file_path) )
    {
        IssueMessage(MessageType::Error, MGF::cannot_open_file_2001, image_url_or_file_path->c_str());
        return 0;
    }

    return map_ui->SetMarkerImage(marker_id, *image_url_or_file_path);
}


double LogicInterpreter::ex_Map_setMarkerText(const int program_index)
{
    const auto& symbol_va_node = GetNode<Nodes::SymbolVariableArguments>(program_index);
    LogicMap& logic_map = GetSymbolLogicMap(symbol_va_node.symbol_index);
    IMapUI* const map_ui = logic_map.GetMapUI();

    if( map_ui == nullptr )
        return 0;

    const int marker_id = Evaluate<int>(symbol_va_node.arguments[0]);
    SharableString text = Evaluate<SharableString>(symbol_va_node.arguments[1]);

    auto evaluate_and_get_color = [&](const int argument_index, const int default_color)
    {
        if( symbol_va_node.arguments[argument_index] >= 0 )
        {
            const SharableString color_string = Evaluate<SharableString>(symbol_va_node.arguments[argument_index]);
            const std::optional<PortableColor> portable_color = PortableColor::FromString(*color_string);

            if( portable_color.has_value() )
                return portable_color->ToColorInt();

            IssueMessage(MessageType::Error, MGF::color_invalid_2036, color_string->c_str());
        }

        return default_color;
    };

    const int background_color = evaluate_and_get_color(2, 0xFFFFFFFF); // default background color is white
    const int text_color = evaluate_and_get_color(3, 0xFF000000); // default text color is black

    return map_ui->SetMarkerText(marker_id, std::move(text), background_color, text_color);
}


double LogicInterpreter::ex_Map_setMarkerOnClick_setMarkerOnClickInfo(const int program_index)
{
    const auto& symbol_va_node = GetNode<Nodes::SymbolVariableArguments>(program_index);
    LogicMap& logic_map = GetSymbolLogicMap(symbol_va_node.symbol_index);
    IMapUI* const map_ui = logic_map.GetMapUI();

    if( map_ui == nullptr )
        return 0;

    const int marker_id = Evaluate<int>(symbol_va_node.arguments[0]);

    const int callback_index = logic_map.AddCallback(EvaluateArgumentsForCallbackUserFunction(symbol_va_node.arguments[1],
                                                                                              FunctionCode::MAPFN_SHOW_CODE));

    return ( symbol_va_node.function_code == FunctionCode::MAPFN_SET_MARKER_ON_CLICK_CODE ) ? map_ui->SetMarkerOnClick(marker_id, callback_index) :
                                                                                              map_ui->SetMarkerOnClickInfoWindow(marker_id, callback_index);
}


double LogicInterpreter::ex_Map_setMarkerDescription(const int program_index)
{
    const auto& symbol_va_node = GetNode<Nodes::SymbolVariableArguments>(program_index);
    LogicMap& logic_map = GetSymbolLogicMap(symbol_va_node.symbol_index);
    IMapUI* const map_ui = logic_map.GetMapUI();

    if( map_ui == nullptr )
        return 0;

    const int marker_id = Evaluate<int>(symbol_va_node.arguments[0]);
    SharableString description = Evaluate<SharableString>(symbol_va_node.arguments[1]);

    return map_ui->SetMarkerDescription(marker_id, std::move(description));
}


double LogicInterpreter::ex_Map_setMarkerOnDrag(const int program_index)
{
    const auto& symbol_va_node = GetNode<Nodes::SymbolVariableArguments>(program_index);
    LogicMap& logic_map = GetSymbolLogicMap(symbol_va_node.symbol_index);
    IMapUI* const map_ui = logic_map.GetMapUI();

    if( map_ui == nullptr )
        return 0;

    const int marker_id = Evaluate<int>(symbol_va_node.arguments[0]);

    const int callback_index = logic_map.AddCallback(EvaluateArgumentsForCallbackUserFunction(symbol_va_node.arguments[1],
                                                                                              FunctionCode::MAPFN_SHOW_CODE));
    return map_ui->SetMarkerOnDrag(marker_id, callback_index);
}


double LogicInterpreter::ex_Map_setMarkerLocation(const int program_index)
{
    const auto& symbol_va_node = GetNode<Nodes::SymbolVariableArguments>(program_index);
    LogicMap& logic_map = GetSymbolLogicMap(symbol_va_node.symbol_index);
    IMapUI* const map_ui = logic_map.GetMapUI();

    if( map_ui == nullptr )
        return 0;

    const int marker_id = Evaluate<int>(symbol_va_node.arguments[0]);
    const double latitude = Evaluate<double>(symbol_va_node.arguments[1]);
    const double longitude = Evaluate<double>(symbol_va_node.arguments[2]);

    return map_ui->SetMarkerLocation(marker_id, latitude, longitude);
}


double LogicInterpreter::ex_Map_getMarkerLatitude_getMarkerLongitude(const int program_index)
{
    const auto& symbol_va_node = GetNode<Nodes::SymbolVariableArguments>(program_index);
    LogicMap& logic_map = GetSymbolLogicMap(symbol_va_node.symbol_index);
    IMapUI* const map_ui = logic_map.GetMapUI();

    if( map_ui == nullptr )
        return 0;

    const int marker_id = Evaluate<int>(symbol_va_node.arguments[0]);
    const std::optional<std::tuple<double, double>> latitude_longitude = map_ui->GetMarkerLocation(marker_id);

    return ( !latitude_longitude.has_value() )                                              ? DEFAULT :
           ( symbol_va_node.function_code == FunctionCode::MAPFN_GET_MARKER_LATITUDE_CODE ) ? std::get<0>(*latitude_longitude):
                                                                                              std::get<1>(*latitude_longitude);
}


double LogicInterpreter::ex_Map_removeMarker(const int program_index)
{
    const auto& symbol_va_node = GetNode<Nodes::SymbolVariableArguments>(program_index);
    LogicMap& logic_map = GetSymbolLogicMap(symbol_va_node.symbol_index);
    IMapUI* const map_ui = logic_map.GetMapUI();

    if( map_ui == nullptr )
        return 0;

    const int marker_id = Evaluate<int>(symbol_va_node.arguments[0]);

    return map_ui->RemoveMarker(marker_id);
}


double LogicInterpreter::ex_Map_showCurrentLocation(const int program_index)
{
    const auto& symbol_va_node = GetNode<Nodes::SymbolVariableArguments>(program_index);
    LogicMap& logic_map = GetSymbolLogicMap(symbol_va_node.symbol_index);
    IMapUI* const map_ui = logic_map.GetMapUI();

    if( map_ui == nullptr )
        return 0;

    const bool show = EvaluateConditional(symbol_va_node.arguments[0]);

    return map_ui->SetShowCurrentLocation(show);
}


double LogicInterpreter::ex_Map_addTextButton(const int program_index)
{
    const auto& symbol_va_node = GetNode<Nodes::SymbolVariableArguments>(program_index);
    LogicMap& logic_map = GetSymbolLogicMap(symbol_va_node.symbol_index);
    IMapUI* const map_ui = logic_map.GetMapUI();

    if( map_ui == nullptr )
        return 0;

    SharableString label = Evaluate<SharableString>(symbol_va_node.arguments[0]);

    const int callback_index = logic_map.AddCallback(EvaluateArgumentsForCallbackUserFunction(symbol_va_node.arguments[1],
                                                                                              FunctionCode::MAPFN_SHOW_CODE));
    return map_ui->AddTextButton(std::move(label), callback_index);
}


double LogicInterpreter::ex_Map_addImageButton(const int program_index)
{
    const auto& symbol_va_node = GetNode<Nodes::SymbolVariableArguments>(program_index);
    LogicMap& logic_map = GetSymbolLogicMap(symbol_va_node.symbol_index);
    IMapUI* const map_ui = logic_map.GetMapUI();

    if( map_ui == nullptr )
        return 0;

    const SharableString image_url_or_file_path = EvaluatePathOrUrl(symbol_va_node.arguments[0]);

    // Try to catch invalid image file here since it is a pain to handle on the Java side
    if( !Encoders::IsDataOrHttpUrl(*image_url_or_file_path) &&
        !PortableFunctions::FileIsRegular(*image_url_or_file_path) )
    {
        IssueMessage(MessageType::Error, MGF::cannot_open_file_2001, image_url_or_file_path->c_str());
        return 0;
    }

    const int callback_index = logic_map.AddCallback(EvaluateArgumentsForCallbackUserFunction(symbol_va_node.arguments[1],
                                                                                              FunctionCode::MAPFN_SHOW_CODE));
    return map_ui->AddImageButton(*image_url_or_file_path, callback_index);
}


double LogicInterpreter::ex_Map_removeButton(const int program_index)
{
    const auto& symbol_va_node = GetNode<Nodes::SymbolVariableArguments>(program_index);
    LogicMap& logic_map = GetSymbolLogicMap(symbol_va_node.symbol_index);
    IMapUI* const map_ui = logic_map.GetMapUI();

    if( map_ui == nullptr )
        return 0;

    const int button_id = Evaluate<int>(symbol_va_node.arguments[0]);

    return map_ui->RemoveButton(button_id);
}


template<typename T>
bool LogicInterpreter::SetBaseMap(LogicMap& logic_map, T base_map_selection)
{
    IMapUI* const map_ui = logic_map.GetMapUI();

    if( map_ui == nullptr )
        return false;

    try
    {
        // if a file, try to catch an invalid map file here since it is a pain to handle on the Java side
        if( std::holds_alternative<std::string>(base_map_selection) &&
            !PortableFunctions::FileIsRegular(std::get<std::string>(base_map_selection)) )
        {
            throw CSProException(MGF::GetMessageText(MGF::cannot_open_file_2001)->c_str(), std::get<std::string>(base_map_selection).c_str());
        }

        return map_ui->SetBaseMap(std::move(base_map_selection));
    }

    catch( const CSProException& exception )
    {
        IssueMessage(MessageType::Error, MGF::Map_base_map_error_94205, logic_map.GetName().c_str(), exception.what());
        return false;
    }
}


double LogicInterpreter::ex_Map_setBaseMap(const int program_index)
{
    const auto& symbol_va_node = GetNode<Nodes::SymbolVariableArguments>(program_index);
    LogicMap& logic_map = GetSymbolLogicMap(symbol_va_node.symbol_index);
    const int& base_map_type = symbol_va_node.arguments[0];
    const int& custom_source_expression = symbol_va_node.arguments[1];

    if( base_map_type > 0 )
    {
        return SetBaseMap(logic_map, BaseMapSelection(static_cast<BaseMap>(base_map_type)));
    }

    else
    {
        const SharableString base_map_text = Evaluate<SharableString>(custom_source_expression);
        return SetBaseMap(logic_map, FromString(*base_map_text, GetCurrentApplicationFilePath()));
    }
}


double LogicInterpreter::ex_Map_setTitle(const int program_index)
{
    const auto& symbol_va_node = GetNode<Nodes::SymbolVariableArguments>(program_index);
    LogicMap& logic_map = GetSymbolLogicMap(symbol_va_node.symbol_index);
    IMapUI* const map_ui = logic_map.GetMapUI();

    if( map_ui == nullptr )
        return 0;

    return map_ui->SetTitle(Evaluate<SharableString>(symbol_va_node.arguments[0]));
}


double LogicInterpreter::ex_Map_zoomTo(const int program_index)
{
    const auto& symbol_va_node = GetNode<Nodes::SymbolVariableArguments>(program_index);
    LogicMap& logic_map = GetSymbolLogicMap(symbol_va_node.symbol_index);
    IMapUI* const map_ui = logic_map.GetMapUI();

    if( map_ui == nullptr )
        return 0;

    // geometry / [padding]
    if( symbol_va_node.arguments[0] == -1 )
    {
        ASSERT(m_engineData->MeetsCompiledLogicVersion(Serializer::Iteration_8_1_000_1));

        const LogicGeometry* const logic_geometry = GetFromSymbolOrEngineItem<LogicGeometry*>(
            symbol_va_node.arguments[1],
            symbol_va_node.arguments[2]
        );

        if( logic_geometry == nullptr ||
            !EnsureGeometryExistsAndHasValidContent(*logic_geometry, "use it as the bounds for a map") )
        {
            return 0;
        }

        const Geometry::BoundingBox& bounding_box = logic_geometry->GetBoundingBox();

        return map_ui->ZoomTo(
            bounding_box.min.y,
            bounding_box.min.x,
            bounding_box.max.y,
            bounding_box.max.x,
            EvaluateOptional<double>(symbol_va_node.arguments[3], 0) / 100
        );
    }

    // latitude / longitude / [zoom]
    else if( symbol_va_node.arguments[3] == -1 )
    {
        return map_ui->ZoomTo(
            Evaluate<double>(symbol_va_node.arguments[0]),
            Evaluate<double>(symbol_va_node.arguments[1]),
            EvaluateOptional<double>(symbol_va_node.arguments[2], -1)
        );
    }

    // min latitude / min longitude / max latitude / max longitude / [padding]
    else
    {
        return map_ui->ZoomTo(
            Evaluate<double>(symbol_va_node.arguments[0]),
            Evaluate<double>(symbol_va_node.arguments[1]),
            Evaluate<double>(symbol_va_node.arguments[2]),
            Evaluate<double>(symbol_va_node.arguments[3]),
            EvaluateOptional<double>(symbol_va_node.arguments[4], 0) / 100
        );
    }
}


double LogicInterpreter::ex_Map_setOnClick(const int program_index)
{
    const auto& symbol_va_node = GetNode<Nodes::SymbolVariableArguments>(program_index);
    LogicMap& logic_map = GetSymbolLogicMap(symbol_va_node.symbol_index);

    const int callback_index = logic_map.AddCallback(EvaluateArgumentsForCallbackUserFunction(symbol_va_node.arguments[0],
                                                                                              FunctionCode::MAPFN_SHOW_CODE));
    logic_map.SetOnClickCallbackId(callback_index);

    return 1;
}


double LogicInterpreter::ex_Map_clear_clearButtons_clearGeometry_clearMarkers(const int program_index)
{
    const auto& symbol_va_node = GetNode<Nodes::SymbolVariableArguments>(program_index);
    LogicMap& logic_map = GetSymbolLogicMap(symbol_va_node.symbol_index);
    IMapUI* const map_ui = logic_map.GetMapUI();

    if( map_ui == nullptr )
        return 0;

    switch( symbol_va_node.function_code )
    {
        case FunctionCode::MAPFN_CLEAR_CODE:
            map_ui->Clear();
            break;

        case FunctionCode::MAPFN_CLEAR_BUTTONS_CODE:
            map_ui->ClearButtons();
            break;

        case FunctionCode::MAPFN_CLEAR_GEOMETRY_CODE:
            map_ui->ClearGeometry();
            break;

        default:
            ASSERT(symbol_va_node.function_code == FunctionCode::MAPFN_CLEAR_MARKERS_CODE);
            map_ui->ClearMarkers();
            break;
    }

    return 1;
}


double LogicInterpreter::ex_Map_getLastClickLatitude_getLastClickLongitude(const int program_index)
{
    const auto& symbol_va_node = GetNode<Nodes::SymbolVariableArguments>(program_index);
    const LogicMap& logic_map = GetSymbolLogicMap(symbol_va_node.symbol_index);

    return ( symbol_va_node.function_code == FunctionCode::MAPFN_GET_LAST_CLICK_LATITUDE_CODE ) ? logic_map.GetLastClickLatitude() :
                                                                                                  logic_map.GetLastClickLongitude();
}


double LogicInterpreter::ex_Map_addGeometry(const int program_index)
{
    const auto& symbol_va_node = GetNode<Nodes::SymbolVariableArguments>(program_index);
    LogicMap& logic_map = GetSymbolLogicMap(symbol_va_node.symbol_index);
    IMapUI* const map_ui = logic_map.GetMapUI();

    if( map_ui == nullptr )
        return 0;

    const LogicGeometry* const logic_geometry = GetFromSymbolOrEngineItem<LogicGeometry*>(symbol_va_node.arguments[0],
                                                                                          m_engineData->MeetsCompiledLogicVersion(Serializer::Iteration_8_0_000_1) ? symbol_va_node.arguments[1] : -1);

    if( logic_geometry == nullptr || !EnsureGeometryExistsAndHasValidContent(*logic_geometry, "add it to a map") )
        return 0;

    return map_ui->AddGeometry(logic_geometry->GetSharedFeatures(), logic_geometry->GetSharedBoundingBox());
}


double LogicInterpreter::ex_Map_removeGeometry(const int program_index)
{
    const auto& symbol_va_node = GetNode<Nodes::SymbolVariableArguments>(program_index);
    LogicMap& logic_map = GetSymbolLogicMap(symbol_va_node.symbol_index);
    IMapUI* const map_ui = logic_map.GetMapUI();

    if( map_ui == nullptr )
        return 0;

    const int geometry_id = Evaluate<int>(symbol_va_node.arguments[0]);

    return map_ui->RemoveGeometry(geometry_id);
}


double LogicInterpreter::ex_Map_saveSnapshot(const int program_index)
{
    const auto& symbol_va_node = GetNode<Nodes::SymbolVariableArguments>(program_index);
    LogicMap& logic_map = GetSymbolLogicMap(symbol_va_node.symbol_index);
    IMapUI* const map_ui = logic_map.GetMapUI();

    if( map_ui == nullptr )
        return 0;

    try
    {
        if( !logic_map.IsShowing() )
            throw CSProException("the map must be showing before you can save a snapshot");

        const std::string image_file_path = EvaluatePath(symbol_va_node.arguments[0]);

        return map_ui->SaveSnapshot(image_file_path);
    }

    catch( const CSProException& exception )
    {
        IssueMessage(MessageType::Error, MGF::Map_snapshot_error_94206, logic_map.GetName().c_str(), exception.what());
        return 0;
    }
}
