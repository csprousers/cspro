#pragma once

/////////////////////////////////////////////////////////////////////////////
// CCSProApp:
// See CSPro.cpp for the implementation of this class
//

class CAplDoc;
class CCSProDoc;
class CAplFileAssociationsDlg;
class CWindowFocusMgr;
class FileTreeNode;


int CALLBACK BrowseCallbackProc( HWND hwnd, UINT uMsg, LPARAM lParam, LPARAM lpData );


class CCSProApp : public CWinApp
{
public:
    CCSProApp();

//members
private:
    CCSProDoc* m_pSessionDoc; //One and only One Session Object
    CDocTemplate* m_pAplTemplate;

    CStringArray m_arrRFLStrings;
    CWindowFocusMgr* m_pWindowFocusMgr;

public:
    HICON               m_hIcon;

    // for registering in InitInstance
    CString             m_csWndClassName;

    bool                m_bFileNew;

//methods
public:
    bool IsFileOpen(wstring_view filename) { return ( GetDoc(filename) != nullptr ); }

    bool UpdateViews(CDocument* pDoc);
    const CDocTemplate* GetAppTemplate() const { return m_pAplTemplate; }
    void SetInitialViews(CDocument* pDoc);
    CDocument* GetDoc(wstring_view filename);
    CDocument* IsDocOpen(LPCTSTR lpszFileName)const;
    BOOL Reconcile(CDocument *pDoc, CString& csErr, bool bSilent, bool bAutoFix );
    bool IsDictNew(const CDDDoc* pDoc);
    void CloseApplication(CDocument* pDoc);   //Cant use CAplDoc* 'cos of reference in zDictF Bruce??
    void CloseDocument(CDocument* pDoc);

    void SaveRFL();
    void RestoreRFL();

    void DropHObjects(CDocument* pDoc);//Remove the hanging objects from the object tree
    void ProcessAppHObjects(CAplDoc* pAplDoc);

    void GetFileAssocsForSaveAs(CAplDoc* pApplDoc, const CString& sNewAppName, CAplFileAssociationsDlg& aplFileAssociationsDlg);
    bool SaveAppFilesWithNewNames(CAplDoc* pApplDoc, const CString& sNewAppName, const CAplFileAssociationsDlg& aplFileAssociationsDlg, const CArray<bool, bool&>& aCreateNewDocFlags);
    bool DuplicateSharedFilesForSaveAs(CAplFileAssociationsDlg& aplFileAssociationsDlg, CArray<bool, bool&>& aCreateNewDocFlags);
    void RenameApp(CAplDoc* pApplDoc, const CString& sNewAppName, const CAplFileAssociationsDlg& aplFileAssociationsDlg, const CArray<bool, bool&>& aCreateNewDocFlags);
    void RenameDictionaryFile(CDocument* pParentDoc, CString sOldDictFName, const CString& sNewDictFName, bool bAddToMRU);
    void RenameFormFile(CAplDoc* pApplDoc,
                        CString sOldFName,
                        const CString& sNewFName,
                        const CStringArray& aNewDictNames,
                        const CArray<bool, bool&>& aDictSkipFlags);
    void RenameOrdFile(CAplDoc* pApplDoc, CString sOldFName, const CString& sNewFName,
                       const CStringArray& aNewDictNames, const CArray<bool, bool&>& aDictSkipFlags);
    void RenameTabSpecFile(CAplDoc* pApplDoc, CString sOldFName,const CString& sNewFName,
                           const CString& sNewDictName);
    void RenameAppCodeFile(CAplDoc* pAplDoc, const CString& new_filename);
    void RenameMessageFile(CAplDoc* pAplDoc, const CString& new_filename);
    void RenameQSFFile(CAplDoc* pApplDoc, const CString& sNewFName);
    void RenameNodeInObjTree(CDocument* pTopLevelDoc, wstring_view old_filename, const CString& sNewFName);

private:
    void SyncExternalTextSources(CAplDoc* pAppDoc);

public:
    BOOL InitInstance() override;
    CDocument* OpenDocumentFile(LPCTSTR lpszFileName) override;
    int ExitInstance() override;
    BOOL PreTranslateMessage(MSG* pMsg) override;

protected:
    DECLARE_MESSAGE_MAP()

