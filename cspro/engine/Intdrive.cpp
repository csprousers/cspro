//------------------------------------------------------------------------
//
//  INTDRIVER.cpp         CSPRO Interprete driver
//
//  History:    Date       Author   Comment
//              ---------------------------
//              16 May 01   vc      Expanding for 3D driver
//              29 Oct 02   RHF     Expanding for tables events
//
//------------------------------------------------------------------------

#include "StandardSystemIncludes.h"
#include "INTERPRE.H"
#include "Engine.h"
#include "Ctab.h"
#include "FrequencyDriver.h"
#include "ImputationDriver.h"
#include "InterpreterMessageIssuer.h"
#include "ProgramControl.h"
#include "SelcaseManager.h"
#include "SyncListener.h"
#include <zPlatformO/PlatformInterface.h>
#include <zLogicO/SpecialFunction.h>
#include <zEngineO/AllSymbols.h>
#include <zEngineO/EngineAccessor.h>
#include <zEngineO/LoopStack.h>
#include <zEngineO/SaveArrayFile.h>
#include <zEngineO/Versioning.h>
#include <zEngineO/Nodes/Various.h>
#include <zEngineF/TraceHandler.h>
#include <zToolsO/Tools.h>
#include <zUtilO/CommonStore.h>
#include <zUtilO/MemoryHelpers.h>
#include <zJson/JsonNode.h>
#include <zHtml/VirtualFileMapping.h>
#include <zAction/Caller.h>
#include <ZBRIDGEO/npff.h>
#include <zMessageO/Messages.h>
#include <zCapiO/CapiQuestionManager.h>
#include <zFreqO/Frequency.h>
#include <zReportO/Pre77ReportManager.h>
#include <zSyncO/IBluetoothAdapter.h>
#include <zSyncO/SyncClient.h>
#include <zSyncO/SyncCredentialStore.h>
#include <zSyncO/SyncObexHandler.h>
#include <zSyncO/SyncServerConnectionFactory.h>

#ifdef WIN_DESKTOP
#include <zSyncO/BluetoothObexServer.h>
#include <zSyncO/WinBluetoothAdapter.h>
#endif


namespace
{
    // Engine error reporter for sync listener
    class EngineSyncListenerErrorReporter : public ISyncListenerErrorReporter
    {
    public:
        EngineSyncListenerErrorReporter(CEngineDriver* pEngineDriver)
            :   m_pEngineDriver(pEngineDriver)
        {
        }

        void OnError(int message_number, va_list parg) override
        {
            return m_pEngineDriver->GetSystemMessageIssuer().IssueVA(MessageType::Error, message_number, parg);
        }

        std::wstring Format(int message_number, va_list parg) override
        {
            return m_pEngineDriver->GetSystemMessageIssuer().GetFormattedMessageVA(message_number, parg);
        }

    private:
        CEngineDriver* m_pEngineDriver;
    };
}


//------------------------------------------------------------------------
//
// --- constructor/destructor/initialization
//
//------------------------------------------------------------------------
CIntDriver::CIntDriver(CEngineDriver& engine_driver)
    :   m_aFixedDimensions(ONE_BASED),
        m_pEngineDriver(&engine_driver),
        m_pEngineArea(m_pEngineDriver->getEngineAreaPtr()),
        m_engineData(m_pEngineDriver->m_engineData),
        m_logicByteCode(m_pEngineArea->GetLogicByteCode()),
        m_symbolTable(m_pEngineArea->GetSymbolTable()),
        m_pEngineDefines(&m_pEngineDriver->m_EngineDefines),
        m_pEngineSettings(&m_pEngineDriver->m_EngineSettings),
        m_usingLogicSettingsV0(true),
        m_currentEncodeType(Nodes::EncodeType::Html)
{
    // --- procedure being executed
    m_iProgType          = 0;
    m_iExLevel           = 0;
    m_iExSymbol          = 0;

    // --- execution flags
    m_iStopExec          = 0;
    m_bStopProc          = false;
    m_iSkipStmt          = 0;
    Disable3D_Driver();                                 // victor May 16, 01
    SetRequestIssued( false );                          // victor May 16, 01
// RHF COM Jul 03, 2002    EnableVector( false ); // RHF Jul 06, 2001

    // --- counters for excount and related functions
    m_iExOccur          = 0;
    m_iExGroup          = 0;                              // RHF Aug 17, 2000
    m_iExDim            = 0;                              // RHF Aug 17, 2000
    m_iExFixedDimensions = 0;                           // rcl, Jul 17, 2004

    // --- Runtime Stack for For Loops                  // RHC Jul 10, 2001
    m_iForStackNext = 0;

    // --- engine links
    m_pCsDriver       = NULL;           // Entifaz!!!   // victor May 16, 01

    m_bExecSpecFunc = false;

    m_bAllowMultipleRelation = false; // RHF Jul 16, 2002

    m_FieldSymbol = 0; // 20100708

#ifdef WIN_DESKTOP
    m_hLastDefaultKL = NULL; // 20120821
    m_hCurrentKL = NULL;
#endif

    m_pParadataDriver = new ParadataDriver(this);

    // Sync
#ifdef WIN_DESKTOP
    m_pBluetoothAdapter = WinBluetoothAdapter::create();
#else
    m_pBluetoothAdapter = PlatformInterface::GetInstance()->GetApplicationInterface()->CreateAndroidBluetoothAdapter();
#endif
    m_pSyncServerConnectionFactory = new SyncServerConnectionFactory(m_pBluetoothAdapter);
    m_pSyncClient = new SyncClient(GetDeviceId(), m_pSyncServerConnectionFactory);
    m_pSyncListener = new SyncListener(std::make_unique<EngineSyncListenerErrorReporter>(m_pEngineDriver));
    m_pSyncClient->setListener(m_pSyncListener);
    m_pSyncCredentialStore = new SyncCredentialStore();


    // add a routine to set VART objects
    m_engineData->engine_accessor->ea_SetVarTValueSetter(
        [&](Symbol* symbol, std::wstring value)
        {
            ASSERT(symbol->IsA(SymbolType::Variable));
            VART* pVarT = assert_cast<VART*>(symbol);

            if( !pVarT->IsUsed() )
                return;

            if( pVarT->GetLength() != 0 )
            {
                SO::MakeExactLength(value, pVarT->GetLength());

                VARX* pVarX = pVarT->GetVarX();
                TCHAR* pBuffer = (TCHAR*)svaraddr(pVarX);
                _tmemcpy(pBuffer, value.data(), pVarT->GetLength());
            }

            else // 20140325 for variable length strings
            {
                CString* pStr = pVarT->GetLogicStringPtr();
                ASSERT(pStr);
                *pStr = WS2CS(value);
            }
        });
}


CIntDriver::~CIntDriver()
{
    delete m_pSyncClient;
    delete m_pSyncServerConnectionFactory;
    delete m_pBluetoothAdapter;
    delete m_pSyncListener;
    delete m_pSyncCredentialStore;
    delete m_pParadataDriver;
}


void CIntDriver::StartApplication()
{
    const Application* application = m_pEngineDriver->m_pPifFile->GetApplication();
    const CNPifFile* pff = m_pEngineDriver->m_pPifFile;

    m_usingLogicSettingsV0 = ( application->GetLogicSettings().GetVersion() == LogicSettings::Version::V0 );

    // initialize the runtime for each dictionary
    for( DICT* pDicT : m_engineData->dictionaries_pre80 )
        pDicT->GetDicX()->StartRuntime();


    // start the paradata
    m_pParadataDriver->LogEngineEvent(ParadataEngineEvent::ApplicationStart);


    // set the initial language
    SetStartupLanguage();


    // load save arrays
    if( application->GetHasSaveArrays() )
    {
        // only issue read errors in batch applications
        std::shared_ptr<SystemMessageIssuer> system_message_issuer = ( application->GetEngineAppType() == EngineAppType::Batch ) ?
            m_pEngineDriver->GetSharedSystemMessageIssuer() : nullptr;

        SaveArrayFile save_array_file;
        save_array_file.ReadArrays(pff, m_engineData->arrays, system_message_issuer);
    }


    // set the initial file handler filenames
    for( LogicFile* logic_file : m_engineData->files_global_visibility )
        logic_file->SetFilename(CS2WS(pff->LookUpUsrDatFile(WS2CS(logic_file->GetName()))));


    // start the frequencies driver
    if( !m_engineData->frequencies.empty() )
        m_frequencyDriver = std::make_unique<FrequencyDriver>(*this);

    // start the imputation driver
    if( !m_engineData->imputations.empty() )
        m_imputationDriver = std::make_unique<ImputationDriver>(*this);

    // run OnStart events
    m_engineData->runtime_events_processor.RunEventsOnStart();
}


