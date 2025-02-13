#include "stdafx.h"
#include "Geometry.h"
#include "Document.h"
#include <zToolsO/MemoryStream.h>
#include <zToolsO/VariantVisitOverload.h>
#include <zMapping/GeoJson.h>
#include <zMapping/GreatCircle.h>
#include <fstream>

#pragma warning(push)
#pragma warning(disable: 4068 4239)
#include <mapbox/feature.hpp>
#include <mapbox/geometry.hpp>
#include <mapbox/variant.hpp>
#pragma warning(pop)

#define _USE_MATH_DEFINES
#include <math.h>


namespace
{
    constexpr double DegreesToRadians(double num)
    {
        return ( num * M_PI ) / 180;
    }


    double SignedRingArea(const Geometry::Polygon::linear_ring_type& ring)
    {
        double area = 0;
        const int num_points = ring.size();

        if (num_points > 2) {
            for (int i = 0; i < num_points; ++i) {
                const auto& p1 = ring[i];
                const auto& p2 = ring[(i + 1) % num_points];
                const auto& p3 = ring[(i + 2) % num_points];
                area += (DegreesToRadians(p1.x) - DegreesToRadians(p3.x)) * std::sin(DegreesToRadians(p2.y));
            }

            area = (area * GreatCircle::EARTH_RADIUS_METERS * GreatCircle::EARTH_RADIUS_METERS) / 2;
        }
        return area;
    }


    double PolygonArea(const Geometry::Polygon& poly)
    {
        // Outer ring
        double total = abs(SignedRingArea(poly.front()));

        // Inner rings are holes so area is subtracted
        for (auto i = std::begin(poly) + 1; i != std::end(poly); ++i)
            total -= abs(SignedRingArea(*i));

        return total;
    }


    double LineStringLength(const Geometry::LineString& points)
    {
        double length = 0;
        for (size_t i = 0; i < points.size() - 1; ++i)
            length += GreatCircle::Distance(points[i].y, points[i].x, points[i+1].y, points[i+1].x);

        return length;
    }


    double PolygonPerimeter(const Geometry::Polygon& poly)
    {
        return LineStringLength(poly.front());
    }


    Geometry::BoundingBox FeatureCollectionBounds(const Geometry::FeatureCollection& collection)
    {
        using limits = std::numeric_limits<Geometry::CoordinateType>;
        Geometry::Point min(limits::max(), limits::max());
        Geometry::Point max(limits::lowest(), limits::lowest());

        for (const Geometry::Feature& feature : collection) {
            mapbox::geometry::for_each_point(feature.geometry, [&](Geometry::Point const& point) {
                if (min.x > point.x) min.x = point.x;
                if (min.y > point.y) min.y = point.y;
                if (max.x < point.x) max.x = point.x;
                if (max.y < point.y) max.y = point.y;
            });
        }

        return Geometry::BoundingBox(min, max);
    }


    template<typename F>
    double MeasurePolygons(const Geometry::Geometry& geom, F& measure_fun)
    {
        return geom.match(
            [](mapbox::geometry::empty) { return 0.0; },
            [](const Geometry::Point&) { return 0.0; },
            [](const Geometry::MultiPoint&) { return 0.0; },
            [](const Geometry::LineString&) {return 0.0; },
            [](const Geometry::MultiLineString&) {return 0.0; },
            [&measure_fun](const Geometry::Polygon& geom) {
                return measure_fun(geom);
            },
            [&measure_fun](const Geometry::MultiPolygon& geom) {
                double total = 0;
                for (const auto& poly : geom)
                    total += measure_fun(poly);
                return total;
            },
            [&measure_fun](const Geometry::GeometryCollection& geom) {
                double total = 0;
                for (const auto& g : geom)
                    total += MeasurePolygons(g, measure_fun);
                return total;
            });
    }


    template <typename F>
    double MeasurePolygons(const Geometry::FeatureCollection& features, F measure_fun)
    {
        double total = 0;
        for (const auto& feature : features)
            total += MeasurePolygons(feature.geometry, measure_fun);
        return total;
    }


