#include "stdafx.h"
#include "HtmlDlgBase.h"
#include "ModalDialogSimulator.h"
#include "SharedHtmlLocalFileServer.h"
#include <zUtilF/UIThreadRunner.h>
#include <zMessageO/Messages.h>
#include <zAction/WebController.h>


namespace SizingFactors
{
    constexpr double ForcedUpdate   = 0.50;
    constexpr int MaxTitleBarHeight = 100;
}

namespace ForceUpdateSize
{
    constexpr UINT TimerId        = 20210902;
    constexpr unsigned ElapseTime = 3 * 1000; // three seconds
}


struct HtmlDlgDisplayOptions
{
    std::optional<int> width;
    std::optional<int> height;
    std::optional<bool> resizable;
    std::optional<PortableColor> border_color;
    std::optional<PortableColor> title_bar_color;
    std::optional<int> title_bar_height;
};


BEGIN_MESSAGE_MAP(HtmlDlgBase, CDialog)
    ON_WM_CTLCOLOR()
    ON_WM_GETMINMAXINFO()
    ON_WM_SIZE()
    ON_WM_TIMER()
    ON_MESSAGE(UWM::Html::CloseDialog, OnCloseDialog)
    ON_MESSAGE(UWM::Html::ProcessDisplayOptions, OnProcessDisplayOptions)
    ON_MESSAGE(UWM::Html::ExecuteSizeUpdate, OnExecuteSizeUpdate)
END_MESSAGE_MAP()


HtmlDlgBase::HtmlDlgBase(CWnd* pParent/* = nullptr*/)
    :   CDialog(IDD_CSHTML, pParent),
        m_resizable(false)
{
}

HtmlDlgBase::~HtmlDlgBase()
{
}


INT_PTR HtmlDlgBase::DoModal()
{
    SimulateModalDialog(this,
        [&](CWnd* parent_window)
        {
            Create(IDD_CSHTML, parent_window);

            // construct the dialog with no width/height, which will be modified in UpdateSize
            SetWindowPos(AfxGetMainWnd(), 0, 0, 0, 0, SWP_SHOWWINDOW);
        });

    return m_nModalResult;
}

INT_PTR HtmlDlgBase::DoModalOnUIThread()
{
    return DialogUIThreadRunner(this).DoModal();
}


void HtmlDlgBase::DoDataExchange(CDataExchange* pDX)
{
    DDX_Control(pDX, IDC_SIMULATED_TITLE_BAR, m_simulatedTitleBar);
    DDX_Control(pDX, IDC_HTML_VIEW, m_htmlViewCtrl);
}


BOOL HtmlDlgBase::OnInitDialog()
{
    CDialog::OnInitDialog();

    ASSERT(GetSystemMetrics(SM_CXEDGE) == GetSystemMetrics(SM_CYEDGE) &&
           GetSystemMetrics(SM_CYEDGE) < GetSystemMetrics(SM_CYSIZEFRAME));

    // the all-around border will be black and the simulated title bar will
    // be gray but both colors can be modified by a HTML callback
    m_borderDetails.emplace(BorderDetails
        {
            GetSystemMetrics(SM_CXEDGE),
            std::make_shared<CBrush>(RGB(0, 0, 0)),
            GetSystemMetrics(SM_CYSIZEFRAME) - GetSystemMetrics(SM_CYEDGE),
            std::make_shared<CBrush>(RGB(0xEE, 0xEE, 0xEE))
        });

    // set up the Action Invoker
    SetUpActionInvoker();

    // modify the accelerator key handler so that all keys are handled by the HTML control
    m_htmlViewCtrl.UseWebView2AcceleratorKeyHandler();

    // navigate to the HTML either via a URI or a HTML filename
    NavigationAddress navigation_address = GetNavigationAddress();

    if( navigation_address.IsUri() )
    {
        m_htmlViewCtrl.NavigateTo(navigation_address.GetUri());
    }

    else
    {
        ASSERT(navigation_address.IsHtmlFilename());

        m_fileServer = std::make_unique<SharedHtmlLocalFileServer>();

        m_htmlViewCtrl.NavigateTo(m_fileServer->GetFilenameUrl(navigation_address.GetHtmlFilename()));
    }

    // set a timer so that if the displayed HTML doesn't call CSPro.setDisplayOptions
    // to set the dialog size, we will eventually modify the dialog size (making it visible)
    m_forceUpdateSizeTimerId = SetTimer(ForceUpdateSize::TimerId, ForceUpdateSize::ElapseTime, nullptr);
                     
    return TRUE;
}


