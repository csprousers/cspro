#include "StdAfx.h"
#include "ObjTCtrl.h"
#include <zPackO/Packer.h>
#include <zPackO/PackSpec.h>


BEGIN_MESSAGE_MAP(CObjTreeCtrl, CTreeCtrl)
    ON_WM_RBUTTONDOWN()
    ON_NOTIFY_REFLECT(TVN_GETDISPINFO, OnGetDisplayInfo)
    ON_NOTIFY_REFLECT(TVN_DELETEITEM, OnDeleteItem)
    ON_NOTIFY_REFLECT(NM_DBLCLK, OnDoubleClick)
    ON_COMMAND(ID_FILE_CSPRO_CLOSE, OnClose)
    ON_COMMAND(ID_FILE_CSPRO_SAVE, OnSave)
    ON_COMMAND(ID_OBJDICTTYPE, OnDictType)
    ON_COMMAND(ID_COPY_FULL_PATH, OnCopyFullPath)
    ON_COMMAND(ID_OPEN_CONTAINING_FOLDER, OnOpenContainingFolder)
    ON_COMMAND(ID_TOOLS_PACK, OnPackApplication)
END_MESSAGE_MAP()


std::unique_ptr<LOGFONT> CObjTreeCtrl::m_defaultLogfont;
CFont CObjTreeCtrl::m_font;


CObjTreeCtrl::CObjTreeCtrl()
    :   m_pActiveObj(nullptr)
{
}


void CObjTreeCtrl::InitImageList()
{
    // Call this function only once to create the image list
    // 20120608 added ILC_COLOR32 for alex's new icons
    m_imageList.Create(16, 16, ILC_COLOR32, 0, 2); // 32, 32 for large icons

    m_imageList.SetBkColor(RGB(255,255,255));

    auto load_icon = [&](auto icon_resource_id, AppFileType app_file_type)
    {
        m_imageIndexMap[app_file_type] = m_imageList.GetImageCount();
        m_imageList.Add(AfxGetApp()->LoadIcon(icon_resource_id));
    };

    load_icon(IDI_BCH_FILE, AppFileType::ApplicationBatch);
    load_icon(IDI_ENT_FILE, AppFileType::ApplicationEntry);
    load_icon(IDI_XTB_FILE, AppFileType::ApplicationTabulation);
    load_icon(IDI_DCF_FILE, AppFileType::Dictionary);
    load_icon(IDI_FRM_FILE, AppFileType::Form);
    load_icon(IDI_LOGIC_FILE, AppFileType::Code);
    load_icon(IDI_MGF_FILE, AppFileType::Message);
    load_icon(IDI_ORD_FILE, AppFileType::Order);
    load_icon(IDI_QSF_FILE, AppFileType::QuestionText);
    load_icon(IDI_REPORT_FILE, AppFileType::Report);
    load_icon(IDI_RESOURCE_FOLDER, AppFileType::ResourceFolder);
    load_icon(IDI_XTS_FILE, AppFileType::TableSpec);

    SetImageList(&m_imageList, TVSIL_NORMAL);

    UpdateWindow();
}


CDocument* CObjTreeCtrl::GetSelectedDocument()
{
    FileTreeNode* file_tree_node = GetSelectedFileTreeNode();
    return ( file_tree_node != nullptr ) ? file_tree_node->GetDocument() : nullptr;
}


template<typename CF>
FileTreeNode* CObjTreeCtrl::FindNode(HTREEITEM hItem, CF callback_function) const
{
    while( hItem != nullptr )
    {
        FileTreeNode* file_tree_node = GetFileTreeNode(hItem);

        if( file_tree_node != nullptr && callback_function(*file_tree_node) )
            return file_tree_node;

        hItem = GetNextSiblingItem(hItem);
    }

    return nullptr;
}


FileTreeNode* CObjTreeCtrl::FindNode(const CDocument* document) const
{
    return FindNode(GetRootItem(),
        [&](const FileTreeNode& file_tree_node)
        {
            return ( document == file_tree_node.GetDocument() );
        });
}


