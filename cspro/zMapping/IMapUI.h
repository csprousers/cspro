#pragma once

#include <zMapping/Geometry.h>
#include <zAppO/MappingDefines.h>


/// <summary>
/// Interface to platform specific mapping widget.
/// </summary>
struct IMapUI
{
    virtual ~IMapUI() { }

    /// <summary>
    /// Display the map on screen
    /// </summary>
    virtual int Show() = 0;

    /// <summary>
    /// Remove the map from display
    /// </summary>
    virtual int Hide() = 0;

    /// <summary>
    /// Saves a snapshot (to JPEG/PNG formats) of the currently displayed map
    /// </summary>
    virtual int SaveSnapshot(const std::wstring& filename) = 0;

    /// <summary>
    /// Add a place mark to the map
    /// </summary>
    /// <param name="latitude">Latitude of point at which to place marker in degrees.</param>
    /// <param name="longitude">Longitude of point at which to place marker in degrees.</param>
    /// <returns>Unique ID of marker</returns>
    virtual int AddMarker(double latitude, double longitude) = 0;

    /// <summary>
    /// Remove a place mark from the map
    /// </summary>
    /// <param name="marker_id">Unique ID of marker returned by AddMarker</param>
    virtual int RemoveMarker(int marker_id) = 0;

    /// <summary>
    /// Remove all place marks from the map
    /// </summary>
    virtual void ClearMarkers() = 0;

    /// <summary>
    /// Set marker icon
    /// </summary>
    /// <param name="marker_id">Unique ID of marker returned by AddMarker</param>
    /// <param name="image_file_path">Image file path on disk</param>
    virtual int SetMarkerImage(int marker_id, std::wstring image_file_path) = 0;

    /// <summary>
    /// Set marker icon as text
    /// </summary>
    /// <param name="marker_id">Unique ID of marker returned by AddMarker</param>
    /// <param name="text">Text to draw as marker icon</param>
    /// <param name="background_color">Color name or HTML color to use for icon text</param>
    /// <param name="text_color">Color name or HTML color to use for icon background</param>
    virtual int SetMarkerText(int marker_id, std::wstring text, int background_color, int text_color) = 0;

    /// <summary>
    /// Set user function callback that is executed when marker is clicked
    /// </summary>
    /// <param name="marker_id">Unique ID of marker returned by AddMarker</param>
    /// <param name="on_click_callback">Index of user function callback registered with map symbol.</param>
    virtual int SetMarkerOnClick(int marker_id, int on_click_callback) = 0;

    /// <summary>
    /// Set user function callback that is executed when popup info window is clicked
    /// </summary>
    /// <param name="marker_id">Unique ID of marker returned by AddMarker</param>
    /// <param name="on_click_callback">Index of user function callback registered with map symbol.</param>
    virtual int SetMarkerOnClickInfoWindow(int marker_id, int on_click_callback) = 0;

    /// <summary>
    /// Set user function callback that is executed when marker is dragged
    /// </summary>
    /// <param name="marker_id">Unique ID of marker returned by AddMarker</param>
    /// <param name="on_drag_callback">Index of user function callback registered with map symbol.</param>
    virtual int SetMarkerOnDrag(int marker_id, int on_drag_callback) = 0;

    /// <summary>
    /// Set text description that is displayed in info window when marker is clicked and in list view
    /// </summary>
    /// <param name="marker_id">Unique ID of marker returned by AddMarker</param>
    /// <param name="description">Text description</param>
    virtual int SetMarkerDescription(int marker_id, std::wstring description) = 0;

    /// <summary>
    /// Set location of place mark on map
    /// </summary>
    /// <param name="marker_id">Unique ID of marker returned by AddMarker</param>
    /// <param name="latitude">Latitude of point at which to place marker in degrees.</param>
    /// <param name="longitude">Longitude of point at which to place marker in degrees.</param>
    virtual int SetMarkerLocation(int marker_id, double latitude, double longitude) = 0;

    /// <summary>
    /// Get location of place mark on map
    /// </summary>
    /// <param name="marker_id">Unique ID of marker returned by AddMarker</param>
    /// <param name="latitude">Latitude of marker in degrees returned.</param>
    /// <param name="longitude">Longitude of marker in degrees returned.</param>
    virtual int GetMarkerLocation(int marker_id, double& latitude, double& longitude) = 0;

