#pragma once

#include <zUtilF/zUtilF.h>
#include <zHtml/CSHtmlDlgRunner.h>


class CLASS_DECL_ZUTILF TextInputDlg : public CSHtmlDlgRunner
{
public:
    TextInputDlg();

    void SetTitle(std::wstring title)                { m_title = std::move(title); }
    void SetInitialValue(std::wstring initial_value) { m_initialValue = std::move(initial_value); }

    void SetNumeric(bool numeric)            { m_numeric = numeric; }
    void SetPassword(bool password)          { m_password = password; }
    void SetUppercase(bool uppercase)        { m_uppercase = uppercase; }
    void SetMultiline(bool multiline)        { m_multiline = multiline; }
    void SetRequireInput(bool require_input) { m_requireInput = require_input; }

    const std::wstring& GetTextInput() const { return m_textInput; }

protected:
    const TCHAR* GetDialogName() override;
    std::wstring GetJsonArgumentsText() override;
    void ProcessJsonResults(const JsonNode<wchar_t>& json_results) override;

private:
    std::wstring m_title;
    std::wstring m_initialValue;

    bool m_numeric;
    bool m_password;
    bool m_uppercase;
    bool m_multiline;
    bool m_requireInput;

    std::wstring m_textInput;
};
