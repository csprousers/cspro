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

protected:
    // LogicView methods that can be overridden, or used by subclasses.

    // Returns the context menu. The submenu at position 0 is displayed.
    // If not specified, the IDR_EDIT menu is used.
    virtual HMENU GetContextMenu();

    // Return true if the context-sensitive help was handled.
    virtual bool OnHandleHelp() { return false; }

    // Updates the margin width based on the line count.
    void UpdateMarginWidth(bool force_margin_width_update = false);

protected:
    DECLARE_MESSAGE_MAP()

    int OnCreate(LPCREATESTRUCT lpCreateStruct);
    void OnSize(UINT nType, int cx, int cy);
    void OnDraw(CDC* pDC) override;
    void OnContextMenu(CWnd* pWnd, CPoint point);

    BOOL PreTranslateMessage(MSG* pMsg) override;

    void OnHelp();

    LRESULT OnFindReplaceCmd(WPARAM, LPARAM lParam);

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

private:
    LogicCtrl* m_logicCtrl;
    std::optional<int> m_lineCountNumberDigitsAtLastUpdateMarginWidth;

    class TextSourceModifier;
    std::unique_ptr<TextSourceModifier> m_textSourceModifier;
};
