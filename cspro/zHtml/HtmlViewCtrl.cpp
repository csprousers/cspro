#include "stdafx.h"
#include "HtmlViewCtrl.h"
#include "CSProHostObject.h"
#include "UriResolver.h"
#include <zToolsO/DirectoryLister.h>
#include <zUtilO/Viewers.h>
#include <WebView2.h>
#include <wrl.h>
#include <wil/com.h>


IMPLEMENT_DYNCREATE(HtmlViewCtrl, CWnd)

BEGIN_MESSAGE_MAP(HtmlViewCtrl, CWnd)
    ON_WM_CREATE()
    ON_WM_SIZE()
    ON_WM_DESTROY()
    ON_MESSAGE(UWM::Html::ActionInvokerProcessAsyncMessage, OnActionInvokerProcessAsyncMessage)
END_MESSAGE_MAP()


struct HtmlViewCtrl::Impl
{
    ~Impl();

    wil::com_ptr<ICoreWebView2> view;
    wil::com_ptr<ICoreWebView2Controller> controller;
};


HtmlViewCtrl::HtmlViewCtrl(bool initialize_webview_in_pre_subclass_window/* = true*/)
    :   m_impl(std::make_unique<HtmlViewCtrl::Impl>()),
        m_initializeWebviewInPreSubclassWindow(initialize_webview_in_pre_subclass_window),
        m_allowExternalDrop(false),
        m_contextMenuEnabled(true),
        m_zoomControlEnabled(false),
        m_openNonLocalhostLinksInBrowser(false),
        m_initialized(false),
        m_acceleratorKeyHandler([this](UINT message, UINT key, INT lParam) { return DefaultAcceleratorKeyHandler(message, key, lParam); })
{
}


HtmlViewCtrl::~HtmlViewCtrl()
{
}


HtmlViewCtrl::Impl::~Impl()
{
    if( controller != nullptr )
    {
        controller->Close();
        controller.reset();
        view.reset();
    }
}


void HtmlViewCtrl::SetAllowExternalDrop(const bool enabled)
{
    m_allowExternalDrop = enabled;

    if( m_impl->controller != nullptr )
    {
        wil::com_ptr<ICoreWebView2Controller4> controller4 = m_impl->controller.query<ICoreWebView2Controller4>();
        ASSERT(controller4 != nullptr);

        if( controller4 != nullptr )
            controller4->put_AllowExternalDrop(enabled);
    }
}


void HtmlViewCtrl::SetContextMenuEnabled(const bool enabled)
{
    m_contextMenuEnabled = enabled;

    if( m_impl->view != nullptr )
    {
        ICoreWebView2Settings* settings;
        m_impl->view->get_Settings(&settings);
        settings->put_AreDefaultContextMenusEnabled(enabled);
    }
}


void HtmlViewCtrl::SetZoomControlEnabled(const bool enabled)
{
    m_zoomControlEnabled = enabled;

    if( m_impl->view != nullptr )
    {
        ICoreWebView2Settings* settings;
        m_impl->view->get_Settings(&settings);
        settings->put_IsZoomControlEnabled(m_zoomControlEnabled);
    }
}


void HtmlViewCtrl::SetOpenNonLocalhostLinksInBrowser(bool open_in_browser)
{
    m_openNonLocalhostLinksInBrowser = open_in_browser;
}


void HtmlViewCtrl::PreSubclassWindow()
{
    __super::PreSubclassWindow();

    if( m_initializeWebviewInPreSubclassWindow )
        InitializeWebView(); // If used in dialog OnCreate is never called so we initialize here
}


int HtmlViewCtrl::OnCreate(LPCREATESTRUCT lpCreateStruct)
{
    if( __super::OnCreate(lpCreateStruct) == -1 )
        return -1;

    InitializeWebView();

    return 0;
}


void HtmlViewCtrl::OnSize(const UINT nType, const int cx, const int cy)
{
    __super::OnSize(nType, cx, cy);

    if( m_impl->controller != nullptr )
    {
        RECT bounds;
        GetClientRect(&bounds);
        m_impl->controller->put_Bounds(bounds);
    }
}


void HtmlViewCtrl::OnDestroy()
{
    m_impl.reset();
}


