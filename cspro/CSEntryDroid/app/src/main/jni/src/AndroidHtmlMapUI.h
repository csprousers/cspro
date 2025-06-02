#pragma once

#include <zMapping/HtmlMapUI.h>
#include <jni.h>


// --------------------------------------------------------------------------
// Android implementation of HTML-based mapping.
// --------------------------------------------------------------------------

class AndroidHtmlMapUI : public HtmlMapUI
{
public:
    AndroidHtmlMapUI(cs::non_null_shared_or_raw_ptr<const MappingProperties> mapping_properties);
    ~AndroidHtmlMapUI();

    void NotifyLifecycle(jobject jHtmlMapActivity);
    void NotifyWebMessageReceived(std::string_view event_json_sv);

    // IMapUI overrides
    bool Show() override;
    bool Hide() override;

    bool SaveSnapshot(const std::string& image_file_path) override;

    MapEvent WaitForEvent() override;

private:
    // HtmlMapUI overrides
    bool IsMapShowing() override;

    void OnPostActionMessage(SharableString action_message_json) override;

    void OnNotifyEvent(std::unique_ptr<MapEvent> event) override;

    void OnSetWindowTitle(const std::string& title) override;

    bool OnShowCurrentLocation() override;

private:
    jobject m_jHtmlMapActivity;
    std::unique_ptr<MapEvent> m_mapEvent;
    std::mutex m_mapEventMutex;
    bool m_waitingForMapEvent;
};
