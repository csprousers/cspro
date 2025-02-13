#pragma once
//////////////////////////////////////////////////////////////////////
// RunAplE.h: interface for CRunAplEntry class.
//////////////////////////////////////////////////////////////////////

#include <Zentryo/zEntryO.h>
#include <ZBRIDGEO/runapl.h>
#include <Cexentry/Entifaz.h>
#include <engine/DEFLD.H>
#include <engine/Settings.h>
#include <zMessageO/Messages.h>
#include <zHtml/UseHtmlDialogs.h>
#include <zCaseO/CaseDefines.h>

struct AppSyncParameters;
class CCapi;
class CDEBlock;
class CDEItemBase;
class CListBox;
class CTreeCtrl;
class Userbar;

//////////////////////////////////////////////////////////////////////////////////////////////////////////////////
// CLASS NAME   : CRunApl
//
// PROTOTYPE    : class CLASS_DECL_ZBRIDGEO CRunAplEntry : public CRunApl
//
// OBJECTIVE    : Interface to Data Entry Engine
//
// REMARKS      : none
//
// A tipical use of the class in ADD mode is:
// LoadCompile()                    --> C_ExentryInit
// Start(CRUNAPL_ADD)                   --> C_ExentryStart
// NextField(FALSE)                        --> C_GotoField
// NextField(TRUE)                         --> C_GotoField
//    ...                                         ...
// NextField(TRUE)                         --> C_GotoField
// Stop();                              --> C_ExentryStop
// End()                            --> C_ExentryEnd

// A tipical use of the class in MODIFY mode is:
// LoadCompile()                    --> C_ExentryInit
// Start( CRUNAPL_MODIFIY );            --> C_ExentryStart
// ModifyStart()                            --> C_ModifyStart
// NextField(FALSE)                             --> C_GotoField
// NextField(TRUE)                              --> C_GotoField
//    ...                                              ...
// NextField(TRUE)                              --> C_GotoField
// ModifyEnd()                              --> C_ModifyStop
// Stop();                              --> C_ExentryStop
// End()                            --> C_ExentryEnd
//
// CHANGES      : 23 Jun 1999, RHF, Creation
//////////////////////////////////////////////////////////////////////////////////////////////////////////////////

//////////////////////////////////////////////////////////////////////////////////////////////////////////////////
// CLASS NAME   : CCaseInfo
//
// PROTOTYPE    : class CLASS_DECL_ZBRIDGEO CCaseInfo
//
// OBJECTIVE    : Keep a key and the data file position of a case
//
// REMARKS      : none.
//
// CHANGES      : 09 Sep 1999, RHF, Creation
//////////////////////////////////////////////////////////////////////////////////////////////////////////////////

////////////////////////////////////////////////////////////////////////////////
// FUNCTION NAME: ModifyStart
//
// PROTOTYPE    : bool ModifyStart();
//
// OBJECTIVE    : Start a modification session.
//
// PARAMETERS   : none
//
// RETURN       : TRUE if everything was OK. FALSE if not.
//
// REMARKS      :  Call Only in modification mode.
//                 Call Only if Start was succesfull.
//
// CHANGES      : 23 Jun 1999, RHF, Creation
////////////////////////////////////////////////////////////////////////////////

////////////////////////////////////////////////////////////////////////////////
// FUNCTION NAME: ModifyStop
//
// PROTOTYPE    : bool ModifyStop();
//
// OBJECTIVE    : Finish a modification session.
//
// PARAMETERS   : none
//
// RETURN       : TRUE if everything was OK. FALSE if not.
//
// REMARKS      :  Call Only in modification mode.
//                 Call Only if ModifyStart was succesfull.
//
// CHANGES      : 23 Jun 1999, RHF, Creation
////////////////////////////////////////////////////////////////////////////////

