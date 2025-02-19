#pragma once

#include <zAppO/LogicSettings.h>
#include <zFreqO/FrequencyPrinterOptions.h>
#include <zDataO/DictionarySource.h>
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

extern const char* SortTypeNames[];

enum class OutputFormat : int { Table = 0, HTML = 1, Json = 2, Text = 3, Excel = 4 };
extern const char* OutputFormatNames[];


class CSFreqDoc : public CDocument
{
    DECLARE_DYNCREATE(CSFreqDoc)

protected:
    // create from serialization only
    CSFreqDoc();

private:
    DictionarySource                 m_dictionarySource;
    std::shared_ptr<const CDataDict> m_dictionary;
    std::vector<FREQUENCIES>         m_freqnames;
    CNPifFile                        m_FreqPiff;
    std::string                      m_baseFilePath;
    LogicSettings                    m_logicSettings;
    std::unique_ptr<CNPifFile>       m_batchPff;

// Attributes
public:
    bool               m_bSaved;
    bool               m_batchmode;

    ItemSerialization  m_itemSerialization;
    bool               m_bUseVset;
    bool               m_bHasFreqStats; //From fqf file
    std::optional<int> m_percentiles;
    bool               m_sortOrderAscending;
    FrequencyPrinterOptions::SortType m_sortType;
    OutputFormat       m_outputFormat;
    std::string        m_universe;
    std::string        m_weight;

    // Operations
public:
    BOOL OnOpenDocument(LPCTSTR lpszPathName) override;

protected:
    BOOL SaveModified() override;

// Implementation
public:
    ~CSFreqDoc();

    void LaunchBatch();
    void RunBatch();

    void ClearAllTemps();
    void AddAllItems();
    int GetPositionInList(wstring_view name_sv, int occurrence, bool reverse_search = false);

    void SetItemCheck(int i, bool sel) { m_freqnames[i].selected = sel;}
    int GetItemOcc(int i) const        { return ( i >= 0 ) ? m_freqnames[i].occ : -2; }

    bool CheckValueSetChanges();

    std::shared_ptr<const CDataDict> GetSharedDictionary() const { return m_dictionary; }
    const CDataDict* GetDataDict() const                         { return m_dictionary.get(); }
    CIMSAString GetSpecFileName()                                { return GetFileName(m_FreqPiff.GetAppFName()); }
    CNPifFile* GetPifFile()                                      { return &m_FreqPiff;}
    const LogicSettings& GetLogicSettings() const                { return m_logicSettings; }

    bool IsChecked(int position) const;
    CString GetNameat(int level, int record,int item, int vset,int occ =0);
    bool GenerateBchForFrq();
    bool CompileApp(XTABSTMENT_TYPE eType= XTABSTMENT_ALL);
    void WriteDefaultFiles(Application* pApplication, const CString& sAppFName);
    bool IsAtLeastOneItemSelected() const;

    std::string GenerateFrqCmd();

    void DoPostRunCleanUp();
    bool ExecuteFileInfo();
    std::string GetDocumentWindowTitle() const;

private:
    bool GetSaveExcludedItems() const { return ( m_itemSerialization == ItemSerialization::Excluded ); }

    void ProcessDictionarySource(DictionarySource dictionary_source);

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

    bool OpenSpecFile(const std::string& spec_file_path, bool silent);
    void SaveSpecFile() const;

    static std::string ConvertPre80SpecFile(InterfaceString file_path);
};
