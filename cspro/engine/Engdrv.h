#pragma once

//---------------------------------------------------------------------------
//  File name: EngDrv.h
//
//  Description:
//          Header for engine-driver class
//
//  History:    Date       Author   Comment
//              ---------------------------
//              20 Nov 99   RHF     Basic conversion
//              10 May 00   vc      Basic customization
//              14 Mar 01   vc      Adding support for batch-skip management
//              04 Apr 01   vc      Tailoring for RepMgr compatibility
//              05 Nov 02   RHF     Adding notes & form support
//---------------------------------------------------------------------------

#include <Zissalib/CsKernel.h>
#include <engine/DEFLD.H>
#include <engine/Exappl.h>
#include <engine/Engarea.h>
#include <engine/Messages.h>
#include <zUtilO/Interapp.h>
#include <zUtilO/ProcessSummary.h>
#include <zCaseO/Note.h>
#include <zDataO/DataRepository.h>
#include <zDataO/WriteCaseParameter.h>
#include <zListingO/Lister.h>


constexpr bool NO_VISUAL_VALUE = false;
constexpr bool VISUAL_VALUE = true;

class Application;
class CIntDriver;
class CEngineCompFunc;
class CNPifFile;
class CDEItemBase;
class CDEForm;
class CommonStore;
class CompilerCreator;
class CWnd;
class ExecutionStackEntry;
class InterpreterAccessor;
class LogicByteCode;
class MessageManager;
class Pre74_CaseLevel;
class Pre74_CaseRecord;
class Serializer;
class Userbar;
namespace Listing { class ErrorLister; class WriteFile; }


class CEngineDriver
{
    // --- Data members --------------------------------------------------------
    // --- engine links
public:
    Application*    m_pApplication;
    CEngineDriver*  m_pEngineDriver; //this
    std::unique_ptr<CIntDriver> m_pIntDriver;
public:
    CEngineDefines  m_EngineDefines;
    CEngineArea     m_EngineArea;
    CSettings       m_EngineSettings;
    std::unique_ptr<CEngineCompFunc> m_pEngineCompFunc;

    CEngineDefines*  m_pEngineDefines;
    CEngineArea*     m_pEngineArea;
    EngineData*      m_engineData;
    CSettings*       m_pEngineSettings;
    CEngineArea*     getEngineAreaPtr() const { ASSERT(m_pEngineArea != 0); return m_pEngineArea; }
    EngineData&      GetEngineData()          { return *m_engineData; }

protected:
    Logic::SymbolTable& GetSymbolTable() const { return m_engineData->symbol_table; }

private:
    std::unique_ptr<const ExecutionStackEntry> m_executionStackEntry;

    // userbar
private:
    std::unique_ptr<Userbar> m_userbar;

public:
    bool HasUserbar() const                           { return ( m_userbar != nullptr ); }
    Userbar& GetUserbar()                             { ASSERT(HasUserbar()); return *m_userbar; }
    void SetUserbar(std::unique_ptr<Userbar> userbar);

    // CommonStore
private:
    std::shared_ptr<CommonStore> m_commonStore;
public:
    // marked as virtual to make it accessible outside of the engine library
    virtual std::shared_ptr<CommonStore> GetCommonStore();

public:
    ModuleType      m_Issamod;
    int             m_ExMode;               // $MODE   for ENTRY & INDEX apps
private:
    std::wstring    m_currentLanguageName;
public:
    const TCHAR*    m_lpszExecutorLabel;    // executor label   VC Nov 20, 95

    // basic arrays
public:
    DICX*           m_Dicxbase;         // executor' Dict info
    SECX*           m_Secxbase;         // executor' Sect info
    VARX*           m_Varxbase;         // executor' Vars info

    // --- associated or automatic files
public:
    CNPifFile*      m_pPifFile;
    std::vector<std::shared_ptr<DataRepository>> m_batchOutputRepositories;

public:
    CString         m_TmpCtabTmp;

