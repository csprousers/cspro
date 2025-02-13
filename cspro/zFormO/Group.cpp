#include "StdAfx.h"
#include "Group.h"
#include "RenDlg.h"


IMPLEMENT_DYNAMIC(CDEGroup, CDEItemBase)


CDEGroup::CDEGroup() : m_bRequired(true),
                       m_eRIType (CDEFormBase::UnknownRI), m_iCurOccurrence(0),
                       m_iTotalOccs(0), m_iMaxLoopOccs(1), m_iDataOccs(0) // RHF MOD Mar 29, 2000 Add m_iDataOccs
{
    SetItemType (Group);

    m_pMaxField = NULL;

    SetName(sEmptyName);
    SetLabel(sEmptyLabel);
}

CDEGroup::~CDEGroup()
{
    RemoveAllItems();   // delete the fields AND indices to them
}


CDEGroup::CDEGroup(const CDEGroup& group)
    :   CDEItemBase(group)
{
    SetRequired     (group.GetRequired());  // CDEGroup-level vars
    SetFormName     (group.GetFormName());
    SetMaxLoopOccs  (group.GetMaxLoopOccs());
    SetMaxField     (group.GetMaxField());
    SetRIType       (group.GetRIType());
    SetTypeName     (group.GetTypeName());

    m_pMaxField = NULL;

    // these guys are run-time vars, set up as done in constructor w/no args

    SetTotalOccs(0);
    SetCurOccurrence(0);
    SetDataOccs(0);

    // Clone the group items
    for (int iItem = 0; iItem < group.GetNumItems(); iItem++)
    {
        CDEItemBase* pItem = group.GetItem(iItem);

        std::unique_ptr<CDEItemBase> pItemCopy = pItem->Clone();
        pItemCopy->SetParent(this);
        AddItem(pItemCopy.release());
    }
}


void CDEGroup::SetupErrMsg(CString& csErr, CString& csMsg ,CDEFormFile* pFormFile)
{
    if (!csErr.IsEmpty())
        csMsg += _T("\n");

    if(GetName().CompareNoCase(_T("BaseGrp")) ==0 ) {
        for(int iIndex =0; iIndex < pFormFile->GetNumLevels(); iIndex++) {
            if(pFormFile->GetLevel(iIndex)->GetRoot() == this) {
                csMsg += pFormFile->GetLevel(iIndex)->GetName();
                break;
            }
        }
    }
    else {
        csMsg += GetName();
    }
    csMsg += _T(":\n");
}


/////////////////////////////////////////////////////////////////////////////
//
//                          CDEGroup::Reconcile
//
//  return code "Delete" means the caller should delete this group
//
/////////////////////////////////////////////////////////////////////////////

namespace
{
    template<typename T>
    std::vector<const T*> GetMultiplyOccurringRecords(const CDataDict& dictionary, size_t level_number)
    {
        std::vector<const T*> dict_records;

        if( level_number < dictionary.GetNumLevels() )
        {
            const DictLevel& dict_level = dictionary.GetLevel(level_number);

            for( int r = 0; r < dict_level.GetNumRecords(); ++r )
            {
                const CDictRecord* dict_record = dict_level.GetRecord(r);

                if( dict_record->GetMaxRecs() > 1 )
                    dict_records.emplace_back(dict_record);
            }
        }

        return dict_records;
    }

    template<typename T>
    std::vector<const T*> GetItemCandidates(const CDEFormFile& form_file, const CDataDict& dictionary, const CDEGroup* pGroup, std::optional<size_t> level_number)
    {
        std::vector<const T*> dict_items;

        // if the group is multiply occurring, only get items from the corresponding record
        if( pGroup->GetRIType() == CDEFormBase::eRIType::Record )
        {
            const DictLevel* dict_level;
            const CDictRecord* dict_record;

            if( ( dictionary.LookupName<CDictRecord>(pGroup->GetTypeName(), &dict_level, &dict_record) ) &&
                ( !level_number.has_value() || dict_level->GetLevelNumber() == *level_number ) )
            {
                for( int i = 0; i < dict_record->GetNumItems(); ++i )
                {
                    const CDictItem* dict_item = dict_record->GetItem(i);

                    // only add items without occurrences
                    if( dict_item->GetOccurs() == 1 && !form_file.IsUsed(*dict_item) )
                    {
                        if( dict_item->AddToTreeFor80() )
                            dict_items.emplace_back(dict_item);
                    }
                }
            }
        }

        // if it is singly occurring, add items from singly-occurring records on this level
        else if( pGroup->GetRIType() == CDEFormBase::UnknownRI )
        {
            ASSERT(level_number.has_value() && *level_number < dictionary.GetNumLevels());
            const DictLevel& dict_level = dictionary.GetLevel(*level_number);

            for( int r = -1; r < dict_level.GetNumRecords(); ++r )
            {
                const CDictRecord* dict_record = dict_level.GetRecord(( r == -1 ) ? COMMON : r);

                if( dict_record->GetMaxRecs() == 1 )
                {
                    for( int i = 0; i < dict_record->GetNumItems(); ++i )
                    {
                        const CDictItem* dict_item = dict_record->GetItem(i);

                        if( !form_file.IsUsed(*dict_item) )
                        {
                            if( dict_item->AddToTreeFor80() )
                                dict_items.emplace_back(dict_item);
                        }
                    }
                }
            }
        }

        return dict_items;
    }
}


