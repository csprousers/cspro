#pragma once

#include <zFormO/zFormO.h>
#include <zFormO/FormBase.h>
#include <zFormO/Group.h>


/***************************************************************************
*
*                       CDELevel : public CDEFormBase
*
***************************************************************************/

class CLASS_DECL_ZFORMO CDELevel : public CDEFormBase
{
    DECLARE_DYNAMIC(CDELevel)

public:
    CDELevel();
    CDELevel(const CDELevel& oldLevel);  // copy constructor
    ~CDELevel();

    eItemType GetFormItemType() const override { return Level; }

    const CDEGroup* GetRoot         ()  const       { return m_pRoot; }
    CDEGroup*       GetRoot         ()              { return m_pRoot; }
    CDEGroup*       GetGroup        (int i) const   { return (CDEGroup*) m_pRoot->GetItem(i);}
    int             GetNumGroups    ()  const       { return m_pRoot->GetNumItems(); }        /// smg: use? cycle thru items and give a count of items that are groups?

    CDEGroup*       GetGroupOnForm  (int iFormNo)   { return m_pRoot->GetGroupOnForm (iFormNo); }

    CDEItemBase*    GetItem         (int i) const   { return m_pRoot->GetItem(i);}
    int             GetNumItems     ()  const       { return m_pRoot->GetNumItems();}

    int             GetHierarchy    () const        { return m_iNesting; }
    void            SetHierarchy    (int iIndex)    { m_iNesting = iIndex ; }

    int             GetTotalOccs    () const        { return m_iTotalOccs; }
    int             GetDataOccs     () const        { return m_iDataOccs; }// RHF Mar 29, 2000

    void            SetTotalOccs    (int iTotal)    { m_iTotalOccs = iTotal ; }
    void            SetDataOccs     (int i)         { m_iDataOccs = i; } // RHF Mar 29, 2000

    int             GetCurOccurrence() const        { return m_iCurOccurrence; }
    void            SetCurOccurrence(int iOccur)    { m_iCurOccurrence = iOccur ; }

    int             GetMaxLoopOccs () const         { return m_iMaxLoopOccs; }
    void            SetMaxLoopOccs (int iMax)       {  m_iMaxLoopOccs = iMax; }

    CDEGroup* FindGroup (const CString& sGroupSearchName);

    void AddItem            (CDEItemBase* pItem)          { m_pRoot->AddItem (pItem); }
    void InsertItemAt       (CDEItemBase* pItem, int ndx) { m_pRoot->InsertItemAt(pItem,ndx); }

    bool RemoveItem         (const CString& sItemName)    { return m_pRoot->RemoveItem(sItemName); }

    void RemoveItem         (int i) { m_pRoot->RemoveItem(i); }
    void RemoveGroup        (int i) { m_pRoot->RemoveItem(i); }
    void RemoveRoster       (CDERoster* pRoster, CDEFormFile* pFF);

    void RemoveAllItems ()                                { m_pRoot->RemoveAllItems(); }
    bool Compare(CDELevel* pLevel);

    void AddGroup           (CDEGroup* pGroup)            { m_pRoot->AddItem (pGroup); }


    // serialization
    // --------------------------------------------------
    bool Build(CSpecFile& frmFile, CDEFormFile* pFF);
    void Save(CSpecFile& frmFile) const override;

    void serialize(Serializer& ar);


private:
    CDEGroup* m_pRoot;
    int m_iTotalOccs;
    int m_iCurOccurrence;
    int m_iMaxLoopOccs; // max # of times to loop
    int m_iNesting;     // bruce says base Level is 1, not 0
    int m_iDataOccs;    // RHF Mar 29, 2000; @RT only; In Modify start with Occurrences read from file.
};
