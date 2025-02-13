#include "stdafx.h"
#include "GeoJson.h"
#include <external/jsoncons/json.hpp>
#include <external/jsoncons/json_traits_macros.hpp>

#pragma warning(push)
#pragma warning(disable: 4068 4239)
#include <mapbox/feature.hpp>
#include <mapbox/geometry.hpp>
#include <mapbox/variant.hpp>
#pragma warning(pop)


using namespace GeoJson;

// Extensions to json parser to handle the mapbox geometry.hpp types
// See jsoncons docs for details on how to add custom specializations to the parser/writer
// https://github.com/danielaparker/jsoncons/blob/b9e95d6a0bcb2dc8a821d33f98a530f10cfe1bc0/doc/ref/json_type_traits/custom-specializations.md

namespace jsoncons {

    template<class Json>
    static Json point_vector_to_json_array(const std::vector<Geometry::Point>& points,
        typename Json::allocator_type allocator)
    {
        json coords(json_array_arg, semantic_tag::none, allocator);
        coords.reserve(points.size());
        for (const auto& p : points) {
            coords.emplace_back(std::vector<Geometry::Point::coordinate_type>{ p.x, p.y });
        }
        return coords;
    }

    template<class Json>
    static std::vector<Geometry::Point> json_coordinate_array_to_point_vector(const Json& json)
    {
        std::vector<Geometry::Point> points;
        points.reserve(json.size());
        for (const auto& point : json.array_range()) {
            points.emplace_back(point[0].template as<Geometry::Point::coordinate_type>(),
                                point[1].template as<Geometry::Point::coordinate_type>());
        }
        return points;
    }

    template<class Json>
    Geometry::Point MakePoint(const Json& coordinates) {
        return Geometry::Point(
            coordinates[0].template as<Geometry::Point::coordinate_type>(),
            coordinates[1].template as<Geometry::Point::coordinate_type>());
    }

    template<class Json>
    struct json_type_traits<Json, Geometry::Point>
    {
        using allocator_type = typename Json::allocator_type;

        static bool is(const Json& j) noexcept
        {
            return j.is_object() && j.contains("type") && j["type"] == "Point";
        }

        static Geometry::Point as(const Json& j)
        {
            return MakePoint(j.at("coordinates"));
        }

        static Json to_json(const Geometry::Point& p,
            allocator_type allocator = allocator_type())
        {
            Json j(json_object_arg, semantic_tag::none, allocator);
            j.try_emplace("type", "Point");
            j.try_emplace("coordinates", std::vector<Geometry::CoordinateType>{ p.x, p.y });
            return j;
        }
    };

    template<class Json>
    Geometry::MultiPoint MakeMultiPoint(const Json& coordinates)
    {
        return Geometry::MultiPoint(json_coordinate_array_to_point_vector(coordinates));
    }

    template<class Json>
    struct json_type_traits<Json, Geometry::MultiPoint>
    {
        using allocator_type = typename Json::allocator_type;

        static bool is(const Json& j) noexcept
        {
            return j.is_object() && j.contains("type") && j["type"] == "MultiPoint";
        }

        static Geometry::MultiPoint as(const Json& j)
        {
            auto coordinates = j.at("coordinates");
            return Geometry::MultiPoint(json_coordinate_array_to_point_vector(coordinates));
        }

        static Json to_json(const Geometry::MultiPoint& mp,
            allocator_type allocator = allocator_type())
        {
            Json j(json_object_arg, semantic_tag::none, allocator);
            j.try_emplace("type", "MultiPoint");
            j.try_emplace("coordinates", point_vector_to_json_array<Json>(mp, allocator));
            return j;
        }
    };

    template <>
    struct is_json_type_traits_declared<Geometry::MultiPoint> : public std::true_type
    {};

    template<class Json>
    Geometry::LineString MakeLineString(const Json& coordinates)
    {
        return Geometry::LineString(json_coordinate_array_to_point_vector(coordinates));
    }

    template<class Json>
    struct json_type_traits<Json, Geometry::LineString>
    {
        using allocator_type = typename Json::allocator_type;

