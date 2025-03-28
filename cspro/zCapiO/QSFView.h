#pragma once

#include <zCapiO/zCapiO.h>
#include <zHtml/HtmlViewCtrl.h>
#include <mutex>

class SharedHtmlLocalFileServer;
class VirtualFileMapping;


/////////////////////////////////////////////////////////////////////////////
// QSFView view
//
// A form view that displays question text using an HTML control.
//
/////////////////////////////////////////////////////////////////////////////
class CLASS_DECL_ZCAPIO QSFView : public CFormView
{
    DECLARE_DYNCREATE(QSFView)

protected:
    QSFView(); // protected constructor used by dynamic creation

public:
    ~QSFView();

    void SetUpQuestionTextView(const std::string& application_file_path);

    void SetText(SharableString text, std::optional<PortableColor> background_color = std::nullopt);
    void SetStyleCss(std::string css);

protected:
    DECLARE_MESSAGE_MAP()

    void DoDataExchange(CDataExchange* pDX) override;

    afx_msg void OnSize(UINT nType, int cx, int cy);
    afx_msg void OnDestroy();

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
    SharableString m_questionText;

    SharableString m_html;
    std::mutex m_htmlMutex;
};
