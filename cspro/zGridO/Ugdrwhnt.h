#pragma once

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
    Details
        -cells are added to this list by the grid
         when movement is made and/or changes to the
         grid are made. Only the cells that are
         affected are added.
        -when the grid is going to redraw itself
         it calls this classes IsInvalid function
         to see if the cell really needs to be
         redrawn
************************************************/

//drawing hint linked list structure
typedef struct UGDrwHintListTag{
    int startCol;
    long startRow;
    int endCol;
    long endRow;

    UGDrwHintListTag * next;

}UGDrwHintList;

typedef struct UGDrwHintVListTag{
    int Col;
    long Row;

    UGDrwHintVListTag * next;

}UGDrwHintVList;




//drawing hint class
class CLASS_DECL_ZGRIDO CUGDrawHint{

private:

    UGDrwHintList * m_List;
    UGDrwHintVList * m_VList;

    int  m_minCol,m_maxCol;
    long m_minRow,m_maxRow;

    int  m_vMinCol,m_vMaxCol;
    long m_vMinRow,m_vMaxRow;

public:
    CUGDrawHint();
    ~CUGDrawHint();

    void AddHint(int col,long row);
    void AddHint(int startCol,long startRow,int endCol,long endRow);

    void AddToHint(int col,long row);
    void AddToHint(int startCol,long startRow,int endCol,long endRow);
    void ClearHints();

    int IsInvalid(int col,long row);
    int IsValid(int col,long row);
    int GetTotalRange(int *startCol,long *startRow,int *endCol,long *endRow);

    void SetAsValid(int col,long row);
    void SetAsValid(int startCol,long startRow,int endCol,long endRow);
};
