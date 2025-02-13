#pragma once
//***************************************************************************
//  File name: PrtView.h
//
//  Description:
//       Table printing view
//
//  History:    Date       Author     Comment
//              -----------------------------
//              11 Nov 03   csc       CSPro 3.0
//
//***************************************************************************

#include <zTableF/zTableF.h>
#include <zTableF/prtvundo.h>
#include <zTableF/PrtNvBar.h>
#include <zUtilO/UndoStack.h>


constexpr double PRT_VIEW_MISSING = 1.0e51;

// type of page object
enum PAGEOB_TYPE { PGOB_UNDETERMINED,
                   PGOB_HEADER_LEFT,
                   PGOB_HEADER_CENTER,
                   PGOB_HEADER_RIGHT,
                   PGOB_FOOTER_LEFT,
                   PGOB_FOOTER_CENTER,
                   PGOB_FOOTER_RIGHT,
                   PGOB_TITLE,
                   PGOB_SUBTITLE,
                   PGOB_DATACELL,
                   PGOB_PAGENOTE,
                   PGOB_ENDNOTE,
                   PGOB_STUBHEAD,        // the spot above stubs, adjacent to boxheads
                   PGOB_COLHEAD,         // column with data
                   PGOB_SPANNER,         // column with no data
                   PGOB_STUB,            // row with data
                   PGOB_STUB_RIGHT,      // right-side row with data
                   PGOB_CAPTION,         // row with no data
                   PGOB_READER_BREAK,    // reader break (like a stub, but without data)
                   PGOB_ROOT,            // root placeholder (unused)
                   PGOB_NOTINCLUDED,     // the object is not rendered in the prtview
};

// hit test possibilities
enum PRTVIEW_HITTEST {  PRTVIEW_NO_HIT,
                        PRTVIEW_HIT_DEAD_SPACE,
                        PRTVIEW_HIT_HEADER,
                        PRTVIEW_HIT_FOOTER,
                        PRTVIEW_HIT_TITLE,
                        PRTVIEW_HIT_SUBTITLE,
                        PRTVIEW_HIT_CELL,
                        PRTVIEW_HIT_PAGENOTE,
                        PRTVIEW_HIT_ENDNOTE,
                        PRTVIEW_HIT_STUB,
                        PRTVIEW_HIT_CAPTION,
                        PRTVIEW_HIT_COLHEAD,
                        PRTVIEW_HIT_SPANNER,
                        PRTVIEW_HIT_STUBHEAD,
                        PRTVIEW_HIT_HBORDER_HEADER,
                        PRTVIEW_HIT_VBORDER_HEADER,
                        PRTVIEW_HIT_HBORDER_FOOTER,
                        PRTVIEW_HIT_VBORDER_FOOTER,
                        PRTVIEW_HIT_HBORDER_TITLE,
                        PRTVIEW_HIT_VBORDER_TITLE,
                        PRTVIEW_HIT_HBORDER_SUBTITLE,
                        PRTVIEW_HIT_VBORDER_SUBTITLE,
                        PRTVIEW_HIT_HBORDER_CELL,
                        PRTVIEW_HIT_VBORDER_CELL,
                        PRTVIEW_HIT_HBORDER_PAGENOTE,
                        PRTVIEW_HIT_VBORDER_PAGENOTE,
                        PRTVIEW_HIT_HBORDER_ENDNOTE,
                        PRTVIEW_HIT_VBORDER_ENDNOTE,
                        PRTVIEW_HIT_HBORDER_STUB,
                        PRTVIEW_HIT_VBORDER_STUB,
                        PRTVIEW_HIT_HBORDER_CAPTION,
                        PRTVIEW_HIT_VBORDER_CAPTION,
                        PRTVIEW_HIT_HBORDER_COLHEAD,
                        PRTVIEW_HIT_VBORDER_COLHEAD,
                        PRTVIEW_HIT_HBORDER_SPANNER,
                        PRTVIEW_HIT_VBORDER_SPANNER,
                        PRTVIEW_HIT_HBORDER_STUBHEAD,
                        PRTVIEW_HIT_VBORDER_STUBHEAD,
};


// indicates what type of resize operation is going on ...
enum RESIZE_ORIENTATION {  RESIZE_INACTIVE=-1,
                           RESIZE_COL,
                           RESIZE_ROW
};

