#include "StdAfx.h"
#include "TabTrCtl.h"
#include "TabDoc.h"
#include "TabView.h"
#include "TabChWnd.h"
#include "TSPDlg.h"
#include "TabPDlg.h"
#include <zToolsO/SharedSettings.h>

#ifdef _DEBUG
#define new DEBUG_NEW
#undef THIS_FILE
static char THIS_FILE[] = __FILE__;
#endif

#define ITEM_ICON       4
#define SUBITEM_ICON    5
#define VSET_ICON       6
#define ITEM_ICONC      7
#define SUBITEM_ICONC   8
#define VSET_ICONC      9
#define LEVEL_ICON      10

HCURSOR _hCursorNo;
HCURSOR _hCursorArrow;


/////////////////////////////////////////////////////////////////////////////
// CTabTreeCtrl

CTabTreeCtrl::CTabTreeCtrl()
{
    m_bPrintTree = true;
    m_bOkToRedraw= true ;
    m_bViewNames4TblView = false;

    m_bDragging = false;
    m_dwDragStart =0;
    m_pDragImage = nullptr;
    m_pLineWnd = nullptr;
    m_pDDTreeCtrl = nullptr;
}

CTabTreeCtrl::~CTabTreeCtrl()
{
    if (m_pDragImage) {
        delete m_pDragImage;
    }
}


BEGIN_MESSAGE_MAP(CTabTreeCtrl, CTreeCtrl)
    //{{AFX_MSG_MAP(CTabTreeCtrl)
    ON_NOTIFY_REFLECT(TVN_DELETEITEM, OnDeleteitem)
    ON_NOTIFY_REFLECT(TVN_GETDISPINFO, OnGetdispinfo)
    ON_WM_RBUTTONUP()
    ON_WM_RBUTTONDOWN()
    ON_WM_LBUTTONDBLCLK()
    ON_COMMAND(ID_ADD_TABLE, OnAddTable)
    ON_COMMAND(ID_INSERT_TABLE, OnInsertTable)
    ON_COMMAND(ID_DELETE_TABLE, OnDeleteTable)
    ON_COMMAND(ID_EDIT_VAR_TALLYATTRIB, OnEditVarTallyAttributes)
    ON_COMMAND(ID_EDIT_TBL_TALLYATTRIB, OnEditTableTallyAttributes)
    ON_COMMAND(ID_EDIT_GENERATELOGIC, OnEditGeneratelogic)
    ON_COMMAND(ID_EDIT_TABLE_EXCLUDE, OnEditExcludeTbl)
    ON_COMMAND(ID_EDIT_EXCLUDEALLBUTTHIS,OnEditExcludeAllButThis)
    ON_COMMAND(ID_EDIT_INCLUDEALL,OnEditIncludeAllTablesInRun)

    ON_COMMAND(ID_EDIT_TBL_FMT, OnEditTableFmt)
    ON_COMMAND(ID_EDIT_APP_FMT, OnEditAppFmt)
    ON_COMMAND(ID_EDIT_COPYTABLESTRUCTURE, OnEditCopytablestructure)
    ON_COMMAND(ID_EDIT_PASTETABLE, OnEditPastetable)

    ON_COMMAND(ID_EDIT_TBL_PRINTFMT, OnEditTablePrintFmt)

    ON_COMMAND(ID_EDIT_TABSET_PROP ,OnEditTabSetProp)
    ON_COMMAND(ID_EDIT_TABLE_PROP ,OnEditTableProp)
    ON_COMMAND(ID_SHIFT_F10, OnShiftF10)

    ON_WM_LBUTTONDOWN()
    ON_NOTIFY_REFLECT(TVN_SELCHANGED, OnSelchanged)
    //}}AFX_MSG_MAP
    ON_NOTIFY_REFLECT(TVN_BEGINDRAG, OnTvnBegindrag)
    ON_WM_LBUTTONUP()
    ON_WM_MOUSEMOVE()
    ON_WM_KEYDOWN()
    ON_NOTIFY_REFLECT(TVN_SELCHANGING, OnTvnSelchanging)
END_MESSAGE_MAP()

/////////////////////////////////////////////////////////////////////////////
// CTabTreeCtrl message handlers
/////////////////////////////////////////////////////////////////////////////
//
//                            CTabTreeCtrl::InsertTableSpec
//
/////////////////////////////////////////////////////////////////////////////

HTREEITEM CTabTreeCtrl::InsertTableSpec(const CString& sTableFileName, CTabulateDoc* pDoc)
{
    //Insert the table spec file in the table tree control
    //Use the table spec file name to insert it into the
    //structure later on to get the table spec file name
    TableSpecTabTreeNode* table_spec_tab_tree_node = new TableSpecTabTreeNode(pDoc, CS2WS(sTableFileName));
    table_spec_tab_tree_node->AddRef();

    TVINSERTSTRUCT tvi;

    tvi.hParent = TVI_ROOT;
    tvi.hInsertAfter = TVI_LAST;

    tvi.item.mask  = TVIF_TEXT | TVIF_PARAM | TVIF_IMAGE | TVIF_SELECTEDIMAGE;
    tvi.item.state = 0;
    tvi.item.stateMask = 0;
    tvi.item.pszText = (LPTSTR)LPSTR_TEXTCALLBACK;
    tvi.item.lParam = reinterpret_cast<LPARAM>(table_spec_tab_tree_node);
    tvi.item.iImage = 0;//I_IMAGECALLBACK;
    tvi.item.iSelectedImage =0;// I_IMAGECALLBACK;

    HTREEITEM hItem = InsertItem(&tvi);
    table_spec_tab_tree_node->SetHItem(hItem);

    return hItem;
}

/////////////////////////////////////////////////////////////////////////////
//
//                            CTabTreeCtrl::InsertTableDependencies
//
/////////////////////////////////////////////////////////////////////////////

bool CTabTreeCtrl::InsertTableDependencies(TableSpecTabTreeNode& table_spec_tab_tree_node)
{
//  add node to dict tree, or add reference

    //For Now NO MULTIPLE DICTS
    CSpecFile specFile(TRUE);

    if( !specFile.Open(table_spec_tab_tree_node.GetPath().c_str(), CFile::modeRead) )
    {
        AfxMessageBox(FormatText(_T("File %s Could not be opened"), table_spec_tab_tree_node.GetPath().c_str()));
        return false;
    }

    CString csLabel = ValFromHeader(specFile, CSPRO_CMD_LABEL);
    std::vector<std::wstring> dictionary_filenames = GetFileNameArrayFromSpecFile(specFile, CSPRO_DICTS);
    specFile.Close();

    if( dictionary_filenames.empty() )
    {
        AfxMessageBox(FormatText(_T("No Dictionaries in Spec file %s"), table_spec_tab_tree_node.GetPath().c_str()));
        return false;
    }

    CString csDictPath = WS2CS(dictionary_filenames.front());

    //  Get the handle to tree controls
    CDDTreeCtrl* pDictTree = GetDDTreeCtrl();
    DictionaryDictTreeNode* dictionary_dict_tree_node = pDictTree->GetDictionaryTreeNode(csDictPath);

    if( dictionary_dict_tree_node != nullptr )
    {
        dictionary_dict_tree_node->AddRef();
    }

    else
    {
        TVITEM pItem ;
        pItem.hItem = pDictTree->InsertDictionary(csLabel, csDictPath, nullptr);
        pItem.mask = TVIF_CHILDREN ;
        pItem.cChildren = 1;
        pDictTree->SetItem(&pItem);
    }

    return true;
}


bool CTabTreeCtrl::ReleaseTableDependencies(TableSpecTabTreeNode& table_spec_tab_tree_node)
{
    //  add node to dict tree, or add reference

    //For Now NO MULTIPLE DICTS

    CString csDictPath;

    //If the tab doc is opened get the dict name from the doc
    //
    if( table_spec_tab_tree_node.GetTabDoc() != nullptr ) {
        CTabulateDoc* pDoc = table_spec_tab_tree_node.GetTabDoc();
        csDictPath = pDoc->GetDictFileName();
        ASSERT(!csDictPath.IsEmpty());
    }
    else {
        CSpecFile specFile(TRUE);
        BOOL bOK = specFile.Open(table_spec_tab_tree_node.GetPath().c_str(), CFile::modeRead);
        if(bOK) {
            std::vector<std::wstring> dictionary_filenames = GetFileNameArrayFromSpecFile(specFile, CSPRO_DICTS);
            specFile.Close();

            if(dictionary_filenames.empty())  {
                AfxMessageBox(FormatText(_T("No Dictionaries in Spec file %s"), table_spec_tab_tree_node.GetPath().c_str()));
                return false;
            }
            csDictPath = WS2CS(dictionary_filenames.front());
        }
        else {
            AfxMessageBox(FormatText(_T("File %s Could not be opened"), table_spec_tab_tree_node.GetPath().c_str()));
            return false;
        }
    }

    //  Get the handle to tree controls
    CDDTreeCtrl* pDictTree = GetDDTreeCtrl();

    DictionaryDictTreeNode* dictionary_dict_tree_node = pDictTree->GetDictionaryTreeNode(csDictPath);
    ASSERT(dictionary_dict_tree_node!= nullptr);

    pDictTree->ReleaseDictionaryNode(*dictionary_dict_tree_node);

    return true;
}


void CTabTreeCtrl::ReleaseTableNode(TableSpecTabTreeNode& table_spec_tab_tree_node)
{
    table_spec_tab_tree_node.Release();

    if(table_spec_tab_tree_node.GetRefCount() == 0 ) {
        if(table_spec_tab_tree_node.GetTabDoc()) {
            //By now the  document Should be saved so go ahead and close the document
            table_spec_tab_tree_node.GetTabDoc()->OnCloseDocument();
            //drop the item from the tree
            DeleteItem(table_spec_tab_tree_node.GetHItem());
        }
        else {
            //remove the item from the tree
            DeleteItem(table_spec_tab_tree_node.GetHItem());
        }
    }
}


void CTabTreeCtrl::ReleaseDoc(TableSpecTabTreeNode& table_spec_tab_tree_node)
{
    if( table_spec_tab_tree_node.GetTabDoc() == nullptr )
        return;

    table_spec_tab_tree_node.SetDocument(nullptr);
    table_spec_tab_tree_node.Release();

    HTREEITEM hNode = table_spec_tab_tree_node.GetHItem();
    HTREEITEM hChild = GetChildItem(hNode);
    while(hChild) {
        DeleteItem(hChild);
        hChild = GetChildItem(hNode);
    }
    if(table_spec_tab_tree_node.GetRefCount() == 0) {
        DeleteItem(hNode);
    }
}


