#include "StdAfx.h"
#include "AplDoc.h"
#include "NewFileCreator.h"
#include <zUtilO/ArrUtil.h>
#include <zUtilF/ProgressDlg.h>
#include <zCapiO/QSFView.h>
#include <Zentryo/Runaple.h>
#include <regex>


namespace
{
    CString GetCapiItemName(const CDEItemBase* pBase)
    {
        const CDEBlock* block = dynamic_cast<const CDEBlock*>(pBase);
        return ( block != nullptr ) ? block->GetName() :
                                      assert_cast<const CDEField*>(pBase)->GetDictItem()->GetQualifiedName();
    }
}


/////////////////////////////////////////////////////////////////////////////
// CAplDoc
#include "shlwapi.h"

IMPLEMENT_DYNCREATE(CAplDoc, CDocument)

CAplDoc::CAplDoc()
    :   m_application(std::make_unique<Application>())
{
    m_bSrcLoaded = false;
    m_bIsClosing = false;
    m_bIsClosing = false;
    m_deployWnd = nullptr;
}

BOOL CAplDoc::OnNewDocument()
{
    if (!CDocument::OnNewDocument())
        return FALSE;
    return TRUE;
}

CAplDoc::~CAplDoc()
{
    if(m_application->GetAppSrcCode()) {
        delete m_application->GetAppSrcCode();
        m_application->SetAppSrcCode(nullptr);
    }
}


BEGIN_MESSAGE_MAP(CAplDoc, CDocument)
    //{{AFX_MSG_MAP(CAplDoc)
    ON_COMMAND(ID_FILE_CSPRO_CLOSE, OnFileClose)
    //}}AFX_MSG_MAP
END_MESSAGE_MAP()

/////////////////////////////////////////////////////////////////////////////
// CAplDoc diagnostics

#ifdef _DEBUG
void CAplDoc::AssertValid() const
{
    CDocument::AssertValid();
}

void CAplDoc::Dump(CDumpContext& dc) const
{
    CDocument::Dump(dc);
}
#endif //_DEBUG


/////////////////////////////////////////////////////////////////////////////
// CAplDoc commands

BOOL CAplDoc::OnOpenDocument(LPCTSTR lpszPathName)
{
    // load the application object
    if( !CDocument::OnOpenDocument(lpszPathName) )
        return FALSE;

    try
    {
        m_application->Open(lpszPathName);
    }

    catch( const CSProException& exception )
    {
        ErrorMessage::Display(exception);
        return FALSE;
    }

    // allocate memory for the CSourceCode object
    CSourceCode* pSourceCode = new CSourceCode(*m_application);
    m_application->SetAppSrcCode(pSourceCode);

    // make sure tabulation applications have a working storage dictionary
    if( m_application->GetEngineAppType() == EngineAppType::Tabulation )
    {
        const std::wstring& working_storage_dictionary_filename = m_application->GetFirstDictionaryFilenameOfType(DictionaryType::Working);

        if( !PortableFunctions::FileIsRegular(working_storage_dictionary_filename) )
        {
            AfxMessageBox(working_storage_dictionary_filename.empty() ?
                _T("Missing working storage dictionary.\n\nA new working storage dictionary will be added to the application.") :
                _T("This application's working storage dictionary is missing. It will be recreated."));

            try
            {
                NewFileCreator::CreateWorkingStorageDictionary(*m_application);
                SetModifiedFlag();
            }

            catch( const CSProException& exception )
            {
                ErrorMessage::Display(exception);
                return FALSE;
            }
        }
    }

    return TRUE;
}

/********************************************************************************
BuildAllTrees Adds the labels of the objects to the tree if it  is opened standalone
then hParent = TVI_ROOTITEM else the hParent is hItem of the Project
*********************************************************************************/

HTREEITEM CAplDoc::BuildAllTrees()
{
    //Get the handle to the Tree Control
    CMDlgBar& dlgBar = assert_cast<CMainFrame*>(AfxGetMainWnd())->GetDlgBar();

    CObjTreeCtrl&   ObjTree = dlgBar.m_ObjTree;
    CDDTreeCtrl&    dictTree = dlgBar.m_DictTree;
    CTabTreeCtrl&   tabTree = dlgBar.m_TableTree;
    CFormTreeCtrl&  formTree = dlgBar.m_FormTree;
    COrderTreeCtrl& orderTree = dlgBar.m_OrderTree;

    AppFileType app_file_type = ( m_application->GetEngineAppType() == EngineAppType::Entry )      ? AppFileType::ApplicationEntry :
                                ( m_application->GetEngineAppType() == EngineAppType::Batch )      ? AppFileType::ApplicationBatch :
                                ( m_application->GetEngineAppType() == EngineAppType::Tabulation ) ? AppFileType::ApplicationTabulation :
                                                                                                     ReturnProgrammingError(AppFileType::ApplicationBatch);

    HTREEITEM hRet = ObjTree.InsertNode(TVI_ROOT, std::make_unique<ApplicationFileTreeNode>(app_file_type, CS2WS(m_application->GetApplicationFilename())));

    //Insert label for the child items

    //Forms
    if(m_application->GetEngineAppType() == EngineAppType::Entry) {

        HTREEITEM hForm = hRet;
        for( const CString& form_filename : m_application->GetFormFilenames() )
        {
            CSpecFile specFormFile (true); //do it silently

            if(specFormFile.Open(form_filename, CFile::modeRead)){

                ObjTree.InsertFormNode(hForm, CS2WS(form_filename), AppFileType::Form);

                CFormNodeID* pID = formTree.GetFormNode(form_filename);

                if(pID) {
                    pID->AddRef();

                }
                else {
                    CString sLabel = ValFromHeader(specFormFile,CSPRO_CMD_LABEL);

                    HTREEITEM hItem = formTree.InsertFormFile(sLabel, form_filename, nullptr);
                    pID =formTree.GetFormNode(form_filename);

                    TVITEM pItem;
                    pItem.hItem = hItem;
                    pItem.mask = TVIF_CHILDREN ;
                    pItem.cChildren = 1;
                    formTree.SetItem(&pItem);
                }

                ASSERT(pID != nullptr);
                formTree.InsertFormDependencies(pID);
                specFormFile.Close();
            }
            else {
                CString sString;
                sString.FormatMessage(IDS_OPENAPPFLD, form_filename.GetString());
                AfxMessageBox(sString);
                continue;
            }
        }
    }

    //Orders
    else if(m_application->GetEngineAppType() == EngineAppType::Batch) {

        HTREEITEM hOrder = hRet;

        for( const CString& form_filename : m_application->GetFormFilenames() )
        {
            CSpecFile specFormFile (true); //do it silently

            if(specFormFile.Open(form_filename, CFile::modeRead)){

                ObjTree.InsertFormNode(hOrder, CS2WS(form_filename), AppFileType::Order);

                FormOrderAppTreeNode* form_order_app_tree_node = orderTree.GetFormOrderAppTreeNode(form_filename);

                if(form_order_app_tree_node != nullptr) {
                    form_order_app_tree_node->AddRef();
                }
                else {
                    CString sLabel = ValFromHeader(specFormFile,CSPRO_CMD_LABEL);

                    HTREEITEM hItem = orderTree.InsertOrderFile(sLabel, form_filename, nullptr);
                    form_order_app_tree_node = orderTree.GetFormOrderAppTreeNode(form_filename);

                    TVITEM pItem;
                    pItem.hItem = hItem;
                    pItem.mask = TVIF_CHILDREN ;
                    pItem.cChildren = 1;
                    orderTree.SetItem(&pItem);
                }

                ASSERT(form_order_app_tree_node != nullptr);
                orderTree.InsertOrderDependencies(*form_order_app_tree_node);
                specFormFile.Close();
            }
            else {
                CString sString;
                sString.FormatMessage(IDS_OPENAPPFLD, form_filename.GetString());
                AfxMessageBox(sString);
                continue;
            }
        }
    }


    // Input TabSpecs
    else if(m_application->GetEngineAppType() == EngineAppType::Tabulation) {

        HTREEITEM hSpec = hRet;

        for( const CString& tab_spec_filename : m_application->GetTabSpecFilenames() )
        {
            CSpecFile specTabFile(TRUE); //do it silently
            if(specTabFile.Open(tab_spec_filename, CFile::modeRead)){

                ObjTree.InsertTableNode(hSpec, CS2WS(tab_spec_filename));

                TableSpecTabTreeNode* table_spec_tab_tree_node = tabTree.GetTableSpecTabTreeNode(tab_spec_filename);

                if( table_spec_tab_tree_node != nullptr ) {
                    table_spec_tab_tree_node->AddRef();
                }
                else {
                    HTREEITEM hItem = tabTree.InsertTableSpec(tab_spec_filename, nullptr);
                    table_spec_tab_tree_node = tabTree.GetTableSpecTabTreeNode(tab_spec_filename);

                    TVITEM pItem;
                    pItem.hItem = hItem;
                    pItem.mask = TVIF_CHILDREN ;
                    pItem.cChildren = 1;
                    tabTree.SetItem(&pItem);
                }

                ASSERT(table_spec_tab_tree_node != nullptr);
                tabTree.InsertTableDependencies(*table_spec_tab_tree_node);

                specTabFile.Close();
            }
            else {
                CString sString;
                sString.FormatMessage(IDS_OPENAPPFLD, tab_spec_filename.GetString());
                AfxMessageBox(sString);
                continue;
            }
        }
    }

    // Insert External Dictionaries
    HTREEITEM hExternal = hRet;

    for( const CString& dictionary_filename : m_application->GetExternalDictionaryFilenames() )
    {
        DictionaryDictTreeNode* dictionary_dict_tree_node = dictTree.GetDictionaryTreeNode(dictionary_filename);

        if( dictionary_dict_tree_node != nullptr )
        {
            ObjTree.InsertNode(hExternal, std::make_unique<DictionaryFileTreeNode>(CS2WS(dictionary_filename)));
            dictionary_dict_tree_node->AddRef();
        }

        else
        {
            try
            {
                LabelSet dictionary_label_set = JsonStream::GetValueFromSpecFile<LabelSet, CDataDict>(JK::labels, dictionary_filename);

                ObjTree.InsertNode(hExternal, std::make_unique<DictionaryFileTreeNode>(CS2WS(dictionary_filename)));

                TVITEM pItem;
                pItem.hItem = dictTree.InsertDictionary(dictionary_label_set.GetLabel(), dictionary_filename, nullptr);
                pItem.mask = TVIF_CHILDREN;
                pItem.cChildren = 1;
                dictTree.SetItem(&pItem);
            }

            catch( const CSProException& exception )
            {
                ErrorMessage::Display(exception);
            }
        }
    }


    // code files
    {
        const CodeFile* logic_main_code_file = m_application->GetLogicMainCodeFile();

        if( logic_main_code_file != nullptr )
        {
            HTREEITEM hCodeParentItem = ObjTree.InsertNode(hRet, std::make_unique<CodeFileTreeNode>(*logic_main_code_file));

            for( const CodeFile& code_file : m_application->GetCodeFiles() )
            {
                if( &code_file != logic_main_code_file )
                    ObjTree.InsertNode(hCodeParentItem, std::make_unique<CodeFileTreeNode>(code_file));
            }
        }

        else
        {
            ASSERT(false);
        }
    }
    

    // message files
    {
        HTREEITEM hMessageParentItem = hRet;
        bool external_messages = false;

        for( const auto& message_text_source : m_application->GetMessageTextSources() )
        {
            HTREEITEM hItem = ObjTree.InsertNode(hMessageParentItem, std::make_unique<MessageFileTreeNode>(message_text_source->GetFilename(), external_messages));

            if( !external_messages )
            {
                hMessageParentItem = hItem;
                external_messages = true;
            }
        }
    }


    // question text
    if( m_application->GetEngineAppType() == EngineAppType::Entry )
    {
        // 20100624 when QSF files didn't exist they were getting set up as ".qsf" which was:
        // 1) not a good filename, and 2) causing problems with Save As
        std::optional<CString> modified_qsf_filename;

        if( m_application->GetQuestionTextFilename().IsEmpty() )
        {
            modified_qsf_filename = m_application->GetApplicationFilename() + FileExtensions::WithDot::QuestionText;
        }

        else if( !SO::EqualsNoCase(PortableFunctions::PathGetFileExtension(m_application->GetQuestionTextFilename()), FileExtensions::QuestionText) )
        {
            modified_qsf_filename = PortableFunctions::PathRemoveFileExtension<CString>(m_application->GetQuestionTextFilename()) + FileExtensions::WithDot::QuestionText;
        }

        if( modified_qsf_filename.has_value() )
        {
            m_application->SetQuestionTextFilename(*modified_qsf_filename);
            SetModifiedFlag(true);
        }

        ObjTree.InsertNode(hRet, std::make_unique<QuestionTextFileTreeNode>(CS2WS(m_application->GetQuestionTextFilename())));
    }


    // reports
    for( const auto& report_named_text_sources : m_application->GetReportNamedTextSources() )
        ObjTree.InsertNode(hRet, std::make_unique<ReportFileTreeNode>(report_named_text_sources->text_source->GetFilename()));


    // resource folders
    for( const CString& folder_name : m_application->GetResourceFolders() )
        ObjTree.InsertNode(hRet, std::make_unique<ResourceFolderTreeNode>(CS2WS(folder_name)));

    return hRet;
}

