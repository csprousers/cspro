// RelGrid.cpp: implementation of the CLangGrid class.
//
//////////////////////////////////////////////////////////////////////

#include "StdAfx.h"
#include "langgrid.h"
#include "langedt.h"
#include "CapiLDlg.h"


#ifdef _DEBUG
#undef THIS_FILE
static char THIS_FILE[]= __FILE__;
#define new DEBUG_NEW
#endif

#define LANG_NAME           0
#define LANG_LABEL          1

const int MIN_WIDTH = 25*5;
const int SIDEH_WIDTH =25*2;
//////////////////////////////////////////////////////////////////////
// Construction/Destruction
//////////////////////////////////////////////////////////////////////



BEGIN_MESSAGE_MAP(CLangGrid,CUGCtrl)
    //{{AFX_MSG_MAP(CLangGrid)
    ON_COMMAND(IDC_ADD, OnEditAdd)
    ON_COMMAND(IDC_DELETE, OnEditDelete)
    ON_COMMAND(IDC_MODIFY, OnEditModify)
    //}}AFX_MSG_MAP
END_MESSAGE_MAP()


/////////////////////////////////////////////////////////////////////////////
//
//                        CLangGrid::OnSetup
//This function is called just after the grid window
//is created or attached to a dialog item.
//It can be used to initially setup the grid
/////////////////////////////////////////////////////////////////////////////

void CLangGrid::OnSetup(){

    EnableExcelBorders(FALSE);
    VScrollAlwaysPresent(FALSE);
    SetNumberCols(2, TRUE);
    SetNumberRows(1, TRUE);
    SetSH_Width(SIDEH_WIDTH);
    SetCurrentCellMode(2);
    SetHighlightRow(TRUE);
    SetMultiSelectMode(FALSE);

    QuickSetText     (LANG_NAME,            HEADER_ROW, _T("Name       "));
    QuickSetAlignment(LANG_NAME,            HEADER_ROW, UG_ALIGNLEFT);
    SetColWidth(LANG_NAME,25*8);

    QuickSetText     (LANG_LABEL,       HEADER_ROW, _T("Label      "));
    QuickSetAlignment(LANG_LABEL,       HEADER_ROW, UG_ALIGNLEFT);
    SetColWidth(LANG_LABEL,25*8);


    m_bAdding = false;
    m_bEditing = false;

}



/////////////////////////////////////////////////////////////////////////////
//
//                        CLangGrid::EditBegin
//
/////////////////////////////////////////////////////////////////////////////

void CLangGrid::EditBegin(int col, long row, UINT vcKey)
{

    ASSERT(row >= 0);
    ASSERT(col >= 0);
    GotoCol(col);
    VScrollEnable(ESB_DISABLE_BOTH);

    CUGCell cell;
    CRect rect;
    CIMSAString cs;
    m_aEditControl.SetSize(2);
    m_bEditing = true;
    m_iEditRow = row;
    ClearSelections();
    // Create label edit
    m_pNameEdit = new CNameEdit2();
    m_pNameEdit->SetRowCol(row, LANG_NAME);
    GetCellRect(LANG_NAME, row, &rect);
    m_pNameEdit->Create(WS_CHILD | WS_VISIBLE | WS_BORDER, rect, this, 100);
    m_pNameEdit->SetFont(&m_font);
    GetCell(LANG_NAME, row, &cell);
    cell.GetText(&cs);
    m_pNameEdit->SetWindowText(cs);
    m_aEditControl.SetAt(LANG_NAME, (CWnd*) m_pNameEdit);


    // Create label edit
    m_pLabelEdit = new CLabelEdit2();
    m_pLabelEdit->SetRowCol(row, LANG_LABEL);
    GetCellRect(LANG_LABEL, row, &rect);
    m_pLabelEdit->Create(WS_CHILD | WS_VISIBLE | WS_BORDER, rect, this, 100);
    m_pLabelEdit->SetFont(&m_font);
    GetCell(LANG_LABEL, row, &cell);
    cell.GetText(&cs);
    m_pLabelEdit->SetWindowText(cs);
    m_aEditControl.SetAt(LANG_LABEL, (CWnd*) m_pLabelEdit);


    // Set focus to field
    m_iEditCol = col;
    m_iMinCol =0;
    m_iMaxCol =1;

    m_aEditControl[col]->SetFocus();
    if (vcKey > 0) {
        m_aEditControl[col]->SendMessage(WM_CHAR, vcKey, 1);
    }
}


