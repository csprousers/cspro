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
    SharableString RunHtmlDlg() override;
#endif

    INT_PTR ProcessResults(const SharableString& results_text) override;

    // methods that subclasses must override
    virtual std::string GetDialogName() = 0;
    virtual SharableString GetJsonArgumentsText() = 0;
    virtual void ProcessJsonResults(const JsonNode& json_results) = 0;
};
