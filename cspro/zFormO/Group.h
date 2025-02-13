#pragma once

#include <zFormO/zFormO.h>
#include <zFormO/ItemBase.h>

class CDataDict;
class CDEForm;
class CDEFormFile;
class CDERoster;
class CDictRecord;
struct DICT_LOOKUP_INFO;


/***************************************************************************
*
*                       CDEGroup : public CDEItemBase
*
***************************************************************************/

enum eReturnType        { OK, Reconciled, Delete };


class CLASS_DECL_ZFORMO CDEGroup : public CDEItemBase
{
    DECLARE_DYNAMIC (CDEGroup)

public:
    bool SearchRosterIntoForms(CDEFormFile* pFF, CDEForm* pForm, CDERoster* pRoster);
    bool SearchInAllForms(CDEFormFile* pFF, CDEField *pField);

    // construction/destruction
    CDEGroup ();
    CDEGroup (const CDEGroup& group);         // copy constructor
    ~CDEGroup ();

	std::unique_ptr<CDEItemBase> Clone() const override { return std::make_unique<CDEGroup>(*this); }

    eItemType GetFormItemType() const override { return Group; }

    void UpdatePointers (CDEGroup* parent); // used in conjunction w/CDEFormFile::StuffPointers()
	void RefreshAssociatedFieldText();

    // the next few funcs are to facilitate CDEFormFile's unique name list

    void BuildUniqueNL(CDEFormFile* pFF);

    // back to in-func stuff

    void FinishFormInit4Rosters(CDEForm* pForm, CDEGroup* pRoster);     // invoked from Build()
    bool FindNCopyFieldIntoForm(CDEForm* pForm, CDEField* pGroupField);

    eRIType     GetRIType              () const { return m_eRIType; }
    const TCHAR* GetRIStr              () const;
    const CString& GetTypeName         () const { return m_csTypeName; }
    const CString& GetRepeatName       () const { return m_csTypeName; }
    const CString& GetMaxField         () const { return m_csMaxField; }
    const CString& GetRecordRepeatName () const;
    const CString& GetFormName         () const { return m_sFormName; }

    int GetItemIndex                   (const CDEItemBase* pItem) const;
    int GetItemIndex                   (const CString& sName) const;


    CDEGroup*       GetGroupOnForm (int iFormNo);

    void SetFormName        (const CString& sName) { m_sFormName = sName; }
    void SetLoopingVars     (const CDictRecord* pDR);
    void SetLoopingVars ();

    bool GetRequired        ()	const					{ return m_bRequired; }
    int  GetMaxLoopOccs		()	const					{ return m_iMaxLoopOccs; }
    int  GetTotalOccs       ()	const					{ return m_iTotalOccs; }
    int  GetDataOccs        ()	const					{ return m_iDataOccs; }// RHF Mar 29, 2000

    void SetRequired        (bool val)				{ m_bRequired = val; }

    void SetRIType          (eRIType val)           { m_eRIType = val; }
    void SetRIType          (const CString& cs);
    void SetTypeName        (const CString& cs)     { m_csTypeName = cs; }
    void SetRepeatName      (const CString& cs)     { m_csTypeName = cs; }

    void SetMaxLoopOccs (const CString& cs);
    void SetMaxLoopOccs (int i)                     { m_iMaxLoopOccs = i; }

    void SetTotalOccs   (int i)                     { m_iTotalOccs = i; }
    void SetDataOccs    (int i)                     { m_iDataOccs = i; } // RHF Mar 29, 2000
    void SetMaxField    (const CString& cs)         { m_csMaxField = cs; }

    const CDictItem* GetMaxDEField() const      { return m_pMaxField; }
    void SetMaxDEField(const CDictItem* pField) { m_pMaxField = pField;}


    // methods for m_aItems //

    int GetNumItems          () const { return (int)m_aItem.size(); }
    int GetUpperBound        () const { return GetNumItems() - 1; }
    CDEItemBase* GetItem(int i) const { return m_aItem[i]; }

    void AddItem(CDEItemBase* pItem)          { m_aItem.emplace_back(pItem); }
    void SetItemAt(CDEItemBase* pItem, int i) { m_aItem[i] = pItem; }
    int  FindItem(const CString& sFFName) const;

