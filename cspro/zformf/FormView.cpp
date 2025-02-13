#include "StdAfx.h"
#include "FormView.h"
#include "FieldColorsDlg.h"
#include "FieldFontDlg.h"
#include "FieldPropertiesDlg.h"
#include "FormFilePropertiesDlg.h"
#include "FormPropertiesDlg.h"
#include "GlobalFontDlg.h"
#include "GridPropertiesDlg.h"
#include "LevelPropertiesDlg.h"
#include "TextPropertiesDlg.h"
#include <zUtilO/BCMenu.h>
#include <zUtilO/TreeCtrlHelpers.h>
#include <zAppO/Application.h>
#include <zDictF/DDTrCtl.H>
#include <zDictF/UWM.h>
#include <numeric>


constexpr int INIT_ScrSz = 500;  // the initial screen size, both in x & y
constexpr LONG ROWOFFSET = 30;
constexpr int minBoxSize = 10;


bool CFormScrollView::okToDrawBox()
{
    return assert_cast<CFormChildWnd*>(GetParentFrame())->CanUserDrawBox();
}

bool CFormScrollView::okToSelectItems()
{
    return !okToDrawBox();
}

void CFormScrollView::okToSelectItems(bool b)
{
    assert_cast<CFormChildWnd*>(GetParentFrame())->CanUserDrawBox(b);
}

BoxType CFormScrollView::GetCurBoxDrawType() const
{
    return assert_cast<CFormChildWnd*>(GetParentFrame())->GetCurBoxBtn();
}


// ********************************************************************************
// CFormScrollView

IMPLEMENT_DYNCREATE(CFormScrollView, CScrollView)


// ********************************************************************************

BEGIN_MESSAGE_MAP(CFormScrollView, CScrollView)
    ON_WM_LBUTTONDOWN()
    ON_WM_RBUTTONUP()
    ON_WM_CREATE()
    ON_WM_SIZE()
    ON_WM_KEYDOWN()
    ON_WM_SETCURSOR()
    ON_WM_MOUSEMOVE()
    ON_WM_MOUSEWHEEL()
    ON_WM_ERASEBKGND()
    ON_WM_HSCROLL()
    ON_WM_VSCROLL()
    ON_COMMAND(ID_EDIT_FFPROP, OnEditFFProp)
    ON_COMMAND(ID_EDIT_FIELDPROP, OnEditFieldProp)
    ON_UPDATE_COMMAND_UI(ID_EDIT_FIELDPROP, OnUpdateEditFieldProp)
    ON_COMMAND(ID_EDIT_LEVELPROP, OnEditLevelProp)
    ON_UPDATE_COMMAND_UI(ID_EDIT_LEVELPROP, OnUpdateEditLevelProp)
    ON_COMMAND(ID_EDIT_TEXTPROP, OnEditTextProp)
    ON_UPDATE_COMMAND_UI(ID_EDIT_TEXTPROP, OnUpdateEditTextProp)
    ON_COMMAND(ID_EDIT_FORMPROP, OnEditFormProp)
    ON_COMMAND(ID_EDIT_GRID_AUTOFIT, OnEditGridAutoFit)
    ON_COMMAND(ID_EDIT_GRIDPROP, OnEditGridProp)
    ON_UPDATE_COMMAND_UI(ID_EDIT_GRIDPROP, OnUpdateEditGridProp)
    ON_COMMAND(ID_ADD_TEXT, OnAddText)
    ON_COMMAND(ID_ADD_FORM, OnAddForm)
    ON_COMMAND(ID_GENERATE_FRM, OnGenerateFrm)
    ON_COMMAND(ID_DELETE_ITEM, OnDeleteItem)
    ON_UPDATE_COMMAND_UI(ID_DELETE_ITEM, OnUpdateDeleteItem)
    ON_COMMAND(ID_DELETE_FORM, OnDeleteForm)
    ON_UPDATE_COMMAND_UI(ID_DELETE_FORM, OnUpdateDeleteForm)
    ON_COMMAND_RANGE(ID_LAYOUT_ALIGN_LEFT, ID_LAYOUT_ALIGN_BOTTOM, OnLayoutAlign)
    ON_UPDATE_COMMAND_UI_RANGE(ID_LAYOUT_ALIGN_LEFT, ID_LAYOUT_ALIGN_BOTTOM, OnUpdateLayoutAlign)
    ON_COMMAND_RANGE(ID_LAYOUT_HORIZONTALLY_SPACE_EVENLY, ID_LAYOUT_VERTICALLY_SPACE_EVENLY, OnLayoutSpaceEvenly)
    ON_UPDATE_COMMAND_UI_RANGE(ID_LAYOUT_HORIZONTALLY_SPACE_EVENLY, ID_LAYOUT_VERTICALLY_SPACE_EVENLY, OnUpdateLayoutSpaceEvenly)
    ON_COMMAND(ID_LAYOUT_HORIZONTALLY_CENTER, OnLayoutHorizontallyCenter)
    ON_UPDATE_COMMAND_UI(ID_LAYOUT_HORIZONTALLY_CENTER, OnUpdateLayoutHorizontallyCenter)
    ON_COMMAND(ID_LAYOUT_VERTICALLY_CENTER, OnLayoutVerticallyCenter)
    ON_UPDATE_COMMAND_UI(ID_LAYOUT_VERTICALLY_CENTER, OnUpdateLayoutVerticallyCenter)
    ON_COMMAND(ID_VIEW_LOGIC, OnViewLogic)
    ON_COMMAND(ID_QSF_EDITOR, OnQSFEditor)
    ON_COMMAND(ID_SHOW_BOXTOOLBAR, OnDisplayBoxToolbar)
    ON_COMMAND(ID_GLOBALFONT, OnGlobalfont)
    ON_COMMAND(ID_EDIT_UNDO, OnEditUndo)
    ON_UPDATE_COMMAND_UI(ID_EDIT_UNDO, OnUpdateEditUndo)
    ON_COMMAND(ID_EDIT_REDO, OnEditRedo)
    ON_UPDATE_COMMAND_UI(ID_EDIT_REDO, OnUpdateEditRedo)
    ON_COMMAND(ID_EDIT_COPY, OnEditCopy)
    ON_COMMAND(ID_EDIT_PASTE, OnEditPaste)
    ON_UPDATE_COMMAND_UI(ID_EDIT_PASTE, OnUpdateEditPaste)
    ON_COMMAND(ID_EDIT_CUT, OnEditCut)
    ON_COMMAND(ID_FIELDFONT, OnFieldFont)
    ON_COMMAND(ID_FIELDCOLORS, OnFieldColors)
    ON_COMMAND(ID_SHIFT_F10, OnShiftF10)
    ON_COMMAND(ID_PROPERTIES, OnProperties)
    ON_COMMAND(ID_EDIT_MULTIPLE_FIELDPROP, OnEditMultipleFieldProperties)
    ON_MESSAGE(WM_IMSA_DROPITEM,OnDropItem)
    ON_MESSAGE(UWM::Dictionary::MarkUsedItems, OnMarkDictItems)
END_MESSAGE_MAP()


// ********************************************************************************
// constructor

CFormScrollView::CFormScrollView()
    :   m_iHeight(0),
        m_iBoxHght (0),
        m_iForm(0),
        m_iDropSpacing(0),             // will be reset in OnDropItem()
        m_pRightClickItem(nullptr),
        m_pRightClickRoster(nullptr),
        m_iRosterColIndex(NONE),
        m_bAddRFT(false),
        m_bAddForm(false),
        m_okToDrawTrackerOutlines (true)
{
    m_aTrackerItems.SetSize(0,15);

    m_aCSProGrid.SetSize(0,5);         // 5 shld be gobs for one form

    EnableAutomation();
};

void CFormScrollView::CalcFieldSpacing()
{
    CRect windowRect;

    GetClientRect (windowRect);

    // i'm going to indent the left side of the screen 50 pixels, and
    // want the right edge of the screen 50 pixels away as well, so subtract
    // 100 from the distance to place between a field's text and data entry box
    m_iDropSpacing = windowRect.right - 100;
}


///////////////////////////////////////////////////////////////////////////////

CFormScrollView::~CFormScrollView()
{
    RemoveAllGrids();
    RemoveAllTrackers();
}

/////////////////////////////////////////////////////////////////////////////
// ok, so the form and grid already have the roster info, only the view needs
// to create grids for each of the rosters therein

void CFormScrollView::RecreateGrids(int iFormNum)
{
    CDEForm* pForm = GetDocument()->GetForm(iFormNum);

    if(!pForm){
        return;
    }
    int max = pForm->GetNumItems();

    for (int i = 0; i < max; i++) {
        CDEItemBase* pItem = pForm->GetItem(i);
        if (pItem->GetItemType() == CDEFormBase::Roster) {
            CreateGrid((CDERoster*) pItem);
        }
    }
}


CDEFormFile* CFormScrollView::GetFormFile()
{
    CFormDoc* pFD = assert_cast<CFormDoc*>(GetDocument());
    return ( pFD != nullptr ) ? &pFD->GetFormFile() : nullptr;
}


/////////////////////////////////////////////////////////////////////////////

CDEForm* CFormScrollView::GetCurForm()
{
    if (GetDocument() == nullptr)  // shld never happen
        return nullptr;

    if (m_iForm == NONE)    // shld never happen
        return nullptr;

    return GetDocument()->GetForm(m_iForm);
}

const CDEForm* CFormScrollView::GetCurForm() const
{
    return const_cast<CFormScrollView*>(this)->GetCurForm();
}

/////////////////////////////////////////////////////////////////////////////
// each form can only represent items from the same level; so, i've assigned
// to each form what level it's representing, and so that's what's retrieved
// when i make the pForm->GetLevel() call

CDELevel* CFormScrollView::GetCurLevel()
{
    CDEForm* pForm = GetCurForm();

    if (pForm)
        return GetDocument()->GetLevel(pForm->GetLevel());
    else
        return nullptr;
}

const DragOptions& CFormScrollView::GetDragOptions()
{
    return ((const CFormChildWnd*)GetParentFrame())->GetDragOptions();
}

/////////////////////////////////////////////////////////////////////////////
// called by the framework after the view is first attached to the document,
// but before the view is initially displayed; set scroll limits in OnUpdate()
// if scroll range will change dynamically (which they will, but do later)

void CFormScrollView::OnInitialUpdate()
{
    CScrollView::OnInitialUpdate();

    CRect rcClient;
    CSize szTotal (INIT_ScrSz, INIT_ScrSz);

    CDEForm* pForm = GetCurForm();

    if (pForm != nullptr)
    {
        szTotal.cx = pForm->GetDims().right;
        szTotal.cy = pForm->GetDims().bottom;
    }

    SetScrollSizes(MM_TEXT, szTotal);
}

// ********************************************************************************
/* CreateFont:
    int nHeight
    int nWidth
    int nEscapement
    int nOrientation
    int nWeight

    BYTE bItalic
    BYTE bUnderline
    BYTE cStrikeOut
    BYTE nCharSet,

    BYTE nOutPrecision,
    BYTE nClipPrecision,
    BYTE nQuality,
    BYTE nPitchAndFamily,

    LPCTSTR lpszFacename

*/

void CFormScrollView::SetupFontRelatedOffsets()
{
    CDC* pDC = GetDC( );    // smg: later add font stuff...maybe not here, but someplace!

    TEXTMETRIC tm;
    pDC->GetTextMetrics(&tm);

    m_iHeight = tm.tmHeight;
    m_iBoxHght = (int) (tm.tmHeight * 1.5);    // data entry box doesn't seem big enuf!
}

// ********************************************************************************
// smg do i nd to unhilite/hilite the field??

void CFormScrollView::OnDraw(CDC* pDC)
{
    ResetForm(pDC);
}


// ********************************************************************************

// ********************************************************************************
// the CRect being passed in is the size needed to display all items on the form,
// which could be bigger or smaller than the client rectangle currently being
// displayed; so, based on the form size needed, determine if a scroll bar is needed;
// then, if there was a diff bet the orig scroll size and the new scroll size, pass
// this back to ResetForm so it can tell DrawTrackerOutlines whether or not it has
// to adjust the tracker outlines currently in place

CPoint CFormScrollView::DetermineScrollSize (CRect rectForm)
{
    CRect rectClient;
    GetClientRect (rectClient);

    CPoint cpOldSP = GetDeviceScrollPosition();

    // 20110408 the bottom of the form was always too close to the bottom of the last form item
    // so it was hard to scroll beyond if the user wanted to add a new item

    if( rectForm.right > rectClient.right || ( rectClient.right - rectForm.right < 50 ) )
        rectForm.right += 50;

    if( rectForm.bottom > rectClient.bottom || ( rectClient.bottom - rectForm.bottom < 50 ) )
        rectForm.bottom += 50;

    SetScrollSizes (MM_TEXT, CSize (rectForm.BottomRight()));

    /*if (rectForm.right  > rectClient.right ||
        rectForm.bottom > rectClient.bottom)
    {
        SetScrollSizes (MM_TEXT, CSize (rectForm.BottomRight()));
    }
    else
        SetScrollSizes (MM_TEXT, CSize (rectClient.BottomRight()));*/

    CPoint cpNewSP = GetDeviceScrollPosition();

    return cpOldSP -(cpNewSP);
}

// ********************************************************************************

void CFormScrollView::DrawTrackerOutlines (CDC* pDC, int x, int y)
{
    for (int i = 0; i < GetNumTrackers(); i++)
    {
        CFormTracker tracker = GetTracker(i);
        CRect rt = tracker.GetRect();

        rt.OffsetRect(x,y);
        tracker.SetRect(rt);

        SetTrackerAt(i, tracker);

        tracker.Draw (pDC);
    }
}


///////////////////////////////////////////////////////////////////////////////////////////////////
//
//                          CGridWnd::ResetForm()
//
///////////////////////////////////////////////////////////////////////////////////////////////////

bool CFormScrollView::ResetForm(CDC* pDC)
{
    CFormDoc*       pDoc = GetDocument();

    CDEForm*        pForm = nullptr;
    CDEItemBase*    pRFT = nullptr ;
    CDEField*       pField = nullptr;

    if (pDoc->IsFormLoaded())   {
        if (pDoc->GetFormFile().GetNumForms() == 0)  {
            return  false;      // indicates it couldn't draw the page
        }
    }
    else  {
        return false;
    }

    pForm = GetCurForm();
    if (!pForm)  {
        // shldn't ever occur
        return  false;
    }


    pDC->SetBkColor(pForm->GetBackgroundColor().ToCOLORREF());

//    OnEraseBkgnd(pDC);  removed csc 8/22/00 -- WM_ERASEBKGND called automatically by Win32
    int iSaveDC = pDC->SaveDC();

    // Draw boxes on form
    pForm->GetBoxSet().Draw(pDC);

    int iRFTs = pForm->GetNumItems();
    if (iRFTs == 0)  {
        return false;
    }

    CDEFormFile* pFF = GetFormFile();

    pFF->UpdatePointers();
    pFF->UpdateFlagsNFonts();
    UpdateDims();

//  first, try to draw the text belonging to each field
    CRect rtField, rectForm (0,0,0,0);
    CFormGrid* pGrid = nullptr;
    for (int i = 0 ; i < iRFTs; i++) {
        pRFT = (CDEItemBase*) pForm->GetItem(i);
        switch (pRFT->GetItemType())
        {
            case CDEFormBase::Roster:
            {
                rectForm.UnionRect(rectForm, pRFT->GetDims());

                pGrid = FindGrid((CDERoster*) pRFT);
                if (pGrid==nullptr) break;
                pGrid->SetGridBkColor(pForm->GetBackgroundColor().ToCOLORREF());
                if (pGrid->GetSafeHwnd())  {
                    pGrid->RedrawWindow();
                }
                break;
            }

            case CDEFormBase::Field:
            {
                pField = (CDEField*) pRFT;
                if (!pField->GetDictItem())
                    return 0;

                // Output field box
                rtField = pField->GetDims();
                rtField.right -= 2;
                rtField.bottom -= 2;
                rectForm.UnionRect(rectForm, rtField);
                if (pField->IsMirror() || pField->IsProtected())  {
                    pDC->FillSolidRect(rtField, pForm->GetBackgroundColor().ToCOLORREF());
                }
                else  {
                    // it's a "keyed" field
                }

                DrawField(pField,pDC);
                pDC->SetBkColor(pForm->GetBackgroundColor().ToCOLORREF());

                // Output text
                CDEText& field_text = pField->GetCDEText();
                field_text.DrawMultiline(pDC);
                rectForm.UnionRect(rectForm, field_text.GetDims());

                break;
            }

            case CDEFormBase::Text:
            {
                CDEText* pText = (CDEText*)pRFT;
                pText->DrawMultiline(pDC);
                rectForm.UnionRect(rectForm, pRFT->GetDims());

                break;
            }
        }
    }

    // find out the amt of change in the old vs the new scroll pos

    CPoint scrollAdj = DetermineScrollSize (rectForm);
    if (IsOkToDrawTOs())  {
        DrawTrackerOutlines (pDC, scrollAdj.x, scrollAdj.y);
    }

    pDC->RestoreDC(iSaveDC);

    return true;
}

///////////////////////////////////////////////////////////////////////////////////
// if the user tried to select an item from the tree ctrl that currently not in the
// view, bring it in
//

void CFormScrollView::ScrollAsNecessary (CRect* rect)
{
    CRect client, temp;
    CPoint scroll = GetDeviceScrollPosition();

    GetClientRect (client);

    client.OffsetRect(scroll);

    temp.UnionRect(client, rect);

    if (temp == client)     // no nd to scroll, given rect fits w/in client
    {
        rect->OffsetRect(GetScrollOffset());
        return;
    }

    // otherwise, gotta figure out proper amt to scroll

    CPoint pt (scroll.x, scroll.y);     // default to curr scroll pos (if any)

    // do i need to scroll left, right, or neither?

    if (rect->left < client.left)
    {
        int width = client.left - rect->left;

        pt.x = client.left - width;

        if (pt.x > 20)      // if moving left, give a little bit of space so e'thing not squashed up lkg
            pt.x -= 20;
    }
    else if (rect->right > client.right)
    {
        int width = rect->right - client.right;

        pt.x += width;
    }

    // now, determine if we need to move up, down, or stay put

    if (rect->top < client.top)
    {
        int height = client.top - rect->top;

        pt.y = client.top - height;

        if (pt.y > 20)
            pt.y -= 20; // ditto spacing comment here; give some room above the obj selected
    }
    else if (rect->bottom > client.bottom)
    {
        int height = rect->bottom - client.bottom;

        pt.y += height;
    }
    rect->OffsetRect(-pt.x, -pt.y);

    ScrollToPosition (pt);
}

///////////////////////////////////////////////////////////////////////////////////
// when the user selects an item (node) from the tree ctrl, this func is invoked;
// it coordinates the hilighting box that gets drawn around each Item with the
// selected item in the tree ctrl

void CFormScrollView::TreeSelectionChanged (std::vector<CFormID*> pIDs)
{
    CDC*    pDC = GetDC();

    bool bResetForm = false;

    CFormID* pFirstSelectedID = pIDs.front();

    // We only allow selections from same form and of same type so looking at first selected is ok
    int iTreesFormIndex = pFirstSelectedID->GetFormIndex();
    CFormDoc* pSelectedDocument = pFirstSelectedID->GetFormDoc();

    auto nodeTypePID = pFirstSelectedID->GetItemType();

    // for the if clause, the user is switching pages, either w/in the same document or
    // across documents (i.e., diff form files); either way, pID vals shld be in effect

    if ( (GetDocument() != pSelectedDocument) ||        // switching docs
         (GetDocument() == pSelectedDocument && m_iForm != iTreesFormIndex))    // same doc, diff pages
    {
        RemoveAllGrids();          // trash any grids that were created in this view
        RemoveAllTrackers();

        RecreateGrids (iTreesFormIndex);    // and recreate the ones needed for this view

        SetPointers (iTreesFormIndex);

        // the following two lines are necessary to handle the tree selection stuff ok;
        // if the user is selecting an item from a diff page than what's currently being
        // displayed, and one page has a scroll bar and the other doesn't, the offsets
        // won't be correct; so i need to clear the screen so the system doesn't have
        // any previous knowledge of it
        CDEForm* pForm = GetCurForm();
        if(pForm) {
            pDC->SetBkColor(pForm->GetBackgroundColor().ToCOLORREF());
        }
//      ClearScreen (pDC);

        ScrollToPosition (CPoint(0,0));     // force the system to have no scroll offset

        bResetForm = true;
    }
    else        // otherwise, we're still wkg on the same page
    {
        RemoveAllTrackersAndRefresh();
    }

    // the following code was common to both, so pulled out; the only exception is the
    // call to UnselectRosterRowOrCol(), which will be unnec when switching pages but no biggie

    if (nodeTypePID == eFTT_FIELD ||    // don't want to change ptrs if something else
        nodeTypePID == eFTT_GRID)
    {
        UnselectRosterRowOrCol();   // unselect any cols the user may have chosen in a grid
        UnselectGridRowOrCol();

        CDEForm* pForm = GetCurForm();

        // Union together item rects for all selected items to determine scroll pos
        std::vector<CRect> itemRects;
        std::transform(pIDs.begin(), pIDs.end(), std::back_inserter(itemRects), [this](CFormID* pID) {return ((CDEItemBase*)pID->GetItemPtr())->GetDims(); });

        CRect allRect = std::accumulate(itemRects.begin(), itemRects.end(),
            CRect(),
            [](CRect&a, CRect& b) {
            a.UnionRect(a, b);
            return a;
        });

        ScrollAsNecessary(&allRect);

        // Add a tracker for each selected item - this needs to be done after scrolling to get correct
        // scroll offset
        for (size_t i = 0; i < pIDs.size(); ++i)
        {
            CDEItemBase* pItem = (CDEItemBase*)pIDs[i]->GetItemPtr();

            int itemIndex = pForm->GetItemIndex(pItem);
            CRect itemRect = itemRects[i];

            itemRect.OffsetRect(GetScrollOffset());

            if (pIDs[i]->GetItemType() == eFTT_FIELD)
                AddTracker(itemIndex, itemRect);    // if it's a field, this will add the txt portion too
            else
                AddTracker(itemRect, itemIndex);   // this will just add the roster/grid guy
        }

        DrawTrackerOutlines(pDC);   // draw the tracker boxes

        SetCurItem((CDEItemBase*)pFirstSelectedID->GetItemPtr());

    }
    else if (nodeTypePID == eFTT_GRIDFIELD)
    {
        bool bFirst = true;
        CDEField* pSelectedField = nullptr;
        for (CFormID* pID : pIDs)
        {
            CFormGrid* pGrid = FindGrid((CDERoster*)pID->GetItemPtr());
            if (pGrid->GetRoster()->GetOrientation() == RosterOrientation::Horizontal) {   // csc8/28/00
                // JH 11/5/05 Swap columns for right to left orientation
                int iSelCol = pID->GetColumnIndex();
                if (pGrid->GetRoster()->GetRightToLeft()) {
                    iSelCol = pGrid->GetNumCols() - iSelCol;
                }
                pGrid->EnsureVisible(1, iSelCol);
                pGrid->SelectColumn(iSelCol, !bFirst);
                pSelectedField = pGrid->GetRoster()->GetCol(iSelCol)->GetField(pID->GetRosterField());
            }
            else {
                pGrid->EnsureVisible(pID->GetColumnIndex(), 1);
                pGrid->SelectRow(pID->GetColumnIndex(), !bFirst);
                pSelectedField = pGrid->GetRoster()->GetCol(pID->GetColumnIndex())->GetField(pID->GetRosterField());
            }
            bFirst = false;
        }
        if (pSelectedField) {
            SetCurItem(pSelectedField);
        }
        else {
            SetCurItem((CDEItemBase*)pFirstSelectedID->GetItemPtr());
        }
    }
    else if (nodeTypePID == eFTT_BLOCK)
    {
        RemoveAllTrackers();

        CDEBlock& form_block = assert_cast<CDEBlock&>(*pFirstSelectedID->GetItemPtr());
        CDEForm*  pForm = GetCurForm();

        std::vector<CDEField*> block_fields = form_block.GetParent()->GetBlockFields(form_block);

        if (!block_fields.empty()) {

            if (block_fields.front()->GetParent()->GetItemType() == CDEFormBase::Roster) {
                // Block fields are in a roster
                auto pRoster = static_cast<CDERoster*>(form_block.GetParent());
                auto pGrid = FindGrid(pRoster);
                bool bFirst = true;
                for (const CDEField* pField : block_fields)
                {
                    int iSelCol = pRoster->GetColIndex(pField);
                    if (pRoster->GetOrientation() == RosterOrientation::Horizontal) {
                        if (pGrid->GetRoster()->GetRightToLeft()) {
                            iSelCol = pGrid->GetNumCols() - iSelCol;
                        }
                        pGrid->EnsureVisible(1, iSelCol);
                        pGrid->SelectColumn(iSelCol, !bFirst);
                    }
                    else {
                        pGrid->EnsureVisible(iSelCol, 1);
                        pGrid->SelectRow(iSelCol, !bFirst);
                    }
                    bFirst = false;
                }
            }
            else {
                // Block fields are regular fields on form

                // Union together item rects for all selected items to determine scroll pos
                std::vector<CRect> itemRects;
                std::transform(block_fields.begin(), block_fields.end(),
                               std::back_inserter(itemRects), [](CDEField* pField) { return pField->GetDims(); });

                CRect allRect = std::accumulate(itemRects.begin(), itemRects.end(),
                    CRect(),
                    [](CRect&a, CRect& b) {
                    a.UnionRect(a, b);
                    return a;
                });

                ScrollAsNecessary(&allRect);

                for (int iField = 0; iField < (int)block_fields.size(); ++iField)
                {
                    auto itemRect = itemRects[iField];
                    itemRect.OffsetRect(GetScrollOffset());
                    AddTracker(pForm->GetItemIndex(block_fields[iField]), itemRect);
                }
                DrawTrackerOutlines(pDC);   // draw the tracker boxes
            }
        }

        SetCurItem((CDEItemBase*)pFirstSelectedID->GetItemPtr());
    }

    if (bResetForm) {
//      ResetForm (pDC);
        RedrawWindow();   // bug fixed csc 8/22/00
    }
    ReleaseDC (pDC);
}


// ********************************************************************************
// GetDeviceScrollPosition() rtns positive x,y coords
//
// for the PtInRect() test, i'll need to subtract the scroll position's offset from my item's
// rectangle; for ex, say my item's "left" member is 50; if i've slid the scroll bar right
// 20 pixels, my "left" member is no longer sitting at pixel 50 on the screen, but rather 30;
// so i've got to subtract the scrolled amt from my "left" member; likewise for the y coord
//
// this is essentially the same as pDC->LPtoDP(rect)

CPoint CFormScrollView::GetScrollOffset()
{
    CPoint  cPT = GetDeviceScrollPosition();

    cPT.x = -cPT.x;
    cPT.y = -cPT.y;

    return cPT;
}

//  ********************************************************************************
/*  hoo boy, this is fun; ok, so far, this func is really here to support resizing
    rosters; essentially, when the user clicks on the roster, one or both of it's
    left/right or top/bottom margins could be out of the client rect (for now i've
    been thinking more along the lines of L/R for a wiiiide roster).

    anyway, if the user clicks on the roster and i use the entire size of the roster,
    then several little probs arise, one of them being what if the center of the roster
    is not in the viewport? then the user will not be able to resize the roster based
    on the view they currently have, so they'd have to scroll L/R trolling for it;

    so what i'm doing is making the tracker rect no bigger than the size of the
    client rect (or not negative, in the case of left or top members);
*/

void CFormScrollView::KeepRectInClient(CRect& rectItem)
{
    if (rectItem.left < 0)
        rectItem.left = 0;

    if (rectItem.top < 0)
        rectItem.top = 0;

    CRect rect;

    GetClientRect(&rect);

    if (rectItem.right > rect.right)
        rectItem.right = rect.right;

    if (rectItem.bottom > rect.bottom)
        rectItem.bottom = rect.bottom;

    // 20110509
    if (rectItem.left > rect.right)
        rectItem.left = rect.right;

    if (rectItem.top > rect.bottom)
        rectItem.top = rect.bottom;

}


// ********************************************************************************
// i'd been doing this in OnLButtonDown(), but the func was getting too long what
// w/having to test for an item being selected (done here), and then figuring out
// if the user is trying to drag it or rubber-band a group selection of fields
// (done in OnLButtonDown() and SelectMultipleElements())

bool CFormScrollView::SelectSingleItem(CDC* pDC, CPoint point, UINT nFlags /*=0*/)
{
    CFormDoc*   pDoc = GetDocument();
    CDEForm*    pForm = pDoc->GetCurForm();
    CDEItemBase* pItem = nullptr;
    bool        bItemSel = false;
    bool        bBoxAndTxtSel = false;
    bool        bTxtOnlySel = false;
    int         x, max = pForm->GetNumItems();
    CRect       rectItem, rectTxt;

//  bool bCtrl = (GetKeyState (VK_CONTROL)< 0);     // another way to know if ctrl key hit
    bool bShift = (GetKeyState (VK_SHIFT) < 0);

    CPoint  cPT = GetScrollOffset();    // my func

    for (x = 0;
         x < max && !bItemSel;  // loop thru all items on the form and see if any were selected
         x++)
    {
        pItem = pForm->GetItem(x);

        rectItem = pItem->GetDims();    // get the position of the item, or box if it's a fld
        rectItem.OffsetRect(cPT);      // adjust the box by the scrolled amt

        if (pItem->GetItemType() == CDEFormBase::Field) // if it's a field, check for box vs. txt selected
        {
            if (rectItem.PtInRect(point))  // then user clicked over the box
            {
                bItemSel = true;

                if (bShift) // then user wants accompanying text too
                {
                    bBoxAndTxtSel = true;
                }
            }
            else    // they may have selected the text portion of the field
            {
                rectTxt = assert_cast<CDEField*>(pItem)->GetCDEText().GetDims(); // get the field's text pos
                rectTxt.OffsetRect(cPT);       // offset the text by scroll amt

                if (rectTxt.PtInRect(point))   // check the pos of the text item
                {
                    bItemSel = true;

                    if (bShift)     // the user wants data entry box along with the text

                        bBoxAndTxtSel = true;
                    else
                    {
                        rectItem = rectTxt;     // don't want the rect of the box, just text
                        bTxtOnlySel = true;
                    }
                }
            }
        }
        else    // check e'body else out, i.e., rosters, text, boxes
        {
            // the following is done for rosters, but eventually (?) will have to be done
            // for all types of items

            KeepRectInClient(rectItem);

            if (rectItem.PtInRect(point))

                bItemSel = true;
        }
    }

    if (bItemSel)   // if user selected an RFT, adjust indices
    {
        --x;

        if (!(nFlags & MK_CONTROL)) // only clear prev trackers if user didn't click
                                    // left mouse btn in conjunction w/ctrl key
            RemoveAllTrackersAndRefresh();

        if (bTxtOnlySel)
        {
            bool found = false;
            for (int itrk = 0; itrk < GetNumTrackers(); itrk++)
            {
                CFormTracker track = GetTracker(itrk);
                if (track.GetIndex() == x)
                {
                    found = true;
                    break;
                }
            }
            AddTracker(rectItem, x, true, found);
        }
        else if (bBoxAndTxtSel)
        {
            AddTracker(x, rectItem);
        }
        else
        {
            for (int itrk = 0; itrk < GetNumTrackers(); itrk++)
            {
                CFormTracker track = GetTracker(itrk);
                if (track.GetIndex() == x)
                {
                    track.SetIsBoxSel(true);
                    SetTrackerAt(itrk,track);
                    break;
                }
            }
            AddTracker(rectItem, x);
        }

        DrawTrackerOutlines (pDC);
    }

    // try and see if the user wants to select a box
    else
    {
        for( size_t box_index = 0; box_index < pForm->GetBoxSet().GetNumBoxes(); ++box_index )
        {
            CRect rect = pForm->GetBoxSet().GetBox(box_index).GetDims();

            // adjust the box by the scrolled amt
            rect.OffsetRect(cPT);

            // only want the user to be able to select a box from
            // a small region around the box edge itself;
            if ((rect.left -5  < point.x && point.x < rect.left+5   && rect.top  <= point.y && point.y <= rect.bottom) ||
                (rect.right-5  < point.x && point.x < rect.right+5  && rect.top  <= point.y && point.y <= rect.bottom) ||
                (rect.top-5    < point.y && point.y < rect.top+5    && rect.left <= point.x && point.x <= rect.right)  ||
                (rect.bottom-5 < point.y && point.y < rect.bottom+5 && rect.left <= point.x && point.x <= rect.right) )
            {
                // only clear prev trackers if user didn't click left mouse btn in conjunction w/ctrl key
                if( ( nFlags & MK_CONTROL ) == 0 )
                    RemoveAllTrackersAndRefresh();

                CFormTracker& track = AddTrackerT<CDEBox>(rect, box_index);
                track.Draw(pDC);

                bItemSel = true;
                break;
            }
        }
    }

    return bItemSel;
}

// ********************************************************************************
// smg: maybe later have an upper/lower visible bnd, so i know which RFTs are currently
// visible and don't have to search thru them all as i'm currently doing; seems petty
// now, but if user chooses to have one huge panel for their entire data entry op,
// could be painful

// crBox is the selected tracker rgn; i have already adjusted it by the scrolled amt
// before coming here

bool CFormScrollView::SelectMultipleElements (CRect crBox)
{
    CDEItemBase* pItem;
    CDEForm*    pForm = GetDocument()->GetCurForm();
    CRect       rect;

    int x, max = pForm->GetNumItems();

    bool bShift = (GetKeyState (VK_SHIFT) < 0); // was the shift key pressed during the drag?

    CPoint  cPT = GetScrollOffset();

    for (x = 0;
         x < max;   // loop thru all the Items on this form
         x++)
    {
        pItem = pForm->GetItem(x);
        rect = pItem->GetDims();    // get the position of the item

        if (pItem->GetItemType() == CDEFormBase::Field)  // need to handle separately
        {
            if (bShift)     // if user has shift key dn, select both field box & text
            {
                rect = pItem->GetDims();    // get the position of the item
                rect.OffsetRect(cPT);       // adjust the box by the scrolled amt

                if (crBox.PtInRect(rect.TopLeft()) &&  // DE box has to sit squarely w/in the tracker box
                     crBox.PtInRect(rect.BottomRight()))
                {
                     AddTracker(x, rect);
                }
                else    // the data entry box not in tracker region, test the field's text blk
                {
                    rect = assert_cast<CDEField*>(pItem)->GetTextDims();
                    rect.OffsetRect(cPT);      // adjust the box by the scrolled amt

                    if (crBox.PtInRect(rect.TopLeft()) &&  // text has to sit squarely w/in the tracker box
                        crBox.PtInRect(rect.BottomRight()))
                    {
                        AddTracker(x, rect);
                    }
                }
            }
            else    // test the text and box separately; need to do the text first
            {
                //  test it's txt blk

                rect = assert_cast<CDEField*>(pItem)->GetTextDims();
                rect.OffsetRect(cPT);      // adjust the box by the scrolled amt

                CRect itemRect = pItem->GetDims();  // get the position of the item
                itemRect.OffsetRect(cPT);      // adjust the box by the scrolled amt

                if (crBox.PtInRect(rect.TopLeft()) &&  // text has to sit squarely w/in the tracker box
                    crBox.PtInRect(rect.BottomRight()))
                {
                    if (crBox.PtInRect(itemRect.TopLeft()) &&  // DE box sits squarely w/in the tracker box
                        crBox.PtInRect(itemRect.BottomRight()))
                    {
                        AddTracker(rect,x,true,true);  // select the text box (let it know it's accompanying DE box was selected)
                        AddTracker(itemRect,x);        // and the data entry box
                    }
                    else
                        AddTracker(rect,x,true); // only the text portion was selected
                }
                else if (crBox.PtInRect(itemRect.TopLeft()) && // only the DE box sits squarely w/in the tracker box
                         crBox.PtInRect(itemRect.BottomRight()))
                {
                    AddTracker(itemRect,x);    // only add the data entry box
                }
            }
        }
        else
        {
            rect.OffsetRect(cPT);
            if (crBox.PtInRect(rect.TopLeft()) &&  // rect has to sit squarely w/in the tracker box
                 crBox.PtInRect(rect.BottomRight()))
            {
                AddTracker(rect, x);
            }
        }
    }

    // now see what boxes the user grabbed
    for( size_t box_index = 0; box_index < pForm->GetBoxSet().GetNumBoxes(); ++box_index )
    {
        CRect rect = pForm->GetBoxSet().GetBox(box_index).GetDims();

        // adjust the box by the scrolled amt
        rect.OffsetRect(cPT);

        // rect has to sit squarely w/in the tracker box
        if( crBox.PtInRect(rect.TopLeft()) && crBox.PtInRect(rect.BottomRight()) )
            AddTrackerT<CDEBox>(rect, box_index);
    }

    return ( m_aTrackerItems.GetUpperBound() >= 0 );
}

///////////////////////////////////////////////////////////////////////////////////////////////////
//
//                      CFormScrollView::RepositionGrid()
//
//////////////////////////////////////////////////////////////////////////////////////////////////

void CFormScrollView::RepositionGrid (CDERoster* pRoster)
{
    CRect rc = pRoster->GetDims();
    CFormGrid* pGrid = FindGrid(pRoster);

    // enforce minimum size of (2,2)  ... csc 12/20/00
    // fix this to account for whether or not scroll bars are needed (bug collector ID ????????)
    const CSize& szMin = pGrid->GetMinSize(true);  // exclude scroll bars
    CSize szSB(::GetSystemMetrics(SM_CXVSCROLL), ::GetSystemMetrics(SM_CYHSCROLL));  // scrollbar sizes

    bool bSBVert = (pGrid->GetTotalRect().Height()>rc.Height());
    bool bSBHorz = (pGrid->GetTotalRect().Width()>rc.Width());

    if (rc.Width()<szMin.cx)  {
        rc.right = rc.left + szMin.cx;
    }
    if (rc.Height()<szMin.cy)  {
        rc.bottom = rc.top + szMin.cy;
    }
    if (rc.Width()<szMin.cx + szSB.cx && bSBVert)  {
        rc.right = rc.left + szMin.cx + szSB.cx;
    }
    if (rc.Height()<szMin.cy + szSB.cy && bSBHorz)  {
        rc.bottom = rc.top + szMin.cy + szSB.cy;
    }


//    const CSize& sz = pGrid->GetMinSize();
//    if (rc.Width()<sz.cx)  {
//        rc.right = rc.left + sz.cx;
//    }
//    if (rc.Height()<sz.cy)  {
//        rc.bottom = rc.top + sz.cy;
//    }
    rc.OffsetRect(-GetScrollPosition());
    pGrid->Resize(rc);
}

