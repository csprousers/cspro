#pragma once

#include <zEditO/LogicView.h>

class CodeDoc;


class CodeView : public LogicView
{
    DECLARE_DYNCREATE(CodeView)

protected:
    CodeView() noexcept = default; // create from serialization only

public:
    const CodeDoc& GetCodeDoc() const { return assert_cast<const CodeDoc&>(*GetDocument()); }
    CodeDoc& GetCodeDoc()             { return assert_cast<CodeDoc&>(*GetDocument()); }

    virtual const LanguageSettings& GetLanguageSettings() const { return GetCodeDoc().GetLanguageSettings(); }

    virtual std::variant<const CDocument*, std::string> GetDocumentOrTitleForBuildWnd() const;

    void RefreshLogicControlLexer();

protected:
    DECLARE_MESSAGE_MAP()

    void OnInitialUpdate() override;
    void OnInitialUpdateWorker(cs::string_sz initial_text);

    // CScintillaView overrides
    // --------------------------------------------------------------------------
    void OnSavePointReached(_Inout_ Scintilla::NotificationData* pSCNotification) override;
    void OnSavePointLeft(_Inout_ Scintilla::NotificationData* pSCNotification) override;

    // Run menu handlers
    // --------------------------------------------------------------------------
    void OnRunCompileOrValidate();
    void OnUpdateRunCompileOrValidate(CCmdUI* pCmdUI);

    void OnRunValidateJsonOnly();
    void OnUpdateRunValidateJsonOnly(CCmdUI* pCmdUI);

    void OnRunFormatJson(UINT nID);
    void OnUpdateRunFormatJson(CCmdUI* pCmdUI);

    void OnRunSpecFileDowngrade();
    void OnUpdateRunSpecFileDowngrade(CCmdUI* pCmdUI);
};
