#pragma once

#include <zUtilO/TemporaryFile.h>
#include <zAppO/LogicSettings.h>
#include <zFreqO/FrequencyPrinterOptions.h>
#include <zTableO/Table.h>


// Table structure
struct FREQUENCIES
{
    CString freqnames;
    int occ;
    bool selected;
    bool bStats;
    bool bNTiles;
    int iTiles;
};

enum class ItemSerialization : int { Included, Excluded };

extern const std::vector<const TCHAR*> SortTypeNames;

enum class OutputFormat : int { Table = 0, HTML = 1, Json = 2, Text = 3, Excel = 4 };
extern const std::vector<const TCHAR*> OutputFormatNames;


class CSFreqDoc : public CDocument
{
protected: // create from serialization only
    CSFreqDoc();
    DECLARE_DYNCREATE(CSFreqDoc)

private:
    CString                          m_csDictFileName;
    std::unique_ptr<TemporaryFile>   m_temporaryDataDictFile;

    std::shared_ptr<const CDataDict> m_pDataDict;
    std::vector<FREQUENCIES>         m_freqnames;
    CNPifFile                        m_FreqPiff;
    CIMSAString                      m_sBaseFilename;
    CString                          m_csDictionarySourceDataFilename;
    LogicSettings                    m_logicSettings;
    std::unique_ptr<CNPifFile>       m_batchPff;

// Attributes
public:
    CTime              m_tDCFTime;
    bool               m_bSaved;
    bool               m_batchmode;
                       
    ItemSerialization  m_itemSerialization;
    bool               m_bUseVset;
    bool               m_bHasFreqStats; //From fqf file
    std::optional<int> m_percentiles;
    bool               m_sortOrderAscending;
    FrequencyPrinterOptions::SortType m_sortType;
    OutputFormat       m_outputFormat;
    CString            m_sUniverse;
    CString            m_sWeight;

    // Operations
public:
    BOOL OnOpenDocument(LPCTSTR lpszPathName) override;

protected:
    BOOL SaveModified() override;

// Implementation
public:
    ~CSFreqDoc();
    bool OpenDictFile(bool silent);
    void CloseUponFileNonExistance();

    void LaunchBatch();
    void RunBatch();

    void ClearAllTemps();
    void AddAllItems();
    int GetPositionInList(wstring_view name, int occurrence, bool reverse_search = false);

    void SetItemCheck(int i, bool sel) { m_freqnames[i].selected = sel;}

    int GetItemOcc (int i)
    {
        if(i>=0)    return m_freqnames[i].occ;
        else return -2;
    }

    bool CheckValueSetChanges();

    std::shared_ptr<const CDataDict> GetSharedDictionary() const { return m_pDataDict; }
    const CDataDict* GetDataDict() const                         { return m_pDataDict.get(); }
    CIMSAString GetDictFileName()                                { return m_csDictFileName; }
    CIMSAString GetSpecFileName()                                { return GetFileName(m_FreqPiff.GetAppFName()); }
    CNPifFile* GetPifFile()                                      { return &m_FreqPiff;}
    const LogicSettings& GetLogicSettings() const                { return m_logicSettings; }

    bool IsChecked(int position) const;
    CString GetNameat(int level, int record,int item, int vset,int occ =0);
    bool GenerateBchForFrq();
    bool CompileApp(XTABSTMENT_TYPE eType= XTABSTMENT_ALL);
    void WriteDefaultFiles(Application* pApplication, const CString& sAppFName);
    bool IsAtLeastOneItemSelected() const;

    CString GenerateFrqCmd();

    void DoPostRunCleanUp();
    bool ExecuteFileInfo();
    CString GetDocumentWindowTitle() const;

private:
    bool GetSaveExcludedItems() const { return ( m_itemSerialization == ItemSerialization::Excluded ); }

    bool ProcessDictionarySource(const CString& filename);
    const CString& GetDictionarySourceFilename() const;

    void ResetFrequencyPff();
    void GenerateBatchPffFromFrequencyPff();

// Generated message map functions
protected:
    DECLARE_MESSAGE_MAP()

    afx_msg void OnFileRun();
    afx_msg void OnUpdateFileRun(CCmdUI* pCmdUI);
    afx_msg void OnFileSave();
    afx_msg void OnUpdateFileSave(CCmdUI* pCmdUI);
    afx_msg void OnFileSaveAs();
    afx_msg void OnUpdateFileSaveAs(CCmdUI* pCmdUI);
    afx_msg void OnToggle();
    afx_msg void OnUpdateToggle(CCmdUI* pCmdUI);
    afx_msg void OnOptionsExcluded();
    afx_msg void OnUpdateOptionsExcluded(CCmdUI* pCmdUI);
    afx_msg void OnOptionsLogicSettings();
    afx_msg void OnViewBatchLogic();

private:
    void ResetValuesToDefault();

    bool RemoveInvalidFrequencyEntries();

    bool OpenSpecFile(const TCHAR* filename, bool silent);
    void SaveSpecFile() const;

    static std::wstring ConvertPre80SpecFile(NullTerminatedString filename);
};
