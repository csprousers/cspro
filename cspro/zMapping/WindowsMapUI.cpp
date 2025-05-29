#include "stdafx.h"
#include "CurrentLocation.h"
#include "WindowsMapUI.h"
#include "WindowsMapDlg.h"
#include "WindowsMapUIThreadRunner.h"


WindowsMapUI::WindowsMapUI(cs::non_null_shared_or_raw_ptr<const MappingProperties> mapping_properties)
    :   HtmlMapUI(std::move(mapping_properties)),
        m_nextMapId(1)
{
}


WindowsMapUI::~WindowsMapUI()
{
    WaitForShowThreadToTerminate();
}


void WindowsMapUI::WaitForShowThreadToTerminate()
{
    const std::shared_ptr<WindowsMapUIThreadRunner> old_ui_thread_runner = std::move(m_uiThreadRunner);

    const std::unique_ptr<std::thread> old_show_thread = std::move(m_showThread);

    if( old_show_thread != nullptr && old_show_thread->joinable() )
        old_show_thread->join();
}


bool WindowsMapUI::Show()
{
    return WindowsShow();
}


bool WindowsMapUI::WindowsShow()
{
    // return if the map is already showing
    if( m_uiThreadRunner != nullptr )
        return false;

    ASSERT(m_showThread == nullptr);

    if( m_mapEvent != nullptr )
    {
        // if the map was hidden using map.hide(), there will be an
        // unprocessed map closing event posted by WindowsMapDlg's destructor
        std::lock_guard<std::mutex> lock(m_mapEventMutex);
        ASSERT(m_mapEvent->code == IMapUI::EventCode::MapClosed);
        m_mapEvent.reset();
    }

    // after showing the map, we must return to the engine,
    // so the map dialog will be launched in a new thread
    m_uiThreadRunner = std::make_shared<WindowsMapUIThreadRunner>(*this);

    m_showThread = std::make_unique<std::thread>([ui_thread_runner = m_uiThreadRunner]()
    {
        ui_thread_runner->RunOnUIThread();
    });

    return true;
}


WindowsMapDlg* WindowsMapUI::GetMapDlgForAction()
{
    return ( m_uiThreadRunner != nullptr ) ? m_uiThreadRunner->GetMapDlg() :
                                             nullptr;
}


template<typename Action>
void WindowsMapUI::PerformMapDlgAction(const Action action)
{
    WindowsMapDlg* const map_dlg = GetMapDlgForAction();

    if( map_dlg != nullptr )
        action(*map_dlg);
}


bool WindowsMapUI::Hide()
{
    if( m_uiThreadRunner == nullptr )
        return false;

    // send a message to close the dialog
    PerformMapDlgAction([&](WindowsMapDlg& map_dlg)
    {
        map_dlg.SendMessage(WM_CLOSE);
    });

    // wait for the dialog to fully close
    WaitForShowThreadToTerminate();

    return true;
}


bool WindowsMapUI::SaveSnapshot(const std::string& image_file_path)
{
    bool result = false;

    PerformMapDlgAction([&](WindowsMapDlg& map_dlg)
    {
        map_dlg.SaveSnapshot(image_file_path);
        result = true;
    });

    return result;
}


int WindowsMapUI::AddMarker(const double latitude, const double longitude)
{
    const Marker& marker = m_markers.try_emplace(m_nextMapId, Marker { latitude,
                                                                       longitude }).first->second;

    PerformMapDlgAction([&](WindowsMapDlg& map_dlg)
    {
        map_dlg.AddMarker(marker, m_nextMapId);
        FitMarkersIMIS();
    });

    return m_nextMapId++;
}


bool WindowsMapUI::RemoveMarker(const int marker_id)
{
    Marker* const marker = GetMarker(marker_id);

    if( marker == nullptr )
        return false;

    PerformMapDlgAction([&](WindowsMapDlg& map_dlg)
    {
        map_dlg.RemoveMarker(marker->leaflet_id);
    });

    m_markers.erase(marker_id);

    return true;
}


void WindowsMapUI::ClearMarkers()
{
    PerformMapDlgAction([&](WindowsMapDlg& map_dlg)
    {
        map_dlg.ClearMarkers();
    });

    m_markers.clear();
}


