/***********************************************
    Ultimate Grid 97
    Copyright 1994 - 1997 Dundas Software Ltd.

    Class
        CUGMem
    Purpose
        This is a derived grid datasource, that
        allows information to be pre-loaded into
        the grid. By default an Instance of this
        datasource is created for each sheet in
        the grid. Since this is the default data
        source for a grid, it makes it easy to
        create and pre-load a grid control with
        data.
    Details
        -This class only stores cells that have
         information set in them, which reduces
         the amount of memory required for large
         grids.
        -This class works as a two dimensional
         linked list, with its optimizations being
         row oriented.
        -Support for sorting, finding, inserting
         and deleting are fully supported.
        -cells are stored in native CUGCell format
         internally.
        -since this datasource is automatically created
         for each grid, it is always given an index
         number of 0.
        -Standard Return Values: UG_SUCCESS - success (0)
                          UG_NA      - not available (-1)
                          1 and up   - error codes
****************************************************/

#include "StdAfx.h"


#include "Ugctrl.h"
//#include "UGMemMan.h"

#ifdef _DEBUG
#define new DEBUG_NEW
#undef THIS_FILE
static char THIS_FILE[]= __FILE__;
#endif

/***************************************************
****************************************************/
CUGMem::CUGMem(){

    //set up the variables
    m_currentRow    = 0;
    m_rowInfo       = NULL;
    m_currentCol    = 0;
    m_colInfo       = NULL;
    m_findRow       = 0;
}
/***************************************************
****************************************************/
CUGMem::~CUGMem(){
    //clear the variables
    Empty();
}


/***************************************************
********** opening and closing *********************
****************************************************/


/***************************************************
****************************************************/
int CUGMem::Open(LPCTSTR name,LPCTSTR option){
    return UG_NA;
}
/***************************************************
****************************************************/
int CUGMem::SetPassword(LPCTSTR user,LPCTSTR pass){
    return UG_NA;
}
/***************************************************
****************************************************/
int CUGMem::Close(){
    return UG_NA;
}
/***************************************************
****************************************************/
int CUGMem::Save(){
    return UG_NA;
}
/***************************************************
****************************************************/
int CUGMem::SaveAs(LPCTSTR name,LPCTSTR option){
    return UG_NA;
}


/***************************************************
************ row and col info **********************
****************************************************/

