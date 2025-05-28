#include "stdafx.h"
#include "WindowsMapUISingleThread.h"
#include "WindowsMapDlg.h"


WindowsMapUISingleThread::WindowsMapUISingleThread(cs::non_null_shared_or_raw_ptr<const MappingProperties> mapping_properties)
    :   WindowsMapUI(std::move(mapping_properties))
{
}

WindowsMapUISingleThread::~WindowsMapUISingleThread()
{
}


bool WindowsMapUISingleThread::WindowsShow()
{
    ASSERT(m_mapDlg == nullptr);

    m_mapDlg = std::make_unique<WindowsMapDlg>(*this);
    m_mapDlg->DoModal();

    m_mapDlg.reset();

    return true;
}


bool WindowsMapUISingleThread::Hide()
{
    ASSERT(m_mapDlg == nullptr);

    // clear any pending events
    m_singleMapEvent.reset();

    NotifyEvent(EventCode::MapClosed);

    return true;
}


WindowsMapDlg* WindowsMapUISingleThread::GetMapDlgForAction()
{
    return m_mapDlg.get();
}


IMapUI::MapEvent WindowsMapUISingleThread::WaitForEvent()
{
    ASSERT(m_mapDlg == nullptr);

    // return an event if one has already been logged
    if( m_singleMapEvent != nullptr )
    {
        const std::unique_ptr<MapEvent> received_map_event = std::move(m_singleMapEvent);
        return *received_map_event;
    }

    // otherwise show the dialog again and then wait for an event
    else
    {
        Show();
        return WaitForEvent();
    }
}


void WindowsMapUISingleThread::NotifyEvent(const EventCode code, const int marker_id/* = -1*/, const int callback_id/* = -1*/,
                                           const double latitude/* = 0*/, const double longitude/* = 0*/,
                                           const MapCamera& camera/* = MapCamera { 0, 0, 0, 0 }*/)
{
    // ignore the map closing event trigged by the WM_CLOSE message below
    if( m_singleMapEvent != nullptr )
    {
        ASSERT(m_singleMapEvent->code != EventCode::MapClosed);
        return;
    }

    // only one event is allowed at a time, so store the event...
    m_singleMapEvent.reset(new MapEvent
    {
        code,
        marker_id,
        callback_id,
        latitude,
        longitude,
        camera
    });

    // ...and then close the map dialog
    if( m_mapDlg != nullptr )
        m_mapDlg->SendMessage(WM_CLOSE);
}
