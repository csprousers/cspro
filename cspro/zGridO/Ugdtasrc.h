#pragma once

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
    Details
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
************************************************/

//defines for the IsSupported function
#define UGDS_SUPPORT_INSERTROW      1
#define UGDS_SUPPORT_APPENDROW      2
#define UGDS_SUPPORT_INSERTCOL      4
#define UGDS_SUPPORT_APPENDCOL      8
#define UGDS_SUPPORT_DELETEROW      16
#define UGDS_SUPPORT_DELETECOL      32
#define UGDS_SUPPORT_EMPTY          64
#define UGDS_SUPPORT_CLOSE          128
#define UGDS_SUPPORT_SAVE           256
#define UGDS_SUPPORT_OPEN           512
#define UGDS_SUPPORT_SETPASSWORD    1024
#define UGDS_SUPPORT_FIND           2048
#define UGDS_SUPPORT_SORT           4096
#define UGDS_SUPPORT_GETNUMROWS     8192
#define UGDS_SUPPORT_GETNUMCOLS     16384
#define UGDS_SUPPORT_GETCOLNAME     32768
#define UGDS_SUPPORT_GETCOLTYPE     65536l
#define UGDS_SUPPORT_SETCELL        131072l
#define UGDS_SUPPORT_GETCELL        262144l
#define UGDS_SUPPORT_TRANSACTIONS   524288l
#define UGDS_SUPPORT_HITBOTTON      1048576l
#define UGDS_SUPPORT_HITTOP         2097152l
#define UGDS_SUPPORT_GETPREVCOL     4194304l



class CLASS_DECL_ZGRIDO CUGDataSource{

protected:

    friend CUGCtrl;
    CUGCtrl * m_ctrl;  //pointer to the main class

    long m_ID;

public:

    CUGDataSource();
    virtual ~CUGDataSource();

    void SetID(long ID);
    long GetID();

    //opening and closing
    virtual int Open(LPCTSTR name,LPCTSTR option);
    virtual int SetPassword(LPCTSTR user,LPCTSTR pass);

    virtual int Close();
    virtual int Save();
    virtual int SaveAs(LPCTSTR name,LPCTSTR option);


    //row and col info
    virtual long GetNumRows();

    virtual int GetNumCols();
    virtual int GetColName(int col,CString * string);
    virtual int GetColType(int col,int *type);  //0-string 1-bool 2-short 3-long 4-float
                                                    //5-double 6-currency 7-data 8-time
                                                    //8-memo 9-blob 10-ole
    virtual int GetColFromName(LPCTSTR name,int *col);

    //add-update-clear
    virtual int AppendRow();
    virtual int AppendRow(CUGCell **cellList,int numCells);
    virtual int InsertRow(long row);
    virtual int AppendCol();
    virtual int InsertCol(int col);
    virtual int DeleteRow(long row);
    virtual int DeleteCol(int col);
    virtual int DeleteCell(int col,long row);
    virtual int Empty();

    //cell info
    virtual int GetCell(int col,long row,CUGCell *cell);
    virtual int SetCell(int col,long row,CUGCell *cell);

    //finding sorting
    virtual int FindFirst(CString *string,int *col,long *row,long flags);
    virtual int FindNext(CString *string,int *col,long *row,int flags);

    virtual int SortBy(int col,int flags);
    virtual int SortBy(int *cols,int num,int flags);

    //options
    virtual int SetOption(int option,long param1,long param2);
    virtual int GetOption(int option,long& param1,long& param2);

    //movement
    virtual int OnHitTop(long numrows,long rowspast,long *rowsfound);
    virtual int OnHitBottom(long numrows,long rowspast,long *rowsfound);

    //grid movement
    virtual void OnRowChange(long oldRow,long newRow);
    virtual void OnColChange(int oldCol,int newCol);
    virtual int OnCanMove(int oldcol,long oldrow,int newcol,long newrow);

    //used to check to see if the data source supports a standard function
    BOOL IsFunctionSupported(long type);

    //transactions
    virtual int StartTransaction();
    virtual int CancelTransaction();
    virtual int FinishTransaction();

    // editing notifications...
    virtual int OnEditStart(int col, long row,CWnd **edit);
    virtual int OnEditVerify(int col,long row,CWnd *edit,UINT *vcKey);
    virtual int OnEditFinish(int col, long row,CWnd *edit,LPCTSTR string,BOOL cancelFlag);
    virtual int OnEditContinue(int oldcol,long oldrow,int* newcol,long* newrow);


    //other functions
    virtual int GetPrevNonBlankCol(int *col,long row);
};
