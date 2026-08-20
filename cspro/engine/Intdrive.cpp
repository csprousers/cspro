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
#include "SelcaseManager.h"
#include <zPlatformO/PlatformInterface.h>
#include <zLogicO/SpecialFunction.h>
#include <zEngineO/AllSymbols.h>
#include <zEngineO/EngineAccessor.h>
#include <zEngineO/JavaScriptProcessor.h>
#include <zEngineO/LoopStack.h>
#include <zEngineO/SaveArrayFile.h>
#include <zEngineO/UserFunctionArgumentEvaluator.h>
#include <zEngineO/Interpreter/ProgramControlException.h>
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
    AddIntDriverInstructions();

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


void CIntDriver::AddIntDriverInstructions()
{
#ifdef _DEBUG
    size_t op_code_counter = 0;
#endif

// OP_ENGVAL = instructions defined in LogicInterpreter that return Engine::Value
#define OP_ENGVAL(op_code, function) \
    ASSERT(op_code == op_code_counter++); \
    ASSERT(m_instructions[static_cast<size_t>(op_code)].index() == 0); \
    ASSERT(std::get<0>(m_instructions[static_cast<size_t>(op_code)]) == static_cast<Engine::Value (CIntDriver::*)(int)>(&CIntDriver::function)); \
    ASSERT(std::get<0>(m_instructions[static_cast<size_t>(op_code)]) != &CIntDriver::ex_unimplemented_LogicInterpreter);

// OP_ENGVAL_ID = instructions defined in CIntDriver that return Engine::Value
#define OP_ENGVAL_ID(op_code, function) \
    { \
        ASSERT(op_code == op_code_counter++); \
        Instruction& instruction = m_instructions[static_cast<size_t>(op_code)]; \
        ASSERT(instruction.index() == 0 && std::get<0>(instruction) == &CIntDriver::ex_unimplemented_LogicInterpreter); \
        instruction = static_cast<Engine::Value (CIntDriver::*)(int)>(&CIntDriver::function); \
    }

// OP_DOUBLE = instructions defined in LogicInterpreter or CIntDriver that return double
#define OP_DOUBLE(op_code, function) \
    { \
        ASSERT(op_code == op_code_counter++); \
        Instruction& instruction = m_instructions[static_cast<size_t>(op_code)]; \
        const bool is_undefined = ( instruction.index() == 0 && std::get<0>(instruction) == &CIntDriver::ex_unimplemented_LogicInterpreter ); \
        ASSERT(is_undefined || ( instruction.index() == 1 && std::get<1>(instruction) == static_cast<double (CIntDriver::*)(int)>(&CIntDriver::function) )); \
        if( is_undefined ) \
            instruction = static_cast<double (CIntDriver::*)(int)>(&CIntDriver::function); \
    }

    OP_ENGVAL(0, ex_numeric_constant);
    OP_DOUBLE(1, exsvar);
    OP_DOUBLE(2, exmvar);
    OP_DOUBLE(3, excpt);
    OP_ENGVAL(4, ex_add);
    OP_ENGVAL(5, ex_sub);
    OP_ENGVAL(6, ex_mult);
    OP_ENGVAL(7, ex_div);
    OP_ENGVAL(8, ex_mod);
    OP_ENGVAL(9, ex_minus);
    OP_ENGVAL(10, ex_exp);
    OP_ENGVAL(11, ex_or);
    OP_ENGVAL(12, ex_and);
    OP_ENGVAL(13, ex_not);
    OP_ENGVAL(14, ex_eq);
    OP_ENGVAL(15, ex_ne);
    OP_ENGVAL(16, ex_le);
    OP_ENGVAL(17, ex_lt);
    OP_ENGVAL(18, ex_ge);
    OP_ENGVAL(19, ex_gt);
    OP_ENGVAL(20, ex_equ);
    OP_ENGVAL(21, ex_string_compute);
    OP_ENGVAL(22, ex_WorkVariable_evaluate);
    OP_ENGVAL(23, ex_if);
    OP_ENGVAL(24, ex_while);
    OP_DOUBLE(25, exbox);
    OP_ENGVAL(26, ex_string_literal);
    OP_ENGVAL_ID(27, excharobj);
    OP_ENGVAL(28, ex_string_eq);
    OP_ENGVAL(29, ex_string_ne);
    OP_ENGVAL(30, ex_string_le);
    OP_ENGVAL(31, ex_string_lt);
    OP_ENGVAL(32, ex_string_ge);
    OP_ENGVAL(33, ex_string_gt);
    OP_DOUBLE(34, excpttbl);
    OP_ENGVAL(35, ex_nop_abort);
    OP_ENGVAL(36, ex_nop_abort);
    OP_ENGVAL_ID(37, ex_UserFunction_call);
    OP_ENGVAL(38, ex_nop_abort);
    OP_ENGVAL(39, ex_nop_abort);
    OP_DOUBLE(40, exskipto);
    OP_DOUBLE(41, exadvance);
    OP_DOUBLE(42, exreenter);
    OP_DOUBLE(43, exnoinput);
    OP_DOUBLE(44, exendsect);
    OP_DOUBLE(45, exendlevl);
    OP_DOUBLE(46, exenter);
    OP_DOUBLE(47, exskipcase);
    OP_ENGVAL(48, ex_nop_abort);
    OP_DOUBLE(49, exstop);
    OP_ENGVAL(50, ex_nop_ignore);
    OP_DOUBLE(51, exctab);
    OP_ENGVAL(52, ex_nop_abort);
    OP_DOUBLE(53, exbreak);
    OP_DOUBLE(54, exexport);
    OP_DOUBLE(55, exset);
    OP_DOUBLE(56, exvisualvalue);
    OP_DOUBLE(57, exhighlight);
    OP_ENGVAL(58, ex_sqrt);
    OP_ENGVAL(59, ex_ex);
    OP_ENGVAL(60, ex_int);
    OP_ENGVAL(61, ex_log);
    OP_ENGVAL(62, ex_seed);
    OP_ENGVAL(63, ex_random);
    OP_DOUBLE(64, exnoccurs);
    OP_DOUBLE(65, exsoccurs_pre80);
    OP_ENGVAL(66, ex_nop_abort);
    OP_DOUBLE(67, excount);
    OP_DOUBLE(68, exsum);
    OP_DOUBLE(69, exavrge);
    OP_DOUBLE(70, exmin);
    OP_DOUBLE(71, exmax);
    OP_DOUBLE(72, exdisplay);
    OP_DOUBLE(73, exerrmsg);
    OP_ENGVAL(74, ex_concat);
    OP_ENGVAL(75, ex_tonumber);
    OP_ENGVAL(76, ex_pos_poschar);
    OP_ENGVAL(77, ex_compare);
    OP_ENGVAL(78, ex_length);
    OP_ENGVAL(79, ex_strip);
    OP_ENGVAL(80, ex_pos_poschar);
    OP_ENGVAL(81, ex_edit);
    OP_ENGVAL(82, ex_cmcode);
    OP_ENGVAL(83, ex_setlb_setub);
    OP_ENGVAL(84, ex_setlb_setub);
    OP_ENGVAL(85, ex_adjuba);
    OP_ENGVAL(86, ex_adjlba);
    OP_ENGVAL(87, ex_adjlbi);
    OP_ENGVAL(88, ex_adjubi);
    OP_ENGVAL(89, ex_nop_abort);
    OP_ENGVAL(90, ex_systime);
    OP_ENGVAL(91, ex_sysdate);
    OP_DOUBLE(92, exdemode);
    OP_ENGVAL(93, ex_special);
    OP_ENGVAL(94, ex_accept);
    OP_DOUBLE(95, exclrcase);
    OP_DOUBLE(96, exxtab);
    OP_DOUBLE(97, extblcoord);
    OP_DOUBLE(98, extblcoord);
    OP_DOUBLE(99, extblcoord);
    OP_DOUBLE(100, extblsum);
    OP_DOUBLE(101, extblmed);
    OP_ENGVAL(102, ex_filename);
    OP_ENGVAL(103, ex_nop_abort);
    OP_ENGVAL(104, ex_nop_abort);
    OP_DOUBLE(105, exloadcase);
    OP_ENGVAL(106, ex_nop_abort);
    OP_DOUBLE(107, exretrieve);
    OP_ENGVAL(108, ex_nop_abort);
    OP_DOUBLE(109, exwritecase);
    OP_DOUBLE(110, exdelcase);
    OP_DOUBLE(111, exfind_locate);
    OP_ENGVAL_ID(112, ex_key_currentkey);
    OP_ENGVAL_ID(113, ex_open);
    OP_ENGVAL_ID(114, ex_close);
    OP_DOUBLE(115, exfind_locate);
    OP_ENGVAL(116, ex_nop_abort);
    OP_ENGVAL(117, ex_nop_abort);
    OP_ENGVAL(118, ex_nop_ignore);
    OP_ENGVAL(119, ex_nop_abort);
    OP_ENGVAL(120, ex_nop_ignore);
    OP_DOUBLE(121, exsetattr);
    OP_ENGVAL(122, ex_nop_abort);
    OP_DOUBLE(123, exfor_dict);
    OP_DOUBLE(124, exnmembers);
    OP_ENGVAL(125, ex_nop_abort);
    OP_ENGVAL(126, ex_nop_abort);
    OP_ENGVAL(127, ex_minvalue_maxvalue);
    OP_ENGVAL(128, ex_minvalue_maxvalue);
    OP_DOUBLE(129, exfor_group);
    OP_ENGVAL(130, ex_nop_abort);
    OP_ENGVAL(131, ex_nop_abort);
    OP_ENGVAL(132, ex_functionCall);
    OP_ENGVAL(133, ex_in);
    OP_ENGVAL(134, ex_do);
    OP_DOUBLE(135, ex_impute);
    OP_DOUBLE(136, exfncurocc);
    OP_DOUBLE(137, exfntotocc);
    OP_DOUBLE(138, exupdate);
    OP_DOUBLE(139, exwrite);
    OP_ENGVAL(140, ex_nop_abort);
    OP_DOUBLE(141, exfor_relation);
    OP_ENGVAL(142, ex_nop_abort);
    OP_ENGVAL_ID(143, exgetbuffer);
    OP_DOUBLE(144, exinsert_delete);
    OP_DOUBLE(145, exinsert_delete);
    OP_DOUBLE(146, exsort);
    OP_ENGVAL_ID(147, exgetlabel);
    OP_ENGVAL_ID(148, exgetlabel);
    OP_ENGVAL(149, ex_nop_abort);
    OP_ENGVAL(150, ex_nop_abort);
    OP_ENGVAL(151, ex_nop_abort);
    OP_ENGVAL_ID(152, exmaketext);
    OP_DOUBLE(153, exmoveto);
    OP_ENGVAL(154, ex_nop_abort);
    OP_ENGVAL_ID(155, ex_getoperatorid);
    OP_ENGVAL(156, ex_for_next);
    OP_ENGVAL(157, ex_for_break);
    OP_ENGVAL_ID(158, ex_setfile);
    OP_DOUBLE(159, exmaxocc_pre80);
    OP_ENGVAL(160, ex_invalueset);
    OP_ENGVAL(161, ex_setvalueset);
    OP_ENGVAL(162, ex_filecreate);
    OP_ENGVAL(163, ex_fileexist);
    OP_ENGVAL(164, ex_filedelete);
    OP_ENGVAL(165, ex_filecopy_filerename);
    OP_ENGVAL(166, ex_filecopy_filerename);
    OP_ENGVAL(167, ex_filesize);
    OP_ENGVAL(168, ex_fileconcat);
    OP_ENGVAL(169, ex_File_read);
    OP_ENGVAL(170, ex_File_write);
    OP_ENGVAL_ID(171, ExExecSystem);
    OP_ENGVAL(172, ex_nop_abort);
    OP_DOUBLE(173, exshowlist);
    OP_ENGVAL(174, ex_tolower_toupper);
    OP_ENGVAL(175, ex_tolower_toupper);
    OP_DOUBLE(176, excountvalid);
    OP_ENGVAL(177, ex_nop_ignore);
    OP_DOUBLE(178, exswap);
    OP_ENGVAL(179, ex_datediff);
    OP_DOUBLE(180, exdeckarray);
    OP_DOUBLE(181, exdeckarray);
    OP_ENGVAL_ID(182, ex_getlanguage);
    OP_ENGVAL_ID(183, ex_setlanguage);
    OP_DOUBLE(184, exendcase);
    OP_ENGVAL(185, ex_userbar);
    OP_DOUBLE(186, exmessageoverrides);
    OP_ENGVAL(187, ex_trace);
    OP_ENGVAL(188, ex_setvaluesets);
    OP_ENGVAL_ID(189, ExExecPFF);
    OP_DOUBLE(190, exseek);
    OP_DOUBLE(191, ex_getcapturetype);
    OP_DOUBLE(192, ex_setcapturetype);
    OP_ENGVAL(193, ex_setfont);
    OP_ENGVAL(194, ex_getorientation_setorientation);
    OP_ENGVAL(195, ex_getorientation_setorientation);
    OP_ENGVAL(196, ex_pathname);
    OP_DOUBLE(197, exgps);
    OP_ENGVAL(198, ex_low_high);
    OP_ENGVAL(199, ex_low_high);
    OP_ENGVAL_ID(200, ex_getrecord);
    OP_DOUBLE(201, ex_setcapturepos);
    OP_ENGVAL(202, ex_abs);
    OP_ENGVAL(203, ex_randomin);
    OP_ENGVAL(204, ex_randomizevs);
    OP_ENGVAL(205, ex_getusername);
    OP_ENGVAL(206, ex_fileempty);
    OP_DOUBLE(207, ex_changekeyboard);
    OP_DOUBLE(208, ex_setoutput);
    OP_DOUBLE(209, exseekMinMax);
    OP_DOUBLE(210, exseekMinMax);
    OP_ENGVAL(211, ex_dateadd);
    OP_ENGVAL(212, ex_datevalid);
    OP_ENGVAL(213, ex_getos);
    OP_ENGVAL_ID(214, ex_getocclabel);
    OP_ENGVAL(215, ex_nop_ignore);
    OP_DOUBLE(216, exsetvalue);
    OP_DOUBLE(217, exgetvalue);
    OP_ENGVAL_ID(218, ex_getvaluealpha);
    OP_ENGVAL(219, ex_nop_abort);
    OP_DOUBLE(220, exsetocclabel);
    OP_DOUBLE(221, exshowocc);
    OP_DOUBLE(222, exshowocc);
    OP_ENGVAL(223, ex_getdeviceid);
    OP_ENGVAL(224, ex_direxist);
    OP_ENGVAL(225, ex_dircreate);
    OP_ENGVAL(226, ex_nop_abort);
    OP_ENGVAL(227, ex_List_var);
    OP_ENGVAL(228, ex_dirlist);
    OP_ENGVAL(229, ex_sysparm);
    OP_ENGVAL(230, ex_connection);
    OP_ENGVAL(231, ex_prompt);
    OP_ENGVAL(232, ex_getimage);
    OP_ENGVAL(233, ex_round);
    OP_ENGVAL(234, ex_nop_abort);
    OP_DOUBLE(235, exsavepartial);
    OP_DOUBLE(236, ex_syncconnect);
    OP_DOUBLE(237, ex_syncdisconnect);
    OP_DOUBLE(238, ex_syncdata);
    OP_DOUBLE(239, ex_syncfile);
    OP_DOUBLE(240, ex_syncserver);
    OP_ENGVAL(241, ex_savesetting);
    OP_ENGVAL(242, ex_loadsetting);
    OP_ENGVAL_ID(243, ex_getcaselabel);
    OP_DOUBLE(244, exsetcaselabel);
    OP_DOUBLE(245, exispartial);
    OP_DOUBLE(246, exsetoperatorid);
    OP_ENGVAL_ID(247, exgetnote);
    OP_ENGVAL_ID(248, exeditnote);
    OP_ENGVAL_ID(249, exputnote);
    OP_DOUBLE(250, exisverified);
    OP_DOUBLE(251, exforcase);
    OP_ENGVAL(252, ex_timestamp);
    OP_DOUBLE(253, exkeylist);
    OP_ENGVAL(254, ex_diagnostics);
    OP_ENGVAL(255, ex_compress);
    OP_ENGVAL(256, ex_decompress);
    OP_DOUBLE(257, exask);
    OP_DOUBLE(258, excountcases);
    OP_ENGVAL_ID(259, ex_getproperty);
    OP_ENGVAL_ID(260, ex_setproperty);
    OP_DOUBLE(261, exlogtext);
    OP_DOUBLE(262, exwarning);
    OP_ENGVAL_ID(263, ex_tr);
    OP_ENGVAL(264, ex_uuid);
    OP_ENGVAL(265, ex_paradata);
    OP_DOUBLE(266, exsqlquery);
    OP_DOUBLE(267, expre77_report);
    OP_DOUBLE(268, expre77_setreportdata);
    OP_DOUBLE(269, exshow);
    OP_DOUBLE(270, exshowarray);
    OP_DOUBLE(271, exselcase);
    OP_ENGVAL(272, ex_timestring);
    OP_ENGVAL(273, ex_string_literal);
    OP_DOUBLE(274, exsymbolreset);
    OP_ENGVAL(275, ex_decryptstring);
    OP_ENGVAL(276, ex_dirdelete);
    OP_ENGVAL(277, ex_Array_var);
    OP_DOUBLE(278, extvar);
    OP_ENGVAL(279, ex_exit);
    OP_ENGVAL_ID(280, ex_getbluetoothname);
    OP_ENGVAL(281, ex_regexmatch);
    OP_ENGVAL(282, ex_nop_abort);
    OP_ENGVAL_ID(283, ex_getvaluelabel);
    OP_ENGVAL(284, ex_Array_clear);
    OP_ENGVAL(285, ex_Array_length);
    OP_ENGVAL(286, ex_Map_show);
    OP_ENGVAL(287, ex_Map_hide);
    OP_ENGVAL(288, ex_Map_addMarker);
    OP_ENGVAL(289, ex_Map_setMarkerImage);
    OP_ENGVAL(290, ex_Map_setMarkerText);
    OP_ENGVAL(291, ex_Map_setMarkerOnClick_setMarkerOnClickInfo);
    OP_ENGVAL(292, ex_Map_setMarkerOnClick_setMarkerOnClickInfo);
    OP_ENGVAL(293, ex_Map_setMarkerDescription);
    OP_ENGVAL(294, ex_Map_setMarkerOnDrag);
    OP_ENGVAL(295, ex_Map_setMarkerLocation);
    OP_ENGVAL(296, ex_Map_getMarkerLatitude_getMarkerLongitude);
    OP_ENGVAL(297, ex_Map_removeMarker);
    OP_ENGVAL(298, ex_Map_setOnClick);
    OP_ENGVAL(299, ex_Map_showCurrentLocation);
    OP_ENGVAL(300, ex_Map_addTextButton);
    OP_ENGVAL(301, ex_Map_addImageButton);
    OP_ENGVAL(302, ex_Map_removeButton);
    OP_ENGVAL(303, ex_Map_setBaseMap);
    OP_ENGVAL(304, ex_Map_setTitle);
    OP_ENGVAL(305, ex_Map_zoomTo);
    OP_ENGVAL(306, ex_List_add);
    OP_ENGVAL(307, ex_List_clear);
    OP_ENGVAL(308, ex_List_insert);
    OP_ENGVAL(309, ex_List_length);
    OP_ENGVAL(310, ex_List_remove);
    OP_ENGVAL(311, ex_List_seek);
    OP_ENGVAL(312, ex_List_show);
    OP_ENGVAL(313, ex_List_compute);
    OP_ENGVAL(314, ex_ValueSet_add);
    OP_ENGVAL(315, ex_ValueSet_clear);
    OP_ENGVAL(316, ex_ValueSet_remove);
    OP_ENGVAL(317, ex_ValueSet_show);
    OP_ENGVAL(318, ex_ValueSet_compute);
    OP_ENGVAL_ID(319, ex_variablevalue);
    OP_ENGVAL(320, ex_Map_clear_clearButtons_clearGeometry_clearMarkers);
    OP_ENGVAL(321, ex_Map_clear_clearButtons_clearGeometry_clearMarkers);
    OP_ENGVAL(322, ex_Map_getLastClickLatitude_getLastClickLongitude);
    OP_ENGVAL(323, ex_Map_getLastClickLatitude_getLastClickLongitude);
    OP_ENGVAL(324, ex_Map_getMarkerLatitude_getMarkerLongitude);
    OP_ENGVAL(325, ex_Path_concat);
    OP_ENGVAL(326, ex_view);
    OP_ENGVAL(327, ex_Pff_exec);
    OP_ENGVAL(328, ex_Pff_getProperty);
    OP_ENGVAL(329, ex_Pff_load);
    OP_ENGVAL(330, ex_Pff_save);
    OP_ENGVAL(331, ex_Pff_setProperty);
    OP_ENGVAL(332, ex_ValueSet_length);
    OP_ENGVAL(333, ex_ischecked);
    OP_ENGVAL_ID(334, ex_protect);
    OP_ENGVAL(335, ex_when);
    OP_DOUBLE(336, ex_syncapp);
    OP_ENGVAL(337, ex_filetime);
    OP_ENGVAL(338, ex_recode);
    OP_DOUBLE(339, exforcase);
    OP_DOUBLE(340, exselcase);
    OP_DOUBLE(341, excountcases);
    OP_DOUBLE(342, exkeylist);
    OP_ENGVAL(343, ex_Barcode_read);
    OP_ENGVAL(344, ex_hash);
    OP_ENGVAL_ID(345, ex_syncmessage);
    OP_ENGVAL(346, ex_SystemApp_clear);
    OP_ENGVAL(347, ex_SystemApp_setArgument);
    OP_ENGVAL(348, ex_SystemApp_getResult);
    OP_ENGVAL(349, ex_SystemApp_exec);
    OP_ENGVAL(350, ex_startswith);
    OP_ENGVAL(351, ex_Pff_compute);
    OP_ENGVAL(352, ex_Audio_clear);
    OP_ENGVAL(353, ex_Audio_concat);
    OP_ENGVAL(354, ex_Audio_load);
    OP_ENGVAL(355, ex_Audio_play);
    OP_ENGVAL(356, ex_Audio_save);
    OP_ENGVAL(357, ex_Audio_stop);
    OP_ENGVAL(358, ex_Audio_record);
    OP_ENGVAL(359, ex_Audio_recordInteractive);
    OP_ENGVAL(360, ex_Audio_compute);
    OP_ENGVAL(361, ex_encode);
    OP_ENGVAL(362, ex_List_sort);
    OP_ENGVAL(363, ex_List_removeDuplicates);
    OP_ENGVAL(364, ex_List_removeIn);
    OP_ENGVAL(365, ex_Path_concat);
    OP_ENGVAL(366, ex_Path_getDirectoryName);
    OP_ENGVAL(367, ex_Path_getExtension);
    OP_ENGVAL(368, ex_Path_getFileName);
    OP_ENGVAL(369, ex_Path_getFileNameWithoutExtension);
    OP_DOUBLE(370, ex_syncparadata);
    OP_ENGVAL(371, ex_HashMap_var);
    OP_ENGVAL(372, ex_HashMap_compute);
    OP_ENGVAL(373, ex_HashMap_clear);
    OP_ENGVAL(374, ex_HashMap_contains);
    OP_ENGVAL(375, ex_HashMap_length);
    OP_ENGVAL(376, ex_HashMap_remove);
    OP_ENGVAL(377, ex_HashMap_getKeys);
    OP_ENGVAL(378, ex_Audio_length);
    OP_ENGVAL(379, ex_ValueSet_sort);
    OP_ENGVAL(380, ex_replace);
    OP_ENGVAL(381, ex_inc);
    OP_DOUBLE(382, exuniverse);
    OP_DOUBLE(383, ex_Freq_unnamed);
    OP_DOUBLE(384, ex_Freq_clear);
    OP_DOUBLE(385, ex_Freq_save);
    OP_DOUBLE(386, ex_Freq_tally);
    OP_ENGVAL_ID(387, ex_Freq_view);
    OP_DOUBLE(388, ex_Freq_var);
    OP_DOUBLE(389, ex_Freq_compute);
    OP_ENGVAL(390, ex_WorkString_evaluate);
    OP_DOUBLE(391, exmaxocc);
    OP_DOUBLE(392, exsoccurs);
    OP_ENGVAL_ID(393, exDataAccessValidityCheck);
    OP_DOUBLE(394, exdictcompute);
    OP_ENGVAL_ID(395, ex_key_currentkey);
    OP_ENGVAL(396, ex_Image_compute);
    OP_ENGVAL(397, ex_Image_captureSignature_takePhoto);
    OP_ENGVAL(398, ex_Image_clear);
    OP_ENGVAL(399, ex_Image_width_height);
    OP_ENGVAL(400, ex_Image_load);
    OP_ENGVAL(401, ex_Image_resample);
    OP_ENGVAL(402, ex_Image_save);
    OP_ENGVAL(403, ex_Image_captureSignature_takePhoto);
    OP_ENGVAL(404, ex_Image_view);
    OP_ENGVAL(405, ex_Image_width_height);
    OP_ENGVAL(406, ex_Document_compute);
    OP_ENGVAL(407, ex_Document_clear);
    OP_ENGVAL(408, ex_Document_load);
    OP_ENGVAL(409, ex_Document_save);
    OP_ENGVAL(410, ex_Document_view);
    OP_ENGVAL(411, ex_Geometry_compute);
    OP_ENGVAL(412, ex_Geometry_clear);
    OP_ENGVAL(413, ex_Geometry_load);
    OP_ENGVAL(414, ex_Geometry_save);
    OP_ENGVAL(415, ex_Map_addGeometry);
    OP_ENGVAL(416, ex_Map_removeGeometry);
    OP_ENGVAL(417, ex_Map_clear_clearButtons_clearGeometry_clearMarkers);
    OP_ENGVAL(418, ex_Geometry_tracePolygon_walkPolygon);
    OP_ENGVAL(419, ex_Geometry_tracePolygon_walkPolygon);
    OP_ENGVAL(420, ex_Geometry_area_perimeter);
    OP_ENGVAL(421, ex_Geometry_area_perimeter);
    OP_ENGVAL(422, ex_Geometry_minLatitude_maxLatitude_minLongitude_maxLongitude);
    OP_ENGVAL(423, ex_Geometry_minLatitude_maxLatitude_minLongitude_maxLongitude);
    OP_ENGVAL(424, ex_Geometry_minLatitude_maxLatitude_minLongitude_maxLongitude);
    OP_ENGVAL(425, ex_Geometry_minLatitude_maxLatitude_minLongitude_maxLongitude);
    OP_ENGVAL(426, ex_Geometry_getProperty);
    OP_ENGVAL(427, ex_Geometry_setProperty);
    OP_DOUBLE(428, exinadvance);
    OP_ENGVAL(429, ex_Map_saveSnapshot);
    OP_DOUBLE(430, ex_synctime);
    OP_ENGVAL(431, ex_htmldialog);
    OP_ENGVAL(432, ex_Path_getRelativePath);
    OP_ENGVAL(433, ex_Path_selectFile);
    OP_ENGVAL_ID(434, ex_invoke);
    OP_ENGVAL(435, ex_Report_save);
    OP_ENGVAL(436, ex_Report_view);
    OP_ENGVAL(437, ex_TextTemplate_write_writeEncoded_writeEncodedLine_writeLine);
    OP_ENGVAL_ID(438, ex_setbluetoothname);
    OP_DOUBLE(439, expersistentsymbolreset);
    OP_ENGVAL(440, ex_Symbol_getJson_getValueJson);
    OP_ENGVAL(441, ex_Symbol_getJson_getValueJson);
    OP_ENGVAL(442, ex_Symbol_setValueFromJson);
    OP_ENGVAL(443, ex_Barcode_createQRCode);
    OP_DOUBLE(444, exScopeChange);
    OP_DOUBLE(445, exdictaccess);
    OP_ENGVAL(446, ex_WorkString_compute);
    OP_ENGVAL(447, ex_ActionInvoker);
    OP_ENGVAL(448, ex_Symbol_getName);
    OP_ENGVAL(449, ex_Symbol_getLabel);
    OP_ENGVAL(450, ex_Map_clear_clearButtons_clearGeometry_clearMarkers);
    OP_ENGVAL_ID(451, exItem_hasValue_isValid);
    OP_ENGVAL_ID(452, exItem_getValueLabel);
    OP_ENGVAL_ID(453, exItem_hasValue_isValid);
    OP_ENGVAL(454, ex_compareNoCase);
    OP_ENGVAL_ID(455, exCase_view);
    OP_ENGVAL(456, ex_JavaScript_eval);
    OP_ENGVAL(457, ex_JavaScript_invoke);
    OP_ENGVAL(458, ex_JavaScript_hasValue);
    OP_ENGVAL(459, ex_JavaScript_getValueJson);
    OP_ENGVAL(460, ex_JavaScript_setValueFromJson);
    OP_ENGVAL(461, ex_JavaScript_getValue);
    OP_ENGVAL(462, ex_JavaScript_setValue);
    OP_ENGVAL(463, ex_JavaScript_UserFunctionCall);
    OP_ENGVAL(464, ex_TextTemplate_write_writeEncoded_writeEncodedLine_writeLine);
    OP_ENGVAL(465, ex_TextTemplate_write_writeEncoded_writeEncodedLine_writeLine);
    OP_ENGVAL(466, ex_TextTemplate_write_writeEncoded_writeEncodedLine_writeLine);
    OP_ENGVAL(467, ex_TextTemplate_write_writeEncoded_writeEncodedLine_writeLine);
    OP_ENGVAL(468, ex_StringWriter_toString);
    OP_ENGVAL(469, ex_Image_getExif);
    OP_ENGVAL(470, ex_StringWriter_clear);
    OP_ENGVAL(471, ex_Video_compute);
    OP_ENGVAL(472, ex_Video_clear);
    OP_ENGVAL(473, ex_Video_load);
    OP_ENGVAL(474, ex_Video_save);
    OP_ENGVAL(475, ex_Video_length);
    OP_ENGVAL(476, ex_Video_width_height);
    OP_ENGVAL(477, ex_Video_width_height);
    OP_ENGVAL(478, ex_ValueSet_removeDuplicates);
    OP_ENGVAL(479, ex_WorkVariable_compute);
    OP_ENGVAL(480, ex_Array_compute);
    OP_ENGVAL(481, ex_UserFunction_compute);
    OP_ENGVAL(482, ex_nop_abortFutureFunction);
    OP_ENGVAL(483, ex_nop_abortFutureFunction);
    OP_ENGVAL(484, ex_nop_abortFutureFunction);
    OP_ENGVAL(485, ex_nop_abortFutureFunction);
    OP_ENGVAL(486, ex_nop_abortFutureFunction);
    OP_ENGVAL(487, ex_nop_abortFutureFunction);
    OP_ENGVAL(488, ex_nop_abortFutureFunction);
    OP_ENGVAL(489, ex_nop_abortFutureFunction);
    OP_ENGVAL(490, ex_nop_abortFutureFunction);
    OP_ENGVAL(491, ex_nop_abortFutureFunction);
#undef OP_ENGVAL
#undef OP_ENGVAL_ID
#undef OP_DOUBLE

    ASSERT(op_code_counter == ( MaxInstructionCode_EV_TODO + 1 ));
}


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