void CIntDriver::StopApplication()
{
    const Application* application = m_pEngineDriver->m_pPifFile->GetApplication();

    // run OnStop events
    m_engineData->runtime_events_processor.RunEventsOnStop();

    // stop the frequency driver
    m_frequencyDriver.reset();

    // stop the imputation driver
    m_imputationDriver.reset();


    // write save arrays
    if( application->GetHasSaveArrays() && application->GetUpdateSaveArrayFile() )
    {
        size_t cases_read = ( m_pEngineDriver->GetProcessSummary()->GetNumberLevels() > 0 ) ?
            m_pEngineDriver->GetProcessSummary()->GetCaseLevelsRead(0) : 0;

        SaveArrayFile save_array_file;
        save_array_file.WriteArrays(m_pEngineDriver->m_pPifFile, m_engineData->arrays,
                                    m_pEngineDriver->GetSharedSystemMessageIssuer(), cases_read, true);
    }


    // stop the paradata
    m_pParadataDriver->LogEngineEvent(ParadataEngineEvent::ApplicationStop);
}


void CIntDriver::PrepareForExportExec(int iSymbol, int iProgType)
{
    m_iSkipStmt = FALSE;

    int iLevel = 0;

    if( NPT(iSymbol)->IsA(SymbolType::Variable) )
    {
        iLevel = VPT(iSymbol)->GetLevel();
    }

    else if( NPT(iSymbol)->IsA(SymbolType::Group) )
    {
        iLevel = GPT(iSymbol)->GetLevel();
    }

#ifdef WIN_DESKTOP
    else if( NPT(iSymbol)->IsA(SymbolType::Crosstab) )
    {
        iLevel = XPT(iSymbol)->GetTableLevel();
    }
#endif

    else if( NPT(iSymbol)->IsA(SymbolType::Block) )
    {
        iLevel = GetSymbolEngineBlock(iSymbol).GetGroupT()->GetLevel();
    }

    else
    {
        ASSERT(0);
        return;
    }

    m_iProgType = iProgType;
    m_iExSymbol = iSymbol;
    m_iExLevel  = iLevel;
    m_iSkipStmt = FALSE;
    m_iStopExec = m_bStopProc;
}


CString CIntDriver::ProcName()
{
    if( m_iExSymbol <= 0 )
        return _T("Unknown");

    const Symbol* pSymbol = NPT(m_iExSymbol);
    CString csObjName = WS2CS(pSymbol->GetName());
    CString csExProcName;

    SymbolType eType = pSymbol->GetType();
    csprochar const* obj_type;

    if( eType == SymbolType::Pre80Dictionary ) {
        obj_type = _T("Dict");
        csExProcName.Format( _T("%s %s Level %d %s"), obj_type, csObjName.GetString(), m_iExLevel, GetProcTypeName(m_iProgType));
    }
    else {
        if( eType == SymbolType::Section ) {
            if( !is_digit(csObjName[0]) )
                obj_type = _T("Sect");
            else
                obj_type = _T("View");
        }
        else if( eType == SymbolType::Group ) {
            GROUPT*     pGroupT=(GROUPT*)pSymbol;
            obj_type = (pGroupT->GetGroupType() == GROUPT::Level) ? _T("Level") : _T("Group");
        }
        else if( eType == SymbolType::Crosstab ) {
            obj_type = _T("Table");
        }
        else if( eType == SymbolType::Block ) {
            obj_type = _T("Block");
        }
        else {
            ASSERT( eType == SymbolType::Variable ); // RHF Oct 29, 2002
            obj_type = _T("Var");
        }

        csExProcName.Format( _T("%s %s %s"), obj_type, csObjName.GetString(), GetProcTypeName(m_iProgType) );
    }

    return csExProcName;
}