// ****************************************************************************

void CAplDoc::OnFileClose()
{
    // TODO: Add your command handler code here
    CDocument::OnFileClose();

}

// ****************************************************************************

void CAplDoc::OnCloseDocument()
{
    //release the references of the dictionaries , forms ,tables
    //then if the references are zero then close the documents corresponding to
    //these objects .
    ReleaseTabSpecs();  //Release Tab Specs
    ReleaseForms();     //Release Forms
    ReleaseOrders();    //Release the orders
    ReleaseEDicts();    //Release the external dictionaries

    CMainFrame* pFrame = assert_cast<CMainFrame*>(AfxGetMainWnd());
    FileTreeNode* file_tree_node = pFrame->GetDlgBar().m_ObjTree.FindNode(this);

    CDocument::OnCloseDocument();

    //remove the application node on the object tree .
    if( file_tree_node != nullptr )
        pFrame->GetDlgBar().m_ObjTree.DeleteNode(*file_tree_node);
}

// ****************************************************************************

BOOL CAplDoc::OnSaveDocument(LPCTSTR lpszPathName)
{
    //Here save the input/output   dictionaries , forms , tables first
    //write them to the disk and then write the application file to
    //register the appropriate date/time stamp
    //Take care of the order of saving

    //Reconcile Dict types
    ReconcileDictTypes();

    //Save the dictionaries
    SaveAllDictionaries();

    //Save the tab specs
    if( m_application->GetEngineAppType() == EngineAppType::Tabulation ) {
        SaveTabSpecs();
    }

    //Save the forms
    else if(m_application->GetEngineAppType() == EngineAppType::Entry) {
        SaveForms();
        if(m_pQuestMgr != nullptr && m_pQuestMgr->IsModified()) {
            m_pQuestMgr->Save(CS2WS(m_application->GetQuestionTextFilename()));
        }
    }

    //Save the orders
    else if( m_application->GetEngineAppType() == EngineAppType::Batch ) {
        SaveOrders();
    }


    // save the logic
    if( m_application->GetAppSrcCode() != nullptr && m_application->GetAppSrcCode()->IsModified() )
    {
        try
        {
            m_application->GetAppSrcCode()->Save();
            m_application->GetAppSrcCode()->SetModifiedFlag(false);
        }

        catch( const CSProException& exception )
        {
            ErrorMessage::Display(exception);
        }
    }

    // save the text sources (external code, messages, and reports)
    auto save_text_source = [](TextSource& text_source)
    {
        try
        {
            text_source.Save();
        }

        catch( const CSProException& exception )
        {
            ErrorMessage::Display(exception);
        }

    };

    for( CodeFile& code_file : m_application->GetCodeFilesIterator() )
    {
        if( !code_file.IsLogicMain() )
            save_text_source(code_file.GetTextSource());
    }

    for( size_t index = 0; index < m_application->GetMessageTextSources().size(); ++index )
    {
        // external message files currently aren't editable so only save the first one
        if( index == 0 )
        {
            save_text_source(*m_application->GetMessageTextSources()[index]);
        }

        else
        {
            ASSERT(( std::dynamic_pointer_cast<TextSourceExternal, TextSource>(m_application->GetMessageTextSources()[index]) != nullptr ));
        }
    }

    for( const auto& report_named_text_sources : m_application->GetReportNamedTextSources() )
        save_text_source(*report_named_text_sources->text_source);


    // save the application object
    try
    {
        m_application->Save(lpszPathName);
        SetModifiedFlag(FALSE);
    }

    catch( const CSProException& exception )
    {
        ErrorMessage::Display(exception);
        return FALSE;
    }

    return TRUE;
}


/////////////////////////////////////////////////////////////////////////////
//
//                        CAplDoc::AreAplDictsOK
//
/////////////////////////////////////////////////////////////////////////////

BOOL CAplDoc::AreAplDictsOK() {            // BMD  28 Jun 00

    bool bOK = true;
    CMainFrame* pFrame = assert_cast<CMainFrame*>(AfxGetMainWnd());
    CDDTreeCtrl& dictTree = pFrame->GetDlgBar().m_DictTree;
    // Examine external dictionaries
    for( const CString& sDictFName : m_application->GetExternalDictionaryFilenames() ) {
        DictionaryDictTreeNode* dictionary_dict_tree_node = dictTree.GetDictionaryTreeNode(sDictFName);
        if (dictionary_dict_tree_node != nullptr && dictionary_dict_tree_node->GetDDDoc() != nullptr) {
            bOK &= dictionary_dict_tree_node->GetDDDoc()->GetDictionaryValidator()->IsValidSave(*dictionary_dict_tree_node->GetDDDoc()->GetDict());
        }
    }
    EngineAppType appType = m_application->GetEngineAppType();
    // Examine form dictionaries
    if(appType == EngineAppType::Entry) {
        CFormTreeCtrl& formTree = pFrame->GetDlgBar().m_FormTree;
        for( const CString& sFormFileName : m_application->GetFormFilenames() ) {
            CFormNodeID* pID = formTree.GetFormNode(sFormFileName);
            if(pID != nullptr && pID->GetFormDoc()) {
                CDEFormFile* pFormFile = &pID->GetFormDoc()->GetFormFile();
                CString sDictName = pFormFile->GetDictionaryFilename();
                DictionaryDictTreeNode* dictionary_dict_tree_node = dictTree.GetDictionaryTreeNode(sDictName);
                if (dictionary_dict_tree_node != nullptr && dictionary_dict_tree_node->GetDDDoc() != nullptr) {
                    bOK &= dictionary_dict_tree_node->GetDDDoc()->GetDictionaryValidator()->IsValidSave(*dictionary_dict_tree_node->GetDDDoc()->GetDict());
                }
            }
        }
    }
    return bOK;
}

// ****************************************************************************

void CAplDoc::SaveAllDictionaries()
{
    CMainFrame* pFrame = assert_cast<CMainFrame*>(AfxGetMainWnd());
    CDDTreeCtrl& dictTree = pFrame->GetDlgBar().m_DictTree;
    //Save the input dictionaries
    for( const CString& sDictFName : m_application->GetExternalDictionaryFilenames() ) {
        //get at the dictionary tree and get the documents
            DictionaryDictTreeNode* dictionary_dict_tree_node = dictTree.GetDictionaryTreeNode(sDictFName);
            if (dictionary_dict_tree_node != nullptr && dictionary_dict_tree_node->GetDDDoc() != nullptr) {
                if(dictionary_dict_tree_node->GetDDDoc()->IsModified()){
                    dictionary_dict_tree_node->GetDDDoc()->OnSaveDocument(sDictFName);
                }
            }
    }

    EngineAppType appType = m_application->GetEngineAppType();

    if(appType == EngineAppType::Entry) {
        SaveFormDicts();
    }
    else if(appType == EngineAppType::Batch) {
        SaveOrderDicts();
    }
    else if(appType == EngineAppType::Tabulation) {
        SaveTableDicts();
    }
}

// ****************************************************************************

void CAplDoc::SaveTabSpecs()
{
    CMainFrame* pFrame = assert_cast<CMainFrame*>(AfxGetMainWnd());
    CTabTreeCtrl& tableTree = pFrame->GetDlgBar().m_TableTree;
    //Save the input dictionaries
    for( const CString& tab_spec_filename : m_application->GetTabSpecFilenames() ) {
        //get at the dictionary tree and get the documents
        TableSpecTabTreeNode* table_spec_tab_tree_node = tableTree.GetTableSpecTabTreeNode(tab_spec_filename);
        if(table_spec_tab_tree_node->GetTabDoc()){
            if(table_spec_tab_tree_node->GetTabDoc()->IsModified()){
                table_spec_tab_tree_node->GetTabDoc()->OnSaveDocument(tab_spec_filename);
            }
            CTabulateDoc* pTabDoc = table_spec_tab_tree_node->GetTabDoc();
            //Update the source code if required
            HTREEITEM hItem =  pTabDoc->GetTabTreeCtrl()->GetSelectedItem();
            if(!hItem)
                return;

            TableElementTreeNode* table_element_tree_node = pTabDoc->GetTabTreeCtrl()->GetTreeNode(hItem);
            if(table_element_tree_node != nullptr && table_element_tree_node->GetTabDoc() == pTabDoc) {
                AfxGetMainWnd()->SendMessage(UWM::Table::PutSourceCode, 0, reinterpret_cast<LPARAM>(table_element_tree_node));
            }
        }
    }
}


void CAplDoc::SaveTableDicts()
{
    CMainFrame* pFrame = assert_cast<CMainFrame*>(AfxGetMainWnd());

    CTabTreeCtrl& tableTree = pFrame->GetDlgBar().m_TableTree;
    CDDTreeCtrl&   dictTree = pFrame->GetDlgBar().m_DictTree;
    // Save the table dicts

    for( const CString& tab_spec_filename : m_application->GetTabSpecFilenames() )
    {
        //Save the dictionaries of the tab specs files
        TableSpecTabTreeNode* table_spec_tab_tree_node = tableTree.GetTableSpecTabTreeNode(tab_spec_filename);

        if(table_spec_tab_tree_node && table_spec_tab_tree_node->GetTabDoc()) {

            CTabSet* pTabSpec = table_spec_tab_tree_node->GetTabDoc()->GetTableSpec();
            int iNumDict = 1; // HARD CODED TO 1 dict in the spec
            // SAVY && change when this is implemented
            for(int iDict = 0; iDict<iNumDict; iDict++)
            {
                //To Do Get the dict path SAVY&&& 11/05/02
                CString sDictName = pTabSpec->GetDictFile();
                DictionaryDictTreeNode* dictionary_dict_tree_node = dictTree.GetDictionaryTreeNode(sDictName);
                if(dictionary_dict_tree_node != nullptr && dictionary_dict_tree_node->GetDDDoc() != nullptr){
                    if(dictionary_dict_tree_node->GetDDDoc()->IsModified()) {
                        dictionary_dict_tree_node->GetDDDoc()->OnSaveDocument(sDictName);
                    }
                }
            }
        }
    }
}


// ****************************************************************************

