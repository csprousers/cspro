/***********************************************
    Ultimate Grid 97
    Copyright 1994 - 1997 Dundas Software Ltd.

    Class
        CUGVScroll
    Purpose
        This class controls the grid's verticle
        scrollbar and is responsible for updating
        it and making sure that it is using the
        specified scroll mode
************************************************/

#include "StdAfx.h"

#include "Ugctrl.h"

#ifdef _DEBUG
#define new DEBUG_NEW
#undef THIS_FILE
static char THIS_FILE[]= __FILE__;
#endif

/////////////////////////////////////////////////////////////////////////////
// CUGVScroll

CUGVScroll::CUGVScroll()
{
    m_lastMaxTopRow = -2;
    m_lastScrollMode = -2;
    m_lastNumLockRow = -2;
    m_trackRowPos = 0;

}

CUGVScroll::~CUGVScroll()
{
}


BEGIN_MESSAGE_MAP(CUGVScroll, CScrollBar)
    //{{AFX_MSG_MAP(CUGVScroll)
    ON_WM_RBUTTONDOWN()
    ON_WM_CREATE()
    //}}AFX_MSG_MAP

END_MESSAGE_MAP()

/////////////////////////////////////////////////////////////////////////////
// CUGVScroll message handlers

/************************************************
*************************************************/
void CUGVScroll::Update(){
    Moved();
}

/************************************************
*************************************************/
void CUGVScroll::Moved(){

    //set the scroll range
    if(m_lastMaxTopRow != m_GI->m_maxTopRow ||
        m_lastScrollMode != m_GI->m_vScrollMode ||
        m_lastNumLockRow != m_GI->m_numLockRows){

        //set the last value vars
        m_lastMaxTopRow = m_GI->m_maxTopRow;
        m_lastScrollMode = m_GI->m_vScrollMode;
        m_lastNumLockRow = m_GI->m_numLockRows;

        //set up the scrollbar if the number of rows is less than 1000
        if(m_GI->m_maxTopRow <=1000){
            if(UG_SCROLLJOYSTICK == m_GI->m_vScrollMode){
                SCROLLINFO ScrollInfo;
                ScrollInfo.cbSize =sizeof(SCROLLINFO);
                ScrollInfo.fMask = SIF_PAGE | SIF_RANGE |SIF_POS;
                ScrollInfo.nPage = 0;
                ScrollInfo.nMin = 0;
                ScrollInfo.nMax = 2;
                ScrollInfo.nPos = 1;
                SetScrollInfo(&ScrollInfo,FALSE);
                Invalidate();
            }
            else{
                SCROLLINFO ScrollInfo;
                ScrollInfo.cbSize =sizeof(SCROLLINFO);
                ScrollInfo.fMask = SIF_PAGE | SIF_RANGE;
                ScrollInfo.nPage = m_GI->m_gridHeight / m_GI->m_defRowHeight;
                ScrollInfo.nMin = m_GI->m_numLockRows;
                ScrollInfo.nMax = m_GI->m_maxTopRow + ScrollInfo.nPage -1;
                SetScrollInfo(&ScrollInfo,FALSE);
                Invalidate();
            }
            m_multiRange = 1;
            m_multiPos  = 1;
        }

        //set up the scrollbar if the number of rows is greater than 1000
        else{
            m_multiRange = (double)1000 / (double)m_GI->m_maxTopRow;
            m_multiPos   = (double)m_GI->m_maxTopRow / (double)1000;
            if(UG_SCROLLJOYSTICK==m_GI->m_vScrollMode){
                SetScrollRange(0,2,FALSE);
                m_multiRange = 1;
                m_multiPos  = 1;
            }
            else{
                SCROLLINFO ScrollInfo;
                ScrollInfo.cbSize =sizeof(SCROLLINFO);
                ScrollInfo.fMask = SIF_PAGE | SIF_RANGE;
                ScrollInfo.nPage = 1;
                ScrollInfo.nMin = m_GI->m_numLockRows;
                ScrollInfo.nMax = 1000;
                SetScrollInfo(&ScrollInfo,FALSE);
            }
        }
        if(m_GI->m_vScrollRect.left == m_GI->m_vScrollRect.right){
            m_ctrl->AdjustComponentSizes();
        }
    }

    //set the scroll pos
    if(m_GI->m_lastTopRow != m_GI->m_topRow){
        if(UG_SCROLLJOYSTICK == m_GI->m_vScrollMode)
            SetScrollPos(1,TRUE);
        else
            SetScrollPos((int)(m_GI->m_topRow * m_multiRange),TRUE);
    }
}