CIntDriver::pDoubleFunction CIntDriver::m_pExFuncs[] =
{
/*-------------------+----------------------------------------------------*/
/*Op.code│ Function │      COMMANDS                                       */
/*-------------------+----------------------------------------------------*/
/*   0 */   &CIntDriver::exnumericconstant,
/*   1 */   &CIntDriver::exsvar,
/*   2 */   &CIntDriver::exmvar,
/*   3 */   &CIntDriver::excpt,
/*   4 */   &CIntDriver::exadd,
/*   5 */   &CIntDriver::exsub,
/*   6 */   &CIntDriver::exmult,
/*   7 */   &CIntDriver::exdiv,
/*   8 */   &CIntDriver::exmod,
/*   9 */   &CIntDriver::exminus,
/*  10 */   &CIntDriver::exexp,
/*  11 */   &CIntDriver::exor,
/*  12 */   &CIntDriver::exand,
/*  13 */   &CIntDriver::exnot,
/*  14 */   &CIntDriver::exeq,
/*  15 */   &CIntDriver::exne,
/*  16 */   &CIntDriver::exle,
/*  17 */   &CIntDriver::exlt,
/*  18 */   &CIntDriver::exge,
/*  19 */   &CIntDriver::exgt,
/*  20 */   &CIntDriver::exequ,
/*  21 */   &CIntDriver::exstringcompute,
/*  22 */   &CIntDriver::exworkvariable,
/*  23 */   &CIntDriver::exif,
/*  24 */   &CIntDriver::exwhile,
/*  25 */   &CIntDriver::exbox,
/*  26 */   &CIntDriver::exnoopAbort, // an old implementation of exstringliteral
/*  27 */   &CIntDriver::excharobj,
/*  28 */   &CIntDriver::exstring_eq, // =
/*  29 */   &CIntDriver::exstring_ne, // <>
/*  30 */   &CIntDriver::exstring_le, // <=
/*  31 */   &CIntDriver::exstring_lt, // <
/*  32 */   &CIntDriver::exstring_ge, // >=
/*  33 */   &CIntDriver::exstring_gt, // >
/*  34 */   &CIntDriver::excpttbl,
/*  35 */   &CIntDriver::exnoopAbort,
/*  36 */   &CIntDriver::exnoopAbort, // an old implementation of exarrayvar
/*  37 */   &CIntDriver::exuserfunctioncall,
/*  38 */   &CIntDriver::exnoopAbort, // an old implementation of exexit
/*  39 */   &CIntDriver::exnoopAbort, // exfor_view,

/*───────┬──────────┬-----------------------------------------------------*/
/*Op.code│ Function │      Data Entry COMMANDS                            */
/*───────┴──────────┴-----------------------------------------------------*/
/*  40 */   &CIntDriver::exskipto,
/*  41 */   &CIntDriver::exadvance,
/*  42 */   &CIntDriver::exreenter,
/*  43 */   &CIntDriver::exnoinput,
/*  44 */   &CIntDriver::exendsect,
/*  45 */   &CIntDriver::exendlevl,
/*  46 */   &CIntDriver::exenter,

/*───────┬──────────┬-----------------------------------------------------*/
/*Op.code│ Function │      Batch COMMANDS                                 */
/*───────┴──────────┴-----------------------------------------------------*/
/*  47 */   &CIntDriver::exskipcase,
/*  48 */   &CIntDriver::exnoopAbort, // previously exnowrite
/*  49 */   &CIntDriver::exstop,
/*  50 */   &CIntDriver::exnoopIgnore_numeric, // previously exWriteForm
/*  51 */   &CIntDriver::exctab,
/*  52 */   &CIntDriver::exnoopAbort, // the removed, batch-only, exfreq
/*  53 */   &CIntDriver::exbreak,
/*  54 */   &CIntDriver::exexport,

/*───────┬──────────┬-----------------------------------------------------*/
/*Op.code│ Function │      future COMMANDS                                */
/*───────┴──────────┴-----------------------------------------------------*/

/*  55 */   &CIntDriver::exset,                        // VC Feb 23, 95

/*───────┬──────────┬-----------------------------------------------------*/
/*Op.code│ Function │      NUMERIC FUNCTIONS                              */
/*───────┴──────────┴-----------------------------------------------------*/
/*  56 */   &CIntDriver::exvisualvalue,
/*  57 */   &CIntDriver::exhighlight,
/*  58 */   &CIntDriver::exsqrt,
/*  59 */   &CIntDriver::exex,
/*  60 */   &CIntDriver::exint,
/*  61 */   &CIntDriver::exlog,
/*  62 */   &CIntDriver::exseed,
/*  63 */   &CIntDriver::exrandom,
/*  64 */   &CIntDriver::exnoccurs,
/*  65 */   &CIntDriver::exsoccurs_pre80,
/*  66 */   &CIntDriver::exnoopAbort, // exvoccurs,
/*  67 */   &CIntDriver::excount,
/*  68 */   &CIntDriver::exsum,
/*  69 */   &CIntDriver::exavrge,
/*  70 */   &CIntDriver::exmin,
/*  71 */   &CIntDriver::exmax,
/*  72 */   &CIntDriver::exdisplay,
/*  73 */   &CIntDriver::exerrmsg,

/*───────┬──────────┬-----------------------------------------------------*/
/*Op.code│ Function │      ALPHA FUNCTIONS                                */
/*───────┴──────────┴-----------------------------------------------------*/
/*  74 */   &CIntDriver::exconcat,
/*  75 */   &CIntDriver::extonumber,
/*  76 */   &CIntDriver::expos_poschar, // pos
/*  77 */   &CIntDriver::excompare,
/*  78 */   &CIntDriver::exlength,
/*  79 */   &CIntDriver::exstrip,
/*  80 */   &CIntDriver::expos_poschar, // poschar
/*  81 */   &CIntDriver::exedit,

/*───────┬──────────┬-----------------------------------------------------*/
/*Op.code│ Function │      DATE FUNCTIONS                                 */
/*───────┴──────────┴-----------------------------------------------------*/
/*  82 */   &CIntDriver::excmcode,
/*  83 */   &CIntDriver::exsetlb_setub, // setub
/*  84 */   &CIntDriver::exsetlb_setub, // setlb
/*  85 */   &CIntDriver::exadjuba,
/*  86 */   &CIntDriver::exadjlba,
/*  87 */   &CIntDriver::exadjlbi,
/*  88 */   &CIntDriver::exadjubi,
/*  89 */   &CIntDriver::exnoopAbort, // exdatechk
/*  90 */   &CIntDriver::exsystime,
/*  91 */   &CIntDriver::exsysdate,

/*───────┬──────────┬-----------------------------------------------------*/
/*Op.code│ Function │      OTHER FUNCTIONS                                */
/*───────┴──────────┴-----------------------------------------------------*/
/*  92 */   &CIntDriver::exdemode,
/*  93 */   &CIntDriver::exspecial,
/*  94 */   &CIntDriver::exaccept,
/*  95 */   &CIntDriver::exclrcase,

/*───────┬──────────┬-----------------------------------------------------*/
/*Op.ccde│ Function │      TABLES/CROSSTAB FUNCTIONS                      */
/*───────┴──────────┴-----------------------------------------------------*/
/*  96 */   &CIntDriver::exxtab,
/*  97 */   &CIntDriver::extblcoord, // tblrow
/*  98 */   &CIntDriver::extblcoord, // tblcol
/*  99 */   &CIntDriver::extblcoord, // tbllay
/* 100 */   &CIntDriver::extblsum,
/* 101 */   &CIntDriver::extblmed,

/*───────┬──────────┬-----------------------------------------------------*/
/*Op.code│ Function │      INDEXED FILES FUNCTIONS                        */
/*───────┴──────────┴-----------------------------------------------------*/
/* 102 */   &CIntDriver::exfilename,
/* 103 */   &CIntDriver::exnoopAbort,           // an old implementation of exselcase
/* 104 */   &CIntDriver::exnoopAbort,           // previously a locked version of exselcase
/* 105 */   &CIntDriver::exloadcase,
/* 106 */   &CIntDriver::exnoopAbort,           // previously a locked version of exloadcase
/* 107 */   &CIntDriver::exretrieve,
/* 108 */   &CIntDriver::exnoopAbort,           // previously a locked version of exretrieve
/* 109 */   &CIntDriver::exwritecase,
/* 110 */   &CIntDriver::exdelcase,
/* 111 */   &CIntDriver::exfind_locate,         // find
/* 112 */   &CIntDriver::exkey,                 // key
/* 113 */   &CIntDriver::exopen,
/* 114 */   &CIntDriver::exclose,
/* 115 */   &CIntDriver::exfind_locate,         // locate
/* 116 */   &CIntDriver::exnoopAbort,           // previously exexec
/* 117 */   &CIntDriver::exnoopAbort,           // an old implementation of exsysparm
/* 118 */   &CIntDriver::exnoopIgnore_numeric,  // previously exioerror
/* 119 */   &CIntDriver::exnoopAbort,           // previously exwriteacl
/* 120 */   &CIntDriver::exnoopIgnore_numeric,  // previously exdemenu
/* 121 */   &CIntDriver::exsetattr,
/* 122 */   &CIntDriver::exnoopAbort,           // previously set file

/*───────┬──────────┬-----------------------------------------------------*/
/*Op.code│ Function │      Data Entry COMMANDS - extension                */
/*───────┴──────────┴-----------------------------------------------------*/
/* 123 */   &CIntDriver::exfor_dict,

/*───────┬──────────┬-----------------------------------------------------*/
/*Op.code│ Function │      NUMERIC FUNCTIONS   - extension                */
/*───────┴──────────┴-----------------------------------------------------*/
/* 124 */   &CIntDriver::exnmembers,
/* 125 */   &CIntDriver::exnoopAbort,  // previously exset_output
/* 126 */   &CIntDriver::exnoopAbort,  // previously exrecord
/* 127 */   &CIntDriver::exvaluelimit, // minvalue
/* 128 */   &CIntDriver::exvaluelimit, // maxvalue
/* 129 */   &CIntDriver::exfor_group,
/* 130 */   &CIntDriver::exnoopAbort,  // previously extbd
/* 131 */   NULL,
/* 132 */   &CIntDriver::exfucall,
/* 133 */   &CIntDriver::exin,         // RHC Oct 16, 2000
/* 134 */   &CIntDriver::exdo,         // RHC Oct 16, 2000
/* 135 */   &CIntDriver::eximpute,     // RHF Oct 25, 2000
/* 136 */   &CIntDriver::exfncurocc,   // RHC Oct 16, 2000
/* 137 */   &CIntDriver::exfntotocc,   // RHC Oct 16, 2000
/* 138 */   &CIntDriver::exupdate,     // RHF Nov 17, 2000
/* 139 */   &CIntDriver::exwritemsg,   // RHF Dec 16, 2000
/* 140 */   &CIntDriver::exnoopAbort,  // an old implementation of exispartial [original: RHF Mar 06, 2001]
/* 141 */   &CIntDriver::exfor_relation,
/* 142 */   NULL,
/* 143 */   &CIntDriver::exgetbuffer,
/* 144 */   &CIntDriver::exinsert_delete,  // Chirag, Jul 22, 2002
/* 145 */   &CIntDriver::exinsert_delete,  // Chirag, Sep 11, 2002
/* 146 */   &CIntDriver::exsort,           // Chirag, Sep 11, 2002
/* 147 */   &CIntDriver::exgetlabel,       // RHF Aug 25, 2000
/* 148 */   &CIntDriver::exgetlabel,       // getsymbol RHF Mar 23, 2001
/* 149 */   &CIntDriver::exnoopAbort,      // an old implementation of exgetnote  [original: RHF Nov 19, 2002]
/* 150 */   &CIntDriver::exnoopAbort,      // an old implementation of exeditnote [original: RHF Nov 19, 2002]
/* 151 */   &CIntDriver::exnoopAbort,      // an old implementation of exputnote  [original: RHF Nov 19, 2002]
/* 152 */   &CIntDriver::exmaketext,       // RHF Jun 08, 2001

/* 153 */   &CIntDriver::exmoveto,         // RHF Dec 09, 2003
/* 154 */   &CIntDriver::exnoopAbort,      // an old implementation of exsavepartial [original: RHF Dec 01, 2003]
/* 155 */   &CIntDriver::exgetoperatorid,  // RHF Dec 03, 2003
/* 156 */   &CIntDriver::exfornext,        // RHC Sep 04, 2000
/* 157 */   &CIntDriver::exforbreak,       // RHC Sep 04, 2000
/* 158 */   &CIntDriver::exsetfile,
/* 159 */   &CIntDriver::exmaxocc_pre80,
/* 160 */   &CIntDriver::exinvalueset,
/* 161 */   &CIntDriver::exsetvalueset,    // RHF Aug 28, 2002

// RHF INIC Oct 15, 2004
/* 162 */   &CIntDriver::exfilecreate,
/* 163 */   &CIntDriver::exfileexist,
/* 164 */   &CIntDriver::exfiledelete,
/* 165 */   &CIntDriver::exfilecopy,
/* 166 */   &CIntDriver::exfilerename,
/* 167 */   &CIntDriver::exfilesize,
/* 168 */   &CIntDriver::exfileconcat,
/* 169 */   &CIntDriver::exfileread,
/* 170 */   &CIntDriver::exfilewrite,
// RHF END Oct 15, 2004

/* 171 */   &CIntDriver::ExExecSystem,

/* 172 */   &CIntDriver::exnoopAbort,        // an old implementation of exshow
/* 173 */   &CIntDriver::exshowlist,

/* 174 */   &CIntDriver::extolower_toupper,  // GHM 20091202 tolower
/* 175 */   &CIntDriver::extolower_toupper,  // GHM 20091202 toupper
/* 176 */   &CIntDriver::excountvalid,       // GHM 20091202
/* 177 */   &CIntDriver::exnoopIgnore_string,// GHM 20091208 previously itemlist
/* 178 */   &CIntDriver::exswap,             // GHM 20100105
/* 179 */   &CIntDriver::exdatediff,         // GHM 20100119
/* 180 */   &CIntDriver::exdeckarray,        // GHM 20100119 putdeck
/* 181 */   &CIntDriver::exdeckarray,        // GHM 20100119 getdeck
/* 182 */   &CIntDriver::exgetlanguage,      // GHM 20100309
/* 183 */   &CIntDriver::exsetlanguage,      // GHM 20100309
/* 184 */   &CIntDriver::exendcase,          // GHM 20100310
/* 185 */   &CIntDriver::exuserbar,          // GHM 20100414
/* 186 */   &CIntDriver::exmessageoverrides, // GHM 20100518
/* 187 */   &CIntDriver::extrace,            // GHM 20100518
/* 188 */   &CIntDriver::exsetvaluesets,     // GHM 20100523
/* 189 */   &CIntDriver::ExExecPFF,          // GHM 20100601
/* 190 */   &CIntDriver::exseek,             // GHM 20100602
/* 191 */   &CIntDriver::exgetcapturetype,   // GHM 20100608
/* 192 */   &CIntDriver::exsetcapturetype,   // GHM 20100608
/* 193 */   &CIntDriver::exsetfont,          // GHM 20100618
/* 194 */   &CIntDriver::exorientation,      // GHM 20100618 getorientation
/* 195 */   &CIntDriver::exorientation,      // GHM 20100618 setorientation
/* 196 */   &CIntDriver::expathname,         // GHM 20110107
/* 197 */   &CIntDriver::exgps,              // GHM 20110223
/* 198 */   &CIntDriver::exlowhigh,          // GHM 20110301 low
/* 199 */   &CIntDriver::exlowhigh,          // GHM 20110301 high
/* 200 */   &CIntDriver::exgetrecord,        // GHM 20110302
/* 201 */   &CIntDriver::exsetcapturepos,    // GHM 20110502
/* 202 */   &CIntDriver::exabs,              // GHM 20110721
/* 203 */   &CIntDriver::exrandomin,         // GHM 20110721
/* 204 */   &CIntDriver::exrandomizevs,      // GHM 20110811
/* 205 */   &CIntDriver::exgetusername,      // GHM 20111028
/* 206 */   &CIntDriver::exfileempty,        // GHM 20120627
/* 207 */   &CIntDriver::exchangekeyboard,   // GHM 20120820
/* 208 */   &CIntDriver::exsetoutput,        // GHM 20121126
/* 209 */   &CIntDriver::exseekMinMax,       // GHM 20130119
/* 210 */   &CIntDriver::exseekMinMax,       // GHM 20130119
/* 211 */   &CIntDriver::exdateadd,          // GHM 20130225
/* 212 */   &CIntDriver::exdatevalid,        // GHM 20130703
/* 213 */   &CIntDriver::exgetos,            // GHM 20131217
/* 214 */   &CIntDriver::exgetocclabel,      // GHM 20140226
/* 215 */   &CIntDriver::exfreealphamem,     // GHM 20140228
/* 216 */   &CIntDriver::exsetvalue,         // GHM 20140228
/* 217 */   &CIntDriver::exgetvalue,         // GHM 20140422
/* 218 */   &CIntDriver::exgetvaluealpha,    // GHM 20140422
/* 219 */   &CIntDriver::exnoopAbort,        // GHM 20140423 an old implementation of exshowarray
/* 220 */   &CIntDriver::exsetocclabel,      // GHM 20141006
/* 221 */   &CIntDriver::exshowocc,          // GHM 20141015 showocc
/* 222 */   &CIntDriver::exshowocc,          // GHM 20141015 hideocc
/* 223 */   &CIntDriver::exgetdeviceid,      // GHM 20141023
/* 224 */   &CIntDriver::exdirexist,         // GHM 20141024
/* 225 */   &CIntDriver::exdircreate,        // GHM 20141024
/* 226 */   &CIntDriver::exnoopAbort,        // GHM 20141024 previously sync
/* 227 */   &CIntDriver::exlistvar,          // GHM 20141106
/* 228 */   &CIntDriver::exdirlist,          // GHM 20141107
/* 229 */   &CIntDriver::exsysparm,          // GHM 20141217
/* 230 */   &CIntDriver::exconnection,       // GHM 20150421
/* 231 */   &CIntDriver::exprompt,           // GHM 20150422
/* 232 */   &CIntDriver::exgetimage,         // GHM 20150809
/* 233 */   &CIntDriver::exround,            // GHM 20150821
/* 234 */   &CIntDriver::exnoopAbort,        // GHM 20151130 an old implementation of exuuid ... now a publishdate placeholder
/* 235 */   &CIntDriver::exsavepartial,      // GHM 20151216
/* 236 */   &CIntDriver::exsyncconnect,
/* 237 */   &CIntDriver::exsyncdisconnect,
/* 238 */   &CIntDriver::exsyncdata,
/* 239 */   &CIntDriver::exsyncfile,
/* 240 */   &CIntDriver::exsyncserver,
/* 241 */   &CIntDriver::exsavesetting,
/* 242 */   &CIntDriver::exloadsetting,
/* 243 */   &CIntDriver::exgetcaselabel,
/* 244 */   &CIntDriver::exsetcaselabel,
/* 245 */   &CIntDriver::exispartial,
/* 246 */   &CIntDriver::exsetoperatorid,
/* 247 */   &CIntDriver::exgetnote,
/* 248 */   &CIntDriver::exeditnote,
/* 249 */   &CIntDriver::exputnote,
/* 250 */   &CIntDriver::exisverified,
/* 251 */   &CIntDriver::exforcase,
/* 252 */   &CIntDriver::extimestamp,
/* 253 */   &CIntDriver::exkeylist,
/* 254 */   &CIntDriver::exdiagnostics,
/* 255 */   &CIntDriver::excompress,
/* 256 */   &CIntDriver::exdecompress,
/* 257 */   &CIntDriver::exask,
/* 258 */   &CIntDriver::excountcases,
/* 259 */   &CIntDriver::exgetproperty,
/* 260 */   &CIntDriver::exsetproperty,
/* 261 */   &CIntDriver::exlogtext,
/* 262 */   &CIntDriver::exwarning,
/* 263 */   &CIntDriver::extr,
/* 264 */   &CIntDriver::exuuid,
/* 265 */   &CIntDriver::exparadata,
/* 266 */   &CIntDriver::exsqlquery,
/* 267 */   &CIntDriver::expre77_report,
/* 268 */   &CIntDriver::expre77_setreportdata,
/* 269 */   &CIntDriver::exshow,
/* 270 */   &CIntDriver::exshowarray,
/* 271 */   &CIntDriver::exselcase,
/* 272 */   &CIntDriver::extimestring,
/* 273 */   &CIntDriver::exstringliteral,
/* 274 */   &CIntDriver::exsymbolreset,
/* 275 */   &CIntDriver::exdecryptstring,
/* 276 */   &CIntDriver::exdirdelete,
/* 277 */   &CIntDriver::exarrayvar,
/* 278 */   &CIntDriver::extvar,
/* 279 */   &CIntDriver::exexit,
/* 280 */   &CIntDriver::exgetbluetoothname,
/* 281 */   &CIntDriver::exregexmatch,
/* 282 */   nullptr, // BLOCK_CODE
/* 283 */   &CIntDriver::exgetvaluelabel,
/* 284 */   &CIntDriver::exarrayclear,
/* 285 */   &CIntDriver::exarraylength,
/* 286 */   &CIntDriver::exmapshow,
/* 287 */   &CIntDriver::exmaphide,
/* 288 */   &CIntDriver::exmapaddmarker,
/* 289 */   &CIntDriver::exmapsetmarkerimage,
/* 290 */   &CIntDriver::exmapsetmarkertext,
/* 291 */   &CIntDriver::exmapsetmarkeronclickonclickinfo, // Map.setMarkerOnClick
/* 292 */   &CIntDriver::exmapsetmarkeronclickonclickinfo, // Map.setMarkerOnClickInfo
/* 293 */   &CIntDriver::exmapsetmarkerdescription,
/* 294 */   &CIntDriver::exmapsetmarkerondrag,
/* 295 */   &CIntDriver::exmapsetmarkerlocation,
/* 296 */   &CIntDriver::exmapgetmarkerlatitudelongitude,  // Map.getMarkerLatitude
/* 297 */   &CIntDriver::exmapremovemarker,
/* 298 */   &CIntDriver::exmapsetonclick,
/* 299 */   &CIntDriver::exmapshowcurrentlocation,
/* 300 */   &CIntDriver::exmapaddtextbutton,
/* 301 */   &CIntDriver::exmapaddimagebutton,
/* 302 */   &CIntDriver::exmapremovebutton,
/* 303 */   &CIntDriver::exmapsetbasemap,
/* 304 */   &CIntDriver::exmapsettitle,
/* 305 */   &CIntDriver::exmapzoomto,
/* 306 */   &CIntDriver::exlistadd,
/* 307 */   &CIntDriver::exlistclear,
/* 308 */   &CIntDriver::exlistinsert,
/* 309 */   &CIntDriver::exlistlength,
/* 310 */   &CIntDriver::exlistremove,
/* 311 */   &CIntDriver::exlistseek,
/* 312 */   &CIntDriver::exlistshow,
/* 313 */   &CIntDriver::exlistcompute,
/* 314 */   &CIntDriver::exvaluesetadd,
/* 315 */   &CIntDriver::exvaluesetclear,
/* 316 */   &CIntDriver::exvaluesetremove,
/* 317 */   &CIntDriver::exvaluesetshow,
/* 318 */   &CIntDriver::exvaluesetcompute,
/* 319 */   &CIntDriver::exvariablevalue,
/* 320 */   &CIntDriver::exmap_clear_clearButtons_clearGeometry_clearMarkers, // Map.clearMarkers
/* 321 */   &CIntDriver::exmap_clear_clearButtons_clearGeometry_clearMarkers, // Map.clearButtons
/* 322 */   &CIntDriver::exmapgetlastclicklatitudelongitude, // Map.getLastClickLatitude
/* 323 */   &CIntDriver::exmapgetlastclicklatitudelongitude, // Map.getLastClickLongitude
/* 324 */   &CIntDriver::exmapgetmarkerlatitudelongitude,    // Map.getMarkerLongitude
/* 325 */   &CIntDriver::expathconcat,
/* 326 */   &CIntDriver::exview,
/* 327 */   &CIntDriver::expffexec,
/* 328 */   &CIntDriver::expffgetproperty,
/* 329 */   &CIntDriver::expffload,
/* 330 */   &CIntDriver::expffsave,
/* 331 */   &CIntDriver::expffsetproperty,
/* 332 */   &CIntDriver::exvaluesetlength,
/* 333 */   &CIntDriver::exischecked,
/* 334 */   &CIntDriver::exprotect,
/* 335 */   &CIntDriver::exwhen,
/* 336 */   &CIntDriver::exsyncapp,
/* 337 */   &CIntDriver::exfiletime,
/* 338 */   &CIntDriver::exrecode,
/* 339 */   &CIntDriver::exforcase,
/* 340 */   &CIntDriver::exselcase,
/* 341 */   &CIntDriver::excountcases,
/* 342 */   &CIntDriver::exkeylist,
/* 343 */   &CIntDriver::exBarcode_read,
/* 344 */   &CIntDriver::exhash,
/* 345 */   &CIntDriver::exsyncmessage,
/* 346 */   &CIntDriver::exsystemapp_clear,
/* 347 */   &CIntDriver::exsystemapp_setargument,
/* 348 */   &CIntDriver::exsystemapp_getresult,
/* 349 */   &CIntDriver::exsystemapp_exec,
/* 350 */   &CIntDriver::exstartswith,
/* 351 */   &CIntDriver::expffcompute,
/* 352 */   &CIntDriver::exAudio_clear,
/* 353 */   &CIntDriver::exAudio_concat,
/* 354 */   &CIntDriver::exAudio_load,
/* 355 */   &CIntDriver::exAudio_play,
/* 356 */   &CIntDriver::exAudio_save,
/* 357 */   &CIntDriver::exAudio_stop,
/* 358 */   &CIntDriver::exAudio_record,
/* 359 */   &CIntDriver::exAudio_recordInteractive,
/* 360 */   &CIntDriver::exAudio_compute,
/* 361 */   &CIntDriver::exencode,
/* 362 */   &CIntDriver::exlistsort,
/* 363 */   &CIntDriver::exlistremoveduplicates,
/* 364 */   &CIntDriver::exlistremovein,
/* 365 */   &CIntDriver::expathconcat,
/* 366 */   &CIntDriver::expathgetdirectoryname,
/* 367 */   &CIntDriver::expathgetextension,
/* 368 */   &CIntDriver::expathgetfilename,
/* 369 */   &CIntDriver::expathgetfilenamewithoutextension,
/* 370 */   &CIntDriver::exsyncparadata,
/* 371 */   &CIntDriver::exhashmapvar,
/* 372 */   &CIntDriver::exhashmapcompute,
/* 373 */   &CIntDriver::exhashmapclear,
/* 374 */   &CIntDriver::exhashmapcontains,
/* 375 */   &CIntDriver::exhashmaplength,
/* 376 */   &CIntDriver::exhashmapremove,
/* 377 */   &CIntDriver::exhashmapgetkeys,
/* 378 */   &CIntDriver::exAudio_length,
/* 379 */   &CIntDriver::exvaluesetsort,
/* 380 */   &CIntDriver::exreplace,
/* 381 */   &CIntDriver::exinc,
/* 382 */   &CIntDriver::exuniverse,
/* 383 */   &CIntDriver::exfrequnnamed,
/* 384 */   &CIntDriver::exfreqclear,
/* 385 */   &CIntDriver::exfreqsave,
/* 386 */   &CIntDriver::exfreqtally,
/* 387 */   &CIntDriver::exFreq_view,
/* 388 */   &CIntDriver::exfreqvar,
/* 389 */   &CIntDriver::exfreqcompute,
/* 390 */   &CIntDriver::exworkstring,
/* 391 */   &CIntDriver::exmaxocc,
/* 392 */   &CIntDriver::exsoccurs,
/* 393 */   &CIntDriver::exDataAccessValidityCheck,
/* 394 */   &CIntDriver::exdictcompute,
/* 395 */   &CIntDriver::exkey, // currentkey
/* 396 */   &CIntDriver::exImage_compute,
/* 397 */   &CIntDriver::exImage_captureSignature_takePhoto, // Image.captureSignature
/* 398 */   &CIntDriver::exImage_clear,
/* 399 */   &CIntDriver::exImage_width_height, // Image.height
/* 400 */   &CIntDriver::exImage_load,
/* 401 */   &CIntDriver::exImage_resample,
/* 402 */   &CIntDriver::exImage_save,
/* 403 */   &CIntDriver::exImage_captureSignature_takePhoto, // Image.takePhoto
/* 404 */   &CIntDriver::exImage_view,
/* 405 */   &CIntDriver::exImage_width_height, // Image.width
/* 406 */   &CIntDriver::exDocument_compute,
/* 407 */   &CIntDriver::exDocument_clear,
/* 408 */   &CIntDriver::exDocument_load,
/* 409 */   &CIntDriver::exDocument_save,
/* 410 */   &CIntDriver::exDocument_view,
/* 411 */   &CIntDriver::exGeometry_compute,
/* 412 */   &CIntDriver::exGeometry_clear,
/* 413 */   &CIntDriver::exGeometry_load,
/* 414 */   &CIntDriver::exGeometry_save,
/* 415 */   &CIntDriver::exmapaddgeometry,
/* 416 */   &CIntDriver::exmapremovegeometry,
/* 417 */   &CIntDriver::exmap_clear_clearButtons_clearGeometry_clearMarkers, // Map.clearGeometry
/* 418 */   &CIntDriver::exGeometry_tracePolygon_walkPolygon, // Geometry.tracePolygon
/* 419 */   &CIntDriver::exGeometry_tracePolygon_walkPolygon, // Geometry.walkPolygon
/* 420 */   &CIntDriver::exGeometry_area_perimeter, // Geometry.area
/* 421 */   &CIntDriver::exGeometry_area_perimeter, // Geometry.perimeter
/* 422 */   &CIntDriver::exGeometry_minLatitude_maxLatitude_minLongitude_maxLongitude, // Geometry.minLatitude
/* 423 */   &CIntDriver::exGeometry_minLatitude_maxLatitude_minLongitude_maxLongitude, // Geometry.maxLatitude
/* 424 */   &CIntDriver::exGeometry_minLatitude_maxLatitude_minLongitude_maxLongitude, // Geometry.minLongitude
/* 425 */   &CIntDriver::exGeometry_minLatitude_maxLatitude_minLongitude_maxLongitude, // Geometry.maxLongitude
/* 426 */   &CIntDriver::exGeometry_getProperty,
/* 427 */   &CIntDriver::exGeometry_setProperty,
/* 428 */   &CIntDriver::exinadvance,
/* 429 */   &CIntDriver::exmapsavesnapshot,
/* 430 */   &CIntDriver::exsynctime,
/* 431 */   &CIntDriver::exhtmldialog,
/* 432 */   &CIntDriver::expathgetrelativepath,
/* 433 */   &CIntDriver::expathselectfile,
/* 434 */   &CIntDriver::exinvoke,
/* 435 */   &CIntDriver::exreport_save,
/* 436 */   &CIntDriver::exReport_view,
/* 437 */   &CIntDriver::exreport_write,
/* 438 */   &CIntDriver::exsetbluetoothname,
/* 439 */   &CIntDriver::expersistentsymbolreset,
/* 440 */   &CIntDriver::exSymbol_getJson_getValueJson, // symbol.getJson
/* 441 */   &CIntDriver::exSymbol_getJson_getValueJson, // symbol.getValueJson
/* 442 */   &CIntDriver::exSymbol_updateValueFromJson,
/* 443 */   &CIntDriver::exBarcode_createQRCode, // Barcode.createQRCode + Image.createQRCode
/* 444 */   &CIntDriver::exScopeChange,
/* 445 */   &CIntDriver::exdictaccess,
/* 446 */   &CIntDriver::exworkstringcompute,
/* 447 */   &CIntDriver::exActionInvoker,
/* 448 */   &CIntDriver::exSymbol_getName,
/* 449 */   &CIntDriver::exSymbol_getLabel,
/* 450 */   &CIntDriver::exmap_clear_clearButtons_clearGeometry_clearMarkers, // Map.clear
/* 451 */   &CIntDriver::exItem_hasValue_isValid, // Item.hasValue
/* 452 */   &CIntDriver::exItem_getValueLabel,
/* 453 */   &CIntDriver::exItem_hasValue_isValid, // Item.isValid
/* 454 */   &CIntDriver::excompareNoCase,
/* 455 */   &CIntDriver::exCase_view,


            // placeholders to allow new logic functions to be added to an existing serialization
            // iteration without causing crashes to old builds at the same iteration
            &CIntDriver::exnoopAbortPlaceholderForFutureFunction,
            &CIntDriver::exnoopAbortPlaceholderForFutureFunction,
            &CIntDriver::exnoopAbortPlaceholderForFutureFunction,
            &CIntDriver::exnoopAbortPlaceholderForFutureFunction,
            &CIntDriver::exnoopAbortPlaceholderForFutureFunction,
            &CIntDriver::exnoopAbortPlaceholderForFutureFunction,
            &CIntDriver::exnoopAbortPlaceholderForFutureFunction,
            &CIntDriver::exnoopAbortPlaceholderForFutureFunction,
            &CIntDriver::exnoopAbortPlaceholderForFutureFunction,
            &CIntDriver::exnoopAbortPlaceholderForFutureFunction,
};


