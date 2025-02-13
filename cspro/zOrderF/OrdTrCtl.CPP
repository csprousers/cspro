//***************************************************************************
//  File name: OrdTrCtl.cpp
//
//  Description:
//       Orders tree control implementation
//
//  History:    Date       Author   Comment
//              ---------------------------
//              11 Feb 99   gsf     Created for Measure 1.0
//
//***************************************************************************

#include "StdAfx.h"
#include "OrdTrCtl.H"
#include <zToolsO/SharedSettings.h>
#include <zToolsO/WinClipboard.h>
#include <zUtilO/BCMenu.h>
#include <zUtilO/Specfile.h>
#include <zJson/JsonStream.h>
#include <zAppO/Application.h>
#include <zInterfaceF/resource_shared.h>


#ifdef _DEBUG
#define new DEBUG_NEW
#undef THIS_FILE
static char THIS_FILE[] = __FILE__;
#endif

const int ORD_ORDERFILE     =  0;
const int ORD_LEVEL         =  1;
const int ORD_ROSTER32      =  2;
const int ORD_FIELD32       =  3;
const int ORD_ORDER         =  4;
const int ORD_ORDLOOP       =  5;
const int ORD_BLOCK         =  6;
const int ORD_EXTERNALLOGIC =  7;
const int ORD_REPORT        =  8;

//Check marks
const int ORD_FIELD32C      =  9;
const int ORD_ORDERFILEC    = 10;
const int ORD_LEVELC        = 11;
const int ORD_ROSTER32C     = 12;
const int ORD_ORDERC        = 13;
const int ORD_ORDLOOPC      = 14;
const int ORD_BLOCKC        = 15;


BEGIN_MESSAGE_MAP(COrderTreeCtrl, CTreeCtrl)
    //{{AFX_MSG_MAP(COrderTreeCtrl)
    ON_WM_LBUTTONDOWN()
    ON_WM_LBUTTONUP()
    ON_WM_MOUSEMOVE()
    ON_WM_KEYDOWN()
    ON_WM_RBUTTONDOWN()
    ON_WM_RBUTTONUP()
    ON_NOTIFY_REFLECT(TVN_SELCHANGED, OnSelchanged)
    ON_NOTIFY_REFLECT(TVN_SELCHANGING, OnSelchanging)
    ON_NOTIFY_REFLECT(NM_DBLCLK, OnDTreeDblclk)
    ON_NOTIFY_REFLECT(TVN_ITEMEXPANDING, OnItemexpanding)
    ON_NOTIFY_REFLECT(TVN_DELETEITEM, OnDeleteitem)
    ON_NOTIFY_REFLECT(TVN_BEGINDRAG, OnBegindrag)
    ON_NOTIFY_REFLECT(TVN_GETDISPINFO, OnGetdispinfo)
    ON_COMMAND(ID_EDIT_REPORT_PROPERTIES, OnEditReportProperties)
    ON_COMMAND(ID_COPY_NAME, OnCopyName)
    ON_COMMAND(ID_VIEW_QUESTIONNAIRE, OnViewQuestionnaire)
    ON_COMMAND(ID_VIEW_LOGIC, OnViewLogic)
    ON_COMMAND(ID_SHIFT_F10, OnShiftF10)
    //}}AFX_MSG_MAP
END_MESSAGE_MAP()


// COrderTreeCtrl
BOOL bDragSendMsg = TRUE;


COrderTreeCtrl::COrderTreeCtrl()
    :   m_bDragging(false),
        m_dwDragStart(0),
        m_pOrderDoc(nullptr),
        m_pDDTreeCtrl(nullptr),
        m_pCurItem(nullptr),
        m_bOkToRedraw(true),
        m_bSendMsg(TRUE)
{
}


COSourceEditView* COrderTreeCtrl::GetOrderView()
{
    POSITION pos = GetOrderDoc()->GetFirstViewPosition();
    return ( pos != nullptr ) ? (COSourceEditView*)GetOrderDoc()->GetNextView(pos) : nullptr;
}


void COrderTreeCtrl::OnSelchanged (NMHDR* pNMHDR, LRESULT* pResult)
{
    NMTREEVIEW* pNMTreeView = reinterpret_cast<NM_TREEVIEW*>(pNMHDR);   // Get tree control structure; mfc-gen
    LPARAM lParam = pNMTreeView->itemNew.lParam;                        // Get new state of tree control

    AppTreeNode* app_tree_node = reinterpret_cast<AppTreeNode*>(lParam);

    if( app_tree_node == nullptr )   // the node can sometimes be null... (found this out when allowing deletion of a page)
        return;

    COrderDoc* pDoc = app_tree_node->GetOrderDocument();

    if(m_bSendMsg){
        AfxGetMainWnd()->SendMessage(UWM::Order::ShowSourceCode, 0, reinterpret_cast<LPARAM>(pDoc));
    }
    if (pDoc != GetOrderDoc())
    {
        SetOrderDoc(pDoc);
    }
    POSITION pos = pDoc->GetFirstViewPosition();
    ASSERT (pos != nullptr);
    COSourceEditView*   pView = (COSourceEditView*)pDoc->GetNextView(pos);
    pView->GetEditCtrl()->EmptyUndoBuffer();

    SetFocus();
    CMDIChildWnd* pActiveWnd = assert_cast<CMDIFrameWnd*>(AfxGetMainWnd())->MDIGetActive();
    if (pActiveWnd && pActiveWnd->IsKindOf(RUNTIME_CLASS(COrderChildWnd))) {
        QuestionnaireView* pQuestionnaireView = ((COrderChildWnd*)pActiveWnd)->GetQuestionnaireView();
        if (pQuestionnaireView && pQuestionnaireView->IsWindowVisible()) {
            pQuestionnaireView->SendMessage(UWM::Designer::TreeSelectionChanged,
                (WPARAM)(app_tree_node->GetName().c_str()));
        }
    }
    *pResult = 0;
}


void COrderTreeCtrl::OnDTreeDblclk(NMHDR* /*pNMHDR*/, LRESULT* pResult)
{
    // Get the node corresponding to the retrieved item
    // If the view is not activated, do so
    LPARAM lParam = GetItemData(GetSelectedItem());
    AppTreeNode* app_tree_node = reinterpret_cast<AppTreeNode*>(lParam);
    COrderDoc* pDoc = app_tree_node->GetOrderDocument();

    if( pDoc != nullptr )
    {
        this->m_bSendMsg = FALSE;
        POSITION pos = pDoc->GetFirstViewPosition();
        ASSERT (pos != nullptr);

        CView*  pView = (CView*) pDoc->GetNextView(pos);
        COrderChildWnd*     pWnd  = (COrderChildWnd*) pView->GetParent();
        pWnd->ActivateFrame();

        this->m_bSendMsg = TRUE;
    }

    *pResult = 0;
}


