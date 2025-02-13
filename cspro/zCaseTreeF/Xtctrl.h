#pragma once
/////////////////////////////////////////////////////////////////////////////
// CxTreeCtrl window

#include <zCaseTreeF/zCaseTreeF.h>
#include <zCaseTreeF/TItmInfo.h>
#include <zCaseTreeF/TreeItem.h>
#include <iostream>
#include <fstream>


class ZCASETREEF_API CxTreeCtrl : public CTreeCtrl
{
// Construction
public:
        CxTreeCtrl();

// Attributes
public:

// Operations
public:

// Overrides
        // ClassWizard generated virtual function overrides
        //{{AFX_VIRTUAL(CxTreeCtrl)
        //}}AFX_VIRTUAL

// Implementation
public:
    virtual ~CxTreeCtrl() { }

    //INSERT FUNCTIONS
    HTREEITEM Insert(CString CIMSA_Label, HTREEITEM hGenericTreeParentItem, HTREEITEM hInsertAfter,
                     int iNonSelectedIconIndex, int iSelectedIconIndex);

    //COPY FUNCTIONS
    void    CopyFrom(CxTreeCtrl * pBaseTree, HTREEITEM hItem);
    void    RecursiveCopy( HTREEITEM hItem, HTREEITEM hParent, CxTreeCtrl * pBaseTree );

    //KEY FUNCTIONS
    CString GetKey(HTREEITEM hItem);
    bool    SaveAllKeysTo(CArray<CString,CString> * pArray);
    bool    HasKey( const CString& csItemKey, HTREEITEM  hItem );
    bool    HasKey( const CString& csItemKey, CTreeItem* hItem );


    //SEARCH FUNCTIONS
    bool    SearchItem ( CString& csItemKey, HTREEITEM *hOutputItem, int* iIsInRoot );

    //SHAPE FUNCTIONS
    void    Collapse();
    bool    LoadShape();
    bool    LoadShapeFrom(CArray<CString,CString> * pArray );
    bool    SaveShapeTo  (CArray<CString,CString> * pArray );
    bool    SaveShape();
    int     GetNumExpandedItems();
    CString GetShape(CString csKeySeparator);

    //SCROLL FUNCTIONS
    void    xSetScrollPos( int iHScrollPos, int iVScrollPos );

    //DESTROY FUNCTIONS
    void    DestroyItemInfo(HTREEITEM hItem, bool bRecursiveChilds);
    void    DeleteChilds( HTREEITEM hItem );

    //OTHER
    bool    HasInfo(CTreeItemInfo* pWantedInfo, HTREEITEM hItem);
    bool    HasInfo(CTreeItemInfo* pWantedInfo, CTreeItem * hItem);
    int     GetTypeOfTree();
    bool    IsVisible(HTREEITEM hItem);

    //DEPRECATED
    //ROOT NOT IN THE CTRL SUPPORT
    void            SetXRootItem( CTreeItem * pRootItem );
    CTreeItem *     GetXRootItem();

    //EVENTS
    int     OnToolHitTest(CPoint point, TOOLINFO * pTI) const;

    //VIRTUAL FUNCTIONS
    virtual bool OnBeforeSetFocus       (CWnd* pOldWnd);
    virtual void OnAfterSetFocus        (CWnd* pOldWnd);

    virtual bool OnBeforeKillFocus      (CWnd* pNewWnd);
    virtual void OnAfterKillFocus       (CWnd* pNewWnd);

    virtual bool OnBeforeKeyDown        (UINT nChar, UINT nRepCnt, UINT nFlags);
    virtual void OnAfterKeyDown         (UINT nChar, UINT nRepCnt, UINT nFlags);

    virtual bool OnBeforeKeyUp          (UINT nChar, UINT nRepCnt, UINT nFlags);
    virtual void OnAfterKeyUp           (UINT nChar, UINT nRepCnt, UINT nFlags);

    virtual bool OnBeforeLButtonDown    (UINT nFlags, CPoint point);
    virtual void OnAfterLButtonDown     (UINT nFlags, CPoint point);

    virtual bool OnBeforeLButtonDblClk  (UINT nFlags, CPoint point);
    virtual void OnAfterLButtonDblClk   (UINT nFlags, CPoint point);

    virtual bool OnBeforeRButtonDown    (UINT nFlags, CPoint point);
    virtual void OnAfterRButtonDown     (UINT nFlags, CPoint point);

    virtual void OnAfterItemExpanded    (NMHDR* pNMHDR, LRESULT* pResult);

        // Generated message map functions
protected:
        //{{AFX_MSG(CxTreeCtrl)
    afx_msg void OnSetFocus(CWnd* pOldWnd);
    afx_msg void OnKillFocus( CWnd* pNewWnd );
    afx_msg void OnLButtonDblClk(UINT nFlags, CPoint point);
    afx_msg void OnLButtonDown(UINT nFlags, CPoint point);
    afx_msg void OnRButtonDown(UINT nFlags, CPoint point);
    afx_msg void OnKeyDown(UINT nChar, UINT nRepCnt, UINT nFlags);
    afx_msg void OnKeyUp( UINT nChar, UINT nRepCnt, UINT nFlags );
    afx_msg void OnShowWindow(BOOL bShow, UINT nStatus);
    afx_msg UINT OnGetDlgCode();
    afx_msg void OnItemexpanded(NMHDR* pNMHDR, LRESULT* pResult);
        //}}AFX_MSG
        DECLARE_MESSAGE_MAP()

protected :
    CTreeItem     * m_pRootItem;
    CTreeItemInfo * m_pRootItemInfo;

    CWnd*   m_pParent;
    bool    m_bSingleRoot;
    int     m_iTypeOfTree;
    bool    m_bCloseOnEscKey;
    bool    m_bCanDestroyItemInfo;
    bool    m_bSingleClick;
    bool    m_bTwoIcons;

    int     m_ixo, m_iyo, m_ix1, m_iy1;



protected:
    //SHAPE
    void    xExpand( HTREEITEM hItem, bool bExpand = true);
    void    xCollapse(HTREEITEM hItem);
    bool    xLoadShape(HTREEITEM hItem);

    //KEYS
    bool    xLoadShapeFrom(CArray<CString,CString>* pArray, HTREEITEM hItem);
    bool    xSaveAllKeysTo(CArray<CString,CString> * pArray, HTREEITEM hItem);
    bool    xSaveShapeTo(CArray<CString,CString> * pArray, HTREEITEM hItem );
    bool    xSaveShape ( HTREEITEM hItem );

    //SEARCH
    bool    xSearchItem( const CString& csItemKey, HTREEITEM hInputItem, HTREEITEM *hOutputItem);
};

/////////////////////////////////////////////////////////////////////////////

//{{AFX_INSERT_LOCATION}}
// Microsoft Visual C++ will insert additional declarations immediately before the previous line.