void CAplDoc::SaveForms()
{
    CMainFrame* pFrame = assert_cast<CMainFrame*>(AfxGetMainWnd());
    CFormTreeCtrl& formTree = pFrame->GetDlgBar().m_FormTree;

    // Save the input dicts

    for( const CString& form_filename : m_application->GetFormFilenames() )
    {
        //get at the dictionary tree and get the documents

        CFormNodeID* pID = formTree.GetFormNode(form_filename);

        if(pID != nullptr && pID->GetFormDoc()) {
            CFormDoc* pFormDoc = pID->GetFormDoc();
            if(pFormDoc->IsModified()) {
                pFormDoc->OnSaveDocument(form_filename);
                pFormDoc->SetModifiedFlag(FALSE);
            }

            //Update the source code if required
            HTREEITEM hItem = pFormDoc->GetFormTreeCtrl()->GetSelectedItem();
            CFormID* pFID = (CFormID*)pFormDoc->GetFormTreeCtrl()->GetItemData(hItem);

            if(pFID && pFID->GetFormDoc() == pFormDoc) {
                AfxGetMainWnd()->SendMessage(UWM::Form::PutSourceCode, 0, reinterpret_cast<LPARAM>(pFID));
            }
        }
    }
}


void CAplDoc::SaveFormDicts()
{
    CMainFrame* pFrame = assert_cast<CMainFrame*>(AfxGetMainWnd());

    CFormTreeCtrl& formTree = pFrame->GetDlgBar().m_FormTree;
    CDDTreeCtrl&   dictTree = pFrame->GetDlgBar().m_DictTree;

    // Save the Form dicts
    for( const CString& sFormFileName : m_application->GetFormFilenames() )
    {
        //Save  the dictioanries of the form files
        CFormNodeID* pID = formTree.GetFormNode(sFormFileName);

        if(pID != nullptr && pID->GetFormDoc()) {
            CDEFormFile* pFormFile = &pID->GetFormDoc()->GetFormFile();
            CString sDictName = pFormFile->GetDictionaryFilename();
            DictionaryDictTreeNode* dictionary_dict_tree_node = dictTree.GetDictionaryTreeNode(sDictName);
            if(dictionary_dict_tree_node != nullptr && dictionary_dict_tree_node->GetDDDoc() != nullptr) {
                if(dictionary_dict_tree_node->GetDDDoc()->IsModified()) {
                    dictionary_dict_tree_node->GetDDDoc()->OnSaveDocument(sDictName);
                }
            }
        }
    }
}


// SAVY Save Orders Updated for CSBatch 05/18/00
void CAplDoc::SaveOrders()
{
    CMainFrame* pFrame = assert_cast<CMainFrame*>(AfxGetMainWnd());

    COrderTreeCtrl& OrderTree = pFrame->GetDlgBar().m_OrderTree;

    for( const CString& form_filename : m_application->GetFormFilenames() ) {
        //get at the dictionary tree and get the documents

        FormOrderAppTreeNode* form_order_app_tree_node = OrderTree.GetFormOrderAppTreeNode(form_filename);

        if(form_order_app_tree_node->GetDocument() != nullptr) {
            COrderDoc* pOrderDoc = form_order_app_tree_node->GetOrderDocument();
            if(pOrderDoc->IsModified()){
                pOrderDoc->OnSaveDocument(form_filename);
                pOrderDoc->SetModifiedFlag(FALSE);
            }

            //Update the source code if required
            HTREEITEM hItem = pOrderDoc->GetOrderTreeCtrl()->GetSelectedItem();

            if(!hItem)
                return;

            AppTreeNode* app_tree_node = pOrderDoc->GetOrderTreeCtrl()->GetTreeNode(hItem);
            if(app_tree_node != nullptr && app_tree_node->GetDocument() == pOrderDoc) {
                AfxGetMainWnd()->SendMessage(UWM::Order::PutSourceCode, 0, reinterpret_cast<LPARAM>(app_tree_node));
            }
        }
    }
}

//SAVY 05/18/00 No Update required for CSBatch
void CAplDoc::SaveOrderDicts()
{
    CMainFrame* pFrame = assert_cast<CMainFrame*>(AfxGetMainWnd());

    COrderTreeCtrl& orderTree = pFrame->GetDlgBar().m_OrderTree;
    CDDTreeCtrl&   dictTree = pFrame->GetDlgBar().m_DictTree;

    //Save the dictionaries of the order files
    for( const CString& sOrderFileName : m_application->GetFormFilenames() )
    {
        FormOrderAppTreeNode* form_order_app_tree_node = orderTree.GetFormOrderAppTreeNode(sOrderFileName);

        if(form_order_app_tree_node != nullptr && form_order_app_tree_node->GetDocument() != nullptr) {
            CDEFormFile* pOrderFile = &form_order_app_tree_node->GetOrderDocument()->GetFormFile();
            DictionaryDictTreeNode* dictionary_dict_tree_node = dictTree.GetDictionaryTreeNode(pOrderFile->GetDictionaryFilename());
            if(dictionary_dict_tree_node != nullptr && dictionary_dict_tree_node->GetDDDoc() != nullptr) {
                if(dictionary_dict_tree_node->GetDDDoc()->IsModified()){
                    dictionary_dict_tree_node->GetDDDoc()->OnSaveDocument(pOrderFile->GetDictionaryFilename());
                }
            }
        }
    }
}


void CAplDoc::ReleaseEDicts()
{
    CMainFrame* pFrame = assert_cast<CMainFrame*>(AfxGetMainWnd());

    CDDTreeCtrl& dictTree = pFrame->GetDlgBar().m_DictTree;

    for( const CString& sDictFName : m_application->GetExternalDictionaryFilenames() )
    {
        //get at the form tree, get the docs, and release them
        DictionaryDictTreeNode* dictionary_dict_tree_node = dictTree.GetDictionaryTreeNode(sDictFName);

        if( dictionary_dict_tree_node != nullptr )
        {
            dictTree.SetRedraw(FALSE);//turn the treecontrol draw off while releasing the dictionaries. 'potential cause of crash due to getlabel  while the objects are getting deleted.
            dictTree.ReleaseDictionaryNode(*dictionary_dict_tree_node);
            dictTree.SetRedraw(TRUE);
        }
    }
}


void CAplDoc::ReleaseForms()
{
    if(m_application->GetEngineAppType() != EngineAppType::Entry)
        return;

    m_bIsClosing = true;

    CMainFrame* pFrame = assert_cast<CMainFrame*>(AfxGetMainWnd());
    CFormTreeCtrl& formTree = pFrame->GetDlgBar().m_FormTree;
    formTree.SetSndMsgFlg(FALSE);
    for( const CString& form_filename : m_application->GetFormFilenames() ) {
        //get at the form tree, get the docs, and release them
        CFormNodeID* pID = formTree.GetFormNode(form_filename);
        // ASSERT(AfxIsValidAddress( (void*)pID, sizeof(CFormNodeID)));
        if(!pID)
            continue;
        formTree.ReleaseFormDependencies(pID);
        formTree.ReleaseFormNodeID(pID);

        pID = formTree.GetFormNode(form_filename);
        if(pID != nullptr && pID->GetFormDoc()) {
            CView* pView = pID->GetFormDoc()->GetView();
            CFormChildWnd* pFormChildWnd = (CFormChildWnd*)pView->GetParentFrame();
            if(pFormChildWnd->IsLogicViewActive()) {
                AfxGetMainWnd()->PostMessage(UWM::Designer::ShowToolbar, (WPARAM)FrameType::Form);
            }
        }
    }

    formTree.SetSndMsgFlg(TRUE);
}

//SAVY 05/18/00 No Update required for CSBatch
void CAplDoc::ReleaseOrders()
{
    if(m_application->GetEngineAppType() != EngineAppType::Batch)
        return;

    m_bIsClosing = true;
    CMainFrame* pFrame = assert_cast<CMainFrame*>(AfxGetMainWnd());

    COrderTreeCtrl& orderTree = pFrame->GetDlgBar().m_OrderTree;

    for( const CString& form_filename : m_application->GetFormFilenames() ) {
        //get at the form tree, get the docs, and release them

        FormOrderAppTreeNode* form_order_app_tree_node = orderTree.GetFormOrderAppTreeNode(form_filename);
        if(form_order_app_tree_node == nullptr)
            continue;
        orderTree.ReleaseOrderDependencies(*form_order_app_tree_node);
        orderTree.ReleaseOrderNode(*form_order_app_tree_node);

        form_order_app_tree_node = orderTree.GetFormOrderAppTreeNode(form_filename);

        if(form_order_app_tree_node != nullptr && form_order_app_tree_node->GetDocument() != nullptr) {
            POSITION pos = form_order_app_tree_node->GetOrderDocument()->GetFirstViewPosition();
            COrderChildWnd* pOrderChildWnd = (COrderChildWnd*)form_order_app_tree_node->GetOrderDocument()->GetNextView(pos)->GetParentFrame();
            if(pOrderChildWnd) {
                AfxGetMainWnd()->PostMessage(UWM::Designer::ShowToolbar, (WPARAM)FrameType::Order);
            }
        }
    }
}

// ****************************************************************************

void CAplDoc::ReleaseTabSpecs()
{
    CMainFrame* pFrame = assert_cast<CMainFrame*>(AfxGetMainWnd());
    CTabTreeCtrl& tableTree = pFrame->GetDlgBar().m_TableTree;

    for( const CString& tab_spec_filename : m_application->GetTabSpecFilenames() )
    {
        //get at the table  tree and get the documents and release the docs
        TableSpecTabTreeNode* table_spec_tab_tree_node = tableTree.GetTableSpecTabTreeNode(tab_spec_filename);
        if(table_spec_tab_tree_node != nullptr) {
            tableTree.ReleaseTableDependencies(*table_spec_tab_tree_node);
            tableTree.ReleaseTableNode(*table_spec_tab_tree_node);
        }
    }
}


std::vector<const CDataDict*> CAplDoc::GetAllDictsInApp()
{
    Application& app = GetAppObject();
    CMainFrame* pFrame = assert_cast<CMainFrame*>(AfxGetMainWnd());
    CStringArray aDictFNames;

    switch (GetEngineAppType()) {
        case EngineAppType::Entry:
            {
                CFormTreeCtrl& formTree = pFrame->GetDlgBar().m_FormTree;
                for( const CString& form_filename : app.GetFormFilenames() ) {

                    CFormNodeID* pFormNode = formTree.GetFormNode(form_filename);
                    ASSERT(pFormNode);
                    CFormDoc* pFormDoc = pFormNode->GetFormDoc();
                    ASSERT_VALID(pFormDoc);
                    CDEFormFile* pFormSpec = &pFormDoc->GetFormFile();
                    aDictFNames.Add(pFormSpec->GetDictionaryFilename());
                }
            }
            break;
        case EngineAppType::Tabulation:
            {
                ASSERT(app.GetTabSpecFilenames().size() == 1);
                CTabTreeCtrl& tabTree = pFrame->GetDlgBar().m_TableTree;
                TableSpecTabTreeNode* table_spec_tab_tree_node = tabTree.GetTableSpecTabTreeNode(app.GetTabSpecFilenames().front());
                CTabulateDoc* pTabDoc = table_spec_tab_tree_node->GetTabDoc();
                ASSERT_VALID(pTabDoc);
                CTabSet* pTabSpec = pTabDoc->GetTableSpec();
                ASSERT_VALID(pTabSpec);

                // get dictionary
                aDictFNames.Add(pTabSpec->GetDictFile());
            }
            break;

        case EngineAppType::Batch:
            {
                COrderTreeCtrl& orderTree = pFrame->GetDlgBar().m_OrderTree;
                for( const CString& form_filename : m_application->GetFormFilenames() ) {

                    // get dictionary children
                    const FormOrderAppTreeNode* form_order_app_tree_node = orderTree.GetFormOrderAppTreeNode(form_filename);
                    ASSERT(form_order_app_tree_node != nullptr);
                    const COrderDoc* pOrderDoc = form_order_app_tree_node->GetOrderDocument();
                    ASSERT_VALID(pOrderDoc);

                    aDictFNames.Add(pOrderDoc->GetFormFile().GetDictionaryFilename());
                }
            }
            break;

        default:
            ASSERT(!_T("INVALID APP TYPE FOR SAVE AS"));
    }

    // add external dicts
    AppendUnique(aDictFNames, GetAppObject().GetExternalDictionaryFilenames());

    // now get the actual dictionaries (rather than the names)
    std::vector<const CDataDict*> dictionaries;

    CDDTreeCtrl& dictTree = pFrame->GetDlgBar().m_DictTree;
    for( int iDict = 0; iDict < aDictFNames.GetSize(); iDict++ )
    {
        DictionaryDictTreeNode* dictionary_dict_tree_node = dictTree.GetDictionaryTreeNode(aDictFNames[iDict]);
        ASSERT(dictionary_dict_tree_node != nullptr);
        ASSERT_VALID(dictionary_dict_tree_node->GetDDDoc());
        dictionaries.emplace_back(dictionary_dict_tree_node->GetDDDoc()->GetDict());
    }

    return dictionaries;
}