eReturnType CDEGroup::Reconcile(CDEFormFile* pFormFile, CString& csErr, bool bSilent, bool bAutoFix)
{
    bool bOccProb = false;
    eReturnType eReturn = OK;
    bool bFirst = true;

    CString csDict;     // name (symbol) of dictionary
    CString csName;     // name (symbol) of an item

    // if group is tied to a record or item, make sure it exists
    // tell caller to delete this group if not

    eRIType gType = GetRIType();

    if(gType == CDEFormBase::UnknownRI) {
        //Check the group for items from different records; if they are not from singly-occurring records
        //then delete the group

        //if the group has items from same record and the record is multiple set the group's occs and RItype

        int iNumItems = this->GetNumItems();
        const CDictRecord* pRecord = NULL;
        if(iNumItems > 0 ) {
            //Get the record type for the first item
            bool bFound = false;
            DICT_LOOKUP_INFO structLookup;
            BOOL bMultiple = FALSE;
            BOOL bDifferent = FALSE;

            for(int iItem =0; iItem < iNumItems; iItem++){
                CDEItemBase* pBase = GetItem(iItem);

                CDEField* pField = DYNAMIC_DOWNCAST(CDEField,pBase);
                if(pField){
                    // stuff the dictionary name (use may have changed it in dict frame)
                    csDict = pFormFile->GetDictionary()->GetName();
                    pField->SetItemDict(csDict);
                    structLookup.csName = pField->GetItemName();
                    bFound = LookupSymbol (pFormFile, csDict, structLookup);  // BMD 20 March 2007 - incorrect reconcile
                    if(bFound) {
                        pRecord = structLookup.pRecord ;
                        if(!bMultiple){
                            bMultiple = pRecord->GetMaxRecs() > 1;
                        }
                    }
                    if(pRecord && bFound && !bDifferent) {//Check if they are same
                        if(pRecord != structLookup.pRecord){
                            bDifferent = TRUE;
                        }
                    }
                }
            }
            if(bMultiple && bDifferent) {//if some fields are multiple and are from different records
                CString csMsg;
                SetupErrMsg (csErr, csMsg,pFormFile);
                //csMsg += "  Group '";
                csMsg += this->GetName();
                //csMsg += "' has items from different records which are multiple\n";
                csMsg += _T(" deleted.\n");
                csErr += csMsg;
                return Delete ; //delete the group
            }
            else if(bMultiple && !bDifferent) {//if the items in the group are from multiply occuring record and
                //and the items are all from the same record
                ASSERT(pRecord);
                SetRIType(Record);
                SetTypeName(pRecord->GetName());
                SetMaxLoopOccs (pRecord->GetMaxRecs());
            }
        }
    }
    else if(gType == CDEFormBase::Record)   {
        csDict = _T("");

        DICT_LOOKUP_INFO structLookup;
        structLookup.csName = GetTypeName();

        bool bFound = LookupSymbol (pFormFile, csDict, structLookup);
        if(!bFound) {//Do offer the choice of making a selection and fixing the stuff
            int iFormNum = this->GetFormNum();
            CDEForm* pForm = pFormFile->GetForm (iFormNum);
            int iLevel = pForm->GetLevel();

#ifdef WIN_DESKTOP
            if( bShowFormSelDlg && this->GetRIType() == CDEFormBase::Record )
            {
                // generate a list of multiply-occurring records
                std::vector<const DictNamedBase*> dict_element_candidates = GetMultiplyOccurringRecords<DictNamedBase>(*pFormFile->GetDictionary(), iLevel);

                if( !dict_element_candidates.empty() )
                {
                    CRenameDlg rename_dlg(*this, dict_element_candidates);

                    if( rename_dlg.DoModal() == IDOK )
                    {
                        bShowFormSelDlg = !rename_dlg.DeleteAllItemsNotFound();
                        const DictNamedBase* selected_dict_candidates = rename_dlg.GetSelectedDictCandidate();

                        if( selected_dict_candidates != nullptr )
                        {
                            this->SetTypeName(selected_dict_candidates->GetName());
                            eReturn = Reconciled;
                            structLookup.csName = GetTypeName();
                            bFound = LookupSymbol(pFormFile, csDict, structLookup);
                        }
                    }
                }
            }
#endif
        }

        if (!bFound) {
            return Delete;
        }
        SetRequired(structLookup.pRecord->GetRequired());    //  BMD  4 May 2000
        if(eReturn == Reconciled){
            ReconcileRecordOccs (pFormFile,csErr, bSilent, bAutoFix, structLookup, &bFirst);
        }
        else {
            eReturn = ReconcileRecordOccs (pFormFile,csErr, bSilent, bAutoFix, structLookup, &bFirst);
        }
    }

    // &&& todo:  gsf added SubItem type 01/24/01
    else if(gType == CDEFormBase::Item || gType == CDEFormBase::SubItem)     {
        //&&& todo: fix this up!! gsf 01/18/00
        // roster based on a (multiple) item
        // tried to check m_csMaxField, but it's always ""
        //
        csDict = _T("");
        DICT_LOOKUP_INFO structLookup;
        structLookup.csName = GetTypeName();
        //        structLookup.csName = GetMaxField();
        bool bFound = LookupSymbol (pFormFile, csDict, structLookup);

        if (!bFound)
            return Delete;

        eReturn = ReconcileItemOccs (pFormFile,csErr, bSilent, bAutoFix, structLookup, &bFirst);
    }


    // ****************************
    // now check all items in this group
    // this loop is "backwards", because we may be deleting things
    // ****************************

    int iItems = GetNumItems();

    for (int i=iItems-1; i>=0; i--) {
        CDEItemBase* pItemBase = GetItem(i);
        CDEFormBase::eItemType type = pItemBase->GetItemType();

        // Field items

        if (type == CDEFormBase::Field) {
            CDEField* pField = assert_cast<CDEField*>(pItemBase);

            // gsf 23-mar-00
            // assume only one dictionary per forms file
            // stuff the dictionary name (use may have changed it in dict frame)
            csDict = pFormFile->GetDictionary()->GetName();

            pField->SetItemDict(csDict);
            //            csDict = pField->GetItemDict();
            // gsf 23-mar-00 end

            DICT_LOOKUP_INFO structLookup;
            structLookup.csName = pField->GetItemName();

            bool bFound = LookupSymbol (pFormFile, csDict, structLookup);

            // gsf begin 18-sep-00
            // force field labels to be same as dictionary labels
            CString sItemOccs;
            if (bFound) {
                //if item is from a multiple record. make sure that this comes from the same record as group
                if(this->GetRIType() == CDEFormBase::Record && structLookup.pRecord->GetMaxRecs() > 1 ) {
                    if(structLookup.pRecord->GetName().CompareNoCase(GetTypeName()) != 0){
                        sItemOccs += _T(" Field '");
                        sItemOccs += pItemBase->GetName();
                        sItemOccs += _T("' deleted. The item does not belong to the same record as the record controlling the form looping.\n");
                        bFound = false;
                        bOccProb = true;
                    }

                }
                if (structLookup.pVSet == NULL) {
                    int iDictOccs = structLookup.pItem->GetOccurs();
                    if (structLookup.pItem->GetItemType() == ItemType::Subitem && structLookup.pItem->GetOccurs() == 1) { // BMD 19 Sep 2001
                        iDictOccs = structLookup.pItem->GetParentItem()->GetOccurs();
                    }
                    if(iDictOccs > 1 && !(GetRIType() == CDEFormBase::Item || GetRIType() == CDEFormBase::SubItem)){
                        sItemOccs += _T(" Field '");
                        sItemOccs += pItemBase->GetName();
                        sItemOccs += _T("' deleted. Occurrences do not match the item occurences in dictionary.\n");
                        bFound = false;
                        bOccProb = true;
                    }
                    else {
                        pField->SetLabel(structLookup.pItem->GetLabel());
                        pField->SetDictItem(structLookup.pItem);
                        if(pField->GetFieldLabelType() == FieldLabelType::DictionaryName){
                            pField->GetCDEText().SetText(structLookup.pItem->GetName());
                        }
                        else if(pField->GetFieldLabelType() == FieldLabelType::DictionaryLabel){
                            pField->GetCDEText().SetText(structLookup.pItem->GetLabel());
                        }
                    }
                }
                else {
                    ASSERT(false);
                }
            }

#ifdef WIN_DESKTOP
            if( !bFound && bShowFormSelDlg && !bOccProb ) //Do offer the choice of making a selection and fixing the stuff
            {
                std::vector<const DictNamedBase*> dict_element_candidates;

                if( pField->GetParent()->GetRIType() == CDEFormBase::Record )
                {
                    dict_element_candidates = GetItemCandidates<DictNamedBase>(*pFormFile, *pFormFile->GetDictionary(), this, std::nullopt);
                }

                else if( pField->GetParent()->GetRIType() == CDEFormBase::UnknownRI )
                {
                    const CDEForm* pForm = pFormFile->GetForm(pField->GetFormNum());
                    dict_element_candidates = GetItemCandidates<DictNamedBase>(*pFormFile, *pFormFile->GetDictionary(), this, pForm->GetLevel());
                }

                if( !dict_element_candidates.empty() )
                {
                    CRenameDlg rename_dlg(*pField, dict_element_candidates);

                    if( rename_dlg.DoModal() == IDOK )
                    {
                        bShowFormSelDlg = !rename_dlg.DeleteAllItemsNotFound();
                        const DictNamedBase* selected_dict_candidates = rename_dlg.GetSelectedDictCandidate();

                        if( selected_dict_candidates != nullptr )
                        {
                            const CDictItem* selected_dict_item = assert_cast<const CDictItem*>(selected_dict_candidates);
                            CString old_name = pField->GetName();

                            pField->SetDictItem(selected_dict_item);
                            pField->SetItemName(selected_dict_item->GetName());
                            pField->SetName(selected_dict_item->GetName());
                            pField->SetLabel(selected_dict_item->GetLabel());

                            if( pField->GetFieldLabelType() == FieldLabelType::DictionaryName)
                            {
                                pField->GetCDEText().SetText(selected_dict_item->GetName());                                
                            }

                            else if( pField->GetFieldLabelType() == FieldLabelType::DictionaryLabel )
                            {
                                pField->GetCDEText().SetText(selected_dict_item->GetLabel());
                            }

                            // 20120710 if a field is renamed, so should the qsf text be
                            if( AfxGetMainWnd() != nullptr )
                            {
                                std::tuple<CDEItemBase*, CString> update(pField, pFormFile->GetDictionary()->MakeQualifiedName(old_name));
                                AfxGetMainWnd()->SendMessage(WM_IMSA_RECONCILE_QSF_FIELD_NAME, reinterpret_cast<WPARAM>(pFormFile), reinterpret_cast<LPARAM>(&update));
                            }

                            eReturn = Reconciled;
                            pField->GetParent()->SetItemSubItemFlags(pFormFile, selected_dict_item);
                            bFound = true;
                        }
                    }
                }
            }
#endif
            // gsf end 18-sep-00
            bOccProb =false;
            if (!bFound) {
                eReturn = Reconciled;
                // construct message
                CString csMsg;
                if (bFirst){
                    bFirst = false;
                    SetupErrMsg (csErr, csMsg,pFormFile);
                }
                if(!sItemOccs.IsEmpty()) {
                    csErr +=sItemOccs;
                }
                else {
                    //csMsg += "  Field '";
                    csMsg += pItemBase->GetName();
                    csMsg += _T(" deleted; not in dictionary.\n");
                    csErr += csMsg;
                }

                // delete this field
                if (bAutoFix) {
                    // trying to follow what's done in CFormScrollView::DeleteSingleItem()
                    if(pField->GetParent()->GetItemType() == CDEItemBase::Roster){
                        CDERoster* pRoster = assert_cast<CDERoster*>(pField->GetParent());
                        pFormFile->RemoveUniqueName(pField->GetName());
                        pRoster->RemoveItem(pRoster->GetItemIndex(pField));
                    }

                    else        // it's a free-standing field on the form
                    {
                        int iFormNum = pField->GetFormNum();
                        CString sFieldName = pField->GetName();

                        CDEForm* pForm = pFormFile->GetForm (iFormNum);
                        if(pForm){//SAVY added this to prevent crash 06/27/01
                            pForm->RemoveItem (pField);
                        }
                        //This is a fix when field name is not same as the item name
                        if(sFieldName.CompareNoCase(structLookup.csName) != 0) {
                            pFormFile->RemoveUniqueName(sFieldName);
                        }

                        pFormFile->RemoveUniqueName(structLookup.csName);
                        RemoveItem(GetItemIndex(pField));
                        // trying to follow what's done in CFormScrollView::DeleteWrapUp()

                        if (pForm && !pForm->AnyKeyedFieldsOnForm())
                        {
                            pForm->SetRecordRepeatName(_T(""));
                            SetLoopingVars();
                        }
                    }
                }
            }
        }       // end "if (type == Field)" stmt

        //  Group/Roster items (Roster added 10-04-00 by smg; hopefully will work ok

        else if(type == CDEFormBase::Group || type == CDEFormBase::Roster){
            CDEGroup* pGroup = (CDEGroup*) pItemBase;
            eReturnType eRet = pGroup->Reconcile(pFormFile, csErr, bSilent, bAutoFix);
            if (type != Group) {
                if (pGroup->GetNumItems() == 0 ){
                    eRet = Delete;
                }
             /*   if (pGroup->GetMaxLoopOccs() > 1 && pGroup->GetParent()->GetMaxLoopOccs() > 1) {
                    if (pGroup->GetTypeName() != pGroup->GetParent()->GetTypeName()) {
                        eRet = Delete;
                    }
                }*/
            }

            if (eRet == Delete) {
                eReturn = Reconciled;
                // construct message
                CString csMsg;
                if (bFirst) {
                    bFirst = false;
                    SetupErrMsg (csErr, csMsg,pFormFile);
                }
                if (type == Group) {
                    csMsg += pGroup->GetName();
                    csMsg += _T(" deleted; not in dictionary.\n");
                }
                else {
                    if (pGroup->GetMaxLoopOccs() == 1) {
                        csMsg += pGroup->GetName();
                        csMsg += _T(" deleted; cannot be in a roster.\n");
                    }
                    else if((pGroup->GetMaxLoopOccs() > 1 && pGroup->GetParent()->GetMaxLoopOccs() > 1) &&
                            (pGroup->GetTypeName() != pGroup->GetParent()->GetTypeName())) {
                        csMsg += pGroup->GetName();
                        csMsg += _T(" deleted; this roster cannot be used here.\n");
                    }
                    else {
                        csMsg += pGroup->GetName();
                        csMsg += _T(" deleted; not in dictionary.\n");
                    }
                }
                csErr += csMsg;

                // delete this group/roster
                if (bAutoFix)
                {
                    // trying to follow what's done in CFormScrollView::DeleteSingleItem()
                    int iFormNum = pItemBase->GetFormNum();

                    CDEForm* pForm = pFormFile->GetForm(iFormNum);

                    csName = pItemBase->GetName();

                    pForm->RemoveItem (csName);
                    pFormFile->RemoveUniqueName(csName);

                    // remove the local group/roster's items before deleting the group itself!
                    // remove grup items from form

                    bool bDeleteGroup = true;
                    bool bRemoveForm  = false;
                    if(pForm->GetGroup()->GetName().CompareNoCase(csName) == 0 ) {
                        if(pFormFile->GetLevel(pForm->GetLevel())->GetNumGroups() == 1) {
                            bDeleteGroup = false;
                        }
                    }
                    pFormFile->RemoveGroupItemsFromForm(pGroup ,false);
                    for (int j=pGroup->GetNumItems()-1; j >= 0; j--)
                    {
                        //SAVY &&&08/04/2000 Release Mode bug
                        //this for now assumes groups do not contain groups
                        //and all the groups items are fields
                        /*int iForm = pGroup->GetItem(j)->GetFormNum();
                        CDEForm* pForm = pFormFile->GetForm(iForm);
                        pForm->RemoveItem(pGroup->GetItem(j)->GetName());
                        if(pForm->GetNumItems() == 0 ) {
                        //Remove this form and Renumber
                        pFormFile->RemoveForm(iForm);
                        }*/

                        /*RemoveGroupItemsFromForm(CDEGroup* pGroup)*/

                        pFormFile->RemoveUniqueName(pGroup->GetItem(j)->GetName());
                        if(!bDeleteGroup) {
                            pGroup->RemoveItem (pGroup->GetItem(j)->GetName());
                        }
                    }

                    if(bDeleteGroup && pForm->GetGroup() && pForm->GetGroup()->GetName().CompareNoCase(csName) == 0 ) {
                        //the group which we are deleting is the same as the base group of the form
                        bRemoveForm =true;
                    }

                    if(bRemoveForm && pFormFile->GetNumForms() > 1) {

                        //remove form
                        pFormFile->RemoveForm (iFormNum);
                        //My renumber forms
                        int max = pFormFile->GetNumForms();
                        for (int j = 0; j < max; j++)
                        {
                            CDEForm* pThisForm = pFormFile->GetForm(j);
                            CDEGroup* pThisGroup = pThisForm->GetGroup();
                            pThisGroup->SetFormNum(j);
                            pThisGroup->RenumberFormsAndItems(pFormFile, j);
                            pThisForm->RenumberItems (j);
                        }
                    }
                    if(bDeleteGroup) {
                        RemoveItem (csName);
                    }
                    // trying to follow what's done in CFormScrollView::DeleteWrapUp()
                    if (!bRemoveForm && !pForm->AnyKeyedFieldsOnForm()){
                        //if the form is not deleted
                        pForm->SetRecordRepeatName(_T(""));
                        pForm->GetGroup()->SetLoopingVars();
                    }

                }
            }
            else if(eRet == Reconciled) {

                // remove excess free cells
                if( type == CDEFormBase::Roster )
                {
                    CDERoster* pRoster = assert_cast<CDERoster*>(pItemBase);

                    for( size_t fc = pRoster->GetNumFreeCells() - 1; fc < pRoster->GetNumFreeCells(); --fc )
                    {
                        const CDEFreeCell& free_cell = pRoster->GetFreeCell(fc);

                        if( free_cell.GetRow() > pRoster->GetMaxLoopOccs() )
                            pRoster->RemoveFreeCell(fc);
                    }
                }

                eReturn = Reconciled;
            }
        }
    }    // end of "for (int i=iItems-1; i>=0; i--)" blk
    //if it is a roster fix the RTL flag
    CDERoster* pRoster = DYNAMIC_DOWNCAST(CDERoster,this);

    if(pRoster){
        if( pRoster->GetRightToLeft() != pFormFile->GetRTLRostersFlag()){
            if(bAutoFix){
                pRoster->SetRightToLeft(pFormFile->GetRTLRostersFlag());
            }
            CString csMsg;
            csMsg += GetName();
            csMsg += _T(" fixed roster right to left orientation.\n");
            csErr += csMsg;
        }

    }
    return eReturn;
}