template<typename T/* = bool*/>
T CIntDriver::ExecuteProgramStatements(int program_index)
{
    std::optional<Engine::Value> last_evaluated_value;

    // execute a block of statements
    while( program_index >= 0 && !m_bStopExec )
    {
        const auto& statement_node = GetNode<ST_NODE>(program_index);

        try
        {
            last_evaluated_value = ExecuteInstruction(static_cast<FunctionCode>(statement_node.st_code), program_index);
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
            break;
    }

    // return whether a request was issued
    if constexpr(std::is_same_v<T,bool>)
    {
        return GetRequestIssued();
    }

    // return the last evaluated value
    else
    {
        if( last_evaluated_value.has_value() )
            return std::move(*last_evaluated_value);

        return Engine::Value::Undefined<double>();
    }
}


Engine::Value CIntDriver::ExecuteInstructions(const int program_index)
{
    return ExecuteProgramStatements<Engine::Value>(program_index);
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
        ExecuteInstruction(initialize_value);
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
                initial_values.emplace_back(Evaluate<SharableString>(array_values->elements[i]));

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
        work_string.SetString(Evaluate<SharableString>(initialize_value));
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
        // check which of the special functions exists
        for( const SpecialFunction::Definition& special_function : SpecialFunction::GetDefinitions() )
        {
            UserFunction* user_function = nullptr;

            try
            {
                if( GetSymbolTable().NameExists(special_function.name) )
                {
                    user_function = &assert_cast<UserFunction&>(GetSymbolTable().FindSymbolOfType(special_function.name, SymbolType::UserFunction));

                    // make sure the function is defined in a valid way
                    if( special_function.returns != user_function->GetReturnType() ||
                        !special_function.ValidateParameters(user_function->GetParameterSymbolTypes()) )
                    {
                        // in CSPro 8.1, special function are checked in logic
                        ASSERT(m_engineData->PredatesCompiledLogicVersion(Serializer::Iteration_8_1_000_1));
                        user_function = nullptr;
                    }
                }
            }

            catch(...)
            {
                // in the future, perhaps symbols of other types should not be allowed to use the names of special functions
                ASSERT(user_function == nullptr);
            }

            m_specialFunctions.emplace_back(user_function);
        }
    }

    return m_specialFunctions;
}