// Function name    : CAplDoc::SetAppObjects
// Description      : Sets the application objects for compile time (Now supports Forms && Orders)
// Return type      : void
// Argument         : void
void CAplDoc::SetAppObjects()
{
    //Assume all the dicts are open since this is the method we
    //are employing for an application
    m_application->GetRuntimeFormFiles().clear();

    //Get the form object from the memory
    CMainFrame* pFrame = assert_cast<CMainFrame*>(AfxGetMainWnd());
    CFormTreeCtrl& formTree = pFrame->GetDlgBar().m_FormTree;
    COrderTreeCtrl& orderTree = pFrame->GetDlgBar().m_OrderTree;
    CTabTreeCtrl& tabTree = pFrame->GetDlgBar().m_TableTree;

    CDDTreeCtrl& dictTree =  pFrame->GetDlgBar().m_DictTree;

    if(m_application->GetEngineAppType() == EngineAppType::Entry) {
        for( const CString& form_filename : m_application->GetFormFilenames() ) {
            CFormNodeID* pFormNode = formTree.GetFormNode(form_filename);
            if (pFormNode == nullptr)
                continue;
            ASSERT(pFormNode->GetFormDoc());

            std::shared_ptr<CDEFormFile> pFormFile = pFormNode->GetFormDoc()->GetSharedFormFile();
            m_application->AddRuntimeFormFile(pFormFile);

            DictionaryDictTreeNode* dictionary_dict_tree_node = dictTree.GetDictionaryTreeNode(pFormFile->GetDictionaryFilename());
            if(dictionary_dict_tree_node != nullptr && dictionary_dict_tree_node->GetDDDoc() != nullptr) {
                pFormFile->SetDictionary(dictionary_dict_tree_node->GetDDDoc()->GetSharedDictionary());
            }
        }
        SetEDictObjects();

        m_application->SetCapiQuestionManager(m_pQuestMgr);
    }

    else if (m_application->GetEngineAppType() == EngineAppType::Batch) {
        for( const CString& form_filename : m_application->GetFormFilenames() ) {
            FormOrderAppTreeNode* form_order_app_tree_node = orderTree.GetFormOrderAppTreeNode(form_filename);

            ASSERT(form_order_app_tree_node->GetDocument() != nullptr);

            std::shared_ptr<CDEFormFile> pOrderFile = form_order_app_tree_node->GetOrderDocument()->GetSharedFormFile();
            m_application->AddRuntimeFormFile(pOrderFile);

            DictionaryDictTreeNode* dictionary_dict_tree_node = dictTree.GetDictionaryTreeNode(pOrderFile->GetDictionaryFilename());
            CDDDoc* pDDDoc = dictionary_dict_tree_node->GetDDDoc();
            if(pDDDoc) {
                pOrderFile->SetDictionary(pDDDoc->GetSharedDictionary());
            }
        }
        SetEDictObjects();
    }

    else if (m_application->GetEngineAppType() == EngineAppType::Tabulation) {
        TableSpecTabTreeNode* table_spec_tab_tree_node = tabTree.GetTableSpecTabTreeNode(m_application->GetTabSpecFilenames().front());
        if(!table_spec_tab_tree_node->GetTabDoc()){
            return;
        }
        ASSERT(table_spec_tab_tree_node->GetTabDoc());
        CTabSet* pTabSet = table_spec_tab_tree_node->GetTabDoc()->GetTableSpec();

        ASSERT(pTabSet);
        SetEDictObjects();
        std::shared_ptr<const CDataDict> pWorkDataDict;

        for( size_t i = 0; i < m_application->GetExternalDictionaryFilenames().size(); ++i )
        {
            CString sDictFName = m_application->GetExternalDictionaryFilenames()[i];
            std::shared_ptr<CDataDict> pDataDict = m_application->GetRuntimeExternalDictionaries()[i];

            DictionaryDescription* dictionary_description = m_application->GetDictionaryDescription(sDictFName);

            if( dictionary_description == nullptr )
            {
                dictionary_description = m_application->AddDictionaryDescription(DictionaryDescription(CS2WS(sDictFName), DictionaryType::Working));
                pWorkDataDict = pDataDict;
            }

            else if( dictionary_description->GetDictionaryType() == DictionaryType::Working )
            {
                pWorkDataDict = pDataDict;
            }

            dictionary_description->SetDictionary(pDataDict.get());
        }

        ASSERT(pWorkDataDict);
        if(pWorkDataDict.get() != pTabSet->GetWorkDict()){
            pTabSet->SetWorkDict(pWorkDataDict);
        }
    }
}



// Function name    : CAplDoc::OpenAllDocuments
// Description      :Call this funciton only after a call of Build AllTrees
// Return type      : void
// Argument         : void

BOOL CAplDoc::OpenAllDocuments()
{
    ProgressDlgSharing share_progress_dialog;

    CMDlgBar& dlgBar = assert_cast<CMainFrame*>(AfxGetMainWnd())->GetDlgBar();

    //See the application type and open the relevant files
    const EngineAppType engine_app_type = m_application->GetEngineAppType();

    if( engine_app_type == EngineAppType::Entry )
    {
        try
        {
            BuildQuestMgr();
        }

        catch( const CSProException& exception )
        {
            ErrorMessage::Display(exception);
            return FALSE;
        }

        if( !ProcessFormOpen() )
            return FALSE;
    }

    else if( engine_app_type == EngineAppType::Batch )
    {
        if( !ProcessOrderOpen() )
            return FALSE;
    }

    else if( engine_app_type == EngineAppType::Tabulation )
    {
        if( !ProcessTabOpen() )
            return FALSE;
    }

    else
    {
        CString sMsg;
        sMsg.FormatMessage(IDS_INVLDAPPTYPE, this->GetPathName().GetString());
        AfxMessageBox(sMsg);
        return FALSE;
    }

    ProcessEDictsOpen();

    //Set the Application Objects for Compilation stuff
    SetAppObjects();

    //Load the source code from the .app file
    CSourceCode* pSourceCode = m_application->GetAppSrcCode();
    if(pSourceCode && !m_bSrcLoaded) {
        //SAVY 06/13 to support order of the formfile
        pSourceCode->SetOrder(this->GetOrder());

        m_bSrcLoaded = pSourceCode->Load();
    }

    ApplicationChildWnd* application_child_wnd = nullptr;

    if( engine_app_type == EngineAppType::Entry )
    {
        if( !Reconcile() )
            return FALSE;

        CFormNodeID* pNode = dlgBar.m_FormTree.GetFormNode(m_application->GetFormFilenames().front());

        if( pNode != nullptr )
        {
            POSITION pos = pNode->GetFormDoc()->GetFirstViewPosition();
            application_child_wnd = assert_cast<ApplicationChildWnd*>(pNode->GetFormDoc()->GetNextView(pos)->GetParentFrame());
        }

        RefreshExternalLogicAndReportNodes();
    }

    else if( engine_app_type== EngineAppType::Batch )
    {
        COrderTreeCtrl& orderTree = dlgBar.m_OrderTree;
        if(!m_application->GetFormFilenames().empty()) {
            FormOrderAppTreeNode* form_order_app_tree_node = orderTree.GetFormOrderAppTreeNode(m_application->GetFormFilenames().front());
            ASSERT(form_order_app_tree_node != nullptr &&
                   form_order_app_tree_node->GetHItem() != nullptr &&
                   form_order_app_tree_node->GetOrderDocument() != nullptr);

            POSITION pos = form_order_app_tree_node->GetOrderDocument()->GetFirstViewPosition();
            COrderChildWnd* pOrderChildWnd = assert_cast<COrderChildWnd*>(form_order_app_tree_node->GetOrderDocument()->GetNextView(pos)->GetParentFrame());
            application_child_wnd = pOrderChildWnd;

            orderTree.Select(form_order_app_tree_node->GetHItem(), TVGN_CARET);

            if(pOrderChildWnd->GetApplicationName().CompareNoCase(this->GetPathName()) != 0) {
                //Update the current item's code to ensure that the code doesnt get messed up
                pOrderChildWnd->SetApplicationName(this->GetPathName());
                AfxGetMainWnd()->SendMessage(UWM::Order::ShowSourceCode, 0, reinterpret_cast<LPARAM>(form_order_app_tree_node->GetOrderDocument()));
            }
        }

        RefreshExternalLogicAndReportNodes();
    }

    else if( engine_app_type== EngineAppType::Tabulation )
    {
        CTabTreeCtrl& tabTree = dlgBar.m_TableTree;
        CTabulateDoc* pTabDoc = nullptr;
        if(!m_application->GetTabSpecFilenames().empty()) {
            TableSpecTabTreeNode* table_spec_tab_tree_node = tabTree.GetTableSpecTabTreeNode(m_application->GetTabSpecFilenames().front());
            ASSERT(table_spec_tab_tree_node != nullptr);
            pTabDoc = table_spec_tab_tree_node->GetTabDoc();
        }
        ASSERT(pTabDoc);

        POSITION pos = pTabDoc->GetFirstViewPosition();
        CTabView* pView = assert_cast<CTabView*>(pTabDoc->GetNextView(pos));
        application_child_wnd = assert_cast<ApplicationChildWnd*>(pView->GetParentFrame());

        CString sErr;
        AfxGetMainWnd()->SendMessage(WM_IMSA_UPDATE_SYMBOLTBL,(WPARAM)pTabDoc,0);
        if (pTabDoc->Reconcile(sErr)) {
            // pTabDoc->SetModifiedFlag(TRUE); // 20100317 things shouldn't be set as modified if the user doesn't modify it
        }
        pTabDoc->DisplayFmtErrorMsg();

        if(pTabDoc->GetTableSpec()->GetNumTables() > 0) {
            pView->GetGrid()->SetTable(pTabDoc->GetTableSpec()->GetTable(0));
            pView->GetGrid()->Update();
        }
        //Set the tree control
        tabTree.ReBuildTree();
    }

    // refresh the message control's lexer language (based on the logic settings in this application)
    if( application_child_wnd != nullptr && application_child_wnd->GetLogicDialogBar().GetMessageEditCtrl() != nullptr )
    {
        application_child_wnd->GetLogicDialogBar().GetMessageEditCtrl()->PostMessage(UWM::Edit::RefreshLexer);
    }

    else
    {
        ASSERT80(false);
    }

    return TRUE;
}


void CAplDoc::RefreshExternalLogicAndReportNodes()
{
    if( m_application->GetFormFilenames().empty() )
        return;

    CMDlgBar& dlgBar = assert_cast<CMainFrame*>(AfxGetMainWnd())->GetDlgBar();

    if( GetEngineAppType() == EngineAppType::Entry )
    {
        CFormTreeCtrl& formTree = dlgBar.m_FormTree;
        CFormNodeID* pNode = formTree.GetFormNode(m_application->GetFormFilenames().front());
        formTree.InsertExternalCodeAndReportNodes(pNode);
    }

    else if( GetEngineAppType() == EngineAppType::Batch )
    {
        COrderTreeCtrl& orderTree = dlgBar.m_OrderTree;
        FormOrderAppTreeNode* form_order_app_tree_node = orderTree.GetFormOrderAppTreeNode(m_application->GetFormFilenames().front());
        orderTree.InsertExternalCodeAndReportNodes(form_order_app_tree_node);
    }
}


