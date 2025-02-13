/***********************************************
    Ultimate Grid 97
    Copyright 1994 - 1997 Dundas Software Ltd.


    class
        CUGMultiSelect
    Purpose
        This class maintains the multiple selection
        list for the grid. This class can also enumerate
        the selections as well.
    Details
        -It is responsible for
         figuring out what gets selected and what does
         not, since keyboard and mouse messages are
         passed to this class
        -This class updates the grid's CUGDrawHint class
         to notify the grid of which cells need redrawing
         if their selection state has changed.
        -this class is also responsible for returning
         queries on a cells selection status, as well
         as full selection enumeration.
************************************************/

#include "StdAfx.h"


#include "Ugctrl.h"
//#include "UGMultiS.h"

#ifdef _DEBUG
#define new DEBUG_NEW
#undef THIS_FILE
static char THIS_FILE[]= __FILE__;
#endif

/***************************************************
****************************************************/
CUGMultiSelect::CUGMultiSelect(){

    //set up the variables
    m_list              = NULL;
    m_blockInProgress   = FALSE;
    m_mode              = 0;    //selection mode  0:off  1:cell  2:row
    m_enumInProgress    = FALSE;
    m_numberBlocks      = 0;
    m_blockJustStarted  = FALSE;
    m_lastCol           = -1;
    m_lastRow           = -1;

}
/***************************************************
****************************************************/
CUGMultiSelect::~CUGMultiSelect(){

    //perform clean-up
    m_numberBlocks      = 0;
    ClearAll();
}
/***************************************************
****************************************************/
int CUGMultiSelect::ClearAll(){

    UG_MSList *next;

    m_blockInProgress = FALSE;

    //check to see if there is a list
    if(m_list == NULL)
        return UG_SUCCESS;

    //delete the list
    do{
        next = m_list->next;
        delete m_list;
        m_list = next;
    }while(m_list != NULL);

    if(m_numberBlocks > 1)
        m_ctrl->RedrawAll();

    m_enumInProgress    = FALSE;
    m_numberBlocks      = 0;
    m_lastCol           = -1;
    m_lastRow           = -1;


    return UG_SUCCESS;
}

/***************************************************
****************************************************/
int CUGMultiSelect::StartBlock(int col,long row){

    if((m_mode&3) == 0)
        return UG_SUCCESS;

    //if the first select block
    m_origCol = col;
    m_origRow = row;

    //update the start variables
    m_startCol = col;
    m_startRow = row;
    m_blockInProgress = TRUE;

    //create a new item
    m_currentItem = new UG_MSList;
    m_currentItem->startCol =   col;
    m_currentItem->endCol   =   col;
    m_currentItem->loCol    =   col;
    m_currentItem->hiCol    =   col;
    m_currentItem->startRow =   row;
    m_currentItem->endRow   =   row;
    m_currentItem->loRow    =   row;
    m_currentItem->hiRow    =   row;

    m_currentItem->selected =   TRUE;
    m_currentItem->next     =   NULL;

    if(m_mode&UG_MULTISELECT_ROW){
        m_currentItem->loCol    =   0;
        m_currentItem->hiCol    =   m_ctrl->GetNumberCols();
    }

    //add the item to the list
    if(m_list != NULL){
        //find the last item, then append the new one
        UG_MSList * next = m_list;
        while(next->next != NULL){
            next = next->next;
        }
        next->next = m_currentItem;
    }
    else{
        //make this item the first
        m_list = m_currentItem;
    }

    m_blockJustStarted = TRUE;

    m_numberBlocks++;

    return UG_SUCCESS;
}

/***************************************************
****************************************************/
int CUGMultiSelect::GetOrigCell(int *col,long *row){

    if(m_blockInProgress == FALSE)
        return 1;

    *col = m_origCol;
    *row = m_origRow;

    return UG_SUCCESS;
}

