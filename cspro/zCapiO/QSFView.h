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

    void SetupFileServer(const CString& application_filename);

    void SetText(const CString& text, std::optional<PortableColor> background_color = std::nullopt);
    void SetStyleCss(std::wstring css);

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

    std::wstring m_backgroundColor;
    std::wstring m_stylesheet;
    CString m_questionText;

    std::string m_html;
    std::mutex m_htmlMutex;
};
