#pragma once

#include <zMapping/zMapping.h>
#include <zMapping/HtmlMapUI.h>
#include <mutex>
#include <thread>

class WindowsMapUIThreadRunner;


// --------------------------------------------------------------------------
// Windows implementation of HTML-based mapping.
// --------------------------------------------------------------------------

class ZMAPPING_API WindowsMapUI : public HtmlMapUI
{
    friend class WindowsMapDlg;

public:
    WindowsMapUI(cs::non_null_shared_or_raw_ptr<const MappingProperties> mapping_properties);
    ~WindowsMapUI();

    bool Show() override;
    bool Hide() override;

    bool SaveSnapshot(const std::string& image_file_path) override;

    MapEvent WaitForEvent() override;

protected:
    // HtmlMapUI overrides
    bool IsMapShowing() override;

    void OnPostActionMessage(SharableString action_message_json) override;

    void OnNotifyEvent(std::unique_ptr<IMapUI::MapEvent> event) override;

    void OnSetWindowTitle(const std::string& title) override;

    bool OnShowCurrentLocation() override;

protected:
    // the following methods can be overridden by subclasses:
    virtual bool WindowsShow();

    virtual WindowsMapDlg* GetMapDlgForAction();

private:
    void WaitForShowThreadToTerminate();

private:
    std::shared_ptr<WindowsMapUIThreadRunner> m_uiThreadRunner;
    std::unique_ptr<std::thread> m_showThread;
    std::unique_ptr<MapEvent> m_mapEvent;
    std::mutex m_mapEventMutex;
};
