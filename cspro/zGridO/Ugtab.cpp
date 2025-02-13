/***********************************************
    Ultimate Grid 97
    Copyright 1994 - 1997 Dundas Software Ltd.


    class
        CUGTab
    Purpose
        This is a general purose tab control
        that is built into the grid. It is
        most commonly used to switch grid sheets,
        but can be used for other purposes.
    Details
        -This control can contain upto 64 tabs
        -A notification is sent when a tab is
         selected, it does not change grid sheets
         automatically. Although grid sheets can
         be changed during this notification.
         This method gives the tab control greater
         abilities.
        -Fonts and colors can be setup for the
         tab control
***********************************************/

#include "StdAfx.h"
#include "Ugctrl.h"

#ifdef _DEBUG
#define new DEBUG_NEW
#undef THIS_FILE
static char THIS_FILE[]= __FILE__;
#endif


/******************************************
*******************************************/
CUGTab::CUGTab()
{
    m_tabCount      = 0;
    m_currentTab    = 0;
    m_tabOffset     = 0;

    m_scrollWidth   = GetSystemMetrics(SM_CXHSCROLL) *2;

    m_defFont.CreateFont(14,0,0,0,500,0,0,0,0,0,0,0,0,_T("Arial"));
    m_font = &m_defFont;

    m_resizeReady   = FALSE;
    m_resizeInProgress = FALSE;

    m_bestWidth     = 150;

    m_bCanClick     = true;     // gsf 09/13/99

}

/******************************************
*******************************************/
CUGTab::~CUGTab()
{
}

/******************************************
*******************************************/
BEGIN_MESSAGE_MAP(CUGTab, CWnd)
    //{{AFX_MSG_MAP(CUGTab)
    ON_WM_PAINT()
    ON_WM_CREATE()
    ON_WM_LBUTTONDOWN()
    ON_WM_SIZE()
    ON_WM_HSCROLL()
    ON_WM_MOUSEMOVE()
    ON_WM_SETCURSOR()
    ON_WM_LBUTTONUP()
    ON_WM_RBUTTONDOWN()
    //}}AFX_MSG_MAP
END_MESSAGE_MAP()


/******************************************
*******************************************/
int CUGTab::GetTabItemWidth(LPCTSTR string){

    CString sString = string;
    int iLength = sString.GetLength();
    if(iLength > IMSAMAXTABLN) {
        int iCount = iLength - IMSAMAXTABLN;
        sString.Delete(IMSAMAXTABLN,iCount);
    }
    //get the string width
    CDC* dc = GetDC();

    CFont * oldfont = dc->SelectObject(m_font);
    CSize size = dc->GetTextExtent(sString);
    dc->SelectObject(oldfont);
    ReleaseDC(dc);

    return size.cx +10;

}
/******************************************
*******************************************/
int CUGTab::AddTab(LPCTSTR string,long ID){


    if(m_tabCount < UTMAXTABS){
        m_tabStrings[m_tabCount]    = string;
        m_tabIDs[m_tabCount]        = ID;
        m_tabWidths[m_tabCount]     = GetTabItemWidth(string);
        m_tabTextColors[m_tabCount] = GetSysColor(COLOR_WINDOWTEXT);
        m_tabBackColors[m_tabCount] = GetSysColor(COLOR_BTNFACE);

        m_tabCount++;

        AdjustScrollBars();

        return UG_SUCCESS;
    }
    return UG_ERROR;
}
/******************************************
*******************************************/
int CUGTab::InsertTab(int pos,LPCTSTR string,long ID){

    if(pos < 0 || pos > m_tabCount)
        return FALSE;
    if(pos == m_tabCount)
        return AddTab(string,ID);

    if(m_tabCount < UTMAXTABS){

        for(int loop = m_tabCount; loop > pos; loop --){
            m_tabStrings[loop]  = m_tabStrings[loop-1];
            m_tabIDs[loop]      = m_tabIDs[loop-1];
            m_tabWidths[loop]       = m_tabWidths[loop-1];
            m_tabTextColors[loop]   = m_tabTextColors[loop-1];
            m_tabBackColors[loop]   = m_tabBackColors[loop-1];
        }
        m_tabCount++;

        if(m_currentTab >= pos)
            m_currentTab++;

        m_tabStrings[pos]       = string;
        m_tabIDs[pos]           = ID;
        m_tabWidths[pos]        = GetTabItemWidth(string);
        m_tabTextColors[pos]    = GetSysColor(COLOR_WINDOWTEXT);
        m_tabBackColors[pos]    = GetSysColor(COLOR_BTNFACE);

        AdjustScrollBars();
        Invalidate(TRUE);

        return UG_SUCCESS;
    }


    return UG_ERROR;
}
/******************************************
*******************************************/
int CUGTab::DeleteTab(long ID){

    for(int loop = 0;loop < m_tabCount; loop++){

        if(m_tabIDs[loop] == ID){

            for(int loop2 = loop;loop2 < m_tabCount-1;loop2++){
                m_tabStrings[loop2]     = m_tabStrings[loop2+1];
                m_tabIDs[loop2]         = m_tabIDs[loop2+1];
                m_tabWidths[loop2]      = m_tabWidths[loop2+1];
                m_tabTextColors[loop2]  = m_tabTextColors[loop2+1];
                m_tabBackColors[loop2]  = m_tabBackColors[loop2+1];
            }

            m_tabCount--;

            if(m_currentTab == loop)
                SetCurrentTab(m_tabIDs[0]);
            else if(m_currentTab > 0)
                m_currentTab--;

            AdjustScrollBars();
            Invalidate(TRUE);

            return TRUE;
        }
    }
    return FALSE;

}