/////////////////////////////////////////////////////////////////////////////////
//
//  BOOL CAplDoc::ProcessEDictsOpen()
//
/////////////////////////////////////////////////////////////////////////////////
BOOL CAplDoc::ProcessEDictsOpen()
{
    BOOL bRet = TRUE;

    //Open External Dictionaries if there are any
    if( !m_application->GetExternalDictionaryFilenames().empty() ) {
        CMainFrame* pFrame = assert_cast<CMainFrame*>(AfxGetMainWnd());

        CDDTreeCtrl& dictTree = pFrame->GetDlgBar().m_DictTree;

        for( int iIndex = 0; iIndex < (int)m_application->GetExternalDictionaryFilenames().size(); iIndex++ ) {
            CString sDictFile = m_application->GetExternalDictionaryFilenames()[iIndex];
            DictionaryDictTreeNode* dictionary_dict_tree_node = dictTree.GetDictionaryTreeNode(sDictFile);

            if( dictionary_dict_tree_node != nullptr && dictionary_dict_tree_node->GetDDDoc() == nullptr )
            {
                if( !dictTree.OpenDictionary(sDictFile, FALSE) )
                {
                    CString sMsg;
                    sMsg.FormatMessage(_T("Failed to open external dictionary %1. It will be removed from the application."), sDictFile.GetString());
                    AfxMessageBox(sMsg);

                    dictTree.ReleaseDictionaryNode(*dictionary_dict_tree_node);

                    m_application->DropExternalDictionaryFilename(sDictFile);
                    iIndex--;

                    bRet = FALSE;
                }
            }
        }
    }

    return bRet;
}


bool CAplDoc::IsAppModified()
{
    if( IsModified() )
        return true;

    // external code files
    for( const CodeFile& code_file : m_application->GetCodeFiles() )
    {
        if( !code_file.IsLogicMain() && code_file.GetTextSource().RequiresSave() )
            return true;
    }

    // message files
    for( const auto& message_text_source : m_application->GetMessageTextSources() )
    {
        if( message_text_source->RequiresSave() )
            return true;
    }

    // reports
    for( const auto& report_named_text_sources : m_application->GetReportNamedTextSources() )
    {
        if( report_named_text_sources->text_source->RequiresSave() )
            return true;
    }

    CCSProApp* pApp = assert_cast<CCSProApp*>(AfxGetApp());

    // external dictionaries
    for( const auto& dictionary_filename : m_application->GetExternalDictionaryFilenames() )
    {
        CDocument* pDoc = pApp->GetDoc(dictionary_filename);

        if( pDoc != nullptr && pDoc->IsModified() )
            return true;
    }

    //Check if the forms are modified
    if(GetEngineAppType() == EngineAppType::Entry) {
        if(m_application->GetAppSrcCode()->IsModified())
            return true;
        if(m_application->GetUseQuestionText() && m_pQuestMgr != nullptr && m_pQuestMgr->IsModified())
            return TRUE;

        for( const auto& sFormFileName : m_application->GetFormFilenames() ) {
            CDocument* pDoc = pApp->GetDoc(sFormFileName);
            if(!pDoc) {
                continue;
            }
            if(pDoc->IsModified()) {
                return true;
            }
            else {

                //check the forms dictionaries
                ASSERT(pDoc->IsKindOf(RUNTIME_CLASS(CFormDoc)));
                CDEFormFile* pFormFile = &assert_cast<CFormDoc*>(pDoc)->GetFormFile();

                CDocument* pDictDoc = pApp->GetDoc(pFormFile->GetDictionaryFilename());
                if(pDictDoc->IsModified())
                    return true;
            }
            if(m_application->GetUseQuestionText() ) {
                CFormDoc* pFormDoc = assert_cast<CFormDoc*>(pDoc);
                CFormChildWnd* pFrame = (CFormChildWnd*)pFormDoc->GetView(FormViewType::Form)->GetParentFrame();
                if( pFrame ) {
                if(pFrame->GetQSFView1()) {
                    CQSFEView* pView1= pFrame->GetQSFView1();
                    if(pView1->IsDirty())
                        return true;
                }
                if(pFrame->GetQSFView2()) {
                    CQSFEView* pView2= pFrame->GetQSFView2();
                    if(pView2->IsDirty())
                        return true;
                }
                }
            }

        }
    }

    else if(GetEngineAppType() == EngineAppType::Batch) {
        if(m_application->GetAppSrcCode()->IsModified())
            return true;

        for( const auto& sOrderFileName : m_application->GetFormFilenames() ) {
            CDocument* pDoc = pApp->GetDoc(sOrderFileName);
            if(!pDoc) {
                continue;
            }
            if(pDoc->IsModified()) {
                return true;
            }
            else {
                //check the order dictionaries
                ASSERT(pDoc->IsKindOf(RUNTIME_CLASS(COrderDoc)));
                CDEFormFile* pOrderFile = &assert_cast<COrderDoc*>(pDoc)->GetFormFile();
                CDocument* pDictDoc = pApp->GetDoc(pOrderFile->GetDictionaryFilename());
                if(pDictDoc->IsModified())
                    return true;
            }
        }
    }

    else if(GetEngineAppType() == EngineAppType::Tabulation) {
        if(m_application->GetAppSrcCode()->IsModified())
            return true;

        for( const CString& tab_spec_filename : m_application->GetTabSpecFilenames() ) {
            CDocument* pDoc = pApp->GetDoc(tab_spec_filename);
            if(!pDoc) {
                continue;
            }
            if(pDoc->IsModified()) {
                return true;
            }
            else {
                //check the table dictionaries
                ASSERT(pDoc->IsKindOf(RUNTIME_CLASS(CTabulateDoc)));
                CTabSet* pTabSpec = ((CTabulateDoc*)pDoc)->GetTableSpec();
                int iNumDicts = 1; // HARDCODED For #1 dictionary in table file //SAVY &&&
                for(int iDict =0; iDict < iNumDicts ; iDict++) {
                    CDocument* pDictDoc = pApp->GetDoc(pTabSpec->GetDictFile());
                    if(pDictDoc->IsModified())
                        return true;
                }
            }
        }
    }

    return false;
}

//SAVY 05/18/00 Updated it for CSBatch
BOOL CAplDoc::Reconcile(CString& csErr, bool bSilent, bool bAutoFix)
{
    BOOL bRet = FALSE;
    if(GetEngineAppType() == EngineAppType::Entry) {
        CMainFrame* pFrame = assert_cast<CMainFrame*>(AfxGetMainWnd());
        CFormTreeCtrl& formTree = pFrame->GetDlgBar().m_FormTree;

        for( const CString& form_filename : m_application->GetFormFilenames() ) {
            //get at the form tree, get the docs, and release them
            CFormNodeID* pID = formTree.GetFormNode(form_filename);
            if(pID != nullptr && pID->GetFormDoc()) {
                CFormScrollView* pFView = (CFormScrollView*)pID->GetFormDoc()->GetView();
                if( pFView )
                    pFView->RemoveAllGrids();
                bRet = pID->GetFormDoc()->GetFormFile().Reconcile(csErr, bSilent, bAutoFix);
                if( pFView )
                    pFView->RecreateGrids(0);
                if (!bRet) {
                    pID->GetFormDoc()->SetModifiedFlag();
                    formTree.ReBuildTree();
                }
            }
        }
        ReconcileDictTypes();
    }

    else if(GetEngineAppType()==EngineAppType::Batch) {
        CMainFrame* pFrame = assert_cast<CMainFrame*>(AfxGetMainWnd());
        COrderTreeCtrl& orderTree = pFrame->GetDlgBar().m_OrderTree;

        for( const CString& form_filename : m_application->GetFormFilenames() ) {
            //get at the order tree, get the docs, and release them
            FormOrderAppTreeNode* form_order_app_tree_node = orderTree.GetFormOrderAppTreeNode(form_filename);
            if(form_order_app_tree_node != nullptr && form_order_app_tree_node->GetDocument() != nullptr) {
                orderTree.SetSndMsgFlg(FALSE);
                orderTree.SetRedraw(FALSE);

                bRet = form_order_app_tree_node->GetOrderDocument()->GetFormFile().OReconcile(csErr, bSilent, bAutoFix);

                if (!bRet) {
                    form_order_app_tree_node->GetDocument()->SetModifiedFlag();
                    orderTree.ReBuildTree(0, nullptr, false);
                    orderTree.SetRedraw(TRUE);
                }
                orderTree.SetRedraw(TRUE);
                orderTree.SetSndMsgFlg(TRUE);
            }
        }
        ReconcileDictTypes();
    }

    else if(GetEngineAppType()==EngineAppType::Tabulation) {
        CMainFrame* pFrame = assert_cast<CMainFrame*>(AfxGetMainWnd());
        CTabTreeCtrl& tabTree = pFrame->GetDlgBar().m_TableTree;

        for( const CString& tab_spec_filename : m_application->GetTabSpecFilenames() ) {
            //get at the tabsped tree, get the docs, and release them
            TableSpecTabTreeNode* table_spec_tab_tree_node = tabTree.GetTableSpecTabTreeNode(tab_spec_filename);
            if(table_spec_tab_tree_node != nullptr && table_spec_tab_tree_node->GetTabDoc()) {
                bRet =  table_spec_tab_tree_node->GetTabDoc()->Reconcile(csErr, bSilent, bAutoFix);
                if (bRet) {//if something has changed
                    table_spec_tab_tree_node->GetTabDoc()->SetModifiedFlag();
                    tabTree.ReBuildTree();
                }
            }
        }
    }

    else {
        bRet = TRUE;
    }

    return bRet;
}


bool CAplDoc::IsNameUnique(const CDocument* pDoc, const CString& name) const
{
    // check reports and code namespaces
    if( !m_application->IsNameUnique(CS2WS(name)) )
        return false;

    // check external dictionaries
    if( !ProcessEDicts(name) )
        return false;

    // check application-specific values
    if( pDoc->IsKindOf(RUNTIME_CLASS(CFormDoc)) )
    {
        if( !ProcessFormDicts(name) || !ProcessForms(name) )
            return false;
    }

    else if( pDoc->IsKindOf(RUNTIME_CLASS(COrderDoc)) )
    {
        if( !ProcessOrderDicts(name) || !ProcessOrders(name) )
            return false;
    }

    else if( pDoc->IsKindOf(RUNTIME_CLASS(CTabulateDoc)) )
    {
        const CTabulateDoc* pTabDoc = (const CTabulateDoc*)pDoc;

        // check the tabset name
        if( pTabDoc->GetTableSpec()->GetName().CompareNoCase(name) == 0 )
            return false;

        // check table dictionary name
        const CDataDict* pDict = pTabDoc->GetTableSpec()->GetDict();
        int iL, iR, iI, iVS;
        if( pDict != nullptr && pDict->LookupName(name, &iL, &iR, &iI, &iVS) )
            return false;

        // check the tables
        for( int i = 0; i < pTabDoc->GetTableSpec()->GetNumTables(); ++i )
        {
            const CTable* pTable = pTabDoc->GetTableSpec()->GetTable(i);
            if( pTable->GetName().CompareNoCase(name) == 0 )
                return false;
        }
    }

    return true;
}