/***************************************************
GotoRow
    Moves the internal pointers to the specified
    row
  return
    UG_SUCCESS  success
    1           fail
****************************************************/
int CUGMem::GotoRow(long row){

    if(m_rowInfo == NULL)
        return 1;

    //find the correct row
    if(row != m_currentRow){
        //if the row is greater than the current, then move forward
        while(row > m_currentRow){
            if(m_rowInfo->next != NULL){
                m_rowInfo = m_rowInfo->next;
                m_currentRow++;
            }
            else{
                //update the current column pointer
                m_colInfo       = m_rowInfo->col;
                m_currentCol    = 0;
                return 1;
            }
        }
        //if the row is less than the current, then move back
        while(row < m_currentRow){
            if(m_rowInfo->prev != NULL){
                m_rowInfo = m_rowInfo->prev;
                m_currentRow--;
            }
            else{
                //update the current column pointer
                m_colInfo       = m_rowInfo->col;
                m_currentCol    = 0;
                return 1;
            }
        }

        //update the current column pointer
        m_colInfo       = m_rowInfo->col;
        m_currentCol    = 0;
    }

    return UG_SUCCESS;
}
/***************************************************
GotoCol
    Moves the internal pointers to the specified
    column
  return
    UG_SUCCESS  success
    1           fail
****************************************************/
int CUGMem::GotoCol(int col){

    if(m_colInfo==NULL)
        return 1;

    //find the correct column
    if(col != m_currentCol){

        while(col > m_currentCol){
            if(m_colInfo->next != NULL){
                m_colInfo = m_colInfo->next;
                m_currentCol++;
            }
            else
                return 1;
        }

        while(col < m_currentCol){
            if(m_colInfo->prev != NULL){
                m_colInfo = m_colInfo->prev;
                m_currentCol--;
            }
            else
                return 1;
        }
    }
    if(m_colInfo != NULL)
        return UG_SUCCESS;
    return 1;
}
/***************************************************
****************************************************/
int CUGMem::PrevRow(){

    if(m_rowInfo == NULL)
        return 1;

    if(m_rowInfo->prev != NULL){

        //update the row pointer
        m_rowInfo = m_rowInfo->prev;
        m_currentRow--;
        //update the current column pointer
        m_colInfo       = m_rowInfo->col;
        m_currentCol    = 0;

        return UG_SUCCESS;
    }
    return 1;
}
/***************************************************
****************************************************/
int CUGMem::NextRow(){

    if(m_rowInfo == NULL)
        return 1;

    if(m_rowInfo->next != NULL){

        //update the row pointer
        m_rowInfo = m_rowInfo->next;
        m_currentRow++;
        //update the current column pointer
        m_colInfo       = m_rowInfo->col;
        m_currentCol    = 0;

        return UG_SUCCESS;
    }
    return 1;
}
/***************************************************
****************************************************/
int CUGMem::PrevCol(){
    if(m_colInfo==NULL)
        return 1;
    if(m_colInfo->prev != NULL){
        //update the current column pointer
        m_colInfo       = m_colInfo->prev;
        m_currentCol--;

        return UG_SUCCESS;
    }
    return 1;
}
/***************************************************
****************************************************/
int CUGMem::NextCol(){
    if(m_colInfo==NULL)
        return 1;
    if(m_colInfo->next != NULL){
        //update the current column pointer
        m_colInfo       = m_colInfo->next;
        m_currentCol--;

        return UG_SUCCESS;
    }
    return 1;
}
/***************************************************
****************************************************/
long CUGMem::GetNumRows(){
    //find the highest row number
    return UG_NA;
}
/***************************************************
****************************************************/
int CUGMem::GetNumCols(){
    //find the highest row number
    return UG_NA;
}
/***************************************************
****************************************************/
int CUGMem::GetColName(int col,CString * string){
    return UG_NA;
}
/***************************************************
    //0-string 1-bool 2-short 3-long 4-float
    //5-double 6-currency 7-data 8-time
    //8-blob 9-graphic 10-ole
****************************************************/
int CUGMem::GetColType(int col,int *type){
    return UG_NA;
}


/***************************************************
************* add-update-clear *********************
****************************************************/