    void InsertItemAt(CDEItemBase* pItem, int i) { m_aItem.insert(m_aItem.begin() + i, pItem); }

    void RemoveItemAt(int i) { m_aItem.erase(m_aItem.begin() + i); }
	bool RemoveItem(const CString& sItemName);
	bool RemoveField(CDEField* pField);
	virtual void RemoveItem(int iIndex);
	void RemoveAllItems();

    // misc

    void UpdateGroupFormIndices (int iStartingFormNo);

    int             RenumberFormsAndItems (CDEFormFile* pFF, int iCurFormNum); // a corollary to the above, but not quite

    void            SetCurOccurrence(int iOcc) { m_iCurOccurrence = iOcc; }
    int             GetCurOccurrence() const   { return m_iCurOccurrence; }

    // reconcile stuff!

    eReturnType Reconcile(CDEFormFile* pFormFile, CString& csErr, bool bSilent, bool bAutoFix);

    eReturnType ReconcileRecordOccs (CDEFormFile* pFormFile, CString& csErr, bool bSilent, bool bAutoFix,
                                     const DICT_LOOKUP_INFO& structLookup, bool* bFirst);
    eReturnType ReconcileItemOccs   (CDEFormFile* pFormFile, CString& csErr, bool bSilent, bool bAutoFix,
                                     const DICT_LOOKUP_INFO& structLookup, bool* bFirst);

    void SetupErrMsg(CString& csErr, CString& csMsg, CDEFormFile* pFormFile);

    //Order reconcile stuff SAVY 07/25
    eReturnType OReconcile(CDEFormFile* pFormFile, CString& csErr, bool bSilent, bool bAutoFix);

    void UpdateFlagsNFonts(const CDEFormFile& form_file);
    void ChangeDName(const CDataDict& dictionary);
    void ChangeGName(const CString& sOldName, const CString& sNewName);
    void ClearUsed();
    void CheckGroups (CDEFormFile* pFormFile);
    void SetItemSubItemFlags(CDEFormFile* pFormFile, const CDictItem* pDictItem, bool bFlag =true);

    bool Compare(CDEGroup* pGroup);

    void SetMaxFieldPointer (CDEFormFile* pFormFile);

	std::vector<CDEField*> GetBlockFields(const CDEBlock& form_block) const;
	const CDEBlock* GetBlock(CDEField* pField) const;
	CDEBlock* GetBlock(CDEField* pField);
	const CDEBlock* GetBlockForFieldAt(int iFieldIndex) const;
	CDEBlock* GetBlockForFieldAt(int iFieldIndex);


    // serialization
    // --------------------------------------------------
    bool Build(CSpecFile& frmFile, CDEFormFile* , bool bSilent = false, CDEForm* pForm = NULL);
    void Save(CSpecFile& frmFile) const override;

	void serialize(Serializer& ar);


protected:
    std::vector<CDEItemBase*> m_aItem; // will contain groups, rosters or keyed fields

private:
    // if the FormNum!=NONE, then signifies that the group is represented by a page

    bool m_bRequired; // is this group required??

    CString m_sFormName; // when user reorders forms, refer to unique name rather than index to reorder

    // m_iMaxLoopOccs driven by one of the following:
    //
    // - a fixed max (defined by a record or item)
    // - an item previously entered (some sibling group?)
    // - infinite

    int m_iMaxLoopOccs; // max # of times to loop

    // if the fixed max is from a record or item, then which?

    eRIType m_eRIType;    // is it a record, item or subitem that loops?
    CString m_csTypeName; // the record/item's name

    // if m_eRIType is an "item" or "subitem", use these two members

    const CDictItem* m_pMaxField; // or, ptr to the field that determines max # of repeats
    CString m_csMaxField;         // (this is the name of the field--yes? if so, y repeat?)

    // m_iTotalOccs can be related to another group (if the record is split among 2+ groups)

    int m_iTotalOccs;     // number of occs in current case (<= MaxLoopOccs)
    int m_iCurOccurrence; // index of current occ in current case
    int m_iDataOccs;      // RHF Mar 29, 2000    // In Modify start with Occurrences read from file
};
