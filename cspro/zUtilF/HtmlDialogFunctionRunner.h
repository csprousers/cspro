#pragma once

#include <zUtilF/zUtilF.h>
#include <zHtml/HtmlDlgBase.h>
#include <zHtml/HtmlDlgBaseRunner.h>


class CLASS_DECL_ZUTILF HtmlDialogFunctionRunner : public HtmlDlgBaseRunner
{
    friend class HtmlDialogFunctionDlg;

public:
    HtmlDialogFunctionRunner(NavigationAddress navigation_address, SharableString input_data, SharableString display_options_json);

    const SharableString& GetResultsText() const { return m_resultsText; }

    static void ParseSingleInputText(const std::string& single_input_text, SharableString& input_data, SharableString& display_options_json);

protected:
    // HtmlDlgBaseRunner overrides
    NavigationAddress GetNavigationAddress() override { return m_navigationAddress; }

#ifdef WIN_DESKTOP
    std::unique_ptr<HtmlDlgBase> CreateHtmlDlg() override;
#else
    SharableString RunHtmlDlg() override;
#endif

    INT_PTR ProcessResults(const SharableString& results_text) override;

private:
    const NavigationAddress m_navigationAddress;
    const SharableString m_inputData;
    const SharableString m_displayOptionsJson;

    SharableString m_resultsText;
};