/////////////////////////////////////////////////////////////////////////////
//
//                        CRecordGrid::EditEnd
//
/////////////////////////////////////////////////////////////////////////////

bool CLangGrid::EditEnd(bool /*bSilent*/)
{

    CIMSAString sName;
    CIMSAString sLabel;

    if (!m_bEditing)
        return false;

    m_aEditControl[0]->GetWindowText(sName);
    m_aEditControl[1]->GetWindowText(sLabel);
    sName.Trim();



    //Check if the Name is valid
    if(!sName.IsName()) {
        AfxMessageBox(_T("Not a Valid name"));
        GotoRow(m_iEditRow);
        m_aEditControl[0]->SetFocus();
        return false;
    }

    CLangInfo langInfo;
    langInfo.m_sLabel= sLabel;
    langInfo.m_sLangName= sName;
    int iDupRow = IsDuplicate(langInfo,m_iEditRow);
    if(iDupRow) {
        CIMSAString sMsg;
        sMsg.Format(_T("Current row %d and row %d are duplicates. Cannot add a duplicate language") , m_iEditRow+1,iDupRow);
        AfxMessageBox(sMsg);
        GotoRow(m_iEditRow);
        m_aEditControl[0]->SetFocus();
        return false;
    }

    CUGCell cell;
    GetCell(0,m_iEditRow,&cell);
    CLangInfo** pLangInfo  = (CLangInfo**)cell.GetExtraMemPtr();
    if(pLangInfo && (*pLangInfo) &&  (*(*pLangInfo) )== langInfo && !m_bAdding) { //do nothing
    }
    else {
        if(m_bAdding ){
            ASSERT(!pLangInfo);
            langInfo.m_eLangInfo = eLANGINFO::NEW_INFO;
            this->m_aLangInfo.Add(langInfo);

            Update();
            m_bChanged  = true;
        }
        else {
            if(pLangInfo && (*pLangInfo) &&  !((*(*pLangInfo)) == langInfo)) {
                langInfo.m_eLangInfo = eLANGINFO::MODIFIED_INFO;

                (*pLangInfo)->m_eLangInfo = eLANGINFO::MODIFIED_INFO;
                (*pLangInfo)->m_sLangName = langInfo.m_sLangName;
                (*pLangInfo)->m_sLabel = langInfo.m_sLabel;

                UpdateLang((*pLangInfo),m_iEditRow,RGB(0,0,0));
                m_bChanged  = true;

            }
        }
    }

    //Add the info / modify the info  . update the grid
    m_bEditing = false;
    for ( int i = 0; i < m_aEditControl.GetSize();i++)
        delete m_aEditControl[i];
    m_aEditControl.RemoveAll();

    return true;
}


/////////////////////////////////////////////////////////////////////////////
//
//                        CRecordGrid::EditQuit
//
/////////////////////////////////////////////////////////////////////////////

void CLangGrid::EditQuit()
{
    SetRedraw(FALSE);

    for (int i = 0; i < m_aEditControl.GetSize();i++)
        delete m_aEditControl[i];

    m_aEditControl.RemoveAll();

    SetRedraw(TRUE);
    m_bEditing = false;
    m_bCanEdit = true;
    m_iEditCol = NONE;
    int numrows = GetNumberRows();
    if (m_bAdding)
        SetNumberRows(numrows-1,TRUE);
    m_bAdding = FALSE;
    RedrawAll();
}

/////////////////////////////////////////////////////////////////////////////
//
//                        CLangGrid::OnColSized
//
/////////////////////////////////////////////////////////////////////////////

void CLangGrid::OnColSized(int /*col*/,int */*width*/) {


}

/////////////////////////////////////////////////////////////////////////////
//
//                        CLangGrid::OnEditAdd
//
/////////////////////////////////////////////////////////////////////////////

void CLangGrid::OnEditAdd() {

    if (m_bAdding)
        return;
    // If tree control has focus, position to item on level grid and post add message

    m_bAdding = true;

    // Push old rec on stack

    int i = GetNumberRows();

    // Update grid
    Update();
    // Begin editing new item
    SetNumberRows(i+1, FALSE);
    GotoRow(i+1);


    EditBegin(LANG_NAME, i, 0);
}

/////////////////////////////////////////////////////////////////////////////
//
//                        CLangGrid::OnEditAdd
//
/////////////////////////////////////////////////////////////////////////////

void CLangGrid::OnEditModify()
{
    int i = GetCurrentRow();

    // Update grid
    Update();
    // Begin editing new item
    GotoRow(i);
    EditBegin(LANG_NAME, i, 0);
}

