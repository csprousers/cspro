#pragma once
//***************************************************************************
//  File name: FrmTrCtl.h
//
//  Description:
//       Forms tree control definitions
//
//  History:    Date       Author   Comment
//              ---------------------------
//              11 Feb 99   gsf     Created for Measure 1.0
//
//***************************************************************************

#include <zformf/zFormF.h>

class CDDTreeCtrl;
class CFormNodeID;

/////////////////////////////////////////////////////////////////////////////

class CLASS_DECL_ZFORMF CFormTreeCtrl : public CTreeCtrl
{
private:

DECLARE_DYNCREATE (CFormTreeCtrl)

    CWnd*           m_pDropWnd;
    CImageList*     m_pDragImage;   // the current image being dragged (i think)
    CImageList      m_cImageList;   // where my icons will be stored during registration

    bool            m_bDragging;    // am i dragging?

    bool            m_bOkToRedraw;  // contrary to what i'd like, SetRedraw() doesn't cut it,
                                    // nd2keep track of whether it's ok to refresh or not,
                                    // specifically, for OnSelchanged()

    HTREEITEM       m_hDragItem;
    HTREEITEM       m_hDropItem;
    HTREEITEM       m_hRestoreItemSelection; //restore item selection after right click context menu is processed

    int             m_nDropIndex;
    DWORD           m_dwDragStart;

    CDocTemplate*   m_pDocTemplate;
    CFormDoc*       m_pFormDoc;     // a ptr to the doc that owns me

    CDDTreeCtrl*    m_pDDTreeCtrl;  // ptr to the tree ctrl containing our dict

    UINT            m_mask;

    CRect           m_dragStartBounds;

    COleDropTarget  m_dropTarget;   // nd for registering the tree ctrl as a recipient of a drop op

    BOOL            m_bSendMsg;     //Flag for sending messages to the main window
    static LOGFONT      m_DefLogFont;
    static CFont        m_font;

public:
    void SwitchRefresh();
    void RestoreItemSelection();
    CFormTreeCtrl();
    virtual ~CFormTreeCtrl();

//  ****************
//  this blk of funcs support multi-selection in the tree control

    void ClearSelection();
    bool IsSelected(HTREEITEM hItem);
    void AddToSelection(HTREEITEM hItem);
    void RemoveFromSelection(HTREEITEM hItem);
    std::vector<HTREEITEM> GetSelected();
    std::vector<CFormID*> GetSelectedFormItems();
    void SelectRange(HTREEITEM hItemFrom, HTREEITEM hItemTo);
    HTREEITEM GetNextItemRec(HTREEITEM hItem);
    bool CanBePartOfMultipleSelection(HTREEITEM hItem);
    void NotifyViewSelectionChanged();
    bool ShouldStartDragging(CPoint p);

    BOOL GetSndMsgFlg() const {return m_bSendMsg;}
    void SetSndMsgFlg(BOOL bFlag) {m_bSendMsg = bFlag;}
//  ****************

    HTREEITEM InsertFormFile(const CString& sLabel , const CString& sFormFileName ,CFormDoc* pDoc);

    void SetDocTemplate (CDocTemplate* pTemplate) { m_pDocTemplate = pTemplate; }

    void             SetFormDoc     (CFormDoc* pDoc) { m_pFormDoc = pDoc; }
    CFormDoc*        GetFormDoc     (void)           { return  m_pFormDoc; }
    CFormScrollView* GetFormView    (void);


    CDDTreeCtrl*    GetDDTreeCtrl   (void) const { return m_pDDTreeCtrl; }
    void SetDDTreeCtrl  (CDDTreeCtrl*   pTreeCtrl)  { m_pDDTreeCtrl = pTreeCtrl; }

    void SetDragging (bool b)       { m_bDragging = b; }
    bool Dragging    (void) const   { return  m_bDragging; }
    void BeginDrag(CPoint point);

    void InitImageList(void);
    void BuildTree(CFormNodeID* pNodeID, bool bRedraw=true);
    void ReBuildTree(int iFormNum=0, CDEFormBase* pItem=NULL);  // default to display form 1 (0-based)
    void AddForm(CFormDoc* pFormDoc, CDEGroup* pGroup, HTREEITEM hParent, int iLevel);
    void AddGroupItems(CFormDoc* pFormDoc, CDEGroup* pGroup, HTREEITEM hParent, int iLevel);
    void AddBaseItem(CFormDoc* pFormDoc, CDEItemBase* pItem, HTREEITEM hParent, int iLevel);
    void AddBlock(CFormDoc* pFormDoc, CDEBlock& form_block, HTREEITEM hParent, int iLevel);

    void DefaultExpand  (HTREEITEM hItem);
    void SelectFTCNode(CFormID* pFormID, int iFormNum, CDEFormBase* pSearchItem);
    void SelectNodes(std::vector<CDEFormBase*> nodes);
    void FindFTCRecursively (HTREEITEM& hGroupOrItem,
                             HTREEITEM& hExpand,
                             int iFormNum,
                             CDEFormBase* pSearchItem);

    void InsertExternalCodeAndReportNodes(const CFormNodeID* pFormNodeId);

    CFormNodeID* GetFormNode (CFormDoc* pDoc) const ;
    CFormNodeID* GetFormNode (wstring_view filename) const;

    void DeleteActiveItem   (HTREEITEM );

