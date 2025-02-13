//***************************************************************************
//  File name: FrmTrCtl.cpp
//
//  Description:
//       Forms tree control implementation
//
//  History:    Date       Author   Comment
//              ---------------------------
//              11 Feb 99   gsf     Created for CSPro 1.0
//
//***************************************************************************

#include "StdAfx.h"
#include "FrmTrCtl.H"
#include "BlockPropertiesDlg.h"
#include "QSFCndVw.h"
#include <zToolsO/SharedSettings.h>
#include <zToolsO/WinClipboard.h>
#include <zUtilO/BCMenu.h>
#include <zJson/JsonStream.h>
#include <zAppO/Application.h>
#include <zInterfaceF/resource_shared.h>


enum class FormTreeIcon
{
    FormFile, FormFileLogic,
    Level, LevelLogic,
    Form, FormLogic,
    FormLooping, FormLoopingLogic,
    Block, BlockLogic, BlockQuestion, BlockLogicQuestion,
    Roster, RosterLogic,
    Field, FieldLogic, FieldQuestion, FieldLogicQuestion,
    FieldProtected, FieldProtectedLogic,
    FieldMirror, FieldMirrorLogic,
    ExternalCode,
    Report,
};

static const int FormTreeIconImageIndices[] =
{
    IDI_FORMFILE, IDI_FORMFILE_LOGIC,
    IDI_LEVEL, IDI_LEVEL_LOGIC,
    IDI_FORM, IDI_FORM_LOGIC,
    IDI_FORM_LOOPING, IDI_FORM_LOOPING_LOGIC,
    IDI_FORM_BLOCK, IDI_FORM_BLOCK_LOGIC, IDI_FORM_BLOCK_QUESTION, IDI_FORM_BLOCK_LOGIC_QUESTION,
    IDI_ROSTER, IDI_ROSTER_LOGIC,
    IDI_FIELD, IDI_FIELD_LOGIC, IDI_FIELD_QUESTION, IDI_FIELD_LOGIC_QUESTION,
    IDI_PROTECTED, IDI_PROTECTED_LOGIC,
    IDI_MIRROR, IDI_MIRROR_LOGIC,
    IDI_LOGIC_FILE,
    IDI_REPORT_FILE,
};


BEGIN_MESSAGE_MAP(CFormTreeCtrl, CTreeCtrl)
    //{{AFX_MSG_MAP(CFormTreeCtrl)
    ON_NOTIFY_REFLECT(TVN_SELCHANGED, OnSelchanged)
    ON_NOTIFY_REFLECT(NM_DBLCLK, OnDTreeDblclk)
    ON_NOTIFY_REFLECT(TVN_ITEMEXPANDING, OnItemexpanding)
    ON_NOTIFY_REFLECT(TVN_DELETEITEM, OnDeleteitem)
    ON_WM_LBUTTONDOWN()
    ON_WM_LBUTTONUP()
    ON_WM_MOUSEMOVE()
    ON_WM_KEYDOWN()
    ON_COMMAND(ID_EDIT_FIELDPROP, OnEditFieldProp)
    ON_COMMAND(ID_EDIT_FORMPROP, OnEditFormProp)
    ON_COMMAND(ID_EDIT_LEVELPROP ,OnEditLevelProp)
    ON_COMMAND(ID_EDIT_FFPROP ,OnEditFFProp)
    ON_COMMAND(ID_EDIT_BLOCKPROP, OnEditBlockProp)
    ON_COMMAND(ID_EDIT_GRID_AUTOFIT, OnEditGridAutoFit)
    ON_COMMAND(ID_EDIT_GRIDPROP, OnEditGridProp)
    ON_COMMAND(ID_EDIT_REPORT_PROPERTIES, OnEditReportProperties)
    ON_COMMAND(ID_DELETE_ITEM, OnDeleteItem)
    ON_COMMAND(ID_DELETE_FORM, OnDeleteForm)
    ON_COMMAND(ID_VIEW_LOGIC, OnViewLogic)
    ON_COMMAND(ID_VIEW_QUESTIONNAIRE, OnViewQuestionnaire)
    ON_COMMAND(ID_COPY_NAME, OnCopyName)
    ON_COMMAND(ID_VIEW_FORM, OnViewForm)
    ON_COMMAND(ID_QSF_EDITOR, OnQSFEditor)
    ON_COMMAND(ID_CAPI_MACROS, OnCapiMacros)
    ON_COMMAND(ID_ADD_BLOCK, OnAddBlock)
    ON_COMMAND(ID_ADD_FORM, OnAddForm)
    ON_NOTIFY_REFLECT(TVN_SELCHANGING, OnSelchanging)
    ON_COMMAND(ID_UNGROUP_ITEMS, OnUngroupItems)
    ON_NOTIFY_REFLECT(TVN_GETDISPINFO, OnGetDispInfo)
    ON_WM_RBUTTONUP()
    ON_WM_RBUTTONDOWN()
    ON_NOTIFY_REFLECT(NM_SETFOCUS, OnSetFocus)
    ON_COMMAND(ID_EDIT_COPY, OnEditCopy)
    ON_COMMAND(ID_SHIFT_F10, OnShiftF10)
    //}}AFX_MSG_MAP
END_MESSAGE_MAP()


IMPLEMENT_DYNCREATE (CFormTreeCtrl, CTreeCtrl)



/////////////////////////////////////////////////////////////////////////////
// CFormTreeCtrl

CFormTreeCtrl::CFormTreeCtrl() : m_bDragging(false), m_dwDragStart(0),
                                 m_pDragImage (NULL), m_pFormDoc(NULL),
                                 m_pDDTreeCtrl (NULL), m_bOkToRedraw(true)
{
    m_mask  = TVIF_TEXT | TVIF_PARAM | TVIF_IMAGE | TVIF_SELECTEDIMAGE;


//  registers the view w/OLE DLLs as a valid drop target; need this so i can drag items
//  within the form control (e.g., traversal reorder)

    m_dropTarget.Register(this);

    m_bSendMsg = TRUE;

    m_dragStartBounds.SetRectEmpty();
}

CFormTreeCtrl::~CFormTreeCtrl()
{
    if (m_pDragImage)
    {
        delete m_pDragImage;
    }
}

/////////////////////////////////////////////////////////////////////////////
// can my tree ctrl refer to > 1 document?? if so, this will change


CFormScrollView* CFormTreeCtrl::GetFormView (void)
{
    if (GetFormDoc() == NULL)
        return NULL;

    CFormScrollView* pFScrollView = (CFormScrollView*) GetFormDoc()->GetView();
    return pFScrollView;
}

/////////////////////////////////////////////////////////////////////////////

void CFormTreeCtrl::OnSelchanging(NMHDR* pNMHDR, LRESULT* pResult)
{
    NM_TREEVIEW* pNMTreeView = (NM_TREEVIEW*)pNMHDR;
    // TODO: Add your control notification handler code here

    if (!pNMTreeView->itemOld.hItem)
    {
        *pResult = 0;
        return;
    }

    CFormScrollView* pView = GetFormView();

    CFormID* pOldID = (CFormID*) pNMTreeView->itemOld.lParam;

    if(m_bSendMsg && pView){
        CFormChildWnd* pFrame = (CFormChildWnd*)pView->GetParentFrame();

        if(pFrame->GetViewMode() != QSFEditorViewMode){
          if (pView && !pView->IsWindowVisible()){


            if(pFrame && pFrame->GetSourceView() && pFrame->GetSourceView()->GetEditCtrl()->IsModified())
                if(AfxGetMainWnd()->SendMessage(UWM::Form::PutSourceCode, 0, (LPARAM) pOldID) == -1){
                    *pResult = 1;
                    return;
                }
          }
        }
    }
    *pResult = 0;
}

/////////////////////////////////////////////////////////////////////////////

void CFormTreeCtrl::OnSelchanged (NMHDR* pNMHDR, LRESULT* pResult)
{
    NMTREEVIEW* pNMTreeView = (NM_TREEVIEW*) pNMHDR;    // Get tree control structure; mfc-gen
    LPARAM      lParam = pNMTreeView->itemNew.lParam;   // Get new state of tree control

    CFormID*    pID = (CFormID*) lParam;

    if (!pID)   // the pID can sometimes be 0... (found this out when allowing deletion of a page)
        return;

    CDEField* pSelField = NULL;
//  OnDTreeDblclk(pNMHDR, pResult);

    CFormDoc*   pDoc = pID->GetFormDoc();
    if (pDoc)
    {
        BOOL bActivate = FALSE;
        //Activate the frame for the selected item if the
        //RHS is CFormChildWnd type;
        CFrameWnd* pFrameWnd =((CFrameWnd*)AfxGetMainWnd())->GetActiveFrame();
        if(pFrameWnd && pFrameWnd->IsKindOf(RUNTIME_CLASS(CFormChildWnd))){
            bActivate = TRUE;
        }

        CFormScrollView* pView = (CFormScrollView*)pDoc->GetView();
        if(!pView)
            return;

        CFormChildWnd* pFrame = (CFormChildWnd*)pView->GetParentFrame();
        if(!pFrame)
            return;

        bool bSendSourceViewMessage = pFrame->GetSourceView() && (pFrame->GetSourceView()->IsWindowVisible() || pFrame->GetViewMode() == LogicViewMode);
        bool bSendQuestionViewMessage = pFrame->GetQuestionnaireView() && (pFrame->GetQuestionnaireView()->IsWindowVisible() || pFrame->GetViewMode() == QuestionnaireViewMode);
        if(bSendSourceViewMessage || bSendQuestionViewMessage)
        {
            CFormID* pCurrentId = pID;
            if(bActivate){
                // The call to ActivateFrame causes the tree control to be rebuilt
                // which invalidates pID. Find the new tree node corresponding to
                // the pID and use its CFormID pointer.
                CDEFormBase *itemPtr = pID->GetItemPtr();
                CFormDoc *itemDoc = pID->GetFormDoc();
                pFrame->ActivateFrame();
                HTREEITEM newTreeItem = xFindItem(GetFormNode(itemDoc)->GetHItem(), itemPtr);
                pID = newTreeItem ? (CFormID *) GetItemData(newTreeItem) : NULL;
            }
            if(m_bSendMsg){
                if(bSendSourceViewMessage) {
                    AfxGetMainWnd()->SendMessage(UWM::Form::ShowSourceCode, 0, (LPARAM)pDoc);
                }
                else if (bSendQuestionViewMessage && pCurrentId) {
                    CString itemName = GetNameOrLabel(pCurrentId, &pSelField);
                    pFrame->GetQuestionnaireView()->SendMessage(UWM::Designer::TreeSelectionChanged, (WPARAM)(itemName.GetString()));
                }
            }
        }
        bool bProcessQuestionText = pFrame->GetUseQuestionText();

        // savy & gsf 04-apr-00
        if (pView && m_bOkToRedraw /*&& pView->IsWindowVisible()*/)
        {
            if (pDoc != GetFormDoc())
            {
                CFormChildWnd*      pWnd  = (CFormChildWnd*) ((CFormScrollView*)pDoc->GetView())->GetParentFrame();
                if(bActivate){
                 //   pWnd->ActivateFrame(); //Dont call activate frame
                    pWnd->ShowWindow(SW_SHOW);  //Show window activates and does it in the current size
                }
            }

            pView->TreeSelectionChanged (std::vector<CFormID*> {pID});

            if (pDoc != GetFormDoc())
            {
                SetFormDoc(pDoc);
            }
        }

        if(bProcessQuestionText)
        {
            if(bActivate){
                pFrame->ActivateFrame();
            }
            if(m_bSendMsg && pFrame->GetViewMode()==FormViewMode){
                AfxGetMainWnd()->SendMessage(UWM::Form::ShowCapiText, reinterpret_cast<WPARAM>(pDoc));
            }
            else if(m_bSendMsg && pFrame->GetViewMode()==QSFEditorViewMode){
                pDoc->SetSelectedCapiQuestion(pID);
                pDoc->UpdateAllViews(nullptr, Hint::CapiEditorUpdateQuestionStyles);
            }
        }
    //} moved 6 lines below----Chirag Problem seen in lot of versions...
        CFSourceEditView* pView2 = (CFSourceEditView*)pDoc->GetView(FormViewType::Logic);
        if (pView2 != NULL) {
            pView2->GetEditCtrl()->EmptyUndoBuffer();
        }
    }

    SetFocus();

#ifdef _DEBUG
    //ShowFormName( GetFormDoc() );
#endif

    *pResult = 0;
}

// ***************************************************************************

void CFormTreeCtrl::OnDTreeDblclk(NMHDR* /*pNMHDR*/, LRESULT* pResult)
{
    // Get the formID corresponding to the retrieved item

    // If the view is not activated, do so

    LPARAM      lParam = GetItemData(GetSelectedItem());
    CFormID*    pID = (CFormID*) lParam;
    CFormDoc*   pDoc = pID->GetFormDoc();

    if(!pDoc) {
        *pResult = 0;
        return;
    }

    CFormScrollView*    pView = (CFormScrollView*) pDoc->GetView();
    CFormChildWnd*      pWnd  = (CFormChildWnd*) pView->GetParentFrame();

    pWnd->ActivateFrame();

    *pResult = 1;
}

// ***************************************************************************

HTREEITEM CFormTreeCtrl::InsertFormFile (const CString &sLabel,
                                         const CString &sFormFileName,
                                         CFormDoc* pDoc)
{
    // Insert the form in the form tree control
    // Use the form file name to insert it into the
    // structure later on to get the form file name

    // pID->form index set in the constructor to 0, which is what i want for the FF Node

    CFormNodeID* pID = new CFormNodeID();

    pID->SetFormDoc     (pDoc);
    pID->SetFFName      (sFormFileName);
    pID->SetFFLabel     (sLabel);
    pID->AddRef         ();
    pID->SetNodeType    (eFTT_FORMFILE);

    if (pDoc == NULL)
        pID->SetItemPtr (NULL);
    else
        pID->SetItemPtr (&pDoc->GetFormFile());


    TVINSERTSTRUCT tvi;

    tvi.hParent      = TVI_ROOT;
    tvi.hInsertAfter = TVI_LAST;

    tvi.item.mask       = TVIF_TEXT | TVIF_PARAM | TVIF_IMAGE | TVIF_SELECTEDIMAGE;
    tvi.item.state      = 0;
    tvi.item.stateMask  = 0;
    tvi.item.pszText    = (LPTSTR) LPSTR_TEXTCALLBACK;
    tvi.item.lParam     = (DWORD) pID;
//  tvi.item.iImage     = FORMFILE;         // must i say IDI_FORMFILE ?
//  tvi.item.iSelectedImage = FORMFILE; // ditto
    tvi.item.iImage = I_IMAGECALLBACK;
    tvi.item.iSelectedImage = I_IMAGECALLBACK;

    HTREEITEM hItem = InsertItem ( &tvi );

    pID->SetHItem (hItem);

    return hItem;
}

// ***************************************************************************
// Get the item corresponding to this Item ;if this has its pDoc as Null
// Then we need to build the tree .
// Send a message to our application to open the corresponding file

void CFormTreeCtrl::OnItemexpanding(NMHDR* pNMHDR, LRESULT* pResult)
{
    NMTREEVIEW* pNMTreeView = (NMTREEVIEW*) pNMHDR;
    HTREEITEM   hItem = pNMTreeView->itemNew.hItem;
    CFormID*    pID = (CFormID*) GetItemData(hItem);

    if(pID->IsKindOf(RUNTIME_CLASS(CFormNodeID)))
    {
        if(pID->GetFormDoc() == NULL)   // this may not be occurring for a while yet
        {
            const CString& sFileName = ((CFormNodeID*) pID)->GetFFName();

            ASSERT(m_pDocTemplate != NULL);

            CFormDoc* pDoc = (CFormDoc*) m_pDocTemplate->OpenDocumentFile(sFileName);

            pID->SetFormDoc(pDoc);

            pDoc->SetFormTreeCtrl (this);

            SetFormDoc (pDoc);  // set the tree ctrl's ptr to the doc

            if (!pDoc->LoadFormSpecFile(sFileName) )
            {
                pDoc->OnCloseDocument();
                // issue return stmt if later code prevents this from happening
            }
            pDoc->LoadDictSpecFile();

            ReBuildTree ();
        }
    }
    *pResult = 0;
}

// ***************************************************************************
// smg:  ensure this func is only called once!  do so from CSPro.exe

void  CFormTreeCtrl::InitImageList(void)
{
    m_cImageList.Create(16, 16, ILC_COLOR32, 0, 2);
    m_cImageList.SetBkColor(GetSysColor(COLOR_WINDOW));

    for( auto icon : FormTreeIconImageIndices )
        m_cImageList.Add(AfxGetApp()->LoadIcon(icon));

    SetImageList(&m_cImageList, TVSIL_NORMAL);
}


CString CFormTreeCtrl::GetNameOrLabel(CFormID* pID, CDEField** pSelField, bool name /*=true*/)
{
    const eNodeType eNT = pID->GetItemType();
    std::optional<CString> value_from_subclass = name ? pID->GetName() :
                                                        pID->GetLabel();

    if (value_from_subclass.has_value())
    {
        return *value_from_subclass;
    }

    else if (eNT == eFTT_FORMFILE)
    {
        return name ? pID->GetFormDoc()->GetFormFile().GetName() :
                      ((CFormNodeID*)pID)->GetFFLabel();
    }

    else if (eNT == eFTT_GRIDFIELD)
    {
        CDERoster* pRoster = (CDERoster*)(pID->GetItemPtr());
        CDECol* pCol = pRoster->GetCol(pID->GetColumnIndex());
        CDEField* pField = pCol->GetField(pID->GetRosterField());
        *pSelField = (CDEField*)pField;

        return name ? pField->GetName() :
                      pField->GetLabel();
    }

    else
    {
        *pSelField = DYNAMIC_DOWNCAST(CDEField, pID->GetItemPtr());

        return name ? pID->GetItemPtr()->GetName() :
                      pID->GetItemPtr()->GetLabel();
    }
}


// ***************************************************************************