bool CIntDriver::HasSpecialFunction(const SpecialFunction::Code special_function)
{
    return ( GetSpecialFunctions()[static_cast<size_t>(special_function)] != nullptr );
}


Engine::Value CIntDriver::ExecSpecialFunction(const int iSymVar, const SpecialFunction::Code special_function,
                                              std::vector<std::variant<double, SharableString>> arguments)
{
    UserFunction* const user_function = GetSpecialFunctions()[static_cast<size_t>(special_function)];

    if( user_function == nullptr )
    {
        const SpecialFunction::Definition& definition = SpecialFunction::GetDefinitions()[static_cast<size_t>(special_function)];
        return ReturnProgrammingError(Engine::Value::Invalid(
            ( definition.returns == SymbolType::WorkVariable ) ? DataType::Numeric :
            ( definition.returns == SymbolType::WorkString )   ? DataType::String :
                                                                 ReturnProgrammingError(DataType::Numeric)
        ));
    }

    // if there is no function body, return the default value
    if( user_function->GetProgramIndex() < 0 )
        return Engine::Value::Invalid(user_function->GetReturnDataType());

    // Now Execute the code
    m_bSkipStmt = false; // RHF Sep 20, 2000.
    //If there is a field (the first field) before a roster and we arrive to this field
    // from a previous endsect. m_bSkipStmt remains true when the field doesn't have proc.
    // So the PreProc of the Roster is not executed (see DeSetNextField GroupCompletion
    // is not called when m_bSkipStmt is true.!!!
    if( m_bStopProc )
        return Engine::Value::Invalid(user_function->GetReturnDataType());

    // TODO: make sure that all functions can work properly when m_iExSymbol is 0; for
    // now only allow this in OnSystemMessage because that is an obscure feature (and if
    // m_iExSymbol is 0, we will activate the special function checking that keeps things
    // like movement statements from executing)
    if( iSymVar <= 0 && special_function != SpecialFunction::Code::OnSystemMessage )
        return Engine::Value::Invalid(user_function->GetReturnDataType());

    const RAII::SetValueAndRestoreOnDestruction proc_type_modifier(m_procType, ProcType::OnFocus);
    const RAII::SetValueAndRestoreOnDestruction symbol_modifier(m_iExSymbol, iSymVar);
    const RAII::SetValueAndRestoreOnDestruction level_modifier(m_iExLevel, ( iSymVar > 0 ) ? SymbolCalculator::GetLevelNumber_base1(NPT_Ref(iSymVar)) : 0);

    m_bSkipStmt = false;
    m_bStopExec = m_bStopProc;

    SetRequestIssued( false ); // reset RequestIssued// RHF Dec 03, 2003

    m_bExecSpecFunc = ( special_function == SpecialFunction::Code::GlobalOnFocus || m_iExSymbol <= 0 );

    NumericStringValuesOnlyUserFunctionArgumentEvaluator<false> argument_evaluator(std::move(arguments));
    Engine::Value return_value = CallUserFunction(*user_function, argument_evaluator);

    m_bExecSpecFunc = false;

    m_bStopExec = false;

    return return_value;
}