    afx_msg void OnAppAbout();
    afx_msg BOOL OnOpenRecentFile(UINT nID);

    afx_msg void OnUpdateIsDocumentOpen(CCmdUI* pCmdUI);

public:
    bool IsApplicationOpen();
protected:
    afx_msg void OnUpdateIsApplicationOpen(CCmdUI* pCmdUI);

    afx_msg void OnFileNew();
    afx_msg void OnFileOpen();
    afx_msg void OnFileClose();
    afx_msg void OnFileSave();

    afx_msg void OnCSProSettings();

    // Save As
private:
    bool SaveAsDictionary(CDDDoc* pDictDoc);
    bool SaveAsFormFile(CFormDoc* pFormDoc);
    bool SaveAsApplication(CAplDoc* pDoc);
protected:
    afx_msg void OnFileSaveAs();

    // Add/Drop Files methods
private:
    void AddFileToApp(FileTreeNode& application_file_tree_item, const FileAssociation& file_association);
    std::vector<CDocument*> GetDropCandidatesForApplication(const CAplDoc* pAplDoc);
protected:
    afx_msg void OnAddFiles();
    afx_msg void OnDropFiles();
public:
    void AddResourceFolderToApp(CAplDoc* pAplDoc, const CString& sFolderName);

public:
    afx_msg void OnViewNames();
protected:
    afx_msg void OnUpdateViewNames(CCmdUI* pCmdUI);
    afx_msg void OnViewAppendLabelsToNames();
    afx_msg void OnUpdateViewAppendLabelsToNames(CCmdUI* pCmdUI);
    afx_msg void OnChangeTab();
    afx_msg void OnUpdateRecentFileMenu(CCmdUI* pCmdUI);
    afx_msg void OnUpdateWindowDicts(CCmdUI* pCmdUI);
    afx_msg void OnUpdateWindowForms(CCmdUI* pCmdUI);
    afx_msg void OnUpdateWindowOrder(CCmdUI* pCmdUI);
    afx_msg void OnUpdateWindowTables(CCmdUI* pCmdUI);
    afx_msg void OnWindowDicts();
    afx_msg void OnWindowForms();
    afx_msg void OnWindowOrder();
    afx_msg void OnWindowTables();
    afx_msg void OnFullscreen();
    afx_msg void OnUpdateFullscreen(CCmdUI* pCmdUI);
    afx_msg void OnOnKeyCharacterMap();
    afx_msg void OnCommonStore();

    afx_msg void OnRunTool(UINT nID);
    afx_msg void OnUpdateTool(CCmdUI* pCmdUI);

    afx_msg void OnToolsVersionShifter();
    afx_msg void OnToolsVersionShifter(CCmdUI* pCmdUI);

    afx_msg void OnHelpWhatIsNew();
    afx_msg void OnHelpExamples();
    afx_msg void OnHelpTroubleshooting();
    afx_msg void OnHelpMailingList();
    afx_msg void OnHelpAndroidApp();
    afx_msg void OnHelpShowSyncLog();

    afx_msg void OnPreferencesFonts();

private:
    void CreateDefaultReport(const std::wstring& report_filename);

private:
    CDocument* GetActiveDocument();

    // gets all documents (of type T) that includes a document with the specified filename;
    // if the filename is null, then the active document's filename will be used
    template<typename T>
    std::vector<T*> GetTopLevelAssociatedDocuments(const TCHAR* filename = nullptr, bool stop_after_first_document = false);

    // gets all documents that includes a document with the active document's filename;
    // if there are multiple top-level documents, the user can select which ones to use,
    // using the dialog title provided with the ability to select more than one
    std::vector<CDocument*> GetSelectedTopLevelAssociatedDocuments(const TCHAR* dialog_title);

public:
    // gets the application that includes a document with the specified filename;
    // if the filename is null, then the active document's filename will be used;
    // if there are multiple applications, the user must select which one to use
    // using the dialog title provided; the result will be std::nullopt when there
    // were multiple applications but the user canceled the dialog without selecting one
    std::optional<CAplDoc*> GetActiveApplication(const TCHAR* filename, const TCHAR* dialog_title);
};
