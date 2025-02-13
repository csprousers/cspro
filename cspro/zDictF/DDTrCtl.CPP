//***************************************************************************
//  File name: DDTrCtl.cpp
//
//  Description:
//       Data Dictionary tree control implementation
//
//  History:    Date       Author   Comment
//              ---------------------------
//              03 Aug 00   bmd     Created for CSPro 2.1
//
//***************************************************************************

#include "StdAfx.h"
#include "DDTrCtl.H"
#include "CapiLDlg.h"
#include "DictionaryMacros.h"
#include <zToolsO/SharedSettings.h>
#include <zUtilO/CSProExecutables.h>
#include <zAppO/PFF.h>
#include <zInterfaceF/resource_shared.h>


BEGIN_MESSAGE_MAP(CDDTreeCtrl, CTreeCtrl)
    ON_NOTIFY_REFLECT(NM_SETFOCUS, OnSetfocus)
    ON_NOTIFY_REFLECT(TVN_SELCHANGED, OnSelchanged)
    ON_NOTIFY_REFLECT(TVN_GETDISPINFO, OnGetDisplayInfo)
    ON_NOTIFY_REFLECT(NM_DBLCLK, OnDTreeDblclk)
    ON_NOTIFY_REFLECT(TVN_ITEMEXPANDING, OnItemexpanding)
    ON_NOTIFY_REFLECT(TVN_DELETEITEM, OnDeleteitem)
    ON_NOTIFY_REFLECT(TVN_BEGINDRAG, OnBegindrag)
    ON_WM_LBUTTONUP()
    ON_WM_LBUTTONDOWN()
    ON_WM_MOUSEMOVE()
    ON_WM_RBUTTONUP()
    ON_COMMAND(ID_EDIT_ADD, OnEditAdd)
    ON_COMMAND(ID_EDIT_DELETE, OnEditDelete)
    ON_COMMAND(ID_EDIT_INSERT, OnEditInsert)
    ON_COMMAND(ID_EDIT_MODIFY, OnEditModify)
    ON_COMMAND(ID_EDIT_ADD_NEXT, OnEditAddNext)
    ON_COMMAND(ID_EDIT_RELATION, OnEditRelation)
    ON_COMMAND(ID_COPY_DICT_NAME, OnCopyName)
    ON_COMMAND(ID_VIEW_QUESTIONNAIRE, OnViewQuestionnaire)
    ON_COMMAND(ID_VIEW_DICTIONARY, OnViewDictionary)
    ON_COMMAND(ID_DICTIONARY_MACROS, OnDictionaryMacros)
    ON_COMMAND(ID_SHIFT_F10, OnShiftF10)
    ON_COMMAND(ID_EDIT_LANGUAGES, OnEditLanguages)
    ON_COMMAND_RANGE(ID_OPEN_WITH_CSDIFF, ID_OPEN_WITH_CSSORT, OnOpenTool)
    ON_COMMAND_RANGE(ID_OPEN_DATA_FILE0, ID_OPEN_DATA_FILE9, OnOpenDataFile)
END_MESSAGE_MAP()


CDDTreeCtrl::CDDTreeCtrl()
{
    m_pDoc = nullptr;
    m_bDragging = false;
    m_dwDragStart = 0;
    m_bUpdateAllViews = true;
    m_bFromTree = false;
}


void CDDTreeCtrl::InitImageList()
{
    // Call this function only once to create the image list;
    // the function also registers the drop target

    // 20120608 added ILC_COLOR32 for alex's new icons
    m_imageList.Create(16, 16, ILC_COLOR32, 0, 2); // 32, 32 for large icons

    m_imageList.SetBkColor(GetSysColor(COLOR_WINDOW));

    // If icons are added or deleted, change #defines in DDTrCtl.h
    auto load_icon = [&](auto resource_id, auto index)
    {
        ASSERT(index == m_imageList.GetImageCount());
        HICON icon = AfxGetApp()->LoadIcon(resource_id);
        m_imageList.Add(icon);
    };

    load_icon(IDI_DICTIONARY, DictIcon::Root);
    load_icon(IDI_DICTIONARY_LEVEL, DictIcon::Level);
    load_icon(IDI_DICTIONARY_ID_RECORD, DictIcon::Common);
    load_icon(IDI_DICTIONARY_RECORD, DictIcon::Record);
    load_icon(IDI_DICTIONARY_ITEM, DictIcon::Item);
    load_icon(IDI_ITEM_USED, DictIcon::ItemUsed);
    load_icon(IDI_DICTIONARY_SUBITEM, DictIcon::Subitem);
    load_icon(IDI_SUBITEM_USED, DictIcon::SubitemUsed);
    load_icon(IDI_DICTIONARY_VALUESET, DictIcon::ValueSet);
    load_icon(IDI_REL, DictIcon::Relation);

    SetImageList(&m_imageList, TVSIL_NORMAL);

    m_dropTarget.Register(this);
}


void CDDTreeCtrl::OnSetfocus(NMHDR* /*pNMHDR*/, LRESULT* pResult)
{
    *pResult = 0;
}


void CDDTreeCtrl::OnSelchanged(NMHDR* pNMHDR, LRESULT* pResult)
{
    NM_TREEVIEW* pNMTreeView = reinterpret_cast<NM_TREEVIEW*>(pNMHDR);  // Get tree control structure
    LPARAM lParam = pNMTreeView->itemNew.lParam;                        // Get new state of tree control
    DictTreeNode* dict_tree_node = reinterpret_cast<DictTreeNode*>(lParam);
    CDDDoc* pDoc = dict_tree_node->GetDDDoc();
    if (pDoc == nullptr) {
        return;
    }

    // Bring Dictionary Frame to top
    CMDIChildWnd* pActiveWnd = assert_cast<CMDIFrameWnd*>(AfxGetMainWnd())->MDIGetActive();
    if (pActiveWnd == nullptr) {
        return;
    }
    if (pActiveWnd->IsKindOf(RUNTIME_CLASS(CDictChildWnd))) {
        CDDDoc* pChildDoc = assert_cast<CDDDoc*>(pActiveWnd->GetActiveDocument());
        ASSERT(pChildDoc != nullptr);
        if (pChildDoc != pDoc) {
            POSITION pos = pDoc->GetFirstViewPosition();
            ASSERT(pos != nullptr);
            CDDGView* pView = assert_cast<CDDGView*>(pDoc->GetNextView(pos));
            CFrameWnd* pFrame = pView->GetParentFrame();
            pFrame->ActivateFrame(); //Do not call Activate frame   Changed 20 Jun 2001 BMD
//            pFrame->ShowWindow(SW_SHOW);    //Do this instead     SMG bug

            // Set the active splitter window
            CSplitterWnd* pSplitWnd = assert_cast<CSplitterWnd*>(pView->GetParent());
            pSplitWnd->SetActivePane(0,0);
        }
    }

    if(pDoc) {
        if (dict_tree_node->GetLevelIndex() >= 0) {
            pDoc->SetLevel(dict_tree_node->GetLevelIndex());
        }
        else {
            pDoc->SetLevel(0);
            pDoc->SetRec(0);
            pDoc->SetItem(NONE);
        }
        if (dict_tree_node->GetRecordIndex() >= 0) {
            pDoc->SetRec(dict_tree_node->GetRecordIndex());
        }
        else {
            pDoc->SetRec(0);
            pDoc->SetItem(NONE);
        }
        if (dict_tree_node->GetItemIndex() >= 0) {
            pDoc->SetItem(dict_tree_node->GetItemIndex());
        }
        else {
            pDoc->SetItem(0);
        }
        if (m_bUpdateAllViews || pNMTreeView->action != TVC_UNKNOWN) {
            pDoc->UpdateAllViews(nullptr, Hint::DictionaryTreeSelectionChanged, dict_tree_node);
        }
        else {
            UpdateWindow();
        }
    }
    if (pActiveWnd) {
        CDictChildWnd* pDictChildWnd = dynamic_cast<CDictChildWnd*> (pActiveWnd);
        if (pDictChildWnd) {
            QuestionnaireView* pQuestionnaireView = pDictChildWnd->GetQuestionnaireView();
            if (pQuestionnaireView && pQuestionnaireView->IsWindowVisible()) {
                pDictChildWnd->SetActiveView(pQuestionnaireView);
                pQuestionnaireView->SendMessage(UWM::Designer::TreeSelectionChanged,
                    (WPARAM)(dict_tree_node->GetName().c_str()));
            }
        }
        else if(pActiveWnd->GetActiveView()) {
            pActiveWnd->GetActiveView()->SendMessage(UWM::Designer::TreeSelectionChanged,
                (WPARAM)(dict_tree_node->GetName().c_str()));
        }
    }
    SetFocus();
    *pResult = 0;
}


