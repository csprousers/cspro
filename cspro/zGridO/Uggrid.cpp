/***********************************************
    Ultimate Grid 97
    Copyright 1994 - 1997 Dundas Software Ltd.


   class CUGGrid
************************************************/

#include "StdAfx.h"
#include <math.h>
#include "Ugctrl.h"

#ifdef _DEBUG
#define new DEBUG_NEW
#undef THIS_FILE
static char THIS_FILE[]= __FILE__;
#endif

/////////////////////////////////////////////////////////////////////////////
// CUGGrid

/************************************************
*************************************************/
CUGGrid::CUGGrid()
{

    m_doubleBufferMode  = FALSE;

    m_keyRepeatCount    = 0;

    m_paintOverRide     = FALSE;

    m_hasFocus          = FALSE;

    m_cellTypeCapture   = FALSE;

    m_bitmap            = NULL;

}

/************************************************
*************************************************/
CUGGrid::~CUGGrid()
{
    if(m_bitmap != NULL)
        delete m_bitmap;
}


/************************************************
*************************************************/
BEGIN_MESSAGE_MAP(CUGGrid, CWnd)
    //{{AFX_MSG_MAP(CUGGrid)
    ON_WM_PAINT()
    ON_WM_KEYDOWN()
    ON_WM_LBUTTONDOWN()
    ON_WM_LBUTTONUP()
    ON_WM_LBUTTONDBLCLK()
    ON_WM_MOUSEMOVE()
    ON_WM_RBUTTONDOWN()
    ON_WM_RBUTTONUP()
    ON_WM_CHAR()
    ON_WM_KILLFOCUS()
    ON_WM_SETFOCUS()
    ON_WM_MOUSEACTIVATE()
    ON_WM_SIZE()
    ON_WM_SETCURSOR()
    ON_WM_KEYUP()
    ON_WM_GETDLGCODE()
    ON_WM_CREATE()
    ON_WM_RBUTTONDBLCLK()
    ON_WM_DESTROY()
    //}}AFX_MSG_MAP

    ON_NOTIFY_EX( TTN_NEEDTEXT, 0, ToolTipNeedText)

    #ifdef WM_MOUSEWHEEL
        ON_WM_MOUSEWHEEL()
    #endif

END_MESSAGE_MAP()

/************************************************
*************************************************/
int CUGGrid::OnCreate(LPCREATESTRUCT lpCreateStruct)
{
    if (CWnd::OnCreate(lpCreateStruct) == -1)
        return -1;

    if(m_GI->m_enableHints)
        EnableToolTips(TRUE);

    return 0;
}
/************************************************
*************************************************/
BOOL CUGGrid::ToolTipNeedText( UINT id, NMHDR * pTTTStruct, LRESULT * pResult ){

    TOOLTIPTEXT *pTTT = (TOOLTIPTEXT *)pTTTStruct;

    CString string;
    int col;
    long row;
    POINT point;

    GetCursorPos(&point);
    ScreenToClient(&point);
    m_ctrl->GetCellFromPoint(point.x,point.y,&col,&row);

    m_ctrl->OnHint(col,row,UG_GRID,&string);

    lstrcpy(pTTT->szText,string);

    return TRUE;
}
/************************************************
*************************************************/
int CUGGrid::OnToolHitTest( CPoint point, TOOLINFO* pTI ) const{

    int col;
    long row;
    static int lastCol = -2;
    static long lastRow = -2;

    if(m_ctrl->GetCellFromPoint(point.x,point.y,&col,&row) == UG_SUCCESS){
        if(col != lastCol || row != lastRow){
            lastCol = col;
            lastRow = row;
            CancelToolTips();
            return -1;
        }

        pTI->cbSize = sizeof(TOOLINFO);
        pTI->uFlags =  TTF_NOTBUTTON | TTF_ALWAYSTIP |TTF_IDISHWND ;
        pTI->uId = (UINT)m_hWnd;
        pTI->hwnd = (HWND)m_hWnd;
        pTI->lpszText = LPSTR_TEXTCALLBACK;
        return 1;
    }
    return -1;
}
/************************************************
OnPaint
    This routine is responsible for gathering the
    information to draw, get the selected state
    plus draw in an optomized fashion
*************************************************/
void CUGGrid::OnPaint()
{

    CPaintDC dc(this); // device context for painting

    //redraw the cells in any invalid region
    RECT rect;
    if( dc.GetClipBox(&rect) != NULLREGION){
        int startCol,endCol;
        long startRow,endRow;
        if(m_ctrl->GetCellFromPoint(rect.left,rect.top,&startCol,&startRow) != UG_SUCCESS){
            startCol = 0;
            startRow = 0;
        }
        if(m_ctrl->GetCellFromPoint(rect.right,rect.bottom,&endCol,&endRow) != UG_SUCCESS){
            endCol = m_GI->m_numberCols;
            endRow = m_GI->m_numberRows;

        }
        m_drawHint.AddHint(0,0,m_GI->m_numberCols,m_GI->m_numberRows);
    }
    else{
        m_drawHint.AddHint(0,0,m_GI->m_numberCols,m_GI->m_numberRows);
    }

    //double buffering
    CDC * db_dc = NULL;
    if(m_doubleBufferMode){
        db_dc = new CDC;
        db_dc->CreateCompatibleDC(NULL);
        db_dc->SelectObject(m_bitmap);
    }

    DrawCellsIntern(&dc,db_dc);

    if(db_dc!= NULL)
        delete db_dc;

}