bool WindowsMapUI::SetMarkerImage(const int marker_id, const std::string& image_url_or_file_path)
{
    Marker* const marker = GetMarker(marker_id);

    if( marker == nullptr )
        return false;

    marker->image_url = GetUrlForUrlOrFile(image_url_or_file_path);

    PerformMapDlgAction([&](WindowsMapDlg& map_dlg)
    {
        map_dlg.SetMarkerImage(*marker);
    });

    return true;
}


bool WindowsMapUI::SetMarkerText(const int marker_id, SharableString text, const int background_color, const int text_color)
{
    Marker* const marker = GetMarker(marker_id);

    if( marker == nullptr )
        return false;

    marker->text = std::move(text);
    marker->background_color = PortableColor::FromColorInt(background_color);
    marker->text_color = PortableColor::FromColorInt(text_color);

    PerformMapDlgAction([&](WindowsMapDlg& map_dlg)
    {
        map_dlg.SetMarkerText(*marker);
    });

    return true;
}


bool WindowsMapUI::SetMarkerOnClick(const int marker_id, const int on_click_callback)
{
    Marker* const marker = GetMarker(marker_id);

    if( marker == nullptr )
        return false;

    marker->on_click_callback = on_click_callback;

    return true;
}


bool WindowsMapUI::SetMarkerOnClickInfoWindow(const int marker_id, const int on_click_callback)
{
    Marker* const marker = GetMarker(marker_id);

    if( marker == nullptr )
        return false;

    marker->on_info_window_click_callback = on_click_callback;

    PerformMapDlgAction([&](WindowsMapDlg& map_dlg)
    {
        map_dlg.SetMarkerDescription(*marker, marker_id);
    });

    return true;
}


bool WindowsMapUI::SetMarkerOnDrag(const int marker_id, const int on_drag_callback)
{
    Marker* const marker = GetMarker(marker_id);

    if( marker == nullptr )
        return false;

    marker->on_drag_callback = on_drag_callback;

    PerformMapDlgAction([&](WindowsMapDlg& map_dlg)
    {
        map_dlg.SetMarkerOnDrag(marker->leaflet_id);
    });

    return true;
}


bool WindowsMapUI::SetMarkerDescription(const int marker_id, SharableString description)
{
    Marker* const marker = GetMarker(marker_id);

    if( marker == nullptr )
        return false;

    marker->description = std::move(description);

    PerformMapDlgAction([&](WindowsMapDlg& map_dlg)
    {
        map_dlg.SetMarkerDescription(*marker, marker_id);
    });

    return true;
}


bool WindowsMapUI::SetMarkerLocation(const int marker_id, const double latitude, const double longitude)
{
    Marker* const marker = GetMarker(marker_id);

    if( marker == nullptr )
        return false;

    marker->latitude = latitude;
    marker->longitude = longitude;

    PerformMapDlgAction([&](WindowsMapDlg& map_dlg)
    {
        map_dlg.SetMarkerLocation(*marker);
        FitMarkersIMIS();
    });

    return true;
}


std::optional<std::tuple<double, double>> WindowsMapUI::GetMarkerLocation(const int marker_id)
{
    Marker* const marker = GetMarker(marker_id);

    if( marker == nullptr )
        return std::nullopt;

    return std::make_tuple(marker->latitude, marker->longitude);
}


int WindowsMapUI::AddImageButton(const std::string& image_url_or_file_path, const int on_click_callback)
{
    const Button& button = m_buttons.try_emplace(m_nextMapId, Button { Button::Type::Image,
                                                                       on_click_callback,
                                                                       GetUrlForUrlOrFile(image_url_or_file_path) }).first->second;

    PerformMapDlgAction([&](WindowsMapDlg& map_dlg)
    {
        map_dlg.AddImageButton(button, m_nextMapId);
    });

    return m_nextMapId++;
}


int WindowsMapUI::AddTextButton(SharableString label, const int on_click_callback)
{
    const Button& button = m_buttons.try_emplace(m_nextMapId, Button { Button::Type::Text,
                                                                       on_click_callback,
                                                                       std::move(label) }).first->second;

    PerformMapDlgAction([&](WindowsMapDlg& map_dlg)
    {
        map_dlg.AddTextButton(button, m_nextMapId);
    });

    return m_nextMapId++;
}


