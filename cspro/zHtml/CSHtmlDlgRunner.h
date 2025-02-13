#pragma once

#include <zHtml/zHtml.h>
#include <zHtml/HtmlDlgBaseRunner.h>
#include <zHtml/UseHtmlDialogs.h>
#include <zJson/JsonNode.h>


class ZHTML_API CSHtmlDlgRunner : public HtmlDlgBaseRunner
{
    friend class CSHtmlDlg;

protected:
    CSHtmlDlgRunner() { }

    // HtmlDlgBaseRunner overrides
    NavigationAddress GetNavigationAddress() override;

#ifdef WIN_DESKTOP
    std::unique_ptr<HtmlDlgBase> CreateHtmlDlg() override;
#else
    std::optional<std::wstring> RunHtmlDlg() override;
#endif

    INT_PTR ProcessResults(const std::optional<std::wstring>& results_text) override;

    // methods that subclasses must override
    virtual const TCHAR* GetDialogName() = 0;
    virtual std::wstring GetJsonArgumentsText() = 0;
    virtual void ProcessJsonResults(const JsonNode<wchar_t>& json_results) = 0;
};