FileTreeNode* CObjTreeCtrl::FindNode(wstring_view path) const
{
    return FindNode(GetRootItem(),
        [&](const FileTreeNode& file_tree_node)
        {
            return SO::EqualsNoCase(path, file_tree_node.GetPath());
        });
}


FileTreeNode* CObjTreeCtrl::FindChildNodeRecursive(const FileTreeNode& parent_file_tree_node, wstring_view path) const
{
    HTREEITEM hItem = GetChildItem(parent_file_tree_node.GetHItem());
    FileTreeNode* child_file_tree_node = nullptr;

    FileTreeNode* sibling_file_tree_node = FindNode(hItem,
        [&](const FileTreeNode& file_tree_node)
        {
            if( SO::EqualsNoCase(path, file_tree_node.GetPath()) )
                return true;

            child_file_tree_node = FindChildNodeRecursive(file_tree_node, path);

            if( child_file_tree_node != nullptr )
                return true;

            return false;
        });

    return ( child_file_tree_node != nullptr ) ? child_file_tree_node :
                                                 sibling_file_tree_node;
}


HTREEITEM CObjTreeCtrl::InsertNode(HTREEITEM hParentItem, std::unique_ptr<FileTreeNode> file_tree_node)
{
    ASSERT(file_tree_node != nullptr && file_tree_node->GetAppFileType().has_value());

    const auto& image_index_search = m_imageIndexMap.find(*file_tree_node->GetAppFileType());
    ASSERT(image_index_search != m_imageIndexMap.cend());

    TV_INSERTSTRUCT tvi;
    tvi.hParent = hParentItem;
    tvi.hInsertAfter = TVI_LAST;
    tvi.item.mask = TVIF_TEXT | TVIF_PARAM | TVIF_IMAGE | TVIF_SELECTEDIMAGE;
    tvi.item.pszText = (LPTSTR)LPSTR_TEXTCALLBACK;
    tvi.item.lParam = 0;
    tvi.item.iImage = image_index_search->second;
    tvi.item.iSelectedImage = image_index_search->second;

    HTREEITEM hItem = CTreeCtrl::InsertItem(&tvi);
    file_tree_node->SetHItem(hItem);

    SetItemData(hItem, reinterpret_cast<DWORD_PTR>(file_tree_node.get()));

    // keep a copy of this FileTreeNode object throughout the lifetime of the tree control
    m_fileTreeNodes.emplace_back(std::move(file_tree_node));

    return hItem;
}


HTREEITEM CObjTreeCtrl::InsertFormNode(HTREEITEM hParentItem, std::wstring form_filename, AppFileType app_file_type)
{
    CMDlgBar& dlgBar = assert_cast<CMainFrame*>(AfxGetMainWnd())->GetDlgBar();
    std::optional<std::wstring> dictionary_filename;

    if( app_file_type == AppFileType::Form )
    {
        CFormTreeCtrl& formTree = dlgBar.m_FormTree;
        CFormNodeID* pNode = formTree.GetFormNode(form_filename);

        if( pNode != nullptr && pNode->GetFormDoc() != nullptr )
            dictionary_filename = CS2WS(pNode->GetFormDoc()->GetFormFile().GetDictionaryFilename());
    }

    else
    {
        ASSERT(app_file_type == AppFileType::Order);
        COrderTreeCtrl& OrderTree = dlgBar.m_OrderTree;
        FormOrderAppTreeNode* form_order_app_tree_node = OrderTree.GetFormOrderAppTreeNode(form_filename);

        if( form_order_app_tree_node != nullptr && form_order_app_tree_node->GetDocument() != nullptr )
            dictionary_filename = CS2WS(form_order_app_tree_node->GetOrderDocument()->GetFormFile().GetDictionaryFilename());
    }

    // if the form file wasn't open, open the file to get the dictionary name
    if( !dictionary_filename.has_value() )
    {
        CSpecFile formSpec(TRUE); //Silently

        if( formSpec.Open(form_filename.c_str(), CFile::modeRead) )
        {
            std::vector<std::wstring> dictionary_filenames = GetFileNameArrayFromSpecFile(formSpec, CSPRO_DICTS);
            formSpec.Close();

            if( !dictionary_filenames.empty() )
                dictionary_filename = dictionary_filenames.front();
        }

        if( !dictionary_filename.has_value() )
            return nullptr;
    }

    // insert the form file
    HTREEITEM hFormItem = ( app_file_type == AppFileType::Form ) ? InsertNode(hParentItem, std::make_unique<FormFileTreeNode>(std::move(form_filename))) :
                                                                   InsertNode(hParentItem, std::make_unique<OrderFileTreeNode>(std::move(form_filename)));

    // insert the form file's dictionary
    InsertNode(hFormItem, std::make_unique<DictionaryFileTreeNode>(*dictionary_filename));

    return hFormItem;
}


