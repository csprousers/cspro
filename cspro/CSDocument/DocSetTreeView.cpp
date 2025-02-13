#include "StdAfx.h"
#include "DocSetTreeView.h"
#include "CSDocument.h"


IMPLEMENT_DYNCREATE(DocSetTreeView, CTreeView)

BEGIN_MESSAGE_MAP(DocSetTreeView, CTreeView)

    ON_WM_RBUTTONDOWN()
    ON_COMMAND(ID_SHIFT_F10, OnShiftF10)

    ON_NOTIFY_REFLECT(NM_DBLCLK, OnDoubleClickAndReturn)
    ON_NOTIFY_REFLECT(NM_RETURN, OnDoubleClickAndReturn)

    // context menu
    ON_COMMAND(ID_EDIT_COMPONENT, OnEditComponent)
    ON_UPDATE_COMMAND_UI(ID_EDIT_COMPONENT, OnUpdateComponentMustHavePath)

    ON_COMMAND(ID_COPY_FULL_PATH_FROM_TREE, OnCopyFullPath)
    ON_UPDATE_COMMAND_UI(ID_COPY_FULL_PATH_FROM_TREE, OnUpdateComponentMustHavePath)

    ON_COMMAND(ID_COPY_FILENAME_FROM_TREE, OnCopyFilename)
    ON_UPDATE_COMMAND_UI(ID_COPY_FILENAME_FROM_TREE, OnUpdateComponentMustHavePath)

    ON_COMMAND(ID_OPEN_CONTAINING_FOLDER_FROM_TREE, OnOpenContainingFolder)
    ON_UPDATE_COMMAND_UI(ID_OPEN_CONTAINING_FOLDER_FROM_TREE, OnUpdateComponentMustHavePath)

END_MESSAGE_MAP()


void DocSetTreeView::OnInitialUpdate()
{
    __super::OnInitialUpdate();

    // modify the style
    long style = GetWindowLong(m_hWnd, GWL_STYLE);
    style |= TVS_HASBUTTONS | TVS_HASLINES | TVS_LINESATROOT | TVS_SHOWSELALWAYS;
    SetWindowLong(m_hWnd, GWL_STYLE, style);

    // set the image list
    const std::initializer_list<std::tuple<ExtendedDocSetComponentType, UINT>> icon_resource_ids
    {
        { DocSetComponent::Type::Spec,            IDI_DOCSET },
        { DocSetComponent::Type::TableOfContents, IDI_TOC },
        { DocSetComponent::Type::Index,           IDI_INDEX },
        { DocSetComponent::Type::Settings,        IDI_SETTINGS },
        { DocSetComponent::Type::Definitions,     IDI_DEFINITIONS },
        { DocSetComponent::Type::ContextIds,      IDI_CONTEXT_IDS },
        { SpecialDocumentType { },                IDR_CSDOC_FRAME },
        { DocSetComponent::Type::Document,        IDR_CSDOC_FRAME },
    };

    if( m_imageList.Create(16, 16, ILC_COLOR32, 0, icon_resource_ids.size()) )
    {
        m_imageList.SetBkColor(GetSysColor(COLOR_WINDOW));

        for( const auto& [icon_type, resource_id] : icon_resource_ids )
        {
            HICON icon = AfxGetApp()->LoadIcon(resource_id);

            if( icon == nullptr )
            {
                ASSERT(false);
                continue;
            }

            m_iconMapping.try_emplace(icon_type, m_imageList.GetImageCount());
            m_imageList.Add(icon);
        }

        GetTreeCtrl().SetImageList(&m_imageList, TVSIL_NORMAL);
    }

    // build the initial tree
    BuildTree(GetInitialDataForTree());
}


DocSetTreeView::DataForTree::DataForTree(const DocSetSpec& doc_set_spec)
    :   title(doc_set_spec.GetTitle()),
        doc_set_components(doc_set_spec.GetComponents()),
        table_of_contents(doc_set_spec.GetTableOfContents()),
        cover_page_document(doc_set_spec.GetCoverPageDocument()),
        default_document(doc_set_spec.GetDefaultDocument())
{
}


