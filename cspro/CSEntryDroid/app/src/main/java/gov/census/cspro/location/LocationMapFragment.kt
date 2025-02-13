package gov.census.cspro.location

import android.app.Activity
import android.content.Context
import android.graphics.Color
import android.location.GnssStatus
import android.location.GpsStatus
import android.location.Location
import android.location.LocationManager
import android.os.Build
import android.os.Bundle
import android.os.Handler
import android.view.LayoutInflater
import android.view.View
import android.view.ViewGroup
import android.widget.Button
import android.widget.ImageButton
import android.widget.ProgressBar
import android.widget.TextView
import androidx.appcompat.app.AppCompatActivity
import androidx.constraintlayout.widget.ConstraintLayout
import androidx.fragment.app.Fragment
import androidx.lifecycle.MutableLiveData
import com.google.android.gms.maps.CameraUpdateFactory
import com.google.android.gms.maps.GoogleMap
import com.google.android.gms.maps.MapView
import com.google.android.gms.maps.OnMapReadyCallback
import com.google.android.gms.maps.model.*
import gov.census.cspro.csentry.R
import gov.census.cspro.engine.BaseMapSelection
import gov.census.cspro.engine.EngineInterface
import gov.census.cspro.engine.functions.GPSFunction
import gov.census.cspro.maps.MapData
import gov.census.cspro.maps.offline.IOfflineTileReader
import gov.census.cspro.maps.offline.MBTilesReader
import gov.census.cspro.maps.offline.OfflineTileProvider
import gov.census.cspro.maps.offline.TpkTilesReader
import java.io.File
import java.util.Date
import kotlin.math.roundToInt


class LocationMapFragment : Fragment(R.layout.fragment_location_map), OnMapReadyCallback {

    var mapView: MapView? = null

    //String constants
    val WAIT_TIME = "WAIT_TIME"
    val BASE_MAP_TYPE = "BASE_MAP_TYPE"
    val BASE_MAP_FILENAME  = "BASE_MAP_FILENAME"
    val ACCURACY = "ACCURACY"
    val NO_READING_TEXT = "---"

    //Numeric constants
    private val POLLING_DURATION = 200 // check the GPS status every 200 milliseconds
    val ACCURACY_GOOD = 15 //below this value accuracy considered good
    val ACCURACY_OK = 50  //below this value accuracy considered OK
    val MAX_ZOOM_LEVEL = 18 //maximum map zoom level

    //Color constants
    val CIRCLE_STROKE_RED = Color.parseColor("#ff0000")
    val CIRCLE_STROKE_YELLOW = Color.parseColor("#ffff00")
    val CIRCLE_STROKE_GREEN = Color.parseColor("#00ff00")
    val CIRCLE_FILL_RED = Color.parseColor("#1eff0000")
    val CIRCLE_FILL_YELLOW = Color.parseColor("#1effff00")
    val CIRCLE_FILL_GREEN = Color.parseColor("#1e00ff00")
    val ACCURACY_LABEL_RED = Color.parseColor("#ff6464")
    val ACCURACY_LABEL_YELLOW = Color.parseColor("#ffff32")
    val ACCURACY_LABEL_GREEN = Color.parseColor("#64ff64")

    //Variables
    private var m_gpsResultLiveData = MutableLiveData<String>()
    private var m_gpsResultCurrentData = ""
    private var m_gpsResultCurrentReportedData = ""
    private var m_locationManager: LocationManager? = null
    private var m_statusListener: GpsStatus.Listener? = null
    private var m_gnssListener: GnssStatus.Callback? = null
    private var m_numSatellites = 0
    private var m_baseMapSelection: BaseMapSelection? = null

    companion object {
        @JvmStatic
        fun newInstance(fType: Int): LocationMapFragment {
            val myFragment = LocationMapFragment()
            val args = Bundle()
            args.putInt("fType", fType)
            myFragment.setArguments(args)
            return myFragment
        }

        var lastLat: Double = 0.0 //-17.502445231877722
        var lastLong: Double = 0.0 //24.280105462913546
    }

    interface GpsResultCurrentDataSetListner {
        fun onGpsResultSet(result: String)
    }
    private var gpsResultCurrentDataSetListner: GpsResultCurrentDataSetListner? = null

