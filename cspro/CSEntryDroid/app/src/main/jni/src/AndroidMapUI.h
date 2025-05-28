#pragma once

#include <zPlatformO/PortableWindowsDefines.h>
#include <zMapping/IMapUI.h>
#include <jni.h>


// --------------------------------------------------------------------------
// Android implementation of mapping that calls through to Java MapUI class
// via JNI.
// --------------------------------------------------------------------------

class AndroidMapUI : public IMapUI
{
public:
    AndroidMapUI();
    ~AndroidMapUI();

    bool Show() override;
    bool Hide() override;

    bool SaveSnapshot(const std::string& image_file_path) override;

    void Clear() override;

    bool SetTitle(SharableString title) override;

    bool IsBaseMapDefined() const override;
    bool SetBaseMap(BaseMapSelection base_map_selection) override;

    bool SetShowCurrentLocation(bool show) override;

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

    bool ZoomTo(double latitude, double longitude, double zoom = -1) override;
    bool ZoomTo(double min_latitude, double min_longitude, double max_latitude, double max_longitude, double padding_percent = 0) override;

    bool SetCamera(const MapCamera& camera) override;

    int AddGeometry(std::shared_ptr<const Geometry::FeatureCollection> geometry, std::shared_ptr<const Geometry::BoundingBox> bounds) override;
    bool RemoveGeometry(int geometry_id) override;
    void ClearGeometry() override;

    MapEvent WaitForEvent() override;

    jobject GetAndroidMapUI() { return m_javaImpl; }

    static jobject CreateJavaBaseMapSelection(JNIEnv* pEnv, const BaseMapSelection& base_map_selection);

private:
    jobject m_javaImpl;
    bool m_baseMapDefined;
};