/***************************************************
AppendRow
    This routine does not need to do anything, since
    cells are automaticly appended during the SetCell
    routine if it does not exist
****************************************************/
int CUGMem::AppendRow(){

    return UG_SUCCESS;
}
/***************************************************
****************************************************/
int CUGMem::InsertRow(long row){

    //adjust the linked list if the list has an item at the insertion point
    if(GotoRow(row)==UG_SUCCESS){
        //create a new row information item for the list
        //then insert it
        UGMemRI *newrow;
        newrow          = new UGMemRI;
        newrow->col     = NULL;
        newrow->next    = m_rowInfo;
        newrow->prev    = m_rowInfo->prev;

        if(newrow->prev != NULL)
            newrow->prev->next = newrow;

        m_rowInfo->prev = newrow;
        m_currentRow++;
    }

    return UG_SUCCESS;

}
/***************************************************
AppendCol
    This routine does not need to do anything, since
    cells are automaticly appended during the SetCell
    routine if it does not exist
****************************************************/
int CUGMem::AppendCol(){

    return UG_SUCCESS;
}
/***************************************************
****************************************************/
int CUGMem::InsertCol(int col){

    //goto the first row
    while(PrevRow()==UG_SUCCESS);

    //check to see if there is a first row
    if(m_rowInfo == NULL)
        return 1;

    //get the screen position column number
    int numCols = m_ctrl->GetNumberCols();
    if(numCols > 0){
        for(int loop = 0 ; loop < numCols;loop++){
            if(m_ctrl->m_GI->m_colInfo[loop].colTranslation == col){
                col = loop;
                break;
            }
        }
    }

    //move the 'col' column over
    UGMemCI *newcol;
    do{
        if(GotoCol(col) ==UG_SUCCESS){
            newcol          = new UGMemCI;
            newcol->cell    = NULL;
            newcol->next    = m_colInfo;
            newcol->prev    = m_colInfo->prev;

            if(newcol->prev != NULL)
                newcol->prev->next = newcol;

            if(m_rowInfo->col == m_colInfo)
                m_rowInfo->col = newcol;

            m_colInfo->prev = newcol;
            m_currentCol ++;

        }
    }while(NextRow()==UG_SUCCESS);

    //adjust the column translations
    if(numCols > 0){
        for(int loop = 0 ; loop < numCols;loop++){
            if(m_ctrl->m_GI->m_colInfo[loop].colTranslation >= col)
                m_ctrl->m_GI->m_colInfo[loop].colTranslation++;
        }
    }

    return UG_SUCCESS;
}
/***************************************************
****************************************************/
int CUGMem::DeleteRow(long row){

    //adjust the linked list if the list has an item at the insertion point
    if(GotoRow(row)==UG_SUCCESS){

        //delete the columns for this row
        UGMemCI *next;
        while(PrevCol()==UG_SUCCESS);  //find the first col
        while(m_colInfo != NULL){      //delete the cols to the end
            next = m_colInfo->next;

            //delete the colinfo
            if(m_colInfo->cell != NULL)
                delete m_colInfo->cell;
            delete m_colInfo;

            m_colInfo = next;
        }

        //update the row links
        UGMemRI *currentrow;
        if(m_rowInfo->prev != NULL)
            m_rowInfo->prev->next = m_rowInfo->next;
        if(m_rowInfo->next != NULL){
            m_rowInfo->next->prev = m_rowInfo->prev;
            currentrow = m_rowInfo->next;
        }
        else if(m_rowInfo->prev != NULL){
            currentrow = m_rowInfo->prev;
            m_currentRow--;
        }
        else{
            currentrow = NULL;
            m_currentRow = 0;
        }
        delete m_rowInfo;
        m_rowInfo = currentrow;

        //update the current column pointer
        if(m_rowInfo != NULL)
            m_colInfo   = m_rowInfo->col;
        else
            m_colInfo   = NULL;
        m_currentCol    = 0;

        return UG_SUCCESS;
    }

    //return success even if there are no rows to delete
    //since the memory manager is autoupdating
    return UG_SUCCESS;
}
/***************************************************
****************************************************/
int CUGMem::DeleteCol(int col){

    //goto the first row
    while(PrevRow()==UG_SUCCESS);

    //check to see if there is a first row
    if(m_rowInfo == NULL)
        return 1;

    //move the 'col' column over
    UGMemCI *colinfo;
    do{
        if(GotoCol(col) ==UG_SUCCESS){

            // delete the columninfo if there are columns to the right
            if(m_colInfo->next != NULL){
                //update the links
                colinfo = m_colInfo->prev;
                if(colinfo != NULL)
                    colinfo->next = m_colInfo->next;

                colinfo = m_colInfo->next;
                if(colinfo != NULL)
                    colinfo->prev = m_colInfo->prev;

                if(m_rowInfo->col == m_colInfo)
                    m_rowInfo->col = colinfo;

                //delete the colinfo
                if(m_colInfo->cell != NULL)
                    delete m_colInfo->cell;
                delete m_colInfo;
            }
            // else if there are no more column to the right, then keep the
            // colinfo, but delete the cellinfo within the colinfo
            // since this may be the initial link to the rowinfo list
            else{
                delete m_colInfo->cell;
                m_colInfo->cell = NULL;
            }
        }
    }while(NextRow()==UG_SUCCESS);

    //adjust the column translations
    int numCols = m_ctrl->GetNumberCols();
    if(numCols > 0){
        for(int loop = 0 ; loop < numCols;loop++){
            if(m_ctrl->m_GI->m_colInfo[loop].colTranslation >= col)
                m_ctrl->m_GI->m_colInfo[loop].colTranslation--;
        }
    }

    //update the current column pointer
    m_colInfo       = m_rowInfo->col;
    m_currentCol    = 0;

    return UG_SUCCESS;
}
/***************************************************
****************************************************/
int CUGMem::DeleteCell(int col,long row){

    if(GotoRow(row)==UG_SUCCESS){
        if(GotoCol(col)==UG_SUCCESS){

            //delete the colinfo cell object
            if(m_colInfo->cell != NULL){
                delete m_colInfo->cell;
                m_colInfo->cell = NULL;
            }

            m_colInfo       = m_rowInfo->col;
            m_currentCol    = 0;

            return UG_SUCCESS;
        }
    }
    return 1;
}
/***************************************************
****************************************************/
int CUGMem::Empty(){

    UGMemRI *   row = m_rowInfo;
    UGMemCI *   col;
    UGMemRI *   nextrow;
    UGMemCI *   nextcol;

    //release the memory
    //find the first row
    while(row != NULL){
        if(row->prev == NULL)
            break;
        row = row->prev;
    }

    while(row != NULL){

        //find the first col
        col = row->col;
        while(col != NULL){
            if(col->prev ==NULL)
                break;
            col = col->prev;
        }

        //delete any columns that are attached to the row
        while(col != NULL){
            nextcol = col->next;
            delete col->cell;
            delete col;
            col = nextcol;
        }

        //delete the row information
        nextrow = row->next;
        delete row;
        row = nextrow;
    }

    //reset the variables
    m_currentRow    = 0;
    m_rowInfo       = NULL;
    m_currentCol    = 0;
    m_colInfo       = NULL;

    return UG_NA;
}