void CFormTreeCtrl::OnGetDispInfo(NMHDR* pNMHDR, LRESULT* pResult)
{
    TV_DISPINFO* pTVDispInfo = reinterpret_cast<TV_DISPINFO*>(pNMHDR); // Get pointer to display information
    LPARAM lParam = pTVDispInfo->item.lParam;

    // On windows 8 with touch keyboard
    //we need to explicitly check for call back image or text
    //This method is being called seperately for the image and the text
    //If the checks are not done for TVI_IMAGE and TVIF_TEXT
    //it does not draw the control at all in release mode
    //in debug mode it crashes on lbuttondown.
    //DO NOT CHANGE THE TEXT OR IMAGE if the flag is not set
    //in the item.mask

    /* if((pTVDispInfo->item.mask & TVIF_IMAGE))
        TRACE("Draw Image \n");
     else
         TRACE("Do not draw Image \n");*/

    CFormID* pID = (CFormID*) lParam;

    if(!pID)
        return;
    CFormDoc* pFormDoc = pID->GetFormDoc();

    if (! pFormDoc && (pTVDispInfo->item.mask & TVIF_TEXT))
    {
        if (pID->IsKindOf (RUNTIME_CLASS(CFormNodeID)))
            lstrcpyn (pTVDispInfo->item.pszText, ((CFormNodeID*) pID)->GetFFLabel(), pTVDispInfo->item.cchTextMax);

        return;
    }

    eNodeType eNT = pID->GetItemType();
    CDEField* pSelField = NULL;

    if( pTVDispInfo->item.mask & TVIF_TEXT )
    {
        CString display_text;

        if( SharedSettings::ViewNamesInTree() )
        {
            display_text = GetNameOrLabel(pID, &pSelField, true);

            if( SharedSettings::AppendLabelsToNamesInTree() )
            {
                CString label = GetNameOrLabel(pID, &pSelField, false);

                if( display_text != label )
                    display_text.AppendFormat(_T(": %s"), label.GetString());
            }
        }

        else
        {
            display_text = GetNameOrLabel(pID, &pSelField, false);
        }

        lstrcpyn(pTVDispInfo->item.pszText, display_text, pTVDispInfo->item.cchTextMax);
    }


    //SAVY Code for image call back stuff
    if( pTVDispInfo->item.mask & TVIF_IMAGE )
    {
        bool has_logic = false;
        bool has_question = false;
        FormTreeIcon form_tree_icon = FormTreeIcon::FormFile;

        if( this->m_bSendMsg )
        {
            CRect rectItem;
            if( GetItemRect(pTVDispInfo->item.hItem, &rectItem, FALSE) )
            {
                has_logic = ( AfxGetMainWnd()->SendMessage(UWM::Form::HasLogic, 0,(LPARAM)pID) != 0 );
                has_question = ( AfxGetMainWnd()->SendMessage(UWM::Form::HasQuestionText, 0, (LPARAM)pID) != 0 );
            }
        }

        switch( eNT )
        {
            case eFTT_FORMFILE:
                form_tree_icon = has_logic ? FormTreeIcon::FormFileLogic : FormTreeIcon::FormFile;
                break;

            case eFTT_LEVEL:
                form_tree_icon = has_logic ? FormTreeIcon::LevelLogic : FormTreeIcon::Level;
                break;

            case eFTT_BLOCK:
                form_tree_icon = has_logic ?
                    ( has_question ? FormTreeIcon::BlockLogicQuestion : FormTreeIcon::BlockLogic ) :
                    ( has_question ? FormTreeIcon::BlockQuestion : FormTreeIcon::Block );
                break;

            case eFTT_FORM:
            {
                const CDEGroup* pGroup = DYNAMIC_DOWNCAST(CDEGroup, pID->GetItemPtr());
                form_tree_icon = ( pGroup->GetMaxLoopOccs() == 1 ) ?
                    ( has_logic ? FormTreeIcon::FormLogic : FormTreeIcon::Form ) :
                    ( has_logic ? FormTreeIcon::FormLoopingLogic : FormTreeIcon::FormLooping );
                break;
            }

            case eFTT_GRID:
                form_tree_icon = has_logic ? FormTreeIcon::RosterLogic : FormTreeIcon::Roster;
                break;

            case eFTT_FIELD:
            case eFTT_GRIDFIELD:
            {
                if( pSelField != nullptr )
                {
                    if( pSelField->IsProtected() )
                        form_tree_icon = has_logic ? FormTreeIcon::FieldProtectedLogic : FormTreeIcon::FieldProtected;

                    else if( pSelField->IsMirror() )
                        form_tree_icon = has_logic ? FormTreeIcon::FieldMirrorLogic : FormTreeIcon::FieldMirror;

                    else
                    {
                        form_tree_icon = has_logic ?
                            ( has_question ? FormTreeIcon::FieldLogicQuestion : FormTreeIcon::FieldLogic ) :
                            ( has_question ? FormTreeIcon::FieldQuestion : FormTreeIcon::Field );
                    }
                }

                break;
            }

            case eFFT_EXTERNALCODE:
                form_tree_icon = FormTreeIcon::ExternalCode;
                break;

            case eFFT_REPORT:
                form_tree_icon = FormTreeIcon::Report;
                break;

            default:
                ASSERT(false);
                break;
        }

        pTVDispInfo->item.iImage = (int)form_tree_icon;
        pTVDispInfo->item.iSelectedImage = (int)form_tree_icon;
    }

    *pResult = 0;
}

// ***************************************************************************
// smg: not sure if this is the best/right/only place to do this, but here goes;
// when the user has the form tree ctrl active (as opposed to the dict tree, etc.)
// and they do smthng in the view causing the tree to be rebuilt (such as deleting
// an item), the tree (as currently designed) gets trashed and rebuilt; fine; but
// doing so makes it lose track of where it was, i.e., what was the current form & field?
// so by getting the form index before i start bludgeoning things and then restoring
// it when done, i'll have at least the same form active, both in the view and on the
// tree...however if a form can be deleted from the tree, the following will have to
// be revised...

void CFormTreeCtrl::ReBuildTree (int iFormNum/*=0*/, CDEFormBase* pItem /*=NULL*/)
{


    // find node corresponding to this form
    // & delete all its descendants

    CFormNodeID* pNodeID = GetFormNode (m_pFormDoc);
    //Make sure that the form view is visible
    CFormScrollView* pView = (CFormScrollView*)m_pFormDoc->GetView();

    if(pView && !pView->IsWindowVisible())
    {
        this->OnViewForm();
    }

    m_bSendMsg = FALSE; //moved this after switching to form so that any changes to logic are update in the source code
    SetRedraw(false);               // Don't redraw tree ctrl while rewkg
    m_bOkToRedraw = false;

    HTREEITEM h, hSib;
    h = GetChildItem (pNodeID->GetHItem());

    while (h != NULL)
    {
        hSib = GetNextSiblingItem(h);
        DeleteItem(h);
        h = hSib;
    }

    BuildTree(pNodeID, false);
    InsertExternalCodeAndReportNodes(pNodeID);

    m_bSendMsg = TRUE;

    SelectFTCNode (pNodeID, iFormNum, pItem);
}

// ***************************************************************************
// select the form's tree ctrl node :)
// the node id being passed in is the root
// this is not a great search, cause it's only searching top-level groups
// right now :{

void CFormTreeCtrl::SelectFTCNode (CFormID* pFormID, int iFormNum, CDEFormBase* pSearchItem)
{
    CFormID*    pID;
    HTREEITEM hLevel, hGroup = NULL, hExpand = NULL;

    hLevel = GetChildItem (pFormID->GetHItem());    // this will give me level 0

    while (hLevel != NULL && hExpand == NULL)
    {
        pID = (CFormID*) GetItemData (hLevel);  // GetItemData() is an MFC func

        if( pID != nullptr && pID->GetItemType() != eFFT_EXTERNALCODE && pID->GetItemType() != eFFT_REPORT )
        {
            hGroup = GetChildItem (pID->GetHItem());

            FindFTCRecursively (hGroup, hExpand, iFormNum, pSearchItem);
        }

        hLevel = GetNextSiblingItem (hLevel);
    }
    if (hExpand == NULL)

        DefaultExpand (pFormID->GetHItem());
    else
    {
        pID = (CFormID*) GetItemData (hExpand); // for debug; so i can see what i have

        ClearSelection();
        SelectItem (hExpand);

        Expand (hExpand, TVE_EXPAND);
    }

    SetRedraw (true);
    m_bOkToRedraw = true;
    Invalidate();
    UpdateWindow();
}


void CFormTreeCtrl::FindFTCRecursively (HTREEITEM&  hGroupOrItem,
                                        HTREEITEM&  hExpand,
                                        int         iFormNum,
                                        CDEFormBase* pSearchItem)
{
    CFormID*    pID;
    HTREEITEM   hPlaceHolder = NULL;
    CDEFormBase* pItem = NULL;
    CDERoster* pRoster;

    while (hGroupOrItem != NULL && hExpand == NULL)
    {
        pID = (CFormID*) GetItemData (hGroupOrItem);

        pItem = pID->GetItemPtr();

        if (pID->GetItemType() == eFTT_GRIDFIELD)
        {
            pRoster = (CDERoster*) pItem;

            //do not get the item from roster itembase array. A roster column can have multiple fields when joined.
            //the roster field index stored in the itemID is pointing to the index in the CDECol array of fields.
            int colIndex  = pID->GetColumnIndex();
            const CDECol* pCol = pRoster->GetCol(colIndex);
            pItem = pCol->GetField(pID->GetRosterField());
        }

        if (pSearchItem == NULL)    // then no specific item to look for, check form #
        {
            if (iFormNum != NONE &&
                iFormNum == pID->GetFormIndex())
            {
                hExpand = hGroupOrItem;
                continue;
            }
        }
        else if (pSearchItem == pItem)
        {
            hExpand = hGroupOrItem;
            continue;
        }
        hPlaceHolder = hGroupOrItem;

        hGroupOrItem = GetChildItem ( pID->GetHItem() );

        FindFTCRecursively (hGroupOrItem, hExpand, iFormNum, pSearchItem);

        if (hExpand)    // found the guy we want to select in the recurse
            continue;   // will cause the loop to fail and hence, return

        // otherwise, continue on with placeholder

        hGroupOrItem = GetNextSiblingItem (hPlaceHolder);
    }
}

void CFormTreeCtrl::SelectNodes(std::vector<CDEFormBase*> nodes)
{
    for (auto node : nodes)
    {
        HTREEITEM hItem = FindItem(node);
        AddToSelection(hItem);
    }
}

// ***************************************************************************
//  when this func is entered, the root node for the form file has already
//  been created, so i just have to do the forms and their objs
/*
TVINSERTSTRUCT:

    HTREEITEM hParent;          y
    HTREEITEM hInsertAfter;     n
    TVITEM    item;             y

TVITEM:
    UINT      mask;         y
    HTREEITEM hItem;
    UINT      state;
    UINT      stateMask;
    LPTSTR    pszText;      y
    int       cchTextMax;
    int       iImage;       y
    int       iSelectedImage;   y
    int       cChildren;
    LPARAM    lParam;       y


  */

LOGFONT CFormTreeCtrl::m_DefLogFont ={ 0, 0, 0, 0 };
CFont  CFormTreeCtrl::m_font;

void CFormTreeCtrl::BuildTree (CFormNodeID* pNodeID, bool bRedraw)
{
    if(m_DefLogFont.lfHeight == 0 )  {
        CFont* pFont = GetFont();
        pFont->GetLogFont(&m_DefLogFont);
    }
    LOGFONT logfont;
    memset(&logfont,0,sizeof(LOGFONT));
    bool bChangeFont  = false;
    m_font.GetSafeHandle() == NULL ? bChangeFont = true : m_font.GetLogFont(&logfont);

    CIMSAString sFontName = GetDesignerFontName();
    sFontName.Trim();

    CIMSAString sDefFontFaceName(m_DefLogFont.lfFaceName);
    if((!sFontName.IsEmpty() && sFontName.CompareNoCase(logfont.lfFaceName) != 0) || (sFontName.IsEmpty() && sDefFontFaceName.CompareNoCase(logfont.lfFaceName) != 0 )){
        bChangeFont = true;
    }

    if(bChangeFont || logfont.lfHeight != m_DefLogFont.lfHeight * (GetDesignerFontZoomLevel()/ 100.0)){
        logfont = m_DefLogFont; //initialize
        logfont.lfHeight = (LONG) (m_DefLogFont.lfHeight * (GetDesignerFontZoomLevel()/ 100.0)); //zoom

        if(!sFontName.IsEmpty()){
            lstrcpy(logfont.lfFaceName,sFontName);
        }
        else {
            lstrcpy(logfont.lfFaceName,m_DefLogFont.lfFaceName);
        }

        m_font.DeleteObject();
        m_font.CreateFontIndirect(&logfont);
        SetFont(&m_font);
    }
//  CFormTreeCtrl&  tc = *this;
    m_bSendMsg = FALSE;
    CFormDoc* pFormDoc = pNodeID->GetFormDoc();

    if(!pFormDoc)
        return;

    pNodeID->SetItemPtr(&pFormDoc->GetFormFile());

    m_pFormDoc = pFormDoc;

    SetRedraw (false);               // Don't try to draw while rebuilding the tree ctrl!
    m_bOkToRedraw = false;

    //Make sure that the form view is visible
    CFormScrollView* pView = (CFormScrollView*)m_pFormDoc->GetView();
    if(pView && !pView->IsWindowVisible()) {
        this->OnViewForm();
    }

    HTREEITEM       hRoot, hParent;
    CDEFormFile*    pFF = &pFormDoc->GetFormFile();
    CDELevel*       pLevel = NULL;
    CDEGroup*       pGroup = NULL;
    CFormID*        pFormID;

    int     iLevel, iGroup;

    TVINSERTSTRUCT tvi;

    tvi.item.mask       = TVIF_TEXT | TVIF_PARAM | TVIF_IMAGE | TVIF_SELECTEDIMAGE;
    tvi.item.state      = 0;
    tvi.item.stateMask  = 0;
    tvi.item.pszText    = LPSTR_TEXTCALLBACK;
    tvi.item.iImage = I_IMAGECALLBACK;
    tvi.item.iSelectedImage = I_IMAGECALLBACK;

    hRoot = pNodeID->GetHItem();

    for (iLevel = 0;
         iLevel < pFF->GetNumLevels();
         iLevel++)
    {
        pFormID = new CFormID();    // indices set to NONE in constructor

        pLevel = pFF->GetLevel(iLevel);

        tvi.hParent = hRoot;
//      tvi.hInsertAfter = TVI_LAST;    // nec?

        tvi.item.lParam = (DWORD) pFormID;

        hParent = InsertItem ( &tvi );

        pFormID->SetFormDoc     (pFormDoc);
        pFormID->SetNodeType    (eFTT_LEVEL);
        pFormID->SetFormIndex   (iLevel);   // will be updated below
        pFormID->SetHItem       (hParent);
        pFormID->SetItemPtr     (pLevel);

        for (iGroup = 0;                        // each level must be on sep forms, so build
             iGroup < pLevel->GetNumGroups();   // a form node for each
             iGroup++)
        {
            pGroup = pLevel->GetGroup (iGroup);
            AddForm (pFormDoc, pGroup, hParent, iLevel);
        }

        pFormID->SetFormIndex (pLevel->GetGroup(0)->GetFormNum());  // ok to do here?
    }
    DefaultExpand (hRoot);

    if (bRedraw)
    {
        SetRedraw (true);
        m_bOkToRedraw = true;
        Invalidate();
        UpdateWindow();
    }

    m_bSendMsg = TRUE;
}


void CFormTreeCtrl::InsertExternalCodeAndReportNodes(const CFormNodeID* pFormNodeId)
{
    if( pFormNodeId == nullptr || pFormNodeId->GetHItem() == nullptr )
    {
        ASSERT(false);
        return;
    }

    // first remove all old nodes
    HTREEITEM hChildItem = GetChildItem(pFormNodeId->GetHItem());

    while( hChildItem != nullptr )
    {
        HTREEITEM hNextItem = GetNextItem(hChildItem, TVGN_NEXT);

        CFormID* pFormId = (CFormID*)GetItemData(hChildItem);

        if( pFormId != nullptr && ( pFormId->GetItemType() == eFFT_EXTERNALCODE ||
                                    pFormId->GetItemType() == eFFT_REPORT ) )
        {
            DeleteItem(hChildItem);
        }

        hChildItem = hNextItem;
    }

    // get the current application
    Application* pApplication = nullptr;
    AfxGetMainWnd()->SendMessage(UWM::Designer::GetApplication, (WPARAM)&pApplication, (LPARAM)pFormNodeId->GetFormDoc());

    if( pApplication == nullptr )
        return;

    HTREEITEM hLastAddedHeaderItem = nullptr;

    // the routine for adding nodes
    auto add_nodes = [&](auto& pFormIds)
    {
        ASSERT(!pFormIds.empty());
        HTREEITEM hHeaderItem = nullptr;

        for( auto& pFormId : pFormIds )
        {
            TVINSERTSTRUCT tvi;
            tvi.item.mask = TVIF_TEXT | TVIF_PARAM | TVIF_IMAGE | TVIF_SELECTEDIMAGE;
            tvi.item.state = 0;
            tvi.item.stateMask = 0;
            tvi.item.pszText = LPSTR_TEXTCALLBACK;
            tvi.item.iImage = I_IMAGECALLBACK;
            tvi.item.iSelectedImage = I_IMAGECALLBACK;

            if( hHeaderItem == nullptr )
            {
                ASSERT(pFormId->GetTextSource() == nullptr);

                tvi.hParent = pFormNodeId->GetHItem();
                tvi.hInsertAfter = ( hLastAddedHeaderItem == nullptr ) ? TVI_FIRST : hLastAddedHeaderItem;
            }

            else
            {
                ASSERT(pFormId->GetTextSource() != nullptr);

                tvi.hParent = hHeaderItem;
                tvi.hInsertAfter = TVI_LAST;
            }

            tvi.item.lParam = (DWORD)pFormId;

            HTREEITEM hItem = InsertItem(&tvi);
            pFormId->SetHItem(hItem);

            if( hHeaderItem == nullptr )
            {
                hHeaderItem = hItem;
                hLastAddedHeaderItem = hHeaderItem;
            }
        }
    };


    // add external code
    {
        std::vector<FormExternalCodeID*> pFormIds;

        for( const CodeFile& code_file : pApplication->GetCodeFiles() )
        {
            if( code_file.IsLogicMain() )
                continue;

            if( pFormIds.empty() )
                pFormIds.emplace_back(new FormExternalCodeID(pFormNodeId->GetFormDoc(), std::nullopt));

            pFormIds.emplace_back(new FormExternalCodeID(pFormNodeId->GetFormDoc(), code_file));
        }

        if( !pFormIds.empty() )
            add_nodes(pFormIds);
    }


    // add reports
    const auto& report_named_text_sources = pApplication->GetReportNamedTextSources();

    if( !report_named_text_sources.empty() )
    {
        std::vector<FormReportID*> pFormIds = { new FormReportID(pFormNodeId->GetFormDoc(), nullptr) };

        for( const auto& report_named_text_source : report_named_text_sources )
            pFormIds.emplace_back(new FormReportID(pFormNodeId->GetFormDoc(), report_named_text_source));

        add_nodes(pFormIds);
    }
}


