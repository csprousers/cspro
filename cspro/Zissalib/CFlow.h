#pragma once

//---------------------------------------------------------------------------
//  File name: CFlow.h
//
//  Description:
//          Implementation for flow descriptors
//
//  History:    Date       Author   Comment
//              ---------------------------
//              16 Dec 99   vc      Created for IMSA' 2nd release of Mar/00
//              30 Jan 00   vc      Expansion to best manage Enter'ed flows
//                                  (xxxJustReturned data/methods were added)
//              21 Feb 00   vc      Adding checkpoint for id-fields
//              20 Jul 00   vc      Tailoring for recent methods added outside this class
//              08 Jan 01   vc      Adding link to new FlowCore
//              25 Jul 01   vc      Adding IsPrimary/IsSecondary/IsExternal
//
//
//---------------------------------------------------------------------------

#include <zLogicO/Symbol.h>
#include <zEngineO/AllSymbolDeclarations.h>

class CDEFormFile;
class CEngineArea;
class CFlowCore;                                        // victor Jan 08, 01
struct EngineData;

//---------------------------------------------------------------------------
//
//  class CSymbolFlow : public Symbol
//
//
//  Description:
//      Supporting flow management in both entry and batch runs
//
//  * Basic approach:
//
//      A CsPro project (APL file) contains one or more FRM files.  The first
//      FRM is assumed to be the input (or main) flow - and contains the only
//      input dictionary - and it "drives" the global flow, i.e. the execution
//      always start with this flow.  Remaining FRM files are called secondary
//      flows - and refer to external dictionaries - and are intended to
//      describe the order of execution of the procedures in entry runs only,
//      depending on whether they are "entered" from another flow.  It is
//      expected that a secondary flow can be "entered" from either the main
//      flow or any other secondary flow, with the only restriction that a
//      same flow cannot be entered twice in the chain-of-flows.
//
//      All procedures, both for input and secondary flows, reside in the
//      unique application (APP) file defined for the CsPro project.  This
//      implies that the APP can have procedures for every element of each
//      dictionary included in aflow of the project.  This also means that
//      all the dictionaries included in the set of flows of the project
//      "share" the unique APP file.
//
//      Each FRM/FormFile describes a flow, both for entry and batch processors.
//      One flow is aimed to guide the processing of one and only one dictionary
//      in data entry.  In batch runs, secondary flows are only intended to
//      provide form' descriptions.  Also in batch, the main flow can include an
//      output dictionary to be written following a ISSA-like behavior.
//
//  * Further details:
//
//      The main goal of the project is to sequentially allow an operator to
//      key-entering cases to the input dictionary in entry environments, or to
//      sequentially read and process the cases coming from the input dictionary
//      in batch runs.  Both entry and batch runs are encapsulated by Level-0
//      Pre/Post procs, and each case is enclosed among Level-1 Pre/Post procs.
//      The protocol ruling the sequence of execution is based on the flow order
//      specified in the FRM file encapsulating this dictionary.
//
//      The main flow can contain a unique input dictionary, or both an input
//      and an output dictionary.  The external flows can contain only one
//      external dictionary.  Other dictionaries can also be specified in the
//      APL and are primarily known as "independent externals" and are not
//      entitled to have any procedure attached.
//
//      Every external dictionary, either coming from secondary flows or being
//      independnt externals, can accessed data file storage from/to by means
//      procedural actions (like LoadCase, WriteCase) imbedded in the logic of
//      the application.  Each external dictionary maintains one and only one
//      case in memory, and that case can be fully or partially cleaned by
//      means of Clear commands, or replaced by another case retrieved from
//      data files using a LoadCase command.  When an Enter command is issued
//      amid the stream of procedures in entry environments, the entry driver
//      tries to reach the first field of the external, entered dictionary:
//      there are no Level-0 procs, since they refer to the session; nor Level-1
//      procs, since the data for the external case is to be created (or modified,
//      when it is already pre-loaded from the data file or partially pre-set by
//      means of procedural actions taken prior to enter the flow) and the data
//      will remain in memory and eventually be written to data files, or not written
//      at all.  The only prologue before reaching a "first" field in the
//      entered dictionary is the first Group' PreProc if any - please remark
//      that the Level-1 of the external dictionary can have a field as first
//      member.
//
//      As for the input dictionary, the natural flow of the external dictionary
//      can be modified by procedural Skips (it is almost unnecessary to mention
//      this - it's a property inherent to every data entry behavior).
//
//      Once a trip inside an entered dictionary is completed, the control
//      returns to the calling point and the next sentence, the one immediatly
//      following the Enter command, receives the execution control.  The
//      sequence of fields visited in the entered dictionary is erased from the
//      data entry path, and every subsequent backwards action will not traverse
//      those fields: the only way to go through the fields of the external
//      dictionary is to issue an Enter command again and "begin by the begin"
//      of such entered dictionary.
//
//      In summary, there can be an independent flow for each input or external
//      dictionary of the project.  External flows can also issue Enter commands
//      to launch a subordinated flow in another dictionary - the only restriction
//      is, a given dictionary cannot appear twice in the chain of Entered
//      dictionaries (and the input dictionary can never be entered, since it
//      lies at the origin of the Enter' chain).
//
//  Remarks:
//
//      1. Above conditions have been inherited from the conception of ISSA
//         and can be modified in some future.
//      2. Wether any kind of automatic access to external dictionaries is
//         introduced in the future, the foundations of this protocol should
//         be fully revised and this object should be replaced.
//
//
//  * Managing a checkpoint for id-fields
//
//      >>> this information is only relevant for primary flows >>> Feb 21, 00
//      Primary flows are attached to the input dictionary.  A basic data entry
//      need is to support the operator's activity to avoid mistakes when identifying
//      cases, both at Add and Modify stages.  The request has been primarily
//      defined as "check Id-fields [checking if the corresponding key is already
//      present on the data file] as soon as last one is entered", plus a number
//      of details on how to proceed in either Add or Modify modes.  This solution
//      deals with the FlowOrder attribute as follows:
//
//      ... each member of a Flow (Groups and Items) has a FlowOrder
//      ... the FlowOrder is loaded when building the Group-tree (refer to
//          CEngineDriver::FlowLoad) of the Flow
//      ... checking the existence of cases in a data file regards Level-1
//          ids only
//      ... a "check-point" is defined as "the FlowOrder corresponding to the
//          'higher' id-field in Level-1", where 'higher' will correspond to
//          the latest placed id-field in the natural path
//      ... a CheckPending flag is used to control the moment where checking
//          should be made.  This flag has 3 values:
//            - 0: a new case has been started, no id-field has been touched
//            - 1: an id-field has been entered (or procedurally modified)
//                 (checking should be made once the checkpoint passed)
//            - 2: checking was already made, no need to perform it again
//      ... when executing data entry tasks, EntryDriver must set CheckPending
//          flag to value 0 at the begining of a new case
//      ... when reaching any Level-1' id-field, the CheckPending flag must
//          be set to value 1, no matter its proc be executed or not
//      ... once any member of the Flow having a FlowOrder greater than the
//          check-point is reached, and if the CheckPending flag is on, the
//          data file must be searched for the primary key resulting from
//          the id-fields, no matter they have received any contents or not.
//          Once this check completed, the CheckPending flag must be set to
//          value 2 to avoid new checking attempts
//      ... the CheckPending flag must also be set to 1 by any procedural
//          change of any id-fields values
//
//
//  METHODS:
//
//      As functionality is very simple, no detailed descriptions are provided.
//  Hint: the "flow order counter" is only used when adding flow elements.
//
//  Construction/destruction/initialization
//      CSymbolFlow                 Constructor
//      ~CSymbolFlow                Destructor
//      Init                        Initialize data
//
//  Getting attributes
//      IsPrimary                   True if the flow is the Primary one
//      IsSecondary                 True if the flow is a Secondary flow
//      IsExternal                  True if the flow is External
//
//  Related objects
//      GetFormFile                 The FormFile object linked to this flow
//      GetNumberOfDics             The number of dictionaries in this flow
//      GetSymDicAt                 The dict' symbol (at 1 to get the output dict of main flow)
//      IsMemberDic                 If a given symbol is a dictionary of this flow
//      GetNumberOfForms            The number of forms in this flow
//      GetSymFormAt                The form' symbol
//      IsMemberForm                If a given symbol is a form of this flow
//      GetNumberOfGroups           Total groups in this flow, both visible and invisible
//      GetNumberOfVisibleGroups    The number of visible groups
//      GetNumberOfinvisibleGroups  The number of invisible groups
//      GetGroupTRoot               The root of groups of this flow in GROUPT table
//
//  Checkpoint for id-fields: SetIdCheckPoint is used during flow' construction,
//          remaining methods are used during flow' execution
//      SetIdCheckPoint             Set a FlowOrder to serve as checkpoint
//      GetIdCheckPoint             Get the checkpoint' FlowOrder
//      SetIdCheckPending           Set the flag of "checking to be made" to a value
//      GetIdCheckPending           Get the flag value of "checking to be made"
//
//  Attributes of the entered (subordinated) child flow
//      GetParentFlow               The flow where this was entered, if any (NULL if none)
//
//  Setting basic attributes
//      SetFormFile                 Attach a FormFile to this flow
//      AddDic                      Add a dictionary to this flow
//      AddForm                     Add a form to this flow
//      SetVisibleGroups            Set the number of visible groups
//      SetInvisibleGroups          Set the number of invisible groups
//      SetGroupTRoot               Set the root of groups of this flow in GROUPT table
//
//  Saving/restoring the environment to enter another flow
//      RestoreAfterEnter           Nulls the flow entered and restores previous execution environment
//
//  Operators
//      <none>
//
//---------------------------------------------------------------------------

