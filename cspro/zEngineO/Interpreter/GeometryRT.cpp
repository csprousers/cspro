#include "stdafx.h"
#include "IncludesRT.h"
#include "Document.h"
#include "Geometry.h"
#include "Map.h"
#include <zPlatformO/PlatformInterface.h>

#pragma warning(push)
#pragma warning(disable: 4068 4239)
#include <mapbox/geometry.hpp>
#pragma warning(pop)


bool LogicInterpreter::EnsureGeometryExistsAndHasValidContent(const LogicGeometry& logic_geometry, const char* const action_for_displayed_error_message)
{
    if( !logic_geometry.HasContent() )
    {
        if( action_for_displayed_error_message != nullptr )
        {
            IssueMessage(MessageType::Error, MGF::Geometry_no_geometry_for_action_100350,
                                             logic_geometry.GetName().c_str(), action_for_displayed_error_message);
        }

        return false;
    }

    else if( !logic_geometry.HasValidContent(true) )
    {
        if( action_for_displayed_error_message != nullptr )
        {
            IssueMessage(MessageType::Error, MGF::Geometry_invalid_geometry_for_action_100351,
                                             logic_geometry.GetName().c_str(), action_for_displayed_error_message);
        }

        return false;
    }

    return true;
}


double LogicInterpreter::ex_Geometry_compute(const int program_index)
{
    const auto& symbol_compute_with_subscript_node = GetOrConvertPre80SymbolComputeWithSubscriptNode(program_index);
    const SymbolReference<Symbol*> lhs_symbol_reference = EvaluateSymbolReference<Symbol*>(symbol_compute_with_subscript_node.lhs_symbol_index,
                                                                                           symbol_compute_with_subscript_node.lhs_subscript_compilation);
    const Symbol* const rhs_symbol = GetFromSymbolOrEngineItem<Symbol*>(symbol_compute_with_subscript_node.rhs_symbol_index, symbol_compute_with_subscript_node.rhs_subscript_compilation);

    if( rhs_symbol == nullptr )
        return 0;

    LogicGeometry* const lhs_logic_geometry = GetFromSymbolOrEngineItem<LogicGeometry*>(lhs_symbol_reference);

    if( lhs_logic_geometry == nullptr )
        return 0;

    try
    {
        if( rhs_symbol->IsA(SymbolType::Geometry) )
        {
            *lhs_logic_geometry = assert_cast<const LogicGeometry&>(*rhs_symbol);
        }

        else if( rhs_symbol->IsA(SymbolType::Document) )
        {
            *lhs_logic_geometry = assert_cast<const LogicDocument&>(*rhs_symbol);
        }

        else
        {
            ASSERT(false);
        }
    }

    catch( const CSProException& exception )
    {
        IssueMessage(MessageType::Error, MGF::Geometry_assignment_error_100354,
                                         rhs_symbol->GetName().c_str(), lhs_logic_geometry->GetName().c_str(), exception.what());
    }

    return 0;
}


double LogicInterpreter::ex_Geometry_clear(const int program_index)
{
    const auto& symbol_va_with_subscript_node = GetOrConvertPre80SymbolVariableArgumentsWithSubscriptNode(program_index);
    LogicGeometry* const logic_geometry = GetFromSymbolOrEngineItem<LogicGeometry*>(symbol_va_with_subscript_node.symbol_index,
                                                                                    symbol_va_with_subscript_node.subscript_compilation);

    if( logic_geometry == nullptr )
        return 0;

    logic_geometry->Reset();

    return 1;
}