void CFormTreeCtrl::AddForm(CFormDoc* pFormDoc, CDEGroup* pGroup, HTREEITEM hParent, int iLevel)
{
    CFormID*    pFormID = new CFormID();

    TVINSERTSTRUCT tvi;

    tvi.hParent         = hParent;

    tvi.item.mask       = TVIF_TEXT | TVIF_PARAM | TVIF_IMAGE | TVIF_SELECTEDIMAGE;
    tvi.item.state      = 0;
    tvi.item.stateMask  = 0;
    tvi.item.pszText    = (LPTSTR) LPSTR_TEXTCALLBACK;
    tvi.item.lParam     = (DWORD) pFormID;
    tvi.item.iImage = I_IMAGECALLBACK;
    tvi.item.iSelectedImage = I_IMAGECALLBACK;

    pFormID->SetNodeType (eFTT_FORM);

    HTREEITEM hLocalParent = InsertItem ( &tvi );

    pFormID->SetFormDoc     (pFormDoc);
    pFormID->SetFormIndex   (pGroup->GetFormNum());
    pFormID->SetHItem       (hLocalParent);
    pFormID->SetItemPtr     (pGroup);

    AddGroupItems(pFormDoc, pGroup, hLocalParent, iLevel);
}


void CFormTreeCtrl::AddGroupItems(CFormDoc* pFormDoc, CDEGroup* pGroup, HTREEITEM hParent, int iLevel)
{
    for (int i = 0;
        i < pGroup->GetNumItems();  // now loop thru all items and insert them under the pg
        i++)
    {
        CDEItemBase* pItem = pGroup->GetItem(i);

        switch (pItem->GetItemType())
        {
        case CDEFormBase::Block:
            AddBlock(pFormDoc, assert_cast<CDEBlock&>(*pItem), hParent, iLevel);
            break;
        case CDEFormBase::Field:
            if (pGroup->GetBlock(static_cast<CDEField*>(pItem)) == nullptr) // items in block are added underneath block
                AddBaseItem(pFormDoc, pItem, hParent, iLevel);
            break;
        case CDEFormBase::Roster:
            AddBaseItem(pFormDoc, pItem, hParent, iLevel);
        }
    }
}


//////////////////////////////////////////////////////////////////////////////
// call this to add each keyed field or roster; if it's a roster, add all
// items belonging to it

void CFormTreeCtrl::AddBaseItem(CFormDoc* pFormDoc, CDEItemBase* pItem, HTREEITEM hParent, int iLevel)
{
    CFormID*    pFormID = new CFormID();

    TVINSERTSTRUCT tvi;

    tvi.hParent         = hParent;

    tvi.item.mask       = TVIF_TEXT | TVIF_PARAM | TVIF_IMAGE | TVIF_SELECTEDIMAGE;
    tvi.item.state      = 0;
    tvi.item.stateMask  = 0;
    tvi.item.pszText    = (LPTSTR) LPSTR_TEXTCALLBACK;
    tvi.item.lParam     = (DWORD) pFormID;
    tvi.item.iImage     = I_IMAGECALLBACK;
    tvi.item.iSelectedImage = I_IMAGECALLBACK;

    HTREEITEM hLocalParent = InsertItem ( &tvi );

    CDEFormBase::eItemType eItem = pItem->GetItemType();

    if (eItem == CDEFormBase::Roster) {
#ifdef _DEBUG
        static_cast<CDERoster*>(pItem)->CheckRosterIntegrity();
#endif
        pFormID->SetNodeType(eFTT_GRID);
        pFormID->SetItemPtr(pItem);
    }
    else if (eItem == CDEFormBase::Field) {

        if (pItem->GetParent()->GetItemType() == CDEFormBase::Roster) {
            pFormID->SetNodeType(eFTT_GRIDFIELD);
            CDERoster* pRoster = (CDERoster*)pItem->GetParent();
            pFormID->SetItemPtr(pRoster);
            const int colIndex = pRoster->GetColIndex((CDEField*)pItem);
            pFormID->SetColumnIndex(colIndex);
            const CDECol* const pCol = pRoster->GetCol(colIndex);
            pFormID->SetRosterField(pCol->GetFieldIndex((CDEField*)pItem));
        }
        else {
            pFormID->SetNodeType(eFTT_FIELD);
            pFormID->SetItemPtr(pItem);
        }
    }

    pFormID->SetFormDoc     (pFormDoc);
    pFormID->SetFormIndex   (pItem->GetFormNum());
    pFormID->SetHItem       (hLocalParent);

    // if it's a roster, loop thru its fields

    if (eItem == CDEFormBase::Roster)
    {
        AddGroupItems (pFormDoc, (CDERoster*) pItem, hLocalParent, iLevel);
    }
}


void CFormTreeCtrl::AddBlock(CFormDoc* pFormDoc, CDEBlock& form_block, HTREEITEM hParent, int iLevel)
{
    CFormID* pFormID = new CFormID();

    TVINSERTSTRUCT tvi;
    tvi.hParent = hParent;
    tvi.item.mask = TVIF_TEXT | TVIF_PARAM | TVIF_IMAGE | TVIF_SELECTEDIMAGE;
    tvi.item.state = 0;
    tvi.item.stateMask = 0;
    tvi.item.pszText = (LPTSTR)LPSTR_TEXTCALLBACK;
    tvi.item.lParam = (DWORD)pFormID;
    tvi.item.iImage = I_IMAGECALLBACK;
    tvi.item.iSelectedImage = I_IMAGECALLBACK;

    HTREEITEM hLocalParent = InsertItem(&tvi);
    pFormID->SetNodeType(eFTT_BLOCK);

    pFormID->SetFormDoc(pFormDoc);
    pFormID->SetFormIndex(form_block.GetFormNum());
    pFormID->SetHItem(hLocalParent);
    pFormID->SetItemPtr(&form_block);    // can get the name and label from here if needed later

    for( CDEField* pField : form_block.GetParent()->GetBlockFields(form_block) )
        AddBaseItem(pFormDoc, pField, hLocalParent, iLevel);
}


// ***************************************************************************
// expands all nodes of the tree

void CFormTreeCtrl::DefaultExpand(HTREEITEM hItem)
{
    // won't put the following types on the default expand
    while (hItem)
    {
        CFormID* pID = (CFormID*) GetItemData (hItem);   // GetItemData() is an MFC func
        eNodeType eNT = pID->GetItemType();

        if( eNT != eFTT_FORMFILE && eNT != eFTT_LEVEL )
            return;

        Expand (hItem, TVE_EXPAND);

        DefaultExpand (GetChildItem (hItem));

        hItem = GetNextSiblingItem(hItem);
    }
}

// ***************************************************************************

CFormNodeID* CFormTreeCtrl::GetFormNode(CFormDoc*  pDoc) const
{
    CFormNodeID* pResult = NULL;
    CFormNodeID* pNodeID = NULL;
    HTREEITEM    hItem  = GetRootItem();
    bool         bFound = false;

    while (hItem && !bFound)
    {
        pNodeID = (CFormNodeID*) GetItemData(hItem);

        if( pNodeID->GetFormDoc() == pDoc )
        {
            pResult = pNodeID;
            bFound = true;
        }
        else
            hItem = GetNextSiblingItem(hItem);
    }
    return pResult;
}

// ***************************************************************************

CFormNodeID* CFormTreeCtrl::GetFormNode(wstring_view filename) const
{
    HTREEITEM hItem = GetRootItem();

    while( hItem != nullptr )
    {
        CFormNodeID* pNodeID = (CFormNodeID*)GetItemData(hItem);

        if( pNodeID != nullptr && SO::EqualsNoCase(filename, pNodeID->GetFFName()) )
            return pNodeID;

        hItem = GetNextSiblingItem(hItem);
    }

    return nullptr;
}

/***************************************************************************************
//Use this only when the starting Item is a FormNode

//Removes the CFormIDs attached to the hItems for this hItem and its children
****************************************************************************************/

void CFormTreeCtrl::RemoveFormIDs(HTREEITEM hItem)
{
    //remove the formID of this Item

    CFormID* pID = (CFormID*) GetItemData(hItem);

    if(pID)
    {
        delete pID;
        SetItemData(hItem,0);
    }
    //check if it has any children

    HTREEITEM hChild = GetChildItem(hItem);

    //Remove the DictIds for the child items

    while(hChild)
    {
        RemoveFormIDs(hChild);
        hChild = GetChildItem(hChild);
    }

    //Get the sibling items
    hItem = GetNextSiblingItem(hItem);

    //if the item has  parent only then call for the deletion of the sibling items and its children

    if(GetParentItem(hItem))

        RemoveFormIDs(hItem);
}


// ***************************************************************************

void CFormTreeCtrl::ReleaseFormNodeID(CFormNodeID*& pID)
{
    ASSERT(pID != NULL);
    pID->Release();

    if(pID->GetRefCount() == 0)
    {
        if(pID->GetFormDoc())
        {
            //By now the doc shld be saved so go ahead and close it
            m_bSendMsg = FALSE;
            pID->GetFormDoc()->OnCloseDocument();
        }
        SetItemData(pID->GetHItem(),NULL);
        DeleteItem (pID->GetHItem());   // drop the item from the tree
        delete pID;
        pID = NULL;
    }
}

// ***************************************************************************
// smg: do we nd this func? i think not

void CFormTreeCtrl::Clean(void)
{
    // Remove all the formIDs if they are STILL available (should not be there )
    // Should be cleared by appropriate close document routines

    HTREEITEM hItem = GetRootItem() ;

    while (hItem)
    {
        RemoveFormIDs(hItem);
        CFormID* pID = (CFormID*) GetItemData(hItem);
        delete pID;
        hItem=GetNextSiblingItem(hItem);    //Get the next sibling item and run on it
    }
}

// ***************************************************************************

void CFormTreeCtrl::ReleaseDoc (CFormNodeID* pNodeID)
{

    if(pNodeID->GetFormDoc() == NULL)
        return;

    pNodeID->SetFormDoc(NULL);
    pNodeID->Release();

    HTREEITEM hNode = pNodeID->GetHItem();
    HTREEITEM hChild = GetChildItem(hNode); // smg: shldn't we check for hNode != NULL?

    while(hChild)       // smg: what exactly are we killing here?
    {
        DeleteItem(hChild);
        hChild = GetChildItem(hNode);
    }
    if(pNodeID->GetRefCount() == 0)
    {
        DeleteItem(hNode);
    }
}

// ***************************************************************************
// if the tree lost focus and regained it, w/the tree item being left
// unchanged, the tracker boxes aren't getting redrawn (as TreeSelChanged
// was only being called from the OnSelChanged or OnSelChanging

void CFormTreeCtrl::OnSetFocus(NMHDR* pNMHDR, LRESULT* pResult)
{
//  NMTREEVIEW* pNMTreeView = (NM_TREEVIEW*)pNMHDR;     // doesn't help me, leave out!
//  LPARAM      lParam = pNMTreeView->itemNew.lParam;
//  CFormID*    pID = (CFormID*) lParam;
    (void)pNMHDR;

    HTREEITEM hNode = GetSelectedItem();

    CFormID*    pID = (CFormID*) GetItemData(hNode);

    CFormDoc*   pDoc = pID->GetFormDoc();

    if (pDoc)
    {
        CFormScrollView* pView = (CFormScrollView*)pDoc->GetView();
        int iNumTrackers = pView->GetNumTrackers();

        pView->RemoveAllTrackers();

        eNodeType eNT = pID->GetItemType();

        // unselect any cols the user may have chosen in a grid; the tree ctrl could
        // have an item or the form itself selected, then select a roster col, but
        // this does not change who the tree ctrl has in focus

//      pView->UnselectRosterRowOrCol();
        pView->UnselectGridRowOrCol();

        CFormChildWnd* pFrame = (CFormChildWnd*) GetFormView()->GetParentFrame();
        if ( pFrame->IsLogicViewActive() )  // can't do a tracker if logic view up
        {
            *pResult = 1;
            return;
        }

        if (eNT == eFTT_FORM)
        {
            if (pID->GetFormIndex() != pView->GetFormIndex())   // user set focus to diff form that what is curr in the view
            {
                pView->TreeSelectionChanged (std::vector<CFormID*> {pID});
            }
            else if (iNumTrackers)   // if we cleared some trackers, then refresh
            {                   // (trying to minimize flicker)
                pView->Invalidate();
                pView->SendMessage(WM_SIZE);
            }
        }
        else if (eNT == eFTT_FIELD)
        {
            CDEItemBase* pItem = (CDEItemBase*) pID->GetItemPtr();

            CDEForm* pForm = pView->GetCurForm();

            int i = pForm->GetItemIndex (pItem);

            // gsf 04-apr-00

            if (i == -1)
            {
                *pResult = 0;
                return;
            }

            CRect rect = pItem->GetDims();

            pView->ScrollAsNecessary (&rect);
            pView->AddTracker (i, rect);    // if it's a field, this will add the txt portion too
            pView->DrawTrackerOutlines (pView->GetDC());// draw the tracker boxes
            pView->SetCurItem ( (CDEItemBase*) pID->GetItemPtr() );
        }
        else if (eNT == eFTT_BLOCK) // then try to select all items w/in it
        {
            const CDEBlock& form_block = assert_cast<CDEBlock&>(*pID->GetItemPtr());
            CDEForm* pForm = pView->GetCurForm();

            for (CDEField* pField : form_block.GetParent()->GetBlockFields(form_block))
            {
                //if the item is on a roster the itemeindex will be -1. Ignore adding trackers in this case.
                if (pForm->GetItemIndex(pField) != -1) {
                    CRect rect = pField->GetDims();
                    pView->ScrollAsNecessary(&rect);
                    pView->AddTracker(rect, pForm->GetItemIndex(pField));   // if it's a field, this will add the txt portion too
                }
            }
            pView->DrawTrackerOutlines (pView->GetDC());// draw the tracker boxes
        }
    }
    *pResult = 0;   // mfc-gen
}

// ***************************************************************************

void CFormTreeCtrl::OnLButtonDown(UINT nFlags, CPoint point)
{
    AfxTrace(L"OnLButtonDown %d %d\n", point.x, point.y);
    m_dwDragStart = GetTickCount(); // global; rtns elapsed # of ms. since system startup

    UINT        flags;
    HTREEITEM   hItem = HitTest(point, &flags); // (mfc) tree ctrl function

    if (flags & TVHT_ONITEM)
    {
        if (nFlags & MK_CONTROL || nFlags & MK_SHIFT)
        {
            // Multiple selection
            bool notifySelChange = false;

            if (hItem)
            {
                if (nFlags & MK_CONTROL)
                {
                    // Ctrl+click toggles selected state of clicked item
                    if (IsSelected(hItem)) {
                        if (GetSelected().size() > 1)
                        {
                            RemoveFromSelection(hItem);
                            notifySelChange = true;
                        }
                    }
                    else if (CanBePartOfMultipleSelection(hItem)) {
                        AddToSelection(hItem);
                        notifySelChange = true;
                    }
                }
                else if (nFlags & MK_SHIFT)
                {
                    // Shift+click selects consecutive items
                    HTREEITEM hSelStart = GetSelectedItem();

                    SelectRange(hSelStart, hItem);

                    notifySelChange = true;
                }

            }

            if (notifySelChange)
            {
                NotifyViewSelectionChanged();
            }

            // Don't call base on button down as that will clear other selected items

            // If we don't call default OnLButtonDown need to set the
            // focus to this window since base class would do that
            if (CWnd::GetFocus() != this)
                SetFocus();
        }
        else {

            // Set bounds for starting drag
            m_dragStartBounds.SetRect(point.x, point.y, point.x, point.y);
            m_dragStartBounds.InflateRect(GetSystemMetrics(SM_CXDRAG), GetSystemMetrics(SM_CYDRAG));

            if (hItem && IsSelected(hItem))
            {
                if (((CFormID*)GetItemData(hItem))->GetItemPtr())
                {
                    AfxTrace(L"OnLButtonDown on sel item %x %s\n", (DWORD)hItem, ((CFormID*)GetItemData(hItem))->GetItemPtr()->GetName().GetString());
                }

                // Left click on selected item could be start of drag
                // do nothing now. Will change selection in button up if no drag

                // If we don't call default OnLButtonDown need to set the
                // focus to this window since base class would do that
                if (CWnd::GetFocus() != this)
                    SetFocus();
            }
            else
            {
                AfxTrace(L"OnLButtonDown no sel item\n");

                // Click without shift/ctrl so clear any multiple selection
                ClearSelection();

                SelectItem(hItem);
                AddToSelection(hItem);

                // If we don't call default OnLButtonDown need to set the
                // focus to this window since base class would do that
                if (CWnd::GetFocus() != this)
                    SetFocus();
            }
        }
    } else {
        // Did not click on tree item, clicked on expander or empty space,
        // use base default behavior
        CTreeCtrl::OnLButtonDown(nFlags, point);
    }

}