DocSetTreeView::DataForTree DocSetTreeView::GetInitialDataForTree()
{
    // silently compile the spec, ignoring errors, in order to build the tree
    DocSetSpec& doc_set_spec = GetDocSetSpec();
    
    try
    {
        assert_cast<CMainFrame*>(AfxGetMainWnd())->CompileDocSetSpecIfNecessary(doc_set_spec, DocSetCompiler::SpecCompilationType::DataForTree);
    }
    catch(...) { }

    return DataForTree(doc_set_spec);
}


std::vector<std::tuple<DocSetTreeView::ExtendedDocSetComponentType, const DocSetComponent*>> DocSetTreeView::GetSortedDocSetComponents() const
{
    // lookup each document's position in the table of contents (when one exists)
    std::vector<std::tuple<ExtendedDocSetComponentType, const DocSetComponent*>> sorted_doc_set_components;
    sorted_doc_set_components.reserve(m_dataForCurrentTree.doc_set_components.size());

    const bool sort_by_toc = m_dataForCurrentTree.table_of_contents.has_value();
    std::unique_ptr<std::vector<std::tuple<const DocSetComponent*, size_t>>> documents_and_toc_indices;

    if( sort_by_toc )
    {
        documents_and_toc_indices = std::make_unique<std::vector<std::tuple<const DocSetComponent*, size_t>>>();
        documents_and_toc_indices->reserve(m_dataForCurrentTree.doc_set_components.size());
    }

    for( const DocSetComponent* doc_set_component : VI_P(m_dataForCurrentTree.doc_set_components) )
    {
        if( doc_set_component->type != DocSetComponent::Type::Document || !sort_by_toc )
        {
            sorted_doc_set_components.emplace_back(doc_set_component->type, doc_set_component);
        }

        else
        {
            documents_and_toc_indices->emplace_back(doc_set_component, m_dataForCurrentTree.table_of_contents->GetPosition(*doc_set_component));
        }
    }

    // sort by type order
    std::sort(sorted_doc_set_components.begin(), sorted_doc_set_components.end(),
        [&](const std::tuple<ExtendedDocSetComponentType, const DocSetComponent*>& lhs_type_and_doc_set_component,
            const std::tuple<ExtendedDocSetComponentType, const DocSetComponent*>& rhs_type_and_doc_set_component)
        {
            return ( std::get<DocSetComponent::Type>(std::get<0>(lhs_type_and_doc_set_component)) <
                     std::get<DocSetComponent::Type>(std::get<0>(rhs_type_and_doc_set_component)) );
        });

    // add any special documents before the documents
    auto add_special_document = [&](const TCHAR* type, const DocSetComponent* doc_set_component)
    {
        if( doc_set_component != nullptr )
        {
            auto documents_pos = std::find_if(sorted_doc_set_components.begin(), sorted_doc_set_components.end(),
                [&](const std::tuple<ExtendedDocSetComponentType, const DocSetComponent*>& type_and_doc_set_component)
                {
                    return ( std::get<0>(type_and_doc_set_component) == DocSetComponent::Type::Document );
                });

            sorted_doc_set_components.emplace(documents_pos, SpecialDocumentType { type }, doc_set_component);
        }
    };

    add_special_document(_T("Cover Page"), m_dataForCurrentTree.cover_page_document.get());
    add_special_document(_T("Default Document"), m_dataForCurrentTree.default_document.get());

    // sort the documents by the position in the table of contents and then add them to sorted contents
    if( sort_by_toc )
    {
        std::sort(documents_and_toc_indices->begin(), documents_and_toc_indices->end(),
            [&](const std::tuple<const DocSetComponent*, size_t>& lhs_doc_and_toc_index,
                const std::tuple<const DocSetComponent*, size_t>& rhs_doc_and_toc_index)
            {
                if( std::get<1>(lhs_doc_and_toc_index) < std::get<1>(rhs_doc_and_toc_index) )
                    return true;

                if( std::get<1>(lhs_doc_and_toc_index) > std::get<1>(rhs_doc_and_toc_index) )
                    return false;

                return ( SO::CompareNoCase(std::get<0>(lhs_doc_and_toc_index)->filename, std::get<0>(rhs_doc_and_toc_index)->filename) < 0 );
            });

        std::for_each(documents_and_toc_indices->begin(), documents_and_toc_indices->end(),
            [&](const std::tuple<const DocSetComponent*, size_t>& doc_and_toc_index)
            {
                sorted_doc_set_components.emplace_back(DocSetComponent::Type::Document, std::get<0>(doc_and_toc_index));
            });
    }

    ASSERT(sorted_doc_set_components.size() == ( m_dataForCurrentTree.doc_set_components.size() +
                                                 ( ( m_dataForCurrentTree.cover_page_document != nullptr ) ? 1 : 0 ) + 
                                                 ( ( m_dataForCurrentTree.default_document != nullptr ) ? 1 : 0 ) ));

    return sorted_doc_set_components;
}


