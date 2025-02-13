//***************************************************************************
//  File name: condgrid.cpp
//
//  Description:
//       Data Dictionary record properties grid implementation
//
//  History:    Date       Author   Comment
//              ---------------------------
//              03 Aug 00   bmd     Created for CSPro 2.1
//
//***************************************************************************
#include "StdAfx.h"
#include "ConditionGrid.h"
#include "ConditionEdit.h"
#include "QSFCndVw.h"
#include <zUtilO/BCMenu.h>
#include <zCapiO/CapiLogicParameters.h>
#include <zCapiO/CapiQuestion.h>
#include <zCapiO/CapiQuestionManager.h>
#include <zCapiO/CapiCondition.h>


constexpr int COND_COL    =  0;
constexpr int LABEL_COL   =  1;
constexpr long HEADER_ROW = -1;

constexpr int MIN_WIDTH   = 25 * 5;
constexpr int SIDEH_WIDTH = 25 * 2;

const TCHAR* const CONDITION = _T("Condition                               ");
#define SHOWBLANK '\x7F'

BEGIN_MESSAGE_MAP(CCondGrid,CUGCtrl)
    ON_COMMAND(ID_QSFCOND_ADD, OnEditAdd)
    ON_COMMAND(ID_QSFCOND_INSERT, OnEditInsert)
    ON_COMMAND(ID_QSFCOND_DELETE, OnEditDelete)
    ON_COMMAND(ID_QSFCOND_MODIFY, OnEditModify)
END_MESSAGE_MAP()


/////////////////////////////////////////////////////////////////////////////
//
//                        CCondGrid::OnSetup
//
/////////////////////////////////////////////////////////////////////////////

void CCondGrid::OnSetup()
{
    m_bCanMove = true;
    EnableExcelBorders(FALSE);
    VScrollAlwaysPresent(FALSE);
    SetNumberCols(1, TRUE);
    SetNumberRows(1, TRUE);
    SetSH_Width(SIDEH_WIDTH);
    SetCurrentCellMode(2);
    SetHighlightRow(TRUE);
    SetMultiSelectMode(FALSE);

    QuickSetText     (COND_COL, HEADER_ROW, CONDITION);
    QuickSetAlignment(COND_COL, HEADER_ROW, UG_ALIGNLEFT);

    m_bAdding = false;
    m_bInserting = false;
    m_bEditing = false;
    m_pLabelEdit = nullptr;
}


/////////////////////////////////////////////////////////////////////////////
//
//                        CCondGrid::EditChange
//
/////////////////////////////////////////////////////////////////////////////

void CCondGrid::EditChange(UINT uChar, bool bSilent /*=false*/)
{
    CQSFCndView* pCondView = (CQSFCndView*)GetParent();
    CFormDoc* pDoc = (CFormDoc*)pCondView->GetDocument();

    bool bAdding;
    bool bInserting;
    long row;
    switch(uChar)  {
    case VK_ESCAPE:
        EditQuit();
        m_bAdding = false;
        m_bInserting = false;
        break;
    case VK_RETURN:
        if (EditEnd(bSilent))
        {
            EditContinue();   // End this row a posibly continue to next
        }
        else
        {
            MessageBeep(0);
            m_pLabelEdit->SetFocus();
        }
        break;
    case VK_CANCEL:
        if (!EditEnd(bSilent)) {
            m_pLabelEdit->SetFocus();
        }
        break;
    case VK_TAB:
        if (EditEnd())
        {
            EditContinue();   // End this row a posibly continue to next
        } else {
            MessageBeep(0);
            m_pLabelEdit->SetFocus();
        }
        break;
    case 255:
        break;

    case VK_UP:
        bAdding = m_bAdding;
        bInserting = m_bInserting;

        if (EditEnd()) {
            pDoc->SetModifiedFlag();
            row = GetCurrentRow();
            if (bAdding || bInserting) {
                GotoRow(row);
            }
            else {
                GotoRow(--row);
            }
            m_bAdding = false;
            m_bInserting = false;
        }
        else {
            m_pLabelEdit->SetFocus();
        }
        break;
    case VK_DOWN:
        bAdding = m_bAdding;
        bInserting = m_bInserting;
        if (EditEnd()) {
            pDoc->SetModifiedFlag();
            row = GetCurrentRow();
            if (bAdding || bInserting) {
                GotoRow(row);
            }
            else {
                GotoRow(++row);
            }
            m_bAdding = false;
            m_bInserting = false;
        }
        else {
            m_pLabelEdit->SetFocus();
        }
        break;
    default:
        ASSERT(FALSE);
        break;
    }

}


