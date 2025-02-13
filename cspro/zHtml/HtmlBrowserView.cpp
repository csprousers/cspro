#include "stdafx.h"
#include "HtmlBrowserView.h"
#include <zUtilF/DynamicLayoutControlResizer.h>


// --------------------------------------------------------------------------
// ReturnProcessingEdit
// --------------------------------------------------------------------------

class ReturnProcessingEdit : public CEdit
{
    BOOL PreTranslateMessage(MSG* pMsg) override
    {
        if( pMsg->message == WM_KEYDOWN )
        {
            // on Return, send the URL to the browser
            if( pMsg->wParam == VK_RETURN )
            {
                std::wstring url = WindowsWS::GetWindowText(this);
                assert_cast<HtmlBrowserView*>(GetParent())->NavigateTo(UriResolver::CreateFromUserUri(url));
                return TRUE;
            }

            // Select All
            else if( pMsg->wParam == 'A' && GetKeyState(VK_CONTROL) < 0 )
            {
                SetSel(0, -1, FALSE);
                return TRUE;
            }
        }

        return __super::PreTranslateMessage(pMsg);
    }
};



// --------------------------------------------------------------------------
// HtmlBrowserView
// --------------------------------------------------------------------------

BEGIN_MESSAGE_MAP(HtmlBrowserView, CFormView)
	ON_WM_SIZE()
END_MESSAGE_MAP()


HtmlBrowserView::HtmlBrowserView()
    :   CFormView(IDD_HTML_BROWSER),
        m_htmlSourceEdit(std::make_unique<ReturnProcessingEdit>())
{
    m_htmlViewCtrl.UseWebView2AcceleratorKeyHandler();

    m_htmlViewCtrl.AddSourceChangedObserver([&](const std::wstring& uri) { OnSourceChanged(uri); });
}


HtmlBrowserView::~HtmlBrowserView()
{
}


BOOL HtmlBrowserView::Create(DWORD dwStyle, CWnd* pParentWnd, UINT nID)
{
    if( !__super::Create(nullptr, nullptr, dwStyle, CRect(), pParentWnd, nID, nullptr) )
        return FALSE;

    OnInitialUpdate();

    return TRUE;
}


void HtmlBrowserView::DoDataExchange(CDataExchange* pDX)
{
    __super::DoDataExchange(pDX);

    DDX_Control(pDX, IDC_HTML_SOURCE, *m_htmlSourceEdit);
    DDX_Control(pDX, IDC_HTML_BROWSER, m_htmlViewCtrl);
}


void HtmlBrowserView::OnSize(UINT nType, int cx, int cy)
{
    // the WebView2 control doesn't seem to respond to Dynamic Layout settings
    if( m_dynamicLayoutControlResizer == nullptr )
        m_dynamicLayoutControlResizer = std::make_unique<DynamicLayoutControlResizer>(*this, std::initializer_list<CWnd*>{ &m_htmlViewCtrl });

    __super::OnSize(nType, cx, cy);

    m_dynamicLayoutControlResizer->OnSize(cx, cy);
}


void HtmlBrowserView::NavigateTo(std::shared_ptr<UriResolver> uri_resolver)
{
    ASSERT(uri_resolver != nullptr);

    if( uri_resolver->HasDomain() )
        m_uriResolversWithDomains.emplace_back(uri_resolver);

    m_htmlViewCtrl.NavigateTo(std::move(uri_resolver));
}


void HtmlBrowserView::OnSourceChanged(const std::wstring& uri) const
{
    // search, in reverse order, for a domain that matches
    const auto& lookup = std::find_if(m_uriResolversWithDomains.crbegin(), m_uriResolversWithDomains.crend(),
                                      [&](const std::shared_ptr<UriResolver>& uri_resolver) { return ( uri_resolver->DomainMatches(uri) ); });

    m_htmlSourceEdit->SetWindowText(( lookup != m_uriResolversWithDomains.crend() ) ? (*lookup)->GetSourceText(uri).c_str() :
                                                                                      uri.c_str());
}