BOOL CAplDoc::ProcessFormDicts(const CString& name) const
{
    CMainFrame* pFrame = assert_cast<CMainFrame*>(AfxGetMainWnd());
    CFormTreeCtrl& formTree = pFrame->GetDlgBar().m_FormTree;
    CDDTreeCtrl&   dictTree = pFrame->GetDlgBar().m_DictTree;

    for( const auto& sFormFileName : m_application->GetFormFilenames() ) {
        CFormNodeID* pID = formTree.GetFormNode(sFormFileName);

        if(pID != nullptr && pID->GetFormDoc()) {

            CDEFormFile* pFormFile = &pID->GetFormDoc()->GetFormFile();
            CString sDictName = pFormFile->GetDictionaryFilename();
            DictionaryDictTreeNode* dictionary_dict_tree_node =  dictTree.GetDictionaryTreeNode(sDictName);
            if(dictionary_dict_tree_node != nullptr && dictionary_dict_tree_node->GetDDDoc() != nullptr) {
                int iL, iR, iI, iVS;
                if(dictionary_dict_tree_node->GetDDDoc()->GetDict()->LookupName(name, &iL, &iR, &iI, &iVS))
                    return FALSE;
            }
        }
    }

    return TRUE;
}



BOOL CAplDoc::ProcessEDicts(const CString& name) const
{
    CMainFrame* pFrame = assert_cast<CMainFrame*>(AfxGetMainWnd());
    CDDTreeCtrl& dictTree = pFrame->GetDlgBar().m_DictTree;
    //Save the input dictionaries
    for( const auto& sDictFName : m_application->GetExternalDictionaryFilenames() ) {
        //get at the dictionary tree and get the documents
            DictionaryDictTreeNode* dictionary_dict_tree_node = dictTree.GetDictionaryTreeNode(sDictFName);
            if(dictionary_dict_tree_node->GetDDDoc() != nullptr) {
                int iL, iR, iI, iVS;
                if(dictionary_dict_tree_node->GetDDDoc()->GetDict()->LookupName(name, &iL, &iR, &iI, &iVS))
                    return FALSE;
            }
    }

    return TRUE;
}


BOOL CAplDoc::ProcessForms(const CString& name) const
{
    CMainFrame* pFrame = assert_cast<CMainFrame*>(AfxGetMainWnd());
    CFormTreeCtrl& formTree = pFrame->GetDlgBar().m_FormTree;

    for( const CString& form_filename : m_application->GetFormFilenames() )
    {
        CFormNodeID* pID = formTree.GetFormNode(form_filename);

        if(pID->GetFormDoc()) {
            if(!pID->GetFormDoc()->GetFormFile().IsNameUnique(name))
                return FALSE;
        }
    }

    return TRUE;
}

//SAVY 05/18/00 No Update required for CSBatch
BOOL CAplDoc::ProcessOrders(const CString& name) const
{
    CMainFrame* pFrame = assert_cast<CMainFrame*>(AfxGetMainWnd());

    COrderTreeCtrl& OrderTree = pFrame->GetDlgBar().m_OrderTree;

    for( const CString& form_filename : m_application->GetFormFilenames() ) {

        FormOrderAppTreeNode* form_order_app_tree_node = OrderTree.GetFormOrderAppTreeNode(form_filename);

        if(form_order_app_tree_node->GetDocument() != nullptr) {
            if(!form_order_app_tree_node->GetOrderDocument()->GetFormFile().IsNameUnique(name))
                return FALSE;
        }
    }

    return TRUE;
}

//SAVY 05/18/00 No Update required for CSBatch
BOOL CAplDoc::ProcessOrderDicts(const CString& name) const
{
    CMainFrame* pFrame = assert_cast<CMainFrame*>(AfxGetMainWnd());

    COrderTreeCtrl& orderTree = pFrame->GetDlgBar().m_OrderTree;
    CDDTreeCtrl&   dictTree = pFrame->GetDlgBar().m_DictTree;

    for( const CString& sOrderFileName : m_application->GetFormFilenames() ) {
        FormOrderAppTreeNode* form_order_app_tree_node = orderTree.GetFormOrderAppTreeNode(sOrderFileName);

        if(form_order_app_tree_node != nullptr && form_order_app_tree_node->GetDocument() != nullptr) {

            CDEFormFile* pOrderFile = &form_order_app_tree_node->GetOrderDocument()->GetFormFile();
            CString sDictName = pOrderFile->GetDictionaryFilename();
            DictionaryDictTreeNode* dictionary_dict_tree_node =  dictTree.GetDictionaryTreeNode(sDictName);
            if(dictionary_dict_tree_node != nullptr && dictionary_dict_tree_node->GetDDDoc() != nullptr) {
                int iL, iR, iI, iVS;
                if(dictionary_dict_tree_node->GetDDDoc()->GetDict()->LookupName(name, &iL, &iR, &iI, &iVS))
                    return FALSE;
            }
        }
    }

    return TRUE;
}


void CAplDoc::SetEDictObjects()
{
    m_application->GetRuntimeExternalDictionaries().clear();

    //Get the form object from the memory
    CMainFrame* pFrame = assert_cast<CMainFrame*>(AfxGetMainWnd());
    CDDTreeCtrl& dictTree = pFrame->GetDlgBar().m_DictTree;

    for( const CString& sDictFName : m_application->GetExternalDictionaryFilenames() ) {
        DictionaryDictTreeNode* dictionary_dict_tree_node = dictTree.GetDictionaryTreeNode(sDictFName);
        if(dictionary_dict_tree_node != nullptr && dictionary_dict_tree_node->GetDDDoc() == nullptr){
            if(!dictTree.OpenDictionary(sDictFName))
                continue;
            CCSProApp* pApp = assert_cast<CCSProApp*>(AfxGetApp());
            pApp->UpdateViews(dictionary_dict_tree_node->GetDDDoc());
        }
        if(dictionary_dict_tree_node != nullptr) {
            m_application->AddRuntimeExternalDictionary(dictionary_dict_tree_node->GetDDDoc()->GetSharedDictionary());
        }
    }
}


BOOL CAplDoc::ProcessFormOpen()
{
   ASSERT(m_application->GetEngineAppType() == EngineAppType::Entry);
   CMDlgBar& dlgBar = assert_cast<CMainFrame*>(AfxGetMainWnd())->GetDlgBar();

    //Open Forms
    CFormTreeCtrl&  formTree = dlgBar.m_FormTree;
    if(m_application->GetFormFilenames().empty()) {
        CString sMsg;
        sMsg.FormatMessage(_T("No Form File Associated with this Application"));
        AfxMessageBox(sMsg);
        return FALSE;
    }

    for(int iIndex=0 ;iIndex < (int)m_application->GetFormFilenames().size(); iIndex ++) {
        CString sFormFile = m_application->GetFormFilenames()[iIndex];
        CFormNodeID* pNode = formTree.GetFormNode(sFormFile);

        if(!pNode->GetFormDoc()) {
            if(!formTree.OpenFormFile(sFormFile,TRUE))
                return FALSE;
            else {
                CDEFormFile* pFile = &pNode->GetFormDoc()->GetFormFile();
                ASSERT(pFile);
                if(pFile->GetDictionaryFilename().IsEmpty()) {
                    CString sMsg;
                    sMsg.FormatMessage(_T("%1 has no associated dictionaries"), pNode->GetFormDoc()->GetPathName().GetString());
                    AfxMessageBox(sMsg);
                    return FALSE;
                }
                CDDTreeCtrl& dictTree = dlgBar.m_DictTree;

                CString sDictFile = pFile->GetDictionaryFilename();
                DictionaryDictTreeNode* dictionary_dict_tree_node = dictTree.GetDictionaryTreeNode(sDictFile);

                CString sMsg;
                sMsg.FormatMessage(_T("Failed to open %1"), sDictFile.GetString());
                if(dictionary_dict_tree_node == nullptr){
                    AfxMessageBox(sMsg);
                    return FALSE;
                }

                if(dictionary_dict_tree_node->GetDDDoc() == nullptr) {
                    if(!dictTree.OpenDictionary(sDictFile,FALSE)) {
                        AfxMessageBox(sMsg);
                        return FALSE;
                    }
                }
            }
        }

        ASSERT(pNode->GetFormDoc());

        pNode->GetFormDoc()->SetCapiQuestionManager(m_application.get(), m_pQuestMgr);

        CFormChildWnd* pFormChildWnd = (CFormChildWnd*)pNode->GetFormDoc()->GetView()->GetParentFrame();
        ASSERT(pFormChildWnd);
        if( pFormChildWnd )
            pFormChildWnd->SetApplicationName(this->GetPathName());

        QSFView* pQTView = (QSFView*)pNode->GetFormDoc()->GetView(FormViewType::QuestionText);
        if (pQTView) {
            pQTView->SetStyleCss(m_pQuestMgr->GetStylesCss());
            pQTView->SetupFileServer(m_application->GetApplicationFilename());
        }
    }

    return TRUE;
}

//SAVY 05/18/00 No Update required for CSBatch
BOOL CAplDoc::ProcessOrderOpen()
{
   ASSERT(m_application->GetEngineAppType() == EngineAppType::Batch);
   CMDlgBar& dlgBar = assert_cast<CMainFrame*>(AfxGetMainWnd())->GetDlgBar();

    //Open Forms
    COrderTreeCtrl& orderTree = dlgBar.m_OrderTree;
    if(m_application->GetFormFilenames().empty()) {
        CString sMsg;
        sMsg.FormatMessage(_T("No Order File Associated with this Application"));
        AfxMessageBox(sMsg);
        return FALSE;
    }

    for(int iIndex=0 ;iIndex < (int)m_application->GetFormFilenames().size(); iIndex ++) {
        CString sOrderFile = m_application->GetFormFilenames()[iIndex];
        FormOrderAppTreeNode* form_order_app_tree_node = orderTree.GetFormOrderAppTreeNode(sOrderFile);

        if(form_order_app_tree_node->GetDocument() == nullptr) {
            if(!orderTree.OpenOrderFile(sOrderFile))
                return FALSE;
            else {
                CDEFormFile* pFile = &form_order_app_tree_node->GetOrderDocument()->GetFormFile();
                ASSERT(pFile);
                if(pFile->GetDictionaryFilename().IsEmpty()) {
                    CString sMsg;
                    sMsg.FormatMessage(_T("%1 has no associated dictionaries"), form_order_app_tree_node->GetOrderDocument()->GetPathName().GetString());
                    AfxMessageBox(sMsg);
                    return FALSE;
                }
                CDDTreeCtrl&    dictTree = dlgBar.m_DictTree;

                CString sDictFile = pFile->GetDictionaryFilename();
                DictionaryDictTreeNode* dictionary_dict_tree_node = dictTree.GetDictionaryTreeNode(sDictFile);

                CString sMsg;
                sMsg.FormatMessage(_T("Failed to open %1"), sDictFile.GetString());
                if(dictionary_dict_tree_node == nullptr){
                    AfxMessageBox(sMsg);
                    return FALSE;
                }

                if(dictionary_dict_tree_node->GetDDDoc() == nullptr) {
                    if(!dictTree.OpenDictionary(sDictFile)) {
                        AfxMessageBox(sMsg);
                        return FALSE;
                    }
                }
            }
        }

        ASSERT(form_order_app_tree_node->GetOrderDocument() != nullptr);
       // orderTree.Select(pNode->GetHItem(),TVGN_CARET);
        POSITION pos = form_order_app_tree_node->GetOrderDocument()->GetFirstViewPosition();
        COrderChildWnd* pOrderChildWnd = (COrderChildWnd*)form_order_app_tree_node->GetOrderDocument()->GetNextView(pos)->GetParentFrame();
        ASSERT(pOrderChildWnd);
        if(pOrderChildWnd->GetApplicationName().IsEmpty()) {
            pOrderChildWnd->SetApplicationName(this->GetPathName());
        }
    }

    return TRUE;
}


