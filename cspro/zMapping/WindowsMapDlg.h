#pragma once

#include <zMapping/WindowsMapUI.h>
#include <zHtml/HtmlViewDlg.h>
#include <zHtml/VirtualFileMapping.h>


class WindowsMapDlg : public HtmlViewDlg
{
public:
    WindowsMapDlg(WindowsMapUI& map_ui, CWnd* pParent = nullptr);
    ~WindowsMapDlg();

    void AddMarker(const WindowsMapUI::Marker& marker, int id);
    void RemoveMarker(int leaflet_id);
    void ClearMarkers();
    void SetMarkerImage(const WindowsMapUI::Marker& marker);
    void SetMarkerText(const WindowsMapUI::Marker& marker);
    void SetMarkerOnDrag(int leaflet_id);
    void SetMarkerDescription(const WindowsMapUI::Marker& marker, int id);
    void SetMarkerLocation(const WindowsMapUI::Marker& marker);
    void FitMarkers();

    void AddImageButton(const WindowsMapUI::Button& button, int id);
    void AddTextButton(const WindowsMapUI::Button& button, int id);
    void RemoveButton(int id);
    void ClearButtons();

    void SetShowCurrentLocation();

    void SetTitle(const std::string& title);

    void ZoomTo(double latitude, double longitude, double);
    void ZoomTo(double min_latitude, double min_longitude, double max_latitude, double max_longitude, double padding_percent);

    void SetUpBaseMap();

    void AddGeometry(const WindowsMapUI::MapGeometry& geometry, int id);
    void RemoveGeometry(int leaflet_id);
    void ClearGeometry();

    struct SnapshotData;
    void SaveSnapshot(const std::string& file_path);

protected:
    DECLARE_MESSAGE_MAP()

    LRESULT OnPostActionMessage(WPARAM wParam, LPARAM lParam);
    LRESULT OnSaveSnapshot(WPARAM wParam, LPARAM lParam);

private:
    void PostActionMessage(std::string_view action_sv);

    template<typename CF>
    void PostActionMessage(cs::string_sz action, const CF& callback_function);

    void OnWebMessageReceived(std::string_view message_sv);
    void OnWebMessageReceived(JsonNode json_node);

    void SetUpInitialMap();

private:
    WindowsMapUI& m_mapUI;
    bool m_loaded;
    std::vector<std::unique_ptr<VirtualFileMappingHandler>> m_geometryVirtualFileMappings;
};
