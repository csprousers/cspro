package gov.census.cspro.maps

import androidx.lifecycle.MutableLiveData
import androidx.lifecycle.ViewModel
import com.google.android.gms.maps.model.LatLng
import gov.census.cspro.maps.geojson.Polygon

class CapturePolygonViewModel(existingPolygon: Polygon?, val mapData: MapData) : ViewModel() {

    val polyline: MutableLiveData<List<LatLng>> = MutableLiveData()

    init {
        // Drop the last point so it does not show up as closed
        existingPolygon?.coordinates?.firstOrNull()?.let { polyline.value = it.dropLast(1) }
    }
}