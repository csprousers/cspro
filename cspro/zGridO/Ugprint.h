#pragma once

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
#ifdef UG_ENABLE_PRINTING

class CLASS_DECL_ZGRIDO CUGPrint{

private:

    typedef struct CUGPrintListTag{
        int     page;
        int     startCol;
        int     endCol;
        long    startRow;
        long    endRow;
        CUGPrintListTag * next;
    }CUGPrintList;

    CUGPrintList*   m_printList;

    int             m_pagesWide;
    int             m_pagesHigh;

    float           m_printVScale;
    float           m_printHScale;

    int             m_printLeftMarginPX;
    int             m_printTopMarginPX;
    int             m_printRightMarginPX;
    int             m_printBottomMarginPX;

    int             m_paperWidthPX;
    int             m_paperHeightPX;


    //print options
    int m_printLeftMargin;  //margin in mm
    int m_printTopMargin;
    int m_printRightMargin;
    int m_printBottomMargin;

    BOOL m_printFrame;      //print a frame(border) around the grid
    BOOL m_printCenter;     //center the grid within the margins
    BOOL m_fitToPage;       //fit the range of cells to one page
    BOOL m_printTopHeading; //print the top heading
    BOOL m_printSideHeading;//print the side heading


    double m_scaleMultiplier;


    int ClearPrintList();
    int AddPageToList(int page,int startCol,int endCol,long startRow,long endRow);
    int GetPageFromList(int page,int *startCol,int *endCol,long *startRow,long *endRow);

    void InternPrintCell(CDC *dc,RECT * rect,int col,long row);

public:

    CUGCtrl *       m_ctrl;
    CUGGridInfo *   m_GI;

    CUGPrint();
    ~CUGPrint();

    int PrintInit(CDC * pDC, CPrintDialog* pPD, int startCol,long startRow,
                int endCol,long endRow);
    int PrintPage(CDC * pDC, int pageNum);
    int PrintSetMargin(int whichMargin,int size);
    int PrintSetScale(double scale);
    int PrintSetOption(int option,long param);
    int PrintGetOption(int option,long *param);
};

#endif
//
//
//
