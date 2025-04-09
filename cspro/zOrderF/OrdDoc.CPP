#include "StdAfx.h"
#include "OrdDoc.H"
#include <zUtilO/Specfile.h>


IMPLEMENT_DYNCREATE(COrderDoc, FormFileBasedDoc)

BEGIN_MESSAGE_MAP(COrderDoc, FormFileBasedDoc)
    ON_COMMAND(ID_CUSTOMORDER, OnCustomOrder)
    ON_UPDATE_COMMAND_UI(ID_CUSTOMORDER, OnUpdateCustomOrder)
END_MESSAGE_MAP()


COrderDoc::COrderDoc()
    :   m_orderSpec(*m_formFile),
        m_bOrderLoaded(false),
        m_pOrderTreeCtrl(nullptr),
        m_iCurOrderIndex(NONE),
        m_iCurLevelIndex(NONE),
        m_iCurItemIndex(NONE)
{
    EnableAutomation();

    AfxOleLockApp();
}


COrderDoc::~COrderDoc()
{
    AfxOleUnlockApp();
}


BOOL COrderDoc::OnNewDocument()
{
    if( !CDocument::OnNewDocument() )
        return FALSE;

    CDEFormFile* pOrder = &GetFormFile();

    pOrder->RemoveAllLevels();
    pOrder->RemoveAllForms();   // shld not be here once order clean

    pOrder->AddLevel(new CDELevel());  // automatically creates one group under it
    return TRUE;
}


BOOL COrderDoc::OnOpenDocument(LPCTSTR lpszPathName)
{
    // open up the order spec file to get its label and the input dict file name/path

    CSpecFile specFile(true);  // use a local as don't want to keep this info just yet

    //  read order spec file to get its label
    BOOL success = specFile.Open(lpszPathName, CFile::modeRead);
    specFile.Close();

    if( !success )   // open didn't go ok
        AfxMessageBox(FormatText(L"Opening %s failed", lpszPathName));

    return success;
}


void COrderDoc::DeleteContents()
{
    if( m_pOrderTreeCtrl == nullptr )
        return;

    FormOrderAppTreeNode* const form_order_app_tree_node = m_pOrderTreeCtrl->GetFormOrderAppTreeNode(*this);   //Get the item CFormNodeItem  corresponding to this pDoc

    if( form_order_app_tree_node->GetRefCount() > 0 )
        m_pOrderTreeCtrl->ReleaseDoc(*form_order_app_tree_node);

    SetCurOrderIndex(NONE);
    SetCurLevelIndex(NONE);
    SetCurItemIndex(NONE);

    CDocument::DeleteContents();
}


COSourceEditView* COrderDoc::GetView()
{
    POSITION pos = GetFirstViewPosition();

    if( pos != nullptr )
        return assert_cast<COSourceEditView*>(GetNextView(pos));

    return nullptr;
}


HTREEITEM COrderDoc::BuildAllTrees(HTREEITEM /*hParent = TVI_ROOT*/, HTREEITEM /*hInsertAfter = TVI_LAST*/)
{
    // BuildAllTrees is invoked from CSPro
    // smg: why are there args when we're ! using?
    TVITEM      pItem;
    HTREEITEM   hItem = nullptr;

    // Get the tree control handles and dict path & label
    COrderTreeCtrl* pOrderTree = GetOrderTreeCtrl();
    CDDTreeCtrl*    pDictTree = pOrderTree->GetDDTreeCtrl();

    CDEFormFile*    pOrderSpec = &GetFormFile();
    CString         csPath = pOrderSpec->GetDictionaryFilename();

    //  either add a node to the dict tree, or add a reference
    pDictTree->AddDictionary(UTF8_TODO::GetUtf8(csPath), nullptr);

    //  either add a node to the order tree, or add a ref
    csPath = GetPathName();             // mfc; rtns the document’s fully qualified path

    FormOrderAppTreeNode* const form_order_app_tree_node = pOrderTree->GetFormOrderAppTreeNode(*this);

    if( form_order_app_tree_node != nullptr )
    {
        form_order_app_tree_node->AddRef();
        hItem = form_order_app_tree_node->GetHItem();
    }

    else
    {
        hItem = pOrderTree->InsertOrderFile(csPath, this);

        pItem.hItem = hItem;
        pItem.mask  = TVIF_CHILDREN;
        pItem.cChildren = 1;

        pOrderTree->SetItem(&pItem);        // smg: what exactly does this do, vs. using InsertItem
    }

    return hItem;
}


bool COrderDoc::InitTreeCtrl()
{
    // build up the form tree ctrl only after we know the loading of the form spec file went ok
    ASSERT(m_pOrderTreeCtrl != nullptr);

    FormOrderAppTreeNode* const form_order_app_tree_node = m_pOrderTreeCtrl->GetFormOrderAppTreeNode(*this);

    m_pOrderTreeCtrl->BuildTree(form_order_app_tree_node);

    return true;
}


