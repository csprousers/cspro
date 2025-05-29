#pragma once

#include <zMapping/WindowsMapUI.h>


class ZMAPPING_API WindowsMapUISingleThread : public WindowsMapUI
{
public:
    WindowsMapUISingleThread(cs::non_null_shared_or_raw_ptr<const MappingProperties> mapping_properties);
    ~WindowsMapUISingleThread();

    bool Hide() override;

    MapEvent WaitForEvent() override;

protected:
    // WindowsMapUI overrides
    bool WindowsShow() override;

    WindowsMapDlg* GetMapDlgForAction() override;

    // HtmlMapUI overrides
    void OnNotifyEvent(std::unique_ptr<IMapUI::MapEvent> event) override;

private:
    std::unique_ptr<WindowsMapDlg> m_mapDlg;
    std::unique_ptr<MapEvent> m_singleMapEvent;
};
