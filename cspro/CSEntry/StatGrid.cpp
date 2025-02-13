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

#include "StdAfx.h"
#include "OperatorStatistics.h"
#include "OperatorStatisticsLog.h"
#include "StatGrid.h"

#ifdef _DEBUG
#define new DEBUG_NEW
#undef THIS_FILE
static char THIS_FILE[]= __FILE__;
#endif

const int DEF_ROWS= 10;
const int DEF_COLS = 13;
const int ARROW_SIZE = 20;

CIMSAString GetTimeStringFromSec(CIMSAString sString);

BEGIN_MESSAGE_MAP(CStatGrid,CUGCtrl)
    //{{AFX_MSG_MAP(CStatGrid)
    //}}AFX_MSG_MAP
END_MESSAGE_MAP()


/***************************************************
****************************************************/
CStatGrid::CStatGrid()
{
    m_pOperatorStatisticsLog = NULL;
    m_sortCol = 2;
    m_sortedAssending = TRUE;
}
/***************************************************
****************************************************/
CStatGrid::~CStatGrid()
{
}

/***************************************************
OnSetup
    This function is called just after the grid window
    is created or attached to a dialog item.
    It can be used to initially setup the grid
****************************************************/
void CStatGrid::OnSetup()
{
    SetSH_Width(0);

    BOOL bSetData = FALSE;
    int iRows = DEF_ROWS ;

    if( m_pOperatorStatisticsLog != NULL )
    {
        iRows = m_pOperatorStatisticsLog->GetOpStatsArray().GetSize();
        if(iRows)
            bSetData = TRUE;
    }

    CString string;

    m_font.CreateFont(15,0,0,0,400,0,0,0,0,0,0,0,0,_T("MS Sans Serif"));
    SetDefFont(&m_font);

    m_sArrowIndex = AddCellType(&m_sortArrow);

    CUGCell cell;
    GetGridDefault(&cell);
    cell.SetAlignment(UG_ALIGNLEFT);
    cell.SetHBackColor(RGB(255,192,192));
    cell.SetHTextColor(RGB(0,0,0));
    SetGridDefault(&cell);

    SetNumberRows(iRows);
    SetNumberCols(DEF_COLS);

    QuickSetText(0,-1,_T("Mode"));

    QuickSetText(1,-1,_T("Operator ID"));
    QuickSetText(2,-1,_T("Start Date"));
    QuickSetText(3,-1,_T("Start Time"));
    QuickSetText(4,-1,_T("Total Elapsed Time"));
    QuickSetText(5,-1,_T("Pause Time"));
    QuickSetText(6,-1,_T("Total Cases"));
    QuickSetText(7,-1,_T("Total Records"));
    QuickSetText(8,-1,_T("Key Strokes / Hr"));
    QuickSetText(9,-1,_T("Errors/1000 KeyStrokes"));
    //QuickSetText(10,-1,"% Fields with verify errors");
    //QuickSetText(11,-1,"% Fields with verify errors (Keyer)");
    //QuickSetText(12,-1,"% Fields with verify errors (Verifier)");
    QuickSetText(10,-1,_T("% Fields with verify errors (Keyer)"));
    QuickSetText(11,-1,_T("% Fields with verify errors (Verifier)"));
    QuickSetText(12,-1,_T("% Fields with verify errors (Total)"));

    /*for(int iCol =0; iCol < DEF_COLS; iCol++ ) {
        QuickSetCellTypeEx(iCol,-1,UGCT_NORMALMULTILINE);
    }*/

    GetHeadingDefault(&cell);
    cell.SetCellTypeEx(UGCT_NORMALMULTILINE);
    SetHeadingDefault(&cell);

    AddMenuItem(1000,_T("Find Dialog - Current Column"));
    AddMenuItem(1001,_T("Find Dialog - All Columns"));
    EnableMenu(1);

    if(bSetData) {

        const CArray<COperatorStatistics*,COperatorStatistics*>& pArr = m_pOperatorStatisticsLog->GetOpStatsArray();

        for(int rowIndex = 0;rowIndex < iRows; rowIndex++){
            COperatorStatistics* pStatObj = pArr.GetAt(rowIndex);
            QuickSetText(0,rowIndex,pStatObj->GetMode());QuickSetAlignment(0,rowIndex,UG_ALIGNLEFT);
            CIMSAString sOPID = pStatObj->GetOPID(); sOPID.Trim();
            QuickSetText(1,rowIndex,sOPID);QuickSetAlignment(1,rowIndex,UG_ALIGNLEFT);
            QuickSetText(2,rowIndex,pStatObj->GetStartDate());QuickSetAlignment(2,rowIndex,UG_ALIGNRIGHT);

            CIMSAString sStartTime = pStatObj->GetStartTime();
            int iIndex = sStartTime.ReverseFind(':');
            if(iIndex != -1) {
                sStartTime.Delete(iIndex,3);
                QuickSetText(3,rowIndex,sStartTime);
                QuickSetAlignment(3,rowIndex,UG_ALIGNRIGHT);
            }

            CIMSAString sString = pStatObj->GetTotalTime();sString.Trim();
            sString = GetTimeStringFromSec(sString);
            QuickSetText(4,rowIndex,sString);
            QuickSetAlignment(4,rowIndex,UG_ALIGNRIGHT);


            sString = pStatObj->GetPauseTime();sString.Trim();
            sString = GetTimeStringFromSec(sString);
            QuickSetText(5,rowIndex,sString);
            QuickSetAlignment(5,rowIndex,UG_ALIGNRIGHT);

            sString = _T("");
            _ultot(pStatObj->GetTotalCases(),sString.GetBuffer(NCHAR),10);
            sString.ReleaseBuffer();
            QuickSetText(6,rowIndex,sString);
            QuickSetAlignment(6,rowIndex,UG_ALIGNRIGHT);

            sString=_T(""); _ultot(pStatObj->GetTotalRecords(),sString.GetBuffer(NCHAR),10);
            sString.ReleaseBuffer();
            QuickSetText(7,rowIndex,sString);
            QuickSetAlignment(7,rowIndex,UG_ALIGNRIGHT);

            sString=IntToString(pStatObj->GetKeyStrokesPHr());
            QuickSetText(8,rowIndex,sString);
            QuickSetAlignment(8,rowIndex,UG_ALIGNRIGHT);

            sString=IntToString(pStatObj->GetErrPKKeyStrokes());
            QuickSetText(9,rowIndex,sString);
            QuickSetAlignment(9,rowIndex,UG_ALIGNRIGHT);

            sString=_T("");sString.Format(_T("%.1f"),pStatObj->GetPercentFWKErr());
            QuickSetText(10,rowIndex,sString);
            QuickSetAlignment(10,rowIndex,UG_ALIGNRIGHT);

            sString=_T("");sString.Format(_T("%.1f"),pStatObj->GetPercentFWVErr());
            QuickSetText(11,rowIndex,sString);
            QuickSetAlignment(11,rowIndex,UG_ALIGNRIGHT);

            sString=_T("");sString.Format(_T("%.1f"),pStatObj->GetPercentFWErr());
            QuickSetText(12,rowIndex,sString);
            QuickSetAlignment(12,rowIndex,UG_ALIGNRIGHT);
        }
    }

    this->BestFit(0,12, DEF_COLS,2);
    for(int  iCol = 0; iCol < DEF_COLS ; iCol++) {
            int iWidth = this->GetColWidth(iCol);
            if(iCol >= 6)
                iWidth = (int)( 3.0 * iWidth );
            this->SetColWidth(iCol,iWidth + ARROW_SIZE);
    }

    int iRowHeight = this->GetTH_Height();
    this->SetTH_Height((int)( 2.5 * iRowHeight ));

    QuickSetCellType(m_sortCol,-1,m_sArrowIndex);
    QuickSetCellTypeEx(m_sortCol,-1,UGCT_SORTARROWUP|UGCT_NORMALMULTILINE);
    if(this->GetNumberRows() > 1)
        SortBy(m_sortCol,UG_SORT_DESCENDING);

    AdjustComponentSizes();
}

