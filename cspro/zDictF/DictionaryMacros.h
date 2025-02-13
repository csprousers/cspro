#pragma once

#include <zCaseO/Case.h>
#include <zCaseO/CaseAccess.h>

struct CaseIteratorRoutine;


// CDictionaryMacros dialog: started 20101108

class CDictionaryMacros : public CDialog
{
    DECLARE_DYNAMIC(CDictionaryMacros)

public:
    CDictionaryMacros(CDDDoc* pDDDoc,CWnd* pParent = NULL);   // standard constructor

// Dialog Data
    enum { IDD = IDD_DICTIONARY_MACROS };

protected:
    virtual void DoDataExchange(CDataExchange* pDX);    // DDX/DDV support

    DECLARE_MESSAGE_MAP()
public:
    void SetRequireRecords(bool required);

    int readEntry(CString text,int startPos,CString& readWord);
    void makeNewNameWork(DictNamedBase& dict_element, const CString& oldName);

    CString GetLabelWithLanguages(const LabelSet& label, bool copy_all_languages) const;
    void SetLabelWithLanguages(LabelSet& label, const CStringArray& csaLabels, bool paste_all_languages);
    int readLabelsEntry(CString text,int startPos,CStringArray& csaLabels,bool paste_all_languages, bool& bSuccessfulRead);

    CString makeValueValid(CString value,CDictItem* pItem,int & numValsModified);

    afx_msg void OnBnClickedDeleteValueSets();
    afx_msg void OnBnClickedRequireRecordsYes();
    afx_msg void OnBnClickedRequireRecordsNo();
    afx_msg void OnBnClickedCopyDictionaryNames();
    afx_msg void OnBnClickedPasteDictionaryNames();
    afx_msg void OnBnClickedCopyValueSets();
    afx_msg void OnBnClickedPasteValueSets();
    afx_msg void OnBnClickedGenerateDataFile();
    afx_msg void OnBnClickedCreateSample();
    afx_msg void OnBnClickedAddItemsToRecord();
    afx_msg void OnBnClickedCompactDataFile();
    afx_msg void OnBnClickedSortDataFile();
    afx_msg void OnBnClickedCreateNotesDictionary();

private:
    std::unique_ptr<CaseAccess> CreateCaseAccess();
    void RunCaseIteratorRoutine(const CaseIteratorRoutine& case_iterator_routine, const TCHAR* action_verb);
    void RunCompactSortDataFile(bool compact_data);

    void AddRandomValue(const CaseItem& case_item, CaseItemIndex& index);
    void AddRandomBinaryValue(const CaseItem& case_item, CaseItemIndex& index);
    double GenerateRandomNumeric(const CDictItem& dictionary_item);
    CString GenerateRandomAlpha(const CDictItem& dictionary_item);
    int CountAlphaValues(const DictValueSet& dict_value_set);

private:
    CDDDoc* m_pDictDoc;
    CDataDict* m_pDict;

    // variables for the random file generation
    int notapplPercent,invalidPercent,regularPercent;
    std::map<const DictValueSet*, int> alphaValueSetValueCounts;
};
