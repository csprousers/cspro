#pragma once

#include <zMapping/zMapping.h>
#include <zMapping/IMapUI.h>
#include <zUtilO/PortableColor.h>
#include <zAppO/Properties/MappingProperties.h>
#include <mutex>
#include <thread>

class OfflineTileProvider;
class OfflineTileReader;
class SharedHtmlLocalFileServer;
class WindowsMapUIThreadRunner;


class ZMAPPING_API WindowsMapUI : public IMapUI
{
    friend class WindowsMapDlg;

public:
    WindowsMapUI(const MappingProperties& mapping_properties);
    ~WindowsMapUI();

    int Show() override;

    int Hide() override;

    int SaveSnapshot(const std::wstring& filename) override;

    int AddMarker(double latitude, double longitude) override;

    int RemoveMarker(int marker_id) override;

    void ClearMarkers() override;

    int SetMarkerImage(int marker_id, std::wstring image_file_path) override;

    int SetMarkerText(int marker_id, std::wstring text, int background_color, int text_color) override;

    int SetMarkerOnClick(int marker_id, int on_click_callback) override;

    int SetMarkerOnClickInfoWindow(int marker_id, int on_click_callback) override;

    int SetMarkerOnDrag(int marker_id, int on_drag_callback) override;

    int SetMarkerDescription(int marker_id, std::wstring description) override;

    int SetMarkerLocation(int marker_id, double latitude, double longitude) override;

    int GetMarkerLocation(int marker_id, double& latitude, double& longitude) override;

    int AddImageButton(std::wstring image_path, int on_click_callback) override;

    int AddTextButton(std::wstring label, int on_click_callback) override;

    int RemoveButton(int button_id) override;

    void ClearButtons() override;

    void Clear() override;

    bool IsBaseMapDefined() const override;

    int SetBaseMap(const BaseMapSelection& base_map_selection) override;

    int SetShowCurrentLocation(bool show) override;

    int SetTitle(std::wstring title) override;

    int ZoomTo(double latitude, double longitude, double zoom = -1) override;

    int ZoomTo(double minLat, double minLong, double maxLat, double maxLong, double paddingPercent = 0) override;

    int SetCamera(const MapCamera& camera) override;

    int AddGeometry(std::shared_ptr<const Geometry::FeatureCollection> geometry, std::shared_ptr<const Geometry::BoundingBox> bounds) override;

    int RemoveGeometry(int geometry_id) override;

    void ClearGeometry() override;

    MapEvent WaitForEvent() override;

protected:
    virtual int WindowsShow();

    virtual WindowsMapDlg* GetMapDlgForAction();

private:
    template<typename Action>
    void PerformMapDlgAction(Action action);

private:
    struct Marker
    {
        double latitude = 0;
        double longitude = 0;
        int on_click_callback = -1;
        int on_drag_callback = -1;
        int on_info_window_click_callback = -1;
        int leaflet_id = -1;
        std::wstring image_url;
        std::wstring description;
        std::wstring text;
        PortableColor background_color = PortableColor::White;
        PortableColor text_color = PortableColor::Black;
    };

    enum class ButtonType { Text, Image };

    struct Button
    {
        ButtonType type;
        int on_click_callback;
        std::wstring content;
    };

    struct Zoom
    {
        double latitude;
        double longitude;
        double latitude2;
        double longitude2;
        double level;
    };

    struct MapGeometry
    {
        std::shared_ptr<const Geometry::FeatureCollection> geometry;
        int leaflet_id;
    };

protected:
    virtual void NotifyEvent(EventCode code, int marker_id = -1, int callback_id = -1,
        double latitude = 0, double longitude = 0, const MapCamera& camera = MapCamera { 0, 0, 0, 0 });

private:
    void WaitForShowThreadToTerminate();

    void EnsureFileServerIsSetup();

    std::wstring GetUrlOfMapHtml() const;
    std::wstring GetUrlForFilename(NullTerminatedStringView filename);

    Marker* GetMarker(int marker_id)
    {
        auto marker_search = m_markers.find(marker_id);
        return ( marker_search != m_markers.cend() ) ? &marker_search->second : nullptr;
    }

    Button* GetButton(int button_id)
    {
        auto button_search = m_buttons.find(button_id);
        return ( button_search != m_buttons.cend() ) ? &button_search->second : nullptr;
    }

    MapGeometry* GetGeometry(int geometry_id)
    {
        auto geometry_search = m_geometries.find(geometry_id);
        return ( geometry_search != m_geometries.cend() ) ? &geometry_search->second : nullptr;
    }

private:
    const MappingProperties& m_mappingProperties;

    std::shared_ptr<WindowsMapUIThreadRunner> m_uiThreadRunner;
    std::unique_ptr<std::thread> m_showThread;
    std::unique_ptr<MapEvent> m_mapEvent;
    std::mutex m_mapEventMutex;

    std::wstring m_fileServerDirectory;
    std::unique_ptr<SharedHtmlLocalFileServer> m_fileServer;

protected:
    std::wstring m_title;
    std::optional<BaseMapSelection> m_baseMapSelection;
    std::optional<Zoom> m_zoom;
    bool m_showCurrentLocation;
    int m_nextMapId;

    std::map<int, Marker> m_markers;
    std::map<int, Button> m_buttons;
    std::map<int, MapGeometry> m_geometries;

    std::shared_ptr<OfflineTileReader> m_tileReader;
    std::unique_ptr<OfflineTileProvider> m_tileProvider;
};
