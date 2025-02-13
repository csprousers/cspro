//***************************************************************************
//  File name: DDGView.cpp
//
//  Description:
//       Data Dictionary grid view implementation
//
//  History:    Date       Author   Comment
//              ---------------------------
//              10 Nov 99   bmd     Created for CSPro 2.0
//
//***************************************************************************

#include "StdAfx.h"
#include "Ddgview.h"
#include "OccDlg.h"
#include "PrintDlg.h"
#include <zUtilO/Filedlg.h>


IMPLEMENT_DYNCREATE(CDDGView, CView)


BEGIN_MESSAGE_MAP(CDDGView, CView)
    ON_WM_CREATE()
    ON_WM_SIZE()
    ON_UPDATE_COMMAND_UI(ID_EDIT_DELETE, OnUpdateEditDelete)
    ON_COMMAND(ID_EDIT_DELETE, OnEditDelete)
    ON_COMMAND(ID_EDIT_ADD, OnEditAdd)
    ON_UPDATE_COMMAND_UI(ID_EDIT_ADD, OnUpdateEditAdd)
    ON_COMMAND(ID_EDIT_INSERT, OnEditInsert)
    ON_UPDATE_COMMAND_UI(ID_EDIT_INSERT, OnUpdateEditInsert)
    ON_COMMAND(ID_EDIT_COPY, OnEditCopy)
    ON_COMMAND(ID_EDIT_CUT, OnEditCut)
    ON_COMMAND(ID_EDIT_PASTE, OnEditPaste)
    ON_COMMAND(ID_EDIT_UNDO, OnEditUndo)
    ON_UPDATE_COMMAND_UI(ID_EDIT_UNDO, OnUpdateEditUndo)
    ON_UPDATE_COMMAND_UI(ID_EDIT_PASTE, OnUpdateEditPaste)
    ON_UPDATE_COMMAND_UI(ID_EDIT_CUT, OnUpdateEditCopyCut)
    ON_UPDATE_COMMAND_UI(ID_EDIT_COPY, OnUpdateEditCopyCut)
    ON_WM_SETFOCUS()
    ON_UPDATE_COMMAND_UI(ID_EDIT_MAKE_SUBITEMS, OnUpdateEditMakeSubitems)
    ON_COMMAND(ID_EDIT_MAKE_SUBITEMS, OnEditMakeSubitems)
    ON_UPDATE_COMMAND_UI(ID_EDIT_FLATTEN_OCCURRENCES, OnUpdateEditFlattenOccurrences)
    ON_COMMAND(ID_EDIT_FLATTEN_OCCURRENCES, OnEditFlattenOccurrences)
    ON_COMMAND(ID_EDIT_MODIFY, OnEditModify)
    ON_UPDATE_COMMAND_UI(ID_EDIT_MODIFY, OnUpdateEditModify)
    ON_COMMAND(ID_FILE_PRINT, OnFilePrint)
    ON_COMMAND(ID_FILE_PRINT_PREVIEW, OnFilePrintPreview)
    ON_COMMAND(ID_EDIT_REDO, OnEditRedo)
    ON_UPDATE_COMMAND_UI(ID_EDIT_REDO, OnUpdateEditRedo)
    ON_COMMAND(ID_EDIT_NOTES, OnEditNotes)
    ON_UPDATE_COMMAND_UI(ID_EDIT_NOTES, OnUpdateEditNotes)
    ON_COMMAND(ID_EDIT_OCCURRENCELABELS, OnEditOccurrenceLabels)
    ON_UPDATE_COMMAND_UI(ID_EDIT_OCCURRENCELABELS, OnUpdateEditOccurrenceLabels)
    ON_MESSAGE(UWM::Dictionary::Find, OnFind)        // Find text
    ON_COMMAND(ID_EDIT_GEN_VALUE_SET, OnEditGenValueSet)
    ON_UPDATE_COMMAND_UI(ID_EDIT_GEN_VALUE_SET, OnUpdateEditGenValueSet)
    ON_COMMAND(ID_SHIFT_F10, OnShiftF10)
    ON_COMMAND(ID_PASTE_VS_LINK, OnEditPasteValueSetLink)
    ON_UPDATE_COMMAND_UI(ID_PASTE_VS_LINK, OnUpdateEditPasteValueSetLink)
END_MESSAGE_MAP()


CDDGView::CDDGView()
    :   m_iGrid(DictionaryGrid::Dictionary),
        m_iCount(0),
        m_sizesSet(false)
{
    EnableAutomation();
}


CDDGrid& CDDGView::GetCurrentGrid()
{
    return ( m_iGrid == DictionaryGrid::Dictionary ) ? m_gridDict :
           ( m_iGrid == DictionaryGrid::Level )      ? m_gridLevel :
           ( m_iGrid == DictionaryGrid::Record)      ? m_gridRecord :
           ( m_iGrid == DictionaryGrid::Item)        ? m_gridItem :
                                                       ReturnProgrammingError<CDDGrid&>(m_gridDict);
}


/////////////////////////////////////////////////////////////////////////////
// CDDGView drawing

void CDDGView::OnDraw(CDC* /*pDC*/)
{
    // TODO: add draw code here
}


/////////////////////////////////////////////////////////////////////////////
//
//                            CDDGView::OnCreate
//
/////////////////////////////////////////////////////////////////////////////

int CDDGView::OnCreate(LPCREATESTRUCT lpCreateStruct)
{
    if (CView::OnCreate(lpCreateStruct) == -1) {
        return -1;
    }

    CDDDoc* pDoc = assert_cast<CDDDoc*>(GetDocument());

    // Load Bitmaps
    m_bmNoteNo.LoadBitmap(IDB_NOTENO);
    m_bmNoteYes.LoadBitmap(IDB_NOTEYES);
    m_bmNoteNoGrayed.LoadBitmap(IDB_NOTENO_GRAYED);
    m_bmNoteYesGrayed.LoadBitmap(IDB_NOTEYES_GRAYED);

    m_lf.lfHeight = (LONG)( 16 * ( GetDesignerFontZoomLevel() / 100.0 ) );

    //CClientDC dc(AfxGetMainWnd());
 //   dc.SetMapMode(MM_TEXT);
 //   int iLogPixels = dc.GetDeviceCaps(LOGPIXELSY);
 //   m_lf.lfHeight = -MulDiv(18, iLogPixels, 72);

 //   //int iPointSize = MulDiv(72, m_lfEntryFont.lfHeight, iLogPixels);


    m_lf.lfWidth            = 0;
    m_lf.lfEscapement       = 0;
    m_lf.lfOrientation      = 0;
    m_lf.lfWeight           = FW_NORMAL;
    m_lf.lfItalic           = FALSE;
    m_lf.lfUnderline        = FALSE;
    m_lf.lfStrikeOut        = FALSE;
    m_lf.lfCharSet          = DEFAULT_CHARSET; //ANSI_CHARSET;
    m_lf.lfOutPrecision     = OUT_DEFAULT_PRECIS;
    m_lf.lfClipPrecision    = CLIP_DEFAULT_PRECIS;
    m_lf.lfQuality          = DEFAULT_QUALITY;
    m_lf.lfPitchAndFamily   = FF_SWISS;

    CString sFontName = GetDesignerFontName();
    sFontName.Trim();
    if(sFontName.IsEmpty()){
        lstrcpy(m_lf.lfFaceName,_T("Arial Unicode MS"));
    }
    else {
        lstrcpy(m_lf.lfFaceName,sFontName);
    }

    RECT rect = {0,0,50,50};
    CUGCell cell;

    // Create Dictionary Grid

    int row_height = (int)( DEF_ROW_HEIGHT * ( GetDesignerFontZoomLevel() / 100.0 ) );
    int top_heading_height = (int)( DEF_TOPHEADING_HEIGHT * ( GetDesignerFontZoomLevel() / 100.0 ) );

    m_gridDict.SetDefRowHeight(row_height);
    m_gridDict.SetGridFont(&m_lf);
    m_gridDict.SetBitmaps(&m_bmNoteNo, &m_bmNoteYes, &m_bmNoteNoGrayed, &m_bmNoteYesGrayed);
    m_gridDict.CreateGrid(WS_CHILD | WS_VISIBLE, rect, this, 1234);
    m_gridDict.GotoCell(0,0); // 20090930 starts horizontal scroll bar at leftmost position
//    m_gridDict.SetTH_Height(30);
    m_gridDict.SetDefFont(m_gridDict.AddFont(m_lf.lfFaceName, m_lf.lfHeight, m_lf.lfWeight));
    m_gridDict.SetDict(pDoc->GetDict());
    m_gridDict.SetTH_Height(top_heading_height);

    // Create Level Grid
    m_gridLevel.SetDefRowHeight(row_height);
    m_gridLevel.SetGridFont(&m_lf);
    m_gridLevel.SetBitmaps(&m_bmNoteNo, &m_bmNoteYes, &m_bmNoteNoGrayed, &m_bmNoteYesGrayed);
    m_gridLevel.CreateGrid(WS_CHILD, rect, this, 1234);
    m_gridLevel.GotoCell(0,0); // 20090930 (see above)
    m_gridLevel.SetDefFont(m_gridLevel.AddFont(m_lf.lfFaceName, m_lf.lfHeight, m_lf.lfWeight));
    m_gridLevel.SetDict(pDoc->GetDict());
    m_gridLevel.SetTH_Height(top_heading_height);

    // Create Record Grid
    m_gridRecord.SetDefRowHeight(row_height);
    m_gridRecord.SetGridFont(&m_lf);
    m_gridRecord.SetBitmaps(&m_bmNoteNo, &m_bmNoteYes, &m_bmNoteNoGrayed, &m_bmNoteYesGrayed);
    m_gridRecord.CreateGrid(WS_CHILD, rect, this, 1234);
    m_gridRecord.GotoCell(0,0); // 20090930 (see above)
    m_gridRecord.SetDefFont(m_gridRecord.AddFont(m_lf.lfFaceName, m_lf.lfHeight, m_lf.lfWeight));
    m_gridRecord.SetDict(pDoc->GetDict());
    m_gridRecord.SetTH_Height(top_heading_height);

    // Create Item Grid
    m_gridItem.SetDefRowHeight(row_height);
    m_gridItem.SetGridFont(&m_lf);
    m_gridItem.SetBitmaps(&m_bmNoteNo, &m_bmNoteYes, &m_bmNoteNoGrayed, &m_bmNoteYesGrayed);
    m_gridItem.CreateGrid(WS_CHILD, rect, this, 1234);
    m_gridItem.GotoCell(0,0); // 20090930 (see above)
    m_gridItem.SetDefFont(m_gridItem.AddFont(m_lf.lfFaceName, m_lf.lfHeight, m_lf.lfWeight));
    m_gridItem.SetDict(pDoc->GetDict());
    m_gridItem.SetTH_Height(top_heading_height);

    return 0;
}


/////////////////////////////////////////////////////////////////////////////
//
//                            CDDGView::OnUpdate
//
/////////////////////////////////////////////////////////////////////////////

void CDDGView::OnUpdate(CView* /*pSender*/, LPARAM lHint, CObject* pHint)
{
    if (!m_sizesSet) {

        CRect rect;
        GetClientRect(&rect);
        m_gridDict.Size(rect);
        m_gridLevel.Size(rect);
        m_gridRecord.Size(rect);
        m_gridItem.Size(rect);

        m_sizesSet = true;
    }

    CDDDoc* pDoc = assert_cast<CDDDoc*>(GetDocument());
    CDataDict* pDict = pDoc->GetDict();

    DictTreeNode* dict_tree_node;
    if (lHint == Hint::DictionaryTreeSelectionChanged) {
        dict_tree_node = assert_cast<DictTreeNode*>(pHint);
    }
    else {
        CDDTreeCtrl* pTreeCtrl = pDoc->GetDictTreeCtrl();
        if (pTreeCtrl == NULL) {
            return;
        }
        HTREEITEM hItem = pTreeCtrl->GetSelectedItem();
        if(!hItem)
            return;
        dict_tree_node = pTreeCtrl->GetTreeNode(hItem);
    }

    CString csTemp;
    CUGCell cell;
    if (dict_tree_node->GetDictElementType() == DictElementType::Dictionary) {
        // Update view of the contents of a dictionary
        m_iGrid = DictionaryGrid::Dictionary;
        ResizeGrid();
        m_gridDict.Update(pDict);
    }
    else if (dict_tree_node->GetDictElementType() == DictElementType::Level) {
        // Update view of the contents of a level
        m_iGrid = DictionaryGrid::Level;
        ResizeGrid();
        m_gridLevel.Update(pDict, dict_tree_node->GetLevelIndex());
    }
    else if (dict_tree_node->GetDictElementType() == DictElementType::Record) {
        // Update view of the contents of a record
        m_iGrid = DictionaryGrid::Record;
        ResizeGrid();
        m_gridRecord.Update(pDict, dict_tree_node->GetLevelIndex(), dict_tree_node->GetRecordIndex());
    }
    else if (dict_tree_node->GetDictElementType() == DictElementType::Item ||
             dict_tree_node->GetDictElementType() == DictElementType::ValueSet) {
        // Update view of the contents of an item
        m_iGrid = DictionaryGrid::Item;
        ResizeGrid();
        m_gridItem.Update(pDict, dict_tree_node->GetLevelIndex(), dict_tree_node->GetRecordIndex(), dict_tree_node->GetItemIndex(), dict_tree_node->GetValueSetIndex());
    }
    pDoc->SetModified(pDoc->IsModified());
}


/////////////////////////////////////////////////////////////////////////////
//
//                            CDDGView::OnSize
//
/////////////////////////////////////////////////////////////////////////////

void CDDGView::OnSize(UINT /*nType*/, int /*cx*/, int /*cy*/)
{
//  CView::OnSize(nType, cx, cy);
    ResizeGrid();
}


/////////////////////////////////////////////////////////////////////////////
//
//                            CDDGView::ResizeGrid
//
/////////////////////////////////////////////////////////////////////////////

void CDDGView::ResizeGrid()
{
    CRect rect;
    GetClientRect(&rect);

    CDDGrid& current_grid = GetCurrentGrid();

    CDDDoc* pDoc = assert_cast<CDDDoc*>(GetDocument());
    pDoc->SetGrid(&current_grid);

    for( CDDGrid* grid : std::initializer_list<CDDGrid*>{ &m_gridDict, &m_gridLevel, &m_gridRecord, &m_gridItem } )
        grid->ShowWindow(( grid == &current_grid ) ? SW_SHOW : SW_HIDE);

    current_grid.Resize(rect);

    if (GetFocus() != this) {
        current_grid.SetFocus();         // BMD remove to reduce flicker 19 Apr 2000
    }
}


/////////////////////////////////////////////////////////////////////////////
//
//                            CDDGView::OnEditUndo
//
/////////////////////////////////////////////////////////////////////////////

void CDDGView::OnEditUndo()
{
    CDDDoc* pDoc = assert_cast<CDDDoc*>(GetDocument());

    if( !pDoc->GetUndoStack().CanUndo() )
        return;

    CDDTreeCtrl* pTreeCtrl = pDoc->GetDictTreeCtrl();
    pTreeCtrl->SetUpdateAllViews(TRUE);

    CDDGrid& current_grid = GetCurrentGrid();

    if( current_grid.IsEditing() )
    {
        current_grid.SendMessage(WM_COMMAND, ID_EDIT_UNDO);
    }

    else
    {
        pDoc->UndoChange(true);
    }
}


/////////////////////////////////////////////////////////////////////////////
//
//                            CDDGView::OnEditRedo
//
/////////////////////////////////////////////////////////////////////////////

void CDDGView::OnEditRedo()
{
    CDDDoc* pDoc = assert_cast<CDDDoc*>(GetDocument());

    if( !pDoc->GetUndoStack().CanRedo() )
        return;

    CDDTreeCtrl* pTreeCtrl = pDoc->GetDictTreeCtrl();
    pTreeCtrl->SetUpdateAllViews(TRUE);
    pDoc->RedoChange();
}