void DocSetTreeView::BuildTree(DataForTree data_for_tree)
{
    CTreeCtrl& tree_ctrl = GetTreeCtrl();

    const DocSetSpec& doc_set_spec = GetDocSetSpec();
    const std::wstring& doc_set_spec_filename = doc_set_spec.GetFilename();

    SetRedraw(FALSE);
    tree_ctrl.DeleteAllItems();

    m_dataForCurrentTree = std::move(data_for_tree);
    ASSERT(!m_dataForCurrentTree.doc_set_components.empty());

    TV_INSERTSTRUCT tvi { };
    tvi.item.mask = TVIF_TEXT | TVIF_PARAM | TVIF_IMAGE | TVIF_SELECTEDIMAGE;
    tvi.hInsertAfter = TVI_LAST;

    auto insert_item = [&](HTREEITEM parent_tree_item, const ExtendedDocSetComponentType& extended_doc_set_component_type,
                           const DocSetComponent* doc_set_component, std::wstring description)
    {
        ASSERT(( parent_tree_item == TVI_ROOT ) == ( tree_ctrl.GetCount() == 0 ));

        tvi.hParent = parent_tree_item;
        tvi.item.pszText = description.data();
        tvi.item.lParam = reinterpret_cast<LPARAM>(doc_set_component);
        tvi.item.iImage = m_iconMapping[extended_doc_set_component_type];
        tvi.item.iSelectedImage = tvi.item.iImage;

        return tree_ctrl.InsertItem(&tvi);
    };

    HTREEITEM spec_tree_item = nullptr;
    std::vector<std::tuple<ExtendedDocSetComponentType, HTREEITEM>> grouped_components;

    // sort the components
    std::vector<std::tuple<ExtendedDocSetComponentType, const DocSetComponent*>> sorted_doc_set_components = GetSortedDocSetComponents();
    
    for( const auto& [extended_doc_set_component_type, doc_set_component] : sorted_doc_set_components )
    {
        // the main spec goes at the root
        if( extended_doc_set_component_type == DocSetComponent::Type::Spec )
        {
            ASSERT(tree_ctrl.GetCount() == 0);

            std::wstring description = ToString(std::get<DocSetComponent::Type>(extended_doc_set_component_type));

            if( m_dataForCurrentTree.title.has_value() )
                SO::AppendFormat(description, _T(" (%s)"), m_dataForCurrentTree.title->c_str());

            spec_tree_item = insert_item(TVI_ROOT, extended_doc_set_component_type, doc_set_component, std::move(description));
        }

        // there will be only one table of contents and index, so add them directly
        else if( extended_doc_set_component_type == DocSetComponent::Type::TableOfContents ||
                 extended_doc_set_component_type == DocSetComponent::Type::Index )
        {
            ASSERT(spec_tree_item != nullptr);
            insert_item(spec_tree_item, extended_doc_set_component_type, doc_set_component, ToString(std::get<DocSetComponent::Type>(extended_doc_set_component_type)));
        }

        // everything else will be added as groups
        else
        {
            ASSERT(spec_tree_item != nullptr);

            // add the group node when necessary
            if( grouped_components.empty() || extended_doc_set_component_type != std::get<0>(grouped_components.back()) )
            {
                const TCHAR* description =
                    ( std::holds_alternative<SpecialDocumentType>(extended_doc_set_component_type) )                        ? _T("Special Documents") :
                    ( std::get<DocSetComponent::Type>(extended_doc_set_component_type) == DocSetComponent::Type::Document ) ? _T("Documents") :
                                                                                                                              ToString(std::get<DocSetComponent::Type>(extended_doc_set_component_type));

                grouped_components.emplace_back(extended_doc_set_component_type, insert_item(spec_tree_item, extended_doc_set_component_type, nullptr, description));
            }

            // show the filename using relative pathing (as long as it is within 2 directories of the spec)
            const wstring_view PreviousDirectoryText_sv = _T("..\\");
            constexpr size_t MaxDirectoriesForRelativePath = 2;

            const std::wstring relative_filename = GetRelativeFNameForDisplay(doc_set_spec_filename, doc_set_component->filename);
            const size_t last_previous_directory_text = relative_filename.rfind(PreviousDirectoryText_sv);
            const bool use_relative_filename = ( last_previous_directory_text == std::wstring::npos ||
                                                 last_previous_directory_text < ( MaxDirectoriesForRelativePath * PreviousDirectoryText_sv.length() ) );

            std::wstring description = use_relative_filename ? relative_filename : doc_set_component->filename;

            if( std::holds_alternative<SpecialDocumentType>(extended_doc_set_component_type) )
                description = SO::CreateParentheticalExpression(std::get<SpecialDocumentType>(extended_doc_set_component_type).type, description);

            insert_item(std::get<1>(grouped_components.back()), extended_doc_set_component_type, doc_set_component, std::move(description));
        }
    }

    // by default expand all nodes
    tree_ctrl.Expand(spec_tree_item, TVE_EXPAND);

    for( const auto& [type, tree_item] : grouped_components )
        tree_ctrl.Expand(tree_item, TVE_EXPAND);

    SetRedraw(TRUE);
}


