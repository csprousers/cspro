#pragma once

// ExptDoc.h : interface of the CExportDoc class
//
/////////////////////////////////////////////////////////////////////////////

#include <ZBRIDGEO/npff.h>
#include <zUtilO/TemporaryFile.h>

class CExportOptionsView;
class CExportView;


// Table structure
struct ITEMS
{
    int rel;
    const CDictItem* pItem;
    int occ;
    int rec;
    bool selected;
};


//ALLTYPES = SPSS, SAS, STATA, R
enum class METHOD { TABS, COMMADEL, SPSS, SAS, STATA, ALLTYPES, SEMI_COLON, CSPRO, R };

enum class ExportRecordType { None = 0, BeforeIds, AfterIds };

enum class ExportItemsSubitems { ItemsOnly = 0, SubitemsOnly, Both };


class CExportDoc : public CDocument
{
protected: // create from serialization only
    CExportDoc();
    DECLARE_DYNCREATE(CExportDoc)

public:
    CString                     m_csDictFileName;
    std::unique_ptr<std::tuple<std::unique_ptr<TemporaryFile>, ConnectionString>> m_embeddedDictionaryInformation;
    std::shared_ptr<const CDataDict> m_pDataDict;
    CIMSAString                 m_csErrorMessage;
    CIMSAString                 m_csProgTitle;

    CArray<ITEMS,ITEMS>         m_aItems;
    CArray<ITEMS,ITEMS>         m_aMergeItems;
    CStdioFile *                m_pLogFile;
    bool                        m_batchmode;
    CIMSAString                 m_csLogFile;
    CIMSAString                 m_sPFFName;
    CIMSAString                 m_sBaseFilename;
    CIMSAString                 m_sBCHPFFName;
    bool                        m_bSaveExcluded;
    int                         m_iLowestLevel;
    CNPifFile                   m_PifFile;
    bool                        m_bPostRunSave;
    CStringArray                m_arrFileVars4Pff;
    CMap<CString, LPCTSTR, int, int> mapFileVarToPifIndex; // JH 11/17/06 mapping from fileVar to assoc in pff

    bool                        m_bAllInOneRecord;
    bool                        m_bJoinSingleWithMultipleRecords;
    ExportRecordType            m_exportRecordType;
    ExportItemsSubitems         m_exportItemsSubitems;
    bool                        m_bForceANSI; // 20120416
    bool                        m_bCommaDecimal;
    LogicSettings               m_logicSettings;

// Attributes
public:
    std::shared_ptr<const CDataDict> GetSharedDictionary() const { return m_pDataDict; }
    const CDataDict* GetDataDict() const                         { return m_pDataDict.get(); }
    CIMSAString GetDictFileName() const                          { return m_csDictFileName;}
    CIMSAString GetSpecFileName() const                          { return m_PifFile.GetAppFName();}
    const LogicSettings& GetLogicSettings() const                { return m_logicSettings; }

//  BOOL AddItemtoList(CString name, int add_rem, int occ = 0);
    bool IsChecked(int position) const;
    CString GetNameat(int level, int record, int item, int vset);

// Operations
public:

// Overrides
    // ClassWizard generated virtual function overrides
    //{{AFX_VIRTUAL(CExportDoc)
    public:
    virtual BOOL OnNewDocument();
    virtual BOOL OnOpenDocument(LPCTSTR lpszPathName);
    virtual void OnCloseDocument();
    protected:
    virtual BOOL SaveModified();
    //}}AFX_VIRTUAL

// Implementation
public:
    CString GetRecordItemStr();
    int GetNumItemsSelected();
    int GetPositionInList(wstring_view name_sv, int occurrence) const;
    int GetPositionInList(int relation_index, wstring_view name_sv, int occurrence) const;
    void AddAllItems();
    void SetItemCheck(int i, bool sel) { m_aItems[i].selected = sel;}
    int GetItemOcc (int i)
    {
        if(i>=0)    return m_aItems[i].occ;
        else return -2;
    }
    bool LaunchBatchApp();
    bool WriteDefaultFiles(Application* pApplication,const CString& sAppFName);
    bool GenerateBatchApp();
    bool m_bmerge;
    METHOD m_convmethod;
    int m_iPreviousRunConvMethod;
    bool RunBatch();
    CStringArray m_records;
    CIMSAString m_csSPSSOutFile;
    CIMSAString m_csSPSSDescFile;
    CIMSAString m_csCSProDCFFile;
    CIMSAString m_csSASDescFile;
    CIMSAString m_csSTATADescFile;
    CIMSAString m_csSTATALabelFile;
    CIMSAString m_csRDescFile;
    CStringArray m_rectypes;
    int GetNumExpRecTypes();
    bool OpenDictFile(const TCHAR* filename, bool silent);
    bool GenerateApplogic(CSpecFile& appFile);
    bool GenerateBatchApp4MultiModel();
    bool WriteDefaultFiles4MultiModel(Application* pApplication, const CString &sAppFName);
    CString GenerateInclude(CString& sRecString);
    CString GetFileName4Rec(const CDictRecord* pRecord);
    void DoPostRunSave();
    void DoPostRunCleanUp();
    bool DeleteOutPutFiles();
    bool IsRecordSingle(const CString& sRecTypeVal);