        static bool is(const Json& j) noexcept
        {
            return j.is_object() && j.contains("type") && j["type"] == "LineString";
        }

        static Geometry::LineString as(const Json& j)
        {
            auto coordinates = j.at("coordinates");
            return MakeLineString(coordinates);
        }

        static Json to_json(const Geometry::LineString& ls,
            allocator_type allocator = allocator_type())
        {
            Json j(json_object_arg, semantic_tag::none, allocator);
            j.try_emplace("type", "LineString");
            j.try_emplace("coordinates", point_vector_to_json_array<Json>(ls, allocator));
            return j;
        }
    };

    template <>
    struct is_json_type_traits_declared<Geometry::LineString> : public std::true_type
    {};

    template<class Json>
    Geometry::Polygon MakePolygon(const Json& coordinates)
    {
        Geometry::Polygon polygon;
        polygon.reserve(coordinates.size());
        for (const auto& json_ring : coordinates.array_range()) {
            Geometry::Polygon::linear_ring_type poly_ring;
            poly_ring.reserve(json_ring.size());
            for (const auto& json_coord : json_ring.array_range()) {
                poly_ring.emplace_back(json_coord[0].template as<Geometry::Polygon::coordinate_type>(),
                    json_coord[1].template as<Geometry::Polygon::coordinate_type>());
            }
            polygon.emplace_back(std::move(poly_ring));
        }
        return polygon;
    }

    template<class Json>
    struct json_type_traits<Json, Geometry::Polygon>
    {
        using allocator_type = typename Json::allocator_type;

        static bool is(const Json& j) noexcept
        {
            return j.is_object() && j.contains("type") && j["type"] == "Polygon";
        }

        static Geometry::Polygon as(const Json& j)
        {
            auto coordinates = j.at("coordinates");
            return MakePolygon(coordinates);
        }

        static Json to_json(const Geometry::Polygon& polygon,
            allocator_type allocator = allocator_type())
        {
            Json j(json_object_arg, semantic_tag::none, allocator);
            j.try_emplace("type", "Polygon");
            json rings(json_array_arg, semantic_tag::none, allocator);
            rings.reserve(polygon.size());
            for (const auto& poly_ring : polygon) {
                rings.emplace_back(point_vector_to_json_array<Json>(poly_ring, allocator));
            }
            j.try_emplace("coordinates", rings);
            return j;
        }
    };

    template <>
    struct is_json_type_traits_declared<::Geometry::Polygon> : public std::true_type
    {};

    template<class Json>
    Geometry::MultiLineString MakeMultiLineString(const Json& coordinates)
    {
        Geometry::MultiLineString multi_line;
        multi_line.reserve(coordinates.size());
        for (const auto& json_line_string : coordinates.array_range()) {
            Geometry::LineString line_string;
            line_string.reserve(json_line_string.size());
            for (const auto& json_coord : json_line_string.array_range()) {
                line_string.emplace_back(json_coord[0].template as<Geometry::MultiLineString::coordinate_type>(),
                    json_coord[1].template as<Geometry::MultiLineString::coordinate_type>());
            }
            multi_line.emplace_back(std::move(line_string));
        }
        return multi_line;
    }

    template<class Json>
    struct json_type_traits<Json, Geometry::MultiLineString>
    {
        using allocator_type = typename Json::allocator_type;

        static bool is(const Json& j) noexcept
        {
            return j.is_object() && j.contains("type") && j["type"] == "MultiLineString";
        }

        static Geometry::MultiLineString as(const Json& j)
        {
            auto coordinates = j.at("coordinates");
            return MakeMultiLineString(coordinates);
        }

        static Json to_json(const Geometry::MultiLineString& multi_line,
            allocator_type allocator = allocator_type())
        {
            Json j(json_object_arg, semantic_tag::none, allocator);
            j.try_emplace("type", "MultiLineString");
            json json_line_strings(json_array_arg, semantic_tag::none, allocator);
            json_line_strings.reserve(multi_line.size());
            for (const auto& line : multi_line) {
                json_line_strings.emplace_back(point_vector_to_json_array<Json>(line, allocator));
            }
            j.try_emplace("coordinates", json_line_strings);
            return j;
        }
    };