/******************************************
*******************************************/
int CUGTab::GetTabCount(){
    return m_tabCount;
}
/******************************************
*******************************************/
int CUGTab::SetCurrentTab(long ID){

    int width = -5;

    for(int loop = 0;loop < m_tabCount; loop++){

        if(m_tabIDs[loop] == ID){

            m_currentTab = loop;
            if(width < 0)
                width = 0;

            if(width < m_tabOffset)
                m_tabOffset = width;
            else if (width > (m_tabOffset + (m_GI->m_tabWidth - (m_tabWidths[loop]*2)-10)))
                m_tabOffset = width - (m_GI->m_tabWidth - (m_tabWidths[loop]*2)-10);

            if(width > m_maxTabOffset)
                m_tabOffset = m_maxTabOffset;

            Invalidate(TRUE);
            return TRUE;
        }
        width += m_tabWidths[loop];
    }
    return FALSE;
}
/******************************************
*******************************************/
int CUGTab::GetCurrentTab(){
    return m_currentTab;
}
/******************************************
*******************************************/
int CUGTab::GetCurrentTabID(){
    return m_tabIDs[m_currentTab];
}
/******************************************
*******************************************/
int CUGTab::GetTabID(int iIndex){
    return m_tabIDs[iIndex];
}
/******************************************
*******************************************/
int CUGTab::AdjustScrollBars(){

    if(m_tabCount < 1)
        return 0;

    //get the total width and setup the scroll bars
    int totalTabWidths = 0;

    for(int t =0;t <m_tabCount;t++)
        totalTabWidths += m_tabWidths[t];

    RECT rect;
    GetClientRect(&rect);
    int width = rect.right - m_scrollWidth;

    int dif = totalTabWidths - width;
    if(dif >0){
        m_maxTabOffset = dif + 20;
    }
    else{
        m_maxTabOffset = 0;
    }

    return 1;


}
/******************************************
*******************************************/
void CUGTab::OnTabSizing(int width){

    RECT rect;
    GetWindowRect(&rect);
    GetParent()->ScreenToClient(&rect);

    RECT parentRect;
    GetParent()->GetClientRect(&parentRect);

    if(width < 5)
        width = 5;
    if(width > parentRect.right - 15)
        width = parentRect.right - 15;

    m_GI->m_tabWidth = width;

    m_ctrl->m_tabSizing = TRUE;
    m_ctrl->AdjustComponentSizes();
    m_ctrl->m_tabSizing = FALSE;
}

/******************************************
*******************************************/
void CUGTab::Update(){
    InvalidateRect(NULL);
    UpdateWindow();
}

