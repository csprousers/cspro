#pragma once

#include <TextView/Tvmisc.h>

//***************************************************************************
//  File name: TVBlock.h
//
//  Description:
//       Interface for the CBlockScrollView class
//
//  History:    Date       Author     Comment
//              -----------------------------
//              1995        csc       created
//              4  Jan 03   csc       Support for font size changes
//              4  Jan 03   csc       Add double-click support (select line)
//
//***************************************************************************


#define SGN(x) ((x)>0?1:((x)==0?0:-1))
typedef enum { HRFIRST, HRNEXT, VRFIRST, VRNEXT } SELRULE;

/////////////////////////////////////////////////////////////////////////////
// CBlockScrollView view

class CBlockScrollView : public CScrollView
{
    DECLARE_DYNCREATE(CBlockScrollView)
protected:
    CBlockScrollView();

// Attributes
public:

// Operations
public:

// Implementation
protected:
    virtual ~CBlockScrollView();
    virtual void OnDraw(CDC* pDC);
    virtual void OnInitialUpdate();     // first time after construct

    // Generated message map functions
    //{{AFX_MSG(CBlockScrollView)
    afx_msg void OnLButtonDown(UINT nFlags, CPoint point);
    afx_msg void OnLButtonUp(UINT nFlags, CPoint point);
    afx_msg void OnMouseMove(UINT nFlags, CPoint point);
    afx_msg void OnKeyDown(UINT nChar, UINT nRepCnt, UINT nFlags);
    afx_msg void OnTimer(UINT nIDEvent);
    afx_msg void OnVScroll(UINT nSBCode, UINT nPos, CScrollBar* pScrollBar);
    afx_msg void OnHScroll(UINT nSBCode, UINT nPos, CScrollBar* pScrollBar);
    afx_msg void OnEditSelectAll();
    afx_msg void OnLButtonDblClk(UINT nFlags, CPoint point);
    //}}AFX_MSG
    DECLARE_MESSAGE_MAP()
private:
    void UpdateBlock (void);                 // update blocked area (InvalidateRect's etc)

public:
#ifdef _DEBUG
    virtual void Dump(CDumpContext& dc) const;
#endif

private:
    BOOL      m_bBlockActive,                // TRUE if a (possibly old) block is dispalyed on the screen
              m_bCaptured,                   // TRUE if we've SetCapture()
              m_bDirty;
    CLPoint   m_ptlOrigin;                   // absolute, pixel coordinates for the block origin
    CLPoint   m_ptlDestination;              // absolute, pixel coordinates for the block destination
    CLRect    m_rclOldBlock;                 // absolute, pixel coordinates for the view
    int       m_iTimer;                      // timer number, used for scrolling while outside of the view

protected:
    int       m_iTextHgt;                    // text height (pixels)
    int       m_iTextWidth;                  // text width (pixels)
    int       m_iScrHgt;                     // number of lines displayed at a time
    int       m_iScrWidth;                   // number of characters across displayed at a time
    BOOL      m_bForceVScrollBarOverride;    // the file is too long; the vertical scrollbars have been remapped
    BOOL      m_bForceHScrollBarOverride;    // the file is too wide; the horizontal scrollbars have been remapped
    CSize     m_szTotalSize;
    CSize     m_szLineSize;                  // scroll parameters, these don't change over the life of the view...

private:
    /*--- conversion methods ---*/
    inline void InvalidateLRect (CLRect);
    inline void ValidateLRect (CLRect);
    CLPoint GetUnscaledScrollPosition (void);
    long PixelToRow (long x)    { VERIFY (m_iTextHgt != 0);    return x/m_iTextHgt;    }
    long PixelToCol (long x)    { VERIFY (m_iTextWidth != 0);  return x/m_iTextWidth;  }
    long RowToPixel (long x)    { VERIFY (m_iTextHgt != 0);    return x*m_iTextHgt;    }
    long ColToPixel (long x)    { VERIFY (m_iTextWidth != 0);  return x*m_iTextWidth;  }