// indicates what the zoom level is (note that zoom levels < 100% trigger multi-page viewing)
enum ZOOM_STATE {
    ZOOM_STATE_4H_4V=0,         // 4 horizontal pages x 4 vertical pages
    ZOOM_STATE_4H_3V=1,
    ZOOM_STATE_3H_3V=2,
    ZOOM_STATE_3H_2V=3,
    ZOOM_STATE_2H_2V=4,
    ZOOM_STATE_2H_1V=5,
    ZOOM_STATE_100_PERCENT=6,   // full size
    ZOOM_STATE_125_PERCENT=7,
    ZOOM_STATE_150_PERCENT=8,
    ZOOM_STATE_175_PERCENT=9,
    ZOOM_STATE_200_PERCENT=10,
    ZOOM_STATE_225_PERCENT=11,
    ZOOM_STATE_250_PERCENT=12,
    ZOOM_STATE_275_PERCENT=13,
    ZOOM_STATE_300_PERCENT=14,
};

const int PRTVIEW_HITTEST_BORDER = 8;

/////////////////////////////////////////////////////////////////////////////
//
//                             CTblPrtViewHitOb
//
// For hit testing objects in a print view
//
/////////////////////////////////////////////////////////////////////////////
class CTblPrtViewHitOb
{
public:
    CTblPrtViewHitOb() { m_iPg=NONE; m_iPgOb=NONE; }
    CTblPrtViewHitOb(const CTblPrtViewHitOb& h) {
        m_iPg=h.m_iPg;
        m_iPgOb=h.m_iPgOb;
    }

public:
    int GetPg() const { return m_iPg; }
    int GetPgOb() const { return m_iPgOb; }
    void SetPg(int iPg) { m_iPg=iPg; }
    void SetPgOb(int iPgOb) { m_iPgOb=iPgOb; }

    const CTblPrtViewHitOb& operator=(const CTblPrtViewHitOb& h) {
        m_iPg=h.m_iPg;
        m_iPgOb=h.m_iPgOb;
        return *this;
    }
    bool operator==(const CTblPrtViewHitOb& h)  {
        return (m_iPg==h.GetPg() && m_iPgOb==h.GetPgOb());
    }

private:
    int     m_iPg;     // page object hit (index into CPgMgr.GetPgLayout), NONE otherwise
    int     m_iPgOb;   // page object hit (index into CPgLayout::GetPgOb), NONE otherwise
};




/////////////////////////////////////////////////////////////////////////////
//
//                             CPgOb
//
// Object to be shown in a page view
//
/////////////////////////////////////////////////////////////////////////////
class CPgOb : public CObject
{
DECLARE_DYNAMIC(CPgOb)

// Attributes
protected:
    CRect               m_rcClientLP;    // coordinates for this object; in TWIPS (logical units), relative to top/left corner of the page
    CRect               m_rcClientDP;    // coordinates for this object, in device units based on most recent rendering on screen
    CSize               m_szExtraLP;     // portion of rcClientLP that comes from "extra" allocation by the layout heuristics
    CSize               m_szMinResize;   // minimum width or height that the object can be resized to
    CSize               m_szMaxResize;   // maximum width or height that the object can be resized to
    bool                m_bCustom;       // true if this object's size (m_rcClientXX) has been customized, usually via user resizing
    bool                m_bPageBreak;    // true if layout should force a hard page break below (stubs) or to the right (col headers) of this object
    CTblBase*           m_pTblBase;      // the tbl object to display
    int                 m_iTbl;          // table for this object
    int                 m_iHPage;        // horizontal page (0=leftmost, ...), or NONE if not applicable
    int                 m_iVPage;        // vertical page (0=topmost, ...), or NONE if not applicable
    UINT                m_uFormat;       // format flags (see CDC::DrawText)
    PAGEOB_TYPE         m_ePgObType;     // page object type
    CIMSAString         m_sText;         // text to display
    CFmt*               m_pPgObFmt;      // format attributes for this pgob (resolved so that it does not contain any defaults)
    bool                m_bHiddenFlag;   //Used for the buildandmeasurecell
    bool                m_bZeroCell;     // if this is cell containing zero value - used for hide if zero

// Construction
public:
    CPgOb();
    CPgOb(const CPgOb& p);
    virtual ~CPgOb();

// Operations
public:
    CRect GetClientRectLP() const { return m_rcClientLP; }
    CRect& GetClientRectLP() { return m_rcClientLP; }
    void SetClientRectLP(const CRect& rcClientLP) { m_rcClientLP = rcClientLP; }

    CRect GetClientRectDP() const { return m_rcClientDP; }
    CRect& GetClientRectDP() { return m_rcClientDP; }
    void SetClientRectDP(const CRect& rcClientDP) { m_rcClientDP = rcClientDP; }

    CSize GetMinResize() const { return m_szMinResize; }
    CSize& GetMinResize() { return m_szMinResize; }
    void SetMinResize(const CSize& szMinResize) { m_szMinResize = szMinResize; }

    CSize GetMaxResize() const { return m_szMaxResize; }
    CSize& GetMaxResize() { return m_szMaxResize; }
    void SetMaxResize(const CSize& szMaxResize) { m_szMaxResize = szMaxResize; }

