#pragma once

#include <zMapping/WindowsMapUI.h>


class ZMAPPING_API WindowsMapUISingleThread : public WindowsMapUI
{
public:
    WindowsMapUISingleThread(const MappingProperties& mapping_properties);
    ~WindowsMapUISingleThread();

    int Hide() override;

    MapEvent WaitForEvent() override;

protected:
    int WindowsShow() override;

    WindowsMapDlg* GetMapDlgForAction() override;

    void NotifyEvent(EventCode code, int marker_id = -1, int callback_id = -1,
        double latitude = 0, double longitude = 0, const MapCamera& camera = MapCamera { 0, 0, 0, 0 }) override;

private:
    std::unique_ptr<WindowsMapDlg> m_mapDlg;
    std::unique_ptr<MapEvent> m_singleMapEvent;
};