BOOL CAplDoc::ProcessTabOpen()
{
    //Open Tables
    ASSERT(m_application->GetEngineAppType() == EngineAppType::Tabulation);
    CMDlgBar& dlgBar = assert_cast<CMainFrame*>(AfxGetMainWnd())->GetDlgBar();
    CTabTreeCtrl&   tabTree = dlgBar.m_TableTree;
    if(m_application->GetTabSpecFilenames().empty()) {
        CString sMsg;
        sMsg.FormatMessage(_T("No Tab Spec File Associated with this Application"));
        AfxMessageBox(sMsg);
        return FALSE;
    }
    if(!ProcessEDictsOpen()){
        return FALSE;
    }
    //Get the working dictionary
    std::shared_ptr<const CDataDict> pWorkDict;
    CDDTreeCtrl& dictTree = dlgBar.m_DictTree;

    for( const DictionaryDescription& dictionary_description : m_application->GetDictionaryDescriptions() )
    {
        if( dictionary_description.GetDictionaryType() != DictionaryType::Working )
            continue;

        DictionaryDictTreeNode* dictionary_dict_tree_node = dictTree.GetDictionaryTreeNode(dictionary_description.GetDictionaryFilename());
        ASSERT(dictionary_dict_tree_node != nullptr && dictionary_dict_tree_node->GetDDDoc() != nullptr);

        pWorkDict = dictionary_dict_tree_node->GetDDDoc()->GetSharedDictionary();
        break;
    }

    for( const CString& tab_spec_filename : m_application->GetTabSpecFilenames() ) {
        TableSpecTabTreeNode* table_spec_tab_tree_node = tabTree.GetTableSpecTabTreeNode(tab_spec_filename);
        CTabulateDoc* pTabDoc = nullptr;
        if(!table_spec_tab_tree_node->GetTabDoc()) {
            if(!tabTree.OpenTableFile(tab_spec_filename,pWorkDict)){
                return FALSE;
            }
            else {
                pTabDoc = table_spec_tab_tree_node->GetTabDoc();
                CTabSet* pFile = table_spec_tab_tree_node->GetTabDoc()->GetTableSpec();
                CDDTreeCtrl& dictTreeBar = dlgBar.m_DictTree;
                CString sDictFile = pFile->GetDictFile();
                DictionaryDictTreeNode* dictionary_dict_tree_node = dictTreeBar.GetDictionaryTreeNode(sDictFile);
                //SAVY &&& Xtabspec has no support for multiple dicts now .
                //add it later
                CString sMsg;
                sMsg.FormatMessage(_T("Failed to open %1"), sDictFile.GetString());
                if(dictionary_dict_tree_node == nullptr){
                    AfxMessageBox(sMsg);
                    return FALSE;
                }
                if(dictionary_dict_tree_node->GetDDDoc() == nullptr) {
                    if(!dictTreeBar.OpenDictionary(sDictFile,FALSE)) {
                        AfxMessageBox(sMsg);
                        return FALSE;
                    }
                }
            }
        }
        /*if(pTabDoc){
            POSITION pos = pTabDoc->GetFirstViewPosition();
            CTabView* pView = (CTabView*)pTabDoc->GetNextView(pos);
            pView->GetGrid()->RedrawWindow();
        }*/
    }
    return TRUE;
}


//SAVY&& Do we need to do this for CSBatch // Address this issue
//Use this function after stuffing the runtime objects
BOOL CAplDoc::Reconcile(BOOL bSilent)
{
    if(!CheckUniqueNames(bSilent))
        return FALSE;
    return TRUE;
}
//This does only the top level names for now
BOOL CAplDoc::CheckUniqueNames(BOOL bSilent)
{
    CString sMsg;
    BOOL bRet = TRUE;
    //Check for name clash among application , forms && dictionary names
    CMapStringToString arrMap; //UNMAME --->PATH
    //arrMap.SetAt(m_application->GetAppName() , ""); Stop Checking for the application name

    for(int iIndex = 0; iIndex < (int)m_application->GetRuntimeFormFiles().size(); iIndex++) {
        auto pFormFile = m_application->GetRuntimeFormFiles()[iIndex];
        ASSERT(pFormFile);
        CString sPath;
        CString sName = pFormFile->GetName();
        sName.MakeUpper();
        if(arrMap.Lookup(sName,sPath)) {
            if(sPath.IsEmpty())
                sPath = _T("Application");
            sMsg += _T("Unique Name - ") + sName + _T(" in ") + sPath + _T(" clashes with the name in ") + m_application->GetFormFilenames()[iIndex];
            sMsg += _T("\n");
            bRet = FALSE;

        }
        else {
            arrMap.SetAt(sName, m_application->GetFormFilenames()[iIndex]);
        }
        //Do for each dictionary of the form
        auto pDict = pFormFile->GetDictionary();
        ASSERT(pDict);
        sName = pDict->GetName();
        sName.MakeUpper();
        if(arrMap.Lookup(sName,sPath)) {
            if(sPath.IsEmpty())
                sPath = _T("Application");
            sMsg += _T("Unique Name - ") + sName + _T(" in ") + sPath + _T(" clashes with the name in ") + pFormFile->GetDictionaryName();
            sMsg += _T("\n");
            bRet = FALSE;

        }
        else {
            arrMap.SetAt(sName,pFormFile->GetDictionaryFilename());
        }
    }

    //Do for the external dictionaries
    for(int iEDict =0; iEDict < (int)m_application->GetRuntimeExternalDictionaries().size(); iEDict++) {
        auto pDict = m_application->GetRuntimeExternalDictionaries()[iEDict];
        ASSERT(pDict);
        CString sPath;
        CString sName = pDict->GetName();
        sName.MakeUpper();
        if(arrMap.Lookup(sName,sPath)) {
            if(sPath.IsEmpty())
                sPath = _T("Application");
            sMsg += _T("Unique Name - ") +  sName + _T(" in ") + sPath + _T(" clashes with the name in ") + m_application->GetExternalDictionaryFilenames()[iEDict];
            sMsg += _T("\n");
            bRet = FALSE;

        }
        else {
            arrMap.SetAt(sName,m_application->GetExternalDictionaryFilenames()[iEDict]);
        }
    }

    if(!bSilent && !bRet) {
        AfxMessageBox(sMsg);
    }

    return bRet;
}

//Call this function after SetObjects other wise you will get empty string array
std::vector<CString> CAplDoc::GetOrder() const
{
    std::vector<CString> proc_names = { _T("GLOBAL") };

    for( const auto& form_file : m_application->GetRuntimeFormFiles() )
        VectorHelpers::Append(proc_names, form_file->GetOrder());

    return proc_names;
}

//////////////////////////////////////////////////////////////////////
//
//  void CAplDoc::void ReconcileDictTypes(){
//
/////////////////////////////////////////////////////////////////////
void CAplDoc::ReconcileDictTypes()
{
    Application& application = GetAppObject();
    this->SetAppObjects(); //Set All the Application objects

    //Fill in Dicts to process
    CArray<CDataDict*,CDataDict*>arrDicts;

    //Remove all the DictionaryDescription which are no longer valid
    auto& dictionary_descriptions = application.GetDictionaryDescriptions();

    for( size_t i = dictionary_descriptions.size() - 1; i < dictionary_descriptions.size(); --i )
    {
        const DictionaryDescription& dictionary_description = dictionary_descriptions[i];

        if( !FindDictName(dictionary_description.GetDictionaryFilename(), dictionary_description.GetParentFilename()) )
            dictionary_descriptions.erase(dictionary_descriptions.begin() + i);
    }

    //Add the DictionaryDescription for objects which do not exist
    //Make sure that the first formfile dictionary type is input
    //Make sure that no other dictype is of input type
    CMainFrame* pFrame = assert_cast<CMainFrame*>(AfxGetMainWnd());
    if(application.GetEngineAppType() == EngineAppType::Entry){
        CFormTreeCtrl& formTree = pFrame->GetDlgBar().m_FormTree;
        for(int iIndex=0; iIndex < (int)application.GetFormFilenames().size(); iIndex++){
            //for each form
            CString sFormFName = application.GetFormFilenames()[iIndex];
            CFormNodeID* pFormNode = formTree.GetFormNode(sFormFName);
            if (pFormNode == nullptr)
                continue;
            CFormDoc* pFDoc = pFormNode->GetFormDoc();
            ASSERT(pFDoc);
            CDEFormFile* pFormFile = &pFDoc->GetFormFile();
            ASSERT(pFormFile);

            CString sDictFName = pFormFile->GetDictionaryFilename();
            DictionaryDescription* dictionary_description = application.GetDictionaryDescription(sDictFName, sFormFName);

            if( dictionary_description == nullptr) {
                dictionary_description = application.AddDictionaryDescription(
                    DictionaryDescription(CS2WS(sDictFName), CS2WS(sFormFName), ( iIndex == 0 ) ? DictionaryType::Input : DictionaryType::External));
            }
            else {
                if(iIndex != 0 && dictionary_description->GetDictionaryType() == DictionaryType::Input ){
                    //Change the dict type to external'cos there can be only one dict with external
                    ASSERT(FALSE); //Test to see when it happens
                    dictionary_description->SetDictionaryType(DictionaryType::External);
                }
            }

            dictionary_description->SetDictionary(pFormFile->GetDictionary());
        }
    }
    else if(application.GetEngineAppType() == EngineAppType::Batch){
        COrderTreeCtrl& orderTree = pFrame->GetDlgBar().m_OrderTree;
        for(int iIndex=0; iIndex < (int)application.GetFormFilenames().size(); iIndex++){
            //for each form
            CString sOrderFName = application.GetFormFilenames()[iIndex];
            FormOrderAppTreeNode* form_order_app_tree_node = orderTree.GetFormOrderAppTreeNode(sOrderFName);
            ASSERT(form_order_app_tree_node != nullptr);
            COrderDoc* pFDoc = form_order_app_tree_node->GetOrderDocument();
            ASSERT(pFDoc);
            CDEFormFile* pFormFile = &pFDoc->GetFormFile();

            CString sDictFName = pFormFile->GetDictionaryFilename();
            DictionaryDescription* dictionary_description = application.GetDictionaryDescription(sDictFName, sOrderFName);

            if( dictionary_description == nullptr ) {
                dictionary_description = application.AddDictionaryDescription(
                    DictionaryDescription(CS2WS(sDictFName), CS2WS(sOrderFName), ( iIndex == 0 ) ? DictionaryType::Input : DictionaryType::External));
            }
            else {
                if(iIndex != 0 && dictionary_description->GetDictionaryType() == DictionaryType::Input ){
                    //Change the dict type to external'cos there can be only one dict with external
                    ASSERT(FALSE); //Test to see when it happens
                    dictionary_description->SetDictionaryType(DictionaryType::External);
                }
            }

            dictionary_description->SetDictionary(pFormFile->GetDictionary());
        }
    }

    //Look in edicts
    for(int iIndex =0 ;iIndex < (int)application.GetExternalDictionaryFilenames().size(); iIndex++){
        CString sDictFName = application.GetExternalDictionaryFilenames()[iIndex];
        DictionaryDescription* dictionary_description = application.GetDictionaryDescription(sDictFName);

        if( dictionary_description == nullptr ) {
            dictionary_description = application.AddDictionaryDescription(DictionaryDescription(CS2WS(sDictFName), DictionaryType::External));
        }
        else {
            if(iIndex < (int)application.GetRuntimeExternalDictionaries().size()){
                if( dictionary_description->GetDictionaryType() == DictionaryType::Input ){
                    //Change the dict type to external'cos there can be only one dict with external
                    ASSERT(FALSE); //Test to see when it happens
                    dictionary_description->SetDictionaryType(DictionaryType::External);
                }
            }
        }

        dictionary_description->SetDictionary(application.GetRuntimeExternalDictionaries()[iIndex].get());
    }
}