bool CIntDriver::ExecuteSymbolProcs(const Symbol& symbol, ProcType proc_type)
{
    bool bRequestIssued = false;

    if( m_bStopProc )
        return bRequestIssued;

    // If there is a field (the first field) before a roster and we arrive to this field
    // from a previous endsect. m_iSkipStmt remains in 1 when the field doesn't have proc.
    // So the PreProc of the Roster is not executed (see DeSetNextField GroupCompletion
    // is not called when m_iSkipStmt is 1.!!!
    m_iSkipStmt = FALSE;

    const RunnableSymbol& runnable_symbol = dynamic_cast<const RunnableSymbol&>(symbol);
    int program_index = runnable_symbol.GetProcIndex(proc_type);

    if( program_index != -1 )
    {
        // setup execution parameters
        m_iProgType = static_cast<int>(proc_type);
        m_iExSymbol = symbol.GetSymbolIndex();
        m_iExLevel = SymbolCalculator::GetLevelNumber_base1(symbol);

        m_iSkipStmt = FALSE;
        m_iStopExec = m_bStopProc;

        // reset RequestIssued
        SetRequestIssued(false);

        // update the trace window
        if( m_traceHandler != nullptr )
        {
            m_traceHandler->Output(FormatTextCS2WS(_T("Entering %s (%s)..."), symbol.GetName().c_str(), GetProcTypeName(proc_type)),
                                   TraceHandler::OutputType::SystemText);
        }


        try
        {
            bRequestIssued = ExecuteProgramStatements(program_index);
        }

        // an exit statement terminates the precedure
        catch( const ExitProgramControlException& ) { }

        
        if( m_traceHandler != nullptr )
        {
            m_traceHandler->Output(FormatTextCS2WS(_T("Exiting %s (%s)..."), symbol.GetName().c_str(), GetProcTypeName(proc_type)),
                                   TraceHandler::OutputType::SystemText);
        }
    }

    return bRequestIssued;
}


