#pragma once
// CaseTree.h : header file
//

/////////////////////////////////////////////////////////////////////////////
// CCaseTree window

#include <zCaseTreeF/zCaseTreeF.h>
#include <zCaseTreeF/GTCtrl.h>

class CCapiRunAplEntry;
class CHITEM;
class InsertItemConfig;
class CDEField;


class ZCASETREEF_API CCaseTree : public CWnd
{
// Construction
public:

    //      bCloseOnEscKey : true => The tree is closed by the Esc key even when there is only one tree layer, and
    //                                                       a PostMessage is sent to the container usefull to delete the pointer.

    CCaseTree(  int                 iTypeOfParentWindow,
                int                 iTypeOfTree,
                bool                bIsInToggleView,
                CDEField*           pWantedField,
                int                 iWantedOcc,
                CWinApp*            pWinApp,
                CCapiRunAplEntry*   pCapiRunAplEntry,
                CWnd*               pParent);

    virtual ~CCaseTree();

    void    Show(int xo, int yo, int x1, int y1);
    bool    IsEmpty();
    void    CloseDialog();
    void    GetWindowRect(LPRECT lpRect ) const;
    void    MoveWindow( int x, int y, int nWidth, int nHeight, BOOL bRepaint = TRUE );
    void    MoveWindow( LPCRECT lpRect, BOOL bRepaint = TRUE );
    CWnd*   SetFocus();
    CWnd*   GetWnd();   //Don't store !!!! ...After any refresh in the tree, GetWnd can return any diferent thing!

    void    ChangeView();
private:

    /*used to set the first time value of m_iLastOpenNodeIdx*/
    int         FirstSetOfLastNodeIdx(int iCurLastOpenNodeIdx = -1);

    bool        RemoveItem( CDEItemBase* pItem, int iOcc, int iLayerIdx, HTREEITEM* hInsertAfter, HTREEITEM* hParent );
    HTREEITEM   InsertItemOccsTittle( CDEItemBase* pItem, int iNumOccsInTheTittle, CGenericTreeCtrl* pTree, HTREEITEM hInsertAfter, HTREEITEM hParent);
    bool        DestroyAndCreateAllTreeLayers(CDEField* pWantedField, int iWantedOcc, CString csWantedKey);

    void        ShowTree(int xo, int yo, int x1, int y1);
    void        UserSelectItem(HTREEITEM hSelectedItem, CPoint ptScrollPos, bool bVisible);
    bool        ShowTreeOfNodes(bool bDisplayNodeChilds = false, CDEField* pWantedFiel = NULL, int iWantedOcc = -1 );

    bool        AddTreeLayer(   int         iTypeOfTree,
                                bool        bFirstCall,
                                HTREEITEM   hItem,
                                CString     csSelectedKey, /*por si la llave del elemento que debe quedar seleccionado es conocida.*/
                                bool        bVisible          /*true => lo crea y lo muestra.*/,
                                CDEField*   pWantedField,
                                int         iWantedOcc);
    void        LoadIcons();
    bool        xInsertItems(CArray<CHITEM*,CHITEM*>* pInOrderArray, CString csRootItemLabel, CDEField* pWantedField, int iWantedOcc);
    bool        InsertNodeVars(int iNodeIdx, CGenericTreeCtrl* pTree, HTREEITEM hItem, CDEField* pWantedField, int iWantedOcc);

    //TO INSERT
    bool        InsertLevel         (CString csParentKey, int iNodeIdx, CDELevel*    pLevel, HTREEITEM hParentItemOfLevel, int iLevelIndex, CDEField* pWantedField, int iWantedOcc, bool bParentItemReplaceRootLevel);
    bool        InsertMultiOccItem  (int iNodeIdx, CDEItemBase* pItem , int iItemIndex, int iItemOcc, HTREEITEM hItemParent, HTREEITEM hInsertAfter, CDEField* pWantedField, int iWantedOcc, int iMaxItemLength/* = 0*/, bool bSpecialInsert, bool bOccIcon);
    bool        InsertItemChilds    (int iNodeIdx, CDEItemBase* pItem , int iItemIndex, int iItemOcc, HTREEITEM hItem, CString csItemKey, CDEField* pWantedField, int iWantedOcc );
    bool        DoStackCalls();

    //
    bool        RefreshItemBase(CArray<CDEItemBase*,CDEItemBase*>* pItemBaseToRefreshArray,     //Items to be updated
                                CArray<CArray<int,int>*,CArray<int,int>*>* pOccsToRefreshArray, //Array with occs to refresh, for each item in pItemBaseToRefreshArray
                                CDEField* pWantedField, int iWantedOcc) ;                       //this is where the selected HTREEITEM will be after the whole process

    bool        ReCreateTreeItem(   CGenericTreeCtrl*           pTree,
                                    /*CDEItemBase*              pItemBaseToRefresh,
                                    int                         iWantedOccToRefresh,*/
                                    HTREEITEM                   hItem,
                                    CDEField*                   pWantedField,
                                    int                         iWantedOcc,
                                    //bool                      bAddItem,
                                    //HTREEITEM                 hGivenParent,
                                    bool                        bOccIcon);


    bool        NeedToPutInStack(  CDEItemBase*    pItem,              //item que estamos insertando
                                   HTREEITEM       hParentItem,        //nodo del árbol desde donde cuelga este item
                                   CDEItemBase*    pLastInsertedItem,
                                   HTREEITEM       hLastParentItem );

    void        InitTree(bool bIsToggle );

    bool        SelectCorrectItem( CDEField* pWantedField, int iWantedOcc,  CString csWantedKey, bool bExpand=false );