/************************************************
*************************************************/
void CUGGrid::DrawCellsIntern(CDC *dc,CDC *db_dc){

    RECT        rect = {0,0,0,0};
    RECT        cellRect;

    CUGCell     cell,tempCell;
    CString     string;
    CSize       size;

    RECT        focusRect = {-1,-1,-1,-1};
    CUGCellType * cellType;

    int         rightBlank  = -1;
    int         bottomBlank = -1;

    int         col,x;
    long        row,y;

    int         selectBlock;

    int         dcID;
    CDC*        origDC = NULL;

    //double buffering
    if(m_doubleBufferMode != NULL){
        origDC = dc;
        dc =  db_dc;
        //get the default font to use
        if(m_GI->m_defFont != NULL){
            origDC->SelectObject(m_GI->m_defFont);
            dc->SelectObject(m_GI->m_defFont);
        }
    }
    else{
        //get the default font to use
        if(m_GI->m_defFont != NULL){
            dc->SelectObject(m_GI->m_defFont);
        }
    }

    m_ctrl->OnScreenDCSetup(origDC,db_dc,UG_GRID);


    //set the right and bottom vars to point to
    //the extremes, if the right or bottom is
    //sooner then they will be updated in the
    //main drawing loop
    m_GI->m_rightCol = m_GI->m_numberCols;
    m_GI->m_bottomRow = m_GI->m_numberRows;


    //main draw loop, this loop goes through all visible
    //cells and checks to see if they need redrawing
    //if they do then the cell is retrieved and drawn
    for(y = 0; y < m_GI->m_numberRows;y++){

        //skip rows hidden under locked rows
        if(y == m_GI->m_numLockRows)
            y = m_GI->m_topRow;

        row = y;

        //calc the top bottom and right side of the rect
        //for the current cell to be drawn
        rect.top = rect.bottom;

        if(m_GI->m_uniformRowHeightFlag)
            rect.bottom += m_GI->m_defRowHeight;
        else
            rect.bottom += m_GI->m_rowHeights[row];

        if(rect.top == rect.bottom)
            continue;

        rect.right = 0;

        //check all visible cells in the current row to
        //see if they need drawing
        for(x = 0;x < m_GI->m_numberCols;x++){

            //skip cols hidden under locked cols
            if(x == m_GI->m_numLockCols)
                x = m_GI->m_leftCol;

            row = y;
            col = x;

            //calc the left and right side of the rect
            rect.left = rect.right;
            rect.right  += m_GI->m_colInfo[col].width;

            if(rect.left == rect.right)
                continue;

            //check to see if the cell need to be redrawn
            if(m_drawHint.IsInvalid(col,row) != FALSE){

                //copy the rect, then use the cellRect from here
                //this is done since the cellRect may be modifiec
                CopyRect(&cellRect,&rect);

                //get the cell to draw
                m_ctrl->GetCellIndirect(col,row,&cell);

                //check to see if the cell is joined
                if(cell.IsPropertySet(UGCELL_JOIN_SET)){
                    m_ctrl->GetCellRect(col,row,&cellRect);
                    m_ctrl->GetJoinStartCell(&col,&row,&cell);
                    if(m_drawHint.IsValid(col,row))
                        continue;
                    m_drawHint.SetAsValid(col,row);
                }

                //get the cell type to draw the cell
                if(cell.IsPropertySet(UGCELL_CELLTYPE_SET)){
                    cellType = m_ctrl->GetCellType(cell.GetCellType());
                }
                else{
                    cellType = m_ctrl->GetCellType(-1);
                }


                dcID = dc->SaveDC();

                //draw the cell, check to see if it is 'current' and/or selected
                if(row == m_GI->m_currentRow && ( col == m_GI->m_currentCol || m_GI->m_highlightRowFlag))
                    cellType->OnDraw(dc,&cellRect,col,row,&cell,0,1);
                else{
                    if(m_GI->m_multiSelect->IsSelected(col,row,&selectBlock))
                        cellType->OnDraw(dc,&cellRect,col,row,&cell,selectBlock+1,0);
                    else
                        cellType->OnDraw(dc,&cellRect,col,row,&cell,0,0);
                }

                //draw a black line at the right edge of the locked columns
                if(m_GI->m_numLockCols >0 && !m_GI->m_bCrossTab){ //SAVY 09/15/2003 to prevent the right line being drawn on locked cells
                    if(col == m_GI->m_leftCol){
                        dc->SelectObject(GetStockObject(BLACK_PEN));
                        dc->MoveTo(cellRect.left-1,cellRect.top);
                        dc->LineTo(cellRect.left-1,cellRect.bottom+1);
                    }
                    else if(col == m_GI->m_numLockCols-1){
                        dc->SelectObject(GetStockObject(BLACK_PEN));
                        dc->MoveTo(cellRect.right-1,cellRect.top);
                        dc->LineTo(cellRect.right-1,cellRect.bottom+1);
                    }
                }
                //draw a black line at the bottom of the locked rows
                if(m_GI->m_numLockRows >0 && !m_GI->m_bCrossTab){ //SAVY 09/15/2003 to prevent the right line being drawn on locked cells
                    if(row == m_GI->m_topRow ){
                        dc->SelectObject(GetStockObject(BLACK_PEN));
                        dc->MoveTo(cellRect.left,cellRect.top-1);
                        dc->LineTo(cellRect.right+1,cellRect.top-1);
                    }
                    else if(row == m_GI->m_numLockRows-1){
                        dc->SelectObject(GetStockObject(BLACK_PEN));
                        dc->MoveTo(cellRect.left,cellRect.bottom-1);
                        dc->LineTo(cellRect.right+1,cellRect.bottom-1);
                    }
                }

                dc->RestoreDC(dcID);
            }

            //check to see if the focus rect should be drawn
            //this function should be called all the time
            //even if it is off screen
            if(row == m_GI->m_currentRow && (col == m_GI->m_currentCol ||
                m_GI->m_highlightRowFlag)){
                CopyRect(&focusRect,&cellRect);
            }

            //check to see if the right side of the rect is past the edge
            //of the grid drawing area, if so then break
            if(rect.right > m_GI->m_gridWidth){
                m_GI->m_rightCol = col;
                break;
            }

        }

        //check to see if there is blank space on the right side of the grid
        //drawing area
        if(rect.right < m_GI->m_gridWidth && m_GI->m_rightCol == m_GI->m_numberCols){
            rightBlank = rect.right;
        }

        //check to see if the bottom of the rect is past the bottom of the
        //grid drawing area, if so then break
        if(rect.bottom > m_GI->m_gridHeight){
            m_GI->m_bottomRow = row;
            break;
        }

        //check for extra rows
        if(y >= (m_GI->m_numberRows-1)){
            long origNumRows = m_GI->m_numberRows;
            long newRow = y+1;
            m_ctrl->VerifyCurrentRow(&newRow);
            if(m_GI->m_numberRows > origNumRows){
                m_drawHint.AddHint(0,y+1,m_GI->m_numberCols-1,y+1);
            }
        }
    }

    //clear the draw hints
    m_drawHint.ClearHints();

    //check to see if there is blank space on the bottom of the grid
    //drawing area
    if(rect.bottom < m_GI->m_gridHeight && m_GI->m_bottomRow == m_GI->m_numberRows)
        bottomBlank = rect.bottom;

    //fill in the blank grid drawing areas
    if(rightBlank >=0 || bottomBlank >=0){
        CBrush brush(m_ctrl->OnGetDefBackColor(UG_GRID));
        GetClientRect(&rect);
        if(rightBlank >=0){
            rect.left = rightBlank;
            dc->FillRect(&rect,&brush);
        }
        if(bottomBlank >=0){
            rect.left = 0;
            rect.top = bottomBlank;
            dc->FillRect(&rect,&brush);
        }
    }

    //double buffering
    if(m_doubleBufferMode){
        dc = origDC;
        dc->BitBlt(0,0,m_GI->m_gridWidth,m_GI->m_gridHeight,db_dc,0,0,SRCCOPY);
    }

    //draw the focus rect, if the flag was set above
    if(m_hasFocus || m_ctrl->m_findDialogRunning ){
        if(m_GI->m_highlightRowFlag){
            focusRect.left = 0;

            if(rect.right < m_GI->m_gridWidth)
                focusRect.right = rect.right;
            else
                focusRect.right = m_GI->m_gridWidth;
        }
        m_ctrl->OnDrawFocusRect(dc,&focusRect);
    }
}