    // message managers
private:
    std::shared_ptr<MessageManager> m_systemMessageManager;
    std::shared_ptr<SystemMessageIssuer> m_systemMessageIssuer;

    std::shared_ptr<MessageManager> m_userMessageManager;
    std::unique_ptr<MessageEvaluator> m_userMessageEvaluator;

    void UpdateMessageIssuers(bool set_on_system_message_defined = false);

public:
    void BuildMessageManagers();

    MessageManager& GetSystemMessageManager()                           { return *m_systemMessageManager; }
    SystemMessageIssuer& GetSystemMessageIssuer()                       { return *m_systemMessageIssuer; }
    std::shared_ptr<SystemMessageIssuer> GetSharedSystemMessageIssuer() { return m_systemMessageIssuer; }

    MessageManager& GetUserMessageManager()     { return *m_userMessageManager; }
    MessageEvaluator& GetUserMessageEvaluator() { return *m_userMessageEvaluator; }

    virtual int DisplayMessage(MessageType message_type, int message_number, const std::wstring& message_text, const void* extra_information = nullptr);

    // listing and write files
private:
    std::shared_ptr<ProcessSummary> m_processSummary;
    std::shared_ptr<Listing::Lister> m_lister;
    std::shared_ptr<Listing::ErrorLister> m_compilerErrorLister;
    std::unique_ptr<Listing::WriteFile> m_writeFile;

public:
    const std::shared_ptr<ProcessSummary>& GetProcessSummary() const { return m_processSummary; }
    std::shared_ptr<ProcessSummary> GetProcessSummary()              { return m_processSummary; }

    Listing::Lister* GetLister()                   { return m_lister.get(); }
    Listing::ErrorLister* GetCompilerErrorLister() { return m_compilerErrorLister.get(); }
    Listing::WriteFile* GetWriteFile()             { return m_writeFile.get(); }

    void OpenListerAndWriteFiles();
    void CloseListerAndWriteFiles();
    void StartLister();
    void StopLister();

    // batch-skip management                            // victor Mar 14, 01
private:
    bool            m_bSkipping;        // "skipping" flag
    C3DObject*      m_pSkippingSource;  // skip-source description
    int             m_iSkippingSource;  // skip-source proc-type (PROCTYPE_PRE or PROCTYPE_POST)
    C3DObject*      m_pSkippingTarget;  // skip-target description
    int             m_iSkippingTarget;  // skip-source proc-type (PROCTYPE_PRE or PROCTYPE_POST)

    // miscellaneous
private:
    bool            m_bUsePrevLimits; // RHF May 14, 2003

    // stop code
public:
    const std::optional<int>& GetStopCode() const { return m_stopCode; }
    void SetStopCode(int stop_code)               { m_stopCode = stop_code; }
    void ResetStopCode()                          { m_stopCode.reset(); }

private:
    std::optional<int> m_stopCode;

public:
    CString         m_csAppFullName;

private:
    bool            m_bHasOutputDict; // RHF Aug 23, 2002
    bool            m_bHasSomeInsDelSortOcc; // RHF Dec 13, 2002
public:
    //SAVY 10/15/03 for tab special values
    bool            m_bTabProcessSpecial;

    // --- Methods -------------------------------------------------------------

    // construction/destruction
public:
    CEngineDriver(Application* pApplication, bool bDoInterpreter, CompilerCreator* compiler_creator = nullptr);
    CEngineDriver(CNPifFile* pPifFile);
    virtual ~CEngineDriver();

    // marked as virtual to make it accessible outside of the engine library
    virtual std::shared_ptr<InterpreterAccessor> CreateInterpreterAccessor();

public:
    void          InitAppName();
    void          SetPifFile( CNPifFile* pPifFile );
    CNPifFile*    GetPifFile()    { ASSERT(m_pPifFile != nullptr);     return m_pPifFile;     }
    Application* GetApplication() { ASSERT(m_pApplication != nullptr); return m_pApplication; }

