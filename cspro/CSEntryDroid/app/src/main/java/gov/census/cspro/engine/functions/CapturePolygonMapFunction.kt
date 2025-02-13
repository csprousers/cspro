package gov.census.cspro.engine.functions

import android.app.Activity
import android.content.Intent
import com.google.android.gms.maps.model.LatLng
import gov.census.cspro.maps.geojson.Polygon
import gov.census.cspro.engine.Messenger
import gov.census.cspro.maps.CapturePolygonActivity
import gov.census.cspro.maps.MapData
import gov.census.cspro.util.putDataHolderExtra

class CapturePolygonMapFunction(val mode: CapturePolygonActivity.PolygonCaptureMode, private val existingPolygon: Polygon?, val mapData: MapData?) : EngineFunction {

    override fun runEngineFunction(activity: Activity?) {
        val intent = Intent(activity, CapturePolygonActivity::class.java)
        intent.putExtra(CapturePolygonActivity.EXTRA_MODE, mode.name)
        if (mapData != null)
            intent.putDataHolderExtra(CapturePolygonActivity.EXTRA_MAP, mapData)
        if (existingPolygon != null)
            intent.putDataHolderExtra(CapturePolygonActivity.EXTRA_POLYGON, existingPolygon)

        Messenger.getInstance().startActivityForResultFromEngineFunction(activity, { result ->
            if (result.resultCode == Activity.RESULT_OK) {
                Messenger.getInstance().engineFunctionComplete(result.data?.getParcelableArrayListExtra<LatLng>(CapturePolygonActivity.EXTRA_POLYGON))
            } else {
                Messenger.getInstance().engineFunctionComplete(null)
            }
        }, intent)
    }
}