bool CIntDriver::ExecuteProcLevel(int iLevel, ProcType proc_type)
{
    // --- 'bCheckOccs' added to process Level-0 procs  // victor May 09, 00
    const GROUPT* pGroupT = m_pEngineDriver->GetGroupTRootInProcess()->GetLevelGPT(iLevel);

    bool bCheckOccs = ( iLevel > 0 );

    return ExecuteProcGroup(pGroupT->GetSymbolIndex(), (ProcType)proc_type, bCheckOccs);
}


bool CIntDriver::ExecuteProcGroup(int iSymGroup, ProcType proc_type, bool bCheckOccs)
{
    // --- 'bCheckOccs' added to process Level-0 procs  // victor May 09, 00

    // look at occurrences
    const GROUPT* pGroupT = GPT(iSymGroup);

    if( Issamod != ModuleType::Entry && bCheckOccs )
    {
        // batch, empty Group: no execution of Group-Proc
        int iCurOccur = pGroupT->GetTotalOccurrences(); // victor May 25, 00

        // to use when working with long prog-strip         // victor Mar 14, 01
        if( iCurOccur < 1 && !m_pEngineSettings->HasSkipStruc() )
            return false;
    }

    return ExecuteSymbolProcs(*pGroupT, proc_type);
}


bool CIntDriver::ExecuteProcVar(int iSymVar, ProcType proc_type)
{
    bool bRequestIssued = false;

    // look at occurrences
    const VART* pVarT = VPT(iSymVar);

    if( Issamod != ModuleType::Entry )
    {
        if( pVarT->GetSubType() == SymbolSubType::Input )
        {
            // batch, input var in empty Group: no execution of Var-proc
            int iCurOccur = pVarT->GetOwnerGPT()->GetCurrentExOccurrence(); // victor May 25, 00

            if( iCurOccur < 1 )
                return bRequestIssued;
        }
    }

    bRequestIssued = ExecuteSymbolProcs(*pVarT, proc_type);

    // RHF INIC Aug 19, 2003
    if( Issamod == ModuleType::Entry && proc_type == ProcType::OnFocus )
    {
        RunGlobalOnFocus(iSymVar);
    }
    // RHF END Aug 19, 2003

    return bRequestIssued;
}