    double GeometryArea(const Geometry::FeatureCollection& features)
    {
        return MeasurePolygons(features, PolygonArea);
    }


    double GeometryPerimeter(const Geometry::FeatureCollection& features)
    {
        return MeasurePolygons(features, PolygonPerimeter);
    }


    void FixPolygonWindingOrder(Geometry::Polygon& poly)
    {
        if (poly.empty()) {
            return;
        }

        // Area of outer ring is positive if vertices are counter-clockwise
        if (SignedRingArea(poly[0]) < 0) {
            std::reverse(std::begin(poly[0]), std::end(poly[0]));
        }

        // Inner rings (holes) should be clockwise so area should be negative
        for (auto i = std::begin(poly) + 1; i != std::end(poly); ++i) {
            if (SignedRingArea(*i) > 0) {
                std::reverse(std::begin(*i), std::end(*i));
            }
        }
    }


    void FixWindingOrder(Geometry::Geometry& geom)
    {
        return geom.match(
            [](mapbox::geometry::empty) { },
            [](Geometry::Point&) {},
            [](Geometry::MultiPoint&) { },
            [](Geometry::LineString&) { },
            [](Geometry::MultiLineString&) { },
            [](Geometry::Polygon& geom) {
                return FixPolygonWindingOrder(geom);
            },
            [](Geometry::MultiPolygon& geom) {
                for (auto& poly : geom)
                    FixPolygonWindingOrder(poly);
            },
            [](Geometry::GeometryCollection& geom) {
                for (auto& g : geom)
                    FixWindingOrder(g);
            });
    }


    void FixWindingOrder(Geometry::FeatureCollection& features)
    {
        for (auto& feature : features)
            FixWindingOrder(feature.geometry);
    }


    Geometry::Polygon* GetFirstPolygon(Geometry::Geometry& geom)
    {
        return geom.match(
                [](mapbox::geometry::empty) -> Geometry::Polygon* { return nullptr; },
                [](Geometry::Point&) -> Geometry::Polygon* { return nullptr; },
                [](Geometry::MultiPoint&)  -> Geometry::Polygon* { return nullptr; },
                [](Geometry::LineString&) -> Geometry::Polygon* { return nullptr; },
                [](Geometry::MultiLineString&) -> Geometry::Polygon* { return nullptr; },
                [](Geometry::Polygon& geom) {
                    return &geom;
                },
                [](Geometry::MultiPolygon&) -> Geometry::Polygon* { return nullptr; },
                [](Geometry::GeometryCollection&)  -> Geometry::Polygon* { return nullptr;});
    }


    Geometry::Polygon* GetFirstPolygon(Geometry::FeatureCollection& features)
    {
        for (auto& feature : features) {
            auto poly = GetFirstPolygon(feature.geometry);
            if (poly)
                return poly;
        }
        return nullptr;
    }


    // Wrap a geojson object in a FeatureCollection if it is not already one.
    // Should always end up with FeatureCollection that contains a Feature that contains geometry.
    std::unique_ptr<Geometry::FeatureCollection> makeFeatureCollection(GeoJson::GeoJsonObject&& geoJsonObject)
    {
        return std::unique_ptr<Geometry::FeatureCollection>(std::visit(
            overload
            {
                [](Geometry::FeatureCollection& arg) { return new Geometry::FeatureCollection(arg); },
                [](Geometry::Feature&& arg)          { return new Geometry::FeatureCollection({ arg }); },
                [](auto&& arg)                       { return new Geometry::FeatureCollection({ Geometry::Feature(arg) }); }

            }, geoJsonObject));
    }


