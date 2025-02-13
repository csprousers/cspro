#pragma once

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
        -Standard return values for functions are:
            UG_SUCCESS  - success (0)
            UG_NA       - not available (-1)
            1 and up    - error codes
************************************************/

typedef struct UGMemCITag{
    CUGCell *       cell;
    UGMemCITag *    next;
    UGMemCITag *    prev;
}UGMemCI;

typedef struct UGMemRITag{
    UGMemCI *       col;
    UGMemRITag *    next;
    UGMemRITag *    prev;
}UGMemRI;



class CLASS_DECL_ZGRIDO CUGMem: public CUGDataSource{

private:

    long        m_currentRow;   //current row
    UGMemRI *   m_rowInfo;      //current row information pointer
    int         m_currentCol;   //current column
    UGMemCI *   m_colInfo;      //current column information pointer

    int         m_findCol;  //current col for the findnext member
    long        m_findRow;  //current row for the findnext member

    int GotoRow(long row);  //moves the internal memory pointers to the row
    int GotoCol(int col);   //moves the internal memory pointers to the col
    int PrevRow();          //moves the internal memory pointers to the prev row
    int NextRow();          //moves the internal memory pointers to the next row
    int PrevCol();          //moves the internal memory pointers to the prev col
    int NextCol();          //moves the internal memory pointers to the next col

    UGMemCI * GetCol(UGMemRI * ri,int col);

public:

    CUGMem();
    virtual ~CUGMem();

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
                                                    //8-blob 9-graphic 10-ole

    //add-update-clear
    virtual int AppendRow();
    virtual int InsertRow(long row);
    virtual int AppendCol();
    virtual int InsertCol(int col);
    virtual int DeleteRow(long row);
    virtual int DeleteCol(int col);
    virtual int Empty();
    virtual int DeleteCell(int col,long row);

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
    virtual int GetOption(int option,long param1,long param2);

    virtual int GetPrevNonBlankCol(int *col,long row);
};
