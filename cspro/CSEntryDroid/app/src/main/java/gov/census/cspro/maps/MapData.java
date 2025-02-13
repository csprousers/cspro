package gov.census.cspro.maps;

import androidx.annotation.ColorInt;
import android.util.SparseArray;

import com.google.android.gms.maps.model.LatLng;
import com.google.android.gms.maps.model.LatLngBounds;

import org.json.JSONException;
import org.json.JSONObject;

import java.io.BufferedReader;
import java.io.FileInputStream;
import java.io.IOException;
import java.io.InputStreamReader;
import java.util.List;

import gov.census.cspro.engine.BaseMapSelection;
import gov.census.cspro.engine.Util;
import gov.census.cspro.maps.geojson.FeatureCollection;
import timber.log.Timber;

/**
 * Data representing the state of a map including markers, buttons, camera...
 *
 */
public class MapData
{
    private final SparseArray<MapMarker> m_markers = new SparseArray<>();
    private int m_nextMarkerId;
    private final SparseArray<MapButton> m_buttons = new SparseArray<>();
    private int m_nextButtonId;
    private final SparseArray<FeatureCollection> m_geometry = new SparseArray<>();
    private int m_nextGeometryId;
    private boolean m_showCurrentLocation;
    private String m_title;
    private BaseMapSelection m_baseMapSelection;
    private LatLngBounds m_zoomToBounds;
    private double m_zoomToBoundsPaddingPct;
    private MapCameraPosition m_cameraPosition;
    private MapCameraPosition m_zoomToPoint;

    public MapData()
    {
        clear();
    }

    MapData(MapData rhs)
    {
        for (int i = 0; i < rhs.m_markers.size(); i++) {
            m_markers.put(rhs.m_markers.keyAt(i), new MapMarker(rhs.m_markers.valueAt(i)));
        }
        m_nextMarkerId = rhs.m_nextMarkerId;
        for (int i = 0; i < rhs.m_buttons.size(); i++) {
            m_buttons.put(rhs.m_buttons.keyAt(i), new MapButton(rhs.m_buttons.valueAt(i)));
        }
        m_nextButtonId = rhs.m_nextButtonId;
        for (int i = 0; i < rhs.m_geometry.size(); i++) {
            m_geometry.put(rhs.m_geometry.keyAt(i), rhs.m_geometry.valueAt(i));
        }
        m_nextGeometryId = rhs.m_nextGeometryId;
        m_showCurrentLocation = rhs.m_showCurrentLocation;
        m_title = rhs.m_title;
        m_baseMapSelection = rhs.m_baseMapSelection;
        m_zoomToPoint = rhs.m_zoomToPoint == null ? null : new MapCameraPosition(rhs.m_zoomToPoint);
        m_zoomToBounds = rhs.m_zoomToBounds;
        m_zoomToBoundsPaddingPct = rhs.m_zoomToBoundsPaddingPct;
        m_cameraPosition = rhs.m_cameraPosition == null ? null : new MapCameraPosition(rhs.m_cameraPosition);
    }

    List<MapMarker> getMarkers()
    {
        return Util.sparseArrayValues(m_markers);
    }

    public void setMarkers(List<MapMarker> markers)
    {
        m_markers.clear();
        for (MapMarker m : markers) {
            m_markers.put(m.getId(), m);
        }
    }

    public List<MapButton> getButtons()
    {
        return Util.sparseArrayValues(m_buttons);
    }

    boolean isShowCurrentLocation()
    {
        return m_showCurrentLocation;
    }

    void setShowCurrentLocation(boolean showCurrentLocation)
    {
        m_showCurrentLocation = showCurrentLocation;
    }

    public String getTitle()
    {
        return m_title;
    }

    public void setTitle(String title)
    {
        m_title = title;
    }

    BaseMapSelection getBaseMapSelection()
    {
        return m_baseMapSelection;
    }

    MapCameraPosition getZoomToPoint()
    {
        return m_zoomToPoint;
    }

    LatLngBounds getZoomToBounds()
    {
        return m_zoomToBounds;
    }

    double getZoomToBoundsPaddingPct()
    {
        return m_zoomToBoundsPaddingPct;
    }

    MapCameraPosition getCameraPosition()
    {
        return m_cameraPosition;
    }

    public static final int BASE_MAP_CUSTOM = 0;
    public static final int BASE_MAP_NORMAL = 1;
    public static final int BASE_MAP_HYBRID = 2;
    public static final int BASE_MAP_SATELLITE = 3;
    public static final int BASE_MAP_TERRAIN = 4;
    public static final int BASE_MAP_NONE = 5;

    int addMarker(double latitude, double longitude)
    {
        final int id = m_nextMarkerId++;
        MapMarker marker = new MapMarker(id, latitude, longitude);
        m_markers.put(id, marker);
        return id;
    }