bool CIntDriver::ExecuteOnSystemMessage(const MessageType message_type, const int message_number, const std::string& message_text)
{
    const UserFunction* const user_function = GetSpecialFunctions()[static_cast<size_t>(SpecialFunction::Code::OnSystemMessage)];
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
        const Engine::Value on_system_message_result = ExecSpecialFunction(
            m_iExSymbol, SpecialFunction::Code::OnSystemMessage, arguments
        );

        ASSERT(on_system_message_result.is<double>());

        issue_message = ( on_system_message_result.as<double>() != 0 );
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
    if( HasSpecialFunction(SpecialFunction::Code::GlobalOnFocus) )
        ExecSpecialFunction(symbol_index, SpecialFunction::Code::GlobalOnFocus, { double(symbol_index) });

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

Engine::Value CIntDriver::evalexpr_INTERPRETER_DLL_TODO(Engine::Value (CIntDriver::*instruction)(int), const int program_index)
{
    ASSERT(instruction != nullptr);
    return (this->*instruction)(program_index);
}


double CIntDriver::evalexpr_INTERPRETER_DLL_TODO(double (CIntDriver::*instruction)(int), const int program_index)
{
    ASSERT(instruction != nullptr);
    return (this->*instruction)(program_index);
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


#include "InterpreterAccessor.h"
#include <zToolsO/ValueConserver.h>
InterpreterExecuteResult CIntDriver::Report_Evaluate_INTERPRETER_DLL_TODO(Report& report)
{
    return Execute(
        [&]()
        {
            // run the code to generate the report
            ValueConserver field_symbol_index_conserver(m_FieldSymbol, m_iExSymbol);
            ValueConserver execution_symbol_index_conserver(m_iExSymbol, report.GetSymbolIndex());

            return ExecuteProgramStatements<Engine::Value>(report.GetProgramIndex());
        });
}


Engine::Value CIntDriver::RunSoonToBeRemovedFeature(const std::string_view feature_sv, const int program_index, void* /*tag*/)
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
        return ReturnProgrammingError(Engine::Value::Invalid<double>());
    }
}


