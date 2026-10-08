#pragma once

#include <zEditO/zEditO.h>
#include <zEditO/LogicCtrl.h>
#include <zEditO/ScintillaDocView.h>


// --------------------------------------------------------------------------
// LogicView
//
// LogicView hosts a Scintilla control in a view.
// --------------------------------------------------------------------------

class CLASS_DECL_ZEDITO LogicView : public Scintilla::CScintillaView
{
    DECLARE_DYNCREATE(LogicView)

protected:
    LogicView();

public:
    LogicCtrl* GetLogicCtrl() { return m_logicCtrl; }

    BOOL PreTranslateMessage(MSG* pMsg) override;

protected:
    DECLARE_MESSAGE_MAP()

    void OnDraw(CDC* pDC) override;      // overridden to draw this view
    void OnPrepareDC(CDC* pDC, CPrintInfo* pInfo) override;

    std::unique_ptr<Scintilla::CScintillaCtrl> CreateScintillaControl() override;

    // Scintilla virtual notifications override
    void OnZoom(_Inout_ Scintilla::NotificationData* pSCNotification) override;
    void OnDwellStart(_Inout_ Scintilla::NotificationData* pSCNotification) override;
    void OnDwellEnd(_Inout_ Scintilla::NotificationData* pSCNotification) override;
    void OnUpdateUI(_Inout_ Scintilla::NotificationData* pSCNotification) override;
    BOOL OnNotify(WPARAM wParam, LPARAM lParam, LRESULT* pResult) override;

    // Generated message map functions
    afx_msg int OnCreate(LPCREATESTRUCT lpCreateStruct);
    afx_msg void OnSize(UINT nType, int cx, int cy);
    afx_msg void OnHelp();
    afx_msg LRESULT OnFindReplaceCmd(WPARAM, LPARAM lParam);

    // LogicView methods that can be overridden

    // return true if the context-sensitive help was handled
    virtual bool OnHandleHelp() { return false; }

protected:
    void UpdateMarginWidth(bool force_margin_width_update = false);

private:
    LogicCtrl* m_logicCtrl;
    std::optional<int> m_lineCountNumberDigitsAtLastUpdateMarginWidth;
};
