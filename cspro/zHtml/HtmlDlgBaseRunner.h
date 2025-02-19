#pragma once

#include <zHtml/zHtml.h>
#include <zHtml/NavigationAddress.h>
#include <zToolsO/ExceptionHolder.h>

class HtmlDlgBase;


class ZHTML_API HtmlDlgBaseRunner
{
protected:
    HtmlDlgBaseRunner();

public:
    virtual ~HtmlDlgBaseRunner() { }

    INT_PTR DoModal()           { return DoModal(false); }
    INT_PTR DoModalOnUIThread() { return DoModal(true); }

    ExceptionHolder& GetExceptionHolder() { return m_exceptionHolder; }

protected:
    const std::string* GetActionInvokerAccessTokenOverride() const { return m_actionInvokerAccessTokenOverride; }

    virtual NavigationAddress GetNavigationAddress() = 0;

#ifdef WIN_DESKTOP
    virtual std::unique_ptr<HtmlDlgBase> CreateHtmlDlg() = 0;
#else
    virtual SharableString RunHtmlDlg() = 0;
#endif

    virtual INT_PTR ProcessResults(const SharableString& results_text) = 0;

    const std::string* RegisterActionInvokerAccessTokenOverride(const std::string& path);

private:
    INT_PTR DoModal(bool on_ui_thread);

protected:
    ExceptionHolder m_exceptionHolder;

private:
    const std::string* m_actionInvokerAccessTokenOverride;
};
