#pragma once

#include <zCapiO/zCapiO.h>
#include <zHtml/HtmlViewCtrl.h>
#include <mutex>

class SharedHtmlLocalFileServer;
class VirtualFileMapping;


// --------------------------------------------------------------------------
// QSFView
//
// A CFormView subclass that displays question text using an HTML control.
// --------------------------------------------------------------------------

class CLASS_DECL_ZCAPIO QSFView : public CFormView
{
    DECLARE_DYNCREATE(QSFView)

protected:
    QSFView(); // protected constructor used by dynamic creation

public:
    ~QSFView();

    void SetUpQuestionTextView(const std::string& application_file_path);

    void SetStyleCss(std::string css);

    void SetCapiTextHtml(SharableString capi_text_html, const COLORREF* background_color);

    static const std::string& DefaultBackgroundColor();

    static std::string CreateCapiTextHtml(std::string_view html_sv, std::string_view css_sv,
                                          std::string_view background_color_sv);
    std::string CreateCapiTextHtml(std::string_view html_sv) const;

protected:
    DECLARE_MESSAGE_MAP()

    void DoDataExchange(CDataExchange* pDX) override;

    void OnSize(UINT nType, int cx, int cy);
    void OnDestroy();

    LRESULT OnRefreshQuestionText(WPARAM wParam, LPARAM lParam);

private:
    void UpdateHtml();

    void SetUpActionInvoker();

private:
    HtmlViewCtrl m_htmlViewCtrl;
    std::unique_ptr<SharedHtmlLocalFileServer> m_fileServer;
    std::unique_ptr<VirtualFileMapping> m_questionTextVirtualFileMapping;

    std::string m_backgroundColor;
    std::string m_stylesheet;
    SharableString m_capiTextHtml;

    SharableString m_html;
    std::mutex m_htmlMutex;
};
