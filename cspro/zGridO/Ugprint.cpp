/***********************************************
    Ultimate Grid 97
    Copyright 1994 - 1997 Dundas Software Ltd.


    class
        CUGPrint
    Purpose
        This class handles the setup and printing
        of the grid to a printer or other printing
        device.
    Details
        -This class has two printing sections,
         setup, and printing. Setup is required
         to figure out how many pages it will take
         to print a range of cells from the grid.
         Plus which cells to print on which pages.
         Once the setup is complete a page can
         now be printed.
        -Printing setup function: PrintInit
        -Page Printing function: PrintPage
        -The rest of the printing functons are optional
         and are used to setup different print options.
************************************************/

#include "StdAfx.h"

#include "Ugctrl.h"
//#include "UGPrint.h"

#ifdef _DEBUG
#define new DEBUG_NEW
#undef THIS_FILE
static char THIS_FILE[]= __FILE__;
#endif


#ifdef UG_ENABLE_PRINTING

/***************************************************
****************************************************/
CUGPrint::CUGPrint(){

    m_pagesWide = 0;
    m_pagesHigh = 0;
    m_printList = NULL;

    m_printLeftMargin   = 25;   //margin in mm
    m_printTopMargin    = 25;
    m_printRightMargin  = 25;
    m_printBottomMargin = 25;

    m_printFrame = TRUE;
    m_printCenter = FALSE;
    m_fitToPage = FALSE;
    m_printTopHeading = FALSE;
    m_printSideHeading = FALSE;

}

/***************************************************
****************************************************/
CUGPrint::~CUGPrint(){
    ClearPrintList();
}