/////////////////////////////////////////////////////////////////////////////
//
//                        CCondGrid::Size
//
/////////////////////////////////////////////////////////////////////////////

void CCondGrid::Size(CRect rect)
{
    CUGCell cell;
    CUGCellType* pCellType;
    CSize size;

    int iUsed = SIDEH_WIDTH;
    GetCell(COND_COL, HEADER_ROW, &cell);
    pCellType = GetCellType(HEADER_ROW, LABEL_COL);
    pCellType->GetBestSize(GetDC(), &size, &cell);
    if (size.cx > rect.Width() - m_GI->m_vScrollWidth - iUsed - 1) {
        SetColWidth(COND_COL, size.cx);
    }
    else {
        SetColWidth(COND_COL, rect.Width() - m_GI->m_vScrollWidth - iUsed - 1);
    }
    Resize(rect);
}


/////////////////////////////////////////////////////////////////////////////
//
//                        CCondGrid::Resize
//
/////////////////////////////////////////////////////////////////////////////

void CCondGrid::Resize(CRect rect)
{
    MoveWindow(&rect, FALSE);
    if (m_pLabelEdit) {
        m_pLabelEdit->PostMessage(WM_SIZE);
    }
}


/////////////////////////////////////////////////////////////////////////////
//
//                        CCondGrid::OnColSized
//
/////////////////////////////////////////////////////////////////////////////

void CCondGrid::OnColSized(int /*col*/, int* /*width*/)
{
}

void CCondGrid::UpdateGrid()
{
    CView* pView = (CView*)GetParent();
    CFormDoc* pDoc = (CFormDoc*)pView->GetDocument();

    ResetGrid();

    m_bCanMove = false;

    if (pDoc->GetCapiEditorViewModel().CanHaveText()) {

        auto conditions = pDoc->GetCapiEditorViewModel().GetQuestion().GetConditions();
        SetNumberRows(conditions.size());

        if (conditions.empty()) {
            // Should have minimum of one row
            SetNumberRows(1);
            QuickSetText(COND_COL, 0, CString());
        }
        else {
            for (size_t i = 0; i < conditions.size(); i++) {
                QuickSetText(COND_COL, i, conditions[i].GetLogic());
            }
        }

        int selected_row = pDoc->GetCapiEditorViewModel().GetSelectedConditionIndex();
        
        if (GetCurrentRow() != selected_row) {
            GotoRow(selected_row);
        }
    }
    else {
        SetNumberRows(0);
    }

    RedrawWindow();
    m_bCanMove = true;
}


/////////////////////////////////////////////////////////////////////////////
//
//                        CCondGrid::OnLClicked
//
/////////////////////////////////////////////////////////////////////////////

void CCondGrid::OnLClicked(int col, long row, int updn, RECT* /*rect*/, POINT* /*point*/, int /*processed*/)
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
                    m_bInserting = false;
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
//                        CCondGrid::OnTH_RClicked
//
/////////////////////////////////////////////////////////////////////////////

void CCondGrid::OnTH_RClicked(int col, long row, int updn, RECT* rect, POINT* point, int processed)
{
    OnRClicked(col, row, updn, rect, point, processed);
}


/////////////////////////////////////////////////////////////////////////////
//
//                        CCondGrid::OnCB_RClicked
//
/////////////////////////////////////////////////////////////////////////////

void CCondGrid::OnCB_RClicked(int updn,RECT* rect,POINT* point,int processed)
{
    OnRClicked(-1, -1, updn, rect, point, processed);
}


/////////////////////////////////////////////////////////////////////////////
//
//                        CCondGrid::OnRClicked
//
/////////////////////////////////////////////////////////////////////////////

void CCondGrid::OnRClicked(int /*col*/, long row, int updn, RECT* /*rect*/, POINT* point, int /*processed*/)
{
    if (updn) {
        if (IsEditing()) {
            EditChange(VK_CANCEL);
        }
        return;
    }
    BCMenu popMenu;   // BMD 29 Sep 2003
    popMenu.CreatePopupMenu();

    bool bDisableDel = false;
    int iNumRows = GetNumberRows();
    if(iNumRows == 1) {
        //if the condition string is  blank
        CIMSAString sCond;
        QuickGetText(COND_COL,0,&sCond);
        sCond.Trim();
        if(sCond.IsEmpty()) {
            bDisableDel  = true;
        }
    }


    popMenu.AppendMenu(MF_STRING, ID_QSFCOND_ADD, _T("&Add Condition\tCtrl+A"));
    popMenu.AppendMenu(MF_STRING, ID_QSFCOND_MODIFY, _T("&Modify Condition\tCtrl+M"));
   // popMenu.AppendMenu(MF_STRING, ID_QSFCOND_INSERT, "&Insert Condition\tIns");
    if(bDisableDel) {
        popMenu.AppendMenu(MF_STRING|MF_DISABLED|MF_GRAYED, ID_QSFCOND_DELETE, _T("&Delete Condition\tCtrl+Del"));
    }
    else {
        popMenu.AppendMenu(MF_STRING, ID_QSFCOND_DELETE, _T("&Delete Condition\tCtrl+Del"));
    }


//  popMenu.LoadToolbar(IDR_DICT_FRAME);   // BMD 29 Sep 2003

    CRect rectWin;
    GetWindowRect(rectWin);
    popMenu.TrackPopupMenu(TPM_RIGHTBUTTON, rectWin.left + point->x, rectWin.top + point->y + GetRowHeight(row), this);
}


