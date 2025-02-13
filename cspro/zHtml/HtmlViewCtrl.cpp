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
        m_contextMenuEnabled(true),
        m_openNonLocalhostLinksInBrowser(false),
        m_initialized(false),
        m_accelerator_key_handler([this](UINT message, UINT key, INT lParam) { return DefaultAcceleratorKeyHandler(message, key, lParam); })
{
}


HtmlViewCtrl::~HtmlViewCtrl()
{
}


void HtmlViewCtrl::SetContextMenuEnabled(bool enabled)
{
    m_contextMenuEnabled = enabled;
    if (m_impl->view) {
        ICoreWebView2Settings* Settings;
        m_impl->view->get_Settings(&Settings);
        Settings->put_AreDefaultContextMenusEnabled(enabled);
    }
}


void HtmlViewCtrl::SetOpenNonLocalhostLinksInBrowser(bool open_in_browser)
{
    m_openNonLocalhostLinksInBrowser = open_in_browser;
}


void HtmlViewCtrl::PreSubclassWindow()
{
    CWnd::PreSubclassWindow();

    if( m_initializeWebviewInPreSubclassWindow )
        InitializeWebView(); // If used in dialog OnCreate is never called so we initialize here
}


int HtmlViewCtrl::OnCreate(LPCREATESTRUCT lpCreateStruct)
{
    if (CWnd::OnCreate(lpCreateStruct) == -1)
        return -1;

    InitializeWebView();

    return 0;
}


