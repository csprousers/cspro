//***************************************************************************
//  File name: DropRule.cpp
//
//  Description:
//       Form document implementation
//
//  History:    Date       Author   Comment
//              ---------------------------
//              15 Jul 99   smg     Created for MEASURE
//
//***************************************************************************

#include "StdAfx.h"
#include "DropRule.h"

// err msgs 01-09 are generic to all drops
// err msgs 10-19 are specific to a single item/record drop
// err msgs 20-29 are specific to a multiple item/record drop

#define ErrMsg01    _T("The dropped item does not have the same level as the current form.")

#define ErrMsg10    _T("A single record cannot be dropped on to a form that loops.")
#define ErrMsg11    _T("You cannot drop an unkeyed item from a single record onto a form that loops.")

#define ErrMsg20    _T("The record controlling the form looping is not the same record as the dropped item.")
#define ErrMsg21    _T("The record controlling the roster is not the same as the dropped item.")
#define ErrMsg22    _T("No unkeyed fields remain in the dropped record.")
#define ErrMsg232   _T("The looping controlling the form is not the same as the looping controlling the dropped item.")

#define ErrMsg23    _T("This item cannot be dropped as one (or more) of its subitems are being keyed.")
#define ErrMsg24    _T("This subitem cannot be dropped as its parent is being keyed.")
#define ErrMsg25    _T("This subitem cannot be dropped as it is overlapping with a keyed sibling subitem.")

#define ErrMsg30    _T("A multiply-occurring item cannot be dropped as a mirror field.")
#define ErrMsg31    _T("The item controlling the roster is not the same as the dropped item.")

#define ErrMsg50    _T("The item being dropped belongs to a higher level record than the current form is associated\nwith. Therefore, the item must be dropped as a display field. However, the item is not currently\nbeing keyed, and therefore, cannot be dropped as a display field until it is being keyed at the\nproper level.")

// ******************************************************************************

void CFormDropRules::ResetFlags()
{
    m_eDropOp = DropOpUndef;
    m_eDropResult = DropResultUndef;

    m_bIllegalDrop  = false;
    m_bQuery4Roster = false;
    m_bDropSubitems = true; // let it be true until determined otherwise

    m_sErrMsg = _T("");         // wipe out any prev err msgs
    m_sDropName = _T("");
    m_sDropLabel = _T("");

    m_iLevel = m_iRec = m_iItem = 0;
}


// ******************************************************************************

void CFormDropRules::DetermineDropNameNLabel(const DictTreeNode* dict_tree_node, const CDictItem& dict_item)
{
    ASSERT(dict_tree_node->GetDictElementType() == DictElementType::Item);
    m_sDropName = dict_item.GetName();
    m_sDropLabel = dict_item.GetLabel();
}