// ***************************************************************************
// check group occurrences against the dictionary

eReturnType CDEGroup::ReconcileRecordOccs (CDEFormFile* pFormFile,CString& csErr,
                                           bool bSilent,
                                           bool bAutoFix,
                                           const DICT_LOOKUP_INFO& structLookup,
                                           bool* bFirst)
{
    UNREFERENCED_PARAMETER (bSilent);
    eReturnType eReturn = OK;

    int iGroupOccs = GetMaxLoopOccs();
    int iDictOccs = structLookup.pRecord->GetMaxRecs();

    if (iGroupOccs != iDictOccs)
    {
        eReturn = Reconciled;
        CString csMsg;

        if (*bFirst)
        {
            *bFirst = false;

            SetupErrMsg (csErr, csMsg,pFormFile);
        }
        //      csMsg += "  Group '";

        if (bAutoFix)
        {
            csMsg += _T("Number of occurrences in ");
            csMsg += this->GetName();
            //csMsg += "' max occurrences does not match dictionary\n";
            CString csTemp;
            csTemp.Format(_T(" changed to %d.\n"),iDictOccs);
            csMsg += csTemp;
            csErr += csMsg;

            if(iDictOccs > 1  && iGroupOccs == 1) { //Single --->Multiple transition
                if(GetItemType() != CDEFormBase::Roster){
                    csMsg += this->GetName();
                    //csMsg += "' max occurrences does not match dictionary\n";
                    csMsg += _T(" deleted;\n"); //Cannot do single to multiple transition
                    csErr += csMsg;
                    //  ASSERT(FALSE);//this should not come here it is taken care of in the group's  unknown RI reconcile
                    return Delete; //delete the group if this comes up for old stuff where the singly occuring groups have RIType as the record
                    //Confirm with SMG that she stopped doing this on Ctl+G .
                }
            }
            if(iDictOccs ==1 && GetItemType() != CDEFormBase::Roster){
                SetRIType(UnknownRI);
                SetTypeName(_T(""));
            }
            else if(GetItemType() == CDEFormBase::UnknownItem){
                SetRIType(Record);
                SetTypeName(structLookup.pRecord->GetName());
            }
            SetMaxLoopOccs (iDictOccs);
            if(GetItemType() == CDEFormBase::Roster){
                CDERoster* pRoster = assert_cast<CDERoster*>(this);
                if(iDictOccs != iGroupOccs){
                    pRoster->FinishStubTextInit();
                }
            }
        }
    }
    return eReturn;
}

// ***************************************************************************

eReturnType CDEGroup::ReconcileItemOccs (CDEFormFile*pFormFile,CString& csErr,
                                         bool bSilent,
                                         bool bAutoFix,
                                         const DICT_LOOKUP_INFO& structLookup,
                                         bool* bFirst)
{
    UNREFERENCED_PARAMETER (bSilent);
    eReturnType eReturn = OK;

    int iGroupOccs = GetMaxLoopOccs();
    int iDictOccs = GetDictOccs (structLookup.pRecord, structLookup.pItem, structLookup.iItem);

    if (iGroupOccs != iDictOccs)
    {
        eReturn = Reconciled;
        CString csMsg;

        if (*bFirst)
        {
            *bFirst = false;

            SetupErrMsg (csErr, csMsg,pFormFile);
        }

        csMsg += this->GetName();
        //csMsg += "' max occurrences does not match dictionary\n";
        csMsg += _T(" deleted; not in dictionary.\n");
        csErr += csMsg;

        if (bAutoFix)
        {
            if(iDictOccs >1  && iGroupOccs == 1) { //Single --->Multiple transition
                return Delete;
            }
            SetMaxLoopOccs(iDictOccs);
            if(GetItemType() == CDEFormBase::Roster){
                CDERoster* pRoster = assert_cast<CDERoster*>(this);
                if(iDictOccs != iGroupOccs){
                    pRoster->FinishStubTextInit();
                }
            }
        }
    }
    return eReturn;
}


//      ************************************************************************************
//      note! this guy's looping at the CDEGroup level, not CDEFormFile

void CDEGroup::BuildUniqueNL(CDEFormFile* pFF)
{
    for( int g = 0; g < GetNumItems(); ++g )
    {
        const CDEItemBase* pItem = GetItem(g);

        pFF->AddUniqueName(pItem->GetName());

        if( pItem->GetItemType() == CDEFormBase::Group || pItem->GetItemType() == CDEFormBase::Roster )
            ((CDEGroup*)pItem)->BuildUniqueNL(pFF);
    }
}


void CDEGroup::UpdatePointers (CDEGroup* parent)
{
    SetParent(parent);             // sets the Group's parent

    int max = GetNumItems();

    for (int i = 0; i < max; i++)
    {
        CDEItemBase* pItem = GetItem(i);

        CDEFormBase::eItemType eItem = pItem->GetItemType();

        if (eItem == CDEFormBase::Group || eItem == CDEFormBase::Roster)
        {
            ((CDEGroup*) pItem)->UpdatePointers (this);
        }

        else    // it's a field
        {
            pItem->SetParent(this);
        }
    }
}


void CDEGroup::RefreshAssociatedFieldText()
{

    CDEItemBase* pItem;
    CDEFormBase::eItemType        eItem;

    int i, max = GetNumItems();

    for (i = 0; i < max; i++)
    {
        pItem = GetItem(i);

        eItem = pItem->GetItemType();

        if (eItem == CDEFormBase::Group || eItem == CDEFormBase::Roster){
            ((CDEGroup*) pItem)->RefreshAssociatedFieldText();
        }

        else    // it's a field
        {
            CDEField* pField = dynamic_cast<CDEField*>(pItem);
            if(pField && pField->GetDictItem()){
                if(pField->GetFieldLabelType() == FieldLabelType::DictionaryName){
                    pField->SetLabel(pField->GetDictItem()->GetLabel());
                    pField->GetCDEText().SetText(pField->GetDictItem()->GetName());
                }
                else if(pField->GetFieldLabelType() == FieldLabelType::DictionaryLabel){
                    pField->SetLabel(pField->GetDictItem()->GetLabel());
                    pField->GetCDEText().SetText(pField->GetDictItem()->GetLabel());
                }
            }
        }
    }
}