    CSize GetExtraLP() const { return m_szExtraLP; }
    CSize& GetExtraLP() { return m_szExtraLP; }
    void SetExtraLP(const CSize& szExtraLP) { m_szExtraLP = szExtraLP; }
    void SetExtraLP(int cx, int cy) { m_szExtraLP=CSize(cx,cy); }

    bool IsCustom() const { return m_bCustom; }
    void SetCustom(bool bCustom=true) { m_bCustom=bCustom; }

    bool IsPageBreak() const { return m_bPageBreak; }
    void SetPageBreak(bool bPageBreak=true) { m_bPageBreak=bPageBreak; }

    CTblBase* GetTblBase() const { return m_pTblBase; }
    void SetTblBase(CTblBase* pTblBase) { m_pTblBase = pTblBase; }

    bool GetHiddenFlag()const  { return m_bHiddenFlag;}
    void SetHiddenFlag(bool bFlag) { m_bHiddenFlag =bFlag;}

    CFmt* GetFmt() const { return m_pPgObFmt; }
    void SetFmt(CFmt* pPgObFmt) {
        SAFE_DELETE(m_pPgObFmt);
        m_pPgObFmt=pPgObFmt;
    }

    int GetTbl() const { return m_iTbl; }
    void SetTbl(int iTbl) { m_iTbl=iTbl; }

    int GetHPage() const { return m_iHPage; }
    void SetHPage(int iHPage) { m_iHPage=iHPage; }

    int GetVPage() const { return m_iVPage; }
    void SetVPage(int iVPage) { m_iVPage=iVPage; }

    UINT GetDrawFormatFlags() const { return m_uFormat; }
    void SetDrawFormatFlags(UINT uFormat) { m_uFormat = uFormat | DT_NOPREFIX; }

    PAGEOB_TYPE GetType() const { return m_ePgObType; }
    void SetType(PAGEOB_TYPE ePgObType) { m_ePgObType = ePgObType; }

    const CIMSAString& GetText() const { return m_sText; }
    void SetText(const CIMSAString& sText) { m_sText = sText; }

    bool GetIsZeroCell() const {return m_bZeroCell;}
    void SetIsZeroCell(bool b) {m_bZeroCell = b;}

// Operators:
public:
    bool operator==(const CPgOb& p) const;
    bool operator!=(const CPgOb& p) const;
    CPgOb& operator=(const CPgOb& p);

// debug support
    virtual void Dump(CDumpContext& dc) const {
        CObject::Dump(dc);
        dc << _T("CPgOb text = ") << m_sText;
    }

};

class CRowColPgOb;

/////////////////////////////////////////////////////////////////////////////
//
//                             CRowColPgOb
//
// A CPgOb object that has children.  Used to represent hierarchies for
// sets of stubs or columns.
//
/////////////////////////////////////////////////////////////////////////////
class CRowColPgOb : public CPgOb
{
// Attributes
public:
    CArray<CRowColPgOb*, CRowColPgOb*>  m_aChildren;
    CRowColPgOb*                        m_pParent;   // my parent
    int                                 m_iLevel;   // depth level
    bool                                m_bHideRowIfAllZero;

// Construction
public:
    CRowColPgOb();
    CRowColPgOb(const CRowColPgOb& rcp);
    virtual ~CRowColPgOb();

// Operations
public:
    int GetNumChildren() const { return m_aChildren.GetSize(); }
    void AddChild(CRowColPgOb* p);
    void InsertChildAt(int iIndex, CRowColPgOb* pChild, int iCount=1);
    void RemoveChildAt(int i);
    void RemoveChild(CRowColPgOb* pChild);
    void RemoveAllChildren();
    int GetChildIndex(CRowColPgOb* pChild) const;
    CRowColPgOb* GetChild(int iIndex) const { return m_aChildren[iIndex]; }

    CRowColPgOb* GetParent() { return m_pParent; }
    void SetParent(CRowColPgOb* pParent) { m_pParent = pParent; }

    int GetLevel() const { return m_iLevel; }
    void SetLevel(int iLevel) { m_iLevel=iLevel; }

    int GetDepth() const;
    int GetNumLeaves() const;
    CRowColPgOb* GetLeaf(int iIndex);
    void GetLeaves(CArray<CRowColPgOb*, CRowColPgOb*>& aLeaves,bool bIncludeHidden=false);
    void GetLeavesAndNodes(CArray<CRowColPgOb*, CRowColPgOb*>& aLeaves,bool bIncludeHidden=false);

    int GetMaxWidthLP(int iLevel=NONE) const;
    int GetMaxHeightLP(int iLevel) const;
    int GetSumWidthLP() const;
    int GetSumHeightLP() const;

