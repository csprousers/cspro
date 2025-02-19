#include "StdAfx.h"
#include "ObjTCtrl.h"
#include <zUtilO/TreeCtrlHelpers.h>
#include <zPackO/Packer.h>
#include <zPackO/PackSpec.h>


BEGIN_MESSAGE_MAP(CObjTreeCtrl, CTreeCtrl)
    ON_WM_RBUTTONDOWN()
    ON_NOTIFY_REFLECT(TVN_GETDISPINFO, OnGetDisplayInfo)
    ON_NOTIFY_REFLECT(TVN_DELETEITEM, OnDeleteItem)
    ON_NOTIFY_REFLECT(NM_DBLCLK, OnDoubleClick)
    ON_COMMAND(ID_FILE_CSPRO_CLOSE, OnClose)
    ON_COMMAND(ID_FILE_CSPRO_SAVE, OnSave)
    ON_COMMAND(ID_FILE_PROPERTIES, OnFileProperties)
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


std::unique_ptr<AppFileTypeImageList> CObjTreeCtrl::CreateAppFileTypeImageList()
{
    auto app_file_type_image_list = std::make_unique<AppFileTypeImageList>();

    // Call this function only once to create the image list
    // 20120608 added ILC_COLOR32 for alex's new icons
    app_file_type_image_list->Create(16, 16, ILC_COLOR32, 0, 2); // 32, 32 for large icons

    app_file_type_image_list->SetBkColor(RGB(255,255,255));

    constexpr std::tuple<unsigned, AppFileType> icons[] =
    {
        { IDI_BCH_FILE,         AppFileType::ApplicationBatch },
        { IDI_ENT_FILE,         AppFileType::ApplicationEntry },
        { IDI_XTB_FILE,         AppFileType::ApplicationTabulation },
        { IDI_DCF_FILE,         AppFileType::Dictionary },
        { IDI_FRM_FILE,         AppFileType::Form },
        { IDI_LOGIC_FILE,       AppFileType::Code },
        { IDI_MGF_FILE,         AppFileType::Message },
        { IDI_ORD_FILE,         AppFileType::Order },
        { IDI_QSF_FILE,         AppFileType::QuestionText },
        { IDI_REPORT_FILE,      AppFileType::Report },
        { IDI_RESOURCE_FOLDER,  AppFileType::Resource },
        { IDI_XTS_FILE,         AppFileType::TableSpec },
    };

    for( const auto& [icon_resource_id, app_file_type] : icons )
        app_file_type_image_list->AddIcon(icon_resource_id, app_file_type);

    return app_file_type_image_list;
}


void CObjTreeCtrl::InitImageList()
{
    ASSERT(m_appFileTypeImageList == nullptr);

    m_appFileTypeImageList = CreateAppFileTypeImageList();

    SetImageList(m_appFileTypeImageList.get(), TVSIL_NORMAL);

    UpdateWindow();
}


CDocument* CObjTreeCtrl::GetSelectedDocument()
{
    FileTreeNode* const file_tree_node = GetSelectedFileTreeNode();
    return ( file_tree_node != nullptr ) ? file_tree_node->GetDocument() :
                                           nullptr;
}


template<typename CF>
FileTreeNode* CObjTreeCtrl::FindNode(HTREEITEM hItem, const CF callback_function) const
{
    while( hItem != nullptr )
    {
        FileTreeNode* const file_tree_node = GetFileTreeNode(hItem);

        if( file_tree_node != nullptr && callback_function(*file_tree_node) )
            return file_tree_node;

        hItem = GetNextSiblingItem(hItem);
    }

    return nullptr;
}


FileTreeNode* CObjTreeCtrl::FindNode(const CDocument* const document) const
{
    return FindNode(GetRootItem(),
        [&](const FileTreeNode& file_tree_node)
        {
            return ( document == file_tree_node.GetDocument() );
        });
}


FileTreeNode* CObjTreeCtrl::FindNode(const std::string_view path_sv) const
{
    return FindNode(GetRootItem(),
        [&](const FileTreeNode& file_tree_node)
        {
            return SO::EqualsNoCase(path_sv, file_tree_node.GetPath());
        });
}


