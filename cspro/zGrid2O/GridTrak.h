#pragma once

#include <zGrid2O/GridHit.h>

// GridTrak.h
// for customized rect tracker, used to move stuff inside the grid

/////////////////////////////////////////////////////////////////////////////////
//
//      class CGridRectTracker : public CRectTracker
//
/////////////////////////////////////////////////////////////////////////////////
class CGridRectTracker : public CRectTracker
{
public:
    enum TrackObject { trackField, trackBox, trackText, trackMultiple };

// Construction
public:
    CGridRectTracker() {
        m_nStyle = CRectTracker::hatchedBorder;
        m_bAllowInvert=FALSE;
        m_bFirstTime=false;
        m_bAlwaysAllowSizing =false;
    }

    CGridRectTracker(LPCRECT lpSrcRect, UINT nStyle=CRectTracker::hatchedBorder) : CRectTracker(lpSrcRect, nStyle)  {
        m_bAllowInvert=FALSE;
        m_bFirstTime=false;
        m_bAlwaysAllowSizing =false;
    }
    CGridRectTracker(const CGridRectTracker& t)  {
        m_rcBounding = t.GetBoundingRect();
        RemoveNoOverlapRects();
        for (int i=0 ; i<t.GetNumNoOverlapRects() ; i++)  {
            AddNoOverlapRect(t.GetNoOverlapRect(i));
        }
        m_hitOb = t.GetHitOb();
        m_trackOb = t.GetTrackObject();
        m_bFirstTime=t.IsFirstTime();

        // base class members
        m_nStyle = t.m_nStyle;
        m_rect = t.m_rect;
        m_sizeMin = t.m_sizeMin;
        m_nHandleSize = t.m_nHandleSize;
        m_bAllowInvert = t.m_bAllowInvert;
        m_rectLast = t.m_rectLast;
        m_sizeLast = t.m_sizeLast;
        m_bErase = t.m_bErase;
        m_bFinalErase = t.m_bFinalErase;
        m_bAlwaysAllowSizing =t.m_bAlwaysAllowSizing;
    }

// Operations
public:
    // bounding rectangle (usually the cell)
    CRect& GetBoundingRect(void) { return m_rcBounding; }
    CRect GetBoundingRect(void) const { return m_rcBounding; }
    void SetBoundingRect(const CRect& rcBounding) { m_rcBounding = rcBounding; }

    // rects that we can't overlap (like other fields in the cell)
    int GetNumNoOverlapRects(void) const { return m_arcNoOverlap.GetSize(); }
    CRect& GetNoOverlapRect(int i) { return m_arcNoOverlap.ElementAt(i); }
    CRect GetNoOverlapRect(int i) const { return m_arcNoOverlap.GetAt(i); }
    void AddNoOverlapRect(CRect rc) { m_arcNoOverlap.Add(rc); }
    void RemoveNoOverlapRects(void) { m_arcNoOverlap.RemoveAll(); }

    // info on what/where we're tracking
    TrackObject GetTrackObject(void) const { return m_trackOb; }
    void SetTrackObject(TrackObject trackOb) { m_trackOb = trackOb; }
    CHitOb& GetHitOb(void) { return m_hitOb; }
    CHitOb GetHitOb(void) const { return m_hitOb; }
    void SetHitOb(const CHitOb& hitOb) { m_hitOb = hitOb; }

    bool IsFirstTime(void) const { return m_bFirstTime; }
    void SetFirstTime(bool bFirstTime=true) { m_bFirstTime = bFirstTime; }

public:
    const CGridRectTracker& operator=(const CGridRectTracker& t)  {
        m_rcBounding = t.GetBoundingRect();
        RemoveNoOverlapRects();
        for (int i=0 ; i<t.GetNumNoOverlapRects() ; i++)  {
            AddNoOverlapRect(t.GetNoOverlapRect(i));
        }
        m_hitOb = t.GetHitOb();
        m_trackOb = t.GetTrackObject();

        // base class members
        m_nStyle = t.m_nStyle;
        m_rect = t.m_rect;
        m_sizeMin = t.m_sizeMin;
        m_nHandleSize = t.m_nHandleSize;
        m_bAllowInvert = t.m_bAllowInvert;
        m_rectLast = t.m_rectLast;
        m_sizeLast = t.m_sizeLast;
        m_bErase = t.m_bErase;
        m_bFinalErase = t.m_bFinalErase;
        m_bAlwaysAllowSizing =t.m_bAlwaysAllowSizing;

        return *this;
    }


// Attributes
private:
    CRect                   m_rcBounding;
    CArray<CRect,CRect&>    m_arcNoOverlap;
    CHitOb                  m_hitOb;    // info on where the tracker is tracking
    TrackObject             m_trackOb;  // info on what the tracker is tracking
    bool                    m_bFirstTime; // true at start of a tracking operation
    bool                    m_bAlwaysAllowSizing; //this is to allow sizing on new unicode text fields

// Implementaton
public:
    BOOL SetCursor(CWnd* pWnd, UINT nHitTest, bool bForceNoResize) const;

    bool  IsSizingAllowed(void) const { return m_bAlwaysAllowSizing; }
    void  SetSizingOverrideFlag(bool bFlag) { m_bAlwaysAllowSizing = bFlag; }

protected:
    virtual void AdjustRect(int nHandle, LPRECT lpRect);
};
