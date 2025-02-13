//***************************************************************************
//  Description:
//       Data Entry application classes implementation
//
//  History:    Date       Author     Comment
//              -----------------------------
//              02 Feb 98   gsf/smg   Created
//
//***************************************************************************

#include "StdAfx.h"
#include "FormFile.h"
#include "DragOptions.h"
#include "RenDlg.h"
#include <zToolsO/Serializer.h>
#include <zUtilO/AppLdr.h>
#include <zUtilO/Versioning.h>
#include <zUtilF/ProgressDlg.h>
#include <zMessageO/Messages.h>


IMPLEMENT_DYNAMIC(CDEFormFile, CDEFormBase)

bool bShowFormSelDlg = true;

constexpr int MIN_NUM_ROSTER_ROWS = 11;  // 10 fields + 1 for stub

constexpr int rowOffset = 30;

constexpr LONG MaxScrSz = 300;


/////////////////////////////////////////////////////////////////////////////
// a form file can contain 1+ forms

void CDEFormFile::BaseConstructor()
{
    m_bDictOrder = true;
    m_bPathOn = false;

    m_bRTLRosters = false;

    m_fieldFont = PortableFont::FieldDefault;
    m_defaultTextFont = PortableFont::TextDefault;
}

CDEFormFile::CDEFormFile()
    :   m_csVersion(CSPRO_VERSION)
{
    BaseConstructor();
}

// this func is invoked by copy constructor and operator=; code's the same,
// so wanted to put in same place to ensure they stay the same!!

void CDEFormFile::CopyFF(const CDEFormFile& ff)
{
    RemoveAllForms();       // remove all associated pages
    RemoveAllLevels();      // Remove all levels too

    // [0] CDEFormBase-level vars

    SetName         (ff.GetName());
    SetLabel        (ff.GetLabel());
    // dims are applicable for a form file

    // [1] set CDEFormFile-level vars

    IsPathOn                (ff.IsPathOn());
    SetVersion              (ff.GetVersion());
    SetDictionaryName       (ff.GetDictionaryName());
    SetDictionaryFilename   (ff.GetDictionaryFilename());

    m_fieldFont = ff.m_fieldFont;
    m_defaultTextFont = ff.m_defaultTextFont;

    m_fieldColors = ff.m_fieldColors;

    // [2] now do all the array stuff

    int iMax = ff.GetNumLevels();               // then the actual levels and all their groups
    CDELevel* pNewLevel;
    CDELevel* pOldLevel;

    for (int i = 0; i < iMax; i++)
    {
        pOldLevel = ff.GetLevel(i);
        pNewLevel = new CDELevel (*pOldLevel);  // this will copy all groups within the level
        AddLevel (pNewLevel);
    }

    iMax = ff.GetNumForms();        // do this after i've copied all the levels and groups
    CDEForm* pNewForm;
    CDEForm* pOldForm;
    CDELevel* pLevel;
    CDEGroup* pParent;

    for (int i = 0; i < iMax; i++)
    {
        pOldForm = ff.GetForm(i);
        pNewForm = new CDEForm (*pOldForm);

        pLevel = GetLevel(pNewForm->GetLevel());       // retrieve the level for the form
        pParent = pLevel->FindGroup (pOldForm->GetGroup()->GetName());  // now find the group assoc w/this form

        pNewForm->SetGroup ( pParent );
        pNewForm->AddGroupItems ( pParent );
        AddForm (pNewForm);
    }

    m_uniqueNames = ff.m_uniqueNames; // and finally, unique names stored in the file

    SetFileName( ff.GetFileName() ); // RHF Nov 13, 2002
}


CDEFormFile::CDEFormFile(const CDEFormFile& ff) // copy constructor
{
    CopyFF(ff);
}



// the following constructor is invoked from cspro when a form is being created;
// hence, initialize the name and label to bogus vals, and create a blank form & 1st level

CDEFormFile::CDEFormFile(const CString& sFormPathName, const CString& sPrimaryDictName)
    :   m_csVersion(CSPRO_VERSION)
{
    BaseConstructor();

    // &&& this will have to change

    CIMSAString sFFName = sPrimaryDictName;

    sFFName = PortableFunctions::PathGetFilename(sFFName);
    sFFName = PortableFunctions::PathRemoveFileExtension<CString>(sFFName);

    sFFName.MakeName();         // converts '-' to '_', etc.
    sFFName += _T("_FF");       // for now, do this until savy passes me internal dict Name(i.e., what file will store, not the file name!)

    SetName(IsNameUnique(sFFName) ? sFFName : CreateUniqueName(sFFName));

    SetFileName( sFormPathName );// RHF Nov 13, 2002

    // set label to the file name
    CString sLabel = sFormPathName;
    sLabel = PortableFunctions::PathGetFilename(sLabel);
    sLabel = PortableFunctions::PathRemoveFileExtension<CString>(sLabel);
    SetLabel(sLabel);

    SetDictionaryFilename(sPrimaryDictName);
}


CDEFormFile::~CDEFormFile()
{
    RemoveAllForms();       // remove all associated pages
    RemoveAllLevels();      // Remove all levels too
}



// **************************************************************************
// allow the == operator? what actually makes two forms being equal anyhoo??

bool CDEFormFile::operator==(const CDEFormFile& ff) const
{
    //fixed SAVY 05/03/2004 code incorrect . vc7 caught it !!
    return  (m_csVersion.CompareNoCase(ff.m_csVersion)==0?true:false);  // &&

    // && smg:are we going to test dictionaries and pages as well???
}

void CDEFormFile::operator=(const CDEFormFile& ff)
{
    CopyFF(ff);
}


void CDEFormFile::IsPathOn(const CString& sVal)
{
    if ((sVal.CompareNoCase (_T("Survey")) == 0 ) || (sVal.CompareNoCase (_T("SystemControlled")) == 0 ))
        // gsf 04-may-00
        IsPathOn (true);
    else            // csArg "Census" or i don't care, default to false!
        IsPathOn (false);
}


/////////////////////////////////////////////////////////////////////////////
//
//               CDEFormFile::Reconcile
//
//  reconciles the forms file with the dictionary(s) (partially implemented),
//  and with itself (not yet implemented)
//
//  if there is a problem, keep the error message string in "csErr", so one can
//  can call this silently and get at the error message later
//
//  bSilent means no message boxes, if true
//  bAutoFix means make adjustments to this object or not
//  bReturn should be set to FALSE if a change was made
//
//  If a field on a form is not found in the data dictionary, it is removed from
//  the form, if bAutoFix = true
//
//  TODO:  more checking with dictionary; make sure all rules are followed governing
//         singles/multiples, etc.
//
/////////////////////////////////////////////////////////////////////////////

bool CDEFormFile::Reconcile (CString& csErr, bool bSilent, bool bAutoFix)
{
    bShowFormSelDlg = true;
    if(bShowFormSelDlg){
        this->ClearUsed();
        this->CheckFormFile();
    }
    csErr = _T("");
    bool bReturn = true;
    bReturn = ReconcileLevels(csErr,false,true);
    this->RenumberFormsAndItems();

    int iLevels = GetNumLevels();

    for (int i = 0; i<iLevels; i++)
    {
        CDELevel* pLevel = GetLevel(i);
        CDEGroup* pGroup = pLevel->GetRoot();

        eReturnType eRet = pGroup->Reconcile(this, csErr, bSilent, bAutoFix);

        if (eRet == Delete)
        {
            // delete the group
            // (actually, this can't happen; it's the root group)

            bReturn = false;
        }
        else if(eRet == Reconciled)
        {
            bReturn = false;
        }
    }

    if(!bReturn) //if some thing has changed renumber the forms
    {
        this->RenumberFormsAndItems();
    }
    RenumberAllForms();
    UpdatePointers();

    //ReconcileForms
    for(int iForm  = 0 ; iForm < this->GetNumForms();iForm++) {
        CDEForm* pForm = this->GetForm(iForm);
        CDEGroup* pGroup = pForm->GetGroup();

        // smg, 28-08-03: if the form repeat name changes, set bReturn to dirty/false!

        if(pGroup && pGroup->GetRIType() == CDEFormBase::UnknownRI) {

            if (!SO::IsBlank(pForm->GetRecordRepeatName()) ) {
                bReturn = false;
                pForm->SetRecordRepeatName(_T(""));
            }
        }
        else {
            ASSERT( pGroup != 0 );
            if (pForm->GetRecordRepeatName() != pGroup->GetTypeName()) {

                bReturn = false;
                pForm->SetRecordRepeatName(pGroup->GetTypeName());
            }
        }
    }
    if(ReconcileSkipTo(csErr))
        bReturn = false;

    if( ReconcileFieldAttributes(csErr) )
        bReturn = false;

    if (!bReturn){
        if (!bSilent && !csErr.IsEmpty()) {
            ErrorMessage::Display(csErr);
        }
    }

    return bReturn;
}




/////////////////////////////////////////////////////////////////////////////
//
//                          LookupSymbol
//
//  lookup symbol in data dictionary(s)
//
//  normally match on both csDict and csName
//  if csDict is null string, then look through all dictionaries
//
//  GSF 23-mar-00: This routine is doing more than it has to (but it's ok)
//  it assumes > 1 dictionary per forms file.  elsewhere in the system
//  we're assuming only 1 dictionary per forms file
//
/////////////////////////////////////////////////////////////////////////////

bool LookupSymbol(CDEFormFile* pFormFile, const CString& csDict, DICT_LOOKUP_INFO& sL)
{
    if( pFormFile->GetDictionary() != nullptr ) {
        CString csDictName = pFormFile->GetDictionary()->GetName();
        if (!csDict.IsEmpty()) {
            if (csDict.CompareNoCase(csDictName) != 0) {
                return false;
            }
        }
        bool b = pFormFile->GetDictionary()->LookupName(sL.csName, &sL.pLevel, &sL.pRecord, &sL.pItem, &sL.pVSet);
        if (b) {
            if( sL.pItem != nullptr && !sL.pItem->AddToTreeFor80() )
                return false;
            pFormFile->GetDictionary()->LookupName(sL.csName, &sL.iLevel, &sL.iRecord, &sL.iItem, &sL.iVSet);
            return true;
        }
    }

    return false;

}


//      ************************************************************************************
//
//      Here's all the funcs relating to the Unique Name List i'm maintaining!!
//
//      ************************************************************************************
//      this builds up the name list for all unique names w/in the form file
//
//      the form file's name            (user can name, but must be unique)
//      the level name                  (which is the same as the dict's level name)
//      the group name                  (i will generate, user can see but can't modify)
//      all keyed & unkeyed items       (if the item is keyed, it will use the dict name,
//                                                             else it will use my generated name; either way,
//                                                             they can't modify it)
//      the form's name


void CDEFormFile::BuildUniqueNL()
{
    RemoveAllUniqueNames();

    AddUniqueName(GetName());      // put the form file name in the list

    CDELevel* pLevel;

    for (int l = 0; l < GetNumLevels(); l++)
    {
        pLevel = GetLevel(l);

        AddUniqueName( pLevel->GetName() );         // ok w/o occs

        pLevel->GetRoot()->BuildUniqueNL (this);
    }

    for (int f=0; f < GetNumForms(); f++)   // now add the form and its text items
    {
        CDEForm* pForm = GetForm(f);

        int form_count = 0;
        while (!IsNameUnique(pForm->GetName()))          // ok as string
        {
            CString str;
            str.Format(_T("%03d"),form_count);
            pForm->SetName(_T("FORM")+str);
            form_count++;
        }
        AddUniqueName(pForm->GetName());

        for (int i = 0; i < pForm->GetNumItems(); )
        {
            CDEItemBase* pItem = pForm->GetItem(i);

            if (pItem->GetItemType() == CDEFormBase::Text)
            {
                int text_count = 0;
                while (!IsNameUnique(pItem->GetName()))          // ok as string
                {
                    CString str;
                    str.Format(_T("%03d"),text_count);
                    pItem->SetName(_T("TEXT")+str);// Since the text ID were not compatible
                    text_count++;
                }
                AddUniqueName(pItem->GetName());
            }
            pItem->SetFormNum(f);
            i++;
        }
    }
}


void CDEFormFile::AddUniqueName(const CString& name)
{
    if( !name.IsEmpty() )
    {
        ASSERT(SO::IsUpper(name) && CIMSAString::IsName(name));
        m_uniqueNames.insert(name);
    }
}


void CDEFormFile::AddUniqueNames(const CDEGroup* pGroup)
{
    // this is a recursive add for redo/undo; specifically, for when the user deletes
    // the entire form; i'm not going to test the uniqueness of each name, i'm just
    // going to assume it's unique and find where to add them to the uniq name list

    // add the unique name of the group
    AddUniqueName(pGroup->GetName());

    for( int i = 0; i < pGroup->GetNumItems(); ++i )
    {
        const CDEItemBase* pItem  = pGroup->GetItem(i);

        if( pItem->GetItemType() == CDEFormBase::Group || pItem->GetItemType() == CDEFormBase::Roster )
            AddUniqueNames(assert_cast<const CDEGroup*>(pItem));

        else
            AddUniqueName(pItem->GetName());
    }
}


void CDEFormFile::RemoveUniqueNames(const CDEGroup* pGroup)
{
    // this is a recursive delete for redo/undo
    RemoveUniqueName(pGroup->GetName());

    for( int i = 0; i < pGroup->GetNumItems(); ++i )
    {
        const CDEItemBase* pItem = pGroup->GetItem(i);

        if( pItem->GetItemType() == CDEFormBase::Group || pItem->GetItemType() == CDEFormBase::Roster )
            RemoveUniqueNames(assert_cast<const CDEGroup*>(pItem));

        else
            RemoveUniqueName(pItem->GetName());
    }
}


// ************************************************************************************
// create a unique name for the form file; i pass in a base name, say "GROUP", and
// then i append '001'.  if this passes muster, bail; if not, work my way up the
// odometer to 999; if this doesn't do it, we're hosed :)  also, accept max of 29
// incoming chars in the string

CString CDEFormFile::CreateUniqueName(CString name, bool add_name/* = true*/)
{
    CString sNewName;

    bool bIsTEXT = name.Find(_T("TEXT")) == 0;

    if(!bIsTEXT)
    {
        // total max length is 32, but i'm adding 3 #s to the uniq name,
        // so truncate 3 chars first!
        TCHAR hunds = _T('0'),
              tens  = _T('0'),
              ones  = _T('0');

        sNewName = name;
        sNewName += _T("000");

        int     len = sNewName.GetLength();

        while (!IsNameUnique(sNewName) || CIMSAString::IsReservedWord(sNewName))
        {
            if (ones < '9')
            {
                ones++;
                sNewName.SetAt (len-1, ones);
            }
            else
            {
                ones = _T('0');
                sNewName.SetAt (len-1, ones);

                if (tens < '9')
                {
                    tens++;
                    sNewName.SetAt (len-2, tens);
                }
                else
                {
                    tens = _T('0');
                    sNewName.SetAt (len-2, tens);

                    if (hunds < '9')
                    {
                        hunds++;
                        sNewName.SetAt (len-3, hunds);
                    }
                    else
                        ASSERT (false);         // we're in trouble! couldn't make the name unique
                }
            }
        }
    }

    else // names for text
    {
        int iStartVal = 0;

        do
        {
            iStartVal++;
            sNewName.Format(_T("TEXT%d"),iStartVal);

        } while( !IsNameUnique(sNewName) && CIMSAString::IsReservedWord(sNewName) );
    }

    if (add_name)                                        // found a unique name, add it to name list
        AddUniqueName(sNewName);

    return sNewName;
}



/*      ************************************************************************************

these two funcs (one at CDEFormFile level, the other at CDEGroup) are for Serpro;

they initialize the m_pParent ptr in the CDEItemBase class; i.e., it looks thru
every CDEItemBase object and sets its parent to the CDEGroup in "charge" of it

************************************************************************************ */

void CDEFormFile::UpdatePointers()
{
    for (int l = 0; l < GetNumLevels(); l++)
    {
        CDELevel* pLevel = GetLevel(l);
        CDEGroup* pRoot = pLevel->GetRoot();

        int gMax = pLevel->GetNumGroups();

        for (int g = 0; g < gMax; g++)
        {
            CDEGroup* pGroup = pLevel->GetGroup(g);

            pGroup->UpdatePointers(pRoot);
            this->UpdateDictItemPointers(pRoot); //SAVY&&& this line is added by me for DictVSet info
        }
    }

    this->SetMaxFieldPointer();
}


void CDEFormFile::RefreshAssociatedFieldText()
{
    CDELevel* pLevel;
    CDEGroup* pGroup;
    CDEGroup* pRoot;

    int g, gMax;

    for (int l = 0; l < GetNumLevels(); l++)
    {
        pLevel = GetLevel(l);

        pRoot = pLevel->GetRoot();

        gMax = pLevel->GetNumGroups();

        for (g=0; g < gMax; g++)
        {
            pGroup = pLevel->GetGroup (g);

            pGroup->RefreshAssociatedFieldText();

            for (int groupItem=0; groupItem < pGroup->GetNumItems();groupItem++){//for all the items in the form group if they are rosters refresh occ label stubs
                CDEItemBase* pBaseItem  = pGroup->GetItem(groupItem);
                if(pBaseItem->GetItemType() == CDEFormBase::Roster){
                    assert_cast<CDERoster*>(pBaseItem)->RefreshStubsFromOccurrenceLabels(*this);
                }
            }
        }
    }
}


