#include "StdAfx.h"
#include "Level.h"
#include <zUtilO/TraceMsg.h>


IMPLEMENT_DYNAMIC(CDELevel, CDEFormBase)


/////////////////////////////////////////////////////////////////////////////
//
//  CDELevel::Construction/Destruction
//
/////////////////////////////////////////////////////////////////////////////

CDELevel::CDELevel()
    :   m_iMaxLoopOccs(0),
        m_iTotalOccs(0),
        m_iCurOccurrence(0),
        m_iNesting(1),       //  bruce says base Level is 1, not 0
        m_iDataOccs(0) // RHF MOD Mar 29, 2000 Add m_iDataOccs

{
//////////////////////////////////////////////////////////////////////////
// ADDED by RCL, Dec 2004
#pragma message( __LOC__ " *** 'new' used in constructor: be careful with direct assignment producing pointer alias" )
#pragma message( __LOC__ " ***   CDELevel a, b;   a = b; // <-- 2 pointers pointing to the same place" )
#pragma message( __LOC__ " *** Hint: Define a copy constructor and make sure that alias are not produced" )
//////////////////////////////////////////////////////////////////////////

    m_pRoot = new CDEGroup();     // create our base obj (hidden from user) to be our root

    m_pRoot->SetName(_T("BaseGrp"));   // this has to be unique!! fix on exit
    m_pRoot->SetLabel(_T("Hidden from user"));

    SetName(sEmptyName);
    SetLabel(sEmptyLabel);
}


CDELevel::CDELevel(const CDELevel& oldLevel)
    :   CDEFormBase(oldLevel)
{
    SetTotalOccs    (oldLevel.GetTotalOccs());  // set CDELevel vars
    SetCurOccurrence(oldLevel.GetCurOccurrence());
    SetMaxLoopOccs  (oldLevel.GetMaxLoopOccs());
    SetHierarchy    (oldLevel.GetHierarchy());
    SetDataOccs     (oldLevel.GetDataOccs());

    m_pRoot = new CDEGroup();     // create our base obj (hidden from user) to be our root
    const CDEGroup* pRoot = oldLevel.GetRoot();

    m_pRoot->SetName  (pRoot->GetName());
    m_pRoot->SetLabel (pRoot->GetLabel());

    int iMax = oldLevel.GetNumGroups();      // now do the group array

    for (int i=0; i < iMax; i++)
    {
        CDEGroup* pOldGroup = oldLevel.GetGroup (i);
        CDEGroup* pNewGroup = new CDEGroup (*pOldGroup);
        pNewGroup->SetParent(GetRoot());
        AddGroup (pNewGroup);
    }
}


CDELevel::~CDELevel()
{
    RemoveAllItems();

    delete m_pRoot;
}


// added this func to facilitate deletion of rosters, since they entail removing
// the unique name of all fields w/in it, as well as the uniq name of the roster itself

void CDELevel::RemoveRoster(CDERoster* pRoster, CDEFormFile* pFF)
{
    CString sRstrName = pRoster->GetName();
    pFF->RemoveUniqueName(sRstrName);

    CDEField* pField = NULL;
    int i, max = pRoster->GetNumItems();

    for (i=0; i < max; i++)  {
        pField = (CDEField*) pRoster->GetItem(i);
        pFF->RemoveUniqueName(pField->GetName());
    }
    pRoster->GetParent()->RemoveItem (sRstrName);   // removes the roster entry from it's parent
}



// smg: this will need adjusting later; right now, groups can't nest w/in groups;
// i.e., the hierarchy will be levels, groups, then fields/rosters/etc; groups
// can't contain groups; when they can, i will need to fix this search to burrow
// down

CDEGroup* CDELevel::FindGroup(const CString& sGroupSearchName)
{
    CDEGroup* pGroup = NULL;
    CDEGroup* pRootGroup = GetRoot();

    if (pRootGroup->GetName() == sGroupSearchName)
        return pRootGroup;

    int i,max=pRootGroup->GetNumItems();

    for (i=0; i < max; i++)
    {
        pGroup = GetGroup (i);
        if (sGroupSearchName == pGroup->GetName())
        {
            return pGroup;
        }
    }
    return NULL;
}