bool CIntDriver::ExecuteProcBlock(int iSymBlock, ProcType proc_type)
{
    return ExecuteSymbolProcs(NPT_Ref(iSymBlock), proc_type);
}


void CIntDriver::ExecuteProcTable(int iCtab, ProcType proc_type)
{
    // the implicit will be executed in other piece of code
    if( proc_type != ProcType::ImplicitCalc )
        ExecuteSymbolProcs(NPT_Ref(iCtab), proc_type);
}


bool CIntDriver::ExecuteProgramStatements(int program_index)
{
    // execute a block of statements
    while( program_index >= 0 && !m_iStopExec )
    {
        const auto& statement_node = GetNode<ST_NODE>(program_index);

        try
        {
            (this->*m_pExFuncs[statement_node.st_code])(program_index);
        }

        catch( LogicStackSaver& logic_stack_saver )
        {
            // add the next statement to the logic stack
            logic_stack_saver.PushStatement(statement_node.next_st);
            throw;
        }

        // advance to the next statement
        program_index = statement_node.next_st;

        // update the execution flags
        m_iStopExec = ( m_iSkipStmt || m_bStopProc );

        if( GetRequestIssued() )
            return true;
    }

    // no request issued
    return false;
}

void CIntDriver::ResetSymbol(Symbol& symbol, int initialize_value/* = -1*/)
{
    bool has_initialize_value = ( initialize_value >= 0 );

    // if there is no initialize value, we can simply reset most symbols
    if( !has_initialize_value && !symbol.IsOneOf(SymbolType::NamedFrequency,
                                                 SymbolType::Variable) )
    {
        symbol.Reset();
    }

    // if there is an initialize value, we can simply execute the node for most symbols
    else if( has_initialize_value && !symbol.IsOneOf(SymbolType::Array,
                                                     SymbolType::NamedFrequency,
                                                     SymbolType::Variable,
                                                     SymbolType::WorkString,
                                                     SymbolType::WorkVariable) )
    {
        ASSERT(!symbol.IsA(SymbolType::Dictionary));
        evalexpr(initialize_value);
    }


    // special processing for arrays
    else if( symbol.IsA(SymbolType::Array) )
    {
        ASSERT(has_initialize_value);

        LogicArray& logic_array = assert_cast<LogicArray&>(symbol);
        logic_array.Reset();

        const Nodes::List* array_values = &GetListNode(initialize_value);
        std::unique_ptr<int[]> pre80_array_values_node;

        if( Versioning::PredatesCompiledLogicVersion(Serializer::Iteration_8_0_000_1) )
        {
            auto old_node = (const int*)array_values;
            int repeat_values = old_node[0];
            int number_values = old_node[1];
            pre80_array_values_node = std::make_unique<int[]>(number_values + 2);
            pre80_array_values_node[0] = number_values + 1;
            pre80_array_values_node[1] = repeat_values;
            memcpy(pre80_array_values_node.get() + 2, old_node + 2, number_values * sizeof(int));
            array_values = (const Nodes::List*)pre80_array_values_node.get();
        }

        ASSERT(array_values->number_elements >= 2);

        bool repeat_values = ( array_values->elements[0] == 1 );

        if( logic_array.IsNumeric() )
        {
            std::vector<double> initial_values;

            for( int i = 1; i < array_values->number_elements; ++i )
                initial_values.emplace_back(evalexpr(array_values->elements[i]));

            logic_array.SetInitialValues(std::move(initial_values), repeat_values);
        }

        else
        {
            std::vector<std::wstring> initial_values;

            for( int i = 1; i < array_values->number_elements; ++i )
                initial_values.emplace_back(EvalAlphaExpr(array_values->elements[i]));

            logic_array.SetInitialValues(std::move(initial_values), repeat_values);
        }
    }


    // special processing for named frequencies
    else if( symbol.IsA(SymbolType::NamedFrequency) )
    {
        const NamedFrequency& named_frequency = assert_cast<const NamedFrequency&>(symbol);
        m_frequencyDriver->ResetFrequency(named_frequency.GetFrequencyIndex(), initialize_value);
    }


    // special processing for variables
    else if( symbol.IsA(SymbolType::Variable) )
    {
        VART* pVarT = assert_cast<VART*>(&symbol);
        CString value;

        if( initialize_value >= 0 )
            value = EvalAlphaExpr<CString>(initialize_value);

        CString* logic_string = pVarT->GetLogicStringPtr();

        if( logic_string != nullptr )
        {
            *logic_string = value;
        }

        else
        {
            value = CIMSAString::MakeExactLength(value, pVarT->GetLength());

            TCHAR* buffer = (TCHAR*)svaraddr(pVarT->GetVarX());
            _tmemcpy(buffer, value.GetBuffer(), pVarT->GetLength());
        }
    }


    // special processing for work strings
    else if( symbol.IsA(SymbolType::WorkString) )
    {
        ASSERT(has_initialize_value);
        WorkString& work_string = assert_cast<WorkString&>(symbol);
        work_string.SetString(EvalAlphaExpr(initialize_value));
    }


    else if( symbol.IsA(SymbolType::WorkVariable) )
    {
        ASSERT(has_initialize_value);
        WorkVariable& work_variable = assert_cast<WorkVariable&>(symbol);
        work_variable.SetValue(evalexpr(initialize_value));
    }


    else
    {
        ASSERT(false);
    }
}