bool CDEGroup::Compare (CDEGroup* pGroup)
{
    bool bRet = false;

    int iMax0 = GetNumItems();
    int iMax1 = pGroup->GetNumItems();

    if(iMax0 != iMax1)
        return bRet;

    if(GetRIType() != pGroup->GetRIType())
        return bRet;
    if(GetName().CompareNoCase(pGroup->GetName()) != 0 )
        return bRet;
    if(GetMaxLoopOccs() != pGroup->GetMaxLoopOccs())
        return bRet;
    if(GetRIType() != CDEFormBase::UnknownRI) {
        if(GetTypeName().CompareNoCase(pGroup->GetTypeName()) !=0 )
            return bRet;
    }
    if(GetMaxField().CompareNoCase(pGroup->GetMaxField()) !=0 )
        return bRet;

    CDEItemBase* pItem0;
    CDEItemBase* pItem1;
    CDEFormBase::eItemType    eItem0;
    CDEFormBase::eItemType    eItem1;
    for (int i = 0; i < iMax0; i++)
    {
        pItem0 = GetItem(i);
        pItem1 = pGroup->GetItem(i);

        if(pItem0->GetName().CompareNoCase(pItem1->GetName()) !=0 )
            return bRet;

        eItem0 = pItem0->GetItemType();
        eItem1 = pItem1->GetItemType();
        if(eItem0 != eItem1)
            return bRet;

        if (eItem0 == CDEFormBase::Group ||  eItem0 == CDEFormBase::Roster){

            if(!((CDEGroup*) pItem0)->Compare ((CDEGroup*)pItem1))
                return bRet;
        }

        else{// it's a field
            if(!assert_cast<CDEField*>(pItem0)->Compare(assert_cast<CDEField*>(pItem1)))
                return bRet;

        }
    }
    return true; // The groups are same
}


// i have the ptr to the item but need to know it's index within the group

int CDEGroup::GetItemIndex(const CDEItemBase* pItem) const
{
    int i, max = GetNumItems();

    CDEItemBase* pCurItem;          // doing this just so i can see the item below, when debugging

    for (i = 0; i < max; i++)
    {
        pCurItem = m_aItem [i];

        if (pCurItem == pItem)
            return i;
    }
    return NONE;
}

// i have the ptr to the item but need to know it's index within the group

int CDEGroup::GetItemIndex(const CString& sName) const
{
    for (int i = 0; i < GetNumItems(); i++)
    {
        const CDEItemBase* pCurItem = m_aItem[i];

        if (pCurItem->GetName() == sName)
            return i;
    }
    return NONE;
}

// if an item is deleted from the form (or tree i suppose), due to the nesting
// of groups, i can't keep an index to it, so pass in it's Name(and dictname
// if it's a Field to be deleted) and find it!

bool CDEGroup::RemoveItem(const CString& sItemName)
{
    for( int i = 0; i < GetNumItems(); ++i )
    {
        CDEItemBase* pItem = GetItem(i);

        if( sItemName.CompareNoCase(pItem->GetName()) == 0 )
        {
            RemoveItem(i);
            return true;
        }

        else if( pItem->GetItemType() == CDEFormBase::Group || pItem->GetItemType() == CDEFormBase::Roster )
        {
            // Recursively search children
            if( static_cast<CDEGroup*>(pItem)->RemoveItem(sItemName) )
                return true;
        }
    }

    return false;
}

bool CDEGroup::RemoveField(CDEField* pField)
{
    int iField = GetItemIndex(pField);
    if (iField != NONE)
    {
        RemoveItem(iField);
        return true;
    }

    return false;
}

/////////////////////////////////////////////////////////////////////////////

void CDEGroup::RemoveItem (int iIndex)
{
    CDEFormBase::eItemType eItem = m_aItem [iIndex]->GetItemType();

    if (eItem == CDEFormBase::Field) {
        CDEField* pField = assert_cast<CDEField*>(m_aItem[iIndex]);
        CDEBlock* pBlock = GetBlock(pField);
        if (pBlock) {
            pBlock->RemoveField();
        }
    }
    delete m_aItem[iIndex];                        // remove the obj itself
    m_aItem.erase(m_aItem.begin() + iIndex);
}

void CDEGroup::RemoveAllItems()
{
    for( CDEItemBase* pItem : m_aItem )
        delete pItem;

    m_aItem.clear();
}

/////////////////////////////////////////////////////////////////////////////
// i need this func when i'm trying to delete a form from the menu bar; i want
// to return the *first* group associated w/the given form number; also, need
// this when i want to do a drop to the current form and need to find it's
// associated group

CDEGroup* CDEGroup::GetGroupOnForm (int iFormNo)
{
    CDEGroup* pGroup = NULL;
    CDEItemBase* pItem;

    bool bFound = false;

    int i, max = GetNumItems();

    for (i = 0; i < max && !bFound; i++)
    {
        pItem = GetItem(i);

        if (pItem->GetItemType() == CDEFormBase::Group)
        {
            pGroup = (CDEGroup*) pItem;

            if (pGroup->GetFormNum() == iFormNo)
            {
                bFound = true;
            }
            else
            {
                pGroup = pGroup->GetGroupOnForm (iFormNo);

                if (pGroup != NULL)
                    bFound = true;
            }
        }
    }
    if (bFound)
        return pGroup;
    else
        return NULL;
}

const TCHAR* CDEGroup::GetRIStr() const 
{
    switch (m_eRIType)
    {
    case CDEFormBase::Record : return FRM_CMD_RECORD;
    case CDEFormBase::Item   : return FRM_CMD_ITEM;
    case CDEFormBase::SubItem: return FRM_CMD_SUBITEM;
    default                  : return FRM_CMD_UNKNOWN;
    }
}

const CString& CDEGroup::GetRecordRepeatName() const
{
    if (m_eRIType == CDEFormBase::Record)
        return m_csTypeName;
    else
        return SO::EmptyCString;
}

// i'll set it = to Unknown if i can't parse it, up to calling func to check results

void CDEGroup::SetRIType(const CString& cs)
{
    if (cs.CompareNoCase(FRM_CMD_RECORD) == 0 )       m_eRIType = CDEFormBase::Record;
    else if(cs.CompareNoCase(FRM_CMD_ITEM) == 0 )     m_eRIType = CDEFormBase::Item;
    else if(cs.CompareNoCase(FRM_CMD_SUBITEM) == 0 )  m_eRIType = CDEFormBase::SubItem;
    else                                              m_eRIType = CDEFormBase::UnknownRI;
}

void CDEGroup::SetLoopingVars(const CDictRecord* pDR)
{
    SetMaxLoopOccs(pDR->GetMaxRecs());
    SetRIType(Record);
    SetTypeName(pDR->GetName());
}

void CDEGroup::SetLoopingVars()
{
    SetMaxLoopOccs (1);
    SetRIType (UnknownRI);
    SetTypeName(_T(""));
}

// only if MaxField is NULL (has not been assigned) will i accept a value
// for Max (else signal an error condition--figure out L8R how2handle; smg)
void CDEGroup::SetMaxLoopOccs(const CString&  cs)
{
    if (!cs.IsEmpty()) {
        m_iMaxLoopOccs = _ttoi(cs); // ok to assign
    } else {
        m_iMaxLoopOccs = -2; // signal error condition
    }
}

/////////////////////////////////////////////////////////////////////////////
// return the index of the located item, else NONE if not found

int CDEGroup::FindItem(const CString& sFFName) const
{
    for( int i = 0; i < GetNumItems(); ++i )
    {
        if( sFFName.CompareNoCase(GetItem(i)->GetName()) == 0 )
            return i;
    }

    return NONE;
}

/////////////////////////////////////////////////////////////////////////////
// for each CDEField i build up in CDEGroup's Build(), come here and reconcile
// this field w/the form; that is, have the CDEForm's unique fields point to the
// same field as just created in the group
//
// CDEGroup will contain both protected and keyed fields

bool CDEGroup::FindNCopyFieldIntoForm (CDEForm* pForm, CDEField* pGroupField)
{
    int i,max = pForm->GetNumItems();

    CString paramName = pGroupField->GetName();

    CDEItemBase* pItem;

    CDEFormBase::eItemType eIT;

    bool bFound = false;

    for (i = 0; i < max && !bFound; i++)
    {
        pItem = pForm->GetItem(i);

        eIT = pItem->GetItemType();

        if (! (eIT == CDEFormBase::Field  ||         // short-circuit, if it's not a field or roster
            eIT == CDEFormBase::Roster) )

            continue;

        if (pItem->GetName() == paramName)
        {
            pForm->RemoveItem (i);  // no delete; only removes the array registry info

            delete pItem;   // the field was only a placeholder before ready to pt to group's field

            pForm->InsertItemAt (pGroupField, i);

            bFound = true;  // short-circuit the looping
        }
    }
    return bFound;
}

// by the time i've read in a [Roster] blk from the form file, i have already
// read in the [Form] blk; however, the form only created a placeholder for
// the roster, so this finds it in the form, and points it to the correct entity
//
// i'm passing in a CDERoster, but calling it a group

void CDEGroup::FinishFormInit4Rosters(CDEForm* pForm, CDEGroup* pRoster)
{
    bool bFound = false;
    CString paramName = pRoster->GetName();

    int max = pForm->GetNumItems();

    for (int i = 0; i < max && !bFound; i++)
    {
        CDEItemBase* pItem = pForm->GetItem(i);

        if (pItem->GetName() == paramName)
        {
            delete pItem;

            pForm->RemoveItem (i);

            pForm->InsertItemAt (pRoster, i);

            bFound = true;
        }
    }
}


// search recursively thru the groups; as you encounter a differently-#ed form,
// go to that form, and renumber all it's items (this must be done from the form,
// as the form contains text blks that the group doesn't see);