    const std::wstring& GetCurrentLanguageName() const      { return m_currentLanguageName; }
    void SetCurrentLanguageName(std::wstring language_name) { m_currentLanguageName = std::move(language_name); }

public:
    bool    exapplinit();
    bool    attrload();

    // batch-skip management                            // victor Mar 14, 01
public:
    bool    IsSkipping() const { return m_bSkipping; }
    bool    IsSkippingTargetReached( int iCurrentSymbol, CNDIndexes& aIndex );
    bool    IsSkippingTargetReached( int iCurrentSymbol, int* aIndex, int iProgType );

    void    SetSkipping( int iSymbol, int* aIndex, int iProgType, bool bTarget = false );

    //////////////////////////////////////////////////////////////////////////
    // SetSkipping -> new 3d versions: SetSkippingSource + SetSkippingTarget
    // rcl, Sept 04, 2004
    void    SetSkippingSource( C3DObject& theSourceObject, int iProgType );
    void    SetSkippingTarget( C3DObject& theTargetObject, int iProgType );
    //////////////////////////////////////////////////////////////////////////

    void    ResetSkipping();
    C3DObject* GetSkippingSource() { return m_pSkippingSource; }
    C3DObject* GetSkippingTarget() { return m_pSkippingTarget; }

    // --- application' executor functions (batch and data entry processors)
public:
    void    initsect( int iSymSec );
    void    initsect( SECT* pSecT ); // SPEED
    void    initwsect();
    void    ClearLevelNode( DICT* pDicT, int iNodeLevel );  // formerly 'clearlevel'
    bool    EvaluateNode( DICT* pDicT, int iNodeLevel );    // formerly 'evalcase'
    bool    sectadd(SECT* pSecT, const CaseRecord* case_record_with_binary_data);

    void    sectcommonadd( DICT* pDicT, int iLevel );

    void    prepvar( int iSymVar, const CNDIndexes& theIndex, bool bVisualValue/*=false*/ ); // RHF Feb 28, 2001 Add bVisualValue
    void    prepvar( VART* pVarT, const CNDIndexes& theIndex, bool bVisualValue/*=false*/ ); // RHF Feb 28, 2001 Add bVisualValue

    // Refresh ascii buffer fo all occurrences
    void    prepvar( VART* pVarT, bool bVisualValue );  // rcl Jun 25 2004

    void    ShowOutOfRange( VART* pVarT, CNDIndexes* theIndex, TCHAR* pAsciiAddr );

    void    varatof( VARX* pVarX, CNDIndexes& theIndex ); // RHF Jul 22, 2000 // rcl Jun 21, 2004
    void    varatof( VARX* pVarX );                       // rcl Jun 21, 2004
    bool    IsBlankField( int iSymVar, int iOccur );    // victor Mar 20, 00
    bool    IsBlankField( VART* pVarT, int iOccur );    // victor May 30, 00
    bool    IsBlankField( VART* pVarT, CNDIndexes& theIndex ); // rcl, Oct 07, 2004

private:
    void InitializeData();
public:
    void OpenRepository(EngineDataRepository& engine_data_repository, const ConnectionString& connection_string, DataRepositoryOpenFlag open_flag,
                        bool load_binary_data_from_currently_open_repository_before_closing);
    void OpenRepository(DICX* pDicX, const ConnectionString& connection_string, DataRepositoryOpenFlag eOpenFlag,
                        bool load_binary_data_from_currently_open_repository_before_closing);
    bool OpenRepositories(bool open_input_repository);
    bool OpenBatchOutputRepositories(const std::vector<ConnectionString>& output_connection_strings, bool setoutput_mode);

    void CloseRepositories(bool close_input_repository);
    void CloseBatchOutputRepositories();