    template <>
    struct is_json_type_traits_declared<Geometry::MultiLineString> : public std::true_type
    {};

    template<class Json>
    Geometry::MultiPolygon MakeMultiPolygon(const Json& coordinates)
    {
        Geometry::MultiPolygon multi_polygon;
        multi_polygon.reserve(coordinates.size());
        for (const auto& json_polygon : coordinates.array_range()) {
            Geometry::Polygon polygon;
            polygon.reserve(json_polygon.size());
            for (const auto& json_ring : json_polygon.array_range()) {
                Geometry::Polygon::linear_ring_type poly_ring;
                poly_ring.reserve(json_ring.size());
                for (const auto& json_coord : json_ring.array_range()) {
                    poly_ring.emplace_back(json_coord[0].template as<Geometry::Polygon::coordinate_type>(),
                        json_coord[1].template as<Geometry::Polygon::coordinate_type>());
                }
                polygon.emplace_back(std::move(poly_ring));
            }
            multi_polygon.emplace_back(std::move(polygon));
        }
        return multi_polygon;
    }

    template<class Json>
    struct json_type_traits<Json, Geometry::MultiPolygon>
    {
        using allocator_type = typename Json::allocator_type;

        static bool is(const Json& j) noexcept
        {
            return j.is_object() && j.contains("type") && j["type"] == "MultiPolygon";
        }

        static Geometry::MultiPolygon as(const Json& j)
        {
            auto coordinates = j.at("coordinates");
            return MakeMultiPolygon(coordinates);
        }

        static Json to_json(const Geometry::MultiPolygon& multi_polygon,
                            allocator_type allocator = allocator_type())
        {
            Json j(json_object_arg, semantic_tag::none, allocator);
            j.try_emplace("type", "MultiPolygon");
            json json_polygons(json_array_arg, semantic_tag::none, allocator);
            json_polygons.reserve(multi_polygon.size());
            for (const auto& polygon : multi_polygon) {
                json json_polygon(json_array_arg, semantic_tag::none, allocator);
                json_polygon.reserve(polygon.size());
                for (const auto& poly_ring : polygon) {
                    json_polygon.emplace_back(point_vector_to_json_array<Json>(poly_ring, allocator));
                }
                json_polygons.emplace_back(std::move(json_polygon));
            }
            j.try_emplace("coordinates", json_polygons);
            return j;
        }
    };

    template <>
    struct is_json_type_traits_declared<Geometry::MultiPolygon> : public std::true_type
    {};

    template<class Json>
    struct json_type_traits<Json, ::Geometry::Geometry>
    {
        using allocator_type = typename Json::allocator_type;

        static bool is(const Json& j) noexcept
        {
            return j.is_object() && j.contains("type");
        }

        static Geometry::Geometry as(const Json& j)
        {
            auto type = j.at("type").template as<std::string_view>();
            if (type == "Point") {
                return j.template as<Geometry::Point>();
            }
            else if (type == "MultiPoint") {
                return j.template as<Geometry::MultiPoint>();
            }
            else if (type == "LineString") {
                return j.template as<Geometry::LineString>();
            }
            else if (type == "MultiLineString") {
                return j.template as<Geometry::MultiLineString>();
            }
            else if (type == "Polygon") {
                return j.template as<Geometry::Polygon>();
            }
            else if (type == "MultiPolygon") {
                return j.template as<Geometry::MultiPolygon>();
            }
            else if (type == "GeometryCollection") {
                return j.template as<Geometry::GeometryCollection>();
            }
            else {
                throw conv_error(conv_errc::conversion_failed);
            }
        }

