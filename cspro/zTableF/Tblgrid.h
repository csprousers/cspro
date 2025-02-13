#pragma once

//***************************************************************************
//  File name: TblGrid.h
//
//  Description:
//       Definition table grid
//
//  History:    Date       Author     Comment
//              -----------------------------
//              1998        gsf       created from MyCug.h
//
//***************************************************************************

#include <zTableF/zTableF.h>
#include <zTableF/GTblob.h>
#include <zGridO/Ugctrl.h>
#include <zGridO/Ugmedit.h>
#include <zTableF/GridExpt.h>

//forward declaration
class CTable;
class CTabSet;
class CTabulateDoc;
class CGTblCol;
class CGTblRow;

typedef struct SUBTBL_BOX {
public:
    RECT      m_rect;
    int       m_iColorIndex;
} SUBTBL_BOX;

/////////////////////////////////////////////////////////////////////////////
//
//                             CTblGrid
//
/////////////////////////////////////////////////////////////////////////////

/*enum GRID_COMPONENT{GRID_INVALID_COMP=-1 ,GRID_TITLE=0, GRID_SUBTITLE,GRID_STUBHEAD,GRID_SPANNER,GRID_COLHEAD,GRID_CAPTION,GRID_STUB,GRID_PAGENOTE,GRID_EndNote,GRID_CELLS};*/

class CLASS_DECL_ZTABLEF CTblGrid:public CUGCtrl
{
private:
    CTable*     m_pTable;       // pointer to table currently connected to grid
//  int*        m_piCurrTable;  // pointer to index (0,1,2...) of current table
    CFont       m_font;
    CPen        m_blackPen;
    CPen        m_whitePen;
    CTabSet*    m_pSpec;

    CGTblCol*    m_pTblColRoot;
    CGTblRow*    m_pTblRowRoot;
    int          m_iGridHeaderRows;
    int          m_iNumGridRows; //Total number of grid rows required
    CArray<CGTblCell* ,CGTblCell*> m_arrCells;
    CTabVar*        m_pDragSourceItem;
    //CUGMaskedEdit   m_InlineEdit;
    CUGEdit   m_InlineEdit;
    bool            m_bGridVarDrop;
    CGTblOb         m_GTitle;
    CGTblOb         m_GSubTitle;
    CGTblOb         m_GStubHead;

    CGTblOb         m_GPageNote;
    CGTblOb         m_GAreaCaption;
    CGTblOb         m_GEndNote;
    int             m_iCurrSelArea;

    CArray<CGTblRow*,CGTblRow*> m_arrGTblRows; //starting from the m_iGridHeaderRow;
    CArray<CGTblCol*,CGTblCol*> m_arrGTblColHeads; //for row m_iGridHeaderRow-1;
    CArray<SUBTBL_BOX,SUBTBL_BOX&>m_arrSubTableBoxes;

    bool            m_bGridUpdate;
    bool            m_bFixLines;

private:
    void UpdateData(); //use Update with bOnlyData = true instead
    void FormatDataAfterRowTransformation();
protected:
    virtual void Moved();               //this function is called whenever a grid movement is made
public:
    CTblGrid();
    ~CTblGrid();

    int         m_iDragDropRow;     // where drag started
    int         m_iDragDropCol;     // where drag started
    bool        m_bReconcileSpecialCells;

    void DefaultGrid(void);
    void ResetGrid(void);

    // brought over from CTable (delete this comment when stuff works)
    void SetupGrid();
    void Update(bool bOnlyData = false);

    void DrawGridTitle();
    void DrawPageOrEndNote();
    void DrawGridCols (CGTblCol* pCol, int iLevel);
    void DrawGridRows (CGTblRow* pRow, int iLevel);
    void DrawGridStubHead();
    void DrawTblBorders();
    void DrawLineAtBottom();
    void FixLines (long lFixRow = -1);
    void ResetGridColWidths(CDC* pDC);
    void DrawSubTableLines(void);
    void ApplyGridColsJoins (CGTblCol* pCol, int iLevel);
//  int StyleLineToGridMask (CStyle* pStyle);
//  int StyleBorderToGridMask (CStyle* pStyle);
//  int StyleAlignToGridMask (CStyle* pStyle);

    int Twips2NumSpaces(int iTwips, CFont* pFont);
    void DeleteGTblObs();

    BOOL IsValidDragDrop(int col,long row);
    bool IsGridEmpty (void);
    CString GetRowColString(int iRow,int iCol);


    void SetSpec (CTabSet* pTS) { m_pSpec = pTS; }
    CTabSet* GetTabSpec (void) { return m_pSpec; }

    CTabVar* GetDragSourceItem(void){ return m_pDragSourceItem;}
    void    SetDragSourceItem(CTabVar* pTabVar){ m_pDragSourceItem = pTabVar;}
    bool    IsGridVarDrop(void) { return m_bGridVarDrop;}
    void    SetGridVarDrop(bool bGridVarDrop) { m_bGridVarDrop = bGridVarDrop;}

