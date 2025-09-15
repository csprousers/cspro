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
#include "Interpreter.h"
#include "Engine.h"
#include "Ctab.h"
#include "FrequencyDriver.h"
#include "ImputationDriver.h"
#include "InterpreterMessageIssuer.h"
#include "ProgramControl.h"
#include "SelcaseManager.h"
#include <zPlatformO/PlatformInterface.h>
#include <zLogicO/SpecialFunction.h>
#include <zEngineO/AllSymbols.h>
#include <zEngineO/EngineAccessor.h>
#include <zEngineO/JavaScriptProcessor.h>
#include <zEngineO/LoopStack.h>
#include <zEngineO/SaveArrayFile.h>
#include <zEngineO/UserFunctionArgumentEvaluator.h>
#include <zEngineF/TraceHandler.h>
#include <zEngineF/WindowsApplicationInterface.h>
#include <zToolsO/Tools.h>
#include <zUtilO/MemoryHelpers.h>
#include <zUtilF/KeyboardLoader.h>
#include <zBridgeO/NPff.h>
#include <zMessageO/Messages.h>
#include <zCapiO/CapiQuestionManager.h>
#include <zFreqO/Frequency.h>
#include <zReportO/Pre77ReportManager.h>



//------------------------------------------------------------------------
//
// --- constructor/destructor/initialization
//
//------------------------------------------------------------------------

namespace
{
    cs::non_null_shared_or_raw_ptr<ApplicationInterface> CreateApplicationInterface()
    {
#ifdef WIN32
        return std::make_shared<WindowsApplicationInterface>();
#elif defined(ANDROID)
        return PlatformInterface::GetInstance()->GetApplicationInterface();
#else
        static_assert_false();
#endif
    }
}


CIntDriver::CIntDriver(CEngineDriver& engine_driver)
    :   LogicInterpreter(engine_driver.getEngineAreaPtr()->GetSharedEngineData(), CreateApplicationInterface()),
        m_aFixedDimensions(ONE_BASED),
        m_pEngineDriver(&engine_driver),
        m_pEngineArea(m_pEngineDriver->getEngineAreaPtr()),
        m_pEngineSettings(&m_pEngineDriver->m_EngineSettings),
        m_keyboardLoader(std::make_unique<KeyboardLoader>())
{
    // --- procedure being executed
    m_procType = ProcType::PreProc;
    m_iExLevel = 0;
    m_iExSymbol = 0;

    // --- execution flags
    m_bStopExec = false;
    ASSERT(!m_bStopProc);
    m_bSkipStmt = false;

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

    m_paradataDriver = std::make_unique<EngineParadataDriver>(*this);

    // add a routine to set VART objects
    m_engineData->engine_accessor->ea_SetVarTValueSetter(
        [&](Symbol* symbol, std::wstring value)
        {
            ASSERT(symbol->IsA(SymbolType::Variable));
            VART* const pVarT = assert_cast<VART*>(symbol);

            if( !pVarT->IsUsed() )
                return;

            ASSERT(pVarT->GetLength() != 0);

            SO::MakeExactLength(value, pVarT->GetLength());

            VARX* pVarX = pVarT->GetVarX();
            TCHAR* pBuffer = (TCHAR*)svaraddr(pVarX);
            _tmemcpy(pBuffer, value.data(), pVarT->GetLength());
        });
}


CIntDriver::~CIntDriver()
{
}


void CIntDriver::StartApplication()
{
    const Application* const application = m_pEngineDriver->m_pPifFile->GetApplication();
    const CNPifFile* const pff = m_pEngineDriver->m_pPifFile;
    ASSERT(application != nullptr && pff != nullptr);

    m_usingLogicSettingsV0 = ( application->GetLogicSettings().GetVersion() == LogicSettings::Version::V0 );

    // initialize the runtime for each dictionary
    for( DICT* const pDicT : m_engineData->dictionaries_pre80 )
        pDicT->GetDicX()->StartRuntime();


    // start the paradata
    m_paradataDriver->LogEngineEvent(ParadataEngineEvent::ApplicationStart);


    // set the initial language
    SetStartupLanguage();


    // load save arrays
    if( application->GetHasSaveArrays() )
    {
        // read errors will only be issued in batch applications
        SaveArrayFile save_array_file;
        save_array_file.ReadArrays(pff,
                                   m_engineData->arrays,
                                   ( application->GetEngineAppType() == EngineAppType::Batch ) ? m_pEngineDriver->GetSharedSystemMessageIssuer() : nullptr);
    }


    // set the initial file handler filenames
    for( LogicFile* const logic_file : m_engineData->files_global_visibility )
        logic_file->SetFilePath(UTF8_TODO::GetUtf8(pff->LookUpUsrDatFile(UTF8_TODO::GetCString(logic_file->GetName()))));


    // start the frequencies driver
    if( !m_engineData->frequencies.empty() )
        m_frequencyDriver = std::make_unique<FrequencyDriver>(*this);

    // start the imputation driver
    if( !m_engineData->imputations.empty() )
        m_imputationDriver = std::make_unique<ImputationDriver>(*this);

    // run OnStart events
    m_engineData->runtime_events_processor.RunEventsOnStart();

    // evaluate any JavaScript added to the application
    if( m_engineData->javascript_processor != nullptr )
        EvaluateApplicationStartupJavaScript();

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
    m_paradataDriver->LogEngineEvent(ParadataEngineEvent::ApplicationStop);
}


void CIntDriver::PrepareForExportExec(int iSymbol, const ProcType proc_type)
{
    m_bSkipStmt = false;

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

    m_procType = proc_type;
    m_iExSymbol = iSymbol;
    m_iExLevel  = iLevel;
    m_bSkipStmt = false;
    m_bStopExec = m_bStopProc;
}


