#include "stdafx.h"
#include "WindowsMapUI.h"
#include "CurrentLocation.h"
#include "WindowsMapDlg.h"
#include "WindowsMapUIThreadRunner.h"


WindowsMapUI::WindowsMapUI(cs::non_null_shared_or_raw_ptr<const MappingProperties> mapping_properties)
    :   HtmlMapUI(std::move(mapping_properties))
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
    m_uiThreadRunner = std::make_unique<WindowsMapUIThreadRunner>(*this);

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


bool WindowsMapUI::Hide()
{
    if( m_uiThreadRunner == nullptr )
        return false;

    WindowsMapDlg* const map_dlg = GetMapDlgForAction();

    // send a message to close the dialog
    if( map_dlg != nullptr )
        map_dlg->SendMessage(WM_CLOSE);

    // wait for the dialog to fully close
    WaitForShowThreadToTerminate();

    return true;
}


bool WindowsMapUI::SaveSnapshot(const std::string& image_file_path)
{
    WindowsMapDlg* const map_dlg = GetMapDlgForAction();

    if( map_dlg == nullptr )
        return false;

    map_dlg->SaveSnapshot(image_file_path);

    return true;
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
