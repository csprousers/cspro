#pragma once

#include <zMapping/WindowsMapUI.h>
#include <zHtml/HtmlViewDlg.h>
#include <zHtml/VirtualFileMapping.h>


class WindowsMapDlg : public HtmlViewDlg
{
public:
    WindowsMapDlg(WindowsMapUI& map_ui, CWnd* pParent = nullptr);
    ~WindowsMapDlg();

    void RemoveMarker(int leaflet_id);
    void AddMarker(const WindowsMapUI::Marker& marker, int id);
    void ClearMarkers();
    void MoveMarker(const WindowsMapUI::Marker& marker);
    void FitPoints();
    void SetDraggable(int leaflet_id);
    void SetDescription(const WindowsMapUI::Marker& marker, int id);
    void SetText(const WindowsMapUI::Marker& marker);
    void SetTitle(std::wstring title);
    void AddTextButton(const WindowsMapUI::Button& button, int id);
    void AddImageButton(const WindowsMapUI::Button& button, int id);
    void RemoveButton(int id);
    void ClearButtons();
    void SetupCurrentLocation();
    void SetZoom(double latitude, double longitude, double zoom);
    void SetZoom(double minLat, double maxLat, double minLong, double maxLong, double paddingPercent);
    void SetMarkerImage(const WindowsMapUI::Marker& marker);
    void SetupBaseMap();
    void AddGeometry(const WindowsMapUI::MapGeometry& geometry, int id);
    void RemoveGeometry(int leaflet_id);
    void ClearGeometry();

    void SaveSnapshot(const std::wstring& filename);

protected:
    DECLARE_MESSAGE_MAP()

    LRESULT OnExecuteJavaScript(WPARAM wParam, LPARAM lParam);
    LRESULT OnSaveSnapshot(WPARAM wParam, LPARAM lParam);

private:
    void OnWebMessageReceived(wstring_view message);
    void ExecuteJavaScript(const TCHAR* javascript);

    void SetupInitialMap();

private:
    WindowsMapUI& m_mapUI;
    bool m_loaded;
    std::vector<std::unique_ptr<VirtualFileMappingHandler>> m_geometryVirtualFileMappings;
};
