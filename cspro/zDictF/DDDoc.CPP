#include "StdAfx.h"
#include "Dddoc.h"
#include "DDLView.h"
#include "RelDlg.h"
#include "SecurityOptionsDlg.h"


IMPLEMENT_DYNCREATE(CDDDoc, DictionaryBasedDoc)

BEGIN_MESSAGE_MAP(CDDDoc, DictionaryBasedDoc)
    ON_COMMAND(ID_OPTIONS_ABS_REL, OnOptionsAbsRel)
    ON_COMMAND(ID_OPTIONS_ZEROFILL, OnOptionsZeroFill)
    ON_COMMAND(ID_OPTIONS_DECCHAR, OnOptionsDecChar)
    ON_COMMAND(ID_EDIT_RELATION, OnEditRelation)
    ON_COMMAND(ID_EDIT_LANGUAGES, OnEditLanguages)
    ON_COMMAND(ID_DICTIONARY_MACROS, OnDictionaryMacros)
    ON_COMMAND(ID_EDIT_SECURITY_OPTIONS, OnEditSecurityOptions)
    ON_COMMAND(ID_OPTIONS_ENABLE_BINARY_ITEMS, OnEnableBinaryItems)
END_MESSAGE_MAP()


CDDDoc::CDDDoc()
    :   m_dictionary(std::make_shared<CDataDict>()),
        m_pTreeCtrl(nullptr),
        m_pGrid(nullptr),
        m_iLevel(NONE),
        m_iRec(NONE),
        m_iItem(NONE),
        m_bPrintPreview(false),
        m_dictClipboard(std::make_unique<DictClipboard>(*this))
{
    EnableAutomation();
    AfxOleLockApp();
}


CDDDoc::~CDDDoc()
{
    AfxOleUnlockApp();
}


CTreeCtrl* CDDDoc::GetTreeCtrl()
{
    return GetDictTreeCtrl();
}


std::shared_ptr<const CDataDict> CDDDoc::GetSharedDictionary() const
{
    return m_dictionary;
}


/////////////////////////////////////////////////////////////////////////////
//
//                        CDDDoc::OnOpenDocument
//
/////////////////////////////////////////////////////////////////////////////

BOOL CDDDoc::OnOpenDocument(LPCTSTR lpszPathName)
{
    //  Open dictionary file for read/write
    //  If not OK
    //      Set doc flag read only
    //      Open dictionary file for read only
    //  If OK
    //      Do rule manager stuff
    //
    try
    {
        m_dictionary->Open(lpszPathName);

        SetPathName(lpszPathName);
        GetDictionaryValidator()->SetCurrDict(GetDict());

        if( m_dictionary->IsPosRelative() )
            GetDictionaryValidator()->AdjustStartPositions();
        
        if( !GetDictionaryValidator()->IsValid(GetDict(), FALSE, TRUE) )
        {
            // Error found and corrected
            SetModified();                                    
        }

        GetDictionaryValidator()->IsValidSave(*m_dictionary);
        m_dictionary->UpdatePointers();

        return TRUE;
    }

    catch( const CSProException& exception )
    {
        ErrorMessage::Display(exception);
        return FALSE;
    }
}


/////////////////////////////////////////////////////////////////////////////
//
//                        CDDDoc::DeleteContents
//
/////////////////////////////////////////////////////////////////////////////

void CDDDoc::DeleteContents()
{
    if(!m_pTreeCtrl) {
        return;
    }
    //Get the item CDictNodeItem  corresponding to this pDoc ;
    DictionaryDictTreeNode* dictionary_dict_tree_node = m_pTreeCtrl->GetDictionaryTreeNode(*this);
    if (dictionary_dict_tree_node == nullptr) {    // BMD 24 May 2006
        return;
    }
    if(dictionary_dict_tree_node->GetRefCount() > 0) {
        m_pTreeCtrl->ReleaseDoc(*dictionary_dict_tree_node);
    }

    m_dictionary = std::make_shared<CDataDict>();
    CDocument::DeleteContents();
}


