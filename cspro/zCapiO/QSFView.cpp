#include "StdAfx.h"
#include "QSFView.h"
#include "CapiStyle.h"
#include <zHtml/CSProHostObject.h>
#include <zHtml/SharedHtmlLocalFileServer.h>
#include <zHtml/UWM.h>


IMPLEMENT_DYNCREATE(QSFView, CFormView)

BEGIN_MESSAGE_MAP(QSFView, CFormView)
    ON_WM_SIZE()
    ON_WM_DESTROY()
    ON_MESSAGE(UWM::Capi::RefreshQuestionText, OnRefreshQuestionText)
END_MESSAGE_MAP()


namespace
{
    const std::wstring DefaultBackgroundColor = PortableColor::FromRGB(0xFE, 0xFD, 0xE2).ToString(); // yellow
}


QSFView::QSFView()
    :   CFormView(IDD_QSFVIEW),
        m_backgroundColor(DefaultBackgroundColor)
{
    m_htmlViewCtrl.SetContextMenuEnabled(false);
    m_htmlViewCtrl.SetOpenNonLocalhostLinksInBrowser(true);

    // Disable all accelerators
    m_htmlViewCtrl.SetAcceleratorKeyHandler([this](UINT, UINT key, INT) {
#ifdef _DEBUG
        // Allow dev tools in debug mode (ctrl+shift+I)
        return !(key == 'I' && GetKeyState(VK_CONTROL) < 0 && GetKeyState(VK_SHIFT) < 0);
#else
        return true;
#endif
    });

    // set up the Action Invoker
    SetUpActionInvoker();
}


QSFView::~QSFView()
{
}


void QSFView::DoDataExchange(CDataExchange* pDX)
{
    CFormView::DoDataExchange(pDX);
    DDX_Control(pDX, IDC_HTML_VIEW, m_htmlViewCtrl);
}


void QSFView::OnSize(UINT nType, int cx, int cy)
{
    CFormView::OnSize(nType, cx, cy);

    // resize the HTML control so that it fills up the client window
    if( m_htmlViewCtrl.m_hWnd != nullptr )
    {
        CRect rcClient;
        GetClientRect(&rcClient);
        m_htmlViewCtrl.MoveWindow(rcClient);
    }
}


void QSFView::OnDestroy()
{
    m_questionTextVirtualFileMapping.reset();
}


void QSFView::SetupFileServer(const CString& application_filename)
{
    m_fileServer = std::make_unique<SharedHtmlLocalFileServer>();

    m_questionTextVirtualFileMapping = std::make_unique<VirtualFileMapping>(
        m_fileServer->CreateVirtualHtmlFile(PortableFunctions::PathGetDirectory(application_filename),
        [&]()
        {
            std::lock_guard<std::mutex> lock(m_htmlMutex);
            return m_html;
        }));

    UpdateHtml();
}


void QSFView::SetText(const CString& text, std::optional<PortableColor> background_color/* = std::nullopt*/)
{
    m_backgroundColor = background_color.has_value() ? background_color->ToStringRGB() :
                                                       DefaultBackgroundColor;
    m_questionText = text;
    UpdateHtml();
}


void QSFView::SetStyleCss(std::wstring css)
{
    m_stylesheet = std::move(css);
    UpdateHtml();
}


void QSFView::UpdateHtml()
{
    constexpr wstring_view Part1 =
        _T("<!DOCTYPE html>\n")
        _T("<html>\n")
        _T("<head>\n")
        _T("<meta http-equiv=\"content-type\" content=\"text/html; charset=utf-8\">\n")
        _T("<title>CSPro</title>")
        _T("<style>body{background-color: #EFEFEF;} table{width: 100%;border-collapse: collapse;} td, th{border: 1px solid #ececec;padding: 5px 3px;}</style>\n")
        _T("<style>\n");
    
    constexpr wstring_view Part2 =
        _T("</style>\n")
        _T("</head>\n")
        _T("<body style=\"background-color: ");

    constexpr wstring_view Part3 =
        _T(";\">\n");

    constexpr wstring_view Part4 =
        _T("</body>\n")
        _T("</html>\n");

    std::wstring html = SO::Concatenate(Part1, m_stylesheet,
                                        Part2, m_backgroundColor,
                                        Part3, m_questionText,
                                        Part4);

    std::lock_guard<std::mutex> lock(m_htmlMutex);
    m_html = UTF8Convert::WideToUTF8(html);

    PostMessage(UWM::Capi::RefreshQuestionText);
}