////////////////////////////////////////////////////////////////////////////////
// FUNCTION NAME: EndGroup
//
// PROTOTYPE    : CDEItemBase* EndGroup(bool bPostProc);
//
// OBJECTIVE    : Finish the data entry of the group currently having the
//                focus.  The idea is (1) receive the field as entered,
//                (2) execute its PROCTYPE_POST proc, (3) execute the PROCTYPE_POST proc
//                of this group, (4) execute the PROCTYPE_PRE proc of the first
//                field following the group or, if no such a field, write
//                the case and offer a new one to the operator.
//
//                Real actions taken are as follows:
//
//                - current field is "entered" as if the operator had
//                  manually used the Enter key. Only if bPostProc is true
//
//                - normal controls on ranges are applied and the engine
//                  can take the decission of come back to the same field,
//                  and the requested EndGroup is not completed.Only if bPostProc is true
//
//                - the PROCTYPE_POST proc of the entered field is executed and if
//                  any instruction modifying the natural flow is executed,
//                  the new target field is returned and the requested
//                  EndGroup is not completed.  The PROCTYPE_PRE proc of the new
//                  field is executed.Only if bPostProc is true
//
//                - then, the PROCTYPE_POST proc of the Group is executed.  If any
//                  procedural changes to the natural flow arises, a new
//                  target field is returned after PROCTYPE_PRE execution and the
//                  requested EndGroup is not completed
//
//                - finally, the first field after the group is selected as
//                  current field, its PROCTYPE_PRE proc is executed and that field
//                  is returned.  The EndGroup has been completed
//
//                - a variation of "completing EndGroup" happens when the
//                  group is the last group of case or node: the PROCTYPE_POST level
//                  is normally executed for this case/node, then it is
//                  written to data file, then a new case/node is initialized
//                  and its PROCTYPE_PRE proc executed, then a first field is selected
//                  and (gasp!) returned to the interfase
//
// PARAMETERS:
//                bPostProc indicating if the PostProc of the item will be executed or not.
//
// RETURN       : when the focus moves to another field following the group,
//                the engine returns its identification and the current field
//                (m_pCurField) is modified accordingly.  The engine manages
//                internally the situation of "no field following the group",
//                writting the case, preparing a new node to be entered, and
//                returning its first field set as current field.  Therefore
//                there should never be NULL returns (I hope)
//
// REMARKS      : When invoking this action, care should be taken to setup
//                a new form to screen if the new current field is located
//                in a different form.
//                See implementation of function C_EndGroup in Wexentry.cpp
//
// CHANGES      : 07 Sep 1999, VC, Creation
////////////////////////////////////////////////////////////////////////////////

////////////////////////////////////////////////////////////////////////////////
// FUNCTION NAME: MoveToField
//
// PROTOTYPE    : CDEItemBase* MoveToField( const CDEField* pEntryField, bool bPostProc=true );
//
// OBJECTIVE    : Try to reach a target field passed as parameter.  The
//                field identification is extracted form the parameter
//                and its occurrence is obtained from current occurrence
//                of its owner group.
//
//                The sense of movement (forward or backwards) must be
//                determined prior to proceed.  When both a "cannot advance"
//                and a "cannot reenter" conditions are detected, no movement
//                is performed.  If not, one of these courses of action are
//                followed:
//
//                - move backwards: the movement is served by an standard
//                    "reenter" function.
//
//                - move forward: in PathOn, it is is solved thru a standard
//                    "advance to" action; in PathOff, it's served by a
//                    "skip to" function.  Be aware of "incomplete trips"
//                    to the target, which can arise in both modes due to
//                    internal conditions of the procedures executed - the
//                    actual field reached can differ from intended target.
//
// PARAMETERS:
// pEntryField    A CDEItemBase* describing the target field.
// bPostProc       indicating if then postproc of the source field will be executed
//
// RETURN       : If a movement has been operated, the engine returns the
//                identification of the reached field; the current field
//                (m_pCurField) is modified according to the movement made.
//                Returns NULL if the target field had a "cannot" condition,
//                no movement has been performed.
//
// REMARKS      : Can be used at any moment after a sucessful Start or
//                ModifyStart.
//                See implementation of movement function C_MoveToField
//                in Wexentry.cpp
//
// CHANGES      : 03 Sep 1999, VC, Creation
////////////////////////////////////////////////////////////////////////////////

////////////////////////////////////////////////////////////////////////////////
// FUNCTION NAME: NextField
//
// PROTOTYPE    : CDEItemBase* NextField( BOOL bSaveCur/* = TRUE */);
//
// OBJECTIVE    : Try to reach the next field to capture data. Executes all the
// procedures until reach this field. When is called at first time, normally
// PreProc of Level0, PreProc of Level1, PreProc of first Group and Preproc of
// the field are executed.
//
// PARAMETERS:
// bSaveCur indicating if the data buffer of the current field need to be saved.
//
// RETURN       : a pointer to the field for capturing data. NULL if there is no
// more fields for capturing.
//              Additionally modify the current field (m_pCurField).
//
// REMARKS      : Call only if Start was successfull. If we are in modification
// mode, ModifyStart must be called before.
//
// CHANGES      : 23 Jun 1999, RHF, Creation
////////////////////////////////////////////////////////////////////////////////