    std::tuple<std::shared_ptr<const std::vector<std::byte>>, std::unique_ptr<Geometry::FeatureCollection>, std::unique_ptr<Geometry::BoundingBox>>
    ParseGeometry(const std::variant<std::shared_ptr<const std::vector<std::byte>>, std::istream*>& content_or_stream)
    {
        try
        {
            std::optional<GeoJson::GeoJsonObject> geojson;
            std::shared_ptr<const std::vector<std::byte>> content;

            if( std::holds_alternative<std::shared_ptr<const std::vector<std::byte>>>(content_or_stream) )
            {
                content = std::get<std::shared_ptr<const std::vector<std::byte>>>(content_or_stream);
                MemoryStream stream(content->data(), content->size());

                geojson.emplace(GeoJson::fromGeoJson(stream));
            }

            else
            {
                geojson.emplace(GeoJson::fromGeoJson(*std::get<std::istream*>(content_or_stream)));
            }

            // GeoJson might be just a single feature or even just a geometry; to make it easier always wrap it in a FeatureCollection.
            std::unique_ptr<Geometry::FeatureCollection> features = makeFeatureCollection(std::move(*geojson));
            std::unique_ptr<Geometry::BoundingBox> bounds = std::make_unique<Geometry::BoundingBox>(FeatureCollectionBounds(*features));

            return std::make_tuple(std::move(content), std::move(features), std::move(bounds));
        }

        catch( const std::exception& exception )
        {
            throw CSProException(_T("Error reading GeoJSON: ") + CSProException::GetErrorMessage(exception));
        }
    }
}



// --------------------------------------------------------------------------
// LogicGeometry
// --------------------------------------------------------------------------

LogicGeometry::LogicGeometry(std::wstring geometry_name)
    :   BinarySymbol(std::move(geometry_name), SymbolType::Geometry)
{
}


LogicGeometry::LogicGeometry(const EngineItem& engine_item, ItemIndex item_index, cs::non_null_shared_or_raw_ptr<BinaryDataAccessor> binary_data_accessor)
    :   BinarySymbol(engine_item, std::move(item_index), std::move(binary_data_accessor))
{
}


LogicGeometry::LogicGeometry(const LogicGeometry& logic_geometry)
    :   BinarySymbol(logic_geometry)
{
    // the copy constructor is only used for symbols cloned in an initial state, so we do not need to copy the data from the other symbol
}


std::unique_ptr<Symbol> LogicGeometry::CloneInInitialState() const
{
    return std::unique_ptr<LogicGeometry>(new LogicGeometry(*this));
}


LogicGeometry& LogicGeometry::operator=(const LogicGeometry& logic_geometry)
{
    if( this != &logic_geometry )
    {
        m_binarySymbolData = logic_geometry.m_binarySymbolData;

        if( logic_geometry.m_features != nullptr )
        {
            m_features = std::make_shared<Geometry::FeatureCollection>(*logic_geometry.m_features);
            m_bounds = std::make_shared<Geometry::BoundingBox>(*logic_geometry.m_bounds);
        }

        else
        {
            m_features.reset();
            m_bounds.reset();
        }
    }

    return *this;
}


LogicGeometry& LogicGeometry::operator=(const LogicDocument& logic_document)
{
    const BinarySymbolData& document_binary_symbol_data = logic_document.GetBinarySymbolData();

    if( document_binary_symbol_data.IsDefined() )
    {
        std::shared_ptr<const std::vector<std::byte>> content;

        try
        {
            std::tie(content, m_features, m_bounds) = ParseGeometry(document_binary_symbol_data.GetSharedContent());
        }

        catch(...)
        {
            throw CSProException(_T("The Document '%s' has data that cannot be converted to Geometry."), logic_document.GetName().c_str());                
        }
        
        ASSERT(content == document_binary_symbol_data.GetSharedContent());

        m_binarySymbolData = document_binary_symbol_data;
        m_binarySymbolData.GetMetadataForModification().SetMimeType(MimeType::Type::GeoJson);
    }

    else
    {
        Reset();
    }

    return *this;
}


void LogicGeometry::Reset()
{
    BinarySymbol::Reset();
    m_features.reset();
    m_bounds.reset();
}


