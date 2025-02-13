#pragma once
/***********************************************
    Ultimate Grid 97
    Copyright 1994 - 1998 Dundas Software Ltd.


    class
        CUGCStatGrid
    Purpose
        General purpose derived grid class.
        This class can be used as a starting
        point for any grid project.
************************************************/
#include <zGridO/Ugctrl.h>
#include <zGridO/UGCTsarw.h>


class COperatorStatisticsLog;

class CStatGrid : public CUGCtrl
{
public:
    COperatorStatisticsLog* m_pOperatorStatisticsLog;

    CUGSortArrowType    m_sortArrow;
    int                 m_sArrowIndex;
    CFont m_font;

    int m_sortCol;
    BOOL m_sortedAssending;

    CStatGrid();
    ~CStatGrid();

    // Overrides
    // ClassWizard generated virtual function overrides
    //{{AFX_VIRTUAL(CStatGrid)
    //}}AFX_VIRTUAL


    //{{AFX_MSG(CStatGrid)
    //}}AFX_MSG
    DECLARE_MESSAGE_MAP()

    virtual void OnSetup();
    virtual void OnMenuCommand(int col,long row,int section,int item);
    virtual void OnTH_LClicked(int col,long row,int updn,RECT *rect,POINT *point,BOOL processed);
    virtual void OnTH_DClicked(int col,long row,RECT *rect,POINT *point,BOOL processed);
    virtual int  OnSortEvaluate(CUGCell *cell1,CUGCell *cell2,int flags);
    virtual void OnKeyDown(UINT *vcKey,BOOL processed);
};