/***************************************************
****************** cell info ***********************
****************************************************/


/***************************************************
return
    UG_SUCCESS  (0)     success
                (1)     cell not found
****************************************************/
int CUGMem::GetCell(int col,long row,CUGCell *cell){



    //check to see if there are no cells at all
    if(m_rowInfo == NULL)
        return 1;


    //find the correct row
    if(row != m_currentRow){
        while(row > m_currentRow){
            if(m_rowInfo->next != NULL){
                m_rowInfo = m_rowInfo->next;
                m_currentRow++;
            }
            else{
                m_colInfo       = m_rowInfo->col;
                m_currentCol    = 0;
                return 1;
            }
        }
        while(row < m_currentRow){
            if(m_rowInfo->prev != NULL){
                m_rowInfo = m_rowInfo->prev;
                m_currentRow--;
            }
            else{
                m_colInfo       = m_rowInfo->col;
                m_currentCol    = 0;
                return 1;
            }
        }
        m_colInfo = m_rowInfo->col;
        m_currentCol = 0;
    }

    //check to see if a col info exists at all for this row
    if(m_colInfo == NULL)
        return 1;

    //find the correct column
    if(col != m_currentCol){
        while(col > m_currentCol){
            if(m_colInfo->next != NULL){
                m_colInfo = m_colInfo->next;
                m_currentCol++;
            }
            else
                return 1;
        }
        while(col < m_currentCol){
            if(m_colInfo->prev != NULL){
                m_colInfo = m_colInfo->prev;
                m_currentCol--;
            }
            else
                return 1;
        }
    }


    //if the cell does not exist then create it
    if(m_colInfo->cell == NULL){
        return 1;
    }

    // ***** copy the information over *****
    cell->AddInfoFrom(m_colInfo->cell);

    return UG_SUCCESS;
}
/***************************************************
****************************************************/
int CUGMem::SetCell(int col,long row,CUGCell *cell){

    UGMemRI *newrow;
    UGMemCI *newcol;

    //check to see if the memory manager is empty
    if(m_rowInfo == NULL){
        m_rowInfo       = new UGMemRI;
        m_rowInfo->col  = NULL;
        m_rowInfo->next = NULL;
        m_rowInfo->prev = NULL;
        m_currentRow =0;
    }

    //find the correct row
    if(row != m_currentRow){
        while(row > m_currentRow){
            //check to see if the row exists
            if(m_rowInfo->next != NULL){
                m_rowInfo = m_rowInfo->next;
                m_currentRow++;
            }
            //if the row does not then create it
            else{
                newrow          = new UGMemRI;
                newrow->col     = NULL;
                newrow->next    = NULL;
                newrow->prev    = m_rowInfo;
                m_rowInfo->next = newrow;

                m_rowInfo = m_rowInfo->next;
                m_currentRow++;
            }
        }
        while(row < m_currentRow){
            //check to see if the row exists
            if(m_rowInfo->prev != NULL){
                m_rowInfo = m_rowInfo->prev;
                m_currentRow--;
            }
            //if the row does not then create it
            else{
                newrow          = new UGMemRI;
                newrow->col     = NULL;
                newrow->next    = m_rowInfo;
                newrow->prev    = NULL;
                m_rowInfo->prev = newrow;

                m_rowInfo = m_rowInfo->prev;
                m_currentRow--;
            }
        }
        m_colInfo = m_rowInfo->col;
        m_currentCol = 0;
    }

    //check to see if a colinfo exists for this row
    if(m_colInfo == NULL){
        m_colInfo       = new UGMemCI;
        m_colInfo->cell = NULL;
        m_colInfo->next = NULL;
        m_colInfo->prev = NULL;
        m_currentCol    = 0;
        m_rowInfo->col  = m_colInfo;
    }

    //find the correct column
    if(col != m_currentCol){
        while(col > m_currentCol){
            //check to see if the col exists
            if(m_colInfo->next != NULL){
                m_colInfo = m_colInfo->next;
                m_currentCol++;
            }
            //if the col does not then create it
            else{
                newcol          = new UGMemCI;
                newcol->cell    = NULL;
                newcol->next    = NULL;
                newcol->prev    = m_colInfo;
                m_colInfo->next = newcol;

                m_colInfo = m_colInfo->next;
                m_currentCol ++;
            }
        }
        while(col < m_currentCol){
            //check to see if the col exists
            if(m_colInfo->prev != NULL){
                m_colInfo = m_colInfo->prev;
                m_currentCol--;
            }
            //if the col does not then create it
            else{
                newcol          = new UGMemCI;
                newcol->cell    = NULL;
                newcol->next    = m_colInfo;
                newcol->prev    = NULL;
                m_colInfo->prev = newcol;

                m_colInfo = m_colInfo->prev;
                m_currentCol --;
            }
        }
    }

    //check to see if a cell info structure exists
    if(m_colInfo->cell == NULL)
        m_colInfo->cell = new CUGCell;

    // ***** copy the information over *****
    cell->CopyInfoTo(m_colInfo->cell);

    return UG_SUCCESS;
}


