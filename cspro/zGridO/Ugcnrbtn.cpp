/***********************************************
    Ultimate Grid 97
    Copyright 1994 - 1997 Dundas Software Ltd.

    Class
        CUGnrBtn
    Purpose
        This class is used to draw the cells corner
        button. The corner button is the region
        at the top left of the grid between
        the top and side headings. The width and
        height of this object are determined by
        the width and height of the side and
        top headings.
    Details
        -This class uses a cell located at col -1
         by row -1 as its source of information
        -This object is drawn using the celltype
         that the above cell object points to
        -This object gives the user the ability
         to size the width and/or height of the
         side and top headings. This is done
         when the mouse is moved to the edge and
         depressed, then moved. Sizing nofications
         are make, which lets the programmer allow
         or dis-allow the sizing, or even set sizing
         limits.
************************************************/

#include "StdAfx.h"
#include "Ugctrl.h"

#ifdef _DEBUG
#define new DEBUG_NEW
#undef THIS_FILE
static char THIS_FILE[]= __FILE__;
#endif

/////////////////////////////////////////////////////////////////////////////
// CUGCnrBtn

/************************************************
Constructor
    initialize member variables and objects here
*************************************************/
CUGCnrBtn::CUGCnrBtn()
{
    //init the variables
    m_canSize = FALSE;
    m_isSizing = FALSE;
}

/************************************************
Destructor
    clean up  member variables and objects here
*************************************************/
CUGCnrBtn::~CUGCnrBtn()
{
}


/************************************************
*************************************************/
BEGIN_MESSAGE_MAP(CUGCnrBtn, CWnd)
    //{{AFX_MSG_MAP(CUGCnrBtn)
    ON_WM_PAINT()
    ON_WM_LBUTTONDBLCLK()
    ON_WM_LBUTTONDOWN()
    ON_WM_LBUTTONUP()
    ON_WM_MOUSEMOVE()
    ON_WM_RBUTTONDOWN()
    ON_WM_RBUTTONUP()
    ON_WM_SETCURSOR()
    //}}AFX_MSG_MAP
END_MESSAGE_MAP()


/////////////////////////////////////////////////////////////////////////////
// CUGCnrBtn message handlers

/************************************************
OnPaint
    Purpose
        Paints the corner button object. The
        corner button object uses a CUGCell located
        at position -1,-1 from the default datasource
        then uses the cell type associated with this
        cell to draw itself with.
    Params
        none
    Return
        none
*************************************************/
void CUGCnrBtn::OnPaint()
{
    CPaintDC dc(this); // device context for painting

    m_ctrl->OnScreenDCSetup(&dc,NULL,UG_TOPHEADING);

    RECT rect;
    GetClientRect(&rect);
    dc.SetBkColor(GetSysColor(COLOR_BTNFACE));

    CUGCellType * cellType;
    CUGCell cell;
    m_ctrl->GetCellIndirect(-1,-1,&cell);

    //get the cell type to draw the cell
    if(cell.IsPropertySet(UGCELL_CELLTYPE_SET))
        cellType = m_ctrl->GetCellType(cell.GetCellType());
    else
        cellType = m_ctrl->GetCellType(-1);

    cellType->OnDraw(&dc,&rect,-1,-1,&cell,0,0);

}
/************************************************
Update
    Purpose
        This function is called by the grid when
        certian properties are changed and when
        the grid is resized.
        This function must repaint itself if
        a change was made that will affect this
        object
    Params
        none
    Return
        none
*************************************************/
void CUGCnrBtn::Update(){

    InvalidateRect(NULL);
    UpdateWindow();
}

/************************************************
Moved
    Purpose
        This function is called whenever the
        current cell in the grid is moved or
        the drag cell is moved. If the movement
        affects this cell then it should be repainted
    Params
        none
    Return
        none
*************************************************/
void CUGCnrBtn::Moved(){

}

/************************************************
OnLButtonDblClk
    Purpose
        This function is called during a double
        click on this object. This function will
        then notify the main grid class of the event.
    Params
        nFlags  - mouse button flags
        point   - point where mouse was clicked
    Return
        none
*************************************************/
void CUGCnrBtn::OnLButtonDblClk(UINT nFlags, CPoint point)
{
    BOOL processed = FALSE;
    m_ctrl->OnCB_DClicked(&m_GI->m_cnrBtnRect,&point,processed);
}

/************************************************
OnLButtonDown
    Purpose
        This function is called when the left mouse
        button was just pressed over this object.
        This function will then notify the main
        grid class of the event.
        If the mouse is at the edge of the cell where
        user resizing can take please then resizing
        is enabled (if the user resize option is enabled)
    Params
        nFlags  - mouse button flags
        point   - point where mouse was clicked
    Return
        none
*************************************************/
void CUGCnrBtn::OnLButtonDown(UINT nFlags, CPoint point)
{
    if(m_canSize){
        m_isSizing = TRUE;
        SetCapture();
    }
    else{
        m_ctrl->OnCB_LClicked(1,&m_GI->m_cnrBtnRect,&point);
    }
}

