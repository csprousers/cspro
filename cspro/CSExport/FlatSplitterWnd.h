#pragma once
/*
 * File:        FlatSplitterWnd.h
 *
 * Author:      Marc Richarme <devix@devix.cjb.net>
 *
 * Created:     28 Jan. 2001
 * Modified:    28 Jan. 2001
 *
 * Use as much as you want, wherever you want...
 * Claim you coded it, I don't mind.
 *
 */

/////////////////////////////////////////////////////////////////////////////
// CFlatSplitterWnd

class CFlatSplitterWnd : public CSplitterWnd
{
public:
    CFlatSplitterWnd();
    virtual ~CFlatSplitterWnd();

    virtual void OnDrawSplitter(CDC* pDC, ESplitType nType, const CRect& rectArg);
    virtual void RecalcLayout();    // call after changing sizes

    static void DeferClientPos(AFX_SIZEPARENTPARAMS* lpLayout,
        CWnd* pWnd, int x, int y, int cx, int cy, BOOL bScrollBar);
    static void LayoutRowCol(CSplitterWnd::CRowColInfo* pInfoArray,
        int nMax, int nSize, int nSizeSplitter);

    // ClassWizard generated virtual function overrides
    //{{AFX_VIRTUAL(CFlatSplitterWnd)
    //}}AFX_VIRTUAL

    // Generated message map functions
protected:
    //{{AFX_MSG(CFlatSplitterWnd)
    //}}AFX_MSG
    DECLARE_MESSAGE_MAP()
};

/////////////////////////////////////////////////////////////////////////////

//{{AFX_INSERT_LOCATION}}
// Microsoft Developer Studio will insert additional declarations immediately before the previous line.