bool CFormDropRules::AnalyzeDrop(DictTreeNode* dict_tree_node, CPoint dropPoint, CDEForm* pForm, CFormDoc* pDoc)
{
    auto pDD = dict_tree_node->GetDDDoc()->GetSharedDictionary();

    if( pDoc->GetSharedDictionary() != pDD )  // user trying to drop from diff dict
    {
        m_bIllegalDrop = true;
        m_sErrMsg = FormatText(_T("A form file cannot use more than one dictionary. You are attempting to drop an item/record ")
                               _T("from a dictionary (%s) different from what is in use now (%s)."),
                               pDD->GetName().GetString(), pDoc->GetSharedDictionary()->GetName().GetString());
        return false;
    }

    ResetFlags();

    m_pFormDoc = pDoc;

    DictElementType dict_element_type = dict_tree_node->GetDictElementType();

    m_iLevel = dict_tree_node->GetLevelIndex();  // level is 0-based
    m_iRec   = dict_tree_node->GetRecordIndex(); // -2 signifies LevelID rec, otherwise begs w/0

    if (dict_element_type == DictElementType::Dictionary)         // invoke CFormDoc::GenerateFormFile ()
    {
        m_eDropOp = DropFile;
        m_sDropName = pDD->GetName();
        m_sDropLabel = pDD->GetLabel();

        return true;
    }
    if (dict_tree_node->GetRelationIndex() != NONE)          // no dragging from relations
    {
        return false;
    }
    else if (dict_element_type == DictElementType::Level)   // dragging a level, deal w/later
    {
        m_eDropOp = DropLevel;
        m_sDropName = pDD->GetLevel(m_iLevel).GetName();
        m_sDropLabel = pDD->GetLevel(m_iLevel).GetLabel();

        return true;
    }
    else                                    // it's either an item or a rec
    {
        CDictRecord* pRec = pDD->GetLevel(m_iLevel).GetRecord(m_iRec);

        int iRecOccs = pRec->GetMaxRecs();

        if (dict_element_type == DictElementType::Record)
        {
            if (iRecOccs == 1)
                return  DragSingleRecord(dict_tree_node, pForm);
            else
                return  DragMultipleRecord(dropPoint, pForm);
        }
        else if (dict_element_type == DictElementType::Item)
        {
            // to invoke the correct func, i need to know if the item being dragged,
            // in any capacity, repeats; so, if it is:
            //
            // [1] an item that has occurrences
            // [2] a subitem that has occurrences
            // [3] a subitem that DOESN'T have occs but its parent item does
            //
            // then i'll classify it as a multiply-occurring item


            // this tells me #1 & #2 above, i.e., how many occurrences the item/subitem has

            m_iNumOccs = pRec->GetItem(dict_tree_node->GetItemIndex())->GetOccurs();

            // this tells me #3 above, i.e., is it a non-repeating subitem
            // belonging to a parent who repeats

            if (m_iNumOccs == 1 && dict_tree_node->IsSubitem())
            {
                const CDictItem* pParent = dict_tree_node->GetDDDoc()->GetDict()->GetParentItem (m_iLevel, m_iRec, dict_tree_node->GetItemIndex());

                m_iNumOccs = pParent->GetOccurs();
            }

            if (iRecOccs == 1)
            {
                if (m_iNumOccs > 1)
                    return  DragMultipleItemFromSingleRecord(dict_tree_node, dropPoint, pForm);
                else
                    return  DragSingleItemFromSingleRecord(dict_tree_node, pForm);
            }
            else
            {
                if (m_iNumOccs > 1)
                    return  DragMultipleItemFromMultipleRecord(dict_tree_node, pForm);
                else
                    return  DragSingleItemFromMultipleRecord(dict_tree_node, dropPoint, pForm);
            }
        }
        
        else
        {
            ASSERT(false);
            return false;
        }
    }
}

// ******************************************************************************
// before finishing a drop when dragging a record, see if there are any *unkeyed*
// items remaining in the record;
//
// - if the record is single, i'll know the drop will be as protected;
// - if the record is multiple, i'll disallow the drop entirely!

bool UnkeyedItemsLeftInRecord(const CFormDoc* pDoc, const CDictRecord& dict_record)
{
    const CDEFormFile* pFF = &pDoc->GetFormFile();

    for( int i = 0; i < dict_record.GetNumItems(); ++i )
    {
        const CDictItem* dict_item = dict_record.GetItem(i);

        // don't consider subitems, only items
        if( dict_item->GetItemType() == ItemType::Item && !pFF->FindItem(dict_item->GetName()) )
        {
            if( dict_item->AddToTreeFor80() )
                return true;
        }
    }

    return false;
}

// ******************************************************************************
// returns true if it's a legal drop;
// set up local vars w/in the class to indicate what happened in the drop