HTREEITEM COrderTreeCtrl::InsertOrderFile(const CString& sLabel, const CString& sOrderFileName, COrderDoc* pDoc)
{
    // Insert the Order in the Order tree control
    // Use the Order file name to insert it into the
    // structure later on to get the Order file name
    FormOrderAppTreeNode* form_order_app_tree_node = new FormOrderAppTreeNode(pDoc, AppFileType::Order, CS2WS(sOrderFileName), CS2WS(sLabel));
    form_order_app_tree_node->AddRef();

    TVINSERTSTRUCT tvi;
    tvi.hParent      = TVI_ROOT;
    tvi.hInsertAfter = TVI_LAST;

    tvi.item.mask       = TVIF_TEXT | TVIF_PARAM | TVIF_IMAGE | TVIF_SELECTEDIMAGE;
    tvi.item.state      = 0;
    tvi.item.stateMask  = 0;
    tvi.item.pszText    = (LPTSTR)LPSTR_TEXTCALLBACK;
    tvi.item.lParam     = reinterpret_cast<LPARAM>(form_order_app_tree_node);
    tvi.item.iImage     = I_IMAGECALLBACK;
    tvi.item.iSelectedImage = I_IMAGECALLBACK;

    HTREEITEM hItem = InsertItem( &tvi );
    form_order_app_tree_node->SetHItem(hItem);

    return hItem;
}


