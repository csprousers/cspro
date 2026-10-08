#pragma once

#include <zEditO/zEditO.h>
#include <zEditO/LogicCtrl.h>
#include <zEditO/ScintillaDocView.h>


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
    LogicCtrl* GetLogicCtrl() { return m_logicCtrl; }

protected:
    // LogicView methods that can be overridden

    // Returns the context menu. The submenu at position 0 is displayed.
    // If not specified, the IDR_EDIT menu is used.
    virtual HMENU GetContextMenu();

    // Return true if the context-sensitive help was handled.
    virtual bool OnHandleHelp() { return false; }

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
    void OnZoom(_Inout_ Scintilla::NotificationData* pSCNotification) override;
    void OnDwellStart(_Inout_ Scintilla::NotificationData* pSCNotification) override;
    void OnDwellEnd(_Inout_ Scintilla::NotificationData* pSCNotification) override;
    void OnUpdateUI(_Inout_ Scintilla::NotificationData* pSCNotification) override;
    BOOL OnNotify(WPARAM wParam, LPARAM lParam, LRESULT* pResult) override;

protected:
    void UpdateMarginWidth(bool force_margin_width_update = false);

private:
    LogicCtrl* m_logicCtrl;
    std::optional<int> m_lineCountNumberDigitsAtLastUpdateMarginWidth;
};