    void        updateWindowText();
    int         GetNumTreeLayers();
    void        xClose(bool bRefresh, bool bWantCloseDialog );              //FABN 16/Aug/2002
    void        Clean();

    bool        Refresh(HTREEITEM                                   hItem,
                        CString                                     csLastSelectedKey,
                        CDEField*                                   pWantedField,
                        int                                         iWantedField,
                        CArray<CDEItemBase*,CDEItemBase*>*          pItemBaseToRefreshArray,
                        CArray<CArray<int,int>*,CArray<int,int>*>*  pOccsToRefreshArray );


    int         getNumToggleTreeLayers();
    bool        xRefresh( HTREEITEM hItem , CDEField* pWantedField, int iWantedOcc, CString csWantedKey);   //FABN 21/Aug/2002
    void        UserSelectItem(CString csItemKey, bool bVisible);   //FABN 22/Aug/2002

    void        InitFonts();
    void        RefreshTreeFont();
    void        Restore();
    void        CloseCurrentTreeLayer();
    bool        RemoveFromArray( CGenericTreeCtrl* pTreeCtrl );
    int         SearchTree( CGenericTreeCtrl* pTreeCtrl );
    bool        SearchField( CDEField* pWantedField,int iWantedOcc, int* iTreeLayerIdx, HTREEITEM* hSelItem, int* iIsInRoot );
    bool        SearchField( CString csKey, int* iTreeLayerIdx, HTREEITEM* hSelItem, int* iIsInRoot );

    bool        IsNode(CGenericTreeCtrl* pTreeLayer, CString csItemKey);
    int         GetNonSelectedIconIdx(CGenericTreeCtrl* pTreeLayer, CString csItemKey);

    bool        RefreshTittleOfOccs(CGenericTreeCtrl* pTree, HTREEITEM hItem);

    //NODE LABELS REFRESH
    bool        RefreshNodeLabels();
    bool        RefreshNodeLabel(int iNodeIdx);

    //SOME BEHAVIOUR FUNCTIONS (in the near future, all behaviour will be inside a little class)
    bool        IsItemSelectable(CGenericTreeCtrl* pTree, HTREEITEM hItem);

// Operations
public:

    // Overrides
    // ClassWizard generated virtual function overrides
    //{{AFX_VIRTUAL(CCaseTree)
    //}}AFX_VIRTUAL

    // Generated message map functions
protected:
    //{{AFX_MSG(CCaseTree)
    //}}AFX_MSG
    DECLARE_MESSAGE_MAP()

    LRESULT OnSelectTreeItem        (WPARAM wParam, LPARAM lParam);
    LRESULT OnUpdateWindowText      (WPARAM wParam, LPARAM lParam);
    LRESULT OnCloseAllTreeLayers    (WPARAM wParam, LPARAM lParam);
    LRESULT OnRestoreTree           (WPARAM wParam, LPARAM lParam);
    LRESULT OnCloseCurrentTreeLayer (WPARAM wParam, LPARAM lParam);
    LRESULT OnRefresh               (WPARAM wParam, LPARAM lParam);
    LRESULT OnUnknownKey            (WPARAM wParam, LPARAM lParam);
    LRESULT OnShowWindow            (WPARAM wParam, LPARAM lParam);

private:
    CCapiRunAplEntry*                               m_pCapiRunAplEntry;
    CGenericTreeCtrl*                               m_pTree;                    //Pointer to the active tree layer
    CTreeCtrl*                                      m_pWhiteBackGround;
    CArray< CGenericTreeCtrl*, CGenericTreeCtrl* >* m_pTreeArray;
    CArray<InsertItemConfig*,InsertItemConfig*>     m_InsertItemConfigArray;
    CDEItemBase*                                    m_pLastInsertedItem;
    int                                             m_iTypeOfTree;
    bool                                            m_bLockedStack;
    CDEField*                                       m_pValidField;
    int                                             m_iValidOcc;
    HTREEITEM                                       m_pLastHITEMParent;
    CWnd*                                           m_pParent;
    CWinApp*                                        m_pWinApp;
    int                                             m_iTypeOfParentWindow;
    int                                             m_iToggleTreeLayers;
    bool                                            m_bIsInToggleView;
    CImageList*                                     m_pImageList;
    bool                                            m_bShowFinalExitApprobation;
    CFont*                                          m_pTreeFont;
    CFont*                                          m_pButtonsFont;
    bool                                            m_bIsClean;
    int                                             m_iNumberOfTogglePressed;
    bool                                            m_bFinishCase;
    CString                                         m_CSMessage;
    int                                             m_iCurrentNumNodes;
    CDEItemBase*                                    m_pRootItem;
    bool                                            m_bFullTreeOfQuestionsAnswers;

    //For store the last wanted position
    CDEField*                                       m_pWantedField;
    int                                             m_iWantedOcc;
    CString                                         m_csWantedKey;

    CArray<CString,CString>*                        m_pShape;

    TreeFilterType                                  m_treeFilterType;
    CDEField*                                       m_pLastSelectedField;
    int                                             m_iLastSelectedOcc;
    bool                                            m_bRecursiveClose;
    CDEField*                                       m_pLastDialogField;
    int                                             m_iLastDialogOcc;
    bool                                            m_bRefreshLocked;
    int                                             m_ixo,m_iyo,m_ix1,m_iy1;
    bool                                            m_bVerifyFlag;
    bool                                            m_bFilteredTree;
    int                                             m_iLastOpenNodeIdx;
    CPoint                                          m_ptLastScrollPos;
    bool                                            m_bShowNames;
};

/////////////////////////////////////////////////////////////////////////////

//{{AFX_INSERT_LOCATION}}
// Microsoft Visual C++ will insert additional declarations immediately before the previous line.
