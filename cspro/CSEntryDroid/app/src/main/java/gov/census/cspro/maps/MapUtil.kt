package gov.census.cspro.maps

import android.app.Activity
import android.location.Location
import android.util.DisplayMetrics
import android.util.SparseArray
import androidx.fragment.app.Fragment
import com.google.android.gms.location.LocationServices
import com.google.android.gms.maps.CameraUpdate
import com.google.android.gms.maps.CameraUpdateFactory
import com.google.android.gms.maps.GoogleMap
import com.google.android.gms.maps.model.CameraPosition
import com.google.android.gms.maps.model.LatLng
import com.google.android.gms.maps.model.LatLngBounds
import com.google.android.gms.maps.model.Marker
import gov.census.cspro.maps.geojson.FeatureCollection
import kotlin.math.min
import kotlin.math.roundToInt

const val CAMERA_ANIMATE_DURATION_CURRENT_LOCATION = 2000
const val MARKER_FIT_PADDING = 150

fun zoomToCurrentLocation(map: GoogleMap, activity: Activity) {
    try {
        LocationServices.getFusedLocationProviderClient(activity).lastLocation.addOnSuccessListener { location: Location? ->
            if (location != null) {
                // Zoom to at least level 16 but don't zoom in if already past that level
                // 16 appears to be what default my location button goes to when zoomed
                // out.
                val zoom = map.cameraPosition.zoom.coerceAtLeast(16f)
                val update = CameraUpdateFactory.newLatLngZoom(LatLng(location.latitude, location.longitude), zoom)
                map.animateCamera(update, CAMERA_ANIMATE_DURATION_CURRENT_LOCATION, null)
            }
        }
    } catch (ignored: SecurityException) {
    }
}

