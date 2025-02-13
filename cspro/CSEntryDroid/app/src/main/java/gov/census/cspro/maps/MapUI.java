package gov.census.cspro.maps;

import android.app.Activity;

import androidx.annotation.ColorInt;

import gov.census.cspro.engine.BaseMapSelection;
import gov.census.cspro.engine.Messenger;
import gov.census.cspro.engine.functions.EngineFunction;
import gov.census.cspro.engine.functions.MapHideFunction;
import gov.census.cspro.engine.functions.MapShowFunction;
import gov.census.cspro.engine.functions.fragments.MapUpdateFunction;
import gov.census.cspro.maps.geojson.FeatureCollection;

import timber.log.Timber;

@SuppressWarnings("unused")
public class MapUI
{
    private MapData m_mapData = new MapData();

    public MapData getMapData()
    {
        return m_mapData;
    }

    public int show()
    {
        Timber.d("Show");
        return (int) Messenger.getInstance().runLongEngineFunction(new MapShowFunction(this));
    }

    public int hide()
    {
        Timber.d("hide");
        return (int) Messenger.getInstance().runLongEngineFunction(new MapHideFunction(this));
    }

    public String saveSnapshot(String imagePath)
    {
        MapUI map = this;

        return Messenger.getInstance().runStringEngineFunction(new EngineFunction()
        {
            @Override
            public void runEngineFunction(Activity activity)
            {
                MapManager.getInstance().saveSnapshot(map, activity, imagePath);
            }
        });
    }

    public MapEvent waitForEvent()
    {
        Timber.d("Wait for event");
        // Update the mapData for observing activity
        // Since this is done through an EngineFunction the engine pauses and waits for events
        // Events will be sent to engine from map by calling EngineFunctionComplete
        // which will "wake up" engine thread.
        return (MapEvent) Messenger.getInstance().runObjectEngineFunction(new MapUpdateFunction(this));
    }

    public int addMarker(double latitude, double longitude)
    {
        return m_mapData.addMarker(latitude, longitude);
    }

    public int removeMarker(int markerId)
    {
        try {
            m_mapData.removeMarker(markerId);
            return 1;
        } catch (IllegalArgumentException ignored) {
            return 0;
        }
    }

    public void clearMarkers()
    {
        m_mapData.clearMarkers();
    }

    int setMarkerImage(int markerId, String imageFilePath)
    {
        try {
            m_mapData.setMarkerImage(markerId, imageFilePath);
            return 1;
        } catch (IllegalArgumentException ignored) {
            return 0;
        }
    }

    int setMarkerText(int markerId, String text, @ColorInt int backgroundColor, @ColorInt int textColor)
    {
        try {
            m_mapData.setMarkerText(markerId, text, backgroundColor, textColor);
            return 1;
        } catch (IllegalArgumentException ignored) {
            return 0;
        }
    }

    int setMarkerOnClick(int markerId, int onClickCallback)
    {
        try {
            m_mapData.setMarkerOnClick(markerId, onClickCallback);
            return 1;
        } catch (IllegalArgumentException ignored) {
            return 0;
        }
    }

    int setMarkerOnClickInfoWindow(int markerId, int onClickCallback)
    {
        try {
            m_mapData.setMarkerOnClickInfoWindow(markerId, onClickCallback);
            return 1;
        } catch (IllegalArgumentException ignored) {
            return 0;
        }
    }

    int setMarkerOnDrag(int markerId, int onDragCallback)
    {
        try {
            m_mapData.setMarkerOnDrag(markerId, onDragCallback);
            return 1;
        } catch (IllegalArgumentException ignored) {
            return 0;
        }
    }

    int setMarkerDescription(int markerId, String description)
    {
        try {
            m_mapData.setMarkerDescription(markerId, description);
            return 1;
        } catch (IllegalArgumentException ignored) {
            return 0;
        }
    }

    int setMarkerLocation(int markerId, double latitude, double longitude)
    {
        try {
            m_mapData.setMarkerLocation(markerId, latitude, longitude);
            return 1;
        } catch (IllegalArgumentException ignored) {
            return 0;
        }
    }

    int getMarkerLocation(int markerId, double[] latLon)
    {
        try {
            m_mapData.getMarkerLocation(markerId, latLon);
            return 1;
        } catch (IllegalArgumentException ignored) {
            return 0;
        }
    }

    public void clear()
    {
        m_mapData.clear();
    }

    public int addImageButton(String imagePath, int callbackId)
    {
       return addButton(imagePath, null, callbackId);
    }

    public int addTextButton(String label, int callbackId)
    {
        return addButton(null, label, callbackId);
    }

    private int addButton(String imagePath, String label, int callbackId)
    {
        return m_mapData.addButton(imagePath, label, callbackId);
    }

    public int removeButton(int buttonId)
    {
        try {
            m_mapData.removeButton(buttonId);
            return 1;
        } catch (IllegalArgumentException ignored)
        {
            return 0;
        }
    }

    public void clearButtons()
    {
        m_mapData.clearButtons();
    }

    public int setBaseMap(BaseMapSelection baseMapSelection)
    {
        m_mapData.setBaseMap(baseMapSelection);
        return 1;
    }

    public int setShowCurrentLocation(boolean show)
    {
        m_mapData.setShowCurrentLocation(show);
        return 1;
    }

    public int setTitle(String title)
    {
        m_mapData.setTitle(title);
        return 1;
    }

    public int zoomToPoint(double latitude, double longitude, float zoom)
    {
        m_mapData.zoomToPoint(latitude, longitude, zoom);
        return 1;
    }

    public int zoomToBounds(double minLat, double minLong, double maxLat, double maxLong, double paddingPct)
    {
        try
        {
            m_mapData.zoomToBounds(minLat, minLong, maxLat, maxLong, paddingPct);
            return 1;
        } catch (IllegalArgumentException ignored) {
            // If bounds are not correct (min > max for example) can get an exception here
            return 0;
        }
    }

    public int setCamera(MapCameraPosition cameraPosition)
    {
        m_mapData.setCameraPosition(cameraPosition);
        return 1;
    }

    public int addGeometry(FeatureCollection geometry)
    {
        return m_mapData.addGeometry(geometry);
    }

    public int removeGeometry(int geometryId)
    {
        try {
            m_mapData.removeGeometry(geometryId);
            return 1;
        } catch (IllegalArgumentException ignored) {
            return 0;
        }
    }

    public void clearGeometry()
    {
        m_mapData.clearGeometry();
    }
}
