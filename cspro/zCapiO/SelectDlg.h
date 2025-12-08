#pragma once

#include <zCapiO/zCapiO.h>
#include <zCapiO/SelectCtrl.h>

#ifdef WIN_DESKTOP


class CLASS_DECL_ZCAPIO CSelectDlg : public CDialog
{
private:
    void RedoSize( int cx, int cy );
    void BestPos();

    CSelectListCtrlOptions* m_pOptions;

    bool                    m_bMouseMove;
    CPoint                  m_cLastPoint;
    CRect                   m_cLastRect;

// Construction
private:
    friend      CSelectListCtrl;
public:
    void        Init();
    void        Start();

    CSelectDlg(CWnd* pParent = NULL);
    ~CSelectDlg();

    void        InvertMarks();                     // Inverts marks. Do not execute if multi is OFF

    void        GetLastRect( CRect& cLastRect );

// Overrides
public:
    INT_PTR DoModal(CSelectListCtrlOptions* pSelCaseOptions);
    BOOL PreTranslateMessage(MSG* pMsg) override;
protected:
    void DoDataExchange(CDataExchange* pDX) override;    // DDX/DDV support
    BOOL OnNotify(WPARAM wParam, LPARAM lParam, LRESULT* pResult) override;

// Implementation
protected:
    bool                m_bInvert;                          // invert button shown
    bool                m_bSearch;                          // search area shown
    bool                m_bCoordCal;                        // Has calculated rects yet or not?

    CToolTipCtrl*       m_pToolTipsCtrl;                    // Tips control

    CRect               m_ListCtrlRect;                     // List Ctrl orig position
    CRect               m_SearchEdtRect;                    // Search edit box orginal position
    CRect               m_FindBtnRect;                      // Find button original position
    CRect               m_OkBtnRect;                        // Ok button original position
    CRect               m_CancelBtnRect;                    // Cancel button original position
    CRect               m_InverBtnRect;                     // Invert button original position
    CRect               m_OrgWndRect;                       // Window original size
    CButton             m_FindBtn;                          // Find button control
    CButton             m_OkBtn;                            // Ok button control
    CButton             m_CancelBtn;                        // Cancel button control
    CButton             m_InverBtn;                         // Invert button control

    CEdit               m_SearchEdt;                        // Search edit control
    CSelectListCtrl     m_ListCtrl;                         // List ctrl. Linked to CSelectListCtrl class

    int                 m_iExtraHeightAvailable;            // if the invert or search buttons are hidden, there may be extra height at the bottom of the dialog

    void SetRelCoords();
    void SetDefaults();
    void SetMenuBar(BOOL OnOff);
    void ApplyInvertAndSearchSettings();


    // Generated message map functions
    virtual BOOL OnInitDialog();
    afx_msg void OnSearchOnOff();
    afx_msg void OnSize(UINT nType, int cx, int cy);
    virtual void OnOK();
    afx_msg void OnFind();
    afx_msg void OnInvertMarks();
    afx_msg void OnSetfocusSearchBox();
    afx_msg BOOL OnHelpInfo(HELPINFO* pHelpInfo);
    afx_msg void OnKillfocusSearchboxEdit();
    afx_msg LRESULT OnFinishedDialog(WPARAM wParam, LPARAM lParam);
    virtual void OnCancel();
    afx_msg void OnLButtonDown(UINT nFlags, CPoint point);
    afx_msg void OnLButtonUp(UINT nFlags, CPoint point);
    afx_msg void OnMouseMove(UINT nFlags, CPoint point);
    afx_msg void OnMove(int x, int y);
    afx_msg void OnChar(UINT nChar, UINT nRepCnt, UINT nFlags);
    afx_msg void OnKeyDown(UINT nChar, UINT nRepCnt, UINT nFlags);
    afx_msg void OnSetFocus(CWnd* pOldWnd);
    afx_msg void OnActivate(UINT nState, CWnd* pWndOther, BOOL bMinimized);

    DECLARE_MESSAGE_MAP()
};

#endif // WIN_DESKTOP
