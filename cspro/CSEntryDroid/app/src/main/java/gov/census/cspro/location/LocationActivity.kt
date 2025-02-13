package gov.census.cspro.location

import android.content.Intent
import android.os.Bundle
import android.view.View
import android.widget.TextView
import androidx.appcompat.app.AppCompatActivity
import androidx.fragment.app.FragmentTransaction
import gov.census.cspro.csentry.R
import gov.census.cspro.csentry.ui.EntryActivity
import gov.census.cspro.engine.Messenger
import gov.census.cspro.engine.functions.GPSFunction
import gov.census.cspro.location.GpsReader.EnableListener
import gov.census.cspro.maps.MapData


class LocationActivity : AppCompatActivity(),
    LocationMaplessFragment.GpsResultCurrentDataSetListner,
    LocationMapFragment.GpsResultCurrentDataSetListner {

    //String constants
    val WAIT_TIME = "WAIT_TIME"
    val ACCURACY = "ACCURACY"
    val DIALOG_TEXT = "DIALOG_TEXT"
    val BASE_MAP_TYPE = "BASE_MAP_TYPE"
    val BASE_MAP_FILENAME  = "BASE_MAP_FILENAME"
    val DIALOG_TYPE = "DIALOG_TYPE"
    val DIALOG_TYPE_INTERACTIVE = 0
    val DIALOG_TYPE_SELECT = 1

    private var m_gpsResultCurrentData = "cancel"
    private var m_gpsReadWasAlreadyRunning = false

    override fun onCreate(savedInstanceState: Bundle?) {
        super.onCreate(savedInstanceState)

        setContentView(R.layout.activity_location)

        val dialogText = intent.getStringExtra(DIALOG_TEXT)
        val dialogTextView: TextView = findViewById(R.id.previewText)
        if (dialogText != null && dialogText.isNotEmpty()) {
            dialogTextView.text = dialogText
        } else {
            dialogTextView.visibility = View.GONE
        }

        val dialogType = intent.getIntExtra(DIALOG_TYPE, DIALOG_TYPE_INTERACTIVE)
        val baseMapType = intent.getIntExtra(BASE_MAP_TYPE, MapData.BASE_MAP_NONE)
        val fragmentType = getFragmentType(dialogType, baseMapType)

        //starting GPS reader
        val reader = GPSFunction.getReader()
        if (!reader.isRunning) {

            // Attempt to start
            reader.start(this, object : EnableListener {
                override fun onSuccess() {
                    //switch to location fragment
                    showLocationFragment(fragmentType)
                }

                override fun onFailure() {
                    //!!AI show error message here?

                    //!!end function with empty result?
                    m_gpsResultCurrentData = ""
                    finish()
                }
            })
        }
        else {
            m_gpsReadWasAlreadyRunning = true
            showLocationFragment(fragmentType)
        }
    }

    override fun onActivityResult(requestCode: Int, resultCode: Int, intent: Intent?) {
        super.onActivityResult(requestCode, resultCode, intent)
        if(requestCode == EntryActivity.EnableLocationInSettings) {
            // Returned from location settings, inform GPS reader
            GPSFunction.getReader().onSettingsResult(this)
        }
    }

    private fun showLocationFragment(fragmentType: Int) {
        //Creating fragments
        val ft: FragmentTransaction = supportFragmentManager.beginTransaction()
        if (fragmentType == 0) {
            ft.replace(R.id.fragmentPlaceholder, LocationMaplessFragment())
        } else {
            ft.replace(R.id.fragmentPlaceholder, LocationMapFragment.newInstance(fragmentType))
        }
        ft.commit()
    }

    private fun getFragmentType(dialogType: Int, baseMapType: Int) : Int {
        if (dialogType == DIALOG_TYPE_SELECT) {
            return 2
        } else if (baseMapType != MapData.BASE_MAP_NONE) {
            return 1
        } else {
            return 0
        }
    }

    override fun onDestroy() {
        super.onDestroy()
        closeGpsReader()
        endFunction(m_gpsResultCurrentData)
    }

    fun closeButtonClick(view: View?) {
        finish()
    }

    private fun endFunction(retVal: String) {
        Messenger.getInstance().engineFunctionComplete(retVal)
    }

    override fun onGpsResultSet(result: String) {
        m_gpsResultCurrentData = result
    }

    private fun closeGpsReader() {
        //Not stopping the GPS reader if it was running before this activity started
        if (m_gpsReadWasAlreadyRunning)
            return

        val reader = GPSFunction.getReader()
        if (reader.isRunning) {
            reader.stop()
        }
    }
}