///////////////////////////////////////////////////////////////////////////////////////////////////
//
//                      CFormScrollView::MoveBox()
//
///////////////////////////////////////////////////////////////////////////////////////////////////

void CFormScrollView::MoveBox(const CFormTracker& track, CPoint cOffset, FormUndoStack& form_undo_stack)
{
    CDEForm* pForm = GetCurForm();
    CDEBox& box = pForm->GetBoxSet().GetBox(track.GetIndex());

    form_undo_stack.PushMoveObj(&box, track.GetIndex(), pForm->GetName());

    CRect& rect = box.GetDims();
    rect.OffsetRect(cOffset);

    GetDocument()->SetModifiedFlag(true);
}

///////////////////////////////////////////////////////////////////////////////////////////////////
//
//                      CFormScrollView::MoveItem()
//
// offset the specified pItem by the value provided; this func is called when the
// user moves the item w/the mouse (as opposed to w/the cursor, which is handled
// in OnKeyDown()); called from MoveOrResizeTrackers() exclusively
///////////////////////////////////////////////////////////////////////////////////////////////////

void CFormScrollView::MoveItem(CFormTracker track, CPoint cOffset, FormUndoStack& form_undo_stack)
{
    CRect rect;
    CDEForm* pForm = GetCurForm();
    int iTrackerIndex = track.GetIndex();
    CDEItemBase* pItem = pForm->GetItem(iTrackerIndex);
    CDEFormBase::eItemType eItem = pItem->GetItemType();

    // this if-else blk is for undo/redo

    if (eItem == CDEFormBase::Text) {
        form_undo_stack.PushMoveObj(pItem, track.GetIndex(), pForm->GetName());
    }
    else {
        if (track.IsFldTxt()) {
            form_undo_stack.PushMoveObj(&assert_cast<CDEField*>(pItem)->GetCDEText(), iTrackerIndex, pForm->GetName());
        }
        else {
            CDEGroup* pParent = pItem->GetParent();
            int iItemIndex = pParent->FindItem(pItem->GetName());
            form_undo_stack.PushMoveObj(pItem, iItemIndex, pItem->GetParent()->GetName());
        }
    }
    if (track.IsFldTxt())  {
        // tracking the text portion of a field

        CDEField* pField = assert_cast<CDEField*>(pItem);
        rect = pField->GetTextDims();
        rect.OffsetRect(cOffset);
        pField->SetTextDims(rect);
    }
    else {
        rect = pItem->GetDims ();
        rect.OffsetRect(cOffset);
        pItem->SetDims (rect);
    }
    if (eItem == CDEFormBase::Roster)  {
        RepositionGrid (STATIC_DOWNCAST(CDERoster,pItem));
    }
    GetDocument()->SetModifiedFlag (true);
}

// ********************************************************************************

CRect CFormScrollView::UnionAllTrackers()
{
    CRect rect(0,0,0,0);

    for (int i = 0; i < GetNumTrackers(); i++)
    {
        rect.UnionRect(rect, GetTracker(i).GetRect());
    }
    return rect;
}


///////////////////////////////////////////////////////////////////////////////////////////////////
//
//                      CForScrollView::MoveOrResizeTrackers()
//
// rtn true if the person moved, or attempted to move, the tracker box
///////////////////////////////////////////////////////////////////////////////////////////////////

bool CFormScrollView::MoveOrResizeTrackers(CPoint point)
{
    CDEForm* pForm = GetCurForm();
    CRect   begRect, endRect;
    CPoint  cPT (0,0);
    CDEItemBase* pItem;

    begRect = UnionAllTrackers();
    CFormTracker track (begRect);
    if (!track.Track (this, point, true))   // return if they're not tracking anything
        return false;       // or if they really did a left-click over the tracker rgn,
                            // rather than attempting to move it

    endRect = track.GetRect();
    CPoint scroll = GetDeviceScrollPosition();
    if (endRect.left+scroll.x < 0 || endRect.top+scroll.y < 0 || endRect.right+scroll.x > 32767 || endRect.bottom+scroll.y>32767)  {
        // the user trying to move the rect out of the view so coords invalid/out of range, bail
        RemoveAllTrackers();
        Invalidate(FALSE);
        UpdateWindow();
        return true;
    }

    CFormDoc* pFD = GetDocument();
    FormUndoStack form_undo_stack;

    endRect.NormalizeRect();    // needed in case the user's rect was inverted

    if (begRect.Height() == endRect.Height() && begRect.Width() == endRect.Width())  {
        // user didn't resize, so Houston, we have a valid move
        cPT.x = endRect.left - begRect.left;    // calc the relative change
        cPT.y = endRect.top  - begRect.top;
        for (int i = 0; i < GetNumTrackers(); i++)  {
            track = GetTracker(i);
            if (track.IsBox()) {
                MoveBox(track, cPT, form_undo_stack);
            }
            else  {
                MoveItem(track, cPT, form_undo_stack);
            }
        }
    }
    else  {
        // user trying to resize
        if (GetNumTrackers() != 1) {
            return true;
        }
        track = GetTracker(0);

        if (track.IsBox()) {
            // user resizing a box
            // don't let user make a box a dot!
            if (endRect.Width() < minBoxSize && endRect.Height() < minBoxSize) {
                return true;
            }

            CDEBox& box = pForm->GetBoxSet().GetBox(track.GetIndex());
            form_undo_stack.PushMoveObj(&box, track.GetIndex(), pForm->GetName());

            CDC* pDC = GetDC();
            OnPrepareDC(pDC);
            pDC->DPtoLP(endRect);
            ReleaseDC(pDC);

            box.SetDims(endRect);
        }

        else {   // resizing an item; only legit (for now) for rosters

            int iItemIndex = track.GetIndex();
            pItem = pForm->GetItem(iItemIndex); // can only have one roster selected at a time
            CDEFormBase::eItemType eItem = pItem->GetItemType();

            if (pItem != nullptr && (eItem == CDEFormBase::Roster || eItem == CDEFormBase::Field)) {
                // wee bit uv a problem lad if item=nullptr

//          csc 9/8/2003 ... this is handled in RepositionGrid better now; see bug collector ID 1175
//              if (endRect.Width() < minBoxSize*5 || endRect.Height() < minBoxSize*5) {      //foobar
//                    // don't let user make a roster a dot or a line
//                  return true;
//                }
                CDEField* pField = DYNAMIC_DOWNCAST(CDEField,pItem);
                CONTENT_TYPE_REFACTOR::LOOK_AT("what to do about non-numeric + non-alpha fields?");
                if(pField && pField->GetDictItem()->GetContentType() == ContentType::Numeric){
                    return true;
                }
                else if(pField && pField->GetDictItem()->GetContentType() == ContentType::Alpha && !pField->UseUnicodeTextBox()){
                    //Check if the field capture type is 1. TextBox + Unicode + Multiline
                    //if it is not Multiline ignore the vertical sizing ??
                    //If it is not a new text box. No Resize
                    return true;
                }

                // if on a roster, set the item index to the value on the group (not the form)
                if( pItem->GetItemType() == CDEFormBase::Roster ) {
                    CDEGroup* pParent = pItem->GetParent();
                    iItemIndex = pParent->FindItem(pItem->GetName());
                }

                form_undo_stack.PushMoveObj(pItem, iItemIndex, pItem->GetParent()->GetName());
                CRect rect = pItem->GetDims();  // original dims

                // nd2determine if user increase/decreased height/width/both
                cPT.x = endRect.Width()  - begRect.Width();     // calc the new width
                cPT.y = endRect.Height() - begRect.Height();    // calc the new height

                // if cPT.x is negative, the user reduced the width
                // if cPT.y is negative, the user reduced the height

                if (cPT.x)  {
                    // user changed the width...but from left or right?
                    if (endRect.left != begRect.left) {
                        // user changed from the left
                        rect.left -= cPT.x;
                    }
                    else {
                        rect.right += cPT.x;
                    }
                }
                if (cPT.y)  {
                    // for alpha fields which use new text boxes do not allow the change in height if it is not multiline
                    bool bChangeHeight = true;
                    CONTENT_TYPE_REFACTOR::LOOK_AT("what to do about non-numeric + non-alpha fields?");
                    if(pField && (pField->GetDictItem()->GetContentType() == ContentType::Alpha && pField->UseUnicodeTextBox() && !pField->AllowMultiLine() )){
                        bChangeHeight = false;
                    }
                    // user changed the height
                    if(bChangeHeight){
                        if (endRect.top != begRect.top)  {
                            // user changed from the top
                            rect.top -= cPT.y;
                            if(rect.Height() < m_iBoxHght){//if the height is less than the min height change it to the min height
                                rect.top -= m_iBoxHght -rect.Height();
                            }
                        }
                        else {
                            rect.bottom += cPT.y;
                            if(rect.Height() < m_iBoxHght){//if the height is less than the min height change it to the min height
                                rect.bottom += m_iBoxHght -rect.Height();
                            }
                        }

                    }
                }
                pItem->SetDims (rect);

                if (eItem == CDEFormBase::Roster){
                    RepositionGrid(STATIC_DOWNCAST(CDERoster, pItem));  // csc 8/22/00
                }
            }
            else {
                return true;

            }
        }
    }
    pFD->PushUndo(std::move(form_undo_stack));   // push the undo obj on to the undo stack!
    UpdateTrackerRectsAndRefresh();
    pForm->UpdateDims();    // adjust the extent of the form, if nec
    pFD->SetModifiedFlag (true);
    Invalidate();
    UpdateWindow();
    SetFocus();     // make sure the focus is w/the view (and not grid, if it's in the trackers)
    return true;
}

// ********************************************************************************
// ok, a little unconventional? if the user has the "Draw Box" btn depressed on the
// toolbar, then this function will create a box from the tracker box being drawn;
// otherwise, if that btn is not depressed, it will try to select whatever elements
// are within the box (though NOTE, as long as the user selects the box portion of
// a data entry field, the text will get dragged along too

bool CFormScrollView::SelectTrackerElements (CPoint point, CFormTracker localTracker)
{
    bool bAnythingSelected = false;

    RemoveAllTrackersAndRefresh();  // clear out extant tracker info before starting...

    if (localTracker.TrackRubberBand(this, point, true) )
    {
        localTracker.m_rect.NormalizeRect();    // needed in case the user's rect was inverted

        if (okToDrawBox())      // is it ok for user to draw a box?
        {
            if (localTracker.m_rect.Width() < minBoxSize && // don't allow a box to be a dot!
                localTracker.m_rect.Height() < minBoxSize)
            {
                bAnythingSelected = false;
            }

            else
            {
                localTracker.m_rect.OffsetRect(GetDeviceScrollPosition());

                auto new_box = std::make_shared<CDEBox>(localTracker.m_rect, GetCurBoxDrawType());

                CDEForm* pForm = GetCurForm();
                pForm->GetBoxSet().AddBox(new_box);

                // since the user can either draw boxes, or select stuff, don't allow
                // the newly drawn box to be selected! (they must click the toolbar's
                // btn to change to selection)

                GetDocument()->SetModifiedFlag(true);

                GetDocument()->PushUndo(FormUndoStack(CFormUndoObj::Action::UR_add, new_box.get(), pForm->GetBoxSet().GetNumBoxes() - 1, pForm->GetName()));

                bAnythingSelected = true;
            }
        }

        else        // user wants to select a group of items
        {
            bAnythingSelected = SelectMultipleElements (localTracker.m_rect);
        }
    }

    return bAnythingSelected;
}


///////////////////////////////////////////////////////////////////////////////////////////////////
//
//                      CFormScrollView::UnselectRosterRowOrCol()
//
// in case the user has a column selected in the roster, deselect it before proceeding;
// however, i don't know which grid may have a col selected; so run thru them all :{
///////////////////////////////////////////////////////////////////////////////////////////////////
void CFormScrollView::UnselectRosterRowOrCol()
{
    CDEForm* pForm = GetCurForm();
    CDEItemBase* pItem;

    if(!pForm)  {
        return;
    }
    for (int i = 0; i < pForm->GetNumItems(); i++)
    {
        pItem = pForm->GetItem(i);

        if (pItem->GetItemType() == CDEFormBase::Roster)  {  // csc 8/24/00
            CFormGrid* pGrid = FindGrid(STATIC_DOWNCAST(CDERoster,pItem));
            ASSERT(pGrid!=nullptr);
            if(pGrid){
                pGrid->Deselect();
            }
        }
    }
}

// ********************************************************************************
// if the user clicked over any trackers, return true (they prob want to move them
// as a group)

bool CFormScrollView::TrackerHitTest (CPoint point)
{
    bool bFound = false;

    for (int i = 0; i < GetNumTrackers() && !bFound; i++)
    {
        if (GetTracker(i).GetTheTrueRect().PtInRect(point) )

            bFound = true;
    }
    return bFound;
}

///////////////////////////////////////////////////////////////////////////////////////////////////
//
//                              CFormScrollView::OnLButtonDown()
//
// several events could have invoked this func
//
// [1] a tracker rgn was previously selected, and the user is trying to move it;
//     exit func when done
// [2] see if the user is trying to select a single item; continue processing
// [3] if no single item was selected, then they could be trying to rubber band a
//     few items together OR draw a box (in which case i convert the tracker box
//     drawn previously in to a box -- not doing now :)
///////////////////////////////////////////////////////////////////////////////////////////////////

void CFormScrollView::OnLButtonDown(UINT nFlags, CPoint point)
{
    bool        bSelect = false;        // did the user select anything
    CPoint      cPT;
    CDC*        pDC = GetDC();      // get a ptr to the view's device context

    OnPrepareDC(pDC);

    UnselectRosterRowOrCol ();  // if any column selected in a grid, unselect

//  [1] user trying to move or resize extant tracker blk(s)

    if (TrackerHitTest (point)) {
        if (MoveOrResizeTrackers(point))    // then the func processed the l-click
        {
            if(nFlags & MK_CONTROL){
                DeselectTracker(point);
                Invalidate();
                UpdateWindow();
            }
            return;     // finished processing OnLButtonDown msg, bail
        }
        // o/w, see if user trying to singly select one of
        // the items in the tracker region;

        bSelect = SelectSingleItem (pDC, point, nFlags);
        if (bSelect)    // if an item was selected, then set it
            SetCurItem ();

        return;
    }

//  [2] attempt to select a previously-dropped item or previously-drawn box

    if (okToSelectItems())  {
        bSelect = SelectSingleItem (pDC, point, nFlags);
        if (bSelect)    // if an item was selected, then set it
            SetCurItem ();
    }

    // if they select a single item by pressing down on it, and want to immed move it,
    // shld i include a call to MoveOrResizeTrackers() here?

    if (bSelect)  {     // see if they want to drag it
        MoveOrResizeTrackers(point);
    }
    else {
        // nothing selected, maybe they're trying to rubber band stuff or draw a box
        CFormTracker localTracker;
        localTracker.m_sizeMin.cx = 1;
        localTracker.m_sizeMin.cy = 1;

//  [3] if no single item was selected above, then they're either trying to rubber band a few items
//  together or draw a box (in which case i convert the tracker box drawn to a box :)

        //  also draws a box if nobody selected!
        bSelect = SelectTrackerElements (point, localTracker);
        SetCurItem();
        if (bSelect)  {
            //Invalidate();
            //SendMessage(WM_PAINT);   csc 8/24/00
            RedrawWindow();
        }
        else  {
            RemoveAllTrackersAndRefresh();  // clear the scr of trackers, nothing selected
        }
    }
}

// ********************************************************************************

bool CFormScrollView::OkToDropSubitems()
{
    return GetDragOptions().UseSubitems();
}

static std::vector<CDEField*> arrFields;

LRESULT CFormScrollView::OnDropItem(WPARAM/* wParam*/, LPARAM lParam)
{
    // if i'm not lkg at a ptr to a dict tree, don't want it!
    arrFields.clear();

    DictTreeNode* dict_tree_node = dynamic_cast<DictTreeNode*>(reinterpret_cast<TreeNode*>(lParam));

    if( dict_tree_node == nullptr )
        return 0;

    // smg: make this test!
    // if the item being dragged is not from the same dict as the one that's curr,
    // complain (for now)

    if( dict_tree_node->GetRelationIndex() != NONE )
    {
        AfxMessageBox(_T("You cannot drag and drop from relations."));
        return 0;
    }

    if( dict_tree_node->GetDictElementType() == DictElementType::ValueSet )
    {
        AfxMessageBox(_T("You cannot drag and drop a value set. Use the item it belongs to."));
        return 0;
    }

    if( dict_tree_node->GetItemOccurs() != NONE )
    {
        // for now, serpro can't handle an individual occurrence drop;
        // i.e., if foo has 5 occs and i drag foo[2], they'll choke;
        // so disallow
        AfxMessageBox(_T("An individual occurrence cannot be dropped. Use must drag the parent."));
        return 0;
    }

    CPoint  dropPoint;
    GetCursorPos (&dropPoint);      // get the location where the user made the drop
    ScreenToClient (&dropPoint);    // make the coords relative to my view

    CPoint  scrollPoint = GetDeviceScrollPosition();
    CRect   vwRect;

    GetClientRect(vwRect);

    m_pgRect = GetCurForm()->GetDims(); // get the size of my page

    // GetClientRect rtns my view w/top left coords = 0,0; diff from GetWindowRect()

    m_pgRect.UnionRect(m_pgRect,vwRect);   // if it's a new pg, coord are (0,0); if small, might be < view sz
    m_pgRect.OffsetRect(scrollPoint);  // adjust it by the scroll position

    CPoint adjustedDP = dropPoint;          // first assign the drop point to the adjusted
    adjustedDP.Offset (scrollPoint);    // then adjusted it by the scroll position

    if (! m_pgRect.PtInRect(adjustedDP) )  // if the drop did not occur w/in the form
    {
        return 0;   // then bail!
    }

    CalcFieldSpacing();
    CFormDoc* pFormDoc = GetDocument();

    //////////////////////////////////////////////

    m_cRules.AnalyzeDrop (dict_tree_node, adjustedDP, GetCurForm(), pFormDoc);

    //////////////////////////////////////////////

    if (m_cRules.IllegalDropOcc())  // result of drop was illegal drop
    {
        AfxMessageBox(m_cRules.GetErrorMsg());
        return 0;
    }

    RemoveAllTrackersAndRefresh();  // sorry, charlie; if the drop's legit, extant trackers get ditched


    //////////////////////////////////////////////

    const CDataDict* pDD = dict_tree_node->GetDDDoc()->GetDict();
    const CDictRecord* pDR = nullptr;

    int iFormNum = GetFormIndex();
    CDEFormBase* pItem = nullptr;

    const DragOptions& drag_options = GetDragOptions();

    eDropType eDropOperation = m_cRules.GetDropOp();

    if (eDropOperation != DropFile)
    {
        if (eDropOperation != DropLevel)
            pDR = pDD->GetLevel(dict_tree_node->GetLevelIndex()).GetRecord(dict_tree_node->GetRecordIndex());
    }

    // if the user is dragging a record, prompt them with the drag opts dialog;
    // if the user hits CANCEL on the dialog, drop still happens, but settings
    // just won't get changed

    if (eDropOperation == DropSingleRec ||
        eDropOperation == DropMultipleRec)
    {
        // if the next stmt is T, then we already know it's going to
        // drop on a roster, and no need to query for opts

        if (m_cRules.GetDropResult() != DropOnRoster)
        {
            // show the drag options dialog and bail out if the user canceled
            if( !assert_cast<CFormChildWnd*>(GetParentFrame())->ShowDragOptionsDlg() )
                return 0;
        }
    }

    if (eDropOperation == DropFile)
    {
        pFormDoc->GetUndoStack().Clear();

        DropADictFile (dict_tree_node);    // make undo/redo compliant?
        SetPointers (0);    // form 0 is active
        iFormNum = 0;
    }

    else if (eDropOperation == DropLevel)
    {
        DropALevel (dict_tree_node);   // isn't implemented, nothing to undo/redo
    }

    else if (eDropOperation == DropSingleRec)
    {
        DropARecordUnroster(dict_tree_node, adjustedDP);  // shld i pt to the 1st item in the rec?
    }

    else if (eDropOperation == DropMultipleRec)
    {
        // the possible (valid) outcomes:
        //
        // [1]  the form was single, user dropped on open real estate, but there
        //      were keyed fields on the form, so no choice but to drop it as a roster
        // [2]  the form is single but user dropped the record on a roster whose
        //      looping is controlled by this record!
        // [3]  the form was multiple, looping being based on the current record,
        //      so drop the record unrostered (i.e., drop the remaining fields)
        // [4]  the form was single and there's no other keyed fields on the form,
        //      so the user has the choice of whether or not to roster--use the drag opt setting

        if (m_cRules.GetDropResult() == DropAsRoster)   // [1]
        {
            DropARecordRoster (dict_tree_node, adjustedDP);
        }

        else if (m_cRules.GetDropResult() == DropOnRoster)  // [2]
        {
            DropARecordOnRoster(dict_tree_node, adjustedDP);
        }

        else if (m_cRules.GetDropResult() == DropUnrostered)    // [3]
        {
            DropARecordUnroster(dict_tree_node, adjustedDP);
        }

        else                    // [4]
        {
            if (drag_options.UseRosters())
                DropARecordRoster (dict_tree_node, adjustedDP);
            else
                DropARecordUnroster(dict_tree_node, adjustedDP);
        }
    }

    else if (eDropOperation == DropSI_FromSR)  // DropSingleItemFromSingleRec)
    {
        DropAnItem (dict_tree_node, adjustedDP, false);
    }

    else if (eDropOperation == DropMI_FromSR)  // DropMultipleItemFromSingleRec)
    {
        if (OkToDropSubitems() &&                                                               // user indicated desire to drop subitems
            dict_tree_node->GetDictElementType() == DictElementType::Item && !dict_tree_node->IsSubitem() &&  // they're dropping the parent item (not subitem)
            !m_cRules.CanUserDropSubitems())                                                    // but we can't drop due to overlapping subitems
        {
            //if the item has no subitem let it go through .
            //SAVY CanUserDropSubitems is returning false when subitems are not present
            //for the dict item . This is wrongly interpreted here as the the item has
            //overlapping subitems . So I am doing an additional check here
            //instead of changing the CanUserDropSubitems(). which could possibly be have other side effects
            //if changed .
            const CDEFormFile* pFF = &pFormDoc->GetFormFile();
            const CDictRecord* pDR = pDD->GetLevel(dict_tree_node->GetLevelIndex()).GetRecord(dict_tree_node->GetRecordIndex());

            int iNumSI = pFF->GetNumDictSubitems(pDR, pDR->GetItem(dict_tree_node->GetItemIndex()));
            if(iNumSI !=0 ) {
                AfxMessageBox (_T("The item you wish to drop has overlapping subitems and therefore \ncan not be dropped--drop individual subitems instead as needed."));
                return 0;
            }

        }
        if (m_cRules.GetDropResult() == DropAsRoster || drag_options.UseRosters())
            DropMultItem_RosterOnItem(dict_tree_node, adjustedDP);

        else if (m_cRules.GetDropResult() == DropAsKeyed || !drag_options.UseRosters())
            DropMultItem_Unroster (dict_tree_node, adjustedDP);
    }

    else if (eDropOperation == DropSI_FromMR)  // DropSingleItemFromMultRec)
    {
        if (m_cRules.GetDropResult() == DropAsKeyed)
        {
            DropAnItem (dict_tree_node, adjustedDP, false);   // then form already looping based on this record
        }

        else if (m_cRules.GetDropResult() == DropOnRoster)  // they dropped it on an existing roster
        {
            DropItemOnRoster(dict_tree_node, adjustedDP);
        }

        else if (m_cRules.GetDropResult() == DropAsRoster || drag_options.UseRosters())
        {
            // for now, can't have mult item in mult rec, so following will work
            m_cRules.SetNumOccs (pDR->GetMaxRecs());

            CDERoster* pRoster = DropMultItem_RosterOnItem(dict_tree_node, adjustedDP);
            // BUT, make the roster loop on the record, not the item!
            pRoster->SetMaxLoopOccs (pDR->GetMaxRecs());
            pRoster->SetRIType (CDEFormBase::Record);
            pRoster->SetTypeName (pDR->GetName());
        }

        else
        {
            DropAnItem (dict_tree_node, adjustedDP, true);
        }
    }

    // this next blk won't be invoked, as not allowing mults w/in mults right now...
    else if (eDropOperation == DropMI_FromMR)   // DropMultItemFromMultRec, i.e., the "parent"
    {
        CDEGroup*    pGroup = GetCurGroup ();
        CDEForm*     pForm = GetCurForm();

        pGroup->SetLoopingVars(pDR);
        pForm->SetRecordRepeatName (pDR->GetName());

        // form repeats
        ASSERT(pForm->isFormMultiple());
        if (m_cRules.GetDropResult() == DropAsRoster || drag_options.UseRosters())
            DropMultItem_RosterOnItem(dict_tree_node, adjustedDP);

        else if (m_cRules.GetDropResult() == DropAsKeyed || !drag_options.UseRosters())
            DropMultItem_Unroster (dict_tree_node, adjustedDP);
    }

    else
    {
        return 0;       // err condition
    }


    // now update the tree to reflect the new document
    pFormDoc->GetFormTreeCtrl()->ReBuildTree (iFormNum, pItem);
    pFormDoc->SetModifiedFlag (true);

    GetCurForm()->UpdateDims(); // adjust the dims of the form based on the guys dropped

    SetFocus ();    // set the focus to *ME*, the view, don't leave it w/the tree ctrl
    Invalidate();
    SendMessage(WM_SIZE);   // force a redraw of the view to recognize our drop!
    MarkDictTree(); // modify the image list for the dict icons

    return 0;
}

//  ====================================================================
//  fix: now i need to add the ptr to the Level/Group hierarchy; but where? it
//  shld be added to the last group that contains elems from the same record
//  type; for now, just add to the first group who's assoc w/the curr form

CDEGroup* CFormScrollView::GetCurGroup()
{
    CDEGroup* pGroup = GetCurLevel()->GetGroupOnForm ( GetFormIndex() );

    if (pGroup == nullptr)
    {
        ASSERT (false); // shld never get here!
    }

    return pGroup;
}

//  ====================================================================
// if the user chooses to generate a new form file, the unique name list
// will get recreated therein

bool CFormScrollView::DropADictFile(DictTreeNode* /*dict_tree_node*/)
{
    // 1st give dialog warning that this request will trash existing file
    const TCHAR* prompt = _T("Dropping the dictionary file will destroy the existing .fmf file\n")
                          _T("and generate a new one--do you wish to continue?");

    if( AfxMessageBox(prompt, MB_YESNOCANCEL | MB_DEFBUTTON2) == IDYES )
    {
        GetDocument()->GenerateFormFile();   // uniq name list will be rebuilt herein
        return true;
    }

    return false;
}

//  ====================================================================

bool CFormScrollView::DropALevel(DictTreeNode* /*dict_tree_node*/)
{
    AfxMessageBox(_T("You cannot drag and drop a level."));
    return false;
}

//  ====================================================================
// this is a helper func to DropARecordUnroster();
// if the user is dropping a record, after i've determined that the item
// itself is not being keyed (and is therefore eligible to be dropped),
// i need to further see if it has any keyed subitems (the purpose of
// this function); if so, the item won't be dropped

bool CFormScrollView::ItemsSubitemBeingKeyed(const CDataDict* pDD, const CDictItem* pDI, CDEFormFile* pFF)
{
    bool bKeyedElsewhere = false;

    const CDictRecord*    pRec;
    const CDictItem*      pItem;
    const CDictItem*      pSubItem = nullptr;

    pDD->LookupName(pDI->GetName(), nullptr, &pRec, &pItem);

    int iItem, max;

    iItem = pDI->GetSonNumber();
    max = pRec->GetNumItems();

    if (++iItem < max)  // iItem, b4 incrementing, is the item; therefore to get next item & check it, increment first
        pSubItem = pRec->GetItem(iItem);

    while (iItem < max && pSubItem->GetItemType() == ItemType::Subitem && !bKeyedElsewhere)
    {
        bKeyedElsewhere = pFF->FindItem (pSubItem->GetName());

        if (++iItem < max)
            pSubItem = pRec->GetItem(iItem);
    }

    return bKeyedElsewhere;
}

// ====================================================================
// if a  SINGLE  record was dropped, then ensure that the group's
// maxLoopOccs is 1, and change the basis on which the group loops from
// record or item to unknown (or, shld i have a 'mixed' status to
// indicate that the group is single because more than one record's
// items are in it??)
//
// if a  MULTIPLE  record, the user chose not to roster it, which means
// the form/group is free of all keyed fields (some display fields may be
// lurking).  in this case change the basis on which the form/group loops
// to the record name dropped, and maxLoopOccs to the dropped record's
// max loops
//
// either way, loop thru the record and spit out each field if possible,
// i.e., if not used elsewhere, drop as keyed, else drop as protected

bool CFormScrollView::DropARecordUnroster(DictTreeNode* dict_tree_node, CPoint dropPoint)
{
    arrFields.clear();
    CDEForm* pForm = GetCurForm();
    m_iDropSpacing = 200;

    int iLevel = dict_tree_node->GetLevelIndex();   // level is 0-based
    int iRec   = dict_tree_node->GetRecordIndex();     // -2 signifies LevelID rec, otherwise begs w/0

    const CDataDict* pDD = GetDocument()->GetSharedDictionary().get();
    const CDictRecord* pDR = pDD->GetLevel(iLevel).GetRecord(iRec);

    CDEGroup*       pParent = GetCurGroup();
    CDEFormFile*    pFF = GetFormFile();

    int iFieldIndex, max = pDR->GetNumItems();
    CRect rect;

    FormUndoStack form_undo_stack;

    for (int i = 0; i < max; i++)
    {
        const CDictItem* pDI = pDR->GetItem(i);
        if( !pDI->AddToTreeFor80() )
            continue;

        if (pDI->GetItemType() == ItemType::Subitem)    // items will handle their subitems as needed
            continue;

        if (pDI->GetOccurs() == 1)
        {
            if (dict_tree_node->GetRecordIndex() != COMMON &&   // don't drop a prev-keyed item as
                pFF->FindItem (pDI->GetName()) )                // display unless this is the common rec
                continue;

            // because i'm dropping the whole record, i don't know for any given item
            // if it has any subitems, or whether it's ok to drop them or not; so test each one!

            bool bSubitemDropOK = pFF->TestSubitemDrop (pDR, pDI, GetDragOptions(), false);
            size_t sz = arrFields.size();

            DropAnItem (pDI, &dropPoint, bSubitemDropOK);  // this func inits field & adds it to the form

            for (size_t i = sz; i < arrFields.size();i++)
            {
                iFieldIndex = pParent->GetUpperBound() - arrFields.size() +  i + 1;  // added item would have been last in group

                form_undo_stack.PushUndoObj(CFormUndoObj::Action::UR_add, arrFields[i], iFieldIndex, GetCurForm()->GetName());//pParent->GetName()
            }

            dropPoint.y += ROWOFFSET;
        }
        else
        {
            if (pFF->FindItem (pDI->GetName()))     // don't drop if already being keyed
                continue;

            if (ItemsSubitemBeingKeyed (pDD, pDI, pFF)) // can't key if a sub being keyed either
                continue;

            CDERoster* pRoster = DropMultItem_RosterOnItem(pDI, pDR, dropPoint);

            int iRstrIndex = pParent->GetUpperBound();  // added item would have been last in group
            form_undo_stack.PushUndoObj(CFormUndoObj::Action::UR_add, pRoster, iRstrIndex, GetCurForm()->GetName());

            dropPoint.y += (long)(pRoster->GetDims().Height() * 1.2);   // 1.2 to give a little extra space
        }
    }
    // only if a keyed field got dropped shld i possibly change the group's props
    // but the item couldn't get dropped unless the form/group had the correct props,
    // so really we'd only need to change the group's properties if we're moving from
    // a singlely-repeating group to a multiply-repeating one...

    if (pDR->GetMaxRecs() > 1)
    {
        pParent->SetLoopingVars (pDR);
        pForm->SetRecordRepeatName (pDR->GetName());
    }

    if (form_undo_stack.GetNumURObjs() > 0) { // if something got dropped, then push the stack
        GetDocument()->PushUndo(std::move(form_undo_stack));
    }

    //SAVY adjust spacing 13 Feb 2008 for Trevors problem of alignment on drop of single record Bug#2037
    AdjustSpacing(arrFields);
    return true;
}

//  ====================================================================

void CFormScrollView::DropARecordRoster(DictTreeNode* dict_tree_node, CPoint dropPoint)
{
    int iLevel = dict_tree_node->GetLevelIndex();   // level is 0-based
    int iRec   = dict_tree_node->GetRecordIndex();  // -2 signifies LevelID rec, otherwise begs w/0
    const CDataDict* pDD = GetDocument()->GetSharedDictionary().get();
    const CDictRecord* pDR = pDD->GetLevel(iLevel).GetRecord(iRec);
    CDEFormFile* pFF = GetFormFile();
    CDEForm* pForm  = GetCurForm();
    CDEGroup* pGroup = GetCurGroup();

    // this will add ALL dict items to the roster, whether keyed elsewhere or not
    CDERoster* pRoster = new CDERoster;
    pRoster->SetRightToLeft(pFF->GetRTLRostersFlag());
    pFF->CreateRoster(pRoster, pDR, GetFormIndex(), dropPoint, GetDragOptions());
    pRoster->SetParent (pGroup);

    int iRstrIndex = pGroup->GetNumItems();
    pGroup->AddItem(pRoster);
    pForm->AddItem(pRoster);

    CreateGrid(pRoster);

    GetDocument()->PushUndo(FormUndoStack(CFormUndoObj::Action::UR_add, pRoster, iRstrIndex, GetCurForm()->GetName()));

    pFF->BuildUniqueNL();   // do i nd 2 do? think not, check
}

//  ====================================================================
// drop a multiply-occurring record on a grid of it's own type;
// add the items to the roster that are not marked in the record as being keyed

void CFormScrollView::DropARecordOnRoster(DictTreeNode* dict_tree_node, CPoint adjustedDP)
{
    CDEFormFile*    pFF = GetFormFile();
    CDERoster*     pRoster = (CDERoster*) GetCurForm()->FindItem (adjustedDP);
    CDEField*       pField;

    int iLevel = dict_tree_node->GetLevelIndex();
    int iRec   = dict_tree_node->GetRecordIndex();

    const CDataDict* pDD = GetDocument()->GetSharedDictionary().get();
    const CDictRecord* pDR = pDD->GetLevel(iLevel).GetRecord(iRec);
    const CDictItem* pDI;

    CDC*            pDC = GetDC();
    int             i, max = pDR->GetNumItems();
    int             iFormIndex = GetFormIndex();
    bool            bDragSI = false;    // ok to drag subitems?

    CIMSAString     sLabel;
    CIMSAString     sDictName = pFF->GetDictionaryName(); // being keyed, drop/add it!

    for (i = 0; i < max; i++)
    {
        pDI = pDR->GetItem(i);

        if( !pDI->AddToTreeFor80() )
            continue;

        if (pDI->GetItemType() == ItemType::Subitem)    // subitem drop will get handled by parent
            continue;

        ASSERT(pDI->GetOccurs() == 1); // for now, we can't have mult w/in mult

        if (pFF->FindItem(pDI->GetName()) )    // only add keyed items to roster
            continue;

        bDragSI = OkToDropSubitems() &&                    // user indicated desire to drop subitems
                  m_cRules.CanUserDropSubitems (pDD, pDI); // and it's ok to do so

        if (bDragSI)
        {
            int max = pDR->GetNumItems();
            const CDictItem* pSI = pDR->GetItem(++i);

            while (i < max && pSI->GetItemType() == ItemType::Subitem)
            {
                /// don't add subitems that have already been added
                if( !pFF->FindItem(pSI->GetName()) )
                {
                    pField = new CDEField(pSI, iFormIndex, sDictName, pDC, GetDragOptions());

                    pFF->CreateRosterField (pField, pRoster);
                }

                if (++i < max)
                    pSI = pDR->GetItem(i);
            }
            --i;    // i've decremented past an item which shld be handled above
        }
        else
        {
            //check if the user can drop item (an item cannot be added from the record if there is a subitem is already present as keyed)
            if (pDI->HasSubitems()) {
                if (m_cRules.CheckItemVsSubItemFromMultiple(pDD, pDI))
                    continue;
            }
            pField = new CDEField(pDI, iFormIndex, sDictName, pDC, GetDragOptions());
            pField->SetDims (0,0,0,0);

            pFF->AddUniqueName(pField->GetName());
            pFF->CreateRosterField (pField, pRoster);
        }
    }

    CGridWnd* pGridWnd = FindGrid(pRoster);
    CRect origRect = pRoster->GetDims();
    pFF->UpdateDictItemPointers(pRoster); // make sure the newly added field pts to its dict item!
    pRoster->UpdateFlagsNFonts(*pFF);
    pGridWnd->BuildGrid();  // rebuilds underlying structure from CDERoster
    pGridWnd->RecalcLayout (CSize (NONE,NONE), false); // recalcs where objs go

    // default to 12 cols and 10 rows min; if bigger than this, use prev dims
    // the func at the end, Resize, *must* get called or crash will occur

    int cols=0, rows=0;
    bool bUseOldDims = false;

    if (pRoster->GetOrientation() == RosterOrientation::Horizontal)
    {
        cols = pRoster->GetNumCols();
        if (cols > MIN_NUM_ROSTER_COLS)
            bUseOldDims = true;
        else
            rows = std::min (pRoster->GetMaxLoopOccs()+1, MIN_NUM_ROSTER_ROWS);    // +1 csc 11/16/00
    }
    else
    {
        rows = pRoster->GetNumCols();
        if (rows > MIN_NUM_ROSTER_COLS)
            bUseOldDims = true;
        else
            cols = std::min (pRoster->GetMaxLoopOccs()+1, MIN_NUM_ROSTER_ROWS);       // +1 csc 11/16/00
    }
    if (bUseOldDims)
    {
        pGridWnd->Resize (origRect);    // change viewport of grid
    }
    else
        pGridWnd->Resize (rows, cols);    // change viewport of grid
}

//  ====================================================================
// drop an item (with or w/o occurs) onto a (record) roster

