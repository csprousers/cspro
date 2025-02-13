#pragma once

#include <zDictF/zDictF.h>
#include <zUtilO/UndoStack.h>
#include <zDictO/DictionaryValidator.h>
#include <zDesignerF/DictionaryBasedDoc.h>

class CDDTreeCtrl;
class CUGCtrl;
class DictClipboard;


/////////////////////////////////////////////////////////////////////////////
//
//                               CDDUndoObject
//
/////////////////////////////////////////////////////////////////////////////

struct DDUndoObject
{
    DDUndoObject(std::unique_ptr<DictNamedBase> dict_element,
                 int iCurLevel = NONE, int iCurRec = NONE, int iCurItem = NONE, int iCurVSet = NONE, int iCurRow = NONE)
        :   m_pUndoObject(std::move(dict_element)),
            m_iCurLevel(iCurLevel),
            m_iCurRec(iCurRec),
            m_iCurItem(iCurItem),
            m_iCurVSet(iCurVSet),
            m_iCurRow(iCurRow)
    {
        ASSERT(m_pUndoObject != nullptr);
    }

    // could be CDataDict, DictLevel, CDictRecord, or CDictItem
    std::unique_ptr<DictNamedBase> m_pUndoObject;

    int m_iCurLevel; // current level number when element was created
    int m_iCurRec;   // current record number ...
    int m_iCurItem;  // current item number ...
    int m_iCurVSet;  // current value set number ...
    int m_iCurRow;   // current value row number ...
};


/////////////////////////////////////////////////////////////////////////////
//
//                               CDDDoc
//
/////////////////////////////////////////////////////////////////////////////

class CLASS_DECL_ZDICTF CDDDoc : public DictionaryBasedDoc
{
    DECLARE_DYNCREATE(CDDDoc)

protected:
    CDDDoc(); // create from serialization only

public:
    ~CDDDoc();

    const CDataDict* GetDict() const                 { return m_dictionary.get(); }
    CDataDict* GetDict()                             { return m_dictionary.get(); }
    const CDataDict& GetDictionary() const           { return *m_dictionary; }
    CDataDict& GetDictionary()                       { return *m_dictionary; }
    std::shared_ptr<CDataDict> GetSharedDictionary() { return m_dictionary; }

    DictionaryValidator* GetDictionaryValidator() { return &m_dictionaryValidator; }

    UndoStack<DDUndoObject>& GetUndoStack()       { return m_undoStack; }

    CDDTreeCtrl* GetDictTreeCtrl() const         { return m_pTreeCtrl; }
    void SetDictTreeCtrl(CDDTreeCtrl* pTreeCtrl) { m_pTreeCtrl = pTreeCtrl; }

    CUGCtrl* GetGrid() const     { return m_pGrid; }
    void SetGrid(CUGCtrl* pGrid) { m_pGrid = pGrid; }

    void SetModified(bool bModified = true);

    void PushUndo(const DictNamedBase& dict_element, int iCurLevel = NONE, int iCurRec = NONE, int iCurItem = NONE, int iCurVSet = NONE, int iCurRow = NONE);

    void UndoChange(bool bCanRedo);
    void RedoChange();

    int GetLevel() const      { return m_iLevel; }
    void SetLevel(int iLevel) { m_iLevel = iLevel; }

    int GetRec() const        { return m_iRec; }
    void SetRec(int iRec)     { m_iRec = iRec; }

    int GetItem() const       { return m_iItem; }
    void SetItem(int iItem)   { m_iItem = iItem; }

    void SetPrintPreview(bool bPrintPreview) { m_bPrintPreview = bPrintPreview; }
    bool IsPrintPreview() const              { return m_bPrintPreview; }

    BOOL OnOpenDocument(LPCTSTR lpszPathName) override;
    BOOL OnSaveDocument(LPCTSTR lpszPathName) override;
    void DeleteContents() override;

    bool GetRelLinksList(const CString& csRelObj, CStringArray& csaValidRelations);
    std::vector<CString> GetPrimSecNames() const;
    bool InitTreeCtrl();
    bool IsDocOK(bool bSilent = false);

    const DictClipboard& GetDictClipboard() const { return *m_dictClipboard; }

    // DictionaryBasedDoc overrides
    CTreeCtrl* GetTreeCtrl() override;
    std::shared_ptr<const CDataDict> GetSharedDictionary() const override;
    bool IsEditing();

protected:
    DECLARE_MESSAGE_MAP()

    afx_msg void OnOptionsAbsRel();
    afx_msg void OnOptionsZeroFill();
    afx_msg void OnOptionsDecChar();
    afx_msg void OnEditLanguages();
    afx_msg void OnDictionaryMacros();
    afx_msg void OnEditSecurityOptions();
    afx_msg void OnEnableBinaryItems();

public:
    afx_msg void OnEditRelation();

private:
    void PushUndoCurrent();
    void PerformUndoChange(DDUndoObject undo_object);
    DDUndoObject DuplicateUndoObject(const DDUndoObject& undo_object);

private:
    std::shared_ptr<CDataDict> m_dictionary;    // Dictionary object

    DictionaryValidator m_dictionaryValidator;  // Dictionary rule manager

    UndoStack<DDUndoObject> m_undoStack;        // Dictionary undo stack

    CDDTreeCtrl* m_pTreeCtrl;                   // Dictionary tree control
    CUGCtrl* m_pGrid;                           // Dictionary grid control

    int m_iLevel;                               // Current level
    int m_iRec;                                 // Current record
    int m_iItem;                                // Current item

    bool m_bPrintPreview;                       // Is doing a Print Preview

    std::unique_ptr<DictClipboard> m_dictClipboard;
};