    void CalcMinSizes(CDC& dc, bool bForceRemeasure=false);
    int CalcColeadSectionLayoutWidth();
    void CalcColHeadWidths(long lInitColWidth, CDC& dc, bool bForceRemeasure=false);
    int CalcSpannerWidths(int iExtraSpace=0);
    void AlignColumnsHorz(CPoint ptTopLeft, int iHPage=NONE, int iVPage=NONE);
    void CalcBoxheadHeights(CDC& dc);
    void PaginateHiddenColHeads(CArray<CRowColPgOb*, CRowColPgOb*>& aColHead);
    int PaginateSpanners();
    void SetColHeight(int iLevel, int iHeight);
    void CalcStubWidth(int& iInitRowWidth, int iFieldSpannerWidth, CDC& dc, bool bForceRemeasure=false);

    void SetStubSizes(const CSize& szLP, bool bRecurse=false, bool bForceRemeasure=false);

    // applies to stubs only - set in BuildAndMeasureCells if hide if all zero flag
    // is set and the associated row is all zero
    bool GetHideRowForAllZeroCells() const;
    void SetHideRowForAllZeroCells(bool b);

// Operators:
public:
    bool operator==(const CRowColPgOb& rcp) const;
    bool operator!=(const CRowColPgOb& rcp) const;
    CRowColPgOb& operator=(const CRowColPgOb& rcp);

// debug
    void DebugCol();
};


/////////////////////////////////////////////////////////////////////////////
//
//                             CTabPrtViewCellInfo
//
// Holds info about the data contained in cells.  Used for transforming
// data, since SerPro always separates counts from percents and we usually
// want them integrated.
//
// See BuildAndMeasureCells() and ApplyTransformation().
//
/////////////////////////////////////////////////////////////////////////////
class CTabPrtViewCellInfo
{
public:
    CTabPrtViewCellInfo(CPgOb* pPgOb=NULL, double dData = PRT_VIEW_MISSING) {
        m_dData=dData;
        m_pPgOb=pPgOb;
    }
    CTabPrtViewCellInfo(const CTabPrtViewCellInfo& x) {
        *this=x;
    }

public:
    double GetData() const { return m_dData; }
    CPgOb* GetPgOb() const { return m_pPgOb; }
    void SetData(double dData) { m_dData=dData; }
    void SetPgOb(CPgOb* pPgOb) { m_pPgOb=pPgOb; }

    const CTabPrtViewCellInfo& operator=(const CTabPrtViewCellInfo& x) {
        m_dData=x.GetData();
        m_pPgOb=x.GetPgOb();
        return *this;
    }
    bool operator==(const CTabPrtViewCellInfo& x)  {
        return (m_dData==x.GetData() && m_pPgOb==x.GetPgOb());
    }

private:
    double      m_dData;     // data associated with the page ob
    CPgOb*      m_pPgOb;     // ptr to the page ob
};


/////////////////////////////////////////////////////////////////////////////
//
//                             CPgLayout
//
// A pageful of objects, and other page-specific information
//
/////////////////////////////////////////////////////////////////////////////
class CPgLayout
{
// Attributes
protected:
    CArray<CPgOb, CPgOb&>       m_aPgOb;            // objects for this page
    float                       m_fPgWidthInches;   // page width in inches (incorporates paper size and orientation)
    float                       m_fPgHgtInches;     // page height in inches (incorporates paper size and orientation)
    CRect                       m_rcUserLP;         // page's client rect (in TWIPS, logical units), taking margins into consideration
    CRect                       m_rcUserDP;         // page's client rect (in device units)
    int                         m_iPrintedPageNum;  // printed page number (since these are user-configurable)

// Construction
public:
    CPgLayout();
    CPgLayout(const CPgLayout& p);

// Operations
public:
    CPgOb& GetPgOb(int iIndex) { return m_aPgOb.ElementAt(iIndex); }
    const CPgOb& GetPgOb(int iIndex) const { return m_aPgOb.GetAt(iIndex); }
    int AddPgOb(CPgOb& p) { ASSERT(p.GetType()!=PGOB_ROOT);    return m_aPgOb.Add(p); }
    void AddRowColPgOb(CRowColPgOb& rcpgob, bool bAddBoxheads, int iHPage=NONE, int iVPage=NONE);
    int GetNumPgObs() const { return m_aPgOb.GetSize(); }
    void RemovePgObAt(int iIndex) { m_aPgOb.RemoveAt(iIndex); }

    const CRect GetUserAreaLP() const { return m_rcUserLP; }
    CRect& GetUserAreaLP() { return m_rcUserLP; }
    void SetUserAreaLP(const CRect& rcUserLP) { m_rcUserLP = rcUserLP; }