/***************************************************
****************  finding sorting ******************
****************************************************/


/***************************************************
****************************************************/
int CUGMem::FindFirst(CString *string,int *col,long *row,long flags){

    if(flags&UG_FIND_ALLCOLUMNS){
        *col = -1;
        *row = 0;
    }
    else{
        *row = -1;
    }

    return FindNext(string,col,row,flags);

}
/***************************************************
****************************************************/
int CUGMem::FindNext(CString *string,int *col,long *row,int flags){

    BOOL success = FALSE;
    CString tempStr;

    //goto the first col/row
    m_findRow = *row;
    m_findCol = *col;

    long lastRow = m_ctrl->GetNumberRows()-1;
    int  lastCol = m_ctrl->GetNumberCols()-1;
    long searchLimit = lastRow+2;       // search with wrap
    long rowCount = 0;
    BOOL colsDone = FALSE;
                // note: there could conceivably be a mismatch
                // here.  Not all columns of the grid are necessarily
                // bound to this datasource.  On the other hand, this
                // does limit us to searching that subset of the
                // the ds shown on the grid.  (assuming col translation
                // has not been applied to intevening rows...)


    if(flags&UG_FIND_CASEINSENSITIVE){
        string->MakeUpper();    // do this once
    }

    // initial positioning
    if(flags&UG_FIND_ALLCOLUMNS){
        if((flags&UG_FIND_UP)) {
            m_findCol--;
            if(m_findCol < 0) {
                m_findCol = lastCol;
                m_findRow--;
                if(m_findRow < 0)
                    m_findRow = lastRow;
            }
        }
        else {
            m_findCol++;
            if(m_findCol > lastCol) {
                m_findCol = 0;
                m_findRow++;
                if(m_findRow > lastRow)
                    m_findRow = 0;
            }
        }
    }
    else{
        if(!(flags&UG_FIND_UP)) {
            m_findRow++;
            if(m_findRow > lastRow)
                m_findRow = 0;
        }
        else {
            m_findRow--;
            if(m_findRow < 0)
                m_findRow = lastRow;
        }
    }
    // end initial positioning

    // loop through all rows plus wrap one row
    while(1) {
        if(rowCount >= searchLimit)
            return 1;

        if(GotoRow(m_findRow) != UG_SUCCESS) {
            // goto next row
            if(!(flags&UG_FIND_UP)) {
                m_findRow++;
                if(m_findRow > lastRow)
                    m_findRow = 0;
            }
            else {
                m_findRow--;
                if(m_findRow < 0)
                    m_findRow = lastRow;
            }
            // increment count
            rowCount++;
            continue;
        }

        // ok - if we get here, the row is valid.  Now, find a valid col
        colsDone = FALSE;
        while(!colsDone) {
            if(GotoCol(m_findCol) == UG_SUCCESS) {
                if(NULL != m_colInfo->cell) {
                    // check for match
                    if(flags&UG_FIND_CASEINSENSITIVE){
                        if(flags&UG_FIND_PARTIAL){
                            tempStr = m_colInfo->cell->GetText();
                            tempStr.MakeUpper();
                            if( tempStr.Find(*string) >= 0 )
                                success = TRUE;
                        }
                        else
                            if( string->CompareNoCase(m_colInfo->cell->GetText()) == 0 )
                                success = TRUE;
                    }
                    else{
                        if(flags&UG_FIND_PARTIAL){
                            tempStr = m_colInfo->cell->GetText();
                            if( tempStr.Find(*string) >= 0 )
                                success = TRUE;
                        }
                        else
                            if( string->Compare(m_colInfo->cell->GetText()) == 0 )
                                success = TRUE;
                    }

                    if(success){
                        *col = m_findCol;
                        *row = m_findRow;
                        return UG_SUCCESS;
                    }
                }
            }
            // move to next col and/or row
            if(flags&UG_FIND_ALLCOLUMNS) {
                if((flags&UG_FIND_UP)) {
                    m_findCol--;
                    if(m_findCol < 0) {
                        colsDone = TRUE;
                        m_findCol = lastCol;
                        m_findRow--;
                        rowCount++;
                        if(m_findRow < 0)
                            m_findRow = lastRow;
                    }
                }
                else {
                    m_findCol++;
                    if(m_findCol > lastCol) {
                        colsDone = TRUE;
                        m_findCol = 0;
                        m_findRow++;
                        rowCount++;
                        if(m_findRow > lastRow)
                            m_findRow = 0;
                    }
                }
            }
            else {
                colsDone = TRUE;
                if(flags&UG_FIND_UP) {
                    m_findRow--;
                    rowCount++;
                    if(m_findRow < 0)
                        m_findRow = lastRow;
                }
                else {
                    m_findRow++;
                    rowCount++;
                    if(m_findRow > lastRow)
                        m_findRow = 0;
                }

            }

        }   // end while(!colsDone)

    }       // end while(1)

    // failed to find match...
    return 1;
}
/***************************************************
flags - UG_SORT_ASSENDING or UG_SORT_DESCENDING
****************************************************/
int CUGMem::SortBy(int col,int flags){

    return SortBy(&col,1,flags);
}
/***************************************************
flags - UG_SORT_ASSENDING or UG_SORT_DESCENDING
****************************************************/
/*
int CUGMem::SortBy(int *cols,int num,int flags){

    if(m_rowInfo == NULL)
        return 1;

    UGMemRI *OldListRowInfo;
    UGMemRI *OldListNextRowInfo;
    UGMemRI *NewListRowInfo;
    UGMemRI *NewListStartRowInfo;
    int     index;
    int     rt;
    CUGCell cell1,cell2;
    BOOL    insert = TRUE;

    //goto the start cell
    GotoRow(0);
    NewListRowInfo      = m_rowInfo;
    NewListStartRowInfo = m_rowInfo;

    //get the next row
    OldListRowInfo = m_rowInfo->next;

    //make the new list only one item long
    m_rowInfo->next = NULL;

    //main row sort loop
    while(OldListRowInfo != NULL){

        //save the next row pointer
        OldListNextRowInfo = OldListRowInfo->next;
        //start sorting using the first index in the column array
        index = 0;

        while(1){
            //get a cell from the old list
            m_colInfo = OldListRowInfo->col;
            m_currentCol = 0;
            if(GotoCol(cols[index])==UG_SUCCESS)
                cell1.CopyInfoFrom(m_colInfo->cell);
            else
                cell1.ClearAll();

            //find the cells position in the new sorted list
            if(insert == TRUE){
                insert = FALSE;
                index == 0;
                NewListRowInfo = NewListStartRowInfo;
            }
            while(NewListRowInfo != NULL){

                //get a cell in the new list
                m_colInfo = NewListRowInfo->col;
                m_currentCol = 0;
                if(GotoCol(cols[index])==UG_SUCCESS)
                    cell2.CopyInfoFrom(m_colInfo->cell);
                else
                    cell2.ClearAll();

                //call the evaluation function
                if(flags&UG_SORT_DESCENDING){
                    rt = m_ctrl->OnSortEvaluate(&cell1,&cell2,flags);
                    if(rt < 0){
                        insert = TRUE;
                        OldListRowInfo->prev = NewListRowInfo->prev;
                        NewListRowInfo->prev = OldListRowInfo;
                        if(OldListRowInfo->prev !=NULL)
                            OldListRowInfo->prev->next = OldListRowInfo;
                        OldListRowInfo->next = NewListRowInfo;
                        if(NewListRowInfo == NewListStartRowInfo)
                            NewListStartRowInfo = OldListRowInfo;
                        break;
                    }
                }
                else{
                    rt = m_ctrl->OnSortEvaluate(&cell1,&cell2,flags);
                    if(rt < 0){
                        insert = TRUE;
                        OldListRowInfo->prev = NewListRowInfo->prev;
                        NewListRowInfo->prev = OldListRowInfo;
                        if(OldListRowInfo->prev !=NULL)
                            OldListRowInfo->prev->next = OldListRowInfo;
                        OldListRowInfo->next = NewListRowInfo;
                        if(NewListRowInfo == NewListStartRowInfo)
                            NewListStartRowInfo = OldListRowInfo;
                        break;
                    }
                }

//              if(index >= (num-1) && rt > 0){
//                  insert = TRUE;
//                  OldListRowInfo->prev = NewListRowInfo;
//                  OldListRowInfo->next = NewListRowInfo->next;
//                  NewListRowInfo->next = OldListRowInfo;
//                  if(OldListRowInfo->next != NULL)
//                      OldListRowInfo->next->prev = OldListRowInfo;
//                  break;
//              }

                if(rt == 0 && index < (num-1)){
                        index ++;
                        break;
                }
                if(NewListRowInfo->next == NULL){
                    insert = TRUE;
                    OldListRowInfo->prev = NewListRowInfo;
                    NewListRowInfo->next = OldListRowInfo;
                    OldListRowInfo->next = NULL;
                    break;
                }
                NewListRowInfo = NewListRowInfo->next;

                if(rt == 0 && index == (num-1)){
                    index = 0;
                    break;
                }
            }
            if(insert)
                break;
        }

        OldListRowInfo = OldListNextRowInfo;
    }

    //reset the linked list poisitioning pointers
    m_currentRow = 0;
    m_currentCol = 0;
    m_rowInfo = NewListStartRowInfo;
    m_colInfo = NewListStartRowInfo->col;

    return UG_SUCCESS;
}
*/