bool CFormDropRules::DragSingleRecord(DictTreeNode* dict_tree_node, CDEForm* pForm)
{
    const CDictRecord* pRec = m_pFormDoc->GetSharedDictionary()->GetLevel(m_iLevel).GetRecord(m_iRec);

    m_sDropName = pRec->GetName();
    m_sDropLabel = pRec->GetLabel();

    // if the record doesn't have the same level as the form, then illegal

    if (m_iLevel > pForm->GetLevel())
    {
        m_bIllegalDrop = true;

        m_sErrMsg = ErrMsg01;

        return false;
    }

    // if the level of the dropped rec is less than the curr form's level
    // then IF it's the COMMON (ID rec), drop it as mirror, otherwise, illegal drop

    if (m_iLevel < pForm->GetLevel() )

    {
        if (dict_tree_node->GetRecordIndex() == COMMON)
        {
            m_eDropOp = DropSingleRec;

            m_eDropResult = DropAsMirror;

            return  true;
        }
        else
        {
            m_bIllegalDrop = true;

            m_sErrMsg = ErrMsg01;

            return false;
        }
    }

    if (pForm->isFormMultiple())
    {
        if (dict_tree_node->GetRecordIndex() == COMMON)
        {
            m_eDropOp = DropSingleRec;

            m_eDropResult = DropAsMirror;

            return  true;
        }
        else
        {
            m_bIllegalDrop = true;

            m_sErrMsg = ErrMsg10;

            return false;
        }
    }

    // form is single

    // if there are no unkeyed items left in this record,
    // then drop it as mirror (display)

    if (UnkeyedItemsLeftInRecord (m_pFormDoc, *pRec))
    {
        m_eDropResult = DropAsKeyed;    // only those fields that can be dropped as keyed will be dropped

        m_eDropOp = DropSingleRec;

        return true;
    }
    else if (dict_tree_node->GetRecordIndex() == COMMON)
    {
        m_eDropOp = DropSingleRec;

        m_eDropResult = DropAsMirror;

        return  true;
    }
    else
    {
        m_bIllegalDrop = true;

        m_sErrMsg = ErrMsg22;

        return false;
    }
}


// ******************************************************************************

bool CFormDropRules::DragMultipleRecord(CPoint dropPoint, CDEForm* pForm)
{
    const CDictRecord* pRec = m_pFormDoc->GetSharedDictionary()->GetLevel(m_iLevel).GetRecord(m_iRec);

    m_sDropName = pRec->GetName();
    m_sDropLabel = pRec->GetLabel();

    // if the record doesn't have the same level as the form, then illegal

    if (m_iLevel != pForm->GetLevel())
    {
        m_bIllegalDrop = true;

        m_sErrMsg = ErrMsg01;

        return false;
    }

    if (!UnkeyedItemsLeftInRecord (m_pFormDoc, *pRec))   // go ahead and do this test now
    {
        m_bIllegalDrop = true;

        m_sErrMsg = ErrMsg22;

        return false;
    }

    if (pForm->isFormMultiple())    // step #3/4
    {
        const CDataDict* pDD = m_pFormDoc->GetSharedDictionary().get();

        // the form loops, see if it's the same as the record i'm dropping

        if (pForm->GetRecordRepeatName() == pDD->GetLevel(m_iLevel).GetRecord(m_iRec)->GetName())
        {
            m_eDropOp = DropMultipleRec;

            m_eDropResult = DropUnrostered;

            return true;
        }
        else    // the form's looping, but on a diff record than the one being dropped; bail
        {
            m_bIllegalDrop = true;

            m_sErrMsg = ErrMsg20;

            return false;
        }
    }

    // form is single


    // Rule #6, steps 5-7, see if anything at drop location

    CDEItemBase* pItem = pForm->FindItem(dropPoint);

    if (pItem &&                        // if something was found at the drop location
        pItem->GetItemType() == CDEFormBase::Roster)// then see if it's a roster
    {
        CDERoster* pRoster = (CDERoster*) pItem;

        // if the record controlling the roster is the same as the
        // record being dropped, then ok to drop

        if (pRoster->GetTypeName() == m_sDropName)
        {
            m_eDropOp = DropMultipleRec;

            m_eDropResult = DropOnRoster;

            return true;
        }
        else    // otherwise, trying to drop on a roster created from a diff rec type, no good
        {
            m_bIllegalDrop = true;

            m_sErrMsg = ErrMsg21;

            return false;
        }
    }

    m_eDropOp = DropMultipleRec;

    if (pForm->AnyKeyedFieldsOnForm())  // then record can only be dropped as a roster
    {
        m_eDropResult = DropAsRoster;
    }
    else
    {
        m_bQuery4Roster = true;     // need to have user answer if they want rec rostered or not
    }
    return true;
}

// ******************************************************************************
// this is a helper func to DragSI_FromSR() && DragMI_FromSR; this is checking two cases:
//
// [1] if this is an item, see if any of its subitems are being keyed
// [2] if this is a subitem, see if its parent is being keyed
//
// if either of these cases are true, then can't drop item

