#pragma once

#include <zUtilF/zUtilF.h>
#include <zHtml/CSHtmlDlgRunner.h>


class CLASS_DECL_ZUTILF ErrmsgDlg : public CSHtmlDlgRunner
{
public:
    ErrmsgDlg();

    void SetTitle(SharableString title) { m_title = std::move(title); }

    void SetMessage(SharableString message) { m_message = std::move(message); }

    void SetButtons(std::vector<SharableString> buttons) { m_buttons = std::move(buttons); }

    void SetDefaultButtonIndex(int default_button_index) { m_defaultButtonIndex = default_button_index; }

    int GetSelectedButtonIndex() const { return m_selectedButtonIndex; }

protected:
    std::string GetDialogName() override;
    SharableString GetJsonArgumentsText() override;
    void ProcessJsonResults(const JsonNode& json_results) override;

private:
    SharableString m_title;
    SharableString m_message;
    std::vector<SharableString> m_buttons;
    int m_defaultButtonIndex;
    int m_selectedButtonIndex;
};