int CDEGroup::RenumberFormsAndItems (CDEFormFile* pFF, int iCurFormNum)
{
    // only want to renumber items on any single form once!

    int i, max = GetNumItems();

    bool bRenumberGroup = (GetFormNum() != iCurFormNum);

    if (bRenumberGroup)
    {
        // make sure we get the correct location of the form, don't rely
        // on the group's index at this point

        int curLocation = pFF->FindForm (GetFormName());

        CDEForm* pForm = pFF->GetForm (curLocation);

        pForm->RenumberItems (iCurFormNum);

        SetFormNum (iCurFormNum);       // set the current group's form #

        if (iCurFormNum < curLocation)  // ins the form into proper place in FF array
        {
            pFF->RemoveFormAt (curLocation);
            pFF->InsertFormAt (pForm, iCurFormNum);
        }
        else if(iCurFormNum > curLocation)             // making sure they're in diff locs; if same, no nd2change!
        {
            pFF->InsertFormAt (pForm, iCurFormNum);
            pFF->RemoveFormAt (curLocation);
        }
    }

    // make a recursive call on each element that's a group

    CDEGroup* pChildGroup;

    for (i = 0; i < max; i++)
    {
        if (m_aItem[i]->GetItemType() == CDEFormBase::Group)
        {
            pChildGroup = (CDEGroup*) GetItem(i);

            iCurFormNum = pChildGroup->RenumberFormsAndItems (pFF, iCurFormNum);

        }
    }

    iCurFormNum = GetFormNum()+1;

    //              ++iCurFormNum;

    return iCurFormNum;     // rtn the next # to be assigned
}


void CDEGroup::UpdateGroupFormIndices (int iStartingFormNo)
{
    int g, gMax = GetNumItems(), formNo;

    CDEItemBase* pItem;

    for (g=0; g < gMax; g++)
    {
        pItem = GetItem(g);

        if (pItem->GetItemType() == CDEFormBase::Group)
        {
            formNo = pItem->GetFormNum();

            if (formNo > iStartingFormNo)

                pItem->SetFormNum (formNo-1);

            ((CDEGroup*) pItem)->UpdateGroupFormIndices (iStartingFormNo);
        }
    }
}


eReturnType CDEGroup::OReconcile (CDEFormFile* pFormFile,
                                  CString& csErr,
                                  bool bSilent,
                                  bool bAutoFix)
{
    eReturnType eReturn = OK;
    bool bFirst = true;

    CString csDict;     // Name(symbol) of dictionary
    CString csName;     // Name(symbol) of an item
    bool bOccChange = false;
    // if group is tied to a record or item, make sure it exists
    // tell caller to delete this group if not

    eRIType gType = GetRIType();
    DICT_LOOKUP_INFO structLookup;

    if(gType == CDEFormBase::UnknownRI){
        //Check the group for items from different records and they are not from singly occuring records
        //then delete the group

        //if the group has items from same record and the record is multiple set the group's occs and RItype
        int iNumItems = this->GetNumItems();
        const CDictRecord* pRecord = NULL;
        if(iNumItems > 0 ) {
            //Get the record type for the first item
            bool bFound = false;
            BOOL bMultiple = FALSE;
            BOOL bDifferent = FALSE;

            for(int iItem =0; iItem < iNumItems; iItem++){
                CDEItemBase* pBase = GetItem(iItem);

                CDEField* pField = DYNAMIC_DOWNCAST(CDEField,pBase);
                if(pField){
                    // stuff the dictionary Name(use may have changed it in dict frame)
                    csDict = pFormFile->GetDictionary()->GetName();
                    pField->SetItemDict(csDict);
                    structLookup.csName = pField->GetItemName();
                    if(bFound) {
                        pRecord = structLookup.pRecord;
                        if(!bMultiple){
                            bMultiple = pRecord->GetMaxRecs() > 1;
                        }
                    }
                    bFound = LookupSymbol (pFormFile, csDict, structLookup);
                    if(pRecord && bFound && !bDifferent) {//Check if they are same
                        if(pRecord != structLookup.pRecord){
                            bDifferent = TRUE;
                        }
                    }
                }
            }
            if(bMultiple && bDifferent) {//if some fields are multiple and are from different records
                CString csMsg;
                SetupErrMsg (csErr, csMsg,pFormFile);
                csMsg += _T("  ");
                csMsg += this->GetName();
                //csMsg += "' has items from different records which are multiple\n";
                csMsg += _T(" deleted.\n");
                csErr += csMsg;
                return Delete ; //delete the group
            }
            else if(bMultiple && !bDifferent) {//if the items in the group are from multiply occuring record and
                //and the items are all from the same record
                ASSERT(pRecord);
                SetRIType(Record);
                SetTypeName(pRecord->GetName());
                SetMaxLoopOccs (pRecord->GetMaxRecs());
                bOccChange = true;
            }
            else if(!bMultiple) {//Make sure that the group is single
                SetMaxLoopOccs(1);
                //bOccChange = true;
            }
        }
    }

    else if(gType == CDEFormBase::Record) {
        csDict = _T("");
        structLookup.csName = GetTypeName();

        bool bFound = LookupSymbol (pFormFile, csDict, structLookup);
        if (!bFound) {
            return Delete;
        }
        if (GetRequired() != structLookup.pRecord->GetRequired()) {
            SetRequired(structLookup.pRecord->GetRequired());    //  BMD  4 May 2000
            eReturn = Reconciled;
            ReconcileRecordOccs(pFormFile, csErr, bSilent, bAutoFix, structLookup, &bFirst);
        }
        else {
            eReturn = ReconcileRecordOccs(pFormFile, csErr, bSilent, bAutoFix, structLookup, &bFirst);
        }
    }
    // &&& todo:  gsf added SubItem type 01/24/01
    else if(gType == CDEFormBase::Item || gType == CDEFormBase::SubItem)     {
        //&&& todo: fix this up!! gsf 01/18/00
        // roster based on a (multiple) item
        // tried to check m_csMaxField, but it's always ""
        //
        csDict = _T("");
        structLookup.csName = GetTypeName();
        //        structLookup.csName = GetMaxField();
        bool bFound = LookupSymbol (pFormFile, csDict, structLookup);

        if (!bFound)
            return Delete;

        eReturn = ReconcileItemOccs(pFormFile,csErr, bSilent, bAutoFix, structLookup, &bFirst);
    }

    // ****************************
    // now check all items in this group
    // this loop is "backwards", because we may be deleting things
    // ****************************

    int iItems = GetNumItems();

    for (int i=iItems-1; i>=0; i--) {
        CDEItemBase* pItemBase = GetItem(i);
        CDEFormBase::eItemType type = pItemBase->GetItemType();

        // Field items

        if (type == CDEFormBase::Field)      {
            CDEField* pField = assert_cast<CDEField*>(pItemBase);

            // gsf 23-mar-00
            // assume only one dictionary per forms file
            // stuff the dictionary Name(use may have changed it in dict frame)
            csDict = pFormFile->GetDictionary()->GetName();

            const CDictItem* pDictItem = pFormFile->GetDictionary()->LookupName<CDictItem>(pField->GetItemName());
            if(pDictItem){
                pField->SetLabel(pDictItem->GetLabel());
                pField->SetDictItem(pDictItem);
            }

            pField->SetItemDict(csDict);
            //            csDict = pField->GetItemDict();
            // gsf 23-mar-00 end

            structLookup.csName = pField->GetItemName();

            bool bFound = LookupSymbol (pFormFile, csDict, structLookup);

            if (!bFound)
            {
                eReturn = Reconciled;

                // construct message
                CString csMsg;

                if (bFirst){
                    bFirst = false;
                    SetupErrMsg (csErr, csMsg,pFormFile);
                }

                csMsg += _T("  ");
                csMsg += pItemBase->GetName();
                csMsg += _T(" deleted; not in dictionary.\n");
                csErr += csMsg;

                // delete this field

                if (bAutoFix)
                {
                    // trying to follow what's done in CFormScrollView::DeleteSingleItem()

                    // it's a free-standing field on the form
                    int iFormNum = pField->GetFormNum();
                    CDEForm* pForm = pFormFile->GetForm (iFormNum);

                    CString sFieldName = pField->GetName();
                    if(pForm){
                        pForm->RemoveItem (pField);
                    }
                    pFormFile->RemoveUniqueName(structLookup.csName);
                    RemoveField (pField);    // removes the field from the group

                    //This is a fix when field name is not same as the item name
                    if(sFieldName.CompareNoCase(structLookup.csName) != 0) {
                        if(pForm){
                            pForm->RemoveItem(sFieldName);
                        }
                        pFormFile->RemoveUniqueName(sFieldName);
                        RemoveField (pField);         // removes the field from the group
                    }
                    // trying to follow what's done in CFormScrollView::DeleteWrapUp()
                    if (pForm && !pForm->AnyKeyedFieldsOnForm()) {
                        pForm->SetRecordRepeatName(_T(""));
                        SetLoopingVars();
                    }
                }
            }
        }

        //  Group/Roster items

        else if(type == CDEFormBase::Group) { // savy, handle for Roster
            CDEGroup* pGroup = (CDEGroup*) pItemBase;
            //            eItemType eType = pGroup->GetItemType();
            eReturnType eRet = pGroup->OReconcile(pFormFile, csErr, bSilent, bAutoFix);

            if (eRet == Delete) {
                eReturn = Reconciled;

                // construct message
                CString csMsg;
                if (bFirst) {
                    bFirst = false;
                    SetupErrMsg (csErr, csMsg,pFormFile);
                }

                //csMsg += "Group ";
                csMsg += pGroup->GetName();
                csMsg += _T(" deleted; not in dictionary.\n");
                csErr += csMsg;

                // delete this group/roster
                if (bAutoFix){
                    // trying to follow what's done in CFormScrollView::DeleteSingleItem()
                    int iFormNum = pItemBase->GetFormNum();

                    CDEForm* pForm = pFormFile->GetForm(iFormNum);
                    if(!pForm)
                        break;
                    CString sFormName = pForm->GetName();
                    csName = pItemBase->GetName();
                    pForm->RemoveItem (csName);
                    pFormFile->RemoveUniqueName(csName);

                    //BEGIN PASTE
                    pFormFile->RemoveGroupItemsFromForm(pGroup);

                    for (int j=0; j < pGroup->GetNumItems(); j++) {
                        pFormFile->RemoveUniqueName(pGroup->GetItem(j)->GetName());
                    }
                    RemoveItem (csName);
                    // remove the local group/roster's items before deleting the group itself!
                    //If the group has a knownRI Type then delete the form corresponding to this one
                    pForm = NULL; //get the form again 'cos it may be removed in "RemoveGroupItemsFromForm" call above
                    //To fix Selma's bug
                    int iForm = pFormFile->FindForm(sFormName);
                    if(iForm != NONE) {
                        pForm = pFormFile->GetForm(iForm);
                        if(pForm &&  pForm->GetNumItems() == 0 ) {
                            pFormFile->RemoveForm(iFormNum);
                        }
                    }
                }
            }
            else if(eRet == Reconciled)
            {
                eReturn = Reconciled;
            }
        }
    }
    if(bOccChange && eReturn == OK){
        eReturn = Reconciled;
        bOccChange = false;
    }
    return eReturn;
}