/***************************************************
****************************************************/
int CUGPrint::ClearPrintList(){

    CUGPrintList* next;

    //delete all the items in the linked list
    while(m_printList != NULL){
        next = m_printList->next;
        delete m_printList;
        m_printList = next;
    }

    return TRUE;
}
/***************************************************
****************************************************/
int CUGPrint::AddPageToList(int page,int startCol,int endCol,long startRow,long endRow){

    CUGPrintList* item = NULL;  //new print list item

    //if the list is already empty then just return
    if(m_printList == NULL){
        m_printList = new CUGPrintList;
        item = m_printList;
        item->next = NULL;
    }
    else{
        //find the last item in the list
        CUGPrintList* current = m_printList;
        while(current != NULL){
            if(current->page == page){
                item = current;
                break;
            }
            if(current->next==NULL){
                item = new CUGPrintList;
                current->next = item;
                item->next = NULL;
                break;
            }
            current = current->next;
        }
    }

    item->page      = page;
    item->startCol  = startCol;
    item->endCol    = endCol;
    item->startRow  = startRow;
    item->endRow    = endRow;

    return TRUE;
}
/***************************************************
****************************************************/
int CUGPrint::GetPageFromList(int page,int *startCol,int *endCol,long *startRow,long *endRow){

    //find the page
    CUGPrintList* next = m_printList;
    while(next != NULL){
        if(next->page ==page)
            break;
        next = next->next;
    }

    //if the page was not found then return FALSE
    if(next == NULL)
        return FALSE;

    //return the page information
    *startCol   = next->startCol;
    *endCol     = next->endCol;
    *startRow   = next->startRow;
    *endRow     = next->endRow;

    return TRUE;
}
/***************************************************
****************************************************/
int CUGPrint::PrintInit(CDC * pDC, CPrintDialog* pPD, int startCol,long startRow,
                        int endCol,long endRow){

    //get a screen device context
    CDC sDC;
    sDC.CreateDC(_T("DISPLAY"), NULL, NULL, NULL);

    //get the metrics for the printer
    float paperWidthPX      = (float)pDC->GetDeviceCaps(HORZRES);
    float paperHeightPX     = (float)pDC->GetDeviceCaps(VERTRES);
    float paperWidthMM      = (float)pDC->GetDeviceCaps(HORZSIZE);
    float paperHeightMM     = (float)pDC->GetDeviceCaps(VERTSIZE);
    int virtualWidthPX = (int)( 3.79 * (paperWidthMM - m_printLeftMargin - m_printRightMargin));
    int virtualHeightPX = (int)( 3.78 * (paperHeightMM - m_printTopMargin - m_printBottomMargin));

    //check the print styles
    if(m_printTopHeading)
        virtualWidthPX -= m_GI->m_sideHdgWidth;
    if(m_printSideHeading)
        virtualHeightPX -= m_GI->m_topHdgHeight;


    //save the scaling values
    m_printVScale = ( (paperHeightPX/paperHeightMM) *
                    (paperHeightMM - m_printTopMargin - m_printBottomMargin) ) /
                    virtualHeightPX;
    m_printHScale = ( (paperWidthPX/paperWidthMM) *
                    (paperWidthMM - m_printLeftMargin - m_printRightMargin) ) /
                    virtualWidthPX;

    //save the margins
    m_printLeftMarginPX = (int)((paperWidthPX/paperWidthMM) * m_printLeftMargin);
    m_printTopMarginPX = (int)((paperHeightPX/paperHeightMM) * m_printTopMargin);
    m_printRightMarginPX = (int)(paperWidthPX - (paperWidthPX/paperWidthMM) * m_printRightMargin);
    m_printBottomMarginPX = (int)(paperHeightPX - (paperHeightPX/paperHeightMM) * m_printBottomMargin);
    m_paperWidthPX = (int)paperWidthPX;
    m_paperHeightPX = (int)paperHeightPX;


    //get the number of pages wide
    int col,width=0,w;
    int scol = startCol;        //start col for a page
    m_pagesWide = 0;

    if(m_printSideHeading)
        width = m_ctrl->GetSH_Width();
    //find the number of pages wide and the number of columns
    //on each page
    for(col =startCol;col <= endCol;col++){
        m_ctrl->GetColWidth(col,&w);
        width += w;

        if(width > virtualWidthPX){
            m_pagesWide++;
            if(m_printSideHeading)
                width = m_ctrl->GetSH_Width();
            else
                width = 0;

            if(w < virtualWidthPX){
                col--;
            }
            //add the column info to the list
            AddPageToList(m_pagesWide,scol,col,0,0);
            scol = col+1;
        }
    }
    if(width >0){
        m_pagesWide++;
        //add the column info to the list
        if(col > endCol)
            col = endCol;
        AddPageToList(m_pagesWide,scol,col,0,0);
    }


    //get the number of pages high
    long row;
    long srow = startRow;       //start row for page
    int height=0,h;
    m_pagesHigh = 0;

    if(m_printTopHeading)
        height = m_ctrl->GetTH_Height();

    //find the number of pages high and the number of rows
    //on each page
    for(row = startRow;row <=endRow;row++){
        m_ctrl->GetRowHeight(row,&h);
        height += h;
        if(height > virtualHeightPX){
            m_pagesHigh ++;
            if(m_printTopHeading)
                height = m_ctrl->GetTH_Height();
            else
                height = 0;

            if(h < virtualHeightPX)
                row--;

            //add the page information to the list
            int c1,c2;
            long r1,r2;
            int page;
            for(col =1;col<=m_pagesWide;col++){
                //get the col info
                GetPageFromList(col,&c1,&c2,&r1,&r2);
                //update the page info
                page = (m_pagesHigh-1)*m_pagesWide +col;
                AddPageToList(page,c1,c2,srow,row);
            }
            srow = row+1;
        }
    }
    if(height >0){
        m_pagesHigh++;
        //add the page information to the list
        int c1,c2;
        long r1,r2;
        int page;
        for(col =1;col<=m_pagesWide;col++){
            //get the col info
            GetPageFromList(col,&c1,&c2,&r1,&r2);
            //update the page info
            page = (m_pagesHigh-1)*m_pagesWide +col;
            if(row > endRow)
                row = endRow;
            AddPageToList(page,c1,c2,srow,row);
        }
    }

    //return the number of pages in total
    return (m_pagesWide * m_pagesHigh);
}
/***************************************************
****************************************************/
int CUGPrint::PrintPage(CDC * pDC, int pageNum){

    int  x,h,w,col,xx;
    long y,row,yy;
    int startCol,endCol;
    long startRow,endRow;
    RECT cellrect={0,0,0,0};
    RECT origCellRect;
    int leftMargin = m_printLeftMarginPX;
    int topMargin =  m_printTopMarginPX;

    //get the co-ords to print
    if(GetPageFromList(pageNum,&startCol,&endCol,&startRow,&endRow)!=TRUE)
        return FALSE;

    //*********************
    // draw the page
    //*********************

    //draw the grid
    cellrect.top = topMargin;
    cellrect.bottom = topMargin;

    //main grid row drawing loop
    yy = startRow;
    if(m_printTopHeading)
        yy = 0 - m_GI->m_numberTopHdgRows;
    for(y = yy; y <= endRow ; y++){

        //set up the rectangle for the row
        if(y < 0 ){
            //set up the rect for the top heading
            cellrect.top    = cellrect.bottom;
            cellrect.bottom+= (int)(m_ctrl->m_CUGTopHdg->GetTHRowHeight(y) * m_printVScale);
            cellrect.left   = leftMargin;
            cellrect.right  = cellrect.left;
        }
        else{
            if(yy < 0){
                yy=0;
                y = startRow;
            }
            //set up the rect for the row to be drawn
            cellrect.top    = cellrect.bottom;
            m_ctrl->GetRowHeight(y,&h);
            cellrect.bottom += (int)(h * m_printVScale);
            cellrect.left   = leftMargin ;
            cellrect.right  = cellrect.left;
        }

        //dont draw the row if it is 0 pixels high
        if(cellrect.top == cellrect.bottom)
            continue;

        //main column drawing loop
        xx =startCol;
        if(m_printSideHeading)
            xx = 0- m_GI->m_numberSideHdgCols;
        for(x = xx; x <= endCol; x++){
            if( x < 0 ){
                //get the side heading co-ords
                cellrect.left = cellrect.right;
                cellrect.right += (int)(m_ctrl->m_CUGSideHdg->GetSHColWidth(x) * m_printHScale);
            }
            else{
                if(xx < 0){
                    xx=0;
                    x = startCol;
                }
                //get the left,right co-ords
                cellrect.left = cellrect.right;
                m_ctrl->GetColWidth(x,&w);
                cellrect.right += (int)(w * m_printHScale);
            }

            //dont draw the column if it is 0 pixels wide
            if(cellrect.left == cellrect.right)
                continue;

            //store the original cell rect, incase it is modified below
            CopyRect(&origCellRect,&cellrect);

            col = x;
            row = y;

            //check to see if the cell is a joined cell
            if(m_ctrl->GetJoinStartCell(&col,&row)==UG_SUCCESS){
                RECT tempRect;
                CUGCell sourceCell, targetCell;
                int  col2, tempCol, tempCol2;
                long row2, tempRow, tempRow2;

                tempCol  = col;
                tempRow  = row;
                m_ctrl->GetJoinRange(&col,&row,&col2,&row2);
                tempCol2 = col2;
                tempRow2 = row2;

                if ((y == startRow && x == col) || (x == startCol && y == row)
                    || (y == startRow && x == startCol)){
                    m_ctrl->GetCell (col,row,&sourceCell);
                    m_ctrl->SetCell (x,y,&sourceCell);
                    m_ctrl->JoinCells (x,y,col2,row2);
                    col = x;
                    row = y;
                    m_ctrl->GetJoinRange(&col,&row,&col2,&row2);
                }

                if(col == x && row == y){
                    CopyRect(&tempRect,&cellrect);
                    if (col2 > endCol || row2 > endRow){
                        if (col2 > endCol)
                            col2 = endCol;
                        if (row2 > endRow)
                            row2 = endRow;
                        m_ctrl->JoinCells (col,row,col2,row2);
                    }
                    while(col2 > col){
                        if (col2 <= endCol){
                            m_ctrl->GetColWidth(col2,&w);
                            tempRect.right+=(int)(w* m_printHScale);
                        }
                        col2--;
                    }
                    while(row2 > row){
                        if (row2 <= endRow){
                            m_ctrl->GetRowHeight(row,&h);
                            tempRect.bottom+=(int)(h* m_printVScale);
                        }
                        row2--;
                    }
                    InternPrintCell(pDC,&tempRect,col,row);
                    m_ctrl->JoinCells (tempCol,tempRow,tempCol2,tempRow2);
                }
            }
            else{
                InternPrintCell(pDC,&cellrect,x,y);
            }

            //draw a section of the print frame if selected
            CopyRect(&cellrect,&origCellRect);
            if(m_printFrame){
                if(y == startRow){
                    pDC->MoveTo(cellrect.left,cellrect.top);
                    pDC->LineTo(cellrect.right+1,cellrect.top);
                }
                else if(y == endRow){
                    pDC->MoveTo(cellrect.left,cellrect.bottom);
                    pDC->LineTo(cellrect.right+1,cellrect.bottom);
                }
                if(x == startCol){
                    if (y < 0){
                        pDC->MoveTo(cellrect.left,cellrect.top);
                        pDC->LineTo(cellrect.left,topMargin);
                    }
                    pDC->MoveTo(cellrect.left,cellrect.top);
                    pDC->LineTo(cellrect.left,cellrect.bottom+1);
                }
                else if(x == endCol){
                    pDC->MoveTo(cellrect.right,cellrect.top);
                    pDC->LineTo(cellrect.right,cellrect.bottom+1);
                }
            }
        }
    }
    return UG_SUCCESS;
}
/***************************************************
****************************************************/
int CUGPrint::PrintSetMargin(int whichMargin,int size){
    return UG_SUCCESS;
}
/***************************************************
****************************************************/
int CUGPrint::PrintSetScale(double scale){
    return UG_SUCCESS;
}
/***************************************************
****************************************************/
int CUGPrint::PrintSetOption(int option,long param){

    if(option == UG_PRINT_LEFTMARGIN)
        m_printLeftMargin = (int)param;
    if(option == UG_PRINT_RIGHTMARGIN)
        m_printRightMargin =  (int)param;
    if(option == UG_PRINT_TOPMARGIN)
        m_printTopMargin =  (int)param;
    if(option == UG_PRINT_BOTTOMMARGIN)
        m_printBottomMargin =  (int)param;
    if(option == UG_PRINT_FRAME){
        if(param == FALSE)
            m_printFrame =  FALSE;
        else
            m_printFrame =  TRUE;
    }
    if(option == UG_PRINT_TOPHEADING){
        m_printTopHeading = param;
    }
    if(option == UG_PRINT_SIDEHEADING){
        m_printSideHeading = param;
    }

    return UG_SUCCESS;
}
/***************************************************
****************************************************/
int CUGPrint::PrintGetOption(int option,long *param) {

    if(option == UG_PRINT_LEFTMARGIN)
        return m_printLeftMargin;

    else if(option == UG_PRINT_RIGHTMARGIN)
        return m_printRightMargin;

    else if(option == UG_PRINT_TOPMARGIN)
        return m_printTopMargin;

    else if(option == UG_PRINT_BOTTOMMARGIN)
        return m_printBottomMargin;

    else if(option == UG_PRINT_FRAME)
        return m_printFrame;

    else
        return UG_ERROR;
}

/***************************************************
****************************************************/
void CUGPrint::InternPrintCell(CDC *dc,RECT * rect,int col,long row){

    CUGCellType * cellType;
    CUGCell cell,cell2;
    int leftMargin = m_printLeftMarginPX;
    int topMargin =  m_printTopMarginPX;

    if ( col < 0 && row < 0){
        rect->bottom = topMargin;
        rect->right = leftMargin;
        rect->left = (int)(leftMargin + ((m_GI->m_sideHdgWidth) * m_printHScale));
        rect->top = (int)(topMargin + ((m_GI->m_topHdgHeight) * m_printVScale));
    }
    //get the cell information
    m_ctrl->GetCellIndirect(col,row,&cell);

    //get the cell type to draw the cell
    if(cell.IsPropertySet(UGCELL_CELLTYPE_SET)){
        cellType = m_ctrl->GetCellType(cell.GetCellType());
    }
    else
        cellType = m_ctrl->GetCellType(-1);

    //paint the cell
    int saveID = dc->SaveDC();
    cellType->OnPrint(dc,rect,col,row,&cell);
    dc->RestoreDC(saveID);
}

#endif
