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

    struct Marker;

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

    void Clear() override;

    MapEvent WaitForEvent() override;

protected:
    virtual bool WindowsShow();

    virtual WindowsMapDlg* GetMapDlgForAction();

private:
    template<typename Action>
    void PerformMapDlgAction(Action action);

protected:
    // HtmlMapUI overrides
    bool IsMapShowing() override;

    void OnPostActionMessage(SharableString action_message_json) override;

    void OnNotifyEvent(std::unique_ptr<IMapUI::MapEvent> event) override;

    void OnSetWindowTitle(const std::string& title) override;

    bool OnShowCurrentLocation() override;

private:
    void WaitForShowThreadToTerminate();

    Marker* GetMarker(int marker_id);

private:
    std::shared_ptr<WindowsMapUIThreadRunner> m_uiThreadRunner;
    std::unique_ptr<std::thread> m_showThread;
    std::unique_ptr<MapEvent> m_mapEvent;
    std::mutex m_mapEventMutex;

protected:
    int m_nextMapId;

    std::map<int, Marker> m_markers;
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



// --------------------------------------------------------------------------
// inline implementations
// --------------------------------------------------------------------------

inline WindowsMapUI::Marker* WindowsMapUI::GetMarker(const int marker_id)
{
    auto marker_search = m_markers.find(marker_id);
    return ( marker_search != m_markers.cend() ) ? &marker_search->second : nullptr;
}