    virtual ~CExportDoc();
#ifdef _DEBUG
    virtual void AssertValid() const;
    virtual void Dump(CDumpContext& dc) const;
#endif

    void    SyncBuff_app();

    //just one call to know everything : retrieve the same info, sorted by many ways
    int     GetSelectedItems(   CArray<const CDictItem*,const CDictItem*>*                                                                  paSelItems                              = NULL,
                                CMap<const CDictItem*,const CDictItem*,CArray<int,int>*,CArray<int,int>*>*                                  pMapSelOccsByItem                       = NULL,
                                CMap<const CDictItem*,const CDictItem*,int,int>*                                                            pMapItems                               = NULL,
                                CMap<int,int,CArray<const CDictItem*,const CDictItem*>*,CArray<const CDictItem*,const CDictItem*>*>*                    paMap_SelectedIdItems_by_LevelIdx       = NULL,
                                CMap<int,int,CArray<const CDictItem*,const CDictItem*>*,CArray<const CDictItem*,const CDictItem*>*>*                    paMap_SelectedNonIdItems_by_LevelIdx    = NULL,
                                CMap<const CDictItem*,const CDictItem*,const CDictItem*,const CDictItem*>*                                              pMapDictIdItems                         = NULL,
                                CMap<const CDictRecord*,const CDictRecord*,int,int>*                                                        pMapLevelIdxByRecord                    = NULL,
                                CMap<const CDictRecord*,const CDictRecord*,bool,bool>*                                                      pMapIsIdBySelRec                        = NULL,
                                CMap<const CDictRecord*,const CDictRecord*,CArray<const CDictItem*,const CDictItem*>*,CArray<const CDictItem*,const CDictItem*>*>*  pMapSelItemsByRecord                    = NULL,
                                CArray<const CDictRecord*,const CDictRecord*>*                                                              paSelIdRecords                          = NULL,
                                CArray<const CDictRecord*,const CDictRecord*>*                                                              paSelNonIdRecords                       = NULL,
                                CArray<const CDictRecord*,const CDictRecord*>*                                                              paSelRecords                            = NULL,
                                CArray<const DictRelation*, const DictRelation*>*                                               paSelRelations                          = NULL);
    CString ExportCmd(  CString&            rcsExportProc,
                    const CString&      rcsUniverse,
                    const CString&      rcsExportFileVar,
                    const CString&      rcsCaseIdItems,
                    const CString&      rcsExportList,
                    int&                tabs,
                    CMapStringToString& rMapUsedFiles,
                    ExportRecordType    exportRecordType,
                    const CString*      pcsRecType);

// Generated message map functions
protected:
    //{{AFX_MSG(CExportDoc)
    afx_msg void OnFileRun();
    afx_msg void OnUpdateFileRun(CCmdUI* pCmdUI);
    afx_msg void OnFileSave();
    afx_msg void OnFileSaveAs();
    afx_msg void OnUpdateFileSave(CCmdUI* pCmdUI);
    afx_msg void OnOptionsExcluded();
    afx_msg void OnUpdateOptionsExcluded(CCmdUI* pCmdUI);
    afx_msg void OnOptionsLogicSettings();    
    afx_msg void OnViewBatchLogic();
    //}}AFX_MSG
    DECLARE_MESSAGE_MAP()

private:
    CString GetItemData(TCHAR* record, CDictItem* pItem);

    void SaveBatchPffAdjustingOnExit(CNPifFile* pPifFile);

public:
    //Some banal functions
    void CheckUniqueName( CString& rcsName );

    void PROC_LEVEL(const DictLevel& dict_level);
    void PreProc(CString* pcsProcLevel);
    void PostProc(CString* pcsProcLevel);
    void SetBehavior(CString* pcsBuff);
    void DeclareNumericVars(CMapStringToString& aMapUsedNumericVars);
    void DeclareFileVars( CArray<CString,CString>& aExportFileVars, CMapStringToString& rMapUsedFiles );
    void Append( const CString& rcsLines, CString* pcsAppBuff = NULL  );
    void CheckFileExtension();