/************************************************
*************************************************/
void CUGGrid::RedrawCell(int col,long row){
    m_drawHint.AddHint(col,row);
    Moved();
}
/************************************************
*************************************************/
void CUGGrid::RedrawRow(long row){
    m_drawHint.AddHint(0,row,m_GI->m_numberCols-1,row);
    Moved();
}
/************************************************
*************************************************/
void CUGGrid::RedrawCol(int col){
    m_drawHint.AddHint(col,0,col,m_GI->m_numberRows-1);
    Moved();
}
/************************************************
*************************************************/
void CUGGrid::RedrawRange(int startCol,long startRow,int endCol,long endRow){
    m_drawHint.AddHint(startCol,startRow,endCol,endRow);
    Moved();
}
/************************************************
Update
    This routine recalcs the internal variables
    plus causes the grid to re-draw itself
*************************************************/
void CUGGrid::Update(){

    InvalidateRect(NULL);
    UpdateWindow();
    return;
}

/************************************************
Update
    This routine recalcs the internal variables
    plus causes the grid to re-draw itself
*************************************************/
void CUGGrid::Moved(){

    int xScroll = 0;
    int yScroll = 0;
    int x;
    long y;
    CDC * pDC, *screenDC,*db_DC;

    pDC = screenDC = db_DC = NULL;
    if(GetUpdateRect(NULL)){
        UpdateWindow();
        return;
    }
    if(m_ctrl->m_findDialogRunning){
        Invalidate();
        UpdateWindow();
        return;
    }

    BOOL redrawAll = FALSE;

    //if the left col has changed then calc the scroll distance
    if(m_GI->m_leftCol != m_GI->m_lastLeftCol){

        if(m_GI->m_leftCol > m_GI->m_lastLeftCol){
            for(x = m_GI->m_lastLeftCol;x < m_GI->m_leftCol; x++){
                xScroll += m_ctrl->GetColWidth(x);
                if(xScroll > m_GI->m_gridWidth){
                    redrawAll = TRUE;
                    break;
                }
            }
        }
        else{
            for(x = m_GI->m_leftCol;x < m_GI->m_lastLeftCol; x++){
                xScroll -= m_ctrl->GetColWidth(x);
                if(xScroll < -m_GI->m_gridWidth){
                    redrawAll = TRUE;
                    break;
                }
            }
        }
    //  if(true /*redrawAll*/ ){    // smg: force the redraw, see note #1 in stdafx.cpp
        //The above code should be a problem in data entry performance
        if(redrawAll){
            xScroll = 0;
            //redraw the whole grid
            m_drawHint.AddHint(0,0,m_GI->m_numberCols,m_GI->m_numberRows);
        }
    }

    //if the top row has changed then calc the scroll distance
    if(m_GI->m_topRow != m_GI->m_lastTopRow){

        if(m_GI->m_topRow > m_GI->m_lastTopRow){
            for(y = m_GI->m_lastTopRow;y < m_GI->m_topRow; y++){
                yScroll += m_ctrl->GetRowHeight(y);
                if(yScroll > m_GI->m_gridHeight){
                    redrawAll = TRUE;
                    break;
                }
            }
        }
        else{
            for(y = m_GI->m_topRow;y < m_GI->m_lastTopRow; y++){
                yScroll -= m_ctrl->GetRowHeight(y);
                if(yScroll < -m_GI->m_gridHeight){
                    redrawAll = TRUE;
                    break;
                }
            }
        }
        if(true /*redrawAll*/ ){
            yScroll = 0;
            //redraw the whole grid
            m_drawHint.AddHint(0,0,m_GI->m_numberCols,m_GI->m_numberRows);
        }
    }

    //create the dc
    screenDC = GetDC();
    if(m_doubleBufferMode){
        pDC = new CDC;
        pDC->CreateCompatibleDC(NULL);
        pDC->SelectObject(m_bitmap);
        db_DC = pDC;
    }
    else{
        pDC = screenDC;
    }

    //do scrolling of the window here
    if(xScroll != 0 || yScroll != 0){
        RECT scrollRect;
        GetClientRect(&scrollRect);
        if(xScroll == 0)
            scrollRect.top += m_GI->m_lockRowHeight;
        if(yScroll == 0)
            scrollRect.left += m_GI->m_lockColWidth;
        pDC->ScrollDC(-xScroll,-yScroll,&scrollRect,&scrollRect,NULL,NULL);

        //add the draw hints for the area the was uncovered by the scroll
        if(xScroll >0)
            m_drawHint.AddHint(m_GI->m_rightCol,0,m_GI->m_numberCols,m_GI->m_numberRows);
        else if(xScroll < 0)
            m_drawHint.AddHint(0,0,m_GI->m_lastLeftCol,m_GI->m_numberRows);
        if(yScroll >0)
            m_drawHint.AddHint(0,m_GI->m_bottomRow,m_GI->m_numberCols,m_GI->m_numberRows);
        else if(yScroll < 0)
            m_drawHint.AddHint(0,0,m_GI->m_numberCols,m_GI->m_lastTopRow);
    }

    //add the last and current cells - add row support later
    if(m_GI->m_highlightRowFlag){
        m_drawHint.AddHint(0,m_GI->m_lastRow,m_GI->m_numberCols,m_GI->m_lastRow);
        m_drawHint.AddHint(0,m_GI->m_currentRow,m_GI->m_numberCols,m_GI->m_currentRow);
    }
    else{
        m_drawHint.AddHint(m_GI->m_lastCol,m_GI->m_lastRow);
        m_drawHint.AddHint(m_GI->m_currentCol,m_GI->m_currentRow);
    }

    //call the grids main drawing routine
    DrawCellsIntern(screenDC,db_DC);

    //double buffering
    if(m_doubleBufferMode){
        delete db_DC;
    }


    //close the device context
    ReleaseDC(screenDC);
}

