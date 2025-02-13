#pragma once

#include <zEngineF/zEngineF.h>
#include <zHtml/CSHtmlDlgRunner.h>

constexpr const TCHAR* ErrmsgDialogName = _T("errmsg");


class CLASS_DECL_ZENGINEF ErrmsgDlg : public CSHtmlDlgRunner
{
public:
    ErrmsgDlg();

    void SetTitle(std::wstring title) { m_title = std::move(title); }

    void SetMessage(std::wstring message) { m_message = std::move(message); }

    void SetButtons(std::vector<std::wstring> buttons) { m_buttons = std::move(buttons); }

    void SetDefaultButtonIndex(int default_button_index) { m_defaultButtonIndex = default_button_index; }

    int GetSelectedButtonIndex() const { return m_selectedButtonIndex; }

protected:
    const TCHAR* GetDialogName() override;
    std::wstring GetJsonArgumentsText() override;
    void ProcessJsonResults(const JsonNode<wchar_t>& json_results) override;

private:
    std::wstring m_title;
    std::wstring m_message;
    std::vector<std::wstring> m_buttons;
    int m_defaultButtonIndex;
    int m_selectedButtonIndex;
};
