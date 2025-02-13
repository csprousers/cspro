package gov.census.cspro.maps;

public class MapEvent
{
    static final int MAP_CLOSED = 1;
    public static final int MARKER_CLICKED = 2;
    public static final int MARKER_INFO_WINDOW_CLICKED = 3;
    static final int MARKER_DRAGGED = 4;
    static final int MAP_CLICKED = 5;
    public static final int BUTTON_CLICKED = 6;

    public final int eventCode;

    final MapMarker marker;

    @SuppressWarnings("unused")
    private final int callbackId;

    @SuppressWarnings("unused")
    private final double latitude;

    @SuppressWarnings("unused")
    private final double longitude;

    private final MapCameraPosition camera;

    MapEvent(int eventCode, int callbackId, double latitude, double longitude, MapCameraPosition camera)
    {
        this.eventCode = eventCode;
        this.callbackId = callbackId;
        this.latitude = latitude;
        this.longitude = longitude;
        this.camera = camera;
        this.marker = null;
    }

    MapEvent(int eventCode, int callbackId, MapCameraPosition camera)
    {
        this.eventCode = eventCode;
        this.callbackId = callbackId;
        latitude = 0;
        longitude = 0;
        this.camera = camera;
        marker = null;
    }

    MapEvent(int eventCode, MapCameraPosition camera)
    {
        this.eventCode = eventCode;
        this.callbackId = -1;
        latitude = 0;
        longitude = 0;
        this.camera = camera;
        marker = null;
    }

    MapEvent(int eventCode, int callbackId, MapMarker marker, MapCameraPosition camera)
    {
        this.eventCode = eventCode;
        this.marker = marker;
        this.callbackId = callbackId;
        this.latitude = marker.getLatitude();
        this.longitude = marker.getLongitude();
        this.camera = camera;
    }

    @SuppressWarnings("unused")
    public int getMarkerId()
    {
        return marker == null ? -1 : marker.getId();
    }

    public int getCallback()
    {
        return callbackId;
    }

    public MapCameraPosition getCamera()
    {
        return camera;
    }
}