void CDEGroup::UpdateFlagsNFonts(const CDEFormFile& form_file)
{
    auto update_text = [&](CDEText& text)
    {
        text.SetUseDefaultFont(text.GetFont() == form_file.GetDefaultTextFont());
    };

    auto update_text_set = [&](CDETextSet& text_set)
    {
        for( CDEText& text : text_set.GetTexts() )
            update_text(text);
    };

    if( this->GetItemType() == CDEFormBase::Roster )
    {
        CDERoster* pRoster = assert_cast<CDERoster*>(this);

        // free cell text
        for( CDEFreeCell& free_cell : pRoster->GetFreeCells() )
            update_text_set(free_cell.GetTextSet());

        for(int iCol = 0 ; iCol < pRoster->GetNumCols(); iCol++){
            CDECol* pCol = pRoster->GetCol(iCol);
            //Do the header
            update_text(pCol->GetHeaderText());

            //Do the col texts
            update_text_set(pCol->GetColumnCell().GetTextSet());
        }

        update_text_set(pRoster->GetStubTextSet());
    }

    for (int i = 0; i < GetNumItems(); i++) {
        CDEItemBase* pItem = GetItem(i);
        eItemType eItem = pItem->GetItemType();
        if (eItem == CDEFormBase::Group || eItem == CDEFormBase::Roster){
            ((CDEGroup*) pItem)->UpdateFlagsNFonts(form_file);
        }

        else{// it's a field
            CDEField* pField = DYNAMIC_DOWNCAST(CDEField,pItem);
            if(pField){
                pField->SetFont(form_file.GetFieldFont());
                update_text(pField->GetCDEText());
            }
        }
    }
}


/////////////////////////////////////////////////////////////////////////////////
//
//      void CDEGroup::ChangeDName(const CDataDict& dictionary)
//
/////////////////////////////////////////////////////////////////////////////////
void CDEGroup::ChangeDName(const CDataDict& dictionary)
{
    int max = GetNumItems();

    for( int i = 0; i < max; i++)
    {
        CDEItemBase* pItem = GetItem(i);
        CDEFormBase::eItemType eItem = pItem->GetItemType();

        if (eItem == CDEFormBase::Group || eItem == CDEFormBase::Roster)
        {
            ((CDEGroup*)pItem)->ChangeDName(dictionary);
        }

        else    // it's a field
        {
            CDEField* pField = DYNAMIC_DOWNCAST(CDEField,pItem);
            if (pField) {//ignoring blocks as they do not have associated dictionary item like groups
                pField->SetItemDict(dictionary.GetName());
            }
        }
    }
}


/////////////////////////////////////////////////////////////////////////////////
//
//      void CDEGroup::ChangeGName(const CString& sOldName, const CString& sNewName)
//
/////////////////////////////////////////////////////////////////////////////////
void CDEGroup::ChangeGName(const CString& sOldName, const CString& sNewName)
{

    CDEItemBase* pItem;
    CDEFormBase::eItemType        eItem;

    int i, max = GetNumItems();
    if(GetRIType() == CDEFormBase::Record) {
        CString sString = GetTypeName();
        if(sString.CompareNoCase(sOldName) ==0 ){
            SetTypeName(sNewName);
        }
    }

    for (i = 0; i < max; i++)
    {
        pItem = GetItem(i);

        eItem = pItem->GetItemType();

        if (eItem == CDEFormBase::Group || eItem == CDEFormBase::Roster){

            ((CDEGroup*) pItem)->ChangeGName(sOldName,sNewName);
        }

        else    // it's a field
        {

        }
    }
}


/////////////////////////////////////////////////////////////////////////////////
//
//      void CDEGroup::ClearUsed()
//
/////////////////////////////////////////////////////////////////////////////////
void CDEGroup::ClearUsed()
{
    CDEItemBase* pItem;
    CDEFormBase::eItemType        eItem;

    int i, max = GetNumItems();
    SetUsed(false);
    for (i = 0; i < max; i++){
        pItem = GetItem(i);
        pItem->SetUsed(false);
        eItem = pItem->GetItemType();
        if (eItem == CDEFormBase::Group || eItem == CDEFormBase::Roster)
            ((CDEGroup*) pItem)->ClearUsed();

        else    // it's a field
        {
            pItem->SetUsed(false);

        }
    }
}


/////////////////////////////////////////////////////////////////////////////////
//
//      void CDEGroup::CheckGroups (CDEFormFile* pFormFile)
//
/////////////////////////////////////////////////////////////////////////////////
void CDEGroup::CheckGroups (CDEFormFile* pFormFile)
{
    CString csDict;     // name (symbol) of dictionary
    CString csName;     // name (symbol) of an item

    csDict = pFormFile->GetDictionary()->GetName();

    // if group is tied to a record or item, make sure it exists
    // tell caller to delete this group if not

    eRIType gType = GetRIType();
    DICT_LOOKUP_INFO structLookup;

    if(gType == CDEFormBase::UnknownRI) {
        //we dont care  for unknownRI
        SetUsed(true);
    }
    else if(gType == CDEFormBase::Record){
        csDict = _T("");
        structLookup.csName = GetTypeName();

        bool bFound = LookupSymbol (pFormFile, csDict, structLookup);
        if (bFound) {
            SetUsed(true);
        }
    }
    else if(gType == CDEFormBase::Item || gType == CDEFormBase::SubItem)         {
        csDict = _T("");
        structLookup.csName = GetTypeName();
        bool bFound = LookupSymbol (pFormFile, csDict, structLookup);

        if (bFound){
            SetUsed(true);
            pFormFile->SetUsed(*structLookup.pItem, true);
            const CDictItem* pDItem = structLookup.pItem;
            if(pDItem->GetItemType() == ItemType::Item) {//Set the sub items to
                int iLevel = structLookup.iLevel;
                int iRecord =structLookup.iRecord;
                int iItem = structLookup.iItem;
                int iNumSubitems = pFormFile->GetNumDictSubitems(structLookup.pRecord,pDItem);
                const CDictItem* pSItem;
                for (int iS = 0 ; iS < iNumSubitems ; iS++)  {
                    pSItem = pFormFile->GetDictionary()->GetLevel(iLevel).GetRecord(iRecord)->GetItem(iItem+1+iS);
                    ASSERT(pSItem);
                    ASSERT(pSItem->GetItemType() == ItemType::Subitem);
                    pFormFile->SetUsed(*pSItem, true);
                }
            }
            else if(pDItem->GetItemType() == ItemType::Subitem) {//if the Item type is subitem then set the parent item to used
                int iIndex = pFormFile->GetDictItemParentIndex(structLookup.pRecord,pDItem);
                pFormFile->SetUsed(*structLookup.pRecord->GetItem(iIndex), true);
            }
        }
    }

    // ****************************
    // now check all items in this group
    // this loop is "backwards", because we may be deleting things
    // ****************************

    int iItems = GetNumItems();

    for (int i=iItems-1; i>=0; i--) {
        CDEItemBase* pItemBase = GetItem(i);
        CDEFormBase::eItemType type = pItemBase->GetItemType();

        // Field items

        if (type == CDEFormBase::Field) {
            CDEField* pField = assert_cast<CDEField*>(pItemBase);

            pField->SetItemDict(csDict);
            structLookup.csName = pField->GetItemName();

            bool bFound = LookupSymbol (pFormFile, csDict, structLookup);
            if (bFound) {
                pField->SetUsed(true);
                pFormFile->SetUsed(*structLookup.pItem, true);
                //Get number of subitems if the item type ITEM
                const CDictItem* pDItem = structLookup.pItem;
                if(pDItem->GetItemType() == ItemType::Item) {//Set the sub items to
                    int iLevel = structLookup.iLevel;
                    int iRecord =structLookup.iRecord;
                    int iItem = structLookup.iItem;
                    int iNumSubitems = pFormFile->GetNumDictSubitems(structLookup.pRecord,pDItem);
                    const CDictItem* pSItem;
                    for (int iS = 0 ; iS < iNumSubitems ; iS++)  {
                        pSItem = pFormFile->GetDictionary()->GetLevel(iLevel).GetRecord(iRecord)->GetItem(iItem+1+iS);
                        ASSERT(pSItem);
                        ASSERT(pSItem->GetItemType() == ItemType::Subitem);
                        pFormFile->SetUsed(*pSItem, true);
                    }
                }
                else if(pDItem->GetItemType() == ItemType::Subitem) {//if the Item type is subitem then set the parent item to used
                    int iIndex = pFormFile->GetDictItemParentIndex(structLookup.pRecord,pDItem);
                    pFormFile->SetUsed(*structLookup.pRecord->GetItem(iIndex), true);
                }
            }

        }       // end "if (type == Field)" stmt
        else if(type == CDEFormBase::Group || type == CDEFormBase::Roster){
            CDEGroup* pGroup = (CDEGroup*) pItemBase;
            pGroup->CheckGroups(pFormFile);
        }
    }
}