void CLangGrid::OnSetFocus(CWnd* /*pOldWnd*/)
{
    if (m_aEditControl.GetSize() > 0 && m_iEditCol < m_aEditControl.GetSize())
        m_aEditControl[m_iEditCol]->SetFocus();
}

/////////////////////////////////////////////////////////////////////////////
//
//                        CLangGrid::EditContinue
//
/////////////////////////////////////////////////////////////////////////////

void CLangGrid::EditContinue()
{
//  EditQuit();
    m_bAdding = FALSE;
    //OnEditAdd();
}


void CLangGrid::OnEditDelete()
{
    int iRow = GetCurrentRow();
    CUGCell cell ;
    GetCell(0,iRow,&cell);
    if(GetNumberRows() == 1) {
        AfxMessageBox(_T("Cannot delete all languages"));
        return;
    }
    CLangInfo** plangInfo  = (CLangInfo**)cell.GetExtraMemPtr();
    if(plangInfo && *plangInfo)  {
        if (AfxMessageBox(_T("Do you want to delete this language ?"), MB_YESNO) == IDYES) {
            if((*plangInfo)->m_eLangInfo != eLANGINFO::NEW_INFO ) {
                (*plangInfo)->m_eLangInfo = eLANGINFO::DELETED_INFO;
                DeleteRow(iRow);
                m_bChanged  = true;
            }
            else {
                for(int iIndex =0; iIndex < m_aLangInfo.GetSize() ; iIndex++) {
                    if(m_aLangInfo.GetAt(iIndex) == **plangInfo){
                        this->m_aLangInfo.RemoveAt(iIndex);
                    }
                }
                DeleteRow(iRow);
            }
        }
        else {
            return;
        }
    }

}

/////////////////////////////////////////////////////////////////////////////
//
//                        CLangGrid::OnDClicked
//
/////////////////////////////////////////////////////////////////////////////

void CLangGrid::OnDClicked(int col,long row,RECT */*rect*/,POINT */*point*/,BOOL /*processed*/) {

    if (row < 0) {
        return;
    }
    if (m_bEditing || m_bAdding)
    {
        if (!EditEnd())
            return;
    }
    EditBegin(col,row,0);
}


void CLangGrid::OnKeyDown(UINT* vcKey,BOOL /*processed*/)
{
    if (*vcKey==VK_PRIOR) {
        *vcKey = 0;
        if(GetKeyState(VK_CONTROL) < 0) {
            ClearSelections();
            GotoRow(0);
        }
        else {
            int height = m_GI->m_gridHeight;
            long newrow = GetCurrentRow();
            while (GetRowHeight(newrow) < height) {
                height -= GetRowHeight(newrow);
                newrow--;
                if (newrow < 1) {
                    break;
                }
            }
            ClearSelections();
            GotoRow(newrow);
        }
    }
    else if (*vcKey==VK_NEXT) {
        *vcKey = 0;
        if(GetKeyState(VK_CONTROL) < 0) {
            ClearSelections();
            GotoRow(GetNumberRows() - 1);
        }
        else {
            int height = m_GI->m_gridHeight;
            long newrow = GetCurrentRow();
            while (GetRowHeight(newrow) < height) {
                height -= GetRowHeight(newrow);
                newrow++;
                if (newrow >= GetNumberRows() - 1) {
                    break;
                }
            }
            ClearSelections();
            GotoRow(newrow);
        }
    }
}

/////////////////////////////////////////////////////////////////////////////
//
//                        CLangGrid::EditChange
//
/////////////////////////////////////////////////////////////////////////////

