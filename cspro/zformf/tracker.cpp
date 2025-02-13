#include "StdAfx.h"
#include "Tracker.h"


CFormTracker::CFormTracker(LPCRECT lpSrcRect/* = nullptr*/, UINT nStyle/* = CRectTracker::hatchedBorder | CRectTracker::resizeOutside*/)
    :   m_iIndex(NONE),
        m_bFldTxt(false),
        m_bFldBox(false),
        m_bBox(false),
        m_pFormView(nullptr)
{
    if( lpSrcRect != nullptr )
    {
        m_rect = lpSrcRect;
    }

    else
    {
        m_sizeMin.cx = 0;   // CRectTracker member; setting this guy to 0 allows
        m_sizeMin.cy = 0;   // the tracker box to be a line! :)
    }

    m_nStyle = nStyle;
}


CRect CFormTracker::GetTheTrueRect() const
{
    CRect r;
    GetTrueRect(r);
    return r;
}


const CRect& CFormTracker::GetRectFromFormObject(const CDEForm& form) const
{
    return m_bBox    ? form.GetBoxSet().GetBox(m_iIndex).GetDims() :
           m_bFldTxt ? assert_cast<const CDEField*>(form.GetItem(m_iIndex))->GetTextDims() :
                       form.GetItem(m_iIndex)->GetDims();
}


void CFormTracker::OnChangedRect(const CRect& /*rectOld*/)
{
    // Called by the framework whenever the tracker rectangle has changed during a call to
    // Track. At the time this function is called, all feedback drawn with DrawTrackerRect
    // has been removed.
}
