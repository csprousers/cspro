#pragma once

// TabView.h : header file
//

/////////////////////////////////////////////////////////////////////////////
// CTabView view

#include <zTableF/zTableF.h>
#include <zTableF/Tblgrid.h>
#include <zTableF/AreaDlg.h>
#include <zTableF/Tblgrid.h>
#include <zFormO/FormFile.h>

class CGTblOb;
class CGRowRowOb;
class CGRowColOb;
class CGTblRow;
class DictTreeNode;

enum TB_DROP_TYPE {TB_DROP_INVALID=0, TB_DROP_ROW,TB_DROP_COL};
enum TB_DROP_OPER {TB_DROP_NOOP=0, TB_DROP,TB_DROP_PLUS,TB_DROP_STAR,TB_DROP_STARPLUS};

class CLASS_DECL_ZTABLEF CTabView : public CView
{
protected:
    CTabView();           // protected constructor used by dynamic creation
    DECLARE_DYNCREATE(CTabView)

// Attributes
public:
    int m_iColRClick;
    long m_lRowRClick;

private:
   CTblGrid*            m_pGrid;            // the one-and-only grid...

   COleDropTarget       m_dropTarget;   // nd for registering the view that it will
   COleDropSource       m_dropSource;   // accept drags and can be initiator of drags

   CTabVar*             m_pCurVar;      // smg: savy, is there another way 'round this?

   int                  m_iCurrSelArea; // area selected in combo box for filter on area
   int                  m_iVertScrollPos;
   int                  m_iHorzScrollPos;
// Overrides
    // ClassWizard generated virtual function overrides
    //{{AFX_VIRTUAL(CTabView)
    public:
    virtual void OnInitialUpdate();
    protected:
    virtual void OnDraw(CDC* pDC);      // overridden to draw this view
    //}}AFX_VIRTUAL
    virtual void OnActivateView(BOOL bActivate, CView* pActivateView, CView* pDeactiveView);

// Implementation
public:
    CTabulateDoc* GetDocument() const { return (CTabulateDoc*)CView::GetDocument(); }
protected:
    virtual ~CTabView();
#ifdef _DEBUG
    virtual void AssertValid() const;
    virtual void Dump(CDumpContext& dc) const;
#endif

public :

    bool SaveTables(CString& sSaveAsName);
    void DeleteUnsavedTables(); // this deletes tables not marked for save in TBW

    LRESULT OnDropItem (WPARAM wParam,LPARAM lParam) ;
    LRESULT OnDictDroppedOn(WPARAM /*wParam*/,LPARAM lParam);

    bool IsDropPointValid(CPoint point);
    bool IsDropValid(WPARAM wParam,LPARAM lParam);
    int  GetDropCursor(CPoint point);

    CTblGrid* GetGrid()   { return m_pGrid;}
    CTable* GetCurTable() { return (m_pGrid ? m_pGrid->GetTable() : NULL);}

    CTabVar* GetCurVar()          { return m_pCurVar; }
    void SetCurVar(CTabVar* pVar) { m_pCurVar = pVar; }

    bool IsTableBlank();
    bool IsTblOnlyRoworOnlyCol();
    bool IsDropValidDictItem(const DictTreeNode& dict_tree_node);

    TB_DROP_TYPE GetDropType(CPoint point);
    TB_DROP_TYPE GetDropType4RorColOnly(CPoint point);
    TB_DROP_TYPE GetDropType4RC(CPoint point);
    TB_DROP_TYPE GetDropType4Stub(CPoint point);

    TB_DROP_OPER GetDropOperation(CPoint point);
    TB_DROP_OPER GetDropOp4RorColOnly(CPoint point);
    TB_DROP_OPER GetDropOp4RC(CPoint point);

    bool GetDictInfo(const DictTreeNode& dict_tree_node, DICT_LOOKUP_INFO& lookupInfo);

    int  GetOcc4TabVar(const CDictItem* pDictItem);
    int  GetOcc4TabVar(DICT_LOOKUP_INFO& lookupInfo);

    void AddSystemTotalVar();
    void RemoveSystemTotalVar();

    CTabVar* AddRowVarPlus(DICT_LOOKUP_INFO& lookupInfo,CTabVar* pTargetItem, bool bAfter = true );
    CTabVar* AddRowVarStar(DICT_LOOKUP_INFO& lookupInfo,CTabVar* pTargetItem, bool bAfter = true );

    CTabVar* AddColVarPlus(DICT_LOOKUP_INFO& lookupInfo,CTabVar* pTargetItem, bool bAfter = true );
    CTabVar* AddColVarStar(DICT_LOOKUP_INFO& lookupInfo,CTabVar* pTargetItem, bool bAfter = true );