    override fun onAttach(context: Context) {
        super.onAttach(context)
        if (context is GpsResultCurrentDataSetListner) {
            gpsResultCurrentDataSetListner = context as GpsResultCurrentDataSetListner
        }
    }

    //This is needed here for backward compatibility with API < 23
    @Deprecated("Deprecated in Java")
    override fun onAttach(activity: Activity) {
        super.onAttach(activity)
        if (activity is GpsResultCurrentDataSetListner) {
            gpsResultCurrentDataSetListner = activity as GpsResultCurrentDataSetListner
        }
    }

    //this determines whether user can select location
    private fun fType(): Int {
        if (arguments!=null) {
            return requireArguments().getInt("fType")
        }

        return 1
    }

    private var firstReading = true
    private var settingInitialLocaiton = true
    private var captureLocationClicked = false

    private var curLocationMarker: Marker? = null
    private var capturedLocationMarker: Marker? = null
    private var accuracyCircle: Circle? = null

    override fun onMapReady(googleMap: GoogleMap) {
        if (googleMap != null) {

            // load the offline map if necessary
            if (m_baseMapSelection?.type == MapData.BASE_MAP_CUSTOM && !loadOffLineMap(googleMap)) {
                m_baseMapSelection = BaseMapSelection(MapData.BASE_MAP_NONE)
            }

            if (googleMap.mapType != m_baseMapSelection?.googleMapType) {
                googleMap.mapType = m_baseMapSelection!!.googleMapType
            }

            if (settingInitialLocaiton) {
                settingInitialLocaiton = false
                googleMap.moveCamera(
                    CameraUpdateFactory.newLatLng(
                        LatLng(
                            lastLat,
                            lastLong
                        )
                    )
                )
            }

            val lla = getLongLatAcc(m_gpsResultCurrentReportedData)

            if (lla.count() >= 3) {
                //updating default location
                lastLat = lla[0]
                lastLong = lla[1]

                //googleMap.clear()
                removeMarkers()

                //circle coloring
                //setting text color based on accuracy
                var circleStrokeColor = CIRCLE_STROKE_RED
                var circleFillColor = CIRCLE_FILL_RED
                if (lla[2] < ACCURACY_GOOD) {
                    circleStrokeColor = CIRCLE_STROKE_GREEN
                    circleFillColor = CIRCLE_FILL_GREEN
                } else if (lla[2] < ACCURACY_OK) {
                    circleStrokeColor = CIRCLE_STROKE_YELLOW
                    circleFillColor = CIRCLE_FILL_YELLOW
                }


                //only
                if (fType() == 1) {
                    accuracyCircle = googleMap.addCircle(
                        CircleOptions()
                            .center(LatLng(lla[0], lla[1]))
                            .radius(lla[2])
                            .strokeWidth(3f)
                            .strokeColor(circleStrokeColor)
                            .fillColor(circleFillColor)
                            .zIndex(1f)
                    )
                }

                //add current location marker
                curLocationMarker = googleMap.addMarker(
                    MarkerOptions()
                        .position(LatLng(lla[0], lla[1]))
                        .icon(BitmapDescriptorFactory.fromResource(R.drawable.ic_my_location2))
                        .anchor(0.5f, 0.5f)
                )

                if (fType() == 2 && captureLocationClicked) { //getting camera position
                    captureLocationClicked = false

                    val cp = googleMap.cameraPosition.target
                    m_gpsResultCurrentData = LocationUtils.locationString(
                        cp.latitude, cp.longitude, -1.0, -1, -1.0f, Date())

                    gpsResultCurrentDataSetListner?.onGpsResultSet(m_gpsResultCurrentData)

                    showData()
                }

                //adding marker from last recorded location
                if (m_gpsResultCurrentData.length > 0) {
                    val llaLast = getLongLatAcc(m_gpsResultCurrentData)
                    if (llaLast.count() >= 3) {
                        capturedLocationMarker = googleMap.addMarker(
                            MarkerOptions()
                                .position(LatLng(llaLast[0], llaLast[1]))
                        )
                    }
                }

                if (fType() == 1 || firstReading) {
                    googleMap.animateCamera(
                        CameraUpdateFactory.newLatLngZoom(
                            LatLng(lla[0], lla[1]),
                            getZoomLevel(accuracyCircle).toFloat()
                        )
                    )
                }

                //hiding the spinner
                if (firstReading) {
                    val spinner: ProgressBar? = getView()?.findViewById(R.id.spinner)
                    if (spinner != null) {
                        spinner.visibility = View.INVISIBLE
                    }
                }

                firstReading = false
            }
        }
    }