void CFormTreeCtrl::OnLButtonUp(UINT nFlags, CPoint point)
{
    m_dragStartBounds.SetRectEmpty();

    if (m_bDragging)
    {
        // [1] Unlocks the window specified & hides the drag image, allowing the window to be updated.
        // [2] Officially terminate the drag operation

        m_pDragImage->DragLeave(GetDesktopWindow());    // [1]
        m_pDragImage->EndDrag();                        // [2]

        delete m_pDragImage;    // scrap him, don't nd anymore

        m_pDragImage = NULL;
        auto selectedItems = GetSelectedFormItems();

        ReleaseCapture();
        m_bDragging = false;
        SelectDropTarget(NULL);

        ClientToScreen(&point);

        // Check if the drop is within the tree ctrl

        CRect rect;
        GetWindowRect(&rect);

        if (rect.PtInRect(point))   // am i dropping on myself?
        {
            if (!selectedItems.empty())
            {
                DropFormTreeItem(selectedItems, point);
            }
        }
    }
    else if (!(nFlags & MK_CONTROL || nFlags & MK_SHIFT))
    {

        // If we were not dragging and had clicked on a selected item we need
        // to select it here because we didn't do it on button down in order to allow
        // possible drag of multiple selection
        UINT        flags;
        HTREEITEM   hItem = HitTest(point, &flags);
        if (hItem && IsSelected(hItem))
        {
            ClearSelection();
            SelectItem(hItem);
            AddToSelection(hItem);
        }
    }

    //CTreeCtrl::OnLButtonUp(nFlags, point);    // mfc-generated
}

// ***************************************************************************

void CFormTreeCtrl::OnMouseMove(UINT nFlags, CPoint point)
{
    bool bRight = false;
    CPoint tPoint = point;

    if (m_bDragging)
    {
       long dwStyle = ::GetWindowLong(this->GetSafeHwnd(),GWL_STYLE);
       ::SetWindowLong(this->GetSafeHwnd(),GWL_STYLE,dwStyle & ~TVS_NOTOOLTIPS);

        CancelToolTips();

        RECT rect;
        GetClientRect( &rect );

        bRight = (point.x < (rect.right - rect.left) /2) ? false : true;

        ClientToScreen(&tPoint);
        ASSERT(m_pDragImage != NULL);

        m_pDragImage->DragMove(tPoint);

        CWnd* pWnd = this->WindowFromPoint(tPoint);

        if(pWnd == this)
        {


            m_pDragImage->DragShowNolock(false); // show the image while locking the window

            HTREEITEM hNItem = this->HitTest(point);
            if(hNItem){
                this->SelectDropTarget(hNItem);
            }
            if( point.y < rect.top + 10 ) {
                SendMessage( WM_VSCROLL, SB_LINEUP);
            }
            else if( point.y > rect.bottom - 10 ) {
                SendMessage( WM_VSCROLL, SB_LINEDOWN);
            }



            if (bRight) // need to scroll up, go slow if cursor near treeview control
            {
                SendMessage(WM_HSCROLL,MAKEWPARAM(SB_LINERIGHT,0) ,(LPARAM)(NULL));
            }
            else        // need to scroll down, go slow if cursor near treeview control
            {
                SendMessage(WM_HSCROLL,MAKEWPARAM(SB_LINELEFT,0) ,(LPARAM)(NULL));
            }
            m_pDragImage->DragShowNolock(true); // show the image being dragged w/o locking the window
        }
    }
    else {
        if (nFlags & MK_LBUTTON && !(nFlags & MK_CONTROL) && !(nFlags & MK_SHIFT) && ShouldStartDragging(point))
        {
            BeginDrag(point);
        }
    }
    CTreeCtrl::OnMouseMove(nFlags, point);
}

// ***************************************************************************

void CFormTreeCtrl::BeginDrag(CPoint )
{
    CPoint      ptAction;

    // This code is to prevent accidental drags

    if( (GetTickCount() - m_dwDragStart) < DRAG_DELAY) {
        return;
    }
    ASSERT(!m_bDragging);   // prob if this guy not true

    GetCursorPos(&ptAction);
    ScreenToClient(&ptAction);

    HTREEITEM   m_hitemDrag = GetSelectedItem();

    // create a dragging bitmap for the given item in a tree view control

    m_pDragImage = CreateDragImageEx(m_hitemDrag);

    if( !m_pDragImage) {    // something's gone wrong if this occurred
        return;
    }

    m_bDragging = true;

    m_pDragImage->DragShowNolock(true); // show the image being dragged w/o locking the window
    m_pDragImage->SetDragCursorImage(0, CPoint(0, 0));
    m_pDragImage->BeginDrag(0, CPoint(0,0));    // start draggin'!

    // ClientToScreen onverts the coords of the given pt or rect to screen coords;
    // it replaces the coords in the structure with the new coords; the new screen
    // coords are relative to the upper-left corner of the system display

    ClientToScreen(&ptAction);

    // [1] move the image that is being dragged
    // [2] locks updates to the window specified and displays the drag image at the pos indicated

    m_pDragImage->DragMove(ptAction);                       // [1]
    m_pDragImage->DragEnter(GetDesktopWindow(), ptAction);  // [2]

    SetCapture();
}


// ***************************************************************************

void CFormTreeCtrl::OnDeleteitem(NMHDR* pNMHDR, LRESULT* pResult)
{
    NM_TREEVIEW* pNMTreeView = (NM_TREEVIEW*)pNMHDR;

    // TODO: Add your control notification handler code here

    HTREEITEM hItem = pNMTreeView->itemOld.hItem;

    CFormID* pID = (CFormID*) GetItemData(hItem);
    SetItemData(hItem,NULL);


    if(pID)
    {
        delete pID ;
    }
    *pResult = 0;
}

// ***************************************************************************
// origTI = the original tree item

void CFormTreeCtrl::DeleteActiveItem (HTREEITEM origTI)
{
    CFormID*    pNodeID = (CFormID*) GetItemData (origTI);

    eNodeType   nodeType = pNodeID->GetItemType();


    if (nodeType == eFTT_FORMFILE || nodeType == eINVALID_TYPE)

        return; // can't delete a form file from the tree; must close it out & delete file

    if (nodeType == eFTT_LEVEL)

        return; // don't think we're gonna allow this either folx

    CFormDoc*           pFormDoc = pNodeID->GetFormDoc();
    CFormScrollView*    pView = GetFormView();

    CFormChildWnd* pFrame = (CFormChildWnd*)pView->GetParentFrame();
    if (pFrame->GetViewMode() != FormViewMode) {
        return;
    }
    pFormDoc->SetModifiedFlag(true);    // mark the file as dirty

    if (nodeType == eFTT_FORM)
    {
        if (pView->OkToDeleteForm())    // now that user can undo delete, don't ask if they're sure!
        {
            pView->DeleteActiveGroup ((CDEGroup*) pNodeID->GetItemPtr());
        }
    }
    else if (nodeType == eFTT_BLOCK)
    {
        bool bDeleteBlockFields = false;
        int iRet = AfxMessageBox(_T("Do you also want to delete the fields in the block?"), MB_YESNOCANCEL);
        if (iRet == IDCANCEL) {
            return;
        }
        if (iRet == IDYES) {
            bDeleteBlockFields = true;
        }
        pView->DeleteBlock(assert_cast<CDEBlock&>(*pNodeID->GetItemPtr()), bDeleteBlockFields);
    }
    else    // it's a field or roster
    {
        pView->DeleteActiveItem ();
    }

    // tree rebuilding will be taken care of in the view; i want to have one func
    // that can be used from the tree ctrl, view, or menu bar, and so i'm choosing
    // the view

    SetFocus();
}

// ***************************************************************************

void CFormTreeCtrl::OnKeyDown(UINT nChar, UINT nRepCnt, UINT nFlags)
{
    switch (nChar)
    {
        case VK_DELETE:
            DeleteActiveItem (GetSelectedItem());   // VK_DELETE = virtual key code for DEL key
            break;
        case VK_DOWN :
            if (GetKeyState(VK_SHIFT) & 0x8000) {
                // Shift+click selects consecutive items
                auto selected = GetSelected();
                if (!selected.empty()) {
                    HTREEITEM nextSel = GetNextVisibleItem(GetSelectedItem());
                    if (nextSel && CanBePartOfMultipleSelection(nextSel)) {
                        if (GetSelectedItem() == selected.front())
                        {
                            SelectItem(nextSel);
                            SelectRange(nextSel, selected.back());
                        }
                        else
                        {
                            SelectItem(nextSel);
                            SelectRange(nextSel, selected.front());
                        }
                        NotifyViewSelectionChanged();
                    }
                }
            }
            else
            {
                HTREEITEM nextSel = GetNextVisibleItem(GetSelectedItem());
                if (nextSel) {
                    ClearSelection();
                    SelectItem(nextSel);
                }
            }
            break;
        case VK_UP:
            if (GetKeyState(VK_SHIFT) & 0x8000) {
                // Shift+click selects consecutive items
                auto selected = GetSelected();
                if (!selected.empty()) {
                    HTREEITEM nextSel = GetPrevVisibleItem(GetSelectedItem());
                    if (nextSel && CanBePartOfMultipleSelection(nextSel)) {
                        if (GetSelectedItem() == selected.front())
                        {
                            SelectItem(nextSel);
                            SelectRange(nextSel, selected.back());
                        }
                        else
                        {
                            SelectItem(nextSel);
                            SelectRange(nextSel, selected.front());
                        }
                        NotifyViewSelectionChanged();
                    }
                }
            }
            else
            {
                HTREEITEM nextSel = GetPrevVisibleItem(GetSelectedItem());
                if (nextSel) {
                    ClearSelection();
                    SelectItem(nextSel);
                }
            }
            break;
        default:
            CTreeCtrl::OnKeyDown(nChar, nRepCnt, nFlags); // BMD 24 Oct 2001
            break;
    }
}

// ***************************************************************************
//  this func either adds a node to the dict tree, or adds a reference

bool CFormTreeCtrl::InsertFormDependencies(CFormNodeID* pID)
{
    CSpecFile specFile(true);

    if( !specFile.Open(pID->GetFFName(), CFile::modeRead) )
    {
        AfxMessageBox(FormatText(_T("File %s Could not be opened"), pID->GetFFName().GetString()));
        return false;
    }

    std::vector<std::wstring> dictionary_filenames = GetFileNameArrayFromSpecFile(specFile, CSPRO_DICTS);
    specFile.Close();

    if( dictionary_filenames.empty() )
    {
        AfxMessageBox(FormatText(_T("No Dictionaries in Spec file %s"), pID->GetFFName().GetString()));
        return false;
    }

    CDDTreeCtrl* pDictTree = GetDDTreeCtrl();    //  Get the handle to tree controls

    for( const std::wstring& dictionary_filename : dictionary_filenames )
    {
        DictionaryDictTreeNode* dictionary_dict_tree_node = pDictTree->GetDictionaryTreeNode(dictionary_filename);

        if( dictionary_dict_tree_node != nullptr )
        {
            dictionary_dict_tree_node->AddRef();
        }

        else
        {
            try
            {
                LabelSet dictionary_label_set = JsonStream::GetValueFromSpecFile<LabelSet, CDataDict>(JK::labels, dictionary_filename);

                TVITEM pItem;
                pItem.hItem = pDictTree->InsertDictionary(dictionary_label_set.GetLabel(), WS2CS(dictionary_filename), nullptr);
                pItem.mask = TVIF_CHILDREN;
                pItem.cChildren = 1;
                pDictTree->SetItem(&pItem);
            }

            catch( const CSProException& exception )
            {
		        ErrorMessage::Display(exception);
            }
        }
    }

    return true;
}

// ***************************************************************************

bool CFormTreeCtrl::ReleaseFormDependencies(CFormNodeID* pID)
{
    CSpecFile specFile(true);
    std::vector<std::wstring> dictionary_filenames;

    // whether or not form doc has been opened (which at the application or project level
    // it may not necessarily be), we need to try and close the form's associated array
    // of dictionaries; if:

    // [1]  the form doc is loaded, then copy the names from there, as the user could have
    //      added a dictionary but not yet serialized that modification
    // [2]  otherwise, if no form doc is loaded, then grab the list of dicts from the .fmf file

    if (pID->GetFormDoc() != NULL)  // [1]
    {
        CDEFormFile* pFF = &pID->GetFormDoc()->GetFormFile();
        dictionary_filenames.emplace_back(CS2WS(pFF->GetDictionaryFilename()));
    }

    else if (specFile.Open(pID->GetFFName(), CFile::modeRead))  // [2]
    {
        dictionary_filenames = GetFileNameArrayFromSpecFile(specFile, CSPRO_DICTS);
        specFile.Close();

        if( dictionary_filenames.empty() )
        {
            AfxMessageBox(FormatText(_T("No Dictionaries in Spec file %s"), pID->GetFFName().GetString()));
            return false;
        }
    }

    else    // associated err msg not particularly accurate...
    {
        AfxMessageBox(FormatText(_T("File %s Could not be opened"), pID->GetFFName().GetString()));
        return false;
    }

    CDDTreeCtrl* pDictTree = GetDDTreeCtrl();

    if (pDictTree == NULL)
        return false;

    for( const std::wstring& dictionary_filename : dictionary_filenames )
    {
        DictionaryDictTreeNode* dictionary_dict_tree_node = pDictTree->GetDictionaryTreeNode(dictionary_filename);

        if( dictionary_dict_tree_node != nullptr )
            pDictTree->ReleaseDictionaryNode(*dictionary_dict_tree_node);
    }

    return true;
}

BOOL CFormTreeCtrl::OpenFormFile (const CString &sFormFile ,BOOL bMakeVisible /* =TRUE*/)
{
    CFormNodeID * pID = GetFormNode(sFormFile);
    if(!pID)
        return FALSE;

    if (pID->GetFormDoc())
        return true;


    CFormDoc* pDoc = (CFormDoc*) m_pDocTemplate->OpenDocumentFile(sFormFile,bMakeVisible);

    CFormScrollView* pView = NULL;
    if(pDoc && !bMakeVisible) {

        pView = (CFormScrollView*)pDoc->GetView();
        pView->GetParentFrame()->ShowWindow(SW_SHOWNOACTIVATE);
        pView->OnInitialUpdate(); // OnInitial Update will not be called when bMakeVisible = FALSE;
    }


    SetFormDoc (pDoc);  // set the tree ctrl's ptr to the doc
    pDoc->SetFormTreeCtrl (this); //set the tree  ctrl before doing the Loadformspecfile

    if (!pDoc->LoadFormSpecFile(sFormFile) )
    {
        pDoc->OnCloseDocument();
        return  false;
    }
    pID->SetFormDoc(pDoc);

//  pDoc->LoadDictSpecFile(false); SAVY 11/03 the Loadformspecfile  does this now.
//  get back to the original method 'cos of app open and fmf open .
//  to fix the problem when the dictionary is not found for the form file
    if(pDoc->LoadDictSpecFile(false)){
        int i = pDoc->GetFormFile().GetNumForms();
        if (i > 0) {

            CString csErr;
            if(!pDoc->GetFormFile().Reconcile(csErr,false,true)) {
                pDoc->SetModifiedFlag(TRUE);
                pDoc->GetUndoStack().Clear();
            }

            pView = (CFormScrollView*)pDoc->GetView();

            CFormScrollView* pFV = pView;
            pDoc->SetCurFormIndex (0);
            pFV->SetFormIndex (0);
            pFV->RecreateGrids (0);
            pFV->SetupFontRelatedOffsets();
        }
    }
    else {
        return false;
    }

    ReBuildTree();

//  GetFormView()->MarkDictTree();

    return true;
}

// ********************************************************************
// these are all the handlers nec for OnRButtonUp()

void CFormTreeCtrl::OnAddBlock()
{
    auto selectedItems = GetSelectedFormItems();
    if (selectedItems.empty())
        return;

    CFormScrollView* pFV = GetFormView();   // get the currently displayed form
    CDEFormFile* pFF = pFV->GetFormFile();

    auto pFirstSelected = selectedItems.front();

    CDEGroup* pParentGroup;
    int insertIndex;
    int blockSize;

    switch (selectedItems.front()->GetItemType())
    {
    case eFTT_FIELD:
    {
        auto pFirstSelectedItem = static_cast<CDEItemBase*>(pFirstSelected->GetItemPtr());
        pParentGroup = pFirstSelectedItem->GetParent();
        insertIndex = pParentGroup->GetItemIndex(pFirstSelectedItem);
        blockSize = selectedItems.size();
        break;

    }
    case eFTT_GRIDFIELD:
    {
        auto pRoster = static_cast<CDERoster*>(pFirstSelected->GetItemPtr());
        auto pFirstSelectedItem = pRoster->GetCol(pFirstSelected->GetColumnIndex())->GetField(pFirstSelected->GetRosterField());
        pParentGroup = pRoster;
        insertIndex = pParentGroup->GetItemIndex(pFirstSelectedItem);
        blockSize = selectedItems.size();
        break;
    }
    case eFTT_FORM:
        // For form add empty block to end of form
        pParentGroup = (CDEGroup*)selectedItems.front()->GetItemPtr();
        insertIndex = pParentGroup->GetNumItems();
        blockSize = 0;
        break;
    case eFTT_GRID:
        // For roster insert empty block at end of roster
        pParentGroup = (CDERoster*)selectedItems.front()->GetItemPtr();
        insertIndex = pParentGroup->GetNumItems();
        blockSize = 0;
        break;
    default:
        return;
    }

    CDEBlock* pNewBlock = new CDEBlock();
    pNewBlock->SetLabel(_T("New Block"));
    pNewBlock->SetName(pFF->CreateUniqueName(_T("BLOCK"), false));  // generate a uniq name but DON'T insert it yet!
    pNewBlock->SetNumFields(blockSize);
    pNewBlock->SetDisplayTogether(true);
    pNewBlock->SetFormNum(pParentGroup->GetFormNum());
    pNewBlock->SetParent(pParentGroup);

    pParentGroup->InsertItemAt(pNewBlock, insertIndex);

    pFV->m_pRightClickItem = pNewBlock;

    OnEditBlockProp();  // let the user modify as desired logic group's name & label

    if (pFV->m_pRightClickItem == NULL) // this is my signal that the user wants to bail
    {
        pParentGroup->RemoveItemAt(insertIndex);
        delete pNewBlock;
        return;
    }

    pFV->GetDocument()->PushUndo(FormUndoStack(CFormUndoObj::Action::UR_add, pNewBlock, insertIndex, pParentGroup->GetName()));

    pFV->GetDocument()->GetFormTreeCtrl()->ReBuildTree(pFV->GetFormIndex(), pNewBlock);

    // a logic group containing a logic group doesn't, per se, point to anything,
    // so remove all extant trackers

    pFV->RemoveAllTrackersAndRefresh();
}