/////////////////////////////////////////////////////////////////////////////
//
//                        CDDDoc::OnInitTreeCtrl
//
/////////////////////////////////////////////////////////////////////////////

bool CDDDoc::InitTreeCtrl()
{
    ASSERT(m_pTreeCtrl);
    DictionaryDictTreeNode* dictionary_dict_tree_node = m_pTreeCtrl->GetDictionaryTreeNode(*this);
    m_pTreeCtrl->BuildTree(*dictionary_dict_tree_node);
    return TRUE;
}


/////////////////////////////////////////////////////////////////////////////
//
//                        CDDDoc::OnSaveDocument
//
/////////////////////////////////////////////////////////////////////////////

BOOL CDDDoc::OnSaveDocument(LPCTSTR lpszPathName)
{
    if( !IsDocOK(false) )
        return FALSE;

    CWaitCursor wait;
    bool success = GetDictionaryValidator()->IsValidSave(*m_dictionary);

    try
    {
        m_dictionary->UpdatePointers();
        m_dictionary->Save(lpszPathName);
        SetModified(FALSE);
    }

    catch( const CSProException& )
    {
        success = false;
    }

    return success;
}


/////////////////////////////////////////////////////////////////////////////
//
//                        CDDDoc::IsDocOK
//
/////////////////////////////////////////////////////////////////////////////

bool CDDDoc::IsDocOK(bool bSilent)
{
    return assert_cast<CDDGrid*>(m_pGrid)->IsEditing() ? assert_cast<CDDGrid*>(m_pGrid)->EditEnd(bSilent) : true;
}


/////////////////////////////////////////////////////////////////////////////
//
// UNDO/REDO FUNCTIONS
//
/////////////////////////////////////////////////////////////////////////////

void CDDDoc::PushUndo(const DictNamedBase& dict_element, int iCurLevel/* = NONE*/, int iCurRec/* = NONE*/, int iCurItem/* = NONE*/, int iCurVSet/* = NONE*/, int iCurRow/* = NONE*/)
{
    m_undoStack.PushUndo(DDUndoObject(dict_element.Clone(), iCurLevel, iCurRec, iCurItem, iCurVSet, iCurRow));
    m_undoStack.ClearRedo();
}


DDUndoObject CDDDoc::DuplicateUndoObject(const DDUndoObject& undo_object)
{
    std::unique_ptr<DictNamedBase> dict_element;
        
    if( undo_object.m_pUndoObject->GetElementType() == DictElementType::Dictionary )
    {
        dict_element = m_dictionary->Clone();
    }

    else if( undo_object.m_pUndoObject->GetElementType() == DictElementType::Level )
    {
        dict_element = m_dictionary->GetLevel(undo_object.m_iCurLevel).Clone();
    }

    else if( undo_object.m_pUndoObject->GetElementType() == DictElementType::Record )
    {
        dict_element = m_dictionary->GetLevel(undo_object.m_iCurLevel).GetRecord(undo_object.m_iCurRec)->Clone();
    }

    else
    {
        ASSERT(undo_object.m_pUndoObject->GetElementType() == DictElementType::Item);
        dict_element = m_dictionary->GetLevel(undo_object.m_iCurLevel).GetRecord(undo_object.m_iCurRec)->GetItem(undo_object.m_iCurItem)->Clone();
    }

    return DDUndoObject(std::move(dict_element), undo_object.m_iCurLevel, undo_object.m_iCurRec, undo_object.m_iCurItem, undo_object.m_iCurVSet, undo_object.m_iCurRow);
}


void CDDDoc::UndoChange(bool bCanRedo)
{
    // Get object off the undo stack
    ASSERT(m_undoStack.CanUndo());
    DDUndoObject undo_object = m_undoStack.Undo();

    // Put corresponding object on the redo stack if necessary
    if( bCanRedo )
        m_undoStack.PushRedo(DuplicateUndoObject(undo_object));

    // Perform undo change
    PerformUndoChange(std::move(undo_object));
}


void CDDDoc::RedoChange()
{
    ASSERT(m_undoStack.CanRedo());
    DDUndoObject redo_object = m_undoStack.Redo();

    // Push current on undo stack
    m_undoStack.PushUndo(DuplicateUndoObject(redo_object));

    // Perform redo change
    PerformUndoChange(std::move(redo_object));
}


