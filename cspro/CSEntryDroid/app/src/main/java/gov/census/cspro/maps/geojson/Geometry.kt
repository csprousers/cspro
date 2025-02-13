package gov.census.cspro.maps.geojson

import com.google.android.gms.maps.model.LatLng

typealias CoordinateList = List<LatLng>

sealed class Geometry

data class Point(var coordinates: LatLng) : Geometry()
data class MultiPoint(val coordinates: CoordinateList) : Geometry()
data class LineString(val coordinates: CoordinateList) : Geometry()
data class MultiLineString(val coordinates: List<CoordinateList>) : Geometry()
data class Polygon(val coordinates: List<CoordinateList>) : Geometry()
data class MultiPolygon(val coordinates: List<List<CoordinateList>>) : Geometry()
data class GeometryCollection(val geometries: List<Geometry>) : Geometry()

