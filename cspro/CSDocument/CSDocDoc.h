#pragma once

#include <CSDocument/TextEditDoc.h>


class CSDocDoc : public TextEditDoc
{
    DECLARE_DYNCREATE(CSDocDoc)

protected:
    CSDocDoc(); // create from serialization only

public:
    // TextEditDoc overrides
    int GetLexerLanguage() const override { return SCLEX_CSPRO_DOCUMENT; }

    bool ConvertTabsToSpacesAndTrimRightEachLine() const override { return true; }

protected:
    BOOL OnOpenDocument(LPCTSTR lpszPathName) override;
    void OnCloseDocument() override;

private:
    void AutomaticallyAssociateWithDocSet(const std::wstring& csdoc_filename);

private:
    SettingsDb m_settingsDb;
};