CDERoster* CFormScrollView::DropItemOnRoster(DictTreeNode* dict_tree_node, CPoint dropPoint)
{
    CDERoster*  pRoster = (CDERoster*) GetCurForm()->FindItem (dropPoint);

    int iLevel = dict_tree_node->GetLevelIndex();
    int iRec   = dict_tree_node->GetRecordIndex();
    int iItem  = dict_tree_node->GetItemIndex();

    const CDataDict* pDD = GetDocument()->GetSharedDictionary().get();
    const CDictRecord* pDR = pDD->GetLevel(iLevel).GetRecord(iRec);
    const CDictItem* pDI = pDR->GetItem(iItem);

    if( !pDI->AddToTreeFor80() )
    {
        ErrorMessage::Display(_T("Adding binary dictionary items to forms is not supported in this release."));
        return nullptr;
    }

    CDEFormFile*    pFF = GetFormFile();
    CDC*            pDC = GetDC();

    int iFormNum = GetFormIndex();
    CIMSAString sDictName = pFF->GetDictionaryName();

    // check w/the drop rules to see if it was possible to drop subitems, and
    // check w/the drag opts dialog to see if the user even wants them!

    bool bDropSubitems = m_cRules.CanUserDropSubitems() && OkToDropSubitems();

    // Add the item that will be it's one and only member

    CDEField* pField;
    bool bAddedField = false;

    if (bDropSubitems)
    {
        const CDictRecord* pRec = pDD->GetLevel(iLevel).GetRecord(iRec);

        int max = pRec->GetNumItems();

        const CDictItem* pSubItem = pRec->GetItem(++iItem);

        while (iItem < max && pSubItem->GetItemType() == ItemType::Subitem)
        {
            pField = new CDEField (pSubItem, iFormNum, sDictName, pDC, GetDragOptions());

            pFF->CreateRosterField (pField, pRoster);
            bAddedField = true;
            if (++iItem < max)
                pSubItem = pRec->GetItem(iItem);
        }
    }
    else
    {
        pField = new CDEField (pDI, iFormNum, sDictName, pDC, GetDragOptions());
        pField->SetDims(0,0,0,0);   // CSC 10/17/00; will cause default layout for fld in grid
        pFF->AddUniqueName(pField->GetName());
        pFF->CreateRosterField (pField, pRoster);   // this adds it to the roster
        bAddedField = true;
    }

    if (bAddedField)
    {
        GetDocument()->PushUndo(FormUndoStack(CFormUndoObj::Action::UR_modify, pRoster, pRoster->GetParent()->GetItemIndex(pRoster), pRoster->GetParent()->GetName()));
    }

    CGridWnd* pGridWnd = FindGrid(pRoster);
    CRect origRect = pRoster->GetDims();
    pFF->UpdateDictItemPointers(pRoster); // make sure the newly added field pts to its dict item!
    pRoster->UpdateFlagsNFonts(*pFF);
    pGridWnd->BuildGrid();  // rebuilds underlying structure from CDERoster

    // csc 9/9/2003 ... changed false to true in 2nd param below, to force redrawing of grid after drop
    //bug collector ID #1178
//    pGridWnd->RecalcLayout (CSize(NONE,NONE), false); // recalcs where objs go
    pGridWnd->RecalcLayout (CSize(NONE,NONE), true); // recalcs where objs go

    // default to 12 cols and 10 rows min; if bigger than this, use prev dims

    int cols=0, rows=0;
    bool bUseOldDims = false;

    if (pRoster->GetOrientation() == RosterOrientation::Horizontal)
    {
        cols = pRoster->GetNumCols();

        if (cols > MIN_NUM_ROSTER_COLS)
            bUseOldDims = true;
        else
            rows = std::min (pRoster->GetMaxLoopOccs()+1, MIN_NUM_ROSTER_ROWS);      // +1 csc 11/16/00
    }
    else
    {
        rows = pRoster->GetNumCols();

        if (rows > MIN_NUM_ROSTER_COLS)
            bUseOldDims = true;
        else
            cols = std::min (pRoster->GetMaxLoopOccs()+1, MIN_NUM_ROSTER_ROWS);       // +1 csc 11/16/00
    }

    if (!bUseOldDims)
    {
        pGridWnd->Resize (rows, cols);    // change viewport of grid
    }

    return  pRoster;
}

// ***************************************************************************

void CFormScrollView::DropMultItem_Unroster (DictTreeNode* dict_tree_node, CPoint dropPoint)
{
    int iLevel = dict_tree_node->GetLevelIndex();   // level is 0-based
    int iRec   = dict_tree_node->GetRecordIndex();  // -2 signifies LevelID rec, otherwise begs w/0
    int iItem  = dict_tree_node->GetItemIndex();

    const CDataDict* pDD = GetDocument()->GetSharedDictionary().get();
    const CDictRecord* pDR = pDD->GetLevel(iLevel).GetRecord(iRec);
    const CDictItem* pDI = pDR->GetItem(iItem);

    CDEFormFile* pFF = GetFormFile();

    // don't drop a prev-keyed item as display unless this is the common rec
    if( dict_tree_node->GetRecordIndex() != COMMON && pFF->FindItem(pDI->GetName()) )
        return;

    // don't need to test for subitem drop, it was done in the drop rules
    DropAnItem(pDI, &dropPoint, OkToDropSubitems());    // this func inits field & adds it to the form

    dropPoint.y += ROWOFFSET;
}

///////////////////////////////////////////////////////////////////////////////////////////////////

CDERoster* CFormScrollView::DropMultItem_RosterOnItem(DictTreeNode* dict_tree_node, CPoint dropPoint)
{
    int iLevel = dict_tree_node->GetLevelIndex();   // level is 0-based
    int iRec   = dict_tree_node->GetRecordIndex();  // -2 signifies LevelID rec, otherwise begs w/0
    int iItem  = dict_tree_node->GetItemIndex();    // item is 0-based

    const CDataDict* pDD = GetDocument()->GetSharedDictionary().get();
    const CDictRecord* pDR = pDD->GetLevel(iLevel).GetRecord(iRec);
    const CDictItem* pDI = pDR->GetItem(iItem);
    CDEFormFile* pFF = GetFormFile();
    CDEGroup* pParent = GetCurGroup();

    CDERoster* pRoster = new CDERoster(pDR);
    pRoster->SetRightToLeft(pFF->GetRTLRostersFlag());
    pRoster->SetParent(pParent);

    // unique roster name will get added to uniq name list below in CreateRoster()
    CDC* pDC = GetDC();
    bool bDropSubitems = OkToDropSubitems() && pFF->GetNumDictSubitems (pDR, pDI);

    pFF->CreateRoster(pRoster, pDI, GetFormIndex(), dropPoint, pDC, GetDragOptions(),
                      m_cRules.GetNumOccs(), bDropSubitems, pDD);

    GetCurForm()->AddItem(pRoster);    // add the roster to the form

    int iRstrIndex = pParent->GetNumItems();
    pParent->AddItem(pRoster);   // add the roster to the group

    CreateGrid(pRoster);

    // DO set up the redo/undo stack here, as this func is not called as a utility
    // func from other drop functions
    GetDocument()->PushUndo(FormUndoStack(CFormUndoObj::Action::UR_add, pRoster, iRstrIndex, GetCurForm()->GetName()));

    return pRoster;
}

///////////////////////////////////////////////////////////////////////////////////////////////////

CDERoster* CFormScrollView::DropMultItem_RosterOnItem(const CDictItem* pDI, const CDictRecord* pDR, CPoint dropPoint)
{
    const CDataDict* pDD = GetDocument()->GetSharedDictionary().get();
    CDEFormFile* pFF = GetFormFile();

    CDERoster* pRoster = new CDERoster(pDR);
    pRoster->SetRightToLeft(pFF->GetRTLRostersFlag());
    pRoster->SetParent (GetCurGroup());

    // unique roster name will get added to uniq name list below in CreateRoster()
    CDC* pDC = GetDC();
    bool bDropSubitems = OkToDropSubitems() &&                   // user indicated desire to drop subitems
                         m_cRules.CanUserDropSubitems (pDD, pDI); // and it's ok to do so

    pFF->CreateRoster(pRoster, pDI, GetFormIndex(), dropPoint, pDC, GetDragOptions(),
                      pDI->GetOccurs(), bDropSubitems, pDD);

    GetCurForm()->AddItem(pRoster);    // add the roster to the form
    GetCurGroup()->AddItem(pRoster);   // add the roster to the group

    // DON'T set up undo/redo stack here; return roster and do on call
    CreateGrid(pRoster);
    return pRoster;
}


// ====================================================================
// this func is invoked when only a single item is being dropped;
// it is not used as a utility routine, as the other DropAnItem()
// func is; therefore, ok to do undo/redo setup here, but DON'T
// do it in the other DropAnItem() func

CDEField* CFormScrollView::DropAnItem(DictTreeNode* dict_tree_node, CPoint dropPoint, bool bChangeLoopVals)
{
    int iLevel = dict_tree_node->GetLevelIndex();   // level is 0-based
    int iRec   = dict_tree_node->GetRecordIndex();  // -2 signifies LevelID rec, otherwise begs w/0
    int iItem  = dict_tree_node->GetItemIndex();    // item is 0-based
    ASSERT(dict_tree_node->GetValueSetIndex() == NONE);

    const CDataDict* pDD = GetDocument()->GetSharedDictionary().get();
    const CDictRecord* pDR = pDD->GetLevel(iLevel).GetRecord(iRec);
    const CDictItem* pDI = pDR->GetItem(iItem);

    if( !pDI->AddToTreeFor80() )
    {
        ErrorMessage::Display(_T("Adding binary dictionary items to forms is not supported in this release."));
        return nullptr;
    }

    CDEGroup* pParent = GetCurGroup();
    BOOL bDelete = FALSE;
    FormUndoStack form_undo_stack;

    if (bChangeLoopVals){
        bDelete = TRUE;
        form_undo_stack.PushUndoObj(CFormUndoObj::Action::UR_delete, &GetDocument()->GetFormFile(), NONE, _T(""));

        pParent->SetLoopingVars (pDR);
        GetCurForm()->SetRecordRepeatName (pDR->GetName());
    }

    m_iDropSpacing = 200;   // i don't want the field's text & box too far apart

    //This is a fix for dropping an item which has subtitems and the drag options
    //have use subitems option. When the user drops the item it  drops the subitem
    //on to the form
    // GSF and savy decided that it should not consider the options for dragging
    // these are only to be used for the default generate / when a record is dropped
    // not when an item is dropped

/* Savy commented this stuff  bool bOkToUseSIs = OkToDropSubitems() &&    // the user *wants* to use subitems
                        m_cRules.CanUserDropSubitems(); // but the rules say no, can't */
    bool bOkToUseSIs = false ; //ignore the OkToDropSubitems
    bOkToUseSIs = bOkToUseSIs && m_cRules.CanUserDropSubitems();

    CDEField* pField = DropAnItem (pDI, &dropPoint, bOkToUseSIs, m_cRules.GetDropResult());

    if(!bDelete) {
        form_undo_stack.PushUndoObj(CFormUndoObj::Action::UR_add, pField, pParent->GetUpperBound(), pParent->GetName());
    }

    GetDocument()->PushUndo(std::move(form_undo_stack));

    return pField;
}

//  ====================================================================

const CDictRecord* CFormScrollView::GetDictIDRecord(const CDictItem* pDI, int* iItem)
{
    const CDataDict* pDD = GetDocument()->GetSharedDictionary().get();

    const DictLevel* pDL;
    const CDictRecord* pDR;
    const DictValueSet* pDV;

    bool bFound = pDD->LookupName(pDI->GetName(), &pDL, &pDR, &pDI, &pDV);

    if (bFound)
    {
        int temp;
        pDD->LookupName(pDI->GetName(), &temp, &temp, iItem, &temp);   // iDL, iDR, iItem, iDVSet
        return pDR;
    }
    else
    {
        return nullptr;    // we're in trouble!
    }
}

//  ====================================================================
//  in an effort to make this function generic so that it can be used when
//
//  [1] dropping a solo item or
//  [2] dropping items from a record
//
//  i've put the code here; notice it differs in that it wants a ptr to a
//  dictionary item, not a ptr to my tree ctrl node stuff (as in the DropAnItem
//  func above)

CDEField* CFormScrollView::DropAnItem(const CDictItem* pDI, CPoint* dropPoint, bool bDropSubitems, eDropResult eDropOp/* = DropResultUndef*/)
{
    CDEFormFile* pFF    = GetFormFile();
    CDEForm*     pForm  = GetCurForm();
    CDEField*    pField = nullptr;

    CString csName;
    CString csLabel;
    CString sDictName = pFF->GetDictionaryName();

    int iItem = 0,  // iItem is 0-based
        max = 0,
        iItemLen;

    const CDictRecord* pDR = GetDictIDRecord(pDI, &iItem);

    CRect box ( (*dropPoint).x, (*dropPoint).y, (*dropPoint).x, (*dropPoint).y);

    // as a final (?) test, see if the item being dropped has any subitems

    bDropSubitems = bDropSubitems && pFF->GetNumDictSubitems (pDR, pDI);

    if (bDropSubitems)
    {
        if (pDR == nullptr)
        {
            ASSERT (false);
        }
        else
            max = pDR->GetNumItems();
    }

    const CDictItem* pSubItem = nullptr;

    int iFormNum = GetFormIndex();
    CDC* pDC = GetDC();
    bool bDone = false;

    while (!bDone)
    {
        if (bDropSubitems)
        {
            if (++iItem < max)
            {
                pSubItem = pDR->GetItem(iItem);

                iItemLen = pSubItem->GetLen();

                if (pSubItem->GetItemType() == ItemType::Item)    // all done w/subitems :)
                {
                    bDone = true;
                    continue;
                }

                pField = new CDEField (pSubItem, iFormNum, sDictName, pDC,
                                       GetDragOptions(), GetDropSpacing(), *dropPoint, (*dropPoint).y);

                csName = pSubItem->GetName();
                csLabel = pSubItem->GetLabel();

                (*dropPoint).y += ROWOFFSET;
            }
            else
            {
                bDone = true;
                continue;
            }
        }
        else
        {
            if (pFF->AreAnySubitemsBeingKeyed (pDR, pDI) ) // can't drop this guy if so
                return nullptr;


            pField = new CDEField (pDI, iFormNum, sDictName, pDC, GetDragOptions(),
                                   GetDropSpacing(), *dropPoint, (*dropPoint).y);

            iItemLen = pDI->GetLen();
            csName  = pDI->GetName();
            csLabel = pDI->GetLabel();
            bDone = true;
        }

        if (eDropOp == DropAsMirror)
        {
            pField->IsMirror(true);
            pField->SetupCaptureInfo(*pDI, GetDragOptions());
            pField->SetName(pFF->CreateUniqueName(pField->GetItemName()));  // can't use dict name if not a keyed field
        }
        else
        {
            if (pFF->FindItem(csName))   // 1st time add or extant?
            {
                pField->IsMirror (true);

                // can't use dict name if not a keyed field

                pField->SetName (pFF->CreateUniqueName (pField->GetItemName()));
            }
            else
            {
                pFF->AddUniqueName(pField->GetName());
            }
        }
        pField->SetParent (GetCurGroup());

        // Add field to form and increase form size if necessary

        pForm->AddItem(pField);
        GetCurGroup()->AddItem(pField);

        m_pgRect.UnionRect(m_pgRect,box);

        pForm->SetDims (m_pgRect);

        box.left = box.right = (*dropPoint).x;
        box.top += ROWOFFSET;
        box.bottom = box.top;
        arrFields.emplace_back(pField);
    }

    // if i dropped subitems above, for the last subitem dropped, i don't want to
    // increment the row count, as the calling func will do this (as nec); so backup!

    if (bDropSubitems)

        (*dropPoint).y -= ROWOFFSET;

    // i'm not using this rtn val anyplace (well, one place wrt occs, but the code's not
    // active due to serpro not dealing w/them), so doesn't really matter right now that,
    // for subitems, this will only rtn the *last* subitem allocated
    return  pField;
}

// ********************************************************************************

int CFormScrollView::OnCreate(LPCREATESTRUCT lpCreateStruct)
{
    if (CScrollView::OnCreate(lpCreateStruct) == -1)    // mfc-generated
        return -1;

    m_dropTarget.Register(this); // registers the view w/OLE DLLs as a valid drop target

    return 0;
}

// ********************************************************************************
// force the redraw of the view

void CFormScrollView::OnSize(UINT nType, int cx, int cy)
{
    CScrollView::OnSize(nType, cx, cy);

    // TODO: Add your message handler code here

}

// ********************************************************************************

void CFormScrollView::OnKeyDown(UINT nChar, UINT nRepCnt, UINT nFlags)
{
    if (IsAnyGridTrackerSelected())  {
        int iNumGrids = GetNumGrids();
        CFormGrid* pGrid = nullptr;
        for (int i = 0; i<iNumGrids; i++)  {
            pGrid = GetGrid(i);
            if (pGrid->IsTrackerActive())  {
                GetGrid(i)->SendMessage(WM_KEYDOWN,nChar,MAKEWORD(nRepCnt,nFlags));
                return;
            }
        }
        ASSERT(FALSE);
    }

    int moveAmount = 1; // 20110509 create a faster way to move elements by holding down the control key

    if( GetKeyState(VK_CONTROL) < 0 )
        moveAmount = 10;

    switch (nChar)  {
    case VK_DELETE:
        DeleteActiveItem ();    // VK_DELETE = virtual key code for DEL key
        break;
    case VK_DOWN:
        MoveBlock(VK_DOWN,moveAmount);
        break;                  // = down arrow key
    case VK_UP:
        MoveBlock(VK_UP,moveAmount);
        break;
    case VK_RIGHT:
        MoveBlock(VK_RIGHT,moveAmount);
        break;
    case VK_LEFT:
        MoveBlock(VK_LEFT,moveAmount);
        break;
    case VK_HOME:
        OnVScroll(SB_TOP,0,nullptr);
        OnHScroll(SB_LEFT,0,nullptr);
        break;
    case VK_END:
        OnVScroll(SB_BOTTOM,0,nullptr);
        OnHScroll(SB_RIGHT,0,nullptr);
        break;
    case VK_PRIOR:
        OnVScroll(SB_PAGEUP,0,nullptr);
        break;
    case VK_NEXT:
        OnVScroll(SB_PAGEDOWN,0,nullptr);
        break;
    case VK_TAB:                                            // BMD 25 Oct 2001
        AfxGetMainWnd()->SendMessage(WM_IMSA_SETFOCUS);
        break;
    default:
        break;
    }
//  CScrollView::OnKeyDown(nChar, nRepCnt, nFlags); // mfc-gen, don't need
}

// ********************************************************************************
// i only need to check when the user's trying to move the tracker region(s) left
// or up; in either case if it makes one of the tracker regions left or top coord
// go out of bounds, then can't do!

// 20110509 added the move amount bit

bool CFormScrollView::BlockCanBeMoved(UINT nChar, int& moveAmount) const
{
    if( bool up = ( nChar == VK_UP ); up || nChar == VK_LEFT )
    {
        const CDEForm* pForm = GetCurForm();

        for( int i = 0; i < GetNumTrackers(); ++i )
        {
            const CFormTracker& track = GetTracker(i);
            const CRect& rect = track.GetRectFromFormObject(*pForm);

            int position = up ? rect.top :
                                rect.left;

            // then moving up would result in a negative #, bail
            if( position == 0 )
                return false;

            // minimize the move amount if necessary
            if( position < moveAmount )
                moveAmount = position;
        }
    }

    return true;
}

// ********************************************************************************

CRect CFormScrollView::MoveBlock(CPoint cp)
{
    int max = GetNumTrackers();

    CFormTracker track;
    CDEForm*     pForm = GetCurForm();
    CDEItemBase* pItem;
    CDEFormBase::eItemType   eItem;
    CRect       rect, newRect = pForm->GetDims();
    CPoint sp = GetDeviceScrollPosition();
    FormUndoStack form_undo_stack;

    for (int i = 0; i < max; i++)
    {
        track = GetTracker(i);

        if (track.IsBox())
        {
            CDEBox& box = pForm->GetBoxSet().GetBox(track.GetIndex());
            form_undo_stack.PushMoveObj(&box, track.GetIndex(), pForm->GetName());
            box.GetDims().OffsetRect(cp);
        }

        else    // a field, roster, text, etc. was selected
        {
            pItem = pForm->GetItem( track.GetIndex() );
            eItem = pItem->GetItemType();

            // this if-else blk is for undo/redo

            if (eItem == CDEFormBase::Text) {
                form_undo_stack.PushMoveObj(pItem, track.GetIndex(), pForm->GetName());
            }
            else {
                CDEGroup* pParent = pItem->GetParent();
                int iItemIndex = pParent->FindItem(pItem->GetName());
                form_undo_stack.PushMoveObj(pItem, iItemIndex, pItem->GetParent()->GetName());
            }
            if (track.IsFldTxt())
            {
                CDEField* pField = assert_cast<CDEField*>(pItem);
                rect = pField->GetTextDims();
                rect.OffsetRect(cp);
                pField->SetTextDims(rect);
            }
            else
            {
                rect = pItem->GetDims();
                rect.OffsetRect(cp);
                pItem->SetDims (rect);
            }
            if (pItem->GetItemType() == CDEFormBase::Roster)
                RepositionGrid (STATIC_DOWNCAST(CDERoster,pItem));
        }
        rect.OffsetRect(sp);       // offset the item's rect by the scroll pos
        newRect.UnionRect(newRect, rect);
    }

    GetDocument()->PushUndo(std::move(form_undo_stack)); // push the undo obj on to the undo stack!

    return newRect;
}

// ********************************************************************************
// if the user has selected either a single box, a single item, or a blk combining
// the two, this does the move when they use the arrow key(s) to reposition it
//
// union the page size with the new coords

void CFormScrollView::MoveBlock(UINT nChar, int i /*=1*/)
{
    if (GetNumTrackers() == 0)  // nothing to move
        return;

    if (!BlockCanBeMoved(nChar,i)) // move would result in out-of-bounds coord
        return;

    // do it

    CDEForm* pForm = GetCurForm();

    CRect newPage, origPg = pForm->GetDims();

    switch (nChar)
    {
        case VK_UP:     newPage = MoveBlock(CPoint ( 0,-i));   break;
        case VK_DOWN:   newPage = MoveBlock(CPoint ( 0, i));   break;
        case VK_LEFT:   newPage = MoveBlock(CPoint (-i, 0));   break;
        case VK_RIGHT:  newPage = MoveBlock(CPoint ( i, 0));   break;
    }

    if (newPage.right  > origPg.right ||
        newPage.bottom > origPg.bottom)
    {
        newPage.InflateRect (50,50);        // pad it a bit
        GetCurForm()->SetDims (newPage);
        SetScrollSizes(MM_TEXT, CSize (newPage.BottomRight()));
    }
    UpdateTrackerRectsAndRefresh(); // refreshs only where the trackers are
    GetDocument()->SetModifiedFlag(true);
}

// ********************************************************************************
// set the indexes for the current form and item within the view and document

void CFormScrollView::SetPointers (int formNum)
{
    CFormDoc* pDoc = GetDocument();
    m_iForm = formNum;
    pDoc->SetCurFormIndex (formNum);
}

// ********************************************************************************
// what we've decided (for now :}
// if i delete a group from the tree ctrl, it may be a form or logic group; in either
// case, however, i may have to look backward in the tree to see if by deleting the
// selected group, higher level groups (in particular logic groups) may need to be
// removed;
//
// similarly, when i choose to delete a form from the menu bar, i've got to find the
// first group corresponding to this form and delete e'thing under it; in this case
// also i will need to look backward in the tree to see if other groups need to be
// removed (for instance, if a logic group now has no underlying form, no need for
// the logic group!)
//
// worse, however, is that there could be more than one form deleted in the process
// (think of it from the tree ctrl point of view; a form could be nested under another,
// and since we want identical functional regardless of where the operation was
// requested, hence the reason for possible deletion of 1+ forms)
//
// so from this func, just find the first group that's associated w/this form; then
// pass in that ptr to DeleteActiveGroup

void CFormScrollView::DeleteGroupAndForms (CDEGroup* pGroup, bool bRemoveForm /*=true*/)
{
    int iFormNo = pGroup->GetFormNum();

    CDEFormFile* pFF = GetFormFile();
    CDEForm*     pForm = pFF->GetForm( iFormNo );

    for (int i = pGroup->GetNumItems() - 1; i >= 0; --i )
    {
        CDEItemBase* pItem = pGroup->GetItem(i);
        CDEFormBase::eItemType eIT = pItem->GetItemType();
        CString sName = pItem->GetName();

        pForm->RemoveItem(sName);
        pFF->RemoveUniqueName(sName);

        if (eIT == CDEFormBase::Group)
        {
            DeleteGroupAndForms ( (CDEGroup*) pItem, bRemoveForm);  // delete all it's children
        }
    }

    pGroup->RemoveAllItems();

    // this group is associated w/a form, so we nd to kill the form

    pForm->GetBoxSet().RemoveAllBoxes();
    pForm->RemoveAllItems();    // shld just be text items at this point

    if (bRemoveForm)
    {
        pFF->RemoveForm (iFormNo);      // this will update subsequent form field indices
        // pFF->RenumberFormsAndItems();
        // UpdateGroupFormIndices (iFormNo);
    }

    MyRenumberForms();
}


// ***************************************************************************
/*
    this func was changed from that found in v2.0.024 to accommodate "feature" 174;
    in actuality, the feature is more of a bug that was given a quick fix by glenn,
    since i was away;

    in v2.0, if the user deletes all forms w/in a level, so that they get down to
    the last form in a level w/a label saying "EmptyLabel", and then tried to get
    the properties on it, it crashed; so glenn put in a fix to the OnEditFormProps()
    to check for pGroup == nullptr; but then you can't edit the form's props until the
    .fmf's been saved out and reread, when really *this* func needed to be corrected;

    for now, we're considering this an improvement, rather than a bug fix, hence why
    it's only going in here; however, if the fix in v2.0 later causes other probs/crashes,
    use this rewrite
 */

// 1+ CDEForms may bite the dust in this process;
//
// [0] before it all, copy out the stuff so the user can restore it if desired
// [1] first, remove all grid objs in the current form view
// [2] next, remove all the items from the group
// [3] if this group represents a form, then delete all boxes and text items from it
// [4] if this form/logic group belongs to a logic group whose sole member is the group
//     i'm deleting, then delete this logic group; work backwards till i find another form
void CFormScrollView::DeleteActiveGroup(CDEGroup* pGroup)
{
    int iLevel = GetCurForm()->GetLevel();

    CDEFormFile* pFF = GetFormFile();
    CDELevel*    pLevel = pFF->GetLevel( iLevel );

    // [0]  these next few lines prep for undo/redo; push the group first, for
    //      when i pop it, i want to restore the form first, then the group, so
    //      when i add the group items back, the form is around to add them to it!

    int iForm = pGroup->GetFormNum();
    int iGroup = pGroup->GetParent()->GetItemIndex(pGroup);
    CDEForm* pForm = pFF->GetForm(iForm);

    FormUndoStack form_undo_stack;
    form_undo_stack.PushUndoObj(CFormUndoObj::Action::UR_delete, pGroup, iGroup, pGroup->GetParent()->GetName()); // copy e'thing under the group
    form_undo_stack.PushUndoObj(CFormUndoObj::Action::UR_delete, pForm, iForm, pFF->GetName());  // only copy the form's boxes & FST
    GetDocument()->PushUndo(std::move(form_undo_stack)); // push the undo obj on to the undo stack!

    // back to regularly scheduled deletion :)

    RemoveAllGrids();              // [1]

    RemoveAllTrackers();

    // if this is the last form for the level, then don't remove it!

    if (pLevel->GetNumGroups() == 1)
    {
        DeleteGroupAndForms (pGroup, false);    // [2]

        DeletionWrapUp (true, GetCurForm(), pGroup);    // [4] will also rebuild the tree

        // i want the entire screen refreshed, not just the tracker region

        Invalidate();
        SendMessage(WM_PAINT);

        return; // done! bail!
    }
    else
    {
        DeleteGroupAndForms (pGroup, true); // [2]
    }

    // [4] now i need to look up/backwards and delete logic control groups, as nec;
    // i.e., those that have no items in them other than the group i just deleted
    CString sName = pGroup->GetName();
    pFF->RemoveUniqueName (sName);
    pLevel->RemoveItem (sName);

    int iFormNo = 0;    // this is the form # to display when all done; default to form 1
    SetPointers (iFormNo);
    RecreateGrids (iFormNo);    // recreate any grids needed for this view

    DeletionWrapUp (true, GetCurForm(), GetCurGroup()); // will rebuild the tree and stuff

    // i want the entire screen refreshed, not just the tracker region

    Invalidate();
    SendMessage(WM_PAINT);
}


void CFormScrollView::DeleteBlock(CDEBlock& form_block, bool delete_block_fields)
{
    CDEForm* pForm = GetCurForm();
    CDEFormFile* pFF = GetFormFile();
    CDEGroup* pGroup = GetCurGroup();
    CDEGroup* pParent = form_block.GetParent();
    FormUndoStack form_undo_stack;

    if (pParent->GetItemType() == CDEFormBase::Roster)
    {
        // If the block is in a roster, push the whole roster so we don't
        // have to deal with col/block/field links
        CDERoster* pRoster = static_cast<CDERoster*>(pParent);
        CDEGroup* pRosterParent = pRoster->GetParent();
        form_undo_stack.PushUndoObj(CFormUndoObj::Action::UR_modify, pRoster, pRosterParent->FindItem(pRoster->GetName()), pRosterParent->GetName());
    }

    else if (pParent->GetItemType() == CDEFormBase::Group)
    {
        // Use individual ops for the fields and block
        const int iBlockIndex = pParent->GetItemIndex(&form_block);
        form_undo_stack.PushUndoObj(CFormUndoObj::Action::UR_delete, &form_block, iBlockIndex, pParent->GetName());
        if (delete_block_fields) {
            for (int iBlFld = 0; iBlFld < form_block.GetNumFields(); ++iBlFld)
            {
                const int iFieldIndex = iBlockIndex + iBlFld + 1;
                CDEField* pField = static_cast<CDEField*>(pParent->GetItem(iFieldIndex));
                form_undo_stack.PushUndoObj(CFormUndoObj::Action::UR_delete, pField, iBlockIndex, pParent->GetName());
            }
        }
    }

    else
    {
        ASSERT(FALSE);
    }

    // Remove block fields from group
    if( delete_block_fields )
    {
        std::vector<CDEField*> block_fields = pParent->GetBlockFields(form_block);

        for( CDEField* pField : block_fields )
        {
            pFF->RemoveUniqueName(pField->GetName());
            pForm->RemoveItem(pField->GetName());
            pParent->RemoveItem(pField->GetName());
        }

        Invalidate();
    }

    // Remove the block
    pFF->RemoveUniqueName(form_block.GetName());
    pForm->RemoveItem(form_block.GetName());
    pParent->RemoveItem(form_block.GetName());


    RemoveAllTrackers();

    if (pParent->GetItemType() == CDEFormBase::Roster) {
        CDERoster* pRoster = static_cast<CDERoster*>(pParent);
        if (pRoster->GetNumCols() == 1) {
            // user deleted the last col, so kill the roster
            CString rosterName = pRoster->GetName();
            CDEGroup* pRosterParent = pRoster->GetParent();

            pForm->RemoveItem(rosterName);  // remove the roster from the form first
            pFF->RemoveUniqueName(rosterName);
            RemoveGrid(rosterName);
            pRosterParent->RemoveItem(rosterName);

            form_undo_stack.GetURObj(0)->SetAction(CFormUndoObj::Action::UR_delete);
        }
        else {
            CFormGrid* pGrid = FindGrid(pRoster);
            pGrid->BuildGrid();
            pGrid->RecalcLayout();
        }
    }

    GetDocument()->PushUndo(std::move(form_undo_stack));

    const bool bRebuildTree = true;
    DeletionWrapUp(bRebuildTree, pForm, pGroup);

    SetFocus();
}

///////////////////////////////////////////////////////////////////////////////////////////////////
//
//                      CFormScrollView::IsAnyGridFieldSelected()
//
// csc 12/1/00
///////////////////////////////////////////////////////////////////////////////////////////////////

bool CFormScrollView::IsAnyGridFieldSelected() const
{
    for (int i = 0; i<GetNumGrids() ; i++)  {
        if (GetGrid(i)->GetCurField() != NONE)  {
            return true;
        }
    }
    return false;
}


///////////////////////////////////////////////////////////////////////////////////////////////////
//
//                      CFormScrollView::IsAnyGridColSelected()
//
// csc 8/24/00
///////////////////////////////////////////////////////////////////////////////////////////////////

bool CFormScrollView::IsAnyGridColSelected() const
{
    for (int i = 0; i<GetNumGrids() ; i++)  {
        if (GetGrid(i)->GetCurCol() != NONE)  {
            return true;
        }
    }
    return false;
}


///////////////////////////////////////////////////////////////////////////////////////////////////
//
//                      CFormScrollView::IsAnyGridTrackerSelected()
//
// csc 12/29/00
///////////////////////////////////////////////////////////////////////////////////////////////////

bool CFormScrollView::IsAnyGridTrackerSelected() const
{
    for (int i = 0; i<GetNumGrids() ; i++)  {
        if (GetGrid(i)->GetNumTrackers()>0)  {
            return true;
        }
    }
    return false;
}

CFormGrid* CFormScrollView::GetGridWhereTrackerSelected()
{
    CFormGrid* pGrid;
    for (int i = 0; i<GetNumGrids() ; i++)  {
        pGrid = GetGrid(i);
        if (pGrid->GetNumTrackers()>0)  {
            return pGrid;
        }
    }
    return nullptr;
}


///////////////////////////////////////////////////////////////////////////////////////////////////
//
//                      CFormScrollView::IsAnyGridRowSelected()
//
// csc 8/24/00
///////////////////////////////////////////////////////////////////////////////////////////////////
bool CFormScrollView::IsAnyGridRowSelected() const
{
    for (int i = 0; i < GetNumGrids() ; i++)  {
        if (GetGrid(i)->GetCurRow() != NONE)  {
            return true;
        }
    }
    return false;
}



///////////////////////////////////////////////////////////////////////////////////////////////////
//
//                              CFormScrollView::DeleteActiveItem()
//
// you can delete the following types of entities from a form:
//
// - a box
// - a text blk
// - a field
// - a roster
// - a group of items selected in a tracker region
//
// you can not delete a group, as there is nothing *visual* about a group that is
// represented on a form
//
// this is the first func that gets called (after OnKeyDown()) when deleting something
///////////////////////////////////////////////////////////////////////////////////////////////////

void CFormScrollView::DeleteActiveItem()
{
    // this is a bit shoddy, but will fix soon; if this func got invoked by the
    // user right-clicking on the form and choosing "Delete Item", then i need
    // to delete it, and not the current item (if there is one); so try this
    CFormDoc* pDoc = GetDocument();
    if (GetNumTrackers())  {
        DeleteTrackerRegionData();
        SetFocus();                 // leave focus w/the form
        RedrawWindow();
        pDoc->SetModifiedFlag(true);
    }
    else if (IsAnyGridFieldSelected()) {   // let this be called before trackers
        DeleteGridField();
        SetFocus();
    }
    else if (IsAnyGridColSelected() || IsAnyGridRowSelected()) {
        DeleteGridCol();
        SetFocus();
    }
    else  {
        // nop for now
    }
}


///////////////////////////////////////////////////////////////////////////////////////////////////
//
//                          CFormScrollView::DeleteSingleItem
//
//  this func is called from within DeleteTrackerRegionData()
///////////////////////////////////////////////////////////////////////////////////////////////////

bool CFormScrollView::DeleteSingleItem (CDEFormFile* pFF, CDEForm* pForm, CDEGroup* /*pGroup*/, CDEItemBase* pItem)
{
    bool bRebuildTree = (pItem->GetItemType() != CDEFormBase::Text);
    CString sName = pItem->GetName();
    CDEFormBase::eItemType eIT = pItem->GetItemType();

    pForm->RemoveItem(sName);  // remove the item from the form first
    pFF->RemoveUniqueName(sName);

    if (eIT == CDEFormBase::Roster)  {
        // if it's a roster, remove the assoc grid obj   csc 8/24/00
        RemoveGrid(sName);
        CDERoster* pRoster = STATIC_DOWNCAST(CDERoster, pItem);
        for (int i = 0; i<pRoster->GetNumItems(); i++)  {
            pFF->RemoveUniqueName(pRoster->GetItem(i)->GetName());
        }
    }
    if (bRebuildTree)  {
        GetCurLevel()->RemoveItem(sName);// THIS is where the memory for the obj is actually deleted
    }
    return bRebuildTree;
}


// ********************************************************************************
//  this func is called from within DeleteTrackerRegionData()

void CFormScrollView::DeletionWrapUp (bool bRebuildTree, CDEForm* pForm, CDEGroup* pGroup)
{
    int iFormNdx = GetFormIndex();

    SetPointers (iFormNdx);

    if (!pForm->AnyKeyedFieldsOnForm())
    {
        pForm->SetRecordRepeatName(_T(""));
        pGroup->SetLoopingVars();
    }

    if (bRebuildTree) // if we deleted a text item, not nec to rebuild tree ctrl
    {
        GetDocument()->GetFormTreeCtrl()->ReBuildTree (iFormNdx);

        MarkDictTree(); // updates the icons on the dict tree to reflect the deletion

        SetFocus ();    // set the focus to *ME*, the view, don't leave it w/the tree ctrl

        GetDocument()->SetModifiedFlag(true);
    }
}

// ********************************************************************************
// the following two funcs are needed between deletes of tracker items; basically,
// after an item has been deleted from a form, the item indices will be off for
// some items (i.e., those items that have a higher index # than the item being
// deleted!).  so after each delete i need to refresh the indice ptrs

void CFormScrollView::UpdateTrackerBoxIndices(int startIndex, int index)
{
    int         loop, temp, max = GetNumTrackers();
    CFormTracker    tracker;

    for (loop=startIndex; loop < max; loop++)
    {
        tracker = GetTracker(loop);

        if (!tracker.IsBox())   // if this ISN'T a tracker for a box, skip it
            continue;

        temp = tracker.GetIndex();

        if (temp > index)
        {
            tracker.SetIndex(temp-1);
            SetTrackerAt(loop, tracker);
        }
    }
}

void CFormScrollView::UpdateTrackerFieldIndices (int startIndex, int index)
{
    int         loop, temp, max = GetNumTrackers();
    CFormTracker    tracker;

    for (loop=startIndex; loop < max; loop++)
    {
        tracker = GetTracker(loop);

        if( tracker.IsBox() )    // if this IS a tracker for a box, skip it
            continue;

        temp = tracker.GetIndex();

        if (temp > index)
        {
            tracker.SetIndex(temp-1);
            SetTrackerAt(loop, tracker);
        }
        else if (temp == index)  // this shld be the text portion of a field who got deleted
        {
            tracker.SetIndex(NONE);    // signal to me that this tracker isn't any good!
            SetTrackerAt(loop, tracker);
        }
    }
}

// ********************************************************************************
// this func takes all the tracker items (both items and boxes) and deletes them;
// note when doing so i've got to start from the last item in the array work back,
// as removing an item will cause a readjustment of the array boundaries and there-
// fore hose the validity of my indices!  however, as the indices in the tracker
// are in ascending order, removing them in this fashion (backwards) shld not cause
// any probs

