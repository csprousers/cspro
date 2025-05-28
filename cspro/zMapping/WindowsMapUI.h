#pragma once

#include <zMapping/zMapping.h>
#include <zMapping/HtmlMapUI.h>
#include <zUtilO/PortableColor.h>
#include <zAppO/Properties/MappingProperties.h>
#include <mutex>
#include <thread>

class WindowsMapUIThreadRunner;


// --------------------------------------------------------------------------
// Windows implementation of HTML-based mapping.
// --------------------------------------------------------------------------

class ZMAPPING_API WindowsMapUI : public HtmlMapUI
{
    friend class WindowsMapDlg;

    struct Button;
    struct MapGeometry;
    struct Marker;
    struct Zoom;

public:
    WindowsMapUI(cs::non_null_shared_or_raw_ptr<const MappingProperties> mapping_properties);
    ~WindowsMapUI();

    bool Show() override;
    bool Hide() override;

    bool SaveSnapshot(const std::string& image_file_path) override;

    int AddMarker(double latitude, double longitude) override;
    bool RemoveMarker(int marker_id) override;
    void ClearMarkers() override;

    bool SetMarkerImage(int marker_id, const std::string& image_url_or_file_path) override;
    bool SetMarkerText(int marker_id, SharableString text, int background_color, int text_color) override;
    bool SetMarkerOnClick(int marker_id, int on_click_callback) override;
    bool SetMarkerOnClickInfoWindow(int marker_id, int on_click_callback) override;
    bool SetMarkerOnDrag(int marker_id, int on_drag_callback) override;
    bool SetMarkerDescription(int marker_id, SharableString description) override;
    bool SetMarkerLocation(int marker_id, double latitude, double longitude) override;
    std::optional<std::tuple<double, double>> GetMarkerLocation(int marker_id) override;

    int AddImageButton(const std::string& image_url_or_file_path, int on_click_callback) override;
    int AddTextButton(SharableString label, int on_click_callback) override;
    bool RemoveButton(int button_id) override;
    void ClearButtons() override;

    void Clear() override;

    bool ZoomTo(double latitude, double longitude, double zoom = -1) override;
    bool ZoomTo(double min_latitude, double min_longitude, double max_latitude, double max_longitude, double padding_percent = 0) override;

    bool SetCamera(const MapCamera& camera) override;

    int AddGeometry(std::shared_ptr<const Geometry::FeatureCollection> geometry, std::shared_ptr<const Geometry::BoundingBox> bounds) override;
    bool RemoveGeometry(int geometry_id) override;
    void ClearGeometry() override;

    MapEvent WaitForEvent() override;

protected:
    virtual bool WindowsShow();

    virtual WindowsMapDlg* GetMapDlgForAction();

private:
    template<typename Action>
    void PerformMapDlgAction(Action action);

protected:
    virtual void NotifyEvent(EventCode code, int marker_id = -1, int callback_id = -1,
                             double latitude = 0, double longitude = 0, const MapCamera& camera = MapCamera { 0, 0, 0, 0 });

protected:
    // HtmlMapUI overrides
    bool IsMapShowing() override;

    void OnPostActionMessage(SharableString action_message_json) override;

    void OnSetWindowTitle(const std::string& title) override;

    bool OnShowCurrentLocation() override;

private:
    void WaitForShowThreadToTerminate();

    constexpr bool AreCoordinatesValid(double latitude, double longitude);

    Marker* GetMarker(int marker_id);
    Button* GetButton(int button_id);
    MapGeometry* GetGeometry(int geometry_id);

private:
    std::shared_ptr<WindowsMapUIThreadRunner> m_uiThreadRunner;
    std::unique_ptr<std::thread> m_showThread;
    std::unique_ptr<MapEvent> m_mapEvent;
    std::mutex m_mapEventMutex;

protected:
    std::unique_ptr<Zoom> m_zoom;
    int m_nextMapId;

    std::map<int, Marker> m_markers;
    std::map<int, Button> m_buttons;
    std::map<int, MapGeometry> m_geometries;
};


struct WindowsMapUI::Marker
{
    double latitude = 0;
    double longitude = 0;
    int on_click_callback = -1;
    int on_drag_callback = -1;
    int on_info_window_click_callback = -1;
    int leaflet_id = -1;
    std::string image_url;
    SharableString description;
    SharableString text;
    PortableColor background_color = PortableColor::White;
    PortableColor text_color = PortableColor::Black;
};


struct WindowsMapUI::Button
{
    enum class Type { Text, Image };
    Type type;
    int on_click_callback;
    SharableString content;
};


struct WindowsMapUI::MapGeometry
{
    std::shared_ptr<const Geometry::FeatureCollection> geometry;
    int leaflet_id;
};


struct WindowsMapUI::Zoom
{
    double latitude;
    double longitude;
    double latitude2;
    double longitude2;
    double level;
};



// --------------------------------------------------------------------------
// inline implementations
// --------------------------------------------------------------------------

inline WindowsMapUI::Marker* WindowsMapUI::GetMarker(const int marker_id)
{
    auto marker_search = m_markers.find(marker_id);
    return ( marker_search != m_markers.cend() ) ? &marker_search->second : nullptr;
}


inline WindowsMapUI::Button* WindowsMapUI::GetButton(const int button_id)
{
    auto button_search = m_buttons.find(button_id);
    return ( button_search != m_buttons.cend() ) ? &button_search->second : nullptr;
}


inline WindowsMapUI::MapGeometry* WindowsMapUI::GetGeometry(const int geometry_id)
{
    auto geometry_search = m_geometries.find(geometry_id);
    return ( geometry_search != m_geometries.cend() ) ? &geometry_search->second : nullptr;
}
