#include "StdAfx.h"
#include "FormDoc.h"
#include "QSFEView.h"
#include "QSFCndVw.h"
#include <zCapiO/QSFView.h>
#include <zDesignerF/QuestionnaireView.h>

IMPLEMENT_DYNCREATE(CFormDoc, FormFileBasedDoc)

BEGIN_MESSAGE_MAP(CFormDoc, FormFileBasedDoc)
    ON_COMMAND(ID_GENERATE_FRM, OnGenerateFrm)
END_MESSAGE_MAP()


/////////////////////////////////////////////////////////////////////////////
// CFormDoc construction/destruction

CFormDoc::CFormDoc()
    :   m_formSpec(*m_formFile),
        m_bFormLoaded(false),
        m_pFormTreeCtrl(nullptr),
        m_iCurFormIndex(NONE)
{
    EnableAutomation();

    AfxOleLockApp();

    TCHAR acPath[_MAX_PATH];
    GetTempPath(_MAX_PATH, acPath);
    m_csClipFile = acPath;
    m_csClipFile += IMSA_FORM_CLIPFILE;

    m_auFormat[FD_TEXT_FORMAT] = RegisterClipboardFormat(_T("CSPro DD Text"));
    m_auFormat[FD_COLUMN_FORMAT]   = RegisterClipboardFormat(_T("CSPro DD Column"));
    m_auFormat[FD_BOX_FORMAT]  = RegisterClipboardFormat(_T("CSPro DD Box"));
    m_auFormat[FD_FORM_FORMAT]  = RegisterClipboardFormat(_T("CSPro DD Form"));
}


CFormDoc::~CFormDoc()
{
    AfxOleUnlockApp();

    if( GetFormTreeCtrl() ) // RHF Dec 12, 2007
        GetFormTreeCtrl()->SetFormDoc( nullptr ); // RHF Dec 12, 2007
}


BOOL CFormDoc::OnNewDocument()
{
    // smg: i don't believe this func will ever get called??
    if( !CDocument::OnNewDocument() )
        return FALSE;

    CDEFormFile* pFF = &GetFormFile();

    pFF->RemoveAllLevels();
    pFF->RemoveAllForms();

    pFF->AddLevel(new CDELevel()); // automatically creates one group under it

    return true;
}


BOOL CFormDoc::OnOpenDocument(LPCTSTR lpszPathName)
{
    // open up the form spec file to get its label and the input dict file name/path

    CSpecFile specFile(true);  // use a local as don't want to keep this info just yet

    // read form spec file to get its label
    BOOL success = specFile.Open(lpszPathName, CFile::modeRead);
    specFile.Close();

    if( !success )   // open didn't go ok
        AfxMessageBox(FormatText(_T("Opening %s failed"), lpszPathName));

    return success;
}


void CFormDoc::DeleteContents()
{
    if( m_pFormTreeCtrl == nullptr )
        return;

    CFormNodeID* pNodeID = GetFormTreeCtrl()->GetFormNode(this);    //Get the item CFormNodeItem  corresponding to this pDoc
    if (pNodeID)  {
        // if .fmf file bad when trying to load and user doesn't want to load, this will be 0
        if (pNodeID->GetRefCount() > 0 )  {
            GetFormTreeCtrl()->ReleaseDoc(pNodeID);
        }
    }

    SetCurFormIndex(NONE);

    CDocument::DeleteContents();
}


CView* CFormDoc::GetView(FormViewType form_view_type/* = FormViewType::Form*/)
{
    POSITION pos = GetFirstViewPosition();

    while( pos != nullptr )
    {
        CView* pView = GetNextView(pos);

        if( ( form_view_type == FormViewType::Form && DYNAMIC_DOWNCAST(CFormScrollView, pView) != nullptr ) ||
            ( form_view_type == FormViewType::Logic && DYNAMIC_DOWNCAST(CFSourceEditView, pView) != nullptr ) ||
            ( form_view_type == FormViewType::QuestionText && DYNAMIC_DOWNCAST(QSFView, pView) != nullptr ) ||
            ( form_view_type == FormViewType::Condition && DYNAMIC_DOWNCAST(CQSFCndView, pView) != nullptr ) ||
            (form_view_type == FormViewType::Questionnaire && DYNAMIC_DOWNCAST(QuestionnaireView, pView) != nullptr))
        {
            return pView;
        }
    }

    return nullptr;
}


void CFormDoc::OnGenerateFrm()
{
    GenerateFormFile();
    AfxGetMainWnd()->SendMessage(UWM::Designer::SelectTab, static_cast<WPARAM>(FrameType::Form));
}


void CFormDoc::GenerateFormFile()
{
    // the following funcs allows the user to create a default .fmf file from a
    // given dictionary; my psuedo-attempt at making a wizard
    CFormScrollView* pFormView = assert_cast<CFormScrollView*>(GetView());
    CFormChildWnd* pParent = assert_cast<CFormChildWnd*>(pFormView->GetParentFrame());

    if( !pParent->ShowDragOptionsDlg() )
        return;

    m_undoStack.ClearUndo();    // can't go back further in stack than undoing new form file as ptrs will be off

    PushUndo(FormUndoStack(CFormUndoObj::Action::UR_delete, &GetFormFile(), NONE, SO::EmptyCString));

    CWaitCursor wait;

    pFormView->CalcFieldSpacing();

    // build the records too (not just the IDs)
    GetFormFile().CreateFormFile(GetSharedDictionary().get(), pFormView->GetDC(), pParent->GetDragOptions(), pFormView->GetDropSpacing(), true);

    pFormView->RemoveAllGrids();        // trash any grids that were created in this view
    pFormView->RemoveAllTrackers();
    pFormView->SetPointers (0);         // show 1st form in view

    pFormView->AdjustSpacing();         // added this for the default spacing
    pFormView->Invalidate();            // refresh the view to reflect the bld
    pFormView->SendMessage(WM_SIZE);

    SetModifiedFlag(true);
    pFormView->MarkDictTree();          // now we're ready to let the dict tree know who's been spoken for
    GetFormTreeCtrl()->ReBuildTree();   // default to show form 1; leave focus w/it
}


HTREEITEM CFormDoc::BuildAllTrees(HTREEITEM /*hParent = TVI_ROOT*/, HTREEITEM /*hInsertAfter = TVI_LAST*/)
{
    // BuildAllTrees is invoked from cspro
    // smg: are these args used in other modules BuildAddTrees? cause i'm never using
    TVITEM      pItem;
    HTREEITEM   hItem = nullptr;

//  Get the tree control handles and dict path & label
    CFormTreeCtrl*  pFormTree = GetFormTreeCtrl();
    CDDTreeCtrl*    pDictTree = pFormTree->GetDDTreeCtrl();
    CDEFormFile*    pFFSpec = &GetFormFile();
    CString         csPath, csName, csLabel;

    // add dictionary ref
    csPath  = pFFSpec->GetDictionaryFilename();
    csLabel = _T("Popstan");    // see note in else blk; comment no longer valid, nd2fix

    //  either add a node to the dict tree, or add a reference
    DictionaryDictTreeNode* dictionary_dict_tree_node = pDictTree->GetDictionaryTreeNode(csPath);
    if (dictionary_dict_tree_node != nullptr)  {
        // then this dictionary already exists on the tree; just add a ref
        dictionary_dict_tree_node->AddRef();
    }
    else  {
        // we get the thrill of adding it to the tree
        // bruce doesn't use the first arg, csLabel, so don't worry that it's not legit
        pItem.hItem = pDictTree->InsertDictionary (csLabel,csPath,nullptr); // rtns handle;
        pItem.mask  = TVIF_CHILDREN;
        pItem.cChildren = 1;
        pDictTree->SetItem(&pItem);    // set the attributes
    }

//  either add a node to the form tree, or add a ref
    csPath = GetPathName();     // mfc; rtns the document's fully qualified path
    csName = m_formSpec.GetName();
    csLabel = m_formSpec.GetLabel();

    CFormNodeID* pFormID = pFormTree->GetFormNode(this);
    if(pFormID) {
        pFormID->AddRef();
        hItem = pFormID->GetHItem();
    }
    else {
        hItem = pFormTree->InsertFormFile(csLabel,this->GetPathName(),this);
        pItem.hItem = hItem;
        pItem.mask  = TVIF_CHILDREN ;
        pItem.cChildren = 1;
        pFormTree->SetItem(&pItem);
    }
    return hItem;
}


bool CFormDoc::InitTreeCtrl()
{
    // build up the form tree ctrl only after we know the loading of the form spec file went ok
    CFormTreeCtrl* pFTC = GetFormTreeCtrl();
    ASSERT(pFTC);

    CFormNodeID* pFormID = pFTC->GetFormNode(this);
    pFTC->BuildTree(pFormID);
    pFormID->SetFormIndex (0);

    return true;
}