void CFormScrollView::DeleteTrackerRegionData()
{
    int max = GetNumTrackers();

    CDEForm*        pForm = GetCurForm();
    CDEFormFile*    pFF    = GetFormFile();
    CDEGroup*       pGroup = GetCurGroup();
    CDEItemBase*    pItem;
    CDEField*       pField;
    CIMSAString     sName;
    CFormTracker    tracker;
    FormUndoStack form_undo_stack;
    bool bRebuildTree = false;

    for (int i = 0; i < max; i++)
    {
        CFormTracker tracker = GetTracker(i);
        int index = tracker.GetIndex();

        // is the user is trying to delete the text portion of a data entry box?

        if (tracker.IsFldTxt())
        {
            // if only the field's text was selected, then the user wants
            // to clear the text; if both the box and text were selected,
            // then the field will be deleted and no biggie if text got deleted

            if (! (tracker.IsFldBoxSel()))  // if the data entry box wasn't selected...
            {
                pField = (CDEField*)(pForm->GetItem(index));

                if (pField == nullptr) // will happen when box got deleted
                    continue;

                CDEText* pText = &pField->GetCDEText();

                CDEGroup* pParent = pField->GetParent();    // stand-alone field or roster field?
                int iFieldIndex = pParent->FindItem(pField->GetName());

                form_undo_stack.PushUndoObj(CFormUndoObj::Action::UR_delete, pText, iFieldIndex, pField->GetName());

                CRect rect = pText->GetDims();
                rect.right = rect.left; // want to keep the origin point, but don't want the
                rect.bottom = rect.top; // rect to have a height or width (else get ghost trackers)

                pField->SetFieldLabelType(FieldLabelType::Custom);
                pField->SetCDEText(rect,_T(""));
            }
        }

        else if( tracker.IsBox() )
        {
            form_undo_stack.PushUndoObj(CFormUndoObj::Action::UR_delete, &pForm->GetBoxSet().GetBox(index), index, pForm->GetName());
            pForm->GetBoxSet().RemoveBox(index);
            UpdateTrackerBoxIndices(i + 1, index);
        }

        else
        {
            pItem = pForm->GetItem(index);

            // this if/then blk is for undo/redo

            if (pItem->IsKindOf(RUNTIME_CLASS(CDEText)))
            {
                form_undo_stack.PushUndoObj(CFormUndoObj::Action::UR_delete, pItem, index, pForm->GetName());
            }
            else if (pItem->IsKindOf(RUNTIME_CLASS(CDEField)))
            {
                CDEField* pField = assert_cast<CDEField*>(pItem);
                CDEGroup* pParent = pField->GetParent();    // stand-alone field or roster field?
                int iFieldIndex = pParent->FindItem(pField->GetName());

                form_undo_stack.PushUndoObj(CFormUndoObj::Action::UR_delete, pItem, iFieldIndex, pParent->GetName());

                CDEBlock* pBlock = pParent->GetBlockForFieldAt(iFieldIndex);
                if (pBlock)
                    form_undo_stack.PushUndoObj(CFormUndoObj::Action::UR_modify, pBlock, pParent->GetItemIndex(pBlock), pParent->GetName());
            }
            else if (pItem->IsKindOf(RUNTIME_CLASS(CDERoster)))
            {
                CDEGroup* pParent = pItem->GetParent();
                int iRstrIndex = pParent->FindItem (pItem->GetName());
                form_undo_stack.PushUndoObj(CFormUndoObj::Action::UR_delete, pItem, iRstrIndex, pParent->GetName());
            }
            bRebuildTree |= DeleteSingleItem(pFF, pForm, pGroup, pItem);

            UpdateTrackerFieldIndices (i+1, index);
        }
    }

    if (form_undo_stack.GetNumURObjs() > 0)
        GetDocument()->PushUndo(std::move(form_undo_stack)); // push the undo obj on to the undo stack!

    RemoveAllTrackers();    // remove the indices of all the deleted items!

    DeletionWrapUp (bRebuildTree, pForm, pGroup);

    SetFocus();
}

// ********************************************************************************

void CFormScrollView::UpdateTrackerRectsAndRefresh()
{
    CDEForm* pForm = GetCurForm();
    CRect bigRect;

    // for the tracker nd to subtract the
    CPoint sp = GetScrollOffset();

    for( int i = 0; i < GetNumTrackers(); ++i )
    {
        CFormTracker& tracker = GetTracker(i);

        bigRect.UnionRect(bigRect, tracker.GetTheTrueRect());  // get the old dims

        CRect rect = tracker.GetRectFromFormObject(*pForm);
        rect.OffsetRect(sp);

        KeepRectInClient(rect);

        tracker.SetRect(rect);

        bigRect.UnionRect(bigRect, tracker.GetTheTrueRect());  // and union w/new pos
    }

    InvalidateRect(bigRect, true); // do a rect, else too much flicker

    SendMessage(WM_SIZE);
}


template<typename T>
CFormTracker& CFormScrollView::AddTrackerT(const CRect& rect, int index)
{
    int tracker_index = GetNumTrackers();
    AddTracker(CFormTracker(rect));

    CFormTracker& tracker = m_aTrackerItems[tracker_index];
    tracker.SetIndex(index);

    if constexpr(std::is_same_v<T, CDEBox>)
    {
        tracker.SetBox(true);
    }

    else
    {
        static_assert_false();
    }

    return tracker;
}

// ********************************************************************************
// adding a tracker box; if a rect for a field, could be either the data entry box's
// coords, or the text portion's coords

void CFormScrollView::AddTracker(const CRect& rect, int i,
                                 bool bIsFldTxt/*=false*/,
                                 bool bIsBoxSel/*false*/)
{
    CFormTracker track(rect);

    track.SetIndex(i);
    track.SetIsFldTxt(bIsFldTxt);
    track.SetIsBoxSel(bIsBoxSel);
    track.SetFormView(this);   // csc, reported by Bounds Checker 11/29/00

    AddTracker(track);
}

void CFormScrollView::UpdateTrackerSh()
{
    for (int i = 0; i < GetNumTrackers(); i++)
    {
        CFormTracker tracks =  GetTracker(i);
        int x = tracks.GetIndex();
        bool found = false;
        for (int itrk = 0; itrk < GetNumTrackers(); itrk++)
        {
            CFormTracker track = GetTracker(itrk);
            if (track.GetIndex() == x && itrk != i)
            {
                found = true;
                break;
            }
        }
        if (found && tracks.IsFldTxt())
        {
            tracks.SetIsBoxSel(true);
            SetTrackerAt(i, tracks);
        }
    }
    for (int x = 0; x < GetNumTrackers()-1; x++ )
    {
        for (int j = x+1; j < GetNumTrackers(); j++)
        {
            CFormTracker track = GetTracker(x);
            CFormTracker track2 = GetTracker(j);
            if (track.GetRect().top > track2.GetRect().top && track.GetIndex() != track2.GetIndex())
            {
                SetTrackerAt(x,track2);
                SetTrackerAt(j,track);
            }
        }
    }

}


// ********************************************************************************
//  added 10-04-00 by smg
//  use this func when you're adding a CDEField and want both the text and data
//  entry box selected; the rect passed in must be of the data entry box for this
//  to work right!

void CFormScrollView::AddTracker(int i, CRect rect)
{
    if (i == NONE) {
        return;
    }
    CFormTracker track (rect);

    track.SetIndex(i);
    track.SetIsFldTxt (false);
    track.SetIsBoxSel (false);
    track.SetFormView(this);   // csc, reported by Bounds Checker 11/29/00

    AddTracker(track);

    // the above added the data entry box; now add it's text portion too

    CDEItemBase* pItem = GetCurForm()->GetItem(i);

    if (pItem == nullptr) {    // gsf 04-apr-00  just being safe
        return;
    }
    rect = assert_cast<CDEField*>(pItem)->GetTextDims();
    rect.OffsetRect(GetScrollOffset());

    AddTracker(rect, i, true, true);
}

// ********************************************************************************

bool CFormScrollView::TrackerSetCursorTest (UINT nHitTest)
{
    bool bFound = false;

    for (int i = 0; i < GetNumTrackers() && !bFound; i++)
    {
        if (GetTracker(i).SetCursor(this, nHitTest))

            bFound = true;
    }
    return bFound;
}

// ********************************************************************************
//

BOOL CFormScrollView::OnSetCursor(CWnd* pWnd, UINT nHitTest, UINT message)
{
//  if (pWnd == this && m_tracker.SetCursor(this, nHitTest))

    if (pWnd == this && TrackerSetCursorTest (nHitTest) )
        return  true;

    return CScrollView::OnSetCursor(pWnd, nHitTest, message);
}


// ********************************************************************************
// a field's text can be tracked separately from it's data entry box; so if the
// item being passed in is a field, it's the data entry portion of the field;
// if it's the text portion of a field, it will be passed in as a CDEText item

bool CFormScrollView::IsItemBeingTracked (CDEItemBase* pItem, CPoint /*point*/)
{
    int i, max = GetNumTrackers();

    CDEItemBase* pLocalItem;
    CDEText*     pLocalText;
    CDEForm*     pForm = GetCurForm();
    CFormTracker track;

    bool bStop = false;
    bool bFound = false;

    for (i = 0; i < max && !bStop; i++)
    {
        track = GetTracker(i);

        if ( track.IsBox() )    // skip it, only looking for CDEItemBase entities
            continue;

        pLocalItem = pForm->GetItem(track.GetIndex());

        if (pLocalItem->GetItemType() == CDEFormBase::Field)
        {
            if (pItem == pLocalItem)    // then the user clicked on the data entry box
            {
                if (track.IsFldBoxSel()) // r we tracking the DE box?
                {
                    bFound=true;
                    bStop = true;
                }
            }
            else // compare the text portion of the local field
            {
                pLocalText = &((CDEField*)pLocalItem)->GetCDEText();
                if (pLocalText == pItem && track.IsFldTxt()) // r we tracking the text?
                {
                    bFound=true;
                    bStop = true;
                }
            }
        }
        else if (pItem == pLocalItem) {
            bFound = true;
            bStop = true;   // short-circuit the testing
        }
    }
    return bFound;
}

// ********************************************************************************
//

bool CFormScrollView::UserRightClickedOverTrackers (CDEItemBase* pRightClickItem,
                                                    CPoint point)
{
    if (GetNumTrackers() == 0)
        return false;

    if (IsItemBeingTracked (pRightClickItem, point))
        return true;

    else
    {
        if(GetKeyState(VK_CONTROL) >= 0)
            RemoveAllTrackersAndRefresh();

        return false;
    }
}

///////////////////////////////////////////////////////////////////////////////////////////////////
//
//                      CFormScrollView::OnRButtonUp()
//
//"point" is not adjusted by the scroll value, hence reason for scrollPt; however,
//whereas popMenu recognizes the cursor's scrolled pos, my FindItem doesn't,
//so i don't permanently adjust the "point" variable by the scrolled amt
//
//selections from the popup menu will be the same, except that depending on where
//the user clicked, not all selections are valid; first, i figure out if
//
//[1] the user clicked over anything (field, roster, text item); if not, clear any
//    tracker items or cols that had been selected within the grid; OR
//
//[2] if the user right-clicked over smthng, is it currently being tracked? if so,
//  set it as the cur item OR
//
//[3] make the selected item a tracker region, or if a roster column, select it
//
//now i'm ready to set up the pop-up menu the user wants to see having right-clicked
//someplace; there will be two essential choices:
//
//[4] user clicked on open real estate, i.e., no item found where the user right-clicked
//  - can not delete the form (reserve the right to change my mind on this one later)
//  - can not group the form...yet!!
//
//[5] 1+ RFTs being tracked
//  - can only select properties if one item selected (eventually i'll have some props
//    common to 1+ items)
//  - can not add text (text would have to be placed at click point, which means
//    writing over top of selected fields, which makes for an ugly form!)
//    - can only view logic or skips if non-text item selected (if 2+ fields selected,
//    prob of how to display)
//
///////////////////////////////////////////////////////////////////////////////////////////////////

void CFormScrollView::OnRButtonUp(UINT /*nFlags*/, CPoint point)
{
    BCMenu popMenu;
    CRect rect;

    CDEItemBase* pItem = nullptr;

    CPoint scrollPt = point;
    scrollPt.Offset (GetDeviceScrollPosition());    // offset the clicked pt by the scroll pos

    m_pRightClickRoster = nullptr;

    bool bRosterCol = false;
    bool bRosterRow = false;
    bool bRosterCell = false;  // csc 9/12/00

    bool bMultipleFieldsSelected = false;

    // find the item located at the mouse cursor
    GetDocument()->GetFormFile().FindItem (GetCurForm(), scrollPt, &pItem);

    if (pItem == nullptr)  {
        // [1] user didn't click over anything
        RemoveAllTrackersAndRefresh();
        UnselectRosterRowOrCol();   // any clear previously-selected col
    }
    else if( AreOnlyMultipleFieldsSelected(pItem) ) { // 20120612
        bMultipleFieldsSelected = true;
    }
    else if (UserRightClickedOverTrackers (pItem, point))  {
        // [2] user clicked over tracker rgn
        SetCurItem();   // sets up the rightClickItem, if possible
    }
    else  {
        // [3] user clicked over smthng but it's not being tracked
//      if(GetKeyState(VK_CONTROL)>=0){
            RemoveAllTrackersAndRefresh();
//      }
        int i = GetCurForm()->GetItemIndex ( pItem );
        rect = pItem->GetDims();
        rect.OffsetRect(GetScrollOffset());

        CDEFormBase::eItemType eType = pItem->GetItemType();

        if (eType == CDEFormBase::Roster)   { // right click on a roster...
            CDERoster* pRoster = STATIC_DOWNCAST(CDERoster,pItem);
            CFormGrid* pGrid = FindGrid(pRoster);

            if (pGrid->GetCurCol()!=NONE && pRoster->GetOrientation() == RosterOrientation::Horizontal)  {
                // column selected; prep to show context menu
                m_pRightClickRoster = pRoster;
                m_pRightClickItem = pRoster->GetItem(pGrid->GetCurCol());
                bRosterCol = true;
            }
            else if (pGrid->GetCurRow()!=NONE && pRoster->GetOrientation() == RosterOrientation::Vertical)  {
                // row selected; prep to show context menu
                m_pRightClickRoster = pRoster;
                m_pRightClickItem = pRoster->GetItem(pGrid->GetCurRow());
                bRosterRow = true;
            }
            else if (pGrid->GetCurCell()!=CPoint(NONE,NONE))  {
                // cell selected; prep to show context menu
                m_pRightClickRoster = pRoster;
                if (pRoster->GetOrientation() == RosterOrientation::Horizontal)  {
                    m_pRightClickItem = pRoster->GetItem(pGrid->GetCurCell().x);
                }
                else  {
                    m_pRightClickItem = pRoster->GetItem(pGrid->GetCurCell().y);
                }
                bRosterCell = true;
            }
            else  {
                // otherwise, select the entire grid
                AddTracker(rect, i);
                m_pRightClickItem = pRoster;
            }
        }
        else if (eType == CDEFormBase::Field) {

            UnselectRosterRowOrCol();   // clear previously-selected col or row
            CDEField* pField = assert_cast<CDEField*>(pItem);
            CDEText* pText = &pField->GetCDEText();

            if (pText->GetDims().PtInRect(scrollPt)){
                rect = pText->GetDims();
                AddTracker(rect, i, true);
                pItem = pText;
                m_pRightClickItem = pText;
            }
            else
            {
                AddTracker(rect, i);
                m_pRightClickItem = pItem;
            }
        }
        else    // it's a genuine text blk!
        {
            UnselectRosterRowOrCol();   // clear previously-selected col or row
            AddTracker(rect, i);
            m_pRightClickItem = pItem;
        }
        DrawTrackerOutlines (GetDC());
    }

    popMenu.CreatePopupMenu();

    auto add_box_entries = [&]()
    {
        bool box_tool_bar_showing = assert_cast<CFormChildWnd*>(GetParentFrame())->IsBoxToolBarShowing();
        popMenu.AppendMenuItems(!box_tool_bar_showing, { { ID_SHOW_BOXTOOLBAR, _T("Add &Boxes") } });
    };

    if (pItem==nullptr)
    {
        // [4]
        popMenu.AppendMenu(MF_ENABLED, ID_EDIT_FORMPROP, _T("Form &Properties"));
        popMenu.AppendMenu(MF_ENABLED, ID_DELETE_FORM, _T("&Delete Form"));
        popMenu.AppendMenu(MF_SEPARATOR);

        popMenu.AppendMenu(MF_STRING, ID_EDIT_TEXTPROP, _T("Add &Text"));

        add_box_entries();

        popMenu.AppendMenu(MF_STRING, ID_ADD_FORM, _T("Add &Form"));

        popMenu.AppendMenu(MF_SEPARATOR);
        popMenu.AppendMenu(MF_ENABLED, ID_VIEW_LOGIC,   _T("View &Logic"));
        if (assert_cast<CFormChildWnd*>(GetParentFrame())->GetUseQuestionText()) {
            popMenu.AppendMenu(MF_ENABLED, ID_QSF_EDITOR, _T("View CAPI &Question"));
        }

        m_bAddRFT = true;
    }
    else  {

        if( bMultipleFieldsSelected ) { // 20120612
            popMenu.AppendMenu(MF_STRING,ID_EDIT_MULTIPLE_FIELDPROP,_T("Field &Properties"));
        }

        else
        {
            //Check if there are multiple trackers
            CDEFormBase::eItemType eSelectedItemsType = CDEFormBase::UnknownItem;

            if (GetNumTrackers() == 2) {    // see if it's the field & it's accompanying text

                CFormTracker pTracker1 = GetTracker(0);
                CFormTracker pTracker2 = GetTracker(1);

                if ((pTracker1.GetIndex() == pTracker2.GetIndex()) &&
                    (pTracker1.IsFldTxt() && pTracker1.IsFldBoxSel() ||
                     pTracker2.IsFldTxt() && pTracker2.IsFldBoxSel()))
                {
                    eSelectedItemsType = CDEFormBase::Field;
                }
                else
                    eSelectedItemsType = GetPropItemTypeFromTrackers();
            }
            else if(GetNumTrackers() > 1) { //Get the item type from the multiple trackers.
                eSelectedItemsType = GetPropItemTypeFromTrackers();
            }
            else {
            // [5]
                eSelectedItemsType = pItem->GetItemType();
            }

            switch (eSelectedItemsType) {   // use this for determining the property opt ONLY
                case CDEFormBase::Text:
                    popMenu.AppendMenu(MF_STRING, ID_EDIT_TEXTPROP, _T("Text &Properties"));
                    break;
                case CDEFormBase::Roster:
                    if (bRosterCol || bRosterRow)  {
                        popMenu.AppendMenu(MF_STRING, ID_EDIT_FIELDPROP, _T("Field &Properties"));
                    }
                    else if (bRosterCell)  {
                        popMenu.AppendMenu(MF_STRING, ID_EDIT_CELLPROP, _T("Cell &Properties"));
                    }
                    else {
                        popMenu.AppendMenu(MF_STRING, ID_EDIT_GRIDPROP, _T("Roster &Properties"));
                        popMenu.AppendMenu(MF_STRING, ID_EDIT_GRID_AUTOFIT, _T("Roster AutoFit"));
                    }
                    break;
                case CDEFormBase::Field:
                    popMenu.AppendMenu(MF_STRING, ID_EDIT_FIELDPROP, _T("Field &Properties"));
                    break;
                default:
                    popMenu.AppendMenu(MF_STRING | MF_GRAYED, ID_EDIT_FIELDPROP, _T("&Properties"));
            }
        }

        popMenu.AppendMenu(MF_STRING, ID_DELETE_ITEM, _T("&Delete"));
        popMenu.AppendMenu(MF_SEPARATOR);

        if (pItem->GetItemType() == CDEFormBase::Roster)   // use right-click item for all other opts
        {
            popMenu.AppendMenu(MF_STRING, ID_DELETE_ITEM, _T("&Delete"));
            popMenu.AppendMenu(MF_SEPARATOR);

            if (bRosterCol || bRosterRow || bRosterCell) {

                popMenu.AppendMenu(MF_STRING, ID_ADD_TEXT, _T("Add &Text"));
            }
            else {
                popMenu.AppendMenu(MF_STRING | MF_GRAYED, ID_ADD_TEXT, _T("Add &Text"));
            }
        }
        else {
            popMenu.AppendMenu(MF_STRING | MF_GRAYED, ID_ADD_TEXT, _T("Add &Text"));
        }

        add_box_entries();

        popMenu.AppendMenu(MF_STRING, ID_ADD_FORM, _T("Add &Form"));
        popMenu.AppendMenu(MF_SEPARATOR);
        popMenu.AppendMenu(MF_STRING, ID_VIEW_LOGIC,   _T("View &Logic"));
        if (assert_cast<CFormChildWnd*>(GetParentFrame())->GetUseQuestionText()) {
            popMenu.AppendMenu(MF_STRING, ID_QSF_EDITOR, _T("View CAPI &Question"));
        }

        m_bAddRFT = false;
        m_bAddForm = false;
    }
    m_cAddRFTPoint = scrollPt;
    GetWindowRect (rect);   // GetClientRect will not work, nd offset of entire screen, not just the formview
    popMenu.LoadToolbar(IDR_FORM_FRAME);   // BMD 09 Apr 2004
    popMenu.TrackPopupMenu(TPM_RIGHTBUTTON, rect.left + point.x, rect.top + point.y, this);
}


void CFormScrollView::OnDisplayBoxToolbar()
{
    m_bAddRFT = false;
    assert_cast<CFormChildWnd*>(GetParentFrame())->ShowBoxToolBar();
}

// ********************************************************************************

void CFormScrollView::SetCurItem()
{
    CDEItemBase* pItem;
    m_pRightClickItem = nullptr;   // default to not found

    int iNumTrackers = GetNumTrackers();

    if (iNumTrackers == 1)
    {
        CFormTracker track = GetTracker(0);

        if (! track.IsBox())
        {
            pItem = GetCurForm()->GetItem(track.GetIndex());

            CDEFormBase::eItemType eType = pItem->GetItemType();

            if (eType == CDEFormBase::Field)        // then check if the entire field or only txt portion shld be selected
            {
                if (track.IsFldTxt())
                {
                    m_pRightClickItem = &assert_cast<CDEField*>(pItem)->GetCDEText();
                }
                else
                {
                    m_pRightClickItem = pItem;
                }
            }
            else if (eType == CDEFormBase::Text || eType == CDEFormBase::Roster)
            {
                m_pRightClickItem = pItem;
            }
        }
    }
    else if (iNumTrackers == 2) // could be the text and box portion of the same field
    {
        CFormTracker track1 = GetTracker(0);
        CFormTracker track2 = GetTracker(1);

        int index1 = track1.GetIndex();
        int index2 = track2.GetIndex();

        if (index1 == index2)
        {
            CDEForm* pForm = GetCurForm();
            CDEItemBase* pItem1 = pForm->GetItem(index1);
            CDEItemBase* pItem2 = pForm->GetItem(index2);

            if (pItem1 == pItem2)
                m_pRightClickItem = pItem1;
        }
    }
}


// ********************************************************************************
// from the Form toolbar, Layout->Properties->Field Props, this will activate the
// selection only if a field is the one-n-only item selected

void CFormScrollView::OnUpdateEditFieldProp(CCmdUI* pCmdUI)
{
    if (m_pRightClickItem && m_pRightClickItem->GetItemType() == CDEFormBase::Field)
    {
       pCmdUI->Enable (true);
       return;
    }
    pCmdUI->Enable (false);
}


// ********************************************************************************
// this func will be invoked if the user selects OnRButtonUp()'s ID_EDIT_FIELDPROP
// entry; it modifies the properties dialog box for the field

void CFormScrollView::OnEditFieldProp()
{
    CFormDoc* pDoc = GetDocument();
    CDEField* pField = nullptr;

    if (m_pRightClickItem != nullptr)  // user wants to modify an extant field
    {
        pField = (CDEField*) m_pRightClickItem;
        m_pRightClickItem = nullptr;
    }
    else
    {
        ASSERT (false); // error
        return;
    }

    CFieldPropDlg dlg (pField, this);   // create the dialog, init

    int rtnVal = dlg.DoModal(); // this actually puts up the dialog

    m_bAddRFT = false;  // leave it this way so if user enters this func from the menu bar, will do the right thing

    if( rtnVal != IDOK )
        return;

    bool bFldTxtModified = ( pField->GetText() != dlg.m_sFldTxt );
    bool bFieldModified = bFldTxtModified;

    FieldLabelType field_label_type = ( !dlg.m_bTextLinkedToDictionary )                               ? FieldLabelType::Custom :
                                      ( GetDragOptions().GetTextUse() == CDEFormBase::TextUse::Label ) ? FieldLabelType::DictionaryLabel :
                                                                                                         FieldLabelType::DictionaryName;

    if( field_label_type != pField->GetFieldLabelType() )
    {
        pField->SetFieldLabelType(field_label_type);
        bFieldModified = true;
    }

    // now muck w/the text blk
    CSize size (GetDC()->GetTextExtent (dlg.m_sFldTxt));
    CRect rect = pField->GetTextDims();

    rect.bottom = rect.top + m_iHeight;
    rect.right = rect.left + size.cx;

    pField->SetCDEText (rect, dlg.m_sFldTxt);

    // if the field is keyed, and user wants to make protected, or
    // the field is protected, but the user wants keyed, allow the switch
    // for mirror fields, no change is allowed

    bool temp = pField->IsEnterKeyRequired();

    if ( temp && !dlg.m_bEnterKey ||
        !temp &&  dlg.m_bEnterKey)
    {
        pField->IsEnterKeyRequired ( (dlg.m_bEnterKey != 0));
        bFieldModified = true;
    }

    temp = pField->IsProtected();

    if ( temp && !dlg.m_bProtected ||
        !temp &&  dlg.m_bProtected)
    {
        pField->IsProtected (dlg.m_bProtected?true:false);
        bFieldModified = true;
    }

    temp = pField->IsHiddenInCaseTree();

    if ( temp && !dlg.m_bHideInCaseTree ||
        !temp &&  dlg.m_bHideInCaseTree)
    {
        pField->IsHiddenInCaseTree (dlg.m_bHideInCaseTree?true:false);
        bFieldModified = true;
    }

    temp = dlg.m_bAlwaysVisualValue ? true : false;

    if( pField->IsAlwaysVisualValue() != temp )
    {
        pField->SetAlwaysVisualValue(temp);
        bFieldModified = true;
    }

    temp = pField->IsSequential();

    if ( temp && !dlg.m_bSequential ||
        !temp &&  dlg.m_bSequential)
    {
        pField->IsSequential ( (dlg.m_bSequential != 0));
        bFieldModified = true;
    }

    temp = pField->IsPersistent();

    if ( temp && !dlg.m_bPersist ||
        !temp &&  dlg.m_bPersist)
    {
        pField->IsPersistent ( (dlg.m_bPersist != 0) );
        bFieldModified = true;
    }

    temp = pField->IsAutoIncrement();

    if ( temp && !dlg.m_bAutoIncrement ||
        !temp &&  dlg.m_bAutoIncrement)
    {
        pField->IsAutoIncrement(dlg.m_bAutoIncrement != 0);
        bFieldModified = true;
    }

    temp = pField->IsUpperCase();

    if ( temp && !dlg.m_bUpperCase ||
        !temp &&  dlg.m_bUpperCase)
    {
        pField->IsUpperCase ( (dlg.m_bUpperCase != 0) );
        bFieldModified = true;
    }

    if( pField->GetCaptureInfo() != dlg.m_captureInfo )
    {
        pField->SetCaptureInfo(dlg.m_captureInfo);
        bFieldModified = true;
    }

    if( pField->UseUnicodeTextBox() != dlg.m_bUseUnicodeTextBox )
    {
        pField->SetUseUnicodeTextBox(dlg.m_bUseUnicodeTextBox);
        bFieldModified = true;
    }

    if( pField->AllowMultiLine() != dlg.m_bMultiLineOption )
    {
        pField->SetMultiLineOption(dlg.m_bMultiLineOption);
        bFieldModified = true;
    }

    if( pField->GetKLID() != dlg.m_KLID ) // 20120822
    {
        pField->SetKLID(dlg.m_KLID);
        bFieldModified = true;
    }

    temp = pField->GetVerifyFlag();
    if(temp != (bool)dlg.m_bVerify){
        pField->SetVerifyFlag(dlg.m_bVerify);
        bFieldModified = true;
    }

    if( pField->GetValidationMethod() != dlg.m_eValidationMethod )
    {
        pField->SetValidationMethod(dlg.m_eValidationMethod);
        bFieldModified = true;
    }

    if (bFldTxtModified)  {
        // only need to do the following if user modified the screen txt
        // if this field belongs to a roster, the grid nds to know the text changed
        if (m_pRightClickRoster!=nullptr)  {
            CFormGrid* pGrid = FindGrid(m_pRightClickRoster);
            int iIndex = NONE;
            ASSERT(pGrid->GetCurCol()!=NONE||pGrid->GetCurRow()!=NONE);
            if (m_pRightClickRoster->GetOrientation() == RosterOrientation::Horizontal)  {
                if (pGrid->GetCurCol()==NONE)  {
                    // edting stub text
                    ASSERT(pGrid->GetCurRow()!=NONE);
                    iIndex=0;
                }
                else  {
                    // editing regular item header text
                    iIndex = pGrid->GetCurCol();
                }
            }
            else  {
                // vertical...
                if (pGrid->GetCurCol()==NONE)  {
                    // editing regular item header text
                    ASSERT(pGrid->GetCurRow()!=NONE);
                    iIndex = pGrid->GetCurRow();
                }
                else  {
                    // editing stub text
                    iIndex = 0;
                }
            }
            ASSERT(iIndex>=0 && iIndex<m_pRightClickRoster->GetNumCols());
            CDECol* pCol = m_pRightClickRoster->GetCol(iIndex);
            pCol->SetHeaderText(pField->GetCDEText());
            // SMG: is this OK?
        }

        UpdateTrackerRectsAndRefresh();
    }

    if (bFieldModified)
    {
        CFormTracker SaveTracker;   // save the whale!
        if (GetNumTrackers ())      // save the tracker b4 rebuildtree trashes it
            SaveTracker = GetTracker(0);
        pDoc->GetFormTreeCtrl()->ReBuildTree(GetFormIndex(),pField);    // and make the tree ctrl recognize it too
        pDoc->SetModifiedFlag(true);
        Invalidate();
        SendMessage(WM_SIZE);   // force a redraw of the view to recognize changes
        if (SaveTracker.GetIndex() != -1) { // if i had a tracker (won't if it's in a roster)
            AddTracker(SaveTracker);       // then reinstate it
            DrawTrackerOutlines (GetDC());
        }
    }

    SetFocus();     // leave focus w/the view
}


/////////////////////////////////////////////////////////////////////////////
//
//                         CFormScrollView::OnEditTextProp variable
//

// from the Form toolbar, Layout->Properties->Text Props, this will activate the
// selection only if a text is the one-n-only item selected

void CFormScrollView::OnUpdateEditTextProp(CCmdUI* pCmdUI)
{
    // smg: this nds revisiting, is not wkg if you expect to get fonts for mult items sel

    if ((m_pRightClickItem && m_pRightClickItem->GetItemType() == CDEFormBase::Text) ||
        GetPropItemTypeFromTrackers() == CDEFormBase::Text)
    {
       pCmdUI->Enable (true);
    }
    else if (m_pRightClickItem == nullptr && m_bAddRFT == true)    // user trying to add text
    {
        pCmdUI->Enable (true);
    }
    else
        pCmdUI->Enable (false);
}

/////////////////////////////////////////////////////////////////////////////
//  We should make a general text box dialog box with font and justification
//  for use here and in Map Viewer -- bmd
/////////////////////////////////////////////////////////////////////////////
/////////////////////////////////////////////////////////////////////////////////
//
//  void CFormScrollView::OnEditTextProp()
//
/////////////////////////////////////////////////////////////////////////////////

void CFormScrollView::OnEditTextProp()
{
    CDEFormFile* pFF = GetFormFile();
    CDEForm*     pForm = GetCurForm();
    CDEText*     pText = nullptr;
    CFormUndoObj::Action iAction = CFormUndoObj::Action::UR_unknown;
    bool bMultiItems = false;

    // first, get the proper ptr; either to the extant CDEText or to the newly-created one

    if (m_pRightClickItem != nullptr)  // user wants to modify an extant text blk
    {
        if(GetNumTrackers()==1){
            CFormTracker track = GetTracker(0);
            pText = (CDEText*) m_pRightClickItem;
        }
        else {
            pText = nullptr;
        }
        m_pRightClickItem = nullptr;
        iAction = CFormUndoObj::Action::UR_modify;
    }
    else if (m_pRightClickItem == nullptr && m_bAddRFT == true) {  // user trying to add a field
        pText = new CDEText();
        CRect rect (m_cAddRFTPoint, m_cAddRFTPoint);
        pText->SetName ( pFF->CreateUniqueName (_T("TEXT")) );
        pText->SetDims (rect);
        pText->SetParent (nullptr);
        pText->SetFont(pFF->GetDefaultTextFont());

//      pOrigText = new CDEText (pText);
        iAction = CFormUndoObj::Action::UR_add;
    }
    else if (GetPropItemTypeFromTrackers() == CDEFormBase::Text) { // user has selected several items, only font in common
        pText = nullptr;
        bMultiItems = true;
    }
    else {
        ASSERT (false);
        return;     // error
    }

    CTxtPropDlg dlgProp(this);
    dlgProp.m_bEnableText = (pText != nullptr);
    if(dlgProp.m_bEnableText){
        dlgProp.m_sText = pText->GetText();
    }
    dlgProp.m_lfDefault = pFF->GetDefaultTextFont();
    dlgProp.m_lfCustom = dlgProp.m_lfDefault;
    dlgProp.m_color = RGB(0,0,0);

    CDEText* pBackupText = pText; // 20120612, if selecting multiple text entries, the dialog will default to the font settings of the first selected text entry

    if( bMultiItems )
    {
        for( int i = 0; !pText && i < GetNumTrackers(); i++ )
        {
            CFormTracker tracker = GetTracker(i);

            if( tracker.IsFldTxt() )
            {
                CDEItemBase* pLocalItem = pForm->GetItem(tracker.GetIndex());
                CDEField* pField = DYNAMIC_DOWNCAST(CDEField,pLocalItem);
                pText = &pField->GetCDEText();
            }

            else if( !tracker.IsBox() )
            {
                CDEItemBase* pItem = pForm->GetItem(tracker.GetIndex());

                if( pItem->IsKindOf(RUNTIME_CLASS(CDEText)) )
                    pText = assert_cast<CDEText*>(pItem);
            }
        }
    }


    if (pText && !pText->GetUseDefaultFont())  {
        dlgProp.m_lfCustom = pText->GetFont();
    }
    if(pText) {
        dlgProp.m_color = pText->GetColor().ToCOLORREF();
    }
    if(pText){
        dlgProp.m_iFont=(pText->GetUseDefaultFont()?0:1);
    }
    else {
        dlgProp.m_iFont =1;
    }

    pText = pBackupText;

    if (iAction == CFormUndoObj::Action::UR_modify && !bMultiItems )    // smg: fix to work w/trackers
    {
        // push the undo obj on to the undo stack!
        GetDocument()->PushUndo(FormUndoStack(CFormUndoObj::Action::UR_modify, pText, pForm->GetItemIndex (pText), pForm->GetName()));
    }

    int iRetVal = dlgProp.DoModal();

    if(iRetVal == IDOK && !m_bAddRFT)
    {
        LOGFONT lfSelFont = ( dlgProp.m_iFont == 1 ? dlgProp.m_lfCustom : dlgProp.m_lfDefault );
        PortableColor color = PortableColor::FromCOLORREF(dlgProp.m_color);

        if( pText && GetNumTrackers() == 1 )
        {
            if( pText->GetLabel().Compare(dlgProp.m_sText) != 0 )
            {
                pText->SetLabel(dlgProp.m_sText);

                // if this is the CDEText associated with a CDEField, then we need to make sure that the
                // field's label type is Custom; first search for the field...
                for( int i = 0; i < pForm->GetNumItems(); i++ )
                {
                    CDEItemBase* pItem = pForm->GetItem(i);

                    if( pItem->GetItemType() == CDEFormBase::Field )
                    {
                        CDEField* pField = assert_cast<CDEField*>(pItem);

                        if( &pField->GetCDEText() == pText )
                        {
                            pField->SetFieldLabelType(FieldLabelType::Custom);
                            break;
                        }
                    }
                }
            }

            pText->SetUseDefaultFont(dlgProp.m_iFont==0);
            pText->SetFont(lfSelFont);
            pText->SetColor(color);
        }
        else
        {
            //Process all the rect trackers for the text items and set the font
            int iNumTrackers = GetNumTrackers();
            CFormTracker tracker ;
            for(int iIndex =0;iIndex < iNumTrackers;iIndex++)
            {
                tracker = GetTracker(iIndex);
                if(tracker.IsFldTxt())
                {
                    CDEItemBase* pLocalItem = pForm->GetItem(tracker.GetIndex());
                    CDEField* pField = DYNAMIC_DOWNCAST(CDEField,pLocalItem);
                    if(pField != nullptr)
                    {
                        pField->GetCDEText().SetUseDefaultFont(dlgProp.m_iFont==0);
                        pField->GetCDEText().SetFont(lfSelFont);
                        pField->GetCDEText().SetColor(color);
                    }
                }
                else if(tracker.IsBox())
                {
                    continue;
                }
                else
                {
                    CDEItemBase* pLocalItem = pForm->GetItem(tracker.GetIndex());
                    CDEText* pText = DYNAMIC_DOWNCAST(CDEText,pLocalItem);//savy&& check for roster here

                    if(pText != nullptr)
                    {
                        pText->SetUseDefaultFont(dlgProp.m_iFont==0);
                        pText->SetFont(lfSelFont);
                        pText->SetColor(color);
                    }
                    else
                    {
                        CDERoster* pRoster = DYNAMIC_DOWNCAST(CDERoster,pLocalItem);
                        if(pRoster!=nullptr)
                        {
                            CFormGrid* pGrid = FindGrid(pRoster);
                            pGrid->ChangeFont(lfSelFont);
                        }
                    }
                }
            }
        }

        if (dlgProp.m_applytoall) {
            FormFileIterator::ForeachCDEText(*pFF, [&](CDEText& text) { text.SetColor(color); });
        }
    }

    else if(m_bAddRFT)
    {
        if(iRetVal != IDOK)
        {
            pFF->RemoveUniqueName (pText->GetName());
            delete pText;                  // against it, so i nd to delete the unused pText
        }
        else if(iRetVal == IDOK)
        {
            if(dlgProp.m_sText.IsEmpty())
            {
                iRetVal = IDCANCEL;
                delete pText;
            }
            else
            {
                LOGFONT lfSelFont = (dlgProp.m_iFont==1?dlgProp.m_lfCustom:dlgProp.m_lfDefault);
                pText->SetLabel(dlgProp.m_sText);
                pText->SetUseDefaultFont(dlgProp.m_iFont==0);
                pText->SetFont(lfSelFont);
                pText->SetColor(PortableColor::FromCOLORREF(dlgProp.m_color));
                CDC* pDC = GetDC();
                int iSaveDC = pDC->SaveDC();
                pDC->SelectObject(pText->GetFont().GetCFont());
                CSize size (pDC->GetTextExtent (dlgProp.m_sText));
                pDC->RestoreDC(iSaveDC);
                ReleaseDC(pDC);

                CRect rect = pText->GetDims();
                rect.bottom = rect.top + size.cy;
                rect.right = rect.left + size.cx;

                pText->SetDims (rect);
                pText->SetText  (dlgProp.m_sText);
                pText->SetFormNum (GetFormIndex());

                GetCurForm()->AddItem(pText);
                GetDocument()->SetModifiedFlag(true);//use this in the !m_bAddRFT branch
            }
        }
    }
    if (iRetVal == IDOK)
    {
        UpdateDims();
        UpdateTrackerRectsAndRefresh();
        Invalidate();
        UpdateWindow();
        GetDocument()->SetModifiedFlag(true);//use this in the !m_bAddRFT branch
        //SendMessage(WM_SIZE); // force a redraw of the view to recognize our add
        if (iAction == CFormUndoObj::Action::UR_add)
        {
            GetDocument()->PushUndo(FormUndoStack(CFormUndoObj::Action::UR_add, pText, GetCurForm()->GetItemIndex (pText), GetCurForm()->GetName()));
        }

    }
    m_bAddRFT = false;

}