FileTreeNode* CObjTreeCtrl::FindChildNodeRecursive(const FileTreeNode& parent_file_tree_node, const std::string_view path_sv) const
{
    const HTREEITEM hItem = GetChildItem(parent_file_tree_node.GetHItem());
    FileTreeNode* child_file_tree_node = nullptr;

    FileTreeNode* const sibling_file_tree_node = FindNode(hItem,
        [&](const FileTreeNode& file_tree_node)
        {
            if( SO::EqualsNoCase(path_sv, file_tree_node.GetPath()) )
                return true;

            child_file_tree_node = FindChildNodeRecursive(file_tree_node, path_sv);

            if( child_file_tree_node != nullptr )
                return true;

            return false;
        });

    return ( child_file_tree_node != nullptr ) ? child_file_tree_node :
                                                 sibling_file_tree_node;
}


HTREEITEM CObjTreeCtrl::InsertNode(const HTREEITEM hParentItem, std::unique_ptr<FileTreeNode> file_tree_node, const HTREEITEM hInsertAfter/* = TVI_LAST*/)
{
    ASSERT(file_tree_node != nullptr && file_tree_node->GetAppFileType().has_value());

    ASSERT(m_appFileTypeImageList != nullptr);
    const int image_index = m_appFileTypeImageList->GetImageIndex(*file_tree_node->GetAppFileType(), file_tree_node->GetPath());

    TV_INSERTSTRUCT tvi;
    tvi.hParent = hParentItem;
    tvi.hInsertAfter = hInsertAfter;
    tvi.item.mask = TVIF_TEXT | TVIF_PARAM | TVIF_IMAGE | TVIF_SELECTEDIMAGE;
    tvi.item.pszText = (LPTSTR)LPSTR_TEXTCALLBACK;
    tvi.item.lParam = 0;
    tvi.item.iImage = image_index;
    tvi.item.iSelectedImage = image_index;

    const HTREEITEM hItem = CTreeCtrl::InsertItem(&tvi);
    file_tree_node->SetHItem(hItem);

    SetItemData(hItem, reinterpret_cast<DWORD_PTR>(file_tree_node.get()));

    // keep a copy of this FileTreeNode object throughout the lifetime of the tree control
    m_fileTreeNodes.emplace_back(std::move(file_tree_node));

    return hItem;
}


HTREEITEM CObjTreeCtrl::InsertNode(const FileTreeNode& application_file_tree_node, std::unique_ptr<FileTreeNode> file_tree_node)
{
    ASSERT(application_file_tree_node.GetAppFileType().has_value() && IsApplicationType(*application_file_tree_node.GetAppFileType()));
    ASSERT(file_tree_node != nullptr && file_tree_node->GetAppFileType().has_value());

    const int app_file_type_order = GetTreeOrder(*file_tree_node->GetAppFileType());
    const HTREEITEM hApplicationRootItem = application_file_tree_node.GetHItem();
    std::optional<HTREEITEM> hInsertAfter;
    HTREEITEM hLastItem = hApplicationRootItem;

    // search siblings of the root's first child for the correct location based on the tree order
    TreeCtrlHelpers::FindInTree(*this, GetChildItem(hApplicationRootItem), false,
        [&](const HTREEITEM hItem)
        {
            const FileTreeNode* const file_tree_node = GetFileTreeNode(hItem);

            if( file_tree_node != nullptr )
            {
                const std::optional<AppFileType> app_file_type = file_tree_node->GetAppFileType();

                if( app_file_type.has_value() )
                {
                    const int this_app_file_type_order = GetTreeOrder(*app_file_type);

                    if( app_file_type_order == this_app_file_type_order )
                    {
                        hInsertAfter = file_tree_node->GetHItem();
                    }

                    // we can stop processing when we reach nodes that have a higher order
                    else if( app_file_type_order < this_app_file_type_order )
                    {
                        if( !hInsertAfter.has_value() )
                            hInsertAfter = hLastItem;

                        return true;
                    }
                }

                hLastItem = file_tree_node->GetHItem();
            }

            return false;
        });

    return InsertNode(hApplicationRootItem, std::move(file_tree_node), hInsertAfter.value_or(TVI_LAST));
}