    private var m_tileProvider: OfflineTileProvider? = null
    private var m_baseMapInitialExtent: LatLngBounds? = null

    private fun loadOffLineMap(map: GoogleMap): Boolean {
        assert(m_baseMapSelection?.filename != null)

        if (m_tileProvider == null) {
            val f = File(m_baseMapSelection!!.filename)
            if (!f.exists()) {
                showErrorText("Base map file doesn't exist.")
                return false
            }

            var reader: IOfflineTileReader

            try {
                reader = if (m_baseMapSelection!!.filename!!.toLowerCase().endsWith("mbtiles")) {
                    MBTilesReader(m_baseMapSelection!!.filename!!)
                } else if (m_baseMapSelection!!.filename!!.toLowerCase().endsWith("tpk")) {
                    TpkTilesReader(m_baseMapSelection!!.filename!!)
                } else {
                    showErrorText("Invalid tile file format. Only mbtiles and tpk are supported.")
                    return false
                }
            } catch (e: Exception) {
                showErrorText("Base map file corrupted.")
                return false
            }

            // No max zoom since reader can scale up bitmaps
            map.setMinZoomPreference(reader.getMinZoom().toFloat())

            val m_baseMapInitialExtent = reader.getFullExtent()
            map.setLatLngBoundsForCameraTarget(m_baseMapInitialExtent)

            m_tileProvider = OfflineTileProvider(reader)
            m_tileProvider?.let {
                val tileOverlay = map.addTileOverlay(TileOverlayOptions().tileProvider(it))
                if (tileOverlay != null) {
                    tileOverlay.clearTileCache()
                }
            }
        }

        return true
    }

    private fun removeMarkers() {
        if (curLocationMarker != null)
            curLocationMarker!!.remove()

        if (capturedLocationMarker != null)
            capturedLocationMarker!!.remove()

        if (accuracyCircle != null)
            accuracyCircle!!.remove()
    }

    private fun showErrorText(message: String) {
        val errorTextLayout: ConstraintLayout? = getView()?.findViewById(R.id.errorTextLayout)
        val errorTextView: TextView? = getView()?.findViewById(R.id.errorTextView)
        if (errorTextLayout != null && errorTextView != null) {
            errorTextView.text = message
            errorTextLayout.visibility = View.VISIBLE
        }
    }

    private fun hideErrorText() {
        val errorTextLayout: ConstraintLayout? = getView()?.findViewById(R.id.errorTextLayout)
        if (errorTextLayout != null) {
            errorTextLayout.visibility = View.GONE
        }
    }

    //Calculating zoom level of the map based on the accuracy circle.
    //if circle == null, then zoom level is set to 14
    private fun getZoomLevel(circle: Circle?): Int {
        if (circle != null) {
            val radius = circle.radius * 1.5
            val scale = radius / 500
            val res = (16 - Math.log(scale) / Math.log(2.0)).toInt()
            if (res < MAX_ZOOM_LEVEL) {
                return (16 - Math.log(scale) / Math.log(2.0)).toInt()
            }
        }
        return MAX_ZOOM_LEVEL
    }

    override fun onCreateView(
        inflater: LayoutInflater,
        container: ViewGroup?,
        savedInstanceState: Bundle?
    ): View? {
        val v: View = inflater.inflate(R.layout.fragment_location_map, container, false)

        // Gets the MapView from the XML layout and creates it
        mapView = v.findViewById<View>(R.id.mapView) as MapView
        mapView!!.onCreate(savedInstanceState)

        mapView!!.getMapAsync(this)

        return v
    }

    override fun onResume() {
        mapView!!.onResume()
        super.onResume()
    }