HTREEITEM CObjTreeCtrl::InsertTableNode(HTREEITEM hParentItem, std::wstring tab_spec_filename)
{
    CMDlgBar& dlgBar = assert_cast<CMainFrame*>(AfxGetMainWnd())->GetDlgBar();
    CTabTreeCtrl& TabTree = dlgBar.m_TableTree;

    TableSpecTabTreeNode* table_spec_tab_tree_node = TabTree.GetTableSpecTabTreeNode(tab_spec_filename);
    std::vector<std::wstring> dictionary_filenames;

    if( table_spec_tab_tree_node != nullptr && table_spec_tab_tree_node->GetTabDoc() != nullptr )
    {
        //For now only one dict in the Table---> Savy &&&
        dictionary_filenames.emplace_back(CS2WS(table_spec_tab_tree_node->GetTabDoc()->GetTableSpec()->GetDictFile()));
    }

    else
    {
        CSpecFile tabSpec(TRUE);

        if( !tabSpec.Open(tab_spec_filename.c_str(), CFile::modeRead) )
            return nullptr;

        dictionary_filenames = GetFileNameArrayFromSpecFile(tabSpec, CSPRO_DICTS);
        tabSpec.Close();

        if( dictionary_filenames.empty() )
            return nullptr;
    }

    // insert table in the obj tree
    HTREEITEM hTableItem = InsertNode(hParentItem, std::make_unique<TableSpecFileTreeNode>(std::move(tab_spec_filename)));

    // insert the dictionaries of the table
    for( const std::wstring& dictionary_filename : dictionary_filenames )
        InsertNode(hTableItem, std::make_unique<DictionaryFileTreeNode>(dictionary_filename));

    return hTableItem;
}


void CObjTreeCtrl::DeleteNode(const FileTreeNode& tree_file_node)
{
    //SAVY :: The DeleteItem message does not get priority when exiting
    //and we get problems 'cos OnDeleteItem does not get called immediately
    //So WE do the delete here and set the item to nullptr .
    if( m_pActiveObj == &tree_file_node )
        m_pActiveObj = nullptr;

    DeleteItem(tree_file_node.GetHItem());
}


void CObjTreeCtrl::InitializeFont()
{
    if( m_defaultLogfont == nullptr )
    {
        m_defaultLogfont = std::make_unique<LOGFONT>();

        CFont* pFont = GetFont();
        pFont->GetLogFont(m_defaultLogfont.get());
    }

    std::wstring font_name = SO::Trim(GetDesignerFontName());

    if( font_name.empty() )
        font_name = m_defaultLogfont->lfFaceName;

    LONG height = static_cast<LONG>(m_defaultLogfont->lfHeight * GetDesignerFontZoomLevel() / 100.0);

    // if the font hasn't changed, we can quit out without setting it again
    if( m_font.GetSafeHandle() != nullptr )
    {
        LOGFONT logfont;
        m_font.GetLogFont(&logfont);

        if( logfont.lfHeight == height && SO::EqualsNoCase(font_name, logfont.lfFaceName) )
            return;
    }

    LOGFONT logfont = *m_defaultLogfont;
    logfont.lfHeight = height;
    lstrcpyn(logfont.lfFaceName, font_name.c_str(), LF_FACESIZE);

    m_font.DeleteObject();
    m_font.CreateFontIndirect(&logfont);
    SetFont(&m_font);
}