double LogicInterpreter::ex_Geometry_load(const int program_index)
{
    const auto& symbol_va_with_subscript_node = GetOrConvertPre80SymbolVariableArgumentsWithSubscriptNode(program_index);
    const std::string file_path = EvaluatePath(symbol_va_with_subscript_node.arguments[0]);
    LogicGeometry* const logic_geometry = GetFromSymbolOrEngineItem<LogicGeometry*>(symbol_va_with_subscript_node.symbol_index,
                                                                                    symbol_va_with_subscript_node.subscript_compilation);

    if( logic_geometry == nullptr )
        return 0;

    try
    {
        logic_geometry->Load(file_path);
    }

    catch( const CSProException& exception )
    {
        IssueMessage(MessageType::Error, MGF::Geometry_load_error_100352,
                                         file_path.c_str(), exception.what());
        return 0;
    }

    return 1;
}


double LogicInterpreter::ex_Geometry_save(const int program_index)
{
    const auto& symbol_va_with_subscript_node = GetOrConvertPre80SymbolVariableArgumentsWithSubscriptNode(program_index);
    const std::string file_path = EvaluatePath(symbol_va_with_subscript_node.arguments[0]);
    LogicGeometry* const logic_geometry = GetFromSymbolOrEngineItem<LogicGeometry*>(symbol_va_with_subscript_node.symbol_index,
                                                                                    symbol_va_with_subscript_node.subscript_compilation);

    if( logic_geometry == nullptr || !EnsureGeometryExistsAndHasValidContent(*logic_geometry, "save the geometry") )
        return DEFAULT;

    try
    {
        logic_geometry->Save(file_path);
    }

    catch( const CSProException& exception )
    {
        IssueMessage(MessageType::Error, MGF::Geometry_save_error_100353,
                                         file_path.c_str(), exception.what());
        return 0;
    }

    return 1;
}


double LogicInterpreter::ex_Geometry_tracePolygon_walkPolygon(const int program_index)
{
    const auto& symbol_va_with_subscript_node = GetOrConvertPre80SymbolVariableArgumentsWithSubscriptNode(program_index);
    const bool trace_polgyon = ( symbol_va_with_subscript_node.function_code == FunctionCode::GEOMETRYFN_TRACE_POLYGON_CODE );
    IMapUI* map_ui;

    if( symbol_va_with_subscript_node.arguments[0] == -1 )
    {
        map_ui = nullptr;
    }

    else
    {
        LogicMap& logic_map = GetSymbolLogicMap(symbol_va_with_subscript_node.arguments[0]);
        map_ui = logic_map.GetMapUI();
    }

    LogicGeometry* const logic_geometry = GetFromSymbolOrEngineItem<LogicGeometry*>(symbol_va_with_subscript_node.symbol_index,
                                                                                    symbol_va_with_subscript_node.subscript_compilation);

    if( logic_geometry == nullptr )
        return 0;

    std::unique_ptr<Geometry::Polygon> captured_polygon;

#ifndef WIN_DESKTOP
    if( trace_polgyon )
    {
        PlatformInterface::GetInstance()->GetApplicationInterface()->CapturePolygonTrace(captured_polygon, logic_geometry->GetFirstPolygon(), map_ui);
    }

    else
    {
        ASSERT(symbol_va_with_subscript_node.function_code == FunctionCode::GEOMETRYFN_WALK_POLYGON_CODE);
        PlatformInterface::GetInstance()->GetApplicationInterface()->CapturePolygonWalk(captured_polygon, logic_geometry->GetFirstPolygon(), map_ui);
    }
#endif

    if( captured_polygon != nullptr )
    {
        BinaryDataMetadata binary_data_metadata;
        binary_data_metadata.SetProperty("label", trace_polgyon ? "Polygon (Traced)" : "Polygon (Walked)");
        binary_data_metadata.SetProperty("source", trace_polgyon ? "Geometry.tracePolygon" : "Geometry.walkPolygon");
        binary_data_metadata.SetProperty("timestamp", GetTimestamp<double>());

        logic_geometry->SetGeometry(std::move(*captured_polygon), std::move(binary_data_metadata));

        return 1;
    }

    return 0;
}


