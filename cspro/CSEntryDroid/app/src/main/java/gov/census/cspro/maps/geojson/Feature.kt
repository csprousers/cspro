package gov.census.cspro.maps.geojson

import com.google.android.gms.maps.model.LatLngBounds

data class Feature(val geometry: Geometry?, val properties: Map<String, Any>)

data class FeatureCollection(val features: List<Feature>, val bounds: LatLngBounds?)