//      ************************************************************************************
//      the following few funcs will create a default form file based on the primary (0th)
//  dictionary; it will do this in the following way:
//
//  [1] loop thru each level; for each level, Level IDs and each record will be placed
//              on their own form
//      [2] for Level 2+ it will copy Level 1+'s IDs (as protected/display) and then add its
//              own IDs; so, for the Level3 IDs, it will show all of Level 1&2's IDs and then
//              list it's own below
//      ************************************************************************************
void CDEFormFile::CreateFormFile(const CDataDict* pDD, const std::variant<CDC*, CSize>& pDC_or_single_character_text_extent,
                                 const DragOptions& drag_options, int iDropSpacing/* = 500*/, bool bBuildRecords/* = false*/)
{
    RemoveAllForms();       // kill any existing forms
    RemoveAllLevels();      // and of course, the levels too
    RemoveAllUniqueNames();

    SetDictionaryName(pDD->GetName());

    AddUniqueName(GetName());      // don't forget the form file's name!

    int iPgNum = 0;

    //  loop thru all the levels & each rec w/in a level--each record will be a sep form

    int j, jmax;

    bool bFormAdded = false;     // for each level, tell me if a form got added!

    // cycle thru each of the dictionary's levels
    for( size_t level_number = 0; level_number < pDD->GetNumLevels(); ++level_number )
    {
        const DictLevel& dict_level = pDD->GetLevel(level_number);
        CDELevel* pFormLevel = new CDELevel();   // new-ing a level creates it's ("hidden") placeholder CDEGroup
        CString sRootName = CreateUniqueName(_T("BaseGrp"), false);        // don't add name to uniq name list
        pFormLevel->GetRoot()->SetName(sRootName);

        AddUniqueName(dict_level.GetName());       // since we're not calling CreateUniqName, must "manually" add it

        pFormLevel->SetName(dict_level.GetName());   // use the dict name for our unique name
        pFormLevel->SetLabel(dict_level.GetLabel());

        if (!bBuildRecords)     // if we're not building w/the records, give a blank form
        {
            CDEGroup* pGroup = new CDEGroup();

            CreateGroup(pGroup, dict_level, iPgNum++);

            pGroup->SetParent(pFormLevel->GetRoot());
            pFormLevel->AddItem(pGroup);
            CreateForm(level_number, pGroup);                 // pass in the curr level
            bFormAdded = true;
        }
        else
        {
            const CDictRecord* pDictRec = dict_level.GetIdItemsRec();

            if (pDictRec->GetNumItems() > 0)
            {
                CDEGroup* pGroup = new CDEGroup();    // name will be set below in CreateGroup

                if (GetNumLevels() > 0)     // if this isn't 1st level, add prev level's IDs
                    CreateGroup(pGroup, pDictRec, iPgNum++, pDC_or_single_character_text_extent, iDropSpacing, drag_options, true, GetLastLevel()->GetGroup (0));
                else
                    CreateGroup(pGroup, pDictRec, iPgNum++, pDC_or_single_character_text_extent, iDropSpacing, drag_options, true);

                pGroup->SetParent(pFormLevel->GetRoot());

                pFormLevel->AddItem(pGroup);

                CreateForm(level_number, pGroup);             // pass in the curr level

                bFormAdded = true;
            }

            jmax = dict_level.GetNumRecords();

            // next, create a group for each record, then build the form if requested

            for (j = 0; j < jmax; j++)
            {
                pDictRec = dict_level.GetRecord(j);    // retrieve the dict record

                CDEGroup* pGroup = new CDEGroup();

                CreateGroup(pGroup, pDictRec, iPgNum++, pDC_or_single_character_text_extent, iDropSpacing, drag_options);

                pGroup->SetParent(pFormLevel->GetRoot());

                pFormLevel->AddItem(pGroup);

                CreateForm(level_number, pGroup);

                bFormAdded = true;
            }
        }
        if (! bFormAdded)       // if no forms got added in this level, then supply an empty one
        {
            CDEGroup* pGroup = new CDEGroup();

            CreateGroup(pGroup, NULL, iPgNum++, pDC_or_single_character_text_extent, iDropSpacing, drag_options);

            pGroup->SetParent(pFormLevel->GetRoot());

            pFormLevel->AddItem(pGroup);

            CreateForm(level_number);

            pGroup->SetFormName( (GetForm(pGroup->GetFormNum()))->GetName());
        }
        else
            bFormAdded = false;     // reset for test of next level

        // ok, finished bldg this level, so add it to the FRM and go do another

        int iIndex = GetNumLevels();
        AddLevel(pFormLevel);

        pFormLevel->SetHierarchy(iIndex);
    }
}


/////////////////////////////////////////////////////////////////////////////
//  forms are siblings to one another in the CDEFormFile, i.e., no dependencies
//      implicit by their placement in the array (since all top-level)

CDEForm* CDEFormFile::CreateForm(int level, CDEGroup* pGroup/*=NULL*/)
{
    CDEForm* pForm = new CDEForm(CreateUniqueName(_T("FORM")), _T("New Form"));

    pForm->SetGroup (pGroup); //SAVY 04/00 for the problem after generate and show form properties
    pForm->SetLevel (level);

    if (pGroup != NULL)
    {
        pGroup->SetFormName(pForm->GetName());

        pForm->SetDims  (pGroup->GetDims());
        pForm->SetLabel(pGroup->GetLabel());

        if (pGroup->GetMaxLoopOccs() > 1)
        {
            pForm->SetRecordRepeatName(pGroup->GetRecordRepeatName());
        }

        // no default text headings/footnotes, at least for now

        pForm->AddGroupItems (pGroup);  // loop thru all items assoc w/this group and add them to the form
    }
    AddForm (pForm);

    return pForm;
}


void CDEFormFile::CreateRosterField (CDEField* pField,
                                     CDERoster* pRoster)
{
    pField->SetParent (pRoster);

    CDECol* pCol = new CDECol();

    pCol->SetHeaderText(pField->GetCDEText());
    pCol->AddField (pField);
    pRoster->AddCol (pCol);
    pRoster->AddItem (pField);   // CDEGroup nds to see it too (for Serpro)
}


/////////////////////////////////////////////////////////////////////////////
// build up a CDERoster from a dictionary record;
void CDEFormFile::CreateRoster(CDERoster* pRoster,
                               const CDictRecord* pDR,
                               int iFormNum,
                               CPoint dropPt,
                               const DragOptions& drag_options,
                               bool bAddItemsFromRecord /* = true */)
{
    // CDEGroup member info

    pRoster->SetMaxLoopOccs (pDR->GetMaxRecs());
    pRoster->SetRIType (Record);
    pRoster->SetTypeName (pDR->GetName());

    // CItemBase member info
    pRoster->SetFormNum (iFormNum);

    // and finally, stuff from CFormBase

    // NOTE! the roster's name can't be the unique dict record name, as the
    //       user could choose to create several rosters based on items from
    //       this record
    pRoster->SetName  (CreateUniqueName (pDR->GetName()) );
    pRoster->SetLabel (pDR->GetLabel());
    pRoster->SetRequired (pDR->GetRequired());
    pRoster->SetDims (dropPt.x, dropPt.y, 0, 0);

    if( drag_options.UseRosters() )
        pRoster->SetOrientation(drag_options.GetRosterOrientation());

    // provide stub column, default to #s down the side
    CDECol* pCol = new CDECol();    // the first will be the stub col
    pCol->SetWidth (10);
    pRoster->AddCol(pCol);

    CDEText* pText = new CDEText(); // use this as a template for stub text
    pText->SetText (_T("@"));           // default to #s down the side
    pText->SetUseDefaultFont(true);
    pText->SetFont(GetDefaultTextFont());

    if( drag_options.UseOccurrenceLabels() || pRoster->GetUseOccurrenceLabels() ) { // 20140226
        pRoster->SetUseOccurrenceLabels(true);
        pRoster->SetAllStubs(pText,pDR);
    }

    else {
        pRoster->SetAllStubs (pText);   // do it
    }

    delete pText;                   // don't need him anymore

    // If not adding all items from the record, bail, have done enuf
    if (!bAddItemsFromRecord) {
        return; // done
    }

    int max = pDR->GetNumItems();
    for (int i = 0; i < max; i++) {
        const CDictItem* pDI = pDR->GetItem(i);
        if( !pDI->AddToTreeFor80() )
            continue;
        if (pDI->GetItemType() == ItemType::Subitem) {
            // Only add items
            continue;
        }
        else if (FindItem (pDI->GetName())) {
            // it's located elsewhere as a keyed field, skip
            continue;
        }
        else if (pDI->GetItemType() == ItemType::Item && GetNumDictSubitems(pDR, pDI)){ //make sure that if item has subitems they are not keyed elsewhere
            int iNumSubItems = GetNumDictSubitems(pDR, pDI);

            bool bKeyedElsewhere = false;
            for (int iSubItem = 1 ; iSubItem <= iNumSubItems ; iSubItem++)
            {
                const CDictItem* pSubItem = pDR->GetItem(i+iSubItem);
                bKeyedElsewhere = FindItem(pSubItem->GetName());
                if (bKeyedElsewhere)
                    break;
            }
            if (bKeyedElsewhere) // do not add the item if its subitems are added elsewhere.
                continue;
        }

        // for now, we can't have mult w/in mult
        if( pDI->GetOccurs() == 1 )
        {
            // only one occurrence of the item...
            pRoster->AddNonOccItem(pDR, pDI, iFormNum, drag_options, this, i);
        }
    }

    pRoster->FillItemPtrs();   // the roster nds to stuff the group's item array
    pRoster->UpdateFlagsNFonts(*this);
}


/////////////////////////////////////////////////////////////////////////////
// build up a roster from a dictionary *item* (i.e., one that occurs! woo-hoo!)
//
// if bDropSubitems=false, then i really don't need to pass in the pDD
// csc 8/24/00
void CDEFormFile::CreateRoster(CDERoster* pRoster, const CDictItem* pDI, int iFormNum, CPoint dropPt, const std::variant<CDC*, CSize>& pDC_or_single_character_text_extent,
                               const DragOptions& drag_options, int iOcc, bool bDropSubitems, const CDataDict* pDD)
{
    // CDERoster member info

    // only change the default setting if the user selected something!
    // (i.e., they could have been forced to get a roster due to drop location,
    // even though the drag opts for rostering is NONE
    if( drag_options.UseRosters() )
        pRoster->SetOrientation(drag_options.GetRosterOrientation());

    // CDEGroup member info

    // let me explain this test (as it will have to be modified once items AND subitems
    // are allowed to have occurrences); i'm passing in either an item or subitem to
    // roster; but in the event this is a subitem that DOESN'T have occs but its parent
    // does, i can't see it by calling GetOccurs(); so i'm passing in the value from
    // the form view's OnDropItem(), who is getting it from the drop rules class;
    // look at CFormDropRules::AnalyzeDrop() for a bit of commentary on determing occs

    if (pDI->GetOccurs() == 1)  {   // then use the parent's vals
        pRoster->SetMaxLoopOccs (iOcc);

        int iLevel, iRec, iItem, iVset;
        pDD->LookupName(pDI->GetName(), &iLevel, &iRec, &iItem, &iVset);
        const CDictItem* pParent = pDD->GetParentItem (iLevel, iRec, iItem);
        pRoster->SetTypeName (pParent->GetName());
    }
    else  {                         // use the item's value
        pRoster->SetMaxLoopOccs (pDI->GetOccurs());
        pRoster->SetTypeName (pDI->GetName());
    }
    // gsf 01/24/01
    if (pDI->GetItemType() == ItemType::Item) {
        pRoster->SetRIType (Item);
    }
    else {
        pRoster->SetRIType (SubItem);
    }

    // CItemBase member info
    pRoster->SetFormNum (iFormNum);

    // and finally, stuff from CFormBase
    pRoster->SetName ( CreateUniqueName (pDI->GetName()) );
    pRoster->SetLabel (pDI->GetLabel());

    CDECol* pCol = new CDECol();    // provide the stub col
    pCol->SetWidth (10);
    pRoster->AddCol (pCol);

    CDEText* pText = new CDEText(); // use this as a template for stub text
    pText->SetText (_T("@"));           // default to #s down the side
    pText->SetUseDefaultFont(true);
    pText->SetFont(GetDefaultTextFont());

    if( drag_options.UseOccurrenceLabels() ) // 20140226
    {
        pRoster->SetUseOccurrenceLabels(true);
        const DictNamedBase* pDictObject = pDI;

        if( pDI->GetItemType() == ItemType::Subitem && pDI->GetParentItem()->GetOccurs() > 1 )
            pDictObject = pDI->GetParentItem(); // the subitem's parent repeats

        else if( pDI->GetOccurs() == 1 )
            pDictObject = pDI->GetRecord(); // the record repeats

        pRoster->SetAllStubs(pText, pDictObject);
    }

    else
    {
        pRoster->SetAllStubs (pText);   // do it
    }

    delete pText;                   // don't need him anymore

    // now get the item/subitem(s) in to the roster
    // i (smg) nd the field to contain the unique dict name, so init the field first
    CDEField* pField = NULL;
    CString csLabel, sDictName = GetDictionaryName();

    if (bDropSubitems) {
        if (pDD != NULL)  {
            // shld never occur, but check
            int iLevel, iRec, iItem, iVset, max;
            const CDictItem* pSubItem;
            pDD->LookupName(pDI->GetName(), &iLevel, &iRec, &iItem, &iVset);
            const CDictRecord* pRec = pDD->GetLevel(iLevel).GetRecord (iRec);
            max = pRec->GetNumItems();
            pSubItem = pRec->GetItem(++iItem);

            while (iItem < max && pSubItem->GetItemType() == ItemType::Subitem)  {
                pField = new CDEField(pSubItem, iFormNum, sDictName, pDC_or_single_character_text_extent, drag_options);
                pField->SetDims(0,0,0,0);    // CSC 10/17/00 give default layout for field
                AddUniqueName(pField->GetName());   // new'ing Field can't add its uniq name
                CreateRosterField (pField, pRoster);
                if (++iItem < max)  {
                    pSubItem = pRec->GetItem(iItem);
                }
            }
        }
    }
    else  {
        pField = new CDEField(pDI, iFormNum, sDictName, pDC_or_single_character_text_extent, drag_options);
        pField->SetDims(0,0,0,0);           // CSC 10/17/00 give default layout for field
        AddUniqueName(pField->GetName());   // new'ing Field can't add its uniq name
        CreateRosterField (pField, pRoster);
    }
    pRoster->SetDims (dropPt.x, dropPt.y, 0,0);
    pRoster->UpdateFlagsNFonts(*this);
}


// **************************************************************************
// the folowing few funcs i've written to give me status info and such on
// on dictionary items
//
/////////////////////////////////////////////////////////////////////////////
// utility func for me so i don't have to go thru the dictionary's rule mgr;
// having passed in a dict item that is an ITEM (though will still work if a
// SUBITEM--will just return 0), find out how many (if any) subitems it has

int CDEFormFile::GetNumDictSubitems(const CDictRecord* pDR, const CDictItem* pDI) const
{
    if (pDI->GetItemType() == ItemType::Subitem)
        return 0;

    int iNumKids = 0,
        max = pDR->GetNumItems();

    int iItem = pDI->GetSonNumber();

    for (int i = iItem+1; i < max; i++)
    {
        if (pDR->GetItem(i)->GetItemType() == ItemType::Subitem)
            iNumKids++;
        else
            break;
    }
    return iNumKids;
}

int CDEFormFile::GetDictItemParentIndex(const CDictRecord* pDR, const CDictItem* pDI)
{
    int index = pDI->GetSonNumber();
    const CDictItem* pItem = pDI;

    if (pItem->GetItemType() == ItemType::Subitem)
    {
        bool bDone = false;

        while (!bDone)
        {
            if (--index < 0)
            {
                index = NONE;
                bDone = true;
            }
            else
            {
                pItem = pDR->GetItem(index);

                bDone = (pItem->GetItemType() == ItemType::Item);
            }
        }
    }
    return index;
}

// ******************************************************************************
// do any of the item's subitem(s) overlap another?
//
// this is a utility to the below func, TestSubitemDrop()

bool CDEFormFile::DoAnySubitemsOverlap(const CDictRecord* pDR,
                                       const CDictItem* pDI)
{
    bool bAnyOverlaps = false;              // as soon as i find one sibling subitem that overlaps, stop

    int iParent  = GetDictItemParentIndex(pDR, pDI);

    if (iParent == NONE)    // bug fix for ISSA's convert; the dict item they're passing
        return true;        // me does not have the SonNumber initialized, so i'm crashing

    int iNumKids = GetNumDictSubitems(pDR, pDR->GetItem(iParent));

    int i, loop;

    const CDictItem* pCurrSubItem = NULL;
    const CDictItem* pNextSubItem = NULL;

    // for this test i check the curr subitem's ending position against
    // the start position of the next subitem; if they overlap, bail!

    for (loop=0, i=iParent+1;
        loop < iNumKids-1 && !bAnyOverlaps;
        loop++, i++)
    {
        if (pNextSubItem == NULL)
            pCurrSubItem = pDR->GetItem(i);
        else
            pCurrSubItem = pNextSubItem;

        pNextSubItem = pDR->GetItem(i+1);

        if ( pCurrSubItem->GetStart() + (pCurrSubItem->GetLen() * pCurrSubItem->GetOccurs())
        > pNextSubItem->GetStart())
        {
            bAnyOverlaps = true;
        }
    }
    return bAnyOverlaps;
}

// this is a utility to the below func, TestSubitemDrop()

// as of 02/00 when i pulled the code out (i need it now at the 'O' level), it's
// identical to CFormDropRules::CheckItemVsSubItemFromMultiple(), with the
// exception that i'm not setting any err msgs as i do in drop rules; here, all
// i care about is whether any of the item's subitem(s) are being keyed

bool CDEFormFile::AreAnySubitemsBeingKeyed(const CDictRecord* pDR, const CDictItem* pDI)
{
    bool bKeyedElsewhere = false;

    int iItem = pDI->GetSonNumber();

    if (pDI->GetItemType() == ItemType::Item)       // shld be...
    {
        int iNumKids = GetNumDictSubitems (pDR, pDI);

        int loop, i;

        for (loop=0, i=iItem+1;
            loop < iNumKids && !bKeyedElsewhere;
            loop++, i++)
        {
            const CDictItem* pSubItems = pDR->GetItem(i);

            bKeyedElsewhere = FindItem (pSubItems->GetName());
        }
    }
    return bKeyedElsewhere;
}

// ====================================================================
// essentially, see if the drag dialog options agree w/what the drop rules determined...
//
// if the user wants to drop subitems when possible, this tests the specific item being
// dropped to see if
//
// [1] it has any subitems to drop in the first place (may not)
// [2] if the subitem(s) can be dropped; to pass this test, they must comply with:
//
//              [a] none of the subitems can physically overlap another (DoAnySubitemsOverlap)
//              [b] check that none of the item's subitem(s) is (are) being keyed

