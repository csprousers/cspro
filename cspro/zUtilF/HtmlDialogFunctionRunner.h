#pragma once

#include <zUtilF/zUtilF.h>
#include <zHtml/HtmlDlgBase.h>
#include <zHtml/HtmlDlgBaseRunner.h>


class CLASS_DECL_ZUTILF HtmlDialogFunctionRunner : public HtmlDlgBaseRunner
{
    friend class HtmlDialogFunctionDlg;

public:
    HtmlDialogFunctionRunner(NavigationAddress navigation_address, std::wstring input_data,
                                                                   std::optional<std::wstring> display_options_json);

    const std::optional<std::wstring>& GetResultsText() const { return m_resultsText; }

    static void ParseSingleInputText(const std::wstring& single_input_text, std::optional<std::wstring>& input_data,
                                                                            std::optional<std::wstring>& display_options_json);

protected:
    // HtmlDlgBaseRunner overrides
    NavigationAddress GetNavigationAddress() override { return m_navigationAddress; }

#ifdef WIN_DESKTOP
    std::unique_ptr<HtmlDlgBase> CreateHtmlDlg() override;
#else
    std::optional<std::wstring> RunHtmlDlg() override;
#endif

    INT_PTR ProcessResults(const std::optional<std::wstring>& results_text) override;

private:
    const NavigationAddress m_navigationAddress;
    const std::wstring m_inputData;
    const std::optional<std::wstring> m_displayOptionsJson;

    std::optional<std::wstring> m_resultsText;
};
