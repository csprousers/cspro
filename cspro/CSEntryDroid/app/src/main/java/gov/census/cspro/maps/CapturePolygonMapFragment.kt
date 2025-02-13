package gov.census.cspro.maps

import android.graphics.Bitmap
import android.graphics.Canvas
import android.graphics.Color
import android.location.Location
import android.os.Bundle
import android.view.View
import android.widget.ImageButton
import androidx.activity.OnBackPressedCallback
import androidx.appcompat.app.AlertDialog
import androidx.appcompat.content.res.AppCompatResources
import androidx.core.content.ContextCompat
import androidx.core.graphics.drawable.DrawableCompat
import androidx.core.os.bundleOf
import androidx.fragment.app.setFragmentResult
import androidx.lifecycle.ViewModelProvider
import com.google.android.gms.location.LocationServices
import com.google.android.gms.maps.GoogleMap
import com.google.android.gms.maps.model.*
import gov.census.cspro.csentry.R
import gov.census.cspro.util.getDataHolderExtra


class CapturePolygonMapFragment : MapFragment() {

    companion object {
        const val CAPTURE_POLYGON_FRAGMENT_RESULT = "CapturePolygonMapFragment"
        const val CAPTURE_POLYGON_ZINDEX = VECTOR_LAYER_ZINDEX + 1
    }

    private lateinit var viewModel: CapturePolygonViewModel
    private lateinit var viewModelFactory: CapturePolygonViewModelFactory

    private var polyline: Polyline? = null
    private var polygon: Polygon? = null
    private var markers: List<Marker>? = null
    private lateinit var vertexBitmap: BitmapDescriptor
    private lateinit var mode: CapturePolygonActivity.PolygonCaptureMode

    override fun onViewCreated(view: View, savedInstanceState: Bundle?) {
        super.onViewCreated(view, savedInstanceState)

        val mapData = requireActivity().intent.getDataHolderExtra<MapData>(CapturePolygonActivity.EXTRA_MAP)
            ?: MapData()
        val existingPolygon = requireActivity().intent.getDataHolderExtra<gov.census.cspro.maps.geojson.Polygon>(CapturePolygonActivity.EXTRA_POLYGON)

        viewModelFactory = CapturePolygonViewModelFactory(existingPolygon, mapData)
        viewModel = ViewModelProvider(this, viewModelFactory).get(CapturePolygonViewModel::class.java)
        if (existingPolygon != null) {
            val bounds = getPolylineBounds()
            if (bounds != null)
                mapData.zoomToBounds(bounds, 0.1)
        }
        setData(viewModel.mapData)

        mode = CapturePolygonActivity.PolygonCaptureMode.valueOf(requireActivity().intent.getStringExtra(CapturePolygonActivity.EXTRA_MODE)!!)

        requireActivity().onBackPressedDispatcher.addCallback(
            viewLifecycleOwner, object : OnBackPressedCallback(true) {
            override fun handleOnBackPressed() {
                if (haveAnyPoints()) {
                    AlertDialog.Builder(requireContext())
                        .setMessage(getString(R.string.polygon_discard_confirm))
                        .setPositiveButton(getString(R.string.modal_dialog_helper_yes_text)) { _, _ ->
                            isEnabled = false
                            activity?.onBackPressed()
                        }
                        .setNegativeButton(getString(R.string.modal_dialog_helper_no_text)) { _, _ -> }
                        .create()
                        .show()
                } else {
                    isEnabled = false
                    activity?.onBackPressed()
                }
            }
        })
    }

    private fun getPolylineBounds(): LatLngBounds? {
        val builder = LatLngBounds.builder()
        val points = viewModel.polyline.value
        if (points != null && points.isNotEmpty()) {
            points.forEach { builder.include(it) }
            return builder.build()
        } else {
            return null
        }
    }

    /**
     * Manipulates the map once available.`
     * This callback is triggered when the map is ready to be used.
     * This is where we can add markers or lines, add listeners or move the camera.
     * If Google Play services is not installed on the device, the user will be prompted to install
     * it inside the SupportMapFragment. This method will only be triggered once the user has
     * installed Google Play services and returned to the app.
     */
    override fun onMapReady(googleMap: GoogleMap?) {
        super.onMapReady(googleMap)

        vertexBitmap = makeVertexBitmap()

        viewModel.polyline.observe(this) { points ->
            redrawPolygon(points)
        }

        // Don't want any listeners set in base class to fire
        m_map.setOnMarkerClickListener(null)
        m_map.setOnInfoWindowClickListener(null)
        m_map.setOnMapClickListener(null)
        m_map.setInfoWindowAdapter(null)

        if (mode == CapturePolygonActivity.PolygonCaptureMode.TRACE)
            m_map.setOnMapClickListener { location -> addPoint(location) }

        m_map.setOnMarkerDragListener(object : GoogleMap.OnMarkerDragListener {
            override fun onMarkerDragStart(maker: Marker) {
                updatePolylineFromMarkers()
            }

            override fun onMarkerDrag(p0: Marker) {
                updatePolylineFromMarkers()
            }

            override fun onMarkerDragEnd(p0: Marker) {
                updatePolylineFromMarkers()
            }
        })

        clearButtons()

        // add these at start in reverse order so that they end up on top of the
        // current location button
        addButton(R.drawable.ic_baseline_delete_24, 0) {  clearPoints(); }
        addButton(R.drawable.ic_baseline_undo_24, 0) {  removePoint(); }
        addButton(R.drawable.ic_outline_save_24, 0) {  save(); }
        if (mode == CapturePolygonActivity.PolygonCaptureMode.WALK)
            addButton(R.drawable.ic_outline_add_location_24, 0) {  addPointAtCurrentLocation(); }

    }