bool CDEFormFile::TestSubitemDrop(const CDictRecord* pDR,
                                  const CDictItem* pDI,
                                  const DragOptions& drag_options,
                                  bool bCheck4KeyedSubs /*=true*/)
{
    // if user didn't set this option in the drag options dialog, no point in continuing!

    if (! drag_options.UseSubitems())
        return false;

    int iNumKids = GetNumDictSubitems (pDR, pDI);

    if (iNumKids == 0 )      // no subitems to drop! so can't do
        return false;

    // now look at my rule mgr, and see if dropping subitems was possible or not

    // i don't care if the record is single or multiple, i'm calling the mult func
    // as it's more restrictive and hence what i want (i.e., won't allow subitems
    // to be dropped as display, etc.)

    if (DoAnySubitemsOverlap (pDR, pDI))

        return false;

    // i don't necessarily care if all the subitems are being keyed elsewhere or not

    if (bCheck4KeyedSubs)
    {
        if (AreAnySubitemsBeingKeyed (pDR, pDI) )
            return false;
    }
    return true;    // passed the audition
}

// **************************************************************************
// finished w.the dict-related funcs i needed; back to regularly-scheduled zFormO code...
// **************************************************************************

// this initializes a blank form for a level

void CDEFormFile::CreateGroup(CDEGroup*    pGroup,
                              const DictLevel& dict_level,
                              int          iFormNum)
{
    // give the group a name based on the dict record name; however,
    // can't use the dict name itself, as ISSA can't handle

    CString sName = dict_level.GetName() + _T("_FORM");

    if (IsNameUnique (sName))       // if nobody else is using this name
    {
        pGroup->SetName(sName);                // this doesn't insert the name in to the uniq name list
        AddUniqueName(sName);
    }
    else
        pGroup->SetName(CreateUniqueName(sName));

    pGroup->SetFormNum(iFormNum);
    pGroup->SetRequired(true);
    pGroup->SetLabel(dict_level.GetLabel());
    pGroup->SetMaxLoopOccs (1);
}


/////////////////////////////////////////////////////////////////////////////
// place all LevelIDs from previous levels on form first before adding curr IDs
void CDEFormFile::CreateGroup(CDEGroup* pGroup, const CDictRecord* pDictRec, int iFormNum, const std::variant<CDC*, CSize>& pDC_or_single_character_text_extent,
                              int iDropSpacing, const DragOptions& drag_options, bool bIdRec /*= false*/, CDEGroup* pPrevGroup /* =NULL */)
{
    int row = rowOffset;
    LONG rightCol = 0;
    CString RecTypeVal;

    pGroup->SetFormNum(iFormNum);

    // give the group a name based on the dict record name; however,
    // can't use the dict name itself, as ISSA can't handle

    CString sRecName = pDictRec->GetName();

    while (sRecName[0] == '_')      // get rid of any leading underscores...which happens w/Level ID record
        sRecName.Delete (0, 1);         // nIndex, nCount

    sRecName += _T("_FORM");

    if (IsNameUnique (sRecName))    // if nobody else is using this name
    {
        pGroup->SetName(sRecName);             // this doesn't insert the name in to the uniq name list
        AddUniqueName(sRecName);
    }
    else
        pGroup->SetName( CreateUniqueName(sRecName));

    if (pDictRec == NULL)           // bail; most likely a new dict being created w/the new form
        return;

    pGroup->SetRequired(pDictRec->GetRequired());
    pGroup->SetLabel        (pDictRec->GetLabel());
    pGroup->SetTypeName(pDictRec->GetName());

    RecTypeVal = pDictRec->GetRecTypeVal();

    if (bIdRec || pDictRec->GetMaxRecs() == 1) // group doesn't loop, not dependant on a rec
    {
        pGroup->SetMaxLoopOccs (1);
    }
    else                                            // group depends on a record
    {
        pGroup->SetRIType (Record);
        pGroup->SetMaxLoopOccs (pDictRec->GetMaxRecs());
    }

    //  first, loop thru all LevelIDs on prev group, if any

    CDEField* pField = NULL;

    if (pPrevGroup)
    {
        // the field's protected, so generate a unique name

        for (int i = 0; i < pPrevGroup->GetNumItems(); i++)
        {
            pField = new CDEField();
            
            *pField = *assert_cast<CDEField*>(pPrevGroup->GetItem(i));

            pField->SetName( CreateUniqueName(pField->GetItemName()) );   //adds the uniq name to the NameList

            pField->IsMirror (true);
            pField->SetFormNum (iFormNum);
            pField->SetParent(pGroup);

            pGroup->AddItem(pField);

            rightCol = std::max (rightCol, pField->GetDims().right);
            rightCol = std::max (rightCol, pField->GetCDEText().GetDims().right);
        }
        ASSERT( pField != 0 );
        row = pField->GetDims().top + rowOffset;
    }

    //  loop thru all Items assoc w/this record

    CPoint cp1(0,0);

    if (drag_options.UseRosters() && pGroup->GetMaxLoopOccs() > 1 && !pDictRec->Is2DRecord())
    {
        pGroup->SetMaxLoopOccs (1);             // unset
        pGroup->SetRIType (UnknownRI);

        if (pDictRec->GetNumItems() == 0 )       // if there's no items in the rec, bail, o/w grid croaks
        {
            rightCol = std::max (rightCol, MaxScrSz);
            if (iFormNum >= 0) pGroup->SetDims (0,0,rightCol,(row > MaxScrSz) ? row : MaxScrSz);
            return;
        }
        cp1.x = 40;
        cp1.y = row;

        CDERoster* pRoster = new CDERoster();
        pRoster->SetRightToLeft(m_bRTLRosters);
        CreateRoster(pRoster, pDictRec, iFormNum, cp1, drag_options);
        pRoster->SetParent(pGroup);
        pGroup->AddItem(pRoster);
    }
    else    // drop the items as regular text/box fields
    {
        CString sDictName = GetDictionaryName();

        int iNumDIs = pDictRec->GetNumItems();

        for (int i1 = 0; i1 < iNumDIs; i1++)
        {
            // otherwise, use the dictionary name to be unique

            const CDictItem* pDictItem = pDictRec->GetItem(i1);

            if (pDictItem->GetItemType() == ItemType::Subitem)    // don't log subitems, only items
                continue;

            if( !pDictItem->AddToTreeFor80() )
                continue;

            if (pDictItem->GetOccurs() > 1)         // create an entry for each occurrence...
            {
                // forced occ fields to be done; now, must force user to get a roster;
                // eventually, when occurring item w/in occurring record implemented,
                // give users the choice!

                // CreateGroupOccurrenceFields (pGroup, pDictItem, iFormNum, &row, pDC, drag_options);

                CDERoster* pRoster = new CDERoster();
                pRoster->SetRightToLeft(m_bRTLRosters);
                pRoster->SetParent(pGroup);

                CPoint cp2(40, row);

                bool bUseSIs = false;

                if (i1+1 < iNumDIs &&    // look at the next guy
                    pDictRec->GetItem(i1+1)->GetItemType() == ItemType::Subitem) // is it a subitem?
                {
                    // if so, *then* check to see if it's possible to use 'em

                    bUseSIs = drag_options.UseSubitems() &&                // does user want subitems
                              !DoAnySubitemsOverlap (pDictRec, pDictItem); // if Y, do any overlap?
                }

                CreateRoster(pRoster, pDictItem, iFormNum, cp2, pDC_or_single_character_text_extent,
                             drag_options, pDictItem->GetOccurs(), bUseSIs, pDictRec->GetDataDict());

                // fix: chris, the roster doesn't have decent dims, so i can't
                // adjust the row by its height...suggestions?
                //        int iRows = min (MIN_NUM_ROSTER_ROWS, pRoster->GetMaxLoopOccs()+1);    // +1 csc 11/16/00

                int iRows = std::min (MIN_NUM_ROSTER_ROWS, pRoster->GetMaxLoopOccs()+1);
                row += (rowOffset * iRows);

                pGroup->AddItem(pRoster);
            }
            else    // item has no occurrences
            {
                bool bSubitemDropOK = TestSubitemDrop (pDictRec, pDictItem, drag_options);

                if (bSubitemDropOK)
                {
                    //                                      int iNumKids = GetNumDictSubitems (pDictRec, pDictItem);

                    int     max = pDictRec->GetNumItems();
                    int     iStartIndex = pDictItem->GetSonNumber()+1;
                    int     iItemLen;
                    bool    bDone = false;

                    for (int i2=iStartIndex; i2 < max && !bDone; i2++)
                    {
                        const CDictItem* pSubItem = pDictRec->GetItem(i2);
                        iItemLen = pSubItem->GetLen();

                        if (pSubItem->GetItemType() == ItemType::Item)    // all done w/subitems :)
                        {
                            bDone = true;
                            continue;
                        }
                        if(pSubItem->GetOccurs() ==1){
                            pField = new CDEField(pSubItem, iFormNum, sDictName, pDC_or_single_character_text_extent,
                                                  drag_options, iDropSpacing, cp1, row);

                            AddUniqueName(pField->GetName());

                            pField->SetParent(pGroup);
                            pGroup->AddItem(pField);

                            row += rowOffset;

                            rightCol = std::max (rightCol, pField->GetDims().right);
                            rightCol = std::max (rightCol, pField->GetCDEText().GetDims().right);
                        }
                        else {
                            CDERoster* pRoster = new CDERoster();
                            pRoster->SetRightToLeft(m_bRTLRosters);
                            pRoster->SetParent(pGroup);
                            CPoint cp (40, row);
                            bool bUseSIs = false; // We are already a subitem
                            CreateRoster(pRoster, pSubItem, iFormNum, cp, pDC_or_single_character_text_extent,
                                         drag_options, pDictItem->GetOccurs(), bUseSIs, pDictRec->GetDataDict());

                            // fix: chris, the roster doesn't have decent dims, so i can't
                            // adjust the row by its height...suggestions?
                            //        int iRows = min (MIN_NUM_ROSTER_ROWS, pRoster->GetMaxLoopOccs()+1);    // +1 csc 11/16/00

                            int iRows = std::min (MIN_NUM_ROSTER_ROWS, pRoster->GetMaxLoopOccs()+1);
                            row += (rowOffset * iRows);

                            pGroup->AddItem(pRoster);
                        }
                    }
                }
                else    // else just drop the item itself
                {
                    pField = new CDEField(pDictItem, iFormNum, sDictName, pDC_or_single_character_text_extent,
                                          drag_options, iDropSpacing, cp1, row);

                    AddUniqueName(pField->GetName());

                    pField->SetParent(pGroup);

                    pGroup->AddItem(pField);

                    row += rowOffset;

                    rightCol = std::max (rightCol, pField->GetDims().right);
                    rightCol = std::max (rightCol, pField->GetCDEText().GetDims().right);
                }
            }
        }
    }

    rightCol = std::max (rightCol, MaxScrSz);

    if (iFormNum >= 0) pGroup->SetDims (0,0,rightCol,(row > MaxScrSz) ? row : MaxScrSz);
}


/////////////////////////////////////////////////////////////////////////////
//
//                             CDEFormFile
//
/////////////////////////////////////////////////////////////////////////////

// if an item is deleted from the form (or tree i suppose), due to the nesting
// of groups, i can't keep an index to it, so pass in it's name and kill it!

bool CDEFormFile::RemoveItem(const CString& sItemName)
{
    for( int l = 0; l < GetNumLevels(); ++l )
    {
        CDELevel* pLevel = GetLevel(l);

        for( int g = 0; g < pLevel->GetNumGroups(); ++g )
        {
            if( pLevel->GetGroup(g)->RemoveItem(sItemName) )
                return true;
        }
    }

    return false;
}

// ***************** forms

CDEForm* CDEFormFile::GetForm(int i) const
{
    if (i < 0 || i >= (int)m_aForm.size()) // if we're asking for smthng beyond the last element
        return NULL;
    else
        return m_aForm [i];
}

void CDEFormFile::RemoveFormAt (int iIndex)
{
    CDEForm* pForm = GetForm(iIndex);

    RemoveUniqueName(pForm->GetName());

    m_aForm.erase(m_aForm.begin() + iIndex);
}

// ***************************************************************************

void CDEFormFile::RemoveForm(int iIndex, bool bRenumber/* = true*/)
{
    CDEForm* pForm = GetForm(iIndex);

    if(pForm){
        RemoveUniqueName(pForm->GetName());

        m_aForm [iIndex]->RemoveAllItems();

        delete m_aForm [iIndex];                // free the memory for the form object
    }

    m_aForm.erase(m_aForm.begin() + iIndex);          // remove the obj from the array admin

    if (bRenumber)
        UpdateFormFieldIndices (iIndex);
}

// ***************************************************************************
// since all items on a form (fields, rosters, etc) contain indices that say on
// which form it's located, if i delete a form i've got to update all subsequent
// item's form ptrs! (decrement them by one); but this goes for the groups they're
// contained under as well, so that will have to be done at the FF level, not
// from the forms

void CDEFormFile::UpdateFormFieldIndices (int iStartingFormNo)
{
    int f, fmax = GetNumForms();

    int i, iMax;

    CDEForm* pForm;
    CDEItemBase* pItem;

    for (f = iStartingFormNo; f < fmax; f++)
    {
        pForm = GetForm(f);

        iMax = pForm->GetNumItems();

        for (i = 0; i < iMax; i++)
        {
            pItem = pForm->GetItem(i);

            pItem->SetFormNum (pItem->GetFormNum() - 1);
            if(pItem->GetParent()) {
                pItem->GetParent()->SetFormNum (pItem->GetFormNum());
            }
        }
    }
}

//      ***************************************************************************
/*
if the user reorders the forms via the tree ctrl (i.e., a drag), it forces
a form-renumbering, not to mention that all items on the form need to know
their "new" (i.e., differently-numbered) form number; so do this here;

since the reorder occurred on the tree ctrl, the order of encountering
forms in the level/group must be used as the new form order; so traverse
recursively thru the groups to determine the form order;

*/

void CDEFormFile::RenumberFormsAndItems()
{
    int i, maxLevels = GetNumLevels();

    CDELevel* pLevel;
    CDEGroup* pGroup;

    int iCurFormNum = 0;

    for (i = 0; i < maxLevels; i++)
    {
        pLevel = GetLevel(i);

        int g, gmax = pLevel->GetNumGroups();

        for (g=0; g < gmax; g++)
        {
            pGroup = pLevel->GetGroup (g);

            iCurFormNum = pGroup->RenumberFormsAndItems (this, iCurFormNum);
        }
    }
}


void CDEForm::RenumberItems(int iNewFormLoc)
{
    int max = GetNumItems();

    for (int i = 0; i < max; i++)
    {
        CDEItemBase* pItem = GetItem(i);
        pItem->SetFormNum (iNewFormLoc);

        //SAVY &&& this sets the formnum for roster items
        if(pItem->GetItemType() == CDEItemBase::eItemType::Roster)
        {
            CDERoster* pRoster = assert_cast<CDERoster*>(pItem);
            int iNumCols = pRoster->GetNumCols();

            for(int j = 0; j < iNumCols; j++)
            {
                CDECol* pCol = pRoster->GetCol(j);
                int iNumFields = pCol->GetNumFields();
                for (int k = 0; k < iNumFields; k++)
                {
                    pCol->GetField(k)->SetFormNum (iNewFormLoc);
                }
            }
        }
    }
}

//      ***************************************************************************
// since we need to remove all Items for each form, let's call
// RemoveForm(i) to do it, as well as freeing itself

void CDEFormFile::RemoveAllForms()
{
    int max = GetNumForms();

    for (int i = 0; i < max; i++)
    {
        RemoveForm (0, false);  // false, don't renumber fields on subsequent forms
    }
}

// ***************** levels

void CDEFormFile::RemoveAllLevels()
{
    for( CDELevel* pLevel : m_aLevel )
    {
        pLevel->RemoveAllItems();
        delete pLevel;
    }

    m_aLevel.clear();
}

// boy is this rubbish; if i delete a form (and it's associated groups), i've got to
// update all the ptrs that reside on subsequent forms; ok, no prob, can do that
// easily enuf in the RemoveForm() func (which i've done there); and i need to do
// *some*thing there, as it has ptrs to text blks that the group has no knowledge
// of; however, the group ptrs themselves can't get updated via the form, so i've
// go to do it here
//
// so, traverse thru all the levels and all the groups to decrement all form index
// which is higher than the argument passed in (and go thru all the groups for
// while a form's index refers to it's array subscript, when i add that form it
// could be anywhere w/in the FF hierarchy, i.e., under Level 1 or 3, so i've got
// to check all groups)

void CDEFormFile::UpdateGroupFormIndices (int iStartingFormNo)
{
    int l, lMax = GetNumLevels();

    CDELevel* pLevel;

    for (l=0; l < lMax; l++)
    {
        pLevel = GetLevel(l);

        pLevel->GetRoot()->UpdateGroupFormIndices (iStartingFormNo);
    }
}


std::vector<CDEField*> CDEFormFile::GetAllFields() const
{
    std::vector<CDEField*> fields;

    std::function<void(const CDEGroup*)> group_iterator =
        [&](const CDEGroup* pGroup)
    {
        for( int i = 0; i < pGroup->GetNumItems(); i++ )
        {
            CDEItemBase* pItem = pGroup->GetItem(i);

            if( pItem->GetItemType() == CDEFormBase::Field )
                fields.emplace_back(assert_cast<CDEField*>(pItem));

            else if( pItem->GetItemType() == CDEFormBase::Roster )
                group_iterator(assert_cast<CDERoster*>(pItem));
        }
    };

    for( int i = 0; i < GetNumLevels(); i++ )
    {
        const CDELevel* pLevel = GetLevel(i);

        for( int j = 0; j < pLevel->GetNumGroups(); j++ )
            group_iterator(pLevel->GetGroup(j));
    }

    return fields;
}


// **************************************************************************
// when developing my form, i need to know if an item being dragged from
// the dictionary and dropped on to a page is extant within that form file
//
// so, this returns TRUE if i've found the item as a keyed field on a form
//
// *NOTE* the name being passed in is the unique DICTIONARY name