void CObjTreeCtrl::DefaultExpand(HTREEITEM hItem, bool initialize_font/* = true*/)
{
    if( initialize_font )
        InitializeFont();

    Expand(hItem, TVE_EXPAND);

    HTREEITEM hChild = GetChildItem(hItem);

    while( hChild != nullptr )
    {
        DefaultExpand(hChild, false);
        hChild = GetChildItem(hChild);
    }

    if( GetParentItem(hItem) != nullptr )
    {
        hItem = GetNextSiblingItem(hItem);

        if( hItem != nullptr )
            DefaultExpand(hItem, false);
    }
}


void CObjTreeCtrl::OnGetDisplayInfo(NMHDR* pNMHDR, LRESULT* pResult)
{
    TV_DISPINFO* pTVDispInfo = reinterpret_cast<TV_DISPINFO*>(pNMHDR);
    const FileTreeNode* file_tree_node = reinterpret_cast<const FileTreeNode*>(pTVDispInfo->item.lParam);

    if( file_tree_node != nullptr && !file_tree_node->GetPath().empty() )
    {
        const std::wstring& text = SharedSettings::ViewNamesInTree() ? file_tree_node->GetPath() :
                                                                       file_tree_node->GetLabel();

        lstrcpyn(pTVDispInfo->item.pszText, text.c_str(), pTVDispInfo->item.cchTextMax);
    }

    *pResult = 0;
}



void CObjTreeCtrl::OnDeleteItem(NMHDR* pNMHDR, LRESULT* pResult)
{
    NM_TREEVIEW* pNMTreeView = reinterpret_cast<NM_TREEVIEW*>(pNMHDR);
    HTREEITEM hItem = pNMTreeView->itemOld.hItem;
    const FileTreeNode* file_tree_node = reinterpret_cast<const FileTreeNode*>(GetItemData(hItem));

    if( m_pActiveObj == file_tree_node )
        m_pActiveObj = nullptr;

    *pResult = 0;
}


void CObjTreeCtrl::OnDoubleClick(NMHDR* /*pNMHDR*/, LRESULT* pResult)
{
    FileTreeNode* file_tree_node = GetSelectedFileTreeNode();

    if( file_tree_node == nullptr )
        return;

    //Get the handle to the Tree Control
    CMDlgBar& dlgBar = assert_cast<CMainFrame*>(AfxGetMainWnd())->GetDlgBar();

    if( file_tree_node->GetAppFileType() == AppFileType::Dictionary )
    {
        CDDTreeCtrl& dictTree = dlgBar.m_DictTree;
        DictionaryDictTreeNode* dictionary_dict_tree_node = dictTree.GetDictionaryTreeNode(file_tree_node->GetPath());
        CDDDoc* pDoc = ( dictionary_dict_tree_node != nullptr ) ? dictionary_dict_tree_node->GetDDDoc() : nullptr;

        if( pDoc != nullptr )
        {
            POSITION pos = pDoc->GetFirstViewPosition();
            ASSERT(pos != nullptr);
            CView* pView = pDoc->GetNextView(pos);
            CDictChildWnd* pWnd = assert_cast<CDictChildWnd*>(pView->GetParentFrame());
            pWnd->ActivateFrame();

        }

        else
        {
            dictTree.OpenDictionary(WS2CS(file_tree_node->GetPath()));
        }
    }

    else if( file_tree_node->GetAppFileType() == AppFileType::Form )
    {
        CFormTreeCtrl& formTree = dlgBar.m_FormTree;
        CFormNodeID* pID = formTree.GetFormNode(file_tree_node->GetPath());
        CFormDoc* pDoc = ( pID != nullptr ) ? pID->GetFormDoc() : nullptr;

        if( pDoc != nullptr )
        {
            POSITION pos = pDoc->GetFirstViewPosition();
            ASSERT(pos != nullptr);
            CView* pView = pDoc->GetNextView(pos);
            CFormChildWnd* pWnd = assert_cast<CFormChildWnd*>(pView->GetParentFrame());
            pWnd->ActivateFrame();

        }

        else
        {
            formTree.OpenFormFile(WS2CS(file_tree_node->GetPath()));
        }
    }

    else if( file_tree_node->GetAppFileType() == AppFileType::Order )
    {
        COrderTreeCtrl& orderTree = dlgBar.m_OrderTree;
        FormOrderAppTreeNode* form_order_app_tree_node = orderTree.GetFormOrderAppTreeNode(file_tree_node->GetPath());
        COrderDoc* pDoc = ( form_order_app_tree_node != nullptr ) ? form_order_app_tree_node->GetOrderDocument() : nullptr;

        if( pDoc != nullptr )
        {
            POSITION pos = pDoc->GetFirstViewPosition();
            ASSERT(pos != nullptr);
            CView* pView = pDoc->GetNextView(pos);
            COrderChildWnd* pWnd = assert_cast<COrderChildWnd*>(pView->GetParentFrame());
            pWnd->ActivateFrame();

        }

        else
        {
            orderTree.OpenOrderFile(WS2CS(file_tree_node->GetPath()));
        }
    }

    else if( file_tree_node->GetAppFileType() == AppFileType::TableSpec )
    {
        CTabTreeCtrl& TableTree = dlgBar.m_TableTree;
        TableSpecTabTreeNode* table_spec_tab_tree_node = TableTree.GetTableSpecTabTreeNode(file_tree_node->GetPath());

        if( table_spec_tab_tree_node != nullptr )
        {
            CTabulateDoc* pDoc = table_spec_tab_tree_node->GetTabDoc();

            if( pDoc != nullptr )
            {
                POSITION pos = pDoc->GetFirstViewPosition();
                ASSERT(pos != nullptr);
                CView* pView = pDoc->GetNextView(pos);
                CTableChildWnd* pWnd = assert_cast<CTableChildWnd*>(pView->GetParentFrame());
                pWnd->ActivateFrame();

            }

            else
            {
                TableTree.OpenTableFile(WS2CS(file_tree_node->GetPath()));
            }
        }
    }

    *pResult = 0;
}