    bool  IsAStarBPlusCMode(CTabVar* pTabVar);
    void  HandleAStarBPlusCModeDisplay(void);
    bool CheckHideCaptionRow(int iRow);
    bool CheckHideAllZeroRow(int iRow);
    bool IsSystemTotal(CGTblOb* pTblOb);
    //Savy (R) sampling app 20081209
    bool IsSystemStat(CGTblOb* pTblOb);
    void  ProcessHideCaptions(void);
    void  ProcessHideSpanners(bool bOnlySystemTotalSpanner = false);
    void  ProcessHideStubs(void);
    void  ProcessHideColHeads(void);
    long GetLastDataRow(void);
    CTabVar* GetColHead(int iCurrentCol);


    void ClearAllRunTimeFmts(CGTblRow* pGTblRow);
    void ClearAllRunTimeFmts(void);
    void BuildAllRunTimeFmts(void);


    void ProcessAreaTokensinRows(void);
    void SetAreaLabels4AllTables();
    bool IsOneRowVarTable();


    void ReconcileSpecialCells();
    void ReconcileTabValFmts();

    bool HasArea();

    void CleanupDataCells();
    void ForceUpdate();

    void ComputeSubTableRects(void);
    void ComputeSubTableRects(CGTblRow* pSubTblRowVar,int& iColorIndex);
    void DrawSubTableRects(bool bComputeRects =false);
    bool CanDrawSubTblBoxes();
    void FixAreaTokensInOneRowTable();
    bool ForceHideAreaCaptionInOneRowTable();

    void DrawLines4AreaCaptionRows(void);
    bool GetFmt4AreaCaptionCells(CGTblCol* pGTblCol ,CFmt& retFmt);

    int GetAdditionalJoinColsForSpanners(CTabVar* pStartTabVar, int iNumJoinSpanners);
    // Overrides
    // ClassWizard generated virtual function overrides
    //{{AFX_VIRTUAL(CTblGrid)
    //}}AFX_VIRTUAL


    //{{AFX_MSG(CTblGrid)
    // NOTE - the ClassWizard will add and remove member functions here.
    //    DO NOT EDIT what you see in these blocks of generated code!

    //}}AFX_MSG
    DECLARE_MESSAGE_MAP()


    //***** Over-ridable Notify Functions *****
    virtual void OnSetup();
    virtual void OnSheetSetup(int sheetNumber);

    //movement and sizing
    virtual int  OnCanMove(int oldcol,long oldrow,int newcol,long newrow);
    virtual int  OnCanViewMove(int oldcol,long oldrow,int newcol,long newrow);
    virtual void OnHitBottom(long numrows,long rowspast,long rowsfound);
    virtual void OnHitTop(long numrows,long rowspast);

    virtual int  OnCanSizeCol(int col);
    virtual void OnColSizing(int col,int *width);
    virtual void OnColSized(int col,int *width);
    virtual int  OnCanSizeRow(long row);
    virtual void OnRowSizing(long row,int *height);
    virtual void OnRowSized(long row,int *height);

    virtual int  OnCanSizeTopHdg();
    virtual int  OnCanSizeSideHdg();
    virtual int  OnTopHdgSizing(int *height);
    virtual int  OnSideHdgSizing(int *width);
    virtual int  OnTopHdgSized(int *height);
    virtual int  OnSideHdgSized(int *width);

    virtual void OnColChange(int oldcol,int newcol);
    virtual void OnRowChange(long oldrow,long newrow);
    virtual void OnCellChange(int oldcol,int newcol,long oldrow,long newrow);
    virtual void OnLeftColChange(int oldcol,int newcol);
    virtual void OnTopRowChange(long oldrow,long newrow);

    //mouse and key strokes
    virtual void OnLClicked(int col,long row,int updn,RECT *rect,POINT *point,int processed);
    virtual void OnRClicked(int col,long row,int updn,RECT *rect,POINT *point,int processed);
    virtual void OnDClicked(int col,long row,RECT *rect,POINT *point,int processed);
    virtual void OnMouseMove(int col,long row,POINT *point,UINT nFlags,BOOL processed=0);
    virtual void OnTH_LClicked(int col,long row,int updn,RECT *rect,POINT *point,BOOL processed=0);
    virtual void OnTH_RClicked(int col,long row,int updn,RECT *rect,POINT *point,BOOL processed=0);
    virtual void OnTH_DClicked(int col,long row,RECT *rect,POINT *point,BOOL processed=0);
    virtual void OnSH_LClicked(int col,long row,int updn,RECT *rect,POINT *point,BOOL processed=0);
    virtual void OnSH_RClicked(int col,long row,int updn,RECT *rect,POINT *point,BOOL processed=0);
    virtual void OnSH_DClicked(int col,long row,RECT *rect,POINT *point,BOOL processed=0);
    virtual void OnCB_LClicked(int updn,RECT *rect,POINT *point,BOOL processed=0);
    virtual void OnCB_RClicked(int updn,RECT *rect,POINT *point,BOOL processed=0);
    virtual void OnCB_DClicked(RECT *rect,POINT *point,BOOL processed=0);

