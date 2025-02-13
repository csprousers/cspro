#pragma once

#include <zformf/FormDoc.h>

class DictTreeNode;

enum eDropType { DropOpUndef,
                 DropFile,
                 DropLevel,

                 DropSingleRec,
                 DropMultipleRec,

                 DropSI_FromSR,     // SI=single item, SR = single record
                 DropSI_FromMR,     // MR = multiple record

                 DropMI_FromSR,     // MI = multiply-occurring item
                 DropMI_FromMR };

enum eDropResult { DropResultUndef,
                   DropAsKeyed, DropAsMirror,
                   DropAsRoster, DropUnrostered,
                   DropOnRoster } ; // drop it on a roster!


class CFormDropRules : public CObject
{
private:

    bool        m_bIllegalDrop;
    CString     m_sErrMsg;

    bool        m_bQuery4Roster;
    bool        m_bDropSubitems;    // use CDEDragOptions

    eDropType   m_eDropOp;  // the type of attempted drop operation
    eDropResult m_eDropResult;

    CString     m_sDropName;    // the name of the dropped item/rec/file
    CString     m_sDropLabel;   // the name of the dropped item

    int         m_iLevel, m_iRec, m_iItem,
                m_iNumOccs; // see note in AnalyzeDrop twds the func's end to get an idea of this

    CFormDoc*   m_pFormDoc; // copy the form doc passed in to here, so i don't have to keep passing around in the funcs!

public:

    void ResetFlags();

    bool        IllegalDropOcc  () const { return m_bIllegalDrop; }

    const CString& GetErrorMsg  () const { return m_sErrMsg; }

    int         GetNumOccs      () const { return m_iNumOccs; }
    void        SetNumOccs      (int i)  { m_iNumOccs = i; }

    bool    QueryForRoster      () const { return m_bQuery4Roster; }

    bool    CanUserDropSubitems (const CDataDict* pDD, const CDictItem* pDI);
    void    CanUserDropSubitems (bool b) { m_bDropSubitems = b; }
    bool    CanUserDropSubitems ()       { return m_bDropSubitems; }

    eDropType   GetDropOp       () const { return m_eDropOp; }
    eDropResult GetDropResult   () const { return m_eDropResult; }

    const CString& GetDropName  () const { return m_sDropName; }
    const CString& GetDropLabel () const { return m_sDropLabel; }

    void DetermineDropNameNLabel(const DictTreeNode* dict_tree_node, const CDictItem& dict_item);

    bool AnalyzeDrop(DictTreeNode* dict_tree_node, CPoint dropPoint, CDEForm* pForm, CFormDoc* pDoc);

    bool DragSingleRecord(DictTreeNode* , CDEForm* );
    bool DragMultipleRecord(CPoint , CDEForm* );

    bool DragSingleItemFromSingleRecord(DictTreeNode* , CDEForm* );
    bool DragSingleItemFromMultipleRecord(DictTreeNode* , CPoint , CDEForm* );

    bool DragMultipleItemFromSingleRecord(DictTreeNode* , CPoint , CDEForm* );
    bool DragMultipleItemFromMultipleRecord(DictTreeNode* , CDEForm* );

    bool CheckItemVsSubItemFromSingle(const CDataDict* pDD, const CDictItem* pDI);
    bool CheckItemVsSubItemFromMultiple(const CDataDict* pDD, const CDictItem* pDI);

    bool CheckForSubitemOverlap(const CDataDict* pDD, const CDictItem* pDI);
};