    const CRect GetUserAreaDP() const { return m_rcUserDP; }
    CRect& GetUserAreaDP() { return m_rcUserDP; }
    void SetUserAreaDP(const CRect& rcUserDP) { m_rcUserDP = rcUserDP; }

    float GetPgWidthInches() const { return m_fPgWidthInches; }
    void SetPgWidthInches(float fPgWidthInches) { m_fPgWidthInches = fPgWidthInches; }

    float GetPgHgtInches() const { return m_fPgHgtInches; }
    void SetPgHgtInches(float fPgHgtInches) { m_fPgHgtInches = fPgHgtInches; }

    LINE GetLinePrecedence(LINE lineA, LINE lineB) const;
    void CoordinateLines(LINE eBorderLeft, LINE eBorderTop, LINE eBorderRight, LINE eBorderBottom);  // arbitrates lines at intersections; tbl borders are passed in

    int GetPrintedPageNum() const { return m_iPrintedPageNum; }
    void SetPrintedPageNum(int iPrintedPageNum) { m_iPrintedPageNum=iPrintedPageNum; }

// Operators:
public:
    CPgLayout& operator=(const CPgLayout& p);
};


/////////////////////////////////////////////////////////////////////////////
//
//                             CPgMgr
//
// Manages all the pages to display in the view
//
/////////////////////////////////////////////////////////////////////////////
class CPgMgr
{
// Attributes:
protected:
    CArray<CPgLayout, CPgLayout&>   m_aPgLayout;   // layout info for a page

// Construction
public:
    CPgMgr();

// Operations
public:
    CPgLayout& GetPgLayout(int iIndex) { return m_aPgLayout[iIndex]; }
    const CPgLayout& GetPgLayout(int iIndex) const { return m_aPgLayout[iIndex]; }
    int AddPgLayout(CPgLayout& p) { return m_aPgLayout.Add(p); }
    int GetNumPages() const { return m_aPgLayout.GetSize(); }
    void RemoveAllPages() { m_aPgLayout.RemoveAll(); }
    void RemovePageAt(int iIndex) { m_aPgLayout.RemoveAt(iIndex); }
};




/////////////////////////////////////////////////////////////////////////////
//
//                  CTabPrtView view
//
// The print view, kind of like "print preview" in most apps.
//
/////////////////////////////////////////////////////////////////////////////
class CLASS_DECL_ZTABLEF CTabPrtView : public CView
{
protected:
    DECLARE_DYNCREATE(CTabPrtView)

// Attributes
public:

    // page manager (each entry holds all the objects to render a page)
    CPgMgr                m_pgMgr;
    int                   m_iCurrPrintPg;       // current page being printed
    CArray<int,int>       m_aSelTblsPages;      // pages to print when printing only selected tables

    // multi-page viewing support
    CArray<int,int>       m_aiViewPg;           // array of pages that are being shown concurrently
    CSize                 m_szPagesToView;      // number of pages to show concurrently if zooming<100% (cx for across, cx for down)
    bool                  m_bBookLayout;        // true if multi-page layout should appear as if for a book: even pages on left, odd pages on right.
                                                //   note that book layout only applies if facing pages are being viewed (2 across, 1 down)
    // scroll bar support
    CScrollBar*           m_pVSBar;             // vertical scroll bar (we do these ourselves)
    CScrollBar*           m_pHSBar;             // horizontal scroll bar
    CSize                 m_szSBar;             // scroll bar sizes (width, height)
    bool                  m_bInitialized;

    // zoom support
//    float                 m_fZoomFactor;        // zooming factor; ex: 1.0=no zooming, 2.0=200%, 0.5=50%
    ZOOM_STATE            m_eZoomState;         // current zoom state
    CArray<int,int>       m_aiSaveViewPg;       // un-zoomed m_aiViewPg, backed up when zooming is active (since zooming is 1 page at a time only)
    CSize                 m_szSavePagesToView;  // number of pages to show concurrently if not zooming (cx for across, cx for down)

    // selection and hit testing support
    CArray<CTblPrtViewHitOb, CTblPrtViewHitOb&>
                          m_aSelected;          // currently selected objects
    // resizing support
    CTblPrtViewHitOb      m_ResizeOb;           // object being resized
    RESIZE_ORIENTATION    m_eResizeOrientation; // indicates if we are resizing a row or column
    int                   m_iCurResizePosDP;    // location of the resizing bar (not necessarily same as the mouse), or NONE, in device units
    int                   m_iMinResizePosDP;    // min resize position -- left/top, in device units
    int                   m_iMaxResizePosDP;    // max resize position -- right/bottom, in device units
    bool                  m_bApplyAcrossPanels; // =true if resizing & related operations apply to stubs/columns across panels