    CTabVar* GetTargetItem(CPoint point);
    CTabVar* GetTabVarFromPoint(CPoint point);
    bool IsStarPlusDrop(CPoint point);
    bool IsTransitionPoint(CPoint point);
    bool DoRuleCheck(CTabVar* pTabVar1 , CTabVar* pTabVar2,bool bCol =true);
    void RefreshViewAfterDrop();
    bool RefreshLogicAfterDrop();
    bool IsStubDrop(CPoint point);
    void MakeDummyVSet (const CDictItem* pDictItem, CStringArray& arrVals);
    void DoFmtDlg4TblOb(CTblOb* pTblOb);
    void DoFmtDlg4SpannerOrCaption(CTabVar* pTabVar);
    void DoFmtDlg4ColheadOrStub(CTabValue* pTabVal);

    void MakeDefTallyFmt4Dlg(CTallyFmt* pCopyFmt, CTallyFmt* pDefFmt);
    void MakeDefFmt4Dlg(CFmt* pCopyFmt, CFmt* pDefFmt);
    void DoFmtDlg4DataCell(CGTblOb* pGTblOb);
    void ProcessSpecials(CTabValue* pTabVal);
    void DoFmtDlg4App();
    void DoFmtDlg4Table();

    void UpdateAreaComboBox(bool bResetSelection);

    bool DoMultiRecordChk(const CDictRecord* pMultRecord,CTabVar* pTargetVar,const TB_DROP_TYPE& eDropType,const TB_DROP_OPER& eDropOper);
    bool DoMultiRecordChk(CTabVar* pTabVar,const CDictRecord* pMultRecord);

    bool DoMultiItemChk(const CDictItem* pOccDictItem,CTabVar* pTargetVar,const TB_DROP_TYPE& eDropType,const TB_DROP_OPER& eDropOper);
    bool DoMultiItemChk(CTabVar* pTabVar,const CDictItem* pOccDictItem);

    void CopyCellsToClipboard(bool bIncludeParents);
    bool IsPointInSubTableRect(CPoint point);

    void SaveScrollPos();
    void RestoreScrollPos();
    void ReconcileFmtsForPaste(CFmtReg& fmtRegFrmClip,CMap<CFmtBase*,CFmtBase*,CFmtBase*,CFmtBase*>& aMapOldFmts2NewFmts);
    void FixFmtsForPasteTable(CTable* pTable,CMap<CFmtBase*,CFmtBase*,CFmtBase*,CFmtBase*>& aMapOldFmts2NewFmts);

    // Generated message map functions
protected:
    //{{AFX_MSG(CTabView)
    afx_msg void OnSize(UINT nType, int cx, int cy);
    afx_msg void OnRuntab();
    afx_msg void OnUpdateRuntab(CCmdUI* pCmdUI);
    afx_msg void OnAddTable();
    afx_msg void OnUpdateAddTable(CCmdUI* pCmdUI);
    afx_msg void OnDeleteTable();
    afx_msg void OnUpdateDeleteTable(CCmdUI* pCmdUI);
    afx_msg void OnInsertTable();
    afx_msg void OnUpdateInsertTable(CCmdUI* pCmdUI);
    afx_msg void OnToggleTree();
    afx_msg void OnUpdateToggleTree(CCmdUI* pCmdUI);
    afx_msg void OnTblRun();
    afx_msg void OnUpdateTblRun(CCmdUI* pCmdUI);
    afx_msg void OnRunPcs();
    afx_msg void OnUpdateRunPcs(CCmdUI* pCmdUI);
    afx_msg void OnFileSaveTable();
    afx_msg void OnUpdateFileSaveTable(CCmdUI* pCmdUI);
    afx_msg void OnRButtonUp(UINT nFlags, CPoint point);
    afx_msg BOOL OnEditVarTallyAttributes(UINT nID);
    afx_msg void OnEditTableTallyAttributes();
    afx_msg void OnEditSubTableTallyAttributes();
    afx_msg void OnEditTblCompFmt();
    afx_msg void OnEditTableFmt();
    afx_msg void OnEditTablePrintFmt();
    afx_msg void OnEditAppFmt();
    afx_msg void OnDeselectAll();
    afx_msg void OnSelectAll();
    afx_msg void OnMouseMove(UINT nFlags, CPoint point);
    afx_msg void OnEditTblSplitSpanners();
    afx_msg void OnEditTblJoinSpanners();

    //}}AFX_MSG
    DECLARE_MESSAGE_MAP()
public:
    afx_msg void OnArea();
    afx_msg void OnUpdateArea(CCmdUI *pCmdUI);
    afx_msg void OnEditCopy();
    afx_msg void OnUpdateEditCopy(CCmdUI *pCmdUI);
    afx_msg void OnEditFmt();
    afx_msg void OnSelChangeAreaComboBox();
    afx_msg void OnEditCopycellsonly();
    afx_msg void OnFilePrint();
    afx_msg void OnShiftF10();
    afx_msg void OnEditCopytablestructure();
    afx_msg void OnUpdateEditCopytablestructure(CCmdUI *pCmdUI);
    afx_msg void OnEditPastetable();
    afx_msg void OnUpdateEditPastetable(CCmdUI *pCmdUI);
};

/////////////////////////////////////////////////////////////////////////////

//{{AFX_INSERT_LOCATION}}
// Microsoft Visual C++ will insert additional declarations immediately before the previous line.
