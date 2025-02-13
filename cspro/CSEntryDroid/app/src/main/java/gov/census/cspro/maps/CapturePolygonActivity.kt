package gov.census.cspro.maps

import android.app.Activity
import android.content.Intent
import android.os.Bundle
import android.view.KeyEvent
import androidx.appcompat.app.AppCompatActivity
import com.google.android.gms.maps.model.LatLng
import gov.census.cspro.commonui.ErrorDialogFragment
import gov.census.cspro.csentry.R
import gov.census.cspro.csentry.ui.EntryActivity
import gov.census.cspro.engine.EngineInterface
import gov.census.cspro.engine.functions.GPSFunction
import gov.census.cspro.util.getDataHolderExtra

class CapturePolygonActivity : AppCompatActivity(), ErrorDialogFragment.OnErrorFragmentDismissed {

    companion object {
        const val EXTRA_POLYGON = "polygon"
        const val EXTRA_MODE = "mode"
        const val EXTRA_MAP = "map"
    }

    enum class PolygonCaptureMode { TRACE, WALK }

    override fun onCreate(savedInstanceState: Bundle?) {
        super.onCreate(savedInstanceState)

        // If is killed in background while this activity is shown, Android will try to restore
        // directly to this activity but that causes issues since the EntryActivity is not restored
        // to the point where it called to display the map. So just close the map activity so
        // that the EntryActivity is restored at the start of the questionnaire.
        if (EngineInterface.getInstance() == null || !EngineInterface.getInstance().isApplicationOpen) {
            finish()
        }

        supportActionBar?.hide()

        setContentView(R.layout.activity_capture_polygon)

        supportFragmentManager.setFragmentResultListener(CapturePolygonMapFragment.CAPTURE_POLYGON_FRAGMENT_RESULT, this) { requestKey, bundle ->
            if (requestKey == CapturePolygonMapFragment.CAPTURE_POLYGON_FRAGMENT_RESULT) {
                val resultIntent = Intent()
                resultIntent.putExtra(EXTRA_POLYGON, bundle.getParcelableArrayList<LatLng>(EXTRA_POLYGON))
                setResult(Activity.RESULT_OK, resultIntent)
                finish()
            }
        }
    }

    private fun getMapFragment(): MapFragment? {
        return supportFragmentManager.findFragmentById(R.id.map_fragment) as MapFragment?
    }

    override fun onStop() {
        MapManager.getInstance().onActivityStoppped()
        super.onStop()
    }

    override fun onKeyDown(keyCode: Int, event: KeyEvent?): Boolean {
        return super.onKeyDown(keyCode, event)
    }

    @Deprecated("Deprecated in Java")
    override fun onActivityResult(requestCode: Int, resultCode: Int, intent: Intent?) {
        super.onActivityResult(requestCode, resultCode, intent)

        if (requestCode == EntryActivity.EnableLocationInSettings) {
            // Returned from location settings, inform GPS reader
            GPSFunction.getReader().onSettingsResult(this)
        }
    }

    override fun errorDialogDismissed(code: Int) {
    }
}