/////////////////////////////////////////////////////////////////////////////////
//
//  void CFormScrollView::OnUpdateEditLevelProp (CCmdUI* pCmdUI)
//
/////////////////////////////////////////////////////////////////////////////////
void CFormScrollView::OnUpdateEditLevelProp (CCmdUI* pCmdUI)
{
    HTREEITEM hItem = GetDocument()->GetFormTreeCtrl()->GetSelectedItem();
    CFormID* pID = (CFormID*)GetDocument()->GetFormTreeCtrl()->GetItemData(hItem);

    if(pID && pID->GetItemType() == eFTT_LEVEL)
    {
        pCmdUI->Enable (true);
    }
    else
    {
        pCmdUI->Enable (false);
    }
}

void CFormScrollView::OnEditLevelProp()
{
    CFormTreeCtrl* formTree = GetDocument()->GetFormTreeCtrl();
    HTREEITEM hItem = formTree->GetSelectedItem();
    CFormID*    pID = (CFormID*)formTree->GetItemData(hItem);

    if (pID->GetItemType() == eFTT_LEVEL)
    {
        CDEFormFile* pFF = &GetDocument()->GetFormFile();
        CDELevel* pLevel = (CDELevel*)pID->GetItemPtr();

        CLevPropDlg dlg(pLevel,this);

        if(dlg.DoModal() == IDOK)
        {
            pFF->RemoveUniqueName(pLevel->GetName());
            pLevel->SetLabel(dlg.m_sLevelLabel);
            pLevel->SetName(dlg.m_sLevelName);
            pFF->AddUniqueName(pLevel->GetName());

            pFF->BuildUniqueNL();

            GetDocument()->SetModifiedFlag(true);
            GetDocument()->GetFormTreeCtrl()->ReBuildTree(GetFormIndex());
        }
    }
}

// ********************************************************************************

void CFormScrollView::OnUpdateEditFFProp (CCmdUI* pCmdUI)
{
    HTREEITEM hItem = GetDocument()->GetFormTreeCtrl()->GetSelectedItem();
    CFormID* pID = (CFormID*)GetDocument()->GetFormTreeCtrl()->GetItemData(hItem);

    if(pID && pID->GetItemType() == eFTT_FORMFILE)
    {
        pCmdUI->Enable (true);
    }
    else
    {
        pCmdUI->Enable (false);
    }
}

void CFormScrollView::OnEditFFProp()
{
    CFormTreeCtrl* formTree = GetDocument()->GetFormTreeCtrl();
    HTREEITEM hItem = formTree->GetSelectedItem();
    CFormID*    pID = (CFormID*)formTree->GetItemData(hItem);

    if(pID->GetItemType() == eFTT_FORMFILE)
    {
        CDEFormFile* pFFile = &pID->GetFormDoc()->GetFormFile();

        if(!pID->GetItemPtr())
        {
            pID->SetItemPtr(pFFile);
        }

        CFFPropDlg dlg(pFFile,this);

        if (dlg.DoModal() != IDOK)
            return;

        pFFile->RemoveUniqueName(pFFile->GetName());
        pFFile->AddUniqueName   (dlg.m_sFFName);

        // smg: gotta change the name before i can build the uniq NL back up!
        pFFile->SetLabel(dlg.m_sFFLabel);
        pFFile->SetName (dlg.m_sFFName);

        pFFile->BuildUniqueNL();

        GetDocument()->SetModifiedFlag(true);

        if(pID->IsKindOf(RUNTIME_CLASS(CFormNodeID)))
        {
            ((CFormNodeID*) pID)->SetFFLabel(dlg.m_sFFLabel);

            GetDocument()->GetFormTreeCtrl()->ReBuildTree(GetFormIndex());
        }
    }
}

// ********************************************************************************

void CFormScrollView::OnUpdateEditGridProp (CCmdUI* pCmdUI)
{
    if (m_pRightClickItem && m_pRightClickItem->GetItemType() == CDEFormBase::Roster)
    {
       pCmdUI->Enable (true);
    }
    else
    {
        pCmdUI->Enable (false);
    }
}

void CFormScrollView::OnEditGridProp()
{
    CFormDoc* pDoc = GetDocument();
    CDERoster* pRoster;

    // first, get the proper ptr; either to the extant CDEText or to the newly-created one

    if (m_pRightClickItem != nullptr)  // user wants to modify an extant text blk
    {
        pRoster = (CDERoster*) m_pRightClickItem;
        m_pRightClickItem = nullptr;
    }
    else
    {
        ASSERT(false);   // Error, should not reach here
        return;
    }

    CGridProp dlg (pRoster, this);  // create the dialog, init

    if (dlg.DoModal() == IDOK)
    {
        bool orientation_changed = ( pRoster->GetOrientation() != dlg.GetRosterOrientation() );

        if( !orientation_changed &&
            pRoster->GetName() == dlg.m_sGridLabel &&
            pRoster->GetFreeMovement() == dlg.GetFreeMovement() )
        {
            return;     // user didn't change anything, bail
        }

        pDoc->SetModifiedFlag(true);

        pRoster->SetLabel(dlg.m_sGridLabel);
        pRoster->SetName(dlg.m_sGridName);

        pDoc->GetFormFile().BuildUniqueNL();

        pRoster->SetFreeMovement(dlg.GetFreeMovement());

        if( orientation_changed )
        {
            pRoster->SetOrientation(dlg.GetRosterOrientation());

            RemoveAllTrackers();    // remove the tracker surrounding the grid as it's size will change

            CFormGrid* pGrid = FindGrid(pRoster);    // csc 11/19/00
            ASSERT_VALID(pGrid);    // csc 11/19/00
            pGrid->Transpose(pRoster->GetOrientation(), true);    // csc 11/19/00
            pGrid->BuildGrid();    // csc 11/19/00
            pGrid->RecalcLayout();    // csc 11/19/00

            AddTracker(pRoster->GetDims(), GetCurForm()->GetItemIndex (pRoster) );
        }

        pDoc->GetFormTreeCtrl()->ReBuildTree(GetFormIndex(), pRoster);
        Invalidate();
        SendMessage(WM_SIZE);   // force a redraw of the view to recognize our modification
    }
}

// ********************************************************************************

void CFormScrollView::OnEditGridAutoFit()
{
    CDERoster* pRoster = nullptr;
    if (m_pRightClickItem != nullptr)
    {
        pRoster = (CDERoster*)m_pRightClickItem;
        m_pRightClickItem = nullptr;
    }
    else
    {
        ASSERT(false);   // Error, should not reach here
        return;
    }
    CFormGrid* pGrid= FindGrid(pRoster);
    ASSERT(pGrid);
    int iRows = -1;
    int iCols = -1;
    if (pRoster->GetOrientation() == RosterOrientation::Horizontal) {
        iRows = std::min(MIN_NUM_ROSTER_ROWS, pRoster->GetMaxLoopOccs() + 1);    // +1 csc 11/16/00
        iCols =  pRoster->GetNumCols();
    }
    else {
        iCols = std::min(MIN_NUM_ROSTER_ROWS, pRoster->GetMaxLoopOccs() + 1);    // +1 csc 11/16/00
        iRows = pRoster->GetNumCols();
    }

    CFormDoc* pFD = GetDocument();

    FormUndoStack form_undo_stack;
    form_undo_stack.PushMoveObj(pRoster, pRoster->GetParent()->GetItemIndex(pRoster), pRoster->GetParent()->GetName());
    pFD->PushUndo(std::move(form_undo_stack));

    pGrid->Resize(iRows, iCols);
    RemoveAllTrackers();

    pFD->SetModifiedFlag(true);
    RedrawWindow();
}

// ********************************************************************************
// the user can only add a new form to the current one if there's at least one item
// assigned to it

CDEGroup* CFormScrollView::OkToCreateNewForm()
{
    CDEItemBase*    pItem = nullptr;
    CDEGroup*       pParent = nullptr;
    CDEForm*        pCurForm = GetCurForm();
    CDEFormFile*    pFF = GetFormFile();
    CDEForm*        pForm;

    bool bFound = false;
    int curlevel = pCurForm->GetLevel();

//  int maxgrp = pLevel->GetNumGroups();//pFF->GetNumForms();// pCurForm->GetNumItems();
    CDEFormBase::eItemType eType;
    int maxfrm = pFF->GetNumForms();//pFF->GetNumForms();// pCurForm->GetNumItems();

    // make sure there's at least one keyed/display field on the form!

    for (int frm = 0; frm < maxfrm;frm++)
    {
        //CDEGroup* pGroup = pLevel->GetGroup(grp);
        pForm = pFF->GetForm(frm);//(frm);
        if (pForm->GetLevel() != curlevel) {
            continue;
        }
        int max = pForm->GetNumItems();
        bool bFounditminfrm = false;

        for (int i = 0; i < max ; i++)
        {
            pItem = pForm->GetItem(i);
            eType = pItem->GetItemType();
            if (eType == CDEFormBase::Roster || eType == CDEFormBase::Field)
            {
                bFounditminfrm = true;
                pParent = pItem->GetParent()->GetParent();  // want the form's parent, not the item's
                break;
            }
        }
        if (!bFounditminfrm)
        {
            bFound = true;
            break;
        }
    }
    if (bFound)
    {
        CString     sMsg;
        sMsg.Format (_T("Cannot add form - all forms must have at least one field before you can add another form.\n")
                     _T("There is at least one form with no fields."));
        AfxMessageBox (sMsg);
        pParent = nullptr;
    }
    return  pParent;
}

// ********************************************************************************
// this func will be invoked in one of two cases:
// - the user selects OnRButtonUp()'s ID_EDIT_FORMPROP entry, or
// - the user choose to add a form (via menu bar, edit, add, form)
//
// for now, when adding a new form, the group that represents it will just be added
// to the top level array of groups w/in the current level, i.e., i will not drill
// down to try and find the last group created...

void CFormScrollView::OnEditFormProp()
{
    CDEForm*  pForm     = nullptr;
    CDEGroup* pParent   = nullptr;
    CDEGroup* pGroup    = nullptr;
    CDEFormFile* pFF    = GetFormFile();

    m_bAddRFT = false;  // was set to true in OnRButtonUp() in case user adding text

    // Get the form

    if (m_bAddForm)
    {
        pParent = OkToCreateNewForm();

        if (pParent == nullptr)
            return;

        CDEGroup* pNewGroup = new CDEGroup();

        pNewGroup->SetName  (pFF->CreateUniqueName (_T("GROUP")));
        pNewGroup->SetLabel (_T("New Form"));

        pNewGroup->SetParent(pParent);

        pForm = new CDEForm (pFF->CreateUniqueName(_T("FORM")), pNewGroup->GetLabel());

        pForm->SetGroup (pNewGroup);
        pForm->SetDims  (0,0,INIT_ScrSz, INIT_ScrSz);
        pForm->SetLevel (GetCurForm()->GetLevel());     // new forms have the same level as the form currently being edited

        // set the question text height to match the last form's height
        if( pFF->GetNumForms() > 0 )
        {
            pForm->SetQuestionTextHeight(pFF->GetForm(pFF->GetNumForms() - 1)->GetQuestionTextHeight());
        }
    }

    else
    {
        pForm = GetCurForm();
    }

    pGroup = pForm->GetGroup();

    if (pGroup == nullptr)     // shld never happen
    {
        return;
    }

    CFormPropDlg dlg(pGroup, this);    // Put up the dialog box

    dlg.m_frmcolor = pForm->GetBackgroundColor().ToCOLORREF();

    if( pForm->GetCapturePosX() < 0 ) // 20120405 the capture position isn't set, so assume the mouse position, though using custom settings will still be unchecked
    {
        POINT pt;
        GetCursorPos(&pt);
        ScreenToClient(&pt);

        dlg.m_iSetcaptureposX = std::max((int)pt.x, 0);
        dlg.m_iSetcaptureposY = std::max((int)pt.y, 0);
        dlg.m_bSetcaptureposChecked = FALSE;
    }

    else
    {
        dlg.m_iSetcaptureposX = pForm->GetCapturePosX();
        dlg.m_iSetcaptureposY = pForm->GetCapturePosY();
        dlg.m_bSetcaptureposChecked = TRUE;
    }

    FormUndoStack form_undo_stack;
    bool bRebuildTree = true;

    if (dlg.DoModal() == IDOK)
    {
        pForm->SetLabel (dlg.m_sFormLabel); // NEVER change the unique name of the form!

        if (m_bAddForm)
        {
            pForm->SetLabel (dlg.m_sFormLabel); // NEVER change the unique name of the form!
            pForm->SetBackgroundColor(PortableColor::FromCOLORREF(dlg.m_frmcolor));

            if( dlg.m_bSetcaptureposChecked )   pForm->SetCapturePos(dlg.m_iSetcaptureposX,dlg.m_iSetcaptureposY); // 20120405
            else                                pForm->SetCapturePos(-1,-1);

            // if they're adding a new form i need to do the following:
            //
            // [1] add the new form to the form file
            // [2] add a new group (at the bottom) of the current level to represent it
            // [3] add the new group (to be represented as a form) to the bottom of the tree ctrl
            // [4] make it the currently selected item in the tree ctrl and ergo, the new form
            //     the currently selected form in the view

            RemoveAllGrids();  // trash any grids that were created in the old view

            CDEFormFile* pFF = GetFormFile();

            int iFormNum = pFF->GetNumForms();
            pFF->AddForm(pForm);

            pGroup->SetFormNum (iFormNum);  // index of added form will be 0-based, so +1
            pGroup->SetFormName (pForm->GetName());

            //GetCurLevel()->AddGroup(pGroup);
            pParent->AddItem(pGroup);

            pFF->RenumberFormsAndItems();
            pFF->RenumberAllForms();

            iFormNum = pGroup->GetFormNum();    // form # changed...

            pFF->RemoveUniqueName   (pGroup->GetName());    // uniq name possibly changed too
            pGroup->SetName         (dlg.m_sFormName);
            pFF->AddUniqueName      (dlg.m_sFormName);

            pFF->BuildUniqueNL();

            pGroup->SetLabel (dlg.m_sFormLabel);

            SetPointers(iFormNum); // no active field, give the form #, no active txt blk

            GetDocument()->SetModifiedFlag(true);
    //      GetDocument()->GetFormTreeCtrl()->ReBuildTree(iFormNum);    // leave focus w/tree ctrl
            MakeFormCurrent(iFormNum);
    //      GetDocument()->GetFormTreeCtrl()->SwitchRefresh();

            form_undo_stack.PushUndoObj(CFormUndoObj::Action::UR_add, pForm, iFormNum, pFF->GetName());  // only copy the form's boxes & FST
            form_undo_stack.PushUndoObj(CFormUndoObj::Action::UR_add, pGroup, GetCurLevel()->GetNumGroups()-1, pGroup->GetParent()->GetName()); // copy e'thing under the group
        }
        else    // editing an existing group/form
        {
            form_undo_stack.PushUndoObj(CFormUndoObj::Action::UR_modify, pForm, pGroup->GetFormNum(), pFF->GetName());   // only copy the form's boxes & FST
            form_undo_stack.PushUndoObj(CFormUndoObj::Action::UR_modify, pGroup, pGroup->GetFormNum(), pGroup->GetParent()->GetName()); // copy e'thing under the group
            pForm->SetLabel (dlg.m_sFormLabel); // NEVER change the unique name of the form!
            pForm->SetBackgroundColor(PortableColor::FromCOLORREF(dlg.m_frmcolor));

            if( dlg.m_bSetcaptureposChecked )   pForm->SetCapturePos(dlg.m_iSetcaptureposX,dlg.m_iSetcaptureposY); // 20120405
            else                                pForm->SetCapturePos(-1,-1);

            if (pGroup->GetLabel().Compare (dlg.m_sFormLabel) != 0) // label's been changed
            {
                pGroup->SetLabel(dlg.m_sFormLabel);

                GetDocument()->SetModifiedFlag(true);
            }

            if (pGroup->GetName().CompareNoCase (dlg.m_sFormName) != 0) // name's been changed
            {
                pFF->RemoveUniqueName   (pGroup->GetName());
                pGroup->SetName         (dlg.m_sFormName);
                pFF->AddUniqueName      (dlg.m_sFormName);

                pFF->BuildUniqueNL();

                GetDocument()->SetModifiedFlag(true);
            }
            GetDocument()->GetFormTreeCtrl()->ReBuildTree(GetFormIndex());

            RemoveAllGrids();
            RecreateGrids(GetFormIndex());
        }

        Invalidate();   // force the redraw
        SendMessage(WM_SIZE);   // force a redraw of the view to recognize our add

        GetDocument()->SetModifiedFlag(true);
    }
    else    // user cancelled out of dialog
    {
        if (m_bAddForm)     // User started to add a form, but decided against it
        {
            pFF->RemoveUniqueName (pForm->GetName());
            delete pForm;

            pFF->RemoveUniqueName (pGroup->GetName());
            delete pGroup;
        }

        else
        {
            bRebuildTree = false;
        }
    }

    if (form_undo_stack.GetNumURObjs() > 0) { // if something got dropped, then push the stack
        GetDocument()->PushUndo(std::move(form_undo_stack));
    }

    if( bRebuildTree )
        GetDocument()->GetFormTreeCtrl()->ReBuildTree();
}

// ********************************************************************************

// when choosing to add text via the menu bar, default the drop location to 1,1 + scroll offset

void CFormScrollView::OnAddText()
{
    m_pRightClickItem = nullptr;
    m_bAddRFT = true;

    m_cAddRFTPoint.x = 1;
    m_cAddRFTPoint.y = 1;

    m_cAddRFTPoint.Offset (GetDeviceScrollPosition());

    OnEditTextProp();
}

void CFormScrollView::OnAddForm()
{
    m_bAddRFT = false;
    if (GetFormFile() == nullptr)
        return;
    else
        m_bAddForm = true;

    m_bAddForm = true;

    OnEditFormProp();

    m_bAddForm = false;

//  smg: the following is so after adding the form to the current level,
//  the form will be selected! (currently, the first form gets selected
//  in the tree

    CFormTreeCtrl* pTree = GetDocument()->GetFormTreeCtrl();    // get the tree

    HTREEITEM hCurItem = pTree->GetChildItem (pTree->GetRootItem()); // quest of Level 1

    int iCurLevel = GetCurForm()->GetLevel();   // rtns a 0-based val

    if (iCurLevel > 0) {    // then get the quest for level 2

        hCurItem = pTree->GetNextSiblingItem (hCurItem);
    }
    if (iCurLevel > 1) {    // then get the quest for level 3

        hCurItem = pTree->GetNextSiblingItem (hCurItem);
    }

    hCurItem = pTree->GetChildItem (hCurItem);  // get first form in cur level

    HTREEITEM hNextItem = hCurItem;

    while (hNextItem != nullptr)   // loop thru the forms to the last one
    {
        hCurItem = hNextItem;
        hNextItem = pTree->GetNextSiblingItem (hCurItem);
    }
    pTree->SelectItem (hCurItem);   // select the newly-created form
}

// ********************************************************************************

void CFormScrollView::RemoveAllTrackers()
{
    m_aTrackerItems.RemoveAll();

    // if we're deleting trackers, then nobody shld be selected...

    m_pRightClickItem = nullptr;
    m_pRightClickRoster = nullptr;
}


void CFormScrollView::RemoveAllTrackersAndRefresh()
{
    CRect rect, bigRect (0,0,0,0);
    for (int i = 0; i < GetNumTrackers(); i++)  {
        rect = GetTracker(i).GetTheTrueRect();
        bigRect.UnionRect(bigRect, rect);
    }
    RemoveAllTrackers();
    if (!bigRect.IsRectEmpty())  {
        InvalidateRect(bigRect);   // csc 11/19/00
        UpdateWindow();             // csc 11/19/00
    }
}

// i'll be needing to call these two funcs myself directly, in which, i *don't* want
// the trackers cleared! so do the hit test to prevent this from occurring

void CFormScrollView::OnHScroll(UINT nSBCode, UINT nPos, CScrollBar* pScrollBar)
{
    CPoint pos;
    GetCursorPos(&pos);

    if (nSBCode != SB_ENDSCROLL)    // Begin scroll -- take down trackers
    {
        RemoveAllTrackersAndRefresh();
    }

    CScrollView::OnHScroll(nSBCode, nPos, pScrollBar);  // mfc-gen

    TRACE(_T("The nPos = %d \n") ,GetScrollPos(SB_HORZ));

    if (nSBCode == SB_ENDSCROLL)    // End scroll -- put back trackers
    {
//      IsOkToDrawTOs (true);
//      DrawTrackerOutlines(GetDC());
    }
}

void CFormScrollView::OnVScroll(UINT nSBCode, UINT nPos, CScrollBar* pScrollBar)
{
    CPoint pos;
    GetCursorPos(&pos);

//  if (OnNcHitTest (pos) == HTVSCROLL)

//      IsOkToDrawTOs (false);

    if (nSBCode != SB_ENDSCROLL)    // Begin scroll -- take down trackers
    {
        RemoveAllTrackersAndRefresh();
    }

    CScrollView::OnVScroll(nSBCode, nPos, pScrollBar);

    if (nSBCode == SB_ENDSCROLL)
    {
//      IsOkToDrawTOs (true);
//      DrawTrackerOutlines(GetDC());
    }
}

// ********************************************************************************

void CFormScrollView::OnGenerateFrm()
{
    const TCHAR* const message =
        _T("A new set of forms based on the input dictionary will be generated.\n\n")
        _T("Any existing forms WILL BE LOST!\n\n")
        _T("Do you want to continue?\n\n")
        _T("You can add variables to the existing forms without using generate forms. Simply drag and drop variables from the dictionary tree onto an existing form.");

    if( AfxMessageBox(message, MB_YESNOCANCEL | MB_ICONQUESTION | MB_DEFBUTTON2) == IDYES )
    {
        GetDocument()->GenerateFormFile();
        // 09/99 workshop participants request; if they're currently viewing the
        // dict tree and generate a new form, they may not realize forms have been
        // added, etc., so switch to the form tree to show them

        AfxGetMainWnd()->SendMessage(UWM::Designer::SelectTab, (WPARAM)FrameType::Form);
    }
}

// ********************************************************************************
// 11/00; we're changing the way creating a new form file works; previously, doing
// a new gave them IDs on a blank form and that's it; now, they will get an entirely
// blank file, and a dialog will appear asking if they'd like us to generate the form
// file for them;

void CFormScrollView::CreateNewFF()
{
    CIMSAString csMsg = _T("Would you like CSPro to create a set of forms\nfor you, based on the input dictionary?\n");

    if (AfxMessageBox(csMsg, MB_YESNOCANCEL | MB_ICONQUESTION | MB_DEFBUTTON2) == IDYES)
    {
        GetDocument()->GenerateFormFile();
        AfxGetMainWnd()->PostMessage(UWM::Designer::SelectTab, (WPARAM)FrameType::Form);
    }
}

// ********************************************************************************
// called from the menu bar (maybe elsewhere?) when the user wants to delete the
// currently displayed form

void CFormScrollView::OnDeleteForm()
{
    CDELevel* pLevel = GetCurLevel();
    CDEGroup* pGroup = pLevel->GetGroupOnForm ( GetFormIndex() );
    m_bAddRFT = false;

    CFormChildWnd* pParent = assert_cast<CFormChildWnd*>(GetParentFrame());
    if (pParent->GetViewMode() != FormViewMode) { //switch to formview
        pParent->OnViewForm();
    }
    if (pGroup == nullptr)

        ASSERT (false);     // little prob here folx...
    else
    {
        if (OkToDeleteForm())   // now that user can undo delete, don't ask if they're sure!
        {
            DeleteActiveGroup (pGroup);
            GetDocument()->GetFormTreeCtrl()->SwitchRefresh();
        //  RedrawWindow();
        }
    }
}


void CFormScrollView::OnUpdateDeleteForm (CCmdUI* pCmdUI)
{
    pCmdUI->Enable (OkToDeleteForm());
}

// ********************************************************************************

bool CFormScrollView::OkToDeleteForm()
{
    CDELevel* pLevel = GetCurLevel();
    if (pLevel->GetNumGroups() == 1)    // if there's only one group left, probe
    {
        CDEGroup* pGroup = pLevel->GetGroup ( 0 );

        if (pGroup->GetNumItems())  // if there's at least one item in the group
        {
            return true;
        }

        CDEForm* pForm = GetCurForm();

        if (pForm->GetNumItems())   // or if there's some boxes or free-standing text
        {
            return true;
        }
        return false;
    }
    return true;
}


// called when the user does a right-click on an item and chooses 'Delete'
// (from either the view or tree ctrl, hence dilemma in code) OR
// when it's called from the menu bar! based
// the GetFocus() call is to see who ordered the delete, the view or the tree

void CFormScrollView::OnDeleteItem()
{
    CFormTreeCtrl* pFT = GetDocument()->GetFormTreeCtrl();

    CWnd* pWnd = GetFocus();

    if (IsAnyGridTrackerSelected())
    {
        CFormGrid* pGrid = GetGridWhereTrackerSelected();
        pGrid->SetHitObj (pGrid->GetTracker(0).GetHitOb());
        pGrid->SendMessage(WM_COMMAND, ID_DELETE_ITEM);
        SetFocus();
    }
    else if (pWnd == this || IsAnyGridColSelected() || IsAnyGridRowSelected())
    {
        DeleteActiveItem();
    }
    else if (pWnd == pFT)
    {
        HTREEITEM hItem = pFT->GetSelectedItem();

        if (!hItem)     // this shld never happen
            return;

        CFormID* pFormID = (CFormID*) pFT->GetItemData (hItem);

        if ( pFormID->GetItemType() == eFTT_FORM )

            SendMessage(WM_COMMAND, ID_DELETE_FORM); // want OnDeleteForm() to be called
        else
            DeleteActiveItem();
    }
//  else if (pWnd ==
}

void CFormScrollView::OnUpdateDeleteItem(CCmdUI* pCmdUI)
{
    if (GetNumTrackers() ||             // anything selected on the form itself
        IsAnyGridTrackerSelected() ||  // any boxes, text items or fields sel w/in the grid
        IsAnyGridColSelected() ||      // any col selected w/in the grid
        IsAnyGridRowSelected())        // or row selected w/in a grid
    {
        pCmdUI->Enable (true);
        return;
    }

    // they may have the form node selected from the form's tree ctrl,
    // so this is what the following is checking

    CFormTreeCtrl* pFT = GetDocument()->GetFormTreeCtrl();

    if (GetFocus() != pFT)  // if the tree ctrl doesn't have focus, no pt in continuing
    {
        pCmdUI->Enable (false);
        return;
    }

    HTREEITEM hItem = pFT->GetSelectedItem();

    if (!hItem)     // this shld never happen
    {
        pCmdUI->Enable (false);
        return;
    }

    CFormID* pFormID = (CFormID*) pFT->GetItemData (hItem);

    eNodeType eNT = pFormID->GetItemType();

    if (eNT == eFTT_FIELD || eFTT_GRID)

        pCmdUI->Enable (true);
    else
        pCmdUI->Enable (false);
}


// ***********************************************************************
// as the user moves around w/the mouse, i want to see the x,y coords;
// will now display on the status bar, lower right corner

void CFormScrollView::OnMouseMove(UINT nFlags, CPoint point)
{
    CScrollView::OnMouseMove(nFlags, point);    // mfc-gen

    CIMSAString csStatus;

    CDC* pDC = GetDC();
    OnPrepareDC(pDC);

    pDC->DPtoLP(&point);
    ReleaseDC(pDC);
    csStatus.Format(_T("%d,%d"), point.x, point.y);

    AfxGetMainWnd()->SendMessage(UWM::Form::UpdateStatusBar, 0, (LPARAM)csStatus.GetString());
}


BOOL CFormScrollView::OnMouseWheel(UINT /*nFlags*/, short zDelta, CPoint /*pt*/) // 20110408
{
    if( zDelta > 0 )
        OnVScroll(SB_LINEUP, 0, nullptr);

    else
        OnVScroll(SB_LINEDOWN, 0, nullptr);

    return FALSE;
}


// ***********************************************************************
// (mfc comment:)  The framework calls this member function when the CWnd object
// background needs erasing (for example, when resized). It is called to prepare
// an invalidated region for painting.

BOOL CFormScrollView::OnEraseBkgnd(CDC* pDC)
{
    // Savy &&&
    //Do this to avoid flicker in the resize

    CRect rect;
    GetClientRect(rect);
    CDEForm* pForm = GetCurForm();
    if (pForm==nullptr) return FALSE;
    pDC->FillSolidRect(rect,pForm->GetBackgroundColor().ToCOLORREF());
    CBrush br(pForm->GetBackgroundColor().ToCOLORREF());
    FillOutsideRect(pDC, &br);
    return TRUE;
}

// ***********************************************************************
//
//  ok, the next little block is going to do formatting operations on any
//  selected items
//
// ***********************************************************************
//
// i think i'll follow this rule for alignment; take the very first item
// in the tracker array and use it's (x,y) coords to offset all subsequent
// tracker items;
//
// NOTE: for now, the first item in the tracker array is NOT the first item
//       visually that you see on the upper left corner of the screen...
//       rather, w/in the tracker region it is the first item to be traversed!

bool CFormScrollView::CheckRosterColumnAlignmentEnable()
{
    for( int i = 0; i < m_aCSProGrid.GetSize(); i++ )
    {
        if( m_aCSProGrid[i]->GetNumSelCols() || m_aCSProGrid[i]->GetNumSelRows() )
            return true;
    }

    return false;
}

bool CFormScrollView::AlignRosterColumns(const std::variant<HorizontalAlignment, VerticalAlignment>& alignment)
{
    for( int i = 0; i < m_aCSProGrid.GetSize(); i++ )
    {
        if( m_aCSProGrid[i]->GetNumSelCols() || m_aCSProGrid[i]->GetNumSelRows() )
        {
            m_aCSProGrid[i]->AlignFields(alignment);
            return true;
        }
    }

    return false;
}

void CFormScrollView::UpdateGridDecimalChar(TCHAR decimalChar)
{
    for( int i = 0; i < m_aCSProGrid.GetSize(); i++ )
    {
        m_aCSProGrid[i]->SetDecimalChar(decimalChar);
    }

}


namespace
{
    using FormAlignmentType = std::variant<HorizontalAlignment, VerticalAlignment>;

    FormAlignmentType IdToAlignment(UINT nID)
    {
        return ( nID == ID_LAYOUT_ALIGN_LEFT )      ? static_cast<FormAlignmentType>(HorizontalAlignment::Left) :
               ( nID == ID_LAYOUT_ALIGN_RIGHT )     ? static_cast<FormAlignmentType>(HorizontalAlignment::Right) :
               ( nID == ID_LAYOUT_ALIGN_TOP )       ? static_cast<FormAlignmentType>(VerticalAlignment::Top) :
               /*( nID == ID_LAYOUT_ALIGN_BOTTOM)*/   static_cast<FormAlignmentType>(VerticalAlignment::Bottom);
    }
}


void CFormScrollView::OnUpdateLayoutAlign(CCmdUI* pCmdUI)
{
    FormAlignmentType alignment = IdToAlignment(pCmdUI->m_nID);

    auto is_enabled = [&]()
    {
        if( GetNumTrackers() > 1 )
            return true;

        if( IsAnyGridTrackerSelected() )
        {
            for( int i = 0; i < GetNumGrids(); i++ )
            {
                if( GetGrid(i)->CanAlign(alignment) )
                    return true;
            }
        }

        // this will allow the alignment of fields within rosters
        return CheckRosterColumnAlignmentEnable();
    };

    pCmdUI->Enable(is_enabled());
}


// *************************************************************************
// 10/27/00, smg: a field's data entry box can now be selected separate from
// its text entry; so i had to modify all the alignment funcs to no longer
// assume that if a field box was selected, its text box was selected too

void CFormScrollView::OnLayoutAlign(UINT nID)
{
    FormAlignmentType alignment = IdToAlignment(nID);

    if( AlignRosterColumns(alignment) ) // 20111114
        return;

    // grid stuff added csc 2/2/01
    if( IsAnyGridTrackerSelected() )
    {
        for( int i = 0; i < GetNumGrids(); ++i )
        {
            CFormGrid* pGrid = GetGrid(i);

            if( pGrid->IsTrackerActive() )
            {
                pGrid->SendMessage(WM_COMMAND, nID);
                return;
            }
        }

        ASSERT(false);
    }

    CFormDoc* pDoc = GetDocument();
    CDEForm* pForm = GetCurForm();
    CPoint ptScroll(GetScrollPos(SB_HORZ), GetScrollPos(SB_VERT));

    // currently aligning boxes and rosters is not supported, so keep track of what elements should be aligned
    std::vector<CFormTracker*> trackers_to_align;

    std::optional<LONG> text_position;
    std::optional<LONG> field_position;

    std::function<void(std::optional<LONG>& position, const CRect& rect)> update_alignment_position;
    std::function<void(LONG position, CRect& rect)> update_rect;

    if( nID == ID_LAYOUT_ALIGN_LEFT )
    {
        update_alignment_position = [&](std::optional<LONG>& position, const CRect& rect)
        {
            position = ( position.has_value() && *position < rect.left ) ? *position : ( ptScroll.x + rect.left );
        };

        update_rect = [](LONG position, CRect& rect) { rect.MoveToX(position); };
    }

    else if( nID == ID_LAYOUT_ALIGN_RIGHT )
    {
        update_alignment_position = [&](std::optional<LONG>& position, const CRect& rect)
        {
            position = ( position.has_value() && *position > rect.right ) ? *position : ( ptScroll.x + rect.right );
        };

        update_rect = [](LONG position, CRect& rect) { rect.MoveToX(position - rect.Width()); };
    }

    else if( nID == ID_LAYOUT_ALIGN_TOP )
    {
        update_alignment_position = [&](std::optional<LONG>& position, const CRect& rect)
        {
            position = ( position.has_value() && *position < rect.top ) ? *position : ( ptScroll.y + rect.top );
        };

        update_rect = [](LONG position, CRect& rect) { rect.MoveToY(position); };
    }

    else
    {
        ASSERT(nID == ID_LAYOUT_ALIGN_BOTTOM);

        update_alignment_position = [&](std::optional<LONG>& position, const CRect& rect)
        {
            position = ( position.has_value() && *position > rect.bottom ) ? *position : ( ptScroll.y + rect.bottom );
        };

        update_rect = [](LONG position, CRect& rect) { rect.MoveToY(position - rect.Height()); };
    }

    // determine the alignment values
    for( int i = 0; i < GetNumTrackers(); ++i )
    {
        CFormTracker& tracker = GetTracker(i);

        if( tracker.IsBox() )
            continue;

        const CDEItemBase* pItem = pForm->GetItem(tracker.GetIndex());

        if( pItem == nullptr )
        {
            ASSERT(false);
            continue;
        }

        if( tracker.IsFldTxt() || pItem->IsKindOf(RUNTIME_CLASS(CDEText)) )
        {
            update_alignment_position(text_position, tracker.GetRect());
        }

        else if( pItem->IsKindOf(RUNTIME_CLASS(CDEField)) )
        {
            update_alignment_position(field_position, tracker.GetRect());
        }

        else
        {
            ASSERT(pItem->IsKindOf(RUNTIME_CLASS(CDEGroup)));
            continue;
        }

        trackers_to_align.emplace_back(&tracker);
    }

    // nothing to align
    if( trackers_to_align.size() < 2 )
        return;

    // if aligning one text and one field, align against each other
    if( trackers_to_align.size() == 2 && text_position.has_value() && field_position.has_value() )
    {
        text_position = ( nID == ID_LAYOUT_ALIGN_LEFT || nID == ID_LAYOUT_ALIGN_TOP ) ? std::min(*text_position, *field_position) :
                                                                                        std::max(*text_position, *field_position);
        field_position = text_position;
    }

    FormUndoStack form_undo_stack;

    // align the items
    for( CFormTracker* tracker: trackers_to_align )
    {
        CDEItemBase* pItem = pForm->GetItem(tracker->GetIndex());
        ASSERT(pItem != nullptr);

        bool item_is_standalone_text = pItem->IsKindOf(RUNTIME_CLASS(CDEText));
        const std::optional<LONG>& position = ( tracker->IsFldTxt() || item_is_standalone_text ) ? text_position :
                                                                                                   field_position;
        ASSERT(position.has_value());

        // adjust the dimensions
        CRect rect = tracker->GetRectFromFormObject(*pForm);
        update_rect(*position, rect);

        if( tracker->IsFldTxt() )
        {
            CDEField* pField = assert_cast<CDEField*>(pItem);
            CDEText* pText = &pField->GetCDEText();
            form_undo_stack.PushMoveObj(pText, tracker->GetIndex(), pForm->GetName());
            pText->SetDims(rect);
        }

        else if( item_is_standalone_text )
        {
            CDEText* pText = assert_cast<CDEText*>(pItem);
            form_undo_stack.PushMoveObj(pText, tracker->GetIndex(), pForm->GetName());
            pText->SetDims(rect);
        }

        else
        {
            ASSERT(pItem->IsKindOf(RUNTIME_CLASS(CDEField)));
            CDEField* pField = assert_cast<CDEField*>(pItem);
            int iFieldIndex = pField->GetParent()->FindItem(pField->GetName());
            form_undo_stack.PushMoveObj(pField, iFieldIndex, pField->GetParent()->GetName());
            pField->SetDims(rect);
        }

        // update the tracker
        tracker->SetRect(rect - ptScroll);
    }

    ASSERT(form_undo_stack.GetNumURObjs() > 0);
    pDoc->PushUndo(std::move(form_undo_stack));

    // leave focus w/the form
    SetFocus();                 
    RedrawWindow();
    pDoc->SetModifiedFlag(true);
    RefreshTrackers();
}