/******************************************
*******************************************/
void CUGTab::OnPaint()
{
    CPaintDC dc(this); // device context for painting

    int idDC = dc.SaveDC();

    CFont * oldfont = dc.SelectObject(m_font);

    CBrush graybrush(GetSysColor(COLOR_BTNFACE));
    CBrush whitebrush(RGB(255,255,255));
    CPen whitepen(PS_SOLID,1,RGB(255,255,255));
    CPen blackpen(PS_SOLID,1,RGB(0,0,0));
    CPen graypen(PS_SOLID,1,GetSysColor(COLOR_BTNFACE));
    CPen darkgraypen(PS_SOLID,1,GetSysColor(COLOR_BTNSHADOW));
    CBrush *pBrush = NULL;

    POINT points[4];

    RECT currentRect;

    RECT rect;
    RECT clientRect;
    GetClientRect(&rect);
    GetClientRect(&clientRect);

    dc.SetBkColor(GetSysColor(COLOR_BTNFACE));
    dc.ExtTextOut(0,0,ETO_OPAQUE,&rect,_T(""),0,NULL);

    rect.left = m_scrollWidth - m_tabOffset;

    rect.bottom --;
    rect.right --;

    //draw each tab
    for(int t = 0;t <m_tabCount;t++){

        if(pBrush != NULL){
            delete pBrush;
            pBrush = NULL;
        }

        rect.right = rect.left+ m_tabWidths[t];

        if(t == m_currentTab){

            dc.SelectObject(&whitebrush);
            dc.SetBkColor(RGB(255,255,255));
            CopyRect(&currentRect,&rect);
        }
        else{
            pBrush = new CBrush(m_tabBackColors[t]);
            //dc.SelectObject(&graybrush);
            dc.SelectObject(pBrush);
            dc.SetBkColor(m_tabBackColors[t]);
        }

        //draw the tab
        points[0].x = rect.left;
        points[0].y = rect.top;
        points[1].x = rect.left + 7;
        points[1].y = rect.bottom;
        points[2].x = rect.right + 2;
        points[2].y = rect.bottom;
        points[3].x = rect.right +9;
        points[3].y = rect.top;
        dc.Polygon(points,4);

        //draw the text
        rect.left+=10;
        rect.top ++;
        dc.SetTextColor(m_tabTextColors[t]);

        //Savy 10/27/98 added code for stripping additional charachters
        CString sString = m_tabStrings[t];
        if( sString.GetLength() > IMSAMAXTABLN) {
            int iCount = sString.GetLength() - IMSAMAXTABLN;
            sString.Delete(IMSAMAXTABLN,iCount);

        }
        dc.ExtTextOut(rect.left,rect.top,0,&rect,sString,NULL);

        rect.left -=10;
        rect.top --;

        rect.left = rect.right;
    }

    if(pBrush != NULL)
        delete pBrush;

    //draw the current tab
    dc.SelectObject(&whitebrush);
    dc.SetBkColor(RGB(255,255,255));
    CopyRect(&rect,&currentRect);

    //draw the tab
    points[0].x = rect.left;
    points[0].y = rect.top;
    points[1].x = rect.left + 7;
    points[1].y = rect.bottom;
    points[2].x = rect.right + 2;
    points[2].y = rect.bottom;
    points[3].x = rect.right +9;
    points[3].y = rect.top;
    dc.Polygon(points,4);

    //blank out the top line
    dc.SelectObject(&whitepen);
    dc.MoveTo(rect.left+1,rect.top);
    dc.LineTo(rect.right+9,rect.top);


    //draw the text
    rect.left+=10;
    rect.top ++;
    dc.SetTextColor(m_tabTextColors[m_currentTab]);
    //Savy 10/27/98 added code for stripping additional charachters
    CString sString = m_tabStrings[m_currentTab];
    if( sString.GetLength() > IMSAMAXTABLN) {
        int iCount = sString.GetLength() - IMSAMAXTABLN;
        sString.Delete(IMSAMAXTABLN,iCount);
    }
    dc.ExtTextOut(rect.left,rect.top,0,&rect,sString,NULL);

    //draw the sizing bar
    CopyRect(&rect,&clientRect);
    rect.right --;
    dc.SelectObject(&blackpen);
    dc.MoveTo(rect.right,rect.top);
    dc.LineTo(rect.right,rect.bottom);
    dc.SelectObject(&darkgraypen);
    rect.right --;
    dc.MoveTo(rect.right,rect.top);
    dc.LineTo(rect.right,rect.bottom);
    dc.SelectObject(&graypen);
    rect.right --;
    dc.MoveTo(rect.right,rect.top);
    dc.LineTo(rect.right,rect.bottom);
    rect.right --;
    dc.MoveTo(rect.right,rect.top);
    dc.LineTo(rect.right,rect.bottom);
    dc.SelectObject(&whitepen);
    rect.right --;
    dc.MoveTo(rect.right,rect.top);
    dc.LineTo(rect.right,rect.bottom);
    dc.SelectObject(&graypen);
    rect.right --;
    dc.MoveTo(rect.right,rect.top);
    dc.LineTo(rect.right,rect.bottom);


    dc.SelectObject(oldfont);

    //draw the black line over the scroll buttons
    dc.SelectObject(&blackpen);
    dc.MoveTo(rect.left,rect.top);
    dc.LineTo(rect.left+m_scrollWidth+1,rect.top);

    m_scroll.Invalidate();
    m_scroll.UpdateWindow();

    dc.RestoreDC(idDC);
}