void CObjTreeCtrl::OnRButtonDown(UINT nFlags, CPoint point)
{
    CTreeCtrl::OnRButtonDown(nFlags, point);

    HTREEITEM hItem = HitTest(point);

    if( hItem == nullptr )
        return;

    const FileTreeNode* file_tree_node = GetFileTreeNode(hItem);
    std::optional<AppFileType> app_file_type = ( file_tree_node != nullptr ) ? file_tree_node->GetAppFileType() : std::nullopt;

    SelectItem(hItem);

    BCMenu popup_menu;
    popup_menu.CreatePopupMenu();

    bool is_document = ( GetParentItem(hItem) == nullptr );
    popup_menu.AppendMenuItems(is_document, { { ID_FILE_CSPRO_CLOSE, _T("Close") },
                                              { ID_FILE_CSPRO_SAVE,  _T("&Save\tCtrl+S") } });

    popup_menu.AppendMenu(MF_SEPARATOR);
    popup_menu.AppendMenuItems(IsApplicationType(*app_file_type), { { ID_TOOLS_PACK, _T("Pack Application") } });

    popup_menu.AppendMenu(MF_SEPARATOR);
    bool is_dictionary = ( app_file_type == AppFileType::Dictionary && GetDictTypeState() );
    popup_menu.AppendMenuItems(is_dictionary, { { ID_OBJDICTTYPE, _T("Dictionary Type") } });

    popup_menu.AppendMenu(MF_SEPARATOR);
    popup_menu.AppendMenuItems({ { ID_COPY_FULL_PATH,         _T("Copy Full Path") },
                                 { ID_OPEN_CONTAINING_FOLDER, _T("Open Containing Folder") } });

    ClientToScreen(&point);
    popup_menu.TrackPopupMenu(TPM_LEFTALIGN | TPM_RIGHTBUTTON, point.x, point.y, this);
}


void CObjTreeCtrl::OnClose()
{
    CDocument* pDoc = GetSelectedDocument();

    if( pDoc != nullptr )
        assert_cast<CCSProApp*>(AfxGetApp())->CloseDocument(pDoc);
}


void CObjTreeCtrl::OnSave()
{
    CDocument* document = GetSelectedDocument();

    if( document != nullptr )
        document->OnSaveDocument(document->GetPathName());
}


