#pragma once

#include <zHtml/zHtml.h>
#include <queue>

class CSProHostObject;
struct ICoreWebView2;
struct ICoreWebView2Controller;
struct ICoreWebView2NavigationCompletedEventArgs;
class UriResolver;
namespace ActionInvoker { class WebController; }


class ZHTML_API HtmlViewCtrl : public CWnd
{
    DECLARE_DYNCREATE(HtmlViewCtrl)

public:
    HtmlViewCtrl(bool initialize_webview_in_pre_subclass_window = true);
    HtmlViewCtrl(const HtmlViewCtrl&) = delete;
    HtmlViewCtrl(HtmlViewCtrl&&) = delete;
    ~HtmlViewCtrl();

    void SetContextMenuEnabled(bool enabled);
    void SetOpenNonLocalhostLinksInBrowser(bool open_in_browser);

    void NavigateTo(std::shared_ptr<UriResolver> uri_resolver);
    void NavigateTo(const std::wstring& uri);
    void SetHtml(std::wstring html);
    void Reload();

    void PostWebMessageAsJson(const std::wstring& message_json);
    void PostWebMessageAsString(const std::wstring& message_string);
    void ExecuteScript(NullTerminatedString javascript);
    void ExecuteScript(NullTerminatedString javascript, std::function<void(const std::wstring&)> result_handler);

    void AddWebViewCreatedObserver(std::function<void()> observer);
    void AddSourceChangedObserver(std::function<void(const std::wstring& event_json)> observer);
    void AddNavigationCompletedObserver(std::function<void(bool)> observer);
    void AddWebEventObserver(std::function<void(const std::wstring& event_json)> observer);

    void SetAcceleratorKeyHandler(std::function<bool(UINT message, UINT key, INT lParam)> handler);
    void UseWebView2AcceleratorKeyHandler();

    void MoveFocus();
    void Resize(UINT nType, int cx, int cy) { OnSize(nType, cx, cy); }

    std::wstring GetSource();

    ActionInvoker::WebController& RegisterCSProHostObject();

    void SaveScreenshot(NullTerminatedString filename); // throws CSProException on error

protected:
    DECLARE_MESSAGE_MAP()

    void PreSubclassWindow() override;

    afx_msg int OnCreate(LPCREATESTRUCT lpCreateStruct);
    afx_msg void OnSize(UINT nType, int cx, int cy);
    afx_msg void OnDestroy();

    ICoreWebView2* GetWebView();
    ICoreWebView2Controller* GetController();

    LRESULT OnActionInvokerProcessAsyncMessage(WPARAM wParam, LPARAM lParam);

private:
    void InitializeWebView();
    static std::wstring GetUserDataDirectory();
    void OnWebViewCreated(ICoreWebView2Controller* controller);
    void OnSourceChanged(const std::wstring& uri);
    void OnNavigationCompleted(ICoreWebView2NavigationCompletedEventArgs* args);
    void ConfigureSettings();
    void SetupWebMessageReceiver();
    void FitWebViewToWindow();
    void SetupAcceleratorHandler();
    bool DefaultAcceleratorKeyHandler(UINT message, UINT key, INT lParam);

private:
    struct Impl;
    std::unique_ptr<Impl> m_impl;

    bool m_initializeWebviewInPreSubclassWindow;

    std::vector<std::function<void()>> m_webview_created_observers;
    std::vector<std::function<void(const std::wstring&)>> m_source_changed_observers;
    std::vector<std::function<void(bool)>> m_navigation_completed_observers;
    std::vector<std::function<void(const std::wstring&)>> m_web_event_observers;
    std::function<bool(UINT message, UINT key, INT lParam)> m_accelerator_key_handler;

    bool m_contextMenuEnabled;
    bool m_openNonLocalhostLinksInBrowser;
    bool m_initialized;

    std::unique_ptr<CSProHostObject> m_csproHostObject;
    std::queue<int> m_csproHostObjectAsyncMessageIds;

    // pending events to execute once the view is created
    struct PendingEvent_NavigateToUri { std::wstring uri; };
    struct PendingEvent_SetHtml       { std::wstring html; };
    struct PendingEvent_ExecuteScript { std::wstring javascript; std::function<void(const std::wstring&)> result_handler; };
    using PendingEvent = std::variant<std::shared_ptr<UriResolver>, PendingEvent_NavigateToUri, PendingEvent_SetHtml, PendingEvent_ExecuteScript>;
    std::vector<PendingEvent> m_pendingEvents;
};