LRESULT HtmlDlgBase::WindowProc(UINT message, WPARAM wParam, LPARAM lParam)
{
    if( message == WM_CTLCOLORDLG && m_borderDetails.has_value() )
    {
        // set the dialog's background color (which will show as the border)
        return reinterpret_cast<LRESULT>(m_borderDetails->border_brush->GetSafeHandle());
    }

    else if( message == WM_NCHITTEST && m_borderDetails.has_value() )
    {
        // because we are drawing the borders of the dialog ourself, we need
        // to check if the mouse is over a simulated border;
        // code based on https://docs.microsoft.com/en-us/windows/win32/dwm/customframe

        RECT rect;
        GetWindowRect(&rect);

        POINT mouse_point = { GET_X_LPARAM(lParam), GET_Y_LPARAM(lParam) };

        bool on_left_border = ( mouse_point.x < ( rect.left + m_borderDetails->border_thickness ) );
        bool on_right_border = ( !on_left_border && mouse_point.x > ( rect.right - m_borderDetails->border_thickness ) );

        bool on_top_border = ( mouse_point.y < ( rect.top + m_borderDetails->border_thickness ) );

        LRESULT result;

        if( on_top_border || mouse_point.y < ( rect.top + m_borderDetails->border_thickness + m_borderDetails->simulated_title_bar_height ) )
        {
            result = on_left_border  ? HTTOPLEFT :
                     on_right_border ? HTTOPRIGHT :
                     on_top_border   ? HTTOP :
                                       HTCAPTION;
        }

        else if( mouse_point.y > ( rect.bottom - m_borderDetails->border_thickness ) )
        {
            result = on_left_border  ? HTBOTTOMLEFT :
                     on_right_border ? HTBOTTOMRIGHT :
                                       HTBOTTOM;
        }

        else
        {
            result = on_left_border  ? HTLEFT :
                     on_right_border ? HTRIGHT :
                                       HTNOWHERE;
        }

        // when using a fixed display size, there is no need to allow resizing cursors
        if( m_fixedDisplaySize.has_value() && result != HTCAPTION )
            result = HTNOWHERE;

        return result;
    }

    else if( message == WM_COMMAND && HIWORD(wParam) == BN_CLICKED && LOWORD(wParam) == IDOK )
    {
        // prevent the dialog from being closed by the default handling of the Enter key
        return 0;
    }

    return CDialog::WindowProc(message, wParam, lParam);
}


HBRUSH HtmlDlgBase::OnCtlColor(CDC* pDC, CWnd* pWnd, UINT nCtlColor)
{
    HBRUSH brush = CDialog::OnCtlColor(pDC, pWnd, nCtlColor);

    // properly color the simulated title bar
    if( pWnd == &m_simulatedTitleBar && m_borderDetails.has_value() )
        brush = static_cast<HBRUSH>(*m_borderDetails->simulated_title_bar_brush);

    return brush;
}


void HtmlDlgBase::OnGetMinMaxInfo(MINMAXINFO* lpMMI)
{
    CDialog::OnGetMinMaxInfo(lpMMI);

    if( m_fixedDisplaySize.has_value() )
    {
        lpMMI->ptMinTrackSize.x = lpMMI->ptMaxTrackSize.x = m_fixedDisplaySize->cx;
        lpMMI->ptMinTrackSize.y = lpMMI->ptMaxTrackSize.y = m_fixedDisplaySize->cy;
    }
}