/************************************************
OnLButtonUp
    Purpose
        This function is called when the left mouse
        button was just released over this object.
        This function will then notify the main
        grid class of the event.
        If resizing was in progress then it is
        terminated and a notification on the resize
        is made to the main grid.
    Params
        nFlags  - mouse button flags
        point   - point where mouse was clicked
    Return
        none
*************************************************/
void CUGCnrBtn::OnLButtonUp(UINT nFlags, CPoint point)
{
//  if(m_isSizing){
//      m_isSizing = FALSE;
//      ReleaseCapture();
//  }
//
//  BOOL processed = FALSE;
//  m_ctrl->OnCB_LClicked(0,&m_GI->m_cnrBtnRect,&point);

    // smg: stuff above is original func, stuff below is what i wrote
    // basically, when i tried to resize the top or side heading from
    // the corner button area, the proper calls were not being made,
    // and my CFormGrid funcs weren't getting invoked; used code from
    // CUGSideHdg as a guide (as it seemed to handle this ok)
    // [[ note is for the old CFormGrid, not the one that currently exists ]]

    if(m_isSizing)
    {
        m_isSizing = FALSE;
        ReleaseCapture();

        if(m_sizingWidth)   //col sizing
        {
            m_sizingWidth = false;

            m_ctrl->OnSideHdgSized(&m_GI->m_sideHdgWidth);
        }
        else if (m_sizingHeight)
        {
            m_sizingHeight = false;

            m_ctrl->OnTopHdgSized(&m_GI->m_topHdgHeight);
        }
        m_ctrl->AdjustComponentSizes();
    }
    else
    {
        m_ctrl->OnCB_LClicked(0,&m_GI->m_cnrBtnRect,&point);
    }

}

/************************************************
OnMouseMove
    Purpose
        This function is called when the mouse is
        moved over this object.
        If user resizing is allowed then this function
        check to see if the mouse is located at a position
        where resizing can take place, and sets a m_canSize
        flag.
        If user resizing has been started then this function
        resizes the top or side heading, according to the
        position of the mouse.
    Params
        nFlags  - mouse button flags
        point   - point where mouse was clicked
    Return
        none
*************************************************/
void CUGCnrBtn::OnMouseMove(UINT nFlags, CPoint point)
{

    //check to see if the mouse is over a separation line
    if(m_isSizing == FALSE && (nFlags&MK_LBUTTON)==0 && m_GI->m_userSizingMode >0){
        m_canSize = FALSE;
        m_sizingWidth = FALSE;
        m_sizingHeight = FALSE;

        RECT rect;
        GetClientRect(&rect);
        if(point.x > rect.right -4 && m_ctrl->OnCanSizeSideHdg()){
            m_canSize = TRUE;
            m_sizingWidth = TRUE;
            SetCursor(m_GI->m_WEResizseCursor);
        }
        else if(point.y > rect.bottom -4 && m_ctrl->OnCanSizeTopHdg()){
            m_canSize = TRUE;
            m_sizingHeight = TRUE;
            SetCursor(m_GI->m_NSResizseCursor);
        }
    }
    else if(m_isSizing){
        if(m_sizingHeight){
            if(point.y <0)
                point.y =0;

            int height = point.y;
            m_ctrl->OnTopHdgSizing(&height);

//          m_ctrl->SetTH_Height(height);
            m_GI->m_topHdgHeight = height;  // smg; i believe this is correct, not above call

            m_ctrl->AdjustComponentSizes();
        }
        else{
            if(point.x <0)
                point.x =0;
            int width = point.x;

            m_ctrl->OnSideHdgSizing(&width);

//          m_ctrl->SetSH_Width(width);
            m_GI->m_sideHdgWidth = width;   // smg: i believe this is the correct call, not the one above

            m_ctrl->AdjustComponentSizes();
        }
    }
}

/************************************************
OnRButtonDown
    Purpose
        This function is called during a right mouse
        down event.
        This function will then notify the main grid
        class of the event.
        If popup menus are enabled then this function
        calls the grids StartMenu function
    Params
        nFlags  - mouse button flags
        point   - point where mouse was clicked
    Return
        none
*************************************************/
void CUGCnrBtn::OnRButtonDown(UINT nFlags, CPoint point)
{
    BOOL processed = FALSE;
    m_ctrl->OnCB_RClicked(1,&m_GI->m_cnrBtnRect,&point,processed);

    if(m_GI->m_enablePopupMenu){
        ClientToScreen(&point);
        m_ctrl->StartMenu(-1,-1,&point,UG_CORNERBUTTON);
    }
}

/************************************************
OnRButtonUp
    Purpose
        This function is called during a right mouse
        up event.
        This function will then notify the main grid
        class of the event.
    Params
        nFlags  - mouse button flags
        point   - point where mouse was clicked
    Return
        none
*************************************************/
void CUGCnrBtn::OnRButtonUp(UINT nFlags, CPoint point)
{
    BOOL processed = FALSE;
    m_ctrl->OnCB_RClicked(0,&m_GI->m_cnrBtnRect,&point,processed);
}

/************************************************
OnSetCursor
    Purpose
        This function sets the cursor to an arrow
        or a resize cursor, depending on the
        resize state flags.
    Params
    Return
*************************************************/
BOOL CUGCnrBtn::OnSetCursor(CWnd* pWnd, UINT nHitTest, UINT message)
{
    // TODO: Add your message handler code here and/or call default
    if(m_canSize){
        if(m_sizingWidth && m_sizingHeight)
            SetCursor(LoadCursor(NULL,IDC_SIZENWSE));
        else if(m_sizingWidth)
            SetCursor(m_GI->m_WEResizseCursor);
        else if(m_sizingHeight)
            SetCursor(m_GI->m_NSResizseCursor);
    }
    else
        SetCursor(m_GI->m_arrowCursor);
    return 1;
}