/************************************************
*************************************************/
void CUGVScroll::VScroll(UINT nSBCode, UINT nPos)
{

    if(GetFocus() != m_ctrl->m_CUGGrid)
        m_ctrl->m_CUGGrid->SetFocus();

    m_ctrl->m_GI->m_moveType = 4;

    switch(nSBCode){
        case SB_LINEDOWN:{
            m_ctrl->MoveTopRow(UG_LINEDOWN);
            break;
        }
        case SB_LINEUP:{
            m_ctrl->MoveTopRow(UG_LINEUP);
            break;
        }
        case SB_PAGEUP:{
            m_ctrl->MoveTopRow(UG_PAGEUP);
            break;
        }
        case SB_PAGEDOWN:{
            m_ctrl->MoveTopRow(UG_PAGEDOWN);
            break;
        }
        case SB_TOP:{
            m_ctrl->MoveTopRow(UG_TOP);
            break;
        }
        case SB_BOTTOM:{
            m_ctrl->MoveTopRow(UG_BOTTOM);
            break;
        }
        case SB_THUMBTRACK:{

            if(m_GI->m_vScrollMode==UG_SCROLLTRACKING){ //tracking
                m_ctrl->SetTopRow((long)((double)nPos * m_multiPos));
            }
            m_trackRowPos = (long)((double)nPos * m_multiPos) + m_GI->m_numLockRows;

            //scroll hint window
            #ifdef UG_ENABLE_SCROLLHINTS
            if(m_GI->m_enableVScrollHints){

                CString string;
                RECT rect;
                GetWindowRect(&rect);
                rect.top = HIWORD(GetMessagePos());
                m_ctrl->ScreenToClient(&rect);
                m_ctrl->m_CUGHint->SetWindowAlign(UG_ALIGNRIGHT|UG_ALIGNVCENTER);
                m_ctrl->m_CUGHint->SetTextAlign(UG_ALIGNCENTER);

                m_ctrl->OnVScrollHint(m_trackRowPos,&string);
                m_ctrl->m_CUGHint->SetText(string,FALSE);
                //TD - set text before move window...
                m_ctrl->m_CUGHint->MoveHintWindow(rect.left-1,rect.top,40);
                m_ctrl->m_CUGHint->Show();
            }
            #endif

            break;
        }
        case SB_ENDSCROLL:{
            break;
        }
        case SB_THUMBPOSITION:{

            #ifdef UG_ENABLE_SCROLLHINTS
            if(m_GI->m_enableVScrollHints){
                m_ctrl->m_CUGHint->Hide();
            }
            #endif

            m_ctrl->SetTopRow((long)((double)nPos * m_multiPos));

            break;
        }
    }
}

void CUGVScroll::OnRButtonDown(UINT nFlags, CPoint point)
{
    // TODO: Add your message handler code here and/or call default
    if(m_GI->m_enablePopupMenu){
        ClientToScreen(&point);
        m_ctrl->StartMenu(0,0,&point,UG_VSCROLL);
    }

    CScrollBar::OnRButtonDown(nFlags, point);
}

int CUGVScroll::OnCreate(LPCREATESTRUCT lpCreateStruct)
{
    if (CScrollBar::OnCreate(lpCreateStruct) == -1)
        return -1;

    // TODO: Add your specialized creation code here
    if(m_GI->m_enableVScrollHints)
        EnableToolTips(TRUE);

    return 0;
}