void CDDGView::OnEditCut()
{
    GetCurrentGrid().SendMessage(WM_COMMAND, ID_EDIT_CUT);
}

void CDDGView::OnEditCopy()
{
    GetCurrentGrid().SendMessage(WM_COMMAND, ID_EDIT_COPY);
}

void CDDGView::OnEditPaste()
{
    GetCurrentGrid().SendMessage(WM_COMMAND, ID_EDIT_PASTE);
}

void CDDGView::OnEditModify()
{
    GetCurrentGrid().SendMessage(WM_COMMAND, ID_EDIT_MODIFY);
}

void CDDGView::OnEditAdd()
{
    GetCurrentGrid().SendMessage(WM_COMMAND, ID_EDIT_ADD);
}

void CDDGView::OnEditInsert()
{
    GetCurrentGrid().SendMessage(WM_COMMAND, ID_EDIT_INSERT);
}

void CDDGView::OnEditDelete()
{
    GetCurrentGrid().SendMessage(WM_COMMAND, ID_EDIT_DELETE);
}

void CDDGView::OnEditNotes()
{
    GetCurrentGrid().SendMessage(WM_COMMAND, ID_EDIT_NOTES);
}


/////////////////////////////////////////////////////////////////////////////
//
//                            CDDGView::OnEditMakeSubitems
//
/////////////////////////////////////////////////////////////////////////////

void CDDGView::OnEditMakeSubitems() {

    if (m_iGrid == DictionaryGrid::Record) {
        m_gridRecord.SendMessage(WM_COMMAND, ID_EDIT_MAKE_SUBITEMS);
    }
}


/////////////////////////////////////////////////////////////////////////////
//
//                            CDDGView::OnUpdateEditUndo
//
/////////////////////////////////////////////////////////////////////////////

void CDDGView::OnUpdateEditUndo(CCmdUI* pCmdUI)
{
    CDictChildWnd* pDictChildWnd = assert_cast<CDictChildWnd*>(GetParentFrame());
    if (pDictChildWnd->isQuestionnaireView()) {
        pCmdUI->Enable(FALSE);
        return;
    }
    std::optional<BOOL> bEnable;
    CDDDoc* pDoc = assert_cast<CDDDoc*>(GetDocument());

    CDDGrid& current_grid = GetCurrentGrid();

    if( current_grid.IsEditing() )
    {
        if( m_iGrid == DictionaryGrid::Dictionary )
        {
            bEnable = m_gridDict.IsEditingNameOrLabelAndCanUndo();
        }

        else
        {
            bEnable = TRUE;
        }
    }

    else
    {
        bEnable = pDoc->GetUndoStack().CanUndo();
    }

    pCmdUI->Enable(bEnable.value_or(FALSE));
}


/////////////////////////////////////////////////////////////////////////////
//
//                            CDDGView::OnUpdateEditRedo
//
/////////////////////////////////////////////////////////////////////////////

void CDDGView::OnUpdateEditRedo(CCmdUI* pCmdUI)
{
    CDictChildWnd* pDictChildWnd = assert_cast<CDictChildWnd*>(GetParentFrame());
    if (pDictChildWnd->isQuestionnaireView()) {
        pCmdUI->Enable(FALSE);
        return;
    }
    BOOL bEnable = FALSE;
    CDDDoc* pDoc = assert_cast<CDDDoc*>(GetDocument());

    CDDGrid& current_grid = GetCurrentGrid();

    if( !current_grid.IsEditing() )
        bEnable = pDoc->GetUndoStack().CanRedo();

    pCmdUI->Enable(bEnable);
}


/////////////////////////////////////////////////////////////////////////////
//
//                            CDDGView::OnUpdateEditCopyCut
//
/////////////////////////////////////////////////////////////////////////////

void CDDGView::OnUpdateEditCopyCut(CCmdUI* pCmdUI)
{
    CDictChildWnd* pDictChildWnd = assert_cast<CDictChildWnd*>(GetParentFrame());
    if (pDictChildWnd->isQuestionnaireView()) {
        pCmdUI->Enable(FALSE);
        return;
    }
    std::optional<bool> enable;

    CDataDict* pDict = assert_cast<CDDDoc*>(GetDocument())->GetDict();
    if (m_iGrid == DictionaryGrid::Dictionary) {
        if (GetFocus() == m_gridDict.m_CUGGrid && m_gridDict.GetCurrentRow() > 0) {
            enable = ( pDict->GetNumLevels() > 0 );
        }
    }
    else if (m_iGrid == DictionaryGrid::Level) {
        if (GetFocus() == m_gridLevel.m_CUGGrid) {
            enable = ( pDict->GetLevel(m_gridLevel.GetLevel()).GetNumRecords() > 0 );
        }
    }
    else if (m_iGrid == DictionaryGrid::Record) {
        if (GetFocus() == m_gridRecord.m_CUGGrid) {
            BOOL bIncludesRT = FALSE;
            int selCol;
            long selRow;
            int rtCode = m_gridRecord.EnumFirstSelected(&selCol, &selRow);
            while (rtCode == UG_SUCCESS) {
                if (selCol != CRecordGrid::GetLabelColumn()) {
                    rtCode = m_gridRecord.EnumNextSelected(&selCol, &selRow);
                    continue;
                }
                long iRows = m_gridRecord.GetNumberRows();
                if (selRow >= iRows) {
                    selRow = iRows - 1;
                }
                CUGCell cell;
                m_gridRecord.GetCell(selCol, selRow, &cell);
                if (m_gridRecord.GetItem(selRow) == RECTYPE || cell.GetTextColor() == GetSysColor(COLOR_GRAYTEXT)) {
                    bIncludesRT = TRUE;
                    break;
                }
                rtCode = m_gridRecord.EnumNextSelected(&selCol, &selRow);
            }
            if (bIncludesRT) {
                enable = false;
            }
            else {
                enable = ( pDict->GetLevel(m_gridRecord.GetLevel()).GetRecord(m_gridRecord.GetRecord())->GetNumItems() > 0 );
            }
        }
    }
    else if (m_iGrid == DictionaryGrid::Item) {
        if (GetFocus() == m_gridItem.m_CUGGrid) {
            enable = pDict->GetLevel(m_gridItem.GetLevel()).GetRecord(m_gridItem.GetRecord())->GetItem(m_gridItem.GetItem())->HasValueSets();
        }
    }

    if( !enable.has_value() && GetCurrentGrid().IsEditing() )
        enable = true;

    pCmdUI->Enable(enable.value_or(false));
}


/////////////////////////////////////////////////////////////////////////////
//
//                            CDDGView::OnUpdateEditPaste
//
/////////////////////////////////////////////////////////////////////////////

void CDDGView::OnUpdateEditPaste(CCmdUI* pCmdUI)
{
    CDictChildWnd* pDictChildWnd = assert_cast<CDictChildWnd*>(GetParentFrame());
    if (pDictChildWnd->isQuestionnaireView()) {
        pCmdUI->Enable(FALSE);
        return;
    }
    std::optional<bool> enable;

    if( GetFocus() == GetCurrentGrid().m_CUGGrid )
    {
        CDDDoc* pDoc = assert_cast<CDDDoc*>(GetDocument());

        if( m_iGrid == DictionaryGrid::Dictionary )
        {
            if( pDoc->GetDictClipboard().IsAvailable<DictLevel>() )
            {
                // enable when a level row is selected, or when no levels exist
                long current_row = m_gridDict.GetCurrentRow();
                enable = ( current_row >= CDictGrid::GetFirstLevelRow() ||
                            m_gridDict.GetNumberRows() <= CDictGrid::GetFirstLevelRow() );
            }                    
        }

        else if( m_iGrid == DictionaryGrid::Level )
        {
            enable = pDoc->GetDictClipboard().IsAvailable<CDictRecord>();
        }

        else if( m_iGrid == DictionaryGrid::Record )
        {
            enable = pDoc->GetDictClipboard().IsAvailable<CDictItem>();
        }

        else if( m_iGrid == DictionaryGrid::Item )
        {
            enable = CItemGrid::IsClipboardValidForValueSetPaste(*pDoc);
        }
    }

    if( !enable.has_value() && GetCurrentGrid().IsEditing() )
        enable = IsClipboardFormatAvailable(_tCF_TEXT);

    pCmdUI->Enable(enable.value_or(false));
}


void CDDGView::OnEditPasteValueSetLink()
{
    m_gridItem.SendMessage(WM_COMMAND, ID_PASTE_VS_LINK);
}

void CDDGView::OnUpdateEditPasteValueSetLink(CCmdUI* pCmdUI)
{
    CDictChildWnd* pDictChildWnd = assert_cast<CDictChildWnd*>(GetParentFrame());
    if (pDictChildWnd->isQuestionnaireView()) {
        pCmdUI->Enable(FALSE);
        return;
    }
    pCmdUI->Enable(( m_iGrid == DictionaryGrid::Item && GetFocus() == m_gridItem.m_CUGGrid ));
}

/////////////////////////////////////////////////////////////////////////////
//
//                            CDDGView::OnUpdateEditModify
//
/////////////////////////////////////////////////////////////////////////////

void CDDGView::OnUpdateEditModify(CCmdUI* pCmdUI)
{
    CDictChildWnd* pDictChildWnd = assert_cast<CDictChildWnd*>(GetParentFrame());
    if (pDictChildWnd->isQuestionnaireView()) {
        pCmdUI->Enable(FALSE);
        return;
    }
    CDDDoc* pDoc = assert_cast<CDDDoc*>(GetDocument());
    CDDTreeCtrl* pTreeCtrl = pDoc->GetDictTreeCtrl();
    if (GetFocus() == pTreeCtrl) {
        HTREEITEM hItem = pTreeCtrl->GetSelectedItem();
        DictTreeNode* dict_tree_node = pTreeCtrl->GetTreeNode(hItem);
        if (dict_tree_node->GetLevelIndex() == NONE) {
            pCmdUI->SetText(_T("&Modify Dict\tCtrl+M"));
            pCmdUI->Enable(FALSE);
        }
        else if (dict_tree_node->GetRecordIndex() == NONE) {
            pCmdUI->SetText(_T("&Modify Level\tCtrl+M"));
            pCmdUI->Enable(TRUE);
        }
        else if (dict_tree_node->GetItemIndex() == NONE) {
            pCmdUI->SetText(_T("&Modify Record\tCtrl+M"));
            if (dict_tree_node->GetRecordIndex() == COMMON) {
                pCmdUI->Enable(FALSE);
            }
            else {
                pCmdUI->Enable(TRUE);
            }
        }
        else if (dict_tree_node->GetValueSetIndex() == NONE) {
            pCmdUI->SetText(_T("&Modify Item\tCtrl+M"));
            pCmdUI->Enable(TRUE);
        }
        else {
            pCmdUI->SetText(_T("&Modify Value Set\tCtrl+M"));
            pCmdUI->Enable(TRUE);
        }
        return;
    }
    CDataDict* pDict = pDoc->GetDict();
    if (m_iGrid == DictionaryGrid::Dictionary) {
        pCmdUI->SetText(_T("&Modify Level\tCtrl+M"));
        if (GetFocus() == m_gridDict.m_CUGGrid && m_gridDict.GetCurrentRow() > 0) {
            pCmdUI->Enable(pDict->GetNumLevels() > 0);
        }
        else {
            pCmdUI->Enable(FALSE);
        }
    }
    else if (m_iGrid == DictionaryGrid::Level) {
        pCmdUI->SetText(_T("&Modify Record\tCtrl+M"));
        if (GetFocus() == m_gridLevel.m_CUGGrid) {
            pCmdUI->Enable(pDict->GetLevel(m_gridLevel.GetLevel()).GetNumRecords() > 0);
        }
        else {
            pCmdUI->Enable(FALSE);
        }
    }
    else if (m_iGrid == DictionaryGrid::Record) {
        pCmdUI->SetText(_T("&Modify Item\tCtrl+M"));
        if (GetFocus() == m_gridRecord.m_CUGGrid) {
            BOOL bIncludesRT = FALSE;
            int selCol;
            long selRow;
            int rtCode = m_gridRecord.EnumFirstSelected(&selCol, &selRow);
            while (rtCode == UG_SUCCESS) {
                if (selCol != CRecordGrid::GetLabelColumn()) {
                    rtCode = m_gridRecord.EnumNextSelected(&selCol, &selRow);
                    continue;
                }
                long iRows = m_gridRecord.GetNumberRows();
                if (selRow >= iRows) {
                    selRow = iRows - 1;
                }
                CUGCell cell;
                m_gridRecord.GetCell(selCol, selRow, &cell);
                if (m_gridRecord.GetItem(selRow) == RECTYPE || cell.GetTextColor() == GetSysColor(COLOR_GRAYTEXT)) {
                    bIncludesRT = TRUE;
                    break;
                }
                rtCode = m_gridRecord.EnumNextSelected(&selCol, &selRow);
            }
            if (bIncludesRT) {
                pCmdUI->Enable(FALSE);
            }
            else {
                pCmdUI->Enable(pDict->GetLevel(m_gridRecord.GetLevel()).GetRecord(m_gridRecord.GetRecord())->GetNumItems() > 0);
            }
        }
        else {
            pCmdUI->Enable(FALSE);
        }
    }
    else if (m_iGrid == DictionaryGrid::Item) {
        if (m_gridItem.GetNumberRows() > 0) {
            long row = m_gridItem.GetCurrentRow();
            if (row >= m_gridItem.GetNumberRows()) {
                row = m_gridItem.GetNumberRows() - 1;
            }
            if (m_gridItem.GetValue(row) == NONE) {
                pCmdUI->SetText(_T("&Modify Value Set\tCtrl+M"));
            }
            else {
                pCmdUI->SetText(_T("&Modify Value\tCtrl+M"));
            }
        }
        else {
            pCmdUI->SetText(_T("&Modify Value\tCtrl+M"));
        }
        if (GetFocus() == m_gridItem.m_CUGGrid) {
            pCmdUI->Enable(pDict->GetLevel(m_gridItem.GetLevel()).GetRecord(m_gridItem.GetRecord())->GetItem(m_gridItem.GetItem())->HasValueSets());
        }
        else {
            pCmdUI->Enable(FALSE);
        }
    }
    else {
        ASSERT(FALSE);
    }
}


/////////////////////////////////////////////////////////////////////////////
//
//                            CDDGView::OnUpdateEditAdd
//
/////////////////////////////////////////////////////////////////////////////

