#include "StdAfx.h"
#include "AplDoc.h"
#include <zUtilO/ArrUtil.h>
#include <zUtilF/ProgressDlg.h>
#include <zCapiO/QSFView.h>
#include <Zentryo/Runaple.h>
#include <zDesignerF/NewFileCreator.h>
#include <regex>


/////////////////////////////////////////////////////////////////////////////
// CAplDoc

IMPLEMENT_DYNCREATE(CAplDoc, CDocument)

BEGIN_MESSAGE_MAP(CAplDoc, CDocument)
    ON_COMMAND(ID_FILE_CSPRO_CLOSE, OnFileClose)
END_MESSAGE_MAP()


CAplDoc::CAplDoc()
    :   m_application(std::make_unique<Application>())
{
    m_bSrcLoaded = false;
    m_bIsClosing = false;
    m_bIsClosing = false;
    m_deployWnd = nullptr;
}


CAplDoc::~CAplDoc()
{
    delete m_application->GetAppSrcCode();
}


void CAplDoc::ReplaceAppObject(std::unique_ptr<Application> application)
{
    ASSERT(application != nullptr);

    application->SetAppSrcCode(m_application->GetAppSrcCode());
    m_application->SetAppSrcCode(nullptr);

    m_application = std::move(application);
}


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
        const std::string& working_storage_dictionary_file_path = m_application->GetFirstDictionaryFilePathOfType(DictionaryType::Working);

        if( !PortableFunctions::FileIsRegular(working_storage_dictionary_file_path) )
        {
            AfxMessageBox(working_storage_dictionary_file_path.empty() ?
                L"Missing working storage dictionary.\n\nA new working storage dictionary will be added to the application." :
                L"This application's working storage dictionary is missing. It will be recreated.");

            try
            {
                NewFileCreator::CreateWorkingStorageDictionary(*m_application, true);
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
BuildAllTrees Adds the labels of the objects to the tree if it is opened standalone
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

    HTREEITEM hRet = ObjTree.InsertNode(TVI_ROOT, std::make_unique<ApplicationFileTreeNode>(m_application));

    //Insert label for the child items

    //Forms
    if(m_application->GetEngineAppType() == EngineAppType::Entry) {

        HTREEITEM hForm = hRet;

        for( const std::string& form_file_path : m_application->GetFormFilePaths() )
        {
            ObjTree.InsertFormNode(hForm, form_file_path, AppFileType::Form);
            formTree.AddFormFile(form_file_path, nullptr, true);
        }
    }

    //Orders
    else if(m_application->GetEngineAppType() == EngineAppType::Batch) {

        HTREEITEM hOrder = hRet;

        for( const std::string& form_file_path : m_application->GetFormFilePaths() )
        {
            CSpecFile specFormFile (true); //do it silently

            if(specFormFile.Open(UTF8_TODO::GetCString(form_file_path), CFile::modeRead)){

                ObjTree.InsertFormNode(hOrder, form_file_path, AppFileType::Order);

                FormOrderAppTreeNode* form_order_app_tree_node = orderTree.GetFormOrderAppTreeNode(form_file_path);

                if(form_order_app_tree_node != nullptr) {
                    form_order_app_tree_node->AddRef();
                }
                else {
                    HTREEITEM hItem = orderTree.InsertOrderFile(UTF8_TODO::GetCString(form_file_path), nullptr);
                    form_order_app_tree_node = orderTree.GetFormOrderAppTreeNode(form_file_path);

                    TVITEM pItem;
                    pItem.hItem = hItem;
                    pItem.mask = TVIF_CHILDREN;
                    pItem.cChildren = 1;
                    orderTree.SetItem(&pItem);
                }

                ASSERT(form_order_app_tree_node != nullptr);
                orderTree.InsertOrderDependencies(*form_order_app_tree_node);
                specFormFile.Close();
            }
            else {
                CString sString;
                sString.FormatMessage(IDS_OPENAPPFLD, UTF8_TODO::GetWide(form_file_path).c_str());
                AfxMessageBox(sString);
                continue;
            }
        }
    }


    // Input TabSpecs
    else if(m_application->GetEngineAppType() == EngineAppType::Tabulation) {

        HTREEITEM hSpec = hRet;

        for( const std::string& table_spec_file_path : m_application->GetTableSpecFilePaths() )
        {
            CSpecFile specTabFile(TRUE); //do it silently
            if(specTabFile.Open(UTF8_TODO::GetCString(table_spec_file_path), CFile::modeRead)){

                ObjTree.InsertTableNode(hSpec, table_spec_file_path);

                TableSpecTabTreeNode* table_spec_tab_tree_node = tabTree.GetTableSpecTabTreeNode(table_spec_file_path);

                if( table_spec_tab_tree_node != nullptr ) {
                    table_spec_tab_tree_node->AddRef();
                }
                else {
                    HTREEITEM hItem = tabTree.InsertTableSpec(UTF8_TODO::GetCString(table_spec_file_path), nullptr);
                    table_spec_tab_tree_node = tabTree.GetTableSpecTabTreeNode(table_spec_file_path);

                    TVITEM pItem;
                    pItem.hItem = hItem;
                    pItem.mask = TVIF_CHILDREN;
                    pItem.cChildren = 1;
                    tabTree.SetItem(&pItem);
                }

                ASSERT(table_spec_tab_tree_node != nullptr);
                tabTree.InsertTableDependencies(*table_spec_tab_tree_node);

                specTabFile.Close();
            }
            else {
                CString sString;
                sString.FormatMessage(IDS_OPENAPPFLD, UTF8_TODO::GetWide(table_spec_file_path).c_str());
                AfxMessageBox(sString);
                continue;
            }
        }
    }

    // Insert External Dictionaries
    HTREEITEM hExternal = hRet;

    for( const std::string& dictionary_file_path : m_application->GetExternalDictionaryFilePaths() )
    {
        dictTree.AddDictionary(dictionary_file_path, nullptr);
        ObjTree.InsertNode(hExternal, std::make_unique<DictionaryFileTreeNode>(dictionary_file_path));
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

        for( const AppMessageFile& app_message_file : m_application->GetMessageFiles() )
        {
            const HTREEITEM hItem = ObjTree.InsertNode(hMessageParentItem, std::make_unique<MessageFileTreeNode>(app_message_file.GetFilePath(), external_messages));

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
        std::optional<std::string> modified_qsf_file_path;

        if( m_application->GetQuestionTextFilePath().empty() )
        {
            modified_qsf_file_path = PortableFunctions::PathAppendFileExtension(m_application->GetApplicationFilePath(), FileExtensions::QuestionText);
        }

        else if( !SO::EqualsNoCase(PortableFunctions::PathGetFileExtension(m_application->GetQuestionTextFilePath()), FileExtensions::QuestionText) )
        {
            modified_qsf_file_path = PortableFunctions::PathReplaceFileExtension(m_application->GetQuestionTextFilePath(), FileExtensions::QuestionText);
        }

        if( modified_qsf_file_path.has_value() )
        {
            m_application->SetQuestionTextFilePath(std::move(*modified_qsf_file_path));
            SetModifiedFlag(true);
        }

        ObjTree.InsertNode(hRet, std::make_unique<QuestionTextFileTreeNode>(m_application->GetQuestionTextFilePath()));
    }


    // reports
    for( const ReportFile& report_file : m_application->GetReportFiles() )
        ObjTree.InsertNode(hRet, std::make_unique<ReportFileTreeNode>(report_file.GetFilePath()));


    // resources
    for( const AppResource& resource : m_application->GetResources() )
        ObjTree.InsertNode(hRet, std::make_unique<ResourceFileTreeNode>(resource.GetPath()));

    return hRet;
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

    CMainFrame* const pFrame = assert_cast<CMainFrame*>(AfxGetMainWnd());
    FileTreeNode* const file_tree_node = pFrame->GetDlgBar().m_ObjTree.FindNode(this);

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
        if(m_questionManager != nullptr && m_questionManager->IsModified()) {
            m_questionManager->Save(m_application->GetQuestionTextFilePath());
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

    bool main_message_file = true;

    for( AppMessageFile& app_message_file : m_application->GetMessageFilesIterator() )
    {
        // external message files currently aren't editable so only save the first one
        ASSERT(main_message_file || ( std::dynamic_pointer_cast<TextSourceExternal, TextSource>(app_message_file.GetSharedTextSource()) != nullptr ));

        if( main_message_file )
        {
            save_text_source(app_message_file.GetTextSource());
            main_message_file = false;
        }
    }

    for( ReportFile& report_file : m_application->GetReportFilesIterator() )
        save_text_source(report_file.GetTextSource());


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
    CMainFrame* const pFrame = assert_cast<CMainFrame*>(AfxGetMainWnd());
    CDDTreeCtrl& dictTree = pFrame->GetDlgBar().m_DictTree;
    // Examine external dictionaries
    for( const std::string& dictionary_file_path : m_application->GetExternalDictionaryFilePaths() ) {
        DictionaryDictTreeNode* const dictionary_dict_tree_node = dictTree.GetDictionaryTreeNode(dictionary_file_path);
        if (dictionary_dict_tree_node != nullptr && dictionary_dict_tree_node->GetDDDoc() != nullptr) {
            bOK &= dictionary_dict_tree_node->GetDDDoc()->GetDictionaryValidator()->IsValidSave(*dictionary_dict_tree_node->GetDDDoc()->GetDict());
        }
    }
    EngineAppType appType = m_application->GetEngineAppType();
    // Examine form dictionaries
    if(appType == EngineAppType::Entry) {
        CFormTreeCtrl& formTree = pFrame->GetDlgBar().m_FormTree;
        for( const std::string& form_file_path : m_application->GetFormFilePaths() ) {
            CFormNodeID* const pID = formTree.GetFormNode(form_file_path);
            if(pID != nullptr && pID->GetFormDoc()) {
                CDEFormFile* pFormFile = &pID->GetFormDoc()->GetFormFile();
                CString sDictName = pFormFile->GetDictionaryFilename();
                DictionaryDictTreeNode* const dictionary_dict_tree_node = dictTree.GetDictionaryTreeNode(UTF8_TODO::GetUtf8(sDictName));
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
    CMainFrame* const pFrame = assert_cast<CMainFrame*>(AfxGetMainWnd());
    CDDTreeCtrl& dictTree = pFrame->GetDlgBar().m_DictTree;
    //Save the input dictionaries
    for( const std::string& dictionary_file_path : m_application->GetExternalDictionaryFilePaths() ) {
        // get at the dictionary tree and get the documents
            DictionaryDictTreeNode* const dictionary_dict_tree_node = dictTree.GetDictionaryTreeNode(dictionary_file_path);
            if (dictionary_dict_tree_node != nullptr && dictionary_dict_tree_node->GetDDDoc() != nullptr) {
                if(dictionary_dict_tree_node->GetDDDoc()->IsModified()){
                    dictionary_dict_tree_node->GetDDDoc()->OnSaveDocument(UTF8_TODO::GetCString(dictionary_file_path));
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
    CMainFrame* const pFrame = assert_cast<CMainFrame*>(AfxGetMainWnd());
    CTabTreeCtrl& tableTree = pFrame->GetDlgBar().m_TableTree;
    //Save the input dictionaries
    for( const std::string& table_spec_file_path : m_application->GetTableSpecFilePaths() ) {
        // get at the dictionary tree and get the documents
        TableSpecTabTreeNode* const table_spec_tab_tree_node = tableTree.GetTableSpecTabTreeNode(table_spec_file_path);
        if(table_spec_tab_tree_node->GetTabDoc()){
            if(table_spec_tab_tree_node->GetTabDoc()->IsModified()){
                table_spec_tab_tree_node->GetTabDoc()->OnSaveDocument(TC::ToWide(table_spec_file_path).c_str());
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
    CMainFrame* const pFrame = assert_cast<CMainFrame*>(AfxGetMainWnd());

    CTabTreeCtrl& tableTree = pFrame->GetDlgBar().m_TableTree;
    CDDTreeCtrl& dictTree = pFrame->GetDlgBar().m_DictTree;
    // Save the table dicts

    for( const std::string& table_spec_file_path : m_application->GetTableSpecFilePaths() )
    {
        //Save the dictionaries of the tab specs files
        TableSpecTabTreeNode* const table_spec_tab_tree_node = tableTree.GetTableSpecTabTreeNode(table_spec_file_path);

        if(table_spec_tab_tree_node && table_spec_tab_tree_node->GetTabDoc()) {

            CTabSet* pTabSpec = table_spec_tab_tree_node->GetTabDoc()->GetTableSpec();
            int iNumDict = 1; // HARD CODED TO 1 dict in the spec
            // SAVY && change when this is implemented
            for(int iDict = 0; iDict<iNumDict; iDict++)
            {
                //To Do Get the dict path SAVY&&& 11/05/02
                CString sDictName = pTabSpec->GetDictFile();
                DictionaryDictTreeNode* const dictionary_dict_tree_node = dictTree.GetDictionaryTreeNode(UTF8_TODO::GetUtf8(sDictName));
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
    CMainFrame* const pFrame = assert_cast<CMainFrame*>(AfxGetMainWnd());
    CFormTreeCtrl& formTree = pFrame->GetDlgBar().m_FormTree;

    // Save the input dicts

    for( const std::string& form_file_path : m_application->GetFormFilePaths() )
    {
        // get at the dictionary tree and get the documents
        CFormNodeID* const pID = formTree.GetFormNode(form_file_path);

        if(pID != nullptr && pID->GetFormDoc()) {
            CFormDoc* pFormDoc = pID->GetFormDoc();
            if(pFormDoc->IsModified()) {
                pFormDoc->OnSaveDocument(TC::ToWide(form_file_path).c_str());
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
    CMainFrame* const pFrame = assert_cast<CMainFrame*>(AfxGetMainWnd());

    CFormTreeCtrl& formTree = pFrame->GetDlgBar().m_FormTree;
    CDDTreeCtrl& dictTree = pFrame->GetDlgBar().m_DictTree;

    // Save the Form dicts
    for( const std::string& form_file_path : m_application->GetFormFilePaths() )
    {
        //Save the dictionaries of the form files
        CFormNodeID* const pID = formTree.GetFormNode(form_file_path);

        if(pID != nullptr && pID->GetFormDoc()) {
            CDEFormFile* const pFormFile = &pID->GetFormDoc()->GetFormFile();
            CString sDictName = pFormFile->GetDictionaryFilename();
            DictionaryDictTreeNode* const dictionary_dict_tree_node = dictTree.GetDictionaryTreeNode(UTF8_TODO::GetUtf8(sDictName));
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
    CMainFrame* const pFrame = assert_cast<CMainFrame*>(AfxGetMainWnd());

    COrderTreeCtrl& OrderTree = pFrame->GetDlgBar().m_OrderTree;

    for( const std::string& form_file_path : m_application->GetFormFilePaths() )
    {
        // get at the dictionary tree and get the documents
        FormOrderAppTreeNode* const form_order_app_tree_node = OrderTree.GetFormOrderAppTreeNode(form_file_path);

        if(form_order_app_tree_node->GetDocument() != nullptr) {
            COrderDoc* pOrderDoc = form_order_app_tree_node->GetOrderDocument();
            if(pOrderDoc->IsModified()){
                pOrderDoc->OnSaveDocument(TC::ToWide(form_file_path).c_str());
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
    CMainFrame* const pFrame = assert_cast<CMainFrame*>(AfxGetMainWnd());

    COrderTreeCtrl& orderTree = pFrame->GetDlgBar().m_OrderTree;
    CDDTreeCtrl& dictTree = pFrame->GetDlgBar().m_DictTree;

    //Save the dictionaries of the order files
    for( const std::string& order_file_path : m_application->GetFormFilePaths() )
    {
        FormOrderAppTreeNode* const form_order_app_tree_node = orderTree.GetFormOrderAppTreeNode(order_file_path);

        if(form_order_app_tree_node != nullptr && form_order_app_tree_node->GetDocument() != nullptr) {
            CDEFormFile* const pOrderFile = &form_order_app_tree_node->GetOrderDocument()->GetFormFile();
            DictionaryDictTreeNode* const dictionary_dict_tree_node = dictTree.GetDictionaryTreeNode(UTF8_TODO::GetUtf8(pOrderFile->GetDictionaryFilename()));
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
    CMainFrame* const pFrame = assert_cast<CMainFrame*>(AfxGetMainWnd());

    CDDTreeCtrl& dictTree = pFrame->GetDlgBar().m_DictTree;

    for( const std::string& dictionary_file_path : m_application->GetExternalDictionaryFilePaths() )
    {
        //get at the form tree, get the docs, and release them
        DictionaryDictTreeNode* const dictionary_dict_tree_node = dictTree.GetDictionaryTreeNode(dictionary_file_path);

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

    CMainFrame* const pFrame = assert_cast<CMainFrame*>(AfxGetMainWnd());
    CFormTreeCtrl& formTree = pFrame->GetDlgBar().m_FormTree;
    formTree.SetSndMsgFlg(FALSE);
    for( const std::string& form_file_path : m_application->GetFormFilePaths() )
    {
        //get at the form tree, get the docs, and release them
        CFormNodeID* pID = formTree.GetFormNode(form_file_path);
        // ASSERT(AfxIsValidAddress( (void*)pID, sizeof(CFormNodeID)));
        if(!pID)
            continue;
        formTree.ReleaseFormDependencies(pID);
        formTree.ReleaseFormNodeID(pID);

        pID = formTree.GetFormNode(form_file_path);
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
    CMainFrame* const pFrame = assert_cast<CMainFrame*>(AfxGetMainWnd());

    COrderTreeCtrl& orderTree = pFrame->GetDlgBar().m_OrderTree;

    for( const std::string& form_file_path : m_application->GetFormFilePaths() )
    {
        //get at the form tree, get the docs, and release them

        FormOrderAppTreeNode* form_order_app_tree_node = orderTree.GetFormOrderAppTreeNode(form_file_path);
        if(form_order_app_tree_node == nullptr)
            continue;
        orderTree.ReleaseOrderDependencies(*form_order_app_tree_node);
        orderTree.ReleaseOrderNode(*form_order_app_tree_node);

        form_order_app_tree_node = orderTree.GetFormOrderAppTreeNode(form_file_path);

        if(form_order_app_tree_node != nullptr && form_order_app_tree_node->GetDocument() != nullptr) {
            POSITION pos = form_order_app_tree_node->GetOrderDocument()->GetFirstViewPosition();
            COrderChildWnd* const pOrderChildWnd = (COrderChildWnd*)form_order_app_tree_node->GetOrderDocument()->GetNextView(pos)->GetParentFrame();
            if(pOrderChildWnd) {
                AfxGetMainWnd()->PostMessage(UWM::Designer::ShowToolbar, (WPARAM)FrameType::Order);
            }
        }
    }
}

// ****************************************************************************

void CAplDoc::ReleaseTabSpecs()
{
    CMainFrame* const pFrame = assert_cast<CMainFrame*>(AfxGetMainWnd());
    CTabTreeCtrl& tableTree = pFrame->GetDlgBar().m_TableTree;

    for( const std::string& table_spec_file_path : m_application->GetTableSpecFilePaths() )
    {
        //get at the table  tree and get the documents and release the docs
        TableSpecTabTreeNode* const table_spec_tab_tree_node = tableTree.GetTableSpecTabTreeNode(table_spec_file_path);
        if(table_spec_tab_tree_node != nullptr) {
            tableTree.ReleaseTableDependencies(*table_spec_tab_tree_node);
            tableTree.ReleaseTableNode(*table_spec_tab_tree_node);
        }
    }
}


std::vector<std::tuple<std::string, std::shared_ptr<CDataDict>>> CAplDoc::GetAllDictionaries()
{
    Application& app = GetAppObject();
    CMainFrame* const pFrame = assert_cast<CMainFrame*>(AfxGetMainWnd());
    CStringArray aDictFNames;

    switch( GetEngineAppType() )
    {
        case EngineAppType::Entry:
        {
            CFormTreeCtrl& formTree = pFrame->GetDlgBar().m_FormTree;

            for( const std::string& form_file_path : app.GetFormFilePaths() )
            {
                CFormNodeID* const pFormNode = formTree.GetFormNode(form_file_path);
                ASSERT(pFormNode);
                CFormDoc* const pFormDoc = pFormNode->GetFormDoc();
                ASSERT_VALID(pFormDoc);
                CDEFormFile* const pFormSpec = &pFormDoc->GetFormFile();
                aDictFNames.Add(pFormSpec->GetDictionaryFilename());
            }

            break;
        }

        case EngineAppType::Batch:
        {
            COrderTreeCtrl& orderTree = pFrame->GetDlgBar().m_OrderTree;

            for( const std::string& form_file_path : m_application->GetFormFilePaths() )
            {
                // get dictionary children
                const FormOrderAppTreeNode* const form_order_app_tree_node = orderTree.GetFormOrderAppTreeNode(form_file_path);
                ASSERT(form_order_app_tree_node != nullptr);
                const COrderDoc* pOrderDoc = form_order_app_tree_node->GetOrderDocument();
                ASSERT_VALID(pOrderDoc);

                aDictFNames.Add(pOrderDoc->GetFormFile().GetDictionaryFilename());
            }

            break;
        }

        case EngineAppType::Tabulation:
        {
            ASSERT(app.GetTableSpecFilePaths().size() == 1);
            CTabTreeCtrl& tabTree = pFrame->GetDlgBar().m_TableTree;
            TableSpecTabTreeNode* const table_spec_tab_tree_node = tabTree.GetTableSpecTabTreeNode(app.GetTableSpecFilePaths().front());
            CTabulateDoc* const pTabDoc = table_spec_tab_tree_node->GetTabDoc();
            ASSERT_VALID(pTabDoc);
            CTabSet* const pTabSpec = pTabDoc->GetTableSpec();
            ASSERT_VALID(pTabSpec);

            // get dictionary
            aDictFNames.Add(pTabSpec->GetDictFile());

            break;
        }

        default:
            ASSERT(false); // INVALID APP TYPE FOR SAVE AS
    }

    // add external dicts
    AppendUnique(aDictFNames, UTF8_TODO::GetCString(GetAppObject().GetExternalDictionaryFilePaths()));

    // now get the actual dictionaries (rather than the names)
    std::vector<std::tuple<std::string, std::shared_ptr<CDataDict>>> dictionaries;

    CDDTreeCtrl& dictTree = pFrame->GetDlgBar().m_DictTree;
    for( int iDict = 0; iDict < aDictFNames.GetSize(); iDict++ )
    {
        DictionaryDictTreeNode* const dictionary_dict_tree_node = dictTree.GetDictionaryTreeNode(UTF8_TODO::GetUtf8(aDictFNames[iDict]));
        ASSERT(dictionary_dict_tree_node != nullptr);
        ASSERT_VALID(dictionary_dict_tree_node->GetDDDoc());
        dictionaries.emplace_back(UTF8_TODO::GetUtf8(aDictFNames[iDict]), dictionary_dict_tree_node->GetDDDoc()->GetSharedDictionary());
    }

    return dictionaries;
}


std::vector<const CDataDict*> CAplDoc::GetAllDictsInApp()
{
    std::vector<const CDataDict*> dictionaries;

    for( const auto& [dictionary_file_path, dictionary] : GetAllDictionaries() )
        dictionaries.emplace_back(dictionary.get());

    return dictionaries;
}


std::vector<std::tuple<std::string, std::shared_ptr<CDEFormFile>>> CAplDoc::GetAllFormFiles()
{
    std::vector<std::tuple<std::string, std::shared_ptr<CDEFormFile>>> form_file_paths;
    CMainFrame* const pFrame = assert_cast<CMainFrame*>(AfxGetMainWnd());

    auto add_form_file = [&](const std::string& form_file_path, FormFileBasedDoc* const form_doc)
    {
        ASSERT_VALID(form_doc);
        form_file_paths.emplace_back(form_file_path, form_doc->GetSharedFormFile());
    };

    if( GetEngineAppType() == EngineAppType::Entry )
    {
        CFormTreeCtrl& form_tree = pFrame->GetDlgBar().m_FormTree;

        for( const std::string& form_file_path : m_application->GetFormFilePaths() )
        {
            CFormNodeID* const form_node = form_tree.GetFormNode(form_file_path);
            ASSERT(form_node != nullptr);

            add_form_file(form_file_path, form_node->GetFormDoc());
        }
    }

    else if( GetEngineAppType() == EngineAppType::Batch )
    {
        COrderTreeCtrl& order_tree = pFrame->GetDlgBar().m_OrderTree;

        for( const std::string& form_file_path : m_application->GetFormFilePaths() )
        {
            FormOrderAppTreeNode* const form_order_app_tree_node = order_tree.GetFormOrderAppTreeNode(form_file_path);
            ASSERT(form_order_app_tree_node != nullptr);

            add_form_file(form_file_path, form_order_app_tree_node->GetOrderDocument());
        }
    }

    else
    {
        ASSERT(false);
    }

    return form_file_paths;
}


std::vector<std::tuple<std::string, std::shared_ptr<CTabSet>>> CAplDoc::GetAllTableSpecs()
{
    std::vector<std::tuple<std::string, std::shared_ptr<CTabSet>>> table_specs;
    CMainFrame* const pFrame = assert_cast<CMainFrame*>(AfxGetMainWnd());

    if( GetEngineAppType() == EngineAppType::Tabulation )
    {
        CTabTreeCtrl& tab_tree = pFrame->GetDlgBar().m_TableTree;

        for( const std::string& table_spec_file_path : m_application->GetTableSpecFilePaths() )
        {
            TableSpecTabTreeNode* const table_spec_tab_tree_node = tab_tree.GetTableSpecTabTreeNode(table_spec_file_path);
            ASSERT(table_spec_tab_tree_node != nullptr && table_spec_tab_tree_node->GetTabDoc() != nullptr);

            table_specs.emplace_back(table_spec_file_path, table_spec_tab_tree_node->GetTabDoc()->GetSharedTableSpec());
        }
    }

    else
    {
        ASSERT(false);
    }

    return table_specs;
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
    CMainFrame* const pFrame = assert_cast<CMainFrame*>(AfxGetMainWnd());
    CFormTreeCtrl& formTree = pFrame->GetDlgBar().m_FormTree;
    COrderTreeCtrl& orderTree = pFrame->GetDlgBar().m_OrderTree;
    CTabTreeCtrl& tabTree = pFrame->GetDlgBar().m_TableTree;

    CDDTreeCtrl& dictTree =  pFrame->GetDlgBar().m_DictTree;

    if( m_application->GetEngineAppType() == EngineAppType::Entry )
    {
        for( const std::string& form_file_path : m_application->GetFormFilePaths() )
        {
            CFormNodeID* const pFormNode = formTree.GetFormNode(form_file_path);
            if (pFormNode == nullptr)
                continue;
            ASSERT(pFormNode->GetFormDoc());

            std::shared_ptr<CDEFormFile> pFormFile = pFormNode->GetFormDoc()->GetSharedFormFile();
            m_application->AddRuntimeFormFile(pFormFile);

            DictionaryDictTreeNode* const dictionary_dict_tree_node = dictTree.GetDictionaryTreeNode(UTF8_TODO::GetUtf8(pFormFile->GetDictionaryFilename()));
            if(dictionary_dict_tree_node != nullptr && dictionary_dict_tree_node->GetDDDoc() != nullptr) {
                pFormFile->SetDictionary(dictionary_dict_tree_node->GetDDDoc()->GetSharedDictionary());
            }
        }

        SetEDictObjects();

        m_application->SetCapiQuestionManager(m_questionManager);
    }

    else if( m_application->GetEngineAppType() == EngineAppType::Batch )
    {
        for( const std::string& form_file_path : m_application->GetFormFilePaths() )
        {
            FormOrderAppTreeNode* const form_order_app_tree_node = orderTree.GetFormOrderAppTreeNode(form_file_path);
            ASSERT(form_order_app_tree_node->GetDocument() != nullptr);

            std::shared_ptr<CDEFormFile> pOrderFile = form_order_app_tree_node->GetOrderDocument()->GetSharedFormFile();
            m_application->AddRuntimeFormFile(pOrderFile);

            DictionaryDictTreeNode* const dictionary_dict_tree_node = dictTree.GetDictionaryTreeNode(UTF8_TODO::GetUtf8(pOrderFile->GetDictionaryFilename()));
            CDDDoc* pDDDoc = dictionary_dict_tree_node->GetDDDoc();
            if(pDDDoc) {
                pOrderFile->SetDictionary(pDDDoc->GetSharedDictionary());
            }
        }

        SetEDictObjects();
    }

    else if( m_application->GetEngineAppType() == EngineAppType::Tabulation )
    {
        TableSpecTabTreeNode* const table_spec_tab_tree_node = tabTree.GetTableSpecTabTreeNode(m_application->GetTableSpecFilePaths().front());
        if(!table_spec_tab_tree_node->GetTabDoc()){
            return;
        }
        ASSERT(table_spec_tab_tree_node->GetTabDoc());
        CTabSet* pTabSet = table_spec_tab_tree_node->GetTabDoc()->GetTableSpec();

        ASSERT(pTabSet);
        SetEDictObjects();
        std::shared_ptr<const CDataDict> pWorkDataDict;

        for( size_t i = 0; i < m_application->GetExternalDictionaryFilePaths().size(); ++i )
        {
            const std::string& dictionary_file_path = m_application->GetExternalDictionaryFilePaths()[i];
            std::shared_ptr<CDataDict> pDataDict = m_application->GetRuntimeExternalDictionaries()[i];

            DictionaryDescription* dictionary_description = m_application->GetDictionaryDescription(dictionary_file_path);

            if( dictionary_description == nullptr )
            {
                dictionary_description = m_application->AddDictionaryDescription(DictionaryDescription(dictionary_file_path, DictionaryType::Working));
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

        CFormNodeID* const pNode = dlgBar.m_FormTree.GetFormNode(m_application->GetFormFilePaths().front());

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
        if(!m_application->GetFormFilePaths().empty()) {
            FormOrderAppTreeNode* const form_order_app_tree_node = orderTree.GetFormOrderAppTreeNode(m_application->GetFormFilePaths().front());
            ASSERT(form_order_app_tree_node != nullptr &&
                   form_order_app_tree_node->GetHItem() != nullptr &&
                   form_order_app_tree_node->GetOrderDocument() != nullptr);

            POSITION pos = form_order_app_tree_node->GetOrderDocument()->GetFirstViewPosition();
            COrderChildWnd* const pOrderChildWnd = assert_cast<COrderChildWnd*>(form_order_app_tree_node->GetOrderDocument()->GetNextView(pos)->GetParentFrame());
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
        if(!m_application->GetTableSpecFilePaths().empty()) {
            TableSpecTabTreeNode* const table_spec_tab_tree_node = tabTree.GetTableSpecTabTreeNode(m_application->GetTableSpecFilePaths().front());
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
    if( m_application->GetFormFilePaths().empty() )
        return;

    CMDlgBar& dlgBar = assert_cast<CMainFrame*>(AfxGetMainWnd())->GetDlgBar();

    if( GetEngineAppType() == EngineAppType::Entry )
    {
        CFormTreeCtrl& formTree = dlgBar.m_FormTree;
        CFormNodeID* const pNode = formTree.GetFormNode(m_application->GetFormFilePaths().front());
        formTree.InsertExternalCodeAndReportNodes(pNode);
    }

    else if( GetEngineAppType() == EngineAppType::Batch )
    {
        COrderTreeCtrl& orderTree = dlgBar.m_OrderTree;
        FormOrderAppTreeNode* const form_order_app_tree_node = orderTree.GetFormOrderAppTreeNode(m_application->GetFormFilePaths().front());
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
    if( !m_application->GetExternalDictionaryFilePaths().empty() ) {
        CMainFrame* const pFrame = assert_cast<CMainFrame*>(AfxGetMainWnd());

        CDDTreeCtrl& dictTree = pFrame->GetDlgBar().m_DictTree;

        for( int iIndex = 0; iIndex < static_cast<int>(m_application->GetExternalDictionaryFilePaths().size()); iIndex++ ) {
            const std::string& dictionary_file_path = m_application->GetExternalDictionaryFilePaths()[iIndex];
            DictionaryDictTreeNode* const dictionary_dict_tree_node = dictTree.GetDictionaryTreeNode(dictionary_file_path);

            if( dictionary_dict_tree_node != nullptr && dictionary_dict_tree_node->GetDDDoc() == nullptr )
            {
                if( !dictTree.OpenDictionary(dictionary_file_path, FALSE) )
                {
                    AfxMessageBox(FormatText("Failed to open external dictionary '%s'. It will be removed from the application.", dictionary_file_path.c_str()));

                    dictTree.ReleaseDictionaryNode(*dictionary_dict_tree_node);

                    m_application->DropExternalDictionary(dictionary_file_path);
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
    for( const AppMessageFile& app_message_file : m_application->GetMessageFiles() )
    {
        if( app_message_file.GetTextSource().RequiresSave() )
            return true;
    }

    // reports
    for( const ReportFile& report_file : m_application->GetReportFiles() )
    {
        if( report_file.GetTextSource().RequiresSave() )
            return true;
    }

    CCSProApp* pApp = assert_cast<CCSProApp*>(AfxGetApp());

    // external dictionaries
    for( const std::string& dictionary_file_path : m_application->GetExternalDictionaryFilePaths() )
    {
        CDocument* const pDoc = pApp->GetDoc(dictionary_file_path);

        if( pDoc != nullptr && pDoc->IsModified() )
            return true;
    }

    //Check if the forms are modified
    if(GetEngineAppType() == EngineAppType::Entry) {
        if(m_application->GetAppSrcCode()->IsModified())
            return true;
        if(m_application->GetUseQuestionText() && m_questionManager != nullptr && m_questionManager->IsModified())
            return TRUE;

        for( const std::string& form_file_path : m_application->GetFormFilePaths() ) {
            CDocument* const pDoc = pApp->GetDoc(form_file_path);
            if(!pDoc) {
                continue;
            }

            if(pDoc->IsModified()) {
                return true;
            }
            else {
                //check the forms dictionaries
                ASSERT(pDoc->IsKindOf(RUNTIME_CLASS(CFormDoc)));
                CDEFormFile* const pFormFile = &assert_cast<CFormDoc*>(pDoc)->GetFormFile();

                CDocument* const pDictDoc = pApp->GetDoc(UTF8_TODO::GetUtf8(pFormFile->GetDictionaryFilename()));
                if(pDictDoc->IsModified())
                    return true;
            }

            if( m_application->GetUseQuestionText() )
            {
                CFormDoc* const pFormDoc = assert_cast<CFormDoc*>(pDoc);
                CFormChildWnd* const pFrame = assert_cast<CFormChildWnd*>(pFormDoc->GetView(FormViewType::Form)->GetParentFrame());

                if( pFrame != nullptr && pFrame->IsQuestionTextModified() )
                    return true;
            }
        }
    }

    else if(GetEngineAppType() == EngineAppType::Batch) {
        if(m_application->GetAppSrcCode()->IsModified())
            return true;

        for( const std::string& order_file_path : m_application->GetFormFilePaths() ) {
            CDocument* const pDoc = pApp->GetDoc(order_file_path);
            if(!pDoc) {
                continue;
            }
            if(pDoc->IsModified()) {
                return true;
            }
            else {
                //check the order dictionaries
                ASSERT(pDoc->IsKindOf(RUNTIME_CLASS(COrderDoc)));
                CDEFormFile* const pOrderFile = &assert_cast<COrderDoc*>(pDoc)->GetFormFile();
                CDocument* const pDictDoc = pApp->GetDoc(UTF8_TODO::GetUtf8(pOrderFile->GetDictionaryFilename()));
                if(pDictDoc->IsModified())
                    return true;
            }
        }
    }

    else if(GetEngineAppType() == EngineAppType::Tabulation) {
        if(m_application->GetAppSrcCode()->IsModified())
            return true;

        for( const std::string& table_spec_file_path : m_application->GetTableSpecFilePaths() ) {
            CDocument* const pDoc = pApp->GetDoc(table_spec_file_path);
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
                    CDocument* const pDictDoc = pApp->GetDoc(UTF8_TODO::GetUtf8(pTabSpec->GetDictFile()));
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
        CMainFrame* const pFrame = assert_cast<CMainFrame*>(AfxGetMainWnd());
        CFormTreeCtrl& formTree = pFrame->GetDlgBar().m_FormTree;

        for( const std::string& form_file_path : m_application->GetFormFilePaths() )
        {
            //get at the form tree, get the docs, and release them
            CFormNodeID* const pID = formTree.GetFormNode(form_file_path);
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
        CMainFrame* const pFrame = assert_cast<CMainFrame*>(AfxGetMainWnd());
        COrderTreeCtrl& orderTree = pFrame->GetDlgBar().m_OrderTree;

        for( const std::string& form_file_path : m_application->GetFormFilePaths() )
        {
            //get at the order tree, get the docs, and release them
            FormOrderAppTreeNode* const form_order_app_tree_node = orderTree.GetFormOrderAppTreeNode(form_file_path);
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
        CMainFrame* const pFrame = assert_cast<CMainFrame*>(AfxGetMainWnd());
        CTabTreeCtrl& tabTree = pFrame->GetDlgBar().m_TableTree;

        for( const std::string& table_spec_file_path : m_application->GetTableSpecFilePaths() ) {
            //get at the tabsped tree, get the docs, and release them
            TableSpecTabTreeNode* const table_spec_tab_tree_node = tabTree.GetTableSpecTabTreeNode(table_spec_file_path);
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
    if( !m_application->IsNameUnique(UTF8_TODO::GetUtf8(name)) )
        return false;

    // check external dictionaries
    if( !IsNameUniqueInExternalDictionaries(name) )
        return false;

    // check application-specific values
    if( pDoc->IsKindOf(RUNTIME_CLASS(CFormDoc)) )
    {
        if( !IsNameUniqueInFormDictionaries(name) || !IsNameUniqueInForms(name) )
            return false;
    }

    else if( pDoc->IsKindOf(RUNTIME_CLASS(COrderDoc)) )
    {
        if( !IsNameUniqueInOrderDictionaries(name) || !IsNameUniqueInOrders(name) )
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
        if( pDict != nullptr && pDict->LookupName(UTF8_TODO::GetUtf8(name), &iL, &iR, &iI, &iVS) )
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


bool CAplDoc::IsNameUniqueInFormDictionaries(const CString& name) const
{
    CMainFrame* const pFrame = assert_cast<CMainFrame*>(AfxGetMainWnd());
    CFormTreeCtrl& formTree = pFrame->GetDlgBar().m_FormTree;
    CDDTreeCtrl& dictTree = pFrame->GetDlgBar().m_DictTree;

    for( const std::string& form_file_path : m_application->GetFormFilePaths() ) {
        CFormNodeID* const pID = formTree.GetFormNode(form_file_path);

        if(pID != nullptr && pID->GetFormDoc()) {

            CDEFormFile* pFormFile = &pID->GetFormDoc()->GetFormFile();
            CString sDictName = pFormFile->GetDictionaryFilename();
            DictionaryDictTreeNode* const dictionary_dict_tree_node = dictTree.GetDictionaryTreeNode(UTF8_TODO::GetUtf8(sDictName));
            if(dictionary_dict_tree_node != nullptr && dictionary_dict_tree_node->GetDDDoc() != nullptr) {
                int iL, iR, iI, iVS;
                if(dictionary_dict_tree_node->GetDDDoc()->GetDict()->LookupName(UTF8_TODO::GetUtf8(name), &iL, &iR, &iI, &iVS))
                    return false;
            }
        }
    }

    return true;
}



bool CAplDoc::IsNameUniqueInExternalDictionaries(const CString& name) const
{
    CMainFrame* const pFrame = assert_cast<CMainFrame*>(AfxGetMainWnd());
    CDDTreeCtrl& dictTree = pFrame->GetDlgBar().m_DictTree;
    //Save the input dictionaries
    for( const std::string& dictionary_file_path: m_application->GetExternalDictionaryFilePaths() ) {
        // get at the dictionary tree and get the documents
        DictionaryDictTreeNode* const dictionary_dict_tree_node = dictTree.GetDictionaryTreeNode(dictionary_file_path);
        if(dictionary_dict_tree_node->GetDDDoc() != nullptr) {
            int iL, iR, iI, iVS;
            if(dictionary_dict_tree_node->GetDDDoc()->GetDict()->LookupName(UTF8_TODO::GetUtf8(name), &iL, &iR, &iI, &iVS))
                return false;
        }
    }

    return true;
}


bool CAplDoc::IsNameUniqueInForms(const CString& name) const
{
    CMainFrame* const pFrame = assert_cast<CMainFrame*>(AfxGetMainWnd());
    CFormTreeCtrl& formTree = pFrame->GetDlgBar().m_FormTree;

    for( const std::string& form_file_path : m_application->GetFormFilePaths() )
    {
        CFormNodeID* const pID = formTree.GetFormNode(form_file_path);

        if(pID->GetFormDoc()) {
            if(!pID->GetFormDoc()->GetFormFile().IsNameUnique(name))
                return false;
        }
    }

    return true;
}

//SAVY 05/18/00 No Update required for CSBatch
bool CAplDoc::IsNameUniqueInOrders(const CString& name) const
{
    CMainFrame* const pFrame = assert_cast<CMainFrame*>(AfxGetMainWnd());

    COrderTreeCtrl& OrderTree = pFrame->GetDlgBar().m_OrderTree;

    for( const std::string& form_file_path : m_application->GetFormFilePaths() )
    {
        FormOrderAppTreeNode* const form_order_app_tree_node = OrderTree.GetFormOrderAppTreeNode(form_file_path);

        if(form_order_app_tree_node->GetDocument() != nullptr) {
            if(!form_order_app_tree_node->GetOrderDocument()->GetFormFile().IsNameUnique(name))
                return false;
        }
    }

    return true;
}

//SAVY 05/18/00 No Update required for CSBatch
bool CAplDoc::IsNameUniqueInOrderDictionaries(const CString& name) const
{
    CMainFrame* const pFrame = assert_cast<CMainFrame*>(AfxGetMainWnd());

    COrderTreeCtrl& orderTree = pFrame->GetDlgBar().m_OrderTree;
    CDDTreeCtrl& dictTree = pFrame->GetDlgBar().m_DictTree;

    for( const std::string& order_file_path : m_application->GetFormFilePaths() ) {
        FormOrderAppTreeNode* const form_order_app_tree_node = orderTree.GetFormOrderAppTreeNode(order_file_path);

        if(form_order_app_tree_node != nullptr && form_order_app_tree_node->GetDocument() != nullptr) {

            CDEFormFile* const pOrderFile = &form_order_app_tree_node->GetOrderDocument()->GetFormFile();
            CString sDictName = pOrderFile->GetDictionaryFilename();
            DictionaryDictTreeNode* const dictionary_dict_tree_node = dictTree.GetDictionaryTreeNode(UTF8_TODO::GetUtf8(sDictName));
            if(dictionary_dict_tree_node != nullptr && dictionary_dict_tree_node->GetDDDoc() != nullptr) {
                int iL, iR, iI, iVS;
                if(dictionary_dict_tree_node->GetDDDoc()->GetDict()->LookupName(UTF8_TODO::GetUtf8(name), &iL, &iR, &iI, &iVS))
                    return false;
            }
        }
    }

    return true;
}


void CAplDoc::SetEDictObjects()
{
    m_application->GetRuntimeExternalDictionaries().clear();

    //Get the form object from the memory
    CMainFrame* const pFrame = assert_cast<CMainFrame*>(AfxGetMainWnd());
    CDDTreeCtrl& dictTree = pFrame->GetDlgBar().m_DictTree;

    for( const std::string& dictionary_file_path : m_application->GetExternalDictionaryFilePaths() ) {
        DictionaryDictTreeNode* const dictionary_dict_tree_node = dictTree.GetDictionaryTreeNode(dictionary_file_path);
        if(dictionary_dict_tree_node != nullptr && dictionary_dict_tree_node->GetDDDoc() == nullptr){
            if(!dictTree.OpenDictionary(dictionary_file_path))
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
    if(m_application->GetFormFilePaths().empty()) {
        CString sMsg;
        sMsg.FormatMessage(_T("No Form File Associated with this Application"));
        AfxMessageBox(sMsg);
        return FALSE;
    }

    for(int iIndex=0 ;iIndex < static_cast<int>(m_application->GetFormFilePaths().size()); iIndex ++) {
        const std::string& form_file_path = m_application->GetFormFilePaths()[iIndex];
        CFormNodeID* const pNode = formTree.GetFormNode(form_file_path);

        if(!pNode->GetFormDoc()) {
            if(!formTree.OpenFormFile(form_file_path, TRUE))
                return FALSE;
            else {
                CDEFormFile* pFile = &pNode->GetFormDoc()->GetFormFile();
                ASSERT(pFile);
                if(pFile->GetDictionaryFilename().IsEmpty()) {
                    AfxMessageBox(FormatText(L"%s has no associated dictionaries", pNode->GetFormDoc()->GetPathName().GetString()));
                    return FALSE;
                }
                CDDTreeCtrl& dictTree = dlgBar.m_DictTree;

                CString sDictFile = pFile->GetDictionaryFilename();
                DictionaryDictTreeNode* const dictionary_dict_tree_node = dictTree.GetDictionaryTreeNode(UTF8_TODO::GetUtf8(sDictFile));

                CString sMsg;
                sMsg.FormatMessage(_T("Failed to open %1"), sDictFile.GetString());
                if(dictionary_dict_tree_node == nullptr){
                    AfxMessageBox(sMsg);
                    return FALSE;
                }

                if(dictionary_dict_tree_node->GetDDDoc() == nullptr) {
                    if(!dictTree.OpenDictionary(UTF8_TODO::GetUtf8(sDictFile), FALSE)) {
                        AfxMessageBox(sMsg);
                        return FALSE;
                    }
                }
            }
        }

        ASSERT(pNode->GetFormDoc());

        pNode->GetFormDoc()->SetCapiQuestionManager(m_application.get(), m_questionManager);

        CFormChildWnd* pFormChildWnd = (CFormChildWnd*)pNode->GetFormDoc()->GetView()->GetParentFrame();
        ASSERT(pFormChildWnd);
        if( pFormChildWnd )
            pFormChildWnd->SetApplicationName(this->GetPathName());

        QSFView* pQTView = (QSFView*)pNode->GetFormDoc()->GetView(FormViewType::QuestionText);
        if (pQTView) {
            pQTView->SetStyleCss(m_questionManager->GetStylesCss());
            pQTView->SetUpQuestionTextView(m_application->GetApplicationFilePath());
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
    if(m_application->GetFormFilePaths().empty()) {
        CString sMsg;
        sMsg.FormatMessage(_T("No Order File Associated with this Application"));
        AfxMessageBox(sMsg);
        return FALSE;
    }

    for(int iIndex=0 ;iIndex < static_cast<int>(m_application->GetFormFilePaths().size()); iIndex ++) {
        const std::string& order_file_path = m_application->GetFormFilePaths()[iIndex];
        FormOrderAppTreeNode* const form_order_app_tree_node = orderTree.GetFormOrderAppTreeNode(order_file_path);

        if(form_order_app_tree_node->GetDocument() == nullptr) {
            if(!orderTree.OpenOrderFile(order_file_path))
                return FALSE;
            else {
                CDEFormFile* pFile = &form_order_app_tree_node->GetOrderDocument()->GetFormFile();
                ASSERT(pFile);
                if(pFile->GetDictionaryFilename().IsEmpty()) {
                    AfxMessageBox(FormatText(L"%s has no associated dictionaries", form_order_app_tree_node->GetOrderDocument()->GetPathName().GetString()));
                    return FALSE;
                }
                CDDTreeCtrl&    dictTree = dlgBar.m_DictTree;

                CString sDictFile = pFile->GetDictionaryFilename();
                DictionaryDictTreeNode* const dictionary_dict_tree_node = dictTree.GetDictionaryTreeNode(UTF8_TODO::GetUtf8(sDictFile));

                CString sMsg;
                sMsg.FormatMessage(_T("Failed to open %1"), sDictFile.GetString());
                if(dictionary_dict_tree_node == nullptr){
                    AfxMessageBox(sMsg);
                    return FALSE;
                }

                if(dictionary_dict_tree_node->GetDDDoc() == nullptr) {
                    if(!dictTree.OpenDictionary(UTF8_TODO::GetUtf8(sDictFile))) {
                        AfxMessageBox(sMsg);
                        return FALSE;
                    }
                }
            }
        }

        ASSERT(form_order_app_tree_node->GetOrderDocument() != nullptr);
       // orderTree.Select(pNode->GetHItem(),TVGN_CARET);
        POSITION pos = form_order_app_tree_node->GetOrderDocument()->GetFirstViewPosition();
        COrderChildWnd* const pOrderChildWnd = (COrderChildWnd*)form_order_app_tree_node->GetOrderDocument()->GetNextView(pos)->GetParentFrame();
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
    if(m_application->GetTableSpecFilePaths().empty()) {
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

        DictionaryDictTreeNode* const dictionary_dict_tree_node = dictTree.GetDictionaryTreeNode(dictionary_description.GetDictionaryFilePath());
        ASSERT(dictionary_dict_tree_node != nullptr && dictionary_dict_tree_node->GetDDDoc() != nullptr);

        pWorkDict = dictionary_dict_tree_node->GetDDDoc()->GetSharedDictionary();
        break;
    }

    for( const std::string& table_spec_file_path : m_application->GetTableSpecFilePaths() ) {
        TableSpecTabTreeNode* const table_spec_tab_tree_node = tabTree.GetTableSpecTabTreeNode(table_spec_file_path);
        CTabulateDoc* pTabDoc = nullptr;
        if(!table_spec_tab_tree_node->GetTabDoc()) {
            if(!tabTree.OpenTableFile(table_spec_file_path, pWorkDict)){
                return FALSE;
            }
            else {
                pTabDoc = table_spec_tab_tree_node->GetTabDoc();
                CTabSet* pFile = table_spec_tab_tree_node->GetTabDoc()->GetTableSpec();
                CDDTreeCtrl& dictTreeBar = dlgBar.m_DictTree;
                CString sDictFile = pFile->GetDictFile();
                DictionaryDictTreeNode* const dictionary_dict_tree_node = dictTreeBar.GetDictionaryTreeNode(UTF8_TODO::GetUtf8(sDictFile));
                //SAVY &&& Xtabspec has no support for multiple dicts now .
                //add it later
                CString sMsg;
                sMsg.FormatMessage(_T("Failed to open %1"), sDictFile.GetString());
                if(dictionary_dict_tree_node == nullptr){
                    AfxMessageBox(sMsg);
                    return FALSE;
                }
                if(dictionary_dict_tree_node->GetDDDoc() == nullptr) {
                    if(!dictTreeBar.OpenDictionary(UTF8_TODO::GetUtf8(sDictFile), FALSE)) {
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

    for(int iIndex = 0; iIndex < static_cast<int>(m_application->GetRuntimeFormFiles().size()); iIndex++) {
        const std::shared_ptr<const CDEFormFile> pFormFile = m_application->GetRuntimeFormFiles()[iIndex];
        ASSERT(pFormFile);
        CString sPath;
        CString sName = pFormFile->GetName();
        sName.MakeUpper();
        if(arrMap.Lookup(sName,sPath)) {
            if(sPath.IsEmpty())
                sPath = _T("Application");
            sMsg += _T("Unique Name - ") + sName + _T(" in ") + sPath + _T(" clashes with the name in ") + UTF8_TODO::GetCString(m_application->GetFormFilePaths()[iIndex]);
            sMsg += _T("\n");
            bRet = FALSE;

        }
        else {
            arrMap.SetAt(sName, UTF8_TODO::GetCString(m_application->GetFormFilePaths()[iIndex]));
        }
        //Do for each dictionary of the form
        auto pDict = pFormFile->GetDictionary();
        ASSERT(pDict);
        sName = UTF8_TODO::GetCString(pDict->GetName());
        sName.MakeUpper();
        if(arrMap.Lookup(sName,sPath)) {
            if(sPath.IsEmpty())
                sPath = _T("Application");
            sMsg += _T("Unique Name - ") + sName + _T(" in ") + sPath + _T(" clashes with the name in ") + UTF8_TODO::GetCString(pFormFile->GetDictionaryName());
            sMsg += _T("\n");
            bRet = FALSE;

        }
        else {
            arrMap.SetAt(sName, pFormFile->GetDictionaryFilename());
        }
    }

    //Do for the external dictionaries
    for(int iEDict =0; iEDict < static_cast<int>(m_application->GetRuntimeExternalDictionaries().size()); iEDict++) {
        auto pDict = m_application->GetRuntimeExternalDictionaries()[iEDict];
        ASSERT(pDict);
        CString sPath;
        CString sName = UTF8_TODO::GetCString(pDict->GetName());
        sName.MakeUpper();
        if(arrMap.Lookup(sName,sPath)) {
            if(sPath.IsEmpty())
                sPath = _T("Application");
            sMsg += _T("Unique Name - ") +  sName + _T(" in ") + sPath + _T(" clashes with the name in ") + UTF8_TODO::GetCString(m_application->GetExternalDictionaryFilePaths()[iEDict]);
            sMsg += _T("\n");
            bRet = FALSE;

        }
        else {
            arrMap.SetAt(sName, UTF8_TODO::GetCString(m_application->GetExternalDictionaryFilePaths()[iEDict]));
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

        if( !FindDictName(dictionary_description.GetDictionaryFilePath(), UTF8_TODO::GetWide(dictionary_description.GetParentFilePath())) )
            dictionary_descriptions.erase(dictionary_descriptions.begin() + i);
    }

    //Add the DictionaryDescription for objects which do not exist
    //Make sure that the first formfile dictionary type is input
    //Make sure that no other dictype is of input type
    CMainFrame* const pFrame = assert_cast<CMainFrame*>(AfxGetMainWnd());
    if(application.GetEngineAppType() == EngineAppType::Entry){
        CFormTreeCtrl& formTree = pFrame->GetDlgBar().m_FormTree;
        for(int iIndex=0; iIndex < static_cast<int>(application.GetFormFilePaths().size()); iIndex++){
            //for each form
            const std::string& form_file_path = application.GetFormFilePaths()[iIndex];
            CFormNodeID* const pFormNode = formTree.GetFormNode(form_file_path);
            if (pFormNode == nullptr)
                continue;
            CFormDoc* pFDoc = pFormNode->GetFormDoc();
            ASSERT(pFDoc);
            CDEFormFile* pFormFile = &pFDoc->GetFormFile();
            ASSERT(pFormFile);

            CString sDictFName = pFormFile->GetDictionaryFilename();
            DictionaryDescription* dictionary_description = application.GetDictionaryDescription(UTF8_TODO::GetUtf8(sDictFName), form_file_path);

            if( dictionary_description == nullptr ) {
                dictionary_description = application.AddDictionaryDescription(
                    DictionaryDescription(UTF8_TODO::GetUtf8(sDictFName), form_file_path, ( iIndex == 0 ) ? DictionaryType::Input : DictionaryType::External));
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
        for(int iIndex=0; iIndex < static_cast<int>(application.GetFormFilePaths().size()); iIndex++){
            //for each form
            const std::string& order_file_path = application.GetFormFilePaths()[iIndex];
            FormOrderAppTreeNode* const form_order_app_tree_node = orderTree.GetFormOrderAppTreeNode(order_file_path);
            ASSERT(form_order_app_tree_node != nullptr);
            COrderDoc* pFDoc = form_order_app_tree_node->GetOrderDocument();
            ASSERT(pFDoc);
            CDEFormFile* pFormFile = &pFDoc->GetFormFile();

            CString sDictFName = pFormFile->GetDictionaryFilename();
            DictionaryDescription* dictionary_description = application.GetDictionaryDescription(UTF8_TODO::GetUtf8(sDictFName), order_file_path);

            if( dictionary_description == nullptr ) {
                dictionary_description = application.AddDictionaryDescription(
                    DictionaryDescription(UTF8_TODO::GetUtf8(sDictFName), order_file_path, ( iIndex == 0 ) ? DictionaryType::Input : DictionaryType::External));
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
    for(int iIndex =0 ;iIndex < static_cast<int>(application.GetExternalDictionaryFilePaths().size()); iIndex++){
        const std::string& dictionary_file_path = application.GetExternalDictionaryFilePaths()[iIndex];
        DictionaryDescription* dictionary_description = application.GetDictionaryDescription(dictionary_file_path);

        if( dictionary_description == nullptr ) {
            dictionary_description = application.AddDictionaryDescription(DictionaryDescription(dictionary_file_path, DictionaryType::External));
        }
        else {
            if(iIndex < static_cast<int>(application.GetRuntimeExternalDictionaries().size())){
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
//BOOL CAplDoc::FindDictName(const std::string& dictionary_file_path, const std::wstring& sFormName)
//  Assumes that the AppObjects are set
/////////////////////////////////////////////////////////////////////
bool CAplDoc::FindDictName(const std::string& dictionary_file_path, const std::wstring& sFormName)
{
    Application& application = this->GetAppObject();

    if(!sFormName.empty() && GetEngineAppType() == EngineAppType::Entry){
        //You cannot get the CDEFormFile name from the object so get the
        //CFormDoc  and look in it
        CMainFrame* const pFrame = assert_cast<CMainFrame*>(AfxGetMainWnd());
        CFormTreeCtrl& formTree = pFrame->GetDlgBar().m_FormTree;
        CFormNodeID* const pFormNode = formTree.GetFormNode(UTF8_TODO::GetUtf8(sFormName));
        if(!pFormNode || !pFormNode->GetFormDoc())
            return false;
        CFormDoc* pFormDoc = pFormNode->GetFormDoc();
        CDEFormFile* pFFSpec = &pFormDoc->GetFormFile();

        if(SO::EqualsNoCase(dictionary_file_path, pFFSpec->GetDictionaryFilename())){
            return true;
        }
    }
    else if(!sFormName.empty() && GetEngineAppType() == EngineAppType::Batch){
        //You cannot get the CDEFormFile name from the object so get the
        //CFormDoc  and look in it
        CMainFrame* const pFrame = assert_cast<CMainFrame*>(AfxGetMainWnd());
        COrderTreeCtrl& formTree = pFrame->GetDlgBar().m_OrderTree;
        FormOrderAppTreeNode* const form_order_app_tree_node = formTree.GetFormOrderAppTreeNode(UTF8_TODO::GetUtf8(sFormName));
        if(form_order_app_tree_node == nullptr || form_order_app_tree_node->GetOrderDocument() == nullptr )
            return false;
        COrderDoc* pOrderDoc = form_order_app_tree_node->GetOrderDocument();
        CDEFormFile* pFFSpec = &pOrderDoc->GetFormFile();

        if(SO::EqualsNoCase(dictionary_file_path, pFFSpec->GetDictionaryFilename())){
            return true;
        }
    }
    else {
        //look in edicts
        for( const std::string& external_dictionary_file_path : application.GetExternalDictionaryFilePaths() )
        {
            if( SO::EqualsNoCase(dictionary_file_path, external_dictionary_file_path) )
                return true;
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
    m_questionManager = std::make_shared<CapiQuestionManager>();

    //if qsf file does not exist then create one
    if( !PortableFunctions::FileIsRegular(m_application->GetQuestionTextFilePath()) )
        m_questionManager->Save(m_application->GetQuestionTextFilePath());

    m_questionManager->Load(m_application->GetQuestionTextFilePath());
}



/////////////////////////////////////////////////////////////////////////////////
//
//  SetCapiTextForAllConditions
//
/////////////////////////////////////////////////////////////////////////////////
void CAplDoc::SetCapiTextForAllConditions(CDEItemBase* item_base, SharableString question_text, const std::string& language_name/* = SO::Empty_string*/)
{
    ASSERT(m_questionManager != nullptr);

    const std::string item_name = CapiName::Create(item_base);
    const CapiQuestion* const existing_question = m_questionManager->GetQuestion(item_name);
    CapiQuestion question = ( existing_question != nullptr ) ? *existing_question :
                                                               CapiQuestion(item_name);

    std::vector<CapiCondition>& conditions = question.GetConditions();

    auto set_text = [&](CapiCondition& condition)
    {
        if( language_name.empty() )
        {
            for( const Language& language : m_questionManager->GetLanguages() )
                condition.SetQuestionText(CapiText(question_text), language.GetName());
        }

        else
        {
            condition.SetQuestionText(CapiText(question_text), language_name);
        }
    };

    if( conditions.empty() )
        conditions.emplace_back();

    for( CapiCondition& condition : conditions )
    {
        if( language_name.empty() )
        {
            for( const Language& language : m_questionManager->GetLanguages() )
                condition.SetQuestionText(CapiText(question_text), language.GetName());
        }

        else
        {
            condition.SetQuestionText(question_text, language_name);
        }
    }

    m_questionManager->SetQuestion(std::move(question));
}



/////////////////////////////////////////////////////////////////////////////////
//
//  bool CAplDoc::IsQHAvailable(CDEItemBase* pBase)
//
/////////////////////////////////////////////////////////////////////////////////
bool CAplDoc::IsQHAvailable(const CDEItemBase* const item_base)
{
    if( !m_application->GetUseQuestionText() || m_questionManager == nullptr )
        return false;

    const CapiQuestion* const question = m_questionManager->GetQuestion(CapiName::Create(item_base));

    if( question == nullptr )
        return false;

    for( const CapiCondition& condition : question->GetConditions() )
    {
        for( const Language& language : m_questionManager->GetLanguages() )
        {
            if( condition.GetQuestionText(language.GetName()) != nullptr ||
                condition.GetHelpText(language.GetName()) != nullptr )
            {
                return true;
            }
        }
    }

    return false;
}


bool CAplDoc::GetLangInfo(CArray<CLangInfo,CLangInfo&>& arrInfo)
{
    ASSERT(m_questionManager != nullptr);
    arrInfo.RemoveAll();
    for (const Language& lang : m_questionManager->GetLanguages()) {
        CLangInfo langInfo;
        langInfo.m_sLangName = UTF8_TODO::GetCString(lang.GetName());
        langInfo.m_sLabel = UTF8_TODO::GetCString(lang.GetLabel());
        arrInfo.Add(langInfo);
    }

    return true;
}


void CAplDoc::ProcessLangs(CArray<CLangInfo,CLangInfo&>& arrInfo)
{
    ASSERT( m_questionManager != nullptr );

    int iNumLanguages = m_questionManager->GetLanguages().size();

    //First process langs which are modified
    for(int iLangInfo=0; iLangInfo < arrInfo.GetSize(); iLangInfo++) {
        CLangInfo langInfo = arrInfo[iLangInfo];
        if (langInfo.m_eLangInfo == eLANGINFO::MODIFIED_INFO) {
            ASSERT(iNumLanguages > iLangInfo);
            CString sName = langInfo.m_sLangName;
            sName.Trim();
            const auto& current_language = m_questionManager->GetLanguages()[iLangInfo];
            m_questionManager->ModifyLanguage(current_language.GetName(), Language(UTF8_TODO::GetUtf8(sName), UTF8_TODO::GetUtf8(langInfo.m_sLabel)));
        }
    }

    //Second langs which are deleted
    for(int iLangInfo=0; iLangInfo < arrInfo.GetSize(); iLangInfo++) {
        CLangInfo langInfo =arrInfo[iLangInfo];
        if(langInfo.m_eLangInfo == eLANGINFO::DELETED_INFO) {
            m_questionManager->DeleteLanguage(UTF8_TODO::GetUtf8(langInfo.m_sLangName));
        }
    }

    //Finally langs which are ADDED
    for(int iLangInfo=0; iLangInfo < arrInfo.GetSize(); iLangInfo++) {
        CLangInfo langInfo =arrInfo[iLangInfo];
        if(langInfo.m_eLangInfo == eLANGINFO::NEW_INFO) {
            CString sName = langInfo.m_sLangName;
            sName.Trim();
            m_questionManager->AddLanguage(Language(UTF8_TODO::GetUtf8(sName), UTF8_TODO::GetUtf8(langInfo.m_sLabel)));
        }
    }
}


void CAplDoc::ChangeCapiName(const CDEItemBase* const item_base, const std::string& old_name)
{
    // 20120710 so that when changing names of items (and thus fields or blocks), we change the field name in the QSF file
    if( !m_application->GetUseQuestionText() || m_questionManager == nullptr )
        return;

    const CapiQuestion* const old_question = m_questionManager->GetQuestion(old_name);

    if( old_question == nullptr )
        return;

    CapiQuestion new_question = *old_question;
    new_question.SetItemName(CapiName::Create(item_base));

    m_questionManager->RemoveQuestion(old_question->GetItemName());

    m_questionManager->SetQuestion(std::move(new_question));
}


void CAplDoc::ChangeCapiDictName(const CDataDict& dictionary)
{
    if( !m_application->GetUseQuestionText() || m_questionManager == nullptr )
        return;

    const std::string& old_dict_name = dictionary.GetOldName();
    std::string new_item_prefix = dictionary.GetName() + ".";

    const std::regex dict_item_regex(FormatText("^%s\\.", old_dict_name.c_str()));

    const std::vector<CapiQuestion> questions = m_questionManager->GetQuestions();

    for( CapiQuestion question : questions )
    {
        m_questionManager->RemoveQuestion(question.GetItemName());
        question.SetItemName(std::regex_replace(question.GetItemName(), dict_item_regex, new_item_prefix));
        m_questionManager->SetQuestion(std::move(question));
    }
}


std::shared_ptr<TextSourceEditable> CAplDoc::GetLogicMainCodeFileTextSource()
{
    // if no main code file exists, create a default one
    CodeFile* logic_main_code_file = m_application->GetLogicMainCodeFile();

    if( logic_main_code_file == nullptr )
    {
        m_application->AddCodeFile(NewFileCreator::CreateOrOpenCodeFile(*m_application));
        SetModifiedFlag(TRUE);

        logic_main_code_file = m_application->GetLogicMainCodeFile();
    }

    auto text_source = std::dynamic_pointer_cast<TextSourceEditable, TextSource>(logic_main_code_file->GetSharedTextSource());
    ASSERT(text_source != nullptr);

    return text_source;
}


std::shared_ptr<TextSourceEditable> CAplDoc::GetMessageTextSource()
{
    // if no message file exists, create a default one
    if( m_application->GetMessageFiles().empty() )
    {
        m_application->AddMessageFile(NewFileCreator::CreateOrOpenMessageFile(*m_application));
        SetModifiedFlag(TRUE);
    }

    for( AppMessageFile& app_message_file : m_application->GetMessageFilesIterator() )
    {
        auto text_source = std::dynamic_pointer_cast<TextSourceEditable, TextSource>(app_message_file.GetSharedTextSource());

        if( text_source != nullptr )
            return text_source;
    }

    return ReturnProgrammingError(nullptr);
}
