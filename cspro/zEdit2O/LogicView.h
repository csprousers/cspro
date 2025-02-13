#pragma once

#include <zEdit2O/zEdit2O.h>
#include <zEdit2O/ScintillaDocView.h>
#include <zEdit2O/LogicCtrl.h>


class CLASS_DECL_ZEDIT2O CLogicView : public Scintilla::CScintillaView
{
    DECLARE_DYNCREATE(CLogicView)

public:
    CLogicView();
    virtual ~CLogicView();

    CLogicCtrl* GetLogicCtrl() { return assert_cast<CLogicCtrl*>(m_pEdit.get()); }

    BOOL PreTranslateMessage(MSG* pMsg) override;

protected:
    DECLARE_MESSAGE_MAP()

    void OnDraw(CDC* pDC) override;      // overridden to draw this view
    void OnPrepareDC(CDC* pDC, CPrintInfo* pInfo) override;
    //}}AFX_VIRTUAL

    //Scintilla virtual notifications override
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

    // CLogicView methods that can be overriden

    // return true if the context-sensitive help was handled
    virtual bool OnHandleHelp() { return false; }

protected:
    void UpdateMarginWidth(bool force_margin_width_update = false);

private:
    std::optional<int> m_lineCountNumberDigitsAtLastUpdateMarginWidth;
};