        static Json to_json(const Geometry::Geometry& geometry,
            allocator_type allocator = allocator_type())
        {
            return geometry.match([&allocator](mapbox::geometry::empty) { return json::null(); },
                                    [&allocator](const Geometry::Point& g) { return json_type_traits<Json, Geometry::Point>::to_json(g, allocator); },
                                    [&allocator](const Geometry::MultiPoint& g) { return json_type_traits<Json, Geometry::MultiPoint>::to_json(g, allocator); },
                                    [&allocator](const Geometry::LineString& g) { return json_type_traits<Json, Geometry::LineString>::to_json(g, allocator); },
                                    [&allocator](const Geometry::MultiLineString& g) { return json_type_traits<Json, Geometry::MultiLineString>::to_json(g, allocator); },
                                    [&allocator](const Geometry::Polygon& g) { return json_type_traits<Json, Geometry::Polygon>::to_json(g, allocator); },
                                    [&allocator](const Geometry::MultiPolygon& g) { return json_type_traits<Json, Geometry::MultiPolygon>::to_json(g, allocator); },
                                    [&allocator](const Geometry::GeometryCollection& g) { return json_type_traits<Json, Geometry::GeometryCollection>::to_json(g, allocator); });
        }
    };

    template<class Json>
    struct json_type_traits<Json, Geometry::GeometryCollection>
    {
        using allocator_type = typename Json::allocator_type;

        static bool is(const Json& j) noexcept
        {
            return j.is_object() && j.contains("type") && j["type"] == "GeometryCollection";
        }

        static Geometry::GeometryCollection as(const Json& j)
        {
            Geometry::GeometryCollection geometry_collection;
            auto json_geometries = j.at("geometries");
            geometry_collection.reserve(json_geometries.size());
            for (const auto& json_geometry : json_geometries.array_range()) {
                geometry_collection.emplace_back(json_geometry.template as<mapbox::geometry::geometry<Geometry::GeometryCollection::coordinate_type>>());
            }
            return geometry_collection;
        }

        static Json to_json(const Geometry::GeometryCollection& geometry_collection,
            allocator_type allocator = allocator_type())
        {
            Json j(json_object_arg, semantic_tag::none, allocator);
            j.try_emplace("type", "GeometryCollection");
            json json_geometries(json_array_arg, semantic_tag::none, allocator);
            json_geometries.reserve(geometry_collection.size());
            for (const auto& geometry : geometry_collection) {
                auto jg = json_type_traits<Json, Geometry::Geometry>::to_json(geometry, allocator);
                json_geometries.emplace_back(jg);
            }
            j.try_emplace("geometries", json_geometries);
            return j;
        }
    };

    template <>
    struct is_json_type_traits_declared<Geometry::GeometryCollection> : public std::true_type
    {};

    template<class Json>
    struct json_type_traits<Json, Geometry::Value>
    {
        using allocator_type = typename Json::allocator_type;

        static bool is(const Json& j) noexcept
        {
            return true;
        }

        static Geometry::Value as(const Json& j)
        {
            if (j.is_null())
                return mapbox::feature::null_value;
            else if (j.is_bool())
                return j.template as<bool>();
            else if (j.is_int64())
                return j.template as<int64_t>();
            else if (j.is_uint64())
                return j.template as<uint64_t>();
            else if (j.is_double())
                return j.template as<double>();
            else if (j.is_string())
                return j.template as<std::string>();
            else if (j.is_array())
                return std::make_shared<Geometry::Value::array_type>(j.template as<Geometry::Value::array_type>());
            else if (j.is_object())
                return std::make_shared<Geometry::Value::object_type>(j.template as<Geometry::Value::object_type>());
            else
                throw conv_error(conv_errc::conversion_failed);
        }

        static Json to_json(const Geometry::Value& value,
            allocator_type allocator = allocator_type())
        {
            return value.match(
                [&allocator](mapbox::feature::null_value_t) { return json::null(); },
                [&allocator](const auto& v) { return Json(v); });
        }
    };

    template<class Json>
    struct json_type_traits<Json, Geometry::Feature>
    {
        using allocator_type = typename Json::allocator_type;

        static bool is(const Json& j) noexcept
        {
            return j.is_object() && j.contains("type") && j["type"] == "Feature";
        }