void CDDGView::OnUpdateEditAdd(CCmdUI* pCmdUI)
{
    CDictChildWnd* pDictChildWnd = assert_cast<CDictChildWnd*>(GetParentFrame());
    if (pDictChildWnd->isQuestionnaireView()) {
        pCmdUI->Enable(FALSE);
        return;
    }
    CDDDoc* pDoc = assert_cast<CDDDoc*>(GetDocument());
    CDDTreeCtrl* pTreeCtrl = pDoc->GetDictTreeCtrl();
    if (GetFocus() == pTreeCtrl) {
        HTREEITEM hItem = pTreeCtrl->GetSelectedItem();
        DictTreeNode* dict_tree_node = pTreeCtrl->GetTreeNode(hItem);
        if (dict_tree_node->GetLevelIndex() == NONE) {
            pCmdUI->SetText(_T("&Add Dict\tCtrl+A"));
            pCmdUI->Enable(FALSE);
        }
        else if (dict_tree_node->GetRecordIndex() == NONE) {
            pCmdUI->SetText(_T("&Add Level\tCtrl+A"));
            pCmdUI->Enable(TRUE);
        }
        else if (dict_tree_node->GetItemIndex() == NONE) {
            pCmdUI->SetText(_T("&Add Record\tCtrl+A"));
            pCmdUI->Enable(TRUE);
        }
        else if (dict_tree_node->GetValueSetIndex() == NONE) {
            pCmdUI->SetText(_T("&Add Item\tCtrl+A"));
            pCmdUI->Enable(TRUE);
        }
        else {
            pCmdUI->SetText(_T("&Add Value Set\tCtrl+A"));
            pCmdUI->Enable(TRUE);
        }
        return;
    }
    if (m_iGrid == DictionaryGrid::Dictionary) {
        pCmdUI->SetText(_T("&Add Level\tCtrl+A"));
        if (GetFocus() == m_gridDict.m_CUGGrid) {
            pCmdUI->Enable(TRUE);
        }
        else {
            pCmdUI->Enable(FALSE);
        }
    }
    else if (m_iGrid == DictionaryGrid::Level) {
        pCmdUI->SetText(_T("&Add Record\tCtrl+A"));
        if (GetFocus() == m_gridLevel.m_CUGGrid) {
            pCmdUI->Enable(TRUE);
        }
        else {
            pCmdUI->Enable(FALSE);
        }
    }
    else if (m_iGrid == DictionaryGrid::Record) {
        pCmdUI->SetText(_T("&Add Item\tCtrl+A"));
        if (GetFocus() == m_gridRecord.m_CUGGrid) {
            pCmdUI->Enable(TRUE);
        }
        else {
            pCmdUI->Enable(FALSE);
        }
    }
    else if (m_iGrid == DictionaryGrid::Item) {
        if (m_gridItem.GetNumberRows() > 0) {
            long row = m_gridItem.GetCurrentRow();
            if (row >= m_gridItem.GetNumberRows()) {
                row = m_gridItem.GetNumberRows() - 1;
            }
            if (m_gridItem.GetValue(row) == NONE) {
                if (row == m_gridItem.GetNumberRows() - 1) {
                    pCmdUI->SetText(_T("&Add Value\tCtrl+A"));
                }
                else if (m_gridItem.GetValue(row + 1) == NONE) {
                    pCmdUI->SetText(_T("&Add Value\tCtrl+A"));
                }
                else {
                    pCmdUI->SetText(_T("&Add Value Set\tCtrl+A"));
                }
            }
            else {
                pCmdUI->SetText(_T("&Add Value\tCtrl+A"));
            }
        }
        else {
            pCmdUI->SetText(_T("&Add Value Set\tCtrl+A"));
        }
        if (GetFocus() == m_gridItem.m_CUGGrid) {
            pCmdUI->Enable(TRUE);
        }
        else {
            pCmdUI->Enable(FALSE);
        }
    }
}


/////////////////////////////////////////////////////////////////////////////
//
//                            CDDGView::OnUpdateEditInsert
//
/////////////////////////////////////////////////////////////////////////////

void CDDGView::OnUpdateEditInsert(CCmdUI* pCmdUI)
{
    CDictChildWnd* pDictChildWnd = assert_cast<CDictChildWnd*>(GetParentFrame());
    if (pDictChildWnd->isQuestionnaireView()) {
        pCmdUI->Enable(FALSE);
        return;
    }
    CDDDoc* pDoc = assert_cast<CDDDoc*>(GetDocument());
    CDDTreeCtrl* pTreeCtrl = pDoc->GetDictTreeCtrl();
    if (GetFocus() == pTreeCtrl) {
        HTREEITEM hItem = pTreeCtrl->GetSelectedItem();
        DictTreeNode* dict_tree_node = pTreeCtrl->GetTreeNode(hItem);
        if (dict_tree_node->GetLevelIndex() == NONE) {
            pCmdUI->SetText(_T("&Insert Dict\tIns"));
            pCmdUI->Enable(FALSE);
        }
        else if (dict_tree_node->GetRecordIndex() == NONE) {
            pCmdUI->SetText(_T("&Insert Level\tIns"));
            pCmdUI->Enable(TRUE);
        }
        else if (dict_tree_node->GetItemIndex() == NONE) {
            pCmdUI->SetText(_T("&Insert Record\tIns"));
            if (dict_tree_node->GetRecordIndex() == COMMON) {
                pCmdUI->Enable(FALSE);
            }
            else {
                pCmdUI->Enable(TRUE);
            }
        }
        else if (dict_tree_node->GetValueSetIndex() == NONE) {
            pCmdUI->SetText(_T("&Insert Item\tIns"));
            pCmdUI->Enable(TRUE);
        }
        else {
            pCmdUI->SetText(_T("&Insert Value Set\tIns"));
            pCmdUI->Enable(TRUE);
        }
        return;
    }
    CDataDict* pDict = pDoc->GetDict();
    if (m_iGrid == DictionaryGrid::Dictionary) {
        pCmdUI->SetText(_T("&Insert Level\tIns"));
        if (GetFocus() == m_gridDict.m_CUGGrid && m_gridDict.GetCurrentRow() > 0) {
            pCmdUI->Enable(pDict->GetNumLevels() > 0);
        }
        else {
            pCmdUI->Enable(FALSE);
        }
    }
    else if (m_iGrid == DictionaryGrid::Level) {
        pCmdUI->SetText(_T("&Insert Record\tIns"));
        if (GetFocus() == m_gridLevel.m_CUGGrid) {
            pCmdUI->Enable(pDict->GetLevel(m_gridLevel.GetLevel()).GetNumRecords() > 0);
        }
        else {
            pCmdUI->Enable(FALSE);
        }
    }
    else if (m_iGrid == DictionaryGrid::Record) {
        pCmdUI->SetText(_T("&Insert Item\tIns"));
        if (GetFocus() == m_gridRecord.m_CUGGrid) {
            BOOL bIncludesRT = FALSE;
            int selCol;
            long selRow;
            int rtCode = m_gridRecord.EnumFirstSelected(&selCol, &selRow);
            while (rtCode == UG_SUCCESS) {
                if (selCol != CRecordGrid::GetLabelColumn()) {
                    rtCode = m_gridRecord.EnumNextSelected(&selCol, &selRow);
                    continue;
                }
                long iRows = m_gridRecord.GetNumberRows();
                if (selRow >= iRows) {
                    selRow = iRows - 1;
                }
                CUGCell cell;
                m_gridRecord.GetCell(selCol, selRow, &cell);
                if (m_gridRecord.GetItem(selRow) == RECTYPE || cell.GetTextColor() == GetSysColor(COLOR_GRAYTEXT)) {
                    bIncludesRT = TRUE;
                    break;
                }
                rtCode = m_gridRecord.EnumNextSelected(&selCol, &selRow);
            }
            if (bIncludesRT) {
                pCmdUI->Enable(FALSE);
            }
            else {
                pCmdUI->Enable(pDict->GetLevel(m_gridRecord.GetLevel()).GetRecord(m_gridRecord.GetRecord())->GetNumItems() > 0);
            }
        }
        else {
            pCmdUI->Enable(FALSE);
        }
    }
    else if (m_iGrid == DictionaryGrid::Item) {
        if (m_gridItem.GetNumberRows() > 0) {
            long row = m_gridItem.GetCurrentRow();
            if (row >= m_gridItem.GetNumberRows()) {
                row = m_gridItem.GetNumberRows() - 1;
            }
            if (m_gridItem.GetValue(row) == NONE) {
                pCmdUI->SetText(_T("&Insert Value Set\tIns"));
            }
            else {
                pCmdUI->SetText(_T("&Insert Value\tIns"));
            }
        }
        else {
            pCmdUI->SetText(_T("&Insert Value Set\tIns"));
        }
        if (GetFocus() == m_gridItem.m_CUGGrid) {
            pCmdUI->Enable(pDict->GetLevel(m_gridItem.GetLevel()).GetRecord(m_gridItem.GetRecord())->GetItem(m_gridItem.GetItem())->HasValueSets());
        }
        else {
            pCmdUI->Enable(FALSE);
        }
    }
}


/////////////////////////////////////////////////////////////////////////////
//
//                            CDDGView::OnUpdateEditDelete
//
/////////////////////////////////////////////////////////////////////////////

void CDDGView::OnUpdateEditDelete(CCmdUI* pCmdUI)
{
    CDictChildWnd* pDictChildWnd = assert_cast<CDictChildWnd*>(GetParentFrame());
    if (pDictChildWnd->isQuestionnaireView()) {
        pCmdUI->Enable(FALSE);
        return;
    }

    CDDDoc* pDoc = assert_cast<CDDDoc*>(GetDocument());
    CDDTreeCtrl* pTreeCtrl = pDoc->GetDictTreeCtrl();
    if (GetFocus() == pTreeCtrl) {
        HTREEITEM hItem = pTreeCtrl->GetSelectedItem();
        DictTreeNode* dict_tree_node = pTreeCtrl->GetTreeNode(hItem);
        if (dict_tree_node->GetLevelIndex() == NONE) {
            pCmdUI->SetText(_T("&Delete Dict\tDel"));
            pCmdUI->Enable(FALSE);
        }
        else if (dict_tree_node->GetRecordIndex() == NONE) {
            pCmdUI->SetText(_T("&Delete Level\tDel"));
            pCmdUI->Enable(TRUE);
        }
        else if (dict_tree_node->GetItemIndex() == NONE) {
            pCmdUI->SetText(_T("&Delete Record\tDel"));
            if (dict_tree_node->GetRecordIndex() == COMMON) {
                pCmdUI->Enable(FALSE);
            }
            else {
                pCmdUI->Enable(TRUE);
            }
        }
        else if (dict_tree_node->GetValueSetIndex() == NONE) {
            pCmdUI->SetText(_T("&Delete Item\tDel"));
            pCmdUI->Enable(TRUE);
        }
        else {
            pCmdUI->SetText(_T("&Delete Value Set\tDel"));
            pCmdUI->Enable(TRUE);
        }
        return;
    }
    CDataDict* pDict = pDoc->GetDict();
    if (m_iGrid == DictionaryGrid::Dictionary) {
        pCmdUI->SetText(_T("&Delete Level\tDel"));
        if (GetFocus() == m_gridDict.m_CUGGrid && m_gridDict.GetCurrentRow() > 0) {
            pCmdUI->Enable(pDict->GetNumLevels() > 0);
        }
        else {
            pCmdUI->Enable(FALSE);
        }
    }
    else if (m_iGrid == DictionaryGrid::Level) {
        pCmdUI->SetText(_T("&Delete Record\tDel"));
        if (GetFocus() == m_gridLevel.m_CUGGrid) {
            pCmdUI->Enable(pDict->GetLevel(m_gridLevel.GetLevel()).GetNumRecords() > 0);
        }
        else {
            pCmdUI->Enable(FALSE);
        }
    }
    else if (m_iGrid == DictionaryGrid::Record) {
        pCmdUI->SetText(_T("&Delete Item\tDel"));
        if (GetFocus() == m_gridRecord.m_CUGGrid) {
            BOOL bIncludesRT = FALSE;
            int selCol;
            long selRow;
            int rtCode = m_gridRecord.EnumFirstSelected(&selCol, &selRow);
            while (rtCode == UG_SUCCESS) {
                if (selCol != CRecordGrid::GetLabelColumn()) {
                    rtCode = m_gridRecord.EnumNextSelected(&selCol, &selRow);
                    continue;
                }
                long iRows = m_gridRecord.GetNumberRows();
                if (selRow >= iRows) {
                    selRow = iRows - 1;
                }
                CUGCell cell;
                m_gridRecord.GetCell(selCol, selRow, &cell);
                if (m_gridRecord.GetItem(selRow) == RECTYPE || cell.GetTextColor() == GetSysColor(COLOR_GRAYTEXT)) {
                    bIncludesRT = TRUE;
                    break;
                }
                rtCode = m_gridRecord.EnumNextSelected(&selCol, &selRow);
            }
            if (bIncludesRT) {
                pCmdUI->Enable(FALSE);
            }
            else {
                pCmdUI->Enable(pDict->GetLevel(m_gridRecord.GetLevel()).GetRecord(m_gridRecord.GetRecord())->GetNumItems() > 0);
            }
        }
        else {
            pCmdUI->Enable(FALSE);
        }
    }
    else if (m_iGrid == DictionaryGrid::Item) {
        if (m_gridItem.GetNumberRows() > 0) {
            long row = m_gridItem.GetCurrentRow();
            if (row >= m_gridItem.GetNumberRows()) {
                row = m_gridItem.GetNumberRows() - 1;
            }
            if (m_gridItem.GetValue(row) == NONE) {
                pCmdUI->SetText(_T("&Delete Value Set\tDel"));
            }
            else {
                pCmdUI->SetText(_T("&Delete Value\tDel"));
            }
        }
        else {
            pCmdUI->SetText(_T("&Delete Value\tDel"));
        }
        if (GetFocus() == m_gridItem.m_CUGGrid) {
            pCmdUI->Enable(pDict->GetLevel(m_gridItem.GetLevel()).GetRecord(m_gridItem.GetRecord())->GetItem(m_gridItem.GetItem())->HasValueSets());
        }
        else {
            pCmdUI->Enable(FALSE);
        }
    }
    else {
        ASSERT(FALSE);
    }
}


/////////////////////////////////////////////////////////////////////////////
//
//                            CDDGView::OnUpdateEditNotes
//
/////////////////////////////////////////////////////////////////////////////

void CDDGView::OnUpdateEditNotes(CCmdUI* pCmdUI)
{
    CDictChildWnd* pDictChildWnd = assert_cast<CDictChildWnd*>(GetParentFrame());
    if (pDictChildWnd->isQuestionnaireView()) {
        pCmdUI->Enable(FALSE);
        return;
    }
    CDataDict* pDict = assert_cast<CDDDoc*>(GetDocument())->GetDict();
    if (m_iGrid == DictionaryGrid::Dictionary) {
        if (GetFocus() == m_gridDict.m_CUGGrid) {
            pCmdUI->Enable(TRUE);
        }
        else {
            pCmdUI->Enable(FALSE);
        }
    }
    else if (m_iGrid == DictionaryGrid::Level) {
        if (GetFocus() == m_gridLevel.m_CUGGrid) {
            pCmdUI->Enable(pDict->GetLevel(m_gridLevel.GetLevel()).GetNumRecords() > 0);
        }
        else {
            pCmdUI->Enable(FALSE);
        }
    }
    else if (m_iGrid == DictionaryGrid::Record) {
        if (GetFocus() == m_gridRecord.m_CUGGrid) {
            long row = m_gridRecord.GetCurrentRow();
            if (row >= 0 && row < m_gridRecord.GetNumberRows()) {  // BMD 26 Aug 2002
                if (m_gridRecord.GetItem(row) == RECTYPE) {
                    pCmdUI->Enable(FALSE);
                }
                else {
                    pCmdUI->Enable(TRUE);
                }
            }
            else {
                pCmdUI->Enable(FALSE);
            }
        }
        else {
            pCmdUI->Enable(FALSE);
        }
    }
    else if (m_iGrid == DictionaryGrid::Item) {
        if (GetFocus() == m_gridItem.m_CUGGrid) {
            long row = m_gridItem.GetCurrentRow();
            if (row >= 0 && row < m_gridItem.GetNumberRows()) {
                if (m_gridItem.GetVPair(row) > 0) {
                    pCmdUI->Enable(FALSE);
                }
                else {
                    pCmdUI->Enable(TRUE);
                }
            }
            else {
                pCmdUI->Enable(FALSE);
            }
        }
        else {
            pCmdUI->Enable(FALSE);
        }
    }
}


/////////////////////////////////////////////////////////////////////////////
//
//                            CDDGView::OnUpdateEditMakeSubitems
//
/////////////////////////////////////////////////////////////////////////////