HTREEITEM CObjTreeCtrl::InsertFormNode(const std::variant<HTREEITEM, const FileTreeNode*> hParentItem_or_application_file_tree_node,
                                       std::string form_file_path, const AppFileType app_file_type)
{
    CMDlgBar& dlgBar = assert_cast<CMainFrame*>(AfxGetMainWnd())->GetDlgBar();
    std::optional<std::string> dictionary_file_path;

    if( app_file_type == AppFileType::Form )
    {
        CFormTreeCtrl& formTree = dlgBar.m_FormTree;
        CFormNodeID* const pNode = formTree.GetFormNode(form_file_path);

        if( pNode != nullptr && pNode->GetFormDoc() != nullptr )
            dictionary_file_path = UTF8_TODO::GetUtf8(pNode->GetFormDoc()->GetFormFile().GetDictionaryFilename());
    }

    else
    {
        ASSERT(app_file_type == AppFileType::Order);
        COrderTreeCtrl& OrderTree = dlgBar.m_OrderTree;
        FormOrderAppTreeNode* const form_order_app_tree_node = OrderTree.GetFormOrderAppTreeNode(form_file_path);

        if( form_order_app_tree_node != nullptr && form_order_app_tree_node->GetDocument() != nullptr )
            dictionary_file_path = UTF8_TODO::GetUtf8(form_order_app_tree_node->GetOrderDocument()->GetFormFile().GetDictionaryFilename());
    }

    // if the form file wasn't open, open the file to get the dictionary name
    if( !dictionary_file_path.has_value() )
    {
        CSpecFile formSpec(TRUE); //Silently

        if( formSpec.Open(UTF8_TODO::GetCString(form_file_path), CFile::modeRead) )
        {
            std::vector<std::string> dictionary_file_paths = GetFileNameArrayFromSpecFile(formSpec, CSPRO_DICTS);
            formSpec.Close();

            if( !dictionary_file_paths.empty() )
                dictionary_file_path = std::move(dictionary_file_paths.front());
        }

        if( !dictionary_file_path.has_value() )
            return nullptr;
    }

    // insert the form file
    auto file_tree_node = ( app_file_type == AppFileType::Form ) ? std::unique_ptr<FileTreeNode>(std::make_unique<FormFileTreeNode>(std::move(form_file_path))) :
                                                                   std::unique_ptr<FileTreeNode>(std::make_unique<OrderFileTreeNode>(std::move(form_file_path)));

    const HTREEITEM hFormItem = std::holds_alternative<HTREEITEM>(hParentItem_or_application_file_tree_node) ?
        InsertNode(std::get<HTREEITEM>(hParentItem_or_application_file_tree_node), std::move(file_tree_node)) :
        InsertNode(*std::get<const FileTreeNode*>(hParentItem_or_application_file_tree_node), std::move(file_tree_node));

    // insert the form file's dictionary
    InsertNode(hFormItem, std::make_unique<DictionaryFileTreeNode>(std::move(*dictionary_file_path)));

    return hFormItem;
}


