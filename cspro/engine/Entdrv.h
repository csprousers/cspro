#pragma once
//---------------------------------------------------------------------------
//  File name: EntDrv.h
//
//  Description:
//          Header for the Entry' driver
//
//  History:    Date       Author   Comment
//              ---------------------------
//              .. ... ..   ..      many, many prehistoric tailoring
//              30 May 01   vc      Adding methods to fit CsDriver behavior (see EntDrv.h & DeWrite.cpp)
//              25 Jul 01   vc      Adding flow-administrator for Entry runs            // victor Jul 25, 01
//
//---------------------------------------------------------------------------
#include <engine/DEFLD.H>
#include <engine/DEPROG.H>
#include <engine/Engarea.h>
#include <engine/Engdrv.h>
#include <engine/Tbd_save.h>
#include <Zissalib/flowatom.h>
#include <engine/Engdrv.h>

class CapiQuestionManager;
class CaseItemReference;
class CCapi;
class CEntryIFaz;
class CFlAdmin;
class CsDriver;
class ResponseProcessor;


class CEntryDriver : public CEngineDriver
{
// --- Definitions ---------------------------------------------------------
public:
    enum    NodeAdvance {                               // victor Dec 10, 01
                NodeAdvanceToNextNode  = INT_MAX - 1,
                NodeAdvanceToEndOfCase = INT_MAX,
                NodeAdvanceNone        = -1
    };

// --- Data members --------------------------------------------------------
private:
    // --- CEntryDriver own data
    CEntryDriver*   m_pEntryDriver;
    CEntryIFaz*     m_pEntryIFaz;

    // --- operating controls
private:
    CsDriver*   m_pCsDriver;            // the active CsDriver // victor Mar 07, 02
    int         m_iActiveLevel;         // the active level // was Decurlevl
public:
    int         Issademode;
    int         Decurmode;
public:
    bool        m_bMustEndEntrySession; // the session must be ended // was Deend
    int         iModification_Node;     // ... value -1 means "stop at next node"
    int         Decorrlevl;
    int         Exit_Code;


    // --- Modify/Verify flags, plus "new case" status
private:
    bool        m_bModifyMode;
    bool        m_bInsertMode;
    bool        m_bVerifyMode;
    bool        m_bNewCase;

    APP_MODE    m_ePartialMode;

    // --- other data
    CString     m_operatorId;

public:
    bool        m_bFileStarted;
    bool        m_bStopAdvance; // SAVY
    bool        m_bStopOnOutOfRange; // SAVY
    bool        m_bIgnoreWrite ; // SAVY

    int         m_iLastRefGroupSym; // RHF Mar 05, 2002 Last group refreshed
    int         m_iLastRefGroupOcc; // RHF Mar 05, 2002 Last group occurrence refreshed

// --- Methods -------------------------------------------------------------
    //SAVY Added this for interactive edit
   bool GetStopAdvance() const     { return m_bStopAdvance;}
   void SetStopAdvance(bool bFlag) { m_bStopAdvance = bFlag;}

   bool GetIgnoreWrite() const     { return m_bIgnoreWrite;}
   void SetIgnoreWrite(bool bFlag) { m_bIgnoreWrite = bFlag;}

   bool GetStopOnOutOfRange() const     { return m_bStopOnOutOfRange;}
   void SetStopOnOutOfRange(bool bFlag) { m_bStopOnOutOfRange = bFlag;}

// --- Methods -------------------------------------------------------------
public:
    // constructor/destructor
    CEntryDriver(Application* pApplication, CEntryIFaz* pEntryIFaz);
    ~CEntryDriver();