BinaryData::ContentCallbackType LogicGeometry::CreateBinaryDataContentFromGeometryCallback() const
{
    ASSERT(m_features != nullptr);

    return
        [features = m_features]() -> std::shared_ptr<const std::vector<std::byte>>
        {
            ASSERT(features != nullptr);

            try
            {
                const std::string json_contents = SaveToString(*features);
                const std::byte* string_data = reinterpret_cast<const std::byte*>(json_contents.data());

                return std::make_shared<const std::vector<std::byte>>(string_data, string_data + json_contents.length());
            }

            catch(...)
            {
                return ReturnProgrammingError(std::make_shared<const std::vector<std::byte>>());
            }
        };
}


bool LogicGeometry::HasValidContent(bool parse_data_if_necessary) const noexcept
{
    ASSERT(HasContent());

    if( m_features == nullptr )
    {
        if( !parse_data_if_necessary )
            return false;

        try
        {
            std::tie(std::ignore,
                     const_cast<LogicGeometry*>(this)->m_features,
                     const_cast<LogicGeometry*>(this)->m_bounds) = ParseGeometry(m_binarySymbolData.GetSharedContent());
        }

        catch(...)
        {
            return false;
        }
    }

    return true;
}


bool LogicGeometry::HasValidContent() const
{
    return HasContent() ? HasValidContent(true) :
                          false;
}


void LogicGeometry::Load(std::wstring filename)
{
    std::shared_ptr<const std::vector<std::byte>> content;
    std::tie(content, m_features, m_bounds) = ParseGeometry(FileIO::Read(filename));
    m_binarySymbolData.SetBinaryData(std::move(content), std::move(filename));
}


void LogicGeometry::Save(std::wstring filename)
{
    ASSERT(HasValidContent(false));

    std::unique_ptr<std::ofstream> os = FileIO::OpenOutputFileStream(filename);
    Save(*os, *m_features);
    os.reset();

    m_binarySymbolData.SetPath(std::move(filename));
    m_binarySymbolData.GetMetadataForModification().SetMimeType(MimeType::Type::GeoJson);
}


void LogicGeometry::Save(std::ostream& os, const Geometry::FeatureCollection& features)
{
    try
    {
        GeoJson::toGeoJson(os, features);
    }

    catch( const std::exception& exception )
    {
        throw CSProException(_T("Error writing GeoJSON: ") + CSProException::GetErrorMessage(exception));
    }
}


std::string LogicGeometry::SaveToString(const Geometry::FeatureCollection& features)
{
    std::stringstream stream;
    Save(stream, features);
    return stream.str();
}


void LogicGeometry::SetGeometry(std::shared_ptr<Geometry::FeatureCollection> geometry)
{
    m_features = std::move(geometry);
    FixWindingOrder(*m_features);

    m_bounds = std::make_shared<Geometry::BoundingBox>(FeatureCollectionBounds(*m_features));

    // if saved, the content must be modified
    m_binarySymbolData.SetBinaryData(CreateBinaryDataContentFromGeometryCallback());
    m_binarySymbolData.ClearPath();
}


void LogicGeometry::SetGeometry(Geometry::Polygon polygon)
{
    std::shared_ptr<Geometry::FeatureCollection> geometry = makeFeatureCollection(std::move(polygon));
    SetGeometry(std::move(geometry));
}


double LogicGeometry::Area() const
{
    ASSERT(HasValidContent(false));

    return GeometryArea(*m_features);
}


double LogicGeometry::Perimeter() const
{
    ASSERT(HasValidContent(false));

    return GeometryPerimeter(*m_features);
}


const Geometry::Polygon* LogicGeometry::GetFirstPolygon() const
{
    // make sure that m_features is parsed from the data
    return HasValidContent(true) ? ::GetFirstPolygon(*m_features) :
                                   nullptr;
}


