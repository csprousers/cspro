#pragma once

#include <zHtml/zHtml.h>
#include <zHtml/WebViewPermission.h>
#include <queue>

class CSProHostObject;
struct ICoreWebView2;
struct ICoreWebView2Controller;
struct ICoreWebView2NavigationCompletedEventArgs;
struct ICoreWebView2NavigationStartingEventArgs;
struct ICoreWebView2PermissionRequestedEventArgs;
struct ICoreWebView2WebMessageReceivedEventArgs;
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

    void SetBrowseInPrivate(bool enabled);
    void SetAllowExternalDrop(bool enabled);
    void SetContextMenuEnabled(bool enabled);
    void SetZoomControlEnabled(bool enabled);
    void SetOpenNonLocalhostLinksInBrowser(bool open_in_browser);
    void SetPermissions(const std::vector<WebViewPermission>& permissions);

    void NavigateTo(std::shared_ptr<UriResolver> uri_resolver);
    void NavigateTo(std::string_view uri_sv);
    void SetHtml(std::wstring html);
    void SetHtml(std::string_view html_sv);
    void Reload();

    void PostWebMessageAsJson(const std::wstring& message_json);
    void PostWebMessageAsJson(std::string_view message_json_sv);
    void PostWebMessageAsString(const std::wstring& message_string);
    void PostWebMessageAsString(std::string_view message_string_sv);
    void ExecuteScript(NullTerminatedString javascript);
    void ExecuteScript(NullTerminatedString javascript, std::function<void(const std::wstring&)> result_handler);
    void ExecuteScript(std::string_view javascript_sv);

    void AddWebViewCreatedObserver(std::function<void()> observer)                              { m_webViewCreatedObservers.emplace_back(std::move(observer)); }
    void AddNavigationStartingObserver(std::function<void(bool&, const std::string&)> observer) { m_navigationStartedObservers.emplace_back(std::move(observer)); }
    void AddSourceChangedObserver(std::function<void(const std::string&)> observer)             { m_sourceChangedObservers.emplace_back(std::move(observer)); }
    void AddNavigationCompletedObserver(std::function<void(bool)> observer)                     { m_navigationCompletedObservers.emplace_back(std::move(observer)); }
    void AddWebEventObserver(std::function<void(std::wstring_view)> observer)                   { m_webEventObservers.emplace_back(std::move(observer)); }

    void SetAcceleratorKeyHandler(std::function<bool(UINT message, UINT key, INT lParam)> handler);
    void UseWebView2AcceleratorKeyHandler();

    void MoveFocus();

    std::string GetSource();

    ActionInvoker::WebController& RegisterCSProHostObject();
    ActionInvoker::WebController* GetActionInvokerWebController();

    void ShowPrintUI();

    void SaveScreenshot(const std::string& file_path); // throws CSProException on error

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
    static const std::wstring& GetUserDataDirectory();
    void OnWebViewCreated(ICoreWebView2Controller* controller);
    void AddCSProHostObject();
    void ProcessPendingEvents();
    void OnNavigationStarted(ICoreWebView2NavigationStartingEventArgs* args);
    void OnSourceChanged();
    void OnNavigationCompleted(ICoreWebView2NavigationCompletedEventArgs* args);
    void OnWebMessageReceived(ICoreWebView2WebMessageReceivedEventArgs* args);
    void OnPermissionRequested(ICoreWebView2PermissionRequestedEventArgs* args);
    void ConfigureSettings();
    void FitWebViewToWindow();
    void SetupAcceleratorHandler();
    bool DefaultAcceleratorKeyHandler(UINT message, UINT key, INT lParam);

    template<typename T>
    static std::string GetSource(T* view_or_args, bool clear_about_blank);

private:
    struct Impl;
    std::unique_ptr<Impl> m_impl;

    bool m_initializeWebviewInPreSubclassWindow;

    std::vector<std::function<void()>> m_webViewCreatedObservers;
    std::vector<std::function<void(bool&, const std::string&)>> m_navigationStartedObservers;
    std::vector<std::function<void(const std::string&)>> m_sourceChangedObservers;
    std::vector<std::function<void(bool)>> m_navigationCompletedObservers;
    std::vector<std::function<void(std::wstring_view)>> m_webEventObservers;

    std::function<bool(UINT message, UINT key, INT lParam)> m_acceleratorKeyHandler;

    bool m_browseInPrivate;
    bool m_allowExternalDrop;
    bool m_contextMenuEnabled;
    bool m_zoomControlEnabled;
    bool m_openNonLocalhostLinksInBrowser;
    std::unique_ptr<std::vector<WebViewPermission>> m_permissions;
    bool m_initialized;

#ifdef ENABLE_ACTION_INVOKER
    std::unique_ptr<CSProHostObject> m_csproHostObject;
    std::queue<int> m_csproHostObjectAsyncMessageIds;
#endif

    // pending events to execute once the view is created
    struct PendingEvent_NavigateToUri  { std::string uri; };
    struct PendingEvent_SetHtml        { std::wstring html; };
    struct PendingEvent_ExecuteScript  { std::wstring javascript; std::function<void(const std::wstring&)> result_handler; };
    struct PendingEvent_PostWebMessage { std::wstring message; bool as_json; };

    using PendingEvent = std::variant<std::shared_ptr<UriResolver>,
                                      PendingEvent_NavigateToUri,
                                      PendingEvent_SetHtml,
                                      PendingEvent_ExecuteScript,
                                      PendingEvent_PostWebMessage>;
    std::vector<PendingEvent> m_pendingEvents;
};
