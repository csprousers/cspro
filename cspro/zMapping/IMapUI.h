#pragma once

#include <zMapping/Geometry.h>
#include <zAppO/MappingDefines.h>


// --------------------------------------------------------------------------
// IMapUI: the interface to a platform-specific mapping widget.
// --------------------------------------------------------------------------

class IMapUI
{
public:
    enum class EventCode;
    struct MapCamera;
    struct MapEvent;

    virtual ~IMapUI() { }

    // Displays the map on the screen.
    virtual bool Show() = 0;

    // Removes the map from the screen.
    virtual bool Hide() = 0;

    // Saves a snapshot (to JPEG/PNG formats) of the currently displayed map.
    virtual bool SaveSnapshot(const std::string& image_file_path) = 0;

    // Clears the map state.
    virtual void Clear() = 0;

    // Sets the title text that is displayed at the top of the map.
    virtual bool SetTitle(SharableString title) = 0;

    // Returns whether a base map has been set.
    virtual bool IsBaseMapDefined() const = 0;

    // Sets the map's base map.
    virtual bool SetBaseMap(BaseMapSelection base_map_selection) = 0;

    // Sets whether to show the current location on the map.
    virtual bool SetShowCurrentLocation(bool show) = 0;

    // Sets the position of the camera for the map display.
    virtual bool SetCamera(const MapCamera& camera) = 0;

    // Pans the camera to center at the specified point and optional zoom level.
    // Latitude and longitude are in degrees.
    // The zoom level is 0 for the entire level, and 20 to show individual buildings.
    virtual bool ZoomTo(double latitude, double longitude, double zoom = -1) = 0;

    // Pan/zoom to fit the rectangular region of the map to the screen
    // Latitude and longitude are in degrees.
    // The optional padding is as as percentage of the screen width.
    virtual bool ZoomTo(double min_latitude, double min_longitude, double max_latitude, double max_longitude, double padding_percent = 0) = 0;

    // Add a place mark to the map.
    // Latitude and longitude are in degrees.
    // Returns a unique ID of the marker.
    virtual int AddMarker(double latitude, double longitude) = 0;

    // Removes a place mark from the map.
    virtual bool RemoveMarker(int marker_id) = 0;

    // Remove all place marks from the map
    virtual void ClearMarkers() = 0;

    // Sets the marker icon as an image.
    virtual bool SetMarkerImage(int marker_id, const std::string& image_url_or_file_path) = 0;

    // Sets the marker icon as text.
    // The colors are supplied as color ints (AARRGGBB);
    virtual bool SetMarkerText(int marker_id, SharableString text, int background_color, int text_color) = 0;

    // Set a user-defined function callback that is executed when the marker is clicked.
    // The callback is the index of the function registered with the Map object.
    virtual bool SetMarkerOnClick(int marker_id, int on_click_callback) = 0;

    // Set a user-defined function callback that is executed when the popup info window is clicked.
    // The callback is the index of the function registered with the Map object.
    virtual bool SetMarkerOnClickInfoWindow(int marker_id, int on_click_callback) = 0;

    // Set a user-defined function callback that is executed when the marker is dragged.
    // The callback is the index of the function registered with the Map object.
    virtual bool SetMarkerOnDrag(int marker_id, int on_drag_callback) = 0;

    // Set the text description that is displayed in the info window when a marker is clicked and in list view.
    virtual bool SetMarkerDescription(int marker_id, SharableString description) = 0;

    // Sets the location of a place mark on the map.
    // Latitude and longitude are in degrees.
    virtual bool SetMarkerLocation(int marker_id, double latitude, double longitude) = 0;

    // Gets the location of a place mark on the map.
    // If there is an error getting the location, the method returns std::nullopt.
    virtual std::optional<std::tuple<double, double>> GetMarkerLocation(int marker_id) = 0;

    // Adds a button to the map as an image.
    // The callback is the index of the function registered with the Map object.
    // Returns a unique ID of the button.
    virtual int AddImageButton(const std::string& image_url_or_file_path, int on_click_callback) = 0;

    // Adds a button to the map as text.
    // The callback is the index of the function registered with the Map object.
    // Returns a unique ID of the button.
    virtual int AddTextButton(SharableString label, int on_click_callback) = 0;

    // Removes a button from the map.
    virtual bool RemoveButton(int button_id) = 0;

    // Removes all buttons from the map.
    virtual void ClearButtons() = 0;

    // Add a vector layer to the map.
    // Returns a unique ID of the geometry.
    virtual int AddGeometry(std::shared_ptr<const Geometry::FeatureCollection> geometry, std::shared_ptr<const Geometry::BoundingBox> bounds) = 0;

    // Removes a vector layer from the map.
    virtual bool RemoveGeometry(int geometry_id) = 0;

    // Removes all vector layers from the map
    virtual void ClearGeometry() = 0;

    // Synchronously waits for an event to occur on the map.
    virtual MapEvent WaitForEvent() = 0;
};


struct IMapUI::MapCamera
{
    double latitude;
    double longitude;
    float zoom;
    float bearing;
};


enum class IMapUI::EventCode
{
    MapClosed = 1,
    MarkerClicked = 2,
    MarkerInfoWindowClicked = 3,
    MarkerDragged = 4,
    MapClicked = 5,
    ButtonClicked = 6
};


struct IMapUI::MapEvent
{
    EventCode code;
    int marker_id = -1;
    int callback_id = -1;
    double latitude = 0;
    double longitude = 0;
    MapCamera camera = { 0, 0, 0, 0 };
};
