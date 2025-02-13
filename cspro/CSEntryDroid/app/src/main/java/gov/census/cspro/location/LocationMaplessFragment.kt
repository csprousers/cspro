package gov.census.cspro.location

import android.animation.ValueAnimator
import android.app.Activity
import android.content.Context
import android.content.res.Resources
import android.graphics.Color
import android.location.GnssStatus
import android.location.GpsStatus
import android.location.Location
import android.location.LocationManager
import android.os.Build
import android.os.Bundle
import android.os.Handler
import android.util.TypedValue
import android.view.View
import android.widget.ImageButton
import android.widget.ImageView
import android.widget.TextView
import androidx.appcompat.app.AppCompatActivity
import androidx.constraintlayout.widget.ConstraintLayout
import androidx.fragment.app.Fragment
import androidx.lifecycle.MutableLiveData
import gov.census.cspro.csentry.R
import gov.census.cspro.engine.EngineInterface
import gov.census.cspro.engine.functions.GPSFunction
import kotlin.math.*


class LocationMaplessFragment : Fragment(R.layout.fragment_location_mapless) {

    //String constants
    val WAIT_TIME = "WAIT_TIME"
    val ACCURACY = "ACCURACY"
    val DIALOG_TEXT = "DIALOG_TEXT"
    val NO_READING_TEXT = "---"

    //Numeric constants
    private val POLLING_DURATION = 200 // check the gps status every 200 milliseconds
    val ACCURACY_GOOD = 15 //below this value accuracy considered good
    val ACCURACY_OK = 50  //below this value accuracy considered OK

    //Color constants
    val ACCURACY_LABEL_RED = Color.parseColor("#ff6464")
    val ACCURACY_LABEL_YELLOW = Color.parseColor("#ffff32")
    val ACCURACY_LABEL_GREEN = Color.parseColor("#64ff64")

    //Variables
    private var m_accuracy = 0
    private var m_waitTime = 0
    private var m_gpsResultLiveData = MutableLiveData<String>()
    private var m_gpsResultCurrentData = ""
    private var m_locationManager: LocationManager? = null
    private var m_statusListener: GpsStatus.Listener? = null
    private var m_gnssListener: GnssStatus.Callback? = null
    private var m_numSatellites = 0

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

    override fun onViewCreated(view: View, savedInstanceState: Bundle?) {
        super.onViewCreated(view, savedInstanceState)

        m_waitTime = activity?.intent?.getIntExtra(WAIT_TIME, 0) ?: 0
        m_accuracy = activity?.intent?.getIntExtra(ACCURACY, 0) ?: 0

        hideData()
        stopSatAnim()
        stopProgressAnim()

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
        m_gpsResultLiveData.observe(viewLifecycleOwner,
            { s ->
                s?.let {
                    //Only updating if accuracy increased from previous reading
                    //or if point moved outside the radious of new accuracy
                    if (movedOrLowerAccuracy(m_gpsResultCurrentData, it)) {
                        m_gpsResultCurrentData = it
                        showData()
                        gpsResultCurrentDataSetListner?.onGpsResultSet(m_gpsResultCurrentData)
                    }
                }
            })

        //setting capture GPS button click listener
        val captureGpsButton: ImageButton? = getView()?.findViewById(R.id.captureGpsButton)
        if (captureGpsButton != null) {
            captureGpsButton.setOnClickListener {
                captureGpsButtonClick()
            }
        }
    }

    private fun distance(lat1: Double, long1: Double, lat2: Double, long2: Double): Double {

        val loc1 = Location("")
        loc1.latitude = lat1
        loc1.longitude = long1

        val loc2 = Location("")
        loc2.latitude = lat2
        loc2.longitude = long2

        return loc1.distanceTo(loc2).toDouble()
    }

    //Compares newly acuired GPS data with the previusly recorded one and decides whether to update the current values
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

        val dist =  distance(oldLat, oldLong, newLat, newLong)

        val distanceText: TextView? = getView()?.findViewById(R.id.textView2)
        if (distanceText != null) {
            distanceText.text = "${dist?.format(2)}"
        }