/////////////////////////////////////////////////////////////////////////////
//
//                        CCondGrid::OnDClicked
//
/////////////////////////////////////////////////////////////////////////////

void CCondGrid::OnDClicked(int /*col*/, long /*row*/, RECT* /*rect*/, POINT* /*point*/, BOOL /*processed*/)
{
#ifdef _NEEDED
    if (row < 0) {
        return;
    }
    if (m_aItem[row].rec < 0) {
        return;
    }
    CView* pView = (CView*) GetParent();
    CDDDoc* pDoc = (CDDDoc*) pView->GetDocument();
    CTreeCtrl* pTreeCtrl = (CTreeCtrl*)pDoc->GetTreeCtrl();
    HTREEITEM hItem = pTreeCtrl->GetSelectedItem();
    hItem = pTreeCtrl->GetChildItem(hItem);
    for (int i = 0 ; i < m_aItem[row].item ; i++) {
        hItem = pTreeCtrl->GetNextSiblingItem(hItem);
    }
    pTreeCtrl->Select(hItem, TVGN_CARET);
    ((CDDGView*) pView)->m_gridItem.PostMessage(WM_SETFOCUS);
#endif
    return;
}


/////////////////////////////////////////////////////////////////////////////
//
//                        CCondGrid::OnKeyDown
//
/////////////////////////////////////////////////////////////////////////////

void CCondGrid::OnKeyDown(UINT* vcKey, BOOL /*processed*/)
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
//                        CCondGrid::OnCharDown
//
/////////////////////////////////////////////////////////////////////////////

void CCondGrid::OnCharDown(UINT* vcKey, BOOL /*processed*/)
{
    if (*vcKey==VK_TAB) {
        AfxGetMainWnd()->SendMessage(WM_IMSA_SETFOCUS);
    }
    else if (*vcKey >= 32) {
        EditBegin(COND_COL, GetCurrentRow(), *vcKey);
    }
    else if (*vcKey == VK_RETURN) {
        EditBegin(COND_COL, GetCurrentRow(), 0);
    }
    else if (*vcKey == 10) {
       //TO DO check this later in CDictGrid
    }
}


/////////////////////////////////////////////////////////////////////////////
//
//                        CCondGrid::OnCellTypeNotify
//
/////////////////////////////////////////////////////////////////////////////

int CCondGrid::OnCellTypeNotify(long /*ID*/, int /*col*/, long /*row*/, long /*msg*/, long /*param*/)
{
    return TRUE;
}


/////////////////////////////////////////////////////////////////////////////
//
//                        CCondGrid::EditBegin
//
/////////////////////////////////////////////////////////////////////////////

void CCondGrid::EditBegin(int col, long row, UINT vcKey)
{
    GetParentFrame()->SetActiveView((CView*)this->GetParent());
    ASSERT(row >= 0);
    ASSERT(col >= 0);
    GotoCol(col);
    VScrollEnable(ESB_DISABLE_BOTH);

    CUGCell cell;
    CRect rect;
    CIMSAString cs;
    m_bEditing = true;
    m_iEditRow = row;
    ClearSelections();

    // Create label edit
    m_pLabelEdit = new CLabelEdit1();
    m_pLabelEdit->SetRowCol(row, COND_COL);
    GetCellRect(COND_COL, row, &rect);
    m_pLabelEdit->Create(WS_CHILD | WS_VISIBLE | WS_BORDER, rect, this, 100);
    m_pLabelEdit->SetFont(&m_font);
    GetCell(COND_COL, row, &cell);
    cell.GetText(&cs);
    m_pLabelEdit->SetWindowText(cs);

    // Set focus to field
    m_pLabelEdit->SetFocus();

    if (vcKey > 0) {
        m_pLabelEdit->SendMessage(WM_CHAR, vcKey, 1);
    }
}