// ********************************************************************
// choosing to ungroup an item from the tree ctrl will not have as many
// tests as necessary from the view (for there i need to ensure that ll
// items in the group were selected for ungrouping)

void CFormTreeCtrl::OnUngroupItems()
{
/*  // this blk is tree-ctrl specific

    HTREEITEM hItem = GetSelectedItem();

    CFormID*  pID = (CFormID*) GetItemData (hItem);

    eNodeType eNT = pID->GetItemType();

    if (eNT != eFTT_LOGIC)

        return;     // error!


    CDEGroup* pCurGroup = (CDEGroup*) pID->GetItemPtr();
    // by now, e'thing shld be ok, ungroup it

    // find insert location of the logic group
    // insert all items from the logic group from that point on
    // delete the logic group

    GetFormView()->UngroupSelectedItems (pCurGroup); */
}

void CFormTreeCtrl::OnEditFieldProp()
{
    CFormScrollView* pFV = GetFormView();   // get the currently displayed form

    auto selectedItems = GetSelectedFormItems();
    if (selectedItems.size() == 1)
        pFV->SendMessage(WM_COMMAND, ID_EDIT_FIELDPROP);
    else if (selectedItems.front()->GetItemType() == eFTT_FIELD) {
        std::vector<CDEField*> fields;
        for( CFormID* pID : GetSelectedFormItems() )
            fields.push_back(static_cast<CDEField*>(pID->GetItemPtr()));
        pFV->OnEditMultipleFieldProperties(fields);
    }
    else if (selectedItems.front()->GetItemType() == eFTT_GRIDFIELD) {
        CFormGrid* pGrid = pFV->FindGrid(static_cast<CDERoster*>(selectedItems.front()->GetItemPtr()));
        pGrid->OnEditMultipleFieldProperties();
    }
}

void CFormTreeCtrl::OnEditLevelProp()
{
    CFormScrollView* pFV = GetFormView();   // get the currently displayed form
    pFV->SendMessage(WM_COMMAND, ID_EDIT_LEVELPROP);
}

void CFormTreeCtrl::OnEditFFProp()
{
    CFormScrollView* pFV = GetFormView();   // get the currently displayed form
    pFV->SendMessage(WM_COMMAND, ID_EDIT_FFPROP);
}


void CFormTreeCtrl::OnEditFormProp()
{
    CFormScrollView* pFV = GetFormView();   // get the currently displayed form
    pFV->SendMessage(WM_COMMAND, ID_EDIT_FORMPROP);
}

void CFormTreeCtrl::OnEditBlockProp()
{
    CFormScrollView* pFV = GetFormView();   // get the currently displayed form

    if (pFV->m_pRightClickItem == NULL ||
        pFV->m_pRightClickItem->GetItemType() != CDEFormBase::Block)
        return;

    CDEBlock* pBlock = (CDEBlock*)pFV->m_pRightClickItem;

    CBlockPropDlg dlg(pBlock, pFV);

    if (dlg.DoModal() == IDOK)
    {
        CDEFormFile* pFF = &pFV->GetDocument()->GetFormFile();

        pFV->GetDocument()->PushUndo(FormUndoStack(CFormUndoObj::Action::UR_modify, pBlock, pBlock->GetParent()->GetItemIndex(pBlock), pBlock->GetParent()->GetName()));

        CString old_name = pBlock->GetName();
        pFF->RemoveUniqueName(pBlock->GetName());

        pBlock->SetLabel(dlg.m_sBlockLabel);
        pBlock->SetName(dlg.m_sBlockName);
        pBlock->SetDisplayTogether(dlg.m_bDisplayOnSameScreen);
        pFF->AddUniqueName(pBlock->GetName());
        pFF->BuildUniqueNL();

        pFV->GetDocument()->SetModifiedFlag(true);

        std::tuple<CDEItemBase*, CString> update(pBlock, old_name);
        AfxGetMainWnd()->SendMessage(WM_IMSA_RECONCILE_QSF_FIELD_NAME, reinterpret_cast<WPARAM>(pFF), reinterpret_cast<LPARAM>(&update));

        ReBuildTree(pFV->GetFormIndex(), pBlock);
        pFV->m_pRightClickItem = pBlock;

    }
    else        // this is my signal on rtn that user bailed and the group shld be deleted
    {
        pFV->m_pRightClickItem = NULL;
    }
}

void CFormTreeCtrl::OnEditGridAutoFit()
{
    CFormScrollView* pFV = GetFormView();   // get the currently displayed form
    pFV->SendMessage(WM_COMMAND, ID_EDIT_GRID_AUTOFIT);
}

void CFormTreeCtrl::OnEditGridProp()
{
    CFormScrollView* pFV = GetFormView();   // get the currently displayed form
    pFV->SendMessage(WM_COMMAND, ID_EDIT_GRIDPROP);
}


void CFormTreeCtrl::OnEditReportProperties()
{
    const CFormID* pID = GetSelectedFormId();

    if( pID == nullptr || pID->GetItemType() != eFFT_REPORT )
        return;

    if( AfxGetMainWnd()->SendMessage(UWM::Designer::EditReportProperties,
        (WPARAM)((FormReportID*)pID)->GetNamedTextSource(), (LPARAM)pID->GetFormDoc()) == 1 )
    {
        // on success, redraw the tree
        Invalidate();
    }
}


CFormID* CFormTreeCtrl::GetSelectedFormId()
{
    HTREEITEM hItem = GetSelectedItem();
    return ( hItem != nullptr ) ? (CFormID*)GetItemData(hItem) : nullptr;
}


void CFormTreeCtrl::OnDeleteItem()
{
    DeleteActiveItem(GetSelectedItem());
}

void CFormTreeCtrl::OnAddForm()
{
    CFormScrollView* pFV = GetFormView();   // get the currently displayed form

    HTREEITEM hItem = GetSelectedItem();

    CFormID*  pID = (CFormID*) GetItemData (hItem);

    eNodeType eNT = pID->GetItemType();

    if (eNT == eFTT_LEVEL)
    {
        pFV->m_pRightClickItem = ((CDELevel*) pID->GetItemPtr())->GetRoot();
    }
    else if (eNT == eFTT_FIELD)
    {
        pFV->m_pRightClickItem = ((CDEItemBase*) pID->GetItemPtr())->GetParent()->GetParent();
    }
    else if (eNT == eFTT_FORM || eNT == eFTT_BLOCK) // if it's not, we shldn't be here
    {
        pFV->m_pRightClickItem = ((CDEItemBase*) pID->GetItemPtr())->GetParent();
    }

    pFV->SendMessage(WM_COMMAND, ID_ADD_FORM);
}

void CFormTreeCtrl::OnDeleteForm()
{
    CFormScrollView* pFV = GetFormView();   // get the currently displayed form
    pFV->SendMessage(WM_COMMAND, ID_DELETE_FORM);
}


// ***************************************************************************
// this is called when the user tries to drag a node w/in the form tree ctrl

void CFormTreeCtrl::DropFormTreeItem (std::vector<CFormID*> pDragFromIDs, CPoint dropPoint)
{
    ASSERT(!pDragFromIDs.empty());

    ScreenToClient(&dropPoint);    // coords now relative to the upper-left corner of my CWnd

    HTREEITEM hItem = HitTest(dropPoint);

    if (!hItem)     // if nothing selected, bail
        return;

    CFormID* pDropOnID = (CFormID*) GetItemData (hItem);
    CFormID* pDragFromID = pDragFromIDs.front();
    eNodeType eDragFromType = pDragFromIDs.front()->GetItemType();
    eNodeType eDropOnType = pDropOnID->GetItemType();

    // can only reorder rosters/fields among themselves, or forms among themselves

    if ((eDragFromType == eFTT_GRID ||
        eDragFromType == eFTT_FIELD)
        &&
        (eDropOnType == eFTT_GRID ||
            eDropOnType == eFTT_FIELD))

        DropItemOnItem(pDragFromIDs, pDropOnID);
    else if (eDragFromType == eFTT_GRID && eDropOnType == eFTT_FORM)
        DropRosterOnForm(pDragFromIDs, pDropOnID);
    else if (eDragFromType == eFTT_GRID && eDropOnType == eFTT_BLOCK)
        DropRosterOnBlock(pDragFromIDs, pDropOnID);
    else if (eDragFromType == eFTT_GRIDFIELD && eDropOnType == eFTT_GRIDFIELD)
        DropRosterItemOnRI(pDragFromIDs, pDropOnID);
    else if (eDragFromType == eFTT_FORM && eDropOnType == eFTT_FORM)
        DropFormOnForm(pDragFromID, pDropOnID);
    else if (eDragFromType == eFTT_FIELD && eDropOnType == eFTT_BLOCK)
        DropItemOnBlock(pDragFromIDs, pDropOnID);
    else if (eDragFromType == eFTT_GRIDFIELD && eDropOnType == eFTT_BLOCK)
        DropRosterItemOnBlock(pDragFromIDs, pDropOnID);
    else if (eDragFromType == eFTT_BLOCK && (eDropOnType == eFTT_FIELD || eDropOnType == eFTT_GRIDFIELD))
        DropBlockOnItem(pDragFromIDs, pDropOnID);
    else if (eDragFromType == eFTT_BLOCK && eDropOnType == eFTT_BLOCK)
        DropBlockOnBlock(pDragFromIDs, pDropOnID);
    else if (eDragFromType == eFTT_BLOCK && eDropOnType == eFTT_GRID)
        DropBlockOnRoster(pDragFromIDs, pDropOnID);
    else if (eDragFromType == eFTT_BLOCK && eDropOnType == eFTT_FORM)
        DropBlockOnForm(pDragFromIDs, pDropOnID);
    else if (eDragFromType == eFTT_FIELD && eDropOnType == eFTT_FORM)
        DropItemOnForm(pDragFromIDs, pDropOnID);
    else if (eDragFromType == eFTT_GRIDFIELD && eDropOnType == eFTT_GRID)
        DropRosterItemOnRoster(pDragFromIDs, pDropOnID);

//  else, invalid drop
}

// ***************************************************************************
// this gets invoked if the user is trying to reorder items w/in the same form

void CFormTreeCtrl::DropItemOnItem (std::vector<CFormID*> dragFromIDs, CFormID* pDropOnID)
{
    CDEItemBase* pDT_Item = dynamic_cast<CDEItemBase*>(pDropOnID->GetItemPtr());

    // Save selected item so we can restore after drop
    CDEItemBase* pSelected = GetSelectedItem() ? (CDEItemBase*) ((CFormID*) GetItemData(GetSelectedItem()))->GetItemPtr() : nullptr;

    // Convert the list of dragged form ids to a list of corresponding CDEItemBase
    std::vector<CDEField*> dragItems;
    dragItems.reserve(dragFromIDs.size());
    std::transform(dragFromIDs.begin(), dragFromIDs.end(), std::back_inserter(dragItems), [](CFormID* pID) { return static_cast<CDEField*>(pID->GetItemPtr()); });

    // Check if the drop target is one of the dragged items - that should be a no op
    auto droppedInDrag = std::find(dragItems.begin(), dragItems.end(), pDT_Item);
    if (droppedInDrag != dragItems.end())
        return;


    CDEGroup* pGroup =  pDT_Item->GetParent();

    ASSERT(pGroup != NULL);
    // Check to make sure that all the items being dropped are from same
    // parent as drop target. If any are not, bail and don't do the drop
    auto droppedDiffParent = std::find_if(dragItems.begin(), dragItems.end(),
        [pGroup](CDEItemBase* const i) { return i->GetParent() != pGroup; });
    if (droppedDiffParent != dragItems.end())
        return;

    UpdateGroupItemsForDrop(pGroup, dragItems, pDT_Item);

    ReBuildTree (pDT_Item->GetFormNum(), pSelected);
    SelectNodes(std::vector<CDEFormBase*>(dragItems.begin(), dragItems.end()));

    GetFormDoc()->SetModifiedFlag(true);
}

// ***************************************************************************
// this gets invoked if the user is trying to reorder items w/in the same roster

void CFormTreeCtrl::DropRosterItemOnRI (std::vector<CFormID*> dragFromIDs, CFormID* pDropOnID)
{
    ASSERT(dragFromIDs.size() > 0);
    CDERoster* pRoster = (CDERoster*)pDropOnID->GetItemPtr();

    // Check that drag items are all from same roster as the drop item
    if (!std::all_of(dragFromIDs.begin(), dragFromIDs.end(), [pRoster](CFormID* pID) { return (CDERoster*)pID->GetItemPtr() == pRoster; }))
        return;

    CDECol* pDropCol = pRoster->GetCol(pDropOnID->GetColumnIndex());
    CDEField* pDropField = pDropCol->GetField(pDropOnID->GetRosterField());

    std::vector<CDEField*> dragFields;
    std::transform(dragFromIDs.begin(), dragFromIDs.end(), std::back_inserter(dragFields), [pRoster](CFormID* pID) { return pRoster->GetCol(pID->GetColumnIndex())->GetField(pID->GetRosterField()); });

    // Check if the drop target is one of the dragged items - that should be a no op
    auto droppedInDrag = std::find(dragFields.begin(), dragFields.end(), pDropField);
    if (droppedInDrag != dragFields.end())
        return;

    // This does the drag and drop for items in the group array
    UpdateGroupItemsForDrop(pRoster, dragFields, pDropField);

    // This updates roster columns to match
    UpdateRosterColumnsForDrop(pRoster, dragFields, pDropField, true);

    ReBuildTree(pRoster->GetFormNum(),dragFields.front());
    GetFormDoc()->SetModifiedFlag(true);
}

void CFormTreeCtrl::DropItemOnBlock(std::vector<CFormID*> dragFromIDs, CFormID* pDropOnID)
{
    ASSERT(pDropOnID->GetItemType() == eFTT_BLOCK);
    CDEBlock* pDropBlock = static_cast<CDEBlock*>(pDropOnID->GetItemPtr());

    // Save selected item so we can restore after drop
    CDEItemBase* pSelected = GetSelectedItem() ? (CDEItemBase*)((CFormID*)GetItemData(GetSelectedItem()))->GetItemPtr() : nullptr;

    // Convert the list of dragged form ids to a list of corresponding CDEItemBase
    std::vector<CDEItemBase*> dragItems;
    dragItems.reserve(dragFromIDs.size());
    std::transform(dragFromIDs.begin(), dragFromIDs.end(), std::back_inserter(dragItems), [](CFormID* pID) { return (CDEItemBase*)pID->GetItemPtr(); });

    CDEGroup* pGroup = pDropBlock->GetParent();

    // Check to make sure that all the items being dropped are from same
    // parent as drop target. If any are not, bail and don't do the drop
    auto droppedDiffParent = std::find_if(dragItems.begin(), dragItems.end(),
        [pGroup](CDEItemBase* const i) { return i->GetParent() != pGroup; });
    if (droppedDiffParent != dragItems.end())
        return;

    std::vector<CDEField*> dragFields;
    std::transform(dragFromIDs.begin(), dragFromIDs.end(), std::back_inserter(dragFields), [](CFormID* pID) { return static_cast<CDEField*>(pID->GetItemPtr()); });

    UpdateGroupItemsForDrop(pGroup, dragFields, pDropBlock);

    ReBuildTree(pDropBlock->GetFormNum(), pSelected);
    SelectNodes(std::vector<CDEFormBase*>(dragItems.begin(), dragItems.end()));

    GetFormDoc()->SetModifiedFlag(true);
}

void CFormTreeCtrl::DropRosterItemOnBlock(std::vector<CFormID*> dragFromIDs, CFormID* pDropOnID)
{
    ASSERT(pDropOnID->GetItemType() == eFTT_BLOCK);
    CDEBlock* pDropBlock = static_cast<CDEBlock*>(pDropOnID->GetItemPtr());

    ASSERT(dragFromIDs.size() > 0);

    // Make sure this is a block in roster
    if (pDropBlock->GetParent()->GetItemType() != CDEFormBase::Roster)
        return;

    CDERoster* pRoster = static_cast<CDERoster*>(pDropBlock->GetParent());

    // Check that drag items are all from same roster as the drop item
    if (!std::all_of(dragFromIDs.begin(), dragFromIDs.end(), [pRoster](CFormID* pID) { return (CDERoster*)pID->GetItemPtr() == pRoster; }))
        return;

    std::vector<CDEField*> dragFields;
    std::transform(dragFromIDs.begin(), dragFromIDs.end(), std::back_inserter(dragFields), [pRoster](CFormID* pID) { return pRoster->GetCol(pID->GetColumnIndex())->GetField(pID->GetRosterField()); });

    // Find first field after block to figure out which column to drop on
    CDEField* pDropField;
    int iDropFieldIndex = pRoster->GetItemIndex(pDropBlock);
    while (iDropFieldIndex < pRoster->GetNumItems() && pRoster->GetItem(iDropFieldIndex)->GetItemType() != CDEFormBase::Field)
        ++iDropFieldIndex;
    if (iDropFieldIndex != pRoster->GetNumItems())
    {
        pDropField = static_cast<CDEField*>(pRoster->GetItem(iDropFieldIndex));
    }
    else {
        // Block is at the end of roster - will add at end of roster
        pDropField = nullptr;
    }

    // Handle the group items
    UpdateGroupItemsForDrop(pRoster, dragFields, pDropBlock);

    // Update columns to match
    UpdateRosterColumnsForDrop(pRoster, dragFields, pDropField, true);

    ReBuildTree(pDropBlock->GetFormNum(), pDropBlock);
    GetFormDoc()->SetModifiedFlag(true);
}