    virtual void OnKeyDown(UINT *vcKey,BOOL processed);
    virtual void OnCharDown(UINT *vcKey,BOOL processed);


    //GetCellIndirect notification
    virtual void OnGetCell(int col,long row,CUGCell *cell);
    //SetCell notification
    virtual void OnSetCell(int col,long row,CUGCell *cell);

    //data source notifications
    virtual void OnDataSourceNotify(int ID,long msg,long param);

    //cell type notifications
    virtual int OnCellTypeNotify(long ID,int col,long row,long msg,long param);

    //editing
    virtual int OnEditStart(int col, long row,CWnd **edit);
    virtual int OnEditVerify(int col,long row,CWnd *edit,UINT *vcKey);
    virtual int OnEditFinish(int col, long row,CWnd *edit,LPCTSTR string,BOOL cancelFlag);
    virtual int OnEditContinue(int oldcol,long oldrow,int* newcol,long* newrow);

    //menu notifications
    virtual void OnMenuCommand(int col,long row,int section,int item);
    virtual int  OnMenuStart(int col,long row,int section);

    //hints
    virtual int OnHint(int col,long row,int section,CString *string);
    virtual int OnVScrollHint(long row,CString *string);
    virtual int OnHScrollHint(int col,CString *string);


    #ifdef __AFXOLE_H__  //OLE must be included

    //drag and drop
    virtual DROPEFFECT OnDragEnter(COleDataObject* pDataObject);
    virtual DROPEFFECT OnDragOver(COleDataObject* pDataObject,int col,long row);
    virtual DROPEFFECT OnDragDrop(COleDataObject* pDataObject,int col,long row);
    virtual void OnDragLeave(CWnd* pWnd);
    virtual DROPEFFECT OnDragScroll(CWnd* pWnd, DWORD dwKeyState, CPoint point);

    DROPEFFECT DoFlip(COleDataObject* pDataObject,int col,long row);
    int StartDragDrop(int iRow, int iCol);
    int CTblGrid::DragDropTarget(BOOL state);
    #endif


    //sorting
    virtual int OnSortEvaluate(CUGCell *cell1,CUGCell *cell2,int flags);


    //DC setup
    virtual void OnScreenDCSetup(CDC *dc,int section);

    virtual void OnAdjustComponentSizes(RECT *grid,RECT *topHdg,RECT *sideHdg,RECT *cnrBtn,
        RECT *vScroll,RECT *hScroll,RECT *tabs);


    virtual COLORREF OnGetDefBackColor(int section);

    //focus rect setup
    virtual void OnDrawFocusRect(CDC *dc,RECT *rect);
    virtual void OnSetFocus(int section);
    virtual void OnKillFocus(int section);

    //column swapping
    virtual BOOL OnColSwapStart(int col);
    virtual BOOL OnCanColSwap(int fromCol,int toCol);

    //tracking window
    virtual void OnTrackingWindowMoved(RECT *origRect,RECT *newRect);
    void  DoRender();

public:
    void UpdateTableBlocking(bool bBlock =false);
    void BlockGTblObParents(CGTblOb* pGTblOb , bool bFlag);
    bool SpecToTable(void);
    void AddChildRows(CTabVar* pTabVar , CGTblRow* pParentRow);
    void AddChildCols(CTabVar* pTabVar , CGTblCol* pParentCol);
    int  ComputeGridCols(CGTblCol* pCol, int iStartGridCol, int iLevel);
    int  ComputeGridRows(CGTblRow* pRow, int* iStartGridRow, int iLevel);
    void ProcessCells();
    void DestroyCells();
    CTabVar*  GetRowVar(long iRowNumber) ;

    void SetTable(CTable* pTable) { m_pTable=pTable;}
    CTable* GetTable(void) { return m_pTable;}
    void ResetSizes();

    CGTblRow* GetRowRoot (void) { return m_pTblRowRoot; }
    CGTblCol* GetColRoot (void) { return m_pTblColRoot; }

    void ProcessFmtDlg(int iCol , long lRow);
    int GetNumHeaderRows (void) { return m_iGridHeaderRows; }
    int GetNumGridRows   (void) { return m_iNumGridRows; }
    void RenderFreqStats(void);
    void RenderFreqNTiles(void);