void CLangGrid::EditChange(UINT uChar)
{

    bool bAdding;
    bool bInserting = false;
    long row;
    switch(uChar)  {
    case VK_ESCAPE:
        EditQuit();
        m_bAdding = FALSE;
//        m_bInserting = FALSE;
        break;
    case VK_RETURN:
        m_iEditCol++;
        if (m_iEditCol > m_iMaxCol)
        {
            m_iEditCol = m_iMaxCol;
            if (EditEnd())
            {
                EditContinue();   // End this row a posibly continue to next
            }
            else
            {
                if (m_iEditCol < m_iMinCol || m_iEditCol > m_iMaxCol) {  // BMD 10 Mar 2003
                    m_iEditCol = m_iMaxCol;
                }
                MessageBeep(0);
                m_aEditControl[m_iEditCol]->SetFocus();
            }
        }
        else
        {
            GotoCell(m_iEditCol, m_iEditRow);
            m_aEditControl[m_iEditCol]->SetFocus();
        }
        break;
    case VK_CANCEL:
        m_iEditCol = m_iMaxCol;
        if (EditEnd()) {
            EditContinue();   // End this row a posibly continue to next
        //  EditQuit();   // End this row a posibly continue to next
        }
        else {
            if (m_iEditCol < m_iMinCol || m_iEditCol > m_iMaxCol) {  // BMD 10 Mar 2003
                m_iEditCol = m_iMaxCol;
            }
            m_aEditControl[m_iEditCol]->SetFocus();
        }
        break;
    case VK_TAB:
        if (m_iEditCol == 0)
        {
            CString csname;
            m_aEditControl[0]->GetWindowText(csname);
            if (csname.IsEmpty())
            {
                //see what you could do if the user tabs

            }

        }
        if (GetKeyState(VK_SHIFT) < 0) {
            m_iEditCol--;
        }
        else {
            m_iEditCol++;
        }
        if (m_iEditCol < m_iMinCol || m_iEditCol > m_iMaxCol)
        {
            m_iEditCol = m_iMaxCol;
            if (EditEnd())
            {
                EditContinue();   // End this row a posibly continue to next
            }
            else
            {
                if (m_iEditCol < m_iMinCol || m_iEditCol > m_iMaxCol) {  // BMD 10 Mar 2003
                    m_iEditCol = m_iMaxCol;
                }
                MessageBeep(0);
                m_aEditControl[m_iEditCol]->SetFocus();
            }
        }
        else
        {
            GotoCell(m_iEditCol, m_iEditRow);
            m_aEditControl[m_iEditCol]->SetFocus();
        }
        break;
    case 255:
        break;

    case VK_UP:
        bAdding = m_bAdding;
//        bInserting = m_bInserting;

        if (EditEnd()) {
            row = GetCurrentRow();
            if (bAdding || bInserting) {
                GotoRow(row);
            }
            else {
                GotoRow(--row);
            }
            m_bAdding = FALSE;
//            m_bInserting = FALSE;
        }
        else {
            if (m_iEditCol < m_iMinCol || m_iEditCol > m_iMaxCol) {  // BMD 10 Mar 2003
                m_iEditCol = m_iMaxCol;
            }
            m_aEditControl[m_iEditCol]->SetFocus();
        }
        break;
    case VK_DOWN:
        bAdding = m_bAdding;
//        bInserting = m_bInserting;
        if (EditEnd()) {
            //Set the flag to modified here for the lang stuff
            //Set the Lang info to proper flag
            row = GetCurrentRow();
            if (bAdding || bInserting) {
                GotoRow(row);
            }
            else {
                GotoRow(++row);
            }
            m_bAdding = FALSE;
//            m_bInserting = FALSE;
        }
        else {
            if (m_iEditCol < m_iMinCol || m_iEditCol > m_iMaxCol) {  // BMD 10 Mar 2003
                m_iEditCol = m_iMaxCol;
            }
            m_aEditControl[m_iEditCol]->SetFocus();
        }
        break;
    default:
        ASSERT(FALSE);
        break;
    }

}


/////////////////////////////////////////////////////////////////////////////
//
//                        CLangGrid::Size
//
/////////////////////////////////////////////////////////////////////////////

void CLangGrid::Size(CRect rect)
{
    CUGCell cell;
    CUGCellType* pCellType;
    CSize size;

    int iUsed = SIDEH_WIDTH;
    if(true) {
        for (int col = 0 ; col <=0 ; col++) {
            GetCell(col, HEADER_ROW, &cell);
            pCellType = GetCellType(HEADER_ROW, col);
            pCellType->GetBestSize(GetDC(), &size, &cell);
            SetColWidth(col, size.cx + BORDER_WIDTH);
            iUsed += size.cx + BORDER_WIDTH;
        }
    }
    GetCell(1, HEADER_ROW, &cell);
    pCellType = GetCellType(HEADER_ROW, 1);
    pCellType->GetBestSize(GetDC(), &size, &cell);
    if (size.cx > rect.Width() - m_GI->m_vScrollWidth - iUsed - 1) {
        SetColWidth(1, size.cx);
    }
    else {
        SetColWidth(1, rect.Width() - m_GI->m_vScrollWidth - iUsed - 1);
    }
    Resize(rect);
}

/////////////////////////////////////////////////////////////////////////////
//
//                        CLangGrid::Resize
//
/////////////////////////////////////////////////////////////////////////////