    void Checks();

    //Relations support
    bool FillRecTypeByRelation( /*input*/CMapStringToString& rMapUsedRecTypes, /*output*/CMap<const DictRelation*, const DictRelation*,CString,LPCTSTR>& rMapRecTypeByRelation );

    void Relation(  const DictRelation&                                 dict_relation,
                    const CString&                                      rcsExportFileVar,
                    const CString&                                      rcsCaseIdItems,
                    const CString&                                      rcsRecType,
                    CMap<const CDictItem*,const CDictItem*,const CDictItem*,const CDictItem*>&  rMapDictIdItems,
                    CMapStringToString&                                 rMapUsedVars,
                    CMapStringToString&                                 rMapUsedFiles,
                    CString&                                            rcsExportProc,
                    int                                                 tabs,
                    CString*                                            pcsSingleItemsPrefix);

    int GetHighestSelectedLevel(const DictRelation& dict_relation, CMap<const CDictRecord*,const CDictRecord*,int,int>& rMapLevelIdxByRecord );
    int GetHighestLevel(CArray<const CDictItem*,const CDictItem*>& raItems, CMap<const CDictRecord*,const CDictRecord*,int,int>& rMapLevelIdxByRecord );
    int GetHighestLevel(CMap<int,int,CArray<const CDictItem*,const CDictItem*>*,CArray<const CDictItem*,const CDictItem*>*>& aMap_SelectedIdItems_by_LevelIdx,
                        CMap<int,int,CArray<const CDictItem*,const CDictItem*>*,CArray<const CDictItem*,const CDictItem*>*>& aMap_SelectedNonIdItems_by_LevelIdx );
    int GetHighestLevel( CMap<int,int,CArray<const CDictItem*,const CDictItem*>*,CArray<const CDictItem*,const CDictItem*>*>& aMap_Items_by_LevelIdx );



    int FillExportFiles(/*input*/ const CDataDict* pDataDict, CMap<const CDictRecord*,const CDictRecord*,bool,bool>& raMapIsIdBySelRec, CArray<const CDictItem*,const CDictItem*>& aSelItems, CArray<const CDictRecord*,const CDictRecord*>& aSelRecords, CArray<const DictRelation*,const DictRelation*>& aSelRelations, bool bSingleFile, CString* pcsExportFileName, CString* pcsExportFilesFolder, CString* pcsExportFilesPrefix, METHOD convmethod,
                        /*output*/
                        CArray<CString,CString>& aExportFiles, CArray<CString,CString>& raExportFileVars,
                        CMap<const CDictRecord*,const CDictRecord*,CString,LPCTSTR>& raMapExportFileVarByRecord,
                        CMap<const DictRelation*,const DictRelation*,CString,LPCTSTR>& raMapExportFileVarByRelation );

    void FillProcs( /*input*/
                    const CDataDict*                                                                pDataDict,
                    bool                                                                            bSingleFile,
                    CMap<int,int,CArray<const CDictItem*,const CDictItem*>*,CArray<const CDictItem*,const CDictItem*>*>&    aMap_SelectedIdItems_by_LevelIdx,
                    CMap<int,int,CArray<const CDictItem*,const CDictItem*>*,CArray<const CDictItem*,const CDictItem*>*>&    aMap_SelectedNonIdItems_by_LevelIdx,
                    CArray<const DictRelation*,const DictRelation*>&                                raSelectedRelations,
                    CMap<const CDictRecord*,const CDictRecord*,int,int>&                                        rMapLevelIdxByRecord,
                    CArray<const CDictItem*,const CDictItem*>&                                                  raSingleItemsPrefix,

                    /*output*/
                    CArray<int,int>&                                                                aLevelsWithExportProc );


    void FillMapExportFilesByProc(//input
                                  bool                                                                          bSingleFile,
                                  CArray<int,int>&                                                              aLevelsWithExportProc,
                                  CArray<CString,CString>&                                                      aExportFileVars,
                                  CMap<int,int,CArray<const CDictItem*,const CDictItem*>*,CArray<const CDictItem*,const CDictItem*>*>&  aMap_SelectedIdItems_by_LevelIdx,
                                  CMap<int,int,CArray<const CDictItem*,const CDictItem*>*,CArray<const CDictItem*,const CDictItem*>*>&  aMap_SelectedNonIdItems_by_LevelIdx,
                                  CMap<const CDictRecord*,const CDictRecord*,CString,LPCTSTR>&                              aMapExportFileVarByRecord,

                                  //RELATIONS
                                  CMap<const DictRelation*,const DictRelation*,CString,LPCTSTR>&                aMapExportFileVarByRelation,
                                  CArray<const DictRelation*,const DictRelation*>&                              aSelRelations,
                                  CMap<const CDictRecord*,const CDictRecord*,int,int>&                                      rMapLevelIdxByRecord,

                                  //output
                                  CMap<int,int,CStringArray*,CStringArray*>&                                    aMapExportFileVarsByProc);