void CDDTreeCtrl::OnGetDisplayInfo(NMHDR* pNMHDR, LRESULT* pResult)
{
    TV_DISPINFO* pTVDispInfo = reinterpret_cast<TV_DISPINFO*>(pNMHDR); // Get pointer to display information
    LPARAM lParam = pTVDispInfo->item.lParam;

    const DictTreeNode* dict_tree_node = reinterpret_cast<const DictTreeNode*>(lParam); // Get position in the data dictionary

    bool view_name = SharedSettings::ViewNamesInTree();
    std::wstring display_text = view_name ? dict_tree_node->GetName() : dict_tree_node->GetLabel();

    // add occurrence details when applicable
    if( dict_tree_node->GetItemOccurs() != NONE )
    {
        ASSERT(dict_tree_node->GetDictElementType() == DictElementType::Item ||
               dict_tree_node->GetDictElementType() == DictElementType::ValueSet);

        SO::AppendFormat(display_text, _T("(%d)"), dict_tree_node->GetItemOccurs() + 1);
    }

    if( view_name && SharedSettings::AppendLabelsToNamesInTree() )
        SO::Append(display_text, _T(": "), dict_tree_node->GetLabel());

    lstrcpyn(pTVDispInfo->item.pszText, display_text.c_str(), pTVDispInfo->item.cchTextMax);

    *pResult = 0;
}


void CDDTreeCtrl::OnDTreeDblclk(NMHDR* /*pNMHDR*/, LRESULT* pResult)
{
    // Get document associated with selected item
    HTREEITEM hItem = GetSelectedItem();
    LPARAM lParam = GetItemData(hItem);
    DictTreeNode* dict_tree_node = reinterpret_cast<DictTreeNode*>(lParam);
    CDDDoc* pDoc = dict_tree_node->GetDDDoc();

    if(!pDoc) {
        *pResult = 0;
        return;
    }

    //If the view is not activated, activate it
    POSITION pos = pDoc->GetFirstViewPosition();
    ASSERT(pos != nullptr);
    CDDGView* pView = assert_cast<CDDGView*>(pDoc->GetNextView(pos));
    CFrameWnd* pFrame = pView->GetParentFrame();
    pFrame->ActivateFrame();

    // Set the active splitter window
    CSplitterWnd* pSplitWnd = assert_cast<CSplitterWnd*>(pView->GetParent());
    pSplitWnd->SetActivePane(0,0);

    if (dict_tree_node->GetDictElementType() == DictElementType::Relation) {
        pDoc->OnEditRelation();
    }
    else {
        pView->GetCurrentGrid().PostMessage(WM_SETFOCUS);
    }

    *pResult = 1;
}


bool CDDTreeCtrl::OpenDictionary(const CString& dictionary_filename, bool bMakeVisible /* =true*/)
{
    //  Open and read the dictionary and build document
    ASSERT(m_pDocTemplate != nullptr);
    CDDDoc* pDoc = assert_nullable_cast<CDDDoc*>(m_pDocTemplate->OpenDocumentFile(dictionary_filename, bMakeVisible));

    if( pDoc == nullptr )
        return false;

    pDoc->SetDictTreeCtrl(this);

     //  Fill in the tree entry for the dicitonary
    DictionaryDictTreeNode* dictionary_dict_tree_node = GetDictionaryTreeNode(dictionary_filename);
    ASSERT(dictionary_dict_tree_node != nullptr);
    dictionary_dict_tree_node->SetDDDoc(pDoc);

    BuildTree(*dictionary_dict_tree_node);

    if(!bMakeVisible) {
        SelectItem(dictionary_dict_tree_node->GetHItem());        // BMD 31 Oct 2001
        POSITION pos = pDoc->GetFirstViewPosition();
        CDDGView* pView = assert_cast<CDDGView*>(pDoc->GetNextView(pos));
        pView->GetParentFrame()->ShowWindow(SW_SHOWNOACTIVATE);

        pView->OnInitialUpdate();
        pView->m_gridDict.RedrawAll();
    }

    return true;
}


HTREEITEM CDDTreeCtrl::InsertDictionary(const CString& sLabel, const CString& sDictFileName, CDDDoc* pDoc)
{
    m_bUpdateAllViews = true;
    //Insert the dictionary in the dict tree control
    //Use the dict file name to insert it into the
    //structure later on to get the dict file name
    DictionaryDictTreeNode* dictionary_dict_tree_node = new DictionaryDictTreeNode(CS2WS(sDictFileName), CS2WS(sLabel));
    dictionary_dict_tree_node->SetDDDoc(pDoc);
    dictionary_dict_tree_node->AddRef();

    TV_INSERTSTRUCT tvi;
    tvi.item.mask = TVIF_TEXT | TVIF_PARAM | TVIF_IMAGE | TVIF_SELECTEDIMAGE;
    tvi.item.pszText = (LPTSTR) LPSTR_TEXTCALLBACK;
    tvi.hParent = TVI_ROOT;
    tvi.hInsertAfter = TVI_LAST;
    tvi.item.lParam = reinterpret_cast<LPARAM>(dictionary_dict_tree_node);
    tvi.item.iImage = DictIcon::Root;
    tvi.item.iSelectedImage = DictIcon::Root;

    HTREEITEM hItem = InsertItem(&tvi);
    dictionary_dict_tree_node->SetHItem(hItem);

    return hItem;
}


void CDDTreeCtrl::OnItemexpanding(NMHDR* pNMHDR, LRESULT* pResult)
{
    NM_TREEVIEW* pNMTreeView = reinterpret_cast<NM_TREEVIEW*>(pNMHDR);

    //Get the item corresponding to this Item if this has its pDoc as nullptr
    //Then we need to build the tree .
    //Send a message to our application to open the corresponding file
    HTREEITEM hItem = pNMTreeView->itemNew.hItem;
    DictTreeNode* dict_tree_node = GetTreeNode(hItem);

    if(dict_tree_node->GetDictElementType() == DictElementType::Dictionary && dict_tree_node->GetDDDoc() == nullptr) {
        const std::wstring& path = assert_cast<const DictionaryDictTreeNode*>(dict_tree_node)->GetPath();
        ASSERT(m_pDocTemplate != nullptr);
        CDDDoc* pDoc = assert_cast<CDDDoc*>(m_pDocTemplate->OpenDocumentFile(path.c_str()));
        dict_tree_node->SetDDDoc(pDoc);
        pDoc->SetDictTreeCtrl(this);
        BuildTree(assert_cast<DictionaryDictTreeNode&>(*dict_tree_node));
    }

    else if( ( pNMTreeView->action & TVE_EXPAND ) != 0 ) {
        CMDIChildWnd* pActiveWnd = assert_cast<CMDIFrameWnd*>(AfxGetMainWnd())->MDIGetActive();
        CWnd* pView = pActiveWnd ? pActiveWnd->GetActiveView() : nullptr;
        if(pView != nullptr) {
            pView->SendMessage(UWM::Dictionary::MarkUsedItems, reinterpret_cast<WPARAM>(dict_tree_node));
        }
    }

    *pResult = 0;
}


LOGFONT CDDTreeCtrl::m_DefLogFont ={ 0, 0, 0, 0 };
CFont CDDTreeCtrl::m_font;