void CLangGrid::Resize(CRect rect) {

    MoveWindow(&rect, FALSE);
    if (m_aEditControl.GetSize() > 0) {
        for (int i = m_iMinCol ; i <= m_iMaxCol ; i++) {
            m_aEditControl[i]->PostMessage(WM_SIZE);
        }
    }
}


/////////////////////////////////////////////////////////////////////////////////
//
//  void CLangGrid::Update()
//
/////////////////////////////////////////////////////////////////////////////////
void CLangGrid::Update()
{

    //    int  ir;
    //    COLORREF rgb;
    ResetGrid();


    SetNumberRows(m_aLangInfo.GetSize());

    //TO DO Avoid Duplicates  SAVY_CAPI
    ASSERT(m_aLangInfo.GetSize() !=0);

    int iRowCount = 0;
    for(int iIndex =0 ; iIndex < m_aLangInfo.GetSize(); iIndex++ ){
        CLangInfo&  langInfo = m_aLangInfo[iIndex];

        if(langInfo.m_eLangInfo == eLANGINFO::DELETED_INFO) {
            continue;
        }
        QuickSetText (0,    iRowCount,langInfo.m_sLangName);
        CIMSAString sLabel =    langInfo.m_sLabel;
        sLabel.Trim();
        QuickSetText (1,iRowCount,sLabel );

        CUGCell cell;
        GetCell(LANG_NAME,iRowCount,&cell);
        void** plangInfo = (void**)cell.AllocExtraMem(sizeof(LPVOID));
        *plangInfo = &langInfo;
        SetCell(0,iRowCount,&cell);

        UpdateLang(&langInfo,iRowCount, RGB(0,0,0));
        iRowCount++;
    }
    if(iRowCount) {
        SetNumberRows(iRowCount);
    }

    RedrawWindow();

}

/////////////////////////////////////////////////////////////////////////////
//
//                            CLangGrid::UpdateCond
//
/////////////////////////////////////////////////////////////////////////////

void CLangGrid::UpdateLang(CLangInfo* pLangInfo, int row, COLORREF /*rgb*/)
{

    if(!pLangInfo) { //See if you need to add a blank line
    }
    else {

        CIMSAString sIndex;
        sIndex.Str(row+1);
        QuickSetText     (-1, row, sIndex);
        QuickSetAlignment(-1, row, UG_ALIGNRIGHT);

        QuickSetText     (0, row, pLangInfo->m_sLangName);
        QuickSetText     (1, row, pLangInfo->m_sLabel);
    }
}


/////////////////////////////////////////////////////////////////////////////
//
//                        CLangGrid::OnCanMove
//
/////////////////////////////////////////////////////////////////////////////

int CLangGrid::OnCanMove(int /*oldcol*/,long /*oldrow*/,int /*newcol*/,long /*newrow*/)
{
    return TRUE;
}


/////////////////////////////////////////////////////////////////////////////
//
//                        CLangGrid::OnLClicked
//
/////////////////////////////////////////////////////////////////////////////

void CLangGrid::OnLClicked(int col,long row,int updn,RECT */*rect*/,POINT */*point*/,int /*processed*/)
{

    if (updn) {
        int r = GetCurrentRow();
        if (row != r) {
            m_bCanEdit = false;
        }
    }
    else {
        if (row < 0 || col < 0) {
            if (m_bEditing) {
                EditChange(VK_CANCEL);
            }
            return;
        }
        if (m_bCanEdit) {
            if (true) { //Here check the conditions for which edit is ok
                ClearSelections();
                GotoRow(row);
                EditBegin(col, row, 0);
            }

        }
        else {
            if (m_bEditing) {
                if (EditEnd()) {
                    m_bAdding = false;
//                    m_bInserting = false;
                }
                else {
                    return;
                }
            }
            else {
                m_bCanEdit = true;
            }
        }
    }

}


/////////////////////////////////////////////////////////////////////////////
//
//                        CLangGrid::OnTH_RClicked
//
/////////////////////////////////////////////////////////////////////////////

void CLangGrid::OnTH_RClicked(int col,long row,int updn,RECT* rect,POINT* point,int processed) {

    OnRClicked(col, row, updn, rect, point, processed);
}


/////////////////////////////////////////////////////////////////////////////
//
//                        CLangGrid::OnCB_RClicked
//
/////////////////////////////////////////////////////////////////////////////

void CLangGrid::OnCB_RClicked(int updn,RECT* rect,POINT* point,int processed) {

    OnRClicked(-1, -1, updn, rect, point, processed);
}