void HtmlDlgBase::OnSize(UINT nType, int cx, int cy)
{
    CDialog::OnSize(nType, cx, cy);

    // quit out if OnInitDialog hasn't been called yet
    if( !m_borderDetails.has_value() )
        return;

    // anchor the HTML view in all ways, filling the entire dialog except for the space
    // for the borders and the additional space on top for the simulated title bar
    CRect client_rect;
    GetClientRect(client_rect);

    CRect move_rect = client_rect;
    move_rect.left += m_borderDetails->border_thickness;
    move_rect.right -= m_borderDetails->border_thickness;
    move_rect.top += m_borderDetails->border_thickness;
    move_rect.bottom = move_rect.top + m_borderDetails->simulated_title_bar_height;
    m_simulatedTitleBar.MoveWindow(move_rect);

    move_rect.top = move_rect.bottom;
    move_rect.bottom = client_rect.bottom - m_borderDetails->border_thickness;
    m_htmlViewCtrl.MoveWindow(move_rect);
}


void HtmlDlgBase::OnTimer(UINT nIDEvent)
{
    ASSERT(m_forceUpdateSizeTimerId == nIDEvent);

    // force an update of the dialog size
    CSize forced_update_size = Screen::GetScaledDisplaySize(SizingFactors::ForcedUpdate);
    UpdateSize(forced_update_size.cx, forced_update_size.cy);

    ASSERT(!m_forceUpdateSizeTimerId.has_value());
}


LRESULT HtmlDlgBase::OnCloseDialog(WPARAM wParam, LPARAM /*lParam*/)
{
    if( wParam == IDOK )
    {
        OnOK();
    }

    else
    {
        ASSERT(wParam == IDCANCEL);
        OnCancel();
    }

    return 0;
}


HtmlDlgDisplayOptions HtmlDlgBase::ParseDisplayOptions(const JsonNode<wchar_t>& json_node)
{
    HtmlDlgDisplayOptions display_options;

    // width + height
    auto parse_dimension = [&](std::optional<int>& width_or_height, const TCHAR* key, LONG max_display_size)
    {
        if( !json_node.Contains(key) )
            return;

        std::wstring dimension_text = json_node.Get<std::wstring>(key);

        width_or_height = static_cast<int>(Screen::ParseDimensionText(dimension_text, max_display_size, 
            [&]()
            {
                throw CSProException(MGF::GetMessageText(2037).c_str(), key, dimension_text.c_str());
            }));
    };

    parse_dimension(display_options.width, JK::width, Screen::GetMaxDisplayWidth());
    parse_dimension(display_options.height, JK::height, Screen::GetMaxDisplayHeight());

    // if width or height are specified, both must be specified
    if( display_options.width.has_value() != display_options.height.has_value() )
        throw CSProException(MGF::GetMessageText(2033));

    // resizable
    if( json_node.Contains(JK::resizable) )
        display_options.resizable = json_node.Get<bool>(JK::resizable);

    // borderColor + titleBarColor
    auto parse_color = [&](std::optional<PortableColor>& portable_color, const TCHAR* key)
    {
        if( !json_node.Contains(key) )
            return;

        std::wstring color_text = json_node.Get<std::wstring>(key);
        portable_color = PortableColor::FromString(color_text);

        if( !portable_color.has_value() )
            throw CSProException(MGF::GetMessageText(2036).c_str(), color_text.c_str());
    };

    parse_color(display_options.border_color, JK::borderColor);
    parse_color(display_options.title_bar_color, JK::titleBarColor);

    // titleBarHeight
    if( json_node.Contains(JK::titleBarHeight) )
    {
        display_options.title_bar_height = std::min(json_node.Get<int>(JK::titleBarHeight), SizingFactors::MaxTitleBarHeight);

        if( *display_options.title_bar_height < 0 )
            throw CSProException("The title bar height cannot be a negative number.");
    }

    return display_options;
}