////////////////////////////////////////////////////////////////////////////////
// FUNCTION NAME: PreviousField
//
// PROTOTYPE    : CDEItemBase* PreviousField( BOOL bSaveCur/* = TRUE */);
//
// OBJECTIVE    : Try to reach the previous field.
//
// PARAMETERS:
// bSaveCur indicating if the data buffer of the current field need to be saved.
//
// RETURN       : a pointer to the field for capturing data. NULL if there is no
// more fields for capturing.
//              Additionally modify the current field (m_pCurField).
//
// REMARKS      : Call only if Start was successfull. If we are in modification
// mode, ModifyStart must be called before.
//
// CHANGES      : 23 Jun 1999, RHF, Creation
////////////////////////////////////////////////////////////////////////////////

////////////////////////////////////////////////////////////////////////////////
// FUNCTION NAME: SetStopNode
//
// PROTOTYPE    : bool SetStopNode( int iNode )
//
// OBJECTIVE    : Set an stop node of the current Tree for modification mode.
//
// PARAMETERS   : iNode, the node to be set.
//
// RETURNS      : TRUE/FALSE. TRUE if the stop node can be seted.
//
// CHANGES      : 08 Sep 1999, RHF, Creation
////////////////////////////////////////////////////////////////////////////////

////////////////////////////////////////////////////////////////////////////////
// FUNCTION NAME: IsNewCase
//
// PROTOTYPE    : bool IsNewCase() const;
//
// OBJECTIVE    : Inform if we are input a new case.
//
// PARAMETERS   : none
//
// RETURNS      : TRUE/FALSE. TRUE if PreLevel 0 or PreLevel 1 was executed
//                recently. FALSE if any PROCTYPE_POST procedure was executed.
//
// CHANGES      : 20 Sep 1999, RHF, Creation
////////////////////////////////////////////////////////////////////////////////

////////////////////////////////////////////////////////////////////////////////
// FUNCTION NAME: GetCapi
//
// PROTOTYPE    : CCapi* GetCapi() const;
//
// OBJECTIVE    : Get the capi handler
//
// PARAMETERS   : none
//
// RETURNS      : NULL if no CAPI attached
//
// CHANGES      : 13 Jan 1999, RHF, Creation
////////////////////////////////////////////////////////////////////////////////

////////////////////////////////////////////////////////////////////////////////
// FUNCTION NAME: GetDeFld
//
// PROTOTYPE    : bool GetDeFld(const CDEField* pEntryField, DEFLD* pDeField ) const;
//
// OBJECTIVE    : Do a DEFLD from a CDEField
//
// PARAMETERS   : pEntryField containing a field and pDeField where the result
//                        will be returned
//
// RETURNS      : true is pDeField was filled. false if pEntryField doesn't have a
//                                dictionary item
//
// CHANGES      : 13 Jan 1999, RHF, Creation
////////////////////////////////////////////////////////////////////////////////

////////////////////////////////////////////////////////////////////////////////
// FUNCTION NAME: EndLevel
//
// PROTOTYPE    : CDEItemBase* EndLevel( bool bPostProcCurField, bool bPostProcAllOthers,
//                                         int iNextLevelToCapture, bool bWriteNode) const;
//
// OBJECTIVE    : Finish the current level.
//
//
// PARAMETERS   : bPostProcCurField = do we execute the postproc of the current field?
//                                bPostProcAllOthers = is this treated as a skip or an advance?
//                                iNextLevelToCapture = which level to go to next
//                                bWriteNode = should engine write this node to disk?

//
// RETURNS      : If a movement has been operated, the engine returns the
//                identification of the reached field; the current field
//                (m_pCurField) is modified according to the movement made.
//                Returns NULL if the target field had a "cannot" condition,
//                no movement has been performed.
//
// CHANGES      : 15 Feb 2000, VC/RHF, Creation
////////////////////////////////////////////////////////////////////////////////


class CLASS_DECL_ZENTRYO CRunAplEntry : public CRunApl
{
public:
    CRunAplEntry(CNPifFile* pPifFile);
    ~CRunAplEntry();

    bool LoadCompile();
    bool End( const bool bCanExit );
    bool Start( const int iMode );
    bool Stop();

    bool FinalizeInitializationTasks();