void CDDGView::OnUpdateEditMakeSubitems(CCmdUI* pCmdUI)
{
    CDictChildWnd* pDictChildWnd = assert_cast<CDictChildWnd*>(GetParentFrame());
    if (pDictChildWnd->isQuestionnaireView()) {
        pCmdUI->Enable(FALSE);
        return;
    }
    BOOL bEnable = FALSE;
    CDataDict* pDict = assert_cast<CDDDoc*>(GetDocument())->GetDict();
    if (m_iGrid == DictionaryGrid::Record) {
        if (GetFocus() == m_gridRecord.m_CUGGrid) {
            const CDictRecord* pRec = pDict->GetLevel(m_gridRecord.GetLevel()).GetRecord(m_gridRecord.GetRecord());
            if( pRec != pDict->GetLevel(m_gridRecord.GetLevel()).GetIdItemsRec() ) {
                int selCol;
                long selRow;
                bEnable = TRUE;
                int rtCode = m_gridRecord.EnumFirstSelected(&selCol, &selRow);
                long iRows = m_gridRecord.GetNumberRows();
                if (selRow >= iRows) {
                    selRow = iRows - 1;
                }
                while (rtCode == UG_SUCCESS) {
                    if (selCol != CRecordGrid::GetLabelColumn()) {
                        rtCode = m_gridRecord.EnumNextSelected(&selCol, &selRow);
                        if (selRow >= iRows) {
                            selRow = iRows - 1;
                        }
                        continue;
                    }
                    CUGCell cell;
                    m_gridRecord.GetCell(selCol, selRow, &cell);
                    if (m_gridRecord.GetItem(selRow) != RECTYPE && cell.GetTextColor() != GetSysColor(COLOR_GRAYTEXT)) {
                        if (pRec->GetItem(m_gridRecord.GetItem(selRow))->GetItemType() == ItemType::Subitem) {
                            bEnable = FALSE;
                            break;
                        }
                    }
                    else {
                        bEnable = FALSE;
                        break;
                    }
                    rtCode = m_gridRecord.EnumNextSelected(&selCol, &selRow);
                }
            }
        }
    }
    pCmdUI->Enable(bEnable);
}


/////////////////////////////////////////////////////////////////////////////
//
//                            CDDGView::OnSetFocus
//
/////////////////////////////////////////////////////////////////////////////

void CDDGView::OnSetFocus(CWnd* /*pOldWnd*/)
{
    GetCurrentGrid().SetFocus();
}


/////////////////////////////////////////////////////////////////////////////
//
//                            CDDGView::PreTranslateMessage
//
/////////////////////////////////////////////////////////////////////////////

BOOL CDDGView::PreTranslateMessage(MSG* pMsg)
{
    if( pMsg->message == WM_KEYDOWN && pMsg->wParam == VK_DELETE )
    {
        CDDGrid& current_grid = GetCurrentGrid();

        if( current_grid.IsEditing() )
        {
            ::SendMessage(pMsg->hwnd, pMsg->message, pMsg->wParam, pMsg->lParam);
            return TRUE;
        }
    }

    return CView::PreTranslateMessage(pMsg);
}


/////////////////////////////////////////////////////////////////////////////
//
//                            CDDGView::OnFind
//
/////////////////////////////////////////////////////////////////////////////

LRESULT CDDGView::OnFind(WPARAM /*wParam*/, LPARAM /*lParam*/)
{
    GetParentFrame()->PostMessage(UWM::Dictionary::Find);
    return 0;
}


/////////////////////////////////////////////////////////////////////////////
//
//                            CDDGView::OnFilePrint
//
/////////////////////////////////////////////////////////////////////////////

void CDDGView::OnFilePrint()
{
    CPrintDlg dlg;
    dlg.m_bPreview = FALSE;
    dlg.m_iPrintToFile = AfxGetApp()->GetProfileInt(_T("Data Dictionary"), _T("PrintToFile"), 0);
    dlg.m_iPrintBrief = AfxGetApp()->GetProfileInt(_T("Data Dictionary"), _T("PrintBrief"), 0);
    dlg.m_iPrintNameFirst = AfxGetApp()->GetProfileInt(_T("Data Dictionary"), _T("PrintNameFirst"), 0);
    if (dlg.DoModal() != IDOK) {
        return;
    }
    AfxGetApp()->WriteProfileInt(_T("Data Dictionary"), _T("PrintToFile"), dlg.m_iPrintToFile);
    AfxGetApp()->WriteProfileInt(_T("Data Dictionary"), _T("PrintBrief"), dlg.m_iPrintBrief);
    AfxGetApp()->WriteProfileInt(_T("Data Dictionary"), _T("PrintNameFirst"), dlg.m_iPrintNameFirst);
    m_bPrintToFile = FALSE;
    m_bPrintBrief = FALSE;
    m_bPrintNameFirst = FALSE;
    if (dlg.m_iPrintToFile == 1) {
        m_bPrintToFile = TRUE;
    }
    if (dlg.m_iPrintBrief == 1) {
        m_bPrintBrief = TRUE;
    }
    if (dlg.m_iPrintNameFirst == 1) {
        m_bPrintNameFirst = TRUE;
    }
    if (m_bPrintToFile) {
        PrintToFile();
    }
    else {
        CView::OnFilePrint();
    }
}


/////////////////////////////////////////////////////////////////////////////
//
//                            CDDGView::OnFilePrintPreview
//
/////////////////////////////////////////////////////////////////////////////

void CDDGView::OnFilePrintPreview()
{
    CDDDoc* pDoc = assert_cast<CDDDoc*>(GetDocument());
    if (!pDoc->IsDocOK(false)) {
        return;
    }
    CPrintDlg dlg;
    dlg.m_bPreview = TRUE;
    dlg.m_iPrintToFile = 0;
    dlg.m_iPrintBrief = AfxGetApp()->GetProfileInt(_T("Data Dictionary"), _T("PrintBrief"), 0);
    dlg.m_iPrintNameFirst = AfxGetApp()->GetProfileInt(_T("Data Dictionary"), _T("PrintNameFirst"), 0);
    if (dlg.DoModal() != IDOK) {
        return;
    }
    AfxGetApp()->WriteProfileInt(_T("Data Dictionary"), _T("PrintBrief"), dlg.m_iPrintBrief);
    AfxGetApp()->WriteProfileInt(_T("Data Dictionary"), _T("PrintNameFirst"), dlg.m_iPrintNameFirst);
    m_bPrintToFile = FALSE;
    m_bPrintBrief = FALSE;
    m_bPrintNameFirst = FALSE;
    if (dlg.m_iPrintBrief == 1) {
        m_bPrintBrief = TRUE;
    }
    if (dlg.m_iPrintNameFirst == 1) {
        m_bPrintNameFirst = TRUE;
    }
    pDoc->SetPrintPreview(TRUE);
    CView::OnFilePrintPreview();
}


/////////////////////////////////////////////////////////////////////////////
//
//                            CDDGView::OnPreparePrinting
//
/////////////////////////////////////////////////////////////////////////////

BOOL CDDGView::OnPreparePrinting( CPrintInfo *pInfo )
{
    m_bContinuePrinting = TRUE;
    return DoPreparePrinting(pInfo);
}


/////////////////////////////////////////////////////////////////////////////
//
//                            CDDGView::OnPrepareDC
//
/////////////////////////////////////////////////////////////////////////////

void CDDGView::OnPrepareDC(CDC* pDC, CPrintInfo* pInfo)
{
    CView::OnPrepareDC(pDC, pInfo);
    if (pInfo != NULL) {
        pInfo->m_bContinuePrinting = m_bContinuePrinting;
    }
}



/////////////////////////////////////////////////////////////////////////////
//
//                            CDDGView::OnPrint
//
/////////////////////////////////////////////////////////////////////////////

namespace
{
    CString PrepareNameForPrinting(CString name)
    {
        // only display up to 32 characters
        int excess_characters = name.GetLength() - 32;

        if( excess_characters > 0 )
            name = name.Left(excess_characters + 3) + _T("...");

        return name;
    }
}