    void LoadAllBinaryDataFromRepository(EngineDataRepository& engine_data_repository);
    void LoadAllBinaryDataFromRepository(DICX* pDicX);

    DataRepository* GetInputRepository();
    Case& GetInputCase();

    int GetInputDictionaryKeyLength() const;

    CString key_string(const DICT* pDicT);

public:
    int     commonlvl( DICT* pDicT, LPCTSTR lastkey, int lastlevel, LPCTSTR key, int level );

    void    BuildGroupOccsFromNodeRecords( DICT* pDicT, int iNodeLevel ); // victor May 16, 00

public:
    void    initextdi();

public:
    int     DataItemCopy(TCHAR* pTargetBuffer, const TCHAR* pSourceBuffer,
                         std::vector<const CDictItem*>& arrItems, int iFromLevel, int iUntilLevel, bool bCompacted);

    // loading functions for Flows & Dics   <begin>     // victor Dec 27, 99
public:
    FLOW*       GetPrimaryFlow() {
        ASSERT( Appl.GetFlowAt( 0 ) );
        return  Appl.GetFlowAt( 0 );
    }
    FLOW*       GetFlowInProcess()        { return m_pFlowInProcess; }
    GROUPT*     GetGroupTRootInProcess()  { return GetFlowInProcess()->GetGroupTRoot(); }
    void        SetFlowInProcess( FLOW* pFlow ) { m_pFlowInProcess = pFlow; }
    void        ResetFlowInProcess()      { m_pFlowInProcess = NULL; }
private:
    int         m_iGlobalFlowOrder; // for FlowLoad.cpp usage only
    int         m_iAbsoluteFlowOrder;    // RHF Dec 10, 2003
    FLOW*       m_pFlowInProcess;   // pointer to current flow for everything

    // << in ApplLoad >>
public:
    bool LoadApplChildren(CString* pcsLines);
    bool LoadApplMessage();


    // Compiled binary handling ...
    // rcl, Mar 2006
private:
    bool m_bBinaryLoaded;

    size_t LoadBaseSymbols(Serializer& ar);
    void InitCompiledWorkDict();

public:
    void LoadCompiledBinary(); // compall() mirror
    void SaveCompiledBinary();


    // << in DictLoad >>
public:
    bool        LoadApplDics();
private:

    // << in FlowLoad >>
    int         GenerateAbsoluteFlowOrder( CFlowCore*  pFlowCore ); // RHF Dec 10, 2003
public:
    bool        LoadApplFlows();
    bool        AddGroupTForOneSec( int iSymSec ); // RHF Nov 08, 2000
private:
    bool        AddGroupTForOneFlow();
    GROUPT*     AddGroupTForLevelZero( int iMaxLevel );
    bool        AddGroupTForLevel( CDEFormFile* pFormFile, CDELevel* pLevel, GROUPT* pGroupTOwner );
    bool        AddItemToOwner( CDEItemBase* pItem, GROUPT* pGroupTOwner, int iItemInOwner );
    bool        AttachItemToOwner( CDEItemBase* pItem, GROUPT* pGroupTOwner, int iItemInOwner );
    bool        SetOwnerIntoItem( int iSymItem, int iSymGroup );
    bool        AddGroupTForGroup( CDEGroup* pGroup, GROUPT* pGroupTOwner );
    CDELevel*   SearchLevelInFormFile( CDEFormFile* pFormFile, int iLevelNum );
    bool        AddGroupTForFlowDics();
    // RHF COM Nov 08, 2000bool        AddGroupTForOneSec( int iSymSec );
    int         AddGroupTForMultVar( int iSymMultVar, GROUPT::Source eGroupSource, int iSymOwner );
    void        InstallItemIntoGroupT( GROUPT* pGroupT,int iSymItem,int iItem,int iFlowOrder );
    int         NumItemsInMultSet( int iSymMultVar, bool bWithForm, bool* pAllMatching = NULL ); // 'pAllMatching' added // victor Jun 15, 00
    int         PassOverMultSet( int iSymMultVar );
    // loading functions for Flows & Dics   <end>       // victor Dec 27, 99
    // RHF INIC Feb 22, 2000
    void        AttachFormItem( CDEForm* pForm, CDEItemBase* pItem );
    void        AttachOneForm( CDEForm* pForm );
    void        AttachForms();
    // RHF END Feb 22, 2000

public:
    // --- coordinating occurrences of Sections and Groups
    // ------ setting up Sect occurrences from related Groups
    int     SetupSectOccsFromGroups( int iSymSec, bool bPartialSave = false );
    // ------ setting up Group occurrences from related Sects
    void    SetupGroupOccsFromSect( SECT* pSecT );
    void    SetupGroupOccsFromVar( VART* pVarT );

private:
    void CheckSectionOccurrences(int iSymSec, bool bPartialSave);

public:
    //miscellaneous
    void SetHasOutputDict( bool bHasOutputDict )   { m_bHasOutputDict = bHasOutputDict; }
    bool GetHasOutputDict() const                  { return m_bHasOutputDict; }