    override fun onPause() {
        super.onPause()
        mapView!!.onPause()
    }

    override fun onLowMemory() {
        super.onLowMemory()
        mapView!!.onLowMemory()
    }

    override fun onViewCreated(view: View, savedInstanceState: Bundle?) {
        super.onViewCreated(view, savedInstanceState)

        if (activity?.intent != null ) {
            m_baseMapSelection = BaseMapSelection(
                activity?.intent!!.getIntExtra(BASE_MAP_TYPE, MapData.BASE_MAP_NORMAL),
                activity?.intent!!.getStringExtra(BASE_MAP_FILENAME))
        }

        if (m_baseMapSelection?.type == MapData.BASE_MAP_NONE && fType() == 2) {
            m_baseMapSelection = BaseMapSelection(MapData.BASE_MAP_NORMAL)
        }

        hideData()
        hideErrorText()

        if (fType() == 2) {
            val captureGpsButton: Button? = getView()?.findViewById(R.id.captureGpsMapButton)
            if (captureGpsButton != null) {
                captureGpsButton.text = getString(R.string.gps_capture_selected_location)
            }
        } else {
            val captureGpsButton: Button? = getView()?.findViewById(R.id.captureGpsMapButton)
            if (captureGpsButton != null) {
                captureGpsButton.text = getString(R.string.gps_capture_current_location)
            }
            val chHorizontalLayout: ConstraintLayout? =
                getView()?.findViewById(R.id.chHorizontalLayout)
            if (chHorizontalLayout != null) {
                chHorizontalLayout.visibility = View.INVISIBLE
            }

            val chVerticalLayout: ConstraintLayout? = getView()?.findViewById(R.id.chVerticalLayout)
            if (chVerticalLayout != null) {
                chVerticalLayout.visibility = View.INVISIBLE
            }
        }

        //noinspection ConstantConditions
        m_locationManager =
            activity?.getSystemService(AppCompatActivity.LOCATION_SERVICE) as LocationManager
        if (Build.VERSION.SDK_INT >= Build.VERSION_CODES.N) {
            m_gnssListener = object : GnssStatus.Callback() {
                override fun onSatelliteStatusChanged(status: GnssStatus) {
                    var numberSatellites = 0
                    for (i in 0 until status.satelliteCount) {
                        if (status.usedInFix(i)) ++numberSatellites
                    }
                    m_numSatellites = numberSatellites
                    super.onSatelliteStatusChanged(status)
                }
            }
            try {
                m_locationManager!!.registerGnssStatusCallback(m_gnssListener as GnssStatus.Callback)
            } catch (ignored: SecurityException) {
            }
        } else {
            m_statusListener = GpsStatus.Listener {
                try {
                    val gpsStatus: GpsStatus? = m_locationManager!!.getGpsStatus(null)
                    var numberSatellites = 0
                    if (gpsStatus != null) {
                        for (satellite in gpsStatus.satellites) {
                            if (satellite.usedInFix()) numberSatellites++
                        }
                    }
                    m_numSatellites = numberSatellites
                } catch (ignored: SecurityException) {
                }
            }
            try {
                m_locationManager!!.addGpsStatusListener(m_statusListener)
            } catch (ignored: SecurityException) {
            }
        }


        // Use LiveData to signal endFunction from the gps
        // polling to avoid crash when polling code receives a reading
        // and dismisses dialog when Fragment is in background
        m_gpsResultLiveData.observe(viewLifecycleOwner,
            { s ->
                s?.let {

                    //Only updating if accuracy increased from previous reading
                    //or if point moved outside the radious of new accuracy
                    if (movedOrLowerAccuracy(m_gpsResultCurrentReportedData, it)) {

                        m_gpsResultCurrentReportedData = it


                        val captureGpsButton: Button? =
                            getView()?.findViewById(R.id.captureGpsMapButton)
                        if (captureGpsButton != null) {
                            if (m_gpsResultCurrentReportedData.length > 0) {
                                captureGpsButton.isEnabled = true
                                captureGpsButton.setTextColor(Color.BLACK)

                                val goToCurrentLocaitonButton: ImageButton? =
                                    getView()?.findViewById(R.id.goToCurrentLocaitonButton)
                                if (goToCurrentLocaitonButton != null) {
                                    goToCurrentLocaitonButton.isEnabled = true
                                }
                            }
                        }
                        mapView!!.getMapAsync(this)
                    }
                }
            })

        //setting capture GPS button click listener
        val captureGpsButton: Button? = getView()?.findViewById(R.id.captureGpsMapButton)
        if (captureGpsButton != null) {
            captureGpsButton.isEnabled = false
            captureGpsButton.setTextColor(Color.rgb(200, 200, 200))
            captureGpsButton.setOnClickListener {
                captureGpsButtonClick()
            }
        }

        //go to current location button click listener
        val goToCurrentLocationButton: ImageButton? =
            getView()?.findViewById(R.id.goToCurrentLocaitonButton)
        if (goToCurrentLocationButton != null) {

            goToCurrentLocationButton.visibility = View.VISIBLE
            goToCurrentLocationButton.isEnabled = false
            goToCurrentLocationButton.setOnClickListener {
                goToCurrentLocation()
            }
        }

        //starting current position loop
        gpsHandler = Handler()
        readerIsRunning = true
        gpsRunnable.run()
    }