double CIntDriver::exsymbolreset(int iExpr)
{
    // this function should reset any locally declared symbols to their default values
    const auto& symbol_reset_node = GetNode<Nodes::SymbolReset>(iExpr);

    ResetSymbol(NPT_Ref(symbol_reset_node.symbol_index), symbol_reset_node.initialize_value);

    return 0;
}


double CIntDriver::expersistentsymbolreset(int iExpr)
{
    std::set<int>& persistent_symbols_needing_reset_set = m_pEngineArea->m_persistentSymbolsNeedingResetSet;

    if( !persistent_symbols_needing_reset_set.empty() )
    {
        const auto& symbol_reset_node = GetNode<Nodes::SymbolReset>(iExpr);
        ASSERT(symbol_reset_node.function_code == FunctionCode::PERSISTENT_SYMBOL_RESET_CODE);

        // only reset the symbol if it has not already been reset
        const auto& persistent_lookup = persistent_symbols_needing_reset_set.find(symbol_reset_node.symbol_index);

        if( persistent_lookup != persistent_symbols_needing_reset_set.cend() )
        {
            persistent_symbols_needing_reset_set.erase(persistent_lookup);
            return exsymbolreset(iExpr);
        }
    }

    return 0;
}


const std::vector<UserFunction*>& CIntDriver::GetSpecialFunctions()
{
    if( m_specialFunctions.empty() )
    {
        auto validate_special_function = [&](const SpecialFunction special_function) -> UserFunction*
        {
            const TCHAR* const special_function_name = ToString(special_function);
            UserFunction* user_function = nullptr;

            if( !GetSymbolTable().NameExists(special_function_name) )
                return nullptr;

            try
            {
                user_function = &assert_cast<UserFunction&>(GetSymbolTable().FindSymbolOfType(special_function_name, SymbolType::UserFunction));
            }

            catch(...)
            {
                // in the future, perhaps symbols of other types should not be allowed to use the names of special functions
                return nullptr;
            }

            ASSERT(user_function != nullptr);

            constexpr SymbolType numeric_type = SymbolType::WorkVariable;
            constexpr SymbolType string_type = SymbolType::WorkString;

            // all functions return numbers (except for OnSyncMessage and OnActionInvokerResult)
            const bool function_is_OnSyncMessage = ( special_function == SpecialFunction::OnSyncMessage );
            const bool function_is_OnActionInvokerResult = ( special_function == SpecialFunction::OnActionInvokerResult );
            const SymbolType expected_return_type = ( function_is_OnSyncMessage || function_is_OnActionInvokerResult ) ? string_type : numeric_type;

            if( user_function->GetReturnType() != expected_return_type )
                return nullptr;

            const std::vector<int>& parameter_symbol_indices = user_function->GetParameterSymbolIndices();

            auto check_parameters = [&](const size_t min_numerics, const size_t max_numerics, const size_t min_strings, const size_t max_strings) -> bool
            {                            
                size_t number_numerics = 0;
                size_t number_strings = 0;

                for( const int symbol_index : parameter_symbol_indices )
                {
                    SymbolType symbol_type = NPT(symbol_index)->GetType();

                    if( Versioning::PredatesCompiledLogicVersion(Serializer::Iteration_7_6_000_1) )
                    {
                        if( symbol_type == SymbolType::Variable )
                            symbol_type = SymbolType::WorkString;
                    }

                    if( symbol_type == numeric_type )
                    {
                        ++number_numerics;
                    }

                    else if( symbol_type == string_type )
                    {
                        ++number_strings;
                    }
                }

                return ( ( number_numerics + number_strings ) == parameter_symbol_indices.size() ) &&
                         ( number_numerics >= min_numerics && number_numerics <= max_numerics ) &&
                         ( number_strings >= min_strings && number_strings <= max_strings );
            };

            bool valid;

            // OnSyncMessage has two string parameters
            if( function_is_OnSyncMessage )
            {
                valid = check_parameters(0, 0, 2, 2);
            }

            // OnActionInvokerResult has three string parameters
            else if( function_is_OnActionInvokerResult )
            {
                valid = check_parameters(0, 0, 3, 3);
            }

            // OnSystemMessage has at least one parameter (up to two numeric parameters and up to one string parameter)
            else if( special_function == SpecialFunction::OnSystemMessage )
            {
                valid = ( !parameter_symbol_indices.empty() && check_parameters(0, 2, 0, 1) );
            }

            // OnRefused doesn't have any parameters
            else if( special_function == SpecialFunction::OnRefused )
            {
                valid = check_parameters(0, 0, 0, 0);
            }

            // OnViewQuestionnaire has one optional string parameter
            else if( special_function == SpecialFunction::OnViewQuestionnaire )
            {
                valid = check_parameters(0, 0, 0, 1);
            }

            // others functions are only valid if there are only numeric parameters
            else
            {
                valid = check_parameters(0, parameter_symbol_indices.size(), 0, 0);
            }

            return valid ? user_function :
                           nullptr;
        };


        // check which of the special functions exists
        for( SpecialFunction special_function = FirstInEnum<SpecialFunction>();
             special_function <= LastInEnum<SpecialFunction>();
             IncrementEnum(special_function) )
        {
            m_specialFunctions.emplace_back(validate_special_function(special_function));
        }
    }

    return m_specialFunctions;
}