    DataRepository* GetInputRepository();
    int GetInputDictionaryKeyLength() const;

private:
    CEntryIFaz*     m_pEntryIFaz;
    CDEItemBase*    m_pCurField;
    DEFLD*          m_pCurEngineField;
    bool            m_bFlagModifyStart; // Flag for control start of modify mode
    bool            m_bSetEndingModify; // 20110111

    std::optional<double> m_nextAutoPartialSaveTimestamp;
    std::optional<double> m_nextParadataDeviceStateTimestamp;

    bool            m_bForward;

    void    SetCurrentObjects( DEFLD* pFld );

    CString GetVal(const DEFLD* pDeField);
    void    EvaluateFieldAndIndices(const CDEField* pField, DEFLD& DeFld) const;

    int     GetStatus( const DEFLD* pDeField );

    void Init();

public:
    bool ModifyStart();
    bool ModifyStop();

    bool HasModifyStarted() const { return m_bFlagModifyStart; }

    // 20110111
    void SetEndingModifyMode(bool val = true) { m_bSetEndingModify = val; }
    bool IsEndingModifyMode() { return m_bSetEndingModify; }

    CDEItemBase* EndGroup( bool bPostProc );
    CDEItemBase* EndGroupOcc( bool bPostProc ); // VC Apr 04, 2000

    CDEItemBase* MoveToField( const DEFLD* pDeField, bool bPostProc = true);
    CDEItemBase* MoveToField( const int iVar, const int iOcc, bool bPostProc = true);
    CDEItemBase* MoveToField( const CDEField* pEntryField, bool bPostProc = true);
    CDEItemBase* MoveToField(const CaseItemReference& case_item_reference, bool bPostProc = true);
    CDEItemBase* MoveToFieldOrBlock(const CDEItemBase* pEntryEntity, bool bPostProc = true);
    CDEItemBase* MoveToBlock(const CDEBlock* pEntryBlock, bool bPostProc = true);

    CDEItemBase* NextField( BOOL bSaveCur, bool advance_past_block = false ); // RHF Aug 08, 2002

    CDEItemBase* PreviousField( BOOL bSaveCur/* = TRUE*/);

    CString GetVal(int iVar, int iOcc);
    CString GetVal(const CDEField* pField);

    void PutVal(const CDEField* pField, CString csValue, bool* value_has_been_modified = nullptr);

    int  GetStatus(int iVar, int iOcc);
    int  GetStatus(const CDEField* pField);
    int  GetStatus3(const DEFLD3* pDeField);// RHF Jul 31, 2000

    bool IsPathOn();

    bool IsNewCase();
    CCapi* GetCapi() const; // RHF Jan 13, 2000
    bool   GetDeFld( const CDEField* pEntryField, DEFLD* pDeField ) const; // RHF Jan 13, 2000

    CDEItemBase* EndLevel( bool bPostProcCurField, bool bPostProcAllOthers, int iNextLevelToCapture, bool bWriteNode);

    int  GetCurrentLevel( int *iNodeNumRelative=NULL, int* iNodeNumAbsolute=NULL ) const;

    CDEItemBase*    ResetCurrentObjects( DEFLD* pReachedFld ); // RHF Aug 09, 2000

    CDEItemBase*    InsertOcc( bool& bRet );
    CDEItemBase*    DeleteOcc( bool& bRet );
    CDEItemBase*    SortOcc(  const bool bAscending, bool& bRet  );
    CDEItemBase*    InsertOccAfter( bool& bRet );       // victor Mar 26, 02

    CDEItemBase* PreviousPersistentField();

    bool    IsAutoEndGroup(); // RHF Nov 07, 2000

    CDEItemBase* AdvanceToEnd( bool bStopOnNextNode, BOOL bSaveCur, int iStopNode=-1 ); // RHF Jan 25, 2001

    // Node Operations
    bool IsNewNode() const;
    bool SetStopNode(int iNode);
    bool DeleteNode(const CaseKey& case_key, int iNode); // Engine off

    bool PartialSaveCase(APP_MODE defaultMode, bool bClearSkipped);

    // Other methods
    CSettings* GetSettings() { return ( m_pEntryIFaz != nullptr ) ? m_pEntryIFaz->GetSettings() : nullptr; }

    void SetStopAdvance(bool bFlag) { m_pEntryIFaz->GetEntryDriver()->SetStopAdvance(bFlag);} //SAVY
    void SetStopOnOutOfRange(bool bFlag) { m_pEntryIFaz->GetEntryDriver()->SetStopOnOutOfRange(bFlag);} //SAVY

