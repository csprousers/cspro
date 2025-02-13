#pragma once

/***********************************************
    Ultimate Grid 97
    Copyright 1994 - 1997 Dundas Software Ltd.

    Class
        CUGGridInfo
    Purpose
        This class contains all of the grid
        specific properties. An instance of this
        class is created for every grid sheet that
        is created. This way every sheet can have
        its own properties.
    Details
        -Since all of the grid's properties are stored
         in CUGGridInfo classes, it makes it quite
         simple to serialize the information
        -The CUGCtrl class holds an array of these
         classes with one element for every grid sheet.
         (the grid by default has one sheet)
        -The properties found in this class should be
         set using the functions found in the CUGCtrl
         class. By setting properties this way, the
         grid will only set the properties for the
         currently selected sheet.
************************************************/

/////////////////////////////////////////////////
typedef struct _UGCOLINFO
{
    int             width;
    CUGDataSource * dataSource;
    CUGCell *       colDefault;
    int             colTranslation;

}UGCOLINFO;


/////////////////////////////////////////////////
class CLASS_DECL_ZGRIDO CUGGridInfo: public CObject{

public:

    CUGGridInfo();
    virtual ~CUGGridInfo();

    //column info
    int     m_numberCols;
    int     m_currentCol;
    int     m_lastCol;
    int     m_leftCol;
    int     m_lastLeftCol;
    int     m_maxLeftCol;
    int     m_defColWidth;
    int     m_rightCol;
    int     m_dragCol;

    UGCOLINFO*  m_colInfo;
    bool        m_bCrossTab; //SAVY 09/15/03 To prevent right line on locked cols/rows


    //row info
    long    m_numberRows;
    long    m_currentRow;
    long    m_lastRow;
    long    m_topRow;
    long    m_lastTopRow;
    long    m_maxTopRow;
    int *   m_rowHeights;
    int     m_defRowHeight;
    int     m_uniformRowHeightFlag; //true or false
    long    m_bottomRow;
    long    m_dragRow;

    //headings
    int     m_numberTopHdgRows;
    int     *m_topHdgHeights;

    int     m_numberSideHdgCols;
    int     *m_sideHdgWidths;


    //defaults
    CUGCell *   m_gridDefaults;
    CUGCell *   m_hdgDefaults;

    //current cell
    CUGCell *   m_currentCell;
    CUGCell     m_cell;         //general purpose cell object

    //sizes
    int m_topHdgHeight;     //pixels
    int m_sideHdgWidth;     //pixels
    int m_vScrollWidth;     //pixels
    int m_hScrollHeight;    //pixels
    int m_tabWidth;         //pixels

    int m_showHScroll;      //TRUE or FALSE
    int m_showVScroll;      //TRUE or FALSE

    int m_gridWidth;        //calcualated using the values above
    int m_gridHeight;

    RECT    m_gridRect;     //calcualated using the values above
    RECT    m_topHdgRect;
    RECT    m_sideHdgRect;
    RECT    m_cnrBtnRect;
    RECT    m_tabRect;
    RECT    m_vScrollRect;
    RECT    m_hScrollRect;

    //highlighting
    int     m_highlightRowFlag;     //TRUE or False
    int     m_multiSelectFlag;      //selection mode  - bits
                                    //0: off  1:cell  2:row
                                    //4: indent headings
                                    //8:  don't move current cell
                                    //16: no deselect

    int     m_currentCellMode;      //mode(bits) 1:focus rect 2:highlight
    BOOL    m_showFocusRect;
    BOOL    m_highLightCurrentCell;


    //other options
    int     m_mouseScrollFlag;      //TRUE or FALSE

    int     m_threeDHeight;         // 1 - n

    int     m_paintMode;            //if false then do not paint

    int     m_enablePopupMenu;      //TRUE or FALSE

    int     m_enableHints;          //TRUE or FALSE
    int     m_enableVScrollHints;   //TRUE or FALSE
    int     m_enableHScrollHints;   //TRUE or FALSE

    int     m_userSizingMode;       //0 -off 1-normal 2-update on the fly
    int     m_userBestSizeFlag;     //TRUE or FALSE

    int     m_enableJoins;          //TRUE or FALSE

    int     m_enableColSwapping;    //TRUE or FALSE

    int     m_enableCellOverLap;    //TRUE or FALSE

    int     m_enableExcelBorders;   //TRUE or FALSE

    //scrollbars
    int     m_vScrollMode;          // 0-normal 2- tracking 3-joystick
    int     m_hScrollMode;          // 0-normal 2- tracking

    //balistic
    int     m_ballisticMode;        //0- off 1-increment 2-squared 3- cubed
    int     m_ballisticDelay;       //slow scroll delay
    int     m_ballisticKeyMode;     //0- off n - number of key repeats for speed
                                    //increase
    int     m_ballisticKeyDelay;    //slow scroll delay

    //double buffer
    int     m_doubleBufferMode;     // 0- off 1 - on

    //column and row locking
    int     m_numLockCols;
    int     m_numLockRows;
    int     m_lockColWidth;
    int     m_lockRowHeight;


    //zooming multiplication factor
    float   m_zoomMultiplier;
    BOOL    m_zoomOn;

    //allow cells to be partially visible when moved into
    BOOL m_noPartlyVisible; //TRUE = must be visible, FALSE= may not be

    CFont * m_defFont;

    int             m_defDataSourceIndex;
    CUGDataSource*  m_defDataSource;
    CUGMem*         m_CUGMem;


    //movement type 0-keyboard 1-lbutton 2-rbutton 3-mousemove
    int     m_moveType;
    //flags - if moved by mouse
    UINT    m_moveFlags;


    //multi-select
    CUGMultiSelect      * m_multiSelect;

    //cursors
    HCURSOR m_arrowCursor;
    HCURSOR m_WEResizseCursor;
    HCURSOR m_NSResizseCursor;


    //edit control variables
    CWnd * m_editCtrl;

    //margins for drawing text in cells
    int m_margin;

    //overlay objects
    CUGPtrList  *m_CUGOverlay;

    //def border pen
    CPen * m_defBorderPen;

    int m_trackingWndMode; // 0-normal 1-stay close
};