/***************************************************
****************************************************/
int CUGMultiSelect::EndBlock(int col,long row){

    if((m_mode&3) ==0)
        return UG_SUCCESS;

    if( m_blockInProgress == FALSE)
        return 1;


    int maxLoCol   = m_currentItem->loCol;
    int maxHiCol   = m_currentItem->hiCol;
    long maxLoRow  = m_currentItem->loRow;
    long maxHiRow  = m_currentItem->hiRow;

    int oldEndCol = m_currentItem->endCol;
    int oldEndRow = m_currentItem->endRow;

    m_currentItem->endCol   =   col;
    m_currentItem->endRow   =   row;

    if(m_currentItem->endCol > m_currentItem->startCol){
        m_currentItem->loCol = m_currentItem->startCol;
        m_currentItem->hiCol = m_currentItem->endCol;
    }
    else{
        m_currentItem->hiCol = m_currentItem->startCol;
        m_currentItem->loCol = m_currentItem->endCol;
    }
    if(m_currentItem->endRow > m_currentItem->startRow){
        m_currentItem->loRow = m_currentItem->startRow;
        m_currentItem->hiRow = m_currentItem->endRow;
    }
    else{
        m_currentItem->hiRow = m_currentItem->startRow;
        m_currentItem->loRow = m_currentItem->endRow;
    }

    if(m_mode&UG_MULTISELECT_ROW){
        m_currentItem->loCol    =   0;
        m_currentItem->hiCol    =   m_ctrl->GetNumberCols();
    }


    m_currentItem->selected =   TRUE;

    if(m_blockJustStarted){
        m_blockJustStarted = FALSE;
        m_numberBlocks++;
    }

    if(maxLoCol > m_currentItem->loCol)
        maxLoCol = m_currentItem->loCol;
    if(maxHiCol < m_currentItem->hiCol)
        maxHiCol = m_currentItem->hiCol;
    if(maxLoRow > m_currentItem->loRow)
        maxLoRow = m_currentItem->loRow;
    if(maxHiRow < m_currentItem->hiRow)
        maxHiRow = m_currentItem->hiRow;

    //add draw hints for the grid
    if(m_currentItem->endRow > oldEndRow){
        m_ctrl->m_CUGGrid->m_drawHint.AddHint(maxLoCol,
            oldEndRow,maxHiCol,m_currentItem->endRow);
    }
    else if(m_currentItem->endRow < oldEndRow){
        m_ctrl->m_CUGGrid->m_drawHint.AddHint(maxLoCol,
            m_currentItem->endRow,maxHiCol,oldEndRow);
    }
    if(m_currentItem->endCol > oldEndCol){
        m_ctrl->m_CUGGrid->m_drawHint.AddHint(oldEndCol,
            maxLoRow,m_currentItem->endCol,maxHiRow);
    }
    else if(m_currentItem->endCol < oldEndCol){
        m_ctrl->m_CUGGrid->m_drawHint.AddHint(m_currentItem->endCol,
            maxLoRow,oldEndCol,maxHiRow);
    }

    return UG_SUCCESS;
}
/***************************************************
****************************************************/
int CUGMultiSelect::ToggleCell(int col,long row){

    if((m_mode&3) == 0)
        return UG_SUCCESS;



    //find the block
    int found = FALSE;
    UG_MSList * next = m_list;

    while(next != NULL){

        if(col == next->loCol && col == next->hiCol){
            if(row == next->loRow && row == next->hiRow){
                if(next->selected)
                    next->selected = FALSE;
                else
                    next->selected = TRUE;
                found = TRUE;
                break;
            }
        }
        next = next->next;
    }
    //if there is no entry for this cell then add an entry
    if(!found){
        //check to see if this cell is in a block
        int selected = IsSelected(col,row);
        //add the cell
        StartBlock(col,row);
        if(selected)
            m_currentItem->selected =   FALSE;
        else
            m_currentItem->selected =   TRUE;
    }

    return UG_SUCCESS;
}

/***************************************************
****************************************************/
int CUGMultiSelect::GetCurrentBlock(int *startCol,long *startRow,int *endCol,long *endRow){

    if((m_mode&3) == 0)
        return UG_SUCCESS;

    if(m_blockInProgress == FALSE)
        return 1;

    *startCol = m_currentItem->loCol;
    *endCol =   m_currentItem->hiCol;
    *startRow = m_currentItem->loRow;
    *endRow =   m_currentItem->hiRow;

    return UG_SUCCESS;
}

