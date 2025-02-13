package gov.census.cspro.maps.geojson

import android.graphics.Color
import com.google.android.gms.maps.GoogleMap
import com.google.android.gms.maps.model.*
import java.lang.Exception

class GeoJsonLayer(val map: GoogleMap, featureCollection: FeatureCollection, var zIndex: Float) {

    private val markers: MutableList<Marker> = mutableListOf()
    private val polylines: MutableList<Polyline> = mutableListOf()
    private val polygons: MutableList<com.google.android.gms.maps.model.Polygon> = mutableListOf()

    val bounds: LatLngBounds? = featureCollection.bounds

    init {
        featureCollection
            .features
            .filter { it.geometry != null }
            .forEach { addGeometry(it.geometry!!, it.properties) }
    }

    fun removeFromMap() {
        markers.forEach { it.remove() }
        polylines.forEach { it.remove() }
        polygons.forEach { it.remove() }
    }

    private fun addPoint(coordinates: LatLng) {
        map.addMarker(MarkerOptions().position(coordinates).zIndex(zIndex))?.let {
            markers.add(it)
        }
    }

    private fun addPolyline(coordinates: List<LatLng>, properties: Map<String, Any>) {
        val options = PolylineOptions().addAll(coordinates).zIndex(zIndex)
        for ((key, value) in properties) {
            when (key) {
                "stroke" -> parseColorProperty(value)?.let { options.color(it) }
                "stroke-width" -> parseNumberProperty(value)?.let { options.width(it) }
            }
        }
        polylines.add(map.addPolyline(options))
    }

    private fun addPolygon(coordinates: List<List<LatLng>>, properties: Map<String, Any>) {
        val options = PolygonOptions().addAll(coordinates.first())
        coordinates.drop(1).forEach {
            options.addHole(it)
        }
        for ((key, value) in properties) {
            when (key) {
                "stroke" -> parseColorProperty(value)?.let { options.strokeColor(it) }
                "stroke-width" -> parseNumberProperty(value)?.let { options.strokeWidth(it) }
                "fill" -> parseColorProperty(value)?.let { options.fillColor(it) }
            }
        }
        options.zIndex(zIndex)
        polygons.add(map.addPolygon(options))
    }

    private fun parseColorProperty(value: Any) = try { if (value is String) Color.parseColor(value) else null  } catch(e: Exception) { null }

    private fun parseNumberProperty(value: Any) = try {
        when (value) {
            is Number -> value.toFloat()
            is String -> value.toFloat()
            else -> null
        }
    } catch(e: Exception) { null }

    private fun addGeometry(geometry: Geometry, properties: Map<String, Any>) {
        when (geometry) {
            is Point -> addPoint(geometry.coordinates)
            is MultiPoint -> geometry.coordinates.forEach { addPoint(it) }
            is LineString -> addPolyline(geometry.coordinates, properties)
            is MultiLineString -> geometry.coordinates.forEach { addPolyline(it, properties) }
            is Polygon -> addPolygon(geometry.coordinates, properties)
            is MultiPolygon -> geometry.coordinates.forEach { addPolygon(it, properties) }
            is GeometryCollection -> geometry.geometries.forEach { addGeometry(it, properties) }
        }
    }
}