        static Geometry::Feature as(const Json& j)
        {
            Geometry::Feature feature;
            auto json_geometry = j.at("geometry");
            if (!json_geometry.is_null())
                feature.geometry = json_geometry.template as<Geometry::Geometry>();
            auto json_properties = j.at("properties");
            if (!json_properties.is_null()) {
                for (const auto& json_property : json_properties.object_range()) {
                    feature.properties[json_property.key()] = json_property.value().template as<Geometry::Value>();
                }
            }
            if (j.contains("id")) {
                auto json_id = j.at("id");
                if (json_id.is_int64())
                    feature.id = json_id.template as<int64_t>();
                else if (json_id.is_uint64())
                    feature.id = json_id.template as<uint64_t>();
                else if (json_id.is_double())
                    feature.id = json_id.template as<double>();
                else if (json_id.is_string())
                    feature.id = json_id.template as<std::string>();
            }
            return feature;
        }

        static Json to_json(const Geometry::Feature& feature,
            allocator_type allocator = allocator_type())
        {
            Json j(json_object_arg, semantic_tag::none, allocator);
            j.try_emplace("type", "Feature");
            j.try_emplace("geometry", feature.geometry);
            Json json_id = feature.id.match(
                [](mapbox::feature::null_value_t) { return json::null(); },
                [](const auto& v) { return Json(v); });
            if (!json_id.is_null())
                j.try_emplace("id", json_id);
            Json json_properties(json_object_arg, semantic_tag::none, allocator);
            for (const auto& [name, value] : feature.properties) {
                json_properties.try_emplace(name, json_type_traits<Json, Geometry::Value>::to_json(value));
            }
            j.try_emplace("properties", json_properties);
            return j;
        }
    };

    template<class Json>
    struct json_type_traits<Json, Geometry::FeatureCollection>
    {
        using allocator_type = typename Json::allocator_type;

        static bool is(const Json& j) noexcept
        {
            return j.is_object() && j.contains("type") && j["type"] == "FeatureCollection";
        }

        static Geometry::FeatureCollection as(const Json& j)
        {
            Geometry::FeatureCollection feature_collection;
            auto json_features = j.at("features");
            feature_collection.reserve(json_features.size());
            for (const auto& json_feature : json_features.array_range()) {
                feature_collection.emplace_back(json_feature.template as<Geometry::Feature>());
            }
            return feature_collection;
        }

        static Json to_json(const Geometry::FeatureCollection& feature_collection,
            allocator_type allocator = allocator_type())
        {
            Json j(json_object_arg, semantic_tag::none, allocator);
            j.try_emplace("type", "FeatureCollection");
            Json json_features(json_array_arg, semantic_tag::none, allocator);
            json_features.reserve(feature_collection.size());
            for (const Geometry::Feature& feature : feature_collection) {
                json_features.emplace_back(json_type_traits<Json, Geometry::Feature>::to_json(feature, allocator));
            }
            j.try_emplace("features", json_features);
            return j;
        }
    };

    template <>
    struct is_json_type_traits_declared<Geometry::FeatureCollection> : public std::true_type
    {};


    void SkipArray(json_cursor& cursor)
    {
        // if this is triggered, look at the change to SkipObject made on 2023-08-21 and see if a similar thing needs to happen for this function
        ASSERT(false);

        int depth = 1;
        for (; !cursor.done(); cursor.next())
        {
            const auto& event = cursor.current();
            switch (event.event_type())
            {
                case staj_event_type::begin_array:
                    ++depth;
                    break;
                case staj_event_type::end_array:
                    --depth;
                    if (depth == 0)
                        return;
                    break;
                default:
                    break;
            }
        }
    }


    void SkipObject(json_cursor& cursor)
    {
        ASSERT(!cursor.done() && cursor.current().event_type() == staj_event_type::begin_object);

        int depth = 1;
        for (cursor.next(); !cursor.done(); cursor.next())
        {
            const auto& event = cursor.current();
            switch (event.event_type())
            {
                case staj_event_type::begin_object:
                    ++depth;
                    break;
                case staj_event_type::end_object:
                    --depth;
                    if (depth == 0)
                        return;
                    break;
                default:
                    break;
            }
        }
    }
}

void GeoJson::toGeoJson(std::ostream& os, const GeoJsonObject& object)
{
    jsoncons::encode_json(object, os);
}