void CFormTreeCtrl::DropBlockOnItem(std::vector<CFormID*> dragFromIDs, CFormID* pDropOnID)
{
    ASSERT(dragFromIDs.size() == 1);
    ASSERT(dragFromIDs.front()->GetItemType() == eFTT_BLOCK);
    ASSERT(pDropOnID->GetItemType() == eFTT_FIELD || pDropOnID->GetItemType() == eFTT_GRIDFIELD);

    CDEBlock* pDragBlock = static_cast<CDEBlock*>(dragFromIDs.front()->GetItemPtr());

    CDEField* pDropField;
    if (pDropOnID->GetItemType() == eFTT_FIELD) {
        pDropField = static_cast<CDEField*>(pDropOnID->GetItemPtr());
    }
    else {
        CDERoster* pRoster = static_cast<CDERoster*>(pDropOnID->GetItemPtr());
        pDropField = pRoster->GetCol(pDropOnID->GetColumnIndex())->GetField(pDropOnID->GetRosterField());
    }

    // Don't allow drop if item is already in block
    if (pDropField->GetParent()->GetBlock(pDropField))
        return;

    CDEGroup* pDropParent = pDropField->GetParent();

    if (pDragBlock->GetParent() != pDropParent)
        return;

    // Get the block items
    std::vector<CFormID*> blockItems;
    HTREEITEM hItem = GetChildItem(dragFromIDs.front()->GetHItem());
    while (hItem) {
        blockItems.push_back(reinterpret_cast<CFormID*>(GetItemData(hItem)));
        hItem = GetNextItem(hItem, TVGN_NEXT);
    }
    ASSERT((int) blockItems.size() == pDragBlock->GetNumFields());

    // Remove the block from current location
    int oldBlockIndex = pDragBlock->GetParent()->GetItemIndex(pDragBlock);
    pDragBlock->GetParent()->RemoveItemAt(oldBlockIndex);

    // Drag and drop the items
    if (!blockItems.empty()) {
        if (pDropOnID->GetItemType() == eFTT_FIELD)
            DropItemOnItem(blockItems, pDropOnID);
        else
            DropRosterItemOnRI(blockItems, pDropOnID);
    }

    // Reinsert the block
    int dropIndex = pDropParent->GetItemIndex(pDropField) - pDragBlock->GetNumFields();
    pDragBlock->GetParent()->InsertItemAt(pDragBlock, dropIndex);

    ReBuildTree(pDragBlock->GetFormNum(), pDragBlock);

    GetFormDoc()->SetModifiedFlag(true);
}

void CFormTreeCtrl::DropItemOnForm(std::vector<CFormID*> dragFromIDs, CFormID* pDropOnID)
{
    ASSERT(pDropOnID->GetItemType() == eFTT_FORM);
    CDEGroup* pDropGroup = static_cast<CDEGroup*>(pDropOnID->GetItemPtr());

    // Save selected item so we can restore after drop
    CDEItemBase* pSelected = GetSelectedItem() ? (CDEItemBase*)((CFormID*)GetItemData(GetSelectedItem()))->GetItemPtr() : nullptr;

    // Convert the list of dragged form ids to a list of corresponding CDEItemBase
    std::vector<CDEItemBase*> dragItems;
    dragItems.reserve(dragFromIDs.size());
    std::transform(dragFromIDs.begin(), dragFromIDs.end(), std::back_inserter(dragItems), [](CFormID* pID) { return (CDEItemBase*)pID->GetItemPtr(); });

    // Check to make sure that all the items being dropped are from same
    // parent as drop target. If any are not, bail and don't do the drop
    auto droppedDiffParent = std::find_if(dragItems.begin(), dragItems.end(),
        [pDropGroup](CDEItemBase* const i) { return i->GetParent() != pDropGroup; });
    if (droppedDiffParent != dragItems.end())
        return;

    std::vector<CDEField*> dragFields;
    std::transform(dragFromIDs.begin(), dragFromIDs.end(), std::back_inserter(dragFields), [](CFormID* pID) { return static_cast<CDEField*>(pID->GetItemPtr()); });

    UpdateGroupItemsForDrop(pDropGroup, dragFields, pDropGroup);

    ReBuildTree(pDropGroup->GetFormNum(), pSelected);
    SelectNodes(std::vector<CDEFormBase*>(dragItems.begin(), dragItems.end()));

    GetFormDoc()->SetModifiedFlag(true);
}

void CFormTreeCtrl::DropRosterOnForm(std::vector<CFormID*> dragFromIDs, CFormID* pDropOnID)
{
    ASSERT(pDropOnID->GetItemType() == eFTT_FORM);
    CDEGroup* pDropGroup = static_cast<CDEGroup*>(pDropOnID->GetItemPtr());

    // Save selected item so we can restore after drop
    CDEItemBase* pSelected = GetSelectedItem() ? (CDEItemBase*)((CFormID*)GetItemData(GetSelectedItem()))->GetItemPtr() : nullptr;

    // Convert the list of dragged form ids to a list of corresponding CDEItemBase
    std::vector<CDEItemBase*> dragItems;
    dragItems.reserve(dragFromIDs.size());
    std::transform(dragFromIDs.begin(), dragFromIDs.end(), std::back_inserter(dragItems), [](CFormID* pID) { return (CDEItemBase*)pID->GetItemPtr(); });

    // Check to make sure that all the items being dropped are from same
    // parent as drop target. If any are not, bail and don't do the drop
    auto droppedDiffParent = std::find_if(dragItems.begin(), dragItems.end(),
        [pDropGroup](CDEItemBase* const i) { return i->GetParent() != pDropGroup; });
    if (droppedDiffParent != dragItems.end())
        return;

    CDERoster* pDragRoster = static_cast<CDERoster*>(dragFromIDs.front()->GetItemPtr());

    int oldRosterIndex = pDragRoster->GetParent()->GetItemIndex(pDragRoster);
    pDragRoster->GetParent()->RemoveItemAt(oldRosterIndex);

    // insert the roster as the first item on the form
    int dropIndex = 0;
    pDropGroup->InsertItemAt(pDragRoster, dropIndex);

    ReBuildTree(pDropGroup->GetFormNum(),pSelected);
    GetFormDoc()->SetModifiedFlag(true);
}

void CFormTreeCtrl::DropBlockOnForm(std::vector<CFormID*> dragFromIDs, CFormID* pDropOnID)
{
    ASSERT(pDropOnID->GetItemType() == eFTT_FORM);
    CDEGroup* pDropGroup = static_cast<CDEGroup*>(pDropOnID->GetItemPtr());

    // Save selected item so we can restore after drop
    CDEItemBase* pSelected = GetSelectedItem() ? (CDEItemBase*)((CFormID*)GetItemData(GetSelectedItem()))->GetItemPtr() : nullptr;

    // Convert the list of dragged form ids to a list of corresponding CDEItemBase
    std::vector<CDEItemBase*> dragItems;
    dragItems.reserve(dragFromIDs.size());
    std::transform(dragFromIDs.begin(), dragFromIDs.end(), std::back_inserter(dragItems), [](CFormID* pID) { return (CDEItemBase*)pID->GetItemPtr(); });

    // Check to make sure that all the items being dropped are from same
    // parent as drop target. If any are not, bail and don't do the drop
    auto droppedDiffParent = std::find_if(dragItems.begin(), dragItems.end(),
        [pDropGroup](CDEItemBase* const i) { return i->GetParent() != pDropGroup; });
    if (droppedDiffParent != dragItems.end())
        return;

    std::vector<CDEField*> dragFields;
    std::transform(dragFromIDs.begin(), dragFromIDs.end(), std::back_inserter(dragFields), [](CFormID* pID) { return static_cast<CDEField*>(pID->GetItemPtr()); });

    // Get the block items
    std::vector<CFormID*> blockItems;
    CDEBlock* pDragBlock = static_cast<CDEBlock*>(dragFromIDs.front()->GetItemPtr());
    HTREEITEM hItem = GetChildItem(dragFromIDs.front()->GetHItem());
    while (hItem) {
        blockItems.push_back(reinterpret_cast<CFormID*>(GetItemData(hItem)));
        hItem = GetNextItem(hItem, TVGN_NEXT);
    }
    ASSERT((int)blockItems.size() == pDragBlock->GetNumFields());

    // Remove the block from current location
    int oldBlockIndex = pDragBlock->GetParent()->GetItemIndex(pDragBlock);
    pDragBlock->GetParent()->RemoveItemAt(oldBlockIndex);

    // Drag and drop the items
    if (!blockItems.empty()) {
        DropItemOnForm(blockItems, pDropOnID);
    }

    // Reinsert the block
    int dropIndex = 0;
    pDropGroup->InsertItemAt(pDragBlock, dropIndex);

    ReBuildTree(pDropGroup->GetFormNum(),pSelected);
    GetFormDoc()->SetModifiedFlag(true);
}

void CFormTreeCtrl::DropBlockOnBlock(std::vector<CFormID*> dragFromIDs, CFormID* pDropOnID)
{
    ASSERT(pDropOnID->GetItemType() == eFTT_BLOCK);
    CDEBlock* pDropBlock = static_cast<CDEBlock*>(pDropOnID->GetItemPtr());
    CDEGroup* pDropGroup = pDropBlock->GetParent();

    // Save selected item so we can restore after drop
    CDEItemBase* pSelected = GetSelectedItem() ? (CDEItemBase*)((CFormID*)GetItemData(GetSelectedItem()))->GetItemPtr() : nullptr;

    // Convert the list of dragged form ids to a list of corresponding CDEItemBase
    std::vector<CDEItemBase*> dragItems;
    dragItems.reserve(dragFromIDs.size());
    std::transform(dragFromIDs.begin(), dragFromIDs.end(), std::back_inserter(dragItems), [](CFormID* pID) { return (CDEItemBase*)pID->GetItemPtr(); });

    // Check if the drop target is one of the dragged items - that should be a no op
    auto droppedInDrag = std::find(dragItems.begin(), dragItems.end(), pDropBlock);
    if (droppedInDrag != dragItems.end())
        return;

    // Check to make sure that all the items being dropped are from same
    // parent as drop target. If any are not, bail and don't do the drop
    auto droppedDiffParent = std::find_if(dragItems.begin(), dragItems.end(),
        [pDropGroup](CDEItemBase* const i) { return i->GetParent() != pDropGroup; });
    if (droppedDiffParent != dragItems.end())
        return;

    std::vector<CDEField*> dragFields;
    std::transform(dragFromIDs.begin(), dragFromIDs.end(), std::back_inserter(dragFields), [](CFormID* pID) { return static_cast<CDEField*>(pID->GetItemPtr()); });

    // Get the block items for drag
    std::vector<CFormID*> dragBlockItems;
    CDEBlock* pDragBlock = static_cast<CDEBlock*>(dragFromIDs.front()->GetItemPtr());
    HTREEITEM hItem = GetChildItem(dragFromIDs.front()->GetHItem());
    while (hItem) {
        dragBlockItems.push_back(reinterpret_cast<CFormID*>(GetItemData(hItem)));
        hItem = GetNextItem(hItem, TVGN_NEXT);
    }
    ASSERT((int)dragBlockItems.size() == pDragBlock->GetNumFields());

    // get the blocks current location
    int dragBlockIndex = pDragBlock->GetParent()->GetItemIndex(pDragBlock);
    //remove the dragblock from the current location
    pDragBlock->GetParent()->RemoveItemAt(dragBlockIndex);

    int dropBlockIndex = pDropBlock->GetParent()->GetItemIndex(pDropBlock);
    //reinsert at the drop location
    pDropGroup->InsertItemAt(pDragBlock, dropBlockIndex);

    // Drag and drop the items
    if (!dragBlockItems.empty()) {
        // Convert the list of dragged form ids to a list of corresponding CDEItemBase
        std::vector<CDEField*> vdragFields;
        vdragFields.reserve(dragFromIDs.size());

        CDERoster* pRoster = pDragBlock->GetParent()->GetItemType() == CDEFormBase::Roster ?
            static_cast<CDERoster*>(pDragBlock->GetParent()) : nullptr;

        if (pRoster) {
            std::transform(dragBlockItems.begin(), dragBlockItems.end(), std::back_inserter(vdragFields), [pRoster](CFormID* pID) { return pRoster->GetCol(pID->GetColumnIndex())->GetField(pID->GetRosterField()); });
        }
        else {
            std::transform(dragBlockItems.begin(), dragBlockItems.end(), std::back_inserter(vdragFields), [](CFormID* pID) { return static_cast<CDEField*>(pID->GetItemPtr()); });
        }

        pDragBlock->SetNumFields(0);
        UpdateGroupItemsForDrop(pDropGroup, vdragFields, pDragBlock);
        if (pRoster) {
            int dropIndex = pRoster->GetItemIndex(pDropBlock) + 1;
            CDEField* dropItem = dropIndex < pRoster->GetNumItems()
                ? static_cast<CDEField*>(pRoster->GetItem(dropIndex))
                : nullptr;
            UpdateRosterColumnsForDrop(pRoster, vdragFields, dropItem, true);
        }
    }

    ReBuildTree(pDropGroup->GetFormNum(),pSelected);
    GetFormDoc()->SetModifiedFlag(true);
}

void CFormTreeCtrl::DropBlockOnRoster(std::vector<CFormID*> dragFromIDs, CFormID* pDropOnID)
{
    ASSERT(pDropOnID->GetItemType() == eFTT_GRID);
    CDERoster* pDropRoster = static_cast<CDERoster*>(pDropOnID->GetItemPtr());
    CDEGroup* pDropGroup = pDropRoster->GetParent();

    // Save selected item so we can restore after drop
    CDEItemBase* pSelected = GetSelectedItem() ? (CDEItemBase*)((CFormID*)GetItemData(GetSelectedItem()))->GetItemPtr() : nullptr;

    // Convert the list of dragged form ids to a list of corresponding CDEItemBase
    std::vector<CDEItemBase*> dragItems;
    dragItems.reserve(dragFromIDs.size());
    std::transform(dragFromIDs.begin(), dragFromIDs.end(), std::back_inserter(dragItems), [](CFormID* pID) { return (CDEItemBase*)pID->GetItemPtr(); });

    // Check to make sure that all the items being dropped are from same
    // parent as drop target. If any are not, bail and don't do the drop
    auto droppedDiffParent = std::find_if(dragItems.begin(), dragItems.end(),
        [pDropGroup](CDEItemBase* const i) { return i->GetParent() != pDropGroup; });
    if (droppedDiffParent != dragItems.end())
        return;

    std::vector<CDEField*> dragFields;
    std::transform(dragFromIDs.begin(), dragFromIDs.end(), std::back_inserter(dragFields), [](CFormID* pID) { return static_cast<CDEField*>(pID->GetItemPtr()); });

    // Get the block items for drag
    std::vector<CFormID*> dragBlockItems;
    CDEBlock* pDragBlock = static_cast<CDEBlock*>(dragFromIDs.front()->GetItemPtr());
    HTREEITEM hItem = GetChildItem(dragFromIDs.front()->GetHItem());
    while (hItem) {
        dragBlockItems.push_back(reinterpret_cast<CFormID*>(GetItemData(hItem)));
        hItem = GetNextItem(hItem, TVGN_NEXT);
    }
    ASSERT((int)dragBlockItems.size() == pDragBlock->GetNumFields());

    // get the blocks current location
    int dragBlockIndex = pDragBlock->GetParent()->GetItemIndex(pDragBlock);
    //remove the dragblock from the current location
    pDragBlock->GetParent()->RemoveItemAt(dragBlockIndex);

    int dropRosterIndex = pDropRoster->GetParent()->GetItemIndex(pDropRoster);
    //reinsert at the drop location
    pDropGroup->InsertItemAt(pDragBlock, dropRosterIndex);

    // Drag and drop the items
    if (!dragBlockItems.empty()) {
        // Convert the list of dragged form ids to a list of corresponding CDEItemBase
        std::vector<CDEField*> vdragFields;
        vdragFields.reserve(dragFromIDs.size());
        std::transform(dragBlockItems.begin(), dragBlockItems.end(), std::back_inserter(vdragFields), [](CFormID* pID) { return static_cast<CDEField*>(pID->GetItemPtr()); });
        pDragBlock->SetNumFields(0);
        UpdateGroupItemsForDrop(pDropGroup, vdragFields, pDragBlock);
    }

    ReBuildTree(pDropGroup->GetFormNum(), pSelected);
    GetFormDoc()->SetModifiedFlag(true);
}
void CFormTreeCtrl::DropRosterOnBlock(std::vector<CFormID*> dragFromIDs, CFormID* pDropOnID)
{
    ASSERT(pDropOnID->GetItemType() == eFTT_BLOCK);
    CDEBlock* pDropBlock = static_cast<CDEBlock*>(pDropOnID->GetItemPtr());
    CDEGroup* pDropGroup = pDropBlock->GetParent();

    // Save selected item so we can restore after drop
    CDEItemBase* pSelected = GetSelectedItem() ? (CDEItemBase*)((CFormID*)GetItemData(GetSelectedItem()))->GetItemPtr() : nullptr;

    // Convert the list of dragged form ids to a list of corresponding CDEItemBase
    std::vector<CDEItemBase*> dragItems;
    dragItems.reserve(dragFromIDs.size());
    std::transform(dragFromIDs.begin(), dragFromIDs.end(), std::back_inserter(dragItems), [](CFormID* pID) { return (CDEItemBase*)pID->GetItemPtr(); });

    // Check to make sure that all the items being dropped are from same
    // parent as drop target. If any are not, bail and don't do the drop
    auto droppedDiffParent = std::find_if(dragItems.begin(), dragItems.end(),
        [pDropGroup](CDEItemBase* const i) { return i->GetParent() != pDropGroup; });
    if (droppedDiffParent != dragItems.end())
        return;

    std::vector<CDEField*> dragFields;
    std::transform(dragFromIDs.begin(), dragFromIDs.end(), std::back_inserter(dragFields), [](CFormID* pID) { return static_cast<CDEField*>(pID->GetItemPtr()); });

    // Get the roster for drag
    CDERoster* pDragRoster = static_cast<CDERoster*>(dragFromIDs.front()->GetItemPtr());

    // get the roster's current location
    int dragRosterIndex = pDragRoster->GetParent()->GetItemIndex(pDragRoster);
    //remove the dragblock from the current location
    pDragRoster->GetParent()->RemoveItemAt(dragRosterIndex);

    int dropBlockIndex = pDropBlock->GetParent()->GetItemIndex(pDropBlock);
    //reinsert at the drop location
    pDropGroup->InsertItemAt(pDragRoster, dropBlockIndex);

    ReBuildTree(pDropGroup->GetFormNum(), pSelected);
    GetFormDoc()->SetModifiedFlag(true);
}


