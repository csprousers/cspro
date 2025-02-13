#pragma once

#include <zTableF/zTableF.h>
#include <zTableF/TabTreeNode.h>
#include <zDictF/DDTrCtl.H>


/////////////////////////////////////////////////////////////////////////////
// CTabTreeCtrl window

class CLASS_DECL_ZTABLEF CTabTreeCtrl : public CTreeCtrl
{
public:
    CTabTreeCtrl();
    ~CTabTreeCtrl();

    bool GetViewNames4TblView() const     { return m_bViewNames4TblView; }
    void SetViewNames4TblView(bool bFlag) { m_bViewNames4TblView = bFlag; }

    void SetTabDoc(CTabulateDoc* pDoc) { m_pDoc = pDoc; }
    CTabulateDoc* GetTabDoc() const    { return m_pDoc; }

    CDDTreeCtrl* GetDDTreeCtrl() const           { return m_pDDTreeCtrl; }
    void SetDDTreeCtrl(CDDTreeCtrl* pDDTreeCtrl) { m_pDDTreeCtrl = pDDTreeCtrl; }

    void InitImageList();
    int GetImage(CTabVar* pTabVar, bool bRow = true);

    TableElementTreeNode* GetTreeNode(HTREEITEM hItem) const { return reinterpret_cast<TableElementTreeNode*>(GetItemData(hItem)); }
    TableSpecTabTreeNode* GetTableSpecTabTreeNode(CDocument& document) const;
    TableSpecTabTreeNode* GetTableSpecTabTreeNode(wstring_view filename) const;

    HTREEITEM InsertTableSpec(const CString& sTableFileName, CTabulateDoc* pDoc);

    bool InsertTableDependencies(TableSpecTabTreeNode& table_spec_tab_tree_node);
    bool ReleaseTableDependencies(TableSpecTabTreeNode& table_spec_tab_tree_node);
    void ReleaseDoc(TableSpecTabTreeNode& table_spec_tab_tree_node);

    void RemoveTableIDs(HTREEITEM hItem);

    void SetDocTemplate(CDocTemplate* pTemplate) {m_pDocTemplate = pTemplate; }

    void ReleaseTableNode(TableSpecTabTreeNode& table_spec_tab_tree_node);

    void BuildTree(TableSpecTabTreeNode& table_spec_tab_tree_node);
    void BuildLevelTree(TableSpecTabTreeNode& table_spec_tab_tree_node);
    void DefaultExpand(HTREEITEM hItem);

    void ReBuildTree(bool bAll=false);
    bool BuildRowColTreeItem(CTabVar* pTabVar, HTREEITEM hParentItem, TableElementType table_element_type = TableElementType::RowItem);

    void BuildTVTree(CTabulateDoc* pDoc);

    bool OpenTableFile(const CString& sTableFile ,const std::shared_ptr<const CDataDict> pWorkDict = nullptr, bool bMakeVisible = true);

    bool GetSndMsgFlg() const     { return m_bSendMsg; }
    void SetSndMsgFlg(bool bFlag) { m_bSendMsg = bFlag; }

    bool IsPrintTree() const { return m_bPrintTree; }

    void OnToggleTree();
    int GetCurLevel();
    void SelectNextTable();
    void SelectPrevTable();
    void SelectTable(CTable* pTable, bool bUpdatePrintView = true);
    CTable* GetPrevTable(CTable* pTable = NULL);
    CTable* GetNextTable(CTable* pTable = NULL);
    void UpdateTableOnSelect(bool bUpdatePrintView = true);

protected:
    DECLARE_MESSAGE_MAP()

    BOOL PreTranslateMessage(MSG* pMsg) override;

    afx_msg void OnDeleteitem(NMHDR* pNMHDR, LRESULT* pResult);
    afx_msg void OnGetdispinfo(NMHDR* pNMHDR, LRESULT* pResult);
    afx_msg void OnRButtonUp(UINT nFlags, CPoint point);
    afx_msg void OnRButtonDown(UINT nFlags, CPoint point);
    afx_msg void OnLButtonDblClk(UINT nFlags, CPoint point);
    afx_msg void OnAddTable();
    afx_msg void OnInsertTable();
    afx_msg void OnDeleteTable();
    afx_msg void OnEditVarTallyAttributes();
    afx_msg void OnEditTableTallyAttributes();
    afx_msg void OnEditGlobalProps();
    afx_msg void OnEditTableFmt();
    afx_msg void OnEditGeneratelogic();
    afx_msg void OnEditExcludeTbl();
    afx_msg void OnEditIncludeAllTablesInRun();
    afx_msg void OnEditExcludeAllButThis();
    afx_msg void OnEditAppFmt();
    afx_msg void OnEditTablePrintFmt();
    afx_msg void OnEditTabSetProp();
    afx_msg void OnEditTableProp();
    afx_msg void OnShiftF10();
    afx_msg void OnEditCopytablestructure();
    afx_msg void OnEditPastetable();

public:
    afx_msg void OnLButtonDown(UINT nFlags, CPoint point);
    afx_msg void OnSelchanged(NMHDR* pNMHDR, LRESULT* pResult);
    afx_msg void OnTvnBegindrag(NMHDR *pNMHDR, LRESULT *pResult);
    afx_msg void OnLButtonUp(UINT nFlags, CPoint point);
    afx_msg void OnMouseMove(UINT nFlags, CPoint point);
    afx_msg void OnKeyDown(UINT nChar, UINT nRepCnt, UINT nFlags);
    afx_msg void OnTvnSelchanging(NMHDR *pNMHDR, LRESULT *pResult);

// Attributes
private:
    CDocTemplate* m_pDocTemplate;
    CTabulateDoc* m_pDoc;
    CDDTreeCtrl* m_pDDTreeCtrl;
    CImageList m_cImageList;
    bool m_bSendMsg;
    bool m_bPrintTree;
    bool m_bOkToRedraw;
    CDictOleDropTarget m_dropTarget;

    CImageList* m_pDragImage;
    bool m_bDragging;    // flag to tell me if i'm dragging or not!
    DWORD m_dwDragStart;
    CWnd* m_pLineWnd;
    HTREEITEM m_hitemDrag;
    HTREEITEM m_hitemDrop;
    bool m_bViewNames4TblView;
    static LOGFONT m_DefLogFont;
    static CFont m_font;
};
