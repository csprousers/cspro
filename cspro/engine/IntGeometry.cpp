#include "StandardSystemIncludes.h"
#include "INTERPRE.H"
#include <zEngineO/Document.h>
#include <zEngineO/Geometry.h>
#include <zEngineO/Map.h>
#include <zPlatformO/PlatformInterface.h>
#include <sstream>

#pragma warning(push)
#pragma warning(disable: 4068 4239)
#include <mapbox/geometry.hpp>
#pragma warning(pop)


bool CIntDriver::EnsureGeometryExistsAndHasValidContent(const LogicGeometry& logic_geometry, const TCHAR* action_for_displayed_error_message)
{
    if( !logic_geometry.HasContent() )
    {
        if( action_for_displayed_error_message != nullptr )
            issaerror(MessageType::Error, 100350, logic_geometry.GetName().c_str(), action_for_displayed_error_message);

        return false;
    }

    else if( !logic_geometry.HasValidContent(true) )
    {
        if( action_for_displayed_error_message != nullptr )
            issaerror(MessageType::Error, 100351, logic_geometry.GetName().c_str(), action_for_displayed_error_message);

        return false;
    }

    return true;
}


double CIntDriver::exGeometry_compute(int program_index)
{
    const auto& symbol_compute_with_subscript_node = GetOrConvertPre80SymbolComputeWithSubscriptNode(program_index);
    const SymbolReference<Symbol*> lhs_symbol_reference = EvaluateSymbolReference<Symbol*>(symbol_compute_with_subscript_node.lhs_symbol_index, symbol_compute_with_subscript_node.lhs_subscript_compilation);
    const Symbol* rhs_symbol = GetFromSymbolOrEngineItem<Symbol*>(symbol_compute_with_subscript_node.rhs_symbol_index, symbol_compute_with_subscript_node.rhs_subscript_compilation);

    if( rhs_symbol == nullptr )
        return 0;

    LogicGeometry* lhs_logic_geometry = GetFromSymbolOrEngineItem<LogicGeometry*>(lhs_symbol_reference);

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
        issaerror(MessageType::Error, 100354, rhs_symbol->GetName().c_str(), lhs_logic_geometry->GetName().c_str(), exception.GetErrorMessage().c_str());
    }

    return 0;
}


double CIntDriver::exGeometry_clear(int program_index)
{
    const auto& symbol_va_with_subscript_node = GetOrConvertPre80SymbolVariableArgumentsWithSubscriptNode(program_index);
    LogicGeometry* logic_geometry = GetFromSymbolOrEngineItem<LogicGeometry*>(symbol_va_with_subscript_node.symbol_index, symbol_va_with_subscript_node.subscript_compilation);

    if( logic_geometry == nullptr )
        return 0;

    logic_geometry->Reset();

    return 1;
}


double CIntDriver::exGeometry_load(int program_index)
{
    const auto& symbol_va_with_subscript_node = GetOrConvertPre80SymbolVariableArgumentsWithSubscriptNode(program_index);
    const std::wstring filename = EvalFullPathFileName(symbol_va_with_subscript_node.arguments[0]);
    LogicGeometry* logic_geometry = GetFromSymbolOrEngineItem<LogicGeometry*>(symbol_va_with_subscript_node.symbol_index, symbol_va_with_subscript_node.subscript_compilation);

    if( logic_geometry == nullptr )
        return 0;

    try
    {
        logic_geometry->Load(filename);
    }

    catch( const CSProException& exception )
    {
        issaerror(MessageType::Error, 100352, filename.c_str(), exception.GetErrorMessage().c_str());
        return 0;
    }

    return 1;
}


double CIntDriver::exGeometry_save(int program_index)
{
    const auto& symbol_va_with_subscript_node = GetOrConvertPre80SymbolVariableArgumentsWithSubscriptNode(program_index);
    const std::wstring filename = EvalFullPathFileName(symbol_va_with_subscript_node.arguments[0]);
    LogicGeometry* logic_geometry = GetFromSymbolOrEngineItem<LogicGeometry*>(symbol_va_with_subscript_node.symbol_index, symbol_va_with_subscript_node.subscript_compilation);

    if( logic_geometry == nullptr || !EnsureGeometryExistsAndHasValidContent(*logic_geometry, _T("save the geometry")) )
        return DEFAULT;

    try
    {
        logic_geometry->Save(filename);
    }

    catch( const CSProException& exception )
    {
        issaerror(MessageType::Error, 100353, filename.c_str(), exception.GetErrorMessage().c_str());
        return 0;
    }

    return 1;
}