    //Extracting longitude/latitude/accuracy components from the GPS reading
    private fun getLongLatAcc(input: String): List<Double> {
        var res = mutableListOf<Double>()

        val gpsData = input.split(";")

        if (gpsData.count() >= 6) {
            res.add(gpsData[0].toDouble())
            res.add(gpsData[1].toDouble())
            res.add(gpsData[4].toDouble())
        }

        return res
    }

    fun captureGpsButtonClick() {
        if (fType() == 1) {
            m_gpsResultCurrentData = m_gpsResultCurrentReportedData

            gpsResultCurrentDataSetListner?.onGpsResultSet(m_gpsResultCurrentData)

            showData()
        }
        else {
            captureLocationClicked = true
        }

        mapView!!.getMapAsync(this)
    }

    fun goToCurrentLocation() {
        settingInitialLocaiton = true
        mapView!!.getMapAsync(this)
    }

    private var gpsHandler: Handler? = null
    private var readerIsRunning: Boolean = false

    private val gpsRunnable: Runnable = object : Runnable {
        override fun run() {
            val reader = GPSFunction.getReader()


            //posting reading
            if (reader.hasNewGPSReading(0)){
                //val amg = reader.readMostAccurateGPS()
                val amg = reader.readLast()
                if (amg != m_gpsResultCurrentReportedData) {
                    m_gpsResultLiveData.postValue(amg)
                }
            }

            //looping forever until instructed to stop
            if (readerIsRunning) {
                gpsHandler!!.postDelayed(this, POLLING_DURATION.toLong())
            }
        }
    }

    override fun onDestroy() {
        super.onDestroy()

        readerIsRunning = false

        mapView!!.onDestroy()

        if (gpsHandler != null) gpsHandler!!.removeCallbacks(gpsRunnable)

        if (Build.VERSION.SDK_INT >= Build.VERSION_CODES.N) {
            if (m_locationManager != null && m_gnssListener != null) {
                m_locationManager!!.unregisterGnssStatusCallback(m_gnssListener!!)
            }
        } else {
            if (m_locationManager != null && m_statusListener != null) {
                m_locationManager!!.removeGpsStatusListener(m_statusListener)
            }
        }
    }

    private fun distance2(lat1: Double, long1: Double, lat2: Double, long2: Double): Double {

        val loc1 = Location("")
        loc1.latitude = lat1
        loc1.longitude = long1

        val loc2 = Location("")
        loc2.latitude = lat2
        loc2.longitude = long2

        return loc1.distanceTo(loc2).toDouble()
    }

