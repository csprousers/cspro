#include "stdafx.h"
#include "HtmlViewDlg.h"
#include "ModalDialogSimulator.h"
#include <zUtilF/UIThreadRunner.h>


BEGIN_MESSAGE_MAP(HtmlViewDlg, CDialog)
    ON_WM_GETMINMAXINFO()
    ON_WM_SIZE()
END_MESSAGE_MAP()


HtmlViewDlg::HtmlViewDlg(CWnd* pParent/* = nullptr*/)
    :   CDialog(IDD_HTML_VIEW, pParent),
        m_closeButton(nullptr),
        m_closeButtonBorder(0),
        m_htmlViewCtrlBorder(0)
{
}


void HtmlViewDlg::SetViewerOptions(const ViewerOptions& viewer_options)
{
    m_viewerOptions = viewer_options;

    // parse any options specified in JSON, suppressing any errors
    if( m_viewerOptions.display_options_node != nullptr )
    {
        ASSERT(!m_viewerOptions.requested_size.has_value() &&
               !m_viewerOptions.title.has_value() &&
               !m_viewerOptions.show_close_button.has_value());

        const JsonNode<wchar_t>& display_options_node = *m_viewerOptions.display_options_node;

        try
        {
            if( display_options_node.Contains(JK::width) )
            {
                m_viewerOptions.requested_size.emplace(display_options_node.Get<int>(JK::width),
                                                       display_options_node.Get<int>(JK::height));
            }

        }
        catch(...) { }

        try
        {
            if( display_options_node.Contains(JK::title) )
                m_viewerOptions.title.emplace(display_options_node.Get<std::wstring>(JK::title));
        }
        catch(...) { }

        try
        {
            if( display_options_node.Contains(JK::showCloseButton) )
                m_viewerOptions.show_close_button.emplace(display_options_node.Get<bool>(JK::showCloseButton));
        }
        catch(...) { }
    }
}


void HtmlViewDlg::SetInitialHtml(std::wstring html)
{
    m_initialContents.emplace(true, std::move(html));
}

void HtmlViewDlg::SetInitialUrl(std::wstring url)
{
    m_initialContents.emplace(false, std::move(url));
}


INT_PTR HtmlViewDlg::DoModal()
{
    SimulateModalDialog(this,
        [&](CWnd* parent_window)
        {
            Create(IDD_HTML_VIEW, parent_window);

            ShowWindow(SW_SHOW);
        });

    return m_nModalResult;
}

INT_PTR HtmlViewDlg::DoModalOnUIThread()
{
    return DialogUIThreadRunner(this).DoModal();
}


void HtmlViewDlg::DoDataExchange(CDataExchange* pDX)
{
    DDX_Control(pDX, IDC_HTML_VIEW, m_htmlViewCtrl);
}


BOOL HtmlViewDlg::OnInitDialog()
{
    CDialog::OnInitDialog();

    WindowHelpers::RemoveDialogSystemIcon(*this);

    // potentially modify the title
    if( m_viewerOptions.title.has_value() )
        SetWindowText(m_viewerOptions.title->c_str());

    // when defined, load the initial contents
    if( m_initialContents.has_value() )
    {
        if( std::get<0>(*m_initialContents) )
        {
            m_htmlViewCtrl.SetHtml(std::move(std::get<1>(*m_initialContents)));
        }

        else
        {
            m_htmlViewCtrl.NavigateTo(std::move(std::get<1>(*m_initialContents)));
        }
    }

    // calculate some values to help with sizing
    CRect dialog_rect;
    GetWindowRect(dialog_rect);
    m_initialWindowSize = dialog_rect.Size();
    
    GetClientRect(dialog_rect);

    m_closeButton = GetDlgItem(IDOK);

    if( m_viewerOptions.show_close_button.value_or(true) )
    {
        CRect close_rect;
        m_closeButton->GetWindowRect(close_rect);
        ScreenToClient(close_rect);
        m_closeButtonBorder = dialog_rect.bottom - close_rect.bottom;

        CRect html_view_rect;
        m_htmlViewCtrl.GetWindowRect(html_view_rect);
        ScreenToClient(html_view_rect);
        m_htmlViewCtrlBorder = html_view_rect.left - dialog_rect.left;
    }

    else
    {
        // when hiding the close button, the HTML view will fill the whole dialog
        ASSERT(m_htmlViewCtrlBorder == 0);
        m_closeButton->ShowWindow(SW_HIDE);
    }

    // to size the dialog, use either a user supplied size (as long as it is greater
    // than the initial window size) or use a percentage of the screen's size
    CSize new_size;

    if( m_viewerOptions.requested_size.has_value() )
    {
        new_size.cx = std::max(m_initialWindowSize.cx, m_viewerOptions.requested_size->cx);
        new_size.cy = std::max(m_initialWindowSize.cy, m_viewerOptions.requested_size->cy);
    }

    else
    {
        new_size = Screen::GetMaxDisplaySize();
    }
        
    ASSERT(new_size.cx <= Screen::GetMaxDisplayWidth() && new_size.cy <= Screen::GetMaxDisplayHeight());
    
    SetWindowPos(nullptr, ( Screen::GetFullWidth() - new_size.cx ) / 2, ( Screen::GetFullHeight() - new_size.cy ) / 2,
        new_size.cx, new_size.cy, SWP_NOZORDER | SWP_NOACTIVATE);

    return TRUE;
}


void HtmlViewDlg::OnGetMinMaxInfo(MINMAXINFO FAR* lpMMI)
{
    CDialog::OnGetMinMaxInfo(lpMMI);

    lpMMI->ptMinTrackSize.x = std::max(lpMMI->ptMinTrackSize.x, m_initialWindowSize.cx);
    lpMMI->ptMinTrackSize.y = std::max(lpMMI->ptMinTrackSize.y, m_initialWindowSize.cy);
}


void HtmlViewDlg::OnSize(UINT nType, int cx, int cy)
{
    CDialog::OnSize(nType, cx, cy);

    // quit out if OnInitDialog hasn't been called yet
    if( m_closeButton == nullptr )
        return;

    CRect dialog_rect;
    GetClientRect(dialog_rect);

    int html_view_bottom_anchor;

    // anchor the close button in the bottom center
    if( m_viewerOptions.show_close_button.value_or(true) )
    {
        CRect close_rect;
        m_closeButton->GetWindowRect(close_rect);
        close_rect.MoveToX(( cx - close_rect.Width() ) / 2);
        close_rect.MoveToY(dialog_rect.bottom - close_rect.Height() - m_closeButtonBorder);
        m_closeButton->SetWindowPos(nullptr, close_rect.left, close_rect.top, -1, -1, SWP_NOSIZE | SWP_NOZORDER | SWP_NOACTIVATE);

        html_view_bottom_anchor = close_rect.top;
    }

    else
    {
        html_view_bottom_anchor = dialog_rect.bottom;
    }

    // anchor the HTML view in all ways
    CRect html_view_rect;
    html_view_rect.left = m_htmlViewCtrlBorder;
    html_view_rect.right = dialog_rect.right - m_htmlViewCtrlBorder;
    html_view_rect.top = m_htmlViewCtrlBorder;
    html_view_rect.bottom = html_view_bottom_anchor - m_htmlViewCtrlBorder;
    m_htmlViewCtrl.MoveWindow(html_view_rect);
}