std::string CIntDriver::ProcName()
{
    if( m_iExSymbol <= 0 )
        return "Unknown";

    const Symbol& symbol = NPT_Ref(m_iExSymbol);
    const SymbolType symbol_type = symbol.GetType();

    if( symbol_type == SymbolType::Pre80Dictionary )
    {
        return FormatText("Dict %s Level %d %s", symbol.GetName().c_str(), m_iExLevel, GetProcTypeName(m_procType));
    }

    else if( symbol_type == SymbolType::Report )
    {
        return "Report " + symbol.GetName();
    }

    else
    {
        const char* type;

        if( symbol_type == SymbolType::Section )
        {
            type = is_digit(symbol.GetName().front()) ? "View" : "Sect";
        }

        else if( symbol_type == SymbolType::Group )
        {
            type = ( assert_cast<const GROUPT&>(symbol).GetGroupType() == GROUPT::Level ) ? "Level" : "Group";
        }

        else if( symbol_type == SymbolType::Crosstab )
        {
            type = "Table";
        }

        else if( symbol_type == SymbolType::Block )
        {
            type = "Block";
        }

        else
        {
            ASSERT(symbol_type == SymbolType::Variable); // RHF Oct 29, 2002
            type = "Var";
        }

        return FormatText("%s %s %s", type, symbol.GetName().c_str(), GetProcTypeName(m_procType));
    }
}


