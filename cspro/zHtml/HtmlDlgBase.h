#pragma once

#ifdef WIN_DESKTOP

#include <zHtml/zHtml.h>
#include <zHtml/HtmlViewCtrl.h>
#include <zHtml/NavigationAddress.h>

class ExceptionHolder;
struct HtmlDlgDisplayOptions;
class SharedHtmlLocalFileServer;
namespace ActionInvoker { class ListenerHolder; }


// the base class for showing CSPro-style HTML-based dialogs;
// one implementation, CSHtmlDlg, is used for showing our own UI elements;
// another implementation, HtmlDialogFunctionDlg, is used for the logic function

class ZHTML_API HtmlDlgBase : public CDialog
{
    friend class HtmlDlgBaseActionInvokerListener;

public:
    HtmlDlgBase(ExceptionHolder* exception_holder, CWnd* pParent = nullptr);
    ~HtmlDlgBase();

    void SetActionInvokerAccessTokenOverride(std::string access_token) { m_actionInvokerAccessTokenOverride = std::make_unique<std::string>(std::move(access_token)); }

    INT_PTR DoModal() override;

    INT_PTR DoModalOnUIThread();

    const SharableString& GetResultsText() const { return m_resultsText; }

protected:
    virtual NavigationAddress GetNavigationAddress() = 0;
    virtual SharableString GetInputData() = 0;

protected:
    DECLARE_MESSAGE_MAP()

    void DoDataExchange(CDataExchange* pDX) override;
    BOOL OnInitDialog() override;

    LRESULT WindowProc(UINT message, WPARAM wParam, LPARAM lParam) override;

    LRESULT OnCloseDialog(WPARAM wParam, LPARAM lParam);

    HBRUSH OnCtlColor(CDC* pDC, CWnd* pWnd, UINT nCtlColor);
    void OnGetMinMaxInfo(MINMAXINFO* lpMMI);
    void OnSize(UINT nType, int cx, int cy);
    void OnTimer(UINT_PTR nIDEvent);

    LRESULT OnProcessDisplayOptions(WPARAM wParam, LPARAM lParam);

    bool UpdateSize(int width, int height);
    LRESULT OnExecuteSizeUpdate(WPARAM wParam, LPARAM lParam);

private:
    void SetUpActionInvoker();

    HtmlDlgDisplayOptions ParseDisplayOptions(const JsonNode& json_node);

protected:
    bool m_resizable;

private:
    CStatic m_simulatedTitleBar;
    HtmlViewCtrl m_htmlViewCtrl;

    std::unique_ptr<SharedHtmlLocalFileServer> m_fileServer;

    std::unique_ptr<std::string> m_actionInvokerAccessTokenOverride;
    std::unique_ptr<ActionInvoker::ListenerHolder> m_actionInvokerListenerHolder;

    std::optional<CSize> m_requestedDisplaySize;
    std::optional<CSize> m_fixedDisplaySize;

    std::optional<UINT_PTR> m_forceUpdateSizeTimerId;

    struct BorderDetails
    {
        int border_thickness;
        std::shared_ptr<CBrush> border_brush;
        int simulated_title_bar_height;
        std::shared_ptr<CBrush> simulated_title_bar_brush;
    };

    std::optional<BorderDetails> m_borderDetails;

    ExceptionHolder* m_exceptionHolder;
    SharableString m_resultsText;
};

#endif