HTREEITEM CObjTreeCtrl::InsertTableNode(const HTREEITEM hParentItem, std::string table_spec_file_path)
{
    CMDlgBar& dlgBar = assert_cast<CMainFrame*>(AfxGetMainWnd())->GetDlgBar();
    CTabTreeCtrl& TabTree = dlgBar.m_TableTree;

    TableSpecTabTreeNode* const table_spec_tab_tree_node = TabTree.GetTableSpecTabTreeNode(table_spec_file_path);
    std::vector<std::string> dictionary_file_paths;

    if( table_spec_tab_tree_node != nullptr && table_spec_tab_tree_node->GetTabDoc() != nullptr )
    {
        //For now only one dict in the Table---> Savy &&&
        dictionary_file_paths.emplace_back(UTF8_TODO::GetUtf8(table_spec_tab_tree_node->GetTabDoc()->GetTableSpec()->GetDictFile()));
    }

    else
    {
        CSpecFile tabSpec(TRUE);

        if( !tabSpec.Open(UTF8_TODO::GetCString(table_spec_file_path), CFile::modeRead) )
            return nullptr;

        dictionary_file_paths = GetFileNameArrayFromSpecFile(tabSpec, CSPRO_DICTS);
        tabSpec.Close();

        if( dictionary_file_paths.empty() )
            return nullptr;
    }

    // insert table in the obj tree
    HTREEITEM hTableItem = InsertNode(hParentItem, std::make_unique<TableSpecFileTreeNode>(std::move(table_spec_file_path)));

    // insert the dictionaries of the table
    for( std::string& dictionary_file_path : dictionary_file_paths )
        InsertNode(hTableItem, std::make_unique<DictionaryFileTreeNode>(std::move(dictionary_file_path)));

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

        CFont* const pFont = GetFont();
        pFont->GetLogFont(m_defaultLogfont.get());
    }

    CString font_name = GetDesignerFontName();
    font_name.Trim();

    if( font_name.IsEmpty() )
        font_name = m_defaultLogfont->lfFaceName;

    const LONG height = static_cast<LONG>(m_defaultLogfont->lfHeight * GetDesignerFontZoomLevel() / 100.0);

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
    lstrcpyn(logfont.lfFaceName, font_name.GetString(), LF_FACESIZE);

    m_font.DeleteObject();
    m_font.CreateFontIndirect(&logfont);
    SetFont(&m_font);
}


void CObjTreeCtrl::DefaultExpand(HTREEITEM hItem, const bool initialize_font/* = true*/)
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


void CObjTreeCtrl::OnGetDisplayInfo(NMHDR* const pNMHDR, LRESULT* const pResult)
{
    TV_DISPINFO* const pTVDispInfo = reinterpret_cast<TV_DISPINFO*>(pNMHDR);
    const FileTreeNode* const file_tree_node = reinterpret_cast<const FileTreeNode*>(pTVDispInfo->item.lParam);

    if( file_tree_node != nullptr && !file_tree_node->GetPath().empty() )
    {
        const std::wstring& text = SharedSettings::ViewNamesInTree() ? UTF8_TODO::GetWide(file_tree_node->GetPath()) :
                                                                       file_tree_node->GetLabel();

        lstrcpyn(pTVDispInfo->item.pszText, text.c_str(), pTVDispInfo->item.cchTextMax);
    }

    *pResult = 0;
}



void CObjTreeCtrl::OnDeleteItem(NMHDR* const pNMHDR, LRESULT* const pResult)
{
    NM_TREEVIEW* const pNMTreeView = reinterpret_cast<NM_TREEVIEW*>(pNMHDR);
    const HTREEITEM hItem = pNMTreeView->itemOld.hItem;
    const FileTreeNode* const file_tree_node = reinterpret_cast<const FileTreeNode*>(GetItemData(hItem));

    if( m_pActiveObj == file_tree_node )
        m_pActiveObj = nullptr;

    *pResult = 0;
}