void CDDTreeCtrl::BuildTree(DictionaryDictTreeNode& dictionary_dict_tree_node, bool bRedraw)
{
    if(m_DefLogFont.lfHeight == 0 )  {
        CFont* pFont = GetFont();
        pFont->GetLogFont(&m_DefLogFont);
    }
    LOGFONT logfont;
    memset(&logfont,0,sizeof(LOGFONT));
    bool bChangeFont  = false;
    m_font.GetSafeHandle() == nullptr ? bChangeFont = true : m_font.GetLogFont(&logfont);

    CIMSAString sFontName = GetDesignerFontName();
    sFontName.Trim();

    CIMSAString sDefFontFaceName(m_DefLogFont.lfFaceName);
    if((!sFontName.IsEmpty() && sFontName.CompareNoCase(logfont.lfFaceName) != 0) || (sFontName.IsEmpty() && sDefFontFaceName.CompareNoCase(logfont.lfFaceName) != 0 )){
        bChangeFont = true;
    }

    if(bChangeFont || logfont.lfHeight != m_DefLogFont.lfHeight * (GetDesignerFontZoomLevel()/ 100.0)){
        logfont = m_DefLogFont; //inititalize
        logfont.lfHeight = (LONG)( m_DefLogFont.lfHeight * (GetDesignerFontZoomLevel() / 100.0) ); //zoom

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
    CDDTreeCtrl& tc = *this;
    CDDDoc* pDoc = dictionary_dict_tree_node.GetDDDoc();

    if(!pDoc) {
        return;
    }
    TV_INSERTSTRUCT tvi;
    HTREEITEM       htiLevel, htiItem;

    SetRedraw(FALSE);               // Don't draw while changing

    const CDataDict* dictionary = pDoc->GetDict();

    tvi.item.mask = TVIF_TEXT | TVIF_PARAM | TVIF_IMAGE | TVIF_SELECTEDIMAGE;
    tvi.item.pszText = (LPTSTR) LPSTR_TEXTCALLBACK;

    // Insert levels
    HTREEITEM htiRoot = dictionary_dict_tree_node.GetHItem();

    for( size_t level_number = 0; level_number < dictionary->GetNumLevels(); ++level_number )
    {
        const DictLevel& dict_level = dictionary->GetLevel(level_number);

        tvi.hParent = htiRoot;

        DictTreeNode* dict_tree_node = new DictTreeNode(DictElementType::Level);
        tvi.item.lParam = reinterpret_cast<LPARAM>(dict_tree_node);
        tvi.item.iImage = DictIcon::Level;
        tvi.item.iSelectedImage = DictIcon::Level;

        dict_tree_node->SetDDDoc(pDoc);
        dict_tree_node->SetLevelIndex(level_number);

        htiLevel = tc.InsertItem(&tvi);
        dict_tree_node->SetHItem(htiLevel);

        tvi.hParent = htiLevel;

        // Insert Id record
        BuildTreeRec(NONE, level_number, COMMON, dictionary_dict_tree_node, htiLevel);

        // Insert records
        for (int iRec = 0 ; iRec < dict_level.GetNumRecords() ; iRec++) {
            BuildTreeRec(NONE, level_number, iRec, dictionary_dict_tree_node, htiLevel);
        }
    }

    // Insert relations
    for( size_t relation_number = 0; relation_number < dictionary->GetNumRelations(); ++relation_number )
    {
        const DictRelation& dict_relation = dictionary->GetRelation(relation_number);

        tvi.hParent = htiRoot;

        DictTreeNode* dict_tree_node = new DictTreeNode(DictElementType::Relation);
        tvi.item.lParam = reinterpret_cast<LPARAM>(dict_tree_node);
        tvi.item.iImage = DictIcon::Relation;
        tvi.item.iSelectedImage = DictIcon::Relation;

        dict_tree_node->SetDDDoc(pDoc);
        dict_tree_node->SetRelationIndex(relation_number);

        htiLevel = tc.InsertItem(&tvi);
        dict_tree_node->SetHItem(htiLevel);

        tvi.hParent = htiLevel;

        // Insert primary
        int iLevel = NONE;
        int iRec = NONE;
        int iItem = NONE;
        int iVSet = NONE;
        dictionary->LookupName(dict_relation.GetPrimaryName(), &iLevel, &iRec, &iItem, &iVSet);
        if (iItem == NONE) {
            // Insert record
            BuildTreeRec(relation_number, iLevel, iRec, dictionary_dict_tree_node, htiLevel);
        }
        else {
            // Insert item
            const CDictRecord* pRec = dictionary->GetLevel(iLevel).GetRecord(iRec);
            const CDictItem* pItem = pRec->GetItem(iItem);
            dict_tree_node = new DictTreeNode(DictElementType::Item);
            tvi.hParent = htiLevel;
            tvi.item.lParam = reinterpret_cast<LPARAM>(dict_tree_node);
            if (pItem->GetItemType() == ItemType::Item) {
                tvi.item.iImage = DictIcon::Item;
                tvi.item.iSelectedImage = DictIcon::Item;
            }
            else {
                tvi.item.iImage = DictIcon::Subitem;
                tvi.item.iSelectedImage = DictIcon::Subitem;
            }

            dict_tree_node->SetDDDoc(pDoc);
            dict_tree_node->SetRelationIndex(relation_number);
            dict_tree_node->SetLevelIndex(iLevel);
            dict_tree_node->SetRecordIndex(iRec);
            dict_tree_node->SetItemIndex(iItem);
            dict_tree_node->SetValueSetIndex(NONE);

            htiItem = tc.InsertItem(&tvi);
            dict_tree_node->SetHItem(htiItem);

            if (pItem->GetItemType() == ItemType::Item) {
                for (int iSubItem = iItem + 1 ; iSubItem < pRec->GetNumItems() ; iSubItem++) {
                    pItem = pRec->GetItem(iSubItem);
                    if (pItem->GetItemType() == ItemType::Item) {
                        break;
                    }
                    // Insert subitem
                    dict_tree_node = new DictTreeNode(DictElementType::Item);
                    tvi.hParent = htiLevel;
                    tvi.item.lParam = reinterpret_cast<LPARAM>(dict_tree_node);
                    tvi.item.iImage = DictIcon::Subitem;
                    tvi.item.iSelectedImage = DictIcon::Subitem;

                    dict_tree_node->SetDDDoc(pDoc);
                    dict_tree_node->SetRelationIndex(relation_number);
                    dict_tree_node->SetLevelIndex(iLevel);
                    dict_tree_node->SetRecordIndex(iRec);
                    dict_tree_node->SetItemIndex(iSubItem);
                    dict_tree_node->SetValueSetIndex(NONE);

                    htiItem =  tc.InsertItem(&tvi);
                    dict_tree_node->SetHItem(htiItem);
                }
            }

        }
        // Insert secondaries
        for( const DictRelationPart& dict_relation_part : dict_relation.GetRelationParts() ) {
            dictionary->LookupName(dict_relation_part.GetSecondaryName(), &iLevel, &iRec, &iItem, &iVSet);
            if (iItem == NONE) {
                BuildTreeRec(relation_number, iLevel, iRec, dictionary_dict_tree_node, htiLevel);
            }
            else {
                // Insert item
                const CDictRecord* pRec = dictionary->GetLevel(iLevel).GetRecord(iRec);
                const CDictItem* pItem = pRec->GetItem(iItem);
                dict_tree_node = new DictTreeNode(DictElementType::Item);
                tvi.hParent = htiLevel;
                tvi.item.lParam = reinterpret_cast<LPARAM>(dict_tree_node);
                if (pItem->GetItemType() == ItemType::Item) {
                    tvi.item.iImage = DictIcon::Item;
                    tvi.item.iSelectedImage = DictIcon::Item;
                }
                else {
                    tvi.item.iImage = DictIcon::Subitem;
                    tvi.item.iSelectedImage = DictIcon::Subitem;
                }

                dict_tree_node->SetDDDoc(pDoc);
                dict_tree_node->SetRelationIndex(relation_number);
                dict_tree_node->SetLevelIndex(iLevel);
                dict_tree_node->SetRecordIndex(iRec);
                dict_tree_node->SetItemIndex(iItem);
                dict_tree_node->SetValueSetIndex(NONE);

                htiItem =  tc.InsertItem(&tvi);
                dict_tree_node->SetHItem(htiItem);

                if (pItem->GetItemType() == ItemType::Item) {
                    for (int iSubItem = iItem + 1 ; iSubItem < pRec->GetNumItems() ; iSubItem++) {
                        pItem = pRec->GetItem(iSubItem);
                        if (pItem->GetItemType() == ItemType::Item) {
                            break;
                        }
                        // Insert subitem
                        dict_tree_node = new DictTreeNode(DictElementType::Item);
                        tvi.hParent = htiLevel;
                        tvi.item.lParam = reinterpret_cast<LPARAM>(dict_tree_node);
                        tvi.item.iImage = DictIcon::Subitem;
                        tvi.item.iSelectedImage = DictIcon::Subitem;

                        dict_tree_node->SetDDDoc(pDoc);
                        dict_tree_node->SetRelationIndex(relation_number);
                        dict_tree_node->SetLevelIndex(iLevel);
                        dict_tree_node->SetRecordIndex(iRec);
                        dict_tree_node->SetItemIndex(iSubItem);
                        dict_tree_node->SetValueSetIndex(NONE);

                        htiItem =  tc.InsertItem(&tvi);
                        dict_tree_node->SetHItem(htiItem);
                    }
                }
            }
        }
    }

    DefaultExpand(htiRoot);
    if (!bRedraw) {
        SetRedraw(TRUE);
        Invalidate();
        UpdateWindow();
    }
}


void CDDTreeCtrl::BuildTreeRec(int iRel, int iLevel, int iRec, DictionaryDictTreeNode& dictionary_dict_tree_node, HTREEITEM htiLevel)
{
    CDDTreeCtrl& tc = *this;
    CDDDoc* pDoc = dictionary_dict_tree_node.GetDDDoc();

    TV_INSERTSTRUCT tvi;
    HTREEITEM       htiRec, htiItem, htiVSet, htiOccurs;

    const CDataDict* dictionary = pDoc->GetDict();
    const CDictItem* pParentItem = nullptr;

    tvi.item.mask = TVIF_TEXT | TVIF_PARAM | TVIF_IMAGE | TVIF_SELECTEDIMAGE;
    tvi.item.pszText = (LPTSTR) LPSTR_TEXTCALLBACK;

    // Insert record
    tvi.hParent = htiLevel;
    DictTreeNode* dict_tree_node = new DictTreeNode(DictElementType::Record);
    tvi.item.lParam = reinterpret_cast<LPARAM>(dict_tree_node);
    if (iRec == COMMON) {
        tvi.item.iImage = DictIcon::Common;
        tvi.item.iSelectedImage = DictIcon::Common;
    }
    else {
        tvi.item.iImage = DictIcon::Record;
        tvi.item.iSelectedImage = DictIcon::Record;
    }

    dict_tree_node->SetDDDoc(pDoc);
    dict_tree_node->SetRelationIndex(iRel);
    dict_tree_node->SetLevelIndex(iLevel);
    dict_tree_node->SetRecordIndex(iRec);

    htiRec = tc.InsertItem(&tvi);
    dict_tree_node->SetHItem(htiRec);

    // Insert items for each record
    const CDictRecord* pRec = dictionary->GetLevel(iLevel).GetRecord(iRec);
    for (int iItem = 0 ; iItem < pRec->GetNumItems() ; iItem++) {
        const CDictItem* pItem = pRec->GetItem(iItem);
        dict_tree_node = new DictTreeNode(DictElementType::Item);
        tvi.hParent = htiRec;
        tvi.item.lParam = reinterpret_cast<LPARAM>(dict_tree_node);
        if (pItem->GetItemType() == ItemType::Item) {
            tvi.item.iImage = DictIcon::Item;
            tvi.item.iSelectedImage = DictIcon::Item;
            pParentItem = pItem;
        }
        else {
            tvi.item.iImage = DictIcon::Subitem;
            tvi.item.iSelectedImage = DictIcon::Subitem;
        }

        dict_tree_node->SetDDDoc(pDoc);
        dict_tree_node->SetRelationIndex(iRel);
        dict_tree_node->SetLevelIndex(iLevel);
        dict_tree_node->SetRecordIndex(iRec);
        dict_tree_node->SetItemIndex(iItem);
        dict_tree_node->SetValueSetIndex(NONE);

        htiItem =  tc.InsertItem(&tvi);
        dict_tree_node->SetHItem(htiItem);

        // Insert value sets for each item
        if (!pItem->HasValueSets() || (pItem->GetNumValueSets() == 1 && pItem->GetLabel() == pItem->GetValueSet(0).GetLabel())) {
            // Don't put out VSets
        }
        else {
            for (int iVSet = 0 ; iVSet < (int)pItem->GetNumValueSets() ; iVSet++ ) {
                dict_tree_node = new DictTreeNode(DictElementType::ValueSet);
                tvi.hParent = htiItem;
                tvi.item.lParam = reinterpret_cast<LPARAM>(dict_tree_node);
                tvi.item.iImage = DictIcon::ValueSet;
                tvi.item.iSelectedImage = DictIcon::ValueSet;

                dict_tree_node->SetDDDoc(pDoc);
                dict_tree_node->SetRelationIndex(iRel);
                dict_tree_node->SetLevelIndex(iLevel);
                dict_tree_node->SetRecordIndex(iRec);
                dict_tree_node->SetItemIndex(iItem);
                dict_tree_node->SetValueSetIndex(iVSet);

                htiVSet = tc.InsertItem(&tvi);
                dict_tree_node->SetHItem(htiVSet);
            }
        }
        // Insert multiple occurrences for each item
        int iNumOccurs = pItem->GetOccurs();
        if (pItem->GetItemType() == ItemType::Subitem && pParentItem->GetOccurs() > 1) {
            iNumOccurs = pParentItem->GetOccurs();
        }
        if (iNumOccurs > 1) {
            for (int iOccurs = 0 ; iOccurs < iNumOccurs ; iOccurs++) {
                dict_tree_node = new DictTreeNode(DictElementType::Item);
                tvi.hParent = htiItem;
                tvi.item.lParam = reinterpret_cast<LPARAM>(dict_tree_node);
                if (pItem->GetItemType() == ItemType::Item) {
                    tvi.item.iImage = DictIcon::Item;
                    tvi.item.iSelectedImage = DictIcon::Item;
                }
                else {
                    tvi.item.iImage = DictIcon::Subitem;
                    tvi.item.iSelectedImage = DictIcon::Subitem;
                }
                dict_tree_node->SetDDDoc(pDoc);
                dict_tree_node->SetRelationIndex(iRel);
                dict_tree_node->SetLevelIndex(iLevel);
                dict_tree_node->SetRecordIndex(iRec);
                dict_tree_node->SetItemIndex(iItem);
                dict_tree_node->SetValueSetIndex(NONE);
                dict_tree_node->SetItemOccurs(iOccurs);

                htiOccurs = tc.InsertItem(&tvi);
                dict_tree_node->SetHItem(htiOccurs);

                // Insert value sets for each occurrence
                if (!pItem->HasValueSets() || (pItem->GetNumValueSets() == 1 && pItem->GetLabel() == pItem->GetValueSet(0).GetLabel())) {
                    // Don't put out VSets
                }
                else {
                    for (int iVSet = 0 ; iVSet < (int)pItem->GetNumValueSets() ; iVSet++ ) {
                        dict_tree_node = new DictTreeNode(DictElementType::ValueSet);
                        tvi.hParent = htiOccurs;
                        tvi.item.lParam = reinterpret_cast<LPARAM>(dict_tree_node);
                        tvi.item.iImage = DictIcon::ValueSet;
                        tvi.item.iSelectedImage = DictIcon::ValueSet;

                        dict_tree_node->SetDDDoc(pDoc);
                        dict_tree_node->SetRelationIndex(iRel);
                        dict_tree_node->SetLevelIndex(iLevel);
                        dict_tree_node->SetRecordIndex(iRec);
                        dict_tree_node->SetItemIndex(iItem);
                        dict_tree_node->SetValueSetIndex(iVSet);
                        dict_tree_node->SetItemOccurs(iOccurs);

                        htiVSet = tc.InsertItem(&tvi);
                        dict_tree_node->SetHItem(htiVSet);
                    }
                }
            }
        }
    }
}



void CDDTreeCtrl::ReBuildTree(DictionaryDictTreeNode& dictionary_dict_tree_node, int iLevel, int iRec, int iItem)
{
    SetRedraw(FALSE);               // Don't draw while changing
    m_bUpdateAllViews = false;
    HTREEITEM hNode = dictionary_dict_tree_node.GetHItem();
    HTREEITEM hChild = GetChildItem(hNode);
    while(hChild) {
        DeleteItem(hChild);
        hChild = GetChildItem(hNode);
    }
    BuildTree(dictionary_dict_tree_node, true);
    SelectNode(dictionary_dict_tree_node, true, iLevel, iRec, iItem);
    EnsureVisible(GetSelectedItem());
    m_bUpdateAllViews = true;
}


void CDDTreeCtrl::SelectNode(const DictionaryDictTreeNode& dictionary_dict_tree_node, bool bExpandRec, int iLevel, int iRec, int iItem)
{
    DictTreeNode* dict_tree_node;
    HTREEITEM hNode = dictionary_dict_tree_node.GetHItem();
    if (iLevel == NONE) {
        Select(hNode, TVGN_CARET);
    }
    else {
        HTREEITEM hLevel = GetChildItem(hNode);
        while(hLevel) {
            dict_tree_node = GetTreeNode(hLevel);
            if (dict_tree_node->GetLevelIndex() == iLevel) {
                if (iRec == NONE) {
                    Select(hLevel, TVGN_CARET);
                    break;
                }
                HTREEITEM hRec = GetChildItem(hLevel);
                while(hRec) {
                    dict_tree_node = GetTreeNode(hRec);
                    if (dict_tree_node->GetRecordIndex() == iRec) {
                        if (iItem == NONE) {
                            Select(hRec, TVGN_CARET);
                            if (bExpandRec) {
                                Expand(hRec, TVE_EXPAND);
                            }
                            break;
                        }
                        HTREEITEM hItem = GetChildItem(hRec);
                        while(hItem) {
                            dict_tree_node = GetTreeNode(hItem);
                            if (dict_tree_node->GetItemIndex() == iItem) {
                                if (dict_tree_node->GetValueSetIndex() == NONE) {
                                    Select(hItem, TVGN_CARET);
                                    break;
                                }
//                                HTREEITEM hVSet = GetChildItem(hItem);
                            }
                            hItem = GetNextSiblingItem(hItem);
                        }
                    }
                    hRec = GetNextSiblingItem(hRec);
                }
            }
            hLevel = GetNextSiblingItem(hLevel);
        }
    }
    SetRedraw(TRUE);
    Invalidate();
//    UpdateWindow();
}


void CDDTreeCtrl::ResetTreeIcons()
{
    HTREEITEM hNode = GetRootItem();
    if (hNode == nullptr) {
        return;
    }
    SetRedraw(FALSE);
    while (hNode != nullptr) {
        HTREEITEM hLevel = GetChildItem(hNode);
        while (hLevel != nullptr) {
            HTREEITEM hRec = GetChildItem(hLevel);
            while (hRec != nullptr) {
                HTREEITEM hItem = GetChildItem(hRec);
                while (hItem != nullptr) {
                    if (GetTreeNode(hItem)->IsSubitem()) {
                        SetItemImage(hItem, DictIcon::Subitem, DictIcon::Subitem);
                    }
                    else {
                        SetItemImage(hItem, DictIcon::Item, DictIcon::Item);
                    }
                    HTREEITEM hVSet = GetChildItem(hItem);
                    while (hVSet != nullptr) {
                        SetItemImage(hVSet, DictIcon::ValueSet, DictIcon::ValueSet);
                        hVSet = GetNextSiblingItem(hVSet);
                    }
                    hItem = GetNextSiblingItem(hItem);
                }
                hRec = GetNextSiblingItem(hRec);
            }
            hLevel = GetNextSiblingItem(hLevel);
        }
        hNode = GetNextSiblingItem(hNode);
    }
    SetRedraw(TRUE);
    Invalidate();
    UpdateWindow();
}


void CDDTreeCtrl::DefaultExpand(HTREEITEM hItem)
{
    HTREEITEM hNode = hItem;
    DictTreeNode* dict_tree_node = GetTreeNode(hNode);
    if (dict_tree_node->GetDictElementType() == DictElementType::Dictionary) {
        CString sFile = dict_tree_node->GetDDDoc()->GetPathName();
        if (sFile.Right(7).CompareNoCase(_T("wrk.dcf")) != 0) {
            Expand(hNode, TVE_EXPAND);
        }
    }
    if (dict_tree_node->GetDictElementType() == DictElementType::Level) {
        Expand(hNode, TVE_EXPAND);
    }
    HTREEITEM hChild = GetChildItem(hNode);
    while(hChild) {
        DefaultExpand(hChild);
        hChild = GetChildItem(hChild);
    }
    if (dict_tree_node->GetDictElementType() != DictElementType::Dictionary) {
        hNode = GetNextSiblingItem(hNode);
        if (hNode) {
            DefaultExpand(hNode);
        }
    }
}


DictionaryDictTreeNode* CDDTreeCtrl::GetDictionaryTreeNode(CDocument& document) const
{
    HTREEITEM hItem = GetRootItem();

    while( hItem != nullptr )
    {
        DictionaryDictTreeNode* dictionary_dict_tree_node = reinterpret_cast<DictionaryDictTreeNode*>(GetItemData(hItem));

        if( dictionary_dict_tree_node->GetDDDoc() == &document )
            return dictionary_dict_tree_node;

        hItem = GetNextSiblingItem(hItem);
    }

    return nullptr;
}


DictionaryDictTreeNode* CDDTreeCtrl::GetDictionaryTreeNode(wstring_view filename) const
{
    HTREEITEM hItem = GetRootItem();

    while( hItem != nullptr )
    {
        DictionaryDictTreeNode* dictionary_dict_tree_node = reinterpret_cast<DictionaryDictTreeNode*>(GetItemData(hItem));

        if( SO::EqualsNoCase(filename, dictionary_dict_tree_node->GetPath()) )
            return dictionary_dict_tree_node;

        hItem = GetNextSiblingItem(hItem);
    }

    return nullptr;
}


void CDDTreeCtrl::RemoveDictIDs(HTREEITEM hItem)
{
    // Use this only when the starting Items is a DictNode
    // void CDDTreeCtrl::RemoveDictIDs(HTREEITEM hItem)
    // Removes the CDictIDs attached to the hItems for this hItem and its children

    //remove the dict ID of this Item
    DictTreeNode* dict_tree_node = GetTreeNode(hItem);
    if(dict_tree_node != nullptr){
        delete dict_tree_node;
        SetItemData(hItem,0);
    }
    //check if it has any children
    HTREEITEM hChild ;
    hChild = GetChildItem(hItem);

    //Remove the DictIds for the child items
    while(hChild) {
        RemoveDictIDs(hChild);
        hChild = GetChildItem(hChild);
    }

    //Get the sibling items
    hItem = GetNextSiblingItem(hItem);

    //if the item has parent only then call for the deletion of the sibling items and its children
    if(GetParentItem(hItem))
        RemoveDictIDs(hItem);

}


void CDDTreeCtrl::Clean()
{
    //  Remove all the dictIDs if they STILL available (should not be there )
    //  Should be cleared by appropriate close document routines
    HTREEITEM hItem = GetRootItem() ;
    while (hItem) {
        RemoveDictIDs(hItem);
        DictTreeNode* dict_tree_node = GetTreeNode(hItem);
        delete dict_tree_node;
        //Get the next sibling item and run on it
        hItem=GetNextSiblingItem(hItem);
    }
}


void CDDTreeCtrl::OnDeleteitem(NMHDR* pNMHDR, LRESULT* pResult)
{
    NM_TREEVIEW* pNMTreeView = reinterpret_cast<NM_TREEVIEW*>(pNMHDR);
    // TODO: Add your control notification handler code here
    HTREEITEM hItem = pNMTreeView->itemOld.hItem;
    DictTreeNode* dict_tree_node = GetTreeNode(hItem);
    delete dict_tree_node;

    *pResult = 0;
}


void CDDTreeCtrl::ReleaseDictionaryNode(DictionaryDictTreeNode& dictionary_dict_tree_node)
{
    dictionary_dict_tree_node.Release();

    if( dictionary_dict_tree_node.GetRefCount() == 0 )
    {
        if( dictionary_dict_tree_node.GetDDDoc() != nullptr )
        {
            //By now the document should be saved so go ahead and close the document
            dictionary_dict_tree_node.GetDDDoc()->OnCloseDocument();

            //drop the item from the tree
            SetRedraw(FALSE);
            DeleteItem(dictionary_dict_tree_node.GetHItem());
            SetRedraw(TRUE);
        }

        else
        {
            //remove the item from the tree
            DeleteItem(dictionary_dict_tree_node.GetHItem());
        }
    }
}


void CDDTreeCtrl::OnBegindrag(NMHDR* pNMHDR, LRESULT* pResult)
{
    CPoint ptAction;

    // This code is to prevent accidental drags.
    if( (GetTickCount() - m_dwDragStart) < DRAG_DELAY) {
        return;
    }
    ASSERT(!m_bDragging);
    GetCursorPos(&ptAction);
    ScreenToClient(&ptAction);
    HTREEITEM   m_hitemDrag ;//= HitTest(ptAction, &nFlags);
    NM_TREEVIEW* pNMTreeView = reinterpret_cast<NM_TREEVIEW*>(pNMHDR);
    *pResult = 0;
    m_hitemDrag = pNMTreeView->itemNew.hItem;
    SelectItem(m_hitemDrag);
    // Get image list for dragging
    m_pDragImage.reset(CreateDragImage(m_hitemDrag));
    if( !m_pDragImage) {
        return;
    }
    m_bDragging = true;
    m_pDragImage->DragShowNolock(TRUE);
    m_pDragImage->SetDragCursorImage(0, CPoint(0, 0));
    m_pDragImage->BeginDrag(0, CPoint(0,0));

    ClientToScreen(&ptAction);
    m_pDragImage->DragMove(ptAction);
    m_pDragImage->DragEnter(GetDesktopWindow(), ptAction);
    SetCapture();
}


void CDDTreeCtrl::OnLButtonUp(UINT nFlags, CPoint point)
{
    if (m_bDragging) {
        ShowCursor(TRUE);
        m_pDragImage->DragLeave(GetDesktopWindow());
        m_pDragImage->EndDrag();
        m_pDragImage.reset();

        ClientToScreen(&point);
        //Check if the drop is with in the main window
        CMDIChildWnd* pActiveWnd = assert_cast<CMDIFrameWnd*>(AfxGetMainWnd())->MDIGetActive();
        CWnd* pView = pActiveWnd->GetActiveView();

        HTREEITEM hItem = GetSelectedItem();
        if( hItem != nullptr ) {
            LPARAM lParam = GetItemData(hItem);
            DictTreeNode* dict_tree_node = reinterpret_cast<DictTreeNode*>(lParam);
            ReleaseCapture();
            m_bDragging = false;
            pView->SendMessage(WM_IMSA_DROPITEM,MAKEWPARAM(point.x, point.y), reinterpret_cast<LPARAM>(dict_tree_node));
        }
        else {
            ReleaseCapture();
            m_bDragging = false;
        }
        SelectDropTarget(nullptr);
        SelectItem(hItem);
    }
    CTreeCtrl::OnLButtonUp(nFlags, point);            // mfc-gen
}


void CDDTreeCtrl::OnMouseMove(UINT nFlags, CPoint point)
{
    bool bRight = false;
    CPoint tPoint = point;

    if (m_bDragging) {
        RECT rect;
        GetClientRect( &rect );
        if (point.x < (rect.right-rect.left) /2) {
            bRight = false;
        }
        else {
            bRight = true;
        }
        ClientToScreen(&point);
        ASSERT(m_pDragImage != nullptr);

        m_pDragImage->DragMove(point);
        CWnd* pWnd = this->WindowFromPoint(point);
        SetCursor4TabDrop(point);

        if( pWnd == this) {
            if (bRight )    {
                // We need to scroll up, go slow if cursor near treeview control
                m_pDragImage->DragShowNolock(FALSE);
                SendMessage(WM_HSCROLL, MAKEWPARAM(SB_LINERIGHT,0), (LPARAM)nullptr);
            }
            else {
                // We need to scroll down, go slow if cursor near treeview control
                m_pDragImage->DragShowNolock(FALSE);
                SendMessage(WM_HSCROLL, MAKEWPARAM(SB_LINELEFT,0), (LPARAM)nullptr);
            }
        }
    }
    m_pDragImage->DragShowNolock(TRUE);
    CTreeCtrl::OnMouseMove(nFlags, point);
}


void CDDTreeCtrl::OnLButtonDown(UINT nFlags, CPoint point)
{
    CMDIChildWnd* pActiveWnd = assert_cast<CMDIFrameWnd*>(AfxGetMainWnd())->MDIGetActive();
    if(pActiveWnd) {
        CDocument* pDoc = pActiveWnd->GetActiveDocument();
        if(pDoc->IsKindOf(RUNTIME_CLASS(CDDDoc))) {
            CDDDoc* dictionary_doc = assert_cast<CDDDoc*>(pDoc);
            if (!dictionary_doc->IsDocOK(false)) {
                return;
            }
        }
        m_dwDragStart = GetTickCount();
    }
    CTreeCtrl::OnLButtonDown(nFlags, point);
}


BOOL CDDTreeCtrl::PreTranslateMessage(MSG* pMsg)
{
    switch(pMsg->message) {
    case WM_RBUTTONDOWN:
        if(m_bDragging) {
            m_pDragImage->DragLeave(GetDesktopWindow());
            m_pDragImage->EndDrag();
            m_pDragImage.reset();
            ReleaseCapture();
            m_bDragging = false;
            SelectDropTarget(nullptr);
            HTREEITEM hItem = GetSelectedItem();
            SelectItem(hItem);
            return FALSE;
        }
        return TRUE;
//      break;
    case WM_KEYDOWN:
        if (pMsg->wParam == '\t') {
            CMDIChildWnd* pActiveWnd = assert_cast<CMDIFrameWnd*>(AfxGetMainWnd())->MDIGetActive();
            if(pActiveWnd == nullptr) {
                return FALSE;
            }
            else {
                CView* pView = pActiveWnd->GetActiveView();
                pView->SetFocus();
                return TRUE;
            }
        }
        else if (pMsg->wParam == VK_ESCAPE) {
            HTREEITEM hItem = GetSelectedItem();
            hItem = GetParentItem(hItem);
            if (hItem != nullptr) {
                SelectItem(hItem);
            }
            return TRUE;
        }
        break;
    }
    return CTreeCtrl::PreTranslateMessage(pMsg);
}


void CDDTreeCtrl::DeleteDrag()
{
    if(m_bDragging ) {
        m_pDragImage->DragLeave(GetDesktopWindow());
        m_pDragImage->EndDrag();
        m_pDragImage.reset();
        ReleaseCapture();
        m_bDragging = false;
        SelectDropTarget(nullptr);
        HTREEITEM hItem = GetSelectedItem();
        SelectItem(hItem);
    }
}


void CDDTreeCtrl::ReleaseDoc(DictionaryDictTreeNode& dictionary_dict_tree_node)
{
    if( dictionary_dict_tree_node.GetDDDoc() == nullptr )
        return;

    dictionary_dict_tree_node.SetDDDoc(nullptr);
    dictionary_dict_tree_node.Release();

    HTREEITEM hNode = dictionary_dict_tree_node.GetHItem();
    HTREEITEM hChild = GetChildItem(hNode);

    while( hChild != nullptr )
    {
        DeleteItem(hChild);
        hChild = GetChildItem(hNode);
    }
}


void CDDTreeCtrl::OnRButtonUp(UINT nFlags, CPoint point)
{
    bool isQuestionnaireView = false;

    // End grid edit
    CMDIChildWnd* pActiveWnd = assert_cast<CMDIFrameWnd*>(AfxGetMainWnd())->MDIGetActive();
    if(pActiveWnd) {
        CDocument* pDoc = pActiveWnd->GetActiveDocument();
        if(pDoc->IsKindOf(RUNTIME_CLASS(CDDDoc))) {
            CDDDoc* dictionary_doc = assert_cast<CDDDoc*>(pDoc);
            POSITION pos = dictionary_doc->GetFirstViewPosition();
            ASSERT(pos != nullptr);
            CDDGView* pView = assert_cast<CDDGView*>(dictionary_doc->GetNextView(pos));

            CDictChildWnd* pDictChildWnd = assert_cast<CDictChildWnd*>(pView->GetParentFrame());
            isQuestionnaireView = pDictChildWnd->isQuestionnaireView();

            // 20131028 in the dictionary grid, right-clicking on the tree could cause problems while in edit mode, so disable such clicks if
            // there were errors in canceling out of edit mode (which would mean a problem with validating the data entered)
            CDDGrid& current_grid = pView->GetCurrentGrid();

            if ( current_grid.IsEditing() )
            {
                current_grid.EditChange(VK_CANCEL);

                if( current_grid.IsEditing() )
                    return;
            }
        }
    }

    // Need to save dddoc in mainframe --- BMD BMD

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
    if(hItem) {
        if (GetSelectedItem() == hItem) {
            SetFocus();
        }
        else {
            SelectItem(hItem);
        }
    }
    else {
        SetFocus();
        return;
    }

    // Put up menu
    CDataDict* selected_dictionary = GetSelectedDictionary(false);
    ASSERT(selected_dictionary!= nullptr);

    DictTreeNode* dict_tree_node = GetTreeNode(hItem);
    CString csTree;
    CString csGrid;
    if (dict_tree_node->GetLevelIndex() == NONE) {
        csTree = _T(" Dict");
        csGrid = _T(" Level");
    }
    else if (dict_tree_node->GetRecordIndex() == NONE) {
        csTree = _T(" Level");
        csGrid = _T(" Record");
    }
    else if (dict_tree_node->GetItemIndex() == NONE) {
        csTree = _T(" Record");
        csGrid = _T(" Item");
    }
    else if (dict_tree_node->GetValueSetIndex() == NONE) {
        csTree = _T(" Item");
        const CDictItem* dict_item = selected_dictionary->GetLevel(dict_tree_node->GetLevelIndex()).GetRecord(dict_tree_node->GetRecordIndex())->GetItem(dict_tree_node->GetItemIndex());
        if (DictionaryRules::CanHaveValueSet(*dict_item))
            csGrid = _T(" Value Set");
    }
    else {
        csTree = _T(" Value Set");
        csGrid = _T(" Value Set");
    }

    // Need to change events to go to mainframe - mainframe will bring up correct frame and dispatch message to frame

    BCMenu popup_menu;    // BMD 29 Sep 2003
    popup_menu.CreatePopupMenu();

    std::unique_ptr<CMenu> open_with_menu;

    UINT dynamicFlag = isQuestionnaireView ? MF_STRING | MF_GRAYED : MF_STRING;
    // 20101106, rosie request ... allow the user to copy the name of the node (for logic editing)
    popup_menu.AppendMenu(MF_STRING, ID_COPY_DICT_NAME, _T("&Copy Name"));
    popup_menu.AppendMenu(MF_SEPARATOR);
    popup_menu.AppendMenu(MF_STRING, ID_VIEW_DICTIONARY, _T("View Dictionary"));
    popup_menu.AppendMenu(MF_STRING, ID_VIEW_QUESTIONNAIRE, _T("View Questionnaire"));
    popup_menu.AppendMenu(MF_SEPARATOR);

    if (dict_tree_node->GetDictElementType() == DictElementType::Relation) {
        popup_menu.AppendMenu(dynamicFlag, ID_EDIT_RELATION, _T("&Modify Relations"));
    }
    else {
        if( dict_tree_node->GetLevelIndex() == NONE ) {
            popup_menu.AppendMenu(MF_STRING,ID_DICTIONARY_MACROS,_T("Dictionary Macros"));

            // add some "open with" options
            open_with_menu = std::make_unique<CMenu>();
            open_with_menu->CreatePopupMenu();

            open_with_menu->AppendMenu(MF_STRING, ID_OPEN_WITH_CSDIFF, _T("Compare Data"));
            open_with_menu->AppendMenu(MF_STRING, ID_OPEN_WITH_EXCEL2CSPRO, _T("Excel to CSPro"));
            open_with_menu->AppendMenu(MF_STRING | ( selected_dictionary->GetAllowExport() ? 0 : MF_GRAYED ), ID_OPEN_WITH_CSEXPORT, _T("Export Data"));
            open_with_menu->AppendMenu(MF_STRING, ID_OPEN_WITH_CSINDEX, _T("Index Data"));
            open_with_menu->AppendMenu(MF_STRING, ID_OPEN_WITH_CSSORT, _T("Sort Data"));
            open_with_menu->AppendMenu(MF_STRING, ID_OPEN_WITH_CSFREQ, _T("Tabulate Frequencies"));

            popup_menu.AppendMenu(MF_SEPARATOR);
            popup_menu.AppendMenu(MF_POPUP, (UINT)open_with_menu->GetSafeHmenu(), _T("Open With"));

            // add a view data option
            UWM::Dictionary::GetApplicationPffParameters get_application_pff_parameters { *selected_dictionary, false, nullptr };

            if( WindowsDesktopMessage::Send(UWM::Dictionary::GetApplicationPff, &get_application_pff_parameters) == 1 )
            {
                ASSERT(get_application_pff_parameters.pff != nullptr);

                m_viewDataFilenames.clear();

                auto add_view_data_menu_option = [&](const TCHAR* data_type, const ConnectionString& connection_string)
                {
                    UINT nID = ID_OPEN_DATA_FILE0 + m_viewDataFilenames.size();

                    // only add up to 10 files (that exist)
                    if( nID > ID_OPEN_DATA_FILE9 || !connection_string.IsFilenamePresent() ||
                                                    !PortableFunctions::FileIsRegular(connection_string.GetFilename()) )
                    {
                        return;
                    }

                    if( m_viewDataFilenames.empty() )
                        popup_menu.AppendMenu(MF_SEPARATOR);

                    CString menu_text = FormatText(_T("View%s Data: %s"), data_type, PortableFunctions::PathGetFilename(connection_string.GetFilename()));
                    popup_menu.AppendMenu(MF_STRING, nID, menu_text);

                    m_viewDataFilenames.emplace_back(WS2CS(connection_string.GetFilename()));
                };

                if( get_application_pff_parameters.is_input_dictionary )
                {
                    for( const ConnectionString& connection_string : get_application_pff_parameters.pff->GetInputDataConnectionStrings() )
                        add_view_data_menu_option(_T(" Input"), connection_string);

                    for( const ConnectionString& connection_string : get_application_pff_parameters.pff->GetOutputDataConnectionStrings() )
                        add_view_data_menu_option(_T(" Output"), connection_string);
                }

                // external dictionaries
                else
                {
                    add_view_data_menu_option(_T(""), get_application_pff_parameters.pff->GetExternalDataConnectionString(selected_dictionary->GetName()));
                }
            }
        }

        else
        {
            if (/*dict_tree_node->GetLevelIndex() == NONE || */(dict_tree_node->GetRecordIndex() == COMMON && csTree == _T(" Record"))) {
                popup_menu.AppendMenu(MF_STRING | MF_GRAYED, ID_EDIT_MODIFY, _T("&Modify\tCtrl+M"));
            }
            else {
                popup_menu.AppendMenu(dynamicFlag, ID_EDIT_MODIFY,_T("&Modify") + csTree + _T("\tCtrl+M"));
            }
            /*if (dict_tree_node->GetLevelIndex() == NONE) {
                popup_menu.AppendMenu(MF_STRING | MF_GRAYED, ID_EDIT_ADD, "&Add\tCtrl+A");
            }
            else*/ {
                popup_menu.AppendMenu(dynamicFlag, ID_EDIT_ADD,_T("&Add") + csTree + _T("\tCtrl+A"));
            }
            if (/*dict_tree_node->GetLevelIndex() == NONE || */(dict_tree_node->GetRecordIndex() == COMMON && csTree == _T(" Record"))) {
                popup_menu.AppendMenu(MF_STRING | MF_GRAYED, ID_EDIT_INSERT, _T("&Insert\tIns"));
            }
            else {
                popup_menu.AppendMenu(dynamicFlag, ID_EDIT_INSERT,_T("&Insert") + csTree + _T("\tIns"));
            }
            if (/*dict_tree_node->GetLevelIndex() == NONE || */(dict_tree_node->GetRecordIndex() == COMMON && csTree == _T(" Record"))) {
                popup_menu.AppendMenu(MF_STRING | MF_GRAYED, ID_EDIT_DELETE, _T("&Delete\tDel"));
            }
            else {
                popup_menu.AppendMenu(dynamicFlag, ID_EDIT_DELETE,_T("&Delete") + csTree + _T("\tDel"));
            }
        }
        if (csTree != csGrid && !csGrid.IsEmpty()) {

            popup_menu.AppendMenu(MF_SEPARATOR);
            CString sMenuItem = _T("Add") + csGrid;
            popup_menu.AppendMenu(dynamicFlag, ID_EDIT_ADD_NEXT, sMenuItem.GetString());
        }
    }

    popup_menu.LoadToolbar(IDR_DICT_FRAME);   // BMD 29 Sep 2003

    CRect rect;
    GetWindowRect(rect);
    popup_menu.TrackPopupMenu(TPM_RIGHTBUTTON, rect.left + point.x + 5, rect.top + point.y + 5, this);

    CTreeCtrl::OnRButtonUp(nFlags, point);
}


template<typename CF>
void CDDTreeCtrl::OnActivatedView(CF callback_function)
{
    CMDIChildWnd* pActiveWnd = assert_cast<CMDIFrameWnd*>(AfxGetMainWnd())->MDIGetActive();

    if( pActiveWnd == nullptr )
        return;

    CDocument* pDoc = pActiveWnd->GetActiveDocument();
    CDDGView* pView;

    auto set_view =
        [&](CDDDoc* dictionary_doc)
        {
            // Get View for this document
            POSITION pos = dictionary_doc->GetFirstViewPosition();
            ASSERT(pos != nullptr);
            pView = assert_cast<CDDGView*>(dictionary_doc->GetNextView(pos));
        };

    if( pDoc->IsKindOf(RUNTIME_CLASS(CDDDoc)) )
    {
        set_view(assert_cast<CDDDoc*>(pDoc));
    }

    else
    {
        // Get document associated with selected item
        HTREEITEM hItem = GetSelectedItem();
        LPARAM lParam = GetItemData(hItem);
        DictTreeNode* dict_tree_node = reinterpret_cast<DictTreeNode*>(lParam);

        set_view(dict_tree_node->GetDDDoc());
        pView->UpdateWindow();

        // Activate frame
        pView->GetParentFrame()->ActivateFrame(SW_SHOW);
        SetFocus();
    }

    callback_function(*pView);
}


void CDDTreeCtrl::OnActivatedViewSendCommand(WPARAM wParam)
{
    OnActivatedView([&](CDDGView& view)
    {
        view.SendMessage(WM_COMMAND, wParam);
    });
}


void CDDTreeCtrl::OnEditAdd()
{
    OnActivatedViewSendCommand(ID_EDIT_ADD);
}


void CDDTreeCtrl::OnEditInsert()
{
    OnActivatedViewSendCommand(ID_EDIT_INSERT);
}


void CDDTreeCtrl::OnEditDelete()
{
    OnActivatedViewSendCommand(ID_EDIT_DELETE);
}

void CDDTreeCtrl::OnEditModify()
{
    OnActivatedViewSendCommand(ID_EDIT_MODIFY);
}


void CDDTreeCtrl::OnEditAddNext()
{
    OnActivatedView([](CDDGView& view)
    {
        view.SendMessage(WM_SETFOCUS);
        view.SendMessage(WM_COMMAND, ID_EDIT_ADD);
    });
}


int CDDTreeCtrl::SetCursor4TabDrop(CPoint /*point*/, bool bFromGrid /*= false*/)
{
    static std::vector<HCURSOR> cursors;

    // load the cursors if necessary
    if( cursors.empty() )
    {
        auto load_cursor = [&](auto resource_id, auto index)
        {
            ASSERT(index == (int)cursors.size());
            cursors.emplace_back(AfxGetApp()->LoadCursor(resource_id));
        };

        load_cursor(IDC_ROW, DictCursor::Row);
        load_cursor(IDC_ROWPLUS, DictCursor::RowPlus);
        load_cursor(IDC_ROWSTAR, DictCursor::RowStar);
        load_cursor(IDC_ROWSTARPLUS, DictCursor::RowStarPlus);
        load_cursor(IDC_COL, DictCursor::Column);
        load_cursor(IDC_COLPLUS, DictCursor::ColumnPlus);
        load_cursor(IDC_COLSTAR, DictCursor::ColumnStar);
        load_cursor(IDC_COLSTARPLUS, DictCursor::ColumnStarPlus);
        load_cursor(IDC_NODROP, DictCursor::NoDrop);

        ASSERT(DictCursor::Arrow == cursors.size());
        cursors.emplace_back(AfxGetApp()->LoadStandardCursor(IDC_ARROW));
    }

    // see which cursor should be used
    CMDIChildWnd* pActiveWnd = assert_cast<CMDIFrameWnd*>(AfxGetMainWnd())->MDIGetActive();
    int iRet = pActiveWnd->SendMessage(UWM::Dictionary::GetTableCursor);

    // set the cursor
    if( iRet >= 0 && iRet < (int)cursors.size() )
    {
        if( iRet != DictCursor::Arrow || !bFromGrid )
            SetCursor(cursors[iRet]);
    }

    return iRet;
}


void CDDTreeCtrl::OnEditRelation()
{
    OnActivatedView([&](CDDGView& view)
    {
        CDDDoc* dictionary_doc = assert_cast<CDDDoc*>(view.GetDocument());
        dictionary_doc->OnEditRelation();
    });
}


void CDDTreeCtrl::OnShiftF10()
{
    PostMessage(WM_RBUTTONUP);
}


void CDDTreeCtrl::OnViewDictionary()
{
    CMDIChildWnd* pActiveWnd = dynamic_cast<CMDIFrameWnd*>(AfxGetMainWnd())->MDIGetActive();
    CDictChildWnd* pDictChildWnd = dynamic_cast<CDictChildWnd*>(pActiveWnd);
    if (pDictChildWnd != nullptr) {
        pDictChildWnd->SendMessage(WM_COMMAND, ID_VIEW_DICTIONARY);
    }
    else {
        // Get document associated with selected item
        HTREEITEM hItem = GetSelectedItem();
        LPARAM lParam = GetItemData(hItem);
        DictTreeNode* dict_tree_node = reinterpret_cast<DictTreeNode*>(lParam);
        CDDDoc* pDoc = dict_tree_node->GetDDDoc();
        if (!pDoc) {
            return;
        }
        //If the view is not activated, activate it
        POSITION pos = pDoc->GetFirstViewPosition();
        ASSERT(pos != nullptr);
        CDDGView* pView = assert_cast<CDDGView*>(pDoc->GetNextView(pos));
        CFrameWnd* pFrame = pView->GetParentFrame();
        pFrame->SendMessage(WM_COMMAND, ID_VIEW_DICTIONARY);
        pFrame->ActivateFrame();
    }

}
void CDDTreeCtrl::OnViewQuestionnaire()
{
    CMDIChildWnd* pActiveWnd = dynamic_cast<CMDIFrameWnd*>(AfxGetMainWnd())->MDIGetActive();
    CDictChildWnd* pDictChildWnd = dynamic_cast<CDictChildWnd*>(pActiveWnd);
    if (pDictChildWnd != nullptr) {
        pDictChildWnd->SendMessage(WM_COMMAND, ID_VIEW_QUESTIONNAIRE);
    }
    else {
        // Get document associated with selected item
        HTREEITEM hItem = GetSelectedItem();
        LPARAM lParam = GetItemData(hItem);
        DictTreeNode* dict_tree_node = reinterpret_cast<DictTreeNode*>(lParam);
        CDDDoc* pDoc = dict_tree_node->GetDDDoc();
        if (!pDoc) {
            return;
        }
        //If the view is not activated, activate it
        POSITION pos = pDoc->GetFirstViewPosition();
        ASSERT(pos != nullptr);
        CDDGView* pView = assert_cast<CDDGView*>(pDoc->GetNextView(pos));
        CFrameWnd* pFrame = pView->GetParentFrame();
        pFrame->SendMessage(WM_COMMAND, ID_VIEW_QUESTIONNAIRE);
        pFrame->ActivateFrame();
    }

}

void CDDTreeCtrl::OnCopyName()
{
    const DictTreeNode* dict_tree_node = GetTreeNode(GetSelectedItem());

    if( dict_tree_node != nullptr )
        WinClipboard::PutText(this, dict_tree_node->GetName());
}


void CDDTreeCtrl::OnDictionaryMacros() // 20101108
{
    CMDIChildWnd* pActiveWnd = assert_cast<CMDIFrameWnd*>(AfxGetMainWnd())->MDIGetActive();

    if( pActiveWnd && pActiveWnd->GetActiveDocument() != nullptr )
    {
        HTREEITEM hItem = GetSelectedItem();
        LPARAM lParam = GetItemData(hItem);
        DictTreeNode* dict_tree_node = reinterpret_cast<DictTreeNode*>(lParam);
        CDDDoc* dictionary_doc = dict_tree_node->GetDDDoc();

        CDictionaryMacros dlg(dictionary_doc);
        dlg.DoModal();

        ReBuildTree(*GetDictionaryTreeNode(*dictionary_doc));
    }
}


void CDDTreeCtrl::OnEditLanguages()
{
    CMDIChildWnd* pActiveWnd = assert_cast<CMDIFrameWnd*>(AfxGetMainWnd())->MDIGetActive();

    if( pActiveWnd == nullptr || pActiveWnd->GetActiveDocument() == nullptr )
        return;

    HTREEITEM hItem = GetSelectedItem();
    LPARAM lParam = GetItemData(hItem);
    DictTreeNode* dict_tree_node = reinterpret_cast<DictTreeNode*>(lParam);
    CDDDoc* dictionary_doc = dict_tree_node->GetDDDoc();

    CDataDict* dictionary = dictionary_doc->GetDict();
    ASSERT(!dictionary->GetLanguages().empty());

    CCapilangDlg dlg;

    for( const Language& language : dictionary->GetLanguages() )
    {
        CLangInfo langInfo;
        langInfo.m_sLangName = WS2CS(language.GetName());
        langInfo.m_sLabel = WS2CS(language.GetLabel());
        dlg.m_Langgrid.m_aLangInfo.Add(langInfo);
    }

    if( dlg.DoModal() != IDOK || !dlg.m_Langgrid.m_bChanged )
        return;

    //First process langs which are modified
    for(int iLangInfo=0; iLangInfo < dlg.m_Langgrid.m_aLangInfo.GetSize(); iLangInfo++) {
        CLangInfo langInfo = dlg.m_Langgrid.m_aLangInfo[iLangInfo];
        if(langInfo.m_eLangInfo == eLANGINFO::MODIFIED_INFO) {
            CString sName = langInfo.m_sLangName;
            sName.Trim();

            dictionary->ModifyLanguage(iLangInfo, Language(CS2WS(sName), CS2WS(langInfo.m_sLabel)));
            dictionary_doc->SetModified(true);
        }
    }

    //Second langs which are ADDED
    for(int iLangInfo=0; iLangInfo < dlg.m_Langgrid.m_aLangInfo.GetSize(); iLangInfo++) {
        CLangInfo langInfo = dlg.m_Langgrid.m_aLangInfo[iLangInfo];
        if(langInfo.m_eLangInfo == eLANGINFO::NEW_INFO) {
            CIMSAString sName = langInfo.m_sLangName;
            sName.Trim();

            dictionary->AddLanguage(Language(CS2WS(sName), CS2WS(langInfo.m_sLabel)));
            dictionary_doc->SetModifiedFlag(true);
        }
    }

    //Finally langs which are deleted
    for(int iLangInfo=0; iLangInfo < dlg.m_Langgrid.m_aLangInfo.GetSize(); iLangInfo++) {
        CLangInfo langInfo = dlg.m_Langgrid.m_aLangInfo[iLangInfo];
        if(langInfo.m_eLangInfo == eLANGINFO::DELETED_INFO) {
            std::optional<size_t> language_index = dictionary->IsLanguageDefined(langInfo.m_sLangName);
            ASSERT(language_index.has_value());

            dictionary->DeleteLanguage(*language_index);
            dictionary_doc->SetModifiedFlag(true);
        }
    }

    //Send a message to mainframe to reset the content and set the combobox with the correct values
    AfxGetMainWnd()->SendMessage(UWM::Dictionary::UpdateLanguageList);
}


CDataDict* CDDTreeCtrl::GetSelectedDictionary(bool requires_saved_dictionary/* = true*/)
{
    CMDIChildWnd* pActiveWnd = assert_cast<CMDIFrameWnd*>(AfxGetMainWnd())->MDIGetActive();

    if( pActiveWnd != nullptr && pActiveWnd->GetActiveDocument() != nullptr )
    {
        HTREEITEM hItem = GetSelectedItem();
        LPARAM lParam = GetItemData(hItem);
        DictTreeNode* dict_tree_node = reinterpret_cast<DictTreeNode*>(lParam);
        CDDDoc* dictionary_doc = dict_tree_node->GetDDDoc();

        if( requires_saved_dictionary && dictionary_doc->IsModified() )
        {
            AfxMessageBox(_T("You must save the dictionary before you can open it in a tool."));
        }

        else
        {
            return dictionary_doc->GetDict();
        }
    }

    return nullptr;
}


void CDDTreeCtrl::OnOpenTool(const UINT nID)
{
    const CDataDict* const dictionary = GetSelectedDictionary();

    if( dictionary == nullptr )
        return;

    const CSProExecutables::Program program =
          ( nID == ID_OPEN_WITH_CSDIFF )      ? CSProExecutables::Program::CSDiff :
          ( nID == ID_OPEN_WITH_EXCEL2CSPRO ) ? CSProExecutables::Program::Excel2CSPro :
          ( nID == ID_OPEN_WITH_CSEXPORT )    ? CSProExecutables::Program::CSExport:
          ( nID == ID_OPEN_WITH_CSFREQ )      ? CSProExecutables::Program::CSFreq :
          ( nID == ID_OPEN_WITH_CSINDEX )     ? CSProExecutables::Program::CSIndex :
        /*( nID == ID_OPEN_WITH_CSSORT ) ? */   CSProExecutables::Program::CSSort;

    CSProExecutables::RunProgramOpeningFile(program, CS2WS(dictionary->GetFullFileName()));
}


void CDDTreeCtrl::OnOpenDataFile(const UINT nID)
{
    const CDataDict* const dictionary = GetSelectedDictionary();
    const size_t view_data_index = nID - ID_OPEN_DATA_FILE0;

    if( dictionary != nullptr && view_data_index < m_viewDataFilenames.size() )
    {
        const std::wstring argument = FormatTextCS2WS(_T("\"%s\" \"%s\""), m_viewDataFilenames[view_data_index].GetString(), dictionary->GetFullFileName().GetString());
        CSProExecutables::RunProgram(CSProExecutables::Program::DataViewer, argument.c_str());
    }
}
