#include "StdAfx.h"
#include "DictionaryTreeCtrl.h"
#include "DictionaryTreeNode.h"


BEGIN_MESSAGE_MAP(DictionaryTreeCtrl, CTreeCtrl)
    ON_NOTIFY_REFLECT(TVN_GETDISPINFO, OnGetDisplayInfo)
    ON_NOTIFY_REFLECT(TVN_SELCHANGED, OnSelectionChanged)
    ON_NOTIFY_REFLECT(NM_DBLCLK, OnDoubleClick)
END_MESSAGE_MAP()


DictionaryTreeCtrl::DictionaryTreeCtrl()
    :   m_includeIds(true),
        m_includeValueSets(true),
        m_dictionaryTreeActionResponder(nullptr)
{
}

DictionaryTreeCtrl::~DictionaryTreeCtrl()
{
}


void DictionaryTreeCtrl::Initialize(std::shared_ptr<const CDataDict> dictionary, const bool include_ids, const bool include_value_sets,
                                    DictionaryTreeActionResponder* dictionary_tree_action_responder)
{
    m_dictionary = std::move(dictionary);
    m_includeIds = include_ids;
    m_includeValueSets = include_value_sets;
    m_dictionaryTreeActionResponder = dictionary_tree_action_responder;
    ASSERT(m_dictionaryTreeActionResponder != nullptr);

    // load the icons
    if( m_imageList == nullptr )
    {
        m_imageList = DictionaryTreeNode::CreateImageList();
        SetImageList(m_imageList.get(), TVSIL_NORMAL);        
    }

    BuildTree();
}


void DictionaryTreeCtrl::BuildTree()
{
    // disable redraws when building the tree
    SetRedraw(false);

    DeleteAllItems();

    if( m_dictionary != nullptr )
    {
        std::vector<HTREEITEM> tree_items_to_expand;

        TV_INSERTSTRUCT tvi { };
        tvi.item.mask = TVIF_TEXT | TVIF_PARAM | TVIF_IMAGE | TVIF_SELECTEDIMAGE;
        tvi.item.pszText = (LPTSTR)LPSTR_TEXTCALLBACK;
        tvi.hInsertAfter = TVI_LAST;

        auto insert_item = [&](HTREEITEM hParent, std::shared_ptr<DictionaryTreeNode> dictionary_tree_node)
        {
            m_dictionaryTreeNodes.emplace_back(dictionary_tree_node);

            tvi.hParent = hParent;
            tvi.item.lParam = reinterpret_cast<LPARAM>(dictionary_tree_node.get());
            tvi.item.iImage = dictionary_tree_node->GetImageListIndex();
            tvi.item.iSelectedImage = tvi.item.iImage;

            const HTREEITEM hItem = InsertItem(&tvi);

            if( dictionary_tree_node->GetDictElementType() == DictElementType::Dictionary ||
                dictionary_tree_node->GetDictElementType() == DictElementType::Level )
            {
                tree_items_to_expand.emplace_back(hItem);
            }

            return hItem;
        };

        const HTREEITEM hDictionaryItem = insert_item(TVI_ROOT, std::make_unique<DictionaryTreeNode>(*m_dictionary));

        for( const DictLevel& dict_level : m_dictionary->GetLevels() )
        {
            const HTREEITEM hLevelItem = insert_item(hDictionaryItem, std::make_unique<DictionaryTreeNode>(dict_level));

            for( int r = m_includeIds ? -1 : 0; r < dict_level.GetNumRecords(); ++r )
            {
                const bool id_record = ( r == -1 );
                const CDictRecord* record =  id_record ? dict_level.GetIdItemsRec() :
                                                         dict_level.GetRecord(r);

                const HTREEITEM hRecordItem = insert_item(hLevelItem, std::make_unique<DictionaryTreeNode>(*record, id_record));

                for( int i = 0; i < record->GetNumItems(); ++i )
                {
                    const CDictItem& dict_item = *record->GetItem(i);

                    if( !show_binary_items && !dict_item.AddToTreeFor80() )
                        continue;

                    std::optional<HTREEITEM> hFirstItem;

                    auto add_dictionary_item = [&](const std::optional<size_t> occurrence)
                    {
                        const HTREEITEM hItemItem = insert_item(hFirstItem.value_or(hRecordItem),
                                                                std::make_unique<DictionaryTreeNode>(dict_item, occurrence));

                        if( m_includeValueSets )
                        {
                            // only add value set entries when there are more than one, or if the label of
                            // the first value set doesn't match the item's label
                            if( ( dict_item.GetNumValueSets() >= 2 ) ||
                                ( dict_item.GetNumValueSets() == 1 && dict_item.GetLabel() != dict_item.GetValueSet(0).GetLabel() ) )
                            {
                                for( const DictValueSet& dict_value_set : dict_item.GetValueSets() )
                                    insert_item(hItemItem, std::make_unique<DictionaryTreeNode>(dict_value_set, dict_item, occurrence));
                            }
                        }

                        return hItemItem;
                    };

                    hFirstItem = add_dictionary_item(std::nullopt);

                    // when there are item/subitem occurrences, add an entry for each occurrence
                    if( dict_item.GetItemSubitemOccurs() > 1 )
                    {
                        for( unsigned occ = 0; occ < dict_item.GetItemSubitemOccurs(); ++occ )
                            add_dictionary_item(occ);
                    }                
                }
            }
        }

        // expand the dictionary root and levels
        for( const HTREEITEM& hItem : tree_items_to_expand )
            Expand(hItem, TVE_EXPAND);
    }

    SetRedraw(true);
    Invalidate();
}


void DictionaryTreeCtrl::OnGetDisplayInfo(NMHDR* pNMHDR, LRESULT* pResult)
{
    const TV_DISPINFO* pTVDispInfo = reinterpret_cast<const TV_DISPINFO*>(pNMHDR);
    const DictionaryTreeNode* dictionary_tree_node = reinterpret_cast<const DictionaryTreeNode*>(pTVDispInfo->item.lParam);

    if( dictionary_tree_node != nullptr )
    {
        _tcscpy_s(pTVDispInfo->item.pszText, pTVDispInfo->item.cchTextMax,
                  SharedSettings::ViewNamesInTree() ? dictionary_tree_node->GetName().c_str() : dictionary_tree_node->GetLabel().c_str());
    }

    *pResult = 0;
}


void DictionaryTreeCtrl::OnSelectionChanged(NMHDR* pNMHDR, LRESULT* pResult)
{    
    const NMTREEVIEWW* pnmtv = reinterpret_cast<const NMTREEVIEWW*>(pNMHDR);
    const DictionaryTreeNode* dictionary_tree_node = reinterpret_cast<const DictionaryTreeNode*>(pnmtv->itemNew.lParam);

    if( dictionary_tree_node != nullptr )
        m_dictionaryTreeActionResponder->OnDictionaryTreeSelectionChanged(*dictionary_tree_node);

    *pResult = 0;
}


void DictionaryTreeCtrl::OnDoubleClick(NMHDR* /*pNMHDR*/, LRESULT* pResult)
{
    const DictionaryTreeNode* dictionary_tree_node = reinterpret_cast<const DictionaryTreeNode*>(GetItemData(GetSelectedItem()));

    if( dictionary_tree_node != nullptr )
        m_dictionaryTreeActionResponder->OnDictionaryTreeDoubleClick(*dictionary_tree_node);

    *pResult = 0;
}