void CDDGView::OnPrint(CDC* pDC, CPrintInfo* pInfo)
{
    pDC->SetMapMode(MM_TWIPS);

    CFont font;
    font.CreateFont (-160, 0, 0, 0, 400, FALSE, FALSE, 0, DEFAULT_CHARSET/*ANSI_CHARSET*/,
                           OUT_DEFAULT_PRECIS, CLIP_DEFAULT_PRECIS,
                           DEFAULT_QUALITY, FF_DONTCARE,
                           _T("Arial Unicode MS"));//_T("Arial"));

    CFont fontI;
    fontI.CreateFont (-160, 0, 0, 0, 400, TRUE, FALSE, 0, DEFAULT_CHARSET/*ANSI_CHARSET*/,
                           OUT_DEFAULT_PRECIS, CLIP_DEFAULT_PRECIS,
                           DEFAULT_QUALITY, FF_DONTCARE,
                           _T("Arial Unicode MS"));//_T("Arial"));

    CFont* pOldFont = (CFont*) pDC->SelectObject(&font);

    // Get line height
    TEXTMETRIC  tm;
    pDC->GetTextMetrics(&tm);
    m_iHeight = tm.tmHeight + tm.tmExternalLeading + 4;
    m_iLineHeight = tm.tmExternalLeading + 4;

    // Get folio
    m_folio.Create(GetDocument()->GetPathName());

    // Calculate top and bottom folio areas
    CRect rectTextArea = m_folio.GetUserArea();
    CRect rectHeadArea = rectTextArea;
    CRect rectFootArea = rectTextArea;
    int iSize = -rectTextArea.Height();
    if (m_folio.HasHeader()) {
        iSize -= 2 * m_iHeight;
        rectHeadArea.bottom = rectHeadArea.top - m_iHeight;
        rectTextArea.top -= 2 * m_iHeight;
    }
    if (m_folio.HasFooter()) {
        iSize -= 2 * m_iHeight;
        rectFootArea.top = rectFootArea.bottom + m_iHeight;
        rectTextArea.bottom += 2 * m_iHeight;
    }

    // Print top folio
    CPoint pt;
    CSize sizeText;
    CIMSAString csText;
    if (m_folio.HasHeader()) {
        int iWidth = rectHeadArea.Width();
        pDC->LPtoDP(rectHeadArea);
        pt = pDC->SetViewportOrg(rectHeadArea.TopLeft());
        pDC->TextOut(0, 0, m_folio.GetHeaderLeft(pInfo->m_nCurPage));
        csText = m_folio.GetHeaderCenter(pInfo->m_nCurPage);
        sizeText = pDC->GetTextExtent(csText);
        pDC->TextOut((iWidth - sizeText.cx)/2, 0, csText);
        csText = m_folio.GetHeaderRight(pInfo->m_nCurPage);
        sizeText = pDC->GetTextExtent(csText);
        pDC->TextOut(iWidth - sizeText.cx, 0, csText);
        pDC->SetViewportOrg(pt);
    }

    // Initialize stuff
    m_uPage = 1;
    m_iYPos = 0;
    m_iIndent = 360;
    m_iPageWidth = rectTextArea.Width();
    int iPageHeight = rectTextArea.Height();
    CSize size = CSize(0,0);
    pDC->LPtoDP(rectTextArea);
    pt = pDC->SetViewportOrg(rectTextArea.TopLeft());

    CPen pen(PS_GEOMETRIC, 3, RGB(0,0,0));
    CPen* pOldPen = pDC->SelectObject(&pen);

    // Print dictionary header
    CDDDoc* pDoc = assert_cast<CDDDoc*>(GetDocument());
    const CDataDict* pDict = pDoc->GetDict();
    if (pInfo->m_nCurPage == m_uPage) {
        size = pDC->GetTextExtent(pDict->GetLabel());
        pDC->TextOut((m_iPageWidth - size.cx)/2, m_iYPos, pDict->GetLabel());
    }
    m_iYPos -= m_iHeight;
    if (pInfo->m_nCurPage == m_uPage) {
        size = pDC->GetTextExtent(PrepareNameForPrinting(pDict->GetName()));
        pDC->TextOut((m_iPageWidth - size.cx)/2, m_iYPos, PrepareNameForPrinting(pDict->GetName()));
    }
    m_iYPos -= m_iHeight;
    m_iYPos -= m_iHeight;
    if (pInfo->m_nCurPage == m_uPage) {
        size = pDC->GetTextExtent(pDoc->GetPathName());
        pDC->TextOut((m_iPageWidth - size.cx)/2, m_iYPos, pDoc->GetPathName());
    }
    m_iYPos -= m_iHeight;
    if (pInfo->m_nCurPage == m_uPage) {
        if (pDoc->IsModified()) {
            csText = _T("Last Modified:  (Changes not saved!)");
        }
        else {
            CFileStatus status;
            CFile::GetStatus(pDoc->GetPathName(), status);
            csText.DateTime(status.m_mtime);
            csText = _T("Last Modified:  ") + csText;
        }
        size = pDC->GetTextExtent(csText);
        pDC->TextOut((m_iPageWidth - size.cx)/2, m_iYPos, csText);
    }
    m_iYPos -= m_iHeight;
    m_iYPos -= m_iHeight;

    // Print notes for dictionary
    if (!m_bPrintBrief) {
        if (!pDict->GetNote().IsEmpty()) {
            CIMSAString csNote = pDict->GetNote();
            CStringArray acsText;
            FormatNote(pDC, csNote, acsText, m_iPageWidth - 3*m_iIndent);
            pDC->SelectObject(&fontI);
            for (int x = 0 ; x < acsText.GetSize() ; x++) {
                if (m_iYPos < iPageHeight) {
                    m_uPage++;
                    m_iYPos = 0;
                    PrintLevelHead(pDC, pInfo);
                }
                if (pInfo->m_nCurPage == m_uPage) {
                    if (x == 0) {
                        pDC->TextOut(m_iIndent - 60, m_iYPos, _T("Note:"));
                    }
                    pDC->TextOut(2*m_iIndent, m_iYPos, acsText[x]);
                }
                m_iYPos -= m_iHeight;
            }
            acsText.RemoveAll();
            pDC->SelectObject(&font);
        }
        m_iYPos -= m_iHeight;
    }

    if ((m_iYPos - 10*m_iHeight) < iPageHeight) {
        m_uPage++;
        m_iYPos = 0;
    }

    // Set horizontal page spacing
    if (m_bPrintNameFirst) {
        m_iLabel = (m_iPageWidth - 1920)/3;
        m_iName = 0;
    }
    else {
        m_iLabel = 0;
        m_iName = 2*(m_iPageWidth - 1920)/3;
    }
    m_iType = m_iPageWidth - 1440;
    m_iReq = m_iPageWidth - 960;
    m_iMax = m_iPageWidth - 480;

    // Print all levels and records
    for( size_t level_number = 0; level_number < pDict->GetNumLevels(); ++level_number ) {
        const DictLevel& dict_level = pDict->GetLevel(level_number);

        for (int r = 0 ; r < dict_level.GetNumRecords() ; r++) {
            const CDictRecord* pRec = dict_level.GetRecord(r);
            if (m_iYPos - m_iHeight < iPageHeight) {
                m_uPage++;
                m_iYPos = 0;
            }
            if (m_iYPos == 0 || (level_number == 0 && r == 0)) {
                PrintLevelHead(pDC, pInfo);
            }
            if (r == 0) {
                if (pInfo->m_nCurPage == m_uPage) {
                    pDC->TextOut(m_iLabel, m_iYPos, dict_level.GetLabel());
                    pDC->TextOut(m_iName, m_iYPos, PrepareNameForPrinting(dict_level.GetName()));
                }
                m_iYPos -= m_iHeight;
                if (!m_bPrintBrief) {
                    if (!dict_level.GetNote().IsEmpty()) {
                        CIMSAString csNote = dict_level.GetNote();
                        CStringArray acsText;
                        FormatNote(pDC, csNote, acsText, m_iPageWidth - 3*m_iIndent);
                        pDC->SelectObject(&fontI);
                        for (int x = 0 ; x < acsText.GetSize() ; x++) {
                            if (m_iYPos < iPageHeight) {
                                m_uPage++;
                                m_iYPos = 0;
                                PrintLevelHead(pDC, pInfo);
                            }
                            if (pInfo->m_nCurPage == m_uPage) {
                                if (x == 0) {
                                    pDC->TextOut(1*m_iIndent - 60, m_iYPos, _T("Note:"));
                                }
                                pDC->TextOut(2*m_iIndent, m_iYPos, acsText[x]);
                            }
                            m_iYPos -= m_iHeight;
                        }
                        acsText.RemoveAll();
                        pDC->SelectObject(&font);
                    }
                }
            }
            if (pInfo->m_nCurPage == m_uPage) {
                pDC->TextOut(m_iLabel + m_iIndent, m_iYPos, pRec->GetLabel());
                pDC->TextOut(m_iName + m_iIndent, m_iYPos, PrepareNameForPrinting(pRec->GetName()));
                size = pDC->GetTextExtent(pRec->GetRecTypeVal());
                pDC->TextOut(m_iType - size.cx, m_iYPos, pRec->GetRecTypeVal());
                if (pRec->GetRequired()) {
                    size = pDC->GetTextExtent(_T("Yes"));
                    pDC->TextOut(m_iReq - size.cx, m_iYPos, _T("Yes"));
                }
                else {
                    size = pDC->GetTextExtent(_T("No"));
                    pDC->TextOut(m_iReq - size.cx, m_iYPos, _T("No"));
                }
                csText.Str(pRec->GetMaxRecs());
                size = pDC->GetTextExtent(csText);
                pDC->TextOut(m_iMax - size.cx, m_iYPos, csText);
                csText.Str(pRec->GetRecLen());
                size = pDC->GetTextExtent(csText);
                pDC->TextOut(m_iPageWidth - size.cx, m_iYPos, csText);
            }
            m_iYPos -= m_iHeight;
            if (m_iYPos < iPageHeight) {
                m_uPage++;
                m_iYPos = 0;
            }
            if (!m_bPrintBrief) {
                if (!pRec->GetNote().IsEmpty()) {
                    CIMSAString csNote = pRec->GetNote();
                    CStringArray acsText;
                    FormatNote(pDC, csNote, acsText, m_iPageWidth - 4*m_iIndent);
                    pDC->SelectObject(&fontI);
                    for (int x = 0 ; x < acsText.GetSize() ; x++) {
                        if (m_iYPos < iPageHeight) {
                            m_uPage++;
                            m_iYPos = 0;
                            PrintLevelHead(pDC, pInfo);
                        }
                        if (pInfo->m_nCurPage == m_uPage) {
                            if (x == 0) {
                                pDC->TextOut(2*m_iIndent - 60, m_iYPos, _T("Note:"));
                            }
                            pDC->TextOut(3*m_iIndent, m_iYPos, acsText[x]);
                        }
                        m_iYPos -= m_iHeight;
                    }
                    acsText.RemoveAll();
                    pDC->SelectObject(&font);
                }
            }
        }
    }


    // Set horizontal page spacing
    m_iZeroFill = m_iPageWidth;

    size = pDC->GetTextExtent(_T("Zero"));
    m_iDecChar = m_iZeroFill - 180 - size.cx;

    size = pDC->GetTextExtent(_T("Char"));
    m_iDec = m_iDecChar - 180 - size.cx;

    size = pDC->GetTextExtent(_T("Dec"));
    m_iOcc = m_iDec - 180 - size.cx;

    size = pDC->GetTextExtent(_T("Occ"));
    m_iItemType = m_iOcc - 180 - size.cx;

    size = pDC->GetTextExtent(_T("Subitem"));
    m_iDataType = m_iItemType - 180 - size.cx;

    size = pDC->GetTextExtent(_T("Alpha"));
    m_iLen = m_iDataType - 180 - size.cx;

    size = pDC->GetTextExtent(_T("999"));
    m_iStart = m_iLen - 180 - size.cx;

    size = pDC->GetTextExtent(_T("99999"));
    int iTemp = m_iStart - 180 - size.cx;
    if (m_bPrintNameFirst) {
        m_iLabel = iTemp / 3;
        m_iName = 0;
    }
    else {
        m_iLabel = 0;
        m_iName = 2 * (iTemp) / 3;
    }
    CDictItem rtItem;
    rtItem.GetLabelSet().SetCurrentLanguage(pDict->GetCurrentLanguageIndex());
    rtItem.SetLabel(_T("    (record type)"));
    rtItem.SetContentType(ContentType::Alpha);

    // Print items for each record
    for( size_t level_number = 0; level_number < pDict->GetNumLevels(); ++level_number ) {
        const DictLevel& dict_level = pDict->GetLevel(level_number);

        for (int r = 0 ; r < dict_level.GetNumRecords() ; r++) {
            m_uPage++;
            m_iYPos = 0;
            const CDictRecord* pRec = dict_level.GetRecord(r);

            // Build array of items
            m_aItem.RemoveAll();
            ITEMSORT item;
            // Add record type
            item.level = NONE;
            item.rec   = NONE;
            item.item  = RECTYPE;
            item.start =  pDict->GetRecTypeStart();
            m_aItem.Add(item);

            // Add id items for each level
            for( size_t id_level_number = 0; id_level_number < pDict->GetNumLevels(); ++id_level_number ) {
                item.level = id_level_number;
                item.rec = COMMON;
                const CDictRecord* pIdRec = pDict->GetLevel(id_level_number).GetIdItemsRec();
                for (int i = 0 ; i < pIdRec->GetNumItems() ; i++) {
                    item.item = i;
                    item.start = pIdRec->GetItem(i)->GetStart();
                    m_aItem.Add(item);
                }
            }
            // Add record items for specified record (if not id)
            item.level = level_number;
            item.rec = r;
            for (int i = 0 ; i < pRec->GetNumItems() ; i++) {
                item.item = i;
                item.start = pRec->GetItem(i)->GetStart();
                m_aItem.Add(item);
            }
            // Sort all the items
            SortItems();

            // Print Items
            for (int i = 0 ; i < m_aItem.GetSize() ; i++) {
                const CDictItem* pItem;
                CString csId = _T("");
                if (m_aItem[i].rec == r) {
                    pItem = pRec->GetItem(m_aItem[i].item);
                }
                else if (m_aItem[i].rec == COMMON) {
                    pItem = pDict->GetLevel(m_aItem[i].level).GetIdItemsRec()->GetItem(m_aItem[i].item);
                    csId = _T("(id)");
                }
                else {
                    pItem = &rtItem;
                    rtItem.SetStart(pDict->GetRecTypeStart());
                    rtItem.SetLen(pDict->GetRecTypeLen());
                }
                if (pItem->HasValueSets()) {
                    if (m_iYPos - 3*m_iHeight < iPageHeight) {
                        m_uPage++;
                        m_iYPos = 0;
                    }
                }
                else {
                    if (m_iYPos < iPageHeight) {
                        m_uPage++;
                        m_iYPos = 0;
                    }
                }
                if (m_iYPos == 0) {
                    PrintRecHead(pDC, pInfo, dict_level, pRec, i);
                }
                if (pInfo->m_nCurPage == m_uPage) {
                    pDC->TextOut(m_iLabel, m_iYPos, pItem->GetLabel());
                    if (m_iLabel == 0) {
                        pDC->TextOut(m_iName - 360, m_iYPos, csId);
                    }
                    else {
                        pDC->TextOut(m_iLabel - 360, m_iYPos, csId);
                    }
                    pDC->TextOut(m_iName, m_iYPos, PrepareNameForPrinting(pItem->GetName()));
                    csText.Str(pItem->GetStart());
                    size = pDC->GetTextExtent(csText);
                    pDC->TextOut(m_iStart - size.cx, m_iYPos, csText);
                    csText.Str(pItem->GetLen());
                    size = pDC->GetTextExtent(csText);
                    pDC->TextOut(m_iLen - size.cx, m_iYPos, csText);
                    if (pItem->GetContentType() == ContentType::Alpha) {
                        size = pDC->GetTextExtent(_T("Alpha"));
                        pDC->TextOut(m_iDataType - size.cx, m_iYPos, _T("Alpha"));
                    }
                    else if (pItem->GetContentType() == ContentType::Numeric) {
                        size = pDC->GetTextExtent(_T("Num"));
                        pDC->TextOut(m_iDataType - size.cx, m_iYPos, _T("Num"));
                    }
                    else {
                        CString type_string = CString(ToString(pItem->GetContentType())).Left(5);
                        size = pDC->GetTextExtent(type_string);
                        pDC->TextOut(m_iDataType - size.cx, m_iYPos, type_string);
                    }
                    if (pItem->GetItemType() == ItemType::Item) {
                        size = pDC->GetTextExtent(_T("Item"));
                        pDC->TextOut(m_iItemType - size.cx, m_iYPos, _T("Item"));
                    }
                    else {
                        size = pDC->GetTextExtent(_T("Subitem"));
                        pDC->TextOut(m_iItemType - size.cx, m_iYPos, _T("Subitem"));
                    }
                    csText.Str(pItem->GetOccurs());
                    size = pDC->GetTextExtent(csText);
                    pDC->TextOut(m_iOcc - size.cx, m_iYPos, csText);
                    csText.Str(pItem->GetDecimal());
                    size = pDC->GetTextExtent(csText);
                    pDC->TextOut(m_iDec - size.cx, m_iYPos, csText);
                    if (pItem->GetDecChar()) {
                        size = pDC->GetTextExtent(_T("Yes"));
                        pDC->TextOut(m_iDecChar - size.cx, m_iYPos, _T("Yes"));
                    }
                    else {
                        size = pDC->GetTextExtent(_T("No"));
                        pDC->TextOut(m_iDecChar - size.cx, m_iYPos, _T("No"));
                    }
                    if (pItem->GetZeroFill()) {
                        size = pDC->GetTextExtent(_T("Yes"));
                        pDC->TextOut(m_iZeroFill - size.cx, m_iYPos, _T("Yes"));
                    }
                    else {
                        size = pDC->GetTextExtent(_T("No"));
                        pDC->TextOut(m_iZeroFill - size.cx, m_iYPos, _T("No"));
                    }
                }
                m_iYPos -= m_iHeight;

                // Print item notes
                if (!m_bPrintBrief) {
                    if (!pItem->GetNote().IsEmpty()) {
                        CIMSAString csNote = pItem->GetNote();
                        CStringArray acsText;
                        FormatNote(pDC, csNote, acsText, m_iPageWidth - 4*m_iIndent);
                        pDC->SelectObject(&fontI);
                        for (int x = 0 ; x < acsText.GetSize() ; x++) {
                            if (m_iYPos < iPageHeight) {
                                m_uPage++;
                                m_iYPos = 0;
                                PrintRecHead(pDC, pInfo, dict_level, pRec, 1);
                            }
                            if (pInfo->m_nCurPage == m_uPage) {
                                if (x == 0) {
                                    pDC->TextOut(1*m_iIndent - 60, m_iYPos, _T("Note:"));
                                }
                                pDC->TextOut(2*m_iIndent, m_iYPos, acsText[x]);
                            }
                            m_iYPos -= m_iHeight;
                        }
                        acsText.RemoveAll();
                        pDC->SelectObject(&font);
                    }
                }

                // Print Value Sets
                if (!m_bPrintBrief) {
                    for( const auto& dict_value_set : pItem->GetValueSets() ) {
                        if (m_iYPos - 2*m_iHeight < iPageHeight) {
                            m_uPage++;
                            m_iYPos = 0;
                        }
                        if (m_iYPos == 0) {
                            PrintRecHead(pDC, pInfo, dict_level, pRec, i);
                        }
                        if (pItem->GetNumValueSets() > 1 || dict_value_set.GetLabel() != pItem->GetLabel()) {
                            if (pInfo->m_nCurPage == m_uPage) {
                                pDC->TextOut(m_iLabel + m_iIndent, m_iYPos, dict_value_set.GetLabel());
                                pDC->TextOut(m_iName + m_iIndent, m_iYPos, _T("(") + PrepareNameForPrinting(dict_value_set.GetName()) + _T(")"));
                            }
                            m_iYPos -= m_iHeight;   // BMD 22 May 2002
                        }

                        // Print value set notes
                        if (!m_bPrintBrief) {
                            if (!dict_value_set.GetNote().IsEmpty()) {
                                CIMSAString csNote = dict_value_set.GetNote();
                                CStringArray acsText;
                                FormatNote(pDC, csNote, acsText, m_iPageWidth - 4*m_iIndent);
                                pDC->SelectObject(&fontI);
                                for (int x = 0 ; x < acsText.GetSize() ; x++) {
                                    if (m_iYPos < iPageHeight) {
                                        m_uPage++;
                                        m_iYPos = 0;
                                        PrintRecHead(pDC, pInfo, dict_level, pRec, 1);
                                    }
                                    if (pInfo->m_nCurPage == m_uPage) {
                                        if (x == 0) {
                                            pDC->TextOut(2*m_iIndent - 60, m_iYPos, _T("Note:"));
                                        }
                                        pDC->TextOut(3*m_iIndent, m_iYPos, acsText[x]);
                                    }
                                    m_iYPos -= m_iHeight;
                                }
                                acsText.RemoveAll();
                                pDC->SelectObject(&font);
                            }
                        }

                        // Print Values
                        int iValSize = 0;

                        for( const auto& dict_value : dict_value_set.GetValues() ) {
                            CString csTemp = dict_value.GetRangeString();
                            CSize this_size = pDC->GetTextExtent(csTemp);
                            if (this_size.cx > iValSize) {
                                iValSize = this_size.cx;
                            }
                        }
                        for( const auto& dict_value : dict_value_set.GetValues() ) {
                            if (m_iYPos < iPageHeight) {
                                m_uPage++;
                                m_iYPos = 0;
                            }
                            if (m_iYPos == 0) {
                                PrintRecHead(pDC, pInfo, dict_level, pRec, i);
                            }
                            if (pInfo->m_nCurPage == m_uPage) {
                                CString csTemp = dict_value.GetRangeString();
                                if (dict_value.IsSpecial()) {
                                    CString special_code = ( dict_value.GetSpecialValue() == MISSING ) ? _T("(m)") :
                                                           ( dict_value.GetSpecialValue() == REFUSED ) ? _T("(r)") :
                                                           ( dict_value.GetSpecialValue() == NOTAPPL ) ? _T("(na)") :
                                                                                                         _T("(d)");
                                    pDC->TextOut(m_iLabel + m_iIndent, m_iYPos, special_code);
                                }
                                pDC->TextOut(m_iLabel + m_iIndent*2, m_iYPos, csTemp);
                                pDC->TextOut(m_iLabel + m_iIndent*2 + iValSize + 360, m_iYPos, dict_value.GetLabel());
                            }
                            m_iYPos -= m_iHeight;
                            // Print value notes
                            if (!m_bPrintBrief) {
                                if (!dict_value.GetNote().IsEmpty()) {
                                    CIMSAString csNote = dict_value.GetNote();
                                    CStringArray acsText;
                                    FormatNote(pDC, csNote, acsText, m_iPageWidth - 4*m_iIndent);
                                    pDC->SelectObject(&fontI);
                                    for (int x = 0 ; x < acsText.GetSize() ; x++) {
                                        if (m_iYPos < iPageHeight) {
                                            m_uPage++;
                                            m_iYPos = 0;
                                            PrintRecHead(pDC, pInfo, dict_level, pRec, 1);
                                        }
                                        if (pInfo->m_nCurPage == m_uPage) {
                                            if (x == 0) {
                                                pDC->TextOut(3*m_iIndent - 60, m_iYPos, _T("Note:"));
                                            }
                                            pDC->TextOut(4*m_iIndent, m_iYPos, acsText[x]);
                                        }
                                        m_iYPos -= m_iHeight;
                                    }
                                    acsText.RemoveAll();
                                    pDC->SelectObject(&font);
                                }
                            }
                        }
                    }
                    m_iYPos -= m_iHeight;
                }
            }
        }
    }
    // Print relations
    bool first_relation = true;
    for( const auto& dict_relation : pDict->GetRelations() ) {
        auto dict_relation_part_itr = dict_relation.GetRelationParts().cbegin();
        ASSERT(dict_relation_part_itr != dict_relation.GetRelationParts().cend());
        if (first_relation || m_iYPos < iPageHeight) {
            m_uPage++;
            m_iYPos = 0;
            PrintRelationHead(pDC, pInfo);
            first_relation = false;
        }
        if (pInfo->m_nCurPage == m_uPage) {
            pDC->TextOut(0, m_iYPos, PrepareNameForPrinting(dict_relation.GetName()));
            pDC->TextOut(m_iPrimary, m_iYPos, dict_relation.GetPrimaryName().c_str());
            if (dict_relation_part_itr->IsPrimaryLinkedByOccurrence()) {
                pDC->TextOut(m_iPrimLink, m_iYPos, _T("(occ)"));
            }
            else {
                pDC->TextOut(m_iPrimLink, m_iYPos, dict_relation_part_itr->GetPrimaryLink().c_str());
            }
            pDC->TextOut(m_iSecondary, m_iYPos, dict_relation_part_itr->GetSecondaryName().c_str());
            if (dict_relation_part_itr->IsSecondaryLinkedByOccurrence()) {
                pDC->TextOut(m_iSecLink, m_iYPos, _T("(occ)"));
            }
            else {
                pDC->TextOut(m_iSecLink, m_iYPos, dict_relation_part_itr->GetSecondaryLink().c_str());
            }
        }
        m_iYPos -= m_iHeight;
        for (++dict_relation_part_itr; dict_relation_part_itr != dict_relation.GetRelationParts().cend(); ++dict_relation_part_itr) {
            if (m_iYPos < iPageHeight) {
                m_uPage++;
                m_iYPos = 0;
                PrintRelationHead(pDC, pInfo);
            }
            if (pInfo->m_nCurPage == m_uPage) {
                if (dict_relation_part_itr->IsPrimaryLinkedByOccurrence()) {
                    pDC->TextOut(m_iPrimLink, m_iYPos, _T("(occ)"));
                }
                else {
                    pDC->TextOut(m_iPrimLink, m_iYPos, dict_relation_part_itr->GetPrimaryLink().c_str());
                }
                pDC->TextOut(m_iSecondary, m_iYPos, dict_relation_part_itr->GetSecondaryName().c_str());
                if (dict_relation_part_itr->IsSecondaryLinkedByOccurrence()) {
                    pDC->TextOut(m_iSecLink, m_iYPos, _T("(occ)"));
                }
                else {
                    pDC->TextOut(m_iSecLink, m_iYPos, dict_relation_part_itr->GetSecondaryLink().c_str());
                }
            }
            m_iYPos -= m_iHeight;
        }
    }

    pInfo->SetMaxPage(m_uPage);
    pDC->SelectObject(pOldPen);

    // Print bottom folio
    pDC->SetViewportOrg(pt);
    if (m_folio.HasFooter()) {
        int iWidth = rectFootArea.Width();
        pDC->LPtoDP(rectFootArea);
        pDC->SetViewportOrg(rectFootArea.TopLeft());
        pDC->TextOut(0, 0, m_folio.GetFooterLeft(pInfo->m_nCurPage));
        csText = m_folio.GetFooterCenter(pInfo->m_nCurPage);
        sizeText = pDC->GetTextExtent(csText);
        pDC->TextOut((iWidth - sizeText.cx)/2, 0, csText);
        csText = m_folio.GetFooterRight(pInfo->m_nCurPage);
        sizeText = pDC->GetTextExtent(csText);
        pDC->TextOut(iWidth - sizeText.cx, 0, csText);
        pDC->SetViewportOrg(pt);
    }

    // Change back to old font
    pDC->SelectObject(pOldFont);

    if (pInfo->m_nCurPage == m_uPage && !pInfo->m_bPreview) {
        m_bContinuePrinting = FALSE;
    }
}