    // scaling factor and drawing support
private:
    int                   m_iScaleFactor;       // scaling precision --
                                                //     1=very quick (nice fonts, very fast, poor layout) ...
                                                //     10=a lot (awkward font rendering, slow, perfect layout)
    int                   m_iLogPixelsY;        // screen DC logpixels, for converting logfont height from logical units to points

    CDC*                  m_pPrtDC;             // printer DC
    bool                  m_bForceRemeasure;    // =true if we need to remeasure min/max sizes (following printer change)
    bool                  m_bAutoFitColumns;    // =true if we autofit columns across each horz page (using extra space stuff)
    CTabSet*              m_pTabSet; //Savy (R) sampling app 20081224

// Construction
public:
    CTabPrtView();

    CTabPrtView(CTabSet* pTabSet);//Today SAvy changes 20081224
// page layout functions
public:
    CPrtViewNavigationBar*  m_pNavBar;
    void Build(bool bPreserveCurrentPgViewInfo=false);
    void BuildHeaders(CPgLayout& pl, int iTbl, CDC& dc);
    void BuildFooters(CPgLayout& pl, int iTbl, CDC& dc);
    void BuildTitles(CPgLayout& pl, int iTbl, CDC& dc, int iPage=1);
    void BuildNotes(CPgLayout& pl, int iTbl, CDC& dc, int iPage=1);
    bool BuildRowsAndCols(const CPgLayout& plTemplate, CPgMgr& pgMgr, int iTbl, CDC& dc);
    void BuildAndMeasureCells(int iTbl, CDC& dc, CArray<CRowColPgOb*, CRowColPgOb*>& aColHead, CArray<CRowColPgOb*, CRowColPgOb*>& aStub, CArray<CPgOb, CPgOb&>& aCell);
    void ApplyTransformation(CArray<CRowColPgOb*, CRowColPgOb*>& aColHead, CArray<CRowColPgOb*, CRowColPgOb*>& aStub, CArray<CTabPrtViewCellInfo, CTabPrtViewCellInfo&>& aCellInfo);
    void DoRowTransformation(CArray<CRowColPgOb*, CRowColPgOb*>& aColHead,CArray<CRowColPgOb*, CRowColPgOb*>& aStub, CArray<CTabPrtViewCellInfo, CTabPrtViewCellInfo&>& aCellInfo,int iStartStub);

    void SplitSpanner(CRowColPgOb& rcpgobColRoot, int iColHead);
    void EquallyDivideColHeads(int iTbl, int iHPage, int iHorzColArea, CArray<CRowColPgOb*, CRowColPgOb*>& aColHead);
    bool ColHeadOrphanProtect(int iColHead, int iHorzColArea, CArray<CRowColPgOb*, CRowColPgOb*>& aColHead) const;
    int AlignColumnsVert(CArray<CRowColPgOb*, CRowColPgOb*>& aColHead, CRowColPgOb& rcpgobColRoot);

    void ResetPageViewLayout();
    void BuildRowColTree(CTabVar* pTabVar, CRowColPgOb* pRCParent, const CFmtReg& fmtReg, bool bColumn, int iLevel=0);
    void ProcessAreaTokensinRows(CTable* pTbl, CArray<CRowColPgOb*, CRowColPgOb*>& aStub);
    void SetDrawFormatFlags(CRowColPgOb* pOb);

    void ForceRemeasure() { m_bForceRemeasure=true; }

// format stuff
public:
    void GetDataCellFormat(CRowColPgOb* pColHead, CRowColPgOb* pStub, CDataCellFmt* pDataCellFmt, int iColHead, int iPanel) const;
    int GetRowPanel(CArray<CRowColPgOb*, CRowColPgOb*>& aStub, int iStub) const;
//    int GetRowPanel(CRowColPgOb* pOb) const;

// resizing support
public:
    bool IsResizing() const { return (m_eResizeOrientation!=RESIZE_INACTIVE); }
    RESIZE_ORIENTATION GetResizeOrientation() const { return m_eResizeOrientation; }
    void SetResizeOrientation(RESIZE_ORIENTATION eResizeOrientation) { m_eResizeOrientation=eResizeOrientation; }

    CTblPrtViewHitOb& GetResizeHitOb() { return m_ResizeOb; }
    const CTblPrtViewHitOb& GetResizeHitOb() const { return m_ResizeOb; }
    void SetResizeOb(const CTblPrtViewHitOb& resizeOb) { m_ResizeOb=resizeOb; }

    void StartResizeCol(const CTblPrtViewHitOb& obHit);
    void StartResizeRow(const CTblPrtViewHitOb& obHit);
    void EndResize();
    void DoResize(const CSize& szAmount);

// Misc functions, helper functions
public:

