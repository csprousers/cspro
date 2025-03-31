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

    DECLARE_DYNCREATE(CAplDoc)

protected:
    CAplDoc();           // protected constructor used by dynamic creation

public:
    ~CAplDoc();

    // Attributes
    const Application& GetAppObject() const           { return *m_application; }
    Application& GetAppObject()                       { return *m_application; }
    std::shared_ptr<Application> GetSharedAppObject() { return m_application; }

    void ReplaceAppObject(std::unique_ptr<Application> application);

    HTREEITEM BuildAllTrees();

    EngineAppType GetEngineAppType() const { return m_application->GetEngineAppType(); }

    std::vector<std::tuple<std::string, std::shared_ptr<CDataDict>>> GetAllDictionaries(); // path / dictionary
    std::vector<const CDataDict*> GetAllDictsInApp();

    std::vector<std::tuple<std::string, std::shared_ptr<CDEFormFile>>> GetAllFormFiles(); // path / form file

    std::vector<std::tuple<std::string, std::shared_ptr<CTabSet>>> GetAllTableSpecs(); // path / table spec

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

    std::shared_ptr<TextSourceEditable> GetLogicMainCodeFileTextSource();
    std::shared_ptr<TextSourceEditable> GetMessageTextSource();

    BOOL AreAplDictsOK(void);
    bool m_bIsClosing;
    std::shared_ptr<CapiQuestionManager> m_questionManager;
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

    bool IsNameUniqueInForms(const CString& name) const;
    bool IsNameUniqueInFormDictionaries(const CString& name) const;
    bool IsNameUniqueInExternalDictionaries(const CString& name) const;

    bool IsNameUniqueInOrders(const CString& name) const;
    bool IsNameUniqueInOrderDictionaries(const CString& name) const;

public:
    bool IsNameUnique(const CDocument* pDoc, const CString& name) const;
    BOOL CheckUniqueNames(BOOL bSilent = FALSE);
    BOOL Reconcile(BOOL bSilent = FALSE);
    std::vector<CString> GetOrder() const;
    void ReconcileDictTypes();
    bool FindDictName(const std::string& dictionary_file_path, const std::wstring& sFormName);
    void BuildQuestMgr();
    SharableString GetCapiTextForFirstCondition(CDEItemBase* item_base, cs::cref_optional<std::string> language_name = std::nullopt);
    void SetCapiTextForAllConditions(CDEItemBase* item_base, SharableString question_text, const std::string& language_name = SO::Empty_string);
    bool IsQHAvailable(const CDEItemBase* item_base);
    bool GetLangInfo(CArray<CLangInfo,CLangInfo&>& arrInfo);
    void ProcessLangs(CArray<CLangInfo,CLangInfo&>& arrInfo);
    void ChangeCapiName(const CDEItemBase* item_base, const std::string& old_name);
    void ChangeCapiDictName(const CDataDict& dictionary);

    BOOL OnOpenDocument(LPCTSTR lpszPathName) override;
    void OnCloseDocument() override;
    BOOL OnSaveDocument(LPCTSTR lpszPathName) override;

protected:
    DECLARE_MESSAGE_MAP()
};

//{{AFX_INSERT_LOCATION}}
// Microsoft Visual C++ will insert additional declarations immediately before the previous line.
