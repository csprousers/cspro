/***********************************************
    Ultimate Grid 97
    Copyright 1994 - 1997 Dundas Software Ltd.

    Class
        CUGDataSource
    Purpose
        This is the interface that the grid uses
        to retrieve its data as well as setting
        data.
        This abstract interface allows the grid to
        connect to any type of data.
        Datasources can be practically anything
        i.e.    arrays
                linked lists
                databases
                flat files
                real-time feeds
                sensors
                calculations
    Datails
        -This is a base class which all other datasource
         must be derived from. By defining a standard
         interface to the data, an abstract layer is
         created which allows the uderlying data to
         come from any source, plus allows the datasource
         to be changed without any re-writing of code.
        -the minimum number of functions that need to be
         over-ridden is only ONE, GetCell. GetCell is
         called by the grid when it needs information
         for display. The more functions that are
         used in a derived class makes for better control
         of the data being supplied.
        -new functions can be added to derived datasource,
         but they can never be called by the grid directly.
        -even though the grid generally works on a cell by
         cell basis, many datasource (such as databases)
         tend to work on a row by row basis. To allow data
         to be written to the datasource in this manner
         transaction writing can be used within a datasource
         by over-ridding the transaction functions.
        -If a derived datasource cannot return the number
         of rows that is contains acuratly, then over-ride
         the datasouce OnHitBottom function. This allows the
         grid to ask the datasource for new rows on the fly.
        - stanard return values from a datasource are
            UG_SUCCESS  - success (0)
            UG_NA       - not available (-1)
            1 and up    - error codes
****************************************************/

#include "StdAfx.h"


#include "Ugctrl.h"

#ifdef _DEBUG
#define new DEBUG_NEW
#undef THIS_FILE
static char THIS_FILE[]= __FILE__;
#endif

/***************************************************
Constructor
    Init all data here
****************************************************/
CUGDataSource::CUGDataSource(){
    m_ctrl = NULL;
    m_ID = -1;
}
/***************************************************
Destructor
    Perform clean up here
****************************************************/
CUGDataSource::~CUGDataSource(){
}
/***************************************************
****************************************************/
void CUGDataSource::SetID(long ID){
    m_ID = ID;
}
/***************************************************
****************************************************/
long CUGDataSource::GetID(){
    return m_ID;
}

/***************************************************
********** opening and closing *********************
****************************************************/

/***************************************************
Open
    Override this function to provide file loading to
    a data source
return
    UG_NA       not available
    UG_SUCCESS  success
    1...        error codes (data source dependant)
****************************************************/
int CUGDataSource::Open(LPCTSTR name,LPCTSTR option){
    return UG_NA;
}
/***************************************************
SetPassword
    Override this function if a password is needed
    for the data source
return
    UG_NA       not available
    UG_SUCCESS  success
    1...        error codes (data source dependant)
****************************************************/
int CUGDataSource::SetPassword(LPCTSTR user,LPCTSTR pass){
    return UG_NA;
}
/***************************************************
Close
    Override this function if a close command is needed
    for the data source
return
    UG_NA       not available
    UG_SUCCESS  success
    1...        error codes (data source dependant)
****************************************************/
int CUGDataSource::Close(){
    return UG_NA;
}
/***************************************************
Save
return
    UG_NA       not available
    UG_SUCCESS  success
    1...        error codes (data source dependant)
****************************************************/
int CUGDataSource::Save(){
    return UG_NA;
}
/***************************************************
return
    UG_NA       not available
    UG_SUCCESS  success
    1...        error codes (data source dependant)
****************************************************/
int CUGDataSource::SaveAs(LPCTSTR name,LPCTSTR option){
    return UG_NA;
}
/***************************************************
************ row and col info **********************
****************************************************/

