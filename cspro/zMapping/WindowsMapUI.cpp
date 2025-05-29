#include "stdafx.h"
#include "WindowsMapUI.h"
#include "CurrentLocation.h"
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


void WindowsMapUI::Clear()
{
    __super::Clear();

    ClearMarkers();
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


void WindowsMapUI::OnNotifyEvent(std::unique_ptr<IMapUI::MapEvent> event)
{
    ASSERT(event != nullptr);

    // wait until any existing events have been processed by the engine
    while( m_mapEvent != nullptr )
        Sleep(5);

    std::lock_guard<std::mutex> lock(m_mapEventMutex);
    m_mapEvent = std::move(event);
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