void CDEGroup::SetItemSubItemFlags(CDEFormFile* pFormFile, const CDictItem* pDictItem ,bool bFlag /*=true*/)
{
    CString csDict = pFormFile->GetDictionary()->GetName();

    bool bFound = false;
    DICT_LOOKUP_INFO structLookup;

    structLookup.csName = pDictItem->GetName();
    bFound = LookupSymbol (pFormFile, csDict, structLookup);

    if (bFound){
        pFormFile->SetUsed(*pDictItem, bFlag);
        const CDictItem* pDItem = pDictItem;
        if(pDItem->GetItemType() == ItemType::Item) {//Set the sub items to
            int iLevel = structLookup.iLevel;
            int iRecord =structLookup.iRecord;
            int iItem = structLookup.iItem;
            int iNumSubitems = pFormFile->GetNumDictSubitems(structLookup.pRecord,pDItem);
            const CDictItem* pSItem;
            for (int iS = 0 ; iS < iNumSubitems ; iS++)  {
                pSItem = pFormFile->GetDictionary()->GetLevel(iLevel).GetRecord(iRecord)->GetItem(iItem+1+iS);
                ASSERT(pSItem);
                ASSERT(pSItem->GetItemType() == ItemType::Subitem);
                pFormFile->SetUsed(*pSItem, bFlag);
            }
        }
        else if(pDItem->GetItemType() == ItemType::Subitem) {//if the Item type is subitem then set the parent item to used
            int iIndex = pFormFile->GetDictItemParentIndex(structLookup.pRecord,pDItem);
            pFormFile->SetUsed(*structLookup.pRecord->GetItem(iIndex), bFlag);
        }
    }
}


bool CDEGroup::SearchInAllForms(CDEFormFile* pFF, CDEField *pField )
{
    for (int i = 0; i < pFF->GetNumForms(); i++)
    {
        if (FindNCopyFieldIntoForm(pFF->GetForm(i),pField))
        {
            SetFormName(pFF->GetForm(i)->GetName());
            SetFormNum(i);
            pFF->GetForm(i)->SetGroup(this);
            return true;
        }
    }
    return false;
}


bool CDEGroup::SearchRosterIntoForms(CDEFormFile* pFF, CDEForm*pForm, CDERoster*pRoster)
{
    int i,max = pForm->GetNumItems();

    CString paramName = pRoster->GetName();

    CDEItemBase* pItem;

    CDEFormBase::eItemType eIT;
    for (i = 0; i < max; i++)
    {
        pItem = pForm->GetItem(i);

        eIT = pItem->GetItemType();

        if (! (eIT == CDEFormBase::Field  ||         // short-circuit, if it's not a field or roster
            eIT == CDEFormBase::Roster) )
            continue;

        if (pItem->GetName() == paramName)
        {
            //                      if (eIT != Roster)
            //                              continue;
            delete pItem;   // the field was only a placeholder before ready to pt to group's field
            pForm->RemoveItem (i);  // no delete; only removes the array registry info
            pForm->InsertItemAt (pRoster, i);
            return true;
        }
    }

    for (int form = 0; form < pFF->GetNumForms(); form++)
    {
        CDEForm* pFormTemp = pFF->GetForm(form);
        max = pFormTemp->GetNumItems();
        for (i = 0; i < max ; i++)
        {
            pItem = pFormTemp->GetItem(i);

            eIT = pItem->GetItemType();

            if (! (eIT == CDEFormBase::Field  ||         // short-circuit, if it's not a field or roster
                eIT == CDEFormBase::Roster) )
                continue;

            if (pItem->GetName() == paramName)
            {
                //                              if (eIT != Roster)
                //                                      continue;
                delete pItem;   // the field was only a placeholder before ready to pt to group's field
                pFormTemp->RemoveItem (i);      // no delete; only removes the array registry info
                pFormTemp->InsertItemAt (pRoster, i);
                SetFormNum(form);
                SetFormName(pFormTemp->GetName());
                pFormTemp->SetGroup(this);
                return true;
            }
        }

    }
    return false;
}


/////////////////////////////////////////////////////////////////////////////////
//
//      void CDEGroup::SetMaxFieldPointer (CDEFormFile* pFormFile)
//
/////////////////////////////////////////////////////////////////////////////////
void CDEGroup::SetMaxFieldPointer(CDEFormFile* pFormFile)
{
    if(!m_csMaxField.IsEmpty() && this->GetMaxLoopOccs() > 1){
        const CDictItem* pItem = nullptr;
        if( pFormFile->GetDictionary() != nullptr ) {
            pItem = pFormFile->GetDictionary()->LookupName<CDictItem>(m_csMaxField);
        }
        if(pItem){
            this->SetMaxDEField(pItem);
        }
    }
    CDEItemBase* pItem;
    CDEFormBase::eItemType eItem;

    int i, max = GetNumItems();

    for (i = 0; i < max; i++) {
        pItem = GetItem(i);
        eItem = pItem->GetItemType();
        if (eItem == CDEFormBase::Group || eItem == CDEFormBase::Roster){
            ((CDEGroup*) pItem)->SetMaxFieldPointer(pFormFile);
        }
        else {   // it's a field
            //do nothing //   pItem->SetParent(this);

        }
    }
}


std::vector<CDEField*> CDEGroup::GetBlockFields(const CDEBlock& form_block) const
{
    std::vector<CDEField*> fields;
    int blockIndex = GetItemIndex(&form_block);
    ASSERT(blockIndex != NONE);
    for (int iFld = 0; iFld < form_block.GetNumFields(); ++iFld)
    {
        ASSERT(GetItem(blockIndex + iFld + 1)->GetItemType() == CDEFormBase::Field);
        fields.emplace_back(assert_cast<CDEField*>(GetItem(blockIndex + iFld + 1)));
    }

    return fields;
}


const CDEBlock* CDEGroup::GetBlock(CDEField* pField) const
{
    const int fieldIndex = GetItemIndex(pField);
    return GetBlockForFieldAt(fieldIndex);
}


CDEBlock* CDEGroup::GetBlock(CDEField* pField)
{
    // This looks ugly but the alternative is duplicating code between
    // const and non-const versions of the function.
    return const_cast<CDEBlock*>(static_cast<const CDEGroup &>(*this).GetBlock(pField));
}


const CDEBlock* CDEGroup::GetBlockForFieldAt(int fieldIndex) const
{
    for (int i = fieldIndex - 1; i >= 0; --i)
    {
        if (GetItem(i)->GetItemType() == CDEFormBase::Block)
        {
            CDEBlock* pBlock = static_cast<CDEBlock*>(GetItem(i));
            if (fieldIndex - i <= pBlock->GetNumFields())
                return pBlock;
            else
                return nullptr;
        }
    }

    return nullptr;
}


CDEBlock* CDEGroup::GetBlockForFieldAt(int fieldIndex)
{
    // This looks ugly but the alternative is duplicating code between
    // const and non-const versions of the function.
    return const_cast<CDEBlock*>(static_cast<const CDEGroup &>(*this).GetBlockForFieldAt(fieldIndex));
}