/////////////////////////////////////////////////////////////////////////////
//
//                            CDDGView::FormatNote
//
/////////////////////////////////////////////////////////////////////////////

int CDDGView::FormatNote(CDC* pDC, const CIMSAString& notes, CStringArray& acsText, int iMaxWidth)
{
    int iChar = 500;
    int iWidth = 0;
    bool bDone = FALSE;
    do {
        notes.Wrap(acsText, 0, iChar, true);
        for (int i = 0 ; i < acsText.GetSize() ; i++) {
            CSize size = pDC->GetTextExtent(acsText[i]);
            if (size.cx > iWidth) {
                iWidth = size.cx;
            }
        }
        if (iWidth < iMaxWidth) {
            bDone = TRUE;
        }
        else {
            iChar = (iChar * iMaxWidth)/iWidth - 2;
            acsText.RemoveAll();
            iWidth = 0;
        }
    } while (!bDone);
    return iWidth;
}


/////////////////////////////////////////////////////////////////////////////
//
//                            CDDGView::PrintLevelHead
//
/////////////////////////////////////////////////////////////////////////////

void CDDGView::PrintLevelHead(CDC* pDC, CPrintInfo* pInfo)
{
    if (pInfo->m_nCurPage == m_uPage) {
        pDC->MoveTo(0, m_iYPos);
        pDC->LineTo(m_iPageWidth, m_iYPos);
        m_iYPos -= m_iLineHeight;

        pDC->TextOut(m_iLabel, m_iYPos, _T("Level Label"));
        pDC->TextOut(m_iName, m_iYPos, _T("Level Name"));
        CSize size = pDC->GetTextExtent(_T("Type"));
        pDC->TextOut(m_iType - size.cx, m_iYPos, _T("Type"));
        size = pDC->GetTextExtent(_T("Rec"));
        pDC->TextOut(m_iPageWidth - size.cx, m_iYPos, _T("Rec"));
        m_iYPos -= m_iHeight;

        pDC->TextOut(m_iLabel + m_iIndent, m_iYPos, _T("Record Label"));
        pDC->TextOut(m_iName + m_iIndent, m_iYPos, _T("Record Name"));
        size = pDC->GetTextExtent(_T("Value"));
        pDC->TextOut(m_iType - size.cx, m_iYPos, _T("Value"));
        size = pDC->GetTextExtent(_T("Req"));
        pDC->TextOut(m_iReq - size.cx, m_iYPos, _T("Req"));
        size = pDC->GetTextExtent(_T("Max"));
        pDC->TextOut(m_iMax - size.cx, m_iYPos, _T("Max"));
        size = pDC->GetTextExtent(_T("Len"));
        pDC->TextOut(m_iPageWidth - size.cx, m_iYPos, _T("Len"));
        m_iYPos -= m_iHeight;

        pDC->MoveTo(0, m_iYPos);
        pDC->LineTo(m_iPageWidth, m_iYPos);
        m_iYPos -= m_iLineHeight;
    }
    else {
        m_iYPos = m_iYPos - 2*m_iHeight - 2*m_iLineHeight;
    }
}


/////////////////////////////////////////////////////////////////////////////
//
//                            CDDGView::PrintRelationHead
//
/////////////////////////////////////////////////////////////////////////////

void CDDGView::PrintRelationHead(CDC* pDC, CPrintInfo* pInfo)
{
    m_iPrimary = m_iPageWidth / 5;
    m_iPrimLink = 2 * m_iPrimary;
    m_iSecondary = 3 * m_iPrimary;
    m_iSecLink = 4 * m_iPrimary;

    if (pInfo->m_nCurPage == m_uPage) {
        pDC->MoveTo(0, m_iYPos);
        pDC->LineTo(m_iPageWidth, m_iYPos);
        m_iYPos -= m_iLineHeight;

        pDC->TextOut(0, m_iYPos, _T("Relation Name"));
        pDC->TextOut(m_iPrimary, m_iYPos, _T("Primary"));
        pDC->TextOut(m_iPrimLink, m_iYPos, _T("Linked by"));
        pDC->TextOut(m_iSecondary, m_iYPos, _T("Secondary"));
        pDC->TextOut(m_iSecLink, m_iYPos, _T("Linked by"));
        m_iYPos -= m_iHeight;

        pDC->MoveTo(0, m_iYPos);
        pDC->LineTo(m_iPageWidth, m_iYPos);
        m_iYPos -= m_iLineHeight;
    }
    else {
        m_iYPos = m_iYPos - 2*m_iHeight - 2*m_iLineHeight;
    }
}


/////////////////////////////////////////////////////////////////////////////
//
//                            CDDGView::PrintRecHead
//
/////////////////////////////////////////////////////////////////////////////

void CDDGView::PrintRecHead(CDC* pDC, CPrintInfo* pInfo, const DictLevel& dict_level, const CDictRecord* pRec, int i)
{
    if (pInfo->m_nCurPage == m_uPage) {
        CString csText = _T("Level: ") + dict_level.GetLabel();
        pDC->TextOut(0, m_iYPos, csText);
        csText = _T("Record: ") + pRec->GetLabel();
        if (i != 0) {
            csText += _T(" -- cont.");
        }
        pDC->TextOut(m_iPageWidth / 2, m_iYPos, csText);
        m_iYPos -= m_iHeight;

        pDC->MoveTo(0, m_iYPos);
        pDC->LineTo(m_iPageWidth, m_iYPos);
        m_iYPos -= m_iLineHeight;

        CSize size = pDC->GetTextExtent(_T("Data"));
        pDC->TextOut(m_iDataType - size.cx, m_iYPos, _T("Data"));
        size = pDC->GetTextExtent(_T("Item"));
        pDC->TextOut(m_iItemType - size.cx, m_iYPos, _T("Item"));
        size = pDC->GetTextExtent(_T("Dec"));
        pDC->TextOut(m_iDecChar - size.cx, m_iYPos, _T("Dec"));
        size = pDC->GetTextExtent(_T("Zero"));
        pDC->TextOut(m_iZeroFill - size.cx, m_iYPos, _T("Zero"));
        m_iYPos -= m_iHeight;

        pDC->TextOut(m_iLabel, m_iYPos, _T("Item Label"));
        pDC->TextOut(m_iName, m_iYPos, _T("Item Name"));
        size = pDC->GetTextExtent(_T("Start"));
        pDC->TextOut(m_iStart - size.cx, m_iYPos, _T("Start"));
        size = pDC->GetTextExtent(_T("Len"));
        pDC->TextOut(m_iLen - size.cx, m_iYPos, _T("Len"));
        size = pDC->GetTextExtent(_T("Type"));
        pDC->TextOut(m_iDataType - size.cx, m_iYPos, _T("Type"));
        pDC->TextOut(m_iItemType - size.cx, m_iYPos, _T("Type"));
        size = pDC->GetTextExtent(_T("Occ"));
        pDC->TextOut(m_iOcc - size.cx, m_iYPos, _T("Occ"));
        size = pDC->GetTextExtent(_T("Dec"));
        pDC->TextOut(m_iDec - size.cx, m_iYPos, _T("Dec"));
        size = pDC->GetTextExtent(_T("Char"));
        pDC->TextOut(m_iDecChar - size.cx, m_iYPos, _T("Char"));
        size = pDC->GetTextExtent(_T("Fill"));
        pDC->TextOut(m_iZeroFill - size.cx, m_iYPos, _T("Fill"));
        m_iYPos -= m_iHeight;

        pDC->MoveTo(0, m_iYPos);
        pDC->LineTo(m_iPageWidth, m_iYPos);
        m_iYPos -= m_iLineHeight;
    }
    else {
        m_iYPos = m_iYPos - 3*m_iHeight - 2*m_iLineHeight;
    }
}


/////////////////////////////////////////////////////////////////////////////
//
//                            CDDGView::OnEndPrintPreview
//
/////////////////////////////////////////////////////////////////////////////

void CDDGView::OnEndPrintPreview(CDC* pDC, CPrintInfo* pInfo, POINT point, CPreviewView* pView)
{
    CDDDoc* pDoc = assert_cast<CDDDoc*>(GetDocument());
    pDoc->SetPrintPreview(FALSE);

    CView::OnEndPrintPreview(pDC, pInfo, point, pView);
}


/////////////////////////////////////////////////////////////////////////////
//
//                            CDDGView::PrintToFile
//
/////////////////////////////////////////////////////////////////////////////