    override fun onPause() {
        super.onPause()
        viewModel.mapData.cameraPosition = cameraPosition
    }

    private fun addButton(resId: Int, index: Int, onClick: View.OnClickListener) {
        val button = ImageButton(context)
        val drawable = DrawableCompat.wrap(AppCompatResources.getDrawable(requireContext(), resId)!!)
        DrawableCompat.setTint(drawable, Color.BLACK)
        button.setImageDrawable(drawable)
        button.setOnClickListener(onClick)
        addToButtonLayout(button, index)
    }

    private fun addPointAtCurrentLocation() {
        try {
            LocationServices.getFusedLocationProviderClient(requireActivity()).lastLocation.addOnSuccessListener { location: Location? ->
                if (location != null) {
                    addPoint(LatLng(location.latitude, location.longitude))
                }
            }
        } catch (ignored: SecurityException) {
        }
    }

    private fun redrawPolygon(points: List<LatLng>?) {
        polyline?.remove()
        polygon?.remove()
        if (points == null) {
            markers?.forEach { it.remove() }
            markers = null
            polyline = null
            polygon = null
        } else {
            var newMarkers = markers ?: mutableListOf()
            if (newMarkers.size > points.size) {
                for (i in newMarkers.size - 1..points.size) {
                    newMarkers[i].remove()
                }
                newMarkers = newMarkers.take(points.size)
            }
            if (newMarkers.size < points.size) {
                for (lastPoint in points.takeLast(points.size - newMarkers.size).map { point ->
                    m_map.addMarker(MarkerOptions()
                        .position(point)
                        .zIndex(CAPTURE_POLYGON_ZINDEX)
                        .icon(vertexBitmap)
                        .anchor(0.5f, 0.5f)
                        .draggable(true))
                }) {
                    if (lastPoint != null) {
                        newMarkers = newMarkers + lastPoint
                    }
                }
            }
            markers = newMarkers

            if (points.size >= 2) {
                polyline = m_map.addPolyline(
                    PolylineOptions()
                        .addAll(points)
                        .zIndex(CAPTURE_POLYGON_ZINDEX)
                        .color(ContextCompat.getColor(requireContext(), R.color.colorAccent))
                        .width(5f))
            }
            if (points.size >= 3) {
                polygon = m_map.addPolygon(
                    PolygonOptions()
                        .addAll(points)
                        .zIndex(CAPTURE_POLYGON_ZINDEX)
                        .fillColor(ContextCompat.getColor(requireContext(), R.color.colorTracePolygonFill))
                        .strokeWidth(0f)
                )
            }
        }
    }

    private fun updatePolylineFromMarkers() {
        viewModel.polyline.value = markers?.map { it.position }
    }

    private fun addPoint(location: LatLng) {
        viewModel.polyline.value = (viewModel.polyline.value ?: emptyList()) + location
    }

    private fun removePoint() {
        viewModel.polyline.value = viewModel.polyline.value?.dropLast(1)
    }

    private fun clearPoints() {
        viewModel.polyline.value = null
    }

    private fun save() {
        val polyline =  viewModel.polyline.value
        if (polyline != null && polyline.size >= 3) {
            val closedPolyline = polyline + polyline[0]
            setFragmentResult(CAPTURE_POLYGON_FRAGMENT_RESULT, bundleOf(CapturePolygonActivity.EXTRA_POLYGON to closedPolyline))
        } else {
            AlertDialog.Builder(requireContext())
                .setMessage(getString( if (mode == CapturePolygonActivity.PolygonCaptureMode.TRACE) R.string.polygon_not_enough_points_tapped else R.string.polygon_not_enough_points_walked))
                .setPositiveButton(getString(R.string.modal_dialog_helper_ok_text)) { _, _ -> }
                .create()
                .show()
        }
    }

    private fun makeVertexBitmap(): BitmapDescriptor {
        val vectorDrawable = ContextCompat.getDrawable(requireContext(), R.drawable.ic_polygon_vertex)!!
        vectorDrawable.setBounds(0, 0, vectorDrawable.intrinsicWidth, vectorDrawable.intrinsicHeight)
        val bitmap = Bitmap.createBitmap(vectorDrawable.intrinsicWidth, vectorDrawable.intrinsicHeight, Bitmap.Config.ARGB_8888)
        val canvas = Canvas(bitmap)
        vectorDrawable.draw(canvas)
        return BitmapDescriptorFactory.fromBitmap(bitmap)
    }

    private fun haveAnyPoints() = viewModel.polyline.value?.size ?: 0 > 0

}