/***************************************************
blocks start from 0 to n-1
****************************************************/
int CUGMultiSelect::IsSelected(int col,long row,int *block){

    if((m_mode&3) == 0)
        return FALSE;

    if(m_list == NULL)
        return FALSE;


    int blockNum = 0;
    int blockCount = 0;

    //go through the list and check to see if the given
    //cell is in it
    int selected = FALSE;
    UG_MSList * next = m_list;

    while(next != NULL){

        if(col >= next->loCol && col <= next->hiCol){
            if(row >= next->loRow && row <= next->hiRow){
                selected = next->selected;
                blockNum = blockCount;
                if(m_mode&16) //if no deselect
                    break;
            }
        }

        blockCount++;

        next = next->next;
    }

    if(block != NULL)
        *block = blockNum;

    return selected;
}
/***************************************************
****************************************************/
int CUGMultiSelect::IsCellInColSelected(int col){

    if((m_mode&3) == 0)
        return FALSE;

    if(m_list == NULL)
        return FALSE;

    if(m_mode&UG_MULTISELECT_ROW)
        col = 1;

    //go through the list and check to see if the given
    //cell is in it
    int selected = FALSE;
    UG_MSList * next = m_list;

    while(next != NULL){

        if(col >= next->loCol && col <= next->hiCol){
            selected = next->selected;
            if(m_mode&16) //if no deselect
                break;
        }

        next = next->next;
    }

    return selected;
}
/***************************************************
****************************************************/
int CUGMultiSelect::IsCellInRowSelected(long row){

    if((m_mode&3) == 0)
        return FALSE;

    if(m_list == NULL)
        return FALSE;

    //go through the list and check to see if the given
    //cell is in it
    int selected = FALSE;
    UG_MSList * next = m_list;

    while(next != NULL){

        if(row >= next->loRow && row <= next->hiRow){
            selected = next->selected;
            if(m_mode&16) //if no deselect
                break;
        }

        next = next->next;
    }

    return selected;
}

/***************************************************
0: off  1:cell  2:row
8:  don't move current cell
16: no deselect
****************************************************/
int CUGMultiSelect::SelectMode(int mode){

    ClearAll();

    m_mode = mode;

    return UG_SUCCESS;
}
/***************************************************
****************************************************/
int CUGMultiSelect::GetSelectMode(){
    return m_mode;
}
/***************************************************
****************************************************/
int CUGMultiSelect::GetNumberBlocks(){
    return m_numberBlocks;
}
/***************************************************
****************************************************/
int CUGMultiSelect::EnumFirstSelected(int *col,long *row){

    //multiselect off
    if((m_mode&3) == 0){
        *col = m_ctrl->GetCurrentCol();
        *row = m_ctrl->GetCurrentRow();
        return UG_SUCCESS;
    }

    //no items selected
    if(m_list == NULL){
        *col = m_ctrl->GetCurrentCol();
        *row = m_ctrl->GetCurrentRow();
        return UG_SUCCESS;
    }

    //set up the vars
    m_enumInProgress= TRUE;
    m_enumStartCol  = m_ctrl->GetCurrentCol();
    m_enumStartRow  = m_ctrl->GetCurrentRow();
    m_enumEndCol    = m_ctrl->GetCurrentCol();
    m_enumEndRow    = m_ctrl->GetCurrentRow();

    //find the total selected region
    UG_MSList * next = m_list;

    while(next != NULL){
        if(m_enumStartCol > next->loCol)
            m_enumStartCol = next->loCol;
        if(m_enumEndCol < next->hiCol)
            m_enumEndCol = next->hiCol;
        if(m_enumStartRow > next->loRow)
            m_enumStartRow = next->loRow;
        if(m_enumEndRow < next->hiRow)
            m_enumEndRow = next->hiRow;

        next = next->next;
    }

    //find the first item
    int x;
    long y;
    //row
    for(y=m_enumStartRow;y<=m_enumEndRow;y++){
        for(x=m_enumStartCol;x<=m_enumEndCol;x++){
            if(IsSelected(x,y)){
                m_enumCol = x+1;
                m_enumRow = y;
                *col = x;
                *row = y;
                return UG_SUCCESS;
            }
        }
    }

    return 1;
}
/***************************************************
****************************************************/
int CUGMultiSelect::EnumNextSelected(int *col,long *row){

    if((m_mode&3)== 0)
        return 1;

    if(!m_enumInProgress)
            return 1;

    //find the next item
    int x;
    long y;
    //row
    for(y=m_enumRow;y<=m_enumEndRow;y++){
        for(x=m_enumCol;x<=m_enumEndCol;x++){
            if(IsSelected(x,y)){
                m_enumCol = x+1;
                m_enumRow = y;
                *col = x;
                *row = y;
                return UG_SUCCESS;
            }
        }
        m_enumCol = m_enumStartCol;
    }


    return 1;
}