std::wstring LogicGeometry::GetProperty(wstring_view key_sv) const
{
    ASSERT(HasValidContent(false));

    // Find first feature with that has matching key
    const std::string key_utf8 = UTF8Convert::WideToUTF8(key_sv);

    for( const Geometry::Feature& feature : *m_features )
    {
        const auto& lookup = feature.properties.find(key_utf8);

        if( lookup != feature.properties.end() )
        {
            return lookup->second.match(
                    [](mapbox::feature::null_value_t) { return std::wstring(); },
                    [](bool b) { return b ? _T("true") : _T("false"); },
                    [](uint64_t i) { return CS2WS(IntToString(i)); },
                    [](int64_t i) { return CS2WS(IntToString(i)); },
                    [](double d) { return DoubleToString(d); },
                    [](const std::string& s) { return UTF8Convert::UTF8ToWide(s); },
                    [](const mapbox::feature::value::array_ptr_type&) { return std::wstring(); }, // TODO: support array and object types
                    [](const mapbox::feature::value::object_ptr_type&) { return std::wstring(); });
        }
    }

    return std::wstring();
}


void LogicGeometry::SetProperty(wstring_view key_sv, const std::variant<double, std::wstring>& value)
{
    ASSERT(HasValidContent(false));

    // Set the property in all features
    const std::string utf8_key = UTF8Convert::WideToUTF8(key_sv);

    if( std::holds_alternative<double>(value) )
    {
        for( Geometry::Feature& feature : *m_features )
            feature.properties[utf8_key] = std::get<double>(value);
    }

    else
    {
        const std::string utf8_value = UTF8Convert::WideToUTF8(std::get<std::wstring>(value));

        for( Geometry::Feature& feature : *m_features )
            feature.properties[utf8_key] = utf8_value;
    }

    // if saved, the content must be modified
    m_binarySymbolData.SetBinaryData(CreateBinaryDataContentFromGeometryCallback());
    m_binarySymbolData.ClearPath();
}


void LogicGeometry::WriteValueToJson(JsonWriter& json_writer) const
{
    const std::function<void()> content_writer_override =
        [&]()
        {
            ASSERT(HasContent());

            try
            {
                // make sure that m_features is parsed from the data
                if( !HasValidContent(true) )
                    return;

                // TODO: refactor the serialization of m_features to work with JsonWriter
                const std::string json_contents = SaveToString(*m_features);
                json_writer.Write(JK::json, Json::Parse(UTF8Convert::UTF8ToWide(json_contents)));
            }
            catch(...) { ASSERT(false); }
        };

    m_binarySymbolData.WriteSymbolValueToJson(*this, json_writer, &content_writer_override);
}


void LogicGeometry::UpdateValueFromJson(const JsonNode<wchar_t>& json_node)
{
    class LogicGeometryContentValidator : public BinarySymbolDataContentValidator
    {
    public:
        LogicGeometryContentValidator(LogicGeometry& logic_geometry)
            :   m_logicGeometry(logic_geometry)
        {
        }

        bool ValidateContent(std::shared_ptr<const std::vector<std::byte>> content) override
        {
            std::tie(std::ignore, m_logicGeometry.m_features, m_logicGeometry.m_bounds) = ParseGeometry(std::move(content));
            return true;
        }

    private:
        LogicGeometry& m_logicGeometry;
    };

    LogicGeometryContentValidator logic_geometry_content_validator(*this);

    const std::function<BinaryData::ContentCallbackType(const JsonNode<wchar_t>&)> non_url_content_reader = 
        [&](const JsonNode<wchar_t>& content_node)
        {
            // TODO: refactor the serialization of m_features to work with JsonNode parsing
            const std::string json_contents = UTF8Convert::WideToUTF8(content_node.Get(JK::json).GetNodeAsString());
            std::stringstream stream(json_contents);

            std::tie(std::ignore, m_features, m_bounds) = ParseGeometry(&stream);

            return CreateBinaryDataContentFromGeometryCallback();
        };

    m_binarySymbolData.UpdateSymbolValueFromJson(*this, json_node, &logic_geometry_content_validator, &non_url_content_reader);

    if( m_binarySymbolData.IsDefined() )
        m_binarySymbolData.GetMetadataForModification().SetMimeType(MimeType::Type::GeoJson);

    ASSERT(m_binarySymbolData.IsDefined() == ( m_features != nullptr ));
}