/***************************************************
GetNumRows
    Returns the number of rows of data available
return
    UG_NA       not available
    number of rows
****************************************************/
long CUGDataSource::GetNumRows(){
    return UG_NA;
}
/***************************************************
GetNumCols
    Returns the number of Columns of data avialable
return
    UG_NA       not available
    number of cols
****************************************************/
int CUGDataSource::GetNumCols(){
    return UG_NA;
}
/***************************************************
GetColName
    Returns the name of a column
return
    UG_NA       not available
    UG_SUCCESS  success
    1...        error codes (data source dependant)
****************************************************/
int CUGDataSource::GetColName(int col,CString * string){
    return UG_NA;
}

/***************************************************
GetColFromName
    Returns the column number for a named column
return
    UG_NA       not available
    UG_SUCCESS  success
    1...        error codes (data source dependant)
****************************************************/
int CUGDataSource::GetColFromName(LPCTSTR name,int *col){
    return UG_NA;
}

/***************************************************
GetColType

    Returns the default data type of a column
    type =  0-string 1-bool 2-short 3-long 4-float
            5-double 6-currency 7-data 8-time
            8-blob 9-graphic 10-ole

return
    UG_NA       not available
    UG_SUCCESS  success
    1...        error codes (data source dependant)
****************************************************/
int CUGDataSource::GetColType(int col,int *type){
    return UG_NA;
}

/***************************************************
************* add-update-clear *********************
****************************************************/

/***************************************************
AppendRow
    Appends a new row at the end of the current data
    in the data source
return
    UG_NA       not available
    UG_SUCCESS  success
    1...        error codes (data source dependant)
****************************************************/
int CUGDataSource::AppendRow(){
    return UG_NA;
}
/***************************************************
****************************************************/
int CUGDataSource::AppendRow(CUGCell **cellList,int numCells){
    return UG_NA;
}
/***************************************************
InsertRow
    Inserts a row at the specified position in the
    current data
return
    UG_NA       not available
    UG_SUCCESS  success
    1...        error codes (data source dependant)
****************************************************/
int CUGDataSource::InsertRow(long row){
    return UG_NA;
}
/***************************************************
AppendCol
    Appends a column to the end of the current data
    in the data source
return
    UG_NA       not available
    UG_SUCCESS  success
    1...        error codes (data source dependant)
****************************************************/
int CUGDataSource::AppendCol(){
    return UG_NA;
}
/***************************************************
InsertCol
    Inserts a new column at the specified position
    in the data
return
    UG_NA       not available
    UG_SUCCESS  success
    1...        error codes (data source dependant)
****************************************************/
int CUGDataSource::InsertCol(int col){
    return UG_NA;
}
/***************************************************
DeleteRow
    Deletes the specified row from the data in the
    data source
return
    UG_NA       not available
    UG_SUCCESS  success
    1...        error codes (data source dependant)
****************************************************/
int CUGDataSource::DeleteRow(long row){
    return UG_NA;
}
/***************************************************
DeleteCol
    Deletes the specified column of data from the
    data in the data source
return
    UG_NA       not available
    UG_SUCCESS  success
    1...        error codes (data source dependant)
****************************************************/
int CUGDataSource::DeleteCol(int col){
    return UG_NA;
}
/***************************************************
Empty
    Empties all the data in the data source
return
    UG_NA       not available
    UG_SUCCESS  success
    1...        error codes (data source dependant)
****************************************************/
int CUGDataSource::Empty(){
    return UG_NA;
}
/***************************************************
Delete
    Deletes the specified cell from the data in the
    data source
return
    UG_NA       not available
    UG_SUCCESS  success
    1...        error codes (data source dependant)
****************************************************/
int CUGDataSource::DeleteCell(int col,long row){
    return UG_NA;
}
/***************************************************
****************** cell info ***********************
****************************************************/

/***************************************************
GetCell
    Updates the given cell with 'set' information for
    the specified cell. (only information that is 'set'
    should be updated, since every data property has an
    'unset' state).
return
    UG_NA       not available
    UG_SUCCESS  success
    1...        error codes (data source dependant)
****************************************************/
int CUGDataSource::GetCell(int col,long row,CUGCell *cell){
    return UG_NA;
}
/***************************************************
SetCell
    Sets the cell - including all 'unset' information

return
    UG_NA       not available
    UG_SUCCESS  success
    1...        error codes (data source dependant)
****************************************************/
int CUGDataSource::SetCell(int col,long row,CUGCell *cell){
    return UG_NA;
}

