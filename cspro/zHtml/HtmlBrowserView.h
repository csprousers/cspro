#pragma once

#include <zHtml/zHtml.h>
#include <zHtml/HtmlViewCtrl.h>
#include <zHtml/UriResolver.h>

class DynamicLayoutControlResizer;
class ReturnProcessingEdit;


class ZHTML_API HtmlBrowserView : public CFormView
{
public:
    HtmlBrowserView();
    ~HtmlBrowserView();

    BOOL Create(DWORD dwStyle, CWnd* pParentWnd, UINT nID);

    HtmlViewCtrl& GetHtmlViewCtrl() { return m_htmlViewCtrl; }

    void NavigateTo(std::shared_ptr<UriResolver> uri_resolver);

protected:
    DECLARE_MESSAGE_MAP()

    void DoDataExchange(CDataExchange* pDX) override;

    void OnSize(UINT nType, int cx, int cy);
    
private:
    void OnSourceChanged(const std::wstring& uri) const;

private:
    std::unique_ptr<ReturnProcessingEdit> m_htmlSourceEdit;
    HtmlViewCtrl m_htmlViewCtrl;
    std::unique_ptr<DynamicLayoutControlResizer> m_dynamicLayoutControlResizer;

    std::vector<std::shared_ptr<UriResolver>> m_uriResolversWithDomains;
};
