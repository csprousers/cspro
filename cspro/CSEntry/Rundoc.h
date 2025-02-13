#pragma once
// RunDoc.h : interface of the CEntryrunDoc class
//
/////////////////////////////////////////////////////////////////////////////

#include <CSEntry/MessageOverrides.h>

class COperatorStatisticsLog;
class CNPifFile;

class CEntryrunDoc : public CDocument
{
protected: // create from serialization only
    CEntryrunDoc();
    DECLARE_DYNCREATE(CEntryrunDoc)
private:
    CNPifFile*               m_pPIFFile;
    CRunAplEntry*            m_pRunApl;
    CDEItemBase*             m_pCurrField;
    APP_MODE                 m_appMode;
    BOOL                     m_bQModified; //Is questionnaire modified
    COperatorStatisticsLog*  m_pOperatorStatisticsLog;
    bool                     m_bInteractiveEdit;
    MessageOverrides         m_messageOverrides;
    std::unique_ptr<CString> m_windowTitleOverride;

public:
    COperatorStatisticsLog* GetOperatorStatisticsLog() { return m_pOperatorStatisticsLog; }
    void SetIntEdit(bool bFlag) { m_bInteractiveEdit = bFlag; }
    bool GetIntEdit() { return m_bInteractiveEdit; }

    const MessageOverrides& GetMessageOverrides() const          { return m_messageOverrides; }
    void SetMessageOverrides(MessageOverrides message_overrides) { m_messageOverrides = std::move(message_overrides); }

    // Operations
public:
    void SetAppMode(APP_MODE appMode) { m_appMode = appMode;
        if(m_pRunApl && m_pRunApl->GetEntryDriver() && appMode == NO_MODE) {
            m_pRunApl->GetEntryDriver()->SetPartialMode(appMode);
        }
    }

    APP_MODE GetAppMode(void) const { return m_appMode; }

    // In modify mode: call to EndGroup when dataocc is reached?
    // In Path On the value will be false
    // In Path Off the value will be true
    bool    IsAutoEndGroup() const {return  m_pRunApl->IsAutoEndGroup() ; } // RHF Nov 07, 2000

    //Get PifFile
    CNPifFile* GetPifFile()             { return m_pPIFFile; }
    const CNPifFile* GetPifFile() const { return m_pPIFFile; }

    //Better if the engine provides these
    CDEItemBase*    GetCurField() { return m_pCurrField;}
    void            SetCurField(CDEField* pField ) { m_pCurrField = pField; }
    CDEFormFile*    GetCurFormFile();

    CRunAplEntry*           GetRunApl()  { return m_pRunApl; }

    int                     GetCurFormNum();
    bool                    InitApplication();
    void SetQModified(BOOL bModified) { m_bQModified = bModified; }
    BOOL GetQModified(void) { return m_bQModified ; }

    void OpenOperatorStatisticsLog();

    void AddKeyStroke();
    void AddEntryError();
    void AddVerifyError(BOOL bKeyerErr = FALSE);   //Default is a Verifier Error

    void IncVerifiedField(void);

    CString MakeTitle(bool reset_to_default = false);
    CString GetWindowTitle();
    void SetWindowTitle(const CString& window_title);

    bool IsPartialAdd() {
        bool bRet = false;
        if(m_pRunApl && m_pRunApl->GetEntryDriver()) {
            bRet = m_pRunApl->GetEntryDriver()->GetPartialMode() == ADD_MODE;
        }
        return bRet;
    }
    // Overrides
    // ClassWizard generated virtual function overrides
    //{{AFX_VIRTUAL(CEntryrunDoc)
public:
    virtual void DeleteContents();
    virtual BOOL OnOpenDocument(LPCTSTR lpszPathName);
    //}}AFX_VIRTUAL

    // Implementation
public:
    virtual ~CEntryrunDoc();

    // Generated message map functions
protected:
    //{{AFX_MSG(CEntryrunDoc)
    //}}AFX_MSG
    DECLARE_MESSAGE_MAP()
};

/////////////////////////////////////////////////////////////////////////////

//{{AFX_INSERT_LOCATION}}
// Microsoft Visual C++ will insert additional declarations immediately before the previous line.