    void SetHasSomeInsDelSortOcc( bool bHasSomeInsDelSortOcc )   { m_bHasSomeInsDelSortOcc = bHasSomeInsDelSortOcc; }
    bool GetHasSomeInsDelSortOcc() const                         { return m_bHasSomeInsDelSortOcc; }

    void            SetUsePrevLimits( bool bUsePrevLimits ) { m_bUsePrevLimits = bUsePrevLimits; }// RHF May 14, 2003

    int             CompareAbsoluteFlowOrder( int iSourceSymVar, int iSourceOcc, int iTargetSymVar, int iTargetOcc ); // RHF Dec 10, 2003

    void            ResetDynamicAttributes(DICX* pDicX = nullptr);

#ifdef WIN_DESKTOP
private:
    CArray<HKL>             m_vAddedKeyboards; // 20120822
    CMap<UINT,UINT,HKL,HKL> m_mKLID2HKLMap;
public:
    HKL                     LoadKLID(UINT klid);
    void                    UnloadAddedKeyboards();
    UINT                    GetKLIDFromHKL(HKL hKL);
#endif

    void serialize(Serializer& ar);

    // --- DAT files access, case' nodes management
public:
    void    ParseCaseLevel(Case* pCasetainer, Pre74_CaseLevel* pPre74_CaseLevel, const CaseLevel* case_level_with_binary_data, DICT* pDicT);
    void    CopyLevelToRepository(DICT* pDicT,Case* pCasetainer,Pre74_CaseLevel* pPre74_CaseLevel,bool bPartialSave = false);

    virtual void PrepareCaseFromEngineForQuestionnaireViewer(DICT* pDicT, Case& data_case);

private:
    void    CopySectionToRepository(DICT* pDicT,Pre74_CaseRecord* pPre74_CaseRecord,int iSymSec,bool bPartialSave);

protected:
    std::optional<WriteCaseParameter> m_writeCaseParameter;

public:
    bool UsingWriteCaseParameter() const                                { return m_writeCaseParameter.has_value(); }
    void ClearWriteCaseParameter()                                      { m_writeCaseParameter.reset(); }
    void SetWriteCaseParameter(WriteCaseParameter write_case_parameter) { m_writeCaseParameter.emplace(std::move(write_case_parameter)); }
    const WriteCaseParameter* GetWriteCaseParameter() const             { return m_writeCaseParameter.has_value() ? &*m_writeCaseParameter : nullptr; }
    WriteCaseParameter* GetWriteCaseParameter()                         { return const_cast<WriteCaseParameter*>(const_cast<const CEngineDriver*>(this)->GetWriteCaseParameter()); }

    // level node processing
protected:
    struct LoadedPre74_CaseLevel
    {
        Pre74_CaseLevel* m_caseLevel;
        bool m_isNew;
    };

