#pragma once

//***************************************************************************
//  File name: FrmTrCtl.h
//
//  Description:
//       Orders tree control definitions
//
//              ---------------------------
//
//***************************************************************************


#include <zFormO/Roster.h>
#include <zDesignerF/AppTreeNode.h>

class Application;
class CDDTreeCtrl;


class CLASS_DECL_ZORDERF COrderTreeCtrl : public CTreeCtrl
{
public:
    COrderTreeCtrl();

    void RestoreItemSelection();
    HTREEITEM InsertOrderFile(const CString& sLabel, const CString& sOrderFileName, COrderDoc* pDoc);

    void SetDocTemplate(CDocTemplate* pTemplate) { m_pDocTemplate = pTemplate; }

    void SetOrderDoc(COrderDoc* pDoc) { m_pOrderDoc = pDoc; }
    COrderDoc* GetOrderDoc()          { return  m_pOrderDoc; }
    COSourceEditView* GetOrderView();

    CDDTreeCtrl* GetDDTreeCtrl() const         { return m_pDDTreeCtrl; }
    void SetDDTreeCtrl(CDDTreeCtrl* pTreeCtrl) { m_pDDTreeCtrl = pTreeCtrl; }

    BOOL GetSndMsgFlg() const     { return m_bSendMsg; }
    void SetSndMsgFlg(BOOL bFlag) { m_bSendMsg = bFlag; }

    void InitImageList();
    void BuildTree(FormOrderAppTreeNode* form_order_app_tree_node, bool bRedraw=true);
    void ReBuildTree(int iOrderNum=0, CDEFormBase* pItem=NULL,bool bSendMsg = true);    // default to display Order 1 (0-based)
    void AddGroupItems(COrderDoc* pOrderDoc, CDEGroup* pGroup, HTREEITEM hParent, int iLevel);
    void AddBaseItem(COrderDoc* pOrderDoc, CDEItemBase* pItem, HTREEITEM hParent);
    void AddBlock(COrderDoc* pOrderDoc, CDEBlock& form_block, HTREEITEM hParent);

    void DefaultExpand(HTREEITEM hItem);
    void SelectFTCNode(const AppTreeNode& app_tree_node, int iOrderNum, CDEFormBase* pItem);

    void InsertExternalCodeAndReportNodes(FormOrderAppTreeNode* form_order_app_tree_node);

    AppTreeNode* GetTreeNode(HTREEITEM hItem) const { return reinterpret_cast<AppTreeNode*>(GetItemData(hItem)); }
    FormOrderAppTreeNode* GetFormOrderAppTreeNode(CDocument& document) const;
    FormOrderAppTreeNode* GetFormOrderAppTreeNode(wstring_view filename) const;

    BOOL OpenOrderFile(const CString &sOrderFile);

    void ReleaseOrderNode(FormOrderAppTreeNode& form_order_app_tree_node);
    void ReleaseDoc(FormOrderAppTreeNode& form_order_app_tree_node);
    bool InsertOrderDependencies(FormOrderAppTreeNode& form_order_app_tree_node);
    bool ReleaseOrderDependencies(FormOrderAppTreeNode& form_order_app_tree_node);

    void DropFormTreeItem(AppTreeNode& drag_from_app_tree_node, CPoint dropPoint);
    void DropItemOnItem(FormElementAppTreeNode& drag_from_app_tree_node, FormElementAppTreeNode& drop_on_app_tree_node);

    void FindFTCRecursively(HTREEITEM& hGroupOrItem, HTREEITEM& hExpand, int iOrderNum, CDEFormBase* pSearchItem);

    CImageList* CreateDragImageEx(HTREEITEM hItem);

protected:
    DECLARE_MESSAGE_MAP()

    afx_msg void OnSelchanged(NMHDR* pNMHDR, LRESULT* pResult);
    afx_msg void OnSelchanging(NMHDR* pNMHDR, LRESULT* pResult);
    afx_msg void OnDTreeDblclk(NMHDR* pNMHDR, LRESULT* pResult);
    afx_msg void OnItemexpanding(NMHDR* pNMHDR, LRESULT* pResult);
    afx_msg void OnDeleteitem(NMHDR* pNMHDR, LRESULT* pResult);
    afx_msg void OnLButtonDown(UINT nFlags, CPoint point);
    afx_msg void OnLButtonUp(UINT nFlags, CPoint point);
    afx_msg void OnBegindrag(NMHDR* pNMHDR, LRESULT* pResult);
    afx_msg void OnMouseMove(UINT nFlags, CPoint point);
    afx_msg void OnKeyDown(UINT nChar, UINT nRepCnt, UINT nFlags);
    afx_msg void OnGetdispinfo(NMHDR* pNMHDR, LRESULT* pResult);
    afx_msg void OnRButtonDown(UINT nFlags, CPoint point);
    afx_msg void OnRButtonUp(UINT nFlags, CPoint point);
    afx_msg void OnShiftF10();
    afx_msg void OnEditReportProperties();
    afx_msg void OnCopyName();
    afx_msg void OnViewQuestionnaire();
    afx_msg void OnViewLogic();

public:
    int             m_iOrder;
    CDEFormBase*    m_pCurItem; // make it private soon; since node could be Order file or level, not
                                // just rosters, groups and fields, need to pt to a Orderbase, not itembase
                                // as i'm doing in the scrollview and Orderdoc

private:
    CWnd*           m_pDropWnd;
    std::unique_ptr<CImageList> m_pDragImage;   // the current image being dragged (i think)
    CImageList      m_cImageList;               // where my icons will be stored during registration

    bool            m_bDragging;    // am i dragging?

    bool            m_bOkToRedraw;  // contrary to what i'd like, SetRedraw() doesn't cut it,
                                    // nd2keep track of whether it's ok to refresh or not,
                                    // specifically, for OnSelchanged()

    HTREEITEM       m_hDragItem;
    HTREEITEM       m_hDropItem;
    HTREEITEM       m_hRestoreItemSelection;

    int             m_nDropIndex;
    DWORD           m_dwDragStart;

    CDocTemplate*   m_pDocTemplate;
    COrderDoc*      m_pOrderDoc;        // a ptr to the doc that owns me

    CDDTreeCtrl*    m_pDDTreeCtrl;  // ptr to the tree ctrl containing our dict

    BOOL            m_bSendMsg;     //Flag for sending messages to the main window

    static LOGFONT  m_DefLogFont;
    static CFont    m_font;
};