    CString GetExportProc(  int                                                                                             iLevelIdx,
                            CStringArray*                                                                                   paExportFileVars,
                            CMap<CString,LPCTSTR,CArray<const CDictRecord*,const CDictRecord*>*,CArray<const CDictRecord*,const CDictRecord*>*>&    rMapSelRecsByFile,
                            CMap<const CDictRecord*,const CDictRecord*,CArray<const CDictItem*,const CDictItem*>*,CArray<const CDictItem*,const CDictItem*>*>&  rMapSelItemsByRecord,
                                        CMap<const CDictItem*,const CDictItem*,CArray<int,int>*,CArray<int,int>*>&                      rMapSelOccsByItem,
                            CArray<CString,CString>&                                                                        raCaseIdsByLevelIdx,
                            CMap<const CDictItem*,const CDictItem*,const CDictItem*,const CDictItem*>&                                              rMapDictIdItems,
                            bool                                                                                            bSingleFile,
                            bool                                                                                            bAllInOneRecord,
                            ExportRecordType                                                                                exportRecordType,
                            bool                                                                                            bJoinSingleMultiple,
                            const CString&                                                                                  rcsUniverse,
                            int                                                                                             tabs,
                            CMapStringToString&                                                                             rMapUsedVars,
                            CMapStringToString&                                                                             rMapUsedFiles,
                            CArray<const CDictItem*,const CDictItem*>&                                                                  aSingleItemsPrefix,
                            CMap<CString,LPCTSTR,CArray<const CDictRecord*,const CDictRecord*>*,CArray<const CDictRecord*,const CDictRecord*>*>&    aMapRecordsToUpLevel,
                            int                                                                                             iPrefixLevel,

                            //RELATIONS
                            CMap<CString,LPCTSTR,CArray<const DictRelation*,const DictRelation*>*,CArray<const DictRelation*,const DictRelation*>*>& rMapSelRelsByFileVar,
                            CMap<const DictRelation*,const DictRelation*,CString,LPCTSTR>&                                                           rMapRecTypeByRelation,
                            CMap<const CDictRecord*,const CDictRecord*,int,int>&                                                                                 rMapLevelIdxByRecord,
                            //RELATIONS

                            CMap<const CDictRecord*,const CDictRecord*,const CDictRecord*,const CDictRecord*>& rMapJoinHelper);


    int PreScanSingleItemsPrefix(   /*input*/CMap<const CDictItem*,const CDictItem*,const CDictItem*,const CDictItem*>& rMapDictIdItems, CMap<const CDictRecord*,const CDictRecord*,int,int>& rMapLevelIdxByRecord,
                                         /*output*/CArray<const CDictItem*,const CDictItem*>& aSingleItemsPrefix );

    bool IsSelectedAnyMultiple(bool bIncludeRelations );
    bool IsSelectedAnySingle(bool bIncludeRelations );


    void    SetOptionsView  ( CExportOptionsView*   pOptionsView    );
    void    SetTreeView     ( CExportView*          pTreeView       );

    CExportView* GetTreeView() {return m_pTreeView;}
    //SAVY --Old style pifDlg for multiple files
    void ProcessRun();
    void MFilesModel2();
    bool ExecuteFileInfo2();
    bool GenerateBatchApp4MultiModel2();
    bool WriteDefaultFiles4MultiModel2(Application* pApplication, const CString &sAppFName);
    void SingleFileModel();
    void SingleFileModel4NoDataDef();
    bool CompileApp();

    void SetJoin( bool bJoin );
    bool IsFlatExport( const CDictRecord* pDictRecord, bool* pbHasAnyMultipleItem );
    void FillExportList(//input
                                CArray<const CDictItem*,const CDictItem*>& aSelItems,
                                CMap<const CDictItem*,const CDictItem*,CArray<int,int>*,CArray<int,int>*>& rMapSelOccsByItem,
                                CMap<const CDictItem*,const CDictItem*,const CDictItem*,const CDictItem*>& rMapDictIdItems,
                                bool bReplaceItemNamesByRecordName,
                                bool bIncludeMultipleItems,
                                bool bIncludeSingleItemsWithMultipleParents,
                                const CDictRecord* pExcludedRec,
                                CString*     pcsRecordLoopIdx,
                                CString*     pm_csMultItemLoopIdx,
                                bool         bFlatExport,


                                //output
                                CString& rcsExportList);

