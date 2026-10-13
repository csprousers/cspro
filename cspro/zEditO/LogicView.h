#pragma once

#include <zEditO/zEditO.h>
#include <zEditO/LogicCtrl.h>
#include <zEditO/ScintillaDocView.h>

class TextSourceEditable;


// --------------------------------------------------------------------------
// LogicView
//
// LogicView hosts a Scintilla control in a view.
//
// Default functionality includes:
//
// - Showing a context menu. Overridable using GetContextMenu().
// --------------------------------------------------------------------------

class CLASS_DECL_ZEDITO LogicView : public Scintilla::CScintillaView
{
    DECLARE_DYNCREATE(LogicView)

protected:
    LogicView();

public:
    ~LogicView() noexcept;

    LogicCtrl* GetLogicCtrl() noexcept { return m_logicCtrl; }

    // Adds an object to the editable text source that manages the modification flag
    // and keeps the text source's buffer in sync.
    void RegisterTextSourceModifier(std::shared_ptr<TextSourceEditable> text_source);

    // Processes the code folding setting (set via the Code menu).
    // If fold_procs is set, the fold.procs property will be sent to the lexer.
    void ProcessCodeFolding(std::optional<bool> fold_procs = std::nullopt);

protected:
    // LogicView methods that can be overridden, or used by subclasses.

    // Returns the context menu. The submenu at position 0 is displayed.
    // If not specified, the IDR_EDIT menu is used.
    virtual HMENU GetContextMenu();

    // Returns true if the text being edited supports single-line comments
    // triggered using the shortcut Ctrl + /. The default implementation
    // uses the lexer language to determine if this is supported
    virtual bool CanCommentLine();

    // Return true if the context-sensitive help was handled.
    virtual bool OnHandleHelp() { return false; }

    // Updates the margin width based on the line count.
    void UpdateMarginWidth(bool force_margin_width_update = false);

protected:
    DECLARE_MESSAGE_MAP()

    int OnCreate(LPCREATESTRUCT lpCreateStruct);
    void OnDraw(CDC* pDC) override;
    void OnContextMenu(CWnd* pWnd, CPoint point);

    BOOL PreTranslateMessage(MSG* pMsg) override;

    LRESULT OnFindReplaceCmd(WPARAM, LPARAM lParam);

    void OnPasteStringLiteral();
    void OnUpdatePasteStringLiteral(CCmdUI* pCmdUI);

    void OnCommentLine();

    void OnDeprecationWarnings(UINT nID);
    void OnUpdateDeprecationWarnings(CCmdUI* pCmdUI);

    void OnCodeFoldingLevel(UINT nID);
    void OnUpdateCodeFoldingLevel(CCmdUI* pCmdUI);
    void OnCodeFoldingAction(UINT nID);
    void OnUpdateCodeFoldingAction(CCmdUI* pCmdUI);

    void OnHelp();

private:
    std::unique_ptr<Scintilla::CScintillaCtrl> CreateScintillaControl() override;

    // Scintilla virtual notifications override
    void OnSavePointReached(Scintilla::NotificationData* pSCNotification) override;
    void OnModified(Scintilla::NotificationData* pSCNotification) override;
    void OnZoom(Scintilla::NotificationData* pSCNotification) override;
    void OnDwellStart(Scintilla::NotificationData* pSCNotification) override;
    void OnDwellEnd(Scintilla::NotificationData* pSCNotification) override;
    void OnUpdateUI(Scintilla::NotificationData* pSCNotification) override;
    void OnAutoCSelection(Scintilla::NotificationData* pSCNotification) override;
    void OnAutoCCancelled(Scintilla::NotificationData* pSCNotification) override;

    void UpdateBraceMatchingUI();

protected:
    LogicCtrl* m_logicCtrl; // non-null after creation

private:
    class TextSourceModifier;

    std::unique_ptr<TextSourceModifier> m_textSourceModifier;
    std::optional<int> m_lineCountNumberDigitsAtLastUpdateMarginWidth;
    std::optional<bool> m_codeFoldingEnabled;
};