CIntDriver::pDoubleFunction CIntDriver::m_pExFuncs[] =
{
/*-------------------+----------------------------------------------------*/
/*Op.code│ Function │      COMMANDS                                       */
/*-------------------+----------------------------------------------------*/
/*   0 */   &CIntDriver::ex_numeric_constant,
/*   1 */   &CIntDriver::exsvar,
/*   2 */   &CIntDriver::exmvar,
/*   3 */   &CIntDriver::excpt,
/*   4 */   &CIntDriver::ex_add,
/*   5 */   &CIntDriver::ex_sub,
/*   6 */   &CIntDriver::ex_mult,
/*   7 */   &CIntDriver::ex_div,
/*   8 */   &CIntDriver::ex_mod,
/*   9 */   &CIntDriver::ex_minus,
/*  10 */   &CIntDriver::ex_exp,
/*  11 */   &CIntDriver::ex_or,
/*  12 */   &CIntDriver::ex_and,
/*  13 */   &CIntDriver::ex_not,
/*  14 */   &CIntDriver::ex_eq,
/*  15 */   &CIntDriver::ex_ne,
/*  16 */   &CIntDriver::ex_le,
/*  17 */   &CIntDriver::ex_lt,
/*  18 */   &CIntDriver::ex_ge,
/*  19 */   &CIntDriver::ex_gt,
/*  20 */   &CIntDriver::ex_equ,
/*  21 */   &CIntDriver::exstringcompute,
/*  22 */   &CIntDriver::ex_WorkVariable_evaluate,
/*  23 */   &CIntDriver::exif,
/*  24 */   &CIntDriver::exwhile,
/*  25 */   &CIntDriver::exbox,
/*  26 */   &CIntDriver::ex_string_literal, // an old implementation of ex_string_literal
/*  27 */   &CIntDriver::excharobj,
/*  28 */   &CIntDriver::ex_string_eq, // =
/*  29 */   &CIntDriver::ex_string_ne, // <>
/*  30 */   &CIntDriver::ex_string_le, // <=
/*  31 */   &CIntDriver::ex_string_lt, // <
/*  32 */   &CIntDriver::ex_string_ge, // >=
/*  33 */   &CIntDriver::ex_string_gt, // >
/*  34 */   &CIntDriver::excpttbl,
/*  35 */   &CIntDriver::exnoopAbort,
/*  36 */   &CIntDriver::exnoopAbort, // an old implementation of ex_Array_var
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
/*  58 */   &CIntDriver::ex_sqrt,
/*  59 */   &CIntDriver::ex_ex,
/*  60 */   &CIntDriver::ex_int,
/*  61 */   &CIntDriver::ex_log,
/*  62 */   &CIntDriver::ex_seed,
/*  63 */   &CIntDriver::ex_random,
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
/*  74 */   &CIntDriver::ex_concat,
/*  75 */   &CIntDriver::ex_tonumber,
/*  76 */   &CIntDriver::ex_pos_poschar, // pos
/*  77 */   &CIntDriver::ex_compare,
/*  78 */   &CIntDriver::ex_length,
/*  79 */   &CIntDriver::ex_strip,
/*  80 */   &CIntDriver::ex_pos_poschar, // poschar
/*  81 */   &CIntDriver::exedit,

/*───────┬──────────┬-----------------------------------------------------*/
/*Op.code│ Function │      DATE FUNCTIONS                                 */
/*───────┴──────────┴-----------------------------------------------------*/
/*  82 */   &CIntDriver::ex_cmcode,
/*  83 */   &CIntDriver::ex_setlb_setub, // setub
/*  84 */   &CIntDriver::ex_setlb_setub, // setlb
/*  85 */   &CIntDriver::ex_adjuba,
/*  86 */   &CIntDriver::ex_adjlba,
/*  87 */   &CIntDriver::ex_adjlbi,
/*  88 */   &CIntDriver::ex_adjubi,
/*  89 */   &CIntDriver::exnoopAbort, // exdatechk
/*  90 */   &CIntDriver::ex_systime,
/*  91 */   &CIntDriver::ex_sysdate,

/*───────┬──────────┬-----------------------------------------------------*/
/*Op.code│ Function │      OTHER FUNCTIONS                                */
/*───────┴──────────┴-----------------------------------------------------*/
/*  92 */   &CIntDriver::exdemode,
/*  93 */   &CIntDriver::ex_special,
/*  94 */   &CIntDriver::ex_accept,
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
/* 113 */   &CIntDriver::ex_open,
/* 114 */   &CIntDriver::ex_close,
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
/* 133 */   &CIntDriver::ex_in,        // RHC Oct 16, 2000
/* 134 */   &CIntDriver::ex_do,        // RHC Oct 16, 2000
/* 135 */   &CIntDriver::ex_impute,    // RHF Oct 25, 2000
/* 136 */   &CIntDriver::exfncurocc,   // RHC Oct 16, 2000
/* 137 */   &CIntDriver::exfntotocc,   // RHC Oct 16, 2000
/* 138 */   &CIntDriver::exupdate,     // RHF Nov 17, 2000
/* 139 */   &CIntDriver::exwrite,      // RHF Dec 16, 2000
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
/* 158 */   &CIntDriver::ex_setfile,
/* 159 */   &CIntDriver::exmaxocc_pre80,
/* 160 */   &CIntDriver::exinvalueset,
/* 161 */   &CIntDriver::exsetvalueset,    // RHF Aug 28, 2002

// RHF INIC Oct 15, 2004
/* 162 */   &CIntDriver::exfilecreate,
/* 163 */   &CIntDriver::exfileexist,
/* 164 */   &CIntDriver::exfiledelete,
/* 165 */   &CIntDriver::ex_filecopy,
/* 166 */   &CIntDriver::ex_filerename,
/* 167 */   &CIntDriver::exfilesize,
/* 168 */   &CIntDriver::exfileconcat,
/* 169 */   &CIntDriver::exfileread,
/* 170 */   &CIntDriver::exfilewrite,
// RHF END Oct 15, 2004

/* 171 */   &CIntDriver::ExExecSystem,

/* 172 */   &CIntDriver::exnoopAbort,        // an old implementation of exshow
/* 173 */   &CIntDriver::exshowlist,

/* 174 */   &CIntDriver::ex_tolower_toupper, // GHM 20091202 tolower
/* 175 */   &CIntDriver::ex_tolower_toupper, // GHM 20091202 toupper
/* 176 */   &CIntDriver::excountvalid,       // GHM 20091202
/* 177 */   &CIntDriver::exnoopIgnore_string,// GHM 20091208 previously itemlist
/* 178 */   &CIntDriver::exswap,             // GHM 20100105
/* 179 */   &CIntDriver::ex_datediff,        // GHM 20100119
/* 180 */   &CIntDriver::exdeckarray,        // GHM 20100119 putdeck
/* 181 */   &CIntDriver::exdeckarray,        // GHM 20100119 getdeck
/* 182 */   &CIntDriver::ex_getlanguage,     // GHM 20100309
/* 183 */   &CIntDriver::ex_setlanguage,     // GHM 20100309
/* 184 */   &CIntDriver::exendcase,          // GHM 20100310
/* 185 */   &CIntDriver::exuserbar,          // GHM 20100414
/* 186 */   &CIntDriver::exmessageoverrides, // GHM 20100518
/* 187 */   &CIntDriver::ex_trace,           // GHM 20100518
/* 188 */   &CIntDriver::exsetvaluesets,     // GHM 20100523
/* 189 */   &CIntDriver::ExExecPFF,          // GHM 20100601
/* 190 */   &CIntDriver::exseek,             // GHM 20100602
/* 191 */   &CIntDriver::exgetcapturetype,   // GHM 20100608
/* 192 */   &CIntDriver::exsetcapturetype,   // GHM 20100608
/* 193 */   &CIntDriver::ex_setfont,         // GHM 20100618
/* 194 */   &CIntDriver::exorientation,      // GHM 20100618 getorientation
/* 195 */   &CIntDriver::exorientation,      // GHM 20100618 setorientation
/* 196 */   &CIntDriver::ex_pathname,        // GHM 20110107
/* 197 */   &CIntDriver::exgps,              // GHM 20110223
/* 198 */   &CIntDriver::ex_low_high,        // GHM 20110301 low
/* 199 */   &CIntDriver::ex_low_high,        // GHM 20110301 high
/* 200 */   &CIntDriver::exgetrecord,        // GHM 20110302
/* 201 */   &CIntDriver::ex_setcapturepos,   // GHM 20110502
/* 202 */   &CIntDriver::ex_abs,             // GHM 20110721
/* 203 */   &CIntDriver::ex_randomin,        // GHM 20110721
/* 204 */   &CIntDriver::exrandomizevs,      // GHM 20110811
/* 205 */   &CIntDriver::ex_getusername,     // GHM 20111028
/* 206 */   &CIntDriver::exfileempty,        // GHM 20120627
/* 207 */   &CIntDriver::ex_changekeyboard,  // GHM 20120820
/* 208 */   &CIntDriver::ex_setoutput,       // GHM 20121126
/* 209 */   &CIntDriver::exseekMinMax,       // GHM 20130119
/* 210 */   &CIntDriver::exseekMinMax,       // GHM 20130119
/* 211 */   &CIntDriver::ex_dateadd,         // GHM 20130225
/* 212 */   &CIntDriver::ex_datevalid,       // GHM 20130703
/* 213 */   &CIntDriver::ex_getos,           // GHM 20131217
/* 214 */   &CIntDriver::exgetocclabel,      // GHM 20140226
/* 215 */   &CIntDriver::exfreealphamem,     // GHM 20140228
/* 216 */   &CIntDriver::exsetvalue,         // GHM 20140228
/* 217 */   &CIntDriver::exgetvalue,         // GHM 20140422
/* 218 */   &CIntDriver::exgetvaluealpha,    // GHM 20140422
/* 219 */   &CIntDriver::exnoopAbort,        // GHM 20140423 an old implementation of exshowarray
/* 220 */   &CIntDriver::exsetocclabel,      // GHM 20141006
/* 221 */   &CIntDriver::exshowocc,          // GHM 20141015 showocc
/* 222 */   &CIntDriver::exshowocc,          // GHM 20141015 hideocc
/* 223 */   &CIntDriver::ex_getdeviceid,     // GHM 20141023
/* 224 */   &CIntDriver::exdirexist,         // GHM 20141024
/* 225 */   &CIntDriver::exdircreate,        // GHM 20141024
/* 226 */   &CIntDriver::exnoopAbort,        // GHM 20141024 previously sync
/* 227 */   &CIntDriver::ex_List_var,        // GHM 20141106
/* 228 */   &CIntDriver::exdirlist,          // GHM 20141107
/* 229 */   &CIntDriver::ex_sysparm,         // GHM 20141217
/* 230 */   &CIntDriver::ex_connection,      // GHM 20150421
/* 231 */   &CIntDriver::ex_prompt,          // GHM 20150422
/* 232 */   &CIntDriver::exgetimage,         // GHM 20150809
/* 233 */   &CIntDriver::ex_round,           // GHM 20150821
/* 234 */   &CIntDriver::exnoopAbort,        // GHM 20151130 an old implementation of exuuid ... now a publishdate placeholder
/* 235 */   &CIntDriver::exsavepartial,      // GHM 20151216
/* 236 */   &CIntDriver::ex_syncconnect,
/* 237 */   &CIntDriver::ex_syncdisconnect,
/* 238 */   &CIntDriver::ex_syncdata,
/* 239 */   &CIntDriver::ex_syncfile,
/* 240 */   &CIntDriver::ex_syncserver,
/* 241 */   &CIntDriver::ex_savesetting,
/* 242 */   &CIntDriver::ex_loadsetting,
/* 243 */   &CIntDriver::exgetcaselabel,
/* 244 */   &CIntDriver::exsetcaselabel,
/* 245 */   &CIntDriver::exispartial,
/* 246 */   &CIntDriver::exsetoperatorid,
/* 247 */   &CIntDriver::exgetnote,
/* 248 */   &CIntDriver::exeditnote,
/* 249 */   &CIntDriver::exputnote,
/* 250 */   &CIntDriver::exisverified,
/* 251 */   &CIntDriver::exforcase,
/* 252 */   &CIntDriver::ex_timestamp,
/* 253 */   &CIntDriver::exkeylist,
/* 254 */   &CIntDriver::exdiagnostics,
/* 255 */   &CIntDriver::ex_compress,
/* 256 */   &CIntDriver::ex_decompress,
/* 257 */   &CIntDriver::exask,
/* 258 */   &CIntDriver::excountcases,
/* 259 */   &CIntDriver::exgetproperty,
/* 260 */   &CIntDriver::exsetproperty,
/* 261 */   &CIntDriver::exlogtext,
/* 262 */   &CIntDriver::exwarning,
/* 263 */   &CIntDriver::ex_tr,
/* 264 */   &CIntDriver::ex_uuid,
/* 265 */   &CIntDriver::ex_paradata,
/* 266 */   &CIntDriver::exsqlquery,
/* 267 */   &CIntDriver::expre77_report,
/* 268 */   &CIntDriver::expre77_setreportdata,
/* 269 */   &CIntDriver::exshow,
/* 270 */   &CIntDriver::exshowarray,
/* 271 */   &CIntDriver::exselcase,
/* 272 */   &CIntDriver::ex_timestring,
/* 273 */   &CIntDriver::ex_string_literal,
/* 274 */   &CIntDriver::exsymbolreset,
/* 275 */   &CIntDriver::ex_decryptstring,
/* 276 */   &CIntDriver::exdirdelete,
/* 277 */   &CIntDriver::ex_Array_var,
/* 278 */   &CIntDriver::extvar,
/* 279 */   &CIntDriver::ex_exit,
/* 280 */   &CIntDriver::ex_getbluetoothname,
/* 281 */   &CIntDriver::ex_regexmatch,
/* 282 */   nullptr, // BLOCK_CODE
/* 283 */   &CIntDriver::exgetvaluelabel,
/* 284 */   &CIntDriver::ex_Array_clear,
/* 285 */   &CIntDriver::ex_Array_length,
/* 286 */   &CIntDriver::ex_Map_show,
/* 287 */   &CIntDriver::ex_Map_hide,
/* 288 */   &CIntDriver::ex_Map_addMarker,
/* 289 */   &CIntDriver::ex_Map_setMarkerImage,
/* 290 */   &CIntDriver::ex_Map_setMarkerText,
/* 291 */   &CIntDriver::ex_Map_setMarkerOnClick_setMarkerOnClickInfo, // Map.setMarkerOnClick
/* 292 */   &CIntDriver::ex_Map_setMarkerOnClick_setMarkerOnClickInfo, // Map.setMarkerOnClickInfo
/* 293 */   &CIntDriver::ex_Map_setMarkerDescription,
/* 294 */   &CIntDriver::ex_Map_setMarkerOnDrag,
/* 295 */   &CIntDriver::ex_Map_setMarkerLocation,
/* 296 */   &CIntDriver::ex_Map_getMarkerLatitude_getMarkerLongitude,  // Map.getMarkerLatitude
/* 297 */   &CIntDriver::ex_Map_removeMarker,
/* 298 */   &CIntDriver::ex_Map_setOnClick,
/* 299 */   &CIntDriver::ex_Map_showCurrentLocation,
/* 300 */   &CIntDriver::ex_Map_addTextButton,
/* 301 */   &CIntDriver::ex_Map_addImageButton,
/* 302 */   &CIntDriver::ex_Map_removeButton,
/* 303 */   &CIntDriver::ex_Map_setBaseMap,
/* 304 */   &CIntDriver::ex_Map_setTitle,
/* 305 */   &CIntDriver::ex_Map_zoomTo,
/* 306 */   &CIntDriver::ex_List_add,
/* 307 */   &CIntDriver::ex_List_clear,
/* 308 */   &CIntDriver::ex_List_insert,
/* 309 */   &CIntDriver::ex_List_length,
/* 310 */   &CIntDriver::ex_List_remove,
/* 311 */   &CIntDriver::ex_List_seek,
/* 312 */   &CIntDriver::ex_List_show,
/* 313 */   &CIntDriver::ex_List_compute,
/* 314 */   &CIntDriver::exvaluesetadd,
/* 315 */   &CIntDriver::exvaluesetclear,
/* 316 */   &CIntDriver::exvaluesetremove,
/* 317 */   &CIntDriver::exvaluesetshow,
/* 318 */   &CIntDriver::exvaluesetcompute,
/* 319 */   &CIntDriver::exvariablevalue,
/* 320 */   &CIntDriver::ex_Map_clear_clearButtons_clearGeometry_clearMarkers, // Map.clearMarkers
/* 321 */   &CIntDriver::ex_Map_clear_clearButtons_clearGeometry_clearMarkers, // Map.clearButtons
/* 322 */   &CIntDriver::ex_Map_getLastClickLatitude_getLastClickLongitude, // Map.getLastClickLatitude
/* 323 */   &CIntDriver::ex_Map_getLastClickLatitude_getLastClickLongitude, // Map.getLastClickLongitude
/* 324 */   &CIntDriver::ex_Map_getMarkerLatitude_getMarkerLongitude,    // Map.getMarkerLongitude
/* 325 */   &CIntDriver::ex_Path_concat,
/* 326 */   &CIntDriver::ex_view,
/* 327 */   &CIntDriver::expffexec,
/* 328 */   &CIntDriver::expffgetproperty,
/* 329 */   &CIntDriver::expffload,
/* 330 */   &CIntDriver::expffsave,
/* 331 */   &CIntDriver::expffsetproperty,
/* 332 */   &CIntDriver::exvaluesetlength,
/* 333 */   &CIntDriver::ex_ischecked,
/* 334 */   &CIntDriver::exprotect,
/* 335 */   &CIntDriver::ex_when,
/* 336 */   &CIntDriver::ex_syncapp,
/* 337 */   &CIntDriver::exfiletime,
/* 338 */   &CIntDriver::ex_recode,
/* 339 */   &CIntDriver::exforcase,
/* 340 */   &CIntDriver::exselcase,
/* 341 */   &CIntDriver::excountcases,
/* 342 */   &CIntDriver::exkeylist,
/* 343 */   &CIntDriver::ex_Barcode_read,
/* 344 */   &CIntDriver::ex_hash,
/* 345 */   &CIntDriver::ex_syncmessage,
/* 346 */   &CIntDriver::ex_SystemApp_clear,
/* 347 */   &CIntDriver::ex_SystemApp_setArgument,
/* 348 */   &CIntDriver::ex_SystemApp_getResult,
/* 349 */   &CIntDriver::ex_SystemApp_exec,
/* 350 */   &CIntDriver::ex_startswith,
/* 351 */   &CIntDriver::expffcompute,
/* 352 */   &CIntDriver::ex_Audio_clear,
/* 353 */   &CIntDriver::ex_Audio_concat,
/* 354 */   &CIntDriver::ex_Audio_load,
/* 355 */   &CIntDriver::ex_Audio_play,
/* 356 */   &CIntDriver::ex_Audio_save,
/* 357 */   &CIntDriver::ex_Audio_stop,
/* 358 */   &CIntDriver::ex_Audio_record,
/* 359 */   &CIntDriver::ex_Audio_recordInteractive,
/* 360 */   &CIntDriver::ex_Audio_compute,
/* 361 */   &CIntDriver::ex_encode,
/* 362 */   &CIntDriver::ex_List_sort,
/* 363 */   &CIntDriver::ex_List_removeDuplicates,
/* 364 */   &CIntDriver::ex_List_removeIn,
/* 365 */   &CIntDriver::ex_Path_concat,
/* 366 */   &CIntDriver::ex_Path_getDirectoryName,
/* 367 */   &CIntDriver::ex_Path_getExtension,
/* 368 */   &CIntDriver::ex_Path_getFileName,
/* 369 */   &CIntDriver::ex_Path_getFileNameWithoutExtension,
/* 370 */   &CIntDriver::ex_syncparadata,
/* 371 */   &CIntDriver::ex_HashMap_var,
/* 372 */   &CIntDriver::ex_HashMap_compute,
/* 373 */   &CIntDriver::ex_HashMap_clear,
/* 374 */   &CIntDriver::ex_HashMap_contains,
/* 375 */   &CIntDriver::ex_HashMap_length,
/* 376 */   &CIntDriver::ex_HashMap_remove,
/* 377 */   &CIntDriver::ex_HashMap_getKeys,
/* 378 */   &CIntDriver::ex_Audio_length,
/* 379 */   &CIntDriver::exvaluesetsort,
/* 380 */   &CIntDriver::ex_replace,
/* 381 */   &CIntDriver::ex_inc,
/* 382 */   &CIntDriver::exuniverse,
/* 383 */   &CIntDriver::ex_Freq_unnamed,
/* 384 */   &CIntDriver::ex_Freq_clear,
/* 385 */   &CIntDriver::ex_Freq_save,
/* 386 */   &CIntDriver::ex_Freq_tally,
/* 387 */   &CIntDriver::ex_Freq_view,
/* 388 */   &CIntDriver::ex_Freq_var,
/* 389 */   &CIntDriver::ex_Freq_compute,
/* 390 */   &CIntDriver::ex_WorkString_evaluate,
/* 391 */   &CIntDriver::exmaxocc,
/* 392 */   &CIntDriver::exsoccurs,
/* 393 */   &CIntDriver::exDataAccessValidityCheck,
/* 394 */   &CIntDriver::exdictcompute,
/* 395 */   &CIntDriver::exkey, // currentkey
/* 396 */   &CIntDriver::ex_Image_compute,
/* 397 */   &CIntDriver::ex_Image_captureSignature_takePhoto, // Image.captureSignature
/* 398 */   &CIntDriver::ex_Image_clear,
/* 399 */   &CIntDriver::ex_Image_width_height, // Image.height
/* 400 */   &CIntDriver::ex_Image_load,
/* 401 */   &CIntDriver::ex_Image_resample,
/* 402 */   &CIntDriver::ex_Image_save,
/* 403 */   &CIntDriver::ex_Image_captureSignature_takePhoto, // Image.takePhoto
/* 404 */   &CIntDriver::ex_Image_view,
/* 405 */   &CIntDriver::ex_Image_width_height, // Image.width
/* 406 */   &CIntDriver::ex_Document_compute,
/* 407 */   &CIntDriver::ex_Document_clear,
/* 408 */   &CIntDriver::ex_Document_load,
/* 409 */   &CIntDriver::ex_Document_save,
/* 410 */   &CIntDriver::ex_Document_view,
/* 411 */   &CIntDriver::ex_Geometry_compute,
/* 412 */   &CIntDriver::ex_Geometry_clear,
/* 413 */   &CIntDriver::ex_Geometry_load,
/* 414 */   &CIntDriver::ex_Geometry_save,
/* 415 */   &CIntDriver::ex_Map_addGeometry,
/* 416 */   &CIntDriver::ex_Map_removeGeometry,
/* 417 */   &CIntDriver::ex_Map_clear_clearButtons_clearGeometry_clearMarkers, // Map.clearGeometry
/* 418 */   &CIntDriver::ex_Geometry_tracePolygon_walkPolygon, // Geometry.tracePolygon
/* 419 */   &CIntDriver::ex_Geometry_tracePolygon_walkPolygon, // Geometry.walkPolygon
/* 420 */   &CIntDriver::ex_Geometry_area_perimeter, // Geometry.area
/* 421 */   &CIntDriver::ex_Geometry_area_perimeter, // Geometry.perimeter
/* 422 */   &CIntDriver::ex_Geometry_minLatitude_maxLatitude_minLongitude_maxLongitude, // Geometry.minLatitude
/* 423 */   &CIntDriver::ex_Geometry_minLatitude_maxLatitude_minLongitude_maxLongitude, // Geometry.maxLatitude
/* 424 */   &CIntDriver::ex_Geometry_minLatitude_maxLatitude_minLongitude_maxLongitude, // Geometry.minLongitude
/* 425 */   &CIntDriver::ex_Geometry_minLatitude_maxLatitude_minLongitude_maxLongitude, // Geometry.maxLongitude
/* 426 */   &CIntDriver::ex_Geometry_getProperty,
/* 427 */   &CIntDriver::ex_Geometry_setProperty,
/* 428 */   &CIntDriver::exinadvance,
/* 429 */   &CIntDriver::ex_Map_saveSnapshot,
/* 430 */   &CIntDriver::ex_synctime,
/* 431 */   &CIntDriver::ex_htmldialog,
/* 432 */   &CIntDriver::ex_Path_getRelativePath,
/* 433 */   &CIntDriver::ex_Path_selectFile,
/* 434 */   &CIntDriver::ex_invoke,
/* 435 */   &CIntDriver::ex_Report_save,
/* 436 */   &CIntDriver::ex_Report_view,
/* 437 */   &CIntDriver::ex_TextTemplate_write_writeEncoded_writeEncodedLine_writeLine, // Report.write prior to CSPro 8.1
/* 438 */   &CIntDriver::ex_setbluetoothname,
/* 439 */   &CIntDriver::expersistentsymbolreset,
/* 440 */   &CIntDriver::ex_Symbol_getJson_getValueJson, // symbol.getJson
/* 441 */   &CIntDriver::ex_Symbol_getJson_getValueJson, // symbol.getValueJson
/* 442 */   &CIntDriver::ex_Symbol_setValueFromJson,
/* 443 */   &CIntDriver::ex_Barcode_createQRCode, // Barcode.createQRCode + Image.createQRCode
/* 444 */   &CIntDriver::exScopeChange,
/* 445 */   &CIntDriver::exdictaccess,
/* 446 */   &CIntDriver::ex_WorkString_assign,
/* 447 */   &CIntDriver::ex_ActionInvoker,
/* 448 */   &CIntDriver::ex_Symbol_getName,
/* 449 */   &CIntDriver::ex_Symbol_getLabel,
/* 450 */   &CIntDriver::ex_Map_clear_clearButtons_clearGeometry_clearMarkers, // Map.clear
/* 451 */   &CIntDriver::exItem_hasValue_isValid, // Item.hasValue
/* 452 */   &CIntDriver::exItem_getValueLabel,
/* 453 */   &CIntDriver::exItem_hasValue_isValid, // Item.isValid
/* 454 */   &CIntDriver::ex_compareNoCase,
/* 455 */   &CIntDriver::exCase_view,
/* 456 */   &CIntDriver::ex_JavaScript_eval,
/* 457 */   &CIntDriver::ex_JavaScript_invoke,
/* 458 */   &CIntDriver::ex_JavaScript_hasValue,
/* 459 */   &CIntDriver::ex_JavaScript_getValueJson,
/* 460 */   &CIntDriver::ex_JavaScript_setValueFromJson,
/* 461 */   &CIntDriver::ex_JavaScript_getValue,
/* 462 */   &CIntDriver::ex_JavaScript_setValue,
/* 463 */   &CIntDriver::ex_JavaScript_UserFunctionCall,
/* 464 */   &CIntDriver::ex_TextTemplate_write_writeEncoded_writeEncodedLine_writeLine, // Report/StringWriter.write
/* 465 */   &CIntDriver::ex_TextTemplate_write_writeEncoded_writeEncodedLine_writeLine, // Report/StringWriter.writeEncoded
/* 466 */   &CIntDriver::ex_TextTemplate_write_writeEncoded_writeEncodedLine_writeLine, // Report/StringWriter.writeEncodedLine
/* 467 */   &CIntDriver::ex_TextTemplate_write_writeEncoded_writeEncodedLine_writeLine, // Report/StringWriter.writeLine
/* 468 */   &CIntDriver::ex_StringWriter_toString,
/* 469 */   &CIntDriver::ex_Image_getExif,
/* 470 */   &CIntDriver::ex_StringWriter_clear,
/* 471 */   &CIntDriver::ex_Video_compute,
/* 472 */   &CIntDriver::ex_Video_clear,
/* 473 */   &CIntDriver::ex_Video_load,
/* 474 */   &CIntDriver::ex_Video_save,
/* 475 */   &CIntDriver::ex_Video_length,
/* 476 */   &CIntDriver::ex_Video_width_height, // Video.width
/* 477 */   &CIntDriver::ex_Video_width_height, // Video.height


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


void CIntDriver::EvaluateApplicationStartupJavaScript()
{
    ASSERT(m_engineData->javascript_processor != nullptr);

    std::optional<std::string> old_message_source;

    if( m_pEngineDriver->GetLister() != nullptr )
        old_message_source = m_pEngineDriver->GetLister()->SetMessageSource("[JavaScript Evaluation (Application Startup)]");

    EngineJavaScriptProcessor& javascript_processor = m_engineData->GetJavaScriptProcessor();
    javascript_processor.EvaluateApplicationStartupBytecode();

    if( old_message_source.has_value() )
        m_pEngineDriver->GetLister()->SetMessageSource(std::move(*old_message_source));
}


bool CIntDriver::ExecuteSymbolProcs(const Symbol& symbol, const ProcType proc_type)
{
    bool bRequestIssued = false;

    if( m_bStopProc )
        return bRequestIssued;

    // If there is a field (the first field) before a roster and we arrive to this field
    // from a previous endsect. m_bSkipStmt remains as true when the field doesn't have proc.
    // So the PreProc of the Roster is not executed (see DeSetNextField GroupCompletion
    // is not called when m_bSkipStmt is true.!!!
    m_bSkipStmt = false;

    const RunnableSymbol& runnable_symbol = dynamic_cast<const RunnableSymbol&>(symbol);
    int program_index = runnable_symbol.GetProcIndex(proc_type);

    if( program_index != -1 )
    {
        // setup execution parameters
        m_procType = proc_type;
        m_iExSymbol = symbol.GetSymbolIndex();
        m_iExLevel = SymbolCalculator::GetLevelNumber_base1(symbol);

        m_bSkipStmt = false;
        m_bStopExec = m_bStopProc;

        // reset RequestIssued
        SetRequestIssued(false);

        // update the trace window
        if( m_traceHandler != nullptr )
        {
            m_traceHandler->Output(FormatText("Entering %s (%s)...", symbol.GetName().c_str(), GetProcTypeName(proc_type)),
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
            m_traceHandler->Output(FormatText("Exiting %s (%s)...", symbol.GetName().c_str(), GetProcTypeName(proc_type)),
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
    while( program_index >= 0 && !m_bStopExec )
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
        m_bStopExec = ( m_bSkipStmt || m_bStopProc );

        if( GetRequestIssued() )
            return true;
    }

    // no request issued
    return false;
}


void CIntDriver::ResetSymbol(Symbol& symbol, const int initialize_value/* = -1*/)
{
    const bool has_initialize_value = ( initialize_value >= 0 );

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

        if( m_engineData->PredatesCompiledLogicVersion(Serializer::Iteration_8_0_000_1) )
        {
            auto old_node = (const int*)array_values;
            int repeat_values = old_node[0];
            int number_values = old_node[1];
            pre80_array_values_node = std::make_unique_for_overwrite<int[]>(number_values + 2);
            pre80_array_values_node[0] = number_values + 1;
            pre80_array_values_node[1] = repeat_values;
            memcpy(pre80_array_values_node.get() + 2, old_node + 2, number_values * sizeof(int));
            array_values = reinterpret_cast<const Nodes::List*>(pre80_array_values_node.get());
        }

        ASSERT(array_values->number_elements >= 2);

        const bool repeat_values = ( array_values->elements[0] == 1 );

        if( logic_array.IsNumeric() )
        {
            std::vector<double> initial_values;

            for( int i = 1; i < array_values->number_elements; ++i )
                initial_values.emplace_back(evalexpr(array_values->elements[i]));

            logic_array.SetInitialValues(std::move(initial_values), repeat_values);
        }

        else
        {
            std::vector<SharableString> initial_values;

            for( int i = 1; i < array_values->number_elements; ++i )
                initial_values.emplace_back(EvaluateSharableString(array_values->elements[i]));

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
            value = EvalAlphaExprCS(initialize_value);

        value = CIMSAString::MakeExactLength(value, pVarT->GetLength());

        TCHAR* buffer = (TCHAR*)svaraddr(pVarT->GetVarX());
        _tmemcpy(buffer, value.GetBuffer(), pVarT->GetLength());
    }


    // special processing for work strings
    else if( symbol.IsA(SymbolType::WorkString) )
    {
        ASSERT(has_initialize_value);
        WorkString& work_string = assert_cast<WorkString&>(symbol);
        work_string.SetString(EvaluateSharableString(initialize_value));
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
            const char* const special_function_name = ToString(special_function);
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
                    const SymbolType symbol_type = NPT(symbol_index)->GetType();

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


double CIntDriver::ExecSpecialFunction(const int iSymVar, const SpecialFunction special_function,
                                       std::vector<std::variant<double, SharableString>> arguments)
{
    const DataType return_type = ( special_function == SpecialFunction::OnSyncMessage ||
                                   special_function == SpecialFunction::OnActionInvokerResult ) ? DataType::String : DataType::Numeric;

    UserFunction* const user_function = GetSpecialFunctions()[static_cast<size_t>(special_function)];

    if( user_function == nullptr || user_function->GetProgramIndex() < 0 )
        return AssignInvalidValue(return_type);

    // Now Execute the code
    m_bSkipStmt = false; // RHF Sep 20, 2000.
    //If there is a field (the first field) before a roster and we arrive to this field
    // from a previous endsect. m_bSkipStmt remains true when the field doesn't have proc.
    // So the PreProc of the Roster is not executed (see DeSetNextField GroupCompletion
    // is not called when m_bSkipStmt is true.!!!
    if( m_bStopProc )
        return AssignInvalidValue(return_type);

    // TODO: make sure that all functions can work properly when m_iExSymbol is 0; for
    // now only allow this in OnSystemMessage because that is an obscure feature (and if
    // m_iExSymbol is 0, we will activate the special function checking that keeps things
    // like movement statements from executing)
    if( iSymVar <= 0 && special_function != SpecialFunction::OnSystemMessage )
        return AssignInvalidValue(return_type);

    const RAII::SetValueAndRestoreOnDestruction proc_type_modifier(m_procType, ProcType::OnFocus);
    const RAII::SetValueAndRestoreOnDestruction symbol_modifier(m_iExSymbol, iSymVar);
    const RAII::SetValueAndRestoreOnDestruction level_modifier(m_iExLevel, ( iSymVar > 0 ) ? SymbolCalculator::GetLevelNumber_base1(NPT_Ref(iSymVar)) : 0);

    m_bSkipStmt = false;
    m_bStopExec = m_bStopProc;

    SetRequestIssued( false ); // reset RequestIssued// RHF Dec 03, 2003

    m_bExecSpecFunc = ( special_function == SpecialFunction::GlobalOnFocus || m_iExSymbol <= 0 );

    NumericStringValuesOnlyUserFunctionArgumentEvaluator<false> argument_evaluator(std::move(arguments));
    const double return_value = CallUserFunction(*user_function, argument_evaluator);

    m_bExecSpecFunc = false;

    m_bStopExec = false;

    return return_value;
}


bool CIntDriver::ExecuteOnSystemMessage(const MessageType message_type, const int message_number, const std::string& message_text)
{
    const UserFunction* const user_function = GetSpecialFunctions()[static_cast<size_t>(SpecialFunction::OnSystemMessage)];
    ASSERT(user_function != nullptr);

    // OnSystemMessage can have one to three arguments (up to two numerics and one string);
    // if only one numeric argument, the message number is provided
    std::vector<std::variant<double, SharableString>> arguments;
    bool on_first_numeric = true;

    for( const int symbol_index : user_function->GetParameterSymbolIndices() )
    {
        if( NPT_Ref(symbol_index).IsA(SymbolType::WorkVariable) )
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


void CIntDriver::RunGlobalOnFocus(const int symbol_index)
{
    if( Issamod != ModuleType::Entry )
        return;

    ASSERT(NPT_Ref(symbol_index).IsA(SymbolType::Variable));

    // run On_Focus
    if( HasSpecialFunction(SpecialFunction::GlobalOnFocus) )
        ExecSpecialFunction(symbol_index, SpecialFunction::GlobalOnFocus, { double(symbol_index) });

    // when the field specifies a particular keyboard to use, update the keyboard input
    m_keyboardLoader->Activate(VPT(symbol_index)->GetKeyboardLayoutId());
}


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



// --------------------------------------------------------------------------
// INTERPRETER_DLL_TODO...
// --------------------------------------------------------------------------

double CIntDriver::evalexpr_INTERPRETER_DLL_TODO(const int program_index)
{
    return evalexpr(program_index);
}


void CIntDriver::RegisterAndLogEvent_INTERPRETER_DLL_TODO(std::shared_ptr<Paradata::Event> event, const void* instance_object/* = nullptr*/)
{
    ASSERT(m_paradataDriver != nullptr);
    m_paradataDriver->RegisterAndLogEvent(std::move(event), instance_object);
}


void CIntDriver::IssueMessageWorker(const MessageType message_type, const int message_number, ...)
{
    va_list parg;
    va_start(parg, message_number);
    m_pEngineDriver->GetSystemMessageIssuer().IssueVA(message_type, message_number, parg);
    va_end(parg);
}


std::string CIntDriver::GetFormattedMessageWorker(const int message_number, ...)
{
    va_list parg;
    va_start(parg, message_number);
    std::string message = m_pEngineDriver->GetSystemMessageIssuer().GetFormattedMessageVA(message_number, parg);
    va_end(parg);

    return message;
}


#include "EngineExecutor.h"
#include <zToolsO/ValueConserver.h>
bool CIntDriver::Report_Evaluate_INTERPRETER_DLL_TODO(Report& report)
{
    return Execute(
        [&]()
        {
            // run the code to generate the report
            ValueConserver field_symbol_index_conserver(m_FieldSymbol, m_iExSymbol);
            ValueConserver execution_symbol_index_conserver(m_iExSymbol, report.GetSymbolIndex());

            ExecuteProgramStatements(report.GetProgramIndex());
        });
}


void CIntDriver::ModifySymbolValue_double_INTERPRETER_DLL_TODO(const Nodes::SymbolValue& symbol_value_node, const std::function<void(double&)>& modify_value_function)
{
    ModifySymbolValue<double>(symbol_value_node, modify_value_function);
}


bool CIntDriver::AssignValueToSymbol_INTERPRETER_DLL_TODO(const Nodes::SymbolValue& symbol_value_node, const double value)
{
    return AssignValueToSymbol(symbol_value_node, value);
}


bool CIntDriver::AssignValueToSymbol_INTERPRETER_DLL_TODO(const Nodes::SymbolValue& symbol_value_node, SharableString value)
{
    return AssignValueToSymbol(symbol_value_node, std::move(value));
}


double CIntDriver::RunSoonToBeRemoveFeature(const std::string_view feature_sv, const int program_index, void* /*tag*/)
{
    if( feature_sv == "prompt_pre77" )
    {
        return exprompt_pre77(program_index);
    }

    else if( feature_sv == "exaccept_pre77" )
    {
        return exaccept_pre77(program_index);
    }

    else
    {
        return ReturnProgrammingError(0.0);
    }
}


bool CIntDriver::IsExecutionInterrupted() const
{
    return ( m_caughtProgramControlException ||
             m_bStopExec ||
             m_bStopProc ||
             GetRequestIssued() );
}


EngineParadataDriver& CIntDriver::GetEngineParadataDriver_INTERPRETER_DLL_TODO()
{
    return *m_paradataDriver;
}