bool CFormDropRules::CheckItemVsSubItemFromSingle(const CDataDict* pDD, const CDictItem* pDI)
{
    bool bKeyedElsewhere = false;
    CDEFormFile* pFF = &m_pFormDoc->GetFormFile();
    const CDictRecord* pDR = pDD->GetLevel(m_iLevel).GetRecord(m_iRec);

    if (pDI->GetItemType() == ItemType::Item)     // then see if any of its subitems are being keyed
    {
        int iNumKids = pFF->GetNumDictSubitems (pDR, pDI);

        int loop, i;

        const CDictItem* pSubItems;

        for (loop=0, i=m_iItem+1;
             loop < iNumKids  && !bKeyedElsewhere;
             loop++, i++)
        {
            pSubItems = pDR->GetItem(i);

            bKeyedElsewhere = pFF->FindItem(pSubItems->GetName());
        }
        if (bKeyedElsewhere)
        {
            m_bIllegalDrop = true;

            m_sErrMsg = ErrMsg23;
        }
    }
    else if (pDI->GetItemType() == ItemType::Subitem)
    {
        int iParent = pFF->GetDictItemParentIndex (pDR, pDI);

        const CDictItem* pParent = pDR->GetItem(iParent);

        bKeyedElsewhere = pFF->FindItem(pParent->GetName());

        if (bKeyedElsewhere)
        {
            m_bIllegalDrop = true;

            m_sErrMsg = ErrMsg24;
        }
    }
    return  bKeyedElsewhere;
}

// ******************************************************************************
// this is a helper func to DragSI_FromMR() & DragMI_FromMR; this is checking two cases:
//
// [1] if this is an item, make sure none of its subitems are being keyed
// [2] if this is a subitem, make sure its parent isn't being keyed
//
// this function is identical to CheckItemVsSubItemFromSingle, except for the err
// msg generated

bool CFormDropRules::CheckItemVsSubItemFromMultiple(const CDataDict* pDD, const CDictItem* pDI)
{
    bool bKeyedElsewhere = false;
    CDEFormFile* pFF = &m_pFormDoc->GetFormFile();
    const CDictRecord* pDR = pDD->GetLevel(m_iLevel).GetRecord (m_iRec);

    if (pDI->GetItemType() == ItemType::Item)     // then see if any of its subitems are being keyed
    {
        int iNumKids = pFF->GetNumDictSubitems (pDR, pDI);

        int loop, i;

        const CDictItem* pSubItems;

        for (loop=0, i=m_iItem+1;
             loop < iNumKids && !bKeyedElsewhere;
             loop++, i++)
        {
            pSubItems = pDR->GetItem(i);

            bKeyedElsewhere = pFF->FindItem(pSubItems->GetName());
        }

        if (bKeyedElsewhere)
        {
            m_bIllegalDrop = true;

            m_sErrMsg = ErrMsg23;
        }
    }
    // if it's a subitem, make sure its parent hasn't already been dropped

    else if (pDI->GetItemType() == ItemType::Subitem)
    {
        int iParent = pFF->GetDictItemParentIndex (pDR, pDI);

        const CDictItem* pParent = pDR->GetItem(iParent);

        bKeyedElsewhere = pFF->FindItem(pParent->GetName());

        // if the parent's already being used, disallow the drop; can't do display
        // field, as usual prob, which occurrence to display?

        if (bKeyedElsewhere)
        {
            m_bIllegalDrop = true;

            m_sErrMsg = ErrMsg24;
        }
    }
    return  bKeyedElsewhere;
}

// ******************************************************************************
// another helper func to DragSI_FromSR(); if the user is dropping a subitem,
// this checks to see if any overlapping sibling subitems have already been dropped
// as keyed items; if so, the subitem can only be dropped as a protected field;
// so get the subitem's parent, then check all the other subitems
//
//
// this is a variation of CDEFormFile::DoAnySubitemsOverlap(), which tests to see
// if any subitem overlaps another; doesn't care about being keyed or not