class CSymbolFlow : public Symbol
{
// --- Data members --------------------------------------------------------
    // --- related objects
private:
    CDEFormFile*    m_pFormFile;        // FormFile descriptor if any
    CFlowCore*      m_pFlowCore;        // core flow    // victor Jan 08, 00

    // --- basic info
private:
    int             m_iNumDim;
    std::vector<int> m_aSymDic;         // participating Dictionaries
    std::vector<int> m_aSymForm;        // participating Forms
    int             m_iNumVisible;      // number of visible groups
    int             m_iNumInvisible;    // number of invisible groups
    GROUPT*         m_pGroupTRoot;      // root group in GROUPT table

    // --- checkpoint for verifying id-fields
private:
    int             m_iIdCheckFlowOrder[MaxNumberLevels + 1];
    int             m_iIdCheckPending[MaxNumberLevels + 1];

    // --- parent flow and all its execution environment (saved & restored)
private:
    CSymbolFlow*    m_pParentFlow;      // Flow where this was entered

    // --- saving/restoring environment of parent flow
private:
                    // info saved to allow later returning to this flow
    int*            m_Progbase;         // program partition starting address
    int             m_Prognext;         // current next sentence to be executed
    int             m_ProgType;         // proc being executed: pre/post
    int             m_ExLevel;          //                    : level
    int             m_ExSymbol;         //                    : isym
                    // additional info from entry driver
    int             m_Decurlevl;        // current level being processed