/***************************************************
****************************************************/
int CUGMultiSelect::EnumFirstBlock(int *startCol,long *startRow,int *endCol,long *endRow){
    m_enumBlockNumber = 0;

    return EnumNextBlock(startCol,startRow,endCol,endRow);
}

/***************************************************
****************************************************/
int CUGMultiSelect::EnumNextBlock(int *startCol,long *startRow,int *endCol,long *endRow){

    int count           = 0;
    UG_MSList * next    = m_list;

    while(next != NULL && count < m_enumBlockNumber){
        next = next->next;
        count++;
    }
    if(next != NULL){
        *startCol = next->loCol;
        *startRow = next->loCol;
        *endCol = next->loCol;
        *endRow = next->loCol;

        return UG_SUCCESS;
    }
    return UG_ERROR;
}

/***************************************************
****************************************************/
void CUGMultiSelect::OnLClick(int col,long row, UINT nFlags){

    if((m_mode&3)== 0)
        return;

    if(col == m_lastCol && row == m_lastRow)
        return;

    m_lastCol = col;
    m_lastRow = row;

    if(nFlags&MK_CONTROL){
        StartBlock(col,row);
    }
    else if(nFlags&MK_SHIFT){
        EndBlock(col,row);
    }
    else{
        AddTotalRangeToDrawHints(&m_ctrl->m_CUGGrid->m_drawHint);
        ClearAll();
        StartBlock(col,row);
    }
}
/***************************************************
****************************************************/
void CUGMultiSelect::OnRClick(int col,long row, UINT nFlags){

    if((m_mode&3)== 0)
        return;

    if(IsSelected(col,row) == FALSE){
        AddTotalRangeToDrawHints(&m_ctrl->m_CUGGrid->m_drawHint);
        ClearAll();
        StartBlock(col,row);
    }
}
/***************************************************
****************************************************/
void CUGMultiSelect::OnKeyMove(int col,long row){

    if((m_mode&3)== 0)
        return;

    if(col == m_lastCol && row == m_lastRow)
        return;

    if(GetKeyState(VK_SHIFT) <0){
        EndBlock(col,row);
    }
    else if(GetKeyState(VK_CONTROL) <0){
        StartBlock(col,row);
    }
    else{
        AddTotalRangeToDrawHints(&m_ctrl->m_CUGGrid->m_drawHint);
        ClearAll();
        StartBlock(col,row);
    }
}

/***************************************************
****************************************************/
void CUGMultiSelect::OnMouseMove(int col,long row, UINT nFlags){

    if((m_mode&3)== 0)
        return;

    if(col == m_lastCol && row == m_lastRow)
        return;

    EndBlock(col,row);
}

/***************************************************
****************************************************/
int CUGMultiSelect::GetTotalRange(int *startCol,long *startRow,int *endCol,long *endRow){

    if((m_mode&3)== 0)
        return 1;

    if(m_list == NULL)
        return 2;

    if(m_mode&UG_MULTISELECT_ROW)
        *endCol = m_GI->m_numberCols -1;

    *startCol = m_list->loCol;
    *startRow = m_list->loRow;
    *endCol = m_list->hiCol;
    *endRow = m_list->hiRow;

    UG_MSList * next = m_list->next;

    while(next != NULL){

        if(next->loCol < *startCol)
            *startCol = next->loCol;

        if(next->loRow < *startRow)
            *startRow = next->loRow;

        if(next->hiCol < *endCol)
            *endCol = next->hiCol;

        if(next->hiRow < *endRow)
            *endRow = next->hiRow;

        next = next->next;
    }

    return UG_SUCCESS;
}

/***************************************************
****************************************************/
void CUGMultiSelect::AddTotalRangeToDrawHints(CUGDrawHint * hint){
    int startCol,endCol;
    long startRow,endRow;
    if(GetTotalRange(&startCol,&startRow,&endCol,&endRow) == UG_SUCCESS)
        hint->AddHint(startCol,startRow,endCol,endRow);
}