bool CFormDropRules::CheckForSubitemOverlap(const CDataDict* pDD, const CDictItem* pDI)
{
    bool bKeyedOverlap = false;     // as soon as i find one sibling subitem that's being keyed, stop

    if (pDI->GetItemType() != ItemType::Subitem)

        return bKeyedOverlap;

    CDEFormFile* pFF = &m_pFormDoc->GetFormFile();
    const CDictRecord* pDR = pDD->GetLevel(m_iLevel).GetRecord(m_iRec);

    int iParent  = pFF->GetDictItemParentIndex (pDR, pDI);
    int iNumKids = pFF->GetNumDictSubitems (pDR, pDR->GetItem(iParent));
    int i, loop;

    const CDictItem* pSubItem;

    for (loop=0, i=iParent+1;
         loop < iNumKids && !bKeyedOverlap;
         loop++, i++)
    {
        if (i == m_iItem)   // don't check the subitem being dropped against itself!
            continue;

        pSubItem = pDR->GetItem(i);

        if (i < m_iItem)    // the i-th item starts before pDI item
        {
            if (pSubItem->GetStart() + (pSubItem->GetLen()*pSubItem->GetOccurs()) > pDI->GetStart())    // items overlap
            {
                if (pFF->FindItem(pSubItem->GetName()) )   // overlapping item is being keyed
                {
                    bKeyedOverlap = true;
                }
            }
        }
        else    // i > m_iItem, the i-th item starts *after* the pDI item
        {
            if (pDI->GetStart() + (pDI->GetLen()*pDI->GetOccurs()) > pSubItem->GetStart())  // items overlap
            {
                if (pFF->FindItem(pSubItem->GetName()) )   // overlapping item is being keyed
                {
                    bKeyedOverlap = true;
                }
            }
        }
    }
    return bKeyedOverlap;
}


// ******************************************************************************
// drag an item that has no occurrences (i.e., 1!) from a record that does not repeat

bool CFormDropRules::DragSingleItemFromSingleRecord(DictTreeNode* dict_tree_node, CDEForm* pForm)
{
    m_iItem  = dict_tree_node->GetItemIndex();  // item is 0-based

    const CDataDict* pDD = m_pFormDoc->GetSharedDictionary().get();
    const CDictItem* pDI = pDD->GetLevel(m_iLevel).GetRecord(m_iRec)->GetItem(m_iItem);

    DetermineDropNameNLabel(dict_tree_node, *pDI);

    if (m_iLevel > pForm->GetLevel())   // item from Level 2 and form is Level 1
    {
        m_bIllegalDrop = true;
        m_sErrMsg = ErrMsg01;
        return false;
    }

    if (m_iLevel < pForm->GetLevel())       // i.e., item from level 1 and form is level 2
    {
        bool bItemAlreadyKeyed = m_pFormDoc->GetFormFile().FindItem(pDI->GetName());

        if (bItemAlreadyKeyed)  // then ok to drop as display
        {
            m_eDropOp = DropSI_FromSR;
            m_eDropResult = DropAsMirror;
            return true;
        }
        else {
            m_bIllegalDrop = true;
            m_sErrMsg = ErrMsg50;
            return false;
        }
    }

    // at this pt we know the levels are =

    if (m_pFormDoc->GetCurForm()->isFormMultiple())
    {
        // orig, was just the following

//      m_eDropOp = DropSI_FromSR;

//      m_eDropResult = DropAsMirror;

//      return true;

    // now i need to check overlapping stuff

        if (CheckItemVsSubItemFromSingle (pDD, pDI))
        {
            return false;   // err msgs set herein
        }
        else if (CheckForSubitemOverlap (pDD, pDI) )
        {
            m_bIllegalDrop = true;
            m_sErrMsg = ErrMsg25;
            return false;
        }
        else
        {
            bool bItemAlreadyKeyed = m_pFormDoc->GetFormFile().FindItem(pDI->GetName());

            if (bItemAlreadyKeyed)  // then ok to drop as display
            {
                m_eDropOp = DropSI_FromSR;
                m_eDropResult = DropAsMirror;
                return true;
            }
            else
            {
                m_bIllegalDrop = true;
                m_sErrMsg = ErrMsg11;
                return false;
            }
        }
    }

    bool bKeyedElsewhere = m_pFormDoc->GetFormFile().FindItem(m_sDropName);


    if (bKeyedElsewhere)
    {
        m_eDropOp = DropSI_FromSR;
        m_eDropResult = DropAsMirror;
        return true;
    }

    // the final frontier: subitems & value sets
    //
    // need to do two types of checks; for
    //
    // Item/Subitem
    //      if its an item & any of its subitems are keyed
    //      or its a subitem & its parent is keyed, can not drop
    // Subitem Overlap
    //      if the user's dropping a subitem which overlaps w/another subitem
    //      currently being keyed, then can't drop
    // Item vs. ValueSet
    //      If one of the valuesets of an item is already being keyed, must
    //      drop the item as mirror

    if (CheckItemVsSubItemFromSingle (pDD, pDI))
    {
        return false;   // err msgs set herein

//      m_eDropResult = DropAsMirror;
//      m_eDropOp = DropSI_FromSR;
    }

    if (CheckForSubitemOverlap (pDD, pDI) )
    {
        m_bIllegalDrop = true;
        m_sErrMsg = ErrMsg25;
        return false;
    }
    else                // it's legit, let be keyed !
    {
        m_eDropResult   = DropAsKeyed;
        m_eDropOp       = DropSI_FromSR;
        m_bDropSubitems = CanUserDropSubitems(pDD, pDI);
    }
    return true;
}