    //Compares newly acquired GPS data with the previusly recorded one and decides whether to update the current values
    private fun movedOrLowerAccuracy(oldRawData: String, newRawData: String): Boolean {
        val oldData = oldRawData.split(";")
        val newData = newRawData.split(";")

        //data is incomplete. Can't get distance
        if (oldData.count() < 6 || newData.count() < 6) {
            return true
        }

        //first checking accuracy. If accuracy improved, we update the point
        val oldAcc: Double = oldData[4].toDouble()
        val newAcc: Double = newData[4].toDouble()

        if (newAcc < oldAcc)
            return true

        //if accuracy is the same or worse we check the distance
        //and only update if new point moved beyond the radius of new point accuracy

        val oldLat: Double = oldData[0].toDouble()
        val oldLong: Double = oldData[1].toDouble()

        val newLat: Double = newData[0].toDouble()
        val newLong: Double = newData[1].toDouble()

        val dist =  distance2(oldLat, oldLong, newLat, newLong)

        return dist > newAcc
    }

    private fun showData() {
        val gpsLabelsText: TextView? = getView()?.findViewById(R.id.gpsDataLabelsTextView2)
        if (gpsLabelsText != null) {
            if (fType() == 1 ){
                gpsLabelsText.text = getString(R.string.gps_labels_with_accuracy)
            }
            else {
                gpsLabelsText.text = getString(R.string.gps_labels_no_accuracy)
            }
        }

        val gpsDataText: TextView? = getView()?.findViewById(R.id.gpsDataTextView2)
        if (gpsDataText != null) {
            gpsDataText.text = formatData(m_gpsResultCurrentData)
        }

        val accuracyDataText: TextView? = getView()?.findViewById(R.id.accuracyDataTextView)
        if (accuracyDataText != null) {
            if (fType() == 1) {
                accuracyDataText.visibility = View.VISIBLE
                val accuracy: Double = getAccuracy(m_gpsResultCurrentData)
                if (accuracy < 0.0) {
                    accuracyDataText.text = NO_READING_TEXT
                    accuracyDataText.setTextColor(Color.WHITE)
                } else {
                    accuracyDataText.text = "${accuracy?.roundToInt()} m"

                    //setting text color based on accuracy
                    if (accuracy < ACCURACY_GOOD) {
                        accuracyDataText.setTextColor(Color.rgb(100, 255, 100))
                    } else if (accuracy < ACCURACY_OK) {
                        accuracyDataText.setTextColor(Color.rgb(255, 255, 50))
                    } else {
                        accuracyDataText.setTextColor(Color.rgb(255, 100, 100))
                    }
                }

            } else {
                accuracyDataText.visibility = View.GONE
            }
        }
    }

    private fun formatData(rawData: String): String {
        val gpsData = rawData.split(";")

        if (gpsData.count() >= 6) {
            val lat: Double? = gpsData[0].toDouble()
            val llong: Double? = gpsData[1].toDouble()

            if (lat != null && llong != null) {
                val formattedCoordinates = EngineInterface.getInstance().formatCoordinates(lat, llong)
                return formattedCoordinates.replace(", ", "\n")
            }
        }
        return "$NO_READING_TEXT\n$NO_READING_TEXT"
    }

    private fun getAccuracy(rawData: String): Double {
        val gpsData = rawData.split(";")

        if (gpsData.count() >= 6) {

            val acc: Double? = gpsData[4].toDouble()
            if (acc != null)
                return acc
        }
        return -1.0
    }

    fun Double.format(digits: Int) = "%.${digits}f".format(this)

    private fun hideData() {
        val gpsLabelsText: TextView? = getView()?.findViewById(R.id.gpsDataLabelsTextView2)
        if (gpsLabelsText != null) {
            if (fType() == 1 ){
                gpsLabelsText.text = getString(R.string.gps_labels_with_accuracy)
            }
            else {
                gpsLabelsText.text = getString(R.string.gps_labels_no_accuracy)
            }
        }

        val gpsDataText: TextView? = getView()?.findViewById(R.id.gpsDataTextView2)
        if (gpsDataText != null) {
            gpsDataText.text = "${NO_READING_TEXT}\n${NO_READING_TEXT}"
        }

        val accuracyDataText: TextView? = getView()?.findViewById(R.id.accuracyDataTextView)
        if (accuracyDataText != null) {
            if (fType() == 1 ) {
                accuracyDataText.visibility = View.VISIBLE
                accuracyDataText.text = NO_READING_TEXT
            }
            else {
                accuracyDataText.visibility = View.GONE
            }

        }
    }
}