    // --- Modify/Verify flags, plus "new case" status
public:
    bool     IsModification() const              { return m_bModifyMode; }
    void     SetModificationMode(bool bModify)   { m_bModifyMode = bModify; }
    bool     IsInsertMode() const                { return m_bInsertMode; }
    void     SetInsertMode(bool insert_mode)     { m_bInsertMode = insert_mode; }
    bool     IsVerify() const                    { return m_bVerifyMode; }
    void     SetVerifyMode(bool bVerify)         { m_bVerifyMode = bVerify; }
    void     SetPartialMode(APP_MODE eMode)      { m_ePartialMode = eMode; }
    APP_MODE GetPartialMode() const              { return m_ePartialMode; }
    bool     IsPartial();


    void    SetAddModeFlags( void )             { Decurmode = ADDMODE; Issademode = ADD; iModification_Node = -1; }     // victor May 30, 01
    bool    MustStopAtNextNode( void )          { return( Decurmode == ADVMODE && iModification_Node == NodeAdvance::NodeAdvanceToNextNode ); } // victor May 30, 01
    bool    IsNewCase( void )                   { return m_bNewCase; }  // victor May 30, 01
    void    SetNewCase( bool bX )               { m_bNewCase = bX; }    // victor May 30, 01

    // --- managing the active level (suppress former mentions to Decurlevl)
public:
    int     GetActiveLevel( void )              { return m_iActiveLevel; }
    void    SetActiveLevel( int iActiveLevel )  { m_iActiveLevel = iActiveLevel; }

    // --- Entifaz interaction
    CEntryIFaz* GetEntryIFaz() {  return m_pEntryIFaz; }

    // --- CsDriver interaction                         // victor Mar 07, 02
public:
    void    SetCsDriver( CsDriver* pCsDriver );
    CsDriver* GetCsDriver( void );

    // --- wexentry-related stuff
public:
    void    reset_lastopenlevels( void );
    void    exentrystart( void );
    void    dedriver_start( void );

    // --- execute-driver
private:
    bool    CheckIdCollision(int iSymDic);
public:
    int     EvaluateNode( int iLevel );

public:
    void    doend( void );

    // --- related to flow management
public:
    bool    MakeField3( DEFLD3* pszFld3, const DEFLD* pszFld ); // RHF Jul 31, 2000

    // --- decorr.cpp
    void    corr_init( void );

    // --- defuncs.cpp
public:
    int     dedemode();

    // --- managing input file and case-id
public:
    void    DoQid( void );

    bool    QidReady( int iLevel );

public:
    void    DeInitLevel( int iLevel );

    void    deset_low( DEFLD* fld );
    bool    ConfirmValue(bool value_is_notappl, VART* pVarT, CNDIndexes& theIndex);
    void    ResetLevelCurOccs( int iLevel, bool bResetItAlso );
    void    ResetGroupCurOccs( int iSymGroup, bool bResetItAlso = true );

    int     DisplayMessage(MessageType message_type, int message_number, const std::wstring& message_text, const void* extra_information = nullptr) override;
    int     DisplayMessage_pre77(MessageType message_type, int message_number, const CString& message_text, const void* extra_information = nullptr);

    // defuncs
public:
    void    WipeGroupFieldColors( int iSymGroup );

    // --> flow-administrator for Entry runs    <begin> // victor Jul 25, 01
private:
    CFlAdmin*   m_pFlAdmin;
public:
    void    DestroyFlAdmin( void );
    CFlAdmin* GetFlAdmin( void )                { return m_pFlAdmin; }
    // --> flow-administrator for Entry runs    <end>   // victor Jul 25, 01

    // --> logic' Enter() command execution     <begin>
private:
    FLOW*   m_pEnteredFlow;
public:
    bool    InEnterMode( void );
    int     GetEnteredSymDic( void );
    void    SetEnterMode( FLOW* pFlow );
    void    ResetEnterMode( void );
    // --> logic' Enter() command execution     <end>

