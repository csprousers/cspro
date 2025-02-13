#pragma once

#include <CSCode/CodeView.h>
#include <zUtilO/SettingsDb.h>


class HtmlDialogCodeView : public CodeView
{
    DECLARE_DYNCREATE(HtmlDialogCodeView)

protected:
    HtmlDialogCodeView(); // create from serialization only

public:
    const LanguageSettings& GetLanguageSettings() const override { return m_languageSettings; }

    std::variant<const CDocument*, std::wstring> GetDocumentOrTitleForBuildWnd() const override;

protected:
    DECLARE_MESSAGE_MAP()

    void OnInitialUpdate() override;

    void OnDestroy();

private:
    std::wstring GetInitialText();

private:
    LanguageSettings m_languageSettings;
    SettingsDb m_settingsDb;
};