// ******************************************************************************
// if this is an item and it has subitems, see if any of the subitems overlap; if
// not, then the user shld be asked if they want the subitems or the item itself
// dropped; this does not check if any of the subitems are keyed, that test is
// done in CheckItemVsSubItemFrom[Single/Multiple]

bool CFormDropRules::CanUserDropSubitems(const CDataDict* pDD, const CDictItem* pDI)
{
    bool bOk2DropSI = true;

    if (pDI->GetItemType() != ItemType::Item)
        bOk2DropSI = false;

    CDEFormFile* pFF = &m_pFormDoc->GetFormFile();
    const CDictRecord* pDR = pDD->GetLevel(m_iLevel).GetRecord (m_iRec);

    int iNumSI = pFF->GetNumDictSubitems (pDR, pDI);

    if (iNumSI == 0)    // i.e., there aren't any subitems!
        bOk2DropSI = false;

// for now, if the user tries to drop a multiply-occurring item that contains
// subitems, in theory they have the choice of whether to make a roster from the
// item crossed by its occurrences, or the subitems crossed by the occurrences;
// but if any of the subitems overlap, then they won't have the choice, will get
// the item by occs!

    if (pFF->DoAnySubitemsOverlap (pDR, pDI))   // if subs overlap, can't do
        bOk2DropSI = false;

    return bOk2DropSI;
}

// ******************************************************************************
// drag an item that has multiple occurrences from a record that does not repeat
// OR could be dragging one of the occurrences of the repeating item