// ***************************************************************************
// Get the item corresponding to this Item ;if this has its pDoc as nullptr
// Then we need to build the tree .
// Send a message to our application to open the corresponding file
void COrderTreeCtrl::OnItemexpanding(NMHDR* pNMHDR, LRESULT* pResult)
{
    NMTREEVIEW* pNMTreeView = (NMTREEVIEW*) pNMHDR;
    HTREEITEM hItem = pNMTreeView->itemNew.hItem;
    AppTreeNode* app_tree_node = GetTreeNode(hItem);

    if( app_tree_node->IsFormFileElement() )
    {
        if(app_tree_node->GetDocument() == nullptr)  // this may not be occurring for a while yet
        {
            const std::wstring& filename = app_tree_node->GetPath();

            ASSERT(m_pDocTemplate != nullptr);
            COrderDoc* pDoc = (COrderDoc*)m_pDocTemplate->OpenDocumentFile(filename.c_str());

            app_tree_node->SetDocument(pDoc);

            pDoc->SetOrderTreeCtrl (this);

            SetOrderDoc(pDoc); // set the tree ctrl's ptr to the doc

            if (!pDoc->LoadOrderSpecFile(WS2CS(filename)) )
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


void COrderTreeCtrl::InitImageList()
{
    m_cImageList.Create(16, 16, ILC_COLOR32, 0, 2); // 32, 32 for large icons
    m_cImageList.SetBkColor(GetSysColor(COLOR_WINDOW));

    auto load_icon = [&](auto resource_id, auto index)
    {
        ASSERT(index == m_cImageList.GetImageCount());
        HICON icon = AfxGetApp()->LoadIcon(resource_id);
        m_cImageList.Add(icon);
    };

    load_icon(IDI_ORD_ORDERFILE, ORD_ORDERFILE);
    load_icon(IDI_ORD_LEVEL, ORD_LEVEL);
    load_icon(IDI_ORD_ROSTER32, ORD_ROSTER32);
    load_icon(IDI_ORD_FIELD32, ORD_FIELD32);
    load_icon(IDI_ORD_ORDER, ORD_ORDER);
    load_icon(IDI_ORD_ORDLOOP, ORD_ORDLOOP);
    load_icon(IDI_FORM_BLOCK, ORD_BLOCK);
    load_icon(IDI_LOGIC_FILE, ORD_EXTERNALLOGIC);
    load_icon(IDI_REPORT_FILE, ORD_REPORT);

    //Icons with check marks
    load_icon(IDI_ORD_FIELD32C, ORD_FIELD32C);
    load_icon(IDI_ORD_ORDERFILEC, ORD_ORDERFILEC);
    load_icon(IDI_ORD_LEVELC, ORD_LEVELC);
    load_icon(IDI_ORD_ROSTER32C, ORD_ROSTER32C);
    load_icon(IDI_ORD_ORDERC, ORD_ORDERC);
    load_icon(IDI_ORD_ORDLOOPC, ORD_ORDLOOPC);
    load_icon(IDI_FORM_BLOCK_LOGIC, ORD_BLOCKC);

    SetImageList(&m_cImageList, TVSIL_NORMAL);
}

// ***************************************************************************
// smg: not sure if this is the best/right/only place to do this, but here goes;
// when the user has the Order tree ctrl active (as opposed to the dict tree, etc.)
// and they do smthng in the view causing the tree to be rebuilt (such as deleting
// an item), the tree (as currently designed) gets trashed and rebuilt; fine; but
// doing so makes it lose track of where it was, i.e., what was the current Order & field?
// so by getting the Order index before i start bludgeoning things and then restoring
// it when done, i'll have at least the same Order active, both in the view and on the
// tree...however if a Order can be deleted from the tree, the following will have to
// be revised...


void COrderTreeCtrl::ReBuildTree(int iOrderNum, CDEFormBase* pItem ,bool bSendMsg /*=true*/)
{
    SetRedraw(false);               // Don't redraw tree ctrl while rewkg
    m_bOkToRedraw = false;

    // find node corresponding to this Order
    // & delete all its descendants
    m_bSendMsg = FALSE;

    FormOrderAppTreeNode* form_order_app_tree_node = GetFormOrderAppTreeNode(*m_pOrderDoc);

    HTREEITEM h = GetChildItem(form_order_app_tree_node->GetHItem());

    while (h != nullptr)
    {
        HTREEITEM hSib = GetNextSiblingItem(h);
        DeleteItem(h);
        h = hSib;
    }

    BuildTree(form_order_app_tree_node, false);
    InsertExternalCodeAndReportNodes(form_order_app_tree_node);

    if(bSendMsg) {
        m_bSendMsg = TRUE;
        SelectFTCNode(*form_order_app_tree_node, iOrderNum, pItem);
    }
}


// ***************************************************************************
//  may nd to adjust this...how often do i want the tree rebuilt? only once i
//  say, and then do additions...but i think the way this is set up, it may
//  build it more often...trace later: smg
//
//  also, when this func is entered, the root node for the Order file has already
//  been created, so i just have to do the Orders and their objs


LOGFONT COrderTreeCtrl::m_DefLogFont ={ 0, 0, 0, 0 };
CFont COrderTreeCtrl::m_font;

void COrderTreeCtrl::BuildTree(FormOrderAppTreeNode* form_order_app_tree_node, bool bRedraw)
{
    if(m_DefLogFont.lfHeight == 0 )  {
        CFont* pFont = GetFont();
        pFont->GetLogFont(&m_DefLogFont);
    }
    LOGFONT logfont;
    memset(&logfont,0,sizeof(LOGFONT));
    bool bChangeFont  = false;
    m_font.GetSafeHandle() == nullptr ? bChangeFont = true : m_font.GetLogFont(&logfont);

    CString sFontName = GetDesignerFontName();
    sFontName.Trim();

    CString sDefFontFaceName(m_DefLogFont.lfFaceName);
    if((!sFontName.IsEmpty() && sFontName.CompareNoCase(logfont.lfFaceName) != 0) || (sFontName.IsEmpty() && sDefFontFaceName.CompareNoCase(logfont.lfFaceName) != 0 )){
        bChangeFont = true;
    }

    if(bChangeFont || logfont.lfHeight != m_DefLogFont.lfHeight * (GetDesignerFontZoomLevel()/ 100.0)){
        logfont = m_DefLogFont; //inititalize
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

    COrderDoc* pOrderDoc = form_order_app_tree_node->GetOrderDocument();

    if(!pOrderDoc)
        return;

    m_pOrderDoc = pOrderDoc;
    form_order_app_tree_node->SetFormBase(&pOrderDoc->GetFormFile());

    m_bSendMsg = FALSE;

    SetRedraw(false);               // Don't try to draw while rebuilding the tree ctrl!
    m_bOkToRedraw = false;

    CDEFormFile* pFF = &pOrderDoc->GetFormFile();

    TVINSERTSTRUCT tvi;

    tvi.item.mask       = TVIF_TEXT | TVIF_PARAM | TVIF_IMAGE | TVIF_SELECTEDIMAGE;
    tvi.item.state      = 0;
    tvi.item.stateMask  = 0;
    tvi.item.pszText    = (LPTSTR)LPSTR_TEXTCALLBACK;
    tvi.item.iImage     = I_IMAGECALLBACK;
    tvi.item.iSelectedImage = I_IMAGECALLBACK;

    HTREEITEM hRoot = form_order_app_tree_node->GetHItem();

    for (int iLevel = 0; iLevel < pFF->GetNumLevels(); iLevel++)
    {
        CDELevel* pLevel = pFF->GetLevel(iLevel);
        FormElementAppTreeNode* form_element_app_tree_node = new FormElementAppTreeNode(pOrderDoc, FormElementType::Level, pLevel);

        tvi.hParent = hRoot;
//      tvi.hInsertAfter = TVI_LAST;    // nec?

        tvi.item.lParam = reinterpret_cast<LPARAM>(form_element_app_tree_node);

        HTREEITEM hParent = InsertItem( &tvi );
        form_element_app_tree_node->SetHItem(hParent);

        // each level must be on sep forms, so build a form node for each
        for (int iGroup = 0; iGroup < pLevel->GetNumGroups(); iGroup++)
        {
            CDEGroup* pGroup = pLevel->GetGroup (iGroup);
            AddGroupItems (pOrderDoc, pGroup, hParent, iLevel);
        }
    }

    DefaultExpand(hRoot);

    if (bRedraw)
    {
        SetRedraw(true);
        m_bOkToRedraw = true;
        Invalidate();
        UpdateWindow();
    }

    m_bSendMsg = TRUE;
}


////////////////////////////////////////////////////////////////////////////
// call this to add each group and the items belonging to it

// smg: this has got to be fixed; if a CDEGroup item is there to control a single block
// of looping, for ex, pop rec is unrostered, then instead of displaying a "Group" item
// on the tree ctrl, it'll be a "Order#" (assuming it's on a sep Order); if, however, it's
// needed to show a group of groups looping, that may or may not be on sep pages, then
// it'll appear in the tree ctrl as a "Group" item

void COrderTreeCtrl::AddGroupItems(COrderDoc* pOrderDoc, CDEGroup* pGroup, HTREEITEM hParent, int iLevel)
{
    HTREEITEM       hLocalParent;

    FormElementAppTreeNode* form_element_app_tree_node = new FormElementAppTreeNode(pOrderDoc, FormElementType::Form, pGroup);

    TVINSERTSTRUCT tvi;
    tvi.hParent         = hParent;

    tvi.item.mask       = TVIF_TEXT | TVIF_PARAM | TVIF_IMAGE | TVIF_SELECTEDIMAGE;
    tvi.item.state      = 0;
    tvi.item.stateMask  = 0;
    tvi.item.pszText    = (LPTSTR)LPSTR_TEXTCALLBACK;
    tvi.item.lParam     = reinterpret_cast<LPARAM>(form_element_app_tree_node);
    tvi.item.iImage     = I_IMAGECALLBACK;
    tvi.item.iSelectedImage = I_IMAGECALLBACK;

    hLocalParent = InsertItem(&tvi);
    form_element_app_tree_node->SetHItem(hLocalParent);

    // now loop thru all items and insert them under the pg
    for (int i = 0; i < pGroup->GetNumItems(); i++)
    {
        CDEItemBase* pItem = pGroup->GetItem(i);
        if (pItem->GetItemType() == CDEFormBase::Block) {
            // add a node for the current group; loop thru all it's entities
            AddBlock(pOrderDoc, assert_cast<CDEBlock&>(*pItem), hLocalParent);
        }
        else if (pItem->GetItemType() == CDEFormBase::Group) {
            // add a node for the current group; loop thru all it's entities
            AddGroupItems(pOrderDoc, (CDEGroup*)pItem, hLocalParent, iLevel);
        }
        else {
            if (pGroup->GetBlock(static_cast<CDEField*>(pItem)) == nullptr) // items in block are added underneath block
                AddBaseItem(pOrderDoc, pItem, hLocalParent);
        }
    }
}


//////////////////////////////////////////////////////////////////////////////
// call this to add each keyed field or roster; if it's a roster, add all
// items belonging to it

void COrderTreeCtrl::AddBaseItem(COrderDoc* pOrderDoc, CDEItemBase* pItem, HTREEITEM hParent)
{
    FormElementType form_element = ( pItem->GetItemType() == CDEFormBase::Field ) ? FormElementType::Field :
                                                                                    FormElementType::Grid;
    FormElementAppTreeNode* form_element_app_tree_node = new FormElementAppTreeNode(pOrderDoc, form_element, pItem);

    TVINSERTSTRUCT tvi;
    tvi.hParent         = hParent;

    tvi.item.mask       = TVIF_TEXT | TVIF_PARAM | TVIF_IMAGE | TVIF_SELECTEDIMAGE;
    tvi.item.state      = 0;
    tvi.item.stateMask  = 0;
    tvi.item.pszText    = (LPTSTR)LPSTR_TEXTCALLBACK;
    tvi.item.lParam     = reinterpret_cast<LPARAM>(form_element_app_tree_node);
    tvi.item.iImage     = I_IMAGECALLBACK;
    tvi.item.iSelectedImage = I_IMAGECALLBACK;

    HTREEITEM hLocalParent = InsertItem( &tvi );
    form_element_app_tree_node->SetHItem(hLocalParent);

    // if it's a roster, loop thru its fields
    if(pItem->GetItemType() == CDEFormBase::Roster){
        CDERoster*     pRoster = (CDERoster*)pItem;
        int itemNdx;
        int formNdx = pRoster->GetFormNum();

        pOrderDoc->GetFormFile().FindItem(pRoster->GetName(), formNdx, &itemNdx);

        tvi.hParent = hLocalParent;
        tvi.item.iImage = I_IMAGECALLBACK;
        tvi.item.iSelectedImage = I_IMAGECALLBACK;

        for (int iCol = 0;iCol < pRoster->GetNumCols();iCol++){
            CDECol* pCol = pRoster->GetCol (iCol);

            for (int iField = 0;iField < pCol->GetNumFields();iField++) {
                // the form_base value must point to the roster!
                FormElementAppTreeNode* grid_field_form_element_app_tree_node = new FormElementAppTreeNode(pOrderDoc, FormElementType::GridField, pRoster);
                grid_field_form_element_app_tree_node->SetColumnIndex(iCol);
                grid_field_form_element_app_tree_node->SetRosterField(iField);

                tvi.item.lParam = reinterpret_cast<LPARAM>(grid_field_form_element_app_tree_node);

                HTREEITEM hChildField = InsertItem( &tvi );
                grid_field_form_element_app_tree_node->SetHItem(hChildField);
            }
        }
    }
}


void COrderTreeCtrl::AddBlock(COrderDoc* pOrderDoc, CDEBlock& form_block, HTREEITEM hParent)
{
    FormElementAppTreeNode* form_element_app_tree_node = new FormElementAppTreeNode(pOrderDoc, FormElementType::Block, &form_block);

    TVINSERTSTRUCT tvi;
    tvi.hParent = hParent;

    tvi.item.mask = TVIF_TEXT | TVIF_PARAM | TVIF_IMAGE | TVIF_SELECTEDIMAGE;
    tvi.item.state = 0;
    tvi.item.stateMask = 0;
    tvi.item.pszText = (LPTSTR)LPSTR_TEXTCALLBACK;
    tvi.item.lParam = reinterpret_cast<LPARAM>(form_element_app_tree_node);
    tvi.item.iImage = I_IMAGECALLBACK;
    tvi.item.iSelectedImage = I_IMAGECALLBACK;

    HTREEITEM hLocalParent = InsertItem(&tvi);
    form_element_app_tree_node->SetHItem(hLocalParent);

    for( CDEField* pField : form_block.GetParent()->GetBlockFields(form_block))
        AddBaseItem(pOrderDoc, pField, hLocalParent);
}


// ***************************************************************************
// expands all nodes of the tree
void COrderTreeCtrl::DefaultExpand(HTREEITEM hItem)
{
    // won't put the following types on the default expand
    while (hItem)
    {
        const AppTreeNode* app_tree_node = GetTreeNode(hItem);

        if( !app_tree_node->IsFormElement() )
            return;

        FormElementType form_element = assert_cast<const FormElementAppTreeNode*>(app_tree_node)->GetFormElementType();

        if( form_element != FormElementType::FormFile &&
            form_element != FormElementType::Level )
        {
            return;
        }

        Expand (hItem, TVE_EXPAND);

        DefaultExpand(GetChildItem(hItem));

        hItem = GetNextSiblingItem(hItem);
    }
}


void COrderTreeCtrl::InsertExternalCodeAndReportNodes(FormOrderAppTreeNode* form_order_app_tree_node)
{
    if( form_order_app_tree_node == nullptr || form_order_app_tree_node->GetHItem() == nullptr )
    {
        ASSERT(false);
        return;
    }

    // first remove all old header nodes
    HTREEITEM hChildItem = GetChildItem(form_order_app_tree_node->GetHItem());

    while( hChildItem != nullptr )
    {
        HTREEITEM hNextItem = GetNextItem(hChildItem, TVGN_NEXT);

        AppTreeNode* app_tree_node = GetTreeNode(hChildItem);

        if( app_tree_node != nullptr && app_tree_node->IsHeader(AppFileType::Code) ||
                                        app_tree_node->IsHeader(AppFileType::Report) )
        {
            DeleteItem(hChildItem);
        }

        hChildItem = hNextItem;
    }

    // get the current application
    Application* pApplication = nullptr;
    AfxGetMainWnd()->SendMessage(UWM::Designer::GetApplication, reinterpret_cast<WPARAM>(&pApplication),
                                                                reinterpret_cast<LPARAM>(form_order_app_tree_node->GetOrderDocument()));

    if( pApplication == nullptr )
        return;

    HTREEITEM hLastAddedHeaderItem = nullptr;

    // the routine for adding nodes
    auto add_nodes = [&](std::vector<AppTreeNode*>& app_tree_nodes)
    {
        ASSERT(!app_tree_nodes.empty());
        HTREEITEM hHeaderItem = nullptr;

        for( AppTreeNode* app_tree_node : app_tree_nodes )
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
                ASSERT(dynamic_cast<HeadingAppTreeNode*>(app_tree_node) != nullptr);

                tvi.hParent = form_order_app_tree_node->GetHItem();
                tvi.hInsertAfter = ( hLastAddedHeaderItem == nullptr ) ? TVI_FIRST : hLastAddedHeaderItem;
            }

            else
            {
                ASSERT(app_tree_node->GetTextSource() != nullptr);

                tvi.hParent = hHeaderItem;
                tvi.hInsertAfter = TVI_LAST;
            }

            tvi.item.lParam = reinterpret_cast<LPARAM>(app_tree_node);

            HTREEITEM hItem = InsertItem(&tvi);
            app_tree_node->SetHItem(hItem);

            if( hHeaderItem == nullptr )
            {
                hHeaderItem = hItem;
                hLastAddedHeaderItem = hHeaderItem;
            }
        }
    };


    // add external code
    {
        std::vector<AppTreeNode*> app_tree_nodes;

        for( const CodeFile& code_file : pApplication->GetCodeFiles() )
        {
            if( code_file.IsLogicMain() )
                continue;

            if( app_tree_nodes.empty() )
                app_tree_nodes.emplace_back(new HeadingAppTreeNode(form_order_app_tree_node->GetDocument(), AppFileType::Code));

            app_tree_nodes.emplace_back(new ExternalCodeAppTreeNode(form_order_app_tree_node->GetDocument(), code_file));
        }

        if( !app_tree_nodes.empty() )
            add_nodes(app_tree_nodes);
    }


    // add reports
    const auto& report_named_text_sources = pApplication->GetReportNamedTextSources();

    if( !report_named_text_sources.empty() )
    {
        std::vector<AppTreeNode*> app_tree_nodes = { new HeadingAppTreeNode(form_order_app_tree_node->GetDocument(), AppFileType::Report) };

        for( const auto& report_named_text_source : report_named_text_sources )
            app_tree_nodes.emplace_back(new ReportAppTreeNode(form_order_app_tree_node->GetDocument(), report_named_text_source));

        add_nodes(app_tree_nodes);
    }
}


FormOrderAppTreeNode* COrderTreeCtrl::GetFormOrderAppTreeNode(CDocument& document) const
{
    HTREEITEM hItem = GetRootItem();

    while( hItem != nullptr )
    {
        AppTreeNode* app_tree_node = GetTreeNode(hItem);

        if( app_tree_node != nullptr && app_tree_node->IsFormFileElement() && app_tree_node->GetDocument() == &document )
            return assert_cast<FormOrderAppTreeNode*>(app_tree_node);

        hItem = GetNextSiblingItem(hItem);
    }

    return nullptr;
}

// ***************************************************************************


FormOrderAppTreeNode* COrderTreeCtrl::GetFormOrderAppTreeNode(wstring_view filename) const
{
    HTREEITEM hItem = GetRootItem();

    while( hItem != nullptr )
    {
        AppTreeNode* app_tree_node = GetTreeNode(hItem);
        ASSERT(app_tree_node == nullptr || app_tree_node->IsFormFileElement());

        FormOrderAppTreeNode* form_order_app_tree_node = assert_nullable_cast<FormOrderAppTreeNode*>(app_tree_node);

        if( form_order_app_tree_node != nullptr && SO::EqualsNoCase(filename, form_order_app_tree_node->GetPath()) )
            return form_order_app_tree_node;

        hItem = GetNextSiblingItem(hItem);
    }

    return nullptr;
}


void COrderTreeCtrl::ReleaseOrderNode(FormOrderAppTreeNode& form_order_app_tree_node)
{
    form_order_app_tree_node.Release();

    if( form_order_app_tree_node.GetRefCount() == 0 )
    {
        if( form_order_app_tree_node.GetOrderDocument() != nullptr )
        {
            //By now the doc shld be saved so go ahead and close it
            form_order_app_tree_node.GetOrderDocument()->OnCloseDocument();
        }

        SetItemData(form_order_app_tree_node.GetHItem(), NULL);
        DeleteItem(form_order_app_tree_node.GetHItem());   // drop the item from the tree
        delete &form_order_app_tree_node;
    }
}


void COrderTreeCtrl::ReleaseDoc(FormOrderAppTreeNode& form_order_app_tree_node)
{
    if( form_order_app_tree_node.GetDocument() == nullptr )
        return;

    form_order_app_tree_node.SetDocument(nullptr);
    form_order_app_tree_node.Release();

    HTREEITEM hNode = form_order_app_tree_node.GetHItem();
    HTREEITEM hChild = GetChildItem(hNode); // smg: shldn't we check for hNode != nullptr?

    while(hChild)       // smg: what exactly are we killing here?
    {
        DeleteItem(hChild);
        hChild = GetChildItem(hNode);
    }

    if( form_order_app_tree_node.GetRefCount() == 0 )
        DeleteItem(hNode);
}


void COrderTreeCtrl::OnLButtonDown(UINT nFlags, CPoint point)
{
    m_dwDragStart = GetTickCount(); // global; rtns elapsed # of ms. since system startup

    CTreeCtrl::OnLButtonDown(nFlags, point);
}


void COrderTreeCtrl::OnLButtonUp(UINT nFlags, CPoint point)
{
    if (m_bDragging)
    {
        // [1] Unlocks the window specified & hides the drag image, allowing the window to be updated.
        // [2] Officially terminate the drag operation

        m_pDragImage->DragLeave(GetDesktopWindow());    // [1]
        m_pDragImage->EndDrag();                        // [2]

        m_pDragImage.reset();    // scrap him, don't nd anymore

        ClientToScreen(&point);

        //Check if the drop is with in the view or my tree ctrl


        HTREEITEM hItem = GetSelectedItem();


        AppTreeNode* app_tree_node = GetTreeNode(hItem);
        if(app_tree_node != nullptr && app_tree_node->GetDocument() != nullptr) {//Dict Order then no drag
            if(app_tree_node->GetOrderDocument()->GetFormFile().IsDictOrder()){
                ReleaseCapture();   // ok, before i bail, cleanup
                m_bDragging = false;
                bDragSendMsg = TRUE;
                CTreeCtrl::OnLButtonUp(nFlags, point);
                AfxMessageBox(_T("The order of procedures for fields and records is currently set to match the dictionary.\n\n")
                              _T("To make the order different, select 'Options/Custom Order' from the main menu."));
                return;                // selecting item below will crash me, so bail
            }
        }

        CRect rect;
        GetWindowRect(&rect);

        if (rect.PtInRect(point) ) // am i dropping on myself?
        {
            if(hItem != nullptr )
            {
                AppTreeNode* selected_app_tree_node = GetTreeNode(GetSelectedItem());
                DropFormTreeItem(*selected_app_tree_node, point);

                ReleaseCapture();   // ok, before i bail, cleanup
                m_bDragging = false;
                bDragSendMsg = TRUE;
                CTreeCtrl::OnLButtonUp(nFlags, point);
                return;                // selecting item below will crash me, so bail
            }
        }

        ReleaseCapture();
        m_bDragging = false;
        SelectDropTarget(nullptr);
        SelectItem(hItem);
    }

    CTreeCtrl::OnLButtonUp(nFlags, point);  // mfc-generated
}


void COrderTreeCtrl::OnMouseMove(UINT nFlags, CPoint point)
{
    bool bRight = false;
    CPoint tPoint = point;

    if (m_bDragging)
    {
        RECT rect;
        GetClientRect( &rect );

        bRight = (point.x < (rect.right - rect.left) /2) ? false : true;

        ClientToScreen(&point);
        ASSERT(m_pDragImage != nullptr);

        m_pDragImage->DragMove(point);

        CWnd* pWnd = this->WindowFromPoint(point);

        if(pWnd == this)
        {
            m_pDragImage->DragShowNolock(false); // show the image while locking the window

            if (bRight) // need to scroll up, go slow if cursor near treeview control
            {
                SendMessage(WM_HSCROLL, MAKEWPARAM(SB_LINERIGHT,0));
            }
            else        // need to scroll down, go slow if cursor near treeview control
            {
                SendMessage(WM_HSCROLL, MAKEWPARAM(SB_LINELEFT,0));
            }
            m_pDragImage->DragShowNolock(true); // show the image being dragged w/o locking the window
        }
    }
    CTreeCtrl::OnMouseMove(nFlags, point);
}


void COrderTreeCtrl::OnBegindrag(NMHDR* pNMHDR, LRESULT* pResult)
{
    NM_TREEVIEW* pNMTreeView = reinterpret_cast<NM_TREEVIEW*>(pNMHDR);    // mfc-gen

    CPoint      ptAction;
    // This code is to prevent accidental drags

    if( (GetTickCount() - m_dwDragStart) < DRAG_DELAY) {

        return;
    }
    ASSERT(!m_bDragging);   // prob if this guy not true

    GetCursorPos(&ptAction);
    ScreenToClient(&ptAction);

    HTREEITEM   m_hitemDrag = pNMTreeView->itemNew.hItem;

    SelectItem(m_hitemDrag);

    // create a dragging bitmap for the given item in a tree view control

    m_pDragImage.reset(CreateDragImageEx(m_hitemDrag));

    if( m_pDragImage == nullptr ) {    // something's gone wrong if this occurred
        return;
    }
    m_bDragging = true;
    bDragSendMsg = FALSE;

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

    *pResult = 0;
}


void COrderTreeCtrl::OnDeleteitem(NMHDR* pNMHDR, LRESULT* pResult)
{
    NM_TREEVIEW* pNMTreeView = reinterpret_cast<NM_TREEVIEW*>(pNMHDR);

    // TODO: Add your control notification handler code here

    HTREEITEM hItem = pNMTreeView->itemOld.hItem;

    AppTreeNode* app_tree_node = GetTreeNode(hItem);
    delete app_tree_node;

    *pResult = 0;
}

// ***************************************************************************
// origTI = the original tree item


void COrderTreeCtrl::OnKeyDown(UINT nChar, UINT nRepCnt, UINT nFlags)
{
    HTREEITEM origTI = GetSelectedItem();

    switch (nChar)
    {
        case VK_DOWN:
            origTI = GetNextVisibleItem(origTI);
            if (origTI)
                SelectItem(origTI);
            break;

        case VK_UP:
            origTI = GetPrevVisibleItem(origTI);
            if (origTI)
                SelectItem(origTI);
            break;

        default:
            CTreeCtrl::OnKeyDown(nChar, nRepCnt, nFlags);  // BMD 24 Oct 2001
            break;
    }
}

// ***************************************************************************
//  this func either adds a node to the dict tree, or adds a reference


bool COrderTreeCtrl::InsertOrderDependencies(FormOrderAppTreeNode& form_order_app_tree_node)
{
    CSpecFile specFile(true);

    if( !specFile.Open(form_order_app_tree_node.GetPath().c_str(), CFile::modeRead) )
    {
        AfxMessageBox(FormatText(_T("File %s Could not be opened"), form_order_app_tree_node.GetPath().c_str()));
        return false;
    }

    std::vector<std::wstring> dictionary_filenames = GetFileNameArrayFromSpecFile(specFile, CSPRO_DICTS);
    specFile.Close();

    if( dictionary_filenames.empty() )
    {
        AfxMessageBox(FormatText(_T("No Dictionaries in Spec file %s"), form_order_app_tree_node.GetPath().c_str()));
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

    return  true;
}


bool COrderTreeCtrl::ReleaseOrderDependencies(FormOrderAppTreeNode& form_order_app_tree_node)
{
    // whether or not Order doc has been opened (which at the application or project level
    // it may not necessarily be), we need to try and close the Order's associated array
    // of dictionaries; if:

    // [1]  the Order doc is loaded, then copy the names from there, as the user could have
    //      added a dictionary but not yet serialized that modification
    // [2]  otherwise, if no Order doc is loaded, then grab the list of dicts from the .fmf file
    CSpecFile specFile(true);
    std::vector<std::wstring> dictionary_filenames;

    if (form_order_app_tree_node.GetDocument() != nullptr) // [1]
    {
        CDEFormFile* pFF = &form_order_app_tree_node.GetOrderDocument()->GetFormFile();
        dictionary_filenames.emplace_back(CS2WS(pFF->GetDictionaryFilename()));
    }

    else if (specFile.Open(form_order_app_tree_node.GetPath().c_str(), CFile::modeRead))  // [2]
    {
        dictionary_filenames = GetFileNameArrayFromSpecFile(specFile, CSPRO_DICTS);
        specFile.Close();

        if( dictionary_filenames.empty() )
        {
            AfxMessageBox(FormatText(_T("No Dictionaries in Spec file %s"), form_order_app_tree_node.GetPath().c_str()));
            return false;
        }
    }

    else    // associated err msg not particularly accurate...
    {
        AfxMessageBox(FormatText(_T("File %s Could not be opened"), form_order_app_tree_node.GetPath().c_str()));
        return false;
    }

    CDDTreeCtrl* pDictTree = GetDDTreeCtrl();

    if (pDictTree == nullptr)
        return false;

    for( const std::wstring& dictionary_filename : dictionary_filenames )
    {
        DictionaryDictTreeNode* dictionary_dict_tree_node = pDictTree->GetDictionaryTreeNode(dictionary_filename);

        if( dictionary_dict_tree_node != nullptr )
            pDictTree->ReleaseDictionaryNode(*dictionary_dict_tree_node);
    }

    return true;
}


BOOL COrderTreeCtrl::OpenOrderFile(const CString &sOrderFile)
{
    FormOrderAppTreeNode* form_order_app_tree_node = GetFormOrderAppTreeNode(sOrderFile);

    if(form_order_app_tree_node->GetDocument() != nullptr)
        return TRUE;

    COrderDoc* pDoc = (COrderDoc*)m_pDocTemplate->OpenDocumentFile(sOrderFile);

    SetOrderDoc(pDoc); // set the tree ctrl's ptr to the doc

    if (!pDoc->LoadOrderSpecFile(sOrderFile) )
    {
        pDoc->OnCloseDocument();
        return FALSE;
    }

    form_order_app_tree_node->SetDocument(pDoc);

    //Set the tree  control
    pDoc->SetOrderTreeCtrl(this);
    if(!pDoc->LoadDictSpecFile()){
        pDoc->OnCloseDocument();
        return FALSE;
    }

    SetSndMsgFlg(FALSE);
    SetRedraw(FALSE);

    CString csErr;
    bool bFlag = pDoc->GetFormFile().OReconcile(csErr,false,true);

 //   SelectItem(GetRootItem());
    ReBuildTree (0,nullptr,false);
    SetSndMsgFlg(TRUE);
    SetRedraw(TRUE);

    if(!bFlag) {
        pDoc->SetModifiedFlag();
    }

    return TRUE;
}


void COrderTreeCtrl::OnSelchanging(NMHDR* pNMHDR, LRESULT* pResult)
{
    NM_TREEVIEW* pNMTreeView = reinterpret_cast<NM_TREEVIEW*>(pNMHDR);
    // TODO: Add your control notification handler code here

    if (!pNMTreeView->itemOld.hItem)
    {
        *pResult = 0;
        return;
    }

    if(m_bSendMsg && bDragSendMsg)
    {
        COSourceEditView* pView = GetOrderView();
        AppTreeNode* app_tree_node = reinterpret_cast<AppTreeNode*>(pNMTreeView->itemOld.lParam);

        if (pView && AfxGetMainWnd()->SendMessage(UWM::Order::PutSourceCode, 0, reinterpret_cast<LPARAM>(app_tree_node)) == -1)
        {
            *pResult = 1;
            return;
        }
    }

    bDragSendMsg = TRUE; //Reset the flag after drop

    *pResult = 0;
}


void COrderTreeCtrl::OnGetdispinfo(NMHDR* pNMHDR, LRESULT* pResult)
{
    TV_DISPINFO* pTVDispInfo = reinterpret_cast<TV_DISPINFO*>(pNMHDR); // Get pointer to display information
    AppTreeNode* app_tree_node = reinterpret_cast<AppTreeNode*>(pTVDispInfo->item.lParam);

    if( app_tree_node == nullptr )
        return;

    // On windows 8 with touch keyboard
    //we need to explicitly check for call back image or text
    //This method is being called seperately for the image and the text
    //If the checks are not done for TVI_IMAGE and TVIF_TEXT
    //it does not draw the control at all in release mode
    //in debug mode it crashes on lbuttondown.
    //DO NOT CHANGE THE TEXT OR IMAGE if the flag is not set
    //in the item.mask

    // setting the text
    if( ( pTVDispInfo->item.mask & TVIF_TEXT ) != 0 )
    {
        bool view_name = SharedSettings::ViewNamesInTree();
        std::wstring display_text = view_name ? app_tree_node->GetName() : app_tree_node->GetLabel();

        if( view_name && SharedSettings::AppendLabelsToNamesInTree() )
        {
            std::wstring label = app_tree_node->GetLabel();

            if( display_text != label )
                SO::Append(display_text, _T(": "), label);
        }

        lstrcpyn(pTVDispInfo->item.pszText, display_text.c_str(), pTVDispInfo->item.cchTextMax);
    }

    // setting the icon
    if( ( pTVDispInfo->item.mask & TVIF_IMAGE ) != 0 )
    {
        std::optional<int> icon_index;

        auto has_logic = [&]()
        {
            if( m_bSendMsg )
            {
                CRect rectItem;

                if( GetItemRect(pTVDispInfo->item.hItem, &rectItem, FALSE) )
                    return ( AfxGetMainWnd()->SendMessage(UWM::Order::HasLogic, 0, reinterpret_cast<LPARAM>(app_tree_node)) != 0 );
            }

            return false;
        };

        if( app_tree_node->IsFormFileElement() )
        {
            icon_index = has_logic() ? ORD_ORDERFILEC : ORD_ORDERFILE;
        }

        else if( app_tree_node->IsFormElement() )
        {
            FormElementAppTreeNode& form_element_app_tree_node = assert_cast<FormElementAppTreeNode&>(*app_tree_node);

            if( form_element_app_tree_node.GetFormElementType() == FormElementType::Level )
            {
                icon_index = has_logic() ? ORD_LEVELC : ORD_LEVEL;
            }

            else if( form_element_app_tree_node.GetFormElementType() == FormElementType::Form )
            {
                const CDEGroup* group = assert_cast<const CDEGroup*>(form_element_app_tree_node.GetFormBase());
                icon_index = ( group != nullptr && group->GetMaxLoopOccs() == 1 ) ? ( has_logic() ? ORD_ORDERC : ORD_ORDER ) :
                                                                                    ( has_logic() ? ORD_ORDLOOPC : ORD_ORDLOOP );
            }

            else if( form_element_app_tree_node.GetFormElementType() == FormElementType::Grid )
            {
                icon_index = has_logic() ? ORD_ROSTER32C : ORD_ROSTER32;
            }

            else if( form_element_app_tree_node.GetFormElementType() == FormElementType::Field ||
                     form_element_app_tree_node.GetFormElementType() == FormElementType::GridField )
            {
                icon_index = has_logic() ? ORD_FIELD32C : ORD_FIELD32;
            }

            else if( form_element_app_tree_node.GetFormElementType() == FormElementType::Block )
            {
                icon_index = has_logic() ? ORD_BLOCKC : ORD_BLOCK;
            }
        }

        else if( app_tree_node->IsNodeOrHeader(AppFileType::Code) )
        {
            icon_index = ORD_EXTERNALLOGIC;
        }

        else if( app_tree_node->IsNodeOrHeader(AppFileType::Report) )
        {
            icon_index = ORD_REPORT;
        }

        if( icon_index.has_value() )
        {
            pTVDispInfo->item.iImage = *icon_index;
            pTVDispInfo->item.iSelectedImage = *icon_index;
        }
    }
    *pResult = 0;
}


void COrderTreeCtrl::DropFormTreeItem(AppTreeNode& drag_from_app_tree_node, CPoint dropPoint)
{
    // if i'm not lkg at a ptr to a dict tree, don't want it!
    if (!drag_from_app_tree_node.IsFormElement() )
        return;

    FormElementAppTreeNode& form_element_drag_from_app_tree_node = assert_cast<FormElementAppTreeNode&>(drag_from_app_tree_node);

    ScreenToClient(&dropPoint);    // coords now relative to the upper-left corner of my CWnd

    HTREEITEM hItem = HitTest(dropPoint);

    if (!hItem)     // if nothing selected, bail
        return;

    AppTreeNode* drop_on_app_tree_node = GetTreeNode(hItem);

    if (!drop_on_app_tree_node->IsFormElement() )
        return;

    FormElementAppTreeNode& form_element_drop_on_app_tree_node = assert_cast<FormElementAppTreeNode&>(*drop_on_app_tree_node);

    // can only reorder rosters/fields among themselves, or forms among themselves
    bool can_drop = false;

    if( form_element_drag_from_app_tree_node.GetFormElementType() == FormElementType::Field &&
        form_element_drop_on_app_tree_node.GetFormElementType() == FormElementType::Field )
    {
        can_drop = true;
    }

    else if( ( form_element_drag_from_app_tree_node.GetFormElementType() == FormElementType::Grid || form_element_drag_from_app_tree_node.GetFormElementType() == FormElementType::Field ) &&
             ( form_element_drop_on_app_tree_node.GetFormElementType() == FormElementType::Grid || form_element_drop_on_app_tree_node.GetFormElementType() == FormElementType::Field ) )
    {
        can_drop = true;
    }

    else if( form_element_drag_from_app_tree_node.GetFormElementType() == FormElementType::Form &&
             form_element_drop_on_app_tree_node.GetFormElementType() == FormElementType::Form )
    {
        can_drop = true;
    }

    if( can_drop )
        DropItemOnItem(form_element_drag_from_app_tree_node, form_element_drop_on_app_tree_node);
}


void COrderTreeCtrl::DropItemOnItem(FormElementAppTreeNode& drag_from_app_tree_node, FormElementAppTreeNode& drop_on_app_tree_node)
{
    CDEItemBase* pDF_Item = (CDEItemBase*)drag_from_app_tree_node.GetFormBase();
    CDEItemBase* pDT_Item = (CDEItemBase*)drop_on_app_tree_node.GetFormBase();

    if (pDF_Item == pDT_Item)   // they dropped on themselves
        return;

    if (pDF_Item->GetParent() != pDT_Item->GetParent())     // they must share the same parent
        return;

    // now drop
    CDEGroup* pGroup = pDF_Item->GetParent();

    int iDF_Index = pGroup->GetItemIndex (pDF_Item),
        iDT_Index = pGroup->GetItemIndex (pDT_Item);

    if (iDT_Index < iDF_Index)
    {
        pGroup->RemoveItemAt (iDF_Index);           // remove first (only removes index, not item)
        pGroup->InsertItemAt (pDF_Item, iDT_Index);
    }
    else    // insert first, then delete, else ptrs off
    {
        pGroup->InsertItemAt (pDF_Item, iDT_Index);
        pGroup->RemoveItemAt (iDF_Index);
    }

    ReBuildTree (pDF_Item->GetFormNum(), pDF_Item);

    GetOrderDoc()->SetModifiedFlag(true);
}


void COrderTreeCtrl::SelectFTCNode(const AppTreeNode& app_tree_node, int iOrderNum, CDEFormBase* pSearchItem)
{
    HTREEITEM hLevel = GetChildItem(app_tree_node.GetHItem());    // this will give me level 0
    HTREEITEM hExpand = nullptr;

    while (hLevel != nullptr && hExpand == nullptr)
    {
        AppTreeNode* this_app_tree_node = GetTreeNode(hLevel);
        HTREEITEM hGroup = GetChildItem(this_app_tree_node->GetHItem());

        FindFTCRecursively (hGroup, hExpand, iOrderNum, pSearchItem);

        hLevel = GetNextSiblingItem(hLevel);
    }

    if (hExpand == nullptr) {
        DefaultExpand(app_tree_node.GetHItem());
    }
    else {
        SelectItem(hExpand);
    }

    SetRedraw(true);
    m_bOkToRedraw = true;
    Invalidate();
    UpdateWindow();
}


void COrderTreeCtrl::FindFTCRecursively(HTREEITEM& hGroupOrItem, HTREEITEM& hExpand, int iOrderNum, CDEFormBase* pSearchItem)
{
    HTREEITEM hPlaceHolder = nullptr;

    while (hGroupOrItem != nullptr && hExpand == nullptr)
    {
        const AppTreeNode* app_tree_node = GetTreeNode(hGroupOrItem);

        if (pSearchItem == nullptr)
        {
            //WE are not using the form numners in this
            hExpand = hGroupOrItem;
        }

        else if (app_tree_node->IsFormElement() && pSearchItem == app_tree_node->GetFormBase())
        {
            hExpand = hGroupOrItem;
            continue;
        }

        hPlaceHolder = hGroupOrItem;

        hGroupOrItem = GetChildItem( app_tree_node->GetHItem() );

        FindFTCRecursively (hGroupOrItem, hExpand, iOrderNum, pSearchItem);

        if (hExpand)    // found the guy we want to select in the recurse
            continue;   // will cause the loop to fail and hence, return

        // otherwise, continue on with placeholder
        hGroupOrItem = GetNextSiblingItem(hPlaceHolder);
    }
}


void COrderTreeCtrl::OnRButtonDown(UINT nFlags, CPoint point)
{
    UINT        flags;
    HTREEITEM hItem;
    if (point.x == 0 && point.y == 0) {
        hItem = GetSelectedItem();
        CRect rect;
        GetItemRect(hItem,rect,true);
        point.x = (rect.right + rect.left) / 2;
        point.y = (rect.bottom + rect.top) / 2;
    }
    else {
        hItem = HitTest(point, &flags);
    }
//    HTREEITEM hItem = HitTest(point, &flags);    // (mfc) tree ctrl function

    if(hItem) {
        m_bSendMsg = FALSE;
        m_hRestoreItemSelection = GetSelectedItem(); //Save Old Item Selection
        SelectItem(hItem);
        return;
    }

    CTreeCtrl::OnRButtonDown(nFlags, point);
}


void COrderTreeCtrl::OnRButtonUp(UINT /*nFlags*/, CPoint point)
{
    HTREEITEM hItem = HitTest(point);  // (mfc) tree ctrl function
    if (!hItem)     // if nothing selected, bail
        return;

    if (GetSelectedItem() == hItem) {
        SetFocus();     // this prevents the item from being expanding (in case it isn't)
    }
    else {
        m_bSendMsg = FALSE; // stop sending logic change messages OnRButtonUp
        SelectItem(hItem);  // otherwise, highlight it
    }

    const AppTreeNode* app_tree_node = GetTreeNode(hItem);

    BCMenu popup_menu;
    popup_menu.CreatePopupMenu();

    bool add_report_options = ( app_tree_node->GetAppFileType() == AppFileType::Report );

    if( add_report_options || app_tree_node->IsFormElement() )
    {
        popup_menu.AppendMenu(MF_STRING, ID_COPY_NAME, _T("&Copy Name"));
        popup_menu.AppendMenu(MF_SEPARATOR);
        popup_menu.AppendMenu(MF_STRING, ID_VIEW_LOGIC, _T("View Logic"));
        popup_menu.AppendMenu(MF_STRING, ID_VIEW_QUESTIONNAIRE, _T("View Questionnaire"));
    }

    if( add_report_options )
    {
        popup_menu.AppendMenu(MF_SEPARATOR);
        popup_menu.AppendMenu(MF_STRING, ID_EDIT_REPORT_PROPERTIES, _T("&Properties"));
    }

    if( popup_menu.GetMenuItemCount() > 0 )
    {
        CRect rect;
        GetWindowRect(rect);    // GetClientRect will not work, nd offset of entire screen
        rect.OffsetRect(5, 5);

        int iMenuSelection = popup_menu.TrackPopupMenu(TPM_RIGHTBUTTON | TPM_RETURNCMD, rect.left + point.x, rect.top + point.y, this);

        if( iMenuSelection != 0 )
            SendMessage(WM_COMMAND, iMenuSelection);
    }

    RestoreItemSelection();
}


void COrderTreeCtrl::RestoreItemSelection()
{
    if (m_hRestoreItemSelection) {
        SelectItem(m_hRestoreItemSelection);
        m_hRestoreItemSelection = nullptr;
        m_bSendMsg = TRUE;
    }
}


void COrderTreeCtrl::OnEditReportProperties()
{
    AppTreeNode* app_tree_node = GetTreeNode(GetSelectedItem());

    if( app_tree_node == nullptr || app_tree_node->GetAppFileType() != AppFileType::Report )
        return;

    if( AfxGetMainWnd()->SendMessage(UWM::Designer::EditReportProperties, reinterpret_cast<WPARAM>(&assert_cast<ReportAppTreeNode*>(app_tree_node)->GetNamedTextSource()),
                                                                          reinterpret_cast<LPARAM>(app_tree_node->GetOrderDocument())) == 1 )
    {
        // on success, redraw the tree
        Invalidate();
    }
}
void COrderTreeCtrl::OnViewQuestionnaire()
{
    CMDIChildWnd* pActiveWnd = assert_cast<CMDIFrameWnd*>(AfxGetMainWnd())->MDIGetActive();
    COrderChildWnd* pOrderChildWnd = dynamic_cast<COrderChildWnd*>(pActiveWnd);
    if (pOrderChildWnd) {
        pOrderChildWnd->SendMessage(WM_COMMAND, ID_VIEW_QUESTIONNAIRE);
    }

}

void COrderTreeCtrl::OnViewLogic()
{
    CMDIChildWnd* pActiveWnd = assert_cast<CMDIFrameWnd*>(AfxGetMainWnd())->MDIGetActive();
    COrderChildWnd* pOrderChildWnd = dynamic_cast<COrderChildWnd*>(pActiveWnd);
    if (pOrderChildWnd) {
        pOrderChildWnd->SendMessage(WM_COMMAND, ID_VIEW_LOGIC);
    }

}
void COrderTreeCtrl::OnCopyName()
{
    const AppTreeNode* app_tree_node = GetTreeNode(GetSelectedItem());

    if( app_tree_node == nullptr )
        return;

    if( app_tree_node->IsFormElement() ||
        app_tree_node->GetAppFileType() == AppFileType::Report )
    {
        WinClipboard::PutText(this, app_tree_node->GetName());
    }
}


CImageList* COrderTreeCtrl::CreateDragImageEx(HTREEITEM hItem)
{
//    if(GetImageList(TVSIL_NORMAL) != nullptr)
 //       return CreateDragImage(hItem);

    CRect rect;
    GetItemRect(hItem, rect, FALSE);
 //   rect.top = rect.left = 0;

    // Create bitmap
    CClientDC       dc (this);
    CDC             memDC;

    if(!memDC.CreateCompatibleDC(&dc))
        return nullptr;

    CBitmap bitmap;
    if(!bitmap.CreateCompatibleBitmap(&dc, rect.Width(), rect.Height()))
        return nullptr;

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


void COrderTreeCtrl::OnShiftF10()
{
    PostMessage(WM_RBUTTONDOWN);
}