bool WindowsMapUI::RemoveButton(const int button_id)
{
    Button* const button = GetButton(button_id);

    if( button == nullptr )
        return false;

    PerformMapDlgAction([&](WindowsMapDlg& map_dlg)
    {
        map_dlg.RemoveButton(button_id);
    });

    m_buttons.erase(button_id);

    return true;
}


void WindowsMapUI::ClearButtons()
{
    PerformMapDlgAction([&](WindowsMapDlg& map_dlg)
    {
        map_dlg.ClearButtons();
    });

    m_buttons.clear();
}


void WindowsMapUI::Clear()
{
    __super::Clear();

    ClearButtons();
    ClearMarkers();
    ClearGeometry();
}


int WindowsMapUI::AddGeometry(std::shared_ptr<const Geometry::FeatureCollection> geometry, std::shared_ptr<const Geometry::BoundingBox> bounds)
{
    ASSERT(geometry != nullptr && bounds != nullptr);

    const MapGeometry& map_geometry = m_geometries.try_emplace(m_nextMapId, MapGeometry { std::move(geometry),
                                                                                          -1 }).first->second;

    PerformMapDlgAction([&](WindowsMapDlg& map_dlg)
    {
        map_dlg.AddGeometry(map_geometry, m_nextMapId);
    });

    return m_nextMapId++;
}


bool WindowsMapUI::RemoveGeometry(const int geometry_id)
{
    MapGeometry* const geometry = GetGeometry(geometry_id);

    if( geometry == nullptr )
        return false;

    PerformMapDlgAction([&](WindowsMapDlg& map_dlg)
    {
        map_dlg.RemoveGeometry(geometry->leaflet_id);
    });

    m_geometries.erase(geometry_id);

    return true;
}


void WindowsMapUI::ClearGeometry()
{
    PerformMapDlgAction([&](WindowsMapDlg& map_dlg)
    {
        map_dlg.ClearGeometry();
    });

    m_geometries.clear();
}


IMapUI::MapEvent WindowsMapUI::WaitForEvent()
{
    std::unique_ptr<MapEvent> received_map_event;

    // wait for an event
    while( m_mapEvent == nullptr )
        Sleep(5);

    // lock guard
    {
        std::lock_guard<std::mutex> lock(m_mapEventMutex);
        received_map_event = std::move(m_mapEvent);
    }

    // if the dialog is closing, wait for the show thread to terminate
    // before returning the event to the engine
    if( received_map_event->code == IMapUI::EventCode::MapClosed )
        WaitForShowThreadToTerminate();

    return *received_map_event;
}


void WindowsMapUI::NotifyEvent(const EventCode code, const int marker_id/* = -1*/, const int callback_id/* = -1*/,
                               const double latitude/* = 0*/, const double longitude/* = 0*/,
                               const MapCamera& camera/* = MapCamera { 0, 0, 0, 0 }*/)
{
    // wait until any existing events have been processed by the engine
    while( m_mapEvent != nullptr )
        Sleep(5);

    std::lock_guard<std::mutex> lock(m_mapEventMutex);

    m_mapEvent.reset(new MapEvent { code,
                                    marker_id,
                                    callback_id,
                                    latitude,
                                    longitude,
                                    camera });
}


bool WindowsMapUI::IsMapShowing()
{
    return ( GetMapDlgForAction() != nullptr );
}


void WindowsMapUI::OnPostActionMessage(SharableString action_message_json)
{
    ASSERT(IsMapShowing());

    WindowsDesktopMessage::PostObject(GetMapDlgForAction(), UWM::Mapping::PostActionMessage,
                                      std::move(action_message_json));
}


void WindowsMapUI::OnSetWindowTitle(const std::string& title)
{
    WindowsMapDlg* const map_dlg = GetMapDlgForAction();

    if( map_dlg != nullptr )
        map_dlg->SetWindowTitle(title);
}


bool WindowsMapUI::OnShowCurrentLocation()
{
    // only show the current location when it can be retrieved
    const std::optional<std::tuple<double, double>> current_location = CurrentLocation::GetCurrentLocation();

    if( !current_location.has_value() )
        return false;

    PostActionMessage("showCurrentLocation",
        [&](JsonWriter& json_writer)
        {
            json_writer.Write(JK::latitude, std::get<0>(*current_location))
                       .Write(JK::longitude, std::get<1>(*current_location));
        });

    return true;
}