/************************************************
OnKeyDown
    Passes all keystrokes to a callback routine
    then processes them, if the callback allows
*************************************************/
void CUGGrid::OnKeyDown(UINT nChar, UINT nRepCnt, UINT nFlags){

    m_GI->m_moveType = 0;   //key(default)

    int increment = 1; //default number of units to move

    //keyboard balistics
    if(m_GI->m_ballisticKeyMode > 0){

        m_keyRepeatCount++;

        int value = (m_keyRepeatCount / m_GI->m_ballisticKeyMode);
        increment = value*value*value + 1;

        if(m_GI->m_ballisticKeyDelay >0){
            if(value == 0 && m_keyRepeatCount >1){
                #ifdef WIN32
                Sleep(m_GI->m_ballisticKeyDelay);
                #endif
            }
        }
    }


    //send a notify message to the cell type class
    BOOL processed = m_ctrl->GetCellType(m_GI->m_currentCol,m_GI->m_currentRow)->
        OnKeyDown(m_GI->m_currentCol,m_GI->m_currentRow,&nChar);

    //send a keydown notify message
    m_ctrl->OnKeyDown(&nChar,processed);

    //process the keystroke
    if(nChar == VK_LEFT)
        m_ctrl->GotoCol(m_GI->m_dragCol - increment);
    else if(nChar == VK_RIGHT)
        m_ctrl->GotoCol(m_GI->m_dragCol + increment);
    else if(nChar == VK_UP)
        m_ctrl->GotoRow(m_GI->m_dragRow - increment);
    else if(nChar == VK_DOWN)
        m_ctrl->GotoRow(m_GI->m_dragRow + increment);
    else if(nChar == VK_PRIOR)
        m_ctrl->MoveCurrentRow(UG_PAGEUP);
    else if(nChar == VK_NEXT)
        m_ctrl->MoveCurrentRow(UG_PAGEDOWN);
    else if(nChar == VK_HOME){
        if(GetKeyState(VK_CONTROL) <0)
            m_ctrl->MoveCurrentRow(UG_TOP);
        else
            m_ctrl->MoveCurrentCol(UG_TOP);
    }
    else if(nChar == VK_END){
        if(GetKeyState(VK_CONTROL) <0)
            m_ctrl->MoveCurrentRow(UG_BOTTOM);
        else
            m_ctrl->MoveCurrentCol(UG_BOTTOM);
    }
}