/***************************************************
****************************************************/
void CStatGrid::OnMenuCommand(int col,long row,int section,int item){

    switch(item){
        case 1000:{
            FindInAllCols(FALSE);
            FindDialog();
            break;
        }
        case 1001:{
            FindInAllCols(TRUE);
            FindDialog();
            break;
        }
    }
}
/***************************************************
****************************************************/
void CStatGrid::OnTH_DClicked(int col,long row,RECT *rect,POINT *point,BOOL processed){
    OnTH_LClicked(col,row,1,rect,point,processed);
}
/***************************************************
****************************************************/
void CStatGrid::OnTH_LClicked(int col,long row,int updn,RECT *rect,POINT *point,BOOL processed)
{
    if(updn == 0)
        return;

    QuickSetCellType(m_sortCol,-1,0);

    if(col == m_sortCol){
        if(m_sortedAssending)
            m_sortedAssending = FALSE;
        else
            m_sortedAssending = TRUE;
    }
    else{
        m_sortCol = col;
        m_sortedAssending = TRUE;
    }

    BOOL bSort = FALSE;
    int iRows = 0;
    if(m_pOperatorStatisticsLog) {

        iRows = m_pOperatorStatisticsLog->GetOpStatsArray().GetSize();
        if(iRows <= 1)
            bSort  = FALSE;
        else
            bSort = TRUE;
    }
    else {
        return;
    }

    if(m_sortedAssending){

        QuickSetCellType(m_sortCol,-1,m_sArrowIndex);
        QuickSetCellTypeEx(m_sortCol,-1,UGCT_SORTARROWDOWN|UGCT_NORMALMULTILINE);

        if(bSort)
            SortBy(col,UG_SORT_ASCENDING);
    }
    else{

        QuickSetCellType(m_sortCol,-1,m_sArrowIndex);
        QuickSetCellTypeEx(m_sortCol,-1,UGCT_SORTARROWUP | UGCT_NORMALMULTILINE);

        if(bSort)
            SortBy(col,UG_SORT_DESCENDING);
    }

    RedrawAll();
}