void CObjTreeCtrl::OnDoubleClick(NMHDR* /*pNMHDR*/, LRESULT* const pResult)
{
    FileTreeNode* const file_tree_node = GetSelectedFileTreeNode();

    if( file_tree_node == nullptr )
        return;

    //Get the handle to the Tree Control
    CMDlgBar& dlgBar = assert_cast<CMainFrame*>(AfxGetMainWnd())->GetDlgBar();

    if( file_tree_node->GetAppFileType() == AppFileType::Dictionary )
    {
        CDDTreeCtrl& dictTree = dlgBar.m_DictTree;
        DictionaryDictTreeNode* const dictionary_dict_tree_node = dictTree.GetDictionaryTreeNode(file_tree_node->GetPath());
        CDDDoc* const pDoc = ( dictionary_dict_tree_node != nullptr ) ? dictionary_dict_tree_node->GetDDDoc() :
                                                                        nullptr;

        if( pDoc != nullptr )
        {
            POSITION pos = pDoc->GetFirstViewPosition();
            ASSERT(pos != nullptr);
            CView* const pView = pDoc->GetNextView(pos);
            CDictChildWnd* const pWnd = assert_cast<CDictChildWnd*>(pView->GetParentFrame());
            pWnd->ActivateFrame();

        }

        else
        {
            dictTree.OpenDictionary(file_tree_node->GetPath());
        }
    }

    else if( file_tree_node->GetAppFileType() == AppFileType::Form )
    {
        CFormTreeCtrl& formTree = dlgBar.m_FormTree;
        CFormNodeID* const pID = formTree.GetFormNode(file_tree_node->GetPath());
        CFormDoc* const pDoc = ( pID != nullptr ) ? pID->GetFormDoc() :
                                                    nullptr;

        if( pDoc != nullptr )
        {
            POSITION pos = pDoc->GetFirstViewPosition();
            ASSERT(pos != nullptr);
            CView* const pView = pDoc->GetNextView(pos);
            CFormChildWnd* const pWnd = assert_cast<CFormChildWnd*>(pView->GetParentFrame());
            pWnd->ActivateFrame();

        }

        else
        {
            formTree.OpenFormFile(file_tree_node->GetPath());
        }
    }

    else if( file_tree_node->GetAppFileType() == AppFileType::Order )
    {
        COrderTreeCtrl& orderTree = dlgBar.m_OrderTree;
        FormOrderAppTreeNode* const form_order_app_tree_node = orderTree.GetFormOrderAppTreeNode(file_tree_node->GetPath());
        COrderDoc* const pDoc = ( form_order_app_tree_node != nullptr ) ? form_order_app_tree_node->GetOrderDocument() :
                                                                          nullptr;

        if( pDoc != nullptr )
        {
            POSITION pos = pDoc->GetFirstViewPosition();
            ASSERT(pos != nullptr);
            CView* const pView = pDoc->GetNextView(pos);
            COrderChildWnd* const pWnd = assert_cast<COrderChildWnd*>(pView->GetParentFrame());
            pWnd->ActivateFrame();

        }

        else
        {
            orderTree.OpenOrderFile(file_tree_node->GetPath());
        }
    }

    else if( file_tree_node->GetAppFileType() == AppFileType::TableSpec )
    {
        CTabTreeCtrl& TableTree = dlgBar.m_TableTree;
        TableSpecTabTreeNode* const table_spec_tab_tree_node = TableTree.GetTableSpecTabTreeNode(file_tree_node->GetPath());

        if( table_spec_tab_tree_node != nullptr )
        {
            CTabulateDoc* const pDoc = table_spec_tab_tree_node->GetTabDoc();

            if( pDoc != nullptr )
            {
                POSITION pos = pDoc->GetFirstViewPosition();
                ASSERT(pos != nullptr);
                CView* const pView = pDoc->GetNextView(pos);
                CTableChildWnd* const pWnd = assert_cast<CTableChildWnd*>(pView->GetParentFrame());
                pWnd->ActivateFrame();

            }

            else
            {
                TableTree.OpenTableFile(file_tree_node->GetPath());
            }
        }
    }

    *pResult = 0;
}