/////////////////////////////////////////////////////////////////////////////////
//
//      bool CDELevel::Compare(CDELevel* pLevel)
//
/////////////////////////////////////////////////////////////////////////////////
bool CDELevel::Compare(CDELevel* pLevel)
{
    bool bRet = false;
    CDEGroup* pGroup0 = NULL;
    CDEGroup* pGroup1 = NULL;

    CDEGroup* pRootGroup0 = GetRoot();
    CDEGroup* pRootGroup1 = pLevel->GetRoot();

    if (pRootGroup0->GetName().CompareNoCase(pRootGroup1->GetName()) !=0 )
        return bRet;

    int iMax0=pRootGroup0->GetNumItems();
    int iMax1=pRootGroup1->GetNumItems();
    if(iMax0 != iMax1)
        return bRet;

    for (int i=0; i < iMax0; i++)
    {
        pGroup0 = GetGroup (i);
        pGroup1 = pLevel->GetGroup(i);
        if(!pGroup0->Compare(pGroup1)){
            return bRet;
        }

    }
    return true; // Levels are same
}


bool CDELevel::Build(CSpecFile& frmFile, CDEFormFile* pFF)
{
    CString csCmd;    // the string command  (left side of =)
    CString csArg;    // the string argument (right side of =)

    bool bKeepParsing = true,
        bRtnVal = true;

    int ln = frmFile.GetLineNumber();       // remember the starting line # of this blk

    while (bKeepParsing && frmFile.GetLine (csCmd, csArg) == SF_OK)
    {
        ASSERT (!csCmd.IsEmpty());

        if (csCmd.CompareNoCase(FRM_CMD_NAME) == 0 )
            SetName(csArg);
        else if( csCmd.CompareNoCase(FRM_CMD_LABEL) == 0 )
            SetLabel(csArg);

        else if( csCmd[0] == '[')       // we are starting a new block ...
        {
            if (csCmd.CompareNoCase(HEAD_GROUP) == 0 )  // "[Group]"
            {
                CDEGroup* pGroup = new CDEGroup();

                bKeepParsing = pGroup->Build (frmFile, pFF);

                if (bKeepParsing)
                {
                    pGroup->SetParent(GetRoot());

                    GetRoot()->AddItem(pGroup); // then add it to the list
                }
                else
                {
                    bRtnVal = false;
                }
            }
            else
            {
                frmFile.UngetLine();  // so calling func can figure out what we have

                bKeepParsing = false;
            }
        }
    }
    if (bRtnVal)    // so far so good, now make sure this level has at least one [Group] !
    {
        if (GetNumGroups() == 0 )
        {
            ErrorMessage::Display(FormatText(_T("Line #%d: Each [Level] block must have at least one associated [Group] block."), ln));
        }

        // anything else to check??  name will be done on rtn to CDEFormFile::Build()
    }
    return  bRtnVal;
}


void CDELevel::Save(CSpecFile& frmFile) const
{
    frmFile.PutLine(HEAD_LEVEL);
    frmFile.PutLine(FRM_CMD_NAME, GetName());
    frmFile.PutLine(FRM_CMD_LABEL, GetLabel());
    frmFile.PutLine(_T("  "));

    int max = GetNumItems();     // but shld in fact all be CDEGroups
    for (int i = 0 ; i < max; i++) {
        const CDEItemBase* pGroup = GetItem(i);
        pGroup->Save(frmFile);
    }
}


void CDELevel::serialize(Serializer& ar)
{
    CDEFormBase::serialize(ar);

    int iGetNumGroups = GetNumGroups();

    ar & m_iTotalOccs
       & m_iCurOccurrence
       & m_iMaxLoopOccs
       & m_iNesting
       & m_iDataOccs
       & iGetNumGroups;

    for( int i = 0; i < iGetNumGroups; i++ )
    {
        if( ar.IsSaving() )
        {
            ar & *(GetGroup(i));
        }

        else
        {
            CDEGroup* pCDEGroup = new CDEGroup;

            try
            {
                ar & *pCDEGroup;
            }
            catch( const FormSerialization::RepeatedItemException& e )
            {
                delete pCDEGroup;
                pCDEGroup = assert_cast<CDEGroup*>(FormSerialization::getItem(e.GetPosition()));
            }

            AddGroup(pCDEGroup);
        }
    }
}