    UINT GetAlignmentFlags(const CFmt* pFmt) const;
    int CalcDrawHeight(const CPgOb& pgob, CDC& dc, int* piLengthDrawn=NULL) const;
    int GetHorzColArea(int iPageWidth, const CTblPrintFmt& fmtTblPrint, int iStubWidth, int iSecondaryStubheadWidth, int iHPage);

    int GetCurrFirstViewPg() const;
    int GetCurrLastViewPg() const;
    int GetNumViewPgs() const { return m_aiViewPg.GetSize(); }
    int AddViewPg(int iViewPg) { return m_aiViewPg.Add(iViewPg); }

    int GetNumHorzPgs() const { return m_szPagesToView.cx; }
    int GetNumVertPgs() const { return m_szPagesToView.cy; }

    bool GetApplyAcrossPanels() const { return m_bApplyAcrossPanels; }
    void SetApplyAcrossPanels(bool bApplyAcrossPanels) { m_bApplyAcrossPanels=bApplyAcrossPanels; }

    bool GetAutoFitColumns() const { return m_bAutoFitColumns; }
    void SetAutoFitColumns(bool bAutoFitColumns) { m_bAutoFitColumns=bAutoFitColumns; }

    ZOOM_STATE GetZoomState() const { return m_eZoomState; }
    void SetZoomState(ZOOM_STATE eZoomState) { m_eZoomState=eZoomState; UpdateZoomCombo(); }
    void SetZoomState(const CSize& szPagesToView);
    CComboBox* GetZoomComboBox();
    void InitZoomCombo();
    void UpdateZoomCombo();
    void Magnify(const CTblPrtViewHitOb& obHit);
    float GetZoomScaleFactor() const;
    int GetLogPixelsY() const { ASSERT(m_iLogPixelsY!=NONE);  return m_iLogPixelsY; }

    int GetHitTestBorder() const { return (int)((float)PRTVIEW_HITTEST_BORDER/GetZoomScaleFactor()); }

    bool IsBookLayout() const { return (m_bBookLayout && GetNumHorzPgs()==2 && GetNumVertPgs()==1); }
    void SetBookLayout(bool bBookLayout=true);

    CPrtViewNavigationBar& GetNavBar();

// Hit testing
public:
    PRTVIEW_HITTEST HitTest(const CPoint& ptDPTest, CTblPrtViewHitOb& obHit) const;

// scrolling
public:
    void GotoTbl(int iTbl, bool bRedraw = true);
    void GotoArea(int iTbl, const CString& sArea, bool bRedraw = true);
    CString GetFirstAreaOnPage(int iPg);
    void GotoPage(int iPage, bool bRedraw = true);
    CPoint GetScrollPos() const { return CPoint(m_pHSBar->GetScrollPos(), m_pVSBar->GetScrollPos()); }
    void GetPageRangeForTable(int iTbl, int& iPgStart, int& iPgEnd);
    int GetPagesPerScreen() const;

// undo/redo support
private:
    UndoStack<std::shared_ptr<CPrtViewCommand>> m_undoStack;

public:
    void UndoBoxheadResize(CArray<CResizeCmdInfo, CResizeCmdInfo&>& aResizeCmdInfo, int iTbl);
    void UndoStubResize(CArray<CResizeCmdInfo, CResizeCmdInfo&>& aResizeCmdInfo, int iTbl);
    void UndoStubHeadResize(CArray<CResizeCmdInfo, CResizeCmdInfo&>& aResizeCmdInfo, int iTbl);
    void UndoFormatPrint(CPrintFormatCmdInfo& formatInfo, int iTbl);
    void PushBoxheadResizeForUndo(int iTbl, bool bUndo=true);
    void PushStubResizeForUndo(int iTbl, bool bUndo=true);
    void PushRestoreDefaultsForUndo(int iTbl, bool bUndo=true);
    void PushFormatPrintCommand(int iTbl, bool bUndo=true);

// Overrides
public:
    void DoInitUpdate() { OnInitialUpdate(); }
    void PrepareMapMode(CDC* pDC, int iPage);
    bool PreparePrinterDC();
    void ResizeScrollBars();
    void UpdateScrollBarRanges();
    void DrawPage(CDC* pDC, int iPage);
    void PrintPage(CDC* pDC, int iPage);
    void AlignVertical(const CString& sText, CRect& rcDraw, UINT& uFormat, CDC* pDC);
    void PutBorders(CDC* pDC, CFmt* pFmt, const CRect& rcOb);
    void PutLeadering(CDC* pDC, const CPgOb& pgob, const CIMSAString& sText, const CRect& rcDraw, const int iDrawHgt);