//////////////////////////////////////////////////////////////////////
//
//BOOL CAplDoc::FindDictName(const std::wstring& sDictName, const std::wstring& sFormName)
//  Assumes that the AppObjects are set
/////////////////////////////////////////////////////////////////////
bool CAplDoc::FindDictName(const std::wstring& sDictName, const std::wstring& sFormName)
{
    Application& application = this->GetAppObject();

    if(!sFormName.empty() && GetEngineAppType() == EngineAppType::Entry){
        //You cannot get the CDEFormFile name from the object so get the
        //CFormDoc  and look in it
        CMainFrame* pFrame = assert_cast<CMainFrame*>(AfxGetMainWnd());
        CFormTreeCtrl& formTree = pFrame->GetDlgBar().m_FormTree;
        CFormNodeID* pFormNode = formTree.GetFormNode(sFormName);
        if(!pFormNode || !pFormNode->GetFormDoc())
            return false;
        CFormDoc* pFormDoc = pFormNode->GetFormDoc();
        CDEFormFile* pFFSpec = &pFormDoc->GetFormFile();

        if(SO::EqualsNoCase(sDictName, pFFSpec->GetDictionaryFilename())){
            return true;
        }
    }
    else if(!sFormName.empty() && GetEngineAppType() == EngineAppType::Batch){
        //You cannot get the CDEFormFile name from the object so get the
        //CFormDoc  and look in it
        CMainFrame* pFrame = assert_cast<CMainFrame*>(AfxGetMainWnd());
        COrderTreeCtrl& formTree = pFrame->GetDlgBar().m_OrderTree;
        FormOrderAppTreeNode* form_order_app_tree_node = formTree.GetFormOrderAppTreeNode(sFormName);
        if(form_order_app_tree_node == nullptr || form_order_app_tree_node->GetOrderDocument() == nullptr )
            return false;
        COrderDoc* pOrderDoc = form_order_app_tree_node->GetOrderDocument();
        CDEFormFile* pFFSpec = &pOrderDoc->GetFormFile();

        if(SO::EqualsNoCase(sDictName, pFFSpec->GetDictionaryFilename())){
            return true;
        }
    }
    else {
        //look in edicts
        for( const CString& sDictFName : application.GetExternalDictionaryFilenames() ) {
            if(SO::EqualsNoCase(sDictName, sDictFName)){
                return true;
            }
        }
    }

    return false;
}


/////////////////////////////////////////////////////////////////////////////////
//
//  void CAplDoc::BuildQuestMgr()
//
/////////////////////////////////////////////////////////////////////////////////
void CAplDoc::BuildQuestMgr()
{
    m_pQuestMgr = std::make_shared<CapiQuestionManager>();

    //if qsf file does not exist then create one
    if( !PortableFunctions::FileIsRegular(m_application->GetQuestionTextFilename()) )
        m_pQuestMgr->Save(CS2WS(m_application->GetQuestionTextFilename()));

    m_pQuestMgr->Load(CS2WS(m_application->GetQuestionTextFilename()));
}

/////////////////////////////////////////////////////////////////////////////////
//
//  CString CAplDoc::GetCapiTextForFirstCondition(CDEField* pField)
//
/////////////////////////////////////////////////////////////////////////////////
CString CAplDoc::GetCapiTextForFirstCondition(CDEItemBase* pBase, wstring_view language_name/* = wstring_view()*/)
{
    ASSERT(m_pQuestMgr != nullptr);
    auto question = m_pQuestMgr->GetQuestion(GetCapiItemName(pBase));
    if (question && !question->GetConditions().empty()) {
        if (language_name.empty())
            language_name = m_pQuestMgr->GetDefaultLanguage().GetName();
        return question->GetConditions().front().GetQuestionText(language_name).GetText();
    }
    else {
        return CString();
    }
}

/////////////////////////////////////////////////////////////////////////////////
//
//  SetCapiTextForAllConditions
//
/////////////////////////////////////////////////////////////////////////////////
void CAplDoc::SetCapiTextForAllConditions(CDEItemBase* pBase, CString question_text, wstring_view language_name/* = wstring_view()*/)
{
    ASSERT(m_pQuestMgr != nullptr);

    CString item_name = GetCapiItemName(pBase);
    auto question = m_pQuestMgr->GetQuestion(item_name);
    if (!question) {
        question = CapiQuestion(item_name);
    }
    auto& conditions = question->GetConditions();

    if (conditions.empty()) {
        CapiCondition new_condition;
        if (language_name.empty()) {
            for (const Language& lang : m_pQuestMgr->GetLanguages()) {
                new_condition.SetQuestionText(question_text, lang.GetName());
            }
        }
        else {
            new_condition.SetQuestionText(question_text, language_name);
        }
        conditions.emplace_back(new_condition);
    }
    else {
        for (CapiCondition& cond : conditions) {
            if (language_name.empty()) {
                for (const Language& lang : m_pQuestMgr->GetLanguages()) {
                    cond.SetQuestionText(question_text, lang.GetName());
                }
            }
            else {
                cond.SetQuestionText(question_text, language_name);
            }
        }
    }

    m_pQuestMgr->SetQuestion(std::move(*question));
}



/////////////////////////////////////////////////////////////////////////////////
//
//  bool CAplDoc::IsQHAvailable(CDEItemBase* pBase)
//
/////////////////////////////////////////////////////////////////////////////////
bool CAplDoc::IsQHAvailable(const CDEItemBase* pBase)
{
    if(!m_application->GetUseQuestionText() || !m_pQuestMgr)
        return false;

    auto question = m_pQuestMgr->GetQuestion(GetCapiItemName(pBase));
    if (!question)
        return false;

    for (const CapiCondition& cond : question->GetConditions()) {

        for (const Language& lang : m_pQuestMgr->GetLanguages()) {
            if (!cond.GetQuestionText(lang.GetName()).GetText().IsEmpty() || !cond.GetHelpText(lang.GetName()).GetText().IsEmpty()) {
                return true;
            }
        }
    }

    return false;
}


bool CAplDoc::GetLangInfo(CArray<CLangInfo,CLangInfo&>& arrInfo)
{
    ASSERT(m_pQuestMgr != nullptr);
    arrInfo.RemoveAll();
    for (const auto& lang : m_pQuestMgr->GetLanguages()) {
        CLangInfo langInfo;
        langInfo.m_sLangName = WS2CS(lang.GetName());
        langInfo.m_sLabel = WS2CS(lang.GetLabel());
        arrInfo.Add(langInfo);
    }

    return true;
}


void CAplDoc::ProcessLangs(CArray<CLangInfo,CLangInfo&>& arrInfo)
{
    ASSERT( m_pQuestMgr != nullptr );

    int iNumLanguages = m_pQuestMgr->GetLanguages().size();

    //First process langs which are modified
    for(int iLangInfo=0; iLangInfo < arrInfo.GetSize(); iLangInfo++) {
        CLangInfo langInfo = arrInfo[iLangInfo];
        if (langInfo.m_eLangInfo == eLANGINFO::MODIFIED_INFO) {
            ASSERT(iNumLanguages > iLangInfo);
            CString sName = langInfo.m_sLangName;
            sName.Trim();
            const auto& current_language = m_pQuestMgr->GetLanguages()[iLangInfo];
            m_pQuestMgr->ModifyLanguage(current_language.GetName(), Language(CS2WS(sName), CS2WS(langInfo.m_sLabel)));
        }
    }

    //Second langs which are deleted
    for(int iLangInfo=0; iLangInfo < arrInfo.GetSize(); iLangInfo++) {
        CLangInfo langInfo =arrInfo[iLangInfo];
        if(langInfo.m_eLangInfo == eLANGINFO::DELETED_INFO) {
            m_pQuestMgr->DeleteLanguage(langInfo.m_sLangName);
        }
    }

    //Finally langs which are ADDED
    for(int iLangInfo=0; iLangInfo < arrInfo.GetSize(); iLangInfo++) {
        CLangInfo langInfo =arrInfo[iLangInfo];
        if(langInfo.m_eLangInfo == eLANGINFO::NEW_INFO) {
            CString sName = langInfo.m_sLangName;
            sName.Trim();
            m_pQuestMgr->AddLanguage(Language(CS2WS(sName), CS2WS(langInfo.m_sLabel)));
        }
    }
}


void CAplDoc::ChangeCapiName(const CDEItemBase* pItem, const CString& old_name) // 20120710 so that when changing names of items (and thus fields or blocks), we change the field name in the QSF file
{
    if( m_application->GetUseQuestionText() && m_pQuestMgr != nullptr )
    {
        auto question = m_pQuestMgr->GetQuestion(old_name);
        if (question) {
            m_pQuestMgr->RemoveQuestion(question->GetItemName());
            CString new_name = GetCapiItemName(pItem);
            question->SetItemName(new_name);
            m_pQuestMgr->SetQuestion(*question);
        }
    }
}


void CAplDoc::ChangeCapiDictName(const CDataDict& dictionary)
{
    if( m_application->GetUseQuestionText() && m_pQuestMgr != nullptr )
    {
        CString old_dict_name = dictionary.GetOldName();
        CString new_item_prefix = dictionary.GetName() + _T(".");

        std::wregex dict_item_regex(FormatText(_T("^%s\\."), old_dict_name.GetString()));

        std::vector<CapiQuestion> questions = m_pQuestMgr->GetQuestions();
        for (auto& question : questions) {
            m_pQuestMgr->RemoveQuestion(question.GetItemName());
            CString new_item_name = std::regex_replace(question.GetItemName().GetString(), dict_item_regex, new_item_prefix.GetString()).c_str();
            question.SetItemName(new_item_name);
            m_pQuestMgr->SetQuestion(std::move(question));
        }
    }
}


namespace CodeMessage
{
    std::shared_ptr<TextSource> GetTextSourceWithDefaultText(Application& application, const std::optional<CString>& base_filename, AppFileType app_file_type)
    {
        std::wstring filename = PortableFunctions::PathAppendFileExtension(CS2WS(base_filename.value_or(application.GetApplicationFilename())), GetFileExtension(app_file_type));
        std::wstring default_text = application.GetLogicSettings().GetDefaultFirstLineForTextSource(application.GetLabel(), app_file_type);

        return TextSourceEditable::FindOpenOrCreate(std::move(filename), std::move(default_text));
    }
}


void CAplDoc::AddDefaultCodeFile(Application& application, std::optional<CString> base_filename/* = std::nullopt*/)
{
    application.AddCodeFile(CodeFile(CodeType::LogicMain, CodeMessage::GetTextSourceWithDefaultText(application, base_filename, AppFileType::Code)));
}


std::shared_ptr<TextSourceEditable> CAplDoc::GetLogicMainCodeFileTextSource()
{
    // if no main code file exists, create a default one
    CodeFile* logic_main_code_file = m_application->GetLogicMainCodeFile();

    if( logic_main_code_file == nullptr )
    {
        AddDefaultCodeFile(*m_application, std::nullopt);
        SetModifiedFlag(TRUE);

        logic_main_code_file = m_application->GetLogicMainCodeFile();
    }

    auto text_source = std::dynamic_pointer_cast<TextSourceEditable, TextSource>(logic_main_code_file->GetSharedTextSource());
    ASSERT(text_source != nullptr);

    return text_source;
}


void CAplDoc::AddDefaultMessageFile(Application& application, std::optional<CString> base_filename/* = std::nullopt*/)
{
    application.AddMessageFile(CodeMessage::GetTextSourceWithDefaultText(application, base_filename, AppFileType::Message));
}


std::shared_ptr<TextSourceEditable> CAplDoc::GetMessageTextSource()
{
    // if no message file exists, create a default one
    if( m_application->GetMessageTextSources().empty() )
    {
        AddDefaultMessageFile(*m_application, std::nullopt);
        SetModifiedFlag(TRUE);
    }

    auto text_source = std::dynamic_pointer_cast<TextSourceEditable, TextSource>(m_application->GetMessageTextSources().front());
    ASSERT(text_source != nullptr);

    return text_source;
}