GeoJsonObject GeoJson::fromGeoJson(std::istream& is)
{
    jsoncons::json_cursor cursor(is);

    // Should start with a begin object
    if (cursor.done())
        throw jsoncons::conv_error(jsoncons::conv_errc::not_map);

    if (cursor.current().event_type() != jsoncons::staj_event_type::begin_object)
        throw jsoncons::conv_error(jsoncons::conv_errc::not_map);

    jsoncons::json_decoder<jsoncons::json> decoder;
    std::string type;
    std::string key;
    jsoncons::json coordinates;
    Geometry::GeometryCollection geometries;
    Geometry::FeatureCollection features;
    Geometry::Feature feature;

    for (cursor.next(); !cursor.done(); cursor.next())
    {
        const auto& event = cursor.current();
        switch (event.event_type())
        {
            case jsoncons::staj_event_type::begin_array:
            {
                if (key == "features") {
                    auto feature_array = jsoncons::staj_array<Geometry::Feature>(cursor);
                    std::copy(std::begin(feature_array), std::end(feature_array), std::back_inserter(features));
                }
                else if (key == "geometries") {
                    auto geometry_array = jsoncons::staj_array<Geometry::Geometry>(cursor);
                    std::copy(std::begin(geometry_array), std::end(geometry_array), std::back_inserter(geometries));
                }
                else if (key == "coordinates") {
                    cursor.read_to(decoder);
                    coordinates = decoder.get_result();
                }
                else {
                    SkipArray(cursor);
                }
                break;
            }
            case jsoncons::staj_event_type::end_array:
            {
                break;
            }
            case jsoncons::staj_event_type::begin_object:
            {
                if (key == "properties") {
                    auto json_properties = jsoncons::staj_object<std::string, jsoncons::json>(cursor);
                    for (const auto& kv : json_properties)
                        feature.properties[kv.first] = kv.second.template as<Geometry::Value>();
                }
                else if (key == "geometry") {
                    cursor.read_to(decoder);
                    feature.geometry = decoder.get_result().template as<Geometry::Geometry>();
                }
                else {
                    SkipObject(cursor);
                }
                break;
            }
            case jsoncons::staj_event_type::string_value:
            {
                if (key == "type") {
                    type = event.get<jsoncons::string_view>();
                }
                else if (key == "id") {
                    feature.id = event.get<std::string>();
                }
            }

            case jsoncons::staj_event_type::null_value:
            {
                if (key == "id") {
                    feature.id = mapbox::feature::null_value;
                }
                break;
            }
            case jsoncons::staj_event_type::bool_value:
            {
                break;
            }
            case jsoncons::staj_event_type::int64_value:
            {
                if (key == "id") {
                    feature.id = event.get<int64_t>();
                }
                break;
            }
            case jsoncons::staj_event_type::uint64_value:
            {
                if (key == "id") {
                    feature.id = event.get<uint64_t>();
                }
                break;
            }
            case jsoncons::staj_event_type::half_value:
            case jsoncons::staj_event_type::double_value:
            {
                if (key == "id") {
                    feature.id = event.get<double>();
                }
                break;
            }
            case jsoncons::staj_event_type::key:
            {
                key = event.get<jsoncons::string_view>();
                break;
            }
            default:
            {
                break;
            }
        }
    }

    if (type == "FeatureCollection") {
        return features;
    }
    else if (type == "Feature") {
        return feature;
    }
    else if (type == "GeometryCollection") {
        return geometries;
    }
    else if (type == "Point") {
        return MakePoint(coordinates);
    }
    else if (type == "MultiPoint") {
        return MakeMultiPoint(coordinates);
    }
    else if (type == "LineString") {
        return MakeLineString(coordinates);
    }
    else if (type == "MultiLineString") {
        return MakeMultiLineString(coordinates);
    }
    else if (type == "Polygon") {
        return MakePolygon(coordinates);
    }
    else if (type == "MultiPolygon") {
        return MakeMultiPolygon(coordinates);
    }
    else {
        throw jsoncons::conv_error(jsoncons::conv_errc::conversion_failed);
    }
}