LRESULT HtmlDlgBase::OnProcessDisplayOptions(WPARAM wParam, LPARAM lParam)
{
    ASSERT(m_borderDetails.has_value());

    try
    {
        auto get_display_options = [&]() -> HtmlDlgDisplayOptions
        {
            const HtmlDlgDisplayOptions* display_options = reinterpret_cast<const HtmlDlgDisplayOptions*>(lParam);

            if( display_options != nullptr )
            {
                return *display_options;
            }

            else
            {
                const std::wstring* display_options_json = reinterpret_cast<const std::wstring*>(wParam);
                ASSERT(display_options_json != nullptr);
                return ParseDisplayOptions(Json::Parse(*display_options_json));
            }
        };
        
        HtmlDlgDisplayOptions display_options = get_display_options();

        // modify the color of the border / title bar
        if( display_options.border_color.has_value() )
            m_borderDetails->border_brush = std::make_shared<CBrush>(display_options.border_color->ToCOLORREF());

        if( display_options.title_bar_color.has_value() )
            m_borderDetails->simulated_title_bar_brush = std::make_shared<CBrush>(display_options.title_bar_color->ToCOLORREF());

        // modify the title bar height
        if( display_options.title_bar_height.has_value() )
            m_borderDetails->simulated_title_bar_height = *display_options.title_bar_height;

        // modify the resizable attribute
        if( display_options.resizable.has_value() )
            m_resizable = *display_options.resizable;

        // modify the dialog size
        if( display_options.width.has_value() )
        {
            ASSERT(display_options.height.has_value());

            if( !UpdateSize(*display_options.width, *display_options.height) )
                throw CSProException(MGF::GetMessageText(2035).c_str(), *display_options.width, *display_options.height);
        }
    }

    catch( const CSProException& exception )
    {
        ErrorMessage::Display(exception);
    }

    return 0;
}


bool HtmlDlgBase::UpdateSize(int width, int height)
{
    // kill the timer that would have eventually set a default dialog size
    if( m_forceUpdateSizeTimerId.has_value() )
    {
        KillTimer(*m_forceUpdateSizeTimerId);
        m_forceUpdateSizeTimerId.reset();
    }

    // make sure that the width and height are valid
    bool valid_dimensions = true;

    auto make_valid_size = [&](int& size, int max_size)
    {
        int initial_size = size;
        size = std::max(1, std::min(size, max_size));
        valid_dimensions &= ( initial_size == size );
    };

    make_valid_size(width, Screen::GetMaxDisplayWidth());
    make_valid_size(height, Screen::GetMaxDisplayHeight());

    m_requestedDisplaySize = std::make_optional<CSize>(width, height);

    // adjust the dimensions to account for the border
    width += 2 * m_borderDetails->border_thickness;
    height += 2 * m_borderDetails->border_thickness + m_borderDetails->simulated_title_bar_height;

    m_fixedDisplaySize = m_resizable ? std::nullopt : std::make_optional<CSize>(width, height);

    PostMessage(UWM::Html::ExecuteSizeUpdate, width, height);

    return valid_dimensions;
}


LRESULT HtmlDlgBase::OnExecuteSizeUpdate(WPARAM wParam, LPARAM lParam)
{
    ASSERT(m_borderDetails.has_value());

    // center the dialog on the parent window
    WindowHelpers::CenterOnParent(GetSafeHwnd(), static_cast<int>(wParam), static_cast<int>(lParam));

    // focus on the HTML control
    m_htmlViewCtrl.MoveFocus();

    return 0;
}



// --------------------------------------------------------------------------
// Action Invoker functionality
// --------------------------------------------------------------------------

class HtmlDlgBaseActionInvokerListener : public ActionInvoker::Listener
{
public:
    HtmlDlgBaseActionInvokerListener(HtmlDlgBase& dlg, ActionInvoker::Caller& caller);

    // Listener overrides
    std::optional<std::wstring> OnGetDisplayOptions(ActionInvoker::Caller& caller) override;
    std::optional<bool> OnSetDisplayOptions(const JsonNode<wchar_t>& json_node, ActionInvoker::Caller& caller) override;