bool CFormDoc::LoadFormSpecFile(const CString& csFileName)
{
    if ( !m_formSpec.Open(csFileName) )        // load and create CDEFormFile here
        return false;

    m_bFormLoaded = true;   // indicate that we've built the CDEFormFile

    return true;
}

////////////////////////////////////////////////////////////////////////////
// rtns false if there were any probs opening the named dictionaries

bool CFormDoc::LoadDictSpecFile(bool bMakeVisible /*= true */)
{
    CFormTreeCtrl* pFormTree  = GetFormTreeCtrl();
    if (pFormTree == nullptr)  {
        return false;
    }

    CDDTreeCtrl* pDictTree = pFormTree->GetDDTreeCtrl();
    if (pDictTree == nullptr)  {
        return false;
    }

    CDEFormFile* pFFSpec = &GetFormFile();
    bool bOK = true;

    if(pFFSpec->GetDictionaryFilename().IsEmpty()) {
        return FALSE;
    }

    // check out all the dicts assoc w/this form
    CString csDictPath = pFFSpec->GetDictionaryFilename();
    DictionaryDictTreeNode* dictionary_dict_tree_node = pDictTree->GetDictionaryTreeNode(csDictPath);  // get the node assoc w/this dict
    if (dictionary_dict_tree_node == nullptr)  {
        // make sure the node's ok
        bOK = false;
    }
    else  {
        if(dictionary_dict_tree_node->GetDDDoc() == nullptr)  {
            // has the dictionary already been opened/assigned?
            bMakeVisible = false;
            if (!pDictTree->OpenDictionary (csDictPath,bMakeVisible))  {
                // if not, open it!
                bOK = false;        // if the open didn't go ok, flag it
            }
        }
    }
    if (bOK)  {
        pFFSpec->SetDictionaryName(GetSharedDictionary()->GetName());
        pFFSpec->SetDictionary(GetSharedDictionary());
    }
    return bOK;
}


CTreeCtrl* CFormDoc::GetTreeCtrl()
{
    return GetFormTreeCtrl();
}


std::shared_ptr<const CDataDict> CFormDoc::GetSharedDictionary() const
{
    CFormTreeCtrl* pFormTree  = GetFormTreeCtrl();
    if (pFormTree == nullptr)  {
        return nullptr;             // err condition, will need to flag better
    }

    CDDTreeCtrl* pDictTree = pFormTree->GetDDTreeCtrl();
    if (pDictTree == nullptr) {
        return nullptr;             // ditto comment above
    }

    const CDEFormFile* pFF = &GetFormFile();
    CString csDictPath = pFF->GetDictionaryFilename();
    if (csDictPath.IsEmpty())  {
        return nullptr;             // ditto comment above
    }

    DictionaryDictTreeNode* dictionary_dict_tree_node = pDictTree->GetDictionaryTreeNode(csDictPath);
    if (dictionary_dict_tree_node == nullptr)  {
        return nullptr;             // ditto comment above
    }

    std::shared_ptr<const CDataDict> pDD;

    if(dictionary_dict_tree_node->GetDDDoc() != nullptr) { // there's a doc assoc w/the node
        pDD = dictionary_dict_tree_node->GetDDDoc()->GetSharedDictionary();
    }
    else if (pDictTree->OpenDictionary (csDictPath, false)) { // open the dict if not already done
        pDD = dictionary_dict_tree_node->GetDDDoc()->GetSharedDictionary();
    }

    return pDD;
}


BOOL CFormDoc::OnSaveDocument(LPCTSTR lpszPathName)
{
    SaveAllDictionaries();

    m_formSpec.UpdatePointers();

    BOOL bOK = m_formSpec.Save(lpszPathName);

    if (bOK)
        SetModifiedFlag(FALSE);

    return bOK;
}


