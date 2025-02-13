#pragma once

// AplDoc.h : header file
//

#include <zAppO/Application.h>
#include <zDictF/langgrid.h>

class CapiQuestionManager;
class CDEItemBase;
class CNPifFile;
class CRunAplEntry;
class TextSourceEditable;


/////////////////////////////////////////////////////////////////////////////
// CAplDoc document


class CAplDoc : public CDocument
{
    friend class CCSProApp;

protected:
    CAplDoc();           // protected constructor used by dynamic creation
    DECLARE_DYNCREATE(CAplDoc)

// Attributes
public:
    const Application& GetAppObject() const           { return *m_application; }
    Application& GetAppObject()                       { return *m_application; }
    std::shared_ptr<Application> GetSharedAppObject() { return m_application; }

    HTREEITEM BuildAllTrees();
    EngineAppType GetEngineAppType() const { return m_application->GetEngineAppType(); }
    std::vector<const CDataDict*> GetAllDictsInApp();
    void SetAppObjects(void);
    BOOL OpenAllDocuments();
    bool IsAppModified();
    BOOL Reconcile(CString& csErr, bool bSilent, bool bAutoFix);
    void SetEDictObjects();
    BOOL ProcessFormOpen();
    BOOL ProcessTabOpen();
    BOOL ProcessOrderOpen();
    BOOL ProcessEDictsOpen();

    void RefreshExternalLogicAndReportNodes();

public:
    static void AddDefaultCodeFile(Application& application, std::optional<CString> base_filename = std::nullopt);
    std::shared_ptr<TextSourceEditable> GetLogicMainCodeFileTextSource();

    static void AddDefaultMessageFile(Application& application, std::optional<CString> base_filename = std::nullopt);
    std::shared_ptr<TextSourceEditable> GetMessageTextSource();

public:
    BOOL AreAplDictsOK(void);
    bool m_bIsClosing;
    std::shared_ptr<CapiQuestionManager> m_pQuestMgr;
    HWND m_deployWnd;
    //Attributes

private:
    std::shared_ptr<Application> m_application;

    bool m_bSrcLoaded;

private:
    void SaveAllDictionaries();
    void SaveOrders();
    void SaveForms();
    void SaveTabSpecs();
    void ReleaseTabSpecs();
    void ReleaseForms();
    void ReleaseOrders();
    void ReleaseEDicts();

    void SaveFormDicts();
    void SaveOrderDicts();
    void SaveTableDicts();

    BOOL ProcessForms(const CString& name) const;
    BOOL ProcessFormDicts(const CString& name) const;
    BOOL ProcessEDicts(const CString& name) const;

    BOOL ProcessOrders(const CString& name) const;
    BOOL ProcessOrderDicts(const CString& name) const;

public:
    bool IsNameUnique(const CDocument* pDoc, const CString& name) const;
    BOOL CheckUniqueNames(BOOL bSilent = FALSE);
    BOOL Reconcile(BOOL bSilent = FALSE);
    std::vector<CString> GetOrder() const;
    void ReconcileDictTypes();
    bool FindDictName(const std::wstring& sDictName, const std::wstring& sFormName);
    void BuildQuestMgr();
    CString GetCapiTextForFirstCondition(CDEItemBase* pBase, wstring_view language_name = wstring_view());
    void SetCapiTextForAllConditions(CDEItemBase* pBase, CString question_text, wstring_view language_name = wstring_view());
    bool IsQHAvailable(const CDEItemBase* pBase);
    bool GetLangInfo(CArray<CLangInfo,CLangInfo&>& arrInfo);
    void ProcessLangs(CArray<CLangInfo,CLangInfo&>& arrInfo);
    void ChangeCapiName(const CDEItemBase* pItem, const CString& old_name);
    void ChangeCapiDictName(const CDataDict& dictionary);

// Operations
public:

// Overrides
    // ClassWizard generated virtual function overrides
    //{{AFX_VIRTUAL(CAplDoc)
    public:
    virtual BOOL OnOpenDocument(LPCTSTR lpszPathName);
    virtual void OnCloseDocument();
    virtual BOOL OnSaveDocument(LPCTSTR lpszPathName);
    protected:
    virtual BOOL OnNewDocument();
    //}}AFX_VIRTUAL

// Implementation
public:
    virtual ~CAplDoc();
#ifdef _DEBUG
    virtual void AssertValid() const;
    virtual void Dump(CDumpContext& dc) const;
#endif

    // Generated message map functions
protected:
    //{{AFX_MSG(CAplDoc)
    afx_msg void OnFileClose();
    //}}AFX_MSG
    DECLARE_MESSAGE_MAP()
};

//{{AFX_INSERT_LOCATION}}
// Microsoft Visual C++ will insert additional declarations immediately before the previous line.
