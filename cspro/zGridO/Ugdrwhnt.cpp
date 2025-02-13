/***********************************************
    Ultimate Grid 97
    Copyright 1994 - 1997 Dundas Software Ltd.

    Class
        CUGDrawHint
    Purpose
        This class is used internally by the grid
        to keep track of which cells need redrawing
        The grid draws its cells in an extremely
        optimized manner which gives it is great
        speed.
        This is the class which helps the optimization
        process by maintaining a list of cells that
        need to be redrawn.
    Datails
        -cells are added to this list by the grid
         when movement is made and/or changes to the
         grid are made. Only the cells that are
         affected are added.
        -when the grid is going to redraw itself
         it calls this classes IsInvalid function
         to see if the cell really needs to be
         redrawn
************************************************/

#include "StdAfx.h"
#include "Ugctrl.h"
//#include "ugdrwhnt.h"

#ifdef _DEBUG
#define new DEBUG_NEW
#undef THIS_FILE
static char THIS_FILE[]= __FILE__;
#endif


/******************************************
*******************************************/
CUGDrawHint::CUGDrawHint(){
    m_List = NULL;
    m_VList = NULL;
}
/******************************************
*******************************************/
CUGDrawHint::~CUGDrawHint(){
    ClearHints();
}
/******************************************
*******************************************/
void CUGDrawHint::AddHint(int col,long row){
    AddHint(col,row,col,row);
}
/******************************************
*******************************************/
void CUGDrawHint::AddHint(int startCol,long startRow,int endCol,long endRow){

    UGDrwHintList   *next = m_List;
    UGDrwHintList   *newhint = new UGDrwHintList;

    if(m_List != NULL){
        while(next->next != NULL){
            next = next->next;
        }
        next->next = newhint;
    }
    else{
        m_List = newhint;
        m_minCol = startCol;
        m_minRow = startRow;
        m_maxCol = endCol;
        m_maxRow = endRow;
    }

    newhint->next = NULL;
    newhint->startCol = startCol;
    newhint->startRow = startRow;
    newhint->endCol = endCol;
    newhint->endRow = endRow;

    if(startCol < m_minCol)
        m_minCol = startCol;
    if(endCol > m_maxCol)
        m_maxCol = endCol;
    if(startRow < m_minRow)
        m_minRow = startRow;
    if(endRow > m_maxRow)
        m_maxRow = endRow;
}
/******************************************
*******************************************/
void CUGDrawHint::AddToHint(int col,long row){

        AddToHint(col,row,col,row);
}
/******************************************
*******************************************/
void CUGDrawHint::AddToHint(int startCol,long startRow,int endCol,long endRow){

    if(m_List == NULL)
        AddHint(startCol,startRow,endCol,endRow);

    if(m_List->startCol > startCol)
        m_List->startCol = startCol;
    if(m_List->endCol < endCol)
        m_List->endCol = endCol;
    if(m_List->startRow > startRow)
        m_List->startRow = startRow;
    if(m_List->endRow < endRow)
        m_List->endRow = endRow;

    if(startCol < m_minCol)
        m_minCol = startCol;
    if(endCol > m_maxCol)
        m_maxCol = endCol;
    if(startRow < m_minRow)
        m_minRow = startRow;
    if(endRow > m_maxRow)
        m_maxRow = endRow;
}
/******************************************
*******************************************/
void CUGDrawHint::ClearHints(){

    //clear the Invalid list
    UGDrwHintList   *current = m_List;
    UGDrwHintList   *next;

    while(current != NULL){
        next = current->next;
        delete current;
        current = next;
    }
    m_List = NULL;

    //clear the valid list
    UGDrwHintVList  *currentV = m_VList;
    UGDrwHintVList  *nextV;

    while(currentV != NULL){
        nextV = currentV->next;
        delete currentV;
        currentV = nextV;
    }
    m_VList = NULL;
}
/******************************************
*******************************************/
int CUGDrawHint::IsInvalid(int col,long row){

    if(m_List == NULL)
        return FALSE;

    if(col < m_minCol || col > m_maxCol)
        return FALSE;
    if(row < m_minRow || row > m_maxRow)
        return FALSE;

    UGDrwHintList   *current = m_List;

    //check the invalid list
    while(current != NULL){

        //check to see if the item in the list covers a greater range
        if(col >= current->startCol && col <= current->endCol){
            if(row >= current->startRow && row <= current->endRow){
                return TRUE;
            }
        }
        current = current->next;
    }

    //if the item is not in the invalid list assume it is valid
    return FALSE;
}

/******************************************
*******************************************/
int CUGDrawHint::IsValid(int col,long row){

    if(m_VList == NULL)
        return FALSE;

    UGDrwHintVList  *currentV = m_VList;
    UGDrwHintVList  *nextV;

    //check the valid list
    while(currentV != NULL){
        nextV = currentV->next;
        if(col == currentV->Col && row == currentV->Row)
            return TRUE;
        currentV = nextV;
    }
    return FALSE;
}

/******************************************
*******************************************/
int CUGDrawHint::GetTotalRange(int *startCol,long *startRow,int *endCol,long *endRow){

    if(m_List == NULL){
        *startCol = 0;  //put in def values just in case the return
        *startRow = 0;  //value is not checked
        *endCol = 0;
        *endRow = 0;
        return FALSE;
    }

    *startCol = m_List->startCol;
    *startRow = m_List->startRow;
    *endCol = m_List->endCol;
    *endRow = m_List->endRow;

    UGDrwHintList   *current = m_List;
    UGDrwHintList   *next;

    while(current != NULL){
        next = current->next;

        //check to see if the item in the list covers a greater range
        if(*startCol > current->startCol)
            *startCol = current->startCol;
        if(*startRow > current->startRow)
            *startRow = current->startRow;
        if(*endCol < current->endCol)
            *endCol = current->endCol;
        if(*endRow < current->endRow)
            *endRow = current->endRow;

        current = next;
    }


    return TRUE;
}

/******************************************
*******************************************/
void CUGDrawHint::SetAsValid(int col,long row){

    UGDrwHintVList  *nextV = m_VList;
    UGDrwHintVList  *newhintV = new UGDrwHintVList;

    if(m_VList != NULL){
        while(nextV->next != NULL){
            nextV = nextV->next;
        }
        nextV->next = newhintV;
    }
    else
        m_VList = newhintV;

    newhintV->next = NULL;
    newhintV->Col = col;
    newhintV->Row = row;
}