    // --> CAPI support
private:
    CCapi*      m_pCapi;
// --- methods -------------------------------------------------------------
public:
    void    SetCapi( CCapi* pCapi ) { m_pCapi = pCapi; }
    CCapi*  GetCapi()               { return m_pCapi; }

// --- methods -------------------------------------------------------------
public:
    void    TakeCareOfEndingNode( int iEndingLevel, bool bIgnoreWrite ); // victor Jun 07, 01
    bool    CheckBeforeWriteEndingNode( int iSymDic, int iMaxLevel, int iEndingLevel, bool bIgnoreWrite ); // victor May 30, 01
    bool    WriteEndingNode( int iEndingLevel, bool bIgnoreWrite ); // victor May 30, 01

public:
    long    WriteData();

    void PrepareCaseFromEngineForQuestionnaireViewer(DICT* pDicT, Case& data_case) override;

private:
    void    ResetCaseLevels();

public:
    bool    PartialSaveCase(bool bClearSkipped = false);

    bool    IsAddedNode( int iLevel );

    // --> LevCt: controlling levels operation <begin>  // victor Feb 29, 00
// --- definitions ---------------------------------------------------------
    // ---- info status of this level-node (similar to the original "virginity"):
    //    - NoData : no fields entered
    //    - NoInfo : only Persistent/Protected entered
    //    - HasInfo: at least one field (non Persistent/Protected) was entered and became green
public:
    enum        eNodeInfo { NoSon, NoData, NoInfo, HasInfo };

    // ---- source of the information:
    //    - Keyboard: the node comes from the operator/application
    //    - FromFile: the node was retrieved from data file (Modify activity)
public:
    enum        eNodeSource { Keyboard, FromFile };

// --- data members --------------------------------------------------------
private:
    eNodeInfo   m_aNodeInfo[MaxNumberLevels + 1];
    eNodeSource m_aNodeSource[MaxNumberLevels + 1];

    // ---- status of current son-node (the immediate descendant):
    //    - NoSon  : there is no current son-node
    //    - NoData : current son-node has NodeInfo NoData
    //    - NoInfo : current son-node has NodeInfo NoInfo
    //    - HasInfo: current son-node has NodeInfo HasInfo
private:
    eNodeInfo   m_aCurrentSonNode[MaxNumberLevels + 1];

    // ---- number of son-nodes written:
    int         m_iWrittenSons[MaxNumberLevels + 1];

    // ---- forced-next-level requested                 // victor Mar 03, 00
    int         m_iForcedNextLevel;

    // ---- forced-no-write requested                   // victor Mar 03, 00
    bool        m_bForcedNoWrite[MaxNumberLevels + 1];

// --- methods -------------------------------------------------------------
public:
    // --- extraction
    eNodeInfo   LevCtGetInfo( int iLevel )              { return m_aNodeInfo[iLevel]; }
    eNodeInfo   LevCtGetSonInfo( int iLevel )           { return m_aCurrentSonNode[iLevel]; }
    bool        LevCtHasNoSon( int iLevel )             { return( m_aCurrentSonNode[iLevel] == NoSon ); }
    bool        LevCtHasSon( int iLevel )               { return( m_aCurrentSonNode[iLevel] != NoSon ); }
    bool        LevCtHasSonNoData( int iLevel )         { return( m_aCurrentSonNode[iLevel] == NoData ); }
    bool        LevCtHasSonNoInfo( int iLevel )         { return( m_aCurrentSonNode[iLevel] == NoInfo ); }
    bool        LevCtHasSonWithInfo( int iLevel )       { return( m_aCurrentSonNode[iLevel] == HasInfo ); }
    eNodeSource LevCtGetSource( int iLevel )            { return m_aNodeSource[iLevel]; }
    int         LevCtGetWrittenSons( int iLevel )       { return m_iWrittenSons[iLevel]; }

    // ---- forced-next-level requested                 // victor Mar 03, 00
    bool        LevCtHasForcedNextLevel( void )         { return( m_iForcedNextLevel > 0 ); }
    int         LevCtGetForcedNextLevel( void )         { return m_iForcedNextLevel; }

    // ---- forced-no-write requested                   // victor Mar 03, 00
    bool        LevCtHasForcedNoWrite( int iLevel )     { return m_bForcedNoWrite[iLevel]; }

