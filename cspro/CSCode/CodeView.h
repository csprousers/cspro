#pragma once

#include <zEdit2O/LogicView.h>

class CodeDoc;


class CodeView : public CLogicView
{
    DECLARE_DYNCREATE(CodeView)

protected:
    CodeView(); // create from serialization only

public:
    ~CodeView();

    const CodeDoc& GetCodeDoc() const { return assert_cast<const CodeDoc&>(*GetDocument()); }
    CodeDoc& GetCodeDoc()             { return assert_cast<CodeDoc&>(*GetDocument()); }

    virtual const LanguageSettings& GetLanguageSettings() const { return GetCodeDoc().GetLanguageSettings(); }

    virtual std::variant<const CDocument*, std::wstring> GetDocumentOrTitleForBuildWnd() const;

    void SetTextAndSetSavePoint(const std::wstring& text);

    void RefreshLogicControlLexer();

protected:
    DECLARE_MESSAGE_MAP()

    void OnInitialUpdate() override;
    void OnInitialUpdateWorker(const std::wstring& initial_text);

    // CScintillaView overrides
    // --------------------------------------------------------------------------
    void OnSavePointReached(_Inout_ Scintilla::NotificationData* pSCNotification) override;
    void OnSavePointLeft(_Inout_ Scintilla::NotificationData* pSCNotification) override;

    // editor handlers
    // --------------------------------------------------------------------------
    void OnFindNext();

    void OnCommentCode();

    // Code menu handlers
    // --------------------------------------------------------------------------
    void OnPasteStringLiteral();
    void OnUpdatePasteStringLiteral(CCmdUI* pCmdUI);

    void OnCodeFoldingLevel(UINT nID);
    void OnUpdateCodeFoldingLevel(CCmdUI* pCmdUI);
    void OnCodeFoldingAction(UINT nID);
    void OnUpdateCodeFoldingAction(CCmdUI* pCmdUI);

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