/************************************************
OnLButtonDown
    Finds the cell that was clicked in
    Sends a notification
    updates the current cells position
    Captures the mouse focus
*************************************************/
void CUGGrid::OnLButtonDown(UINT nFlags, CPoint point){

    if(m_ctrl->m_editInProgress)
        return;

    int col = -1;
    long row = -1;
    RECT rect;
    BOOL processed = FALSE;

    SetCapture();

    //setup the move type flags
    m_GI->m_moveType = 1;   //lbutton
    m_GI->m_moveFlags = nFlags;

    //check to see what cell was clicked in, and move there
    if(m_ctrl->GetCellFromPoint(point.x,point.y,&col,&row,&rect) == UG_SUCCESS){

        m_ctrl->GotoCell(col,row);

        //send a notification to the cell type
        processed = m_ctrl->GetCellType(col,row)->OnLClicked(col,row,1,&rect,&point);

        if(processed)
            m_cellTypeCapture = TRUE;
    }

    m_ctrl->OnLClicked(col,row,1,&rect,&point,processed);

    m_GI->m_moveType = 0;   //key(default)
}

/************************************************
OnLButtonUp
    Finds the cell that was clicked in
    Sends a notification
    Releases the mouse focus
*************************************************/
void CUGGrid::OnLButtonUp(UINT nFlags, CPoint point){

    int col = -1;
    long row = -1;
    RECT rect;
    BOOL processed = FALSE;

    if(m_ctrl->GetCellFromPoint(point.x,point.y,&col,&row,&rect) == UG_SUCCESS){

        //send a notification to the cell type
        processed = m_ctrl->GetCellType(col,row)->OnLClicked(col,row,0,&rect,&point);
    }

    m_ctrl->OnLClicked(col,row,0,&rect,&point,processed);

    ReleaseCapture();

    m_cellTypeCapture = FALSE;
}

