package gov.census.cspro.maps;

public class MapCameraPosition
{
    public final double latitude;

    public final double longitude;

    final float bearing;

    public final float zoom;

    MapCameraPosition(double latitude, double longitude, float zoom, float bearing)
    {
        this.latitude = latitude;
        this.longitude = longitude;
        this.bearing = bearing;
        this.zoom = zoom;
    }

    MapCameraPosition(MapCameraPosition rhs)
    {
        latitude = rhs.latitude;
        longitude = rhs.longitude;
        bearing = rhs.bearing;
        zoom = rhs.zoom;
    }
}