bool CDEFormFile::FindItem(const CString& sDictName) const
{
    auto field_matches = [&](const CDEField* pField)
    {
        return ( sDictName.CompareNoCase(pField->GetItemName()) == 0 && !pField->IsMirror() );
    };

    for( int f = 0; f < GetNumForms(); ++f )
    {
        const CDEForm* pForm = GetForm(f);

        for( int i = 0; i < pForm->GetNumItems(); ++i )
        {
            const CDEItemBase* pItem = pForm->GetItem(i);

            if( pItem->GetItemType() == CDEFormBase::Field )
            {
                const CDEField* pField = assert_cast<const CDEField*>(pItem);

                if( field_matches(pField) )
                    return true;
            }

            else if( pItem->GetItemType() == CDEFormBase::Roster )
            {
                const CDERoster* pGrid = assert_cast<const CDERoster*>(pItem);

                for( int iColNum = 0; iColNum < pGrid->GetNumCols(); ++iColNum )
                {
                    const CDECol* pCol = pGrid->GetCol(iColNum);

                    for( int iFldNum = 0; iFldNum < pCol->GetNumFields(); ++iFldNum )
                    {
                        if( field_matches(pCol->GetField(iFldNum)) )
                            return true;
                    }
                }
            }
        }
    }

    return false;
}

// **************************************************************************
// this func is, so far, for serpro (ruben, you owe me :); if the unique dict
// name is passed in, see if it exists (at this point in any state--keyed,
// protected, or otherwise) and if so, set the argument's ptr to the correct
// form; rtn NULL otherwise
//
// *NOTE* the name being passed in is the unique DICTIONARY name, not the
//                form file field's unique name
//
// RUBEN: when you get to the point that you need occurrences, i.e., the func
// is the same as the one above (except for the params), then let me know,
// cause i want to make them both call in to one func; SMG

bool CDEFormFile::FindItem(const CString& sDictName, CDEForm** pLocatedForm)
{
    int fMax = GetNumForms();
    bool bItemFound = false;
    CDEForm* pForm = nullptr;

    for (int f = 0; f < fMax && !bItemFound; f++)
    {
        pForm = GetForm(f);

        int iMax = pForm->GetNumItems();

        for (int i = 0; i < iMax && !bItemFound; i++)
        {
            CDEItemBase* pItem = pForm->GetItem(i);

            CDEFormBase::eItemType eItem = pItem->GetItemType();

            if (eItem == CDEFormBase::Field)
            {
                CDEField* pField = assert_cast<CDEField*>(pItem);

                if (pField->GetItemName() == sDictName)
                {
                    bItemFound = true;
                }
            }
            else if(eItem == CDEFormBase::Roster)
            {
                CDERoster* pGrid = assert_cast<CDERoster*>(pItem);

                int iColNum, iNumCols = pGrid->GetNumCols();
                CDECol* pCol;
                CDEField* pField;

                for (iColNum=0; iColNum < iNumCols && !bItemFound; iColNum++)
                {
                    pCol = pGrid->GetCol (iColNum);

                    int iFldNum, iNumFlds = pCol->GetNumFields();

                    for (iFldNum=0; iFldNum < iNumFlds && !bItemFound; iFldNum++)
                    {
                        pField = pCol->GetField (iFldNum);
                        if (pField->GetItemName() == sDictName)
                        {
                            bItemFound = true;
                        }
                    }
                }
            }
        }
    }

    *pLocatedForm = bItemFound ? pForm : nullptr;

    return bItemFound;
}

// **************************************************************************
// basically, i've got the name and the form # but want the form's index to
// the item
//
// *NOTE* the name being passed in is the unique FORM FILE name

bool CDEFormFile::FindItem(const CString& sFFName, int iFormIndex, int* iItemIndex) const
{
    const CDEForm* pForm = GetForm(iFormIndex);

    for( int i = 0; i < pForm->GetNumItems(); ++i )
    {
        if( sFFName.CompareNoCase(pForm->GetItem(i)->GetName()) == 0 )
        {
            *iItemIndex = i;
            return true;
        }
    }

    *iItemIndex = NONE;
    return false;
}

// **************************************************************************
// i would also like to be able to find an Item based on its coordinates; so,
// passing in a ptr to the form on which the click occurred and the (x,y) of
// the clicked point, see if it falls within any region; if so, rtn the ptr
// in the pItem param else NULL
//
// *NOTE* the name being passed in is the unique FORM FILE name
bool CDEFormFile::FindItem(CDEForm* pForm, CPoint cPT, CDEItemBase** pItem)
{
    if( pForm == NULL )
        return false;

    CRect curRect;

    for( int i = 0; i < pForm->GetNumItems(); ++i )
    {
        CDEItemBase* pLocal = pForm->GetItem(i);

        switch (pLocal->GetItemType())
        {
        case CDEFormBase::Field      : // union the data entry box & it's assoc text blk, as i just
            // want to rtn a ptr to the field, not it's text blk

            curRect = pLocal->GetDims();
            // smg: now that a field's text and DE box are independent, don't union
            if (!curRect.PtInRect(cPT))
                curRect = assert_cast<CDEField*>(pLocal)->GetTextDims();
            break;
        case CDEFormBase::Roster     : // fall thru
        case CDEFormBase::Text       : curRect = pLocal->GetDims(); break;
        case CDEFormBase::Group      : // not applicable, a group doesn't have an associated physical location
            break;
        }

        if( curRect.PtInRect(cPT) )
        {
            *pItem = pLocal;

            if( pLocal->GetItemType() == CDEFormBase::Field )
            {
                if( assert_cast<CDEField*>(pLocal)->GetCDEText().GetDims().PtInRect(cPT) )
                    *pItem = &assert_cast<CDEField*>(pLocal)->GetCDEText();
            }

            return true;
        }
    }

    return false;
}

// **************************************************************************
// a more robust find; in the event i need the ptrs to locate each layer
//
// *NOTE* the name being passed in is the unique FORM FILE name

bool CDEFormFile::FindItem(const CString& sFFName, CDEForm** pForm, CDEItemBase** pItem)
{
    for( int f = 0; f < GetNumForms(); ++f )
    {
        *pForm = GetForm(f);

        for( int i = 0; i < (*pForm)->GetNumItems(); ++i )
        {
            *pItem = (*pForm)->GetItem(i);

            if( sFFName.CompareNoCase((*pItem)->GetName()) == 0 )
                return true;
        }
    }

    return false;
}

// **************************************************************************
// rtn the index to the form, given it's unique name

int CDEFormFile::FindForm(const CString& sName, bool bByGroupName ) // RHF Nov 28, 2002 Add bByGroupName
{
    int i, max = GetNumForms();

    CDEForm* pForm;

    for( i = 0; i < max; i++ ){
        pForm = GetForm(i);

        if(pForm ) {
            // RHF INIC Nov 28, 2002
            if( bByGroupName ) {
                CDEGroup*   pGroup=pForm->GetGroup();
                if( pGroup != NULL && pGroup->GetName() == sName )
                    return i;
            }
            // RHF END Nov 28, 2002
            else {
                if( pForm->GetName() == sName)
                    return i;
            }
        }
    }
    return NONE;   // essentially, err condition
}


// return a ptr to the roster, and it's index too
CDERoster* CDEFormFile::FindRoster(const CString& sRosterName)
{
    int max = GetNumForms();

    for (int i = 0; i < max; i++)
    {
        CDEForm* pForm = GetForm(i);
        int jMax = pForm->GetNumItems();

        for (int j = 0; j < jMax; j++)
        {
            CDEFormBase* pItem = pForm->GetItem(j);

            if (pItem->GetName() == sRosterName)
                return assert_cast<CDERoster*>(pItem);
        }
    }

    return nullptr;
}


/********************************************************************************************
APPLICATION RUNTIME FUNCTIONS
*********************************************************************************************/
// load the run-time CDataDicts
// Entry Run Uses this. The client application is responsible for the cleanup
//

bool CDEFormFile::LoadRTDicts(CAppLoader* pLoader)
{
    // Add the runtime dicts again
    auto dictionary = std::make_shared<CDataDict>();
    m_dictionary = dictionary;

    if (pLoader && pLoader->GetBinaryFileLoad())
    {
        try // 20121115 for the portable environment
        {
            APP_LOAD_TODO_GetArchive() & *dictionary;
            dictionary->BuildNameList();
            dictionary->UpdatePointers();
        }

        catch(...)
        {
            ErrorMessage::Display(FormatText(MGF::GetMessageText(MGF::ErrorReadingPen).c_str(), pLoader->GetArchiveName().c_str()));
            return false;
        }
    }
    else
    {
        try
        {
            dictionary->Open(m_dictionaryFilename, true);
        }

        catch( const CSProException& exception )
        {
            ErrorMessage::Display(FormatText(_T("Failed to load dictionary %s (%s)"), m_dictionaryFilename.GetString(),
                                                                                      exception.GetErrorMessage().c_str()));
            return false;
        }
    }

    return true;
}

#ifdef GENERATE_BINARY
bool CDEFormFile::SaveRTDicts(const std::wstring& archive_name)
{
    ASSERT(BinaryGen::isGeneratingBinary());
    if( !BinaryGen::isGeneratingBinary() )
        return true;

    try // 20121109 for the portable environment
    {
        APP_LOAD_TODO_GetArchive() & *std::const_pointer_cast<CDataDict>(m_dictionary);
    }

    catch(...)
    {
        ErrorMessage::Display(FormatText(_T("There was an error writing to the binary file %s"), archive_name.c_str()));
        return false;
    }

    return true;
}
#endif // GENERATE_BINARY


bool CDEFormFile::SetDictItem(CDEField* pField)
{
    //Search thru the dicts for the dict item
    if( m_dictionary != nullptr && SO::EqualsNoCase(pField->GetItemDict(), m_dictionary->GetName()) )
    {
        const CDictItem* dict_item = m_dictionary->LookupName<CDictItem>(pField->GetItemName());

        if( dict_item != nullptr )
        {
            pField->SetDictItem(dict_item);
            return true;
        }
    }

    pField->SetDictItem(NULL); //Inititalise it to NULL 'cos you are getting stray pointers;
    return false;
}


void CDEFormFile::UpdateDictItemPointers(CDEGroup* parent)
{
    for (int i = 0; i < parent->GetNumItems(); i++)
    {
        CDEItemBase* pItem = parent->GetItem(i);

        CDEFormBase::eItemType eItem = pItem->GetItemType();

        if (eItem == CDEFormBase::Group || eItem == CDEFormBase::Roster) {
            UpdateDictItemPointers(assert_cast<CDEGroup*>(pItem));
        }
        else if (eItem != CDEFormBase::Block) { // it's a field
            SetDictItem(assert_cast<CDEField*>(pItem));
        }
    }
}


std::vector<CString> CDEFormFile::GetOrder() const
{
    // add the form file name
    std::vector<CString> proc_names = { GetName() };

    std::function<void(const CDEGroup*)> get_order_for_group;

    get_order_for_group = [&](const CDEGroup* pGroup)
    {
        proc_names.emplace_back(pGroup->GetName());

        for( int i = 0; i < pGroup->GetNumItems(); ++i )
        {
            const CDEItemBase* pItem = pGroup->GetItem(i);
            CDEFormBase::eItemType eItem = pItem->GetItemType();

            if( eItem == CDEFormBase::Group || eItem == CDEFormBase::Roster )
            {
                get_order_for_group(assert_cast<const CDEGroup*>(pItem));
            }

            else    // it's a field
            {
                proc_names.emplace_back(pItem->GetName());
            }
        }
    };

    // add each level (and then the level's group)
    for (int iIndex = 0; iIndex < GetNumLevels(); iIndex++) {
        const CDELevel* pLevel = GetLevel(iIndex);
        proc_names.emplace_back(pLevel->GetName());
        get_order_for_group(pLevel->GetRoot());
    }

    return proc_names;
}


#ifdef USE_BINARY
#else
/////////////////////////////////////////////////////////////////////////////
////SAVY 07/25
//USE THIS FUNC FOR ORDER RECONCILE
//               CDEFormFile::OReconcile
//
//  reconciles the order file with the dictionary(s)
//
//  if there is a problem, keep the error message string in "csErr", so one can
//  can call this silently and get at the error message later
//
//  bSilent means no message boxes, if true
//  bAutoFix means make adjustments to this object or not
//
//  If a field on a form is not found in the data dictionary, it is removed from
//  the form, if bAutoFix = true
//
//  TODO:  more checking with dictionary; make sure all rules are followed governing
//         singles/multiples, etc.
//
/////////////////////////////////////////////////////////////////////////////
bool CDEFormFile::OReconcile(CString& csErr, bool bSilent, bool bAutoFix)
{
    csErr = _T("");
    bool bRet = true;

    //Check and add Levels
    bRet = FCheckNAddLevels();
    if(bRet && !bAutoFix && !IsDictOrder()) {
        if(!bSilent && !csErr.IsEmpty()){
            ErrorMessage::Display(csErr);
        }
        return false;
    }
    csErr =_T("");
    int iLevels = GetNumLevels();

    for (int i = 0; i<iLevels; i++) {
        CDELevel* pLevel = GetLevel(i);
        CDEGroup* pGroup = pLevel->GetRoot();

        eReturnType eRet = pGroup->OReconcile(this, csErr, bSilent, bAutoFix);

        if (eRet == Delete) {
            // delete the group
            // (actually, this can't happen; it's the root group)

            bRet = false;
        }
        else if(eRet == Reconciled) {
            bRet = false;
        }
    }

    if (!bRet){
        if (!bSilent && !csErr.IsEmpty() && !IsDictOrder()) {
            ErrorMessage::Display(csErr);
        }
    }
    if(true) { //if some thing has changed renumber the forms
        RenumberFormsNItems4BCH();
    }
    if(!CheckNAddMissingItems())
        bRet = false;

    if(!bRet && !IsDictOrder()) { //if some thing has changed renumber the forms
        // this->RenumberFormsAndItems();
        RenumberFormsNItems4BCH();
    }
    else if(!bRet && IsDictOrder()){
        //just always force it to regenerate the stuff
        if(m_dictionary !=nullptr){
            ErrorMessage::Display(_T("Items in dictionary changed, items will be processed in the new dictionary order."));
            CreateOrderFile(*m_dictionary, true);
        }
    }
    if(bRet && IsDictOrder() && m_dictionary != nullptr) {
        CDEFormFile formFile;
        formFile.SetDictionary(m_dictionary);
        formFile.CreateOrderFile(*m_dictionary, true);
        formFile.UpdatePointers();
        if(!this->Compare(&formFile)){
            ErrorMessage::Display(_T("Items in dictionary changed, items will be processed in the new dictionary order."));
            CreateOrderFile(*m_dictionary, true);
            bRet =false;
        }
    }
    return bRet;
}