ICoreWebView2* HtmlViewCtrl::GetWebView()
{
    return m_impl->view.get();
}


ICoreWebView2Controller* HtmlViewCtrl::GetController()
{
    return m_impl->controller.get();
}


void HtmlViewCtrl::InitializeWebView()
{
    if( m_initialized )
        return;

    m_initialized = true;

    HRESULT hr = CreateCoreWebView2EnvironmentWithOptions(nullptr, GetUserDataDirectory().c_str(), nullptr,
        Microsoft::WRL::Callback<ICoreWebView2CreateCoreWebView2EnvironmentCompletedHandler>(
        [this](HRESULT, ICoreWebView2Environment* env) -> HRESULT
        {
            env->CreateCoreWebView2Controller(m_hWnd, Microsoft::WRL::Callback<ICoreWebView2CreateCoreWebView2ControllerCompletedHandler>(
                [this](HRESULT result, ICoreWebView2Controller* controller) -> HRESULT
                {
                    RETURN_IF_FAILED(result);
                    OnWebViewCreated(controller);
                    return S_OK;
                }).Get());

            return S_OK;
        }).Get());

    if (!SUCCEEDED(hr))
    {
        if (hr == HRESULT_FROM_WIN32(ERROR_FILE_NOT_FOUND))
        {
            AfxMessageBox(_T("Couldn't find Edge installation. Do you have a version installed ")
                          _T("that's compatible with this WebView2 SDK version?"));
        }
        else
        {
            AfxMessageBox(FormatText(_T("Failed to create webview environment: 0x%08x"), hr), MB_OK);
        }
    }
}


const std::wstring& HtmlViewCtrl::GetUserDataDirectory()
{
    static const std::wstring calculated_user_data_directory =
        []()
        {
            #define InstanceLockPrefix    "CSIL"
            #define InstanceLockExtension ".wb2"
            constexpr std::string_view InstanceLockFilter_sv = InstanceLockPrefix "*" InstanceLockExtension;
            constexpr size_t MaxInstances = 25;

            // because multiple instances of applications may be using WebView2 controls, make sure that
            // each is using a unique directory; if not, an operation such as using execpff with wait would
            // cause both CSEntry instances to hang because of threading issues with WaitForSingleObject
            const std::string user_data_root_directory = Path::Combine(GetAppDataPath(), "webview");
            PortableFunctions::PathMakeDirectories(user_data_root_directory);

            // try to delete each temporary instance lock file; the ones that can't be deleted are locked
            std::vector<bool> instance_lock_flags(MaxInstances, false);

            for( const std::string& instance_file_path : DirectoryLister().SetNameFilter(InstanceLockFilter_sv)
                                                                          .GetPaths(user_data_root_directory) )
            {
                constexpr size_t InstanceLockPrefix_length = std::string_view(InstanceLockPrefix).length();
                const std::string filename = PortableFunctions::PathGetFilename(instance_file_path);
                const char* const instance_number_pos = filename.data() + InstanceLockPrefix_length;
                const size_t instance_number = static_cast<size_t>(atoi(instance_number_pos));

                if( !PortableFunctions::FileDelete(instance_file_path) && instance_number < instance_lock_flags.size() )
                    instance_lock_flags[instance_number] = true;
            }

            // use the first instance number not locked (or the max value if all are locked)
            const auto& instance_lookup = std::find(instance_lock_flags.cbegin(), instance_lock_flags.cend(), false);
            const int instance_number = static_cast<int>(std::distance(instance_lock_flags.cbegin(), instance_lookup));

            // create the unique directory name (\CSIL0, \CSIL1, etc.)
            std::string user_data_directory = Path::Combine(user_data_root_directory,
                                                            SO::Concatenate(InstanceLockPrefix, IntToString(instance_number)));

            // create a temporary file that will only be deleted when this instance ends
            class TemporaryInstanceFile
            {
            public:
                TemporaryInstanceFile(std::string temporary_filename)
                    :   m_temporaryFilename(std::move(temporary_filename))
                {
                    m_file = PortableFunctions::FileOpen(m_temporaryFilename, "wb");
                }

                ~TemporaryInstanceFile()
                {
                    if( m_file != nullptr )
                    {
                        fclose(m_file);
                        PortableFunctions::FileDelete(m_temporaryFilename);
                    }
                }

            private:
                std::string m_temporaryFilename;
                FILE* m_file;
            };

            static TemporaryInstanceFile temporary_instance_file(user_data_directory + InstanceLockExtension);

            return TC::ToWide(user_data_directory);
        }();

    return calculated_user_data_directory;
}


