package gov.census.cspro.maps

import androidx.lifecycle.ViewModel
import androidx.lifecycle.ViewModelProvider
import gov.census.cspro.maps.geojson.Polygon

class CapturePolygonViewModelFactory(private val existingPolygon: Polygon?, private val mapData: MapData) : ViewModelProvider.Factory {

    override fun <T : ViewModel> create(modelClass: Class<T>): T {
        if (modelClass.isAssignableFrom(CapturePolygonViewModel::class.java)) {
            return CapturePolygonViewModel(existingPolygon, mapData) as T
        }
        throw IllegalArgumentException("Unknown ViewModel class")
    }
}