void CDEFormFile::CreateGroupForOrder(CDEGroup* pGroup,
                                      const CDictRecord* pDictRec,
                                      int iFormNum,
                                      bool bIdRec /* = false*/,
                                      CDEGroup* pPrevGroup /* =NULL */)
{
    int row = rowOffset;
    LONG rightCol = 0;
    CString RecTypeVal;

    CString sDictName = pDictRec->GetDataDict()->GetName();

    pGroup->SetFormNum  (iFormNum);
    CString sRecName = pDictRec->GetName();

    // get rid of any leading underscores...which happens w/Level ID record
    while (sRecName[0] == '_')  {
        sRecName.Delete (0, 1);         // nIndex, nCount
    }

    sRecName += _T("_EDT");                 // append the string "_FRM"

    // if nobody else is using this name
    if (IsNameUnique (sRecName)){
        pGroup->SetName         (sRecName);             // this doesn't insert the name in to the uniq name list
        AddUniqueName(sRecName);
    }
    else {

        pGroup->SetName         ( CreateUniqueName(sRecName));
    }

    if (pDictRec == NULL){              // bail; most likely a new dict being created w/the new form
        return;
    }

    pGroup->SetRequired(pDictRec->GetRequired());
    pGroup->SetLabel    (pDictRec->GetLabel());
    pGroup->SetTypeName(pDictRec->GetName());

    RecTypeVal = pDictRec->GetRecTypeVal();
    if (bIdRec)         {
        pGroup->SetMaxLoopOccs (1);
    }
    else{
        pGroup->SetRIType (Record);
        pGroup->SetMaxLoopOccs (pDictRec->GetMaxRecs());
    }

    //  first, loop thru all LevelIDs on prev group, if any
    if (pPrevGroup){
        // the field's protected, so generate a unique name
        CDEField* pField = nullptr;

        for (int i = 0; i < pPrevGroup->GetNumItems(); i++){
            pField = new CDEField();

            *pField = *assert_cast<CDEField*>(pPrevGroup->GetItem(i));
            pField->SetItemDict(sDictName);
            pField->SetName( CreateUniqueName(pField->GetItemName()) );       //adds the uniq name to the NameList

            pField->IsMirror (true);
            pField->SetFormNum (iFormNum);
            pField->SetParent(pGroup);

            pGroup->AddItem(pField);

            rightCol = std::max (rightCol, pField->GetDims().right);
            rightCol = std::max (rightCol, pField->GetCDEText().GetDims().right);
        }
        ASSERT(pField != nullptr);
        row = pField->GetDims().top + rowOffset;
    }

    //  loop thru all Items assoc w/this record
    for (int i1 = 0; i1 < pDictRec->GetNumItems(); i1++) {
        // otherwise, use the dictionary name to be unique
        const CDictItem* pDictItem = pDictRec->GetItem(i1);
        if( !pDictItem->AddToTreeFor80() )
            continue;
        if (pDictItem->GetItemType() == ItemType::Subitem && pDictItem->GetOccurs() == 1) {
            CDEField* pField = new CDEField(pDictItem->GetName(),sDictName);
            pField->SetLabel(pDictItem->GetLabel());
            pField->SetItemDict(sDictName);
            CDEGroup* pParentGroup = pGroup;
            const CDictRecord* pRecord = pDictItem->GetRecord();
            int iStart  = i1 - 1;
            while(iStart >= 0 ) {
                if(pRecord->GetItem(iStart)->GetItemType() == ItemType::Item){
                    CString sFindName = pRecord->GetItem(iStart)->GetName();
                    if(pRecord->GetItem(iStart)->GetOccurs() > 1){
                        pParentGroup = this->FindItemGroup(pGroup,sFindName,true);
                    }
                    else {
                        pParentGroup = this->FindItemGroup(pGroup,sFindName);
                    }
                    ASSERT(pParentGroup);
                    break;
                }
                iStart --;
            }
            pField->SetParent(pParentGroup);
            pField->SetFormNum(pParentGroup->GetFormNum());
            pParentGroup->AddItem(pField);
            continue;
        }

        // create an entry for each occurrence...
        if (pDictItem->GetOccurs() > 1) {
            // forced occ fields to be done; now, must force user to get a roster;
            // eventually, when occurring item w/in occurring record implemented,
            // give users the choice!
            CDEGroup* pRetGroup = OCreateGroupField(pGroup,pDictItem,iFormNum);
            pGroup->AddItem(pRetGroup);
            // CreateGroupOccurrenceFields (pGroup, pDictItem, iFormNum, &row, pDC, drag_options);
            //ASSERT(FALSE); //THIS IS NOT YET IMPLEMENTED.
            //IMPEMENT WITH THE NEWROSTER STUFF WHICH IS BEING CREATED.
            // CreateGroupRosterField (pGroup, pDictItem, iFormNum, &row, pDC, drag_options);
        }
        else {   // item has no occurrences

            //FIX THIS LATER
            //FOR NOW SUBITEMS DROP IS FALSE;

            //HOW DO THE SUBITEMS GO ON TO THE ORDER FILE . IS THERE A LIMITATION LIKE FORMS STUFF

            //   bool bSubitemDropOK = TestSubitemDrop (pDictRec, pDictItem, drag_options);
            bool bSubitemDropOK = false;

            if (bSubitemDropOK){
                //              int iNumKids = GetNumDictSubitems (pDictRec, pDictItem);
                int maxItems = pDictRec->GetNumItems();
                int iStartIndex = pDictItem->GetSonNumber()+1;
                int iItemLen;

                bool bDone = false;

                for (int i2=iStartIndex; i2 < maxItems && !bDone; i2++){
                    const CDictItem* pSubItem = pDictRec->GetItem(i2);
                    iItemLen = pSubItem->GetLen();

                    if (pSubItem->GetItemType() == ItemType::Item){       // all done w/subitems :)
                        bDone = true;
                        continue;
                    }

                    CDEField* pField = new CDEField(pDictItem->GetName(), sDictName) ;
                    pField->SetItemDict(sDictName);
                    pField->SetLabel(pDictItem->GetLabel());
                    pField->SetFormNum(iFormNum);
                    AddUniqueName(pField->GetName());
                    pField->SetParent(pGroup);
                    pGroup->AddItem(pField);

                    row += rowOffset;
                    rightCol = std::max (rightCol, pField->GetDims().right);
                    rightCol = std::max (rightCol, pField->GetCDEText().GetDims().right);
                }
            }
            else {      // else just drop the item itself
                //Order File does not use pDC info
                CDEField* pField = new CDEField(pDictItem->GetName(), sDictName) ;
                pField->SetFormNum(iFormNum);
                pField->SetItemDict(sDictName);
                pField->SetLabel(pDictItem->GetLabel());
                AddUniqueName(pField->GetName());

                pField->SetParent(pGroup);

                pGroup->AddItem(pField);
                //              CDEForm* pForm = this->GetForm(iFormNum);
                row += rowOffset;
                rightCol = std::max (rightCol, pField->GetDims().right);
                rightCol = std::max (rightCol, pField->GetCDEText().GetDims().right);
            }
        }
    }

    rightCol = std::max (rightCol, MaxScrSz);
    if (iFormNum >= 0){
        pGroup->SetDims (0,0,rightCol,(row > MaxScrSz) ? row : MaxScrSz);
    }
}
#endif // USE_BINARY


#ifdef USE_BINARY
#else
///////////////////////////////////////////////////////////////////////////////////////////////
//  //CALL THIS FUNCTION AFTER A CALL TO CHECKNADDLEVELS
//                  void CDEFormFile::CheckNAddMissingItems()
//
///////////////////////////////////////////////////////////////////////////////////////////////
bool CDEFormFile::CheckNAddMissingItems()
{
    //Assumes all the levels are in the formfile
    //Now check and add records and items for each level

    bool bRet = true; // meaning no change has taken place
    ASSERT(m_dictionary);

    for( size_t level_number = 0; level_number < m_dictionary->GetNumLevels(); ++level_number )
    {
        const DictLevel& dict_level = m_dictionary->GetLevel(level_number);
        CDELevel* pOrderLevel = this->GetLevel(level_number);
        int iCompare = pOrderLevel->GetName().CompareNoCase(dict_level.GetName());
        ASSERT(iCompare == 0 );

        //Check the IDItems Record
        const CDictRecord* pIDRecord = dict_level.GetIdItemsRec();

        CDEGroup* pRecGroup1 = FindGroup(pOrderLevel->GetRoot(),pIDRecord->GetName());
        if(!pRecGroup1)  {
            //Go through the items and find the group which has any of these items
            for(int iItem=0; iItem<pIDRecord->GetNumItems();iItem++) {
                pRecGroup1 = FindItemGroup(pOrderLevel->GetRoot(),pIDRecord->GetItem(iItem)->GetName());
                if(pRecGroup1)
                    break;
            }
        }
        if(pRecGroup1) {
            //Ensure all the items are present .If not add them to the correct group
            if(!EnsureAllItemsPresent(pOrderLevel,pIDRecord) )
                bRet =false;
        }
        else {
            //Create a group   corresponding to this record and add all items
            bRet = false;
            int iPageNum;
            CDEGroup* pGroup = new CDEGroup();
            int iGroupIndex = pOrderLevel->GetNumGroups() -1;
            iPageNum = 0;
            if(iGroupIndex < 0 ) {
                iPageNum = 0;
            }
            else {
                iPageNum = pOrderLevel->GetGroup(iGroupIndex)->GetFormNum()+1;
            }
            this->CreateGroupForOrder(pGroup,pIDRecord,iPageNum,true );
            pGroup->SetTypeName(pIDRecord->GetName());
            pGroup->SetRIType (Record);
            pGroup->SetMaxLoopOccs (pIDRecord->GetMaxRecs());

            pOrderLevel->AddGroup(pGroup);
            CreateForm(level_number, pGroup);                // pass in the curr level

            // I Should reset all the other page numbers . Do it Later
            this->RenumberFormsAndItems();

            ErrorMessage::Display(FormatText(_T("%s added; new in dictionary"), pIDRecord->GetName().GetString()));
        }

        //Check for each record and for each item in the dict
        for (int iRec = 0; iRec < dict_level.GetNumRecords(); iRec++) {
            const CDictRecord* pRecord = dict_level.GetRecord(iRec);
            CDEGroup* pRecGroup2 = FindGroup(pOrderLevel->GetRoot(),pRecord->GetName());
            if(!pRecGroup2)  {
                //Go through the items and find the group which has any of these items
                for(int iItem=0; iItem<pRecord->GetNumItems();iItem++) {
                    pRecGroup2 = FindItemGroup(pOrderLevel->GetRoot(),pRecord->GetItem(iItem)->GetName());
                    if(pRecGroup2)
                        break;
                }
            }
            if(pRecGroup2) {
                //Ensure all the items are present .If not add them to the correct group
                if(!EnsureAllItemsPresent(pOrderLevel,pRecord) )
                    bRet =false;
            }
            else {
                //Create a group   corresponding to this record and add all items
                bRet = false;
                int iPageNum;
                CDEGroup* pGroup = new CDEGroup();
                int iGroupIndex = pOrderLevel->GetNumGroups() -1;

                if(iGroupIndex < 0 ) {
                    iPageNum = 0;
                }
                else {
                    iPageNum = pOrderLevel->GetGroup(iGroupIndex)->GetFormNum()+1;
                }
                this->CreateGroupForOrder(pGroup,pRecord,iPageNum,false );
                pGroup->SetTypeName(pRecord->GetName());
                pGroup->SetRIType (Record);
                pGroup->SetMaxLoopOccs (pRecord->GetMaxRecs());

                pOrderLevel->AddGroup(pGroup);
                CreateForm(level_number, pGroup);            // pass in the curr level

                // I Should reset all the other page numbers . Do it Later
                //                this->RenumberFormsAndItems();

                if(true) { //if some thing has changed renumber the forms
                    RenumberFormsNItems4BCH();
                }

                ErrorMessage::Display(FormatText(_T("%s added; new in dictionary"), pRecord->GetName().GetString()));
            }
        }
    }

    return bRet;
}
#endif // USE_BINARY

//The record which has the sRecName as the TypeName . If it doesnt  have one create a group
//with this type name
CDEGroup* CDEFormFile::FindGroup(CDEGroup* pRootGroup, const CString& sRecName)
{
    CDEGroup* pRetGroup = NULL;
    ASSERT(pRootGroup);
    for(int iIndex = 0; iIndex < pRootGroup->GetNumItems(); iIndex ++) {
        CDEGroup* pGroup = DYNAMIC_DOWNCAST(CDEGroup,pRootGroup->GetItem(iIndex));
        if(pGroup) {
            if(pGroup->GetTypeName().CompareNoCase(sRecName)==0 ) {
                pRetGroup = pGroup;
            }
            else{
                pRetGroup = FindGroup(pGroup,sRecName);
            }
        }
        if(pRetGroup)
            break;
    }
    return pRetGroup;
}

//find the group which has a field corresponding to this  item name
CDEGroup* CDEFormFile::FindItemGroup(CDEGroup* pRootGroup, const CString& sItemName ,bool bUseTypeName /*=false*/)
{
    CDEGroup* pRetGroup = NULL;
    ASSERT(pRootGroup);
    if(bUseTypeName) {
        if(pRootGroup->GetTypeName() == sItemName){
            return pRootGroup;
        }
    }
    for(int iIndex = 0; iIndex < pRootGroup->GetNumItems(); iIndex ++) {
        CDEField* pField = DYNAMIC_DOWNCAST(CDEField,pRootGroup->GetItem(iIndex));
        if(bUseTypeName){
            if(pRootGroup->GetTypeName() == sItemName){
                pRetGroup  = pRootGroup;
                break;
            }

        }
        else if(pField && pField->GetItemName().CompareNoCase(sItemName)==0) {
            pRetGroup = pRootGroup;
            break;
        }

        CDEGroup* pGroup = DYNAMIC_DOWNCAST(CDEGroup,pRootGroup->GetItem(iIndex));
        if(pGroup) {
            pRetGroup = FindItemGroup(pGroup,sItemName,bUseTypeName); //recurse through the groups
        }
        if(pRetGroup){
            break;
        }
    }
    return pRetGroup;
}

//Assumes at least one item of the record is contained by a group in pLevel
bool CDEFormFile::EnsureAllItemsPresent(CDELevel* pLevel, const CDictRecord* pRecord)
{
    bool bRet = true ; //Nothing has changed
    CDEGroup* pRGroup = pLevel->GetRoot();
    CDEGroup* pLastGroup =NULL;
    CString sDictName = m_dictionary->GetName();
    ASSERT(pRGroup);
    for(int iItem1 = 0; iItem1<pRecord->GetNumItems(); iItem1++) {
        const CDictItem* pItem = pRecord->GetItem(iItem1);
        ASSERT(pItem);
        if( !pItem->AddToTreeFor80() )
            continue;
        CString sFindName1 = pItem->GetName();
        CDEGroup* pRetGroup1 = NULL;
        if(pItem->GetOccurs() > 1 && pItem->GetItemType() == ItemType::Item) {//if item occurs look for the item's looping group with typename set to true
            pRetGroup1 = this->FindItemGroup(pRGroup,sFindName1,true);
        }
        else {
            pRetGroup1 = this->FindItemGroup(pRGroup,sFindName1);
        }

        if(pRetGroup1 ) { //if Item exists
            pLastGroup = pRetGroup1;
            continue;
        }
        else  {
            if(!pLastGroup) {
                for(int iItem2 = 0; iItem2<pRecord->GetNumItems(); iItem2++)  {
                    pLastGroup = this->FindItemGroup(pRGroup,pRecord->GetItem(iItem2)->GetName());
                    if(pLastGroup)
                        break;
                }
            }
            if(!pLastGroup) {
                //finally look @ the possiblity of the group being there but none of the items present
                pLastGroup = this->FindGroup(pRGroup,pRecord->GetName());
            }
            ASSERT(pLastGroup); //There should be atleast one
            //Add this CDEField which is absent in to this group
            if(pRecord->GetItem(iItem1)->GetOccurs()>1){
                if( pLastGroup && (pLastGroup->GetRIType() == CDEFormBase::Item || pLastGroup->GetRIType() == CDEFormBase::SubItem )){
                    pLastGroup = pLastGroup->GetParent();
                }
                //check if the item has subitems and then check if the group with this type name
                //exists
                if(GetNumDictSubitems(pRecord,pRecord->GetItem(iItem1)) != 0) {
                    CDEGroup* pGroup  = this->FindItemGroup(pRGroup,sFindName1,true);
                    if(pGroup)
                        continue;
                }
                CDEGroup* pRetGroup2 = OCreateGroupField (pLastGroup,pRecord->GetItem(iItem1),pLastGroup->GetFormNum());
                pLastGroup->AddItem(pRetGroup2);

                CDEForm* pForm = this->GetForm(pLastGroup->GetFormNum());
                //ASSERT(pForm);

                if( pForm ) // 20120521 changing the name of a repeating item was resulting in a NULL pForm
                {
                    pRetGroup2->SetFormName(pForm->GetName());
                    pForm->AddGroupItems(pRetGroup2);
                }

                else
                {
                    // figure out what level we're on
                    int level = 0;

                    for( ; level < GetNumLevels(); level++ )
                    {
                        if( GetLevel(level) == pLevel )
                            break;
                    }

                    CreateForm(level,pRetGroup2); // 20120521
                }
            }
            else {
                if(pLastGroup &&( pLastGroup->GetRIType() == CDEFormBase::Item || pLastGroup->GetRIType() == CDEFormBase::SubItem )){
                    pLastGroup = pLastGroup->GetParent();
                }
                CDEField* pField = new CDEField(pRecord->GetItem(iItem1)->GetName(),sDictName);
                pField->SetLabel(pRecord->GetItem(iItem1)->GetLabel());
                if(pField->GetFieldLabelType() == FieldLabelType::DictionaryName){
                    pField->GetCDEText().SetText(pRecord->GetItem(iItem1)->GetName());
                }
                else if(pField->GetFieldLabelType() == FieldLabelType::DictionaryLabel){
                    pField->GetCDEText().SetText(pRecord->GetItem(iItem1)->GetLabel());
                }
                pField->SetItemDict(sDictName);
                CDEGroup* pParentGroup = pLastGroup;
                if(pItem->GetItemType() ==  ItemType::Subitem){
                    int iStart = iItem1 - 1;
                    while(iStart >= 0 ) {
                        if(pRecord->GetItem(iStart)->GetItemType() == ItemType::Item){
                            CString sFindName2 = pRecord->GetItem(iStart)->GetName();
                            if(pRecord->GetItem(iStart)->GetOccurs() > 1) {
                                pParentGroup = this->FindItemGroup(pRGroup,sFindName2,true);
                            }
                            else {
                                pParentGroup = this->FindItemGroup(pRGroup,sFindName2);
                            }
                            ASSERT(pParentGroup);
                            break;
                        }
                        iStart --;
                    }
                }
                pField->SetParent(pParentGroup);
                pField->SetFormNum(pParentGroup->GetFormNum());

                if(pParentGroup->GetItemType() == CDEItemBase::eItemType::Roster){
                    CDERoster* pRoster = assert_cast<CDERoster*>(pParentGroup);
                    CDECol* pCol = new CDECol();
                    CDEText text;
                    text.SetText (pField->GetLabel());
                    pCol->SetHeaderText (text);
                    pCol->AddField (pField);
                    pRoster->AddCol (pCol);
                    pRoster->FillItemPtrs2();
                }
                else {
                    pParentGroup->AddItem(pField);
                }

                CDEForm* pForm = this->GetForm(pParentGroup->GetFormNum());
                ASSERT(pForm);
                pForm->AddItem(pField);
            }
            /*CDEForm* pForm = this->GetForm(pLastGroup->GetFormNum());
            ASSERT(pForm);
            pForm->RemoveAllItems();
            pForm->AddGroupItems(pLastGroup);*/
            bRet = false;
        }
    }
    return bRet;
}

void CDEFormFile::RemoveGroupItemsFromForm(CDEGroup* pGroup ,bool bRemove /*=true*/)
{
    int iForm = pGroup->GetFormNum();

    //  CDEForm* pGForm = this->GetForm(iForm); smg: don't need, as (rd nxt line)
    //  pGForm->RemoveItem(pGForm->GetName());  you can't remove the form from itself!

    //Go thru the group items and removes them from form

    for (int iIndex = 0; iIndex < pGroup->GetNumItems(); iIndex++) {

        //loop through the items
        CDEItemBase* pBase = pGroup->GetItem(iIndex);
        if(pBase) {
            if(pBase->GetItemType() == CDEItemBase::eItemType::Group){
                RemoveGroupItemsFromForm(assert_cast<CDEGroup*>(pBase), bRemove);
            }

            int iItemForm = pBase->GetFormNum();
            CDEForm* pItemForm = this->GetForm(iForm);
            //////////////////////////////////////////////////////////////////////////
            // got an assert? see below
            ASSERT( pItemForm != 0 );                       // rcl, Nov 2004
            // TODO:
            // pItemForm becomes 0 when some renaming in dictionary has happened
            // but ord file did not rename its names accordingly, so a mess is here
            // What should we do?
            // in the meantime...                           // rcl, Nov 2004
            if( pItemForm == 0 )
                continue;                                   // rcl, Nov 2004
            //////////////////////////////////////////////////////////////////////////

            pItemForm->RemoveItem(pBase->GetName());

            if(bRemove && pItemForm->GetNumItems() == 0 && GetNumForms() > 1) {         // smg: don't delete the last form!!
                //Remove this form and Renumber
                RemoveForm (iItemForm);
                break; //savy once the form is removed you dont need to go through it any more
            }
            else {
                if (!pItemForm->AnyKeyedFieldsOnForm())
                {
                    pItemForm->SetRecordRepeatName(_T(""));
                    pGroup->SetLoopingVars();
                }
            }
        }
    }
}

