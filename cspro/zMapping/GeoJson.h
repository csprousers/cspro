#pragma once

#include <zMapping/zMapping.h>
#include <zMapping/Geometry.h>
#include <ostream>
#include <istream>


namespace GeoJson {

    // A GeoJson object can be any of these entities at the top-level
    // So we represent them with a std::variant.
    // Note that these entities can be recursive e.g. GeometryCollection
    // can contain any geometry including another GeometryCollection.
    // See the spec https://tools.ietf.org/html/rfc7946 for details.
    using GeoJsonObject = std::variant<Geometry::Feature, Geometry::FeatureCollection,
                                       Geometry::Point, Geometry::MultiPoint,
                                       Geometry::LineString, Geometry::MultiLineString,
                                       Geometry::Polygon, Geometry::MultiPolygon,
                                       Geometry::GeometryCollection>;

    /// <summary>
    /// Write a GeoJson entity to an output stream as GeoJson
    /// </summary>
    ZMAPPING_API void toGeoJson(std::ostream& os, const GeoJsonObject& object);

    /// <summary>
    /// Read a GeoJson entity from an input stream
    /// </summary>
    ZMAPPING_API GeoJsonObject fromGeoJson(std::istream& os);
}