/******************************************
*******************************************/
int CUGTab::OnCreate(LPCREATESTRUCT lpCreateStruct)
{
    if (CWnd::OnCreate(lpCreateStruct) == -1)
        return -1;

    // TODO: Add your specialized creation code here
    RECT rect;
    GetClientRect(&rect);
    rect.right = m_scrollWidth;

    m_scroll.Create(WS_CHILD|WS_VISIBLE,rect,this,UTABSCROLLID);

    //10/26/98   Added this to enable tooltips

    EnableToolTips(TRUE);
    return 0;
}

/******************************************
*******************************************/
void CUGTab::OnLButtonDown(UINT nFlags, CPoint point)
{
    // gsf 09/13/99
    if (!m_bCanClick) {
        return;
    }
    // end of gsf 09/13/99

    //check to see if the tabs are to be resized
    if(m_resizeReady){
        SetCapture();
        m_resizeInProgress = TRUE;
    }

    //find the tab that the mouse is over
    else{
        int left = m_scrollWidth - m_tabOffset;

        for(int t =0;t <m_tabCount;t++){

            int right = left + m_tabWidths[t];

            //check to see if the mouse is over a given tab
            if(point.x >= left && point.x <= (right+2)){

                m_currentTab = t;

                //check to make sure that the tab is fully visible
                RECT rect;
                GetClientRect(&rect);
                left -=5;
                if(left < m_scrollWidth){
                    m_tabOffset -= (m_scrollWidth - left);
                    if(m_tabOffset < 0)
                        m_tabOffset = 0;
                }
                right += 19;
                if(right > rect.right){
                    m_tabOffset += (right - rect.right);
                    if(m_tabOffset > m_maxTabOffset)
                        m_tabOffset = m_maxTabOffset;
                }

                Invalidate(TRUE);

                //send a notify
                m_ctrl->OnTabSelected(m_tabIDs[t]);
            }

            left = right;
        }
    }
    CWnd::OnLButtonDown(nFlags, point);
}

/******************************************
*******************************************/
void CUGTab::OnSize(UINT nType, int cx, int cy)
{
    CWnd::OnSize(nType, cx, cy);

    RECT rect;
    GetClientRect(&rect);
    rect.right = m_scrollWidth;
    rect.top++;
    m_scroll.MoveWindow(&rect,TRUE);

    AdjustScrollBars();
}

/******************************************
*******************************************/
void CUGTab::OnHScroll(UINT nSBCode, UINT nPos, CScrollBar* pScrollBar)
{
    // gsf 09/13/99
    if (!m_bCanClick) {
        return;
    }
    // end of gsf 09/13/99

    // TODO: Add your message handler code here and/or call default
        if(nSBCode ==   SB_LINEUP){
            if(m_tabOffset >0){
                m_tabOffset -=5;
                Invalidate(TRUE);
            }
        }
        if(nSBCode ==   SB_LINEDOWN){
            if(m_tabOffset < m_maxTabOffset-5){
                m_tabOffset +=5;
                Invalidate(TRUE);
            }
        }

    CWnd::OnHScroll(nSBCode, nPos, pScrollBar);
}

/******************************************
*******************************************/
BOOL CUGTab::PreCreateWindow(CREATESTRUCT& cs)
{
    // TODO: Add your specialized code here and/or call the base class
    cs.style |= WS_CLIPCHILDREN;
    return CWnd::PreCreateWindow(cs);
}