//Check and add levels for forms
bool CDEFormFile::FCheckNAddLevels()
{
    bool bRet = true ; // Nothing changed

    if( m_dictionary == nullptr )
        return bRet;

    int iNDictLevels = (int)m_dictionary->GetNumLevels();

    for(int iDLevel =0 ; iDLevel < iNDictLevels;iDLevel++) {
        bool bFound = FindNMatchFLevel(*m_dictionary, iDLevel);
        if(!bFound)
        {
            int iPgNum =0 ;
            bRet = false;

            int iForm = 0;
            for(iForm = 0; iForm < this->GetNumForms(); iForm++){

                int iLevel = this->GetForm(iForm)->GetLevel();
                if(iLevel < iDLevel) //get total forms of all the levels above this one
                    iPgNum++;

            }
            //Go through all the forms whose level is greater than this and increment their level by one
            for(iForm = 0; iForm < this->GetNumForms() ; iForm++) {
                CDEForm* pForm = this->GetForm(iForm);
                if(pForm->GetLevel() > iDLevel) {
                    pForm->SetLevel(pForm->GetLevel()+1);
                }
            }

            CDELevel* pFormLevel = new CDELevel();
            this->InsertLevelAt(pFormLevel,iDLevel);

            const DictLevel& dict_level = m_dictionary->GetLevel(iDLevel);
            AddUniqueName(dict_level.GetName());

            pFormLevel->SetName(dict_level.GetName());
            pFormLevel->SetLabel(dict_level.GetLabel());

            CDEGroup* pGroup = new CDEGroup();  //create the root group
            pGroup->SetParent(pFormLevel->GetRoot());
            pFormLevel->AddItem(pGroup);
            CreateForm(iDLevel, pGroup);

            pFormLevel->SetHierarchy(iDLevel);
        }
    }

    //if the number of levels of the order are more than the number of the one's in dictionary
    //remove the last one
    while(iNDictLevels != this->GetNumLevels()) {
        //Delete the excess levels from the orders
        bRet = false;
        int iLevel = GetNumLevels()-1;
        ASSERT(iLevel >=0);

        CDELevel* pLevel = m_aLevel[iLevel];
        m_aLevel.erase(m_aLevel.begin() + iLevel);
        delete pLevel;
        //Search all the forms corresponidng to this level and delete them
        for(int iForm = this->GetNumForms() -1; iForm >=0; iForm--) {
            CDEForm* pForm = this->GetForm(iForm);
            if(pForm->GetLevel() == iLevel) {
                this->RemoveForm(iForm);
            }
        }
    }
    if(!bRet){
        RenumberFormsAndItems();
    }

    return bRet;
}

bool CDEFormFile::FindNMatchFLevel(const CDataDict& dictionary, int iDictLevel)
{
    const DictLevel& dict_level = dictionary.GetLevel(iDictLevel);
    CString sDLName = dict_level.GetName();
    bool bFound = false;
    for(int iFLevel =iDictLevel ; iFLevel < this->GetNumLevels(); iFLevel++) {
        CDELevel* pFLevel = this->GetLevel(iFLevel);
        CDEGroup* pRGroup = pFLevel->GetRoot();
        ASSERT(pRGroup);

        if(pRGroup->GetNumItems() ==0 && pFLevel->GetName().CompareNoCase(sDLName) ==0) {
            //See if the names are same if the levels do not have any forms
            bFound =true;
        }
        if(!bFound) {
            //go through all the items of the pDictLevel;
            const CDictRecord* pRec = dict_level.GetIdItemsRec();

            for(int iItem = 0; iItem < pRec->GetNumItems(); iItem++) {
                CString sItemName = pRec->GetItem(iItem)->GetName();
                CDEForm* pForm = NULL;
                CDEItemBase* pBase = NULL;
                this->FindItem(sItemName,&pForm,&pBase);

                if(pBase && pBase->GetItemType() == CDEItemBase::eItemType::Field){
                    if( assert_cast<CDEField*>(pBase)->IsKeyed() && pForm->GetLevel() >=iDictLevel ){
                        bFound = true;
                        iFLevel = pForm->GetLevel();
                        pFLevel = this->GetLevel(iFLevel);
                        break;
                    }
                }
            }
            //check the groups items against the dict record names
            //if they match then we found the match
            if(!bFound){
                //Check for each record and for each item in the dict
                for (int iRec = 0; iRec < dict_level.GetNumRecords(); iRec++) {
                    pRec = dict_level.GetRecord(iRec);
                    for(int iItem = 0; iItem < pRec->GetNumItems(); iItem++) {
                        CString sItemName = pRec->GetItem(iItem)->GetName();
                        CDEForm* pForm = NULL;
                        CDEItemBase* pBase = NULL;
                        this->FindItem(sItemName,&pForm,&pBase);

                        if(pBase && pBase->GetItemType() == CDEItemBase::eItemType::Field){
                            if( assert_cast<CDEField*>(pBase)->IsKeyed() && pForm->GetLevel() >=iDictLevel ){
                                bFound = true;
                                iFLevel = pForm->GetLevel();
                                pFLevel = this->GetLevel(iFLevel);

                                break;
                            }
                        }
                    }
                    if(bFound)
                        break;
                }
            }

        }

        //see to it that the form level name matches the dict level name
        //and the positions match too
        if(bFound) {
            pFLevel->SetName(dict_level.GetName());
            if(iDictLevel != iFLevel) {

                //HERE YOU CHANGE THE FORM'S  LEVEL NUMS WITH THE NEW ONE
                CDELevel* pTLevel = m_aLevel[iDictLevel];
                std::vector<CDEForm*> arrTempForm;

                int iForm = 0;

                for( iForm = 0; iForm< this->GetNumForms(); iForm++) {
                    CDEForm* pForm = this->GetForm(iForm);
                    if(pForm->GetLevel() == iDictLevel) {
                        arrTempForm.emplace_back(pForm);
                    }
                }
                m_aLevel[iDictLevel] = pFLevel;
                m_aLevel[iFLevel] = pTLevel;

                //Change the level number for the forms to this one
                for(iForm = 0; iForm< this->GetNumForms(); iForm++) {
                    CDEForm* pForm = this->GetForm(iForm);
                    if(pForm->GetLevel() == iFLevel) {
                        pForm->SetLevel(iDictLevel);
                    }
                }

                //Change the level number for the forms to this one
                for(iForm = 0; iForm < (int)arrTempForm.size(); iForm++) {
                    CDEForm* pForm = arrTempForm[iForm];
                    pForm->SetLevel(iFLevel);
                }
            }
        }

        if(bFound)
            break;
    }
    return bFound;
}

//GET FIELD FUNCTION FROM RUBEN
//function start
CDEField* CDEFormFile::GetField(int iSym)
{
    for( int iForm = 0; iForm < GetNumForms(); ++iForm )
    {
        CDEField* pField = GetForm(iForm)->GetField(iSym);

        if( pField != nullptr )
            return pField;
    }

    return nullptr;
}

/////////////////////////////////////////////////////////////////////////////////
//
//CDEGroup* CDEFormFile::OCreateGroupField (CDEGroup* pGroup, const CDictItem* pDI, int iFormNum)
//
/////////////////////////////////////////////////////////////////////////////////
CDEGroup* CDEFormFile::OCreateGroupField(CDEGroup* pGroup, const CDictItem* pDI, int iFormNum)
{
    CString sDictName = pDI->GetRecord()->GetDataDict()->GetName();

    CDEGroup* pFieldOccGroup = new CDEGroup();
    pFieldOccGroup->SetParent(pGroup);
    pFieldOccGroup->SetFormNum(iFormNum);
    pFieldOccGroup->SetMaxLoopOccs(pDI->GetOccurs());
    pFieldOccGroup->SetTypeName(pDI->GetName());

    // gsf 01/24/01
    bool bProcess = true;
    if (pDI->GetItemType() == ItemType::Item) {
        pFieldOccGroup->SetRIType(Item);
        if(GetNumDictSubitems(pDI->GetRecord(),pDI) != 0){
            bProcess = false;
        }
    }
    else {
        pFieldOccGroup->SetRIType(SubItem);
    }

    //Create a field for this Item
    if(bProcess){//if item with occs has subitems then only the subitems shld go into
        //group wrt discussion Engine cant handle both in batch as well
        CDEField* pField = new CDEField(pDI->GetName(), sDictName) ;
        pField->SetItemDict(sDictName);
        pField->SetLabel(pDI->GetLabel());
        pField->SetFormNum(iFormNum);

        AddUniqueName(pField->GetName());
        pField->SetParent(pFieldOccGroup);

        pFieldOccGroup->AddItem(pField);
    }
    pFieldOccGroup->SetLabel(pDI->GetLabel());
    pFieldOccGroup->SetName(CreateUniqueName(pDI->GetName()));

    return pFieldOccGroup;
}

#ifdef USE_BINARY
#else
/////////////////////////////////////////////////////////////////////////////////
//
//      void CDEFormFile::CreateOrderFile (CDataDict& dictionary, bool bBuildRecords/*=false*/)
//
/////////////////////////////////////////////////////////////////////////////////
void CDEFormFile::CreateOrderFile(CDataDict& dictionary, bool bBuildRecords/* = false*/)
{
    dictionary.UpdatePointers(); // must update dict pointers before using  BMD 10 Apr 2001

    CreateOrderFile(const_cast<const CDataDict&>(dictionary), bBuildRecords);
}

void CDEFormFile::CreateOrderFile(const CDataDict& dictionary, bool bBuildRecords/* = false*/)
{
    RemoveAllForms();   // kill any existing forms
    RemoveAllLevels();  // and of course, the levels too
    RemoveAllUniqueNames();

    SetDictionaryName(dictionary.GetName());
    AddUniqueName(GetName());  // don't forget the form file's name!
    int iPgNum = 0;
    //  loop thru all the levels & each rec w/in a level--each record will be a sep form

    bool bFormAdded = false;     // for each level, tell me if a form got added!

    // cycle thru each of the dictionary's levels
    for( size_t level_number = 0; level_number < dictionary.GetNumLevels(); ++level_number ) {
        const DictLevel& dict_level = dictionary.GetLevel(level_number);
        CDELevel* pFormLevel = new CDELevel();   // new-ing a level creates it's ("hidden") placeholder CDEGroup

        AddUniqueName(dict_level.GetName());       // since we're not calling CreateUniqName, must "manually" add it
        pFormLevel->SetName(dict_level.GetName());   // use the dict name for our unique name
        pFormLevel->SetLabel(dict_level.GetLabel());
        const CDictRecord* pDictRec = dict_level.GetIdItemsRec();

        if (pDictRec->GetNumItems() > 0){
            CDEGroup* pGroup = new CDEGroup();    // name will be set below in CreateGroup
            if (GetNumLevels() > 0)     {// if this isn't 1st level, add prev level's IDs
                // CreateGroup(pGroup, pDictRec, iPgNum++, pDC, iDropSpacing, drag_options, true, GetLastLevel()->GetGroup (0));
                CreateGroupForOrder(pGroup,pDictRec,iPgNum,true,GetLastLevel()->GetGroup(0));
                iPgNum++;
            }
            else{
                //  CreateGroup(pGroup, pDictRec, iPgNum++, pDC, iDropSpacing, drag_options, true);
                CreateGroupForOrder(pGroup,pDictRec,iPgNum,true);
                iPgNum++;

            }
            pGroup->SetParent(pFormLevel->GetRoot());
            pFormLevel->AddItem(pGroup);
            CreateForm(level_number, pGroup);             // pass in the curr level
            bFormAdded = true;
        }
        if (bBuildRecords){
            // next, create a group for each record, then build the form if requested
            for (int j = 0; j < dict_level.GetNumRecords(); j++){
                CDEGroup* pGroup = new CDEGroup();
                CreateGroupForOrder(pGroup, dict_level.GetRecord(j), iPgNum, false);
                iPgNum++;
                pGroup->SetParent(pFormLevel->GetRoot());
                pFormLevel->AddItem(pGroup);
                CreateForm(level_number, pGroup);
                bFormAdded = true;
            }
        }
        if (!bFormAdded){      // if no forms got added in this level, then supply an empty one
            CDEGroup* pGroup = new CDEGroup();
            //CreateGroup(pGroup, NULL, iPgNum++, pDC, iDropSpacing, drag_options);
            CreateGroupForOrder(pGroup,NULL,iPgNum,false);
            iPgNum++;
            pGroup->SetParent(pFormLevel->GetRoot());
            pFormLevel->AddItem(pGroup);
            CreateForm(level_number);
            pGroup->SetFormName( (GetForm(pGroup->GetFormNum()))->GetName());
        }
        else {
            bFormAdded = false;         // reset for test of next level
        }
        // ok, finished bldg this level, so add it to the FRM and go do another
        int iIndex = GetNumLevels();
        AddLevel(pFormLevel);
        pFormLevel->SetHierarchy(iIndex);
    }
}
#endif // USE_BINARY


/////////////////////////////////////////////////////////////////////////////////
//
//      void CDEFormFile::UpdateFlagsNFonts()
//
/////////////////////////////////////////////////////////////////////////////////
void CDEFormFile::UpdateFlagsNFonts(UpdateFlagsNFontsAction action/* = UpdateFlagsNFontsAction::SetUseDefaultFontFlagAndFieldFont*/)
{
    if( action == UpdateFlagsNFontsAction::SetFontToDefaultTextFont )
    {
        FormFileIterator::ForeachCDEText(*this,
            [&](CDEText& text)
            {
                if( text.GetUseDefaultFont() )
                    text.SetFont(m_defaultTextFont);
            });
    }

    else
    {
        FormFileIterator::ForeachCDEText(*this,
            [&](CDEText& text)
            {
                text.SetUseDefaultFont(text.GetFont() == m_defaultTextFont);
            });

        for( int l = 0; l < GetNumLevels(); ++l )
        {
            CDELevel* pLevel = GetLevel(l);

            for( int g = 0; g < pLevel->GetNumGroups(); ++g )
            {
                CDEGroup* pGroup = pLevel->GetGroup(g);
                pGroup->UpdateFlagsNFonts(*this);
            }
        }
    }
}


bool CDEFormFile::FindField(const CString& sFldName, CDEForm** pForm, CDEItemBase** pItem)
{
    for( int f = 0; f < GetNumForms(); ++f )
    {
        *pForm = GetForm(f);

        for( int i = 0; i < (*pForm)->GetNumItems(); ++i )
        {
            *pItem = (*pForm)->GetItem(i);

            if( sFldName.CompareNoCase((*pItem)->GetName()) == 0 )
                return true;

            if( (*pItem)->GetItemType() == CDEFormBase::Roster ) {
                //look in the roster
                CDEGroup* pGroup = assert_cast<CDEGroup*>(*pItem);
                for(int iIndex = 0; iIndex < pGroup->GetNumItems(); iIndex++){
                    if(sFldName.CompareNoCase(pGroup->GetItem(iIndex)->GetName())==0){
                        *pItem = pGroup->GetItem(iIndex);
                        return true;
                    }
                }
            }
        }
    }

    *pForm = nullptr;
    *pItem = nullptr;

    return false;
}


/////////////////////////////////////////////////////////////////////////////////
//
//      bool CDEFormFile::ReconcileLevels(CString& csErr, bool bSilent, bool bAutoFix)
//
/////////////////////////////////////////////////////////////////////////////////
bool CDEFormFile::ReconcileLevels(CString& csErr, bool /*bSilent*/, bool /*bAutoFix*/)
{
    int iNumLevels = GetNumLevels();
    bool bRet = true;
    CString sMsg;

    int iDictLevels = (int)m_dictionary->GetNumLevels();

    if(iDictLevels < iNumLevels){//remove additional levels from form file
        for(int iIndex = iNumLevels-1  ;iIndex >= iDictLevels; iIndex--){
            bRet = false;
            CDELevel* pLevel = m_aLevel[iIndex];
            // sMsg += "Removing level:" + pLevel->GetName() + "\n";
            sMsg +=  pLevel->GetName() + _T(" deleted; not in dictionary.\n");
            m_aLevel.erase(m_aLevel.begin() + iIndex);
            delete pLevel;
            //Search all the forms corresponidng to this level and delete them
            for(int iForm = this->GetNumForms() -1; iForm >=0; iForm--) {
                CDEForm* pForm = this->GetForm(iForm);
                if(pForm && pForm->GetLevel() == iIndex) {
                    this->RemoveForm(iForm);
                }
            }

        }
    }
    iNumLevels = GetNumLevels();
    ASSERT(iNumLevels <= iDictLevels); //at this stage the number of levels in the form file cannot be more than
    //the number of levels in the dictionary

    //for the levels which are remaining compare the names if they do not match remove them
    //from formfile
    for(int iIndex=0; iIndex <iDictLevels; iIndex++){
        if(iIndex > GetNumLevels() -1){
            break;
        }
        if(m_dictionary->GetLevel(iIndex).GetName().CompareNoCase(GetLevel(iIndex)->GetName())==0){
            continue;
        }
        else {//Names do not match so remove the level
              //give the user a chance to select the corresponding level from the dictionary
#ifdef WIN_DESKTOP
            if( bShowFormSelDlg )
            {
                CDELevel* pLevel = GetLevel(iIndex);
                std::vector<const DictNamedBase*> dict_element_candidates = { &m_dictionary->GetLevel(iIndex) }; //for now only one

                CRenameDlg rename_dlg(*pLevel, dict_element_candidates);

                if( rename_dlg.DoModal() == IDOK )
                {
                    bShowFormSelDlg = !rename_dlg.DeleteAllItemsNotFound();
                    const DictNamedBase* selected_dict_candidates = rename_dlg.GetSelectedDictCandidate();

                    if( selected_dict_candidates != nullptr )
                    {
                        GetLevel(iIndex)->SetName(selected_dict_candidates->GetName());
                        bRet = false;
                        continue;
                    }
                }
            }
#endif
            bRet = false;
            CDELevel* pLevel = m_aLevel[iIndex];
            //sMsg += "Removing level:" + pLevel->GetName() + "\n";
            sMsg +=  pLevel->GetName() + _T(" deleted; not in dictionary.\n");
            m_aLevel.erase(m_aLevel.begin() + iIndex);
            delete pLevel;
            //Search all the forms corresponidng to this level and delete them
            for(int iForm = this->GetNumForms() -1; iForm >=0; iForm--) {
                CDEForm* pForm = this->GetForm(iForm);
                if(pForm && pForm->GetLevel() == iIndex) {
                    this->RemoveForm(iForm);
                }
            }
            //insert a dummy level at this position
            CDELevel* pFormLevel = new CDELevel();
            this->InsertLevelAt(pFormLevel,iIndex);

            const DictLevel& dict_level = m_dictionary->GetLevel(iIndex);
            AddUniqueName(dict_level.GetName());

            pFormLevel->SetName(dict_level.GetName());
            pFormLevel->SetLabel(dict_level.GetLabel());

            CDEGroup* pGroup = new CDEGroup();  //create the root group
            pGroup->SetName(CreateUniqueName(dict_level.GetName()));
            pGroup->SetParent(pFormLevel->GetRoot());
            pFormLevel->AddItem(pGroup);
            CreateForm(iIndex, pGroup);

            iNumLevels = GetNumLevels();
        }
    }
    iNumLevels = GetNumLevels();
    //At this stage the levels are consistent with the ones in the dictionary
    //Now add levels which are not available to the formfile
    if(iDictLevels > iNumLevels){
        for(int iIndex=iNumLevels; iIndex < iDictLevels;iIndex++){
            bRet = false;
            CDELevel* pFormLevel = new CDELevel();
            this->AddLevel(pFormLevel);

            const DictLevel& dict_level = m_dictionary->GetLevel(iIndex);
            AddUniqueName(dict_level.GetName());

            pFormLevel->SetName(dict_level.GetName());
            pFormLevel->SetLabel(dict_level.GetLabel());

            CDEGroup* pGroup = new CDEGroup();  //create the root group
            pGroup->SetName(CreateUniqueName(dict_level.GetName()));

            pGroup->SetParent(pFormLevel->GetRoot());
            pFormLevel->AddItem(pGroup);
            CreateForm(iIndex, pGroup);

        }
    }
    if(!bRet){
        //csErr += "Form Levels inconsistent with the levels in dictionary \n";
        csErr += sMsg;
    }

    return bRet;
}