void HtmlViewCtrl::OnWebViewCreated(ICoreWebView2Controller* const controller)
{
    if( controller != nullptr )
    {
        m_impl->controller = controller;
        m_impl->controller->get_CoreWebView2(&m_impl->view);
    }

    // set the background color to white (without transparency);
    // prior to CSPro 8.1 this didn't ever seem necessary, but without this code,
    // the margin of DataManager's HtmlView would be transparent
    try
    {
        wil::com_ptr<ICoreWebView2Controller2> controller2 = m_impl->controller.query<ICoreWebView2Controller2>();
        controller2->put_DefaultBackgroundColor(COREWEBVIEW2_COLOR { 255, 255, 255, 255});
    }
    catch(...) { ASSERT(false); }

    // if a message box is displayed while the control is being created, the control is created
    // without being set to visible, so we manually set it to be visible
    m_impl->controller->put_IsVisible(TRUE);


    ConfigureSettings();
    FitWebViewToWindow();

    for( const auto& observer : m_webViewCreatedObservers )
        observer();

    SetupAcceleratorHandler();

    HRESULT hr = m_impl->view->add_NavigationStarting(
        Microsoft::WRL::Callback<ICoreWebView2NavigationStartingEventHandler>(
        [this](ICoreWebView2* /*sender*/, ICoreWebView2NavigationStartingEventArgs* args) -> HRESULT
        {
            OnNavigationStarted(args);
            return S_OK;
        }).Get(), nullptr);
    ASSERT(SUCCEEDED(hr));

    hr = m_impl->view->add_SourceChanged(
        Microsoft::WRL::Callback<ICoreWebView2SourceChangedEventHandler>(
        [this](ICoreWebView2* /*sender*/, ICoreWebView2SourceChangedEventArgs* /*args*/) -> HRESULT
        {
            OnSourceChanged();
            return S_OK;
        }).Get(), nullptr);
    ASSERT(SUCCEEDED(hr));

    hr = m_impl->view->add_NavigationCompleted(
        Microsoft::WRL::Callback<ICoreWebView2NavigationCompletedEventHandler>(
        [this](ICoreWebView2* /*sender*/, ICoreWebView2NavigationCompletedEventArgs* args) -> HRESULT
        {
            OnNavigationCompleted(args);
            return S_OK;
        }).Get(), nullptr);
    ASSERT(SUCCEEDED(hr));

    hr = m_impl->view->add_WebMessageReceived(
        Microsoft::WRL::Callback<ICoreWebView2WebMessageReceivedEventHandler>(
        [this](ICoreWebView2* /*sender*/, ICoreWebView2WebMessageReceivedEventArgs* args) -> HRESULT
        {
            OnWebMessageReceived(args);
            return S_OK;
        }).Get(), nullptr);
    ASSERT(SUCCEEDED(hr));

    if( m_csproHostObject != nullptr )
        AddCSProHostObject();

    ProcessPendingEvents();
}


void HtmlViewCtrl::AddCSProHostObject()
{
    ASSERT(m_csproHostObject != nullptr);

    try
    {
        VARIANT host_object_as_variant = { };
        host_object_as_variant.vt = VT_DISPATCH;
        host_object_as_variant.pdispVal = m_csproHostObject->GetIDispatch(FALSE);

        // add the host object...
        HRESULT hr = m_impl->view->AddHostObjectToScript(L"cspro", &host_object_as_variant);

        // ...and its CSPro JavaScript class "before the HTML document has been parsed
        // and before any other script included by the HTML document is run"
        if( SUCCEEDED(hr) )
        {
            hr = m_impl->view->AddScriptToExecuteOnDocumentCreated(TC::ToWide(m_csproHostObject->GetJavaScriptClassText()).c_str(),
                                                                   nullptr);

            if( SUCCEEDED(hr) )
                return;
        }
    }
    catch(...) { ASSERT(false); }

    ErrorMessage::Display("There was an error adding the CSPro host object and some functionality will not work as expected.");
}