void DocSetTreeView::RebuildTreeIfNecessary()
{
    const DocSetSpec& doc_set_spec = GetDocSetSpec();

    // rebuild the tree if the title, or any components, have changed
    if( m_dataForCurrentTree.title != doc_set_spec.GetTitle() ||
        !VectorHelpers::ValueOfSharedPointersIsEqual(m_dataForCurrentTree.doc_set_components, doc_set_spec.GetComponents()) ||
        m_dataForCurrentTree.table_of_contents != doc_set_spec.GetTableOfContents() ||
        m_dataForCurrentTree.cover_page_document != doc_set_spec.GetCoverPageDocument() ||
        m_dataForCurrentTree.default_document != doc_set_spec.GetDefaultDocument() )
    {
        BuildTree(DataForTree(doc_set_spec));
    }
}


DocSetComponent* DocSetTreeView::GetDocSetComponent(HTREEITEM tree_item)
{
    return ( tree_item != nullptr ) ? reinterpret_cast<DocSetComponent*>(GetTreeCtrl().GetItemData(tree_item)) :
                                      nullptr;
}


template<typename CF>
void DocSetTreeView::DoWithSelectedDocSetComponent(CF callback_function)
{
    DocSetComponent* selected_doc_set_component = GetSelectedDocSetComponent();

    if( selected_doc_set_component != nullptr )
        callback_function(*selected_doc_set_component);
}


void DocSetTreeView::OnRButtonDown(UINT nFlags, CPoint point)
{
    __super::OnRButtonDown(nFlags, point);

    CTreeCtrl& tree_ctrl = GetTreeCtrl();
    HTREEITEM clicked_tree_item = tree_ctrl.HitTest(point);

    if( clicked_tree_item == nullptr )
        return;

    tree_ctrl.SelectItem(clicked_tree_item);

    ClientToScreen(&point);
    ShowContextMenu(point.x, point.y);
}


void DocSetTreeView::OnShiftF10()
{
    CTreeCtrl& tree_ctrl = GetTreeCtrl();
    HTREEITEM selected_tree_item = tree_ctrl.GetSelectedItem();

    if( selected_tree_item == nullptr )
        return;

    CRect rect;

    if( !tree_ctrl.GetItemRect(selected_tree_item, rect, TRUE) )
        return;

    ClientToScreen(rect);
    ShowContextMenu(rect.right, rect.bottom);
}


void DocSetTreeView::ShowContextMenu(int x, int y)
{
    CMenu menu;
    menu.LoadMenu(IDR_DOCSET_TREE_CONTEXT);
    ASSERT(menu.GetMenuItemCount() == 1);

    CMenu* submenu = menu.GetSubMenu(0);

    CMFCPopupMenu* popup_menu = new CMFCPopupMenu;
    popup_menu->SetAutoDestroy(FALSE);

    popup_menu->Create(this, x, y, submenu->GetSafeHmenu());
}


