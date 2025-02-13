#pragma once
//***************************************************************************
//  File name: DDLView.h
//
//  Description:
//       Header for Layout View application
//
//  History:    Date       Author   Comment
//              ---------------------------
//              03 Aug 00   bmd     Created for CSPro 2.1
//
//***************************************************************************


typedef enum {LAYOUT_NOHIT=-1,
              LAYOUT_ON_NAME=0,
              LAYOUT_RECTYPE,
              LAYOUT_IDITEMBEF,
              LAYOUT_IDITEM,
              LAYOUT_IDITEMAFT,
              LAYOUT_IDSUBITEM,
              LAYOUT_RTITEM,
              LAYOUT_RTSUBITEM}  LayoutType;

#define LAYCOLOR_BACKGROUND RGB(192,192,192)
#define LAYCOLOR_RULER      RGB(192,192,192)
#define LAYCOLOR_RECTYPE    RGB(255,255,  0)
#define LAYCOLOR_IDITEMBEF  RGB(255,255,255)
#define LAYCOLOR_IDITEM     RGB(191,  0,191)
#define LAYCOLOR_IDITEMAFT  RGB(255,191,255)
#define LAYCOLOR_ITEM       RGB(  0,255,255)
#define LAYCOLOR_SUBITEM    RGB(127,255,127)
#define LAYCOLOR_GAP        RGB(128,128,128)

#define LAY_BORDER             7


/////////////////////////////////////////////////////////////////////////////
//
//                            CItemPosInfo
//
/////////////////////////////////////////////////////////////////////////////

class CItemPosInfo : public CObject {

DECLARE_DYNAMIC (CItemPosInfo)

private:
    int         m_iLevel;           // Level number
    int         m_iRec;             // Record number
    int         m_iItem;            // Item number (RT = NONE,  ID = COMMON)
    int         m_iOccs;            // Item occurrence number (1-based)
    int         m_iStart;           // Starting position (1-based, just like CDictItem)
    int         m_iEnd;             // Ending position (1-based)
    int         m_iLine;            // Line where this item is shown in layout mode, global to DD level
    int         m_iIdLevel;         // if iRec==COMMON, gives the iLevel of the id item
    int         m_iIdRec;           // if iRec==COMMON, gives the iRec of the id item
    LayoutType  m_type;

public:
    // Construction
    CItemPosInfo (void);
    CItemPosInfo (int iLevel, int iRec, int iItem, int iOccs, int iStart, int iEnd,
                  int iLine, int iIdLevel, int iIdRec, LayoutType type);
    CItemPosInfo (CItemPosInfo& ipi);

    // Extraction
    int GetLevel(void) const { return m_iLevel; }
    int GetRec(void) const { return m_iRec; }
    int GetItem(void) const { return m_iItem; }
    int GetOccs(void) const { return m_iOccs; }
    int GetStart(void) const { return m_iStart; }
    int GetEnd(void) const { return m_iEnd; }
    int GetLine(void) const { return m_iLine; }
    int GetIdLevel(void) const { return m_iIdLevel; }
    int GetIdRec(void) const { return m_iIdRec; }
    LayoutType GetType(void) const { return m_type; }

    // Operators
void operator= (CItemPosInfo& ipi);
};

/////////////////////////////////////////////////////////////////////////////
//
//                            CLayItemArray
//
/////////////////////////////////////////////////////////////////////////////

class CLayItemArray : public CObject {

DECLARE_DYNAMIC (CLayItemArray)

private:
    CObArray      m_aItemPos;

public:
    // construction/destruction
    CLayItemArray (void)  {
        m_aItemPos.RemoveAll();
    }
    CLayItemArray (CLayItemArray& rpi)  {
        m_aItemPos.RemoveAll();
        for (int i = 0 ; i < rpi.m_aItemPos.GetSize() ; i++)  {
            m_aItemPos.Add(rpi.m_aItemPos[i]);
        }
    }
    ~CLayItemArray (void)  {
        for (int i = 0 ; i < m_aItemPos.GetSize() ; i++)  {
            ASSERT_VALID(m_aItemPos[i]);
            delete m_aItemPos[i];
        }
    }