    // --- assignment
    void        LevCtInitLevel( int iLevel );
    void        LevCtStartLevel( int iLevel );
    void        LevCtSetInfo( int iLevel, eNodeInfo NewInfo = NoData );
    void        LevCtSetSonInfo( int iLevel, eNodeInfo NewInfo = NoSon );
    void        LevCtAddWrittenSon( int iLevel );

    // ---- forced-next-level requested                 // victor Mar 03, 00
    void        LevCtSetForcedNextLevel( int iLevel )   { m_iForcedNextLevel = ( iLevel < 1 ) ? 0 : ( iLevel > (int)MaxNumberLevels ) ? (int)MaxNumberLevels : iLevel; }
    void        LevCtResetForcedNextLevel( void )       { m_iForcedNextLevel = 0; }

    // ---- forced-no-write requested                   // victor Mar 03, 00
    void        LevCtSetForcedNoWrite( int iLevel )     { m_bForcedNoWrite[iLevel] = true; }
    void        LevCtResetForcedNoWrite( int iLevel )   { m_bForcedNoWrite[iLevel] = false; }
    // --> LevCt: controlling levels operation <end>    // victor Feb 29, 00

    bool        ReportToInterface( int iSymbol, int iOcc, int iDirection, CFlowAtom::AtomType xAtomType ); // RHF Mar 05, 2002

    const CString& GetOperatorId() const                     { return m_operatorId; }
    void           SetOperatorId(const CString& operator_id) { m_operatorId = operator_id; }

private:
    std::shared_ptr<CapiQuestionManager> m_pQuestMgr;

public:
    void BuildQuestMgr();
    CapiQuestionManager* GetQuestMgr()                       { return m_pQuestMgr.get(); }
    std::shared_ptr<CapiQuestionManager> GetSharedQuestMgr() { return m_pQuestMgr; }


    // persistent and auto increment field handling
private:
    std::map<VART*,CString>* m_pmapPersistentFields;
    bool m_bPersistentFieldsLastUpdatedOnInsertOrModify;

    std::unique_ptr<std::map<VART*, std::wstring>> m_prefilledNonPersistentFields;

    std::map<VART*, int64_t>* m_pmapAutoIncrementFields;
    DataRepository* m_pLastMappedAutoIncrementFieldsRepository;

    bool LoadAutoIncrementFields();

public:
    bool LoadPersistentFields(std::optional<double> position_in_repository = std::nullopt);
    void InitializePersistentFields(int iLevel);
    void UpdatePersistentField(VART* pVarT);
    bool HasPersistentFields() const;

    void InitializeAutoIncrementFields();
    void UpdateAutoIncrementField(VART* pVarT);

    void PrefillKeyFromPff();
    void PrefillNonPersistentFields();

    ResponseProcessor* GetResponseProcessor(const DEFLD* defld);

    // notes
    CString GetNoteContent(const DEFLD& defld);
    void SetNote(const DEFLD& defld, const CString& note_content);
    bool EditNote(bool case_note, const DEFLD* defld = nullptr);

    // level node processing
private:
    std::set<Pre74_CaseLevel*> m_setUpdatedLevelNodes;

public:
    int GetTotalNumberNodes();
    int GetNodeNumber(Pre74_CaseLevel* pPre74_CaseLevel);
    Pre74_CaseLevel* GetCaseLevelNode(int iLevel);

    bool IsLevelNodeNew(int iLevel) const;
    void AddLevelNode(int iLevel);
    void GetNextLevelNode(int iSearchedLevel);
    void UpdateLevelNode(int iEndingLevel, bool bPartialSave = false, Case* data_case = nullptr);
    void UpdateLevelNodeAtEnd(int iEndingLevel);
    void ClearUnprocessedLevels(int iLevel);

    // case viewing
public:
    void ViewCurrentCase();
    void ViewCase(double position_in_repository);
};