/////////////////////////////////////////////////////////////////////////////////
//
//      bool CDEFormFile::ReconcileName(CDataDict* pDict)
//
/////////////////////////////////////////////////////////////////////////////////
bool CDEFormFile::ReconcileName(const CDataDict& dictionary)
{
    ASSERT(m_dictionary.get() == &dictionary);

    //Get the object from the pDict;
    const DictNamedBase* dict_element = dictionary.GetChangedObject();
    if( dict_element == nullptr ) {
        return false;
    }

    if(dict_element->GetElementType() == DictElementType::Level){//level name changed
        const DictLevel& dict_level = assert_cast<const DictLevel&>(*dict_element);
        int iNumLevels= GetNumLevels();
        for(int iIndex = 0 ;iIndex < iNumLevels ; iIndex ++) {
            CDELevel* pLevel = GetLevel(iIndex);
            CString sOldName = dictionary.GetOldName();
            if(pLevel->GetName().CompareNoCase(sOldName)==0 ){
                pLevel->SetName(dict_level.GetName());
                return true;
            }
        }
    }

    else if(dict_element->GetElementType() == DictElementType::Record){//record name changed
        const CDictRecord* pDictRecord = assert_cast<const CDictRecord*>(dict_element);
        //look in all groups which in all levels which have this record as the RIType
        //change the corresponding forms also if they are repeating
        CString sOldName = dictionary.GetOldName();
        ChangeGName(sOldName,pDictRecord->GetName());
        return true;
    }

    else if(dict_element->GetElementType() == DictElementType::Dictionary){//dictionary name changed
        const CDataDict* pDDict = assert_cast<const CDataDict*>(dict_element);
        ///change all the dict names for the items ;
        SetDictionaryName(pDDict->GetName());
        ChangeDName(dictionary);
        return true;
    }

    else if(dict_element->GetElementType() == DictElementType::Item){//Item name changed
        const CDictItem* pDictItem = assert_cast<const CDictItem*>(dict_element);
        //look in all groups which in all levels which have this record as the RIType
        //change the corresponding forms also if they are repeating
        CString sOldName = dictionary.GetOldName();
        CDEForm* pForm = NULL;
        CDEItemBase* pBase = NULL;
        FindField(sOldName,&pForm,&pBase);

        CDEField* pField = DYNAMIC_DOWNCAST(CDEField,pBase);
        if(pField) {
            pField->SetItemName(pDictItem->GetName());
            pField->SetName(pDictItem->GetName());

            //Do all the mirror fields
            for (int iIndex = 0; iIndex < GetNumForms(); iIndex++){
                pForm = GetForm(iIndex);
                for (int iItem=0; iItem < pForm->GetNumItems();iItem++ ){
                    CDEItemBase* pItem = pForm->GetItem(iItem);

                    pField = DYNAMIC_DOWNCAST(CDEField,pItem);
                    if(pField && pField->GetPlusTarget().CompareNoCase(sOldName) ==0 ) {
                        pField->SetPlusTarget(pDictItem->GetName());
                    }

                    if (pField && pField->IsMirror()){
                        if(pField->GetItemName().CompareNoCase(sOldName) == 0 ) {
                            pField->SetItemName(pDictItem->GetName());
                        }
                    }
                }
            }
            return true;
        }
    }

    return false;
}
/////////////////////////////////////////////////////////////////////////////////
//
//      void CDEFormFile::ChangeDName(const CDataDict& dictionary)
//
/////////////////////////////////////////////////////////////////////////////////
void CDEFormFile::ChangeDName(const CDataDict& dictionary)
{
    for (int l = 0; l < GetNumLevels(); l++)
    {
        CDELevel* pLevel = GetLevel(l);

        int gMax = pLevel->GetNumGroups();

        for (int g = 0; g < gMax; g++)
        {
            CDEGroup* pGroup = pLevel->GetGroup(g);
            pGroup->ChangeDName(dictionary);
        }
    }
}


/////////////////////////////////////////////////////////////////////////////////
//
//      void CDEFormFile::ChangeDName(CDataDict* pDict)
//
/////////////////////////////////////////////////////////////////////////////////
void CDEFormFile::ChangeGName(const CString& sOldName, const CString& sNewName)
{
    CDELevel* pLevel;
    CDEGroup* pGroup;
    CDEGroup* pRoot;

    int g, gMax;

    for (int l = 0; l < GetNumLevels(); l++)
    {
        pLevel = GetLevel(l);

        pRoot = pLevel->GetRoot();

        gMax = pLevel->GetNumGroups();

        for (g=0; g < gMax; g++)
        {
            pGroup = pLevel->GetGroup (g);
            pGroup->ChangeGName(sOldName,sNewName);
        }
    }
    int iNumForms = GetNumForms();
    for(int iIndex =0 ;iIndex < iNumForms ; iIndex++){
        CDEForm* pForm = GetForm(iIndex);
        CString sRepeat = pForm->GetRecordRepeatName();
        if(sRepeat.CompareNoCase(sOldName) ==0 ){
            pForm->SetRecordRepeatName(sNewName);
        }

    }
}


/////////////////////////////////////////////////////////////////////////////////
//
//      void CDEFormFile::ClearUsed()
//
/////////////////////////////////////////////////////////////////////////////////
void CDEFormFile::ClearUsed()
{
    m_usedDictItems.clear();

    for (int l = 0; l < GetNumLevels(); l++){
        CDELevel* pLevel = GetLevel(l);
        pLevel->SetUsed(false);

        int gMax = pLevel->GetNumGroups();
        for (int g = 0; g < gMax; g++)
        {
            CDEGroup* pGroup = pLevel->GetGroup (g);
            pGroup->ClearUsed();
        }
    }
}

void CDEFormFile::SetUsed(const CDictItem& dict_item, bool used/* = true*/)
{
    if( used )
        m_usedDictItems.insert(&dict_item);

    else
        m_usedDictItems.erase(&dict_item);
}

bool CDEFormFile::IsUsed(const CDictItem& dict_item) const
{
    return ( m_usedDictItems.find(&dict_item) != m_usedDictItems.cend() );
}


/////////////////////////////////////////////////////////////////////////////////
//
//      void CDEFormFile::CheckLevels()
//
/////////////////////////////////////////////////////////////////////////////////
void CDEFormFile::CheckLevels()
{
    int iNumLevels = GetNumLevels();

    for(int iIndex=0; iIndex <iNumLevels; iIndex++){
        if(iIndex >= (int)m_dictionary->GetNumLevels()){//if we cant find the levels
            break;
        }
        if(m_dictionary->GetLevel(iIndex).GetName().CompareNoCase(GetLevel(iIndex)->GetName())==0){
            GetLevel(iIndex)->SetUsed(true);
        }
        else {
            continue;
        }
    }
}


/////////////////////////////////////////////////////////////////////////////////
//
//      void CDEFormFile::CheckFormFile()
//
/////////////////////////////////////////////////////////////////////////////////
void CDEFormFile::CheckFormFile()
{
    CheckLevels();

    int iLevels = GetNumLevels();
    for (int i = 0; i<iLevels; i++){
        CDELevel* pLevel = GetLevel(i);
        CDEGroup* pGroup = pLevel->GetRoot();
        pGroup->CheckGroups(this);
    }
}


bool CDEFormFile::ReconcileSkipTo(CString& sMsg)
{
    //For each form
    bool bRet = false;
    int iNumForms = this->GetNumForms();
    for(int iForm = 0; iForm < iNumForms ;  iForm++){
        CDEForm* pForm = GetForm(iForm);
        CDEGroup* pGroup = pForm->GetGroup();
        int iNumItems = pGroup->GetNumItems();
        for(int iIndex = 0; iIndex < iNumItems ; iIndex ++) {
            CDEItemBase* pBase = pGroup->GetItem(iIndex);
            if(pBase->GetItemType() == CDEItemBase::eItemType::Field){
                CDEField* pField = assert_cast<CDEField*>(pBase);
                if(!CheckValidSkip(pGroup,pField,sMsg)){
                    bRet = true;
                }
            }
            else if(pBase->GetItemType() == CDEItemBase::eItemType::Roster){
                CDEGroup* pThisGroup = assert_cast<CDEGroup*>(pBase);
                int iNumRosItems = pThisGroup->GetNumItems();
                for(int iRosItems = 0; iRosItems < iNumRosItems ; iRosItems++) {
                    CDEItemBase* pThisBase = pThisGroup->GetItem(iRosItems);
                    if(pThisBase->GetItemType() == CDEItemBase::eItemType::Field){
                        CDEField* pField = assert_cast<CDEField*>(pThisBase);
                        if(!CheckValidSkip(pThisGroup,pField,sMsg)){
                            bRet = true;
                        }
                    }
                }
            }
            else {
                continue;
            }
        }
    }
    return bRet;
}

bool CDEFormFile::CheckValidSkip(CDEGroup* pGroup, CDEField* pSource, CString& sMsg)
{
    //Go through all the fields of the group
    int iNumItems = pGroup->GetNumItems();
    CString sPlusTarget = pSource->GetPlusTarget();
    sPlusTarget.Trim();
    if(sPlusTarget.IsEmpty() || sPlusTarget.CompareNoCase(_T("<END>")) ==0){
        return true;
    }
    bool bFoundSource = false ;
    bool bFoundTarget = false;
    for(int iIndex = 0; iIndex < iNumItems ; iIndex ++) {
        //Go through only siblings of the group items  .No need of recursion by design
        CDEItemBase* pBase = pGroup->GetItem(iIndex);
        if(pSource ==  pBase) {
            bFoundSource = true;
            continue;
        }
        if(pBase->GetName().CompareNoCase(sPlusTarget)== 0 ) {
            bFoundTarget = true;
        }
        if(bFoundTarget){
            if(!bFoundSource) {
                //Target found before source
                pSource->SetPlusTarget(_T(""));
                sMsg += pSource->GetName() + _T(" 'Skip to' field reset");
                sMsg += _T("; Cannot Skip to") + sPlusTarget + _T(".\n");
                return false;
            }
            else {
                return true;
            }
        }

    }
    pSource->SetPlusTarget(_T(""));
    if(bFoundSource && !bFoundTarget) {
        sMsg += pSource->GetName() + _T(" 'Skip to' field reset");
        CString sForm = _T("form");
        if(pSource->GetParent()->GetItemType() == CDEItemBase::eItemType::Roster){
            sForm = _T("roster");
        }
        sMsg += _T("; ")+ sPlusTarget +_T(" not on ") + sForm + _T(".\n");
    }
    return false;
}

/////////////////////////////////////////////////////////////////////////////////
//
//      bool CDEFormFile::Compare(CDEFormFile* pFormFile)
//  false if different form files . true if same . Do not use these functions
//  for anything else .They are meant only for reconcile stuff.
/////////////////////////////////////////////////////////////////////////////////
bool CDEFormFile::Compare(CDEFormFile* pFormFile)
{
    bool bRet = false;
    int iNumLevels0 = GetNumLevels();
    int iNumLevels1 = pFormFile->GetNumLevels();
    if(iNumLevels0 != iNumLevels1) {
        return bRet;
    }

    CDELevel*               pLevel0;
    CDELevel*       pLevel1;
    for (int i = 0; i < iNumLevels0; i++){        // cycle thru each of the levels
        pLevel0 = GetLevel(i);
        pLevel1 = pFormFile->GetLevel(i);
        if(!pLevel0->Compare(pLevel1))
            return bRet;
    }
    return true; //They are same so return true
}

void CDEFormFile::RenumberAllForms()
{
    bool flag = false;
    int i = 0;
    for (i = 0; i < GetNumForms();i++)
    {
        CDEForm* pForm = GetForm(i);
        CDEGroup* pGroup = pForm->GetGroup();
        if (pGroup!=NULL)
        {
            pGroup->SetFormName(pForm->GetName());
            pGroup->SetFormNum(i);
        }
        else
        {
            m_aForm.erase(m_aForm.begin() + i);// pForm
            flag = true;
            continue;
        }
        pForm->RenumberItems(i);
    }
    if (!flag) return;
    for ( i = 0; i < GetNumLevels();i++)
    {
        CDEGroup* pGroup = m_aLevel[i]->GetRoot();
        for (int j = 0; j < pGroup->GetNumItems(); j++)
        {
            if (pGroup->GetItem(j)->GetItemType() == CDEFormBase::Group)
            {
                CDEGroup* pGrouptemp = assert_cast<CDEGroup*>(pGroup->GetItem(j));
                if (pGrouptemp->GetFormNum() >= GetNumForms())
                {
                    pGroup->RemoveItemAt(j);
                }
            }
        }
    }
}


/////////////////////////////////////////////////////////////////////////////////
//
// CDEFormFile::RenumberFormsNItems4BCH()
//
/////////////////////////////////////////////////////////////////////////////////
void CDEFormFile::RenumberFormsNItems4BCH()
{
    for (int iIndex = (int)m_aForm.size()-1; iIndex >=0 ; iIndex--){
        CDEForm* pForm = m_aForm[iIndex];
        if( pForm == NULL){
            m_aForm.erase(m_aForm.begin() + iIndex);
        }
    }
    //For each form in the array
    int iNumForms = this->GetNumForms();
    for (int iForm = 0; iForm < iNumForms ; iForm++) {
        CDEForm* pForm = this->GetForm(iForm);
        int iNumItems = pForm->GetNumItems();
        for (int iItem = 0 ;iItem < iNumItems; iItem++) {
            CDEItemBase* pBase = pForm->GetItem(iItem);
            pBase->SetFormNum(iForm);
            CDEGroup* pGroup = assert_cast<CDEGroup*>(pBase->GetParent());
            if(pGroup) {
                pGroup->SetFormName(pForm->GetName());
                pGroup->SetFormNum(iForm);
            }
        }
    }

}

/////////////////////////////////////////////////////////////////////////////////
//
//      void CDEFormFile::SetMaxFieldPointer()
//
/////////////////////////////////////////////////////////////////////////////////
void CDEFormFile::SetMaxFieldPointer()
{
    CDELevel* pLevel;
    CDEGroup* pGroup;
    CDEGroup* pRoot;

    int g, gMax;

    for (int l = 0; l < GetNumLevels(); l++)
    {
        pLevel = GetLevel(l);

        pRoot = pLevel->GetRoot();

        gMax = pLevel->GetNumGroups();

        for (g=0; g < gMax; g++)
        {
            pGroup = pLevel->GetGroup (g);
            pGroup->SetMaxFieldPointer (this);
        }
    }
}


bool CDEFormFile::ReconcileFieldAttributes(CString& sMsg)
{
    //For each form
    bool bRet = false;
    int iNumForms = this->GetNumForms();
    for(int iForm = 0; iForm < iNumForms ;  iForm++){
        CDEForm* pForm = GetForm(iForm);
        CDEGroup* pGroup = pForm->GetGroup();
        int iNumItems = pGroup->GetNumItems();
        for(int iIndex = 0; iIndex < iNumItems ; iIndex ++) {
            CDEItemBase* pBase1 = pGroup->GetItem(iIndex);
            if(pBase1->GetItemType() == CDEItemBase::eItemType::Field){
                CDEField* pField = assert_cast<CDEField*>(pBase1);
                if(!CheckFieldAttributes(pField,sMsg)){
                    bRet = true;
                }
            }
            else if(pBase1->GetItemType() == CDEItemBase::eItemType::Roster){
                CDEGroup* pThisGroup = assert_cast<CDEGroup*>(pBase1);
                int iNumRosItems = pThisGroup->GetNumItems();
                for(int iRosItems = 0; iRosItems < iNumRosItems ; iRosItems++) {
                    CDEItemBase* pBase2 = pThisGroup->GetItem(iRosItems);
                    if(pBase2->GetItemType() == CDEItemBase::eItemType::Field){
                        CDEField* pField = assert_cast<CDEField*>(pBase2);
                        if(!CheckFieldAttributes(pField,sMsg)){
                            bRet = true;
                        }
                    }
                }
            }
            else {
                continue;
            }
        }
    }
    return bRet;
}