/************************************************
OnLButtonDblClk
    Finds the cell that was clicked in
    Sends a notification
*************************************************/
void CUGGrid::OnLButtonDblClk(UINT nFlags, CPoint point){

    int col = -1;
    long row = -1;
    RECT rect;
    BOOL processed = FALSE;

    if(m_ctrl->GetCellFromPoint(point.x,point.y,&col,&row,&rect) == UG_SUCCESS){

        //send a notification to the cell type
        processed = m_ctrl->GetCellType(col,row)->OnDClicked(col,row,&rect,&point);
    }

    m_ctrl->OnDClicked(col,row,&rect,&point,processed);
}

/************************************************
OnMouseMove
    Finds the cell that the mouse is in and if the
    left mouse button is down then the current cell
    postion is updated
    Sends optional notification messages
    Also scrolls the current cell if the mouse is
    outside the window area
*************************************************/
void CUGGrid::OnMouseMove(UINT nFlags, CPoint point){

    int col;
    long row;

    if(m_ctrl->m_editInProgress)
        return;

    if(m_cellTypeCapture){
        col = m_GI->m_currentCol;
        row = m_GI->m_currentRow;
        //send a notification to the cell type
        BOOL processed = m_ctrl->GetCellType(col,row)->OnMouseMove(col,row,&point,nFlags);
        //send a notification to the main grid class
        m_ctrl->OnMouseMove(col,row,&point,nFlags,processed);

        return;
    }


    m_GI->m_moveType = 3;   //mouse move
    m_GI->m_moveFlags = nFlags;

    //check to see if the mouse is over a cell
    if(m_ctrl->GetCellFromPoint(point.x,point.y,&col,&row) == UG_SUCCESS){
        //send a notification to the cell type
        BOOL processed = m_ctrl->GetCellType(col,row)->OnMouseMove(col,row,&point,nFlags);
        //send a notification to the main grid class
        m_ctrl->OnMouseMove(col,row,&point,nFlags,processed);
    }

    if(nFlags & MK_LBUTTON){

        MSG msg;
        int moved = FALSE;

        while(1){
            //if the mouse is off the left side
            if(point.x < 0){
                //if ballistic mode
                if(m_GI->m_ballisticMode){
                    int increment = (int)pow( (double) (point.x * -1)/ m_GI->m_defColWidth +1,
                        m_GI->m_ballisticMode);
                    m_ctrl->GotoCol(m_GI->m_dragCol - increment);
                    if(increment == 1)
                        Sleep(m_GI->m_ballisticDelay);
                }
                else
                    m_ctrl->MoveCurrentCol(UG_LINEUP);
                moved = TRUE;
            }
            //if the mouse is off the right side
            else if(point.x > m_GI->m_gridWidth){
                //if ballistic mode
                if(m_GI->m_ballisticMode){
                    int increment = (int)pow((double) (point.x - m_GI->m_gridWidth) /
                        m_GI->m_defColWidth +1, m_GI->m_ballisticMode);
                    m_ctrl->GotoCol(m_GI->m_dragCol + increment);
                    if(increment == 1)
                        Sleep(m_GI->m_ballisticDelay);
                }
                else
                    m_ctrl->MoveCurrentCol(UG_LINEDOWN);
                moved = TRUE;
            }
            //if the mouse is above the top
            if(point.y < 0){
                //if ballistic mode
                if(m_GI->m_ballisticMode){
                    long increment = (long)pow((double) (point.y* -1) / m_GI->m_defRowHeight +1,
                        m_GI->m_ballisticMode);
                    m_ctrl->GotoRow(m_GI->m_dragRow - increment);
                    if(increment == 1)
                        Sleep(m_GI->m_ballisticDelay);
                }
                else
                    m_ctrl->MoveCurrentRow(UG_LINEUP);
                moved = TRUE;
            }
            //if the mouse is below the bottom
            else if(point.y > m_GI->m_gridHeight){
                //if ballistic mode
                if(m_GI->m_ballisticMode){
                    long increment = (long)pow((double) (point.y-m_GI->m_gridHeight) /
                        m_GI->m_defRowHeight +1, m_GI->m_ballisticMode);
                    m_ctrl->GotoRow(m_GI->m_dragRow + increment);
                    if(increment == 1)
                        Sleep(m_GI->m_ballisticDelay);
                }
                else
                    m_ctrl->MoveCurrentRow(UG_LINEDOWN);
                moved = TRUE;
            }

            //if the moved flag was not set then the mouse must be within
            //the grid area itself
            if(!moved){
                if(m_ctrl->GetCellFromPoint(point.x,point.y,&col,&row) == UG_SUCCESS){

                    //send a notification to the cell type

                    //
                    if(m_GI->m_numLockCols == 0 && m_GI->m_numLockRows == 0)
                        m_ctrl->GotoCell(col,row);
                    else{
                        if(m_GI->m_dragCol > m_GI->m_numLockCols && col < m_GI->m_numLockCols)
                            m_ctrl->MoveCurrentCol(UG_LINEUP);
                        else
                            m_ctrl->GotoCol(col);

                        if(m_GI->m_dragRow > m_GI->m_numLockRows && row < m_GI->m_numLockRows)
                            m_ctrl->MoveCurrentRow(UG_LINEUP);
                        else{
                            m_ctrl->GotoRow(row);
                        }
                    }
                }
            }

            moved = FALSE;

            //check for messages, if there are not then scroll some more
            while(PeekMessage(&msg,NULL,0,0,PM_NOREMOVE)){
                if(msg.message == WM_MOUSEMOVE || msg.message == WM_LBUTTONUP){

                    m_GI->m_moveType = 0;   //key(default)

                    return;
                }
                GetMessage(&msg,NULL,0,0);
                TranslateMessage(&msg);
                DispatchMessage(&msg);
            }
        }
    }

    m_GI->m_moveType = 0;   //key - default
}