void CFormTreeCtrl::DropRosterItemOnRoster(std::vector<CFormID*> dragFromIDs, CFormID* pDropOnID)
{
    ASSERT(pDropOnID->GetItemType() == eFTT_GRID);
    CDERoster* pRoster = static_cast<CDERoster*>(pDropOnID->GetItemPtr());

    ASSERT(dragFromIDs.size() > 0);

    // Save selected item so we can restore after drop
    CDEItemBase* pSelected = GetSelectedItem() ? (CDEItemBase*)((CFormID*)GetItemData(GetSelectedItem()))->GetItemPtr() : nullptr;

    // Check that drag items are all from same roster as the drop item
    if (!std::all_of(dragFromIDs.begin(), dragFromIDs.end(), [pRoster](CFormID* pID) { return (CDERoster*)pID->GetItemPtr() == pRoster; }))
        return;

    std::vector<CDEField*> dragFields;
    std::transform(dragFromIDs.begin(), dragFromIDs.end(), std::back_inserter(dragFields), [pRoster](CFormID* pID) { return pRoster->GetCol(pID->GetColumnIndex())->GetField(pID->GetRosterField()); });

    // Handle the group items
    UpdateGroupItemsForDrop(pRoster, dragFields, pRoster);

    // Update columns to match
    UpdateRosterColumnsForDrop(pRoster, dragFields, nullptr, false);

    ReBuildTree(pRoster->GetFormNum(), pSelected);
    GetFormDoc()->SetModifiedFlag(true);
}

// ***************************************************************************
// forms can be dropped on any other form, as long as they are in the same level;
// if they do not share the same parents, then the properties of the parents must
// be similar; i.e., if one loops then the other must loop based on the same prop

void CFormTreeCtrl::DropFormOnForm (CFormID* pDragFromID, CFormID* pDropOnID)
{
    CDEGroup* pDF_Group = (CDEGroup*) pDragFromID->GetItemPtr();
    CDEGroup* pDO_Group = (CDEGroup*) pDropOnID->GetItemPtr();

    CFormDoc* pFormDoc = GetFormDoc();

    CDEForm* pDF_Form = (CDEForm*) pFormDoc->GetForm (pDF_Group->GetFormNum());
    CDEForm* pDO_Form = (CDEForm*) pFormDoc->GetForm (pDO_Group->GetFormNum());

    if (pDF_Form == pDO_Form)   // they dropped on themselves

        return;

    if (pDF_Form->GetLevel() != pDO_Form->GetLevel())   // they must be in the same level

        return;

    // now drop!

    CDEGroup* pDO_Parent = pDO_Group->GetParent();
    CDEGroup* pDF_Parent = pDF_Group->GetParent();

    if (pDO_Parent != pDF_Parent)   // can only (for now) rearrange forms who share parentage
        return;

    int iDO_Index = pDO_Parent->GetItemIndex (pDO_Group);
    int iDF_Index = pDF_Parent->GetItemIndex (pDF_Group);

    CDEGroup* pParent = pDO_Parent; // to not show favoritism or mislead

    if (iDO_Index < iDF_Index)  // form we're dragging has higher index pos than the form we're dragging on to
    {
        pParent->RemoveItemAt (iDF_Index);          // remove first (only removes index, not item)
        pParent->InsertItemAt (pDF_Group, iDO_Index);
    }
    else    // insert first, then delete, else ptrs off
    {
        pParent->InsertItemAt (pDF_Group, iDO_Index);
        pParent->RemoveItemAt (iDF_Index);
    }

    pFormDoc->GetFormFile().RenumberAllForms();

    GetFormView()->SetPointers (iDO_Index); // let the view & doc ptrs know who's active


    ReBuildTree (pDF_Group->GetFormNum(), pDF_Group);

    GetFormView()->MakeFormCurrent(iDF_Index);

    pFormDoc->SetModifiedFlag(true);
}

void CFormTreeCtrl::UpdateGroupItemsForDrop(CDEGroup* pGroup, const std::vector<CDEField*>& dragFields, CDEItemBase* pDropItem)
{
    // Remove items being dropped from existing positions
    for (CDEField* pDragField : dragFields)
    {
        int iDragField = pGroup->GetItemIndex(pDragField);

        // Remove from block
        CDEBlock* pDraggedBlock = pGroup->GetBlockForFieldAt(iDragField);
        if (pDraggedBlock)
            pDraggedBlock->RemoveField();

        // Remove from group
        pGroup->RemoveItemAt(iDragField);
    }

    int iDropItem = 0;

    CDEBlock* pDroppedBlock = nullptr;
    switch (pDropItem->GetItemType())
    {
    case CDEFormBase::Field:
        // Update the item index now that fields have been removed
        iDropItem = pGroup->GetItemIndex(pDropItem);
        pDroppedBlock = pGroup->GetBlockForFieldAt(iDropItem);
        break;
    case CDEFormBase::Block:
        pDroppedBlock = static_cast<CDEBlock*>(pDropItem);
        // Update the item index now that fields have been removed
        iDropItem = pGroup->GetItemIndex(pDropItem);
        ++iDropItem; // Drop just after block
        break;
    case CDEFormBase::Roster:
        pDroppedBlock = nullptr;
        iDropItem = pGroup->GetItemIndex(pDropItem); // start of roster/group
        if (iDropItem == -1) //when drop items from roster are dropped on itself
            iDropItem = 0;
        break;
    case CDEFormBase::Group:
        pDroppedBlock = nullptr;
        iDropItem = 0; // start of roster/group
        break;
    default:
        ASSERT(!"This routine only handles drop on block or field");
    }

    // Insert them back into form at same index as drop target
    // Do this in reverse order so that after insert items
    // are in original order again.
    for (auto i = dragFields.rbegin(); i != dragFields.rend(); ++i)
    {
        pGroup->InsertItemAt(*i, iDropItem);

        if (pDroppedBlock)
            pDroppedBlock->AddField();
    }
}

void CFormTreeCtrl::UpdateRosterColumnsForDrop(CDERoster* pRoster, const std::vector<CDEField*>& dragFields, CDEField* pDropField, bool dropAtEndIfDropFieldNull)
{
    if (pDropField) {
        int iDropColIndex = pRoster->GetColIndex(pDropField);
        CDECol* pDropCol = pRoster->GetCol(iDropColIndex);
        int dropFieldIndexInCol = pDropCol->GetFieldIndex(pDropField);
        if (dropFieldIndexInCol != 0) {
            // Dropping in middle of column, need to split it

            // Create a new col for all fields before the drop field
            CDECol* pNewColumn = new CDECol();
            pNewColumn->SetHeaderText(pDropCol->GetHeaderText());

            for (int i = 0; i < dropFieldIndexInCol; ++i)
            {
                CDEField* pColFld = pDropCol->GetField(0);
                pDropCol->RemoveFieldAt(0);
                pNewColumn->AddField(pColFld);
            }

            pNewColumn->ResetSizeAndHeader();
            pDropCol->ResetSizeAndHeader();

            // Insert the new column before the drop col
            pRoster->InsertColAt(pNewColumn, iDropColIndex);
        }
    }

    std::vector<CDECol*> dragCols;

    // Collect columns to move and check if any need to be split
    for (CDEField* pDragField : dragFields)
    {
        CDECol* pDragCol = pRoster->GetCol(pRoster->GetColIndex(pDragField));

        if (std::find(dragCols.begin(), dragCols.end(), pDragCol) != dragCols.end())
            continue; // already processed this column

        // if some items in col are dragged and others are not then need to split the column
        std::vector<CDEField*> nonDragFields;
        for (int i = 0; i < pDragCol->GetNumFields(); ++i)
        {
            CDEField* pColFld = pDragCol->GetField(i);
            if (std::find(dragFields.begin(), dragFields.end(), pColFld) == dragFields.end())
            {
                nonDragFields.push_back(pColFld);
            }
        }

        if (!nonDragFields.empty()) {
            // Add all the non dragged fields to a new column
            CDECol* pNewColumn = new CDECol();
            pNewColumn->SetHeaderText(pDragCol->GetHeaderText());

            for (CDEField* pNonDragField : nonDragFields)
            {
                pDragCol->RemoveFieldAt(pDragCol->GetFieldIndex(pNonDragField));
                pNewColumn->AddField(pNonDragField);
            }

            pNewColumn->ResetSizeAndHeader();
            pDragCol->ResetSizeAndHeader();
            pRoster->InsertColAt(pNewColumn, pRoster->GetColIndex(pDragCol));
        }

        dragCols.push_back(pDragCol);
    }

    int iDropColIndex = pDropField ? pRoster->GetColIndex(pDropField) : (dropAtEndIfDropFieldNull ? pRoster->GetNumCols() : 1);

    // Move all the drag columns
    for (auto iCol = dragCols.begin(); iCol != dragCols.end(); ++iCol)
    {
        int colIndex = pRoster->GetColIndex(*iCol);
        if (colIndex <= iDropColIndex) {
            --iDropColIndex;
        }
        pRoster->RemoveCol(colIndex);
    }
    // Don't let drop col go below 1 or we end up in col header, this can happen when dropping first col on
    // top of itself
    iDropColIndex = std::max(iDropColIndex, 1);

    for (auto iCol = dragCols.rbegin(); iCol != dragCols.rend(); ++iCol)
    {
        pRoster->InsertColAt(*iCol, iDropColIndex);
    }

    CFormScrollView* pFV = GetFormView();

    CFormGrid* pGrid = pFV->FindGrid(pRoster);

    pGrid->BuildGrid();
    pGrid->RecalcLayout();
}

// ***************************************************************************
// the following funcs were added by glenn, modified by smg

void CFormTreeCtrl::OnViewLogic()
{
    CFormDoc* pDoc = GetFormDoc();

    if (pDoc)
    {
        CFormScrollView* pView = (CFormScrollView*)pDoc->GetView();
        CFormChildWnd* pFrame = (CFormChildWnd*)pView->GetParentFrame();

        pFrame->SendMessage(WM_COMMAND, ID_VIEW_LOGIC);
    }
}

void CFormTreeCtrl::OnViewQuestionnaire()
{
    CFormDoc* pDoc = GetFormDoc();
    if (pDoc)
    {
        CFormChildWnd* pFormChildWnd = dynamic_cast<CFormChildWnd*>(((CFrameWnd*)AfxGetMainWnd())->GetActiveFrame());
        if (pFormChildWnd) {
            pFormChildWnd->SendMessage(WM_COMMAND, ID_VIEW_QUESTIONNAIRE);
        }
    }
}
void CFormTreeCtrl::OnCopyName() // GHM 20101104
{
    CFormDoc* pDoc = GetFormDoc();

    if( pDoc == nullptr )
        return;

    std::optional<CString> name;

    CFormScrollView* pFV = GetFormView();

    if( pFV->m_pRightClickItem != nullptr ) // GHM 20101206
    {
        name = pFV->m_pRightClickItem->GetName();
    }

    else
    {
        HTREEITEM hItem = pDoc->GetFormTreeCtrl()->GetSelectedItem();
        CFormID* pID = reinterpret_cast<CFormID*>(pDoc->GetFormTreeCtrl()->GetItemData(hItem));

        if( pID != nullptr && pID->GetItemPtr() != nullptr )
            name = pID->GetItemPtr()->GetName();

        else if( pID->GetItemType() == eFFT_REPORT )
            name = ((const FormReportID*)pID)->GetName();
    }

    if( name.has_value() )
        WinClipboard::PutText(this, *name);
}


void CFormTreeCtrl::OnViewForm()
{
    CFormDoc* pDoc = GetFormDoc();

    if (pDoc)
    {
        CFormScrollView* pView = (CFormScrollView*)pDoc->GetView();
        CFormChildWnd* pFrame = (CFormChildWnd*)pView->GetParentFrame();
        pFrame->SendMessage(WM_COMMAND, ID_VIEW_FORM);
    }
}


void CFormTreeCtrl::OnQSFEditor()
{
    CFormDoc* pDoc = GetFormDoc();

    if (pDoc)
    {
        CFormScrollView* pView = (CFormScrollView*)pDoc->GetView();
        CFormChildWnd* pFrame = (CFormChildWnd*)pView->GetParentFrame();

        pFrame->SendMessage(WM_COMMAND, ID_QSF_EDITOR);
    }
}

void CFormTreeCtrl::OnCapiMacros()
{
    CFormDoc* pDoc = GetFormDoc();

    if (pDoc)
    {
        CFormScrollView* pView = (CFormScrollView*)pDoc->GetView();
        CFormChildWnd* pFrame = (CFormChildWnd*)pView->GetParentFrame();
        pFrame->SendMessage(WM_COMMAND, ID_CAPI_MACROS);
    }
}

void CFormTreeCtrl::OnRButtonUp(UINT nFlags, CPoint point)
{
     // Set selection and focus to tree control
    (void)nFlags;

    HTREEITEM hItem;
    if (point.x == 0 && point.y == 0) {
        hItem = GetSelectedItem();
        CRect rect;
        GetItemRect(hItem,rect,true);
        point.x = (rect.right + rect.left) / 2;
        point.y = (rect.bottom + rect.top) / 2;
    }
    else {
        hItem = HitTest(point);
    }

    if (!hItem)     // if nothing selected, bail
        return;

    if (IsSelected(hItem))
    {
        SetFocus();     // this prevents the item from being expanding (in case it isn't)
    }
    else {
        m_bSendMsg = FALSE; //stop sending logic change messages OnRButtonUp
        ClearSelection();
        SelectItem(hItem);  // otherwise, highlight it
        AddToSelection(hItem);
    }

    CFormID*            pID = (CFormID*) GetItemData (hItem);
    if (pID->GetItemPtr())
    {
        AfxTrace(L"OnRButtonUp sel item %x %s\n", (DWORD)hItem, ((CFormID*)GetItemData(hItem))->GetItemPtr()->GetName().GetString());
    }

    CFormScrollView*    pFV = GetFormView();    // get the currently displayed form
    CFormChildWnd* pFrame = (CFormChildWnd*)pFV->GetParentFrame();
    bool bIsFormViewActive = pFrame->GetViewMode() == FormViewMode;
    UINT deleteFlags = 0;
    // get the proper form up in the view
    bool isQuestionnaireView = pFrame->GetViewMode() == QuestionnaireViewMode;

    pFV->TreeSelectionChanged (GetSelectedFormItems());

    eNodeType eNT = pID->GetItemType();

    if (eNT == eINVALID_TYPE)
        return;     // no prop dialogs for these guys...though default level to form?


    CDEItemBase* pItem = (CDEItemBase*)pID->GetItemPtr();

    bool bAmIActive = pFrame->IsFormFrameActive();

    BCMenu popMenu;

    popMenu.CreatePopupMenu();

    auto selectedItems = GetSelectedFormItems();
    bool selectedItemsInBlock = false;

    if (pItem)
    {
        selectedItemsInBlock = std::any_of(selectedItems.begin(), selectedItems.end(),
            [&](const CFormID* pItem) {
                HTREEITEM hParent = GetParentItem(pItem->GetHItem());
                return hParent != NULL && ((CFormID*)GetItemData(hParent))->GetItemType() == eFTT_BLOCK;
            });
    }

    if (selectedItems.size() > 1)
    {
        // Multiple selection
        popMenu.AppendMenu(MF_STRING, ID_EDIT_FIELDPROP, _T("&Properties"));
        deleteFlags = bIsFormViewActive ? MF_STRING : MF_STRING | MF_GRAYED;
        popMenu.AppendMenu(deleteFlags, ID_DELETE_ITEM, _T("&Delete"));
        popMenu.AppendMenu(MF_SEPARATOR);
        popMenu.AppendMenu(MF_STRING | MF_GRAYED, ID_SHOW_BOXTOOLBAR, _T("Add &Text"));
        popMenu.AppendMenu(MF_STRING | MF_GRAYED, ID_ADD_FORM, _T("Add &Boxes"));
        popMenu.AppendMenu(MF_STRING, ID_ADD_FORM, _T("Add &Form"));
        popMenu.AppendMenu(MF_STRING | (selectedItemsInBlock ? MF_GRAYED : 0), ID_ADD_BLOCK, _T("Add B&lock"));
        popMenu.AppendMenu(MF_SEPARATOR);

    }

    else
    {
        bool add_report_options = ( pID->GetItemType() == eFFT_REPORT && pID->GetTextSource() != nullptr );

        // GHM 20101104, rosie request ... allow the user to copy the name of the node (for logic editing)
        if( pFrame->IsLogicViewActive() && ( add_report_options || ( eNT != eFFT_EXTERNALCODE && eNT != eFFT_REPORT ) ) ) 
        {
            popMenu.AppendMenu(MF_STRING, ID_COPY_NAME, _T("&Copy Name"));
            popMenu.AppendMenu(MF_SEPARATOR);
        }

        if( add_report_options )
        {
            popMenu.AppendMenu(isQuestionnaireView ? MF_STRING | MF_GRAYED : MF_STRING, ID_EDIT_REPORT_PROPERTIES, _T("&Properties"));
        }

        else if (!bAmIActive)    // i.e., the dictionary/table/etc is in the frame, but not me!
        {
            popMenu.AppendMenu(MF_STRING | MF_GRAYED, ID_EDIT_FFPROP, _T("&Properties"));
            popMenu.AppendMenu(MF_STRING | MF_GRAYED, ID_DELETE_FORM, _T("&Delete Form"));
            popMenu.AppendMenu(MF_SEPARATOR);
            popMenu.AppendMenu(MF_STRING | MF_GRAYED, ID_ADD_FORM, _T("Add &Form"));
            popMenu.AppendMenu(MF_STRING | MF_GRAYED, ID_ADD_BLOCK, _T("Add B&lock"));
            popMenu.AppendMenu(MF_SEPARATOR);
        }

        else
        {
            bool addBlockEnabled = false;

            CDECol* pCol = NULL; // annoyance; compiler complains if i do decl w/in case stmt
            switch (eNT)
            {
            case eFTT_FORMFILE:

                popMenu.AppendMenu(MF_ENABLED, ID_EDIT_FFPROP, _T("&Properties"));
                popMenu.AppendMenu(MF_STRING | MF_GRAYED, ID_DELETE_ITEM, _T("&Delete"));
                break;

            case eFTT_LEVEL:

                popMenu.AppendMenu(MF_ENABLED, ID_EDIT_LEVELPROP, _T("&Properties"));
                popMenu.AppendMenu(MF_STRING | MF_GRAYED, ID_DELETE_ITEM, _T("&Delete"));
                break;

            case eFTT_FORM:

                popMenu.AppendMenu(isQuestionnaireView ? MF_STRING | MF_GRAYED : MF_STRING, ID_EDIT_FORMPROP, _T("&Properties"));
                deleteFlags = bIsFormViewActive ? MF_ENABLED : MF_STRING | MF_GRAYED;
                popMenu.AppendMenu(deleteFlags, ID_DELETE_FORM, _T("&Delete Form"));
                addBlockEnabled = true;
                break;

            case eFTT_BLOCK:

                popMenu.AppendMenu(isQuestionnaireView ? MF_STRING | MF_GRAYED : MF_STRING, ID_EDIT_BLOCKPROP, _T("&Properties"));
                deleteFlags = bIsFormViewActive ? MF_STRING : MF_STRING | MF_GRAYED;
                popMenu.AppendMenu(deleteFlags, ID_DELETE_ITEM, _T("&Delete Block"));

                pFV->m_pRightClickItem = pItem;
                break;

            case eFTT_GRID:         // give roster properties dialog for both;

                popMenu.AppendMenu(isQuestionnaireView ? MF_STRING | MF_GRAYED : MF_STRING, ID_EDIT_GRIDPROP, _T("&Properties"));
                popMenu.AppendMenu(isQuestionnaireView ? MF_STRING | MF_GRAYED : MF_STRING, ID_EDIT_GRID_AUTOFIT, _T("AutoFit"));
                deleteFlags = bIsFormViewActive ? MF_STRING : MF_STRING | MF_GRAYED;
                popMenu.AppendMenu(deleteFlags, ID_DELETE_ITEM, _T("&Delete Roster"));
                addBlockEnabled = true;

                pFV->m_pRightClickItem = pItem;
                break;

            case eFTT_GRIDFIELD:

                popMenu.AppendMenu(isQuestionnaireView ? MF_STRING | MF_GRAYED : MF_STRING, ID_EDIT_FIELDPROP, _T("&Properties"));
                deleteFlags = bIsFormViewActive ? MF_STRING : MF_STRING | MF_GRAYED;
                popMenu.AppendMenu(deleteFlags, ID_DELETE_ITEM, _T("&Delete Field"));
                addBlockEnabled = !selectedItemsInBlock;

                pCol = ((CDERoster*)pItem)->GetCol(pID->GetColumnIndex());
                pFV->m_pRightClickItem = pCol->GetField(pID->GetRosterField());

                break;

            case eFTT_FIELD:

                popMenu.AppendMenu(isQuestionnaireView ? MF_STRING | MF_GRAYED : MF_STRING, ID_EDIT_FIELDPROP, _T("&Properties"));
                deleteFlags = bIsFormViewActive ? MF_STRING : MF_STRING | MF_GRAYED;
                popMenu.AppendMenu(deleteFlags, ID_DELETE_ITEM, _T("&Delete Field"));
                addBlockEnabled = !selectedItemsInBlock;

                pFV->m_pRightClickItem = pItem;
                break;

            default:  break;
            }

            if( eNT != eFFT_EXTERNALCODE && eNT != eFFT_REPORT )
            {
                popMenu.AppendMenu(MF_SEPARATOR);
                popMenu.AppendMenu(MF_STRING | MF_GRAYED, ID_SHOW_BOXTOOLBAR, _T("Add &Text"));
                popMenu.AppendMenu(MF_STRING | MF_GRAYED, ID_ADD_FORM, _T("Add &Boxes"));
                popMenu.AppendMenu(pFrame->GetViewMode() == QuestionnaireViewMode ? MF_STRING | MF_GRAYED : MF_STRING, ID_ADD_FORM, _T("Add &Form"));
                popMenu.AppendMenu(MF_STRING | (addBlockEnabled && !isQuestionnaireView ? 0 : MF_GRAYED), ID_ADD_BLOCK, _T("Add B&lock"));
                popMenu.AppendMenu(MF_SEPARATOR);
            }
        }
    }

    if( eNT != eFFT_EXTERNALCODE && eNT != eFFT_REPORT )
    {
        popMenu.AppendMenu(MF_STRING, ID_VIEW_FORM, _T("View &Form"));
        popMenu.AppendMenu(MF_STRING, ID_VIEW_LOGIC, _T("View &Logic"));

        if( pFrame->GetUseQuestionText())
        {
            popMenu.AppendMenu(MF_STRING, ID_QSF_EDITOR, _T("View CAPI &Question"));
            popMenu.AppendMenu(MF_STRING, ID_VIEW_QUESTIONNAIRE, _T("View Questionnaire"));
            popMenu.AppendMenu(MF_SEPARATOR);
            popMenu.AppendMenu(isQuestionnaireView ? MF_STRING | MF_GRAYED : MF_STRING, ID_CAPI_MACROS, _T("Question Text &Macros"));
            
        }
        else {
            popMenu.AppendMenu(MF_STRING, ID_VIEW_QUESTIONNAIRE, _T("View Questionnaire"));
        }
    }

    pFV->m_bAddRFT = false;
    pFV->m_bAddForm = false;

    CRect rect;
    GetWindowRect(rect);   // GetClientRect will not work, nd offset of entire screen

    rect.OffsetRect (5,5);
    popMenu.LoadToolbar(IDR_FORM_FRAME);   // BMD 09 Apr 2004
    int iMenuSelection = popMenu.TrackPopupMenu (TPM_RIGHTBUTTON | TPM_RETURNCMD, rect.left + point.x, rect.top + point.y, this);
    if (!pFrame->IsLogicViewActive()) {//if we are not in logicview leave the rightclicked item as the selection
        m_hRestoreItemSelection = NULL;
        m_bSendMsg = TRUE;
    }
    switch (iMenuSelection) {
    case 0:
        RestoreItemSelection();
        break;
    default:
        SendMessage(WM_COMMAND, iMenuSelection);
        RestoreItemSelection();
        break;
    }
//  CTreeCtrl::OnRButtonUp(nFlags, point);
}