// ***************************************************************************

void CFormScrollView::RefreshTrackers()
{
    CRect rect, bigRect (0,0,0,0);

    for (int i = 0; i < GetNumTrackers(); i++)
    {
        rect = GetTracker(i).GetTheTrueRect();

        bigRect.UnionRect(bigRect, rect);
    }

    if (!bigRect.IsRectEmpty())
    {
        InvalidateRect(bigRect, true); // do a rect, else too much flicker
        //SendMessage(WM_SIZE);
        UpdateWindow();
    }
}


// ***************************************************************************
// when the user wants to view logic, the logic stuff of serpro works w/the tree
// ctrl (savy knows how, *i* don't), so if the active item (got trackers? :),
// is not selected in the tree ctrl, view logic gives them the logic for the FORM,
// not the specific item; so this little util has to be run from the view's onViewLogic
// and the formChildWnd

void CFormScrollView::SetCurTreeItem()
{
    CFormDoc*       pFD  = GetDocument();
    CFormTreeCtrl*  pFTC = pFD->GetFormTreeCtrl();
    CFormNodeID*    pRootID = pFTC->GetFormNode (pFD);  // ptr to root of form tree
    CDEItemBase*    pItem = GetCurItem();

    if (pItem != nullptr)
    {
        CDEFormBase::eItemType eIT = pItem->GetItemType();

        if (eIT == CDEFormBase::Roster || eIT == CDEFormBase::Field)
        {
            // this will expand the tree, if nec
            pFTC->SelectFTCNode (pRootID, GetFormIndex(), pItem);
        }
    }
}

void CFormScrollView::OnViewLogic()
{
    m_bAddRFT = false;
    CFormChildWnd* pParent = assert_cast<CFormChildWnd*>(GetParentFrame());
    pParent->OnViewLogic();
}


void CFormScrollView::OnQSFEditor()
{
    m_bAddRFT = false;
    CFormChildWnd* pParent = assert_cast<CFormChildWnd*>(GetParentFrame());
    pParent->OnQsfEditor();
}

// void CFormScrollView::OnEditDragOpts()  moved to CFormChildWnd :)


// ********************************************************************************
//
// this region for the grid funcs related to chris' instantiation of it (rather than UG)
//
//
// ********************************************************************************

// don't delete the grid's m_pRoster member; its removal will be done by CDEFormFile

void CFormScrollView::RemoveAllGrids()
{
    int iSize = GetNumGrids();

    CFormGrid* pGrid;

    for (int i = 0; i < iSize; i++)
    {
        pGrid = GetGrid(i);

        pGrid->DestroyWindow();   // still need to do w/chris' stuff?

        delete pGrid;
    }
    m_aCSProGrid.RemoveAll();
}

void CFormScrollView::RemoveGrid(CIMSAString sName)
{
    int iSize = GetNumGrids();
    bool bFound = false;
    CFormGrid* pGrid;
    for (int i = 0; i < iSize && !bFound; i++) {
        pGrid = GetGrid(i);
        if (pGrid->GetRoster()->GetName() == sName) {
            pGrid->DestroyWindow();
            delete pGrid;
            m_aCSProGrid.RemoveAt(i);
            bFound = true;
        }
    }
}

// created this to support undo/redo; when the user deletes a form, the group
// assoc w/that form gets canned too; however, if any grids on that form/within
// the group, need to take them down...let's go

void CFormScrollView::RemoveGrid(CDEGroup* pGroup)
{
    int i, max = pGroup->GetNumItems();
    CDEItemBase* pItem;
    CDEFormBase::eItemType eItem;

    for (i = 0; i < max; i++)
    {
        pItem = pGroup->GetItem(i);
        eItem = pItem->GetItemType();

        if (eItem == CDEFormBase::Group)
            RemoveGrid((CDEGroup*) pItem);
        else if (eItem == CDEFormBase::Roster)
            RemoveGrid(pItem->GetName());
    }
}

/////////////////////////////////////////////////////////////////////////////

void CFormScrollView::CreateGrid(CDERoster* pRoster)
{
    CDEFormFile* pFFile = &GetDocument()->GetFormFile();
    pRoster->RefreshStubsFromOccurrenceLabels(*pFFile);

    CFormGrid* pGrid = new CFormGrid (pRoster);
    pGrid->SetDecimalChar(GetDecimalCharacter());

    int formindex = pRoster->GetFormNum();
    if (formindex >= 0 )
    {
        pGrid->SetGridBkColor(pFFile->GetForm(formindex)->GetBackgroundColor().ToCOLORREF());
    }

    CPoint ptScroll(GetScrollPos(SB_HORZ), GetScrollPos(SB_VERT));
    bool bNoSize = (pRoster->GetDims().BottomRight()==CPoint(0,0))?true:false;
    pRoster->SetDims(pRoster->GetDims() - ptScroll);// csc 1/11/01
    if (bNoSize){
        // user is creating a new roster, no viewport size available ... show based on num rows/cols
        int iRows = std::min (MIN_NUM_ROSTER_ROWS, pRoster->GetMaxLoopOccs()+1);    // +1 csc 11/16/00
        int iCols = std::min (MIN_NUM_ROSTER_COLS, pRoster->GetNumCols());
        if (pRoster->GetOrientation() == RosterOrientation::Vertical)  {
            int iTmp = iRows;
            iRows = iCols;
            iCols = iTmp;
        }
        pFFile->UpdateDictItemPointers(pRoster);
        pGrid->Create (pRoster->GetDims().TopLeft(), iRows, iCols, this, pRoster);
    }
    else if (pRoster->GetDims().BottomRight()==CPoint(-1,-1)){//SAVY added this line for the oonvert stuff
        pFFile->UpdateDictItemPointers(pRoster);
        pGrid->Create(pRoster->GetDims().TopLeft(), -1, -1, this ,pRoster);
    }
    else  {
        // this roster already existed at some point, use previous dimensions...
        pFFile->UpdateDictItemPointers(pRoster);
        pGrid->Create(pRoster->GetDims(), this, pRoster);
    }
    AddGrid(pGrid);
}

/////////////////////////////////////////////////////////////////////////////
// find the grid object that is associated w/the given roster

CFormGrid* CFormScrollView::FindGrid(CDERoster* pRoster)
{
    CFormGrid* pGrid;

    for (int i = 0; i < GetNumGrids(); i++)
    {
        pGrid = GetGrid(i);

        if (pGrid->GetRoster() == pRoster)  // their addr is the same
        {
            return  pGrid;
        }
    }

    return nullptr;    // couldn't find it, problem!
}

/////////////////////////////////////////////////////////////////////////////
// find the grid object that is associated w/the given roster

void CFormScrollView::RefreshGridOccLabelStubs()
{    
    const CDEFormFile* pFF = GetFormFile();

    for (int i = 0; i < GetNumGrids(); i++)
    {
        CFormGrid* pGrid = GetGrid(i);
        pGrid->GetRoster()->RefreshStubsFromOccurrenceLabels(*pFF);
        pGrid->RefreshOccLabelsStubText();
    }
}

/////////////////////////////////////////////////////////////////////////////
// in case the user has a column selected in the roster, deselect it before proceeding;
// however, i don't know which grid may have a col selected; so run thru them all :{

void CFormScrollView::UnselectGridRowOrCol()
{
    CDEForm* pForm = GetCurForm();
    CDEItemBase* pItem;
    if(!pForm)
        return;

    for (int i = 0; i < pForm->GetNumItems(); i++)
    {
        pItem = pForm->GetItem(i);

        if (pItem->GetItemType() == CDEFormBase::Roster)
        {
            CFormGrid* pGrid = FindGrid((CDERoster*) pItem);

            if (pGrid != nullptr)
            {
                if (pGrid->GetCurCol() != NONE)
                {
                    pGrid->DeselectColumns ();
                    pGrid->Invalidate();            // if pop-up menu present, only this
                    pGrid->SendMessage(WM_SIZE);    // will clear the residual shadow
                }
                else if (pGrid->GetCurRow() != NONE)
                {
                    pGrid->DeselectRows ();
                    pGrid->Invalidate();            // if pop-up menu present, only this
                    pGrid->SendMessage(WM_SIZE);    // will clear the residual shadow
                }
            }
        }
    }
}


//////////////////////////////////////////////////////////////////////////////////////
//
//                      CGridWnd::DeleteGridCol()
//
/////////////////////////////////////////////////////////////////////////////////////

void CFormScrollView::DeleteGridCol()
{
    CDEForm*    pForm = GetCurForm();
    CDERoster*  pRoster = nullptr;
    CFormGrid* pGrid = nullptr;
    CDEFormFile* pFF = GetFormFile();
    int         i, iCurCol=NONE, iNumCols, iSelCol=NONE;
    bool bFound = false;

    FormUndoStack form_undo_stack;

    for (i = 0; i < GetNumGrids() && !bFound ; i++)  {
        pGrid = GetGrid(i);
        pRoster = pGrid->GetRoster();
        // Added By chirag
        // If all the columns are selected then clear the whole grid as grid
        bool deletegrid = false;
        if (pRoster->GetOrientation() == RosterOrientation::Horizontal )
        {
            if (pGrid->GetNumSelCols() == (pGrid->GetNumCols() - 1))
            {
                deletegrid = true;
            }
        }
        else
        {
            if (pGrid->GetNumSelRows() == (pGrid->GetNumRows() - 1))
            {
                deletegrid = true;
            }
        }
        if (deletegrid)
        {
            RemoveAllTrackers();
            pGrid->Deselect();
            int x = GetCurForm()->GetNumItems();
            CFormTracker track;
            track.m_rect = pRoster->GetDims();//pGrid->GetClientRect();
            track.SetIndex(x-1);
            track.SetBox (false);
            AddTracker(track); // the Item's in the bounding rectangle, flag it
            DeleteActiveItem();
            return;
        }

        iNumCols = ( pRoster->GetOrientation() == RosterOrientation::Horizontal ) ? pGrid->GetNumCols() : pGrid->GetNumRows();
        iCurCol = ( pRoster->GetOrientation() == RosterOrientation::Horizontal ) ? pGrid->GetCurCol() : pGrid->GetCurRow();
        if(iCurCol == NONE ) {
            continue;
        }

        ASSERT(iCurCol!=NONE);

        for (int iCol=iNumCols-1 ; iCol>=0 ; iCol--)  {
            iSelCol=NONE;
            if (pRoster->GetOrientation() == RosterOrientation::Horizontal)  {
                if (pGrid->IsColSelected(iCol))  {
                    iSelCol=iCol;
                }
            }
            else {
                if (pGrid->IsRowSelected(iCol))  {
                    iSelCol=iCol;
                }
            }
            if (iSelCol != NONE)  {

                if (pRoster->GetRightToLeft()) {
                    iSelCol = iNumCols - iSelCol; // swapp for right to left
                }

                // first, push on stack then kill any free cells assoc w/this col
                for( size_t fc = pRoster->GetNumFreeCells() - 1; fc < pRoster->GetNumFreeCells(); --fc )
                {
                    CDEFreeCell& free_cell = pRoster->GetFreeCell(fc);

                    if( free_cell.GetColumn() == iSelCol )
                    {
                        pRoster->RemoveFreeCell(fc);
                    }

                    else
                    {
                        free_cell.SetColumn(free_cell.GetColumn() - 1);
                    }
                }

                // back to the CDECol
                CDECol* pCol = pRoster->GetCol(iSelCol);
                int iRosterIndex = pRoster->GetParent()->GetItemIndex(pRoster);
                form_undo_stack.PushUndoObj(CFormUndoObj::Action::UR_modify, pRoster, iRosterIndex, GetCurForm()->GetName());

                // first, remove all the fields' unique names from the form file;
                // then, squash and clean up removal of the column from the grid

                for (i = 0; i < pCol->GetNumFields(); i++)  {
                    pFF->RemoveUniqueName(pCol->GetField(i)->GetName());
                }

                pRoster->RemoveAndDeleteCol(iSelCol);
                if (iSelCol<iCurCol)  {
                    iCurCol--;
                }
            }
        }
        if (pRoster->GetOrientation() == RosterOrientation::Horizontal )  {    // csc 9/13/04; see BC#1410
            pGrid->DeselectColumns();
        }
        else {
            pGrid->DeselectRows();                      // csc 9/13/04; see BC#1410
        }

        if (pRoster->GetNumCols()==1)  {
            // user deleted the last col, so kill the roster  csc 8/24/00
            CString sName = pRoster->GetName();
            CDEGroup* pParent = pRoster->GetParent();

            pForm->RemoveItem (sName);  // remove the roster from the form first
            pFF->RemoveUniqueName (sName);
            RemoveGrid(sName);
            pParent->RemoveItem (sName);
            pRoster = nullptr;
            iSelCol=NONE;
        }
        bFound = true;
    }
    ASSERT (bFound);

    CFormDoc* pFD = GetDocument();
    pFD->PushUndo(std::move(form_undo_stack));   // push the undo obj on to the undo stack!

    if (pRoster!=nullptr)  {
        pGrid->BuildGrid();
        pGrid->RecalcLayout();

        pFD->GetFormTreeCtrl()->ReBuildTree ( GetFormIndex(), pRoster);
        MarkDictTree(); // updates the icons on the dict tree to reflect the deletion
        pFD->SetModifiedFlag(true);

        // see if we need to shrink the grid to get rid of extra space
        CRect rcNewGrid(pRoster->GetDims());
        if (pGrid->GetTotalRect().Width()<rcNewGrid.Width())  {
            pGrid->Resize(NONE, pGrid->GetNumCols());
        }
        if (pGrid->GetTotalRect().Height()<rcNewGrid.Height())  {
            pGrid->Resize(pGrid->GetNumRows(), NONE);
        }
        // select the next column
        // smg, 26-08-03; we weren't checking the orientation to determine
        // whether the # of cols shld be checked, or the # of rows!
        if (pRoster->GetOrientation() == RosterOrientation::Horizontal) {
            if (iCurCol>=pGrid->GetNumCols())  {
                iCurCol--;
            }
        }
        else  // it's vertical orientation, check the # of rows!
            if (iCurCol>=pGrid->GetNumRows())  {
                iCurCol--;
            }

        if (pRoster->GetOrientation() == RosterOrientation::Horizontal)  {
            pGrid->SelectColumn(iCurCol);
        }
        else  {
            pGrid->SelectRow(iCurCol);
        }
    }
    else {
        pFD->GetFormTreeCtrl()->ReBuildTree ( GetFormIndex(), pRoster);
        MarkDictTree(); // updates the icons on the dict tree to reflect the deletion
        pFD->SetModifiedFlag(true);
    }

}

/////////////////////////////////////////////////////////////////////////////
// this is invoked when the user wants to delete a field w/in a column;
// if there is only one field in the col, the column gets nuked too; if
// there is one+ fields remaining, column stays put

void CFormScrollView::DeleteGridField()
{
    CDEForm*    pForm = GetCurForm();
    CDERoster*  pRoster = nullptr;
    CFormGrid* pGrid = nullptr;
    CDEFormFile* pFF = GetFormFile();
    CFormDoc*   pFD = GetDocument();
    int         i, iSelCol=NONE;
    bool bFound = false;
    CIMSAString sName;

    for (i = 0; i < GetNumGrids() && !bFound ; i++)  {

        pGrid = GetGrid(i);
        iSelCol = pGrid->GetCurCol();
        if (iSelCol == NONE)
            continue;

        // ok, if i made it to here, i've found a field to delete

        pRoster = pGrid->GetRoster();
        CDECol* pCol = pRoster->GetCol (iSelCol);
        int iSelField = pGrid->GetCurField();
        ASSERT (iSelField!=NONE);
        CDEField* pField = pCol->GetField (iSelField);

        sName       = pField->GetName();
        int iIndex  = pRoster->FindItem(sName);
        ASSERT (iIndex != NONE);

        CFormUndoObj::Action undoAction = (pCol->GetNumFields() == 1 && pRoster->GetNumCols() == 2) ?
            CFormUndoObj::Action::UR_delete :
            CFormUndoObj::Action::UR_modify;

        CDEGroup* pParent = pRoster->GetParent();
        int iRstrIndx = pParent->FindItem(pRoster->GetName());

        pFD->PushUndo(FormUndoStack(undoAction, pRoster, iRstrIndx, pParent->GetName()));    // push the undo obj on to the undo stack!

        pFF->RemoveUniqueName(sName);        // remove uniq name of the field
        pRoster->RemoveField(pField);
        pField = nullptr;

        if (pCol->GetNumFields() == 0)  // there are no fields remaining
        {
            pCol = nullptr;
            pRoster->RemoveAndDeleteCol(iSelCol);
            pGrid->DeselectColumns();

            // IF we deleted the column, see if it was the last one in the grid

            if (pRoster->GetNumCols()==1)  {
                // user deleted the last row, so kill the roster
                sName = pRoster->GetName();
                CDEGroup* pParent = pRoster->GetParent();
                pForm->RemoveItem (sName);  // remove the roster from the form first
                pFF->RemoveUniqueName (sName);
                RemoveGrid(sName);        // this has to occur before pParent->RemoveItem
                pParent->RemoveItem (sName);    // this will delete the roster
                pRoster = nullptr;             // set the ptr to nullptr
                iSelCol=NONE;
            }
        }
        bFound = true;
    }
    ASSERT(bFound);

    if (pRoster!=nullptr)  {
        pGrid->BuildGrid();
        pGrid->RecalcLayout();

        // see if we need to shrink the grid to get rid of extra space
        CRect rcNewGrid(pRoster->GetDims());
        if (pGrid->GetTotalRect().Width()<rcNewGrid.Width())  {
            pGrid->Resize(NONE, pGrid->GetNumCols());
        }
        if (pGrid->GetTotalRect().Height()<rcNewGrid.Height())  {
            pGrid->Resize(pGrid->GetNumRows(), NONE);
        }
        // select the next column
        ASSERT(iSelCol != NONE);
        if (iSelCol>=pGrid->GetNumCols())  {
            iSelCol--;
        }
//      pGrid->SelectColumn(iSelCol);
    }

    if (pRoster == nullptr)
        pFD->GetFormTreeCtrl()->ReBuildTree(GetFormIndex());
    else
        pFD->GetFormTreeCtrl()->ReBuildTree(GetFormIndex(), pRoster);
    MarkDictTree(); // updates the icons on the dict tree to reflect the deletion
    GetDocument()->SetModifiedFlag(true);
}

/////////////////////////////////////////////////////////////////////////////////
//
//  void CFormScrollView::DrawField(CDEField* pField)
//
/////////////////////////////////////////////////////////////////////////////////

void CFormScrollView::DrawField(CDEField* pField ,CDC* pDC)
{
    ASSERT(nullptr!= pField);

    if(!pField->GetDictItem())
        return;
 //   CDC* pDC = GetDC();
    int iSaveDC = pDC->SaveDC();

    pDC->SelectObject(pField->GetFont().GetCFont());
    CSize szChar = pDC->GetTextExtent(_T("0"),1);

    CRect rcFld = pField->GetDims();

    const CDictItem* pDictItem = pField->GetDictItem();
    ASSERT(pDictItem);

    // 20100608 if the user is using a control draw the border in a different color, and thicker
    CaptureType evaluated_capture_type = pField->GetEvaluatedCaptureInfo().IsSpecified() ?
        pField->GetEvaluatedCaptureInfo().GetCaptureType() : CaptureInfo::GetBaseCaptureType(*pDictItem);

    bool bUsingControl = ( evaluated_capture_type != CaptureType::TextBox );

    HPEN hBlueBorderPen = nullptr; // 20110509 not deleting the pen was creating problems when moving blocks on and off the screen

    if( bUsingControl )
    {
        // 20130417 use a dark aqua for the number pad
        COLORREF color = ( evaluated_capture_type == CaptureType::NumberPad ) ? RGB(95, 150, 160) :
                                                                                RGB(0, 0, 128);
        hBlueBorderPen = CreatePen(PS_SOLID, 2, color);

        pDC->SelectObject(hBlueBorderPen); // blue border, two pixels wide
        rcFld.left++;
        rcFld.top++;
    }

    pDC->MoveTo(rcFld.left,rcFld.top+1 );
    pDC->LineTo(rcFld.right-1, rcFld.top+1);
    pDC->LineTo(rcFld.right-1, rcFld.bottom);
    pDC->LineTo(rcFld.left, rcFld.bottom);

    if( bUsingControl )
        pDC->LineTo(rcFld.left,rcFld.top);

    else
        pDC->LineTo(rcFld.left,rcFld.top+1);

    //pDC->Rectangle(&rcFld);
    //pDC->FrameRect (rcFld, (CBrush*) pDC->SelectStockObject(BLACK_BRUSH));
  //  CBrush* pOldBrush = pDC->SelectObject(&brush);
   CRect rect(rcFld.left+1,rcFld.top+2 ,rcFld.right-1,rcFld.bottom);

   if( bUsingControl ) // 20100608 the fill rectangle should be slightly smaller due to the larger border
   {
       rect.right--;
       rect.bottom--;
       DeleteObject(hBlueBorderPen);
   }



 //   pDC->Rectangle(&rect);
   if(pField->IsProtected() || pField->IsMirror()){
        pDC->FillSolidRect(&rect, GetCurForm()->GetBackgroundColor().ToCOLORREF());
   }
   else if (pField->IsPersistent() || pField->IsAutoIncrement()){
        COLORREF clrBkgnd = GetCurForm()->GetBackgroundColor().ToCOLORREF();
        int red = GetRValue(clrBkgnd);
        int green = GetGValue(clrBkgnd);
        int blue = GetBValue(clrBkgnd);
        red = red + (255 - red)/2;
        green = green + (255 - green)/2;
        blue = blue + (255-blue)/2;
        clrBkgnd = RGB(red,green,blue);

        pDC->FillSolidRect(rect,clrBkgnd);
   }

   else {
        pDC->FillSolidRect(&rect, rgbWhite);
   }

    // TODO SAVY -- fix when pDictItem is OK (Ctrl+G bug)
    //                    CDictItem* pDictItem= fld.GetDEField()->GetDictItem();
    //                    ASSERT(nullptr!=pDictItem);
    //                    int iLength = pDictItem->GetLen();
    //                    if(!pDictItem->GetDecChar()  && pDictItem->GetDecimal() != 0){
    //                        iLength++; //The length does not account for the decimal character in this case
    //                    }

    bool bNewTextBox = pField->UseUnicodeTextBox();
    CONTENT_TYPE_REFACTOR::LOOK_AT("what to do about non-numeric + non-alpha fields?");
    bool bTickMarks = (pDictItem->GetContentType() == ContentType::Alpha && !bNewTextBox) || pDictItem->GetContentType()==ContentType::Numeric; // here check if the field is alpha and it is new textbox

    if (bTickMarks) {
        int iLength = pDictItem->GetLen();
        if(!pDictItem->GetDecChar()  && pDictItem->GetDecimal() != 0) {
            iLength++; //If Decimal character does not go in data file the length does not include the decimal character
        }
        int iLeft = rcFld.left+1; //account for the left border
        int iBottom = rcFld.bottom-1; //account for the bottom border
        int iHeight = rcFld.Height() -2 ;//account for the border

        /*for (int iIndex=0; iIndex < iLength-1; iIndex ++) {
            pDC->MoveTo(iLeft + szChar.cx * (iIndex+1) +(iIndex+1)*2 +iIndex*GRIDSEP_SIZE,iBottom);
            pDC->LineTo(iLeft + szChar.cx * (iIndex+1)+(iIndex+1)*2+iIndex*GRIDSEP_SIZE ,iBottom-iHeight/4);
        }*/
        // 20100708 moved to after the decimal point is drawn

        pDC->SetBkColor(RGB(255,255,255));
        if(pDictItem->GetDecimal() != 0) {

            if( pField->IsProtected() || pField->IsMirror() ) // 20130525 added this condition because the decimal part of a protected field was getting drawn with a white background
                pDC->SetBkColor(GetCurForm()->GetBackgroundColor().ToCOLORREF());

            for(int iIndex =0 ; iIndex <iLength ; iIndex++) {
                //int iX = rect.left + sizeChar.cx * (iIndex) +(2*iIndex + 1)+iIndex*SEP_SIZE;
                int iX = iLeft + szChar.cx * (iIndex) +(2*iIndex + 1)+iIndex*GRIDSEP_SIZE - 1;
                int iY = iBottom - szChar.cy+1;
                if(iIndex == (iLength  - (int)pDictItem->GetDecimal() -1))
                {
                    pDC->TextOut(iX,iY,GetDecimalCharacter());

                    if( bUsingControl ) // 20100708 the decimal point was eliminating some of the border
                    {
                        pDC->MoveTo(iX - 1,iY); // top border
                        pDC->LineTo(iX + szChar.cx,iY);

                        pDC->MoveTo(iX - 1,iBottom + 1); // bottom border
                        pDC->LineTo(iX + szChar.cx,iBottom + 1);
                    }

                    break;
                }
            }
        }

        for (int iIndex=0; iIndex < iLength-1; iIndex ++) {
            pDC->MoveTo(iLeft + szChar.cx * (iIndex+1) +(iIndex+1)*2 +iIndex*GRIDSEP_SIZE,iBottom);
            pDC->LineTo(iLeft + szChar.cx * (iIndex+1)+(iIndex+1)*2+iIndex*GRIDSEP_SIZE ,iBottom-iHeight/4);
        }

    }
    pDC->RestoreDC(iSaveDC);
}


CSize sizeChar ; //Global variable used in the computation

void CFormScrollView::UpdateDims()
{
    CDEFormFile* pFFSpec = &GetDocument()->GetFormFile();
    CDELevel*   pLevel = nullptr;
    CDEGroup*   pGroup = nullptr;
    CDEGroup*   pRoot = nullptr;
    CDC*        pDC = GetDC();
    int         iSaveDC = pDC->SaveDC();

    pDC->SelectObject(pFFSpec->GetFieldFont().GetCFont());

    sizeChar = pDC->GetTextExtent(_T("0"),1);   // 0=the string, 1=# of chars in the str
    int iNumLevels = pFFSpec->GetNumLevels();

    for (int iLevel = 0; iLevel < iNumLevels; iLevel++) {

        pLevel = pFFSpec->GetLevel(iLevel);
        pRoot = pLevel->GetRoot();
        int iNumGrps = pLevel->GetNumGroups();

        for (int iGrp=0; iGrp < iNumGrps; iGrp++){
            pGroup = pLevel->GetGroup (iGrp);
            ComputeDims (pGroup);
        }
    }
    pDC->RestoreDC(iSaveDC);
    ReleaseDC(pDC);
}

void CFormScrollView::ComputeDims (CDEGroup* pGroup)
{
    CDEItemBase* pItem = nullptr;
    CDEFormBase::eItemType   eItem;
    int iNumItem = pGroup->GetNumItems();

    for (int iItem=0; iItem < iNumItem; iItem++){
        pItem = pGroup->GetItem(iItem);
        eItem = pItem->GetItemType();

        //Note : Chris Does for roster and old rosters we dont support
        if (eItem == CDEFormBase::Group /*|| eItem == CDEFormBase::Roster || eItem == CDEFormBase::Roster*/){
            ComputeDims ((CDEGroup*) pItem);
        }
        else {  // it's a field
            CDEField* pField = DYNAMIC_DOWNCAST(CDEField,pItem);
            //Calc rect from the current font and the pDictItem
            if(pField != nullptr){
                ComputeRect (pField);
            }
        }
    }
}

bool CFormScrollView::ComputeRect(CDEField* pField)
{
    bool bRet  = false;
    const CDictItem* pDictItem = pField->GetDictItem();
    if(!pDictItem)
        return bRet;

    int iLength = pDictItem->GetLen();
    if(iLength ==0) {
        return bRet;
    }
    if(!pDictItem->GetDecChar()  && pDictItem->GetDecimal() != 0) {
        iLength++; //If Decimal character does not go in data file the length does not include the decimal character
    }
    CRect clientRect = pField->GetDims();   // this is the dims of the data entry box

    //SAVY &&& CRect textRect = pText->GetDims(): To do the text items 'cos the font is different for each

    // first calculate the size of the data entry box dims

    int iX  = clientRect.left;
    int iY =  clientRect.top;

    int iXB = GetSystemMetrics(SM_CXBORDER);
    int iYB = GetSystemMetrics(SM_CYBORDER);

    int iRight = iX + sizeChar.cx*iLength + 2*iXB;
    int iHeight = sizeChar.cy + 2*iYB;

    // add space for tick marks for all but alpha fields with arabic font

    CONTENT_TYPE_REFACTOR::LOOK_AT("what to do about non-numeric + non-alpha fields?");
    if(pDictItem->GetContentType() != ContentType::Alpha || !pField->GetFont().IsArabic()) {
        iRight += (iLength -1)*SEP_SIZE + 2*iLength;
    }

    if(iRight != clientRect.right) {
        bRet  = true;
        clientRect.right = iRight ;
    }
    if(clientRect.bottom - iY  != iHeight){
        clientRect.bottom = clientRect.top + sizeChar.cy+2*iYB; //account for the border
        bRet = true;
    }
    if(bRet){
        bool bNewTextField =pField->UseUnicodeTextBox();
        CONTENT_TYPE_REFACTOR::LOOK_AT("what to do about non-numeric + non-alpha fields?");
        bool bChangeDims = (pField->GetDictItem()->GetContentType() == ContentType::Alpha && !bNewTextField) || pField->GetDictItem()->GetContentType() == ContentType::Numeric ;
        bool bResetUnicodeTextBoxHeight = (pField->GetDictItem()->GetContentType() == ContentType::Alpha && bNewTextField && !pField->AllowMultiLine());
        if(bChangeDims){
            pField->SetDims(clientRect);
        }
        else if(bResetUnicodeTextBoxHeight){
            CRect textBoxRect = pField->GetDims();
            textBoxRect.bottom = clientRect.bottom;
            pField->SetDims(textBoxRect);
        }
    }

    // next, calculate the size of the accompanying text string
    // (it's not quite right, savy will finish)

    iLength = pField->GetText().GetLength();
    if (iLength == 0)   // the field string could be empty
        return bRet;    // if so, bail

    bool bRet2 = false;
    CDEText& field_text = pField->GetCDEText();
    clientRect = field_text.GetDims();

    CDC* pDC = GetDC();

    CSize field_text_size = field_text.CalculateDimensions(pDC);

    iX  = clientRect.left;
    iY =  clientRect.top;
    iRight = iX + field_text_size.cx;
    // don't need to recalc the height, it will stay the same

    if(iRight != clientRect.right) {
        bRet2 = true;
        clientRect.right = iRight;
    }
    if(clientRect.bottom - iY  != field_text_size.cy){
        clientRect.bottom = clientRect.top + field_text_size.cy;
        bRet2 = true;
    }

    ReleaseDC(pDC);

    if (bRet2)
        pField->SetTextDims(clientRect);

    return bRet || bRet2;
}

//////////////////////////////////////////////////////////////////////////////
//
// the next few funcs work to adjust the spacing
//
// this code blk is the same for both AdjustSpacing() overloads, so do once here!

CDEFormBase::TextLayout CFormScrollView::AdjustSpacingPrep(CFormDoc** pFD, CDEFormFile** pFF)
{
    *pFD = GetDocument();
    ASSERT(*pFD);
    *pFF = &(*pFD)->GetFormFile();

    CDEFormBase::TextLayout eTextLayout = GetDragOptions().GetTextLayout();
    if(eTextLayout == CDEFormBase::TextLayout::Right){
        (*pFF)->UpdatePointers();
        this->UpdateDims();
    }

    CDC* pDC = GetDC();
    int iSaveDC = pDC->SaveDC();
    pDC->SelectObject((*pFF)->GetDefaultTextFont().GetCFont());
    sizeChar = pDC->GetTextExtent(_T("0"),1);
    m_iDropSpacing = sizeChar.cx*2;
    pDC->RestoreDC(iSaveDC);

    return eTextLayout;
}

void CFormScrollView::AdjustFieldSpacing(CDEField* pField, CDEFormBase::TextLayout eTextLayout, int iMaxRPoint)
{
    CRect rect;
    int iWidth;
    if(eTextLayout == CDEFormBase::TextLayout::Left){   // then the text is to the left of the data entry box

        rect = pField->GetDims();
        iWidth = rect.Width();
        rect.left = iMaxRPoint + m_iDropSpacing;
        rect.right = rect.left + iWidth;
        pField->SetDims(rect);
    }
    else if (eTextLayout == CDEFormBase::TextLayout::Right) {   // else the DE box is on the left, txt to the right

        rect = pField->GetTextDims();
        iWidth = rect.Width();
        rect.left = iMaxRPoint + m_iDropSpacing;
        rect.right = rect.left + iWidth;
        pField->SetTextDims(rect);
    }
}

/////////////////////////////////////////////////////////////////////////////////
//
//  called after generate form file call
//
/////////////////////////////////////////////////////////////////////////////////

void CFormScrollView::AdjustSpacing()
{
    CDEFormFile* pFF;
    CFormDoc*    pFD;
    CDEFormBase::TextLayout eTextLayout = AdjustSpacingPrep(&pFD, &pFF);
    int iNumForms = pFF->GetNumForms();
    CDEForm* pForm;
    CDEItemBase* pBase;
    CDEField*    pField;

    for (int iForm = 0;iForm < iNumForms; iForm++){
        pForm = pFF->GetForm(iForm);
        //for each field and its associated text
        int iNumItems = pForm->GetNumItems();
        int iMaxRPoint = GetMaxRPoint(pForm);
        for (int iItem =0; iItem < iNumItems ; iItem++){
            pBase = pForm->GetItem(iItem);
            pField = DYNAMIC_DOWNCAST(CDEField , pBase);
            if (pField)
                AdjustFieldSpacing (pField, eTextLayout, iMaxRPoint);
        }
    }
}

/////////////////////////////////////////////////////////////////////////////////
//
//  Make a call to this function after drag & drop of items from dict tree
//
/////////////////////////////////////////////////////////////////////////////////

void CFormScrollView::AdjustSpacing(const std::vector<CDEField*>& fields)
{
    CDEFormFile* pFF;
    CFormDoc* pFD;
    CDEFormBase::TextLayout eTextLayout = AdjustSpacingPrep(&pFD, &pFF);

    int iMaxRPoint = GetMaxRPoint(fields);

    for( CDEField* pField : fields )
        AdjustFieldSpacing(pField, eTextLayout, iMaxRPoint);
}

/////////////////////////////////////////////////////////////////////////////////
//
//  int CFormScrollView::GetMaxRPoint(CDEForm* pForm)
//
/////////////////////////////////////////////////////////////////////////////////

int CFormScrollView::GetMaxRPoint(CDEForm* pForm)
{
    std::vector<CDEField*> fields;

    for( int i = 0; i < pForm->GetNumItems(); ++i )
    {
        CDEItemBase* pBase = pForm->GetItem(i);

        if( pBase->GetItemType() == CDEFormBase::eItemType::Field )
            fields.emplace_back(assert_cast<CDEField*>(pBase));
    }

    return GetMaxRPoint(fields);
}

/////////////////////////////////////////////////////////////////////////////////

int CFormScrollView::GetMaxRPoint(const std::vector<CDEField*>& fields)
{
    //for each field and its associated text if the fields parent is not a roster
    int iRet = 0;
    CDEFormBase::TextLayout eTextLO = GetDragOptions().GetTextLayout();
    CDEFormFile* pFFSpec = &GetDocument()->GetFormFile();
    ASSERT(pFFSpec);

    CDC* pDC = GetDC();
    int iSaveDC = pDC->SaveDC();

    pDC->SelectObject(pFFSpec->GetDefaultTextFont().GetCFont());

    for( CDEField* pField : fields ) {
        CDEText& text = pField->GetCDEText();
        CRect rect = text.GetDims();
        CSize size = pDC->GetTextExtent(text.GetLabel());

        rect.right = rect.left + size.cx;   // adjust the width
        rect.bottom = rect.top + size.cy;   // adjust the height
        text.SetDims(rect);

        if(eTextLO == CDEFormBase::TextLayout::Left) {
            if(iRet < rect.right){
                iRet = rect.right;
            }
        }
        else if (eTextLO == CDEFormBase::TextLayout::Right) {   // has to be right for now, nothing else avail
            rect = pField->GetDims();
            if(iRet < rect.right){
                iRet = rect.right;
            }
        }
    }

    pDC->RestoreDC(iSaveDC);

    return iRet;
}


/////////////////////////////////////////////////////////////////////////////////
//
//  LRESULT CFormScrollView::OnMarkDictItems(WPARAM wParam,LPARAM lParam)
//
/////////////////////////////////////////////////////////////////////////////////
LRESULT CFormScrollView::OnMarkDictItems(WPARAM wParam, LPARAM /*lParam*/)
{
    const DictTreeNode* dict_tree_node = reinterpret_cast<const DictTreeNode*>(wParam);
    MarkDictTree(dict_tree_node);
    return 1;
}


/////////////////////////////////////////////////////////////////////////////////
//
//  void CFormScrollView::MarkDictTree()
//
// after a drop occurs from the dict on to the form, or when the form frame is
// activated, call this func to change the dict icons, reflecting which items
// have been used on the form and which have not
//
/////////////////////////////////////////////////////////////////////////////////

void CFormScrollView::MarkDictTree(const DictTreeNode* starting_dict_tree_node/* = nullptr*/)
{
    CFormDoc* pFormDoc = GetDocument();
    CDEFormFile* pFormSpec = &pFormDoc->GetFormFile();
    CFormTreeCtrl* pFormTree = pFormDoc->GetFormTreeCtrl();

    // when first starting up the form, there may not be a tree yet
    if( pFormTree == nullptr )
        return;

    CDDTreeCtrl* pDictTree = pFormTree->GetDDTreeCtrl();
    const CDataDict* dictionary = pFormDoc->GetSharedDictionary().get();
    ASSERT(dictionary != nullptr);

    // only mark entities for the current dictionary
    auto is_correct_dictionary = [&](const DictTreeNode& dict_tree_node)
    {
        return ( dict_tree_node.GetDDDoc()->GetDict() == dictionary );
    };

    if( starting_dict_tree_node != nullptr && !is_correct_dictionary(*starting_dict_tree_node) )
        return;

    // don't draw the tree while changing the icons
    pDictTree->SetRedraw(false);

    TreeCtrlHelpers::IterateOverVisibleNodes(*pDictTree, ( starting_dict_tree_node != nullptr ) ? starting_dict_tree_node->GetHItem() : pDictTree->GetRootItem(),
        [&](HTREEITEM hItem)
        {
            const DictTreeNode* dict_tree_node = pDictTree->GetTreeNode(hItem);
            ASSERT(dict_tree_node->GetHItem() == hItem);

            // only mark items and subitems
            if( dict_tree_node->GetDictElementType() == DictElementType::Item )
            {
                bool used = is_correct_dictionary(*dict_tree_node) ? pFormSpec->FindItem(WS2CS(dict_tree_node->GetName())) : false;

                int image_index = dict_tree_node->IsSubitem() ? ( used ? DictIcon::SubitemUsed : DictIcon::Subitem ) :
                                                                ( used ? DictIcon::ItemUsed : DictIcon::Item );

                pDictTree->SetItemImage(hItem, image_index, image_index);
            }
        });

   pDictTree->SetRedraw(true);
   pDictTree->Invalidate();
   pDictTree->UpdateWindow();
}