double CIntDriver::exGeometry_tracePolygon_walkPolygon(int program_index)
{
    const auto& symbol_va_with_subscript_node = GetOrConvertPre80SymbolVariableArgumentsWithSubscriptNode(program_index);
    const bool trace_polgyon = ( symbol_va_with_subscript_node.function_code == FunctionCode::GEOMETRYFN_TRACE_POLYGON_CODE );
    IMapUI* map_ui = nullptr;

    if( symbol_va_with_subscript_node.arguments[0] != -1 )
    {
        LogicMap& logic_map = GetSymbolLogicMap(symbol_va_with_subscript_node.arguments[0]);
        map_ui = logic_map.GetMapUI();
    }

    LogicGeometry* logic_geometry = GetFromSymbolOrEngineItem<LogicGeometry*>(symbol_va_with_subscript_node.symbol_index, symbol_va_with_subscript_node.subscript_compilation);

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
        logic_geometry->SetGeometry(std::move(*captured_polygon));

        BinaryDataMetadata& binary_data_metadata = logic_geometry->GetMetadataForModification();
        binary_data_metadata.SetProperty(_T("label"), trace_polgyon ? _T("Polygon (Traced)") : _T("Polygon (Walked)"));
        binary_data_metadata.SetProperty(_T("source"), trace_polgyon ? _T("Geometry.tracePolygon") : _T("Geometry.walkPolygon"));
        binary_data_metadata.SetProperty(_T("timestamp"), GetTimestamp());

        return 1;
    }

    return 0;
}


double CIntDriver::exGeometry_area_perimeter(int program_index)
{
    const auto& symbol_va_with_subscript_node = GetOrConvertPre80SymbolVariableArgumentsWithSubscriptNode(program_index);
    const LogicGeometry* logic_geometry = GetFromSymbolOrEngineItem<LogicGeometry*>(symbol_va_with_subscript_node.symbol_index, symbol_va_with_subscript_node.subscript_compilation);

    if( logic_geometry == nullptr || !EnsureGeometryExistsAndHasValidContent(*logic_geometry, nullptr) )
        return DEFAULT;

    return ( symbol_va_with_subscript_node.function_code == FunctionCode::GEOMETRYFN_AREA_CODE ) ? logic_geometry->Area() :
                                                                                                   logic_geometry->Perimeter();
}


double CIntDriver::exGeometry_minLatitude_maxLatitude_minLongitude_maxLongitude(int program_index)
{
    const auto& symbol_va_with_subscript_node = GetOrConvertPre80SymbolVariableArgumentsWithSubscriptNode(program_index);
    const LogicGeometry* logic_geometry = GetFromSymbolOrEngineItem<LogicGeometry*>(symbol_va_with_subscript_node.symbol_index, symbol_va_with_subscript_node.subscript_compilation);

    if( logic_geometry == nullptr || !EnsureGeometryExistsAndHasValidContent(*logic_geometry, nullptr) )
        return DEFAULT;

    const Geometry::BoundingBox& bounding_box = logic_geometry->GetBoundingBox();

    return ( symbol_va_with_subscript_node.function_code == FunctionCode::GEOMETRYFN_MIN_LATITUDE_CODE )  ?   bounding_box.min.y :
           ( symbol_va_with_subscript_node.function_code == FunctionCode::GEOMETRYFN_MAX_LATITUDE_CODE )  ?   bounding_box.max.y :
           ( symbol_va_with_subscript_node.function_code == FunctionCode::GEOMETRYFN_MIN_LONGITUDE_CODE ) ?   bounding_box.min.x :
         /*( symbol_va_with_subscript_node.function_code == FunctionCode::GEOMETRYFN_MAX_LONGITUDE_CODE ) ? */bounding_box.max.x;
}


double CIntDriver::exGeometry_getProperty(int program_index)
{
    const auto& symbol_va_with_subscript_node = GetOrConvertPre80SymbolVariableArgumentsWithSubscriptNode(program_index);
    const std::wstring property_name = EvalAlphaExpr(symbol_va_with_subscript_node.arguments[0]);
    const LogicGeometry* logic_geometry = GetFromSymbolOrEngineItem<LogicGeometry*>(symbol_va_with_subscript_node.symbol_index, symbol_va_with_subscript_node.subscript_compilation);

    if( logic_geometry == nullptr || !EnsureGeometryExistsAndHasValidContent(*logic_geometry, nullptr) )
        return AssignBlankAlphaValue();

    return AssignAlphaValue(logic_geometry->GetProperty(property_name));
}


double CIntDriver::exGeometry_setProperty(int program_index)
{
    const auto& symbol_va_with_subscript_node = GetOrConvertPre80SymbolVariableArgumentsWithSubscriptNode(program_index);
    const std::wstring property_name = EvalAlphaExpr(symbol_va_with_subscript_node.arguments[0]);
    const std::variant<double, std::wstring> property_value = EvaluateVariantExpression(static_cast<DataType>(symbol_va_with_subscript_node.arguments[1]), symbol_va_with_subscript_node.arguments[2]);
    LogicGeometry* logic_geometry = GetFromSymbolOrEngineItem<LogicGeometry*>(symbol_va_with_subscript_node.symbol_index, symbol_va_with_subscript_node.subscript_compilation);

    if( logic_geometry == nullptr || !EnsureGeometryExistsAndHasValidContent(*logic_geometry, _T("set property values")) )
        return 0;

    logic_geometry->SetProperty(property_name, property_value);

    return 1;
}