void CDDDoc::PerformUndoChange(DDUndoObject undo_object)
{
    POSITION pos = GetFirstViewPosition();
    ASSERT(pos != nullptr);
    CDDGView* pView = assert_cast<CDDGView*>(GetNextView(pos));

    DictNamedBase* dict_element = undo_object.m_pUndoObject.get();

    if( dict_element->GetElementType() == DictElementType::Dictionary )
    {
        *m_dictionary = *assert_cast<CDataDict*>(dict_element);
    }

    else if( dict_element->GetElementType() == DictElementType::Level )
    {
        ASSERT((int)m_dictionary->GetNumLevels() > undo_object.m_iCurLevel);
        m_dictionary->RemoveLevel(undo_object.m_iCurLevel);
        DictLevel* dict_level = assert_cast<DictLevel*>(dict_element);
        m_dictionary->InsertLevel(undo_object.m_iCurLevel, std::move(*dict_level));
    }

    else if( dict_element->GetElementType() == DictElementType::Record )
    {
        ASSERT(undo_object.m_iCurLevel < (int)m_dictionary->GetNumLevels());
        DictLevel& dict_level = m_dictionary->GetLevel(undo_object.m_iCurLevel);
        ASSERT(undo_object.m_iCurRec < dict_level.GetNumRecords());
        dict_level.RemoveRecordAt(undo_object.m_iCurRec);
        CDictRecord* pRec = assert_cast<CDictRecord*>(dict_element);
        dict_level.InsertRecordAt(undo_object.m_iCurRec, pRec);
    }

    else
    {
        ASSERT(dict_element->GetElementType() == DictElementType::Item);
        ASSERT(undo_object.m_iCurLevel < (int)m_dictionary->GetNumLevels());
        DictLevel& dict_level = m_dictionary->GetLevel(undo_object.m_iCurLevel);
        ASSERT(undo_object.m_iCurRec < dict_level.GetNumRecords());
        CDictRecord* pRec = dict_level.GetRecord(undo_object.m_iCurRec);
        ASSERT(undo_object.m_iCurItem < pRec->GetNumItems());
        pRec->RemoveItemAt(undo_object.m_iCurItem);
        CDictItem* pItem = assert_cast<CDictItem*>(dict_element);
        pRec->InsertItemAt(undo_object.m_iCurItem, pItem);
    }

    m_dictionary->BuildNameList();
    m_dictionary->UpdatePointers();
    GetDictionaryValidator()->IsValid(m_dictionary.get(), TRUE, TRUE);
    SetModified(true);

    DictionaryDictTreeNode* dictionary_dict_tree_node = m_pTreeCtrl->GetDictionaryTreeNode(*this);

    if (undo_object.m_iCurRec == NONE) {
        m_pTreeCtrl->ReBuildTree(*dictionary_dict_tree_node);
        pView->m_iGrid = DictionaryGrid::Dictionary;
        pView->m_gridDict.SetRedraw(FALSE);
        pView->m_gridDict.Update(m_dictionary.get());
        pView->m_gridDict.ClearSelections();
        if (undo_object.m_iCurLevel != NONE) {
            pView->m_gridDict.GotoRow(undo_object.m_iCurLevel + CDictGrid::GetFirstLevelRow());
        }
        pView->m_gridDict.SetRedraw(TRUE);
        pView->m_gridDict.InvalidateRect(nullptr);
        pView->m_gridDict.SetFocus();
    }

    else if (undo_object.m_iCurItem == NONE) {
        m_pTreeCtrl->ReBuildTree(*dictionary_dict_tree_node, undo_object.m_iCurLevel);
        pView->m_iGrid = DictionaryGrid::Level;
        pView->m_gridLevel.SetRedraw(FALSE);
        pView->m_gridLevel.Update(m_dictionary.get(), undo_object.m_iCurLevel);
        pView->m_gridLevel.ClearSelections();
        if (undo_object.m_iCurRec != NONE) {
            pView->m_gridLevel.GotoRow(pView->m_gridLevel.GetFirstRow() + undo_object.m_iCurRec);
        }
        pView->m_gridLevel.SetRedraw(TRUE);
        pView->m_gridLevel.InvalidateRect(nullptr);
        pView->m_gridLevel.SetFocus();
    }

    else if (undo_object.m_iCurVSet == NONE) {
        m_pTreeCtrl->ReBuildTree(*dictionary_dict_tree_node, undo_object.m_iCurLevel, undo_object.m_iCurRec);
        pView->m_iGrid = DictionaryGrid::Record;
        pView->m_gridRecord.SetRedraw(FALSE);
        pView->m_gridRecord.Update(m_dictionary.get(), undo_object.m_iCurLevel, undo_object.m_iCurRec);
        pView->m_gridRecord.ClearSelections();
        pView->m_gridRecord.GotoRow(pView->m_gridRecord.GetRow(undo_object.m_iCurItem));
        pView->m_gridRecord.SetRedraw(TRUE);
        pView->m_gridRecord.InvalidateRect(nullptr);
        pView->m_gridRecord.SetFocus();
    }

    else {
        m_pTreeCtrl->ReBuildTree(*dictionary_dict_tree_node, undo_object.m_iCurLevel, undo_object.m_iCurRec, undo_object.m_iCurItem);
        pView->m_iGrid = DictionaryGrid::Item;
        pView->m_gridItem.SetRedraw(FALSE);
        pView->m_gridItem.Update(m_dictionary.get(), undo_object.m_iCurLevel, undo_object.m_iCurRec, undo_object.m_iCurItem, 0);
        pView->m_gridItem.ClearSelections();
        if (undo_object.m_iCurItem!= NONE) {
            pView->m_gridItem.GotoRow(undo_object.m_iCurRow);
        }
        pView->m_gridItem.SetRedraw(TRUE);
        pView->m_gridItem.InvalidateRect(nullptr);
        pView->m_gridItem.SetFocus();
    }
}