bool COrderDoc::LoadOrderSpecFile(const InterfaceString& order_file_path)
{
    if( !m_orderSpec.Open(order_file_path) )       // load and create CDEFormFile here
        return false;

    m_bOrderLoaded = true;  // indicate that we've built the CDEFormFile

    int i = m_orderSpec.GetNumLevels(); // will rtn # items

    if (i > 0)          // as long as there's at least one level,
    {
        SetCurLevelIndex (0);           // then set the 1st as the curr one
//      GetView()->SetLevelIndex (0);
    }

    i = m_orderSpec.GetNumForms();  // eventually won't be doing this, no forms in an .ord file

    if (i > 0)
    {
        SetCurOrderIndex (0);
//      GetView()->SetOrderIndex (0);
    }

    return true;
}


bool COrderDoc::LoadDictSpecFile()
{
    COrderTreeCtrl* pOrderTree  = GetOrderTreeCtrl();

    if (pOrderTree == nullptr)
        return false;

    CDDTreeCtrl* pDictTree = pOrderTree->GetDDTreeCtrl();

    if (pDictTree == nullptr)
        return false;

    CDEFormFile*    pOrderSpec = &GetFormFile();
    bool            bOK = true;
    CString         csDictPath = pOrderSpec->GetDictionaryFilename();

    DictionaryDictTreeNode* const dictionary_dict_tree_node = pDictTree->GetDictionaryTreeNode(UTF8_TODO::GetUtf8(csDictPath));  // get the node assoc w/this dict

    if (dictionary_dict_tree_node == nullptr)    // make sure the node's ok
    {
        bOK = false;
    }

    else
    {
        if(dictionary_dict_tree_node->GetDDDoc() == nullptr)    // has the dictionary already been opened/assigned?
        {
            if (!pDictTree->OpenDictionary(UTF8_TODO::GetUtf8(csDictPath), false) )    // if not, open it!
                bOK = false;        // if the open didn't go ok, flag it
        }
    }

    if (bOK)  {
        pOrderSpec->SetDictionaryName(dictionary_dict_tree_node->GetDDDoc()->GetDict()->GetName());
        pOrderSpec->SetDictionary(dictionary_dict_tree_node->GetDDDoc()->GetSharedDictionary());
    }

    return bOK;
}


CTreeCtrl* COrderDoc::GetTreeCtrl()
{
    return GetOrderTreeCtrl();
}


std::shared_ptr<const CDataDict> COrderDoc::GetSharedDictionary() const
{
    COrderTreeCtrl* pOrderTree = GetOrderTreeCtrl();

    if (pOrderTree == nullptr)
        return nullptr;           // err condition, will need to flag better

    CDDTreeCtrl* pDictTree = pOrderTree->GetDDTreeCtrl();

    if (pDictTree == nullptr)
        return nullptr;            // ditto comment above

    const CDEFormFile* pOrder = &GetFormFile();

    CString csDictPath = pOrder->GetDictionaryFilename();

    if (csDictPath.IsEmpty())
        return nullptr;            // ditto comment above

    DictionaryDictTreeNode* const dictionary_dict_tree_node = pDictTree->GetDictionaryTreeNode(UTF8_TODO::GetUtf8(csDictPath));

    if( dictionary_dict_tree_node == nullptr )
        return nullptr;            // ditto comment above

    if( dictionary_dict_tree_node->GetDDDoc() != nullptr ) // there's a doc assoc w/the node
    {
        return dictionary_dict_tree_node->GetDDDoc()->GetSharedDictionary();
    }

    else if( pDictTree->OpenDictionary(UTF8_TODO::GetUtf8(csDictPath), false) ) // open the dict if not already done
    {
        return dictionary_dict_tree_node->GetDDDoc()->GetSharedDictionary();
    }

    return nullptr;
}


BOOL COrderDoc::OnSaveDocument(LPCTSTR lpszPathName)
{
    SaveAllDictionaries();
    bool bOK = m_orderSpec.Save(lpszPathName);

    if (bOK)
        SetModifiedFlag(false);

    return bOK;
}


BOOL COrderDoc::SaveModified()
{
    if (!IsModified())
        return true;

    CString name = m_strPathName;

    if (name.IsEmpty())
        VERIFY (name.LoadString(AFX_IDS_UNTITLED));

    CString prompt;
    AfxFormatString1(prompt, AFX_IDP_ASK_TO_SAVE, name);

    switch (AfxMessageBox(prompt, MB_YESNOCANCEL, AFX_IDP_ASK_TO_SAVE))
    {
        case IDCANCEL: return false;       // don't continue
        case IDYES   :
                        {// If so, either Save or Update, as appropriate
                         SaveAllDictionaries();
                        if (!DoSave (m_strPathName))
                            return false;       // don't continue
                        }
                        break;
        case IDNO    : // If not saving changes, revert the document
                        break;
        default      : ASSERT(false);
                        break;
    }
    return true;    // keep going

//  return CDocument::SaveModified();   // mfc-gen, don't use
}


void COrderDoc::OnCloseDocument()
{
    m_bOrderLoaded = false;

    CDocument::OnCloseDocument();
}