void CDDGView::PrintToFile()
{
    // Get Dictionary Listing File Name
    CDDDoc* pDoc = assert_cast<CDDDoc*>(GetDocument());
    CString csDictListFile = pDoc->GetPathName();
    csDictListFile = csDictListFile.Left(csDictListFile.ReverseFind('.')) + FileExtensions::WithDot::Listing;
    CString csFilter = _T("All Files (*.*)|*.*||");
    CIMSAFileDialog dlg (FALSE, NULL, csDictListFile, OFN_HIDEREADONLY | OFN_OVERWRITEPROMPT, csFilter );
    if (dlg.DoModal() == IDCANCEL) {
        return;
    }
    csDictListFile = dlg.GetPathName();
    CStdioFileUnicode/*CStdioFile*/ ListFile; // 20121231 for unicode
    if(!ListFile.Open(csDictListFile, CFile::modeCreate | CFile::modeWrite | CFile::typeText)) {
        CString csError;
        csError.Format(_T("Cannot open:  %s\n\nFile in use by another program.\n"), (LPCTSTR)csDictListFile);
        AfxMessageBox(csError, MB_ICONEXCLAMATION);
        ListFile.Close();
        return;
    }
    int iLineLen = 110;
    const CDataDict* pDict = pDoc->GetDict();
    CString csLine;
    CIMSAString csText;

    // Dictionary Identification
    csText = pDict->GetLabel();
    ListFile.WriteString(CString(SPACE, (iLineLen - csText.GetLength())/2) + csText + _T("\n"));
    csText = PrepareNameForPrinting(pDict->GetName());
    ListFile.WriteString(CString(SPACE, (iLineLen - csText.GetLength())/2) + csText + _T("\n\n"));
    csText = pDoc->GetPathName();
    ListFile.WriteString(CString(SPACE, (iLineLen - csText.GetLength())/2) + csText + _T("\n"));
    if (pDoc->IsModified()) {
        csText = _T("Last Modified:  (Changes not saved!)");
    }
    else {
        CFileStatus status;
        CFile::GetStatus(pDoc->GetPathName(), status);
        csText.DateTime(status.m_mtime);
        csText = _T("Last Modified:  ") + csText;
    }
    ListFile.WriteString(CString(SPACE, (iLineLen - csText.GetLength())/2) + csText + _T("\n\n"));
    // Write Dictionary Note
    if (!m_bPrintBrief) {
        if (!pDict->GetNote().IsEmpty()) {
            CIMSAString csNote = pDict->GetNote();
            CStringArray acsText;
            csNote.Wrap(acsText,10, iLineLen - 4);
            for (int x = 0 ; x < acsText.GetSize() ; x++) {
                if (x == 0) {
                    acsText[0] = acsText[0].Left(4) + _T("Note: ") + acsText[0].Mid(10);
                }
                ListFile.WriteString(acsText[x] + _T("\n"));
            }
            acsText.RemoveAll();
            ListFile.WriteString(_T("\n"));
        }
    }

    // Questionnaire/Record Heading
    int iName = (iLineLen - 20) / 3;
    int iLabel = iLineLen - 20 - iName;

    ListFile.WriteString(CString(_T('-'),iLineLen) + _T("\n"));
    if (m_bPrintNameFirst) {
        ListFile.WriteString(_T("Level Name ") + CString(SPACE,iName - 11) + _T("Level Label") +
                             CString(SPACE,iLabel - 11) + _T(" Type            Rec\n"));
        ListFile.WriteString(_T("  Record Name ") + CString(SPACE,iName - 12) + _T("Record Label") +
                             CString(SPACE,iLabel - 14) + _T("Value  Req  Max  Len\n"));
    }
    else {
        ListFile.WriteString(_T("Level Label") + CString(SPACE,iLabel - 11) + _T("Level Name ") +
                             CString(SPACE,iName - 11) + _T(" Type            Rec\n"));
        ListFile.WriteString(_T("  Record Label") + CString(SPACE,iLabel - 12) + _T("Record Name ") +
                             CString(SPACE,iName - 14) + _T("Value  Req  Max  Len\n"));
    }
    ListFile.WriteString(CString(_T('-'),iLineLen) + _T("\n"));

    // Questionnaire/Record Listing
    for( size_t level_number = 0; level_number < pDict->GetNumLevels(); ++level_number ) {
        const DictLevel& dict_level = pDict->GetLevel(level_number);
        if (m_bPrintNameFirst) {
            csText = PrepareNameForPrinting(dict_level.GetName());
            if (csText.GetLength() > iName) {
                csLine = csText.Left(iName);
            }
            else {
                csLine = csText + CString(SPACE, iName - csText.GetLength());
            }
            csLine += dict_level.GetLabel();
            if (csLine.GetLength() > iLineLen) {
                csLine = csLine.Left(iLineLen);
            }
        }
        else {
            csText = dict_level.GetLabel();
            if (csText.GetLength() > iLabel) {
                csLine = csText.Left(iLabel);
            }
            else {
                csLine = csText + CString(SPACE, iLabel - csText.GetLength());
            }
            csLine += PrepareNameForPrinting(dict_level.GetName());
        }
        ListFile.WriteString(csLine + _T("\n"));
        // Write Level Note
        if (!m_bPrintBrief) {
            if (!dict_level.GetNote().IsEmpty()) {
                CIMSAString csNote = dict_level.GetNote();
                CStringArray acsText;
                csNote.Wrap(acsText,10, iLineLen - 4);
                for (int x = 0 ; x < acsText.GetSize() ; x++) {
                    if (x == 0) {
                        acsText[0] = acsText[0].Left(4) + _T("Note: ") + acsText[0].Mid(10);
                    }
                    ListFile.WriteString(acsText[x] + _T("\n"));
                }
                acsText.RemoveAll();
            }
        }
        for (int r = 0 ; r < dict_level.GetNumRecords() ; r++) {
            const CDictRecord* pRec = dict_level.GetRecord(r);
            if (m_bPrintNameFirst) {
                csText = PrepareNameForPrinting(pRec->GetName());
                if (csText.GetLength() > iName) {
                    csLine = _T("  ") + csText.Left(iName);
                }
                else {
                    csLine = _T("  ") + csText + CString(SPACE, iName - csText.GetLength());
                }
                csLine += pRec->GetLabel();
            }
            else {
                csText = pRec->GetLabel();
                if (csText.GetLength() > iLabel) {
                    csLine = _T("  ") + csText.Left(iLabel);
                }
                else {
                    csLine = _T("  ") + csText + CString(SPACE, iLabel - csText.GetLength());
                }
                csLine += PrepareNameForPrinting(pRec->GetName());
            }
            if (csLine.GetLength() > iLineLen - 20) {
                csLine = csLine.Left(iLineLen - 20);
            }
            else {
                csLine += CString(SPACE,iLineLen - 20 - csLine.GetLength());
            }
            csText = pRec->GetRecTypeVal();
            csLine += CString(SPACE,5 - csText.GetLength()) + csText;
            if (pRec->GetRequired()) {
                csLine += _T("  Yes");
            }
            else {
                csLine += _T("   No");
            }
            csText.Str(pRec->GetMaxRecs());
            csLine += CString(SPACE,5 - csText.GetLength()) + csText;
            csText.Str(pRec->GetRecLen());
            csLine += CString(SPACE,5 - csText.GetLength()) + csText;
            ListFile.WriteString(csLine + _T("\n"));

            // Write Record Note
            if (!m_bPrintBrief) {
                if (!pRec->GetNote().IsEmpty()) {
                    CIMSAString csNote = pRec->GetNote();
                    CStringArray acsText;
                    csNote.Wrap(acsText,12, iLineLen - 4);
                    for (int x = 0 ; x < acsText.GetSize() ; x++) {
                        if (x == 0) {
                            acsText[0] = acsText[0].Left(6) + _T("Note: ") + acsText[0].Mid(12);
                        }
                        ListFile.WriteString(acsText[x] + _T("\n"));
                    }
                    acsText.RemoveAll();
                }
            }
        }
    }

    iName = (iLineLen - 40) / 3;
    iLabel = iLineLen - 40 - iName;
    CString csId;
    CDictItem rtItem;
    rtItem.GetLabelSet().SetCurrentLanguage(pDict->GetCurrentLanguageIndex());
    rtItem.SetLabel(_T("  (record type)"));
    rtItem.SetContentType(ContentType::Alpha);
    for( size_t level_number = 0; level_number < pDict->GetNumLevels(); ++level_number ) {
        const DictLevel& dict_level = pDict->GetLevel(level_number);
        for (int r = 0 ; r < dict_level.GetNumRecords() ; r++) {
            const CDictRecord* pRec = dict_level.GetRecord(r);
            // Record/Item Heading
            ListFile.WriteString(_T("\f"));
            csText = _T("Level: ") + dict_level.GetLabel();
            csLine = csText + CString(SPACE, iLineLen/2 - csText.GetLength());
            csText = _T("Record: ") + pRec->GetLabel();
            ListFile.WriteString(csLine + csText + _T("\n"));
            ListFile.WriteString(CString(_T('-'),iLineLen) + _T("\n"));
            ListFile.WriteString(CString(SPACE,iLineLen - 40) +
                              _T("           Data Item            Dec Zero\n"));
            if (m_bPrintNameFirst) {
                csLine = _T("Item Name ") + CString(SPACE, iName - 10) + _T("Item Label");
            }
            else {
                csLine = _T("Item Label") + CString(SPACE, iLabel - 10) + _T("Item Name ");
            }
            ListFile.WriteString(csLine + CString(SPACE, iLineLen - csLine.GetLength() - 40) +
                               _T("Start  Len Type Type  Occ  Dec Char Fill\n"));
            ListFile.WriteString(CString(_T('-'),iLineLen) + _T("\n"));

            // Build array of items
            m_aItem.RemoveAll();
            ITEMSORT item;
            // Add record type
            item.level = NONE;
            item.rec   = NONE;
            item.item  = RECTYPE;
            item.start =  pDict->GetRecTypeStart();
            m_aItem.Add(item);

            // Add id items for each level
            for( size_t id_level_number = 0; id_level_number < pDict->GetNumLevels(); ++id_level_number ) {
                item.level = id_level_number;
                item.rec = COMMON;
                const CDictRecord* pIdRec = pDict->GetLevel(id_level_number).GetIdItemsRec();
                for (int i = 0 ; i < pIdRec->GetNumItems() ; i++) {
                    item.item = i;
                    item.start = pIdRec->GetItem(i)->GetStart();
                    m_aItem.Add(item);
                }
            }
            // Add record items for specified record (if not id)
            item.level = level_number;
            item.rec = r;
            for (int i = 0 ; i < pRec->GetNumItems() ; i++) {
                item.item = i;
                item.start = pRec->GetItem(i)->GetStart();
                m_aItem.Add(item);
            }
            // Sort all the items
            SortItems();

            // Print Items
            for (int i = 0 ; i < m_aItem.GetSize() ; i++) {
                const CDictItem* pItem;
                csId = _T("");
                if (m_aItem[i].rec == r) {
                    pItem = pRec->GetItem(m_aItem[i].item);
                }
                else if (m_aItem[i].rec == COMMON) {
                    pItem = pDict->GetLevel(m_aItem[i].level).GetIdItemsRec()->GetItem(m_aItem[i].item);
                    csId = _T("(id) ");
                }
                else {
                    pItem = &rtItem;
                    rtItem.SetStart(pDict->GetRecTypeStart());
                    rtItem.SetLen(pDict->GetRecTypeLen());
                }
                if (m_bPrintNameFirst) {
                    csText = PrepareNameForPrinting(pItem->GetName());
                    if (csText.GetLength() > iName) {
                        csLine = csText.Left(iName);
                    }
                    else {
                        csLine = csText + CString(SPACE,iName - csText.GetLength());
                    }
                    if (!csId.IsEmpty()) {
                        csLine = csLine.Left(iName - 5) + csId;
                    }
                    csLine += pItem->GetLabel();
                }
                else {
                    csText = pItem->GetLabel();
                    if (csText.GetLength() > iLabel) {
                        csLine = csText.Left(iLabel);
                    }
                    else {
                        csLine = csText + CString(SPACE,iLabel - csText.GetLength());
                    }
                    if (!csId.IsEmpty()) {
                        csLine = csLine.Left(iLabel - 5) + csId;
                    }
                    csLine += PrepareNameForPrinting(pItem->GetName());
                }
                if (csLine.GetLength() > iLineLen - 40) {
                    csLine = csLine.Left(iLineLen - 40);
                }
                else {
                    csLine += CString(SPACE,iLineLen - csLine.GetLength() - 40);
                }
                csText.Str(pItem->GetStart(),5);
                csLine += csText;
                csText.Str(pItem->GetLen(),5);
                csLine += csText;
                if (pItem->GetContentType() == ContentType::Alpha) {
                    csLine += _T("   AN");
                }
                else if (pItem->GetContentType() == ContentType::Numeric) {
                    csLine += _T("    N");
                }
                else {
                    csLine += _T("  ") + CString(ToString(pItem->GetContentType())).Left(3).MakeUpper();
                }
                if (pItem->GetItemType() == ItemType::Item) {
                    csLine += _T("    I");
                }
                else {
                    csLine += _T("  Sub");
                }
                csText.Str(pItem->GetOccurs(),5);
                csLine += csText;
                csText.Str(pItem->GetDecimal(),5);
                csLine += csText;
                if (pItem->GetDecChar()) {
                    csLine += _T("  Yes");
                }
                else {
                    csLine += _T("   No");
                }
                if (pItem->GetZeroFill()) {
                    csLine += _T("  Yes\n");
                }
                else {
                    csLine += _T("   No\n");
                }
                ListFile.WriteString(csLine);

                // Write Item Note
                if (!m_bPrintBrief) {
                    if (!pItem->GetNote().IsEmpty()) {
                        CIMSAString csNote = pItem->GetNote();
                        CStringArray acsText;
                        csNote.Wrap(acsText,10, iLineLen - 4);
                        for (int x = 0 ; x < acsText.GetSize() ; x++) {
                            if (x == 0) {
                                acsText[0] = acsText[0].Left(4) + _T("Note: ") + acsText[0].Mid(10);
                            }
                            ListFile.WriteString(acsText[x] + _T("\n"));
                        }
                        acsText.RemoveAll();
                    }
                }

                // Print value sets
                if (!m_bPrintBrief) {
                    for( const auto& dict_value_set : pItem->GetValueSets() ) {
                        if (pItem->GetNumValueSets() > 1 || dict_value_set.GetLabel() != pItem->GetLabel()) {
                            if (m_bPrintNameFirst) {
                                csText = _T("(")+ PrepareNameForPrinting(dict_value_set.GetName()) + _T(")");
                                if (csText.GetLength() > iName) {
                                    csLine = _T("  ") + csText.Left(iName);
                                }
                                else {
                                    csLine = _T("  ") + csText + CString(SPACE,iName - csText.GetLength());
                                }
                                csLine += dict_value_set.GetLabel();
                                if (csLine.GetLength() > iLineLen) {
                                    csLine = csLine.Left(iLineLen);
                                }
                            }
                            else {
                                csText = dict_value_set.GetLabel();
                                if (csText.GetLength() > iLabel) {
                                    csLine = _T("  ") + csText.Left(iLabel);
                                }
                                else {
                                    csLine = _T("  ") + csText + CString(SPACE,iLabel - csText.GetLength());
                                }
                                csLine += _T("(") + PrepareNameForPrinting(dict_value_set.GetName()) + _T(")");
                            }
                            ListFile.WriteString(csLine + _T("\n"));
                        }

                        // Print Value Set Note
                        if (!dict_value_set.GetNote().IsEmpty()) {
                            CIMSAString csNote = dict_value_set.GetNote();
                            CStringArray acsText;
                            csNote.Wrap(acsText,12, iLineLen - 4);
                            for (int x = 0 ; x < acsText.GetSize() ; x++) {
                                if (x == 0) {
                                    acsText[0] = acsText[0].Left(6) + _T("Note: ") + acsText[0].Mid(12);
                                }
                                ListFile.WriteString(acsText[x] + _T("\n"));
                            }
                            acsText.RemoveAll();
                        }

                        // Print Values
                        int iValSize = 0;

                        for( const auto& dict_value : dict_value_set.GetValues() ) {
                            CString csTemp = dict_value.GetRangeString();
                            if (csTemp.GetLength() > iValSize) {
                                iValSize = csTemp.GetLength();
                            }
                        }
                        for( const auto& dict_value : dict_value_set.GetValues() ) {
                            CString csTemp = dict_value.GetRangeString();
                            csTemp += CString(SPACE, iValSize + 2 - csTemp.GetLength());
                            if (dict_value.IsSpecial()) {
                                CString special_code = ( dict_value.GetSpecialValue() == MISSING ) ? _T("    (m) ") :
                                                       ( dict_value.GetSpecialValue() == REFUSED ) ? _T("    (r) ") :
                                                       ( dict_value.GetSpecialValue() == NOTAPPL ) ? _T("   (na) ") :
                                                                                                     _T("    (d) ");
                                csTemp = special_code + csTemp;
                            }
                            else {
                                csTemp = _T("        ") + csTemp;
                            }
                            csLine = csTemp + dict_value.GetLabel();
                            if (m_bPrintNameFirst) {
                                csLine = CString(SPACE,iName) + csTemp + dict_value.GetLabel();
                            }
                            else {
                                csLine = csTemp + dict_value.GetLabel();
                            }
                            if (csLine.GetLength() > iLineLen) {
                                csLine = csLine.Left(iLineLen);
                            }
                            ListFile.WriteString(csLine + _T("\n"));

                            // Print Value Note
                            if (!dict_value.GetNote().IsEmpty()) {
                                CIMSAString csNote = dict_value.GetNote();
                                CStringArray acsText;
                                csNote.Wrap(acsText,14, iLineLen - 4);
                                for (int x = 0 ; x < acsText.GetSize() ; x++) {
                                    if (x == 0) {
                                        acsText[0] = acsText[0].Left(8) + _T("Note: ") + acsText[0].Mid(14);
                                    }
                                    ListFile.WriteString(acsText[x] + _T("\n"));
                                }
                                acsText.RemoveAll();
                            }
                        }
                    }
                }
            }
        }
    }

    // Relation Listing
    iName = iLineLen / 5;

    ListFile.WriteString(_T("\f"));
    ListFile.WriteString(CString(_T('-'),iLineLen) + _T("\n"));
    ListFile.WriteString(_T("Relation Name") + CString(SPACE,iName - 13) +
                         _T("Primary") + CString(SPACE,iName - 7) +
                         _T("Linked by") + CString(SPACE,iName - 9) +
                         _T("Secondary") + CString(SPACE,iName - 9) +
                         _T("Linked by\n"));
    ListFile.WriteString(CString(_T('-'),iLineLen) + _T("\n"));

    for( const auto& dict_relation : pDict->GetRelations() ) {
        csLine = PrepareNameForPrinting(dict_relation.GetName());
        if (csLine.GetLength() > iName) {
            csLine.Left(iName);
        }
        else {
            csLine += CString(SPACE,iName - PrepareNameForPrinting(dict_relation.GetName()).GetLength());
        }
        csLine += dict_relation.GetPrimaryName().c_str();
        if (csLine.GetLength() > 2*iName) {
            csLine.Left(2*iName);
        }
        else {
            csLine += CString(SPACE, iName - dict_relation.GetPrimaryName().length());
        }
        auto dict_relation_part_itr = dict_relation.GetRelationParts().cbegin();
        ASSERT(dict_relation_part_itr != dict_relation.GetRelationParts().cend());
        if (dict_relation_part_itr->IsPrimaryLinkedByOccurrence()) {
            csLine += _T("(occ)") + CString(SPACE,iName - 5);
        }
        else {
            csLine += dict_relation_part_itr->GetPrimaryLink().c_str();
            if (csLine.GetLength() > 3*iName) {
                csLine.Left(3*iName);
            }
            else {
                csLine += CString(SPACE,iName - dict_relation_part_itr->GetPrimaryLink().length());
            }
        }
        csLine += dict_relation_part_itr->GetSecondaryName().c_str();
        if (csLine.GetLength() > 4*iName) {
            csLine.Left(4*iName);
        }
        else {
            csLine += CString(SPACE,iName - dict_relation_part_itr->GetSecondaryName().length());
        }
        if (dict_relation_part_itr->IsSecondaryLinkedByOccurrence()) {
            csLine += _T("(occ)");
        }
        else {
            csLine += dict_relation_part_itr->GetSecondaryLink().c_str();
        }
        ListFile.WriteString(csLine + _T("\n"));
        for (++dict_relation_part_itr; dict_relation_part_itr != dict_relation.GetRelationParts().cend(); ++dict_relation_part_itr) {
            csLine = CString(SPACE,2*iName);
            if (dict_relation_part_itr->IsPrimaryLinkedByOccurrence()) {
                csLine += _T("(occ)") + CString(SPACE,iName - 5);
            }
            else {
                csLine += dict_relation_part_itr->GetPrimaryLink().c_str();
                if (csLine.GetLength() > 3*iName) {
                    csLine.Left(3*iName);
                }
                else {
                    csLine += CString(SPACE,iName - dict_relation_part_itr->GetPrimaryLink().length());
                }
            }
            csLine += dict_relation_part_itr->GetSecondaryName().c_str();
            if (csLine.GetLength() > 4*iName) {
                csLine.Left(4*iName);
            }
            else {
                csLine += CString(SPACE,iName - dict_relation_part_itr->GetSecondaryName().length());
            }
            if (dict_relation_part_itr->IsSecondaryLinkedByOccurrence()) {
                csLine += _T("(occ)");
            }
            else {
                csLine += dict_relation_part_itr->GetSecondaryLink().c_str();
            }
            ListFile.WriteString(csLine + _T("\n"));
        }
    }

    ListFile.Close();

    // Show listing file in Text Viewer               07 Jun 2004 BMD
    ViewFileInTextViewer(csDictListFile);
}