    std::optional<bool> OnCloseDialog(const JsonNode<wchar_t>& result_node, ActionInvoker::Caller& caller) override;

    bool OnEngineProgramControlExecuted() override;

private:
    HtmlDlgBase& m_dlg;
    ActionInvoker::Caller& m_actionInvokerCaller;
};


HtmlDlgBaseActionInvokerListener::HtmlDlgBaseActionInvokerListener(HtmlDlgBase& dlg, ActionInvoker::Caller& caller)
    :   m_dlg(dlg),
        m_actionInvokerCaller(caller)
{
}


std::optional<std::wstring> HtmlDlgBaseActionInvokerListener::OnGetDisplayOptions(ActionInvoker::Caller& caller)
{
    if( !caller.IsFromWebView(m_actionInvokerCaller) )
        return std::nullopt;

    auto json_writer = Json::CreateStringWriter();

    json_writer->BeginObject();

    if( m_dlg.m_requestedDisplaySize.has_value() )
    {
        json_writer->Write(JK::width, static_cast<int>(m_dlg.m_requestedDisplaySize->cx))
                    .Write(JK::height, static_cast<int>(m_dlg.m_requestedDisplaySize->cy));
    }

    json_writer->Write(JK::resizable, m_dlg.m_resizable);

    if( m_dlg.m_borderDetails.has_value() )
    {
        auto write_brush_color = [&](const TCHAR* key, const std::shared_ptr<CBrush>& brush)
        {
            LOGBRUSH lb;

            if( brush != nullptr && brush->GetLogBrush(&lb) )
                json_writer->Write(key, PortableColor::FromCOLORREF(lb.lbColor));
        };

        write_brush_color(JK::borderColor, m_dlg.m_borderDetails->border_brush);
        write_brush_color(JK::titleBarColor, m_dlg.m_borderDetails->simulated_title_bar_brush);

        json_writer->Write(JK::titleBarHeight, m_dlg.m_borderDetails->simulated_title_bar_height);
    }

    json_writer->EndObject();

    return json_writer->GetString();
}


std::optional<bool> HtmlDlgBaseActionInvokerListener::OnSetDisplayOptions(const JsonNode<wchar_t>& json_node, ActionInvoker::Caller& caller)
{
    if( !caller.IsFromWebView(m_actionInvokerCaller) )
        return std::nullopt;

    HtmlDlgDisplayOptions display_options = m_dlg.ParseDisplayOptions(json_node);

    m_dlg.SendMessage(UWM::Html::ProcessDisplayOptions, 0, reinterpret_cast<LPARAM>(&display_options));
    return true;
}


std::optional<bool> HtmlDlgBaseActionInvokerListener::OnCloseDialog(const JsonNode<wchar_t>& result_node, ActionInvoker::Caller& /*caller*/)
{
    if( !result_node.IsEmpty() )
        m_dlg.m_resultsText = result_node.GetNodeAsString();

    m_dlg.PostMessage(UWM::Html::CloseDialog, IDOK);
    return true;
}


bool HtmlDlgBaseActionInvokerListener::OnEngineProgramControlExecuted()
{
    m_dlg.PostMessage(UWM::Html::CloseDialog, IDCANCEL);
    return true;
}


void HtmlDlgBase::SetUpActionInvoker()
{
    ActionInvoker::WebController& web_controller = m_htmlViewCtrl.RegisterCSProHostObject();

    if( m_actionInvokerAccessTokenOverride != nullptr )
        web_controller.GetCaller().AddAccessTokenOverride(*m_actionInvokerAccessTokenOverride);

    web_controller.GetListener().SetOnGetInputDataCallback([&]() { return GetInputData(); });

    m_actionInvokerListenerHolder = ActionInvoker::ListenerHolder::Create<HtmlDlgBaseActionInvokerListener>(*this, web_controller.GetCaller());
}