void HtmlViewCtrl::ProcessPendingEvents()
{
    for( PendingEvent& pending_event : m_pendingEvents )
    {
        if( std::holds_alternative<std::shared_ptr<UriResolver>>(pending_event) )
        {
            NavigateTo(std::move(std::get<std::shared_ptr<UriResolver>>(pending_event)));
        }

        else if( std::holds_alternative<PendingEvent_NavigateToUri>(pending_event) )
        {
            NavigateTo(std::get<PendingEvent_NavigateToUri>(pending_event).uri);
        }

        else if( std::holds_alternative<PendingEvent_SetHtml>(pending_event) )
        {
            SetHtml(std::move(std::get<PendingEvent_SetHtml>(pending_event).html));
        }

        else if( std::holds_alternative<PendingEvent_ExecuteScript >(pending_event) )
        {
            PendingEvent_ExecuteScript& pending_event_execute_script = std::get<PendingEvent_ExecuteScript>(pending_event);

            if( pending_event_execute_script.result_handler )
            {
                ExecuteScript(pending_event_execute_script.javascript, std::move(pending_event_execute_script.result_handler));
            }

            else
            {
                ExecuteScript(pending_event_execute_script.javascript);
            }
        }

        else
        {
            ASSERT(std::holds_alternative<PendingEvent_PostWebMessage>(pending_event));
            const PendingEvent_PostWebMessage& pending_event_post_web_message = std::get<PendingEvent_PostWebMessage>(pending_event);

            if( pending_event_post_web_message.as_json )
            {
                PostWebMessageAsJson(pending_event_post_web_message.message);
            }

            else
            {
                PostWebMessageAsString(pending_event_post_web_message.message);
            }
        }
    }

    m_pendingEvents.clear();
}


void HtmlViewCtrl::OnNavigationStarted(ICoreWebView2NavigationStartingEventArgs* const args)
{
    if( m_navigationStartedObservers.empty() && !m_openNonLocalhostLinksInBrowser )
        return;

    const std::string uri = GetSource(args, false);

    if( !m_navigationStartedObservers.empty() )
    {
        bool cancel_navigation = false;

        for( const auto& observer : m_navigationStartedObservers )
        {
            observer(cancel_navigation, uri);

            if( cancel_navigation )
                break;
        }

        if( cancel_navigation )
        {
            args->put_Cancel(TRUE);
            return;
        }
    }

    if( m_openNonLocalhostLinksInBrowser )
    {
        constexpr std::string_view LocalhostPrefix_sv = "http://localhost";

        if( !SO::StartsWithNoCase(uri, LocalhostPrefix_sv) )
        {
            args->put_Cancel(true);
            Viewer().ViewHtmlUrl(uri);
        }
    }
}


void HtmlViewCtrl::OnSourceChanged()
{
    if( m_sourceChangedObservers.empty() )
        return;

    const std::string uri = GetSource();

    for( const auto& observer : m_sourceChangedObservers )
        observer(uri);
}


void HtmlViewCtrl::OnNavigationCompleted(ICoreWebView2NavigationCompletedEventArgs* const args)
{
    if( m_navigationCompletedObservers.empty() )
        return;

    BOOL success;
    args->get_IsSuccess(&success);

    for( const auto& observer : m_navigationCompletedObservers )
        observer(success);
}


void HtmlViewCtrl::OnWebMessageReceived(ICoreWebView2WebMessageReceivedEventArgs* const args)
{
    if( m_webEventObservers.empty() )
        return;

    PWSTR message;
    args->get_WebMessageAsJson(&message);

    const std::wstring_view message_sv(message);

    for( const auto& observer : m_webEventObservers )
        observer(message_sv);

    CoTaskMemFree(message);
}