double LogicInterpreter::ex_Geometry_area_perimeter(const int program_index)
{
    const auto& symbol_va_with_subscript_node = GetOrConvertPre80SymbolVariableArgumentsWithSubscriptNode(program_index);
    const LogicGeometry* const logic_geometry = GetFromSymbolOrEngineItem<LogicGeometry*>(symbol_va_with_subscript_node.symbol_index,
                                                                                          symbol_va_with_subscript_node.subscript_compilation);

    if( logic_geometry == nullptr || !EnsureGeometryExistsAndHasValidContent(*logic_geometry, nullptr) )
        return DEFAULT;

    return ( symbol_va_with_subscript_node.function_code == FunctionCode::GEOMETRYFN_AREA_CODE ) ? logic_geometry->Area() :
                                                                                                   logic_geometry->Perimeter();
}


double LogicInterpreter::ex_Geometry_minLatitude_maxLatitude_minLongitude_maxLongitude(const int program_index)
{
    const auto& symbol_va_with_subscript_node = GetOrConvertPre80SymbolVariableArgumentsWithSubscriptNode(program_index);
    const LogicGeometry* const logic_geometry = GetFromSymbolOrEngineItem<LogicGeometry*>(symbol_va_with_subscript_node.symbol_index,
                                                                                          symbol_va_with_subscript_node.subscript_compilation);

    if( logic_geometry == nullptr || !EnsureGeometryExistsAndHasValidContent(*logic_geometry, nullptr) )
        return DEFAULT;

    const Geometry::BoundingBox& bounding_box = logic_geometry->GetBoundingBox();

    return ( symbol_va_with_subscript_node.function_code == FunctionCode::GEOMETRYFN_MIN_LATITUDE_CODE )  ?   bounding_box.min.y :
           ( symbol_va_with_subscript_node.function_code == FunctionCode::GEOMETRYFN_MAX_LATITUDE_CODE )  ?   bounding_box.max.y :
           ( symbol_va_with_subscript_node.function_code == FunctionCode::GEOMETRYFN_MIN_LONGITUDE_CODE ) ?   bounding_box.min.x :
         /*( symbol_va_with_subscript_node.function_code == FunctionCode::GEOMETRYFN_MAX_LONGITUDE_CODE ) ? */bounding_box.max.x;
}


double LogicInterpreter::ex_Geometry_getProperty(const int program_index)
{
    const auto& symbol_va_with_subscript_node = GetOrConvertPre80SymbolVariableArgumentsWithSubscriptNode(program_index);
    const SharableString property_name = EvaluateSharableString(symbol_va_with_subscript_node.arguments[0]);
    const LogicGeometry* const logic_geometry = GetFromSymbolOrEngineItem<LogicGeometry*>(symbol_va_with_subscript_node.symbol_index,
                                                                                          symbol_va_with_subscript_node.subscript_compilation);

    if( logic_geometry == nullptr || !EnsureGeometryExistsAndHasValidContent(*logic_geometry, nullptr) )
        return AssignStringNull();

    return AssignString(logic_geometry->GetProperty(*property_name));
}


double LogicInterpreter::ex_Geometry_setProperty(const int program_index)
{
    const auto& symbol_va_with_subscript_node = GetOrConvertPre80SymbolVariableArgumentsWithSubscriptNode(program_index);
    const SharableString property_name = EvaluateSharableString(symbol_va_with_subscript_node.arguments[0]);
    const std::variant<double, SharableString> property_value = EvaluateVariant(static_cast<DataType>(symbol_va_with_subscript_node.arguments[1]),
                                                                                symbol_va_with_subscript_node.arguments[2]);
    LogicGeometry* const logic_geometry = GetFromSymbolOrEngineItem<LogicGeometry*>(symbol_va_with_subscript_node.symbol_index,
                                                                                    symbol_va_with_subscript_node.subscript_compilation);

    if( logic_geometry == nullptr || !EnsureGeometryExistsAndHasValidContent(*logic_geometry, "set property values") )
        return 0;

    logic_geometry->SetProperty(*property_name, property_value);

    return 1;
}
