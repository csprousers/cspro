/***********************************************
    Ultimate Grid 97
    Copyright 1994 - 1997 Dundas Software Ltd.

    Class
        CUGHScroll
    Purpose
        This class controls the grid's horizontal
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
// CUGHScroll

CUGHScroll::CUGHScroll()
{
    m_lastMaxLeftCol = -1;
    m_lastNumLockCols = -1;
    m_trackColPos = 0;

}

CUGHScroll::~CUGHScroll()
{
}


BEGIN_MESSAGE_MAP(CUGHScroll, CScrollBar)
    //{{AFX_MSG_MAP(CUGHScroll)
    ON_WM_RBUTTONDOWN()
    ON_WM_CREATE()
    //}}AFX_MSG_MAP
END_MESSAGE_MAP()

/////////////////////////////////////////////////////////////////////////////
// CUGHScroll message handlers

/************************************************
*************************************************/
void CUGHScroll::Update(){

    Moved();

}
/************************************************
*************************************************/
void CUGHScroll::Moved(){

    //update the range if the max left col has changed
    //or if the number of locked columns has changed
    if(m_lastMaxLeftCol != m_GI->m_maxLeftCol ||
        m_lastNumLockCols != m_GI->m_numLockCols){

        m_lastMaxLeftCol = m_GI->m_maxLeftCol;
        m_lastNumLockCols = m_GI->m_numLockCols;

        //set the scroll range
        SCROLLINFO ScrollInfo;
        ScrollInfo.cbSize = sizeof(SCROLLINFO);
        ScrollInfo.fMask =  SIF_PAGE | SIF_RANGE;
        ScrollInfo.nPage =  (m_GI->m_gridWidth  - m_GI->m_lockColWidth) /
                            m_GI->m_defColWidth;
        ScrollInfo.nMin =   0;
        ScrollInfo.nMax =   (m_GI->m_maxLeftCol - m_GI->m_numLockCols) +
                            ScrollInfo.nPage -1;
        SetScrollInfo(&ScrollInfo,FALSE);

        if(m_GI->m_hScrollRect.top == m_GI->m_hScrollRect.bottom){
            m_ctrl->AdjustComponentSizes();
        }
    }

    //set the scroll pos
    if(m_GI->m_lastLeftCol != m_GI->m_leftCol)
        SetScrollPos(m_GI->m_leftCol - m_GI->m_numLockCols,TRUE);

    Invalidate();
}

/************************************************
*************************************************/
void CUGHScroll::HScroll(UINT nSBCode, UINT nPos)
{

    if(GetFocus() != m_ctrl->m_CUGGrid)
        m_ctrl->m_CUGGrid->SetFocus();

    m_ctrl->m_GI->m_moveType = 4;

    switch(nSBCode){
        case SB_LINEDOWN:{
            m_ctrl->MoveLeftCol(UG_LINEDOWN);
            break;
        }
        case SB_LINEUP:{
            m_ctrl->MoveLeftCol(UG_LINEUP);
            break;
        }
        case SB_PAGEUP:{
            m_ctrl->MoveLeftCol(UG_PAGEUP);
            break;
        }
        case SB_PAGEDOWN:{
            m_ctrl->MoveLeftCol(UG_PAGEDOWN);
            break;
        }
        case SB_TOP:{
            m_ctrl->MoveLeftCol(UG_TOP);
            break;
        }
        case SB_BOTTOM:{
            m_ctrl->MoveLeftCol(UG_BOTTOM);
            break;
        }
        case SB_THUMBTRACK:{
            if(m_GI->m_hScrollMode==1){ //tracking
                m_ctrl->SetLeftCol(nPos+m_GI->m_numLockCols);
            }
            m_trackColPos = nPos+m_GI->m_numLockCols;

            //scroll hint window
            #ifdef UG_ENABLE_SCROLLHINTS
            if(m_GI->m_enableHScrollHints){

                CString string;
                RECT rect;
                GetWindowRect(&rect);
                rect.left = LOWORD(GetMessagePos());
                m_ctrl->ScreenToClient(&rect);
                m_ctrl->m_CUGHint->SetWindowAlign(UG_ALIGNCENTER|UG_ALIGNBOTTOM);
                // TD
                m_ctrl->m_CUGHint->SetTextAlign(UG_ALIGNCENTER);

                m_ctrl->OnHScrollHint(m_trackColPos,&string);
                m_ctrl->m_CUGHint->SetText(string,FALSE);
                // TD set text before move window...
                m_ctrl->m_CUGHint->MoveHintWindow(rect.left,rect.top-1,20);
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
            if(m_GI->m_enableHScrollHints){
                m_ctrl->m_CUGHint->Hide();
            }
            #endif

            m_ctrl->SetLeftCol(nPos+m_GI->m_numLockCols);

            break;
        }
    }
}

void CUGHScroll::OnRButtonDown(UINT nFlags, CPoint point)
{
    // TODO: Add your message handler code here and/or call default
    if(m_GI->m_enablePopupMenu){
        ClientToScreen(&point);
        m_ctrl->StartMenu(0,0,&point,UG_HSCROLL);
    }

    CScrollBar::OnRButtonDown(nFlags, point);
}

int CUGHScroll::OnCreate(LPCREATESTRUCT lpCreateStruct)
{
    if (CScrollBar::OnCreate(lpCreateStruct) == -1)
        return -1;

    // TODO: Add your specialized creation code here
    if(m_GI->m_enableHScrollHints)
        EnableToolTips(TRUE);

    return 0;
}