/***************************************************
flags - UG_SORT_ASSENDING or UG_SORT_DESCENDING
****************************************************/
int CUGMem::SortBy(int *cols,int num,int flags){

    UGMemRI*    row1;
    UGMemRI*    row2;
    UGMemCI*    temp;
    UGMemCI*    col1;
    UGMemCI*    col2;
    UGMemRI*    currentPos;
    BOOL        rowSorted = FALSE;
    int         rt;
    CUGCell*    cell1;
    CUGCell*    cell2;

    // if empty then do not sort
    if(m_rowInfo == NULL)
        return UG_SUCCESS;
    if(m_rowInfo->next == NULL)
        return UG_SUCCESS;

    //save the start of the linked list
    GotoRow(0);
    currentPos = m_rowInfo;

    //loop until no more rows are sorted
    while(1){
        row1 = currentPos;
        row2 = currentPos->next;

        //check each column in the sort
        for(int loop = 0;loop < num;loop++){

            col1 = GetCol(row1,cols[loop]);
            col2 = GetCol(row2,cols[loop]);

            if(col1 != NULL)
                cell1 = col1->cell;
            else
                cell1 = NULL;
            if(col2 != NULL)
                cell2 = col2->cell;
            else
                cell2 = NULL;

            rt = m_ctrl->OnSortEvaluate(cell1,cell2,flags);

            //swap the row order
            if(rt > 0){

                temp = row1->col;
                row1->col = row2->col;
                row2->col = temp;

                rowSorted = TRUE;

                break;
            }
            if(rt < 0){
                break;
            }
        }

        currentPos = currentPos->next;

        if(currentPos == NULL){
            currentPos = m_rowInfo;
            if(rowSorted == FALSE)
                break;
            rowSorted = FALSE;
        }
        else if(currentPos->next == NULL){
            currentPos = m_rowInfo;
            if(rowSorted == FALSE)
                break;
            rowSorted = FALSE;
        }
    }

    //set the column associated with row 0
    m_colInfo = m_rowInfo->col;
    m_currentCol = 0;

    return UG_SUCCESS;
}