    /// <summary>
    /// Add a button to the map
    /// </summary>
    /// <param name="image_path">Path to image to use as button icon.</param>
    /// <param name="on_click_callback">Index of user function callback registered with map symbol.</param>
    /// <returns>Unique ID of button</returns>
    virtual int AddImageButton(std::wstring image_path, int on_click_callback) = 0;

    /// <summary>
    /// Add a button to the map
    /// </summary>
    /// <param name="label">Text to display on button.</param>
    /// <param name="on_click_callback">Index of user function callback registered with map symbol.</param>
    /// <returns>Unique ID of button</returns>
    virtual int AddTextButton(std::wstring label, int on_click_callback) = 0;

    /// <summary>
    /// Remove all buttons from the map
    /// </summary>
    virtual int RemoveButton(int button_id) = 0;

    /// <summary>
    /// Remove all place marks from the map
    /// </summary>
    virtual void ClearButtons() = 0;

    /// <summary>
    /// Clear map state
    /// </summary>
    virtual void Clear() = 0;

    /// <summary>
    /// Returns whether a base map has been set
    /// </summary>
    virtual bool IsBaseMapDefined() const = 0;

    /// <summary>
    /// Set base map displayed in map
    /// </summary>
    ///<param name = "base_map_selection">Type of basemap or path to base map file</param>
    virtual int SetBaseMap(const BaseMapSelection& base_map_selection) = 0;

    /// <summary>
    /// Set base map displayed in map
    /// </summary>
    ///<param name = "show">True to show, false to hide</param>
    virtual int SetShowCurrentLocation(bool show) = 0;

    /// <summary>
    /// Set title text that is displayed above map
    /// </summary>
    ///<param name = "title">Text to display</param>
    virtual int SetTitle(std::wstring title) = 0;

    /// <summary>
    /// Pan camera to center at point and set zoom level if specified
    /// </summary>
    /// <param name="latitude">Latitude of point in degrees.</param>
    /// <param name="longitude">Longitude of point in degree.</param>
    /// <param name="zoom">Optional zoom level (0 is entire world, 20 show individual buildings).</param>
    virtual int ZoomTo(double latitude, double longitude, double zoom = -1) = 0;

    /// <summary>
    /// Pan/zoom to fit rectangular region of map to screen
    /// <param name="minLat">Latitude of southern edge of region.</param>
    /// <param name="minLong">Longitude of eastern edge of region.</param>
    /// <param name="maxLat">Latitude of northern edge of region.</param>
    /// <param name="maxLong">Longitude of western edge of region.</param>
    /// <param name="paddingPercent">Padding as percentage of screen width</param>
    /// </summary>
    virtual int ZoomTo(double minLat, double minLong, double maxLat, double maxLong, double paddingPercent = 0) = 0;

    struct MapCamera
    {
        double latitude_;
        double longitude_;
        float zoom_;
        float bearing_;
    };

    /// <summary>
    /// Set position of camera for map display
    /// <param name="camera">new camera position</param>
    /// </summary>
    virtual int SetCamera(const MapCamera& camera) = 0;

    /// <summary>
    /// Add a vector layer to the map
    /// </summary>
    /// <param name="geometry">A feature collection to add to the map</param>
    /// <returns>Unique ID of layer</returns>
    virtual int AddGeometry(std::shared_ptr<const Geometry::FeatureCollection> geometry, std::shared_ptr<const Geometry::BoundingBox> bounds) = 0;

    /// <summary>
    /// Remove a vector layer from the map
    /// </summary>
    /// <param name="geometry_id">Unique ID of geometry layer returned by AddGeometry</param>
    virtual int RemoveGeometry(int geometry_id) = 0;

    /// <summary>
    /// Remove all vector layers from the map
    /// </summary>
    virtual void ClearGeometry() = 0;

    enum class EventCode
    {
        MapClosed = 1,
        MarkerClicked = 2,
        MarkerInfoWindowClicked = 3,
        MarkerDragged = 4,
        MapClicked = 5,
        ButtonClicked = 6
    };

    struct MapEvent
    {
        EventCode code_;
        int marker_id_;
        int callback_id_;
        double latitude_;
        double longitude_;
        MapCamera camera_;
    };

    virtual MapEvent WaitForEvent() = 0;
};