/////////////////////////////////////////////////////////////////////////////////
//
//  eItemType CFormScrollView::GetPropItemTypeFromTrackers()
//
/////////////////////////////////////////////////////////////////////////////////
CDEFormBase::eItemType CFormScrollView::GetPropItemTypeFromTrackers()
{
    int iNumTrackers = GetNumTrackers();
    CDEFormBase::eItemType eRetType = CDEFormBase::UnknownItem;
    CFormTracker track;

    for (int iIndex=0; iIndex < iNumTrackers;iIndex++){
        track = GetTracker(iIndex);
        if ( track.IsBox()/* ||
            (track.IsFldTxt() && !track.IsFldBoxSel())*/ ){
            continue;
        }

        if(track.IsFldTxt()){
            eRetType = CDEFormBase::Text;
            break;
        }
        CDEItemBase* pBase = GetCurForm()->GetItem(track.GetIndex());
        if(pBase->GetItemType() == CDEFormBase::Text) {
            eRetType = CDEFormBase::Text;
            break;
        }
    }

    return eRetType;
}

void CFormScrollView::OnGlobalfont()
{
    //invoke the dialog and set the font
    CGlobalFDlg dlgGlobal;
    dlgGlobal.m_lfDefault = PortableFont::TextDefault;
    dlgGlobal.m_pFormView = this;
    CDEFormFile* pFF = GetFormFile();
    dlgGlobal.m_lfCurrentFont =  pFF->GetDefaultTextFont();
    dlgGlobal.m_lfSelectedFont = dlgGlobal.m_lfCurrentFont;
    dlgGlobal.m_iFont =1;

    CFormDoc* pDoc = GetDocument();

    if(dlgGlobal.DoModal() == IDOK){
        pDoc->SetModifiedFlag(TRUE);
        if(dlgGlobal.m_iFont == 0 ) {
            pFF->SetDefaultTextFont(dlgGlobal.m_lfDefault);
        }
        else if(dlgGlobal.m_iFont == 1) {
            pFF->SetDefaultTextFont(dlgGlobal.m_lfSelectedFont);
        }
    }
}

/////////////////////////////////////////////////////////////////////////////////
//
//  void CFormScrollView::ChangeFont(const PortableFont& font) //apply to all items
//
/////////////////////////////////////////////////////////////////////////////////
void CFormScrollView::ChangeFont(const PortableFont& font)
{
    CDEFormFile* pFFSpec = &GetDocument()->GetFormFile();
    pFFSpec->SetDefaultTextFont(font);

    int iNumLevels = pFFSpec->GetNumLevels();

    for (int iLevel = 0; iLevel < iNumLevels; iLevel++) {

        CDELevel* pLevel = pFFSpec->GetLevel(iLevel);
        int iNumGrps = pLevel->GetNumGroups();

        for (int iGrp=0; iGrp < iNumGrps; iGrp++){
            CDEGroup* pGroup = pLevel->GetGroup (iGrp);
            ChangeFont(pGroup, font);
        }
    }

    //Go through all the forms and set the text items to this default font
    int iNumForms = pFFSpec->GetNumForms();
    for(int iForm=0; iForm < iNumForms;iForm++){
        CDEForm* pForm = pFFSpec->GetForm(iForm);
        int iNumItems = pForm->GetNumItems();
        for(int iIndex=0;iIndex<iNumItems;iIndex++){
            if(pForm->GetItem(iIndex)->GetItemType() == CDEFormBase::Text){
                CDEText* pText =DYNAMIC_DOWNCAST(CDEText,pForm->GetItem(iIndex));
                pText->SetUseDefaultFont(true);
                pText->SetFont(font);
            }
        }
    }
    Invalidate();
    UpdateWindow();
}

/////////////////////////////////////////////////////////////////////////////////
//
//  void CFormScrollView::ChangeFont(CDEGroup* pGroup, const PortableFont& font)
//
/////////////////////////////////////////////////////////////////////////////////
void CFormScrollView::ChangeFont(CDEGroup* pGroup, const PortableFont& font)
{
    int iNumItem = pGroup->GetNumItems();

    for (int iItem=0; iItem < iNumItem; iItem++) {
        CDEItemBase* pItem = pGroup->GetItem(iItem);
        CDEFormBase::eItemType eItem = pItem->GetItemType();

        if(eItem == CDEFormBase::Roster) {
            CFormGrid* pGrid = FindGrid(((CDERoster*)pItem));
            if(pGrid) {
                pGrid->ChangeFont(font);
            }
            else {
                ChangeFont((CDERoster*)pItem, font);
            }
        }
        else if (eItem == CDEFormBase::Group) {
            ChangeFont((CDEGroup*) pItem, font);
        }
        else if (eItem == CDEFormBase::Field) {
            CDEField* pField = DYNAMIC_DOWNCAST(CDEField,pItem);
            pField->GetCDEText().SetUseDefaultFont(true);
            pField->GetCDEText().SetFont(font);
        }
        else {
            ASSERT(eItem == CDEFormBase::Block);
        }
    }
}

/////////////////////////////////////////////////////////////////////////////////
//
//  void CFormScrollView::DeselectTracker(CPoint point)
//
/////////////////////////////////////////////////////////////////////////////////
void CFormScrollView::DeselectTracker(CPoint point)
{
    int iNumTrackers = m_aTrackerItems.GetSize();
    for(int iIndex =0; iIndex<iNumTrackers;iIndex++){
        CFormTracker rectTracker = m_aTrackerItems[iIndex];
        if(rectTracker.GetRect().PtInRect(point)){
            m_aTrackerItems.RemoveAt(iIndex);
            break;
        }
    }

}


/////////////////////////////////////////////////////////////////////////////////
//
//void CFormScrollView::ChangeFont(CDERoster* pRoster, const PortableFont& font)
//
/////////////////////////////////////////////////////////////////////////////////

void CFormScrollView::ChangeFont(CDERoster* pRoster, const PortableFont& font)
{
    ASSERT(pRoster);
    ASSERT((int)pRoster->GetStubTextSet().GetNumTexts() == pRoster->GetMaxLoopOccs());

    // build header row
    for (int iCol=0 ; iCol<pRoster->GetNumCols() ; iCol++)  {
        CDECol* pCol = pRoster->GetCol(iCol);
        pCol->GetHeaderText().SetUseDefaultFont(true);
        pCol->GetHeaderText().SetFont(font);
    }

    // add cells (w/o stub text)...
    for (int iCol=0 ; iCol<pRoster->GetNumCols() ; iCol++)  {
        CDECol* pCol = pRoster->GetCol(iCol);
        CDECell& column_cell = pCol->GetColumnCell();
        for( CDEText& text : column_cell.GetTextSet().GetTexts() ) {
            text.SetUseDefaultFont(true);
            text.SetFont(font);
        }
        for (int i = 0 ; i<pCol->GetNumFields() ; i++)  {
            CDEField* pFld = pCol->GetField(i);
            pFld->GetCDEText().SetUseDefaultFont(true);
            pFld->GetCDEText().SetFont(font);
        }
    }
    for (int iRow=0 ; iRow<pRoster->GetMaxLoopOccs() ; iRow++)  {
        // set stub text (the only thing that differs btwn rows)
        pRoster->GetStubTextSet().GetText(iRow).SetUseDefaultFont(true);
        pRoster->GetStubTextSet().GetText(iRow).SetFont(font);
    }


    // free cells
    for( CDEFreeCell& free_cell : pRoster->GetFreeCells() )
    {
        for( CDEText& text : free_cell.GetTextSet().GetTexts() )
        {
            text.SetUseDefaultFont(true);
            text.SetFont(font);
        }
    }
}


void CFormScrollView::OnEditUndo()
{
    CFormDoc* pFD = GetDocument();

    if( pFD->GetUndoStack().CanUndo() )
        pFD->UndoChange(true);
}

void CFormScrollView::OnUpdateEditUndo(CCmdUI* pCmdUI)
{
    pCmdUI->Enable(GetDocument()->GetUndoStack().CanUndo());
}

void CFormScrollView::OnEditRedo()
{
    CFormDoc* pFD = GetDocument();

    if( pFD->GetUndoStack().CanRedo() )
        pFD->RedoChange();
}

void CFormScrollView::OnUpdateEditRedo(CCmdUI* pCmdUI)
{
    pCmdUI->Enable(GetDocument()->GetUndoStack().CanRedo());
}

//  Description.
//      When the user does copy. The selected Items will be written in a file.
//      Then the file will be opened and all the data will be put in the clipboard.using fnc(FiletoClip)
//      While retriving It will read the clipboard and Write back tha file then it will read
//      make the file.
void CFormScrollView::OnEditCopy()
{
    CFormDoc* pDoc = assert_cast<CFormDoc*>(GetDocument());

    // Get levels to copy
    CFormGrid* pGrid = GetGridWhereSelected();
    if (pGrid != nullptr)
    {
        pGrid->SendMessage(WM_COMMAND, ID_EDIT_COPYF);
        return;
    }
     // Copy levels to clipboard
    CSpecFile clipFile;

    CString csClipFile = pDoc->GetClipFile();
    clipFile.Open(csClipFile, CFile::modeWrite);

    CFormTreeCtrl*  pFrmTree = pDoc->GetFormTreeCtrl();
    if (GetFocus( ) == pFrmTree)
    {
//      pFrmTree->SendMessage(WM_COMMAND,ID_EDIT_COPY);
        HTREEITEM origTI = pFrmTree->GetSelectedItem();
        CFormID*    pNodeID = (CFormID*) pFrmTree->GetItemData (origTI);

        eNodeType   nodeType = pNodeID->GetItemType();
        if (nodeType == eFTT_FORM)
        {
            CDELevel* pLevel = GetCurLevel();
            CDEGroup* pGroup = pLevel->GetGroupOnForm ( GetFormIndex() );

            CDEForm* pForm = GetCurForm();
            pForm->Save(clipFile);
            pGroup->Save(clipFile);

            clipFile.Close();
            UINT uFormat = pDoc->GetClipBoardFormat(FD_FORM_FORMAT);
            pDoc->FileToClip(uFormat);
            return;
        }
        else if (nodeType == eFTT_BLOCK)
        {
            //save all the fields of the current block
            //and save the block as well .
            CDEBlock& form_block = assert_cast<CDEBlock&>(*pNodeID->GetItemPtr());
            CDEGroup* pParent = form_block.GetParent();
            std::vector<CDEField*> block_fields = pParent->GetBlockFields(form_block);
            bool isGridField = pParent->IsKindOf(RUNTIME_CLASS(CDERoster));

            for (CDEField* pField : block_fields){
                pField->Save(clipFile, isGridField);
            }
            form_block.Save(clipFile);
            clipFile.Close();
            UINT uFormat = pDoc->GetClipBoardFormat(FD_TEXT_FORMAT);
            pDoc->FileToClip(uFormat);
            return;
        }
    }

    CDEForm* pForm = GetCurForm();

    for( int i = 0; i < GetNumTrackers(); ++i )
    {
        const CFormTracker& tracker = GetTracker(i);

        // is the user is trying to delete the text portion of a data entry box?
        if (tracker.IsFldTxt())
        {
            // if only the field's text was selected, then the user wants
            // to clear the text; if both the box and text were selected,
            // then the field will be deleted and no biggie if text got deleted

            if ( !tracker.IsFldBoxSel() )  // if the data entry box wasn't selected...
            {
                CDEField* pField = (CDEField*)pForm->GetItem(tracker.GetIndex());

                if (pField == nullptr) // will happen when box got deleted
                    continue;

                const CDEText& text = pField->GetCDEText();

                clipFile.WriteString(CString(_T("[Text]\r\n")));
                text.Save(clipFile, false);
            }
        }

        else if( tracker.IsBox() )
        {
            const CDEBox& box = pForm->GetBoxSet().GetBox(tracker.GetIndex());
            clipFile.PutLine(_T("[Box]"));
            clipFile.PutLine(FRM_CMD_BOX, box.GetSerializedText());
        }

        else
        {
            CDEItemBase* pItem = pForm->GetItem(tracker.GetIndex());

            if (pItem->IsKindOf(RUNTIME_CLASS(CDEText)))
                clipFile.PutLine(_T("[Text]"));

            pItem->Save(clipFile,false);
            // this if/then blk is for undo/redo
        }
    }

    SetFocus();
    clipFile.Close();
    UINT uFormat = pDoc->GetClipBoardFormat(FD_TEXT_FORMAT);
    pDoc->FileToClip(uFormat);
}


/////////////////////////////////////////////////////////////////////////////////
//
//      CFormScrollView::OnEditPaste()
//
/////////////////////////////////////////////////////////////////////////////////

void CFormScrollView::OnEditPaste()
{
    CSpecFile clipFile;
    CFormDoc* pDoc = assert_cast<CFormDoc*>(GetDocument());
    if (IsClipboardFormatAvailable(pDoc->GetClipBoardFormat(FD_TEXT_FORMAT)))
    {
        UINT uFormat = pDoc->GetClipBoardFormat(FD_TEXT_FORMAT);
        pDoc->ClipToFile(uFormat);
    }
    else if (IsClipboardFormatAvailable(pDoc->GetClipBoardFormat(FD_COLUMN_FORMAT)))
    {
        UINT uFormat = pDoc->GetClipBoardFormat(FD_COLUMN_FORMAT);
        pDoc->ClipToFile(uFormat);
    }
    else if (IsClipboardFormatAvailable(pDoc->GetClipBoardFormat(FD_FORM_FORMAT)))
    {
        UINT uFormat = pDoc->GetClipBoardFormat(FD_FORM_FORMAT);
        pDoc->ClipToFile(uFormat);
    }
    else
    {
        return;
    }
    CString csClipFile = pDoc->GetClipFile();
    clipFile.Open(csClipFile, CFile::modeRead);

    m_cAddRFTPoint = CPoint(0,0); // Set the reference point to zero zero

    CDEFormFile* pFF = GetFormFile();
    CDEGroup*       pGroup = GetCurGroup();
    CIMSAString     sName;
    CFormTracker    tracker;

    CIMSAString csCmd;    // the string command  (left side of =)
    CIMSAString csArg;    // the string argument (right side of =)

    CString sErrMsg;

    int iBlockPos = -1;
    int iFormNum = GetFormIndex();
    FormUndoStack form_undo_stack;

    if (IsClipboardFormatAvailable(pDoc->GetClipBoardFormat(FD_TEXT_FORMAT)))
    {
        RemoveAllTrackers();

        while (clipFile.GetLine(csCmd, csArg) == SF_OK)         // csc 5/5/97
        {
            //if (clipFile.GetLine(csCmd,csArg);
            if (csCmd == _T("[Block]"))
            {
                CDEBlock*   pBlock = new CDEBlock();
                pBlock->Build(clipFile,false);
                iBlockPos = iBlockPos == -1 ? 0 : iBlockPos;

                if (iBlockPos != 0)
                    pGroup->InsertItemAt(pBlock, iBlockPos);
                else
                    pGroup->AddItem(pBlock);

                pBlock->SetFormNum(pGroup->GetFormNum());
                form_undo_stack.PushUndoObj(CFormUndoObj::Action::UR_add, pBlock, pGroup->GetItemIndex(pBlock), pGroup->GetName());

            }

            else if (csCmd == _T("[Text]"))
            {
                CDEText*    pText = new CDEText();
                pText->Build(clipFile);
                //if (pText->Build(clipFile))
                CRect rect = pText->GetDims() + CPoint(10,10);
                pText->SetDims(rect);
                pText->SetParent(GetCurGroup());
                pText->SetFormNum(GetFormIndex());
                GetCurForm()->AddItem(pText);
                GetDocument()->SetModifiedFlag(true);//use this in the !m_bAddRFT branch
                int x = GetCurForm()->GetNumItems();

                // if the label of an field is copied and pasted, its name is blank, so let's give it one
                // so that CreateUniqueName doesn't give it a name like 000, 001, etc.
                if( pText->GetName().GetLength() == 0 )
                    pText->SetName(_T("TEXT"));

                pText->SetName (pFF->CreateUniqueName (pText->GetName()));  // can't use dict name if not a keyed field

                //CRect rect = pText->GetDims();
                UpdateTrackerRectsAndRefresh();
                AddTracker(rect,x-1); // only the text portion was selected
                form_undo_stack.PushUndoObj(CFormUndoObj::Action::UR_add, pText, GetCurForm()->GetItemIndex (pText), GetCurForm()->GetName());

            }

            else if (csCmd == _T("[Field]"))
            {
                CFormTreeCtrl*  pFormTree = pDoc->GetFormTreeCtrl();

                if (pFormTree == nullptr)
                {
                    clipFile.Close(); // 20120621
                    return;     // when first starting up the form, won't have a tree yet, so test
                }
                CDEField* pField = new CDEField();
                pField->Build(clipFile);

                DictTreeNode* dict_tree_node = GetDictTreeNode(pField->GetItemName());
                if (dict_tree_node != nullptr)
                {
                    CPoint adjustedDP = CPoint(0,0);            // first assign the drop point to the adjusted
                    m_cRules.AnalyzeDrop (dict_tree_node, adjustedDP, GetCurForm(), pDoc);

                    //////////////////////////////////////////////

                    if (m_cRules.IllegalDropOcc())  // result of drop was illegal drop
                    {
                        CString sMsg = m_cRules.GetErrorMsg();
                        AfxMessageBox (sMsg);
                        delete pField;
                        clipFile.Close(); // 20120621
                        return;
                    }

                    const CDataDict* pDict = pDoc->GetSharedDictionary().get();
                    const CDictItem* pItem = pDict->LookupName<CDictItem>(pField->GetItemName());

                    CPoint dropPoint = pField->GetDims().TopLeft();

                    CRect rect = pField->GetDims() + CPoint(10,10);
                    CRect recttxt = pField->GetCDEText().GetDims() + CPoint(10,10);
                    pField->GetCDEText().SetDims(recttxt) ;
                //
                    pField->SetDims(rect);
                    pField->SetDictItem(pItem);
                    //GetCurForm()->FindItem(csFName);
                    pField->SetFormNum(GetFormIndex());
                    pField->GetCDEText().SetFormNum(GetFormIndex());
                    pField->SetParent(GetCurGroup());
                    if (m_cRules.GetDropResult() == DropAsMirror)
                    {
                        pField->IsMirror (true);
                        pField->SetName (pFF->CreateUniqueName (pField->GetItemName()));    // can't use dict name if not a keyed field
                    }

                    GetCurForm()->AddItem(pField);
                    GetCurGroup()->AddItem(pField);
                    if (iBlockPos == -1) {//store the blockpos where the new block is to be added if the paste involves a block. block is written to the end of the clipboard
                        iBlockPos = GetCurGroup()->GetNumItems() - 1;
                    }
                    GetDocument()->SetModifiedFlag(true);//use this in the !m_bAddRFT branch
                    UpdateTrackerRectsAndRefresh();
                    CRect rect1;
                    rect1.UnionRect(rect,recttxt);
                    int x = GetCurForm()->GetNumItems();
                    AddTracker(x-1,rect); // only the text portion was selected
                    if(pField->GetLabel().IsEmpty()&& pItem){
                        pField->SetLabel(pItem->GetLabel());
                    }
                    form_undo_stack.PushUndoObj(CFormUndoObj::Action::UR_add, pField, GetCurGroup()->GetUpperBound(), GetCurForm()->GetName());

                }
                else
                {
                    CString msg;
                    msg.Format(_T("Field %s not found in Dictionary "), pField->GetItemName().GetString());
                    AfxMessageBox(msg);
                    delete pField;
                    clipFile.Close(); // 20120621
                    return;
                }

            }

            else if (csCmd == _T("[Box]"))
            {
                clipFile.GetLine(csCmd, csArg);

                auto box = std::make_shared<CDEBox>(csArg);

                // move the box a bit so that it does not completely overlap a potentially existing box
                box->GetDims() += CPoint(10, 10);

                CDEForm* pForm = GetCurForm();

                size_t box_index = pForm->GetBoxSet().GetNumBoxes();
                pForm->GetBoxSet().AddBox(box);

                AddTrackerT<CDEBox>(box->GetDims(), box_index);

                form_undo_stack.PushUndoObj(CFormUndoObj::Action::UR_add, box.get(), box_index, pForm->GetName());
            }
        }
    }

    else if (IsClipboardFormatAvailable(pDoc->GetClipBoardFormat(FD_COLUMN_FORMAT)))
    {
        CDEBlock* pCurrentBlock = nullptr;
        while (clipFile.GetLine(csCmd, csArg) == SF_OK)         // csc 5/5/97
        {
            //if (clipFile.GetLine(csCmd,csArg);
            if (csCmd == _T("[Block]"))
            {
                pCurrentBlock = new CDEBlock();
                pCurrentBlock->Build(clipFile, false);
            }
            else if (csCmd == _T("[Column]"))
            {
                CDECol* pCol = new CDECol();
                pCol->Build(clipFile,pGroup);
                CDEField* pField = pCol->GetField(0);
                const CDataDict* pDict = pDoc->GetSharedDictionary().get();
                const CDictRecord* pRec;
                const CDictItem* pItem;
                pDict->LookupName(pField->GetItemName(), nullptr, &pRec, &pItem);
                pField->SetDictItem(pItem);
                if(pField->GetLabel().IsEmpty()&& pItem){
                    pField->SetLabel(pItem->GetLabel());
                }
                DictTreeNode* dict_tree_node = GetDictTreeNode(pField->GetItemName());
                if (dict_tree_node != nullptr)
                {
                    CPoint adjustedDP = CPoint(0,0);            // first assign the drop point to the adjusted
                    m_cRules.AnalyzeDrop (dict_tree_node, adjustedDP, GetCurForm(), pDoc);

                    if (m_cRules.IllegalDropOcc())  // result of drop was illegal drop
                    {
                        CString sMsg = m_cRules.GetErrorMsg();
                        AfxMessageBox (sMsg);
                        delete pCol;
                        clipFile.Close(); // 20120621
                        return;
                    }

                    //GetCurForm()->FindItem(csFName);
                    pField->SetParent(GetCurGroup());
                    bool flagnew = false;
                    CDERoster* pRoster = nullptr;
                    CFormGrid*  pGrid = GetGridWhereSelected();

                    if (pGrid == nullptr)
                    {
                        RosterOrientation roster_orientation = FormDefaults::RosterOrientation;
                        if (GetNumGrids()>0)
                        {
                            CFormGrid*  pfirstGrid = GetGrid(0);
                            roster_orientation = pfirstGrid->GetRoster()->GetOrientation();
                        }
                        pRoster = new CDERoster();
                        pRoster->SetRightToLeft(pFF->GetRTLRostersFlag());
                        flagnew = true;

                        if (pCurrentBlock) {//add the block first as they are written out first in the clipboard
                            pRoster->AddItem(pCurrentBlock);
                            pCurrentBlock->SetFormNum(iFormNum);
                            form_undo_stack.PushUndoObj(CFormUndoObj::Action::UR_add, pCurrentBlock, pRoster->GetItemIndex(pCurrentBlock), pRoster->GetName());
                            pCurrentBlock = nullptr;
                        }
                        pRoster->AddItem(pField);
                        pRoster->SetOrientation(roster_orientation);
                        pFF->CreateRoster(pRoster, pRec, iFormNum, CPoint(0,0), GetDragOptions(), false);

                        pRoster->SetParent (GetCurGroup());
                        GetCurGroup()->AddItem(pRoster);
                        GetCurForm()->AddItem(pRoster);

                        pRoster->AddCol (pCol);
                        pRoster->FinishFieldInit();
                        pRoster->UpdateFlagsNFonts(*pFF);
                        CreateGrid(pRoster);
                        pGrid = FindGrid(pRoster);
                        pGrid->BuildGrid();  // rebuilds underlying structure from CDERoster
                        pGrid->RecalcLayout (CSize(NONE,NONE), false); // recalcs where objs go
                        CRect origRect = pRoster->GetDims();

                        RemoveAllTrackers();
                        pGrid->Resize (origRect);    // change viewport of grid
                        form_undo_stack.PushUndoObj(CFormUndoObj::Action::UR_add, pRoster, pRoster->GetParent()->GetItemIndex(pRoster), pRoster->GetParent()->GetName());
                    }
                    else
                    {
                        pRoster = pGrid->GetRoster();
                        form_undo_stack.PushUndoObj(CFormUndoObj::Action::UR_modify, pRoster, pRoster->GetParent()->GetItemIndex(pRoster), pRoster->GetParent()->GetName());

                        if (pCurrentBlock) {//add the block first as they are written out first in the clipboard
                            pRoster->AddItem(pCurrentBlock);
                            pCurrentBlock->SetFormNum(pRoster->GetFormNum());
                            form_undo_stack.PushUndoObj(CFormUndoObj::Action::UR_add, pCurrentBlock, pRoster->GetItemIndex(pCurrentBlock), pRoster->GetName());
                            pCurrentBlock = nullptr;
                        }

                        pRoster->AddCol(pCol);
                        pRoster->FinishFieldInit();
                        pRoster->AddItem(pCol->GetField(0));   // CDEGroup nds to see it too (for Serpro)
                        pRoster->UpdateFlagsNFonts(*pFF);

                        CPoint ptScroll(GetScrollPos(SB_HORZ), GetScrollPos(SB_VERT));
                        pRoster->SetDims(pRoster->GetDims() - ptScroll);// csc 1/11/01

                        pGrid->BuildGrid();  // rebuilds underlying structure from CDERoster
                        pGrid->RecalcLayout (CSize(NONE,NONE), false); // recalcs where objs go
                        CRect origRect = pRoster->GetDims();
                        int cols = 0, rows = 0;
                        bool bUseOldDims = false;
                        if (pRoster->GetOrientation() == RosterOrientation::Horizontal)
                        {
                            cols = pRoster->GetNumCols();

                            if (cols > MIN_NUM_ROSTER_COLS)
                                bUseOldDims = true;
                            else
                                rows = std::min (pRoster->GetMaxLoopOccs()+1, MIN_NUM_ROSTER_ROWS);      // +1 csc 11/16/00
                        }
                        else
                        {
                            rows = pRoster->GetNumCols();

                            if (rows > MIN_NUM_ROSTER_COLS)
                                bUseOldDims = true;
                            else
                                cols = std::min (pRoster->GetMaxLoopOccs()+1, MIN_NUM_ROSTER_ROWS);       // +1 csc 11/16/00
                        }
                        if (bUseOldDims)
                            pGrid->Resize(origRect);    // change viewport of grid
                        else
                            pGrid->Resize (rows, cols);    // change viewport of grid

                        pGrid->SelectColumn(pGrid->GetNumCols()-1,true);
                    }
                    if (flagnew)
                    {
                        int x = GetCurForm()->GetNumItems();
                        CFormTracker track;
                        track.m_rect = pRoster->GetDims();//pGrid->GetClientRect();
                        track.SetIndex(x-1);
                        track.SetBox (false);
                        AddTracker(track); // the Item's in the bounding rectangle, flag it
                    }
                }
                else
                {
                    CString msg;
                    msg.Format(_T("Field %s not found in Dictionary"), pField->GetItemName().GetString());
                    AfxMessageBox(msg);
                    delete pCol;
                    clipFile.Close(); // 20120621
                    return;
                }
            }
            else if (csCmd == _T("[Grid]"))
            {
                if (GetGridWhereSelected() == nullptr)
                {
                    CDERoster* pRoster = new CDERoster();
                    pRoster->SetRightToLeft(pFF->GetRTLRostersFlag());

                    pRoster->Build(clipFile,true);
                    pRoster->SetParent (GetCurGroup());
                    pRoster->SetFormName(GetCurForm()->GetName());
                    pRoster->SetFormNum(GetFormIndex());

                    for (int j = 1 ; j < pRoster->GetNumCols(); j++)
                    {
                        const CDataDict* pDict = pDoc->GetSharedDictionary().get();
                        CDEField* pField = pRoster->GetCol(j)->GetField(0);
                        const CDictItem* pItem = pDict->LookupName<CDictItem>(pField->GetItemName());
                        pField->SetDictItem(pItem);
                        if(pField->GetLabel().IsEmpty()&& pItem){
                            pField->SetLabel(pItem->GetLabel());
                        }
                        DictTreeNode* dict_tree_node = GetDictTreeNode(pField->GetItemName());

                        if (dict_tree_node != nullptr)
                        {
                            CPoint adjustedDP = CPoint(0,0);            // first assign the drop point to the adjusted
                            m_cRules.AnalyzeDrop (dict_tree_node, adjustedDP, GetCurForm(), pDoc);

                            if (m_cRules.IllegalDropOcc())  // result of drop was illegal drop
                            {
                                CString sMsg = m_cRules.GetErrorMsg();
                                AfxMessageBox (sMsg);
                                delete pRoster;
                                clipFile.Close(); // 20120621
                                return;
                            }
                        }
                        else
                        {
                            CString msg;
                            msg.Format(_T("Field %s not found in Dictionary "), pField->GetItemName().GetString());
                            AfxMessageBox(msg);
                            delete pRoster;
                            clipFile.Close(); // 20120621
                            return;
                        }
                    }
                    GetCurGroup()->FinishFormInit4Rosters (GetCurForm(), pRoster);
                    GetCurGroup()->AddItem(pRoster);      // add the item to the page

                    GetCurForm()->AddItem(pRoster);

                    pRoster->UpdateFlagsNFonts(*pFF);
                    CreateGrid(pRoster);
                    CFormGrid*  pGrid = FindGrid(pRoster);
                    pGrid->BuildGrid();  // rebuilds underlying structure from CDERoster
                    pGrid->RecalcLayout (CSize(NONE,NONE), false); // recalcs where objs go
                    CRect origRect = pRoster->GetDims();

                    int x = GetCurForm()->GetNumItems();
                    RemoveAllTrackers();
                    CFormTracker track;
                    track.m_rect = pRoster->GetDims();//pGrid->GetClientRect();
                    track.SetIndex(x-1);
                    track.SetBox (false);
                    AddTracker(track); // the Item's in the bounding rectangle, flag it
                    //pGrid->SelectColumn(pGrid->GetNumCols()-1,true);
                    pGrid->Resize (origRect);    // change viewport of grid

                    form_undo_stack.PushUndoObj(CFormUndoObj::Action::UR_add, pRoster, pRoster->GetParent()->GetItemIndex(pRoster), pRoster->GetParent()->GetName());
                }
                else
                {
                    CDERoster* pRoster = new CDERoster();
                    pRoster->SetRightToLeft(pFF->GetRTLRostersFlag());
                    CFormGrid*  pGrid = GetGridWhereSelected();//WhereTrackerSelected();
                    CDERoster* pRostermrg = pGrid->GetRoster();
                    form_undo_stack.PushUndoObj(CFormUndoObj::Action::UR_modify, pRostermrg, pRostermrg->GetParent()->GetItemIndex(pRostermrg), pRostermrg->GetParent()->GetName());

                    pRoster->Build(clipFile,true);
                    pRoster->SetParent (GetCurGroup());
                    pRoster->SetFormName(GetCurForm()->GetName());
                    pRoster->SetFormNum(GetFormIndex());

                    for (int j = 1 ; j < pRoster->GetNumCols(); j++)
                    {
                        CDECol* pCol = new CDECol(*pRoster->GetCol(j));
                        const CDataDict* pDict = pDoc->GetSharedDictionary().get();
                        ASSERT( pCol != 0 );
                        CDEField* pField = pCol->GetField(0);
                        const CDictItem* pItem = pDict->LookupName<CDictItem>(pField->GetItemName());
                        if(pField->GetLabel().IsEmpty()&& pItem){
                            pField->SetLabel(pItem->GetLabel());
                        }
                        pField->SetDictItem(pItem);
                        DictTreeNode* dict_tree_node = GetDictTreeNode(pField->GetItemName());

                        if (dict_tree_node != nullptr)
                        {
                            CPoint adjustedDP = CPoint(0,0);            // first assign the drop point to the adjusted
                            m_cRules.AnalyzeDrop (dict_tree_node, adjustedDP, GetCurForm(), pDoc);

                            if (m_cRules.IllegalDropOcc())  // result of drop was illegal drop
                            {
                                CString sMsg = m_cRules.GetErrorMsg();
                                AfxMessageBox (sMsg);
                                delete pRoster;
                                clipFile.Close(); // 20120621
                                return;
                            }
                        }
                        else
                        {
                            CString s;
                            s.Format(_T("Field %s not found in Dictionary "), pField->GetItemName().GetString());
                            AfxMessageBox(s);
                            delete pRoster;
                            clipFile.Close(); // 20120621
                            return;
                        }
                        pRostermrg->AddCol (pCol);
                        for (int iField = 0; iField < pCol->GetNumFields(); ++iField)
                        {
                            CDEField* pFldCpy = assert_cast<CDEField*>(pCol->GetField(iField)->Clone().release());
                            pRostermrg->AddItem(pFldCpy);
                            pCol->SetFieldAt(pFldCpy, iField);
                        }
                        pRostermrg->UpdateFlagsNFonts(*pFF);
                    }


                    CPoint ptScroll(GetScrollPos(SB_HORZ), GetScrollPos(SB_VERT));
                    pRostermrg->SetDims(pRostermrg->GetDims() - ptScroll);// csc 1/11/01
                    pGrid->BuildGrid();  // rebuilds underlying structure from CDERoster
                    pGrid->RecalcLayout (CSize(NONE,NONE), false); // recalcs where objs go
                    CRect origRect = pRostermrg->GetDims();
                    int cols = 0, rows = 0;
                    bool bUseOldDims = false;
                    if (pRostermrg->GetOrientation() == RosterOrientation::Horizontal)
                    {
                        cols = pRostermrg->GetNumCols();

                        if (cols > MIN_NUM_ROSTER_COLS)
                            bUseOldDims = true;
                        else
                            rows = std::min (pRostermrg->GetMaxLoopOccs()+1, MIN_NUM_ROSTER_ROWS);      // +1 csc 11/16/00
                    }
                    else
                    {
                        rows = pRostermrg->GetNumCols();

                        if (rows > MIN_NUM_ROSTER_COLS)
                            bUseOldDims = true;
                        else
                            cols = std::min (pRostermrg->GetMaxLoopOccs()+1, MIN_NUM_ROSTER_ROWS);       // +1 csc 11/16/00
                    }
                    if (bUseOldDims)
                        pGrid->Resize(origRect);    // change viewport of grid
                    else
                        pGrid->Resize (rows, cols);    // change viewport of grid

                    delete pRoster;
                }
            }
        }
    }
    else if (IsClipboardFormatAvailable(pDoc->GetClipBoardFormat(FD_FORM_FORMAT)))
    {
        //pFF->RenumberFormsAndItems();
        MyRenumberForms();
        while (clipFile.GetLine(csCmd, csArg) == SF_OK)         // csc 5/5/97
        {
            if (csCmd == _T("[Form]"))
            {
                CDEForm* pForm = new CDEForm();
                const CDataDict* pDict = pDoc->GetSharedDictionary().get();
                pForm->Build(clipFile,pDict->GetName());
                int count = 0;
                while (!pFF->IsNameUnique(pForm->GetName()))
                {
                    CString msg;
                    msg.Format(_T("FORM%03d"),count);
                    pForm->SetName(msg);
                    count++;
                }
                pFF->AddUniqueName(pForm->GetName());

                pGroup = new CDEGroup();
                CDELevel* pLevel = GetCurLevel();

                while (clipFile.GetLine(csCmd, csArg) == SF_OK)         // csc 5/5/97
                {
                    if (csCmd == _T("[Group]"))
                    {
                        pForm->SetGroup(pGroup);
                        //MyRenumberForms();
                        pGroup->SetFormName(pForm->GetName());
                        //pGroup->SetFormNum(pFF->GetNumForms());

                        pGroup->Build(clipFile,pFF, true, pForm);
                        //pForm->AddGroupItems(pGroup);
                    }
                }
                if (!CanAddForm(pForm))
                {
                    //pFF->RemoveForm(index);
                    //pLevel->RemoveGroup(pLevel->GetNumGroups()-1);

                    //                  pGroup->RemoveAllItems();
                    delete pGroup;
                    delete pForm;
                    clipFile.Close(); // 20120621
                    return;
                }
                CString cUniqueName = pGroup->GetName();
                int count1 = 1;
                while (!pFF->IsNameUnique(cUniqueName))
                {
                    cUniqueName.Format( _T("%s%d"), pGroup->GetName().GetString(), count1);
                    count1++;
                }
                if (pGroup->GetName().CompareNoCase(cUniqueName)!=0)
                {
                    CString msg;
                    msg.Format(_T("Form name already exists.\n Pasted form renamed from \" %s \" to \" %s \""), pGroup->GetName().GetString(), cUniqueName.GetString());
                    AfxMessageBox(msg);
                    pGroup->SetName(cUniqueName);
                }
                pFF->AddUniqueName(pGroup->GetName());

                pGroup->SetParent(pLevel->GetRoot());
                pLevel->AddGroup(pGroup);
                pFF->AddForm(pForm);

                MyRenumberForms();
                RemoveAllGrids();              // [1]


                CFormTreeCtrl* pFTC = pDoc->GetFormTreeCtrl();
                pFTC->ReBuildTree();
                pFTC->Invalidate();
                pFTC->UpdateWindow();
                SetFocus(); // leave focus w/the view, not the tree

                form_undo_stack.PushUndoObj(CFormUndoObj::Action::UR_add, pGroup, pLevel->GetNumGroups()-1, pGroup->GetParent()->GetName()); // copy e'thing under the group
                form_undo_stack.PushUndoObj(CFormUndoObj::Action::UR_add, pForm, pFF->GetNumForms()-1, pFF->GetName());  // only copy the form's boxes & FST
            }
        }
    }

    if (form_undo_stack.GetNumURObjs() > 0) { // if something got dropped, then push the stack
        GetDocument()->PushUndo(std::move(form_undo_stack));
    }

    clipFile.Close();
    Invalidate();
    UpdateWindow();

    CArray<CFormTracker,CFormTracker> arrTrackerItems;
    arrTrackerItems.Append(this->m_aTrackerItems);
    pDoc->GetFormTreeCtrl()->ReBuildTree(GetFormIndex());
    MarkDictTree();
    GetDocument()->SetModifiedFlag(true);//use this in the !m_bAddRFT branch

    SetFocus();
    if(m_aTrackerItems.GetCount() == 0 && arrTrackerItems.GetCount() > 0){
        m_aTrackerItems.Append(arrTrackerItems);
    }
}