/******************************************
*******************************************/
void CUGTab::OnMouseMove(UINT nFlags, CPoint point)
{
    // TODO: Add your message handler code here and/or call default

    //resize in progress
    if(m_resizeInProgress){
        OnTabSizing(point.x);
    }

    //check for resize position
    else{
        RECT rect;
        GetClientRect(&rect);
        if(point.x > rect.right - 7){
            if(!m_resizeReady){
                m_resizeReady = TRUE;
            }
        }
        else if(m_resizeReady){
            m_resizeReady = FALSE;
        }
    }
}

/******************************************
*******************************************/
BOOL CUGTab::OnSetCursor(CWnd* pWnd, UINT nHitTest, UINT message)
{
    // TODO: Add your message handler code here and/or call default
    if(m_resizeReady)
        SetCursor(m_GI->m_WEResizseCursor);
    else
        SetCursor(LoadCursor(NULL,IDC_ARROW));

    return TRUE;
}

/******************************************
*******************************************/
void CUGTab::OnLButtonUp(UINT nFlags, CPoint point)
{
    // gsf 09/13/99
    if (!m_bCanClick) {
        return;
    }
    // end of gsf 09/13/99

    // TODO: Add your message handler code here and/or call default
    ReleaseCapture();
    m_resizeInProgress = FALSE;

}

/******************************************
*******************************************/
void CUGTab::OnRButtonDown(UINT nFlags, CPoint point)
{
    // gsf 09/13/99
    if (!m_bCanClick) {
        return;
    }
    // end of gsf 09/13/99

    if(m_GI->m_enablePopupMenu){

        int tab = -1;
        //find the tab the mouse is over
        int left = m_scrollWidth - m_tabOffset;

        for(int t =0;t <m_tabCount;t++){

            int right = left + m_tabWidths[t];

            //check to see if the mouse is over a given tab
            if(point.x >= left && point.x <= (right+2)){
                tab = t;
                break;
            }

            left = right;
        }

        ClientToScreen(&point);
        m_ctrl->StartMenu(tab,0,&point,UG_TAB);
    }

}
/******************************************
*******************************************/
int CUGTab::SetTabBackColor(long ID,COLORREF color){

    for(int loop = 0;loop < m_tabCount; loop++){
        if(m_tabIDs[loop] == ID){
            m_tabBackColors[loop] = color;
            return TRUE;
        }
    }
    return FALSE;
}
/******************************************
*******************************************/
int CUGTab::SetTabTextColor(long ID,COLORREF color){

    for(int loop = 0;loop < m_tabCount; loop++){
        if(m_tabIDs[loop] == ID){
            m_tabTextColors[loop] = color;
            return TRUE;
        }
    }
    return FALSE;
}
/******************************************
//10/23/98 Savy added this function to update tab text
*******************************************/
int CUGTab::SetTabText(long ID,CString sString){

    for(int loop = 0;loop < m_tabCount; loop++){
        if(m_tabIDs[loop] == ID){
            m_tabStrings[loop] = sString;
            m_tabWidths[loop] =  GetTabItemWidth(sString);
            return TRUE;
        }
    }
    return FALSE;
}

/******************************************
*******************************************/
int CUGTab::SetTabFont(CFont * font){

    if(font != NULL)
        m_font = font;

    for(int loop = 0;loop < m_tabCount; loop++){
        m_tabWidths[loop] = GetTabItemWidth(m_tabStrings[loop]);
    }
    AdjustScrollBars();
    Invalidate(TRUE);


    return TRUE;
}

int CUGTab::OnToolHitTest(CPoint point, TOOLINFO* pTI) const{
    //Get the tab on which this falls

    int left = m_scrollWidth - m_tabOffset;

    for(int t =0;t <m_tabCount;t++){

        int right = left + m_tabWidths[t];

        //check to see if the mouse is over a given tab
        if(point.x >= left && point.x <= (right+2)){

            //Get the String corresponding to the  tab
            CString csTTip = m_tabStrings[t];

            // We got the tab
            const int iID = MAKELONG(point.x, point.y)+1;

            pTI->hwnd = m_hWnd;
            pTI->uId = iID;
            pTI->rect = CRect(point, CSize(1,1)); // bounding
            pTI->lpszText = _tcsdup((const csprochar*)csTTip);

            // MFC will free this memory later
            return iID;
            }

            left = right;
        }

    return -1;
}