void COrderDoc::ReleaseDicts()
{
    COrderTreeCtrl* const pOrderTree = GetOrderTreeCtrl();

    if( pOrderTree == nullptr )
        return;

    CDDTreeCtrl* const pDictTree = pOrderTree->GetDDTreeCtrl();

    if( pDictTree == nullptr )
        return;

    CDEFormFile* const pOrderSpec = &GetFormFile();
    DictionaryDictTreeNode* const dictionary_dict_tree_node = pDictTree->GetDictionaryTreeNode(UTF8_TODO::GetUtf8(pOrderSpec->GetDictionaryFilename()));

    if( dictionary_dict_tree_node != nullptr )
        pDictTree->ReleaseDictionaryNode(*dictionary_dict_tree_node);
}


void COrderDoc::SaveAllDictionaries()
{
    COrderTreeCtrl* pOrderTree  = GetOrderTreeCtrl();
    if (pOrderTree == nullptr)
        return;

    CDDTreeCtrl* pDictTree = pOrderTree->GetDDTreeCtrl();

    // get at the dictionary tree and get the documents
    DictionaryDictTreeNode* const dictionary_dict_tree_node = pDictTree->GetDictionaryTreeNode(UTF8_TODO::GetUtf8(m_orderSpec.GetDictionaryFilename()));
    if( dictionary_dict_tree_node != nullptr && dictionary_dict_tree_node->GetDDDoc() != nullptr ) {
        if(dictionary_dict_tree_node->GetDDDoc()->IsModified()){
            dictionary_dict_tree_node->GetDDDoc()->OnSaveDocument(dictionary_dict_tree_node->GetDDDoc()->GetPathName());
        }
    }
}


BOOL COrderDoc::IsOrderModified()
{
    if( IsModified() )
        return TRUE;

    COrderTreeCtrl* const pOrderTree  = GetOrderTreeCtrl();

    if( pOrderTree == nullptr )
        return FALSE;

    CDDTreeCtrl* const pDictTree = pOrderTree->GetDDTreeCtrl();
    DictionaryDictTreeNode* const dictionary_dict_tree_node = pDictTree->GetDictionaryTreeNode(UTF8_TODO::GetUtf8(m_orderSpec.GetDictionaryFilename()));

    if( dictionary_dict_tree_node != nullptr && dictionary_dict_tree_node->GetDDDoc() != nullptr )
        return dictionary_dict_tree_node->GetDDDoc()->IsModified();

    return FALSE;
}


void COrderDoc::OnCustomOrder()
{
    CString csMsg;
    if (m_orderSpec.IsDictOrder()) {
        csMsg  = _T("From now on, you will be able to change the order of procedures for fields and records.\n");
        csMsg += _T("You can drag and drop on the tree on the left to do so.\n");
        csMsg += _T("Current order of procedures WILL NOT CHANGE.\n\n");
        csMsg += _T("Do you want to continue?");
        if (AfxMessageBox(csMsg, MB_YESNO | MB_DEFBUTTON2) == IDYES) {
            m_orderSpec.SetDictOrder(false);
            SetModifiedFlag(true);
        }
    }
    else {
        csMsg  = _T("From now on, the order of procedures for fields and records will be the same as the dictionary.\n");
        csMsg += _T("Current order of procedures WILL BE CHANGED to match the dictionary.\n\n");
        csMsg += _T("Do you want to continue?");
        if (AfxMessageBox(csMsg, MB_YESNO | MB_DEFBUTTON2) == IDYES) {
            m_orderSpec.SetDictOrder(true);
            const CDataDict* pDD = m_orderSpec.GetDictionary();
            CString csErr;
            COrderTreeCtrl* pTree = GetOrderTreeCtrl();
            pTree->SelectItem (pTree->GetRootItem());
            pTree->SetRedraw(FALSE);

            m_orderSpec.CreateOrderFile(*pDD, true);

            if (true){
                pTree->SetSndMsgFlg(FALSE);
                pTree->SelectItem (pTree->GetRootItem());
                pTree->ReBuildTree(0,NULL,false);
                pTree->SetRedraw(TRUE);
                pTree->SetSndMsgFlg(TRUE);
                AfxGetMainWnd()->SendMessage(UWM::Order::ShowSourceCode, 0, (LPARAM)this);
            }
            pTree->SetRedraw(TRUE);
            SetModifiedFlag(true);
        }
    }
    if (m_orderSpec.IsDictOrder()) {
        AfxGetMainWnd()->SendMessage(WM_IMSA_SET_STATUSBAR_PANE, (WPARAM)_T("Dictionary Order"));
    }
    else {
        AfxGetMainWnd()->SendMessage(WM_IMSA_SET_STATUSBAR_PANE, (WPARAM)_T("Custom Order"));
    }
}


void COrderDoc::OnUpdateCustomOrder(CCmdUI* pCmdUI)
{
    pCmdUI->SetCheck(!m_orderSpec.IsDictOrder());
}