void HtmlViewCtrl::OnSize(UINT nType, int cx, int cy)
{
    CWnd::OnSize(nType, cx, cy);
    if (m_impl->controller) {
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
    if (m_initialized)
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


std::wstring HtmlViewCtrl::GetUserDataDirectory()
{
    static std::wstring user_data_directory;

    if( user_data_directory.empty() )
    {
        #define InstanceLockPrefix    _T("CSIL")
        #define InstanceLockExtension _T(".wb2")
        constexpr wstring_view InstanceLockFilter = InstanceLockPrefix _T("*") InstanceLockExtension;
        constexpr size_t MaxInstances = 25;

        // because multiple instances of applications may be using WebView2 controls, make sure that
        // each is using a unique directory; if not, an operation such as using execpff with wait would
        // cause both CSEntry instances to hang because of threading issues with WaitForSingleObject
        std::wstring user_data_root_directory = PortableFunctions::PathAppendToPath(GetAppDataPath(), _T("webview"));
        PortableFunctions::PathMakeDirectories(user_data_root_directory);

        // try to delete each temporary instance lock file; the ones that can't be deleted are locked
        std::vector<bool> instance_lock_flags(MaxInstances, false);

        for( const std::wstring& instance_filename : DirectoryLister().SetNameFilter(InstanceLockFilter)
                                                                      .GetPaths(user_data_root_directory) )
        {
            constexpr size_t InstanceLockPrefix_length = std::wstring_view(InstanceLockPrefix).length();
            const TCHAR* instance_number_pos = PortableFunctions::PathGetFilename(instance_filename) + InstanceLockPrefix_length;
            size_t instance_number = static_cast<size_t>(_ttoi(instance_number_pos));

            if( !PortableFunctions::FileDelete(instance_filename) && instance_number < instance_lock_flags.size() )
                instance_lock_flags[instance_number] = true;
        }

        // use the first instance number not locked (or the max value if all are locked)
        const auto& instance_lookup = std::find(instance_lock_flags.cbegin(), instance_lock_flags.cend(), false);
        int instance_number = (int)std::distance(instance_lock_flags.cbegin(), instance_lookup);

        // create the unique directory name (\CSIL0, \CSIL1, etc.)
        user_data_directory = PortableFunctions::PathAppendToPath(user_data_root_directory,
                                                                  SO::Concatenate(InstanceLockPrefix, IntToString(instance_number)));

        // create a temporary file that will only be deleted when this instance ends
        class TemporaryInstanceFile
        {
        public:
            TemporaryInstanceFile(std::wstring temporary_filename)
                :   m_temporaryFilename(std::move(temporary_filename))
            {
                m_file = PortableFunctions::FileOpen(m_temporaryFilename, _T("wb"));
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
            std::wstring m_temporaryFilename;
            FILE* m_file;
        };

        static TemporaryInstanceFile temporary_instance_file(user_data_directory + InstanceLockExtension);
    }

    return user_data_directory;
}


void HtmlViewCtrl::OnWebViewCreated(ICoreWebView2Controller* controller)
{
    if (controller != nullptr) {
        m_impl->controller = controller;
        m_impl->controller->get_CoreWebView2(&m_impl->view);
    }

    ConfigureSettings();
    FitWebViewToWindow();

    for (auto& observer : m_webview_created_observers)
        observer();

    SetupWebMessageReceiver();
    SetupAcceleratorHandler();


    HRESULT hr = m_impl->view->add_NavigationStarting(
        Microsoft::WRL::Callback<ICoreWebView2NavigationStartingEventHandler>(
        [this](ICoreWebView2*, ICoreWebView2NavigationStartingEventArgs* args) -> HRESULT
        {
            if( m_openNonLocalhostLinksInBrowser )
            {
                constexpr wstring_view LocalhostPrefix = _T("http://localhost");

                wil::unique_cotaskmem_string uri;
                args->get_Uri(&uri);

                if( !SO::StartsWithNoCase(uri.get(), LocalhostPrefix) )
                {
                    args->put_Cancel(true);
                    Viewer().ViewHtmlUrl(uri.get());
                }
            }
            return S_OK;
        }).Get(), nullptr);
    ASSERT(SUCCEEDED(hr));


    // add the host object...
    if( m_csproHostObject != nullptr )
    {
        try
        {
            VARIANT host_object_as_variant = { };
            host_object_as_variant.vt = VT_DISPATCH;
            host_object_as_variant.pdispVal = m_csproHostObject->GetIDispatch(FALSE);

            hr = m_impl->view->AddHostObjectToScript(L"cspro", &host_object_as_variant);

            // ...and its CSPro JavaScript class "before the HTML document has been parsed
            // and before any other script included by the HTML document is run"
            if( SUCCEEDED(hr) )
            {
                hr = m_impl->view->AddScriptToExecuteOnDocumentCreated(
                    m_csproHostObject->GetJavaScriptClassText().c_str(), nullptr);
            }
        }

        catch(...)
        {
            hr = E_FAIL;
        }

        if( FAILED(hr) )
        {
            ErrorMessage::Display(_T("There was an error adding the CSPro host object and ")
                                  _T("some functionality will not work as expected."));
        }
    }


    hr = m_impl->view->add_NavigationCompleted(
        Microsoft::WRL::Callback<ICoreWebView2NavigationCompletedEventHandler>(
        [this](ICoreWebView2*, ICoreWebView2NavigationCompletedEventArgs* args) -> HRESULT
        {
            OnNavigationCompleted(args);
            return S_OK;
        }).Get(), nullptr);
    ASSERT(SUCCEEDED(hr));

    hr = m_impl->view->add_SourceChanged(
        Microsoft::WRL::Callback<ICoreWebView2SourceChangedEventHandler>(
        [this](ICoreWebView2* sender, ICoreWebView2SourceChangedEventArgs* /*args*/) -> HRESULT
        {
            wil::unique_cotaskmem_string uri;
            sender->get_Source(&uri);
            OnSourceChanged(( _tcscmp(uri.get(), _T("about:blank")) != 0 ) ? uri.get() : _T(""));
            return S_OK;
        }).Get(), nullptr);
    ASSERT(SUCCEEDED(hr));


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

        else
        {
            ASSERT(std::holds_alternative<PendingEvent_ExecuteScript>(pending_event));
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
    }

    m_pendingEvents.clear();
}


void HtmlViewCtrl::OnSourceChanged(const std::wstring& uri)
{
    for( auto& listener : m_source_changed_observers )
        listener(uri);
}


void HtmlViewCtrl::OnNavigationCompleted(ICoreWebView2NavigationCompletedEventArgs* args)
{
    BOOL success;
    args->get_IsSuccess(&success);
    for (auto& listener : m_navigation_completed_observers)
        listener(success);
}


void HtmlViewCtrl::ConfigureSettings()
{
    ICoreWebView2Settings* Settings;
    m_impl->view->get_Settings(&Settings);
    Settings->put_IsScriptEnabled(TRUE);
    Settings->put_AreDefaultScriptDialogsEnabled(TRUE);
    Settings->put_IsWebMessageEnabled(TRUE);
    Settings->put_IsStatusBarEnabled(FALSE);
    Settings->put_IsZoomControlEnabled(FALSE);
    Settings->put_AreDefaultContextMenusEnabled(m_contextMenuEnabled);
}


void HtmlViewCtrl::FitWebViewToWindow()
{
    RECT bounds;
    GetClientRect(&bounds);
    m_impl->controller->put_Bounds(bounds);
}


void HtmlViewCtrl::SetupWebMessageReceiver()
{
    HRESULT hr = m_impl->view->add_WebMessageReceived(
        Microsoft::WRL::Callback<ICoreWebView2WebMessageReceivedEventHandler>(
        [this](ICoreWebView2*, ICoreWebView2WebMessageReceivedEventArgs* args) -> HRESULT
        {
            PWSTR message;
            args->get_WebMessageAsJson(&message);
            for (auto& listener : m_web_event_observers)
                listener(message);
            CoTaskMemFree(message);
            return S_OK;
        }).Get(), nullptr);
    ASSERT(SUCCEEDED(hr));
}


void HtmlViewCtrl::PostWebMessageAsJson(const std::wstring& message_json)
{
    if (m_impl->view) {
        HRESULT hr = m_impl->view->PostWebMessageAsJson(message_json.c_str());
        ASSERT(SUCCEEDED(hr));
    }
    else {
        ASSERT(!"WAIT FOR THE VIEW TO BE INITIALIZED");
    }
}


void HtmlViewCtrl::PostWebMessageAsString(const std::wstring& message_string)
{
    if (m_impl->view) {
        HRESULT hr = m_impl->view->PostWebMessageAsString(message_string.c_str());
        ASSERT(SUCCEEDED(hr));
    }
    else {
        ASSERT(!"WAIT FOR THE VIEW TO BE INITIALIZED");
    }
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


void HtmlViewCtrl::AddWebViewCreatedObserver(std::function<void()> observer)
{
    m_webview_created_observers.emplace_back(std::move(observer));
}


void HtmlViewCtrl::AddSourceChangedObserver(std::function<void(const std::wstring& event_json)> observer)
{
    m_source_changed_observers.emplace_back(std::move(observer));
}


void HtmlViewCtrl::AddNavigationCompletedObserver(std::function<void(bool)> observer)
{
    m_navigation_completed_observers.emplace_back(std::move(observer));
}


void HtmlViewCtrl::AddWebEventObserver(std::function<void(const std::wstring& event_json)> observer)
{
    m_web_event_observers.emplace_back(std::move(observer));
}


void HtmlViewCtrl::SetAcceleratorKeyHandler(std::function<bool(UINT message, UINT key, INT lParam)> handler)
{
    m_accelerator_key_handler = std::move(handler);
}


void HtmlViewCtrl::UseWebView2AcceleratorKeyHandler()
{
    m_accelerator_key_handler = std::function<bool(UINT, UINT, INT)>();
}


void HtmlViewCtrl::NavigateTo(std::shared_ptr<UriResolver> uri_resolver)
{
    ASSERT(uri_resolver != nullptr);

    if( m_impl->view != nullptr )
    {
        uri_resolver->Navigate(*this, [&](const wchar_t* uri) { return m_impl->view->Navigate(uri); });
    }

    else
    {
        m_pendingEvents.emplace_back(std::move(uri_resolver));
    }
}


void HtmlViewCtrl::NavigateTo(const std::wstring& uri)
{
    if( m_impl->view != nullptr )
    {
        HRESULT hr = m_impl->view->Navigate(uri.c_str());
        ASSERT(SUCCEEDED(hr));
    }

    else
    {
        m_pendingEvents.emplace_back(PendingEvent_NavigateToUri { uri });
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


void HtmlViewCtrl::Reload()
{
    if( m_impl->view != nullptr )
        m_impl->view->Reload();
}


void HtmlViewCtrl::SetupAcceleratorHandler()
{
    HRESULT hr = GetController()->add_AcceleratorKeyPressed(
        Microsoft::WRL::Callback<ICoreWebView2AcceleratorKeyPressedEventHandler>(
        [this](ICoreWebView2Controller*, ICoreWebView2AcceleratorKeyPressedEventArgs* args) -> HRESULT
        {
            BOOL handled = FALSE;
            if (m_accelerator_key_handler) {
                UINT key;
                args->get_VirtualKey(&key);
                INT lParam;
                args->get_KeyEventLParam(&lParam);
                COREWEBVIEW2_KEY_EVENT_KIND kind;
                args->get_KeyEventKind(&kind);
                UINT msg = WM_KEYDOWN;
                switch (kind) {
                    case COREWEBVIEW2_KEY_EVENT_KIND_KEY_DOWN:
                        msg = WM_KEYDOWN;
                        break;
                    case COREWEBVIEW2_KEY_EVENT_KIND_KEY_UP:
                        msg = WM_KEYUP;
                        break;
                    case COREWEBVIEW2_KEY_EVENT_KIND_SYSTEM_KEY_DOWN:
                        msg = WM_SYSKEYDOWN;
                        break;
                    case COREWEBVIEW2_KEY_EVENT_KIND_SYSTEM_KEY_UP:
                        msg = WM_SYSKEYUP;
                        break;
                }
                handled = m_accelerator_key_handler(msg, key, lParam) == true;
            }
            args->put_Handled(handled);
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


std::wstring HtmlViewCtrl::GetSource()
{
    if( m_impl->view != nullptr )
    {
        wil::unique_cotaskmem_string uri;
        m_impl->view->get_Source(&uri);
        return uri.get();
    }

    return std::wstring();
}


ActionInvoker::WebController& HtmlViewCtrl::RegisterCSProHostObject()
{
    ASSERT(m_csproHostObject == nullptr);

    m_csproHostObject = std::make_unique<CSProHostObject>(this);

    return m_csproHostObject->GetActionInvokerWebController();
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
            const std::shared_ptr<const std::wstring> response = m_csproHostObject->GetActionInvokerWebController().ProcessMessage(m_csproHostObjectAsyncMessageIds.front(), true);

            if( response != nullptr )
                ExecuteScript(*response);

            m_csproHostObjectAsyncMessageIds.pop();
        }
    }

    return 0;
}


void HtmlViewCtrl::SaveScreenshot(NullTerminatedString filename)
{
    std::optional<std::wstring> mime_type = MimeType::GetTypeFromFileExtension(PortableFunctions::PathGetFileExtension(filename));
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

        if( !SUCCEEDED(SHCreateStreamOnFileEx(filename.c_str(), STGM_READWRITE | STGM_CREATE, FILE_ATTRIBUTE_NORMAL, TRUE, nullptr, &stream)) )
            throw std::exception();

        // capture the screenshot
        if( !SUCCEEDED(m_impl->view->CapturePreview(*image_format, stream.get(), nullptr)) )
            throw std::exception();
    }

    catch(...)
    {
        throw CSProException(_T("There was an error saving the screenshot: %s"), PortableFunctions::PathGetFilename(filename));
    }
}


HtmlViewCtrl::Impl::~Impl()
{
    if (controller) {
        controller->Close();
        controller.reset();
        view.reset();
    }
}