    // extraction
    CItemPosInfo*& GetItemPos (int i)  { return (CItemPosInfo*&) (m_aItemPos[i]);  }
    CItemPosInfo* GetItemPos (int i) const { return (CItemPosInfo*) (m_aItemPos[i]);  }
    int GetSize(void) const { return m_aItemPos.GetSize(); }

    // assignment
    int AddItemPos (CItemPosInfo* pItemPos) {
        ASSERT (pItemPos->IsKindOf(RUNTIME_CLASS(CItemPosInfo)));
        CItemPosInfo* pNewItemPos = new CItemPosInfo(*pItemPos);
        return m_aItemPos.Add(pNewItemPos);
    }
    void RemoveAllItems (void)  {
        while (m_aItemPos.GetSize() > 0)  {
            ASSERT_VALID(m_aItemPos[0]);
            RemoveItemPosAt(0);
        }
    }
    void RemoveItemPosAt (int iIndex)  {
        CItemPosInfo* pIpi = (CItemPosInfo*) m_aItemPos[iIndex];
        m_aItemPos.RemoveAt (iIndex);
        ASSERT_VALID(pIpi);
        delete pIpi;
    }
};


/////////////////////////////////////////////////////////////////////////////
//
//                            CDDLView
//
/////////////////////////////////////////////////////////////////////////////

class CDDLView : public CScrollView {

protected:
    CDDLView();           // protected constructor used by dynamic creation
    DECLARE_DYNCREATE(CDDLView)

// Attributes
private:
    bool            m_bBuilt;
    bool            m_bInitialized;
    CFont*          m_pFont;
    CFont*          m_pFontBold;
    int             m_iTextWidth;
    int             m_iTextHgt;
    CIMSAString     m_csRuler;
    CPoint          m_ptContentsOrigin;
    CLayItemArray   m_aItemLay;

// Operations
public:
    void Init(void);
    void Build(void);

private:
    // Building
    int BuildRec(int iLevel, int iRec, int iLineDD, int iIdLevel = NONE, int iIdRec = NONE);
    void BuildRuler(void);
    int  GetRTLine(int iLevel, int iRec) const;
    int  GetItemLine (int iRec, int iItem, int iIdRec = NONE);
    bool IsOverlap (const CDictItem* pItem1, const CDictItem* pItem2) const {
        return (pItem1->GetStart() + pItem1->GetLen()*pItem1->GetOccurs() > pItem2->GetStart());
    }

    // Drawing
    void LayoutContents (CDC* pDC);
    void LayoutNames(CDC* pDC);
    void LayoutRuler(CDC* pDC);

    // Tool tip support
    virtual int OnToolHitTest(CPoint point, TOOLINFO* pTI) const;

    LRESULT OnFind(WPARAM wParam,LPARAM lParam);

    // Misc
protected:
    bool HitTest (CPoint pt, CItemPosInfo& ipi) const;

public:
// Overrides
    // ClassWizard generated virtual function overrides
    //{{AFX_VIRTUAL(CDDLView)
    public:
    virtual BOOL OnScroll(UINT nScrollCode, UINT nPos, BOOL bDoScroll = TRUE);
    protected:
    virtual void OnDraw(CDC* pDC);      // overridden to draw this view
    virtual void OnInitialUpdate();     // first time after construct
    virtual void OnUpdate(CView* pSender, LPARAM lHint, CObject* pHint);
    //}}AFX_VIRTUAL

// Implementation
protected:
    virtual ~CDDLView();
    CDDDoc* GetDocument() const { return STATIC_DOWNCAST(CDDDoc, CView::GetDocument()); }

#ifdef _DEBUG
    virtual void AssertValid() const;
    virtual void Dump(CDumpContext& dc) const;
#endif

    // Generated message map functions
    //{{AFX_MSG(CDDLView)
    afx_msg void OnContextMenu(CWnd* pWnd, CPoint point);
    afx_msg void OnLButtonUp(UINT nFlags, CPoint point);
    afx_msg void OnFilePrint();
    afx_msg BOOL OnEraseBkgnd(CDC* pDC);
    //}}AFX_MSG
    DECLARE_MESSAGE_MAP()
};

/////////////////////////////////////////////////////////////////////////////

//{{AFX_INSERT_LOCATION}}
// Microsoft Visual C++ will insert additional declarations immediately before the previous line.