    /*--- positioning methods ---*/
    void SetOrigin (CLPoint ptlX)  {
        m_ptlOrigin.x = PixelToCol (ptlX.x);
        m_ptlOrigin.y = PixelToRow (ptlX.y);
        ASSERT ( m_ptlOrigin.x >= 0 && m_ptlOrigin.y >= 0 );
    }
    void SetDestination (CLPoint ptlX)  {
        m_ptlDestination.x = PixelToCol (ptlX.x);
        m_ptlDestination.y = PixelToRow (ptlX.y);
        ASSERT ( m_ptlDestination.x >= 0 && m_ptlDestination.y >= 0 );
    }
    BOOL InwardMotion (CLPoint ptlX)  {
        return (labs(m_ptlOrigin.x-m_ptlDestination.x)>labs(m_ptlOrigin.x-PixelToCol(ptlX.x))) ||
               (labs(m_ptlOrigin.y-m_ptlDestination.y)>labs(m_ptlOrigin.y-PixelToRow(ptlX.y)));
    }
    BOOL PassedOrigin (CLRect rcNew)  {
        CLRect rclOld = PixelToChar (m_rclOldBlock);
        rcNew = PixelToChar (rcNew);
        return  ( (rclOld.left>=m_ptlOrigin.x && rcNew.right <= m_ptlOrigin.x) ||
                  (rcNew.left>=m_ptlOrigin.x && rclOld.right <= m_ptlOrigin.x) ||
                  (rclOld.top>=m_ptlOrigin.y && rcNew.bottom <= m_ptlOrigin.y) ||
                  (rcNew.top>=m_ptlOrigin.y && rclOld.bottom <= m_ptlOrigin.y) );
    }

public:
    BOOL IsBlockActive (void)  {  return m_bBlockActive;  }
    void SetClean (void)  {  m_bDirty = NO;  m_rclOldBlock = GetBlockedRectPixel();  }
    CLRect GetBlockedRectPixel (void)  {
        return CLRect( ColToPixel (std::min(m_ptlOrigin.x, m_ptlDestination.x)),
                       RowToPixel (std::min(m_ptlOrigin.y, m_ptlDestination.y)),
                       ColToPixel (std::max(m_ptlOrigin.x, m_ptlDestination.x))+m_iTextWidth,
                       RowToPixel (std::max(m_ptlOrigin.y, m_ptlDestination.y))+m_iTextHgt );
    }
    CLRect GetBlockedRectChar (void)  {
        return CLRect(std::min(m_ptlOrigin.x, m_ptlDestination.x),
            std::min(m_ptlOrigin.y, m_ptlDestination.y),
            std::max(m_ptlOrigin.x, m_ptlDestination.x),
            std::max(m_ptlOrigin.y, m_ptlDestination.y) );
    }
    CLRect PixelToChar (CLRect rclX)  {
        return CLRect ( PixelToCol (rclX.left), PixelToRow (rclX.top),
                        PixelToCol (rclX.right)-1, PixelToRow (rclX.bottom)-1);
    }
    long GetBlockedBufferSize (void)  {
        if ( m_bBlockActive )  {
            CLRect rclX = GetBlockedRectChar ();
            ASSERT ( (long) (rclX.right - rclX.left + 3)*(rclX.bottom-rclX.top+1)+1 >= 0L );              // <0 will signal overflow to us!
            ASSERT ( (double) (rclX.right - rclX.left + 3)*(rclX.bottom-rclX.top+1)+1 <= 65535*32767 );   // straightup overflow

// csc 4 Jan 03 ... exclude last CR/LF in block (thus subtracting 2 bytes)  when one line partial // BMD 25 Mar 2003
            if ((rclX.bottom != rclX.top) || rclX.left > 0) {
                return ( (long) (rclX.right - rclX.left + 3)*(rclX.bottom-rclX.top+1)+1);
            }
            else {
                return ( (long) (rclX.right - rclX.left + 3)*(rclX.bottom-rclX.top+1)+1) - 2;
            }
        }
        else  {
            return 0L;
        }
    }
    void GetBlockedBuffer (TCHAR FAR*);
    void GetBlockForSS (TCHAR FAR*);

    /*--- misc functions ---*/
    void UpdateStatusBar (void);                  // cause the main frame to update the block scroll position

    void OnSelLine (CPoint pt, SELRULE type);    // BMD 24 Mar 2003

};

/////////////////////////////////////////////////////////////////////////////