    void FillExportList(//input
                                CArray<const CDictRecord*,const CDictRecord*>* paSelRecs,
                                CMap<const CDictRecord*,const CDictRecord*,CArray<const CDictItem*,const CDictItem*>*,CArray<const CDictItem*,const CDictItem*>*>&  rMapSelItemsByRecord,
                                CMap<const CDictItem*,const CDictItem*,CArray<int,int>*,CArray<int,int>*>& rMapSelOccsByItem,
                                CMap<const CDictItem*,const CDictItem*,const CDictItem*,const CDictItem*>& rMapDictIdItems,
                                bool bReplaceItemNamesByRecordName,
                                bool bIncludeMultipleItems,
                                bool bIncludeSingleItemsWithMultipleParent,
                                const CDictRecord* pExcludedRec,
                                CString*     pcsRecordLoopIdx,
                                CString*     pm_csMultItemLoopIdx,
                                bool         bFlatExport,

                                //output
                                CString& rcsExportList);

    void FillExportList(//input
                    const CDictRecord* pSelRec,
                    CMap<const CDictRecord*,const CDictRecord*,CArray<const CDictItem*,const CDictItem*>*,CArray<const CDictItem*,const CDictItem*>*>&  rMapSelItemsByRecord,
                    CMap<const CDictItem*,const CDictItem*,CArray<int,int>*,CArray<int,int>*>&  rMapSelOccsByItem,
                    CMap<const CDictItem*,const CDictItem*,const CDictItem*,const CDictItem*>& rMapDictIdItems,
                    bool bReplaceItemNamesByRecordName,
                    bool bIncludeMultipleItems,
                    bool bIncludeSingleItemsWithMultipleParent,
                    const CDictRecord* pExcludedRec,
                    CString*     pcsRecordLoopIdx,
                    CString*     pm_csMultItemLoopIdx,
                    bool         bFlatExport,

                    //output
                    CString& rcsExportList);

    CString GetExportList( CArray<const CDictItem*,const CDictItem*>& aSelItems, CMap<const CDictItem*,const CDictItem*,CArray<int,int>*,CArray<int,int>*>& rMapSelOccsByItem, CMap<const CDictItem*,const CDictItem*,const CDictItem*,const CDictItem*>& rMapDictIdItems, bool bReplaceItemNamesByRecordName, bool bIncludeMultipleItems, bool bIncludeSingleItemsWithMultipleParent,
                           const CDictRecord* pExcludedRec, CString* pcsRecordLoopIdx, CString* pm_csMultItemLoopIdx, bool bFlatExport );

    bool    CheckSpecialFilter00();

    bool    WantFlatExport();
    bool    CanEnableJoin(bool bAllInOneRecord);
    void    UpdateOptionsPane();

    void    SaveSettingsToRegistry(); // 20130703

    int     m_tabs;
    CString m_csExportApp;

    CArray<CString,CString> m_aExportFiles;
    CArray<CString,CString> m_aExportFileVars;

    CMapStringToString  m_aMapUsedFiles;

    //m_aExportFiles.GetSize() can be > than m_aMapUsedFiles.GetCount()

    CString m_csExportFileName;
    CString m_csExportFilesFolder;
    CString m_csExportFilesPrefix;
    CString m_csUniverse;

    CExportView*        m_pTreeView;
    CExportOptionsView* m_pOptionsView;

    bool    m_bJOIN_SingleMultiple_UseSingleAfterMultiple;

    CMapStringToString  m_aMapUsedUniqueNames;
    CString             m_csRecLoopIdx;
    CString             m_csMultItemLoopIdx;
    CString             m_csMultSubItemLoopIdx;

    CString GetDocumentWindowTitle() const;
    CString GetDirectoryForOutputs() const;
    bool GetInputDataFilenames();
    bool CheckInInputOutputFilenamesAreDifferent() const;

private:
    bool ProcessDictionarySource(wstring_view filename_sv);
    CString GetDictionarySourceFilename() const;

    bool OpenSpecFile(const TCHAR* filename, bool silent);
    void SaveSpecFile() const;

    static std::wstring ConvertPre80SpecFile(NullTerminatedString filename);
};

/////////////////////////////////////////////////////////////////////////////

//{{AFX_INSERT_LOCATION}}
// Microsoft Visual C++ will insert additional declarations immediately before the previous line.
