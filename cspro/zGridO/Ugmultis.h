#pragma once

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


typedef struct UG_MSListTag{
    int     startCol,endCol;
    long    startRow,endRow;
    int     loCol,hiCol;
    long    loRow,hiRow;
    int     selected;
    UG_MSListTag * next;
}UG_MSList;


#define UG_MULTISELECT_OFF  0
#define UG_MULTISELECT_CELL 1
#define UG_MULTISELECT_ROW  2
#define UG_MULTISELECT_HDGS 4
#define UG_MULTISELECT_DONTMOVE 8
#define UG_MULTISELECT_NODESELECT   16


class CLASS_DECL_ZGRIDO CUGMultiSelect{

protected:

    UG_MSList * m_list;     //list of selected regions

    int         m_startCol; //start region co-ords
    long        m_startRow;
    int         m_endCol;   //end region co-ords
    long        m_endRow;
    int         m_blockInProgress;
    UG_MSList * m_currentItem;

    int         m_mode; //selection mode  - bits
                        //0: off  1:cell  2:row
                        //4: indent headings
                        //8:  don't move current cell
                        //16: no deselect


    int         m_numberBlocks;
    BOOL        m_blockJustStarted;

    //enum info
    int         m_enumStartCol;
    long        m_enumStartRow;
    int         m_enumEndCol;
    long        m_enumEndRow;
    int         m_enumCol;
    long        m_enumRow;
    int         m_enumInProgress;

    int         m_enumBlockNumber;

    friend CUGCtrl;
    CUGCtrl * m_ctrl;
    CUGGridInfo * m_GI;

    int         m_lastCol;
    long        m_lastRow;

    int         m_origCol;
    int         m_origRow;

public:


    CUGMultiSelect();
    ~CUGMultiSelect();

    int ClearAll();

    int StartBlock(int col,long row);
    int EndBlock(int col,long row);

    void OnLClick(int col,long row, UINT nFlags);
    void OnRClick(int col,long row, UINT nFlags);
    void OnKeyMove(int col,long row);
    void OnMouseMove(int col,long row, UINT nFlags);

    int ToggleCell(int col,long row);

    int GetOrigCell(int *col,long *row);
    int GetCurrentBlock(int *startCol,long *startRow,int *endCol,long *endRow);
    int GetTotalRange(int *startCol,long *startRow,int *endCol,long *endRow);

    int GetNumberBlocks();

    int IsSelected(int col,long row,int *block = NULL);
    int IsCellInColSelected(int col);
    int IsCellInRowSelected(long row);

    int SelectMode(int mode);
    int GetSelectMode();

    int EnumFirstSelected(int *col,long *row);
    int EnumNextSelected(int *col,long *row);

    int EnumFirstBlock(int *startCol,long *startRow,int *endCol,long *endRow);
    int EnumNextBlock(int *startCol,long *startRow,int *endCol,long *endRow);

    void AddTotalRangeToDrawHints(CUGDrawHint * hint);
};