    // --- link to engine
    CEngineArea*    m_pEngineArea;
    EngineData*     m_engineData;


// --- Methods -------------------------------------------------------------
    // --- construction/destruction/initialization
public:
    CSymbolFlow(std::wstring name, CEngineArea* pEngineArea);
    ~CSymbolFlow();

    // --- getting attributes
public:
    bool            IsPrimary() const           { return( GetSubType() == SymbolSubType::Primary ); }
    bool            IsSecondary() const         { return( GetSubType() == SymbolSubType::Secondary ); }
    bool            IsExternal() const          { return( GetSubType() == SymbolSubType::External ); }

    // --- related objects
public:
    CDEFormFile*    GetFormFile() const         { return m_pFormFile; }
    void            SetFormFile( CDEFormFile* pFormFile ) {
                        m_pFormFile = pFormFile;
    }
    CFlowCore*      GetFlowCore( void )         { return m_pFlowCore; } // victor Jan 08, 01
    void            SetFlowCore( CFlowCore* pFlowCore ) {               // victor Jan 08, 01
                        m_pFlowCore = pFlowCore;
    }

    // --- basic info
public:
    void SetNumDim( int iNumDim ) { m_iNumDim = iNumDim; }
    int  GetNumDim() const        { return m_iNumDim; }

    // ... its own symbol
    int             GetSymbol( void ) const     { return GetSymbolIndex(); }

    // ... Flow Dicts
    void            AddDic( int iSymDic );
    int             GetNumberOfDics() const          { return (int)m_aSymDic.size(); }
    int             GetSymDicAt( int iNumDic ) const {
                        return( iNumDic < GetNumberOfDics() ? m_aSymDic[iNumDic] : 0 );
    }
    bool            IsMemberDic( int iSymDic );
    // ... Flow Forms
    void            AddForm( int iSymForm );
    int             GetNumberOfForms() const     { return (int)m_aSymForm.size(); }
    int             GetSymFormAt( int iNumForm ) {
                        return( iNumForm < GetNumberOfForms() ? m_aSymForm[iNumForm] : 0 );
    }
    bool            IsMemberForm( int iSymForm );
    // ... Flow Groups
    int             GetNumberOfGroups( void ) {
                        return( m_iNumVisible + m_iNumInvisible );
    }
    int             GetNumberOfVisibleGroups( void ) {
                        return m_iNumVisible;
    }
    void            SetVisibleGroups( int iNumVisible ) {
                        m_iNumVisible = iNumVisible;
    }
    int             GetNumberOfInvisibleGroups( void ) {
                        return m_iNumInvisible;
    }
    void            SetInvisibleGroups( int iNumInvisible ) {
                        m_iNumInvisible = iNumInvisible;
    }
    // ... Flow root
    GROUPT*         GetGroupTRoot() const           { return m_pGroupTRoot; }
    void            SetGroupTRoot(GROUPT* pGroupT)  { m_pGroupTRoot = pGroupT; }

    // --- checkpoint for verifying id-fields
public:
    void            BuildIdCheckPoints( void );
    void            ScanIdCheckInGroup( int iSymGroup );
    void            SetIdCheckPoint( int iLevel, int iFlowOrder ) {
                        m_iIdCheckFlowOrder[iLevel] = iFlowOrder;
    }
    int             GetIdCheckPoint( int iLevel ) {
                        return m_iIdCheckFlowOrder[iLevel];
    }
    void            SetIdCheckPending( int iLevel, int iStatus ) {
                        if( iStatus < 0 )
                            iStatus = 0;
                        else if( iStatus > 2 )
                            iStatus = 2;
                        m_iIdCheckPending[iLevel] = iStatus;
    }
    int             GetIdCheckPending( int iLevel ) {
                        return m_iIdCheckPending[iLevel];
    }

    // --- parent flow
public:
    CSymbolFlow*    GetParentFlow( void )       { return m_pParentFlow; }

    // --- saving/restoring environment of parent flow
public:
    void            RestoreAfterEnter( void );

private:
    const Logic::SymbolTable& GetSymbolTable() const;
};