    void removeMarker(int markerId)
    {
        m_markers.remove(markerId);
    }

    void clearMarkers()
    {
        m_markers.clear();
    }

    void setMarkerImage(int markerId, String imageFilePath)
    {
        MapMarker marker = getMarker(markerId);
        marker.setImagePath(imageFilePath);
    }

    void setMarkerText(int markerId, String text, @ColorInt int backgroundColor, @ColorInt int textColor)
    {
        MapMarker marker = getMarker(markerId);
        marker.setText(text);
        marker.setBackgroundColor(backgroundColor);
        marker.setTextColor(textColor);
    }

    void setMarkerOnClick(int markerId, int onClickCallback)
    {
        MapMarker marker = getMarker(markerId);
        marker.setOnClickCallback(onClickCallback);
    }

    void setMarkerOnClickInfoWindow(int markerId, int onClickCallback)
    {
        MapMarker marker = getMarker(markerId);
        marker.setOnClickInfoWindowCallback(onClickCallback);
    }

    void setMarkerOnDrag(int markerId, int onDragCallback)
    {
        MapMarker marker = getMarker(markerId);
        marker.setOnDragCallback(onDragCallback);
    }

    void setMarkerDescription(int markerId, String description)
    {
        MapMarker marker = getMarker(markerId);
        marker.setDescription(description);
    }

    void setMarkerLocation(int markerId, double latitude, double longitude)
    {
        MapMarker marker = getMarker(markerId);
        marker.setLocation(latitude,longitude);
    }

    void getMarkerLocation(int markerId, double[] latLon)
    {
        MapMarker marker = getMarker(markerId);
        latLon[0] = marker.getLatitude();
        latLon[1] = marker.getLongitude();
    }

    private MapMarker getMarker(int markerId)
    {
        MapMarker marker = m_markers.get(markerId);
        if (marker == null)
            throw new IllegalArgumentException("Invalid markerId");
        return marker;
    }

    int addButton(String imagePath, String label, int callbackId)
    {
        final int id = m_nextButtonId++;
        MapButton button = new MapButton(id, imagePath, label, callbackId);
        m_buttons.put(id, button);
        return id;
    }

    public int addButton(int imageResourceId, String label, int callbackId)
    {
        final int id = m_nextButtonId++;
        MapButton button = new MapButton(id, imageResourceId, label, callbackId);
        m_buttons.put(id, button);
        return id;
    }

    void removeButton(int buttonId)
    {
        MapButton button = m_buttons.get(buttonId);
        if (button == null)
            throw new IllegalArgumentException("Invalid buttonId");
        m_buttons.remove(buttonId);
    }

    void clearButtons()
    {
        m_buttons.clear();
    }

    public void setBaseMap(BaseMapSelection baseMapSelection)
    {
        m_baseMapSelection = baseMapSelection;
    }

    public void setCameraPosition(MapCameraPosition cameraPosition)
    {
        m_cameraPosition = cameraPosition;
        m_zoomToPoint = null;
        m_zoomToBounds = null;
    }

    void zoomToPoint(double latitude, double longitude, float zoom)
    {
        m_zoomToPoint = new MapCameraPosition(latitude, longitude, zoom, 0);
        m_cameraPosition = null;
        m_zoomToBounds = null;
    }

    void zoomToBounds(double minLat, double minLong, double maxLat, double maxLong, double paddingPct)
    {
        m_zoomToBounds = new LatLngBounds(new LatLng(minLat, minLong), new LatLng(maxLat, maxLong));
        m_zoomToBoundsPaddingPct = paddingPct;
        m_zoomToPoint = null;
        m_cameraPosition = null;
    }

    void zoomToBounds(LatLngBounds bounds, double paddingPct)
    {
        m_zoomToBounds = bounds;
        m_zoomToBoundsPaddingPct = paddingPct;
        m_zoomToPoint = null;
        m_cameraPosition = null;
    }

    public void clear()
    {
        m_baseMapSelection = new BaseMapSelection(BASE_MAP_NORMAL);
        m_nextMarkerId = 1;
        m_markers.clear();
        m_nextButtonId = 1;
        m_buttons.clear();
        m_showCurrentLocation = true;
        m_title = null;
        m_cameraPosition = null;
        m_zoomToPoint = null;
        m_zoomToBounds = null;
        m_zoomToBoundsPaddingPct = 0;
    }

    public List<FeatureCollection> getGeometry() {
        return Util.sparseArrayValues(m_geometry);
    }

    public int addGeometry(FeatureCollection geometry) {
        final int id = m_nextGeometryId++;
        m_geometry.put(id, geometry);
        return id;
    }

    public void removeGeometry(int geometryId) {
        m_geometry.remove(geometryId);
    }

    public void clearGeometry() {
        m_geometry.clear();
    }

}