bool CFormDropRules::DragMultipleItemFromSingleRecord(DictTreeNode* dict_tree_node, CPoint dropPoint, CDEForm* pForm)
{
    m_iItem  = dict_tree_node->GetItemIndex();  // item is 0-based

    const CDataDict* pDD = m_pFormDoc->GetSharedDictionary().get();
    const CDictRecord* pDR = pDD->GetLevel(m_iLevel).GetRecord(m_iRec);
    const CDictItem* pDI = pDR->GetItem(m_iItem);

    DetermineDropNameNLabel(dict_tree_node, *pDI);

    if (m_iLevel != pForm->GetLevel())  // item from Level 2 and form is Level 1
    {
        m_bIllegalDrop = true;
        m_sErrMsg = ErrMsg01;

        return false;
    }

    // at this pt we know the levels are =

    if (m_pFormDoc->GetCurForm()->isFormMultiple())
    {
        m_bIllegalDrop = true;
        m_sErrMsg = ErrMsg30;

        return false;
    }

    bool bKeyedElsewhere = m_pFormDoc->GetFormFile().FindItem(m_sDropName);

    if (bKeyedElsewhere)
    {
        m_bIllegalDrop = true;
        m_sErrMsg = ErrMsg30;
        return false;
    }

    // now let's pay attn to whether i've got a 'parent' occurrence or a 'child' occurrence
    ASSERT(dict_tree_node->GetItemOccurs() == NONE);

    // need to do two types of checks; for
    //
    // ITEMS: if any of its subitems are keyed, can't drop it as keyed (can make display)
    // SUBITEMS: if its parent has been dropped (or overlapping siblings; deal
    //          with shortly), then drop as keyed


    /*

    test #1: CheckItemVsSubItemFromSingle()

    if this is an item but any of its subitems are being keyed, the drop is invalid; OR
    if this is a subitem & its parent is being keyed, the drop is invalid

    test #2: CheckForSubitemOverlap()

    if it's a subitem being dropped & it overlaps with another subitem already being
    keyed, the drop is illegal (if it's not a subitem, test will not fail)

    */

    if (CheckItemVsSubItemFromSingle(pDD, pDI) ||
        CheckForSubitemOverlap      (pDD, pDI))
    {
        m_bIllegalDrop = true;
        m_sErrMsg = ErrMsg30;

        return false;
    }
    // ok, drop is legit, just find out what action to take


    // Rule #3, steps 6-8, analyze placement of drop location; so first,
    // i need to find what, if anything, is at the drop point

    CDEItemBase* pItem = pForm->FindItem(dropPoint);

    if (pItem && pItem->GetItemType() == CDEFormBase::Roster)
    {
        ASSERT(dict_tree_node->GetDictElementType() == DictElementType::Item);

        CDERoster* pGrid = (CDERoster*) pItem;

        // if the record controlling the roster is the same as the
        // one to which the item belongs, then ok to drop the item (step #5)

        if (!dict_tree_node->IsSubitem())    // then test against record
        {
            if (pGrid->GetTypeName() == pDR->GetName())
            {
                m_eDropOp = DropSI_FromMR;
                m_eDropResult = DropOnRoster;
                m_bDropSubitems = CanUserDropSubitems(pDD, pDI);
                return true;
            }
            // else handled below--it's an illegal drop
        }
        else    // it's a subitem
        {
            const CDictItem* pParent = pDD->GetParentItem (m_iLevel, m_iRec, m_iItem);

            if (pGrid->GetTypeName() == pParent->GetName())
            {
                m_eDropOp = DropSI_FromMR;
                m_eDropResult = DropOnRoster;
                m_bDropSubitems = CanUserDropSubitems(pDD, pDI);
                return true;
            }
        }
        // otherwise, trying to drop on a roster created from a diff rec type, no good

        m_bIllegalDrop = true;
        m_sErrMsg = ErrMsg21;
        return false;
    }
    m_eDropOp = DropMI_FromSR;
    m_eDropResult = DropAsRoster;   // later, when serpro fixes their code, shld allow user the choice
    m_bDropSubitems = CanUserDropSubitems(pDD, pDI);

    return true;
}


// ******************************************************************************
// drag an item that has no (i.e., 1) occurrences from a record that repeats

bool CFormDropRules::DragSingleItemFromMultipleRecord(DictTreeNode* dict_tree_node, CPoint dropPoint, CDEForm* pForm)
{
    m_iItem  = dict_tree_node->GetItemIndex();  // item is 0-based

    const CDataDict* pDD = m_pFormDoc->GetSharedDictionary().get();
    const CDictRecord* pDR = pDD->GetLevel(m_iLevel).GetRecord(m_iRec);
    const CDictItem* pDI = pDR->GetItem(m_iItem);

    if( !pDI->AddToTreeFor80() )
    {
        ErrorMessage::Display(_T("Adding binary dictionary items to forms is not supported in this release."));
        return false;
    }

    DetermineDropNameNLabel(dict_tree_node, *pDI);

    // if the item's level isn't from the same level as the form, then illegal

    if (m_iLevel != pForm->GetLevel())
    {
        m_bIllegalDrop = true;

        m_sErrMsg = ErrMsg01;

        return false;
    }

    bool bKeyedElsewhere = m_pFormDoc->GetFormFile().FindItem(m_sDropName);

    if (bKeyedElsewhere)    // prob of which occurrence to display
    {
        m_bIllegalDrop = true;

        m_sErrMsg = ErrMsg30;

        return false;
    }

    // ///////////////////////////
    // ITEM vs. SUBITEM
    //
    // if this rtns true, then either an Item had keyed subitems, or a subitem's parent was keyed

    if (CheckItemVsSubItemFromMultiple (pDD, pDI))

        return false;


    if (CheckForSubitemOverlap (pDD, pDI))
    {
        m_bIllegalDrop = true;

        m_sErrMsg = ErrMsg25;

        return false;
    }

    // keep tryin

    if (pForm->isFormMultiple())
    {
        if (pForm->GetRecordRepeatName() == pDR->GetName())
        {
            m_eDropOp = DropSI_FromMR;
            m_eDropResult = DropAsKeyed;
            m_bDropSubitems = CanUserDropSubitems(pDD, pDI);

            return true;
        }
        else
        {
            m_bIllegalDrop = true;
            m_sErrMsg = ErrMsg20;

            return false;
        }
    }

    // Rule #3, steps 6-8, analyze placement of drop location; so first,
    // i need to find what, if anything, is at the drop point

    CDEItemBase* pItem = pForm->FindItem(dropPoint);

    if (pItem && pItem->GetItemType() == CDEFormBase::Roster)
    {
        CDERoster* pGrid = (CDERoster*) pItem;

        // if the record controlling the roster is the same as the
        // one to which the item belongs, then ok to drop the item (step #5)

        if (pGrid->GetTypeName() == pDR->GetName())
        {
            m_eDropOp = DropSI_FromMR;
            m_eDropResult = DropOnRoster;
            m_bDropSubitems = CanUserDropSubitems(pDD, pDI);

            return true;
        }
        else    // otherwise, trying to drop on a roster created from a diff rec type, no good
        {
            m_bIllegalDrop = true;
            m_sErrMsg = ErrMsg21;

            return false;
        }

    }

    m_eDropOp = DropSI_FromMR;      // this will be true regardless of result

    if (pForm->AnyKeyedFieldsOnForm())      // then drop the item as a roster
    {
        m_eDropResult = DropAsRoster;
    }
    else    // need to have user answer if they want rec rostered or not
    {
        m_bDropSubitems = CanUserDropSubitems(pDD, pDI);
        m_bQuery4Roster = true;     // need to have user answer if they want rec rostered or not
    }
    return true;
}

