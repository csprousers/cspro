#pragma once

// TabDoc.h : header file
//

/////////////////////////////////////////////////////////////////////////////
// CTabulateDoc document
#include <zTableF/zTableF.h>
#include <zTableF/TabTrCtl.h>
#include <zTableO/Table.h>
#include <zDesignerF/ApplicationDoc.h>
#include <Zsrcmgro/SrcCode.h>
#include <ZTBDO/cLinkTab.h>

#define TD_TABLE_FORMAT 0


class CLASS_DECL_ZTABLEF CTabulateDoc : public ApplicationDoc
{
    DECLARE_DYNCREATE(CTabulateDoc)

protected:
    CTabulateDoc(); // create from serialization only

public:
    ~CTabulateDoc();

    static const TCHAR* GetExtensionWithDot() { return FileExtensions::WithDot::TableSpec; }

    const CString& GetClipFile() const   { return m_csClipFile; }
    UINT GetClipBoardFormat(UINT format) { return m_auFormat[format]; }
    void FileToClip(UINT uFormat);
    bool ClipToFile (UINT uFormat);

    // DictionaryBasedDoc overrides
    CTreeCtrl* GetTreeCtrl() override;
    std::shared_ptr<const CDataDict> GetSharedDictionary() const override;

public:
    BOOL OnNewDocument() override;
    BOOL OnOpenDocument(LPCTSTR lpszPathName) override;
    BOOL OnSaveDocument(LPCTSTR lpszPathName) override;
    void OnCloseDocument() override;

private:
    void MakeNameMap(CTable* pTable);
    void MakeNameMap(CTabVar* pTabVar,CMapStringToString& arrNames);

public:
    std::shared_ptr<CTabSet> GetSharedTableSpec() { return m_TableSpec; }
    const CTabSet* GetTableSpec() const           { return m_TableSpec.get(); }
    CTabSet* GetTableSpec()                       { return m_TableSpec.get(); }

    bool IsTabModified() const;

    CTabTreeCtrl* GetTabTreeCtrl() const         { return m_pTabTreeCtrl; }
    void SetTabTreeCtrl(CTabTreeCtrl* pTreeCtrl) { m_pTabTreeCtrl = pTreeCtrl ; }

    bool LoadSpecFile(const CString& csFileName, std::shared_ptr<const CDataDict> pWorkDict = nullptr);
    HTREEITEM BuildAllTrees();

    void ReleaseDicts();

    void DisplayFmtErrorMsg();

    CMapStringToString& GetAreaLabelLookup() { return m_TableSpec->GetAreaLabelLookup(); }
    bool InitTreeCtrl();

    const CString& GetDictFileName() const   { return m_csDictFileName; }
    void SetDictFileName (const CString& cs) { m_csDictFileName = cs; }

    const CString& GetAreaFileName() const   { return m_sAreaFileName; }
    void SetAreaFileName (const CString& cs) { m_sAreaFileName = cs; }

    bool GetVarList(CTabVar* pTabVar , CString& sVarList);
    void SaveAllDictionaries();

    void MakeCrossTabStatement(CTable* pRefTable , CString& sCrossTabStatement);

    bool ReconcileLinkTables(CArray<CLinkTable*,CLinkTable*>&arrLinkTable, CString& sErr);
    CTable* GetTableFromLink(CLinkTable* pLinkTable);
    CLinkTable* GetLinkFromTable(CArray<CLinkTable*,CLinkTable*>& arrLinkTable, CTable* pTable);
    bool ReconcileTblNLinkObj(CTable* pTable, CLinkTable* pLinkTable, CString& sErr, bool bSilent = false );
    bool MatchExpr(CIMSAString sTblString, CIMSAString sLinkString);
    bool GetUnitStatement(CTable* pTable,CString& sUnitStatement);
    CString MakeVarList4DummyVSet(const CDictItem* pDictItem);
    bool OpenAreaNameFile();
    bool BuildAreaLookupMap();
    void CloseAreaNameFile();

    bool Reconcile(CString& sError, bool bSilent = false, bool bAutoFix = false);
    bool ReconcileUniverseAndWeights(CTable* pTable, CString& sError);
    bool CheckUnitSyntax(CTable* pTable, CUnitSpec& unit, XTABSTMENT_TYPE eStatementType,
                         bool bTable); // tbl or subtable

    //Begin Changes for Paste Table Checks
    bool SyntaxCheckPasteTbl(CTable* pTbl, CString& sError);
    bool CheckSyntaxAll(CTable* pTable,CString& sError);
    bool CheckSyntaxSubtable(CTable* pTable,int iSubtable,CString& sError);
    bool CheckSyntax(CTable* pTable,int iSubtable, XTABSTMENT_TYPE eStatementType,CString& sError);
    //End Paste Table Checks
   // const CFmtBase*  GetFormat(CTblOb* pTblOb);
    void SetErrorString(std::wstring error = std::wstring()) { m_sErrString = WS2CS(error); }
    const CString& GetErrorString() const                    { return m_sErrString; }

private:
    CString        m_sDictLabel;

    static CLIPFORMAT NEAR m_cfPrivate;     // savi

    UINT           m_auFormat[6]; // Array for clipboard formats
    CString        m_csClipFile;  // File name for temporary clipbord data
    std::shared_ptr<CTabSet> m_TableSpec;  // Table Spec
    CTabTreeCtrl*  m_pTabTreeCtrl;

    CString        m_csDictFileName;
    CMap<CTabVar*, CTabVar*, CString, CString> m_varNameMap;

    CString        m_sAreaFileName;
    CString        m_sErrString;
    CArray<CUnitSpec,CUnitSpec&> m_arrUnitSpecForPasteTblChk;
};


struct TBL_PROC_INFO
{
    CTabulateDoc* pTabDoc;
    CTable* pTable;
    CSourceCode_EventType eEventType;
    CString sTblLogic;
    CLinkTable* pLinkTable;
    bool bGetLinkTables;
};

//{{AFX_INSERT_LOCATION}}
// Microsoft Visual C++ will insert additional declarations immediately before the previous line.
