#pragma once

// Forward declarations of the Mapbox geometry templates.
// If you actually use the classes in a cpp file
// include them <external/mapbox/...>

namespace mapbox
{
    namespace geometry
    {
        template <typename T>
        struct point;

        template <typename T, template <typename...> class Cont>
        struct multi_point;

        template <typename T, template <typename...> class Cont>
        struct line_string;

        template <typename T, template <typename...> class Cont>
        struct multi_line_string;

        template <typename T, template <typename...> class Cont>
        struct polygon;

        template <typename T, template <typename...> class Cont>
        struct multi_polygon;

        template <typename T, template <typename...> class Cont>
        struct geometry_collection;

        template <typename T, template <typename...> class Cont>
        struct geometry;

        template <typename T>
        struct box;
    }

    namespace feature
    {
        template <class T, template <typename...> class>
        struct feature_collection;

        template <class T>
        struct feature;

        struct value;
    }
}


namespace Geometry
{
    // Type aliases for the mapbox geometry.hpp classes (https://github.com/mapbox/geometry.hpp)
    // These are C++ classes that correspond to GeoJson entities
    using CoordinateType = double;
    using Point = mapbox::geometry::point<CoordinateType>;
    using MultiPoint = mapbox::geometry::multi_point<CoordinateType, std::vector>;
    using LineString = mapbox::geometry::line_string<CoordinateType, std::vector>;
    using MultiLineString = mapbox::geometry::multi_line_string<CoordinateType, std::vector>;
    using Polygon = mapbox::geometry::polygon<CoordinateType, std::vector>;
    using MultiPolygon = mapbox::geometry::multi_polygon<CoordinateType, std::vector>;
    using GeometryCollection = mapbox::geometry::geometry_collection<CoordinateType, std::vector>;
    using Geometry = mapbox::geometry::geometry<CoordinateType, std::vector>;
    using Feature = mapbox::feature::feature<CoordinateType>;
    using FeatureCollection = mapbox::feature::feature_collection<CoordinateType, std::vector>;
    using Value = mapbox::feature::value;
    using BoundingBox = mapbox::geometry::box<CoordinateType>;
}