    // ClassWizard generated virtual function overrides
    //{{AFX_VIRTUAL(CTabPrtView)
    public:
    virtual void OnPrepareDC(CDC* pDC, CPrintInfo* pInfo = NULL);
    protected:
    virtual void OnDraw(CDC* pDC);      // overridden to draw this view
    virtual void OnInitialUpdate();     // first time after construct
    virtual void OnPrint(CDC* pDC, CPrintInfo* pInfo);
    virtual BOOL OnPreparePrinting(CPrintInfo* pInfo);
    virtual void OnEndPrinting(CDC* pDC, CPrintInfo* pInfo);
    virtual void OnBeginPrinting(CDC* pDC, CPrintInfo* pInfo);
    //}}AFX_VIRTUAL


// Implementation
protected:
    virtual ~CTabPrtView();
#ifdef _DEBUG
    virtual void AssertValid() const;
    virtual void Dump(CDumpContext& dc) const;
#endif

    // Generated message map functions
    //{{AFX_MSG(CTabPrtView)
    afx_msg void OnSize(UINT nType, int cx, int cy);
    afx_msg void OnHScroll(UINT nSBCode, UINT nPos, CScrollBar* pScrollBar);
    afx_msg void OnVScroll(UINT nSBCode, UINT nPos, CScrollBar* pScrollBar);
    afx_msg BOOL OnEraseBkgnd(CDC* pDC);
    afx_msg void OnPaint();
    afx_msg void OnKeyDown(UINT nChar, UINT nRepCnt, UINT nFlags);
    afx_msg BOOL OnSetCursor(CWnd* pWnd, UINT nHitTest, UINT message);
    afx_msg void OnViewFacing();
    afx_msg void OnViewBookLayout();
    afx_msg void OnUpdateViewZoom(CCmdUI* pCmdUI);
    afx_msg void OnUpdateViewFacing(CCmdUI* pCmdUI);
    afx_msg void OnUpdateViewBooklayout(CCmdUI* pCmdUI);
    afx_msg void OnLButtonDown(UINT nFlags, CPoint point);
    afx_msg BOOL OnMouseWheel(UINT nFlags, short zDelta, CPoint pt);
    //}}AFX_MSG
    LONG OnZoom(WPARAM wParam, LPARAM lParam);
    void OnEditTablePrintFmt();
    void OnEditColBreak();
    void OnEditStubBreak();
    void OnEditApplyAcrossPanels();
    void OnEditAutoFitColumns();
    void OnRestorePrtViewDefaults();
    void OnEditUndo();
    void OnEditRedo();
    void OnUpdateEditUndo(CCmdUI* pCmdUI);
    void OnUpdateEditRedo(CCmdUI* pCmdUI);
    void OnViewFirstPage();
    void OnViewLastPage();
    void OnViewPrevPage();
    void OnViewNextPage();
    void OnGoto();
    void GetCellFormat4Captions(CRowColPgOb* pColHead,CRowColPgOb* pStub, CFmt* pCellFmt, int iColHead, int iPanel) const ;

    void GetLineFmt4NonDataCell(CRowColPgOb* pRCPgOb , const CFmtReg& fmtReg, bool bColumn,FMT_ID eGridComp, CFmt& retFmt);
    void FixLineFmt(CRowColPgOb* pRCPgOb , const CFmtReg& fmtReg, bool bColumn);

    bool ForceHideAreaCaptionInOneRowTable(CTable* pTbl);
    void OnUpdateViewFirstPage(CCmdUI* pCmdUI);
    void OnUpdateViewLastPage(CCmdUI* pCmdUI);
    void OnUpdateViewPrevPage(CCmdUI* pCmdUI);
    void OnUpdateViewNextPage(CCmdUI* pCmdUI);
    DECLARE_MESSAGE_MAP()

public:
    void UpdateViewZoom(CCmdUI* pCmdUI) { OnUpdateViewZoom(pCmdUI); }
    void UpdateViewFacing(CCmdUI* pCmdUI) { OnUpdateViewFacing(pCmdUI); }
    void UpdateViewBooklayout(CCmdUI* pCmdUI) { OnUpdateViewBooklayout(pCmdUI); }

    afx_msg void OnContextMenu(CWnd* /*pWnd*/, CPoint /*point*/);
    afx_msg void OnLButtonUp(UINT nFlags, CPoint point);
    afx_msg void OnMouseMove(UINT nFlags, CPoint point);
    afx_msg void OnSelChangeZoomComboBox();

    virtual BOOL PreTranslateMessage(MSG* pMsg);
    afx_msg void OnViewZoomIn();
    afx_msg void OnViewZoomOut();
    afx_msg void OnPrintViewClose();

};

//{{AFX_INSERT_LOCATION}}
// Microsoft Visual C++ will insert additional declarations immediately before the previous line.