void HtmlViewCtrl::ConfigureSettings()
{
    ICoreWebView2Settings* settings;
    m_impl->view->get_Settings(&settings);

    settings->put_IsScriptEnabled(TRUE);
    settings->put_AreDefaultScriptDialogsEnabled(TRUE);
    settings->put_IsWebMessageEnabled(TRUE);
    settings->put_IsStatusBarEnabled(FALSE);
    settings->put_IsZoomControlEnabled(m_zoomControlEnabled);
    settings->put_AreDefaultContextMenusEnabled(m_contextMenuEnabled);

    SetAllowExternalDrop(m_allowExternalDrop);
}


void HtmlViewCtrl::FitWebViewToWindow()
{
    RECT bounds;
    GetClientRect(&bounds);
    m_impl->controller->put_Bounds(bounds);
}


void HtmlViewCtrl::PostWebMessageAsJson(const std::wstring& message_json)
{
    if( m_impl->view != nullptr )
    {
        HRESULT hr = m_impl->view->PostWebMessageAsJson(message_json.c_str());
        ASSERT(SUCCEEDED(hr));
    }

    else
    {
        m_pendingEvents.emplace_back(PendingEvent_PostWebMessage { message_json, true });
    }
}


void HtmlViewCtrl::PostWebMessageAsJson(const std::string_view message_json_sv)
{
    AssertValidJson(message_json_sv);
    PostWebMessageAsJson(TC::ToWide(message_json_sv));
}


void HtmlViewCtrl::PostWebMessageAsString(const std::wstring& message_string)
{
    if( m_impl->view != nullptr )
    {
        HRESULT hr = m_impl->view->PostWebMessageAsString(message_string.c_str());
        ASSERT(SUCCEEDED(hr));
    }

    else
    {
        m_pendingEvents.emplace_back(PendingEvent_PostWebMessage { message_string, false });
    }
}


void HtmlViewCtrl::PostWebMessageAsString(const std::string_view message_string_sv)
{
    PostWebMessageAsString(TC::ToWide(message_string_sv));
}


void HtmlViewCtrl::ExecuteScript(NullTerminatedString javascript)
{
    if( m_impl->view != nullptr )
    {
        m_impl->view->ExecuteScript(javascript.c_str(), nullptr);
    }

    else
    {
        m_pendingEvents.emplace_back(PendingEvent_ExecuteScript { javascript, { } });
    }
}


void HtmlViewCtrl::ExecuteScript(NullTerminatedString javascript, std::function<void(const std::wstring&)> result_handler)
{
    ASSERT(result_handler);

    if( m_impl->view != nullptr )
    {
        m_impl->view->ExecuteScript(javascript.c_str(),
            Microsoft::WRL::Callback<ICoreWebView2ExecuteScriptCompletedHandler>(
            [rh = std::move(result_handler)](HRESULT, PCWSTR result) -> HRESULT
            {
                rh(result);
                return S_OK;
            }).Get());
    }

    else
    {
        m_pendingEvents.emplace_back(PendingEvent_ExecuteScript { javascript, std::move(result_handler) });
    }
}


void HtmlViewCtrl::ExecuteScript(const std::string_view javascript_sv)
{
    ExecuteScript(TC::ToWide(javascript_sv));
}


void HtmlViewCtrl::SetAcceleratorKeyHandler(std::function<bool(UINT message, UINT key, INT lParam)> handler)
{
    m_acceleratorKeyHandler = std::move(handler);
}


void HtmlViewCtrl::UseWebView2AcceleratorKeyHandler()
{
    m_acceleratorKeyHandler = std::function<bool(UINT, UINT, INT)>();
}


void HtmlViewCtrl::NavigateTo(std::shared_ptr<UriResolver> uri_resolver)
{
    ASSERT(uri_resolver != nullptr);

    if( m_impl->view != nullptr )
    {
        uri_resolver->Navigate(*this, [&](const std::string& uri) { return m_impl->view->Navigate(TC::ToWide(uri).c_str()); });
    }

    else
    {
        m_pendingEvents.emplace_back(std::move(uri_resolver));
    }
}


void HtmlViewCtrl::NavigateTo(const std::string_view uri_sv)
{
    if( m_impl->view != nullptr )
    {
        HRESULT hr = m_impl->view->Navigate(TC::ToWide(uri_sv).c_str());
        ASSERT(SUCCEEDED(hr));
    }

    else
    {
        m_pendingEvents.emplace_back(PendingEvent_NavigateToUri { std::string(uri_sv) });
    }
}