void DocSetTreeView::OnDoubleClickAndReturn(NMHDR* /*pNMHDR*/, LRESULT* pResult)
{
    CTreeCtrl& tree_ctrl = GetTreeCtrl();
    HTREEITEM selected_tree_item = tree_ctrl.GetSelectedItem();
    const DocSetComponent* selected_doc_set_component = GetDocSetComponent(selected_tree_item);

    // when a component is selected, edit it
    if( selected_doc_set_component != nullptr )
    {
        PostMessage(WM_COMMAND, ID_EDIT_COMPONENT);
        *pResult = 1;
    }

    // otherwise this is a group, so toggle the expanded status
    else if( selected_tree_item != nullptr )
    {
        tree_ctrl.Expand(selected_tree_item, TVE_TOGGLE);
        *pResult = 1;
    }

    else
    {
        *pResult = 0;
    }
}


void DocSetTreeView::OnUpdateComponentMustHavePath(CCmdUI* pCmdUI)
{
    BOOL enable = FALSE;
    DoWithSelectedDocSetComponent([&](const DocSetComponent& /*doc_set_component*/) { enable = TRUE; });
    pCmdUI->Enable(enable);
}


void DocSetTreeView::OnEditComponent()
{
    DoWithSelectedDocSetComponent([&](const DocSetComponent& doc_set_component)
    {
        // the spec is already open
        if( doc_set_component.type == DocSetComponent::Type::Spec )
            return;

        CSDocumentApp& csdoc_app = *assert_cast<CSDocumentApp*>(AfxGetApp());

        csdoc_app.SetDocSetParametersForNextOpen(doc_set_component, GetDocSetSpecDoc().GetSharedAssociatedDocSetSpec());
        csdoc_app.OpenDocumentFile(doc_set_component.filename.c_str(), FALSE);
    });
}


void DocSetTreeView::OnCopyFullPath()
{
    DoWithSelectedDocSetComponent([&](const DocSetComponent& doc_set_component)
    {
        WinClipboard::PutText(this, doc_set_component.filename);
    });
}


void DocSetTreeView::OnCopyFilename()
{
    DoWithSelectedDocSetComponent([&](const DocSetComponent& doc_set_component)
    {
        WinClipboard::PutText(this, PortableFunctions::PathGetFilename(doc_set_component.filename));
    });
}


void DocSetTreeView::OnOpenContainingFolder()
{
    DoWithSelectedDocSetComponent([&](const DocSetComponent& doc_set_component)
    {
        OpenContainingFolder(doc_set_component.filename);
    });
}


bool operator==(const DocSetTreeView::ExtendedDocSetComponentType& lhs, const DocSetTreeView::ExtendedDocSetComponentType& rhs)
{
    return ( lhs.index() != rhs.index() )                         ? false :
           ( std::holds_alternative<DocSetComponent::Type>(lhs) ) ? ( std::get<DocSetComponent::Type>(lhs) == std::get<DocSetComponent::Type>(rhs) ) :
                                                                    true;
}


bool operator!=(const DocSetTreeView::ExtendedDocSetComponentType& lhs, const DocSetTreeView::ExtendedDocSetComponentType& rhs)
{
    return !operator==(lhs, rhs);
}


bool operator<(const DocSetTreeView::ExtendedDocSetComponentType& lhs, const DocSetTreeView::ExtendedDocSetComponentType& rhs)
{
    if( lhs.index() == rhs.index() )
    {
        return std::holds_alternative<DocSetComponent::Type>(lhs) ? ( std::get<DocSetComponent::Type>(lhs) < std::get<DocSetComponent::Type>(rhs) ) :
                                                                    false;
    }

    // Special Documents will be sorted before Documents
    else if( std::holds_alternative<DocSetComponent::Type>(lhs) )
    {
        return ( std::get<DocSetComponent::Type>(lhs) < DocSetComponent::Type::Document );
    }

    else
    {
        return ( std::get<DocSetComponent::Type>(rhs) == DocSetComponent::Type::Document );
    }
}