/////////////////////////////////////////////////////////////////////////////
//
//                        CLangGrid::OnKillFocus
//
/////////////////////////////////////////////////////////////////////////////

void CLangGrid::OnKillFocus(int /*section*/) {

    m_bCanEdit = false;
}

/////////////////////////////////////////////////////////////////////////////
//
//                        CLangGrid::OnCellTypeNotify
//
/////////////////////////////////////////////////////////////////////////////

int CLangGrid::OnCellTypeNotify(long /*ID*/,int /*col*/,long /*row*/,long /*msg*/,long /*param*/){

    return TRUE;
}


/////////////////////////////////////////////////////////////////////////////
//
//                        CLangGrid::OnCanSizeTopHdg
//
/////////////////////////////////////////////////////////////////////////////

int CLangGrid::OnCanSizeTopHdg()
{
    return FALSE;
}

/////////////////////////////////////////////////////////////////////////////
//
//                        CLangGrid::OnCanSizeTopHdg
//
/////////////////////////////////////////////////////////////////////////////

int CLangGrid::OnCanSizeSideHdg()
{
    return FALSE;
}
/////////////////////////////////////////////////////////////////////////////
//
//                        CLangGrid::OnCanSizeCol
//
/////////////////////////////////////////////////////////////////////////////

int CLangGrid::OnCanSizeCol(int /*col*/){

    return TRUE;
}


/////////////////////////////////////////////////////////////////////////////
//
//                        CLangGrid::OnColSizing
//
/////////////////////////////////////////////////////////////////////////////

void CLangGrid::OnColSizing(int /*col*/,int* width) {

    if (*width < 30) {
        *width = 30;
    }
}

/////////////////////////////////////////////////////////////////////////////
//
//                        CDDGrid::OnRowChange
//
/////////////////////////////////////////////////////////////////////////////

void CLangGrid::OnRowChange(long /*oldrow*/,long /*newrow*/){
    m_bCanEdit = FALSE;
}

int CLangGrid::OnCanSizeRow(long /*row*/)
{
    return FALSE;
}

/////////////////////////////////////////////////////////////////////////////////
//
//  void CLangGrid::ResetGrid()
//
/////////////////////////////////////////////////////////////////////////////////
void CLangGrid::ResetGrid()
{

    SetRedraw(FALSE);

    int iNumberofRows  = GetNumberRows() ;
    for(int iIndex = 0; iIndex <iNumberofRows; iIndex++)
    {
        DeleteRow(0);
    }
    SetRedraw(TRUE);
}
/////////////////////////////////////////////////////////////////////////////
//
//                        CLangGrid::OnCharDown
//
/////////////////////////////////////////////////////////////////////////////

void CLangGrid::OnCharDown(UINT* vcKey,BOOL /*processed*/)
{

    if(*vcKey==VK_ESCAPE) {
        GetParent()->SendMessage(WM_COMMAND,IDCANCEL);
    }
    else if (*vcKey==VK_TAB) {
        AfxGetMainWnd()->SendMessage(WM_IMSA_SETFOCUS);
    }
    else if (*vcKey >= 32) {
        EditBegin(LANG_NAME, GetCurrentRow(), *vcKey);
    }
    else if (*vcKey == VK_RETURN) {
        EditBegin(LANG_NAME, GetCurrentRow(), 0);
    }
    else if (*vcKey == 10) {
       //TO DO check this later in CDictGrid
    }
}

int CLangGrid::IsDuplicate(CLangInfo& langInfo ,int iIgnoreRow /*=-1*/)
{
    int iRet = 0;
    int iRows = GetNumberRows() ;

    for (int iIndex =0 ; iIndex <iRows; iIndex++){
        if(iIgnoreRow == iIndex)
            continue;
        CUGCell cell;
        GetCell(0,iIndex,&cell);
        CLangInfo** pLangInfo  = (CLangInfo**)cell.GetExtraMemPtr();
        CLangInfo* pLocal= NULL;
        if(pLangInfo) {
            pLocal = *pLangInfo;
            if(pLocal) {
                CIMSAString sLang = langInfo.m_sLangName;
                sLang.Trim();
                CIMSAString sLocalLang ;
                sLocalLang = pLocal->m_sLangName;
                sLocalLang.Trim();
                if(sLang.CompareNoCase(sLocalLang) ==0 ) {
                    iRet = iIndex+1;
                    break;
                }
            }
        }
        else {
            continue;
        }
    }

    return iRet;
}