/////////////////////////////////////////////////////////////////////////////
//
//                        CCondGrid::EditEnd
//
/////////////////////////////////////////////////////////////////////////////

bool CCondGrid::EditEnd(bool /*bSilent*/)
{
    if (!m_bEditing){
        m_bAdding = false;
        m_bInserting = false;
        return false;
    }
    CIMSAString sCondition;
    m_pLabelEdit->GetWindowText(sCondition);
    sCondition = sCondition.Trim();
    int iDupRow = IsDuplicate(sCondition,m_iEditRow);
    if(iDupRow) {
        CIMSAString sMsg;
        sMsg.Format(_T("Current row %d and row %d are duplicates. Cannot add a duplicate condition") , m_iEditRow+1,iDupRow);
        AfxMessageBox(sMsg);
        GotoRow(m_iEditRow);
        m_pLabelEdit->SetFocus();
        return false;
    }

    CView* pView = (CView*)GetParent();
    CFormDoc* pDoc = (CFormDoc*)pView->GetDocument();
    CapiEditorViewModel& view_model = pDoc->GetCapiEditorViewModel();

    // check if the condition is valid
    if( !sCondition.IsEmpty() )
    {
        CapiEditorViewModel::SyntaxCheckResult result = view_model.CheckSyntax(CapiLogicParameters::Type::Condition, CS2WS(sCondition));
        if (std::holds_alternative<CapiEditorViewModel::SyntaxCheckError>(result)) {
            AfxMessageBox(FormatText(_T("Compilation error: %s"), std::get<CapiEditorViewModel::SyntaxCheckError>(result).error_message.c_str()));
            GotoRow(m_iEditRow);
            m_pLabelEdit->SetFocus();
            return false;
        }
    }

    pDoc->GetCapiEditorViewModel().SetCondition(m_iEditRow, sCondition);
    QuickSetText(COND_COL, m_iEditRow, sCondition);

    m_bEditing = false;
    m_bAdding = false;
    m_bInserting = false;
    delete m_pLabelEdit;
    m_pLabelEdit = nullptr;

    return true;
}


/////////////////////////////////////////////////////////////////////////////
//
//                        CCondGrid::EditQuit
//
/////////////////////////////////////////////////////////////////////////////

void CCondGrid::EditQuit()
{
    SetRedraw(FALSE);

    delete m_pLabelEdit;
    m_pLabelEdit = nullptr;

    SetRedraw(TRUE);
    m_bEditing = false;
    m_bCanEdit = true;

    //int numrows = GetNumberRows();
    if (m_bAdding) {
        GotoRow(0);
        if (GetNumberRows() > 1)
            SetNumberRows(GetNumberRows() - 1);
    }

    RedrawAll();
    //((CMDIFrameWnd*)AfxGetMainWnd())->MDIGetActive()->SendMessage(WM_INITMENU, IDR_DICT_FRAME);
    SetFocus();
    VScrollEnable(ESB_ENABLE_BOTH);
    VScrollRedraw();

    m_bAdding = false;
    m_bInserting = false;

}


/////////////////////////////////////////////////////////////////////////////
//
//                        CCondGrid::OnSetFocus
//
/////////////////////////////////////////////////////////////////////////////

void CCondGrid::OnSetFocus(CWnd* /*pOldWnd*/)
{
    if (m_pLabelEdit)
        m_pLabelEdit->SetFocus();
}


/////////////////////////////////////////////////////////////////////////////
//
//                        CCondGrid::OnKillFocus
//
/////////////////////////////////////////////////////////////////////////////

void CCondGrid::OnKillFocus(int /*section*/)
{
    m_bCanEdit = false;
}



void CCondGrid::OnEditAdd()
{
    if (m_bAdding || m_bEditing)
        return;
    m_bAdding = true;

    int iEdit;
    CString first_condition;
    QuickGetText(COND_COL, 0, &first_condition);

    if(first_condition.Trim().IsEmpty()) {
        // If first row is empty then just edit that condition rather than adding a second
        iEdit = 0;
    } else {
        SetNumberRows(GetNumberRows() + 1);
        iEdit = GetNumberRows() - 1;
    }

    GotoRow(iEdit);

    // Begin editing new item
    EditBegin(COND_COL, iEdit, 0);
}


/////////////////////////////////////////////////////////////////////////////
//
//                        CCondGrid::OnEditInsert
//
/////////////////////////////////////////////////////////////////////////////

void CCondGrid::OnEditInsert()
{
}