// ******************************************************************************
// drag an item that has occurrences from a record that repeats
//
// smg, 02/00: as of now, this func won't be called, because we're not allowing
// an item to repeat in a multiple record; so i haven't modified these rules

bool CFormDropRules::DragMultipleItemFromMultipleRecord(DictTreeNode* dict_tree_node, CDEForm* pForm)
{
    m_iItem  = dict_tree_node->GetItemIndex();  // item is 0-based

    const CDataDict* pDD = m_pFormDoc->GetSharedDictionary().get();
    const CDictRecord* pDR = pDD->GetLevel(m_iLevel).GetRecord(m_iRec);
    const CDictItem* pDI = pDR->GetItem(m_iItem);

    DetermineDropNameNLabel(dict_tree_node, *pDI);

    // if the item's level isn't from the same level as the form, then illegal

    if (m_iLevel != pForm->GetLevel())
    {
        m_bIllegalDrop = true;
        m_sErrMsg = ErrMsg01;
        return false;
    }


    bool bKeyedElsewhere = m_pFormDoc->GetFormFile().FindItem(m_sDropName);

    if (bKeyedElsewhere)    // prob of which occurrence to display
    {
        m_bIllegalDrop = true;
        m_sErrMsg = ErrMsg30;
        return false;
    }

    // ///////////////////////////
    // ITEM vs. SUBITEM
    //
    // if this rtns true, then either an Item had keyed subitems, or a subitem's parent was keyed

    if (CheckItemVsSubItemFromMultiple (pDD, pDI))
        return false;


    if (CheckForSubitemOverlap (pDD, pDI))
    {
        m_bIllegalDrop = true;
        m_sErrMsg = ErrMsg25;
        return false;
    }


    // now let's pay attn to whether i've got a 'parent' occurrence or a 'child' occurrence

    // ** user dropping the "parent" occurrence
    ASSERT(dict_tree_node->GetItemOccurs() == NONE);

    if (pForm->isFormMultiple())    // then looping better be on this rec or bail
    {
        if (pForm->GetRecordRepeatName() != pDR->GetName())
        {
            m_bIllegalDrop = true;
            m_sErrMsg = ErrMsg20;
            return false;
        }

        m_eDropOp = DropMI_FromMR;
        m_eDropResult = DropAsRoster; //multi item from multi record only as roster

        m_bQuery4Roster = true;

        return true;
    }

    // form is single with fields
    else if (pForm->AnyKeyedFieldsOnForm())
    {
        //if there are other keyed fields on the form and if the item occurs, the form cannot be single
        //it has to have the item's parent as the looping form.
        m_bIllegalDrop = true;
        m_sErrMsg = ErrMsg232;
        return false;
    }

    // form is blank
    else
    {
        m_eDropOp = DropMI_FromMR;
        m_eDropResult = DropAsKeyed;
        return true;
    }
}