void CFormScrollView::OnUpdateEditPaste(CCmdUI* pCmdUI)
{
    bool flgclip = false;
    CFormDoc* pDoc = assert_cast<CFormDoc*>(GetDocument());
    flgclip = flgclip ? flgclip: IsClipboardFormatAvailable(pDoc->GetClipBoardFormat(FD_TEXT_FORMAT));
    flgclip = flgclip ? flgclip: IsClipboardFormatAvailable(pDoc->GetClipBoardFormat(FD_COLUMN_FORMAT));
    flgclip = flgclip ? flgclip: IsClipboardFormatAvailable(pDoc->GetClipBoardFormat(FD_BOX_FORMAT));
    flgclip = flgclip ? flgclip: IsClipboardFormatAvailable(pDoc->GetClipBoardFormat(FD_FORM_FORMAT));
    pCmdUI->Enable(flgclip);
}

DictTreeNode* CFormScrollView::GetDictTreeNode(const CString& name)
{
    CFormDoc* pDoc = assert_cast<CFormDoc*>(GetDocument());
    const CDataDict* pDict = pDoc->GetSharedDictionary().get();
    const DictLevel* pLevel;
    const CDictRecord* pRec;
    const CDictItem* pItem;
    const DictValueSet* pVSet;
    CDDTreeCtrl* pDictTree = pDoc->GetFormTreeCtrl()->GetDDTreeCtrl();
    DictTreeNode* dict_tree_node = nullptr;
    if (pDict->LookupName(name,&pLevel,&pRec, &pItem,&pVSet))
    {
        //HITEM hItem = pDictTree->GetRootItem();
        HTREEITEM hNode = pDictTree->GetRootItem();
        dict_tree_node = pDictTree->GetTreeNode(hNode);

        while (hNode != nullptr)
        {
            HTREEITEM hLevel = pDictTree->GetChildItem(hNode);
            int lev = 0;
            while (hLevel != nullptr)
            {
                if (pLevel == &pDict->GetLevel(lev))
                {
                    int rec = 0;
                    HTREEITEM hRecord = pDictTree->GetChildItem(hLevel);
                    dict_tree_node = pDictTree->GetTreeNode(hLevel);
                    while (hRecord != nullptr) // Kopa kabana
                    {
                        const CDictRecord* ptempRecord = nullptr;
                        if (rec == 0)
                            ptempRecord = pLevel->GetIdItemsRec();
                        else
                            ptempRecord = pLevel->GetRecord(rec-1);
                        if (pRec && ptempRecord == pRec)
                        {
                            dict_tree_node = pDictTree->GetTreeNode(hRecord);
                            HTREEITEM hItem = pDictTree->GetChildItem(hRecord);
                            int item = 0;
                            while( hItem != nullptr)
                            {
                                const CDictItem* ptempItem = ptempRecord->GetItem(item);
                                if (pItem  && ptempItem == pItem )
                                {
                                    dict_tree_node = pDictTree->GetTreeNode(hItem);
                                    return dict_tree_node;
                                }
                                hItem = pDictTree->GetNextSiblingItem(hItem);
                                item++;
                            }
                        }
                        hRecord = pDictTree->GetNextSiblingItem(hRecord);
                        rec++;
                    }
                }
                lev++;
                hLevel = pDictTree->GetNextSiblingItem(hLevel);
            }
        }
    }
    return dict_tree_node;
}

void CFormScrollView::OnPrepareDC(CDC* pDC, CPrintInfo* pInfo)
{
    CSize szTotal (INIT_ScrSz, INIT_ScrSz);
    CDEForm* pForm = GetCurForm();
    if (pForm == nullptr){
        SetScrollSizes(MM_TEXT, szTotal);
    }
    CScrollView::OnPrepareDC(pDC, pInfo);
}

CFormGrid* CFormScrollView::GetGridWhereSelected()
{
    CFormTracker tracker;
    int max = GetNumTrackers();
    CFormGrid*  pGrid = nullptr;
    for (int i = 0; i < max; i++)
    {
        tracker = GetTracker(i);
        int index = tracker.GetIndex();
        CDEItemBase* pItem = GetCurForm()->GetItem(index);
        if (pItem != nullptr && pItem->GetItemType() == CDEFormBase::Roster)
        {
            return FindGrid((CDERoster*)pItem);
        }
    }
    max = GetNumGrids();
    for (int i = 0; i < max; i++)
    {
        pGrid = GetGrid(i);
        if (pGrid->GetNumTrackers()>0)//pGrid->IsCellSelected(row,col))
            return pGrid;

        for (int row = 0; row < pGrid->GetNumRows(); row++)
        {
            if (pGrid->IsRowSelected(row))
                return pGrid;
        }

        for (int col = 0; col < pGrid->GetNumCols(); col++)
        {
            if (pGrid->IsColSelected(col))
                return pGrid;
        }
    }
    return nullptr;
}

void CFormScrollView::OnEditCut()
{
    OnEditCopy();

    CFormDoc* pDoc = assert_cast<CFormDoc*>(GetDocument());
    CFormTreeCtrl* pFrmTree = pDoc->GetFormTreeCtrl();
    HTREEITEM origTI = pFrmTree->GetSelectedItem();

    //if the current selected item is a block - delete this block now as the clipboard is ready
    if( origTI != nullptr ) {
        if (((CFormID*)pFrmTree->GetItemData(origTI))->GetItemType() == eFTT_BLOCK) {
            pFrmTree->DeleteActiveItem(origTI);
            return;
        }
    }

    DeleteActiveItem();
}


void CFormScrollView::MakeFormCurrent(int iFormIndex)
{
    if (iFormIndex != GetFormIndex())
    {
        GetDocument()->SetCurFormIndex(iFormIndex);
        SetFormIndex(iFormIndex);
        RemoveAllGrids();
        RecreateGrids (iFormIndex);
        Invalidate();
    }
}


void CFormScrollView::MyRenumberForms()
{
    CDEFormFile* pFF = GetFormFile();
//  CDEGroup*       pGroup = GetCurGroup();
    int i , max = pFF->GetNumForms();
    for (i = 0; i < max; i++)
    {
        CDEForm* pForm = pFF->GetForm(i);
        CDEGroup* pGroup = pForm->GetGroup();
        pGroup->SetFormNum(i);
        pGroup->RenumberFormsAndItems(pFF,i);

        // int maxitm = pForm->GetNumItems();
        pForm->RenumberItems(i);
        /*
        CDEItemBase* pItem;
        for (int j=0; j < maxitm; j++)      // add in all the uniq names for the text items
        {
            pItem = pForm->GetItem(j);
            pItem->SetFormNum(i);
        }
        */
    }
}

bool CFormScrollView::CanAddForm(CDEForm* pForm)
{
    CFormDoc* pDoc = assert_cast<CFormDoc*>(GetDocument());
    for (int i = 0; i < pForm->GetNumItems(); i++)
    {
        CDEItemBase* pItem = pForm->GetItem(i);

        if (pItem->GetItemType() == CDEFormBase::Field )
        {
            CDEField* pField = assert_cast<CDEField*>(pItem);
            DictTreeNode* dict_tree_node = GetDictTreeNode(pField->GetItemName());
            if (dict_tree_node != nullptr)
            {
                CPoint adjustedDP = CPoint(0,0);            // first assign the drop point to the adjusted
                m_cRules.AnalyzeDrop (dict_tree_node, adjustedDP, GetCurForm(), pDoc);

                //////////////////////////////////////////////

                if (m_cRules.IllegalDropOcc())  // result of drop was illegal drop
                {
                    CString sMsg = m_cRules.GetErrorMsg();
                    AfxMessageBox (sMsg);
                    return false;
                }
                if (m_cRules.GetDropResult() == DropAsMirror)
                {
                    pField->IsMirror (true);
                    pField->SetName (GetFormFile()->CreateUniqueName (pField->GetItemName()));    // can't use dict name if not a keyed field
                }
            }
            else
            {
                CString sMsg = _T("Field not found in Dictionary");
                AfxMessageBox (sMsg);
                return false;
            }

        }
        else if (pItem->GetItemType() == CDEFormBase::Roster)
        {
            CDERoster* pRoster = (CDERoster*)pItem;
            for (int j = 1 ; j < pRoster->GetNumCols(); j++)
            {
                CDEField* pField = pRoster->GetCol(j)->GetField(0);
                DictTreeNode* dict_tree_node = GetDictTreeNode(pField->GetItemName());
                if(dict_tree_node == nullptr)
                {
                    AfxMessageBox (_T("Field not found in Dictionary"));
                    return false;
                }

                CPoint adjustedDP = CPoint(0,0);            // first assign the drop point to the adjusted
                m_cRules.AnalyzeDrop (dict_tree_node, adjustedDP, GetCurForm(), pDoc);

                if (m_cRules.IllegalDropOcc())  // result of drop was illegal drop
                {
                    CString sMsg = m_cRules.GetErrorMsg();
                    AfxMessageBox (sMsg);
                    return false;
                }
            }

        }
    }

    return true;
}


void CFormScrollView::OnFieldFont()
{
    CDEFormFile* pFF = GetFormFile();
    FieldFontDlg dlg(pFF->GetFieldFont());

    if( dlg.DoModal() != IDOK )
        return;

    pFF->SetFieldFont(dlg.GetFieldFont());

    GetDocument()->SetModifiedFlag(true);
    RemoveAllGrids();          // trash any grids that were created in this view
    RemoveAllTrackers();
    RecreateGrids (GetFormIndex());
    Invalidate();
    UpdateWindow();
}


void CFormScrollView::OnFieldColors()
{
    CDEFormFile* pFF = GetFormFile();

    FieldColorsDlg field_colors_dlg(*pFF);

    if( field_colors_dlg.DoModal() != IDOK || pFF->GetFieldColors() == field_colors_dlg.GetFieldColors() )
        return;

    pFF->SetFieldColors(field_colors_dlg.GetFieldColors());

    GetDocument()->SetModifiedFlag(true);
}



void CFormScrollView::OnLayoutSpaceEvenly(UINT nID)
{
    struct SPACING
    {
        CRect rect;
        int itxt;
        int ifld;
        int index;
    };

    bool horizontal = ( nID == ID_LAYOUT_HORIZONTALLY_SPACE_EVENLY );

    if( GetNumTrackers() > 100 )
    {
        AfxMessageBox(_T("Too many items selected; cannot space evenly."));
        return;
    }

    UpdateTrackerSh();

    CFormDoc* pDoc = GetDocument();
    CDEForm* pForm = GetCurForm();
    std::vector<SPACING> spacing;

    // Make a pair of fields in an array to space it evenly
    for (int i = 0; i < GetNumTrackers(); i++)
    {
        const CFormTracker& tracker = GetTracker(i);
        int index = tracker.GetIndex();

        if (tracker.IsFldTxt())
        {
            if ( (tracker.IsFldBoxSel()))  // if the data entry box wasn't selected...
            {
                CDEField* pField = (CDEField*)pForm->GetItem(index);
                SPACING sp;
                CRect rectTracker = tracker.GetRect();
                CRect rectFieldDims = pField->GetDims();
                sp.rect.UnionRect(&rectTracker, &rectFieldDims);
                int fld = -1;
                for (int x = 0; x < GetNumTrackers(); x++)
                {
                    if (x!=i && GetTracker(x).GetIndex() == index)
                    {
                        fld = x;
                        break;
                    }

                }
                sp.ifld = fld;
                sp.itxt = i;
                sp.index = index;
                spacing.emplace_back(sp);
                continue;
            }
            else
            {
                SPACING sp;
                sp.rect = tracker.GetRect();
                sp.itxt = i;
                sp.ifld = -1;
                sp.index = index;
                spacing.emplace_back(sp);
                continue;
            }
        }

        else if( tracker.IsBox() )
        {
            continue;
        }

        else
        {
            CDEItemBase* pItem = pForm->GetItem(index);
            if (pItem->IsKindOf(RUNTIME_CLASS(CDEText)))
            {
                SPACING sp;
                sp.rect = pItem->GetDims();
                sp.itxt = i;
                sp.ifld = -1;
                sp.index = index;
                spacing.emplace_back(sp);
                continue;
            }
            else if (pItem->IsKindOf(RUNTIME_CLASS(CDEField)))
            {
                bool found = false;
                for (int x = 0; x < GetNumTrackers(); x++)
                {
                    if (x != i && GetTracker(x).GetIndex() == index)
                    {
                        found = true;
                        break;
                    }
                }
                if (!found)
                {
                    SPACING sp;
                    sp.rect = pItem->GetDims();
                    sp.ifld = i;
                    sp.itxt = -1;
                    sp.index = index;
                    spacing.emplace_back(sp);
                    continue;
                }
            }
            else
            {
                continue;
            }
        }
    }


    if( spacing.size() < 3 )
        return;

    // sort the spacing
    std::sort(spacing.begin(), spacing.end(),
        [&](const SPACING& s1, const SPACING& s2)
        {
            return horizontal ? ( s1.rect.left < s2.rect.left ) :
                                ( s1.rect.top < s2.rect.top );
        });

    LONG total_gap = 0;

    for( size_t i = 1; i < spacing.size(); ++i )
    {
        total_gap += horizontal ? ( spacing[i].rect.left - spacing[i - 1].rect.right ) :
                                  ( spacing[i].rect.top - spacing[i - 1].rect.bottom );
    }

    LONG gap = total_gap / (int)( spacing.size() - 1 );

    // since gap is an integer, it can occasionally lead to the last item having a noticably larger gap than
    // the other items, so we'll increment the gap midway through the list
    size_t point_at_which_to_increment_gap = spacing.size() - ( total_gap % ( spacing.size() - 1 ) );

    FormUndoStack form_undo_stack;
    CPoint ptScroll(GetScrollPos(SB_HORZ), GetScrollPos(SB_VERT));

    LONG left_or_top_pos = horizontal ? spacing.front().rect.right :
                                        spacing.front().rect.bottom;
    LONG new_pos = left_or_top_pos;

    for( size_t i = 0; i < spacing.size(); ++i ) // first and last one is not to be moved
    {
        if( i == point_at_which_to_increment_gap )
            ++gap;

        bool on_last_entry = ( ( i + 1 ) == spacing.size() );

        if( i != 0 && !on_last_entry )
        {
            left_or_top_pos = new_pos;
            new_pos = 0;
        }

        CDEItemBase* pItem = pForm->GetItem(spacing[i].index);
        CDEItemBase* pTxtItem = nullptr;
        CDEItemBase* pFieldItem = nullptr;

        if (pItem->IsKindOf(RUNTIME_CLASS(CDEField)))
        {
            pFieldItem = pItem;
            if (spacing[i].itxt >=0)
                pTxtItem = &assert_cast<CDEField*>(pItem)->GetCDEText();
        }

        else if (pItem->IsKindOf(RUNTIME_CLASS(CDEText)))
        {
            pTxtItem = pItem;
        }

        auto adjust_rect = [&](CRect& rect)
        {
            if( horizontal )
            {
                int left = left_or_top_pos + gap + ( rect.left - spacing[i].rect.left );
                rect.right = left + rect.Width();
                new_pos = std::max(new_pos, rect.right);
                rect.left = left;
            }

            else
            {
                int top = left_or_top_pos + gap + ( rect.top - spacing[i].rect.top );
                rect.bottom = top + rect.Height();
                new_pos = std::max(new_pos, rect.bottom);
                rect.top = top;
            }
        };

        if (pTxtItem != nullptr)  // If the text is selected then move the text
        {
            form_undo_stack.PushMoveObj(pTxtItem, spacing[i].index, pForm->GetName());

            if( i == 0 || on_last_entry )
            {
                if (pFieldItem!=nullptr)
                {
                    int iFieldIndex = pFieldItem->GetParent()->FindItem(pFieldItem->GetName());
                    form_undo_stack.PushMoveObj(pFieldItem, iFieldIndex, pFieldItem->GetParent()->GetName());
                }

                continue;
            }

            CRect rect = pTxtItem->GetDims();
            adjust_rect(rect);

            pTxtItem->SetDims(rect);
            CFormTracker tracker = GetTracker(spacing[i].itxt);
            rect -= ptScroll;
            tracker.SetRect(rect);// Adjust the scroll pos
            SetTrackerAt(spacing[i].itxt,tracker);
        }

        if (pFieldItem != nullptr && spacing[i].ifld != -1)
        {
            int iFieldIndex = pFieldItem->GetParent()->FindItem(pFieldItem->GetName());
            form_undo_stack.PushMoveObj(pFieldItem, iFieldIndex, pFieldItem->GetParent()->GetName());

            if( i == 0 || on_last_entry )
                continue;

            CRect rect = pFieldItem->GetDims();
            adjust_rect(rect);

            pFieldItem->SetDims(rect);
            CFormTracker tracker = GetTracker(spacing[i].ifld);
            rect -= ptScroll;
            tracker.SetRect(rect);// Adjust the scroll pos
            SetTrackerAt(spacing[i].ifld,tracker);
        }
    }

    if( form_undo_stack.GetNumURObjs() == 0 ) // user had only selected boxes
        return;

    GetDocument()->PushUndo(std::move(form_undo_stack));

    RedrawWindow();
    pDoc->SetModifiedFlag(true);
    RefreshTrackers();
}


void CFormScrollView::OnUpdateLayoutSpaceEvenly(CCmdUI* pCmdUI)
{
    pCmdUI->Enable(GetNumTrackers() > 2);
}


void CFormScrollView::OnLayoutHorizontallyCenter()
{
    if( AlignRosterColumns(HorizontalAlignment::Center) ) // 20111114
        return;

    CFormDoc* pDoc = GetDocument();

    int i, index, max = GetNumTrackers();
    CDEForm*        pForm = GetCurForm();
    CDEItemBase*    pItem  = nullptr;
    CDEField*       pField = nullptr;
    CIMSAString     sName;
    CFormTracker    tracker;
    int ifldpos = -1;
    int itxtpos = -1;
    CRect winrect;
    GetClientRect(&winrect);
    int icentrex = winrect.Width()/2;
    bool bSomethingToAlign = false;

    if (GetNumTrackers() <=0)
        return;

    FormUndoStack form_undo_stack;
    CRect rselection(0,0,0,0);
    for (i = 0; i < max; i++)
    {
        tracker = GetTracker(i);
        index = tracker.GetIndex();
        rselection.UnionRect(rselection,tracker.GetRect());

    }
    if (rselection.Width() > 0)
        icentrex = rselection.left + rselection.Width()/2;

    CPoint ptScroll(GetScrollPos(SB_HORZ), GetScrollPos(SB_VERT));
    for (i = 0; i < max; i++)
    {
        tracker = GetTracker(i);
        index = tracker.GetIndex();

        // is the user is trying to delete the text portion of a data entry box?

        if (tracker.IsFldTxt())
        {
            // if only the field's text was selected, then the user wants
            // to clear the text; if both the box and text were selected,
            // then the field will be deleted and no biggie if text got deleted

            if ( (tracker.IsFldBoxSel()))  // if the data entry box wasn't selected...
            {
                pField = (CDEField*)(pForm->GetItem(index));

                if (pField == nullptr) // will happen when box got deleted
                    continue;

                CDEText* pText = &pField->GetCDEText();

                CDEGroup* pParent = pField->GetParent();    // stand-alone field or roster field?
                int iFieldIndex = pParent->FindItem(pField->GetName());

                form_undo_stack.PushMoveObj(pText, index, pForm->GetName());
                CRect trect = pText->GetDims();
                CRect frect = pField->GetDims();
                if (max == 2 )
                    itxtpos =  trect.left  + icentrex - ((trect.left + trect.right ) / 2);
                else
                    itxtpos =  trect.left  + icentrex - ((trect.left + frect.right ) / 2);
                ifldpos = frect.left + icentrex - ((trect.left + frect.right ) / 2);

                trect.right = itxtpos + trect.Width();
                trect.left = itxtpos;
                frect.right = ifldpos + frect.Width();
                frect.left = ifldpos;
//              form_undo_stack.PushMoveObj(pText, index, pForm->GetName());
                pText->SetDims(trect);
                tracker.SetRect(trect-ptScroll);
                SetTrackerAt(i,tracker);
                if (max == 2 ) continue;
                form_undo_stack.PushMoveObj(pField, iFieldIndex, pField->GetParent()->GetName());
                pField->SetDims(frect);
                bSomethingToAlign = true;
                for (int j = 0; j < max;j++)
                {
                    CFormTracker ftracker = GetTracker(j);
                    //index = tracker.GetIndex();
                    if (index == ftracker.GetIndex() && i != j)
                    {
                        ftracker.SetRect(frect-ptScroll);
                        SetTrackerAt(j,ftracker);
                    }
                }
            }
            else
            {
                pField = (CDEField*)(pForm->GetItem(index));

                if (pField == nullptr) // will happen when box got deleted
                    continue;

                CDEText* pText = &pField->GetCDEText();
                CRect rect = pText->GetDims();
                itxtpos = rect.left + icentrex - ((rect.left + rect.right ) / 2);
                rect.right = itxtpos + rect.Width();
                rect.left = itxtpos;
                form_undo_stack.PushMoveObj(pText, index, pForm->GetName());
                pText->SetDims(rect);
                tracker.SetRect(rect-ptScroll);
                SetTrackerAt(i,tracker);
                bSomethingToAlign = true;
            }
        }
        else if( tracker.IsBox() )
        {
//          pForm->RemoveBox (index);
//          UpdateTrackerBoxIndices(i+1, index);
        }
        else
        {
            pItem = pForm->GetItem(index);

            // this if/then blk is for undo/redo
            if (pItem->IsKindOf(RUNTIME_CLASS(CDEText)))
            {
                CDEText* pText = assert_cast<CDEText*>(pItem);
                CRect rect = pText->GetDims();
                itxtpos = rect.left + icentrex - ((rect.left + rect.right ) / 2);
                rect.right = itxtpos + rect.Width();
                rect.left = itxtpos;
                form_undo_stack.PushMoveObj(pText, index, pForm->GetName());
                pText->SetDims(rect);
                tracker.SetRect(rect-ptScroll);
                SetTrackerAt(i,tracker);
                bSomethingToAlign = true;
            }
            else if (pItem->IsKindOf(RUNTIME_CLASS(CDEField)))
            {
                bool bDone = false;
                for (int j = 0; j < max;j++)
                {
                    CFormTracker ftracker = GetTracker(j);
                    //index = tracker.GetIndex();
                    if (index == ftracker.GetIndex() && i != j && ftracker.IsFldTxt())
                    {
                        bDone = true;
                        break;
                    }
                }
                if (bDone && max!=2) continue;

                pField = assert_cast<CDEField*>(pItem);
                CRect trect = pField->GetCDEText().GetDims();

                CDEGroup* pParent = pField->GetParent();    // stand-alone field or roster field?
                int iFieldIndex = pParent->FindItem(pField->GetName());
                form_undo_stack.PushMoveObj(pField, iFieldIndex, pField->GetParent()->GetName());


                CRect frect = pField->GetDims();
//              if (frect != tracker.GetRect())
//              {
//                  tracker.SetRect(frect);
//                  SetTrackerAt(i,tracker);
//                  continue;
//              }
                ifldpos = frect.left + icentrex - ((frect.left + frect.right ) / 2);
                frect.right = ifldpos + frect.Width();
                frect.left = ifldpos;
                pField->SetDims(frect);
                tracker.SetRect(frect-ptScroll);
                SetTrackerAt(i,tracker);
                bSomethingToAlign = true;
            }
            else
            {
                continue;
            }
        }
    }

    if (form_undo_stack.GetNumURObjs() == 0)    // user had only selected boxes
        return;

    GetDocument()->PushUndo(std::move(form_undo_stack));

    SetFocus();                 // leave focus w/the form
    RedrawWindow();
    pDoc->SetModifiedFlag(true);
    RefreshTrackers();
}

void CFormScrollView::OnUpdateLayoutHorizontallyCenter(CCmdUI* pCmdUI)
{
    pCmdUI->Enable(GetNumTrackers() > 1 || CheckRosterColumnAlignmentEnable()); // 20111114 to allow for the alignment of fields within rosters

}


void CFormScrollView::OnLayoutVerticallyCenter()
{
    if( AlignRosterColumns(VerticalAlignment::Middle) ) // 20111114
        return;

    CFormDoc* pDoc = GetDocument();

    int i, index, max = GetNumTrackers();
    CDEForm*        pForm = GetCurForm();
    CDEItemBase*    pItem;
    CDEField*       pField;
    CIMSAString     sName;
    CFormTracker    tracker;
    int ifldpos = -1;
    int itxtpos = -1;
    CRect winrect;
    GetClientRect(&winrect);
    int icentrey = winrect.Height()/2;

    if (GetNumTrackers()<=0)
        return;

    FormUndoStack form_undo_stack;
    CRect rselection(0,0,0,0);
    for (i = 0; i < max; i++)
    {
        tracker = GetTracker(i);
        index = tracker.GetIndex();
        rselection.UnionRect(rselection,tracker.GetRect());

    }
    if (rselection.Height() > 0)
    {

        icentrey = GetScrollPos(SB_VERT) + rselection.top + rselection.Height()/2;
    }

    CPoint ptScroll(GetScrollPos(SB_HORZ), GetScrollPos(SB_VERT));
    for (i = 0; i < max; i++)
    {
        tracker = GetTracker(i);
        index = tracker.GetIndex();

        // is the user is trying to delete the text portion of a data entry box?

        if (tracker.IsFldTxt())
        {
            // if only the field's text was selected, then the user wants
            // to clear the text; if both the box and text were selected,
            // then the field will be deleted and no biggie if text got deleted

            if ( (tracker.IsFldBoxSel()))  // if the data entry box wasn't selected...
            {
                pField = (CDEField*)(pForm->GetItem(index));

                if (pField == nullptr) // will happen when box got deleted
                    continue;

                CDEText* pText = &pField->GetCDEText();

                CDEGroup* pParent = pField->GetParent();    // stand-alone field or roster field?
                int iFieldIndex = pParent->FindItem(pField->GetName());


                form_undo_stack.PushMoveObj(pText, index, pForm->GetName());
                CRect trect = pText->GetDims();
                CRect frect = pField->GetDims();
                if (max == 2)
                    itxtpos =  trect.top  + icentrey - ((trect.top + trect.bottom ) / 2);
                else
                    itxtpos =  trect.top  + icentrey - ((trect.top + frect.bottom ) / 2);
                ifldpos = frect.top + icentrey - ((trect.top + frect.bottom ) / 2);

                trect.bottom = itxtpos + trect.Height();
                trect.top = itxtpos;
                frect.bottom = ifldpos + frect.Height();
                frect.top = ifldpos;
                pText->SetDims(trect);
                tracker.SetRect(trect-ptScroll);
                SetTrackerAt(i,tracker);
                if (max==2) continue;
                form_undo_stack.PushMoveObj(pField, iFieldIndex, pField->GetParent()->GetName());
                pField->SetDims(frect);
                for (int j = 0; j < max;j++)
                {
                    CFormTracker ftracker = GetTracker(j);
                    //index = tracker.GetIndex();
                    if (index == ftracker.GetIndex() && i != j)
                    {
                        ftracker.SetRect(frect-ptScroll);
                        SetTrackerAt(j,ftracker);
                    }
                }
            }
            else
            {
                pField = (CDEField*)(pForm->GetItem(index));

                if (pField == nullptr) // will happen when box got deleted
                    continue;

                CDEText* pText = &pField->GetCDEText();
                CRect rect = pText->GetDims();
                itxtpos = rect.top + icentrey - ((rect.top + rect.bottom ) / 2);
                rect.bottom = itxtpos + rect.Height();
                rect.top = itxtpos;
                form_undo_stack.PushMoveObj(pText, index, pForm->GetName());
                pText->SetDims(rect);
                tracker.SetRect(rect-ptScroll);
                SetTrackerAt(i,tracker);
            }
        }
        else if( tracker.IsBox() )
        {
//          pForm->RemoveBox (index);
//          UpdateTrackerBoxIndices(i+1, index);
        }
        else
        {
            pItem = pForm->GetItem(index);

            // this if/then blk is for undo/redo
            if (pItem->IsKindOf(RUNTIME_CLASS(CDEText)))
            {
                CDEText* pText = assert_cast<CDEText*>(pItem);
                CRect rect = pText->GetDims();
                itxtpos = rect.top + icentrey - ((rect.top + rect.bottom ) / 2);
                rect.bottom = itxtpos + rect.Height();
                rect.top = itxtpos;
                form_undo_stack.PushMoveObj(pText, index, pForm->GetName());
                pText->SetDims(rect);
                tracker.SetRect(rect-ptScroll);
                SetTrackerAt(i,tracker);
            }
            else if (pItem->IsKindOf(RUNTIME_CLASS(CDEField)))
            {
                bool bDone = false;
                for (int j = 0; j < max;j++)
                {
                    CFormTracker ftracker = GetTracker(j);
                    //index = tracker.GetIndex();
                    if (index == ftracker.GetIndex() && i != j && ftracker.IsFldTxt())
                    {
                        bDone = true;
                        break;
                    }
                }
                if (bDone && max!=2) continue;
                pField = assert_cast<CDEField*>(pItem);
                CDEGroup* pParent = pField->GetParent();    // stand-alone field or roster field?
                int iFieldIndex = pParent->FindItem(pField->GetName());
                form_undo_stack.PushMoveObj(pField, iFieldIndex, pField->GetParent()->GetName());

                CDEText* pText = &pField->GetCDEText();

                CRect trect = pText->GetDims();
                CRect frect = pField->GetDims();
                ifldpos = frect.top + icentrey - ((frect.top + frect.bottom ) / 2);
                frect.bottom = ifldpos + frect.Height();
                frect.top = ifldpos;
                pField->SetDims(frect);
                tracker.SetRect(frect-ptScroll);
                SetTrackerAt(i,tracker);
            }
            else
            {
                continue;
            }
        }
    }
    if (form_undo_stack.GetNumURObjs() == 0)    // user had only selected boxes
        return;

    GetDocument()->PushUndo(std::move(form_undo_stack));

    SetFocus();                 // leave focus w/the form
    RedrawWindow();
    pDoc->SetModifiedFlag(true);
    RefreshTrackers();
}

void CFormScrollView::OnUpdateLayoutVerticallyCenter(CCmdUI* pCmdUI)
{
    pCmdUI->Enable(GetNumTrackers() > 1 || CheckRosterColumnAlignmentEnable()); // 20111114 to allow for the alignment of fields within rosters
}


///New Roster 2 stuff for 3D
//  ====================================================================
//  the user has dragged a single item from a multiple record and they
//  want (or are forced) to roster it

CDERoster* CFormScrollView::DropAnItemRoster(DictTreeNode* dict_tree_node, CPoint dropPoint)
{
    int iLevel = dict_tree_node->GetLevelIndex();   // level is 0-based
    int iRec   = dict_tree_node->GetRecordIndex();  // -2 signifies LevelID rec, otherwise begs w/0
    int iItem  = dict_tree_node->GetItemIndex();    // item is 0-based

    int iFormNum = GetFormIndex();

    const CDataDict* pDD = GetDocument()->GetSharedDictionary().get();
    const CDictRecord* pDR = pDD->GetLevel(iLevel).GetRecord(iRec);
    const CDictItem* pDI = pDR->GetItem(iItem);

    CDC* pDC = GetDC();

    CDEFormFile* pFF = GetFormFile();

    //  init the new roster first, but DON'T build the roster using the record

    CDERoster* pRoster = new CDERoster ();
    pRoster->SetRightToLeft(pFF->GetRTLRostersFlag());
    pFF->CreateRoster(pRoster, pDR, iFormNum, dropPoint, GetDragOptions(), false);

    pRoster->SetParent (GetCurGroup());


    CDEField* pField = new CDEField (pDI, iFormNum, pFF->GetDictionaryName(), pDC, GetDragOptions());

    pFF->AddUniqueName(pField->GetName());
    pFF->CreateRosterField (pField,pRoster);

    CDEGroup*   pGroup = GetCurGroup ();
    CDEForm*    pForm = GetCurForm();

    pGroup->AddItem(pRoster);
    pForm->AddItem(pRoster);

    CreateGrid(pRoster);

    return  pRoster;
}


void CFormScrollView::OnShiftF10()
{
    CWnd* pWnd = GetFocus();
    if (pWnd->IsKindOf(RUNTIME_CLASS(CFormTreeCtrl))) {
        pWnd->PostMessage(WM_COMMAND, ID_SHIFT_F10);
    }
}


// 20111114 it was possible to select columns from different roster at the same time
void CFormScrollView::RemoveSelectionsFromOtherRosters(CFormGrid* pSelectedRoster)
{
    for( int i = 0; i < m_aCSProGrid.GetSize(); i++ )
    {
        if( m_aCSProGrid[i] != pSelectedRoster )
        {
            m_aCSProGrid[i]->DeselectColumns();
            m_aCSProGrid[i]->DeselectRows();
            m_aCSProGrid[i]->DeselectCells();
            m_aCSProGrid[i]->DeselectTrackers();
        }
    }
}


void CFormScrollView::OnProperties() // 20120612 this is triggered by pressing Alt + Enter
{
    CDEForm* pForm = GetCurForm();
    CDEItemBase* pFirstSelectedItem = nullptr;

    if( GetNumTrackers() )
    {
        bool bHasFieldText = false;
        bool bHasBox = false;
        bool bHasText = false;
        bool bHasField = false;
        bool bHasRoster = false;

        for( int i = 0; i < GetNumTrackers(); i++ ) // see what's been selected
        {
            CFormTracker tracker = GetTracker(i);
            int index = tracker.GetIndex();

            if( tracker.IsFldTxt() )
            {
                bHasFieldText = true;

                if( !pFirstSelectedItem )
                {
                    CDEItemBase* pLocalItem = pForm->GetItem(index);
                    CDEField* pField = DYNAMIC_DOWNCAST(CDEField,pLocalItem);
                    pFirstSelectedItem = &pField->GetCDEText();
                }
            }

            else if( tracker.IsBox() )
                bHasBox = true; // do nothing

            else
            {
                CDEItemBase* pItem = pForm->GetItem(index);

                if( pItem->IsKindOf(RUNTIME_CLASS(CDEText)) )           bHasText = true;
                else if( pItem->IsKindOf(RUNTIME_CLASS(CDEField)) )     bHasField = true;
                else if( pItem->IsKindOf(RUNTIME_CLASS(CDERoster)) )    bHasRoster = true;

                if( !pFirstSelectedItem )
                    pFirstSelectedItem = pItem;
            }
        }


        if( ( bHasFieldText || bHasText ) && !bHasBox && !bHasField && !bHasRoster )
        {
            m_pRightClickItem = GetNumTrackers() == 1 ? pFirstSelectedItem : nullptr; // edit the text if only one is selected; otherwise we will edit the font properties
            OnEditTextProp();
        }

        else if( bHasField && !bHasFieldText && !bHasText && !bHasBox && !bHasRoster  )
        {
            if( GetNumTrackers() == 1 )
            {
                m_pRightClickItem = pFirstSelectedItem;
                OnEditFieldProp();
            }

            else
                OnEditMultipleFieldProperties();
        }

        else if( bHasRoster && !bHasFieldText && !bHasText && !bHasBox && !bHasField )
        {
            if( GetNumTrackers() == 1 )
            {
                m_pRightClickItem = pFirstSelectedItem;
                OnEditGridProp();
            }
        }
    }

    else if( IsAnyGridColSelected() || IsAnyGridRowSelected() )
    {
        for( int i = 0; i < GetNumGrids(); i++ ) // find the selected grid
        {
            if( ( IsAnyGridColSelected() && GetGrid(i)->GetCurCol() != NONE ) || ( IsAnyGridRowSelected() && GetGrid(i)->GetCurRow() != NONE ) )
            {
                GetGrid(i)->SendMessage(WM_COMMAND,ID_EDIT_COLUMNPROP);
                break;
            }
        }
    }

    else if( IsAnyGridTrackerSelected() ) // at this point, it should be a field on a roster
    {
        CFormGrid* pGrid = GetGridWhereTrackerSelected();

        CHitOb hitObj = pGrid->GetTracker(0).GetHitOb();

        pGrid->SetHitObj(hitObj);

        if( hitObj.GetField() >= 0 )
        {
            pGrid->SendMessage(WM_COMMAND, ID_EDIT_FIELDPROP);
            pGrid->DeselectColumns(); // 20120625 for whatever reason these were getting selected, and then pressing alt+enter on the field again led to a crash
            pGrid->DeselectRows();
        }

        else if( hitObj.GetText() >= 0 )
        {
            pGrid->SendMessage(WM_COMMAND, ID_EDIT_TEXTPROP);
        }

        else if( hitObj.GetBox() >= 0 )
        {
            pGrid->SendMessage(WM_COMMAND, ID_EDIT_BOXPROP);
        }

        else
        {
            ASSERT(false);
        }
    }
}


bool CFormScrollView::AreOnlyMultipleFieldsSelected(CDEItemBase* pRightClickItem) // 20120612
{
    int numFields = 0;
    bool clickedItemWasInTracker = false;

    CDEForm* pForm = GetCurForm();

    for( int i = 0; i < GetNumTrackers(); i++ )
    {
        CFormTracker tracker = GetTracker(i);
        int index = tracker.GetIndex();

        if( tracker.IsFldTxt() || tracker.IsBox() )
            return false;

        else
        {
            CDEItemBase* pItem = pForm->GetItem(index);

            if( !pItem->IsKindOf(RUNTIME_CLASS(CDEField)) )
                return false;

            if( pItem == pRightClickItem )
                clickedItemWasInTracker = true;

            numFields++;
        }
    }

    return clickedItemWasInTracker && numFields > 1;
}


void CFormScrollView::OnEditMultipleFieldProperties() // 20120612 (this is called when non-roster fields are called)
{
    // create a list of all the fields that are selected
    std::vector<CDEField*> fields;

    CDEForm* pForm = GetCurForm();

    for( int i = 0; i < GetNumTrackers(); i++ )
        fields.push_back((CDEField*)pForm->GetItem(GetTracker(i).GetIndex()));

    OnEditMultipleFieldProperties(fields);
}

void CFormScrollView::OnEditMultipleFieldProperties(std::vector<CDEField*>& fields) // 20120613
{
    assert_cast<CFormChildWnd*>(GetParentFrame())->RunMultipleFieldPropertiesDialog(&fields, GetCurGroup(), GetCurForm()->GetName());
}


TCHAR CFormScrollView::GetDecimalCharacter()
{
    Application* application = nullptr;
    AfxGetMainWnd()->SendMessage(UWM::Designer::GetApplication, (WPARAM)&application, (LPARAM)GetDocument());
    bool use_comma = ( application == nullptr ) ? false : application->GetDecimalMarkIsComma();
    return use_comma ? ',' : '.';
}