    void GetComponent(int col, long row, FMT_ID& eGridComp, CGTblOb** pGTblOb);
    CIMSAString GetComponentString(int col, long row);
    void DrawBitMap(bool bDrawNew = false);
    void DoDragScroll();

    void ApplyStateInfo();
    void SaveStateInfo();
    void SavePageAndEndNoteStateInfo();
    void ApplyPageAndEndNoteStateInfo();
    bool IsValidCell4InlineEdit(int iCol, long lRow);
    void ApplyTransformation();
    void DoRowTransformation(CGTblRow* pRow);
    bool IsMultiSelectState();
    bool GetMultiSelObj(CArray<CTblBase*,CTblBase*>&arrTblBase,FMT_ID eFmtID);

    int PutHTMLTable(_tostream& os, bool bBlockedOnly=true, bool bIncludeParents=true);


    int PutRTFTable(_tostream& os, bool bBlockedOnly=true, bool bIncludeParents=true);

    void PutTable(CTableGridExporter& exporter, _tostream& os, bool bBlockedOnly=true, bool bIncludeParents=true);
   // void GetRowHeaders(long iRow, CDWordArray& aRowHeaders);
    int GetRowStubLevel(long iRow);
    void PutFormats(CTableGridExporter& exporter, _tostream& os, bool bBlockedOnly, bool bIncludeParents);
    bool ShouldExportCell(int iCol, long iRow, bool bBlockedOnly, bool bIncludeParents,
                          CFmt& fmtComponent,
                          CTableGridExporter::CJoinRegion& join);
    bool IsAreaCaptionRow(long row);
    void AdjustJoinForVisibleExport(CTableGridExporter::CJoinRegion& adjustJoin,
                                    const CTableGridExporter::CJoinRegion& origJoin,
                                    int iCol, int iRow,
                                    int iVisColsToJoinStart, int iVisRowsToJoinStart,
                                    bool bBlockedOnly, bool bIncludeParents);
    int PutASCIITable(_tostream& os, bool bBlockedOnly=true, bool bIncludeParents=true);
    CFmt* GetFmtFromRowCol(int iCol , long lRow);
    short GetAlignmentFlags(const CFmt* const pFmt);
    int StyleLineToGridMask (const CFmt* const pFmt);
    void ApplyFormat2Cell(CUGCell& gridCell , const CFmt* const pFmt);

    std::unique_ptr<CFmt> GetFmt4Cell(int iCol, int iRow);
    void GetFmt4NonDataCell(CGTblOb* pGTblOb ,FMT_ID eGridComp, CFmt& retFmt);
    //void GetFmt4DataCell(CGTblOb* pGTblOb , CDataCellFmt& retFmt);
    void GetFmt4DataCell(int iCol, int iRow, CDataCellFmt& retFmt);
    void GetFmt4DataCell2(CGTblRow* pGTblRow , CGTblCol* pGTblCol ,CDataCellFmt& retFmt,CSpecialCell* pSpecialCell=NULL);


    void GetLineFmt4NonDataCell(CGRowColOb* pGTblOb ,FMT_ID eGridComp, CFmt& retFmt);
    int  GetRowPanel(int iRow);
    int  GetNextPanel(int iCurPanelRow, int& iNextPanelRow);
    int  GetNextColOffSet(int iCurColOffSet);
    int  GetColNumForFirstPanel(int iCurColOffSet);

    //void CopyNonDefVals(CFmt& toFmt,const CFmt& frmFmt);

    void ApplyFormat2StubCol(); //Column which have "stubs and captions"
    void ApplyFormat2SpannersNColHeads();
   // void ApplyFormat2DataCells();
  //  void ApplyFormat2DataCells2(int iStartCol ,long lStartRow , int iEndCol, long lEndRow);
    void ApplyFormat2DataCells2(int iCellCol ,long lCellRow , CUGCell* cell);
    CGTblCell* GetGTblCell(int col, long row);
    CArray<CGTblCell*,CGTblCell*>& GetGTblCellsArray() {return m_arrCells;}
    bool       IsDataCell(int col, long row);

    bool IsCellHidden(int col, long row);
    bool IsCellBlocked(int col, long row);
    bool ShowSpannerOrHorizMergedCell(int col, bool bBlockedOnly);
    bool ShowVertMergedCell(long iRow, bool bBlockedOnly, bool bIncludeParents);

    int GetStartRow4SpannerNColHeadProcessing();

    int GetCurSelArea() {return m_iCurrSelArea;}
    void SetCurSelArea(int iCurrSelArea) {m_iCurrSelArea = iCurrSelArea;}

    const CArray<SUBTBL_BOX,SUBTBL_BOX&>& GetSubTablBoxArr(){ return m_arrSubTableBoxes;}

    LRESULT OnUpdateTable(WPARAM wParam,LPARAM lParam);
    afx_msg void OnPaint();
    afx_msg void OnShiftF10();
};