/***************************************************
****************************************************/
int CUGMem::GetPrevNonBlankCol(int *col,long row){

    //move to the row if not already there
    if(m_currentRow != row){
        if(GotoRow(row) != UG_SUCCESS){ //no cells on the specified row
            return 1;
        }
    }

    //move to the col if not already there
    if(m_currentCol != *col){
        //if the specified col was not found then 1 is returned by
        //GotoCol, but it will find the closest col
        if(GotoCol(*col) != UG_SUCCESS){
            //if the first cell found is blank then there is no prev
            if(m_colInfo == NULL){
                return 1;
            }
            if(m_colInfo->cell == NULL){
                return 1;
            }
        }
    }

    //if there is no col information for the row then there is no prev col
    if(m_colInfo == NULL)
        return 1;

    // if the specified cell (or a prev cell in the same row was found
    // then find the prev non-blank cell
    UGMemCI * tempCI    = m_colInfo;
    int tempCol         = m_currentCol;

    //main prev search loop
    while(tempCol >= 0){

        if(tempCI->cell != NULL && *col != tempCol){
            if(tempCI->cell->GetTextLength() > 0){
                *col = tempCol;
                return UG_SUCCESS;
            }
        }
        tempCI = tempCI->prev;
        tempCol --;
    }

    return 1;
}

/***************************************************
********************* options **********************
****************************************************/


/***************************************************
****************************************************/
int CUGMem::SetOption(int option,long param1,long param2){
    return UG_NA;
}
/***************************************************
****************************************************/
int CUGMem::GetOption(int option,long param1,long param2){
    return UG_NA;
}
/***************************************************
GotoCol
    Moves the internal pointers to the specified
    column
  return
    UG_SUCCESS  success
    1           fail
****************************************************/
UGMemCI * CUGMem::GetCol(UGMemRI * ri,int col){

    if(ri == NULL)
        return NULL;

    int count       = 0;
    BOOL isNULL     = FALSE;
    UGMemCI * ci    = ri->col;

    if(ci == NULL)
        return NULL;

    while(count < col){
        if(ci->next != NULL){
            ci = ci->next;
            count++;
        }
        else{
            isNULL = TRUE;
            break;
        }
    }
    while(count > col){
        if(ci->prev != NULL){
            ci = ci->prev;
            count++;
        }
        else{
            isNULL = TRUE;
            break;
        }
    }

    if(isNULL)
        return NULL;

    return ci;
}