/////////////////////////////////////////////////////////////////////////////
//
//                        CDDDoc::OnOptionsAbsRel
//
/////////////////////////////////////////////////////////////////////////////

void CDDDoc::OnOptionsAbsRel()
{
    bool change_to_relative = !m_dictionary->IsPosRelative();

    if( AfxMessageBox(change_to_relative ? IDS_OPTIONS_REL_WARN : IDS_OPTIONS_ABS_WARN , MB_YESNO | MB_DEFBUTTON2) != IDYES )
        return;

    PushUndoCurrent();
    SetModified(true);

    m_dictionary->SetPosRelative(change_to_relative);

    if( change_to_relative )
    {
        GetDictionaryValidator()->AdjustStartPositions();
        UpdateAllViews(nullptr);
    }

    AfxGetMainWnd()->SendMessage(WM_IMSA_SET_STATUSBAR_PANE, (WPARAM)( change_to_relative ? _T("Relative Positions") : _T("Absolute Positions") ));
}


/////////////////////////////////////////////////////////////////////////////
//
//                            CDDDoc::OnOptionsZeroFill
//
/////////////////////////////////////////////////////////////////////////////

void CDDDoc::OnOptionsZeroFill()
{
    PushUndoCurrent();
    SetModified(true);

    bool new_zero_fill = !m_dictionary->IsZeroFill();
    m_dictionary->SetZeroFill(new_zero_fill);

    const TCHAR* text_setting = new_zero_fill ? _T("Yes") : _T("No");
    CString message = FormatText(_T("ZeroFill default set to '%s'.\n\nDo you want to change ZeroFill to '%s' in all existing numeric items?"),
                                 text_setting, text_setting);

    if( AfxMessageBox(message, MB_YESNO | MB_DEFBUTTON2) == IDYES )
    {
        DictionaryIterator::Foreach<CDictItem>(*m_dictionary,
            [&](CDictItem& dict_item)
            {
                if( dict_item.GetContentType() == ContentType::Numeric )
                    dict_item.SetZeroFill(new_zero_fill);
            });

        UpdateAllViews(nullptr);
    }
}