bool CDEGroup::Build (CSpecFile& frmFile, CDEFormFile* pFF, bool bSilent /* = false */, CDEForm* pForm /*= NULL*/)
{
    CString csCmd, csArg;
    bool bDone = false;
    bool bRtnVal = true;
    eItemType eItem=UnknownItem;

    int ln = frmFile.GetLineNumber();
    while (!bDone && frmFile.GetLine(csCmd, csArg) == SF_OK) {

        ASSERT (csCmd.GetLength() > 0);
        if (csCmd[0] == '[')  {
            if (csCmd.CompareNoCase(HEAD_ENDGROUP) == 0 )  {
                // "[EndGroup]", bail
                bDone = true;
            }
            else if( csCmd.CompareNoCase(HEAD_GROUP) == 0 )  {
                // new sub "[Group]"
                CDEGroup* pGroup = new CDEGroup();
                if (pGroup->Build(frmFile, pFF))  {
                    pGroup->SetParent(this);
                    AddItem(pGroup);
                }
                else  {         // prob occurred
                    bRtnVal = false;
                    bDone = true;
                }
            }
            else  {

                if (csCmd.CompareNoCase(HEAD_GRID)==0)  {
                    eItem = CDEFormBase::Roster;
                }
                else if (csCmd.CompareNoCase(HEAD_FIELD)==0)  {
                    eItem = Field;
                }
                else if (csCmd.CompareNoCase(HEAD_BLOCK) == 0) {
                    eItem = Block;
                }
                if (eItem == CDEFormBase::Field ||eItem == CDEFormBase::Roster || eItem == CDEFormBase::Block)  {
                    if ((pFF->GetForm (GetFormNum()) == NULL) &&(pForm ==NULL))  {
                        ErrorMessage::Display(FormatText(_T("Line #%d: [Group]'s \"Form=\" entry missing or refers to non-existent form."), ln));
                        return false;       // nd to bail right away
                    }
                    if(eItem == CDEFormBase::Roster)  {
                        CDERoster* pRoster = new CDERoster();
                        pRoster->SetRightToLeft(pFF->GetRTLRostersFlag());
                        if (pRoster->Build(frmFile))  {
                            // if the build went ok
                            if (SearchRosterIntoForms(pFF,pForm,pRoster)) {
                                pRoster->SetParent(this);
                                pRoster->SetFormNum(GetFormNum());
                                pForm = pFF->GetForm(GetFormNum());
                                AddItem(pRoster);      // add the item to the page
                                //      FinishFormInit4Rosters (pForm, pRoster);
                                if(GetCSProVersionNumeric(pFF->GetVersion()) <= 6.1){
                                    //for version 6.1 and below, force the roster's header text to field text when there is only one field in the col
                                    for(int iCol =0; iCol < pRoster->GetNumCols(); iCol++){
                                        if(pRoster->GetCol(iCol)->GetNumFields()==1){
                                            CDEField* pField = pRoster->GetCol(iCol)->GetField(0);
                                            pField->GetCDEText().SetLabel(pRoster->GetCol(iCol)->GetHeaderText().GetLabel());
                                        }
                                    }
                                }
                            }
                            else
                            {
                                bDone = true;
                                bRtnVal = false;        // prob occurred
                                delete pRoster;
                            }
                        }
                        else  {
                            bDone = true;
                            bRtnVal = false;    // prob occurred
                            delete pRoster;
                        }
                    }
                    else if(eItem == CDEFormBase::Field)  {
                        CDEField* pField = new CDEField();
                        int localLN = frmFile.GetLineNumber();
                        if (pField->Build(frmFile))  {
                            pField->SetParent(this);
                            AddItem(pField);           // add the item to the group
                            //                          int iFN = GetFormNum();//pField->GetFormNum(); Changed by chirag for copy pasrte stuff
                            if (pForm == NULL)  {
                                ErrorMessage::Display(FormatText(_T("Line #%1: [Field]'s \"Form=\" entry missing or refers to non-existent form."), localLN));
                                return false;       // nd to bail right away
                            }

                            if (!FindNCopyFieldIntoForm (pForm, pField))  {
                                //Search in all forms and reset.
                                if (!SearchInAllForms(pFF,pField))
                                {
                                    ErrorMessage::Display(FormatText( _T("Line %d: [Group]'s field \"%s\" not found on the form"), localLN, (LPCTSTR)pField->GetName()));
                                    RemoveItem (pField->GetName());
                                }
                                else
                                {
                                    pForm = pFF->GetForm(GetFormNum());
                                }
                            }
                        }
                        else  {
                            bDone = true;
                            bRtnVal = false;    // prob occurred
                            delete pField;
                        }
                    }
                    else if (eItem == CDEFormBase::Block) {
                        CDEBlock* pBlock = new CDEBlock();
                        pBlock->SetFormNum(GetFormNum());
                        pBlock->SetParent(this);

                        if (pBlock->Build(frmFile, bSilent)) {
                            InsertItemAt(pBlock, pBlock->GetSerializedPositionInParentGroup());
                        }
                        else {
                            bDone = true;
                            bRtnVal = false;    // prob occurred
                            delete pBlock;
                        }
                    }
                }
            }
        }
        else if( csCmd.CompareNoCase(FRM_CMD_NAME) == 0 )
            SetName(csArg);
        else if( csCmd.CompareNoCase(FRM_CMD_LABEL) == 0 )
            SetLabel(csArg);
        else if( csCmd.CompareNoCase(_T("Required")) == 0 )
            SetRequired(TEXT_TO_BOOL(csArg));
        else if( csCmd.CompareNoCase(_T("Type")) == 0 )
            SetRIType (csArg);
        else if( csCmd.CompareNoCase(_T("TypeName")) == 0 )
            SetTypeName(csArg);
        else if( csCmd.CompareNoCase(_T("Max")) == 0 )
            SetMaxLoopOccs(csArg);
        else if( csCmd.CompareNoCase(_T("MaxField")) == 0 )
            SetMaxField (csArg);
        else if( csCmd.CompareNoCase(FRM_CMD_FORMNUM) == 0 ) {
            if(pForm){//Added this check to fix trevors bug
                SetFormNum(pFF->GetNumForms());
                SetFormName( pForm->GetName() );
            }
            else
            {
                SetFormNum (csArg);
                if (pForm== NULL)
                {
                    pForm = pFF->GetForm(GetFormNum());
                    SetFormName(pForm->GetName());
                }
            }
        }
        // ignore attributes no longer used
        else if( csCmd.CompareNoCase(_T("LogicControl")) == 0 ) {
        }
        else {
            // Incorrect attribute
            if (!bSilent) {
                ErrorMessage::Display(FormatText(_T("Line#%d: Incorrect [%s] attribute\n\n%s"), (int)frmFile.GetLineNumber(), HEAD_GROUP, (LPCTSTR)csCmd));
            }
            bRtnVal = false;    // keep parsing, don't set bDone yet
        }
    }
    if (bRtnVal)  {   // then we're good so far, do a few more checks
        if (GetName() == sEmptyName) {
            // if the group was missing it's Name= block, provide one
            ErrorMessage::Display(FormatText(_T("Line #%d: [Group] missing \"Name=\" entry--a unique name was provided."), ln));
            SetName(pFF->CreateUniqueName(_T("GROUP"), false));
        }

        int i = GetFormNum();
        if (i == NONE)  {
            ErrorMessage::Display(FormatText(_T("Line #%d: [Group] entry must have a \"Form=\" entry."), ln));
            bRtnVal = false;
        }
        else {
            //CDEForm* pForm = pFF->GetForm (i);
            if (pForm == NULL)  {  // form doesn't exist
                ErrorMessage::Display(FormatText(_T("Line #%d: [Group]'s \"Form=\" entry refers to a non-existent form (%d)."), ln, i));
                return false;   // bail right away
            }
            else {
                pForm->SetGroup (this);
            }
        }
    }

    return bRtnVal;
}


/*      //////////////////////////////////////////////////////////////////////////

[1] first, write out all the header info related to the [Group] blk
[2] next, write out all [Group], [Field], and [Roster] (or [Grid] blks from
the form using full notation

////////////////////////////////////////////////////////////////////////// */

void CDEGroup::Save(CSpecFile& frmFile) const
{
    //      [1] write out the hdr stuff

    frmFile.PutLine(HEAD_GROUP);
    frmFile.PutLine(_T("Required"),  BOOL_TO_TEXT(GetRequired()) );
    frmFile.PutLine(FRM_CMD_NAME, GetName());
    frmFile.PutLine(FRM_CMD_LABEL, GetLabel());
    frmFile.PutLine(FRM_CMD_FORMNUM, GetFormNum()+1);

    if (GetRIType() != CDEFormBase::UnknownRI)
    {
        frmFile.PutLine(_T("Type"), GetRIStr());
        frmFile.PutLine(_T("TypeName"), GetTypeName());
    }
    frmFile.PutLine(_T("Max"), GetMaxLoopOccs());

    if ( !SO::IsBlank(GetMaxField()) )         // ditto; if no val assigned,
        frmFile.PutLine(_T("MaxField"), GetMaxField());   // don't try2print!

    //      [2] do the full saves

    int i, max = GetNumItems();
    CDEItemBase* pItem;

    for (i = 0 ; i < max; i++)              // items in a [Group] shld not contain [Text] blks (unless
    {                                                               // of course they are associated/belong to a [Field] blk
        pItem = GetItem(i);

        frmFile.PutLine(_T("  "));             // blank line to sep the groups

        if (pItem->GetItemType() != CDEFormBase::Block) { // Blocks written out later
            pItem->Save(frmFile);   // smg: it will know which save (roster/field/group) to call
        }
    }

    // Write out the blocks
    for (int iBlock = 0; iBlock < GetNumItems(); ++iBlock) {
        if (GetItem(iBlock)->GetItemType() == CDEFormBase::Block) {
            auto pBlock = static_cast<CDEBlock*>(GetItem(iBlock));
            pBlock->Save(frmFile);
        }
    }

    frmFile.PutLine(HEAD_ENDGROUP);
    frmFile.PutLine(_T("  "));
}


void CDEGroup::serialize(Serializer& ar) // 20121114
{
    CDEItemBase::serialize(ar);

    if (!isA(Group) ||
        ((ar.IsSaving() && !FormSerialization::isRepeated(this, ar)) ||
        (ar.IsLoading() && !FormSerialization::CheckRepeated(this, ar))))
    {
        int iGetNumItems = GetNumItems();

        ar & m_bRequired;

        ar.IgnoreUnusedVariable<bool>(Serializer::Iteration_7_6_000_1); // m_bLogicControl

        ar.SerializeEnum(m_eRIType)
           & m_csTypeName
           & m_iMaxLoopOccs
           & m_csMaxField
           & m_sFormName;

        if (ar.IsSaving()) {
            // Don't save the blocks in the item list, they get serialized later
            //If the group has items, write out the non block items.
            //if there are no Nonblock items write out zero as the read will still need it.
            int numNonBlockItems = 0;
            if(iGetNumItems > 0) {
                numNonBlockItems = std::count_if(m_aItem.cbegin(), m_aItem.cend(),
                                                 [](const CDEItemBase* pItem) { return pItem->GetItemType() != CDEFormBase::Block; });
            }
            ar & numNonBlockItems;
        }
        else {
            ar & iGetNumItems;
        }

        ar & m_iTotalOccs
           & m_iCurOccurrence
           & m_iDataOccs;

        for( int i = 0; i < iGetNumItems; i++ )
        {
            if (ar.IsSaving())
            {
                CDEFormBase::eItemType itemType = GetItem(i)->GetItemType();
                if (itemType != CDEFormBase::Block) {
                    ar.SerializeEnum(itemType);
                    FormSerialization::SerializeItem(ar, GetItem(i));
                }
            }

            else
            {
                CDEItemBase* pItem = FormSerialization::getType_createItem(ar);

                try
                {
                    FormSerialization::SerializeItem(ar, pItem);
                }
                catch( const FormSerialization::RepeatedItemException& e )
                {
                    delete pItem;
                    pItem = assert_cast<CDEItemBase*>(FormSerialization::getItem(e.GetPosition()));
                }

                pItem->SetParent(this);
                AddItem(pItem);
            }
        }

        // Serialize the blocks
        if( ar.IsSaving() )
        {
            //If the group has items, write out the block items.
            //if there are no block items write out zero as the read will still need it.
            int numBlocks = 0;
            if (iGetNumItems > 0) {
                numBlocks = std::count_if(m_aItem.cbegin(), m_aItem.cend(),
                                          [](const CDEItemBase* pItem) { return pItem->GetItemType() == CDEFormBase::Block; });
            }
            ar << numBlocks;

            for (int i = 0; i < iGetNumItems; i++)
            {
                CDEFormBase::eItemType itemType = GetItem(i)->GetItemType();
                if (itemType == CDEFormBase::Block) {
                    CDEBlock* pBlock = assert_cast<CDEBlock*>(GetItem(i));
                    ar << *pBlock;
                }
            }

        }
        else
        {
            int numBlocks;
            ar >> numBlocks;

            for (int i = 0; i < numBlocks; i++)
            {
                CDEBlock* pBlock = new CDEBlock();
                pBlock->SetParent(this);

                ar >> *pBlock;

                InsertItemAt(pBlock, pBlock->GetSerializedPositionInParentGroup());
            }
        }
    }
}
