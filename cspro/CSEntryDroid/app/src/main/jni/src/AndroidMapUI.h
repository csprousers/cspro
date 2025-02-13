#pragma once

#include <zPlatformO/PortableWindowsDefines.h>
#include <zMapping/IMapUI.h>
#include <jni.h>


/**
* Map user interface widget implementation for Android
* that calls through to Java MapUI class via JNI.
*/
class AndroidMapUI : public IMapUI
{
public:
    AndroidMapUI();
    ~AndroidMapUI();

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

    jobject GetAndroidMapUI()
    {
        return java_impl_;
    }

    static jobject CreateJavaBaseMapSelection(JNIEnv* pEnv, const BaseMapSelection& base_map_selection);

private:
    jobject java_impl_;
    bool m_baseMapDefined;
};