/////////////////////////////////////////////////////////////////////////////
//
//                            CDDDoc::OnOptionsDecChar
//
/////////////////////////////////////////////////////////////////////////////

void CDDDoc::OnOptionsDecChar()
{
    PushUndoCurrent();
    SetModified(true);

    bool new_dec_char = !m_dictionary->IsDecChar();
    m_dictionary->SetDecChar(new_dec_char);

    const TCHAR* text_setting = new_dec_char ? _T("Yes") : _T("No");
    CString message = FormatText(_T("DecChar default set to '%s'.\n\nDo you want to change DecChar to '%s' in all existing decimal items?"),
                                 text_setting, text_setting);

    if( AfxMessageBox(message, MB_YESNO | MB_DEFBUTTON2) == IDYES )
    {
        DictionaryIterator::Foreach<CDictItem>(*m_dictionary,
            [&](CDictItem& dict_item)
            {
                if( dict_item.GetContentType() == ContentType::Numeric && dict_item.GetDecimal() > 0 )
                {
                    bool evaluated_dec_char = ( new_dec_char && dict_item.GetLen() > 1 );
                    dict_item.SetDecChar(evaluated_dec_char);
                }
            });

        UpdateAllViews(nullptr);
    }
}


/////////////////////////////////////////////////////////////////////////////
//
//                            CDDDoc::IsEditing
//
/////////////////////////////////////////////////////////////////////////////

bool CDDDoc::IsEditing()
{
    POSITION pos = GetFirstViewPosition();
    ASSERT(pos != nullptr);
    CDDGView* pView = assert_cast<CDDGView*>(GetNextView(pos));

    return pView->GetCurrentGrid().IsEditing();
}


/////////////////////////////////////////////////////////////////////////////
//
//                            CDDDoc::PushUndoCurrent
//
/////////////////////////////////////////////////////////////////////////////

void CDDDoc::PushUndoCurrent()
{
    POSITION pos = GetFirstViewPosition();
    ASSERT(pos != nullptr);
    CDDGView* pView = assert_cast<CDDGView*>(GetNextView(pos));

    if (pView->m_iGrid == DictionaryGrid::Dictionary) {
        PushUndo(*GetDict(), m_iLevel);
    }
    else if (pView->m_iGrid == DictionaryGrid::Level) {
        PushUndo(*GetDict(), m_iLevel, m_iRec);
    }
    else if (pView->m_iGrid == DictionaryGrid::Record) {
        PushUndo(*GetDict(), m_iLevel, m_iRec, m_iItem);
    }
    else if (pView->m_iGrid == DictionaryGrid::Item) {
        PushUndo(*GetDict(), m_iLevel, m_iRec, m_iItem);
    }
}

/////////////////////////////////////////////////////////////////////////////
//
//                            CDDDoc::SetModified
//
/////////////////////////////////////////////////////////////////////////////

void CDDDoc::SetModified(bool bModified/* = true*/)
{
    SetModifiedFlag(bModified);

    if (bModified) {
        AfxGetMainWnd()->PostMessage(WM_IMSA_UPDATE_SYMBOLTBL, reinterpret_cast<WPARAM>(this));
        POSITION pos = GetFirstViewPosition();
        ASSERT(pos != nullptr);
        GetNextView(pos);
        CDDLView* pView = (CDDLView*)GetNextView(pos);
        pView->Invalidate();
    }
}


////////////////////////////////////////////////////////////////////
//
//                        CDDDoc::OnEditRelation
//
////////////////////////////////////////////////////////////////////

void CDDDoc::OnEditRelation()
{
    CRelDlg dlg;
    dlg.m_pDoc = this;

    if( dlg.DoModal() != IDOK )
        return;

    CDDTreeCtrl* pTreeCtrl = GetDictTreeCtrl();
    pTreeCtrl->GetRootItem();
    HTREEITEM hItem = pTreeCtrl->GetRootItem();
    DictionaryDictTreeNode* dictionary_dict_tree_node = reinterpret_cast<DictionaryDictTreeNode*>(pTreeCtrl->GetItemData(hItem));
    pTreeCtrl->ReBuildTree(*dictionary_dict_tree_node);
}