BOOL CFormDoc::SaveModified()
{
    if (!IsFormModified())
        return true;

    CString name = m_strPathName;

    if (name.IsEmpty())
        VERIFY (name.LoadString(AFX_IDS_UNTITLED));

    CString prompt;
    AfxFormatString1(prompt, AFX_IDP_ASK_TO_SAVE, name);

    switch (AfxMessageBox(prompt, MB_YESNOCANCEL, AFX_IDP_ASK_TO_SAVE))
    {
        case IDCANCEL: return false;       // don't continue
        case IDYES   :      // If so, either Save or Update, as appropriate
                    {
                        SaveAllDictionaries();
                        if (!DoSave(m_strPathName))
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


void CFormDoc::OnCloseDocument()
{
    m_bFormLoaded = false;
    CFormScrollView* pFV = assert_nullable_cast<CFormScrollView*>(GetView());
    if(pFV){
        pFV->RemoveAllGrids();
    }

    CDocument::OnCloseDocument();
}


void CFormDoc::ReleaseDicts()
{
    // this func is invoked from w/in cspro and the CFormTreeCtrl
    // it won't get called, however, when closing an .apl (i think just a .fmf)
    CFormTreeCtrl*  pFormTree  = GetFormTreeCtrl();

    if (pFormTree == nullptr)
        return;

    CDDTreeCtrl* pDictTree = pFormTree->GetDDTreeCtrl();

    if (pDictTree == nullptr)
        return;

    CDEFormFile* pFFSpec = &GetFormFile();
    DictionaryDictTreeNode* dictionary_dict_tree_node = pDictTree->GetDictionaryTreeNode(pFFSpec->GetDictionaryFilename());

    if (dictionary_dict_tree_node != nullptr)
        pDictTree->ReleaseDictionaryNode(*dictionary_dict_tree_node);
}


void CFormDoc::SaveAllDictionaries()
{
    CFormTreeCtrl*  pFormTree  = GetFormTreeCtrl();

    if (pFormTree == nullptr)
        return;

    CDDTreeCtrl* pDictTree = pFormTree->GetDDTreeCtrl();
    DictionaryDictTreeNode* dictionary_dict_tree_node = pDictTree->GetDictionaryTreeNode(m_formSpec.GetDictionaryFilename());

    if( dictionary_dict_tree_node != nullptr && dictionary_dict_tree_node->GetDDDoc() != nullptr ) {
        if(dictionary_dict_tree_node->GetDDDoc()->IsModified()){
            dictionary_dict_tree_node->GetDDDoc()->OnSaveDocument(dictionary_dict_tree_node->GetDDDoc()->GetPathName());
        }
    }
}


BOOL CFormDoc::IsFormModified()
{
    if(IsModified())
        return TRUE;

    BOOL bRet = FALSE;

    CFormTreeCtrl*  pFormTree  = GetFormTreeCtrl();

    if (pFormTree == nullptr)
        return bRet;

    CDDTreeCtrl* pDictTree = pFormTree->GetDDTreeCtrl();
    DictionaryDictTreeNode* dictionary_dict_tree_node = pDictTree->GetDictionaryTreeNode(m_formSpec.GetDictionaryFilename());

    if( dictionary_dict_tree_node != nullptr && dictionary_dict_tree_node->GetDDDoc() != nullptr ) {
        bRet = dictionary_dict_tree_node->GetDDDoc()->IsModified();
    }

    return bRet;
}


// *************************************************************************
//
// the following code is used for implementing undo/redo functionality
//
// *************************************************************************

// first for FormUndoStack

FormUndoStack FormUndoStack::ToggleUndoRedo(CDEFormFile* pFF)
{
    FormUndoStack toggled_stack;

    // Reverse order of operations when toggling
    for( auto itr = m_aURObjs.rbegin(); itr != m_aURObjs.rend(); ++itr )
        toggled_stack.m_aURObjs.emplace_back((*itr)->ToggleUndoRedo(pFF));

    return toggled_stack;
}


FormUndoStack::FormUndoStack(CFormUndoObj::Action eAction, CObject* pURObj, int iIndex, const CString& sParentName)
{
    PushUndoObj(eAction, pURObj, iIndex, sParentName);
}


// and now for CFormUndoObj

CFormUndoObj::CFormUndoObj() :
    m_eAction(Action::UR_unknown),
    m_pUndoItem(nullptr),
    m_iIndex(NONE)
{
}


CFormUndoObj::CFormUndoObj(Action action, CObject* pUndoItem, int iIndex, const CString& sParentName)
    :   m_eAction(action),
        m_pUndoItem(pUndoItem),
        m_iIndex (iIndex),
        m_sParentName(sParentName)
{
}


CFormUndoObj::CFormUndoObj (CFormUndoObj& uo)
    :   m_eAction(uo.m_eAction),
        m_pUndoItem(uo.m_pUndoItem),
        m_iIndex (uo.m_iIndex),
        m_sParentName(uo.m_sParentName){

}


CFormUndoObj::~CFormUndoObj()
{
    ASSERT_VALID (m_pUndoItem);
    delete m_pUndoItem;
}


CFormUndoObj* CFormUndoObj::ToggleUndoRedo(CDEFormFile* pFF)
{
    int iAction = GetAction();

    if (iAction == UR_modify)
    {
        return ToggleModifyOp(pFF);
    }
    else if (iAction == UR_move)
    {
        return ToggleMoveOp(pFF);
    }
    else
    {
        return ToggleAddDeleteOp(pFF);
    }
}


CFormUndoObj* CFormUndoObj::ToggleMoveOp(CDEFormFile* pFF)
{
    CFormUndoObj*   pOrigUndoObj = this;

    CObject* pOrigUndoItem = pOrigUndoObj->GetUndoItem();
    CObject*        pNewUndoItem = nullptr;
    CDEForm*        pForm;

    int iIndex = pOrigUndoObj->GetIndex();
    CString sParentsName = pOrigUndoObj->GetParentsName();

    if (pOrigUndoItem->IsKindOf(RUNTIME_CLASS(CDEBox))) {
        int iFormIndex = pFF->FindForm(sParentsName);
        pForm = pFF->GetForm(iFormIndex);
        pNewUndoItem = new CDEBox(pForm->GetBoxSet().GetBox(iIndex));
    }

    else if (pOrigUndoItem->IsKindOf(RUNTIME_CLASS(CDEText))) {
        int iFormIndex = pFF->FindForm(sParentsName);
        CDEText* pCurText;

        if (iFormIndex == NONE)     // then this shld be the txt portion of a field
        {
            CDEItemBase* pField = nullptr; // sys complains and won't resolve func call unless i declare it as itembase
            pFF->FindItem(sParentsName, &pForm, &pField);
            ASSERT(pField != nullptr); // then i'm hosed, don't know what i've got
            pCurText = &assert_cast<CDEField*>(pField)->GetCDEText();
        }
        else    // it's legit, proceed as if it's a free-standing text item
        {
            pForm = pFF->GetForm(iFormIndex);
            CDEItemBase* pField = pForm->GetItem(iIndex);
            if (pField->IsKindOf(RUNTIME_CLASS(CDEField)))
                pCurText = &assert_cast<CDEField*>(pField)->GetCDEText();
            else
                pCurText = ((CDEText*)pField);
        }
        pNewUndoItem = new CDEText(*pCurText);
    }

    else if (pOrigUndoItem->IsKindOf(RUNTIME_CLASS(CDEField))) {

        CDEField* pOrigFld = (CDEField*)pOrigUndoItem;
        CDEGroup* pParent = pOrigFld->GetParent();
        CDEField* pCurFld = (CDEField*)(pParent->GetItem(iIndex));
        pNewUndoItem = new CDEField(*pCurFld);
    }

    else if (pOrigUndoItem->IsKindOf(RUNTIME_CLASS(CDERoster))) {

        CDERoster* pOrigRstr = (CDERoster*)pOrigUndoItem;
        CDEGroup* pParent = pOrigRstr->GetParent();
        CDEItemBase* pCurRstr = pParent->GetItem(iIndex);
        CDERoster* pNewRstr = new CDERoster();    // create an empty roster
        pNewRstr->SetDims(pCurRstr->GetDims());     // and save the only thing we're int in;
        pNewRstr->SetName(pCurRstr->GetName());     // it's uniq name and dimensions
        pNewRstr->SetParent(pCurRstr->GetParent());     // it's uniq name and dimensions
        pNewUndoItem = pNewRstr;
    }

    else {
        ASSERT(FALSE);
    }

    return new CFormUndoObj(UR_move, pNewUndoItem, iIndex, sParentsName);
}


CFormUndoObj* CFormUndoObj::ToggleModifyOp(CDEFormFile* pFF)
{
    CFormUndoObj*   pOrigUndoObj = this;
    CObject*        pOrigUndoItem = nullptr;
    CObject*        pNewUndoItem = nullptr;
    int iIndex;
    CIMSAString sParentsName;

    pOrigUndoItem   = pOrigUndoObj->GetUndoItem();
    iIndex          = pOrigUndoObj->GetIndex();
    sParentsName    = pOrigUndoObj->GetParentsName();

    if (pOrigUndoItem->IsKindOf(RUNTIME_CLASS(CDEBox)))  {

        int iFormIndex = pFF->FindForm(sParentsName);
        CDEForm* pForm = pFF->GetForm(iFormIndex);
        pNewUndoItem = new CDEBox(pForm->GetBoxSet().GetBox(iIndex));
    }

    else if (pOrigUndoItem->IsKindOf(RUNTIME_CLASS(CDEText)))  {
        int iFormIndex = pFF->FindForm(sParentsName);
        CDEForm* pForm = pFF->GetForm(iFormIndex);

        CDEItemBase* pItem = pForm->GetItem(iIndex);
        CDEText* pText = nullptr;
        if (pItem->GetItemType() == CDEFormBase::Text)  // then it's a free-standing text item
        {
            pText = new CDEText (*(CDEText*) pItem);
        }
        else if (pItem->GetItemType() == CDEFormBase::Field)    // better be a field! a little more work if it is
        {
            pText = new CDEText ( ((CDEField *)pItem)->GetCDEText());

        }
        pNewUndoItem = new CDEText (*pText);
        //GetView()->RemoveAllTrackers();
    }

    else if (pOrigUndoItem->IsKindOf(RUNTIME_CLASS(CDERoster))) {
        CDERoster* pOrigRstr = static_cast<CDERoster*>(pOrigUndoItem);    // cast it as a CDERoster
        CDEGroup* pParent = pOrigRstr->GetParent();
        CDERoster* pCurrentRoster = static_cast<CDERoster*>(pParent->GetItem(iIndex));
        pNewUndoItem = pCurrentRoster->Clone().release();
    }

    else if (pOrigUndoItem->IsKindOf(RUNTIME_CLASS(CDEGroup))) {
        CDEForm* pForm = pFF->GetForm(iIndex);
        CDEGroup* pOrigGroup = (CDEGroup*) pForm->GetGroup();
        pNewUndoItem = new CDEGroup (*pOrigGroup);
    }

    else if (pOrigUndoItem->IsKindOf(RUNTIME_CLASS(CDEForm)))  {
        CDEForm* pOrigForm = pFF->GetForm(iIndex);
        pNewUndoItem = new CDEForm (*pOrigForm);
    }

    else if (pOrigUndoItem->IsKindOf(RUNTIME_CLASS(CDEBlock))) {
        CDEBlock* pOrigBlk = static_cast<CDEBlock*>(pOrigUndoItem);
        CDEGroup* pParent = pOrigBlk->GetParent();
        CDEBlock* pCurBlk = static_cast<CDEBlock*>(pParent->GetItem(iIndex));
        pNewUndoItem = pCurBlk->Clone().release();
    }

    else {
        ASSERT(FALSE);  // an error, but proceed for now
    }

    return new CFormUndoObj(UR_modify, pNewUndoItem, iIndex, sParentsName);
}


CFormUndoObj* CFormUndoObj::ToggleAddDeleteOp(CDEFormFile* pFF)
{
    CFormUndoObj*   pOrigUndoObj = this;
    CObject*        pOrigUndoItem = pOrigUndoObj->GetUndoItem();
    CObject*        pNewUndoItem = nullptr;
    int iIndex = pOrigUndoObj->GetIndex();
    CIMSAString sParentsName = pOrigUndoObj->GetParentsName();

    if (pOrigUndoItem->IsKindOf(RUNTIME_CLASS(CDEBox))) {
        pNewUndoItem = new CDEBox(*(CDEBox*)pOrigUndoItem);
    }
    else if (pOrigUndoItem->IsKindOf(RUNTIME_CLASS(CDEText))) {
        pNewUndoItem = new CDEText(*(CDEText*)pOrigUndoItem);
    }
    else if (pOrigUndoItem->IsKindOf(RUNTIME_CLASS(CDEField))) {
        pNewUndoItem = new CDEField(*(CDEField*)pOrigUndoItem);
    }
    else if (pOrigUndoItem->IsKindOf(RUNTIME_CLASS(CDERoster))) {  // put ahead of CDEGroup
        pNewUndoItem = new CDERoster(*static_cast<CDERoster*>(pOrigUndoItem));
    }
    else if (pOrigUndoItem->IsKindOf(RUNTIME_CLASS(CDEForm))) {
        pNewUndoItem = new CDEForm(*(CDEForm*)pOrigUndoItem);
    }
    else if (pOrigUndoItem->IsKindOf(RUNTIME_CLASS(CDEGroup))) {
        pNewUndoItem = new CDEGroup(*static_cast<CDEGroup*>(pOrigUndoItem));
    }
    else if (pOrigUndoItem->IsKindOf(RUNTIME_CLASS(CDECol))) {
        pNewUndoItem = new CDECol(*static_cast<CDECol*>(pOrigUndoItem));
    }
    else if (pOrigUndoItem->IsKindOf(RUNTIME_CLASS(CDEFreeCell))) {
        pNewUndoItem = new CDEFreeCell(*(CDEFreeCell*)pOrigUndoItem);
    }
    else if (pOrigUndoItem->IsKindOf(RUNTIME_CLASS(CDEFormFile))) {
        pNewUndoItem = new CDEFormFile(*pFF);
    }
    else if (pOrigUndoItem->IsKindOf(RUNTIME_CLASS(CDEBlock))) {
        pNewUndoItem = new CDEBlock(*(CDEBlock*)pOrigUndoItem);
    }
    else {
        ASSERT(FALSE);
    }

    return new CFormUndoObj(pOrigUndoObj->GetAction() == UR_add ? UR_delete : UR_add, pNewUndoItem, iIndex, sParentsName);
}


void FormUndoStack::PushMoveObj(const CObject* pUndoObj, int iIndex, const CString& sParentName)
{
    CObject* pNewItem = nullptr;

    // for the little guys, box, text, and field, make a copy of the entity
    // but for the roster, just save the uniq name and dims; don't need other guys

    if (pUndoObj->IsKindOf(RUNTIME_CLASS(CDEBox)))  {
        pNewItem = new CDEBox(*(const CDEBox*) pUndoObj);
    }
    else if (pUndoObj->IsKindOf(RUNTIME_CLASS(CDEText)))  {
        pNewItem = new CDEText (*(const CDEText*) pUndoObj );
    }
    else if (pUndoObj->IsKindOf(RUNTIME_CLASS(CDEField)))  {
        pNewItem = new CDEField (*(const CDEField*) pUndoObj );
    }
    else if (pUndoObj->IsKindOf(RUNTIME_CLASS(CDERoster)))  {
        const CDERoster* pCurRstr = (const CDERoster*) pUndoObj ;     // cast it
        CDERoster* pNewRstr = new CDERoster();    // create an empty roster
        pNewRstr->SetName(pCurRstr->GetName());
        pNewRstr->SetDims(pCurRstr->GetDims());
        pNewRstr->SetParent(pCurRstr->GetParent());
        pNewItem = pNewRstr;
    }
    else {
        return; // an error for now
    }

    // create a new element for the array member
    m_aURObjs.insert(m_aURObjs.begin(), std::make_shared<CFormUndoObj>(CFormUndoObj::Action::UR_move, pNewItem, iIndex, sParentName));
}


// i will be passing in CDEFields, CDETexts, CDERosters, etc. to this func for copying &
// insertion at the head of the array; i need to first make a copy of the obj, then add
// it to the array

void FormUndoStack::PushUndoObj(CFormUndoObj::Action iAction, CObject* pUndoObj, int iIndex, const CString& sParentName)
{
 #define COPY(type) new type(*(type*) pUndoObj )
    CObject* pNewItem = nullptr;

    // make a copy of the entity
    if (pUndoObj->IsKindOf(RUNTIME_CLASS(CDEBox)))  {
        pNewItem = new CDEBox(*(CDEBox*) pUndoObj);
    }
    else if (pUndoObj->IsKindOf(RUNTIME_CLASS(CDEText)))  {
        pNewItem = new CDEText (*(CDEText*) pUndoObj );
    }
    else if (pUndoObj->IsKindOf(RUNTIME_CLASS(CDEField)))  {
        pNewItem = new CDEField (*(CDEField*) pUndoObj );
    }
    else if (pUndoObj->IsKindOf(RUNTIME_CLASS(CDERoster)))  {  // put this ahead of CDEGroup
        pNewItem = COPY(CDERoster);
    }
    else if (pUndoObj->IsKindOf(RUNTIME_CLASS(CDEForm)))  {
        pNewItem = new CDEForm (*(CDEForm*) pUndoObj );
    }
    else if (pUndoObj->IsKindOf(RUNTIME_CLASS(CDEGroup)))  {
        pNewItem = COPY(CDEGroup);
    }
    else if (pUndoObj->IsKindOf(RUNTIME_CLASS(CDECol)))  {
        pNewItem = COPY(CDECol);
    }
    else if (pUndoObj->IsKindOf(RUNTIME_CLASS(CDEFreeCell)))  {
        pNewItem = new CDEFreeCell (*(CDEFreeCell*) pUndoObj );
    }
    else if (pUndoObj->IsKindOf(RUNTIME_CLASS(CDEFormFile))) {
        pNewItem = COPY(CDEFormFile);
    }
    else if (pUndoObj->IsKindOf(RUNTIME_CLASS(CDEBlock))) {
        pNewItem = COPY(CDEBlock);
    }
    else {
        ASSERT(FALSE);
        return; // an error for now
    }

    // create a new element for the array member
    m_aURObjs.insert(m_aURObjs.begin(), std::make_shared<CFormUndoObj>(iAction, pNewItem, iIndex, sParentName));
}


// ***********************************************************************
// these funcs perform the push (for undo)
// ***********************************************************************

// push the object, which is about to be modified or deleted, on to the undo stack

void CFormDoc::PushUndo(FormUndoStack form_undo_stack)
{
    ASSERT(form_undo_stack.GetNumURObjs() > 0);
    m_undoStack.PushUndo(std::move(form_undo_stack));
    m_undoStack.ClearRedo();
}

// ***********************************************************************
// these funcs perform the undo
// ***********************************************************************

// Get the most recently pushed object off the undo stack; if redo permitted,
// take the current state of affairs and put on redo stack

void CFormDoc::UndoChange(bool bCanRedo)
{
    ASSERT(m_undoStack.CanUndo());
    FormUndoStack form_undo_stack = m_undoStack.Undo();

    SetModifiedFlag(true);

    // if it's ok to redo, then put the *current* obj's environment (corresponding
    // to the one being pulled off the undo stack) on to the redo stack, in case
    // they want to redo their undo!

    if( bCanRedo )   // then set up the redo stack
        m_undoStack.PushRedo(form_undo_stack.ToggleUndoRedo(&GetFormFile()));

    PerformUndoChange(form_undo_stack);
}

// ***********************************************************************
// restore objs after a delete

void CFormDoc::PerformUndoChange(FormUndoStack& form_undo_stack)
{
    bool bRebuildTree = false;
    CFormScrollView* pView = assert_cast<CFormScrollView*>(GetView());
    pView->RemoveAllTrackers();
    for( int i = 0; i < form_undo_stack.GetNumURObjs(); ++i )
    {
        CFormUndoObj* pUndoObj = form_undo_stack.GetURObj(i);
        switch (pUndoObj->GetAction())
        {
        case CFormUndoObj::Action::UR_delete:
            bRebuildTree |= PerformUndoDelete(pUndoObj);
            break;
        case CFormUndoObj::Action::UR_add:
            bRebuildTree |= PerformUndoAdd(pUndoObj);
            break;
        case CFormUndoObj::Action::UR_move:
            bRebuildTree |= PerformUndoMove(pUndoObj);
            break;
        case CFormUndoObj::Action::UR_modify:
            bRebuildTree |= PerformUndoModify(pUndoObj);
            break;
        default:
            ASSERT(FALSE);
        }
    }

    GetFormFile().RenumberAllForms();    // all subsequent forms must have their items renumbered!

    if (bRebuildTree)
    {
        CFormTreeCtrl* pFTC = GetFormTreeCtrl();
        pFTC->ReBuildTree(GetCurFormIndex()); //select the current form
        pFTC->Invalidate();
        pFTC->UpdateWindow();
        pView->SetFocus();  // leave focus w/the view, not the tree
    }

    GetFormFile().BuildUniqueNL();

    pView->Invalidate();
//  pView->UpdateWindow();
}

// *********************************************************************
// for this, restore the environment with the current obj(s) (if redo was
// possible, then that's already been set up in CFormDoc::UndoChange); if
// this func is called, then the obj had been deleted, so need to add it back

bool CFormDoc::PerformUndoDelete(CFormUndoObj* pUndoObj)
{
    CObject* pUndoItem = nullptr;
    int iIndex;
    CIMSAString sParentsName;

    CFormScrollView* pFV = assert_cast<CFormScrollView*>(GetView());
    CDEFormFile* pFF = &GetFormFile();
    CDEForm* pForm = nullptr;

    int iFormIndex;
    bool bRebuildTree = false;

    pUndoItem = pUndoObj->GetUndoItem();

    sParentsName = pUndoObj->GetParentsName();
    iIndex = pUndoObj->GetIndex();

    if (pUndoItem->IsKindOf(RUNTIME_CLASS(CDEBox))) {
        iFormIndex = pFF->FindForm(sParentsName);
        pForm = pFF->GetForm(iFormIndex);
        pFV->MakeFormCurrent(iFormIndex);
        pForm->GetBoxSet().InsertBox(iIndex, std::make_shared<CDEBox>(*assert_cast<CDEBox*>(pUndoItem)));
    }

    else if (pUndoItem->IsKindOf(RUNTIME_CLASS(CDEFormFile))) {
        pFV->RemoveAllGrids();         // trash any grids that were created in this view
        pFV->RemoveAllTrackers();

        SetFFSpec (*(CDEFormFile*) pUndoItem);

        pFV->SetPointers (0);   // show 1st form in view
        bRebuildTree = true;
    }

    else if (pUndoItem->IsKindOf(RUNTIME_CLASS(CDEForm))) {
        // when restoring forms, do the form before restoring it's associated group

        // need to be able to distinguish between a form being deleted,
        // or, if this was the last form in a level, all the items were
        // deleted but the form itself was NOT deleted--deal with nxt week

        pFV->RemoveAllGrids();         // trash any grids that were created in this view
        pFV->RemoveAllTrackers();
        CDEForm* pForm = new CDEForm (*(CDEForm*) pUndoItem);

        // GHM 20120515 a bug occurred in a one-form application when a form was deleted and then brought back using undo
        // (there would be two groups sharing the same name because of the default form that was created automatically after the deletion)
        for( int i = 0; i < pFF->GetNumForms(); i++ )
        {
            CDEForm* pExistingForm = pFF->GetForm(i);

            if( !pExistingForm->GetName().CompareNoCase(pForm->GetName()) )
            {
                CDEGroup* pGroup = pExistingForm->GetGroup();
                CDELevel* pLevel = pFF->GetLevel(pExistingForm->GetLevel());

                pFF->RemoveUniqueName(pExistingForm->GetName());
                pFF->RemoveUniqueName(pGroup->GetName());

                CFormScrollView * pFV = (CFormScrollView *)GetView();
                pFV->DeleteGroupAndForms(pGroup); // this will remove the form, which should be empty

                pLevel->RemoveItem(pGroup->GetName()); // this will delete the group that the form was part of

                break;
            }
        }

        pFF->InsertFormAt (pForm, iIndex);
        pFF->AddUniqueName(pForm->GetName());
        int i, max = pForm->GetNumItems();
        CDEItemBase* pItem;
        for (i=0; i < max; i++)     // add in all the uniq names for the text items
        {
            pItem = pForm->GetItem(i);
            pFF->AddUniqueName(pItem->GetName());
        }
        //when a form is reinserted due to undo, all the form indices in the tree control are not in sync. The tree control needs to be rebuilt to reference the correct form indices
        pFV->MakeFormCurrent(iIndex);
        bRebuildTree = true;
    }

    // total SHIIIITE! since CDERoster inherits from CDEGroup, the test for
    // CDEGroup was coming up true for rosters! so put it first to short-circuit
    else if (pUndoItem->IsKindOf(RUNTIME_CLASS(CDERoster)))  {
        CDERoster* pOrigRstr = (CDERoster*) pUndoItem;    // cast it as a CDERoster
        CDEGroup*   pParent = pOrigRstr->GetParent();

        iFormIndex = pOrigRstr->GetFormNum();
        pForm = pFF->GetForm(iFormIndex);

        pFV->MakeFormCurrent(iFormIndex);

        CDERoster* pNewRstr = new CDERoster(*pOrigRstr);

        pForm->AddItem(pNewRstr); // do i need the roster's orig index...?
        pParent->InsertItemAt (pNewRstr, iIndex);
        pFF->AddUniqueName(pNewRstr->GetName()); // add the unique name of roster

        CDEField* pField = nullptr;
        int i, max = pNewRstr->GetNumItems();   // and now add the uniq name of
        for (i=0; i < max; i++) {           // each field w/in the roster
            pField = (CDEField*) pNewRstr->GetItem(i);
            pFF->AddUniqueName(pField->GetName());
        }
        pFV->CreateGrid(pNewRstr);
        bRebuildTree = true;
    }

    else if (pUndoItem->IsKindOf(RUNTIME_CLASS(CDEGroup))) {
        // when restoring groups having a 1-to-1 corresp w/forms,
        // restore the form first, then the group

        CDEGroup* pGroup = new CDEGroup (*(CDEGroup*) pUndoItem);
        int iFormNum = pGroup->GetFormNum();
        pFV->SetFormIndex (iFormNum);
        pForm = pFF->GetForm(iFormNum);
        int iLevelNum = pForm->GetLevel();
        CDELevel* pLevel = pFF->GetLevel (iLevelNum);

        CDEGroup* pParentGroup = pLevel->FindGroup (sParentsName);

        ASSERT(pParentGroup);

        pParentGroup->InsertItemAt (pGroup, iIndex);    // add the group back to its parent
        //pFV->MyRenumberForms();
//          pFF->RenumberFormsAndItems();   // all subsequent forms must have their items renumbered!
        pFF->AddUniqueNames (pGroup);   // this will add the uniq names of all items
        pForm->AddGroupItems (pGroup);  // add all the fields/rosters in the group to the form
        pForm->SetGroup(pGroup);

        int i, max = pGroup->GetNumItems(); // create grids for any rosters
        CDEItemBase* pItem;                 // this shld really be recursive
        for (i=0; i < max; i++)
        {
            pItem = pGroup->GetItem(i);
            if (pItem->GetItemType() == CDEFormBase::Roster)
                pFV->CreateGrid((CDERoster*) pItem);
        }
        GetFormTreeCtrl()->ReBuildTree(iFormNum);
        SetCurFormIndex(iFormNum);
        bRebuildTree = false;
    }

    else if (pUndoItem->IsKindOf(RUNTIME_CLASS(CDEText)))  {
        CDEText* pOrigText = (CDEText*) pUndoItem;      // cast it as a CDEText

        iFormIndex = pFF->FindForm(sParentsName);
        if (iFormIndex == NONE)     // then, i hope, this is the txt portion of a field
        {
            CDEItemBase* pField = nullptr; // sys complains and won't resolve func call unless i declare it as itembase
            pFF->FindItem(sParentsName, &pForm, &pField);
            ASSERT(pField != nullptr); // then i'm hosed, don't know what i've got
            ((CDEField*) pField)->SetCDEText (*pOrigText);
        }
        else    // it's legit, proceed as if it's a free-standing text item
        {
            pForm = pFF->GetForm(iFormIndex);

            CDEText* pNewText = new CDEText (* pOrigText);

            pForm->InsertItemAt (pNewText, iIndex);
            pFF->AddUniqueName(pNewText->GetName());
            pFV->MakeFormCurrent(iFormIndex);
        }

        CRect rect = pOrigText->GetDims();
        int x = GetCurForm()->GetNumItems();
        pFV->AddTracker (rect,x-1); // only the text portion was selected
    }

    else if (pUndoItem->IsKindOf(RUNTIME_CLASS(CDEField)))  {
        CDEField* pOrigFld = (CDEField*) pUndoItem;     // cast it as a CDEField
        CDEGroup* pGroup = pOrigFld->GetParent();

        iFormIndex = pOrigFld->GetFormNum();
        pForm = pFF->GetForm(iFormIndex);

        if (iFormIndex != pFV->GetFormIndex())
        {
            pFV->MakeFormCurrent(iFormIndex);
        }
        CDEField* pNewFld = new CDEField (* pOrigFld);

        pForm->AddItem(pNewFld);
        pGroup->InsertItemAt (pNewFld, iIndex);
        pFF->AddUniqueName(pNewFld->GetName());

        int x = GetCurForm()->GetNumItems();
        CRect rect = pNewFld->GetDims();
        pFV->AddTracker (x-1,rect); // only the text portion was selected
        bRebuildTree = true;
    }

    else if (pUndoItem->IsKindOf(RUNTIME_CLASS(CDEBlock))) {
        CDEBlock* pOrigBlock = STATIC_DOWNCAST(CDEBlock, pUndoItem);
        CDEGroup* pGroup = pOrigBlock->GetParent();

        iFormIndex = pOrigBlock->GetFormNum();
        pForm = pFF->GetForm(iFormIndex);

        if (iFormIndex != pFV->GetFormIndex())
        {
            pFV->MakeFormCurrent(iFormIndex);
        }
        CDEBlock* pNewBlock = assert_cast<CDEBlock*>(pOrigBlock->Clone().release());
        pGroup->InsertItemAt(pNewBlock, iIndex);
        pFF->AddUniqueName(pNewBlock->GetName());
        bRebuildTree = true;
    }

    else if (pUndoItem->IsKindOf(RUNTIME_CLASS(CDECol)))  {
        ASSERT(FALSE); // Don't use delete col, use modify roster instead
                        // otherwise too complex to deal with blocks
    }

    else if (pUndoItem->IsKindOf(RUNTIME_CLASS(CDEFreeCell))) {

        auto new_free_cell = std::make_shared<CDEFreeCell>(static_cast<const CDEFreeCell&>(*pUndoItem));;
        CDERoster* pRoster = pFF->FindRoster(sParentsName);
        ASSERT(pRoster != nullptr);
        pRoster->InsertFreeCell(iIndex, new_free_cell);
        CFormGrid* pGrid = pFV->FindGrid(pRoster);
        pGrid->BuildGrid();
        pGrid->RecalcLayout();
    }

    else {
        // deal with other CDE guys
        ASSERT(FALSE);
    }

    return bRebuildTree;
}

// *********************************************************************
// if this func is called, then the obj had been added, so need to delete again

bool CFormDoc::PerformUndoAdd (CFormUndoObj* pUndoObj)
{
    CObject* pUndoItem = nullptr;
    int iIndex;
    CIMSAString sParentsName;

    CDEFormFile* pFF = &GetFormFile();
    CDEForm* pForm = nullptr;

    int iFormIndex;
    bool bRebuildTree = false;
    bool bSelectItem = true;    // need to pass back; will i reselect restored items?

    CFormScrollView* pFV = assert_cast<CFormScrollView*>(GetView());

    pUndoItem = pUndoObj->GetUndoItem();

    sParentsName = pUndoObj->GetParentsName();
    iIndex = pUndoObj->GetIndex();

    if (pUndoItem->IsKindOf(RUNTIME_CLASS(CDEBox))) {
        iFormIndex = pFF->FindForm(sParentsName);
        pForm = pFF->GetForm(iFormIndex);
        pFV->MakeFormCurrent(iFormIndex);

        pForm->GetBoxSet().RemoveBox(iIndex);
        bSelectItem = false;
    }

    else if (pUndoItem->IsKindOf(RUNTIME_CLASS(CDEFormFile))) { // same code as for delete
        CFormScrollView* pFV = assert_cast<CFormScrollView*>(GetView());
        pFV->RemoveAllGrids();         // trash any grids that were created in this view
        pFV->RemoveAllTrackers();

        SetFFSpec (*(CDEFormFile*) pUndoItem);

        pFV->SetPointers (0);   // show 1st form in view
        bRebuildTree = true;
    }

    else if (pUndoItem->IsKindOf(RUNTIME_CLASS(CDEForm))) { // when restoring forms, do so before restoring it's associated group
        CDEForm* pForm = (CDEForm*) pUndoItem;  // cast the guy as the form
        int i, max = pForm->GetNumItems();
        CDEItemBase* pItem;
        for (i=0; i < max; i++)     // remove all the uniq names for the text items
        {
            pItem = pForm->GetItem(i);
            pFF->RemoveUniqueName(pItem->GetName());
        }
        pFF->RemoveUniqueName(pForm->GetName());
        pFF->RemoveFormAt (iIndex);

        //pFV->MyRenumberForms();
        pFF->RenumberFormsAndItems();   // all subsequent forms must have their items renumbered!
        GetFormTreeCtrl()->SwitchRefresh();
        bRebuildTree = true;
        bSelectItem = false;

    }

    // CDERoster has to go before CDEGroup!
    else if (pUndoItem->IsKindOf(RUNTIME_CLASS(CDERoster)))  {

        CDERoster* pOrigRstr = (CDERoster*) pUndoItem;    // cast it as a CDERoster
        CIMSAString sName = pOrigRstr->GetName();
        CDEGroup* pGroup = pOrigRstr->GetParent();
        CFormScrollView* pFV = assert_cast<CFormScrollView*>(GetView());

//          pForm->->ShowWindow(SW_SHOW);  //Show window activates and does it in the current size

        iFormIndex = pOrigRstr->GetFormNum();
        pForm = pFF->GetForm(iFormIndex);
        pFV->MakeFormCurrent(iFormIndex);

        pFV->RemoveGrid(sName);   // take down the view's grid
        pForm->RemoveItem(sName);      // next, remove the roster from the form
        int iLevel = pForm->GetLevel();
        CDERoster* pRoster = (CDERoster*) pGroup->GetItem(iIndex);
        pFF->GetLevel (iLevel)->RemoveRoster(pRoster, pFF);// finally, remove the roster from the Level


                    // it will also remove all uniq names from the FF
        bRebuildTree = true;
        bSelectItem = false;
    }

    else if (pUndoItem->IsKindOf(RUNTIME_CLASS(CDEGroup))) {

        CDEGroup* pOrigGroup = (CDEGroup*) pUndoItem;
        pFF->RemoveUniqueNames (pOrigGroup);

        CDEGroup* pParent = pOrigGroup->GetParent();
        int iGroupIndex = pParent->GetItemIndex (pOrigGroup->GetName());
        CDEGroup* pGroup = (CDEGroup*) pParent->GetItem(iGroupIndex);
        //GetLevel(pForm->GetLevel())->RemoveGroup(iIndex);
        pParent->RemoveItemAt (iGroupIndex);
        assert_cast<CFormScrollView*>(GetView())->RemoveGrid(pGroup);    // remove any grids assoc w/this group
        delete pGroup;
        bRebuildTree = true;
        bSelectItem = false;
    }

    else if (pUndoItem->IsKindOf(RUNTIME_CLASS(CDEText)))  {
        CDEText* pOrigText = (CDEText*) pUndoItem;  // cast it as CDEText
        iFormIndex = pFF->FindForm(sParentsName);

        if (iFormIndex == NONE)     // then, i hope, this is the txt portion of a field
        {
            CDEItemBase* pField = nullptr; // sys complains and won't resolve func call unless i declare it as itembase
            pFF->FindItem(sParentsName, &pForm, &pField);
            ASSERT(pField != nullptr); // then i'm hosed, don't know what i've got
            ((CDEField*) pField)->SetCDEText (_T(""));
        }
        else    // it's legit, proceed as if it's a free-standing text item
        {
            pForm = pFF->GetForm(iFormIndex);
            CIMSAString sName = pOrigText->GetName();

            pFF->RemoveUniqueName(sName);
            pForm->RemoveItem(sName);
            bSelectItem = false;
            pFV->MakeFormCurrent(iFormIndex);
        }
    }

    else if (pUndoItem->IsKindOf(RUNTIME_CLASS(CDEField)))  {

        CDEField* pOrigFld = (CDEField*) pUndoItem; // cast it as a CDEField
        CDEGroup* pGroup = pOrigFld->GetParent();
        CIMSAString sName = pOrigFld->GetName();

        iFormIndex = pOrigFld->GetFormNum();
        pForm = pFF->GetForm(iFormIndex);
        pFV->MakeFormCurrent(iFormIndex);

        pFF->RemoveUniqueName(sName);
        pForm->RemoveItem(sName);

        pGroup->RemoveItem(iIndex);    // deletes the memory too
        bRebuildTree = true;
        bSelectItem = false;
    }

    else if (pUndoItem->IsKindOf(RUNTIME_CLASS(CDEBlock))) {
        CDEBlock* pOrigBlock = STATIC_DOWNCAST(CDEBlock, pUndoItem);
        CDEGroup* pGroup = pOrigBlock->GetParent();

        iFormIndex = pOrigBlock->GetFormNum();
        pForm = pFF->GetForm(iFormIndex);
        pFV->MakeFormCurrent(iFormIndex);

        pFF->RemoveUniqueName(pOrigBlock->GetName());
        pGroup->RemoveItem(iIndex);

        bRebuildTree = true;
        bSelectItem = false;
    }

    else if (pUndoItem->IsKindOf(RUNTIME_CLASS(CDECol))) {
        ASSERT(FALSE); // Don't use delete col, use modify roster instead
                        // otherwise to complex to deal with blocks
    }

    else if (pUndoItem->IsKindOf(RUNTIME_CLASS(CDEFreeCell)))  {
        CDERoster* pRoster = pFF->FindRoster(sParentsName);
        ASSERT(pRoster != nullptr);
        pRoster->RemoveFreeCell(iIndex);
        CFormGrid* pGrid = assert_cast<CFormScrollView*>(GetView())->FindGrid(pRoster);
        pGrid->BuildGrid();
        pGrid->RecalcLayout();
    }

    else {
        ASSERT(FALSE);
    }

    return bRebuildTree;
}

// *********************************************************************
// if this func is called, then the obj had was modified so need to update
//
// THIS FUNC ISN'T FINISHED--i've noted which ones are wkg

bool CFormDoc::PerformUndoModify (CFormUndoObj* pUndoObj)
{
    CObject* pUndoItem = nullptr;
    int iIndex;
    CIMSAString sParentsName;

    CDEFormFile* pFF = &GetFormFile();
    CDEForm* pForm = nullptr;
    CFormScrollView* pFV = assert_cast<CFormScrollView*>(GetView());

    int iFormIndex;
    bool bRebuildTree = true;
    bool bSelectItem = true;    // need to pass back; will i reselect restored items?


    pUndoItem = pUndoObj->GetUndoItem();

    sParentsName = pUndoObj->GetParentsName();
    iIndex = pUndoObj->GetIndex();

    if (pUndoItem->IsKindOf(RUNTIME_CLASS(CDEBox))) {   // box works
        iFormIndex = pFF->FindForm(sParentsName);
        pForm = pFF->GetForm(iFormIndex);
        pFV->MakeFormCurrent(iFormIndex);

        // need to replace the box at the curr index w/the old guy
        CDEBox& box = pForm->GetBoxSet().GetBox((size_t)iIndex);
        box = assert_cast<CDEBox&>(*pUndoItem);

        assert_cast<CFormScrollView*>(GetView())->RemoveAllTrackers();
    }

    else if (pUndoItem->IsKindOf(RUNTIME_CLASS(CDEForm))) { // when restoring forms, do so before restoring it's associated group
        CDEForm* pForm = (CDEForm*) pUndoItem;  // cast the guy as the form
        //iFormIndex = pFF->FindForm(sParentsName);
        //pFF->RemoveUniqueName(pForm->GetName());
        CDEForm* pOrigForm = pFF->GetForm(iIndex);
        pOrigForm->SetName(pForm->GetName());
        pOrigForm->SetLabel(pForm->GetLabel());

        //pFV->MyRenumberForms();
        pFF->RenumberFormsAndItems();   // all subsequent forms must have their items renumbered!
        pFV->MakeFormCurrent(iIndex);
        GetFormTreeCtrl()->ReBuildTree(iIndex);
        bRebuildTree = false;
        bSelectItem = false;
    }

    else if (pUndoItem->IsKindOf(RUNTIME_CLASS(CDERoster))) {
        CDERoster* pOrigRstr = (CDERoster*)pUndoItem; // cast it as a CDERoster
        CIMSAString sName = pOrigRstr->GetName();
        CDEGroup* pParent = pOrigRstr->GetParent();
        CDERoster* pCurrentRoster = static_cast<CDERoster*>(pParent->GetItem(iIndex));

        iFormIndex = pOrigRstr->GetFormNum();
        pFV->MakeFormCurrent(iFormIndex);
        pForm = pFF->GetForm(iFormIndex);

        pFV->RemoveGrid(sName);   // take down the view's grid
        pForm->RemoveItem(sName);      // next, remove the roster from the form
        pParent->RemoveItemAt(iIndex);
        for (int i = 0; i < pCurrentRoster->GetNumItems(); i++) {
            CDEField* pField = (CDEField*)pCurrentRoster->GetItem(i);
            pFF->RemoveUniqueName(pField->GetName());
        }

        CDERoster* pNewRoster = new CDERoster(*pOrigRstr);
        pParent->InsertItemAt(pNewRoster, iIndex);
        pForm->AddItem(pNewRoster);
        for (int i = 0; i < pNewRoster->GetNumItems(); i++) {
            CDEField* pField = (CDEField*)pNewRoster->GetItem(i);
            pFF->AddUniqueName(pField->GetName());
        }

        pNewRoster->UpdateFlagsNFonts(*pFF);
        pFV->CreateGrid(pNewRoster);

        bRebuildTree = true;
        bSelectItem = false;
    }

    else if (pUndoItem->IsKindOf(RUNTIME_CLASS(CDEGroup))) {
        CDEGroup* pUndoGroup= (CDEGroup*) pUndoItem;

        CDEForm* pForm = pFF->GetForm(iIndex);
        //int iGroupIndex = pParent->GetItemIndex (pGroup->GetName());

        CDEGroup* pOrigGroup = (CDEGroup*) pForm->GetGroup();
        pOrigGroup->SetFormName(pUndoGroup->GetFormName());
        pFF->RemoveUniqueNames (pOrigGroup);
        pOrigGroup->SetName(pUndoGroup->GetName());
        pOrigGroup->SetLabel(pUndoGroup->GetLabel());
        pFF->AddUniqueNames (pOrigGroup);   // this will add the uniq names of all items


        bRebuildTree = true;
        bSelectItem = false;
    }

    else if (pUndoItem->IsKindOf(RUNTIME_CLASS(CDEText)))  {    // txt works

        iFormIndex = pFF->FindForm(sParentsName);
        pForm = pFF->GetForm(iFormIndex);
        pFV->MakeFormCurrent(iFormIndex);

        // need to replace the text at the curr index w/pUndoItem

        CDEItemBase* pItem = pForm->GetItem(iIndex);
        if (pItem->GetItemType() == CDEFormBase::Text)  // then it's a free-standing text item
        {
            pForm->RemoveItem(iIndex);
            CDEText* pText = new CDEText (*(CDEText*) pUndoItem);
            pForm->InsertItemAt (pText, iIndex);
        }
        else if (pItem->GetItemType() == CDEFormBase::Field)    // better be a field! a little more work if it is
        {
            CDEField* pField = (CDEField*) pItem;
            pField->SetCDEText (*(CDEText*) pUndoItem);
        }
        assert_cast<CFormScrollView*>(GetView())->RemoveAllTrackers();
        bRebuildTree = false;
    }

    else if (pUndoItem->IsKindOf(RUNTIME_CLASS(CDEField)))  {

        CDEField* pOrigFld = (CDEField*) pUndoItem; // cast it as a CDEField
        CDEGroup* pGroup = pOrigFld->GetParent();
        CIMSAString sName = pOrigFld->GetName();

        iFormIndex = pOrigFld->GetFormNum();
        pForm = pFF->GetForm(iFormIndex);

        pFV->MakeFormCurrent(iFormIndex);

        CDEField* pField = (CDEField*) pGroup->GetItem(iIndex);
        pFF->RemoveUniqueName(pField->GetName());
        pForm->RemoveItem(pField->GetName());    // remove the curr field
        pGroup->RemoveItemAt (iIndex);  // this does not delete the memory, only index entry
        delete pField;

        CDEField* pNewFld = new CDEField (* pOrigFld); // and restore with the old
        pForm->AddItem(pNewFld); // i think i'm going to need the field's orig index...?
        pGroup->InsertItemAt (pNewFld, iIndex);
        //pNewFld->GetCDEText().SetLabel("Test");
        pFF->AddUniqueName(pNewFld->GetName()); // add the unique name of field

        bRebuildTree = true;
        bSelectItem = false;
    }

    else if (pUndoItem->IsKindOf(RUNTIME_CLASS(CDEBlock))) {
        CDEBlock* pOrigBlk = static_cast<CDEBlock*>(pUndoItem);
        CDEGroup* pGroup = pOrigBlk->GetParent();
        CIMSAString sName = pOrigBlk->GetName();

        iFormIndex = pOrigBlk->GetFormNum();
        pForm = pFF->GetForm(iFormIndex);

        pFV->MakeFormCurrent(iFormIndex);

        CDEBlock* pBlock = static_cast<CDEBlock*>(pGroup->GetItem(iIndex));
        pFF->RemoveUniqueName(pBlock->GetName());
        pForm->RemoveItem(pBlock->GetName());
        pGroup->RemoveItemAt(iIndex);
        delete pBlock;

        CDEBlock* pNewBlk = assert_cast<CDEBlock*>(pOrigBlk->Clone().release());
        pForm->AddItem(pNewBlk);
        pGroup->InsertItemAt(pNewBlk, iIndex);
        pFF->AddUniqueName(pNewBlk->GetName());

        bRebuildTree = true;
        bSelectItem = false;
    }

    else {
        ASSERT(FALSE);
    }

    return bRebuildTree;
}

// *********************************************************************

bool CFormDoc::PerformUndoMove(CFormUndoObj* pUndoObj)
{
    CObject* pUndoItem = nullptr;
    int iIndex;
    CIMSAString sParentsName;

    CDEFormFile* pFF = &GetFormFile();
    CDEForm* pForm = nullptr;

    int iFormIndex;
    bool bRebuildTree = false;
    bool bSelectItem = true;    // need to pass back; will i reselect restored items?

    CFormScrollView* pFV = assert_cast<CFormScrollView*>(GetView());
    CPoint ptScroll(pFV->GetScrollPos(SB_HORZ), pFV->GetScrollPos(SB_VERT));

    pUndoItem = pUndoObj->GetUndoItem();

    sParentsName = pUndoObj->GetParentsName();

    iIndex = pUndoObj->GetIndex();

    if (pUndoItem->IsKindOf(RUNTIME_CLASS(CDEBox))) {   // box works
        iFormIndex = pFF->FindForm(sParentsName);
        pForm = pFF->GetForm(iFormIndex);

        pFV->MakeFormCurrent(iFormIndex);

        // need to replace the box at the curr index w/the old guy
        CDEBox& box = pForm->GetBoxSet().GetBox((size_t)iIndex);
        box = assert_cast<CDEBox&>(*pUndoItem);
    }

    else if (pUndoItem->IsKindOf(RUNTIME_CLASS(CDEText)))  {    // txt works
        CDEText* pOrigText = (CDEText*) pUndoItem;
        iFormIndex = pFF->FindForm(sParentsName);
        pFV->MakeFormCurrent(iFormIndex);
        if (iFormIndex == NONE)     // then this shld be the txt portion of a field
        {
            CDEItemBase* pField = nullptr; // sys complains and won't resolve func call unless i declare it as itembase
            pFF->FindItem(sParentsName, &pForm, &pField);
            ASSERT(pField == nullptr); // then i'm hosed, don't know what i've got
        }
        else    // it's legit, proceed as if it's a free-standing text item
        {       // need to replace the text at the curr index w/pUndoItem
            pForm = pFF->GetForm(iFormIndex);
            CDEItemBase* pField = pForm->GetItem(iIndex);
            CFormTracker track;
            if (pField->IsKindOf(RUNTIME_CLASS(CDEField)))
            {
                assert_cast<CDEField*>(pField)->GetCDEText().SetDims (pOrigText->GetDims());
                track.SetIsFldTxt (true);
            }
            else
            {
                pField->SetDims(pOrigText->GetDims());
            }

            track.m_rect = pOrigText->GetDims()-ptScroll;//pGrid->GetClientRect();
            track.SetIndex (iIndex);

            pFV->AddTracker (track);    // the Item's in the bounding rectangle, flag it
        }
    }

    else if (pUndoItem->IsKindOf(RUNTIME_CLASS(CDEField)))  {
        CDEField* pOrigFld = (CDEField*) pUndoItem; // cast it as a CDEField
        CDEGroup* pParent = pOrigFld->GetParent();  // use it to find its parent
        CDEField* pCurFld = (CDEField*) (pParent->GetItem(iIndex)); // use parent to get cur fld
        pCurFld->SetDims (pOrigFld->GetDims());
        iFormIndex = pOrigFld->GetFormNum();
        pFV->MakeFormCurrent(iFormIndex);
        pForm = pFF->GetForm(iFormIndex);
        CFormTracker track;
        track.m_rect = pCurFld->GetDims()-ptScroll;//pGrid->GetClientRect();
        int index = pForm->GetItemIndex (pCurFld);
        track.SetIndex (index);
        pFV->AddTracker (track);    // the Item's in the bounding rectangle, flag it
        bSelectItem = false;
    }

    else if (pUndoItem->IsKindOf(RUNTIME_CLASS(CDERoster)))  {

        CDERoster* pOrigRstr = (CDERoster*) pUndoItem;    // cast it as a CDERoster
        CDEGroup* pParent = pOrigRstr->GetParent();
        CDERoster* pCurRstr = (CDERoster*) (pParent->GetItem(iIndex));
        pCurRstr->SetDims (pOrigRstr->GetDims());

        iFormIndex = pCurRstr->GetFormNum();
        pFV->MakeFormCurrent(iFormIndex);

        pFV->RepositionGrid(pCurRstr);
        bSelectItem = false;
    }

    else {
        ASSERT(FALSE);
    }

    pFV->UpdateTrackerSh();
    return bRebuildTree;
}


// ***********************************************************************
// these funcs perform the redo
// ***********************************************************************

// Get the most recently pushed object off the redo stack and put in on undo stack

void CFormDoc::RedoChange()
{
    ASSERT(m_undoStack.CanRedo());
    FormUndoStack form_undo_stack = m_undoStack.Redo(); // the form redo stack

    SetModifiedFlag(true);

    // first copy the redo stack and put on to the undo stack
    m_undoStack.PushUndo(form_undo_stack.ToggleUndoRedo(&GetFormFile()));

    PerformUndoChange(form_undo_stack);
}

void CFormDoc::SetSelectedCapiQuestion(CFormID* form_id)
{
    m_capi_editor_view_model.Clear();

    if (form_id) {
        CDEItemBase* item_base = nullptr;
        eNodeType nType = form_id->GetItemType();

        if (nType == eFTT_GRIDFIELD)
        {
            CDERoster* pRoster = DYNAMIC_DOWNCAST(CDERoster, form_id->GetItemPtr());
            if (pRoster != nullptr)
                item_base = pRoster->GetCol(form_id->GetColumnIndex())->GetField(form_id->GetRosterField());
        } else {
            item_base = DYNAMIC_DOWNCAST(CDEItemBase, form_id->GetItemPtr());
        }
        if (item_base)
            m_capi_editor_view_model.SetItem(item_base);
    }
}

void CFormDoc::SetCapiQuestionManager(Application* application, std::shared_ptr<CapiQuestionManager> question_manager)
{
    m_question_manager = std::move(question_manager);
    m_capi_editor_view_model.SetQuestionManager(application, m_question_manager);
}


void CFormDoc::FileToClip(UINT uFormat)
{
    CString csClipFile = GetClipFile();
    if (!PortableFunctions::FileExists(csClipFile)) {
        // Clip file doesn't exist
        return;
    }

    CFile file(csClipFile, CFile::modeRead);
    int iSize = file.GetLength() + 1;

    if (iSize > 0)  {
        HGLOBAL hGlobalMemory;
        VERIFY ((hGlobalMemory = GlobalAlloc (GHND, iSize)) != nullptr);
        char FAR *lpGlobalMemory = (char FAR *) GlobalLock (hGlobalMemory);
        file.Read(lpGlobalMemory, (UINT) iSize);
        GlobalUnlock (hGlobalMemory);
        OpenClipboard(GetView()->m_hWnd);
        EmptyClipboard ();
        SetClipboardData (uFormat, hGlobalMemory);
        CloseClipboard ();
    }
    file.Close();

    TRY  {
        CFile::Remove(csClipFile);
    }
    CATCH(CFileException, e) {
        CIMSAString cs;
        cs.Format(_T("Warning:  Clipboard scratch file %s cannot be removed."), csClipFile.GetString());
        AfxMessageBox(cs,MB_OK | MB_ICONINFORMATION);
    }
    END_CATCH
}

/////////////////////////////////////////////////////////////////////////////
//
//                            CDDDoc::ClipToFile
//
/////////////////////////////////////////////////////////////////////////////

bool CFormDoc::ClipToFile(UINT uFormat)
{
    HGLOBAL hClipData, hText;
    ASSERT(IsClipboardFormatAvailable(uFormat));     // OnUpdateUI should prevent us from arriving here if this isn't the case

    OpenClipboard(GetView()->m_hWnd);
    if ((hClipData = GetClipboardData (uFormat)) == nullptr) {
        // Nothing there
        CloseClipboard();
        return FALSE;
    }
    if ((hText = GlobalAlloc (GHND, GlobalSize(hClipData)+1)) == nullptr) {
        AfxMessageBox(_T("Clipboard error"));
        CloseClipboard();
        return FALSE;
    }
    char FAR* lpClipData = (char FAR*) GlobalLock(hClipData);
    char FAR* lpszText = (char FAR*) GlobalLock(hText);
    lstrcpyA (lpszText, lpClipData);
    GlobalUnlock (hClipData);
    GlobalUnlock (hText);
    CloseClipboard();

    CString csClipFile = GetClipFile();
    //CStdioFile file(csClipFile, CFile::modeCreate|CFile::typeBinary|CFile::modeWrite);
    //file.WriteString (lpszText);

    // GHM 20120625 changed for unicode
    CFile file(csClipFile, CFile::modeCreate|CFile::typeBinary|CFile::modeWrite);
    file.Write(lpszText,strlen(lpszText));

    file.Close();
    return TRUE;
}