bool CDEFormFile::CheckFieldAttributes(CDEField* pSource, CString& sMsg)
{
    int initial_message_length = sMsg.GetLength();

    const CDictItem* pDictItem = pSource->GetDictItem();
    bool bNumeric = ( pDictItem->GetContentType() == ContentType::Numeric );
    bool bAlpha = ( pDictItem->GetContentType() == ContentType::Alpha );

    try
    {
        pSource->GetCaptureInfo().Validate(*pDictItem);
    }

    catch( const CaptureInfo::ValidationException& exception )
    {
        CaptureInfo new_capture_info = pSource->GetCaptureInfo().MakeValid(*pDictItem,
            pDictItem->GetFirstValueSetOrNull(), false);

        sMsg.AppendFormat(_T("%s: %s\nThe capture type has been reset to: %s.\n"), pSource->GetName().GetString(),
                          exception.GetErrorMessage().c_str(), new_capture_info.GetDescription().GetString());

        pSource->SetCaptureInfo(new_capture_info);
    }

    if( pSource->IsMirror() && pSource->GetCaptureInfo() != CaptureInfo::GetBaseCaptureType(*pDictItem) )
    {
        // do this silently because it really only affects the coloring of the field in the designer
        pSource->SetCaptureInfo(CaptureInfo::GetBaseCaptureType(*pDictItem));
    }


    // to check when the field data type changes to Numeric and the field uses the multiline text box.
    if( !bAlpha && pSource->AllowMultiLine() )
    {
        pSource->SetMultiLineOption(false);
        pSource->SetUseUnicodeTextBox(false); // 20120612 added this and made the next if and else if so that only one message would be given

        sMsg.AppendFormat(_T("Non-alpha field %s cannot have the multiline entry option. ")
                          _T("The field text box has been changed to use the default style.\n"), pSource->GetName().GetString());
    }

    // to check when the field data type changes to Numeric and the field uses the unicode text box.
    else if( !bAlpha && pSource->UseUnicodeTextBox() )
    {
        pSource->SetUseUnicodeTextBox(false);

        sMsg.AppendFormat(_T("Non-alpha field %s cannot have the remove tickmarks entry option. ")
                          _T("The field text box has been changed to use the default style.\n"), pSource->GetName().GetString());
    }

    if( pSource->IsSequential() ) // 20120504
    {
        if( !bNumeric || ( pDictItem->GetOccurs() == 1 && pDictItem->GetRecord()->GetMaxRecs() == 1 ) )
            pSource->IsSequential(false);
    }

    if( pSource->IsPersistent() || pSource->IsAutoIncrement() )
    {
        if( pDictItem->GetRecord()->GetSonNumber() != COMMON ) // not an ID item
        {
            sMsg.AppendFormat(_T("Non-ID field %s cannot be %s.\n"), pSource->GetName().GetString(),
                              pSource->IsPersistent() ? _T("persistent") : _T("auto increment"));

            pSource->IsPersistent(false);
            pSource->IsAutoIncrement(false);
        }

        else if( pSource->IsPersistent() && pSource->IsAutoIncrement() )
        {
            sMsg.AppendFormat(_T("ID field %s cannot be both persistent and auto increment. ")
                              _T("The field is now only persistent.\n"), pSource->GetName().GetString());

            pSource->IsAutoIncrement(false);
        }

        else if( pSource->IsAutoIncrement() )
        {
            if( !bNumeric )
            {
                sMsg.AppendFormat(_T("ID field %s cannot be auto increment because it is not numeric.\n"), pSource->GetName().GetString());

                pSource->IsAutoIncrement(false);
            }

            else if( pDictItem->GetLevel() != &m_dictionary->GetLevel(0) )
            {
                sMsg.AppendFormat(_T("ID field %s cannot be auto increment because it is not on the first level.\n"), pSource->GetName().GetString());

                pSource->IsAutoIncrement(false);
            }
        }
    }

    // success means that no messages have been added
    return ( initial_message_length == sMsg.GetLength() );
}


bool CDEFormFile::Open(const CString& csFileName, bool bSilent /* =false */ )
{
    if( !PortableFunctions::FileIsRegular(csFileName) )
    {
        if( !bSilent )
            ErrorMessage::Display(_T("FRMCLASS.CPP, CDEForm::Open() - Internal error: Form File does not exist!"));

        return false;
    }

    CSpecFile frmFile;
    bool bRetVal = true;        // default to true, load went ok

    CString sMsg = _T("Failed to load .fmf file -- fatal error(s) found.");

    if (frmFile.Open(csFileName, CFile::modeRead))      {
        CString csCmd, csArg;
        std::shared_ptr<ProgressDlg> dlgProgress;
        int len = (int)( frmFile.GetLength() / 100 );
        if (len == 0 ) {
            len = 1;
        }

        if (!bSilent) {
            dlgProgress = ProgressDlgFactory::Instance().Create();
            dlgProgress->SetStatus(_T("Checking form file ... please wait"));
            dlgProgress->SetStep(1);
            dlgProgress->SetRange(0, len);
        }

        while (frmFile.GetLine(csCmd, csArg) == SF_OK && bRetVal) {

            if( !csCmd.CompareNoCase(_T("[NoEdit]")) ) // 20110824
            {
#ifdef WIN_DESKTOP
                CString appName;

                GetModuleFileName(AfxGetApp()->m_hInstance,appName.GetBuffer(_MAX_PATH + 1),_MAX_PATH);
                appName.ReleaseBuffer();

                if( !appName.Right(9).CompareNoCase(_T("cspro.exe")) )
                    return FALSE;
#endif
            }

            if (csCmd.CompareNoCase(HEAD_FORM_FILE) == 0 )  {
                CString sVersion(CSPRO_VERSION);
                if ( !frmFile.IsVersionOK (sVersion))   {//first get the version
                    //                                      sMsg.FormatMessage (_T("Form File is not Version 2.2; load anyway?"));
                    //                    if (AfxMessageBox(sMsg, MB_YESNO | MB_ICONQUESTION) != IDYES)  {
                    //                        return false;
                    //                    }
                    //                    else {//set the version to the correct one to set the default font stuff
                    SetVersion(sVersion);
                    //                    }
                }

                bRetVal = Build (frmFile, (bSilent ? NULL : dlgProgress));
                if (bRetVal) {  // so far so good
                    bRetVal = BuildWrapUp();
                }
                SetVersion (CSPRO_VERSION);    // store the version info //now set it to the latest version stuff as usual
            }
            else {
                sMsg = _T("[FormFile] header block missing or corrupted");
                bRetVal = false;
            }
        }
        if (!bSilent || !bRetVal) {
            if (dlgProgress->CheckCancelButton() || !bRetVal)  {
                ErrorMessage::Display(sMsg);
                bRetVal = false;
            }
            else  {
                dlgProgress->SetPos((int)( frmFile.GetLength() / 100 ));
            }
        }
        frmFile.Close();
    }

    if(bRetVal){
        UpdateFlagsNFonts(UpdateFlagsNFontsAction::SetFontToDefaultTextFont);
        UpdateFlagsNFonts(UpdateFlagsNFontsAction::SetUseDefaultFontFlagAndFieldFont);
    }

    RenumberAllForms();

    return bRetVal;
}


bool CDEFormFile::Build(CSpecFile& frmFile, std::shared_ptr<ProgressDlg> pDlgProgress/* = nullptr*/)
{
    CString csCmd;    // the string command  (left side of =)
    CString csArg;    // the string argument (right side of =)

    TCHAR* pszArgs;
    TCHAR* pszArg;

    while (frmFile.GetLine(csCmd, csArg) == SF_OK)  {      // csc 5/5/97

        ASSERT (!csCmd.IsEmpty());

        if (csCmd.CompareNoCase (FRM_CMD_NAME) == 0 ) {
            SetName(csArg);
            if (pDlgProgress != NULL) {
                pDlgProgress->SetStatus(FormatText(_T("Checking form file %s ... please wait"), GetName().GetString()));
            }
        }
        else if( csCmd.CompareNoCase (FRM_CMD_LABEL) == 0 ) {
            SetLabel(csArg);
        }
        else if( csCmd.CompareNoCase (FRM_DEF_TXTFONT) == 0 ) {
            if(!csArg.IsEmpty()){
                m_defaultTextFont.BuildFromPre80String(csArg);
            }
        }
        else if( csCmd.CompareNoCase (FRM_CMD_DICTORDER) == 0 ) {
            SetDictOrder(TEXT_TO_BOOL(csArg));
        }
        else if( csCmd.CompareNoCase (FRM_FLD_FONT) == 0 ){
            if(!csArg.IsEmpty()){
                m_fieldFont.BuildFromPre80String(csArg);
            }
        }
        else if( csCmd.CompareNoCase(FRM_FIELDCOLOR_BASE) == 0 ) {
            m_fieldColors.BuildFromArgument(csArg);
        }

        else if( csCmd.CompareNoCase (FRM_CMD_PATH) == 0 )
            IsPathOn(csArg);

        else if( csCmd.CompareNoCase(CSPRO_CMD_RTLROSTERS) == 0 )
            SetRTLRostersFlag(TEXT_TO_BOOL(csArg));

        else if( csCmd.CompareNoCase (FRM_CMD_FORANGE) == 0 ) {
            // removed this (unused) attribute on 20150114
        }

        else if( csCmd[0] == '[')       // we are starting a new section ...
        {
            if (pDlgProgress)
            {
                pDlgProgress->SetPos((int)( frmFile.GetLength() / 100 ));

                if (pDlgProgress->CheckCancelButton())
                {
                    return false;
                }
            }
            if (csCmd.CompareNoCase(CSPRO_DICTS) == 0 )
            {
                int DictCnt=0;  // count of the # of [Dictionaries] sections; only 1 blk allowed per formfile

                while (frmFile.GetLine(csCmd, csArg) == SF_OK && csCmd [0] != '[')
                {
                    if (csCmd.CompareNoCase(FRM_CMD_FILE) == 0 )     // i.e., "File="
                    {
                        pszArgs = csArg.GetBuffer(csArg.GetLength());
                        if(csArg.Find(_T(","))>=0){
                            pszArg = strtoken(pszArgs, _T(","), NULL);  // this yields the time/date stuff
                            pszArg = strtoken(NULL, _T(","), NULL);     // now we've got the file name
                        }
                        else {
                            pszArg = pszArgs;
                        }

                        CString sPath;
                        sPath = frmFile.GetFilePath();
                        PathRemoveFileSpec(sPath.GetBuffer(MAX_PATH));
                        sPath.ReleaseBuffer();

                        SetDictionaryFilename(WS2CS(MakeFullPath(sPath, pszArg)));
                        DictCnt++;
                    }
                }

                if (DictCnt == 0 )
                {
                    ErrorMessage::Display(_T(".frm file has no dictionaries in the [Dictionaries] block"));
                    return false;
                }

                if (DictCnt > 1)
                {
                    ErrorMessage::Display(_T(".frm file has more than one [Dictionaries] block"));
                    return false;
                }

                frmFile.UngetLine();    // unget that new section hdg...
            }

            else if( csCmd.CompareNoCase (HEAD_FORM) == 0 )
            {
                CDEForm* pForm = new CDEForm();

                if (!pForm->Build(frmFile, GetDictionaryFilename()))
                {
                    return false;
                }
                m_aForm.emplace_back(pForm);
            }

            else if( csCmd.CompareNoCase(HEAD_LEVEL) == 0 )
            {
                CDELevel* pLevel = new CDELevel();

                if (!pLevel->Build(frmFile, this))
                {
                    return false;
                }
                if (pLevel->GetName() == sEmptyName)
                {
                    pLevel->SetName(CreateUniqueName(_T("LEVEL"), false));
                }

                int iIndex = GetNumLevels();
                AddLevel(pLevel);
                pLevel->SetHierarchy(iIndex);
            }
            else   // signal invalid section heading
            {
                csArg.Format(_T("Invalid section heading at line %d:"), frmFile.GetLineNumber());
                csArg += _T("\n") + csCmd;
                ErrorMessage::Display(csArg);
                frmFile.SkipSection();
            }
        }
    }

    BuildUniqueNL();

    // in zFormF where i load the formfile, *there* determine the record by which
    // a form repeats; nd to do there, as i need a ptr to the data dict!

    return true;               // assume for now all went ok
}


/////////////////////////////////////////////////////////////////////////////
// do a little more error checking before saying the FormFile is ok

bool CDEFormFile::BuildWrapUp()
{
    bool bWentOK = true;

    if (GetNumLevels() == 0 )
    {
        ErrorMessage::Display(_T("No [Level] blocks were found in the .frm file"));
        bWentOK = false;
    }

    int i, num = GetNumForms();

    // i believe this next err will be caught during other parts of the Build,
    // specifically, during the [Group] blk's build (unless of course they're
    // missing, but i would've complained about this by now and bailed

    if (num == 0 )
    {
        ErrorMessage::Display(_T("No [Form] blocks were found in the .frm file"));
        bWentOK = false;
    }
    else    // check that each field on each [form] has been initialized; if not, drop
    {
        CDEItemBase* pItem;
        CDEForm* pForm;
        int j;

        for (i = 0; i < num; i++)
        {
            pForm = GetForm(i);

            // the calc for # of items must be done each time, for if an item is
            // deleted, the max value will decrement!

            for (j = 0; j < pForm->GetNumItems(); )
            {
                pItem = pForm->GetItem(j);

                if (pItem->GetItemType() == CDEFormBase::Text)       // txt items won't be in a group!
                {
                    j++;
                    continue;
                }

                // the following is a good indicator that the item never got
                // assoc to an item in the [Group] blks

                if (pItem->GetParent() == NULL)
                {
                    // delete it and continue, don't make it a fatal error
                    ErrorMessage::Display(FormatText(_T("Item \"%s\" on Form #%d was not found in any [Group] block.\n")
                                                     _T("The item will be removed from the form"), pItem->GetName().GetString(), i + 1));
                    delete pItem;   // removing the item from the form doesn't delete it's memory

                    pForm->RemoveItem (j);
                }
                else    // increment j if the item wasn't deleted
                    j++;
            }
        }
    }
    return bWentOK;
}


bool CDEFormFile::Save(const CString& csFileName) const
{
    CSpecFile frmFile;

    if (!frmFile.Open(csFileName, CFile::modeWrite))
    {
        ErrorMessage::Display(_T("Error: Could not open file ") + csFileName);
        return false;
    }

    Save(frmFile);
    frmFile.Close();

    if (!PortableFunctions::FileIsRegular(csFileName)) {
        ErrorMessage::Display(_T("FRMCLASS.CPP, CDEFormFile::Save() - Internal error: FRM file does not exist!"));
    }

    return true;
}

/////////////////////////////////////////////////////////////////////////////

void CDEFormFile::Save(CSpecFile& frmFile) const
{
    const_cast<CDEFormFile*>(this)->RenumberAllForms();

    const_cast<CDEFormFile*>(this)->UpdateFlagsNFonts();//Update the font stuff

    frmFile.PutLine(HEAD_FORM_FILE);
    frmFile.PutLine(CMD_VERSION, GetVersion());        // want to check against CSPRO_VERSION
    frmFile.PutLine(FRM_CMD_NAME, GetName());
    frmFile.PutLine(FRM_CMD_LABEL, GetLabel());
    frmFile.PutLine(FRM_DEF_TXTFONT, m_defaultTextFont.GetPre80String());
    frmFile.PutLine(FRM_FLD_FONT, m_fieldFont.GetPre80String());
    m_fieldColors.Save(frmFile);
    frmFile.PutLine(FRM_CMD_PATH, IsPathOn() ? _T("SystemControlled") : _T("OperatorControlled")); // originally "Survey" and "Census"

    if(this->m_bRTLRosters)
        frmFile.PutLine(CSPRO_CMD_RTLROSTERS, CSPRO_ARG_YES);

    if(!IsDictOrder())
        frmFile.PutLine(FRM_CMD_DICTORDER, CSPRO_ARG_NO);

    frmFile.PutLine(_T("  "));                  // toss out a blank line to sep the options

    frmFile.PutLine(CSPRO_DICTS);
    frmFile.PutLine(FRM_CMD_FILE, GetRelativeFName(frmFile.GetFilePath(), GetDictionaryFilename()));
    frmFile.PutLine(_T("  "));

    //      read in the [form] blk; it will contain text, box, and header info but only names of the items
    //      that fall on the form; those details will be obtained when building the group info and the
    //      initialization of those items will be finished up


    for (int i = 0; i < GetNumForms(); i++) {
        CDEForm* pForm = GetForm(i);
        pForm->Save(frmFile);
    }

    for (int i = 0; i < GetNumLevels(); i++) {
        CDELevel* pLevel = GetLevel(i);
        pLevel->Save(frmFile);
    }

}


void CDEFormFile::serialize(Serializer& ar)
{
    CDEFormBase::serialize(ar);

    ar & m_defaultTextFont
       & m_bDictOrder
       & m_fieldFont
       & m_fieldColors
       & m_bPathOn;

    ar.IgnoreUnusedVariable<bool>(Serializer::Iteration_8_0_000_1); // m_bFixedFont

    ar.SerializeFilename(m_dictionaryFilename);

    ar & m_aLevel;

    ar & m_aForm;

    ar & m_dictionaryName;

    if( ar.PredatesVersionIteration(Serializer::Iteration_8_0_000_1) ) // m_aUniqNames
    {
        // CDEUniqueName
        for( int i = ar.Read<int>(); i >= 1; --i )
        {
            ar.Read<CString>(); // m_sName;
            ar.Read<int>();     // m_iOcc
        }
    }

    ar.SerializeFilename(m_csFormPathName);

    if( ar.IsLoading() )
        UpdateFlagsNFonts();

    FormSerialization::reset();

#if defined(_DEBUG) && defined(WIN_DESKTOP)
    // allow a way for developers to recover people's form files from .pen files
    if( CString(GetCommandLine()).Find(_T("/extract")) >= 0 )
    {
        const std::wstring filename = PortableFunctions::PathAppendToPath(GetWindowsSpecialFolder(WindowsSpecialFolder::Desktop), FormatText(_T("%s.fmf"), GetName().GetString()));
        Save(WS2CS(filename));
    }
#endif
}