void CObjTreeCtrl::OnDictType()
{
    AfxGetMainWnd()->SendMessage(WM_COMMAND, ID_DICTTYPE);
}


bool CObjTreeCtrl::GetDictTypeState()
{
    //if the currently selected item is not dictionary return
    const FileTreeNode* file_tree_node = GetSelectedFileTreeNode();

    if( file_tree_node == nullptr || file_tree_node->GetAppFileType() != AppFileType::Dictionary )
        return false;

    HTREEITEM hParentItem = GetParentItem(file_tree_node->GetHItem());

    if( hParentItem == nullptr )
    {
        return false; // means it is a stand alone item
    }

    const FileTreeNode* parent_file_tree_node = GetFileTreeNode(hParentItem);
    ASSERT(parent_file_tree_node->GetAppFileType().has_value());

    if( IsApplicationType(*parent_file_tree_node->GetAppFileType()) )
    {
        return true; // an external dictionary
    }

    //else the dict belongs to form/order
    HTREEITEM hRootItem = GetParentItem(hParentItem);

    if( hRootItem == nullptr )
    {
        return false; // then it is not an application
    }

    // Check if the parent item is a form / order
    if( parent_file_tree_node->GetAppFileType() == AppFileType::Form ||
        parent_file_tree_node->GetAppFileType() == AppFileType::Order )
    {
        return true;
    }

    return false;
}


bool CObjTreeCtrl::GetDictTypeArgs(CString& sAplFileName, CString& sDictFName, CString& sParentFName)
{
    const FileTreeNode* file_tree_node = GetSelectedFileTreeNode();

    if( file_tree_node == nullptr || file_tree_node->GetAppFileType() != AppFileType::Dictionary )
        return false;

    sDictFName = WS2CS(file_tree_node->GetPath());

    HTREEITEM hParent = GetParentItem(file_tree_node->GetHItem());
    if(!hParent)//means it is a stand alone item
        return false;

    FileTreeNode* parent_file_tree_node = GetFileTreeNode(hParent);
    ASSERT(parent_file_tree_node->GetAppFileType().has_value());

    if( IsApplicationType(*parent_file_tree_node->GetAppFileType()) )
    {
        sAplFileName = WS2CS(parent_file_tree_node->GetPath());
        return true; // an external dictionary
    }

    //else the dict belongs to form/order
    HTREEITEM hRoot = GetParentItem(hParent);
    if(!hRoot) //then it is not an application
        return false;

    //Check if the parent item is a form / order
    if( parent_file_tree_node->GetAppFileType() == AppFileType::Form ||
        parent_file_tree_node->GetAppFileType() == AppFileType::Order )
    {
        sParentFName = WS2CS(parent_file_tree_node->GetPath());

        FileTreeNode* pRootObjID = GetFileTreeNode(hRoot);
        sAplFileName = WS2CS(pRootObjID->GetPath());

        return true;
    }

    return false;
}



void CObjTreeCtrl::OnCopyFullPath()
{
    const FileTreeNode* file_tree_node = GetSelectedFileTreeNode();

    if( file_tree_node != nullptr )
        WinClipboard::PutText(this, file_tree_node->GetPath());
}


void CObjTreeCtrl::OnOpenContainingFolder()
{
    const FileTreeNode* file_tree_node = GetSelectedFileTreeNode();

    if( file_tree_node != nullptr )
        OpenContainingFolder(file_tree_node->GetPath());
}


void CObjTreeCtrl::OnPackApplication()
{
    const FileTreeNode* file_tree_node = GetSelectedFileTreeNode();

    if( file_tree_node == nullptr )
        return;

    try
    {
        PackSpec pack_spec;
        pack_spec.AddEntry(PackEntry::Create(file_tree_node->GetPath()));
        pack_spec.SetZipFilename(PortableFunctions::PathRemoveFileExtension(file_tree_node->GetPath()) + _T(".zip"));

        Packer().Run(nullptr, pack_spec);

        OpenContainingFolder(pack_spec.GetZipFilename().c_str());
    }

    catch( const CSProException& exception )
    {
        ErrorMessage::Display(_T("There was an error packing the application:\n\n") + exception.GetErrorMessage());
    }
}