/////////////////////////////////////////////////////////////////////////////
//
//                        CCondGrid::OnEditDelete
//
/////////////////////////////////////////////////////////////////////////////

void CCondGrid::OnEditDelete()
{
    int iRow = GetCurrentRow();
    if (AfxMessageBox(_T("Do you want to delete this condition ?"), MB_YESNO) == IDYES) {
        CView* pView = (CView*) GetParent();
        CFormDoc* pDoc = (CFormDoc*) pView->GetDocument();
        pDoc->GetCapiEditorViewModel().DeleteCondition(iRow);
        pDoc->UpdateAllViews(nullptr, Hint::CapiEditorUpdateCondition);
    }
}


/////////////////////////////////////////////////////////////////////////////
//
//                        CCondGrid::OnEditModify
//
/////////////////////////////////////////////////////////////////////////////
void CCondGrid::OnEditModify()
{
    int i = GetCurrentRow();
    if (i < 0)
        return;

    // Begin editing new item
    GotoRow(i);
    EditBegin(COND_COL, i, 0);
}


/////////////////////////////////////////////////////////////////////////////
//
//                        CCondGrid::OnCanSizeTopHdg
//
/////////////////////////////////////////////////////////////////////////////

int CCondGrid::OnCanSizeTopHdg()
{
    return FALSE;
}

/////////////////////////////////////////////////////////////////////////////
//
//                        CCondGrid::OnCanSizeTopHdg
//
/////////////////////////////////////////////////////////////////////////////

int CCondGrid::OnCanSizeSideHdg()
{
    return FALSE;
}


/////////////////////////////////////////////////////////////////////////////
//
//                        CCondGrid::OnCanSizeCol
//
/////////////////////////////////////////////////////////////////////////////
int CCondGrid::OnCanSizeCol(int /*col*/)
{
    return FALSE;
}


/////////////////////////////////////////////////////////////////////////////
//
//                        CCondGrid::OnColSizing
//
/////////////////////////////////////////////////////////////////////////////
void CCondGrid::OnColSizing(int /*col*/, int* width)
{
    if (*width < 30) {
        *width = 30;
    }
}

/////////////////////////////////////////////////////////////////////////////
//
//                        CDDGrid::OnRowChange
//
/////////////////////////////////////////////////////////////////////////////

void CCondGrid::OnRowChange(long /*oldrow*/, long newrow)
{
    m_bCanEdit = FALSE;
    CView* pView = (CView*) GetParent();
    CFormDoc* pDoc = (CFormDoc*) pView->GetDocument();

    if (m_bCanMove) {
        pDoc->GetCapiEditorViewModel().SetSelectedConditionIndex(newrow);
        pDoc->UpdateAllViews(pView, Hint::CapiEditorUpdateCondition);
    }
}

int CCondGrid::OnCanSizeRow(long /*row*/)
{
    return FALSE;
}

void CCondGrid::OnUpdate()
{
    UpdateGrid();
}

/////////////////////////////////////////////////////////////////////////////
//
//                        CCondGrid::EditContinue
//
/////////////////////////////////////////////////////////////////////////////

void CCondGrid::EditContinue()
{
//  EditQuit();
    m_bAdding = false;

//  OnEditAdd();
}


/////////////////////////////////////////////////////////////////////////////////
//
//  void CCondGrid::ResetGrid()
//
/////////////////////////////////////////////////////////////////////////////////
void CCondGrid::ResetGrid()
{
    SetRedraw(FALSE);

    int iNumberofRows  = GetNumberRows() ;
    for(int iIndex = 0; iIndex <iNumberofRows; iIndex++)
    {
        DeleteRow(0);
    }
    SetRedraw(TRUE);
}

int CCondGrid::IsDuplicate(const CString& qsfCond ,int iIgnoreRow /*=-1*/)
{
    int iRet = 0;
    int iRows = GetNumberRows() ;

    for (int iIndex =0 ; iIndex <iRows; iIndex++){
        if(iIgnoreRow == iIndex)
            continue;
        CString sCond;
        QuickGetText(COND_COL, iIndex, &sCond);
        if (sCond == qsfCond) {
            iRet = iIndex+1;
            break;
        }
    }

    return iRet;
}

void CCondGrid::Cut()
{
    if (IsEditing())
        m_pLabelEdit->Cut();
}

void CCondGrid::Copy()
{
    if (IsEditing())
        m_pLabelEdit->Copy();
}

void CCondGrid::Paste()
{
    if (IsEditing())
        m_pLabelEdit->Paste();
}

void CCondGrid::Undo()
{
    if (IsEditing())
        m_pLabelEdit->Undo();
}