LRESULT QSFView::OnRefreshQuestionText(WPARAM /*wParam*/, LPARAM /*lParam*/)
{
    if( m_questionTextVirtualFileMapping != nullptr )
        m_htmlViewCtrl.NavigateTo(m_questionTextVirtualFileMapping->GetUrl());

    return 0;
}



// --------------------------------------------------------------------------
// Action Invoker functionality
// --------------------------------------------------------------------------

class QuestionTextActionInvokerListener : public ActionInvoker::Listener
{
public:
    QuestionTextActionInvokerListener(ActionInvoker::Caller& caller);

    // Listener overrides
    std::optional<std::wstring> OnGetDisplayOptions(ActionInvoker::Caller& caller) override;
    std::optional<bool> OnSetDisplayOptions(const JsonNode<wchar_t>& json_node, ActionInvoker::Caller& caller) override;

    bool OnEngineProgramControlExecuted() override;

private:
    ActionInvoker::Caller& m_actionInvokerCaller;
    std::optional<unsigned> m_height;
};


class QuestionTextActionInvokerWebControllerPreProcessMessageWorker : public ActionInvoker::WebController::PreProcessMessageWorker
{
public:
    QuestionTextActionInvokerWebControllerPreProcessMessageWorker(std::shared_ptr<ActionInvoker::Listener> action_invoker_listener);

    // PreProcessMessageWorker overrides
    std::shared_ptr<ActionInvoker::Listener> GetListener() override;

private:
    std::shared_ptr<ActionInvoker::Listener> m_actionInvokerListener;
};


QuestionTextActionInvokerListener::QuestionTextActionInvokerListener(ActionInvoker::Caller& caller)
    :   m_actionInvokerCaller(caller)
{
}


std::optional<std::wstring> QuestionTextActionInvokerListener::OnGetDisplayOptions(ActionInvoker::Caller& caller)
{
    if( !caller.IsFromWebView(m_actionInvokerCaller) )
        return std::nullopt;

    if( !m_height.has_value() )
    {
        m_height.emplace();

        if( WindowsDesktopMessage::Send(UWM::Capi::GetWindowHeight, &*m_height) != 1 )
        {
            m_height.reset();
            return Json::Text::EmptyObject;
        }
    }

    auto json_writer = Json::CreateStringWriter();

    json_writer->BeginObject()
                .Write(JK::height, *m_height)
                .EndObject();

    return json_writer->GetString();
}


std::optional<bool> QuestionTextActionInvokerListener::OnSetDisplayOptions(const JsonNode<wchar_t>& json_node, ActionInvoker::Caller& caller)
{
    if( !caller.IsFromWebView(m_actionInvokerCaller) )
        return std::nullopt;

    if( !json_node.Contains(JK::height) )
        return false;

    unsigned height = std::min(json_node.Get<unsigned>(JK::height), FormDefaults::QuestionTextHeightMax);

    if( height != m_height )
    {
        WindowsDesktopMessage::Post(UWM::Capi::SetWindowHeight, height);

        // because the message is posted, cache the height so that an immediate call to CS.getDisplayOptions will return the correct value
        m_height = height;
    }

    return true;
}


bool QuestionTextActionInvokerListener::OnEngineProgramControlExecuted()
{
    WindowsDesktopMessage::Post(UWM::Html::ActionInvokerEngineProgramControlExecuted);
    return true;
}


QuestionTextActionInvokerWebControllerPreProcessMessageWorker::QuestionTextActionInvokerWebControllerPreProcessMessageWorker(std::shared_ptr<ActionInvoker::Listener> action_invoker_listener)
    :   m_actionInvokerListener(std::move(action_invoker_listener))
{
}


std::shared_ptr<ActionInvoker::Listener> QuestionTextActionInvokerWebControllerPreProcessMessageWorker::GetListener()
{
    // CS_TODO: we could update the field values prior to the message being processed;
    // this is what happens on Android, copying the 7.7 behavior, but it was not implemented
    // on Windows in 7.7 so it is left undone

    return m_actionInvokerListener;
}


void QSFView::SetUpActionInvoker()
{
    ActionInvoker::WebController& web_controller = m_htmlViewCtrl.RegisterCSProHostObject();

    web_controller.SetPreProcessMessageWorker(std::make_unique<QuestionTextActionInvokerWebControllerPreProcessMessageWorker>(
                                              std::make_shared<QuestionTextActionInvokerListener>(web_controller.GetCaller())));
}