bool CIntDriver::IsExecutionInterrupted() const noexcept
{
    // INTERPRETER_DLL_TODO 2 of 4 checks now handled in LogicInterpreter
    if( LogicInterpreter::IsExecutionInterrupted() )
        return true;

    return ( // m_caughtProgramControlException ||
             m_bStopExec ||
             //m_bStopProc ||
             GetRequestIssued() );
}


EngineParadataDriver& CIntDriver::GetEngineParadataDriver_INTERPRETER_DLL_TODO()
{
    return *m_paradataDriver;
}


FrequencyDriver* CIntDriver::GetFrequencyDriver_INTERPRETER_DLL_TODO()
{
    ASSERT(m_frequencyDriver != nullptr);
    return m_frequencyDriver.get();
}


int CIntDriver::SymbolTableSearch_INTERPRETER_DLL_TODO(const std::string_view full_symbol_name_sv, const SymbolType preferred_symbol_type,
                                                       const std::vector<SymbolType>* const allowable_symbol_types) const
{
    return m_pEngineArea->SymbolTableSearch(full_symbol_name_sv, preferred_symbol_type, allowable_symbol_types);
}


void CIntDriver::Execute_INTERPRETER_DLL_TODO(const bool before_running_callback_function)
{
    if( before_running_callback_function )
    {
        // these statements clear any preexisting stuff that might have been going on
        m_bSkipStmt = false;
        m_bStopExec = m_bStopProc;
        SetRequestIssued(false);
    }

    else
    {
        m_bStopExec = ( m_bSkipStmt || m_bStopProc );
    }
}


void CIntDriver::ClearParadataCachedObjects_INTERPRETER_DLL_TODO()
{
    m_paradataDriver->ClearCachedObjects();
}