////////////////////////////////////////////////////////////////////
//
//                        CDDDoc::GetPrimSecNames
//
////////////////////////////////////////////////////////////////////

std::vector<CString> CDDDoc::GetPrimSecNames() const
{
    std::vector<CString> prim_sec_names;

    for( const DictLevel& dict_level : m_dictionary->GetLevels() )
    {
        for (int r = 0; r < dict_level.GetNumRecords(); r++) {
            const CDictRecord* pRecord = dict_level.GetRecord(r);
            if (pRecord->GetMaxRecs() > 1) {
                prim_sec_names.emplace_back(pRecord->GetName());
            }
            else {
                for (int i = 0; i < pRecord->GetNumItems();i++) {
                    const CDictItem* pItem = pRecord->GetItem(i);
                    if (pItem->GetOccurs() > 1) {
                        prim_sec_names.emplace_back(pRecord->GetName() + _T(".") + pItem->GetName());
                    }
                }
            }
        }
    }

    return prim_sec_names;
}

////////////////////////////////////////////////////////////////////
//
//                     CDDDoc::GetRelLinksList
//
////////////////////////////////////////////////////////////////////

bool CDDDoc::GetRelLinksList(const CString& sRelObj, CStringArray& saLinksList)
{
    saLinksList.RemoveAll();
    saLinksList.Add(REL_OCC);

    const CDictRecord* dict_record;
    const CDictItem* dict_item;

    CString csname = sRelObj.Right(sRelObj.GetLength() - sRelObj.ReverseFind('.') - 1);
    m_dictionary->LookupName(csname, nullptr, &dict_record, &dict_item);

    if (dict_item != nullptr) {
        for (int i = 0; i < dict_record->GetNumItems(); i++) {
            const CDictItem* pTempItem = dict_record->GetItem(i);
            if (pTempItem->GetItemType() == ItemType::Subitem && pTempItem->GetParentItem() == dict_item) {
                saLinksList.Add(pTempItem->GetName());
            }
        }
    }
    else if (dict_record != nullptr) {
        for (int i = 0; i < dict_record->GetNumItems(); i++) {
            const CDictItem* pTempItem = dict_record->GetItem(i);
            if (pTempItem->GetItemType() == ItemType::Item && pTempItem->GetOccurs() == 1) {
                saLinksList.Add(pTempItem->GetName());
            }
        }
    }
    return true;
}



void CDDDoc::OnEditLanguages()
{
    m_pTreeCtrl->SendMessage(WM_COMMAND, ID_EDIT_LANGUAGES);
}


void CDDDoc::OnDictionaryMacros()
{
    m_pTreeCtrl->SendMessage(WM_COMMAND, ID_DICTIONARY_MACROS);
}


void CDDDoc::OnEditSecurityOptions()
{
    SecurityOptionsDlg security_options_dlg(*m_dictionary);

    if( security_options_dlg.DoModal() != IDOK )
        return;

    m_dictionary->SetAllowDataViewerModifications(security_options_dlg.GetAllowDataViewerModifications());
    m_dictionary->SetAllowExport(security_options_dlg.GetAllowExport());
    m_dictionary->SetCachedPasswordMinutes(security_options_dlg.GetCachedPasswordMinutes());

    SetModified();
}


void CDDDoc::OnEnableBinaryItems()
{
    if( m_dictionary->EnableBinaryItems() )
    {
        std::vector<std::wstring> binary_items;

        DictionaryIterator::Foreach<CDictItem>(*m_dictionary,
            [&](const CDictItem& dict_item)
            {
                if( IsBinary(dict_item) )
                    binary_items.emplace_back(dict_item.GetName());
            });

        if( !binary_items.empty() )
        {
            ErrorMessage::Display(_T("You cannot disable binary items because the following items use binary data:\n\n") +
                                  SO::CreateSingleString(binary_items, _T("\n")));
            return;
        }
    }

    m_dictionary->EnableBinaryItems(!m_dictionary->EnableBinaryItems());
}