    BOOL OpenFormFile       (const CString &sFormFile ,BOOL bMakeVisible = TRUE);

    void RemoveFormIDs      (HTREEITEM hItem);
    void Clean              (void);
    void ReleaseFormNodeID  (CFormNodeID*& pNodeID);
    void ReleaseDoc         (CFormNodeID* pNodeID);
    bool InsertFormDependencies (CFormNodeID* pID);
    bool ReleaseFormDependencies(CFormNodeID* pID);


    void DropFormTreeItem   (std::vector<CFormID*> pDragFromIDs, CPoint dropPoint);
    void DropItemOnItem     (std::vector<CFormID*> dragFromIDs, CFormID* pDropOnID);
    void DropFormOnForm     (CFormID* pDragFromID, CFormID* pDropOnID);
    void DropRosterItemOnRI (std::vector<CFormID*> dragFromIDs, CFormID* pDropOnID);
    void DropRosterOnForm(std::vector<CFormID*> dragFromIDs, CFormID* pDropOnID);
    void DropItemOnBlock(std::vector<CFormID*> dragFromIDs, CFormID* pDropOnID);
    void DropRosterItemOnBlock(std::vector<CFormID*> dragFromIDs, CFormID* pDropOnID);
    void DropBlockOnItem(std::vector<CFormID*> dragFromIDs, CFormID* pDropOnID);
    void DropBlockOnForm(std::vector<CFormID*> dragFromIDs, CFormID* pDropOnID);
    void DropBlockOnBlock(std::vector<CFormID*> dragFromIDs, CFormID* pDropOnID);
    void DropBlockOnRoster(std::vector<CFormID*> dragFromIDs, CFormID* pDropOnID);
    void DropRosterOnBlock(std::vector<CFormID*> dragFromIDs, CFormID* pDropOnID);
    void DropItemOnForm(std::vector<CFormID*> dragFromIDs, CFormID* pDropOnID);
    void DropRosterItemOnRoster(std::vector<CFormID*> dragFromIDs, CFormID* pDropOnID);

    CImageList* CreateDragImageEx(HTREEITEM hItem);

// Overrides
    // ClassWizard generated virtual function overrides
    //{{AFX_VIRTUAL(CFormTreeCtrl)
    public:
    virtual BOOL PreTranslateMessage(MSG* pMsg);
    //}}AFX_VIRTUAL
// Implementation


    // Generated message map functions
protected:
    //{{AFX_MSG(CFormTreeCtrl)
    afx_msg void OnSelchanged(NMHDR* pNMHDR, LRESULT* pResult);
    afx_msg void OnDTreeDblclk(NMHDR* pNMHDR, LRESULT* pResult);
    afx_msg void OnItemexpanding(NMHDR* pNMHDR, LRESULT* pResult);
    afx_msg void OnDeleteitem(NMHDR* pNMHDR, LRESULT* pResult);
    afx_msg void OnLButtonDown(UINT nFlags, CPoint point);
    afx_msg void OnLButtonUp(UINT nFlags, CPoint point);
    afx_msg void OnMouseMove(UINT nFlags, CPoint point);
    afx_msg void OnKeyDown(UINT nChar, UINT nRepCnt, UINT nFlags);
    afx_msg void OnEditFieldProp();
    afx_msg void OnEditFormProp();
    afx_msg void OnEditLevelProp();
    afx_msg void OnEditFFProp();
    afx_msg void OnEditBlockProp();
    afx_msg void OnEditGridAutoFit();
    afx_msg void OnEditGridProp();
    afx_msg void OnEditReportProperties();
    afx_msg void OnDeleteItem();
    afx_msg void OnDeleteForm();
    afx_msg void OnViewLogic();
    afx_msg void OnViewQuestionnaire();
    afx_msg void OnCopyName();
    afx_msg void OnQSFEditor();
    afx_msg void OnCapiMacros();
    afx_msg void OnViewForm();
    afx_msg void OnAddBlock();
    afx_msg void OnAddForm();
    afx_msg void OnSelchanging(NMHDR* pNMHDR, LRESULT* pResult);
    afx_msg void OnUngroupItems();
    afx_msg void OnGetDispInfo(NMHDR* pNMHDR, LRESULT* pResult);
    afx_msg void OnRButtonUp(UINT nFlags, CPoint point);
    afx_msg void OnRButtonDown(UINT nFlags, CPoint point);
    afx_msg void OnSetFocus(NMHDR* pNMHDR, LRESULT* pResult);
    afx_msg void OnEditCopy();
    afx_msg void OnShiftF10();
    //}}AFX_MSG

    DECLARE_MESSAGE_MAP()

    HTREEITEM FindItem( CDEFormBase* pItemBase );
    HTREEITEM xFindItem( HTREEITEM hInputItem, CDEFormBase* pFormBase );

    void UpdateGroupItemsForDrop(CDEGroup* pGroup, const std::vector<CDEField*>& dragFields, CDEItemBase* pDropItem);
    void UpdateRosterColumnsForDrop(CDERoster* pRoster, const std::vector<CDEField*>& dragFields, CDEField* pDropField, bool dropAtEndIfDropFieldNull);
    CString GetNameOrLabel(CFormID* pID, CDEField** pSelField, bool name =true);

private:
    CFormID* GetSelectedFormId();
};