bool CIntDriver::HasSpecialFunction(const SpecialFunction special_function)
{
    return ( GetSpecialFunctions()[static_cast<size_t>(special_function)] != nullptr );
}


namespace
{
    class SpecialFunctionArgumentEvaluator : public UserFunctionArgumentEvaluator
    {
    public:
        SpecialFunctionArgumentEvaluator(const std::vector<std::variant<double, std::wstring>>& arguments)
            :   m_arguments(arguments)
        {
        }

    public:
        double GetNumeric(int parameter_number) override
        {
            if( static_cast<size_t>(parameter_number) < m_arguments.size() && std::holds_alternative<double>(m_arguments[parameter_number]) )
                return std::get<double>(m_arguments[parameter_number]);

            return DEFAULT;
        }

        std::wstring GetString(int parameter_number) override
        {
            if( static_cast<size_t>(parameter_number) < m_arguments.size() && std::holds_alternative<std::wstring>(m_arguments[parameter_number]) )
                return std::get<std::wstring>(m_arguments[parameter_number]);

            return std::wstring();
        }

    private:
        const std::vector<std::variant<double, std::wstring>>& m_arguments;
    };
}


double CIntDriver::ExecSpecialFunction(const int iSymVar, const SpecialFunction special_function,
                                       const std::vector<std::variant<double, std::wstring>>& arguments)
{
    const DataType return_type = ( special_function == SpecialFunction::OnSyncMessage ||
                                   special_function == SpecialFunction::OnActionInvokerResult ) ? DataType::String : DataType::Numeric;

    UserFunction* user_function = GetSpecialFunctions()[static_cast<size_t>(special_function)];

    if( user_function == nullptr || user_function->GetProgramIndex() < 0 )
        return AssignInvalidValue(return_type);

    // Now Execute the code
    m_iSkipStmt = FALSE; // RHF Sep 20, 2000.
    //If there is a field (the first field) before a roster and we arrive to this field
    // from a previous endsect. m_iSkipStmt remains in 1 when the field doesn't have proc.
    // So the PreProc of the Roster is not executed (see DeSetNextField GroupCompletion
    // is not called when m_iSkipStmt is 1.!!!
    if( m_bStopProc )
        return AssignInvalidValue(return_type);

    // TODO: make sure that all functions can work properly when m_iExSymbol is 0; for 
    // now only allow this in OnSystemMessage because that is an obscure feature (and if
    // m_iExSymbol is 0, we will activate the special function checking that keeps things
    // like movement statements from executing)
    if( iSymVar <= 0 && special_function != SpecialFunction::OnSystemMessage )
        return AssignInvalidValue(return_type);

    const RAII::SetValueAndRestoreOnDestruction prog_type_modifier(m_iProgType, PROCTYPE_ONFOCUS);
    const RAII::SetValueAndRestoreOnDestruction symbol_modifier(m_iExSymbol, iSymVar);
    const RAII::SetValueAndRestoreOnDestruction level_modifier(m_iExLevel, ( iSymVar > 0 ) ? SymbolCalculator::GetLevelNumber_base1(NPT_Ref(iSymVar)) : 0);

    m_iSkipStmt = FALSE;
    m_iStopExec = m_bStopProc;

    SetRequestIssued( false ); // reset RequestIssued// RHF Dec 03, 2003

    m_bExecSpecFunc = ( special_function == SpecialFunction::GlobalOnFocus || m_iExSymbol <= 0 );

    SpecialFunctionArgumentEvaluator argument_evaluator(arguments);
    const double return_value = CallUserFunction(*user_function, argument_evaluator);

    m_bExecSpecFunc = false;

    m_iStopExec = FALSE;

    return return_value;
}


bool CIntDriver::ExecuteOnSystemMessage(MessageType message_type, int message_number, const std::wstring& message_text)
{
    const UserFunction* user_function = GetSpecialFunctions()[static_cast<size_t>(SpecialFunction::OnSystemMessage)];
    ASSERT(user_function != nullptr);

    // OnSystemMessage can have one to three arguments (up to two numerics and one string);
    // if only one numeric argument, the message number is provided
    std::vector<std::variant<double, std::wstring>> arguments;
    bool on_first_numeric = true;

    for( int symbol_index : user_function->GetParameterSymbolIndices() )
    {
        if( NPT(symbol_index)->IsA(SymbolType::WorkVariable) )
        {
            if( on_first_numeric )
            {
                arguments.emplace_back(static_cast<double>(message_number));
                on_first_numeric = false;
            }

            else
            {
                arguments.emplace_back(static_cast<double>(message_type));
            }
        }

        else
        {
            arguments.emplace_back(message_text);
        }
    }

    // if a system message occurs as a result of code run in OnSystemMessage, a poorly designed
    // application could have an endless loop, so stop running this special function after
    // a certain number of times
    constexpr int RecursionCountMax = 5;
    static int infinite_loop_prevention = 0;

    bool issue_message = true;

    if( ++infinite_loop_prevention <= RecursionCountMax )
    {
        issue_message = ( ExecSpecialFunction(m_iExSymbol, SpecialFunction::OnSystemMessage, arguments) != 0 );
        --infinite_loop_prevention;
    }

    return issue_message;
}

void CIntDriver::RunGlobalOnFocus(int iVar)
{
    if (Issamod != ModuleType::Entry)
        return;

    ASSERT(NPT(iVar)->IsA(SymbolType::Variable));

    // run On_Focus
    if (HasSpecialFunction(SpecialFunction::GlobalOnFocus))
        ExecSpecialFunction(iVar, SpecialFunction::GlobalOnFocus, { (double)iVar });

#ifdef WIN_DESKTOP
    VART* pVarT = VPT(iVar);
    UpdateKeyboardInputMethod(pVarT->GetHKL()); // 20120821
#endif
}


#ifdef WIN_DESKTOP
void CIntDriver::UpdateKeyboardInputMethod(HKL hKL) // 20120821
{
    if (!hKL) // default keyboard
    {
        if (m_hCurrentKL) // currently not the default
        {
            ActivateKeyboardLayout(m_hLastDefaultKL, 0);
            m_hLastDefaultKL = NULL;
            m_hCurrentKL = NULL;
        }
    }

    else // setting the keyboard
    {
        if (!m_hLastDefaultKL) // get the current keyboard
            m_hLastDefaultKL = GetKeyboardLayout(0);

        if (m_hCurrentKL != hKL)
        {
            m_hCurrentKL = hKL;
            ActivateKeyboardLayout(m_hCurrentKL, 0);
        }
    }
}
#endif


LoopStack& CIntDriver::GetLoopStack()
{
    if( m_loopStack == nullptr )
    {
        class InterpreterLoopStack : public LoopStack
        {
        public:
            InterpreterLoopStack(CEngineDriver* pEngineDriver)
                :   m_messageIssuer(pEngineDriver)
            {
            }

        protected:
            MessageIssuer& GetMessageIssuer() override
            {
                return m_messageIssuer;
            }

        private:
            InterpreterMessageIssuer m_messageIssuer;
        };

        m_loopStack = std::make_unique<InterpreterLoopStack>(m_pEngineDriver);
    }

    return *m_loopStack;
}


double CIntDriver::exScopeChange(const int program_index)
{
    const auto& scope_change_node = GetNode<Nodes::ScopeChange>(program_index);
    ASSERT(scope_change_node.program_index != -1);

    const RAII::PushOnVectorAndPopOnDestruction scope_change_holder(m_scopeChangeNodeIndices, &scope_change_node);

    return ExecuteProgramStatements(scope_change_node.program_index);
}