/***************************************************
****************  finding sorting ******************
****************************************************/

/***************************************************
FindFirst
    Returns the first row that contained the specified
    string in the specified column
return
    UG_NA       not available
    UG_SUCCESS  success
    1...        error codes (data source dependant)
****************************************************/
int CUGDataSource::FindFirst(CString *string,int *col,long *row,long flags){
    return UG_NA;
}

/***************************************************
FindFirst
    Returns the next row that contained the specified
    string in the specified column, see above
return
    UG_NA       not available
    UG_SUCCESS  success
    1...        error codes (data source dependant)
****************************************************/
int CUGDataSource::FindNext(CString *string,int *col,long *row,int flags){
    return UG_NA;
}

/***************************************************
SortBy
    Sorts (or indexes) the data by the specified column

return
    UG_NA       not available
    UG_SUCCESS  success
    1...        error codes (data source dependant)
****************************************************/
int CUGDataSource::SortBy(int col,int flags){
    return UG_NA;
}
/***************************************************
SortBy
    Sorts the data starting with the specified column
    then sub-sorts it up to 'num' cols
return
    UG_NA       not available
    UG_SUCCESS  success
    1...        error codes (data source dependant)
****************************************************/
int CUGDataSource::SortBy(int *cols,int num,int flags){
    return UG_NA;
}

/***************************************************
********************* options **********************
****************************************************/

/***************************************************
SetOption
    Datasource dependant function. Used to set data source
    specific information and modes of operation
return
    UG_NA       not available
    UG_SUCCESS  success
    1...        error codes (data source dependant)
****************************************************/
int CUGDataSource::SetOption(int option,long param1,long param2){
    return UG_NA;
}
/****************************************************
****************************************************/
int CUGDataSource::GetPrevNonBlankCol(int *col,long row){
    return UG_NA;
}
/****************************************************
****************************************************/
int CUGDataSource::StartTransaction(){
    return UG_NA;
}
/****************************************************
****************************************************/
int CUGDataSource::CancelTransaction(){
    return UG_NA;
}
/****************************************************
****************************************************/
int CUGDataSource::FinishTransaction(){
    return UG_NA;
}


/***************************************************
GetOption
    Datasource dependant function. Used to get data source
    specific information and modes of operation
return
    UG_NA       not available
    UG_SUCCESS  success
    1...        error codes (data source dependant)
****************************************************/
int CUGDataSource::GetOption(int option,long& param1,long& param2){
    return UG_NA;
}
/***************************************************
****************************************************/
int CUGDataSource::OnHitBottom(long numrows,long rowspast,long *rowsfound){
    return UG_NA;
}
/***************************************************
****************************************************/
int CUGDataSource::OnHitTop(long numrows,long rowspast,long *rowsfound){
    return UG_NA;
}
/***************************************************
****************************************************/
void CUGDataSource::OnRowChange(long oldRow,long newRow){
}
/***************************************************
****************************************************/
void CUGDataSource::OnColChange(int oldCol,int newCol){
}
/***************************************************
****************************************************/
int CUGDataSource::OnCanMove(int oldcol,long oldrow,int newcol,long newrow){
    return TRUE;
}


// Edit notifications...
/***************************************************
****************************************************/
int CUGDataSource::OnEditStart(int col, long row,CWnd **edit){
    return TRUE;
}
/***************************************************
****************************************************/
int CUGDataSource::OnEditVerify(int col,long row,CWnd *edit,UINT *vcKey){
    return TRUE;
}
/***************************************************
****************************************************/
int CUGDataSource::OnEditFinish(int col, long row,CWnd *edit,LPCTSTR string,BOOL cancelFlag){
    return TRUE;
}
/***************************************************
****************************************************/
int CUGDataSource::OnEditContinue(int oldcol,long oldrow,int* newcol,long* newrow){
    return TRUE;
}