        return dist > newAcc
    }

    private fun showData() {
        val gpsDataText: TextView? = getView()?.findViewById(R.id.gpsDataTextView)
        val gpsDataTextLayout: ConstraintLayout? = getView()?.findViewById(R.id.gpsDataLayout)

        if (gpsDataText != null) {
            gpsDataText.text = formatData(m_gpsResultCurrentData)

        }

        //accuracy reading
        val gpsAccuracyText: TextView? = getView()?.findViewById(R.id.gpsDataTextView3)

        if (gpsAccuracyText != null) {
            gpsAccuracyText.text = formatAccuracyData(m_gpsResultCurrentData)
        }

        //leaving this here for now as example of animation
        /*
        if (gpsDataTextLayout != null && gpsDataTextLayout.visibility == View.GONE) {
            val dip = 80f
            val r: Resources = resources
            val px = TypedValue.applyDimension(
                TypedValue.COMPLEX_UNIT_DIP,
                dip,
                r.displayMetrics
            ).toInt()

            gpsDataTextLayout.maxHeight = 0
            gpsDataTextLayout.alpha = 0f
            gpsDataTextLayout.visibility = View.VISIBLE


            ValueAnimator.ofInt(0, px).apply {
                duration = 250
                start()

                addUpdateListener { updateAnimation ->
                    gpsDataTextLayout.maxHeight = updateAnimation.animatedValue as Int
                }
            }

            ValueAnimator.ofFloat(0f, 1f).apply {
                duration = 500
                start()

                addUpdateListener { updateAnimation ->
                    gpsDataTextLayout.alpha = updateAnimation.animatedValue as Float
                }
            }

        }
        */
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

    private fun formatAccuracyData(rawData: String): String {
        val gpsData = rawData.split(";")

        if (gpsData.count() >= 6) {
            val acc: Double? = gpsData[4].toDouble()

            val gpsAccuracyText: TextView? = getView()?.findViewById(R.id.gpsDataTextView3)
            if (gpsAccuracyText != null){
                if (acc != null) {
                    if (acc < ACCURACY_GOOD) {
                        gpsAccuracyText.setTextColor(Color.rgb(0,150,0))
                    }
                    else if (acc < ACCURACY_OK) {
                        gpsAccuracyText.setTextColor(Color.rgb(150,150,0))
                    }
                    else {
                        gpsAccuracyText.setTextColor(Color.rgb(150,0,0))
                    }
                }
            }

            return "${acc?.roundToInt()} m"
        }

        return NO_READING_TEXT
    }

    fun Double.format(digits: Int) = "%.${digits}f".format(this)

    private fun hideData() {
        val gpsDataText: TextView? = getView()?.findViewById(R.id.gpsDataTextView)
        if (gpsDataText != null) {
            gpsDataText.text = "$NO_READING_TEXT\n$NO_READING_TEXT"
        }

        val gpsAccuracyDataText: TextView? = getView()?.findViewById(R.id.gpsDataTextView3)
        if (gpsAccuracyDataText != null) {
            gpsAccuracyDataText.text = NO_READING_TEXT
        }
    }

    fun captureGpsButtonClick() {
        val captureButton: ImageButton? = getView()?.findViewById(R.id.captureGpsButton)
        if (captureButton != null) {
            captureButton.isEnabled = false
        }

        val progressLabel: TextView? = getView()?.findViewById(R.id.captureStatusTextView)
        if (progressLabel != null) {
            progressLabel.text = getString(R.string.gps_capturing)
        }
        startSatAnim()

        m_waitTime = activity?.intent?.getIntExtra(WAIT_TIME, 0)!!
        startProgressAnim()

        gpsHandler = Handler()
        gpsRunnable.run()
    }

    var m_satAnimSizeValueAnimator: ValueAnimator? = null
    var m_satAnimAlphaValueAnimator: ValueAnimator? = null

    fun startSatAnim() {
        val satAnimLayout: ConstraintLayout? = getView()?.findViewById(R.id.satAnimLayout)

        if (satAnimLayout != null) {
            satAnimLayout.minHeight = 1
            satAnimLayout.minWidth = 1
            satAnimLayout.alpha = 1f
            satAnimLayout.visibility = View.VISIBLE

            val dip = 300f
            val r: Resources = resources
            val px = TypedValue.applyDimension(
                TypedValue.COMPLEX_UNIT_DIP,
                dip,
                r.displayMetrics
            ).toInt()

            //animating
            m_satAnimSizeValueAnimator = ValueAnimator.ofInt(0, px).apply {
                duration = 500
                repeatMode = ValueAnimator.RESTART
                repeatCount = ValueAnimator.INFINITE
                start()

                addUpdateListener { updateAnimation ->
                    satAnimLayout.maxHeight = updateAnimation.animatedValue as Int
                    satAnimLayout.maxWidth = updateAnimation.animatedValue as Int
                }
            }

            m_satAnimAlphaValueAnimator = ValueAnimator.ofFloat(1f, 0f).apply {
                duration = 500
                repeatMode = ValueAnimator.RESTART
                repeatCount = ValueAnimator.INFINITE
                start()

                addUpdateListener { updateAnimation ->
                    satAnimLayout.alpha = updateAnimation.animatedValue as Float
                }
            }
        }
    }

    fun stopSatAnim() {
        val satAnimLayout: ConstraintLayout? = getView()?.findViewById(R.id.satAnimLayout)
        if (satAnimLayout != null) {
            satAnimLayout.visibility = View.INVISIBLE

            m_satAnimSizeValueAnimator?.end()
            m_satAnimAlphaValueAnimator?.end()
        }
    }

    var m_progressValueAnimator: ValueAnimator? = null
    fun startProgressAnim() {
        val progressAnimImage: ImageView? = getView()?.findViewById(R.id.imageView2)
        if (progressAnimImage != null) {
            //geting the destination width
            var px: Int = 0
            val satImage: ImageView? = getView()?.findViewById(R.id.imageView)
            if (satImage != null){
                px = satImage.width
            }
            else {
                val dip = 131f
                val r: Resources = resources
                px = TypedValue.applyDimension(
                    TypedValue.COMPLEX_UNIT_DIP,
                    dip,
                    r.displayMetrics
                ).toInt()
            }

            progressAnimImage.layoutParams.width = px
            progressAnimImage.visibility = View.VISIBLE
            progressAnimImage.requestLayout()

            //animating
            m_satAnimSizeValueAnimator = ValueAnimator.ofInt(px, 1).apply {
                duration = m_waitTime.toLong()
                start()

                addUpdateListener { updateAnimation ->
                    progressAnimImage.layoutParams.width = updateAnimation.animatedValue as Int
                    if (progressAnimImage.layoutParams.width <= 1){
                        progressAnimImage.visibility = View.INVISIBLE
                    }
                    progressAnimImage.requestLayout()
                }
            }


        }


    }

    fun stopProgressAnim() {
        val progressAnimImage: ImageView? = getView()?.findViewById(R.id.imageView2)
        if (progressAnimImage != null) {
            progressAnimImage.visibility = View.INVISIBLE

            m_progressValueAnimator?.end()
        }
    }

    private var gpsHandler: Handler? = null

    private val gpsRunnable: Runnable = object : Runnable {
        override fun run() {
            val reader = GPSFunction.getReader()
            if (m_waitTime < 0) // time out
            {
                val progressLabel: TextView? = getView()?.findViewById(R.id.captureStatusTextView)
                if (progressLabel != null) {
                    if (m_gpsResultCurrentData.length > 0) {
                        progressLabel.text = getString(R.string.gps_recapture_tap_prompt)
                    }
                    else {
                        progressLabel.text = getString(R.string.gps_capture_tap_prompt)
                    }
                }
                val captureButton: ImageButton? =getView()?.findViewById(R.id.captureGpsButton)
                if (captureButton != null) {
                    captureButton.isEnabled = true
                }
                stopSatAnim()
                stopProgressAnim()
            } else if (reader.hasNewGPSReading(m_accuracy)) { // a successful reading has been made
                m_gpsResultLiveData.postValue(reader.readLast())

                m_waitTime -= POLLING_DURATION
                gpsHandler!!.postDelayed(this, POLLING_DURATION.toLong())
            } else {
                m_waitTime -= POLLING_DURATION
                gpsHandler!!.postDelayed(this, POLLING_DURATION.toLong())
            }
        }
    }

    override fun onDestroy() {
        super.onDestroy()

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

}