/************************************************
OnRButtonDown
    Finds the cell that the mouse is in
    Sends a notification
    Pops up a menu if one exists
*************************************************/
void CUGGrid::OnRButtonDown(UINT nFlags, CPoint point){

    if(m_ctrl->m_editInProgress)
        return;

    int col = -1;
    long row = -1;
    BOOL processed = FALSE;
    RECT rect;

    m_GI->m_moveType = 2;   //2button
    m_GI->m_moveFlags = nFlags;

    if(m_ctrl->GetCellFromPoint(point.x,point.y,&col,&row,&rect) == UG_SUCCESS){
        m_ctrl->GotoCell(col,row);

        //send a notification to the cell type
        processed = m_ctrl->GetCellType(col,row)->OnRClicked(col,row,1,&rect,&point);
    }


    m_ctrl->OnRClicked(col,row,1,&rect,&point,processed);


    m_GI->m_moveType = 0;   //key(default)

    if(m_GI->m_enablePopupMenu){
        ClientToScreen(&point);
        m_ctrl->StartMenu(col,row,&point,UG_GRID);
    }

    m_GI->m_moveType = 0;//key - default
}

/************************************************
OnRButtonUp
    Finds the cell that the mouse is in
    Sends a notification
    Destroys the menu if it was enabled
*************************************************/
void CUGGrid::OnRButtonUp(UINT nFlags, CPoint point){

    int col = -1;
    long row = -1;
    RECT rect;
    BOOL processed = FALSE;

    if(m_ctrl->GetCellFromPoint(point.x,point.y,&col,&row,&rect) == UG_SUCCESS){

        //send a notification to the cell type
        processed = m_ctrl->GetCellType(col,row)->OnRClicked(col,row,0,&rect,&point);
    }

    m_ctrl->OnRClicked(col,row,0,&rect,&point,processed);
}

/************************************************
OnChar
    Sends all messages to the main class
*************************************************/
void CUGGrid::OnChar(UINT nChar, UINT nRepCnt, UINT nFlags){

    BOOL processed = FALSE;

    //send a notification to the cell type
    processed = m_ctrl->GetCellType(m_GI->m_currentCol,m_GI->m_currentRow)->
        OnCharDown(m_GI->m_currentCol,m_GI->m_currentRow,&nChar);

    m_ctrl->OnCharDown(&nChar,processed);
}