void CObjTreeCtrl::OnRButtonDown(const UINT nFlags, CPoint point)
{
    CTreeCtrl::OnRButtonDown(nFlags, point);

    const HTREEITEM hItem = HitTest(point);

    if( hItem == nullptr )
        return;

    const FileTreeNode* const file_tree_node = GetFileTreeNode(hItem);
    const std::optional<AppFileType> app_file_type = ( file_tree_node != nullptr ) ? file_tree_node->GetAppFileType() :
                                                                                     std::nullopt;

    SelectItem(hItem);

    BCMenu popup_menu;
    popup_menu.CreatePopupMenu();

    const bool is_document = ( GetParentItem(hItem) == nullptr );
    popup_menu.AppendMenuItems(is_document, { { ID_FILE_CSPRO_CLOSE, L"Close" },
                                              { ID_FILE_CSPRO_SAVE,  L"&Save\tCtrl+S" } });

    if( app_file_type.has_value() )
    {
        const bool is_part_of_application = ( GetApplicationFileTreeNode(file_tree_node) != nullptr );
        popup_menu.AppendMenu(MF_SEPARATOR);
        popup_menu.AppendMenuItems(is_part_of_application, { { ID_FILE_PROPERTIES, L"&Properties" } });

        popup_menu.AppendMenu(MF_SEPARATOR);
        popup_menu.AppendMenuItems(IsApplicationType(*app_file_type), { { ID_TOOLS_PACK, L"Pack Application" } });
    }

    popup_menu.AppendMenu(MF_SEPARATOR);
    popup_menu.AppendMenuItems({ { ID_COPY_FULL_PATH,         L"Copy Full Path" },
                                 { ID_OPEN_CONTAINING_FOLDER, L"Open Containing Folder" } });

    ClientToScreen(&point);
    popup_menu.TrackPopupMenu(TPM_LEFTALIGN | TPM_RIGHTBUTTON, point.x, point.y, this);
}


void CObjTreeCtrl::OnClose()
{
    CDocument* const pDoc = GetSelectedDocument();

    if( pDoc != nullptr )
        assert_cast<CCSProApp*>(AfxGetApp())->CloseDocument(pDoc);
}


void CObjTreeCtrl::OnFileProperties()
{
    const FileTreeNode* const file_tree_node = GetSelectedFileTreeNode();
    const FileTreeNode* const application_file_tree_node = GetApplicationFileTreeNode(file_tree_node);

    if( application_file_tree_node == nullptr )
    {
        ASSERT(false);
        return;
    }

    WindowsDesktopMessage::PostObject(UWM::Designer::ShowFileProperties, file_tree_node->GetPath(),
                                                                         application_file_tree_node->GetDocument());
}


void CObjTreeCtrl::OnSave()
{
    CDocument* const document = GetSelectedDocument();

    if( document != nullptr )
        document->OnSaveDocument(document->GetPathName());
}


const FileTreeNode* CObjTreeCtrl::GetApplicationFileTreeNode(const FileTreeNode* file_tree_node)
{
    // application-related files are all added under the application, so we just need
    // to keep visiting parent nodes until we reach an application
    while( file_tree_node != nullptr )
    {
        std::optional<AppFileType> app_file_type = file_tree_node->GetAppFileType();

        if( app_file_type.has_value() && IsApplicationType(*app_file_type) )
            return file_tree_node;

        file_tree_node = GetFileTreeNode(GetParentItem(file_tree_node->GetHItem()));
    }

    return nullptr;
}


void CObjTreeCtrl::OnCopyFullPath()
{
    const FileTreeNode* const file_tree_node = GetSelectedFileTreeNode();

    if( file_tree_node != nullptr )
        WinClipboard::PutText(this, file_tree_node->GetPath());
}


void CObjTreeCtrl::OnOpenContainingFolder()
{
    const FileTreeNode* const file_tree_node = GetSelectedFileTreeNode();

    if( file_tree_node != nullptr )
        OpenContainingFolder(file_tree_node->GetPath());
}


void CObjTreeCtrl::OnPackApplication()
{
    const FileTreeNode* const file_tree_node = GetSelectedFileTreeNode();

    if( file_tree_node == nullptr )
        return;

    try
    {
        PackSpec pack_spec;
        pack_spec.AddEntry(PackEntry::Create(file_tree_node->GetPath()));
        pack_spec.SetZipFilePath(PortableFunctions::PathReplaceFileExtension(file_tree_node->GetPath(), FileExtensions::Zip));

        Packer().Run(nullptr, pack_spec);

        OpenContainingFolder(pack_spec.GetZipFilePath());
    }

    catch( const CSProException& exception )
    {
        ErrorMessage::Display("There was an error packing the application:\n\n" + std::string(exception.what()));
    }
}