TableSpecTabTreeNode* CTabTreeCtrl::GetTableSpecTabTreeNode(CDocument& document) const
{
    HTREEITEM hItem = GetRootItem();

    while( hItem != nullptr )
    {
        TableElementTreeNode* table_element_tree_node = GetTreeNode(hItem);

        if( table_element_tree_node != nullptr &&
            table_element_tree_node->GetTableElementType() == TableElementType::TableSpec &&
            table_element_tree_node->GetDocument() == &document )
        {
            return assert_cast<TableSpecTabTreeNode*>(table_element_tree_node);
        }

        hItem = GetNextSiblingItem(hItem);
    }

    return nullptr;
}


TableSpecTabTreeNode* CTabTreeCtrl::GetTableSpecTabTreeNode(wstring_view filename) const
{
    HTREEITEM hItem = GetRootItem();

    while( hItem != nullptr )
    {
        TableElementTreeNode* table_element_tree_node = GetTreeNode(hItem);

        if( table_element_tree_node != nullptr &&
            table_element_tree_node->GetTableElementType() == TableElementType::TableSpec &&
            SO::EqualsNoCase(filename, table_element_tree_node->GetPath()) )
        {
            return assert_cast<TableSpecTabTreeNode*>(table_element_tree_node);
        }

        hItem = GetNextSiblingItem(hItem);
    }

    return nullptr;
}


/////////////////////////////////////////////////////////////////////////////
//
//                            CTabTreeCtrl::RemoveTableIDs
//
//  Use this only when the starting Items is a TableNode
//  Removes the CTableIDs attached to the hItems for this hItem and its children
//
/////////////////////////////////////////////////////////////////////////////

void CTabTreeCtrl::RemoveTableIDs(HTREEITEM hItem)
{
    //remove the table ID of this Item
    TableElementTreeNode* table_element_tree_node = GetTreeNode(hItem);
    if( table_element_tree_node != nullptr ){
        delete table_element_tree_node;
        SetItemData(hItem,0);
    }
    //check if it has any children
    HTREEITEM hChild;
    hChild = GetChildItem(hItem);

    //Remove the DictIds for the child items
    while(hChild) {
        RemoveTableIDs(hChild);
        hChild = GetChildItem(hChild);
    }

    //Get the sibling items
    hItem = GetNextSiblingItem(hItem);

    //if the item has  parent only then call for the deletion of the sibling items and its children
    if(GetParentItem(hItem))
        RemoveTableIDs(hItem);

}

/////////////////////////////////////////////////////////////////////////////
//
//                            CTabTreeCtrl::InitImageList
//
/////////////////////////////////////////////////////////////////////////////

void CTabTreeCtrl::InitImageList()
{
    m_cImageList.Create(16, 16, 0, 0, 4); // x,y of icon, flags, initial sz, grow by

    m_cImageList.SetBkColor(RGB(255,255,255));

    HICON icon0 = AfxGetApp()->LoadIcon (IDR_TABLE_FRAME);  // 0
    HICON icon1 = AfxGetApp()->LoadIcon (IDI_TABLE);        // 1
    HICON icon2 = AfxGetApp()->LoadIcon (IDI_TABLE_LOGIC);  // 2
    HICON icon3 = AfxGetApp()->LoadIcon (IDI_TABLECOL);     // 3 ** not used **
    HICON icon4 = AfxGetApp()->LoadIcon (IDI_ITEM);         // 4
    HICON icon5 = AfxGetApp()->LoadIcon (IDI_SUBITEM);      // 5
    HICON icon6 = AfxGetApp()->LoadIcon (IDI_VSET);         // 6
    HICON icon7 = AfxGetApp()->LoadIcon (IDI_ITEMC);        // 7
    HICON icon8 = AfxGetApp()->LoadIcon (IDI_SUBITEMC);     // 8
    HICON icon9 = AfxGetApp()->LoadIcon (IDI_VSETC);        // 9
    HICON icon10 = AfxGetApp()->LoadIcon (IDI_TLEVEL);      // 10

    HICON icon11 = AfxGetApp()->LoadIcon (IDI_XTBL);        // 11
    HICON icon12 = AfxGetApp()->LoadIcon (IDI_XTBLOG);      // 12

    m_cImageList.Add (icon0);
    m_cImageList.Add (icon1);
    m_cImageList.Add (icon2);
    m_cImageList.Add (icon3);
    m_cImageList.Add (icon4);
    m_cImageList.Add (icon5);
    m_cImageList.Add (icon6);
    m_cImageList.Add (icon7);
    m_cImageList.Add (icon8);
    m_cImageList.Add (icon9);
    m_cImageList.Add (icon10);

    m_cImageList.Add (icon11);
    m_cImageList.Add (icon12);

    SetImageList (&m_cImageList, TVSIL_NORMAL);
    m_dropTarget.Register(this);
    _hCursorNo = ::LoadCursor(nullptr,IDC_NO);
    _hCursorArrow = ::LoadCursor(nullptr,IDC_ARROW);
}


/////////////////////////////////////////////////////////////////////////////
//
//                            CTabTreeCtrl::OpenTableFile
//
/////////////////////////////////////////////////////////////////////////////
bool CTabTreeCtrl::OpenTableFile(const CString& sTableFile ,const std::shared_ptr<const CDataDict> pWorkDict/* = nullptr*/, bool bMakeVisible/* = true*/)
{
    TableSpecTabTreeNode * table_element_tree_node = GetTableSpecTabTreeNode(sTableFile);

    if(table_element_tree_node->GetTabDoc())
        return TRUE;

    CTabulateDoc* pDoc = (CTabulateDoc*)m_pDocTemplate->OpenDocumentFile(sTableFile, bMakeVisible);
    if(!pDoc)
        return FALSE;

    pDoc->SetTabTreeCtrl (this);


    POSITION pos = pDoc->GetFirstViewPosition();
    CTabView* pView = (CTabView*)pDoc->GetNextView(pos);
    if(!bMakeVisible) {
        pView->GetParentFrame()->ShowWindow(SW_SHOWNOACTIVATE);
        ((CTableChildWnd*)pView->GetParentFrame())->SetIsViewer(false);
        pView->OnInitialUpdate(); // OnInitial Update will not be called when bMakeVisible = FALSE;

    }

    if( !pDoc->LoadSpecFile(sTableFile, pWorkDict) )
    {
        pDoc->OnCloseDocument();
        table_element_tree_node->SetDocument(nullptr);
        return FALSE;
    }
    SetTabDoc(pDoc);   // set the tree ctrl's ptr to the doc
    table_element_tree_node->SetDocument(pDoc);

    return TRUE;
}

/////////////////////////////////////////////////////////////////////////////
//
//                            CTabTreeCtrl::OnReBuildTree
//
//      Use this every time the tree must change.  It removes all entries under
//      the node for the document, then builds them back up.
//
/////////////////////////////////////////////////////////////////////////////

void CTabTreeCtrl::ReBuildTree(bool bAll /*=false*/)
{
    SetRedraw(FALSE);               // Don't draw while changing

    // find node corresponding to this table
    TableSpecTabTreeNode* table_spec_tab_tree_node = GetTableSpecTabTreeNode(*m_pDoc);

    if(!bAll) {
        // delete all descendants of this node
        HTREEITEM hNode = table_spec_tab_tree_node->GetHItem();
        HTREEITEM h = GetChildItem(hNode);
        while (h != nullptr) {
            HTREEITEM hSib = GetNextSiblingItem(h);
            DeleteItem(h);
            h = hSib;
        }

        // reconstruct descendent nodes
        if(m_bPrintTree) {
            BuildTree(*table_spec_tab_tree_node);
        }
        else {
            BuildLevelTree(*table_spec_tab_tree_node);
        }
    }
    else {
        //for each document
        HTREEITEM hItem = this->GetRootItem();
        while(hItem != nullptr) {
            // delete all descendants of this node
            HTREEITEM hNode = table_spec_tab_tree_node->GetHItem();
            HTREEITEM h = GetChildItem(hNode);
            while (h != nullptr) {
                HTREEITEM hSib = GetNextSiblingItem(h);
                DeleteItem(h);
                h = hSib;
            }

            // reconstruct descendent nodes
            if(m_bPrintTree) {
                BuildTree(*table_spec_tab_tree_node);
            }
            else {
                BuildLevelTree(*table_spec_tab_tree_node);
            }
            hItem = this->GetNextSiblingItem(hItem);
        }

    }
    if(table_spec_tab_tree_node != nullptr && table_spec_tab_tree_node->GetHItem() != nullptr){
        Expand(table_spec_tab_tree_node->GetHItem(), TVE_EXPAND);
    }
}


/////////////////////////////////////////////////////////////////////////////
//
//                            CTabTreeCtrl::BuildTree
//
//      This is called once, when document is opened
//
/////////////////////////////////////////////////////////////////////////////
static  CTabulateDoc* pDocObject ;
static CTable* pTableObject;
LOGFONT CTabTreeCtrl::m_DefLogFont ={ 0, 0, 0, 0 };
CFont  CTabTreeCtrl::m_font;