void HtmlViewCtrl::SetHtml(std::wstring html)
{
    if( m_impl->view != nullptr )
    {
        HRESULT hr = m_impl->view->NavigateToString(html.c_str());
        ASSERT(SUCCEEDED(hr));
    }

    else
    {
        m_pendingEvents.emplace_back(PendingEvent_SetHtml { std::move(html) });
    }
}


void HtmlViewCtrl::SetHtml(const std::string_view html_sv)
{
    SetHtml(TC::ToWide(html_sv));
}


void HtmlViewCtrl::Reload()
{
    if( m_impl->view != nullptr )
        m_impl->view->Reload();
}


void HtmlViewCtrl::SetupAcceleratorHandler()
{
    HRESULT hr = GetController()->add_AcceleratorKeyPressed(
        Microsoft::WRL::Callback<ICoreWebView2AcceleratorKeyPressedEventHandler>(
        [this](ICoreWebView2Controller*, ICoreWebView2AcceleratorKeyPressedEventArgs* const args) -> HRESULT
        {
            if( m_acceleratorKeyHandler )
            {
                UINT key;
                args->get_VirtualKey(&key);

                INT lParam;
                args->get_KeyEventLParam(&lParam);

                COREWEBVIEW2_KEY_EVENT_KIND kind;
                args->get_KeyEventKind(&kind);

                const UINT msg = ( kind == COREWEBVIEW2_KEY_EVENT_KIND_KEY_DOWN )        ? WM_KEYDOWN :
                                 ( kind == COREWEBVIEW2_KEY_EVENT_KIND_KEY_UP )          ? WM_KEYUP :
                                 ( kind == COREWEBVIEW2_KEY_EVENT_KIND_SYSTEM_KEY_DOWN ) ? WM_SYSKEYDOWN :
                                 ( kind == COREWEBVIEW2_KEY_EVENT_KIND_SYSTEM_KEY_UP )   ? WM_SYSKEYUP :
                                                                                           ReturnProgrammingError(WM_KEYDOWN);

                args->put_Handled(m_acceleratorKeyHandler(msg, key, lParam));
            }

            else
            {
                args->put_Handled(FALSE);
            }

            return S_OK;
        }).Get(), nullptr);
    ASSERT(SUCCEEDED(hr));
}


bool HtmlViewCtrl::DefaultAcceleratorKeyHandler(UINT message, UINT key, INT lParam)
{
    bool ctrl = GetKeyState(VK_CONTROL) < 0;
    bool shift = GetKeyState(VK_SHIFT) < 0;
    bool alt = GetKeyState(VK_MENU) < 0;

    bool should_go_to_webview = false;

    switch (key) {
        // Shortcuts used by the editor
    case 'A': // select all
    case 'X': // cut
    case 'C': // copy
    case 'V': // paste
    case 'F': // find
        should_go_to_webview = ctrl && !shift && !alt; // ctrl only for these shortcuts
        break;
    default:
        // Other shortcuts with ctrl or alt are not passed to webview and are instead posted
        // to this process for handling.
        should_go_to_webview = !ctrl && !alt;
    }

    if (should_go_to_webview) {
        return false;
    }
    else {
        PostMessage(message, key, lParam);
        return true;
    }
}


void HtmlViewCtrl::MoveFocus()
{
    if (m_impl->controller) {
        m_impl->controller->MoveFocus(COREWEBVIEW2_MOVE_FOCUS_REASON_NEXT);
    }
}


template<typename T>
std::string HtmlViewCtrl::GetSource(T* const view_or_args, const bool clear_about_blank)
{
    if( view_or_args != nullptr )
    {
        wil::unique_cotaskmem_string uri;

        if constexpr(std::is_same_v<T, ICoreWebView2NavigationStartingEventArgs>)
        {
            view_or_args->get_Uri(&uri);
        }

        else
        {
            view_or_args->get_Source(&uri);
        }

        if( !clear_about_blank || wcscmp(uri.get(), L"about:blank") != 0 )
            return TC::ToUtf8(uri.get());
    }

    return std::string();
}