    LoadedPre74_CaseLevel m_aLoadedLevels[MaxNumberLevels];
    void UpdateLoadedLevels(Pre74_CaseLevel* pPre74_CaseLevel,bool bIsNew);
    void ClearLoadedLevels(int iLevel);

    // notes
private:
    void ParseCaseNotes_pre80(DICX* pDicX);
    const Pre74_CaseLevel* FindNoteCaseLevel_pre80(const DICX* pDicX, int field_symbol) const;
    NoteByCaseLevel* GetNoteByCaseLevel_pre80(const NamedReference& named_reference,
        const std::optional<CString>& operator_id, int field_symbol) const;
    void CreateNote_pre80(DICX* pDicX, std::shared_ptr<NamedReference> named_reference, const CString& operator_id,
        const CString& note_content, int field_symbol);
    void DeleteNote_pre80(DICX* pDicX, const NoteByCaseLevel& note_by_case_level);

    Note* FindNote(const NamedReference& named_reference, const std::optional<CString>& operator_id, int field_symbol) const;

public:
    void SetNotesModified(const Symbol* dictionary_symbol);
    void UpdateCaseNotesLevelKeys_pre80(DICX* pDicX);
    void GetNamedReferenceFromField(const DEFLD& defld, std::shared_ptr<NamedReference>& named_reference, int& field_symbol);
    CString GetNoteContent(std::shared_ptr<NamedReference> named_reference, const std::optional<CString>& operator_id, int field_symbol);
    void SetNote(std::shared_ptr<NamedReference> named_reference, const std::optional<CString>& operator_id,
        const CString& note_content, int field_symbol, bool add_paradata_event_if_applicable = true);
    void SetNote_pre80(std::shared_ptr<NamedReference> named_reference, const std::optional<CString>& operator_id,
        const CString& note_content, int field_symbol, bool add_paradata_event_if_applicable = true);
    std::tuple<CString, bool> EditNote(std::shared_ptr<NamedReference> named_reference,
        const std::optional<CString>& operator_id, int field_symbol, bool called_from_interface = true);
    std::tuple<CString, bool> EditNote_pre77(std::shared_ptr<NamedReference> named_reference,
        const std::optional<CString>& operator_id, int field_symbol, bool called_from_interface = true);
    void DeleteNote_pre80(DICX* pDicX, const Note& note);

    std::vector<std::wstring> GetOccurrenceLabels(const VART* pVarT, const CaseItemReference* case_item_reference = nullptr);

    // shows a dialog where notes can be reviewed; in a data entry application, the return value, if set,
    // indicates the field that the user wants to go to 
    std::shared_ptr<const CaseItemReference> ReviewNotes();


    // -----
    // FLOW_TODO methods and variables used by the new engine drivers
public:
    virtual bool UseNewDriver() const { return false; }
    bool UseOldDriver() const         { return !UseNewDriver(); }

private:
    [[noreturn]] void IssueLoadException(int message_number, ...);

    void LoadApplication();
    std::shared_ptr<EngineDictionary> LoadApplicationDictionary(std::shared_ptr<const CDataDict> dictionary,
                                                                std::optional<SymbolSubType> flow_subtype = std::nullopt);
    void LoadApplicationFlow(std::shared_ptr<CDEFormFile> form_file, SymbolSubType flow_subtype);
};


// process options & flags
#define Issamod         m_pEngineDriver->m_Issamod
#define ExMode          m_pEngineDriver->m_ExMode

// basic arrays
#define Dicxbase        m_pEngineDriver->m_Dicxbase
#define Secxbase        m_pEngineDriver->m_Secxbase
#define Varxbase        m_pEngineDriver->m_Varxbase

#define Chdesbase       m_pEngineDriver->m_Chdesbase
#define Chdesmxent      m_pEngineDriver->m_Chdesmxent
