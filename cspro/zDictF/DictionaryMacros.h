#pragma once

#include <zCaseO/Case.h>
#include <zCaseO/CaseAccess.h>

struct CaseIteratorRoutine;
class ConnectionString;


// DictionaryMacrosDlg dialog: started 20101108

class DictionaryMacrosDlg : public CDialog
{
public:
    DictionaryMacrosDlg(CDDDoc* pDDDoc, CWnd* pParent = nullptr);

protected:
    DECLARE_MESSAGE_MAP()

    void DoDataExchange(CDataExchange* pDX) override;

    void OnBnClickedDeleteValueSets();
    void OnBnClickedRequireRecordsYes();
    void OnBnClickedRequireRecordsNo();
    void OnBnClickedCopyDictionaryNames();
    void OnBnClickedPasteDictionaryNames();
    void OnBnClickedCopyValueSets();
    void OnBnClickedPasteValueSets();
    void OnBnClickedGenerateDataFile();
    void OnBnClickedCreateSample();
    void OnBnClickedAddItemsToRecord();
    void OnBnClickedCompactDataFile();
    void OnBnClickedSortDataFile();
    void OnBnClickedCreateNotesDictionary();

private:
    void SetRequireRecords(bool required);

    int readEntry(CString text,int startPos,CString& readWord);
    void makeNewNameWork(DictNamedBase& dict_element, const CString& oldName);

    CString GetLabelWithLanguages(const LabelSet& label, bool copy_all_languages) const;
    void SetLabelWithLanguages(LabelSet& label, const CStringArray& csaLabels, bool paste_all_languages);
    int readLabelsEntry(CString text,int startPos,CStringArray& csaLabels,bool paste_all_languages, bool& bSuccessfulRead);

    CString makeValueValid(CString value,CDictItem* pItem,int & numValsModified);

    static CString GetTempDataFileName(CString csFilename);
    static ConnectionString GetTempDataFileConnectionString(const ConnectionString& connection_string);

    std::unique_ptr<CaseAccess> CreateCaseAccess();
    void RunCaseIteratorRoutine(const CaseIteratorRoutine& case_iterator_routine, const char* action_verb);
    void RunCompactSortDataFile(bool compact_data);

    void AddRandomValue(const CaseItem& case_item, CaseItemIndex& index);
    void AddRandomBinaryValue(const CaseItem& case_item, CaseItemIndex& index);
    double GenerateRandomNumeric(const CDictItem& dict_item);
    CString GenerateRandomAlpha(const CDictItem& dict_item);
    int CountAlphaValues(const DictValueSet& dict_value_set);

private:
    CDDDoc* m_pDictDoc;
    CDataDict* m_pDict;

    // variables for the random file generation
    int notapplPercent;
    int invalidPercent;
    int regularPercent;
    std::map<const DictValueSet*, int> alphaValueSetValueCounts;
};