void CTabTreeCtrl::BuildTree(TableSpecTabTreeNode& table_spec_tab_tree_node)
{
    if(m_DefLogFont.lfHeight == 0 )  {
        CFont* pFont = GetFont();
        pFont->GetLogFont(&m_DefLogFont);
    }
    LOGFONT logfont;
    memset(&logfont,0,sizeof(LOGFONT));
    bool bChangeFont = false;
    m_font.GetSafeHandle() == nullptr ? bChangeFont = true : m_font.GetLogFont(&logfont);

    CIMSAString sFontName = GetDesignerFontName();
    sFontName.Trim();

    CIMSAString sDefFontFaceName(m_DefLogFont.lfFaceName);
    if((!sFontName.IsEmpty() && sFontName.CompareNoCase(logfont.lfFaceName) != 0) || (sFontName.IsEmpty() && sDefFontFaceName.CompareNoCase(logfont.lfFaceName) != 0 )){
        bChangeFont = true;
    }

    if(bChangeFont || logfont.lfHeight != m_DefLogFont.lfHeight * (GetDesignerFontZoomLevel()/ 100.0)){
        logfont = m_DefLogFont; //inititalize
        logfont.lfHeight = (LONG)(m_DefLogFont.lfHeight * (GetDesignerFontZoomLevel() / 100.0)); //zoom

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

    pDocObject = table_spec_tab_tree_node.GetTabDoc();
    if(!pDocObject) {
        return;
    }
    POSITION pos = pDocObject->GetFirstViewPosition();
    ASSERT(pos != nullptr);

    SetTabDoc(pDocObject);

    TV_INSERTSTRUCT tvi;

    SetRedraw(FALSE);               // Don't draw while changing
    m_bOkToRedraw = false;
    m_bSendMsg = false;

    CTabSet* pSpec = pDocObject->GetTableSpec();

    tvi.item.mask = TVIF_TEXT | TVIF_PARAM | TVIF_IMAGE | TVIF_SELECTEDIMAGE|TVIF_STATE;

    tvi.item.state= 0;
    tvi.item.stateMask= TVIS_BOLD;
    tvi.item.pszText = (LPTSTR)LPSTR_TEXTCALLBACK;

    HTREEITEM htiRoot = table_spec_tab_tree_node.GetHItem();
    int iNumTbls = pSpec->GetNumTables();

    tvi.hParent = htiRoot;


    for (int iIndex = 0 ; iIndex < iNumTbls ; iIndex++) {
        pTableObject = pSpec->GetTable(iIndex);
        tvi.hParent = htiRoot;

        TableElementTreeNode* table_element_tree_node = new TableElementTreeNode(pDocObject, pTableObject);
        tvi.item.lParam = reinterpret_cast<LPARAM>(table_element_tree_node);

        if (pTableObject->GetGenerateLogic()) {
             if (pTableObject->IsTableExcluded4Run()) {
                tvi.item.iImage = 11;
                tvi.item.iSelectedImage = 11;
             }
             else {
                tvi.item.iImage = 1;
                tvi.item.iSelectedImage = 1;
             }
        }
        else {
             if (pTableObject->IsTableExcluded4Run()) {
                tvi.item.iImage = 12;
                tvi.item.iSelectedImage = 12;
             }
             else {
                tvi.item.iImage = 2;
                tvi.item.iSelectedImage = 2;
            }
        }

        HTREEITEM htiLevel = InsertItem(&tvi);
        table_element_tree_node->SetHItem(htiLevel);

        CTabVar* pRowTabVar = pTableObject->GetRowRoot();
        BuildRowColTreeItem(pRowTabVar, htiLevel);

        CTabVar* pColTabVar = pTableObject->GetColRoot();
        BuildRowColTreeItem(pColTabVar, htiLevel,TableElementType::ColItem);
    }

    //DefaultExpand(htiRoot);
    SetRedraw(TRUE);
    m_bOkToRedraw = true;
    m_bSendMsg = true;
    Invalidate();
    UpdateWindow();
}


void CTabTreeCtrl::DefaultExpand(HTREEITEM hItem)
{
    while(hItem != nullptr )
    {
        Expand(hItem, TVE_EXPAND);
        DefaultExpand(GetChildItem(hItem));
        hItem = GetNextSiblingItem(hItem);
    }
}


void CTabTreeCtrl::BuildLevelTree(TableSpecTabTreeNode& table_spec_tab_tree_node)
{
    pDocObject = table_spec_tab_tree_node.GetTabDoc();
    if(!pDocObject) {
        return;
    }

    POSITION pos = pDocObject->GetFirstViewPosition();
    ASSERT(pos != nullptr);

    SetTabDoc(pDocObject);

    TV_INSERTSTRUCT tvi;

    SetRedraw(FALSE);               // Don't draw while changing
    m_bOkToRedraw = false;
    m_bSendMsg = false;

    CTabSet* pSpec = pDocObject->GetTableSpec();

    tvi.item.mask = TVIF_TEXT | TVIF_PARAM | TVIF_IMAGE | TVIF_SELECTEDIMAGE|TVIF_STATE;

    tvi.item.state= 0;
    tvi.item.stateMask= TVIS_BOLD;

    tvi.item.pszText = (LPTSTR)LPSTR_TEXTCALLBACK;

    HTREEITEM htiRoot = table_spec_tab_tree_node.GetHItem();
    int iNumLevel = pSpec->GetNumLevels();

    tvi.hParent = htiRoot;

//For each Level
    for(int iLevel = 0; iLevel < iNumLevel ; iLevel++) {
        //For each Table in the level
        CTabLevel* pTabLevel = pSpec->GetLevel(iLevel);

        tvi.hParent = htiRoot;

        TableElementTreeNode* level_table_element_tree_node = new TableElementTreeNode(pDocObject, pTabLevel, iLevel);
        tvi.item.lParam = reinterpret_cast<LPARAM>(level_table_element_tree_node);

        tvi.item.iImage = LEVEL_ICON;
        tvi.item.iSelectedImage = LEVEL_ICON;

        HTREEITEM htiLevel = InsertItem(&tvi);
        level_table_element_tree_node->SetHItem(htiLevel);

        for (int iIndex = 0 ; iIndex < pTabLevel->GetNumTables() ; iIndex++) {
            pTableObject = pTabLevel->GetTable(iIndex);
            tvi.hParent = htiLevel;

            TableElementTreeNode* table_table_element_tree_node = new TableElementTreeNode(pDocObject, pTableObject);
            tvi.item.lParam = reinterpret_cast<LPARAM>(table_table_element_tree_node);

            if (pTableObject->GetGenerateLogic()) {
                if (pTableObject->IsTableExcluded4Run()) {
                    tvi.item.iImage = 11;
                    tvi.item.iSelectedImage = 11;
                }
                else{
                    tvi.item.iImage = 1;
                    tvi.item.iSelectedImage = 1;
                }
            }
            else {
                if (pTableObject->IsTableExcluded4Run()) {
                    tvi.item.iImage = 12;
                    tvi.item.iSelectedImage = 12;
                }
                else{
                    tvi.item.iImage = 2;
                    tvi.item.iSelectedImage = 2;
                }
            }

            HTREEITEM htiTable = InsertItem(&tvi);
            table_table_element_tree_node->SetHItem(htiTable);

            CTabVar* pRowTabVar = pTableObject->GetRowRoot();
            BuildRowColTreeItem(pRowTabVar, htiTable);

            CTabVar* pColTabVar = pTableObject->GetColRoot();
            BuildRowColTreeItem(pColTabVar, htiTable,TableElementType::ColItem);
        }
        Expand(htiLevel, TVE_EXPAND);
    }

  //  DefaultExpand(htiRoot);

    SetRedraw(TRUE);
    m_bOkToRedraw = true;
    m_bSendMsg = true;

    Invalidate();
    UpdateWindow();
}


bool CTabTreeCtrl::BuildRowColTreeItem(CTabVar* pTabVar, HTREEITEM hParentItem, TableElementType table_element_type/* = TableElementType::RowItem*/)
{
    TV_INSERTSTRUCT tvi;

    tvi.item.mask = TVIF_TEXT | TVIF_PARAM | TVIF_IMAGE | TVIF_SELECTEDIMAGE|TVIF_STATE;
    tvi.item.pszText = (LPTSTR)LPSTR_TEXTCALLBACK;
    tvi.item.state= 0;
    tvi.item.stateMask= TVIS_BOLD;
    tvi.hParent = hParentItem;

    TableElementTreeNode* table_element_tree_node = new TableElementTreeNode(pDocObject, table_element_type, pTableObject, pTabVar);
    tvi.item.lParam = reinterpret_cast<LPARAM>(table_element_tree_node);

    int iImage = -1;
    if(table_element_type != TableElementType::RowItem)
         iImage = GetImage(pTabVar,false);
    else
         iImage = GetImage(pTabVar);

    tvi.item.iImage = iImage;
    tvi.item.iSelectedImage = iImage;

    HTREEITEM hItem = InsertItem(&tvi);
    table_element_tree_node->SetHItem(hItem);

    if(pTabVar->GetNumChildren()) {
        int iSize = pTabVar->GetNumChildren();
        for(int iIndex =0 ; iIndex < iSize; iIndex++) {
            bool bBoth = pTabVar->GetChild(iIndex)->GetName().CompareNoCase(_T("TT_BOTH")) ==0;
            bool bCustom = pTabVar->GetChild(iIndex)->GetType() == VT_CUSTOM ;
            if(bBoth && bCustom ){
                continue;
            }
            BuildRowColTreeItem(pTabVar->GetChild(iIndex), hItem, table_element_type);
        }
    }
    return true;
}


/////////////////////////////////////////////////////////////////////////////////
//
//  int CTabTreeCtrl::GetImage(CXTItemSpec* pXtabItem,bool bRow /*=true*/)
//
/////////////////////////////////////////////////////////////////////////////////
int CTabTreeCtrl::GetImage(CTabVar* pTabVar ,bool bRow /*=true*/)
{
    const CDictItem* pDictItem;
    const DictValueSet* pDictVSet;

  //  return ITEM_ICON;
    if(pTabVar->IsRoot() && bRow)
        return VSET_ICON;
    else if(pTabVar->IsRoot() && !bRow)
        return VSET_ICONC;

    pDocObject->GetTableSpec()->LookupName(pTabVar->GetName(), nullptr, nullptr, &pDictItem, &pDictVSet);
    if(!pDictItem && !pDictVSet) {
         if(bRow) {
                return ITEM_ICON;
            }
            else {
                return ITEM_ICONC;
            }
    }
    if (pDictVSet == nullptr) {
        if (pDictItem->GetItemType() == ItemType::Subitem) {
            if(bRow) {
                return SUBITEM_ICON;
            }
            else {
                return SUBITEM_ICONC;
            }
        }
        else {
            if(bRow) {
                return ITEM_ICON;
            }
            else {
                return ITEM_ICONC;
            }
        }
    }
    if (pDictItem->GetNumValueSets() > 1) {
        if(bRow) {
            return VSET_ICON;
        }
        else {
            return VSET_ICONC;
        }
    }
    CString sVSetLabel = pDictVSet->GetLabel();
    CString sItemLabel = pDictItem->GetLabel();
    if (sVSetLabel.CompareNoCase(sItemLabel)) {
        if(bRow) {
            return VSET_ICON;
        }
        else {
            return VSET_ICONC;
        }
    }
    else {
        if (pDictItem->GetItemType() == ItemType::Subitem) {
            if(bRow) {
                return SUBITEM_ICON;
            }
            else {
                return SUBITEM_ICONC;
            }
        }
        else {
            if(bRow) {
                return ITEM_ICON;
            }
            else {
                return ITEM_ICONC;
            }
        }
    }
}


/////////////////////////////////////////////////////////////////////////////////
//
//  void CTabTreeCtrl::OnDeleteitem(NMHDR* pNMHDR, LRESULT* pResult)
//
/////////////////////////////////////////////////////////////////////////////////
void CTabTreeCtrl::OnDeleteitem(NMHDR* pNMHDR, LRESULT* pResult)
{
    NM_TREEVIEW* pNMTreeView = (NM_TREEVIEW*)pNMHDR;
    // TODO: Add your control notification handler code here
    HTREEITEM hItem = pNMTreeView->itemOld.hItem;
    TableElementTreeNode* table_element_tree_node = GetTreeNode(hItem);
    if( table_element_tree_node != nullptr ) {
        delete table_element_tree_node;
    }

    *pResult = 0;
}


/////////////////////////////////////////////////////////////////////////////////
//
//  void CTabTreeCtrl::OnGetdispinfo(NMHDR* pNMHDR, LRESULT* pResult)
//
/////////////////////////////////////////////////////////////////////////////////
void CTabTreeCtrl::OnGetdispinfo(NMHDR* pNMHDR, LRESULT* pResult)
{
    TV_DISPINFO* pTVDispInfo = reinterpret_cast<TV_DISPINFO*>(pNMHDR);
    const TableElementTreeNode* table_element_tree_node = reinterpret_cast<const TableElementTreeNode*>(pTVDispInfo->item.lParam);
    CTabView* pTabView = nullptr;

     if( table_element_tree_node != nullptr && table_element_tree_node->GetTabDoc() )
     {
        POSITION pos = table_element_tree_node->GetTabDoc()->GetFirstViewPosition();

        while( pos != nullptr )
        {
            CView* pView = table_element_tree_node->GetTabDoc()->GetNextView(pos);

            if( pView->IsKindOf(RUNTIME_CLASS(CTabView)) )
            {
                pTabView = assert_cast<CTabView*>(pView);
                break;
            }
        }
     }

     if( pTabView == nullptr )
         return;

    bool view_name;
    bool append_labels_to_names;

    if( assert_cast<CTableChildWnd*>(pTabView->GetParentFrame())->IsViewer() )
    {
        view_name = GetViewNames4TblView();
        append_labels_to_names = false;
    }

    else
    {
        view_name = SharedSettings::ViewNamesInTree();
        append_labels_to_names = ( view_name && SharedSettings::AppendLabelsToNamesInTree() );
    }

    std::wstring display_text = view_name ? table_element_tree_node->GetName() : table_element_tree_node->GetLabel();

    if( view_name && SharedSettings::AppendLabelsToNamesInTree() )
    {
        std::wstring label = table_element_tree_node->GetLabel();

        if( display_text != label )
            SO::Append(display_text, _T(": "), label);
    }

    lstrcpyn(pTVDispInfo->item.pszText, display_text.c_str(), pTVDispInfo->item.cchTextMax);

    *pResult = 0;
}


/////////////////////////////////////////////////////////////////////////////////
//
//  void CTabTreeCtrl::OnRButtonUp(UINT nFlags, CPoint point)
//
/////////////////////////////////////////////////////////////////////////////////

void CTabTreeCtrl::OnRButtonUp(UINT nFlags, CPoint point)
{
    // disable mouse clicks if universe or parms dialog boxes are up
    // Set selection and focus to tree control
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
    CMenu hMenu;

    bool bAllowAIDs = true; // allow "Add/Ins/Del Table" opts?

    if(hItem) {
        if (GetSelectedItem() == hItem) {
            SetFocus();
        }
        else {
            SelectItem(hItem);
            UpdateTableOnSelect();
        }

        TableElementTreeNode* table_element_tree_node = GetTreeNode(hItem);
        CTblGrid* pGrid = nullptr;
        CTabView* pTabView = nullptr;

        if (table_element_tree_node != nullptr && table_element_tree_node->GetTabDoc()) {
            POSITION pos = table_element_tree_node->GetTabDoc()->GetFirstViewPosition();
            while(pos) {
                CView* pView = table_element_tree_node->GetTabDoc()->GetNextView(pos);
                if(pView->IsKindOf(RUNTIME_CLASS(CTabView))) {
                    pTabView = (CTabView*)pView;
                    break;
                }
            }
            if(pTabView) {
                pGrid = pTabView->GetGrid ();
            }
        }

        if ( pGrid != nullptr && pGrid->IsGridEmpty() ) {  // case #1: table completely empty, no items dragged out

            //bAllowAIDs = false; // can't add/ins new table if nothing on current one!
            bAllowAIDs = true;
        }
        hMenu.CreatePopupMenu();

        this->ClientToScreen(&point);

        const bool bViewer = (pTabView == nullptr) ?
                             false :
                             assert_cast<CTableChildWnd*>(pTabView->GetParentFrame())->IsViewer();

        CString sQuoteStr;
        if (!bViewer) {
            switch (table_element_tree_node->GetTableElementType()) {
                case TableElementType::TableSpec:
                    sQuoteStr = _T("Properties");
                    hMenu.AppendMenu(MF_STRING, ID_EDIT_TABSET_PROP,sQuoteStr);
                    AppendMenu(hMenu, MF_SEPARATOR, NULL, nullptr);
                    break;
                case TableElementType::Table:
                    // BMD 23 Jan 2006
                    sQuoteStr = _T("Properties");
                    hMenu.AppendMenu(MF_STRING, ID_EDIT_TABLE_PROP,sQuoteStr);
                    AppendMenu(hMenu, MF_SEPARATOR, NULL, nullptr);

                    sQuoteStr = _T("Exclude From Run");
                    if(table_element_tree_node->GetTable()->IsTableExcluded4Run()){
                        hMenu.AppendMenu(MF_STRING|MF_CHECKED,ID_EDIT_TABLE_EXCLUDE,sQuoteStr);
                    }
                    else {
                        hMenu.AppendMenu(MF_STRING|MF_UNCHECKED,ID_EDIT_TABLE_EXCLUDE,sQuoteStr);
                    }
                    sQuoteStr = _T("Exclude All But This");
                    hMenu.AppendMenu(MF_STRING|MF_UNCHECKED,ID_EDIT_EXCLUDEALLBUTTHIS,sQuoteStr);

                    sQuoteStr = _T("Include All Tables");
                    hMenu.AppendMenu(MF_STRING|MF_UNCHECKED,ID_EDIT_INCLUDEALL,sQuoteStr);

                    AppendMenu(hMenu, MF_SEPARATOR, NULL, nullptr);
                    if (table_element_tree_node->GetTable()->GetNumRows() > 0 || table_element_tree_node->GetTable()->GetNumCols() > 0) {
                            sQuoteStr = _T("Tally Attributes (Table)");
                            hMenu.AppendMenu(MF_STRING, ID_EDIT_TBL_TALLYATTRIB,sQuoteStr);
                            AppendMenu(hMenu, MF_SEPARATOR, NULL, nullptr);
                    }
                    if(assert_cast<CTableChildWnd*>(pTabView->GetParentFrame())->IsLogicViewActive()){
                        sQuoteStr = _T("Generate Logic(Table)");
                        if(table_element_tree_node->GetTable()->GetGenerateLogic()){
                            hMenu.AppendMenu(MF_STRING|MF_CHECKED,ID_EDIT_GENERATELOGIC,sQuoteStr);
                        }
                        else {
                            hMenu.AppendMenu(MF_STRING|MF_UNCHECKED,ID_EDIT_GENERATELOGIC,sQuoteStr);
                        }
                        AppendMenu(hMenu, MF_SEPARATOR, NULL, nullptr);
                    }
                    break;
                case TableElementType::RowItem:
                    if(table_element_tree_node->GetTabVar() != table_element_tree_node->GetTable()->GetRowRoot()){
                        pTabView->SetCurVar(table_element_tree_node->GetTabVar());
                        sQuoteStr = _T("Tally Attributes (") + table_element_tree_node->GetTabVar()->GetText() + _T(")");
                        AppendMenu(hMenu, MF_STRING, ID_EDIT_VAR_TALLYATTRIB, sQuoteStr);
                    }
                    // BMD 23 Jan 2006
                    if (table_element_tree_node->GetTable()->GetNumRows() > 0 || table_element_tree_node->GetTable()->GetNumCols() > 0) {
                        sQuoteStr = _T("Tally Attributes (Table)");
                        hMenu.AppendMenu(MF_STRING, ID_EDIT_TBL_TALLYATTRIB,sQuoteStr);
                        AppendMenu(hMenu, MF_SEPARATOR, NULL, nullptr);
                    }
                    break;
                case TableElementType::ColItem:
                    if(table_element_tree_node->GetTabVar() != table_element_tree_node->GetTable()->GetColRoot()){
                        pTabView->SetCurVar(table_element_tree_node->GetTabVar());
                        sQuoteStr = _T("Tally Attributes (") + table_element_tree_node->GetTabVar()->GetText() + _T(")");
                        AppendMenu(hMenu, MF_STRING, ID_EDIT_VAR_TALLYATTRIB, sQuoteStr);
                    }
                    // BMD 23 Jan 2006
                    if (table_element_tree_node->GetTable()->GetNumRows() > 0 || table_element_tree_node->GetTable()->GetNumCols() > 0) {
                        sQuoteStr = _T("Tally Attributes (Table)");
                        hMenu.AppendMenu(MF_STRING, ID_EDIT_TBL_TALLYATTRIB,sQuoteStr);
                        AppendMenu(hMenu, MF_SEPARATOR, NULL, nullptr);
                    }
                    break;
                default:
                    break;
            }
        }

        //since no component on the grid is selected this is always grayed .
        //we cant say edit spanner/caption 'cos we wont know which panel is selected
        sQuoteStr=_T("Format");
        hMenu.AppendMenu(MF_GRAYED, ID_EDIT_COMPONENT_FMT, sQuoteStr);

        sQuoteStr = _T("Format (Table)");
        hMenu.AppendMenu(MF_STRING, ID_EDIT_TBL_FMT,sQuoteStr);

        sQuoteStr = _T("Format (Application)");
        hMenu.AppendMenu(MF_STRING, ID_EDIT_APP_FMT,sQuoteStr);

        AppendMenu(hMenu, MF_SEPARATOR, NULL, nullptr);
        sQuoteStr = _T("Format Print (Table)");
        hMenu.AppendMenu(MF_STRING, ID_EDIT_TBL_PRINTFMT,sQuoteStr);
        AppendMenu(hMenu, MF_SEPARATOR, NULL, nullptr);

        if (!bViewer) {
            hMenu.AppendMenu(MF_STRING, ID_EDIT_COPYTABLESTRUCTURE, _T("Copy Table Spec"));
            CTabulateDoc* pDoc = table_element_tree_node->GetTabDoc();
            if (IsClipboardFormatAvailable(pDoc->GetClipBoardFormat(TD_TABLE_FORMAT))) {
                hMenu.AppendMenu(MF_STRING, ID_EDIT_PASTETABLE, _T("Paste Table Spec"));
            }
            else {
                hMenu.AppendMenu(MF_GRAYED, ID_EDIT_PASTETABLE, _T("Paste Table Spec"));
            }

            AppendMenu(hMenu, MF_SEPARATOR, NULL, nullptr);
            if (bAllowAIDs) {
                hMenu.AppendMenu(MF_STRING, ID_ADD_TABLE, _T("Add Table"));
                hMenu.AppendMenu(MF_STRING, ID_INSERT_TABLE, _T("Insert Table"));
                hMenu.AppendMenu(MF_STRING, ID_DELETE_TABLE, _T("Delete Table"));
            }
            else {
                hMenu.AppendMenu(MF_GRAYED, ID_ADD_TABLE, _T("Add Table"));
                hMenu.AppendMenu(MF_GRAYED, ID_INSERT_TABLE, _T("Insert Table"));
                hMenu.AppendMenu(MF_GRAYED, ID_DELETE_TABLE, _T("Delete Table"));
            }
        }

        // remove trailing separator in menu if one exists
        int iLastItem = hMenu.GetMenuItemCount() - 1;
        if (hMenu.GetMenuState(iLastItem, MF_BYPOSITION) & MF_SEPARATOR) {
            hMenu.RemoveMenu(iLastItem, MF_BYPOSITION);
        }
        hMenu.TrackPopupMenu (TPM_RIGHTBUTTON, point.x, point.y, this);

    }
    CTreeCtrl::OnRButtonUp(nFlags, point);

}

/////////////////////////////////////////////////////////////////////////////////
//
//  void CTabTreeCtrl::OnRButtonDown(UINT nFlags, CPoint point)
//
/////////////////////////////////////////////////////////////////////////////////

void CTabTreeCtrl::OnRButtonDown(UINT nFlags, CPoint point)
{
    UINT flags ;
    HTREEITEM hItem = HitTest(point, &flags);    // (mfc) tree ctrl function

    if(hItem) {
        SelectItem(hItem);
        return;
    }
    CTreeCtrl::OnRButtonDown(nFlags, point);
}

/////////////////////////////////////////////////////////////////////////////////
//
//  void CTabTreeCtrl::OnLButtonDown(UINT nFlags, CPoint point)
//
/////////////////////////////////////////////////////////////////////////////////

void CTabTreeCtrl::OnLButtonDown(UINT nFlags, CPoint point)
{
    m_hitemDrag = nullptr;
    m_hitemDrop = nullptr;
    HTREEITEM hItem = HitTest(point);

    if(hItem) {
        if (GetSelectedItem() == hItem) {
            SetFocus();
        }
        else {
            SelectItem(hItem);
        }
    }
    else {
        return;
    }
    UpdateTableOnSelect();

    CMDIChildWnd* pActiveWnd = ((CMDIFrameWnd*)AfxGetMainWnd())->MDIGetActive();
    if(pActiveWnd->IsKindOf(RUNTIME_CLASS(CTableChildWnd))){
        if(hItem) {
            TableElementTreeNode* table_element_tree_node = GetTreeNode(hItem);
            if( table_element_tree_node != nullptr && table_element_tree_node->GetTableElementType() == TableElementType::Table){
                m_dwDragStart = GetTickCount();
            }
        }
    }

    CTreeCtrl::OnLButtonDown(nFlags, point);
}


void CTabTreeCtrl::UpdateTableOnSelect(bool bUpdatePrintView /*= true*/)
{
    HTREEITEM hItem = GetSelectedItem();
    TableElementTreeNode* table_element_tree_node = GetTreeNode(hItem);
    CTabulateDoc* pDoc = table_element_tree_node->GetTabDoc();

    if (!pDoc)
        return;

    POSITION pos = pDoc->GetFirstViewPosition();
    CTabView* pView = (CTabView*)pDoc->GetNextView(pos);
    pView->GetParentFrame()->ActivateFrame();
    CTable* pCurTable = pView->GetCurTable();

    if(table_element_tree_node->GetTable() && pCurTable != table_element_tree_node->GetTable()) {
        CFrameWnd* pFrameWnd =((CFrameWnd*)AfxGetMainWnd())->GetActiveFrame();
        CTableChildWnd* pTabChldWnd = DYNAMIC_DOWNCAST(CTableChildWnd,pFrameWnd);
        CTabPrtView* pPrintView = pTabChldWnd->GetPrintView();
        if(pPrintView && bUpdatePrintView){
            CTabSet* pTabSet = pDoc->GetTableSpec();
            int iNumPrintTables = pTabSet->GetNumTables();
            for(int iIndex =0; iIndex < iNumPrintTables ; iIndex++){
                if(pTabSet->GetTable(iIndex) == table_element_tree_node->GetTable()){
                    pPrintView->GotoTbl(iIndex);
                    pView->GetGrid()->SetTable(table_element_tree_node->GetTable());
                    pView->GetGrid()->Update();
                    break;
                }
            }
        }
        else {
            pView->GetGrid()->SetTable(table_element_tree_node->GetTable());
            pView->UpdateAreaComboBox(true);
            pView->GetGrid()->Update();
        }
    }
}
/////////////////////////////////////////////////////////////////////////////////
//
//  void CTabTreeCtrl::OnLButtonDblClk(UINT nFlags, CPoint point)
//
/////////////////////////////////////////////////////////////////////////////////

void CTabTreeCtrl::OnLButtonDblClk(UINT nFlags, CPoint point)
{
#if 0
    HTREEITEM hItem = HitTest(point);
    if(hItem) {
        if (GetSelectedItem() == hItem) {
            SetFocus();
        }
        else {
            SelectItem(hItem);
        }
    }
    else {
        return;
    }

    TableElementTreeNode* table_element_tree_node = GetTreeNode(hItem);
    CTabulateDoc* pDoc = table_element_tree_node->GetTabDoc();

    POSITION pos = pDoc->GetFirstViewPosition();
    CTabView* pView = (CTabView*)pDoc->GetNextView(pos);
    pView->GetParentFrame()->ActivateFrame();
    CTable* pCurTable = pView->GetCurTable();
    if(table_element_tree_node->GetTable() && pCurTable != table_element_tree_node->GetTable()) {
        pView->GetGrid()->SetTable(table_element_tree_node->GetTable());
        pView->GetGrid()->Update();
    }
#endif

    CTreeCtrl::OnLButtonDblClk(nFlags, point);
}


/////////////////////////////////////////////////////////////////////////////////
//
//  void CTabTreeCtrl::OnAddTable()
//
/////////////////////////////////////////////////////////////////////////////////
void CTabTreeCtrl::OnAddTable()
{
    TableElementTreeNode* table_element_tree_node = GetTreeNode(GetSelectedItem());

    if(table_element_tree_node->GetTabDoc()) {
        POSITION pos = table_element_tree_node->GetTabDoc()->GetFirstViewPosition();
        CTabView* pTabView = nullptr;
        while(pos) {
            CView* pView = table_element_tree_node->GetTabDoc()->GetNextView(pos);
            if(pView->IsKindOf(RUNTIME_CLASS(CTabView))) {
                pTabView = (CTabView*)pView;
                break;
            }
        }

        if(pTabView) {
            pTabView->SendMessage(WM_COMMAND, ID_ADD_TABLE);
        }
    }
}


void CTabTreeCtrl::OnInsertTable()
{
    HTREEITEM origTI = GetSelectedItem();

    TableElementTreeNode* table_element_tree_node = GetTreeNode(origTI);
    if(table_element_tree_node->GetTabDoc()) {
        POSITION pos = table_element_tree_node->GetTabDoc()->GetFirstViewPosition();
        CTabView* pTabView = nullptr;
        while(pos) {
            CView* pView = table_element_tree_node->GetTabDoc()->GetNextView(pos);
            if(pView->IsKindOf(RUNTIME_CLASS(CTabView))) {
                pTabView = (CTabView*)pView;
                break;
            }
        }

        if(pTabView) {
            pTabView->SendMessage(WM_COMMAND, ID_INSERT_TABLE);
        }
    }
}


/////////////////////////////////////////////////////////////////////////////////
//
//  void CTabTreeCtrl::OnDeleteTable()
//
/////////////////////////////////////////////////////////////////////////////////
void CTabTreeCtrl::OnDeleteTable()
{
    HTREEITEM origTI = GetSelectedItem();

    TableElementTreeNode* table_element_tree_node = GetTreeNode(origTI);

    if(table_element_tree_node->GetTabDoc()) {
        POSITION pos = table_element_tree_node->GetTabDoc()->GetFirstViewPosition();
        CTabView* pTabView = nullptr;
        while(pos) {
            CView* pView = table_element_tree_node->GetTabDoc()->GetNextView(pos);
            if(pView->IsKindOf(RUNTIME_CLASS(CTabView))) {
                pTabView = (CTabView*)pView;
                break;
            }
        }

        if(pTabView) {
            pTabView->SendMessage(WM_COMMAND, ID_DELETE_TABLE);
        }
    }
}


void CTabTreeCtrl::OnEditVarTallyAttributes()
{
    HTREEITEM origTI = GetSelectedItem();

    TableElementTreeNode* table_element_tree_node = GetTreeNode(origTI);

    if (table_element_tree_node->GetTabVar()->GetName() == _T("")) {

        // it's not a dict item, but just the row/col header in the tree
    }
    else {  // *this* is a dict item, process it!

        if (table_element_tree_node->GetTabDoc()) {

            POSITION pos = table_element_tree_node->GetTabDoc()->GetFirstViewPosition();
            CTabView* pTabView = nullptr;
            while(pos) {
                CView* pView = table_element_tree_node->GetTabDoc()->GetNextView(pos);
                if(pView->IsKindOf(RUNTIME_CLASS(CTabView))) {
                   pTabView = (CTabView*)pView;
                   break;
                }
            }
            if(pTabView) {
                pTabView->SetCurVar(table_element_tree_node->GetTabVar());
                pTabView->SendMessage(WM_COMMAND, ID_EDIT_VAR_TALLYATTRIB);
            }
        }
    }
}


void CTabTreeCtrl::OnEditTableFmt()
{
    HTREEITEM origTI = GetSelectedItem();

    TableElementTreeNode* table_element_tree_node = GetTreeNode(origTI);

    if (table_element_tree_node->GetTabDoc()) {

        POSITION pos = table_element_tree_node->GetTabDoc()->GetFirstViewPosition();
        CTabView* pTabView = nullptr;
        while(pos) {
            CView* pView = table_element_tree_node->GetTabDoc()->GetNextView(pos);
            if(pView->IsKindOf(RUNTIME_CLASS(CTabView))) {
                pTabView = (CTabView*)pView;
                break;
            }
        }
        if(pTabView) {
            //pTabView->SetCurVar(table_element_tree_node->GetItem());
            pTabView->SendMessage(WM_COMMAND, ID_EDIT_TBL_FMT);
        }
    }
}


void CTabTreeCtrl::OnEditExcludeTbl()
{
    CFrameWnd* pFrameWnd =((CFrameWnd*)AfxGetMainWnd())->GetActiveFrame();
    if(pFrameWnd && pFrameWnd->IsKindOf(RUNTIME_CLASS(CTableChildWnd))){
        ((CTableChildWnd*)pFrameWnd)->SendMessage(WM_COMMAND, ID_EDIT_TABLE_EXCLUDE);
    }
}


void CTabTreeCtrl::OnEditExcludeAllButThis()
{
    CFrameWnd* pFrameWnd =((CFrameWnd*)AfxGetMainWnd())->GetActiveFrame();
    if(pFrameWnd && pFrameWnd->IsKindOf(RUNTIME_CLASS(CTableChildWnd))){
        ((CTableChildWnd*)pFrameWnd)->SendMessage(WM_COMMAND, ID_EDIT_EXCLUDEALLBUTTHIS);
    }
}


void CTabTreeCtrl::OnEditIncludeAllTablesInRun()
{
    CFrameWnd* pFrameWnd =((CFrameWnd*)AfxGetMainWnd())->GetActiveFrame();
    if(pFrameWnd && pFrameWnd->IsKindOf(RUNTIME_CLASS(CTableChildWnd))){
        ((CTableChildWnd*)pFrameWnd)->SendMessage(WM_COMMAND, ID_EDIT_INCLUDEALL);
    }
}


void CTabTreeCtrl::OnEditGeneratelogic()
{
    CFrameWnd* pFrameWnd =((CFrameWnd*)AfxGetMainWnd())->GetActiveFrame();
    if(pFrameWnd && pFrameWnd->IsKindOf(RUNTIME_CLASS(CTableChildWnd))){
        ((CTableChildWnd*)pFrameWnd)->SendMessage(WM_COMMAND, ID_EDIT_GENERATELOGIC);
    }
}


/////////////////////////////////////////////////////////////////////////////////
//
//  void CTabTreeCtrl::OnEditAppFmt()
//
/////////////////////////////////////////////////////////////////////////////////
void CTabTreeCtrl::OnEditAppFmt()
{
    HTREEITEM origTI = GetSelectedItem();

    TableElementTreeNode* table_element_tree_node = GetTreeNode(origTI);

    if (table_element_tree_node->GetTabDoc()) {

        POSITION pos = table_element_tree_node->GetTabDoc()->GetFirstViewPosition();
        CTabView* pTabView = nullptr;
        while(pos) {
            CView* pView = table_element_tree_node->GetTabDoc()->GetNextView(pos);
            if(pView->IsKindOf(RUNTIME_CLASS(CTabView))) {
                pTabView = (CTabView*)pView;
                break;
            }
        }
        if(pTabView) {
            //pTabView->SetCurVar(table_element_tree_node->GetItem());
            pTabView->SendMessage(WM_COMMAND, ID_EDIT_APP_FMT);
        }
    }

}


/////////////////////////////////////////////////////////////////////////////////
//
//  void CTabTreeCtrl::OnEditCopytablestructure()
//
/////////////////////////////////////////////////////////////////////////////////
void CTabTreeCtrl::OnEditCopytablestructure()
{
    //Get the document and currently selected table
    HTREEITEM hItem;
    hItem = GetSelectedItem();
    TableElementTreeNode* table_element_tree_node = GetTreeNode(hItem);
    CTabulateDoc* pDoc = table_element_tree_node->GetTabDoc();
    CTable* pTable = table_element_tree_node->GetTable();
    ASSERT(pTable);

    CSpecFile clipFile;
    if (pTable)
    {
        CString csClipFile = pDoc->GetClipFile();
        clipFile.Open(csClipFile, CFile::modeWrite);
        pDoc->GetTableSpec()->GetFmtReg().Save(clipFile);
        pTable->Save(clipFile);
        clipFile.Close();
        UINT uFormat = pDoc->GetClipBoardFormat(TD_TABLE_FORMAT);
        pDoc->FileToClip(uFormat);
        return;
    }
}


/////////////////////////////////////////////////////////////////////////////////
//
//  void CTabTreeCtrl::OnEditPastetable()
//
/////////////////////////////////////////////////////////////////////////////////
void CTabTreeCtrl::OnEditPastetable()
{
    HTREEITEM hItem;
    hItem = GetSelectedItem();
    TableElementTreeNode* table_element_tree_node = GetTreeNode(hItem);
    CTabulateDoc* pDoc = table_element_tree_node->GetTabDoc();
    POSITION pos = pDoc->GetFirstViewPosition();
    CTabView* pView = (CTabView*)pDoc->GetNextView(pos);

    pView->OnEditPastetable();
}

/////////////////////////////////////////////////////////////////////////////////
//
//  void CTabTreeCtrl::OnEditTablePrintFmt()
//
/////////////////////////////////////////////////////////////////////////////////
void CTabTreeCtrl::OnEditTablePrintFmt()
{
    HTREEITEM origTI = GetSelectedItem();

    TableElementTreeNode* table_element_tree_node = GetTreeNode(origTI);

    if (table_element_tree_node->GetTabDoc()) {

        POSITION pos = table_element_tree_node->GetTabDoc()->GetFirstViewPosition();
        CTabView* pTabView = nullptr;
        while(pos) {
            CView* pView = table_element_tree_node->GetTabDoc()->GetNextView(pos);
            if(pView->IsKindOf(RUNTIME_CLASS(CTabView))) {
                pTabView = (CTabView*)pView;
                break;
            }
        }
        if(pTabView) {
            //pTabView->SetCurVar(table_element_tree_node->GetItem());
            pTabView->SendMessage(WM_COMMAND, ID_EDIT_TBL_PRINTFMT);
        }
    }
}

/////////////////////////////////////////////////////////////////////////////////
//
//  void CTabTreeCtrl::OnEditGlobalProps()
//
/////////////////////////////////////////////////////////////////////////////////

void CTabTreeCtrl::OnEditGlobalProps()
{
    HTREEITEM origTI = GetSelectedItem();

    TableElementTreeNode* table_element_tree_node = GetTreeNode(origTI);

    if (table_element_tree_node->GetTabDoc()) {

        POSITION pos = table_element_tree_node->GetTabDoc()->GetFirstViewPosition();
        CTabView* pTabView = nullptr;

        while(pos) {
            CView* pView = table_element_tree_node->GetTabDoc()->GetNextView(pos);
            if(pView->IsKindOf(RUNTIME_CLASS(CTabView))) {
               pTabView = (CTabView*)pView;
               break;
            }
        }
    }
}


void CTabTreeCtrl::OnEditTableTallyAttributes()
{
    HTREEITEM origTI = GetSelectedItem();

    TableElementTreeNode* table_element_tree_node = GetTreeNode(origTI);

    if (table_element_tree_node->GetTabDoc()) {
        POSITION pos = table_element_tree_node->GetTabDoc()->GetFirstViewPosition();
        CTabView* pTabView = nullptr;
        while(pos) {
            CView* pView = table_element_tree_node->GetTabDoc()->GetNextView(pos);
            if(pView->IsKindOf(RUNTIME_CLASS(CTabView))) {
                pTabView = (CTabView*)pView;
                break;
            }
        }
        if(pTabView) {
            //pTabView->SetCurVar(table_element_tree_node->GetItem());
            pTabView->SendMessage(WM_COMMAND, ID_EDIT_TBL_TALLYATTRIB);
        }
    }

}
/////////////////////////////////////////////////////////////////////////////////
//
//  void CTabTreeCtrl::OnToggleTree()
//
/////////////////////////////////////////////////////////////////////////////////

void CTabTreeCtrl::OnToggleTree()
{
    m_bPrintTree = !m_bPrintTree;
    ReBuildTree(true);
}


int CTabTreeCtrl::GetCurLevel()
{
    //Get HItem for the current seleected item
    HTREEITEM hItem = GetSelectedItem();
    if( hItem == nullptr || m_bPrintTree ){
        return -1;
    }
    else {
        return GetTreeNode(hItem)->GetLevelNum();
    }
}



/////////////////////////////////////////////////////////////////////////////////
//
//  void CTabTreeCtrl::BuildTVTree(CTabulateDoc* pDoc)
//
/////////////////////////////////////////////////////////////////////////////////
void CTabTreeCtrl::BuildTVTree(CTabulateDoc* pDoc)
{
    if(!pDoc) {
        return;
    }

    //insert node for the document
    HTREEITEM hItem = InsertItem(pDoc->GetTableSpec()->GetLabel());

    TableSpecTabTreeNode* table_spec_tab_tree_node = new TableSpecTabTreeNode(pDoc, _T(""));
    table_spec_tab_tree_node->AddRef();
    table_spec_tab_tree_node->SetHItem(hItem);
    SetItemData(hItem, reinterpret_cast<DWORD_PTR>(table_spec_tab_tree_node));

    //build the tables  and its child nodes .
    TV_INSERTSTRUCT tvi;

    SetRedraw(FALSE);               // Don't draw while changing
    m_bOkToRedraw = false;
    m_bSendMsg = false;

    CTabSet* pSpec = pDoc->GetTableSpec();

    tvi.item.mask = TVIF_TEXT | TVIF_PARAM | TVIF_IMAGE | TVIF_SELECTEDIMAGE|TVIF_STATE;

    tvi.item.state = 0;
    tvi.item.stateMask = TVIS_BOLD;

    tvi.item.pszText = (LPTSTR)LPSTR_TEXTCALLBACK;

    HTREEITEM htiRoot = hItem;
    int iNumTbls = pSpec->GetNumTables();

    tvi.hParent = htiRoot;

    for (int iIndex = 0 ; iIndex < iNumTbls ; iIndex++) {
        pTableObject = pSpec->GetTable(iIndex);
        tvi.hParent = htiRoot;

        TableElementTreeNode* table_element_tree_node = new TableElementTreeNode(pDoc, pTableObject);
        tvi.item.lParam = reinterpret_cast<LPARAM>(table_element_tree_node);

        if (pTableObject->GetGenerateLogic()) {
            if (pTableObject->IsTableExcluded4Run()) {
                tvi.item.iImage = 11;
                tvi.item.iSelectedImage = 11;
            }
            else{
                tvi.item.iImage = 1;
                tvi.item.iSelectedImage = 1;
            }
        }
        else {
            if (pTableObject->IsTableExcluded4Run()) {
                tvi.item.iImage = 12;
                tvi.item.iSelectedImage = 12;
            }
            else{
                tvi.item.iImage = 2;
                tvi.item.iSelectedImage = 2;
            }
        }

        HTREEITEM htiLevel = InsertItem(&tvi);
        table_element_tree_node->SetHItem(htiLevel);
    }

//    DefaultExpand(htiRoot);  // gsf 01/30/01
    Expand(htiRoot,TVE_EXPAND);
    SetRedraw(TRUE);
    m_bOkToRedraw = true;
    m_bSendMsg = true;

    Invalidate();
    UpdateWindow();
}



void CTabTreeCtrl::OnSelchanged(NMHDR* pNMHDR, LRESULT* pResult)
{
    NM_TREEVIEW* pNMTreeView = reinterpret_cast<NM_TREEVIEW*>(pNMHDR);
    TableElementTreeNode* table_element_tree_node = reinterpret_cast<TableElementTreeNode*>(pNMTreeView->itemNew.lParam);

    if( table_element_tree_node == nullptr )
        return;

    CTabulateDoc* pTabDoc = table_element_tree_node->GetTabDoc();

    if (pTabDoc){
        BOOL bActivate = FALSE;
        //Activate the frame for the selected item if the
        //RHS is CTableChildWnd type;
        CFrameWnd* pFrameWnd =((CFrameWnd*)AfxGetMainWnd())->GetActiveFrame();
        if(pFrameWnd && pFrameWnd->IsKindOf(RUNTIME_CLASS(CTableChildWnd))){
            bActivate = TRUE;
        }

        POSITION pos = pTabDoc->GetFirstViewPosition();
        CView* pView = (CTabView*) pTabDoc->GetNextView(pos);

        if(!pView)
            return;

        CTableChildWnd* pFrame = (CTableChildWnd*)pView->GetParentFrame();

        if(!pFrame)
            return;

        if(pFrame->GetSourceView() && pFrame->GetSourceView()->IsWindowVisible())
        {
            if(bActivate){
                pFrame->ActivateFrame();
            }
            if(m_bSendMsg){
                AfxGetMainWnd()->SendMessage(UWM::Table::ShowSourceCode, 0, reinterpret_cast<LPARAM>(pTabDoc));
            }
        }

        // savy & gsf 04-apr-00
        if (pView && m_bOkToRedraw){
            if (pTabDoc != m_pDoc){
                POSITION pos2 = pTabDoc->GetFirstViewPosition();
                CTabView* pView2 = (CTabView*)pTabDoc->GetNextView(pos2);
                CTableChildWnd* pWnd = (CTableChildWnd*)pView2->GetParent();
                if(bActivate){
                    pWnd->ShowWindow(SW_SHOW);  //Show window activates and does it in the current size
                }
            }
//          pView->TreeSelectionChanged ( table_element_tree_node != nullptr );

            if (pTabDoc != m_pDoc)
            {
                SetTabDoc(pTabDoc);
            }
        }
    // moved 6 lines below----Chirag Problem seen in lot of versions...
        pos = pTabDoc->GetFirstViewPosition();
        ASSERT (pos != nullptr);
        CView* pViewtp = (CView*)pTabDoc->GetNextView(pos);
        UNREFERENCED_PARAMETER(pViewtp);
        CTSourceEditView* pView2 = DYNAMIC_DOWNCAST(CTSourceEditView, pTabDoc->GetNextView(pos));
        if (pView2 != nullptr) {
            pView2->GetEditCtrl()->EmptyUndoBuffer();
        }
    }

    UpdateTableOnSelect();
    SetFocus();

    *pResult = 0;
}


void CTabTreeCtrl::SelectNextTable()
{
    CFrameWnd* pFrameWnd =((CFrameWnd*)AfxGetMainWnd())->GetActiveFrame();
    CTableChildWnd* pTabChldWnd = DYNAMIC_DOWNCAST(CTableChildWnd,pFrameWnd);
    if(pTabChldWnd) {
        CTabulateDoc* pDoc = (CTabulateDoc*)pTabChldWnd->GetActiveDocument();
        if(pDoc) {
            POSITION pos = pDoc->GetFirstViewPosition();
            CTabView* pView = (CTabView*)pDoc->GetNextView(pos);
            CTable* pTable = pView->GetGrid()->GetTable();
            CTable* pNextTable = GetNextTable(pTable);
            CTabPrtView* pPrintView = pTabChldWnd->GetPrintView();
            if(pPrintView){
                CTabSet* pTabSet = pDoc->GetTableSpec();
                int iNumPrintTables = pTabSet->GetNumTables();
                for(int iIndex =0; iIndex < iNumPrintTables ; iIndex++){
                    if(pTabSet->GetTable(iIndex) == pNextTable){
                        pPrintView->GotoTbl(iIndex);
                        pPrintView->UpdateWindow();
                        break;
                    }
                }
            }
            if(pNextTable) {
                SelectTable(pNextTable);
            }
        }
    }
}

void CTabTreeCtrl::SelectPrevTable()
{
    CFrameWnd* pFrameWnd =((CFrameWnd*)AfxGetMainWnd())->GetActiveFrame();
    CTableChildWnd* pTabChldWnd = DYNAMIC_DOWNCAST(CTableChildWnd,pFrameWnd);
    if(pTabChldWnd) {
        CTabulateDoc* pDoc = (CTabulateDoc*)pTabChldWnd->GetActiveDocument();
        if(pDoc) {
            POSITION pos = pDoc->GetFirstViewPosition();
            CTabView* pView = (CTabView*)pDoc->GetNextView(pos);
            CTable* pTable = pView->GetGrid()->GetTable();
            CTable* pPrevTable = GetPrevTable(pTable);
            CTabPrtView* pPrintView = pTabChldWnd->GetPrintView();
            if(pPrintView){
                CTabSet* pTabSet = pDoc->GetTableSpec();
                int iNumPrintTables = pTabSet->GetNumTables();
                for(int iIndex =0; iIndex < iNumPrintTables ; iIndex++){
                    if(pTabSet->GetTable(iIndex) == pPrevTable){
                        pPrintView->GotoTbl(iIndex);
                        pPrintView->UpdateWindow();
                        break;
                    }
                }
            }
            if(pPrevTable) {
                SelectTable(pPrevTable);
            }
        }
    }
}

CTable* CTabTreeCtrl::GetNextTable(CTable* pTable /*= nullptr*/)
{
    CTable* pRetTable = nullptr;
    CFrameWnd* pFrameWnd =((CFrameWnd*)AfxGetMainWnd())->GetActiveFrame();
    CTableChildWnd* pTabChldWnd = DYNAMIC_DOWNCAST(CTableChildWnd,pFrameWnd);
    CTabulateDoc* pDoc = nullptr;
    if(pTabChldWnd) {
        pDoc = (CTabulateDoc*)pTabChldWnd->GetActiveDocument();
    }
    else {
        return pRetTable;
    }

    if(!pTable) {
        if(pDoc) {
            POSITION pos = pDoc->GetFirstViewPosition();
            CTabView* pView = (CTabView*)pDoc->GetNextView(pos);
            pTable = pView->GetGrid()->GetTable();
        }
    }
    if(pTable){
        int iNumTbls = pDoc->GetTableSpec()->GetNumTables();
        bool bFound = false;
        for(int iIndex = 0; iIndex <iNumTbls ; iIndex ++){
            if(bFound){
                pRetTable = pDoc->GetTableSpec()->GetTable(iIndex);
                break;
            }
            else if(pDoc->GetTableSpec()->GetTable(iIndex) == pTable){
                bFound = true;
                continue;
            }
        }
    }
    return pRetTable;
}

CTable* CTabTreeCtrl::GetPrevTable(CTable* pTable /*= nullptr*/)
{
    CTable* pRetTable = nullptr;
    CFrameWnd* pFrameWnd =((CFrameWnd*)AfxGetMainWnd())->GetActiveFrame();
    CTableChildWnd* pTabChldWnd = DYNAMIC_DOWNCAST(CTableChildWnd,pFrameWnd);
    CTabulateDoc* pDoc = nullptr;
    if(pTabChldWnd) {
        pDoc = (CTabulateDoc*)pTabChldWnd->GetActiveDocument();
    }
    else {
        return pRetTable;
    }

    if(!pTable) {
        if(pDoc) {
            POSITION pos = pDoc->GetFirstViewPosition();
            CTabView* pView = (CTabView*)pDoc->GetNextView(pos);
            pTable = pView->GetGrid()->GetTable();
        }
    }
    if(pTable){
        int iNumTbls = pDoc->GetTableSpec()->GetNumTables();
        bool bFound = false;
        for(int iIndex = iNumTbls-1; iIndex >=0 ; iIndex--){
            if(bFound){
                pRetTable = pDoc->GetTableSpec()->GetTable(iIndex);
                break;
            }
            else if(pDoc->GetTableSpec()->GetTable(iIndex) == pTable){
                bFound = true;
                continue;
            }
        }
    }
    return pRetTable;
}

void CTabTreeCtrl::SelectTable(CTable* pTable, bool bUpdatePrintView /*= true*/)
{
    CFrameWnd* pFrameWnd =((CFrameWnd*)AfxGetMainWnd())->GetActiveFrame();
    CTableChildWnd* pTabChldWnd = DYNAMIC_DOWNCAST(CTableChildWnd,pFrameWnd);
    CTabulateDoc* pDoc = nullptr;
    if(pTabChldWnd) {
        pDoc = (CTabulateDoc*)pTabChldWnd->GetActiveDocument();
    }
    else {
        return;
    }

    TableSpecTabTreeNode* table_spec_tab_tree_node = GetTableSpecTabTreeNode(*pDoc);

    if( table_spec_tab_tree_node == nullptr )
        return;

    HTREEITEM hItem = table_spec_tab_tree_node->GetHItem();
    HTREEITEM hChildItem = GetChildItem(hItem);
    if(IsPrintTree() && hChildItem){
        while (hChildItem){
            GetItemData(hChildItem);
            TableElementTreeNode* table_element_tree_node = GetTreeNode(hChildItem);
            if(table_element_tree_node != nullptr && table_element_tree_node->GetTable() == pTable) {
                SelectItem(hChildItem);
                UpdateTableOnSelect(bUpdatePrintView);
                break;
            }
            else {
                hChildItem = GetNextSiblingItem(hChildItem);
            }
        }
    }
    else if(hChildItem){ //this is the level tree
        HTREEITEM hLevelItem = hChildItem;
        while(hLevelItem) {
            hChildItem = GetChildItem(hLevelItem);
            while (hChildItem){
                GetItemData(hChildItem);
                TableElementTreeNode* table_element_tree_node = GetTreeNode(hChildItem);
                if(table_element_tree_node != nullptr && table_element_tree_node->GetTable() == pTable) {
                    SelectItem(hChildItem);
                    UpdateTableOnSelect(bUpdatePrintView);
                    break;
                }
                else {
                    hChildItem = GetNextSiblingItem(hChildItem);
                }
            }
            hLevelItem = GetNextSiblingItem(hLevelItem);
        }
    }
}


void CTabTreeCtrl::OnTvnBegindrag(NMHDR *pNMHDR, LRESULT *pResult)
{
    CPoint      ptAction;

    // This code is to prevent accidental drags.
    if( (GetTickCount() - m_dwDragStart) < DRAG_DELAY) {
        return;
    }
    m_hitemDrop = nullptr;
    m_hitemDrag = nullptr;
    ASSERT(!m_bDragging);
    GetCursorPos(&ptAction);
    ScreenToClient(&ptAction);
    NM_TREEVIEW* pNMTreeView = (NM_TREEVIEW*)pNMHDR;
    *pResult = 0;
    m_hitemDrag = pNMTreeView->itemNew.hItem;
    SelectItem(m_hitemDrag);
    if(m_hitemDrag) {
        TableElementTreeNode* table_element_tree_node = GetTreeNode(m_hitemDrag);
        if( table_element_tree_node != nullptr && table_element_tree_node->GetTableElementType() != TableElementType::Table){
            return;
        }
    }
    // Get image list for dragging
    //m_pDragImage = CreateDragImage(m_hitemDrag);
    m_pDragImage = new CImageList();
    m_pDragImage->Create(16, 16, 0, 0, 4); // x,y of icon, flags, initial sz, grow by
    m_pDragImage->SetBkColor(RGB(255,255,255));
    HICON icon0 = AfxGetApp()->LoadIcon(IDI_TABLE);
    m_pDragImage->Add(icon0);

    if( !m_pDragImage) {
        return;
    }
    m_bDragging = true;
    m_pDragImage->DragShowNolock(TRUE);
    m_pDragImage->SetDragCursorImage(0, CPoint(0, 0));
    m_pDragImage->BeginDrag(0, CPoint(0,0));
    CPoint temp = CPoint(15,0) + ptAction;
    m_pDragImage->DragEnter(GetDesktopWindow(), temp);
    SetCapture();
    if(!m_pLineWnd) {
        m_pLineWnd = new CWnd();
        int iID = 1024;
        RECT rect;
        rect.left = 0;
        rect.top =0;
        rect.bottom = 2;
        rect.right = 20;
        m_pLineWnd->Create(nullptr,_T(""),WS_VISIBLE|WS_BORDER|WS_CHILD,rect,this->GetParent(),iID);
    }

}

void CTabTreeCtrl::OnLButtonUp(UINT nFlags, CPoint point)
{
    if(m_pLineWnd && m_pLineWnd->GetSafeHwnd()){
        m_pLineWnd->DestroyWindow();
        delete m_pLineWnd;
        m_pLineWnd = nullptr;
    }
    if (m_bDragging) {
        CPoint tPoint = point;
        ShowCursor(TRUE);
        m_pDragImage->DragLeave(GetDesktopWindow());
        m_pDragImage->EndDrag();
        delete m_pDragImage;
        m_pDragImage = nullptr;

        ClientToScreen(&tPoint);
        CRect rect;
        GetWindowRect(&rect);
        if (!rect.PtInRect(tPoint) ){// am i dropping on myself?
            m_hitemDrag = nullptr;
            m_hitemDrop = nullptr;
            ReleaseCapture();
            m_bDragging = false;
            CTreeCtrl::OnLButtonUp(nFlags, point);            // mfc-gen
            return;
        }

        //Check if the drop is with in the main window
        HTREEITEM hItem = GetSelectedItem();
        if(hItem != nullptr ) {
            ReleaseCapture();
            m_bDragging = false;
            if(m_hitemDrop && m_hitemDrag != m_hitemDrop){
                TableElementTreeNode* source_table_element_tree_node = GetTreeNode(m_hitemDrag);
                TableElementTreeNode* target_table_element_tree_node = GetTreeNode(m_hitemDrop);

                CTabulateDoc* pDoc = source_table_element_tree_node->GetTabDoc();
                if(source_table_element_tree_node && target_table_element_tree_node && pDoc && IsPrintTree()) {
                    CTabSet* pTabSet = pDoc->GetTableSpec();

                    if(target_table_element_tree_node->GetTableElementType() == TableElementType::TableSpec){
                        if(source_table_element_tree_node->GetTable() != pTabSet->GetTable(0)){
                            pTabSet->MovePrintTable(source_table_element_tree_node->GetTable(),pTabSet->GetTable(0));
                            ReBuildTree(true);
                        }
                    }
                    else{
                        if(source_table_element_tree_node->GetTable() != target_table_element_tree_node->GetTable()){
                            pTabSet->MovePrintTable(source_table_element_tree_node->GetTable(),target_table_element_tree_node->GetTable(),true);
                            ReBuildTree(true);
                        }
                    }
                }
                else if(source_table_element_tree_node && target_table_element_tree_node && pDoc && !IsPrintTree()) {
                    CTabSet* pTabSet = pDoc->GetTableSpec();
                    //SAVY&&& Worry about levels when they come in
                    if(target_table_element_tree_node->GetTableElementType() == TableElementType::TableSpec){
                        pTabSet->MoveLevelTable(source_table_element_tree_node->GetTable(),pTabSet->GetTable(0));
                        ReBuildTree(true);
                    }
                    else{
                        pTabSet->MoveLevelTable(source_table_element_tree_node->GetTable(),target_table_element_tree_node->GetTable(),true);
                        ReBuildTree(true);
                    }
                }
            }
        }
        else {
            ReleaseCapture();
            m_bDragging = false;
            SelectItem(hItem);
        }
    }

    m_hitemDrag = nullptr;
    m_hitemDrop = nullptr;

    CTreeCtrl::OnLButtonUp(nFlags, point);            // mfc-gen
}


void CTabTreeCtrl::OnMouseMove(UINT nFlags, CPoint point)
{
    if (m_bDragging){
        CPoint ptScreen(point);
        ClientToScreen(&ptScreen);

        m_pDragImage->DragMove(ptScreen+CPoint(15,0));

        m_pDragImage->DragShowNolock(FALSE);

        HTREEITEM hItem = nullptr;
        if ((hItem = HitTest(point, &nFlags)) != nullptr){
            TableElementTreeNode* table_element_tree_node = GetTreeNode(hItem);
            if(table_element_tree_node != nullptr && ( table_element_tree_node->GetTableElementType() == TableElementType::TableSpec ||
                                                       table_element_tree_node->GetTableElementType() == TableElementType::Table ) ) {
                RECT rect;
                GetItemRect(hItem,&rect,TRUE);
                m_hitemDrop = hItem;

                if(m_pLineWnd && m_pLineWnd->GetSafeHwnd()){
                    ::MoveWindow(m_pLineWnd->GetSafeHwnd(),rect.left-12,rect.bottom+11,40,2,TRUE);
                    m_pLineWnd->ShowWindow(SW_SHOW);
                }
            }
            else{
                m_hitemDrop = nullptr;
                if(m_pLineWnd && m_pLineWnd->GetSafeHwnd()){
                    m_pLineWnd->ShowWindow(SW_HIDE);
                }
            }
        }
        m_pDragImage->DragShowNolock(TRUE);
        CRect rect;
        GetWindowRect(&rect);
        if(!rect.PtInRect(ptScreen)) {
            SetCursor(_hCursorNo);
        }
        else {
            SetCursor(_hCursorArrow);
        }

    }

    CTreeCtrl::OnMouseMove(nFlags, point);
}


BOOL CTabTreeCtrl::PreTranslateMessage(MSG* pMsg)
{
    switch(pMsg->message) {
    case WM_RBUTTONDOWN:
        if(m_bDragging) {
            m_hitemDrag = nullptr;
            m_hitemDrop = nullptr;
            m_pDragImage->DragLeave(GetDesktopWindow());
            m_pDragImage->EndDrag();
            delete m_pDragImage;
            m_pDragImage = nullptr;
            ReleaseCapture();
            m_bDragging = false;
            SelectDropTarget(nullptr);
            HTREEITEM hItem = GetSelectedItem();
            SelectItem(hItem);
            if(m_pLineWnd && m_pLineWnd->GetSafeHwnd()){
                m_pLineWnd->DestroyWindow();
                delete m_pLineWnd;
                m_pLineWnd = nullptr;
            }
            return FALSE;
        }
        return TRUE;
    }

    return CTreeCtrl::PreTranslateMessage(pMsg);
}


void CTabTreeCtrl::OnEditTabSetProp()
{
    CTabSet* pTabSet = m_pDoc->GetTableSpec();
    ASSERT (pTabSet != nullptr);
    CTabSetPropDlg dlg (pTabSet, this);
    if (dlg.DoModal() == IDOK) {
        pTabSet->SetLabel(dlg.m_sTSLabel);
        pTabSet->SetName(dlg.m_sTSName);
        m_pDoc->SetModifiedFlag(true);
        ReBuildTree();
    }
}


void CTabTreeCtrl::OnEditTableProp()
{
    ASSERT(m_pDoc->GetTableSpec() != nullptr);

    CTabPropDlg dlg(m_pDoc->GetTableSpec(), this);

    if( dlg.DoModal() != IDOK )
        return;

    TableElementTreeNode* table_element_tree_node = GetTreeNode(GetSelectedItem());

    TBL_PROC_INFO tblProcInfo;
    tblProcInfo.pTabDoc = m_pDoc;
    tblProcInfo.pTable = table_element_tree_node->GetTable();
    tblProcInfo.sTblLogic = table_element_tree_node->GetTable()->GetName();//Stores the old name
    tblProcInfo.eEventType = CSourceCode_AllEvents;
    tblProcInfo.pLinkTable = nullptr;
    tblProcInfo.bGetLinkTables = false;

    table_element_tree_node->GetTable()->SetName(dlg.m_sTabName);

    AfxGetMainWnd()->SendMessage(UWM::Table::RenameProc, 0, reinterpret_cast<LPARAM>(&tblProcInfo));
    m_pDoc->SetModifiedFlag(true);
    ReBuildTree();
}


void CTabTreeCtrl::OnKeyDown(UINT nChar, UINT nRepCnt, UINT nFlags)
{
    // TODO: Add your message handler code here and/or call default

    CTreeCtrl::OnKeyDown(nChar, nRepCnt, nFlags);
    switch (nChar)
    {
        case VK_DELETE:
        case VK_DOWN :
        case VK_UP:
            UpdateTableOnSelect();
            SetFocus();
            break;
        default:
             break;
    }
}

void CTabTreeCtrl::OnTvnSelchanging(NMHDR *pNMHDR, LRESULT *pResult)
{
    LPNMTREEVIEW pNMTreeView = reinterpret_cast<LPNMTREEVIEW>(pNMHDR);
    // TODO: Add your control notification handler code here
    *pResult = 0;
    if (!pNMTreeView->itemOld.hItem){
        return;
    }

    TableElementTreeNode* old_table_element_tree_node = reinterpret_cast<TableElementTreeNode*>(pNMTreeView->itemOld.lParam);

    CTabulateDoc* pTabDoc = old_table_element_tree_node->GetTabDoc();
    if(!pTabDoc)
        return;

    POSITION pos = pTabDoc->GetFirstViewPosition();
    ASSERT (pos != nullptr);
    CView* pViewtp = (CView*)pTabDoc->GetNextView(pos);
    UNREFERENCED_PARAMETER(pViewtp);
    CTSourceEditView* pView2 = DYNAMIC_DOWNCAST(CTSourceEditView, pTabDoc->GetNextView(pos));
    if (pView2 != nullptr) {
        if(AfxGetMainWnd()->SendMessage(UWM::Table::PutSourceCode, 0, reinterpret_cast<LPARAM>(old_table_element_tree_node)))
            return;
    }
    *pResult = 0;
}

/////////////////////////////////////////////////////////////////////////////
//
//                         CTabTreeCtrl::OnShiftF10
//
/////////////////////////////////////////////////////////////////////////////

void CTabTreeCtrl::OnShiftF10() {

    PostMessage(WM_RBUTTONUP);
}