    void SetIgnoreWrite(bool bFlag) { m_pEntryIFaz->GetEntryDriver()->SetIgnoreWrite(bFlag);} //SAVY
    void SetInteractiveEdit(bool bFlag) { m_pEntryIFaz->GetSettings()->SetInteractiveEdit(bFlag);} // 20130711 modified where this is located

    // BMD  15 Feb 2005
    CEntryDriver* GetEntryDriver() { return m_pEntryIFaz->GetEntryDriver();}
    // --- for CsDriver only                    <begin> // victor Feb 20, 02
    void ResetDoorCondition( void );
    // --- for CsDriver only                    <end>   // victor Feb 20, 02


    bool HasSpecialFunction(SpecialFunction special_function);
    double ExecSpecialFunction(SpecialFunction special_function, double argument = 0);

    void SetProgressForPreEntrySkip(); // 20130415

    // Notes for Field
    bool EditNote(bool case_note);
    std::shared_ptr<const CaseItemReference> ReviewNotes();

    CDEFormFile*     GetFormFileInProcess();
    CDEFormFile*     GetPrimaryFormFile();
    bool             InEnterMode();

    int              GetNumLevels( bool bPrimaryFlow ); // If not primary, current
    CString          GetCurrentKey( int iLevel );

    bool SetCurrentLanguage(wstring_view language_name);
    std::vector<Language> GetLanguages(bool include_only_capi_languages = true) const;
    bool ChangeLanguage();

    int             GetCurrentLevel();
    CDEItemBase*    GetCurItemBase();


    CDEField*       GetDeField( DEFLD* pFld );
    void            RunGlobalOnFocus( int iVar );

    bool            QidReady( int iLevel );

    bool            RestorePartial();

    void            ToggleCapi( int iSymVar );

    int             GetKeyLen( const CDataDict* pDataDict, int iLevel );

private:
    void SetupOperatorId();
public:
    void SetOperatorId(const CString& operator_id);
    CString GetOperatorId() const;

    bool            HasSomeRequest(); // RHF Nov 06, 2003
    CDEItemBase*    RunCsDriver( bool bCheckRange ); // RHF Nov 06, 2003
//BUCEN_2003 Changes End

    CNPifFile*  GetPifFile();  //FABN Oct 2005

    //FABN Jan 03, 2006 : fast access to know which is the form that holds the given item.
    CDEForm*        GetForm( CDEItemBase* pItemBase, bool bPrimaryFlow );

    //FABN Jan 04, 2006 : GetEntryObject can be used by the UI to know how should be displayed the data entry page.
    CDEFormBase*    GetEntryObject( CDEItemBase* pItem, bool bPrimaryFlow );

    //FABN Jan 04, 2006 : is just GetEntryObject for the current item
    CDEFormBase*    GetCurEntryObject( bool bPrimaryFlow );

    bool RunSync(const AppSyncParameters& params);

    Userbar* GetUserbar();
    void PauseUserbar(bool pause);

    void ExecuteCallbackUserFunction(int field_symbol_index, UserFunctionArgumentEvaluator& user_function_argument_evaluator);

    void StopIfNecessary(); // 20121023 for stopping after OnKey and OnChar calls
    bool IsStopRequested(); // 20140131
    bool IsStopRequestedAndTurnOffStop(); // 20140131

    enum class ShowRefusalProcessorAction { ReturnWhetherRefusedValuesAreHidden, ShowRefusedValues };
    bool ShowRefusalProcessor(ShowRefusalProcessorAction action, bool process_other_displayed_fields_in_block);

    void ProcessAdd();
    void ProcessInsert(double insert_before_position_in_repository);

    enum class ProcessModifyAction { GotoNode, AddNode, InsertNode };
    bool ProcessModify(double dPositionInRepository,bool* pbMoved,PartialSaveMode* pePartialSaveStatusMode,CDEItemBase** ppItem,int iNode,ProcessModifyAction eModifyAction);
    bool ProcessModify(double dPositionInRepository,bool* pbMoved,PartialSaveMode* pePartialSaveStatusMode,CDEItemBase** ppItem);

    // calls to expose the entry and engine drivers
    Case& GetInputCase() { return GetEntryDriver()->GetInputCase(); }
    Pre74_CaseLevel* GetCaseLevelNode(int iLevel) { return GetEntryDriver()->GetCaseLevelNode(iLevel); }

    void RunPeriodicEvents();

    bool InAddMode() const;
    bool InModifyMode() const;
    bool InVerifyMode() const;

    void ViewCurrentCase();
    void ViewCase(double position_in_repository);
};