void CFormTreeCtrl::RestoreItemSelection()
{
    //Restore selection states after Rbuttonup
    if (m_hRestoreItemSelection) {
        SetRedraw(false);
        SelectItem(m_hRestoreItemSelection);
        m_bSendMsg = TRUE;
        SetRedraw(true);

        m_hRestoreItemSelection = NULL;
    }
}
BOOL CFormTreeCtrl::PreTranslateMessage(MSG* pMsg)
{
    if (pMsg->message == WM_KEYDOWN) {
        if (pMsg->wParam == '\t') {
            CMDIChildWnd* pActiveWnd = ((CMDIFrameWnd*)AfxGetMainWnd())->MDIGetActive();
            if(pActiveWnd == NULL) {
                return FALSE;
            }
            else {
                CView* pView = pActiveWnd->GetActiveView();
                pView->SetFocus();
                return TRUE;
            }
        }
    }
    return CTreeCtrl::PreTranslateMessage(pMsg);
}

void CFormTreeCtrl::OnRButtonDown(UINT nFlags, CPoint point)
{
    // TODO: Add your message handler code here and/or call default
    UINT        flags;

    HTREEITEM   hItem = HitTest(point, &flags);    // (mfc) tree ctrl function
    m_hRestoreItemSelection = GetSelectedItem(); //Save Old Item Selection
    if(hItem) {

        // If we don't call default OnRButtonDown need to set the
        // focus to this window since base class would do that
        if (CWnd::GetFocus() != this)
            SetFocus();

        return;
    }

    CTreeCtrl::OnRButtonDown(nFlags, point);
}

CImageList* CFormTreeCtrl::CreateDragImageEx(HTREEITEM hItem)
{
//    if(GetImageList(TVSIL_NORMAL) != NULL)
 //       return CreateDragImage(hItem);

    CRect rect;
    GetItemRect(hItem, rect, FALSE);
 //   rect.top = rect.left = 0;

    // Create bitmap
    CClientDC       dc (this);
    CDC             memDC;

    if(!memDC.CreateCompatibleDC(&dc))
        return NULL;

    CBitmap bitmap;
    if(!bitmap.CreateCompatibleBitmap(&dc, rect.Width(), rect.Height()))
        return NULL;

    CBitmap* pOldMemDCBitmap = memDC.SelectObject( &bitmap );
    CFont* pOldFont = memDC.SelectObject(GetFont());


    CRect rect2(0,0, rect.Width(), rect.Height());
    memDC.FillSolidRect(&rect2, RGB(0, 255, 0));
    memDC.SetTextColor(GetSysColor(COLOR_GRAYTEXT));
    memDC.TextOut(16, 0, GetItemText(hItem));


 //   memDC.FillSolidRect(&rect, RGB(0, 255, 0)); // Here green is used as mask color
 //   memDC.SetTextColor(GetSysColor(COLOR_GRAYTEXT));
 //   memDC.TextOut(rect.left, rect.top, GetItemText(hItem));

    memDC.SelectObject( pOldFont );
    memDC.SelectObject( pOldMemDCBitmap );

    // Create imagelist
    CImageList* pImageList = new CImageList;
    pImageList->Create(rect.Width(), rect.Height(),
        ILC_COLOR | ILC_MASK, 0, 1);
    pImageList->Add(&bitmap, RGB(0, 255, 0)); // Here green is used as mask color

    return pImageList;
}

void CFormTreeCtrl::OnEditCopy()
{
    /*
    HTREEITEM origTI = GetSelectedItem();
    CFormID*    pNodeID = (CFormID*) GetItemData (origTI);

    eNodeType   nodeType = pNodeID->GetItemType();
    if (nodeType == eFTT_FORMFILE || nodeType == eINVALID_TYPE)

        return; // can't delete a form file from the tree; must close it out & delete file

    if (nodeType == eFTT_LEVEL)

        return; // don't think we're gonna allow this either folx

    CFormDoc*           pFormDoc = pNodeID->GetFormDoc();
    CFormScrollView*    pView = GetFormView();
    CDELevel* pLevel = GetCurLevel();
    CDEGroup* pGroup = pLevel->GetGroupOnForm ( GetFormIndex() );

    int iLevel = GetCurForm()->GetLevel();

    CDEFormFile* pFF = pView->GetFFSpec();
    CDELevel*    pLevel = pFF->GetLevel ( iLevel );

    int iForm = pGroup->GetFormNum();
    int iGroup = pGroup->GetParent()->GetItemIndex(pGroup);
    CDEForm* pForm = pFF->GetForm (iForm);

//  pFormDoc->SetModifiedFlag(true);    // mark the file as dirty

    if (nodeType == eFTT_FORM)
    {
        //pForm->Save()
        //if (pView->OkToDeleteForm())  // now that user can undo delete, don't ask if they're sure!
        {
            //pView->DeleteActiveGroup ((CDEGroup*) pNodeID->GetItemPtr());
        }
    }
    else if (nodeType == eFTT_LOGIC)
    {
        //      pView->DeleteActiveGroup ((CDEGroup*) pNodeID->GetItemPtr());
    }
    //AfxMessageBox("Wakaw");
    */
}

void CFormTreeCtrl::SwitchRefresh()
{
    CFormNodeID* pNodeID = GetFormNode (m_pFormDoc);

    //Make sure that the form view is visible
    CFormScrollView* pView = (CFormScrollView*)m_pFormDoc->GetView();

    if(pView && pView->IsWindowVisible())
    {
        pView->TreeSelectionChanged(std::vector<CFormID*> {pNodeID});
    }
}

/////////////////////////////////////////////////////////////////////////////
//
//                         CFormTreeCtrl::OnShiftF10
//
/////////////////////////////////////////////////////////////////////////////

void CFormTreeCtrl::OnShiftF10() {

    PostMessage(WM_RBUTTONUP);
}


HTREEITEM CFormTreeCtrl::FindItem( CDEFormBase* pItemBase ){
    HTREEITEM hParent = GetRootItem();
    while(hParent){
        HTREEITEM hItem = xFindItem(hParent,pItemBase);
        if(hItem)
            return hItem;
        hParent = GetNextSiblingItem(hParent);
    }
    return NULL;
}
HTREEITEM CFormTreeCtrl::xFindItem( HTREEITEM hInputItem, CDEFormBase* pFormBase ){

    CFormID* pNodeID = (CFormID*) GetItemData( hInputItem );
    if( pNodeID && pNodeID->GetItemPtr()==pFormBase )
        return hInputItem;

    HTREEITEM hNextItem = GetChildItem( hInputItem );
    while( hNextItem ){
        HTREEITEM hItem = xFindItem( hNextItem, pFormBase );
        if( hItem )
            return hItem;
        hNextItem = GetNextItem( hNextItem, TVGN_NEXT);
    }

    return NULL;
}

void CFormTreeCtrl::ClearSelection()
{
    HTREEITEM hItem = GetRootItem();

    // Clear selection upto the first item
    while (hItem)
    {
        if (IsSelected(hItem))
            RemoveFromSelection(hItem);
        hItem = GetNextItemRec(hItem);
    }

}

bool CFormTreeCtrl::IsSelected(HTREEITEM hItem)
{
    return (GetItemState(hItem, TVIS_SELECTED) & TVIS_SELECTED) != 0;
}

void CFormTreeCtrl::AddToSelection(HTREEITEM hItem)
{
    SetItemState(hItem, TVIS_SELECTED, TVIS_SELECTED);
}

void CFormTreeCtrl::RemoveFromSelection(HTREEITEM hItem)
{
    SetItemState(hItem, 0, TVIS_SELECTED);
}

std::vector<HTREEITEM> CFormTreeCtrl::GetSelected()
{
    std::vector<HTREEITEM> selected;
    HTREEITEM hItem = GetRootItem();

    // Clear selection upto the first item
    while (hItem)
    {
        if (IsSelected(hItem))
            selected.push_back(hItem);
        hItem = GetNextItemRec(hItem);
    }
    return selected;
}

std::vector<CFormID*> CFormTreeCtrl::GetSelectedFormItems()
{
    std::vector<HTREEITEM> selectedTreeItems = GetSelected();
    std::vector<CFormID*> result;
    std::transform(selectedTreeItems.begin(), selectedTreeItems.end(), std::back_inserter(result), [this](HTREEITEM hItem) {return (CFormID*)GetItemData(hItem); });
    return result;
}

void CFormTreeCtrl::SelectRange(HTREEITEM hItemFrom, HTREEITEM hItemTo)
{

    AfxTrace(L"SelectRange: %s --> %s\n", ((CFormID*)GetItemData(hItemFrom))->GetItemPtr()->GetName().GetString(), ((CFormID*)GetItemData(hItemTo))->GetItemPtr()->GetName().GetString());
    HTREEITEM hFirst = NULL;
    HTREEITEM hLast = NULL;

    // Figure out if hItemFrom or hItemTo comes first
    HTREEITEM hItem = hItemFrom;
    for (; hItem != NULL && hItem != hItemTo; hItem = GetNextVisibleItem(hItem))
        ;

    if (hItem == hItemTo) {
        // From is first
        hFirst = hItemFrom;

        for (hItem = hItemFrom; hItem != NULL && hItem != GetNextVisibleItem(hItemTo) && CanBePartOfMultipleSelection(hItem); hItem = GetNextVisibleItem(hItem))
            hLast = hItem;

        if (hLast == NULL)
            hLast = hFirst;
    }
    else
    {
        // To is first
        hLast = hItemFrom;

        for (hItem = hItemFrom; hItem != NULL && hItem != GetPrevVisibleItem(hItemTo) && CanBePartOfMultipleSelection(hItem); hItem = GetPrevVisibleItem(hItem))
            hFirst = hItem;

        if (hFirst == NULL)
            hFirst = hLast;
    }

    // Clear selection up to hFirst
    for (hItem = GetRootItem(); hItem != NULL && hItem != hFirst; hItem = GetNextVisibleItem(hItem))
        RemoveFromSelection(hItem);

    // Select between hFirst and hLast
    for (hItem = hFirst; hItem != NULL && hItem != GetNextVisibleItem(hLast); hItem = GetNextVisibleItem(hItem))
        AddToSelection(hItem);

    // Clear selection after hItemFrom
    for (hItem = GetNextVisibleItem(hLast); hItem != NULL; hItem = GetNextVisibleItem(hItem))
        RemoveFromSelection(hItem);

}

/// <summary>Gets next item in tree using depth first traversal</summary>
HTREEITEM CFormTreeCtrl::GetNextItemRec(HTREEITEM hItem)
{
    HTREEITEM hNext = GetChildItem(hItem);
    if (hNext)
        return hNext;
    hNext = GetNextSiblingItem(hItem);
    if (hNext)
        return hNext;


    HTREEITEM hParent = GetParentItem(hItem);
    while (hParent != NULL)
    {
        hNext = GetNextSiblingItem(hParent);
        if (hNext)
            return hNext;

        hParent = GetParentItem(hParent);
    }

    return NULL;
}

bool CFormTreeCtrl::CanBePartOfMultipleSelection(HTREEITEM hItem)
{
    // Only allow fields, no higher level nodes
    CFormID* pID = (CFormID*)GetItemData(hItem);
    if (pID->GetItemType() != eFTT_FIELD && pID->GetItemType() != eFTT_GRIDFIELD)
        return false;

    // Must be same type and from same form as others in selection
    HTREEITEM hCurSel = GetSelectedItem();
    if (hCurSel)
    {
        CFormID* pSelectedID = (CFormID*)GetItemData(hCurSel);
        if (pSelectedID->GetItemType() != pID->GetItemType())
            return false;

        if (pSelectedID->GetFormDoc() != pID->GetFormDoc() || pSelectedID->GetFormIndex() != pID->GetFormIndex())
            return false;

        if (pID->GetItemType() == eFTT_GRIDFIELD)
        {
            // Ensure that roster fields are from same roster
            if (pID->GetItemPtr() != pSelectedID->GetItemPtr())
                return false;
        }
    }

    return true;
}

void CFormTreeCtrl::NotifyViewSelectionChanged()
{
    CFormID* pID = (CFormID*)GetItemData(GetSelectedItem());
    if (pID)
    {
        CFormDoc*   pDoc = pID->GetFormDoc();
        if (pDoc)
        {
            CFormScrollView* pView = (CFormScrollView*)pDoc->GetView();
            if (pView)
                pView->TreeSelectionChanged(GetSelectedFormItems());
        }
    }
}

bool CFormTreeCtrl::ShouldStartDragging(CPoint p)
{
    return !m_dragStartBounds.IsRectEmpty() && !m_dragStartBounds.PtInRect(p);
}