std::string HtmlViewCtrl::GetSource()
{
    return GetSource(m_impl->view.get(), true);
}


ActionInvoker::WebController& HtmlViewCtrl::RegisterCSProHostObject()
{
    ASSERT(m_csproHostObject == nullptr);

    m_csproHostObject = std::make_unique<CSProHostObject>(this);

    if( m_impl->view != nullptr )
        AddCSProHostObject();

    return m_csproHostObject->GetActionInvokerWebController();
}


ActionInvoker::WebController* HtmlViewCtrl::GetActionInvokerWebController()
{
    return ( m_csproHostObject != nullptr ) ? &m_csproHostObject->GetActionInvokerWebController() :
                                              nullptr;
}


LRESULT HtmlViewCtrl::OnActionInvokerProcessAsyncMessage(WPARAM wParam, LPARAM /*lParam*/)
{
    ASSERT(m_csproHostObject != nullptr);

    // to prevent multiple asynchronous messages from being processed at the same time, we will
    // add the message IDs to a vector and then process them only from the initial execution point;
    // this solves an issue with:
    //      1) async message 1
    //      2) processing message 1, ActionInvoker::Runtime::CheckAccessToken displays an AfxMessageBox, which enters this window's message loop
    //      2) async message 2
    //      4) processing message 2 prior to the AfxMessageBox processing, which would lead to a deadlock in ActionInvoker::WebController::ProcessMessage

    m_csproHostObjectAsyncMessageIds.push(wParam);

    // only process messages if this is the initial method processing these calls
    if( m_csproHostObjectAsyncMessageIds.size() == 1 )
    {
        while( !m_csproHostObjectAsyncMessageIds.empty() )
        {
            const SharableString response = m_csproHostObject->GetActionInvokerWebController().ProcessMessage(m_csproHostObjectAsyncMessageIds.front(), true);

            if( response.IsSet() )
                ExecuteScript(*response);

            m_csproHostObjectAsyncMessageIds.pop();
        }
    }

    return 0;
}


void HtmlViewCtrl::ShowPrintUI()
{
    if( m_impl->view == nullptr )
        return;

    wil::com_ptr<ICoreWebView2_16> web_view2_16;

    if( m_impl->view->QueryInterface(IID_PPV_ARGS(&web_view2_16)) == S_OK )
        web_view2_16->ShowPrintUI(COREWEBVIEW2_PRINT_DIALOG_KIND::COREWEBVIEW2_PRINT_DIALOG_KIND_BROWSER);
}


void HtmlViewCtrl::SaveScreenshot(const std::string& file_path)
{
    std::optional<std::string> mime_type = MimeType::GetTypeFromFileExtension(PortableFunctions::PathGetFileExtension(file_path));
    std::optional<COREWEBVIEW2_CAPTURE_PREVIEW_IMAGE_FORMAT> image_format;

    if( mime_type.has_value() )
    {
        image_format = ( *mime_type == MimeType::Type::ImageJpeg ) ? COREWEBVIEW2_CAPTURE_PREVIEW_IMAGE_FORMAT_JPEG :
                       ( *mime_type == MimeType::Type::ImagePng )  ? COREWEBVIEW2_CAPTURE_PREVIEW_IMAGE_FORMAT_PNG :
                                                                     std::optional<COREWEBVIEW2_CAPTURE_PREVIEW_IMAGE_FORMAT>();
    }


    if( !image_format.has_value() )
        throw CSProException("Snapshots can only be saved to JPEG or PNG formats.");

    try
    {
        // create the file stream
        wil::com_ptr<IStream> stream;

        if( !SUCCEEDED(SHCreateStreamOnFileEx(TC::ToWide(file_path).c_str(), STGM_READWRITE | STGM_CREATE, FILE_ATTRIBUTE_NORMAL, TRUE, nullptr, &stream)) )
            throw std::exception();

        // capture the screenshot
        if( !SUCCEEDED(m_impl->view->CapturePreview(*image_format, stream.get(), nullptr)) )
            throw std::exception();
    }

    catch(...)
    {
        throw CSProException("There was an error saving the screenshot: " + PortableFunctions::PathGetFilename(file_path));
    }
}
