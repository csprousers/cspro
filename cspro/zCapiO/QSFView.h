#pragma once

#include <zCapiO/zCapiO.h>
#include <zCapiO/CapiText.h>
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

    // if passing the text as a SharableString, it should already be evaluated as HTML
    void SetCapiText(std::variant<SharableString, CapiText> capi_text, const COLORREF* background_color);

    void SetStyleCss(std::string css);

protected:
    DECLARE_MESSAGE_MAP()

    void DoDataExchange(CDataExchange* pDX) override;

    void OnSize(UINT nType, int cx, int cy);
    void OnDestroy();

    LRESULT OnRefreshQuestionText(WPARAM wParam, LPARAM lParam);

private:
    static const std::string& DefaultBackgroundColor();

    void UpdateHtml();

    void SetUpActionInvoker();

private:
    HtmlViewCtrl m_htmlViewCtrl;
    std::unique_ptr<SharedHtmlLocalFileServer> m_fileServer;
    std::unique_ptr<VirtualFileMapping> m_questionTextVirtualFileMapping;

    std::string m_backgroundColor;
    std::string m_stylesheet;
    std::variant<SharableString, CapiText> m_capiText;

    SharableString m_html;
    std::mutex m_htmlMutex;
};
