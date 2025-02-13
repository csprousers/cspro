#pragma once
// ===================================================================================
//                  Class Specification : COXRectTracker
// ===================================================================================

// Header file : OXRectTracker.h

// Copyright © Dundas Software Ltd. 1997 - 1998, All Rights Reserved
// Some portions Copyright (C)1994-5    Micro Focus Inc, 2465 East Bayshore Rd, Palo Alto, CA 94303.

// //////////////////////////////////////////////////////////////////////////

// Properties:
//  NO  Abstract class (does not have any objects)
//  YES Derived from CRectTracker

//  NO  Is a Cwnd.
//  NO  Two stage creation (constructor & Create())
//  NO  Has a message map
//  NO  Needs a resource (template)

//  NO  Persistent objects (saveable on disk)
//  NO  Uses exceptions

// //////////////////////////////////////////////////////////////////////////

// Desciption :
//
//  Subclassed CRectTracker. Used for resizing splitter windows, and COXSizeControlBars, too

// Remark:
//

// Prerequisites (necessary conditions):
//      ***

/////////////////////////////////////////////////////////////////////////////

#include <zUToolO/OXDllExt.h>
#include <zUToolO/zUtoolO.h>


// extra tracker styles...
#define RectTracker_OnlyMoveHorz        0x0100L         // only move horizontally
#define RectTracker_OnlyMoveVert        0x0200L         // only move vertically

class OX_CLASS_DECL COXRectTracker : public CRectTracker
{
// Data members -------------------------------------------------------------
public:

    CRect   m_LimitRect;                // limiting rectangle - all movement must be within this.
    CRect   m_OrigRect;                 // original position at start of track

protected:

private :

// Member functions ---------------------------------------------------------
public:
    COXRectTracker();
    // --- In  :
    // --- Out :
    // --- Returns :
    // --- Effect : Constructor of object
    //              It will initialize the internal state

    virtual void AdjustRect(int nHandle, LPRECT);
    // --- In  : See CRectTracker::AdjustRect
    // --- Out : See CRectTracker::AdjustRect
    // --- Returns : See CRectTracker::AdjustRect
    // --- Effect : See CRectTracker::AdjustRect

    virtual void DrawTrackerRect(LPCRECT lpRect, CWnd* pWndClipTo, CDC* pDC, CWnd* pWnd);
    // --- In  : See CRectTracker::DrawTrackerRect
    // --- Out : See CRectTracker::DrawTrackerRect
    // --- Returns : See CRectTracker::DrawTrackerRect
    // --- Effect : See CRectTracker::DrawTrackerRect

    BOOL TrackFromHitTest(int nHitTest, CWnd* pWnd, CPoint point,
                CWnd* pWndClipTo = NULL, BOOL bAllowInvert = FALSE);
    // --- In  : nHitTest : for values see CRectTracker::HitTest()
    //           See CRectTracker::Track()
    // --- Out :
    // --- Returns : TRUE is tracking/moving occurred, otherwise FALSE (i.e. esc pressed)
    // --- Effect : Overload for CRectTracker::Track()
    //              Instructs the rect tracker which side is tracked

    virtual ~COXRectTracker();
    // --- In  :
    // --- Out :
    // --- Returns :
    // --- Effect : Destructor of object

protected:

private :

};

// COXDragRectTracker - subclassed COXRectTracker
class OX_CLASS_DECL COXDragRectTracker : public COXRectTracker
{
// Data members -------------------------------------------------------------
public:
    virtual void DrawTrackerRect(LPCRECT lpRect, CWnd* pWndClipTo, CDC* pDC, CWnd* pWnd);

protected:

private :

// Member functions ---------------------------------------------------------
public:

protected:

private :
};
