#pragma once
// GTCtrl.h : header file
//

/////////////////////////////////////////////////////////////////////////////
// CGenericTreeCtrl window
class CGenericTreeCtrl;

#include <zCaseTreeF/zCaseTreeF.h>
#include <zCaseTreeF/TItmInfo.h>
#include <zCaseTreeF/TreeItem.h>

#include <zCaseTreeF/MsgParam.h>
#include <zCaseTreeF/HITEM.h>

enum class TreeFilterType { FieldNote, None };
typedef void (*FuncPtr) (); //FuncPtr is a pointer to a function taking no args and returning void

 typedef         int     (CGenericTreeCtrl::*pFunction)( int );

#include <zCaseTreeF/Xtctrl.h>


//IMPORTANT : In next version, CGenericTreeCtrl will be renamed to CFlowTCtrl -> the tree ctrl for given flow

class ZCASETREEF_API CGenericTreeCtrl : public CxTreeCtrl
{
// Construction
public:

        CGenericTreeCtrl( CWnd* pParent, bool bCanDestroyItemInfo , int iTypeOfTree, bool bIsToggleView, bool bSingleRoot, bool bCloseOnEscKey, bool bSingleClick, bool bTwoIcons);
        CGenericTreeCtrl();


        bool    HasNotes();
        void    DestroyInfo();

        int     SearchItemBase  (   CDEItemBase*                    pWantedItemBase,    /*input*/
                                    const CArray<int,int>&          aWantedOccs,        /*input*/
                                    CArray<HTREEITEM,HTREEITEM>&    pArray,             /*output*/
                                    CArray<bool,bool>&              aFoundOccs );       /*output*/
        int     SearchItemBase  (CDEItemBase* pWantedItemBase, CArray<HTREEITEM,HTREEITEM>& ahItems);
        int     GetNumTreeOccs  (CDEItemBase* pWantedItemBase);
        bool    IsNoteIcon(int iIconIdx);
        bool    IsToggleView();




        bool IsFiltered();
        void Restore();
        void xUpdateTreeItemInfo( HTREEITEM hInputItem );
        void updateTreeItemInfo();

        //
        bool Filter ( TreeFilterType treeFilterType);
        void Filter ( TreeFilterType treeFilterType, CArray<HTREEITEM,HTREEITEM>* filterItemsArray, bool bMatch=true );
        void xFilter( TreeFilterType treeFilterType, HTREEITEM hInputItem, CArray<HTREEITEM,HTREEITEM>* filterItemsArray, bool bMatch);

        //



        void SetParent( CWnd* pParent, int Type );

        bool HasField(CDEField * pWantedField, int iWantedOccurrence , HTREEITEM hItem);
        bool HasField(CDEField * pWantedField, int iWantedOccurrence , CTreeItem * pRootItem);

        bool HasItemBase(CDEItemBase* pWantedItemBase, int iWantedOcc, HTREEITEM hItem);
        bool HasItemBase(CDEItemBase* pWantedItemBase, HTREEITEM hItem);


        bool SearchField(CDEField * pWantedField, int iWantedOccurrence, HTREEITEM *hOutputItem, int* iIsInRoot);


        HTREEITEM InsertNode( CHITEM* chitem, HTREEITEM hParent);


        virtual ~CGenericTreeCtrl();

// Attributes
public:

// Operations
public:

// Overrides
        // ClassWizard generated virtual function overrides
        //{{AFX_VIRTUAL(CGenericTreeCtrl)
        protected:
        virtual BOOL OnCommand(WPARAM wParam, LPARAM lParam);
        //}}AFX_VIRTUAL


// Implementation
public:


        // Generated message map functions
protected:


        //{{AFX_MSG(CGenericTreeCtrl)
        afx_msg void OnChar(UINT nChar, UINT nRepCnt, UINT nFlags);
        afx_msg void OnClick(NMHDR* pNMHDR, LRESULT* pResult);
        afx_msg void OnSelchanging(NMHDR* pNMHDR, LRESULT* pResult);
        //}}AFX_MSG

        DECLARE_MESSAGE_MAP()

    //VIRTUAL FUNCTIONS
    virtual bool OnBeforeKeyDown    (UINT nChar, UINT nRepCnt, UINT nFlags);
    virtual void OnAfterKeyDown     (UINT nChar, UINT nRepCnt, UINT nFlags);

    virtual bool OnBeforeKeyUp      (UINT nChar, UINT nRepCnt, UINT nFlags);
    virtual void OnAfterKeyUp       (UINT nChar, UINT nRepCnt, UINT nFlags);

    virtual bool OnBeforeLButtonDblClk(UINT nFlags, CPoint point);
    virtual void OnAfterLButtonDblClk (UINT nFlags, CPoint point);
    void DoSelect( UINT nFlags, CPoint point );

protected :

        void UpdateParentWndText(CString csWndText );

private:
        void    InitDefaults();


        void    xSearchItemBase(CDEItemBase* pWantedItemBase, HTREEITEM hItem, CArray<HTREEITEM,HTREEITEM>& ahItems);
        void    xSearchItemBase(/*in*/CDEItemBase* pWantedItemBase, /*in*/const CArray<int,int>& aWantedOccs, /*in*/HTREEITEM hItem, /*out*/CArray<HTREEITEM,HTREEITEM>& ahItems, /*out*/CArray<bool,bool>& aFoundOccs );

        bool    xSearchField(CDEField * pWantedField, int iWantedOccurrence, HTREEITEM hInputItem, HTREEITEM *hOutputItem);








        bool  m_bEnterSwitch;
        int   m_iParentType;


        bool  m_bOnKeyUpByPass;  //cuando está en true ==> no debe escucharse el OnKeyUp, debido a la tecla ESC.

        //int     m_iNumItems;  //FABN ERASE 14/Aug/2002





        bool m_bIsToggleView;



        CString                         m_csRootLabel;
        CArray<CTreeItemInfo*, CTreeItemInfo*> m_TreeItemInfoArray;




        bool                                                                    m_bFiltrate;


};

/////////////////////////////////////////////////////////////////////////////

//{{AFX_INSERT_LOCATION}}
// Microsoft Visual C++ will insert additional declarations immediately before the previous line.