/************************************************
*************************************************/
BOOL CUGGrid::PreCreateWindow(CREATESTRUCT& cs){
    // TODO: Add your specialized code here and/or call the base class
    cs.style = cs.style | WS_CLIPCHILDREN | WS_CLIPSIBLINGS;
    return CWnd::PreCreateWindow(cs);
}
/************************************************
*************************************************/
void CUGGrid::OnSetFocus(CWnd* pOldWnd){

    m_hasFocus = TRUE;

    m_cellTypeCapture = FALSE;

    m_keyRepeatCount = 0;

    CWnd::OnSetFocus(pOldWnd);

    m_ctrl->OnSetFocus(UG_GRID);

    RedrawCell(m_GI->m_currentCol,m_GI->m_currentRow);

}
/************************************************
*************************************************/
void CUGGrid::OnKillFocus(CWnd* pNewWnd){

    m_hasFocus = FALSE;

    CWnd::OnKillFocus(pNewWnd);
    RedrawCell(m_GI->m_currentCol,m_GI->m_currentRow);
    m_ctrl->OnKillFocus(UG_GRID);
}
/************************************************
OnMouseActivate
    Activates the grid
*************************************************/
int CUGGrid::OnMouseActivate(CWnd* pDesktopWnd, UINT nHitTest, UINT message)
{
    SetFocus();
    return MA_ACTIVATE;
}

/************************************************
*************************************************/
int CUGGrid::SetDoubleBufferMode(int mode){

    if(m_bitmap != NULL){
        delete m_bitmap;
        m_bitmap = NULL;
    }

    m_doubleBufferMode = mode;

    if(mode == 1){
        m_bitmap = new CBitmap;

        CDC * pDC = GetDC();
        RECT rect;

        GetClientRect(&rect);
        if(rect.right ==0)
            rect.right =1;
        if(rect.bottom ==0)
            rect.bottom =1;

        if(m_bitmap->CreateCompatibleBitmap(pDC,rect.right,rect.bottom) == 0){
            delete m_bitmap;
            m_bitmap = NULL;
            m_doubleBufferMode = FALSE;
            return 1;
        }

        //clear the bitmap
        CBitmap * oldbitmap = pDC->SelectObject(m_bitmap);
        CBrush brush(m_ctrl->OnGetDefBackColor(UG_GRID));
        pDC->FillRect(&rect,&brush);
        if(oldbitmap)
            pDC->SelectObject(oldbitmap);

        ReleaseDC(pDC);

    }

    return UG_SUCCESS;
}

/************************************************
*************************************************/
void CUGGrid::OnSize(UINT nType, int cx, int cy)
{
    CWnd::OnSize(nType, cx, cy);

    // TODO: Add your message handler code here
    GetClientRect(&m_clientRect);

    if(m_doubleBufferMode >0)
        SetDoubleBufferMode(m_doubleBufferMode);
}

/************************************************
*************************************************/
#ifdef WM_MOUSEWHEEL
BOOL CUGGrid::OnMouseWheel(UINT nFlags, short zDelta, CPoint pt)
{
    // TODO: Add your message handler code here and/or call default
    int distance = zDelta / 120;
    m_ctrl->SetTopRow(m_GI->m_topRow - (distance * 3));

    return CWnd::OnMouseWheel(nFlags, zDelta, pt);
}
#endif

/************************************************
*************************************************/
BOOL CUGGrid::OnSetCursor(CWnd* pWnd, UINT nHitTest, UINT message)
{
    SetCursor(m_GI->m_arrowCursor);
    return 1;
}

/************************************************
*************************************************/
void CUGGrid::OnKeyUp(UINT nChar, UINT nRepCnt, UINT nFlags)
{
    m_keyRepeatCount = 0;

    //CWnd::OnKeyUp(nChar, nRepCnt, nFlags);
}

/************************************************
*************************************************/
UINT CUGGrid::OnGetDlgCode()
{
    // TODO: Add your message handler code here and/or call default
    if(IsChild(GetFocus()) )
        return DLGC_WANTALLKEYS|DLGC_WANTARROWS;
    else
        return CWnd::OnGetDlgCode() | DLGC_WANTARROWS | DLGC_WANTALLKEYS;
}

/************************************************
*************************************************/
void CUGGrid::OnDestroy()
{

    CWnd::OnDestroy();

    // TODO: Add your message handler code here
}
