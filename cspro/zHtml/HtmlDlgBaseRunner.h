#pragma once

#include <zHtml/zHtml.h>
#include <zHtml/NavigationAddress.h>

class HtmlDlgBase;


class ZHTML_API HtmlDlgBaseRunner
{
protected:
    HtmlDlgBaseRunner()
        :   m_actionInvokerAccessTokenOverride(nullptr)
    {
    }

public:
    virtual ~HtmlDlgBaseRunner() { }

    INT_PTR DoModal()           { return DoModal(false); }
    INT_PTR DoModalOnUIThread() { return DoModal(true); }

protected:
    const std::wstring* GetActionInvokerAccessTokenOverride() const { return m_actionInvokerAccessTokenOverride; }

    virtual NavigationAddress GetNavigationAddress() = 0;

#ifdef WIN_DESKTOP
    virtual std::unique_ptr<HtmlDlgBase> CreateHtmlDlg() = 0;
#else
    virtual std::optional<std::wstring> RunHtmlDlg() = 0;
#endif

    virtual INT_PTR ProcessResults(const std::optional<std::wstring>& results_text) = 0;

    const std::wstring* RegisterActionInvokerAccessTokenOverride(const std::wstring& path);

private:
    INT_PTR DoModal(bool on_ui_thread);

private:
    const std::wstring* m_actionInvokerAccessTokenOverride;
};