/////////////////////////////////////////////////////////////////////////////
//
//                            CDDGView::SortItems
//
/////////////////////////////////////////////////////////////////////////////

static inline int compare(const void* elem1, const void* elem2)
{
    if (((ITEMSORT*) elem1)->start == ((ITEMSORT*) elem2)->start) {
        return (((ITEMSORT*) elem1)->item - ((ITEMSORT*) elem2)->item);
    }
    else {
        return (((ITEMSORT*) elem1)->start - ((ITEMSORT*) elem2)->start);
    }
}


void CDDGView::SortItems()
{
    qsort(m_aItem.GetData(), m_aItem.GetSize(), sizeof(ITEMSORT), compare);
}


/////////////////////////////////////////////////////////////////////////////
//
//                            CDDGView::OnEditOccurrenceLabels
//
/////////////////////////////////////////////////////////////////////////////

void CDDGView::OnEditOccurrenceLabels()
{
    COccDlg dlg;
    CDDDoc* pDoc = assert_cast<CDDDoc*>(GetDocument());
    dlg.m_pDoc = pDoc;
    CDDTreeCtrl* pTreeCtrl = pDoc->GetDictTreeCtrl();
    CWnd* pWnd = GetFocus();
    CDataDict* pDict = pDoc->GetDict();
    CDictItem* pDictItem = NULL;
    CDictRecord* pRecord = NULL;
    int iLevel = pDoc->GetLevel();
    int iRec = pDoc->GetRec();

    auto add_occurrence_labels = 
        [&](const OccurrenceLabels& occurrence_labels, unsigned occurrences)
        {
            for( unsigned i = 0; i < occurrences; ++i )
                dlg.m_OccGrid.m_Labels.Add(occurrence_labels.GetLabel(i));
        };

    if (GetFocus() == (CWnd*)pTreeCtrl) {
        HTREEITEM hItem = pTreeCtrl->GetSelectedItem();
        DictTreeNode* dict_tree_node = pTreeCtrl->GetTreeNode(hItem);
        if (dict_tree_node->GetRecordIndex() != NONE ) {
            if (dict_tree_node->GetItemIndex() == NONE) {
                pRecord = pDict->GetLevel(dict_tree_node->GetLevelIndex()).GetRecord(dict_tree_node->GetRecordIndex());
                add_occurrence_labels(pRecord->GetOccurrenceLabels(), pRecord->GetMaxRecs());
            }
            else {
                pDictItem = pDict->GetLevel(dict_tree_node->GetLevelIndex()).GetRecord(dict_tree_node->GetRecordIndex())->GetItem(dict_tree_node->GetItemIndex());
                add_occurrence_labels(pDictItem->GetOccurrenceLabels(), pDictItem->GetOccurs());
            }
        }
        else {
            return;
        }
    }
    else {
        if (m_iGrid == DictionaryGrid::Dictionary) {
            return;
        }
        else if (m_iGrid == DictionaryGrid::Level) {
            if (GetFocus() == m_gridLevel.m_CUGGrid) {
                pRecord = pDict->GetLevel(m_gridLevel.GetLevel()).GetRecord(m_gridLevel.GetCurrentRow() - m_gridLevel.GetFirstRow());
                if (pRecord->GetMaxRecs() <= 1)
                    return;
                //when on level grid and editing occ labels for records the iRec is not set in the CDDDoc derive the record occ from the grid
                iRec = m_gridLevel.GetCurrentRow() - m_gridLevel.GetFirstRow();
                add_occurrence_labels(pRecord->GetOccurrenceLabels(), pRecord->GetMaxRecs());
            }
            else {
                return;
            }
        }
        else if (m_iGrid == DictionaryGrid::Record) {
            if (GetFocus() == m_gridRecord.m_CUGGrid)
            {
                pDictItem = pDict->GetLevel(m_gridRecord.GetLevel()).GetRecord(m_gridRecord.GetRecord())->GetItem(m_gridRecord.GetItem(m_gridRecord.GetCurrentRow()));
                if (pDictItem->GetOccurs() <= 1) {
                    return;
                }
                add_occurrence_labels(pDictItem->GetOccurrenceLabels(), pDictItem->GetOccurs());
            }
        }
        else if (m_iGrid == DictionaryGrid::Item) {
            return;
        }
        else {
            ASSERT(FALSE);
        }

    }
    if (pRecord != NULL) {
        dlg.m_sLabel = pRecord->GetLabel();
    }
    else if (pDictItem != NULL) {
        dlg.m_sLabel = pDictItem->GetLabel();
    }

    if (dlg.DoModal() != IDOK) {
        return;
    }

    auto did_occurrence_labels_change =
        [&](const OccurrenceLabels& occurrence_labels, unsigned occurrences)
        {
            for( unsigned i = 0; i < occurrences; ++i )
            {
                if( dlg.m_OccGrid.m_Labels[i] != occurrence_labels.GetLabel(i) )
                    return true;
            }

            return false;
        };

    auto set_occurrence_labels =
        [&](OccurrenceLabels& occurrence_labels, unsigned occurrences)
        {
            for( unsigned i = 0; i < occurrences; ++i )
                occurrence_labels.SetLabel(i, dlg.m_OccGrid.m_Labels[i]);

            pDoc->SetModified(true);
        };


    if (pRecord != NULL) {
        auto& occurrence_labels = pRecord->GetOccurrenceLabels();
        if (did_occurrence_labels_change(occurrence_labels, pRecord->GetMaxRecs())) {
            if (GetFocus() == (CWnd*)pTreeCtrl) {
                pDoc->PushUndo(*pRecord, iLevel, iRec, 0);
            }
            else {
                pDoc->PushUndo(*pRecord, iLevel, iRec);
            }

            set_occurrence_labels(occurrence_labels, pRecord->GetMaxRecs());
        }
    }
    else if (pDictItem != NULL) {
        auto& occurrence_labels = pDictItem->GetOccurrenceLabels();
        if (did_occurrence_labels_change(occurrence_labels, pDictItem->GetOccurs())) {
            int iItem = pDoc->GetItem();
            if (GetFocus() == (CWnd*)pTreeCtrl) {
                pDoc->PushUndo(*pDictItem, iLevel, iRec, iItem, 0);
            }
            else {
                pDoc->PushUndo(*pDictItem, iLevel, iRec, iItem);
            }

            set_occurrence_labels(occurrence_labels, pDictItem->GetOccurs());
        }
        pTreeCtrl->Invalidate();
    }
    pWnd->SetFocus();
}


void CDDGView::OnUpdateEditOccurrenceLabels(CCmdUI* pCmdUI)
{
    CDictChildWnd* pDictChildWnd = assert_cast<CDictChildWnd*>(GetParentFrame());
    if (pDictChildWnd->isQuestionnaireView()) {
        pCmdUI->Enable(FALSE);
        return;
    }
    CDDDoc* pDoc = assert_cast<CDDDoc*>(GetDocument());
    CDDTreeCtrl* pTreeCtrl = pDoc->GetDictTreeCtrl();
    const CDataDict* pDict = pDoc->GetDict();

    if (GetFocus() == (CWnd*)pTreeCtrl) {
        HTREEITEM hItem = pTreeCtrl->GetSelectedItem();
        DictTreeNode* dict_tree_node = pTreeCtrl->GetTreeNode(hItem);
        if (dict_tree_node->GetRecordIndex() != NONE ) {
            if (dict_tree_node->GetItemIndex() == NONE) {
                const CDictRecord* pRecord = pDict->GetLevel(dict_tree_node->GetLevelIndex()).GetRecord(dict_tree_node->GetRecordIndex());
                if (pRecord->GetMaxRecs() > 1) {
                    pCmdUI->Enable(TRUE);
                }
                else {
                    pCmdUI->Enable(FALSE);
                }
            }
            else {
                const CDictItem* pDictItem = pDict->GetLevel(dict_tree_node->GetLevelIndex()).GetRecord(dict_tree_node->GetRecordIndex())->GetItem(dict_tree_node->GetItemIndex());
                if (pDictItem->GetOccurs() > 1) {
                    pCmdUI->Enable(TRUE);
                }
                else {
                    pCmdUI->Enable(FALSE);
                }
            }
        }
        else
            pCmdUI->Enable(FALSE);
    }
    else {
        if (m_iGrid == DictionaryGrid::Dictionary) {
            pCmdUI->Enable(FALSE);
        }
        else if (m_iGrid == DictionaryGrid::Level) {
            // 20140723 there was a problem when a new record, the first record, was being added (after
            // the default one was deleted)
            long lCurrentRow;

            if ( GetFocus() == m_gridLevel.m_CUGGrid && ( lCurrentRow = m_gridLevel.GetCurrentRow() ) >= 0 ) {
                const CDictRecord* pRecord = pDict->GetLevel(m_gridLevel.GetLevel()).GetRecord(lCurrentRow - m_gridLevel.GetFirstRow());
                if (pRecord->GetMaxRecs() <= 1) {
                    pCmdUI->Enable(FALSE);
                }
                else {
                    pCmdUI->Enable(TRUE);
                }
            }
            else {
                pCmdUI->Enable(FALSE);
            }
        }
        else if (m_iGrid == DictionaryGrid::Record) {
            if (GetFocus() == m_gridRecord.m_CUGGrid) {
                const CDictRecord* pRecord = pDict->GetLevel(m_gridRecord.GetLevel()).GetRecord(m_gridRecord.GetRecord());
                int itemNum = m_gridRecord.GetItem(m_gridRecord.GetCurrentRow());
                if (itemNum < 0 || itemNum >= pRecord->GetNumItems()) {
                    pCmdUI->Enable(FALSE);
                }
                else {
                    const CDictItem* pDictItem = pDict->GetLevel(m_gridRecord.GetLevel()).GetRecord(m_gridRecord.GetRecord())->GetItem(itemNum);
                    if (pDictItem->GetOccurs() <= 1 || m_gridRecord.GetNumberRowsSelected() > 1) {
                        pCmdUI->Enable(FALSE);
                    }
                    else {
                        pCmdUI->Enable(TRUE);
                    }
                }
            }
        }
        else if (m_iGrid == DictionaryGrid::Item) {
            pCmdUI->Enable(FALSE);
        }
        else {
            pCmdUI->Enable(FALSE);
        }
    }
}

void CDDGView::OnEditGenValueSet()
{
    if (m_iGrid == DictionaryGrid::Item) {
        m_gridItem.SendMessage(WM_COMMAND, ID_EDIT_GEN_VALUE_SET);
    }
}

void CDDGView::OnUpdateEditGenValueSet(CCmdUI* pCmdUI)
{
    CDictChildWnd* pDictChildWnd = assert_cast<CDictChildWnd*>(GetParentFrame());
    if (pDictChildWnd->isQuestionnaireView()) {
        pCmdUI->Enable(FALSE);
        return;
    }
    BOOL bEnable = FALSE;
    const CDataDict* pDict = assert_cast<CDDDoc*>(GetDocument())->GetDict();
    if (m_iGrid == DictionaryGrid::Item) {
        const CDictItem* pDictItem = pDict->GetLevel(m_gridItem.GetLevel()).GetRecord(m_gridItem.GetRecord())->GetItem(m_gridItem.GetItem());
        bEnable = IsNumeric(*pDictItem);
    }
    pCmdUI->Enable(bEnable);
}

void CDDGView::OnShiftF10()
{
    CWnd* pWnd = GetFocus();
    if (pWnd->IsKindOf(RUNTIME_CLASS(CDDTreeCtrl))) {
        pWnd->PostMessage(WM_COMMAND, ID_SHIFT_F10);
    }
    else {
        GetCurrentGrid().PostMessage(WM_COMMAND, ID_SHIFT_F10);
    }
}


void CDDGView::OnUpdateEditFlattenOccurrences(CCmdUI* pCmdUI) // 20130224 code modified from OnUpdateEditMakeSubitems
{
    CDictChildWnd* pDictChildWnd = assert_cast<CDictChildWnd*>(GetParentFrame());
    if (pDictChildWnd->isQuestionnaireView()) {
        pCmdUI->Enable(FALSE);
        return;
    }
    BOOL bEnable = FALSE;

    if( m_iGrid == DictionaryGrid::Record )
    {
        const CDataDict* pDict = ((CDDDoc*)GetDocument())->GetDict();

        if( GetFocus() == m_gridRecord.m_CUGGrid && m_gridRecord.GetNumberRowsSelected() == 1 )
        {
            const CDictRecord* pRec = pDict->GetLevel(m_gridRecord.GetLevel()).GetRecord(m_gridRecord.GetRecord());

            int selCol;
            long selRow;

            if( m_gridRecord.EnumFirstSelected(&selCol, &selRow) == UG_SUCCESS )
            {
                if( selRow < m_gridRecord.GetNumberRows() ) // 20140405 to fix an occasional "Encountered an improper argument" error
                {
                    CUGCell cell;
                    m_gridRecord.GetCell(selCol, selRow, &cell);

                    int iItem = m_gridRecord.GetItem(selRow);

                    if( iItem != RECTYPE && cell.GetTextColor() != GetSysColor(COLOR_GRAYTEXT) )
                    {
                        if( pRec->GetNumItems() > iItem && pRec->GetItem(iItem)->GetOccurs() > 1 )
                            bEnable = TRUE;
                    }
                }
            }
        }
    }

    pCmdUI->Enable(bEnable);
}


void CDDGView::OnEditFlattenOccurrences() // 20130224
{
    if( m_iGrid == DictionaryGrid::Record )
        m_gridRecord.SendMessage(WM_COMMAND, ID_EDIT_FLATTEN_OCCURRENCES);
}