/***************************************************
****************************************************/
int CStatGrid::OnSortEvaluate(CUGCell *cell1,CUGCell *cell2,int flags)
{
    if(cell1 == NULL && cell2 == NULL)
        return 0;
    else if(cell1 == NULL)
        return -1;
    else if(cell2 == NULL)
        return 1;

    CIMSAString sCell1 = cell1->GetText();
    CIMSAString sCell2 = cell2->GetText();
    int iRet = sCell1.CompareNoCase(sCell2);

    if(flags == UG_SORT_ASCENDING)
        return iRet;
    else
        return -iRet;
}

CIMSAString GetTimeStringFromSec(CIMSAString sString)
{
    sString.Trim();
    int iTotalTime  = _ttoi(sString);
    div_t divResult = div(iTotalTime,3600);
    int iHours = divResult.quot;
    CIMSAString sHours;sHours.Format(_T("%2d"),iHours);sHours.Trim();
    sHours=sHours.AdjustLenLeft(2,'0');
    divResult = div(divResult.rem,60);
    int iMin =   divResult.quot;
    CIMSAString sMin;sMin.Format(_T("%2d"),iMin);sMin.Trim();
    sMin=sMin.AdjustLenLeft(2,'0');

    CIMSAString sRet;
    sRet.Format(_T("%s:%s"), (LPCTSTR)sHours, (LPCTSTR)sMin);
    return sRet;
}

void CStatGrid::OnKeyDown(UINT *vcKey,BOOL processed)
{
    if (*vcKey==VK_ESCAPE)
        GetParent()->SendMessage(WM_COMMAND,IDOK);
}
