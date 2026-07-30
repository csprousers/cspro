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
    ASSERT(std::get<0>(m_instructions[static_cast<size_t>(op_code)]) == &CIntDriver::function); \
    ASSERT(std::get<0>(m_instructions[static_cast<size_t>(op_code)]) != &CIntDriver::ex_unimplemented_LogicInterpreter);

// OP_DOUBLE = instructions defined in LogicInterpreter or CIntDriver that return double
#define OP_DOUBLE(op_code, function) \
    { \
        ASSERT(op_code == op_code_counter++); \
        Instruction& instruction = m_instructions[static_cast<size_t>(op_code)]; \
        const bool is_undefined = ( instruction.index() == 0 && std::get<0>(instruction) == &CIntDriver::ex_unimplemented_LogicInterpreter ); \
        ASSERT(is_undefined || ( instruction.index() == 1 && std::get<1>(instruction) == &CIntDriver::function )); \
        if( is_undefined ) \
            instruction = static_cast<double (CIntDriver::*)(int)>(&CIntDriver::function); \
    }

    OP_DOUBLE(0, ex_numeric_constant);
    OP_DOUBLE(1, exsvar);
    OP_DOUBLE(2, exmvar);
    OP_DOUBLE(3, excpt);
    OP_DOUBLE(4, ex_add);
    OP_DOUBLE(5, ex_sub);
    OP_DOUBLE(6, ex_mult);
    OP_DOUBLE(7, ex_div);
    OP_DOUBLE(8, ex_mod);
    OP_DOUBLE(9, ex_minus);
    OP_DOUBLE(10, ex_exp);
    OP_DOUBLE(11, ex_or);
    OP_DOUBLE(12, ex_and);
    OP_DOUBLE(13, ex_not);
    OP_DOUBLE(14, ex_eq);
    OP_DOUBLE(15, ex_ne);
    OP_DOUBLE(16, ex_le);
    OP_DOUBLE(17, ex_lt);
    OP_DOUBLE(18, ex_ge);
    OP_DOUBLE(19, ex_gt);
    OP_DOUBLE(20, ex_equ);
    OP_DOUBLE(21, ex_string_compute);
    OP_DOUBLE(22, ex_WorkVariable_evaluate);
    OP_DOUBLE(23, exif);
    OP_DOUBLE(24, exwhile);
    OP_DOUBLE(25, exbox);
    OP_ENGVAL(26, ex_string_literal);
    OP_DOUBLE(27, excharobj);
    OP_DOUBLE(28, ex_string_eq);
    OP_DOUBLE(29, ex_string_ne);
    OP_DOUBLE(30, ex_string_le);
    OP_DOUBLE(31, ex_string_lt);
    OP_DOUBLE(32, ex_string_ge);
    OP_DOUBLE(33, ex_string_gt);
    OP_DOUBLE(34, excpttbl);
    OP_DOUBLE(35, exnoopAbort);
    OP_DOUBLE(36, exnoopAbort);
    OP_DOUBLE(37, exuserfunctioncall);
    OP_DOUBLE(38, exnoopAbort);
    OP_DOUBLE(39, exnoopAbort);
    OP_DOUBLE(40, exskipto);
    OP_DOUBLE(41, exadvance);
    OP_DOUBLE(42, exreenter);
    OP_DOUBLE(43, exnoinput);
    OP_DOUBLE(44, exendsect);
    OP_DOUBLE(45, exendlevl);
    OP_DOUBLE(46, exenter);
    OP_DOUBLE(47, exskipcase);
    OP_DOUBLE(48, exnoopAbort);
    OP_DOUBLE(49, exstop);
    OP_DOUBLE(50, exnoopIgnore_numeric);
    OP_DOUBLE(51, exctab);
    OP_DOUBLE(52, exnoopAbort);
    OP_DOUBLE(53, exbreak);
    OP_DOUBLE(54, exexport);
    OP_DOUBLE(55, exset);
    OP_DOUBLE(56, exvisualvalue);
    OP_DOUBLE(57, exhighlight);
    OP_DOUBLE(58, ex_sqrt);
    OP_DOUBLE(59, ex_ex);
    OP_DOUBLE(60, ex_int);
    OP_DOUBLE(61, ex_log);
    OP_DOUBLE(62, ex_seed);
    OP_DOUBLE(63, ex_random);
    OP_DOUBLE(64, exnoccurs);
    OP_DOUBLE(65, exsoccurs_pre80);
    OP_DOUBLE(66, exnoopAbort);
    OP_DOUBLE(67, excount);
    OP_DOUBLE(68, exsum);
    OP_DOUBLE(69, exavrge);
    OP_DOUBLE(70, exmin);
    OP_DOUBLE(71, exmax);
    OP_DOUBLE(72, exdisplay);
    OP_DOUBLE(73, exerrmsg);
    OP_DOUBLE(74, ex_concat);
    OP_DOUBLE(75, ex_tonumber);
    OP_DOUBLE(76, ex_pos_poschar);
    OP_DOUBLE(77, ex_compare);
    OP_DOUBLE(78, ex_length);
    OP_DOUBLE(79, ex_strip);
    OP_DOUBLE(80, ex_pos_poschar);
    OP_DOUBLE(81, exedit);
    OP_DOUBLE(82, ex_cmcode);
    OP_DOUBLE(83, ex_setlb_setub);
    OP_DOUBLE(84, ex_setlb_setub);
    OP_DOUBLE(85, ex_adjuba);
    OP_DOUBLE(86, ex_adjlba);
    OP_DOUBLE(87, ex_adjlbi);
    OP_DOUBLE(88, ex_adjubi);
    OP_DOUBLE(89, exnoopAbort);
    OP_DOUBLE(90, ex_systime);
    OP_DOUBLE(91, ex_sysdate);
    OP_DOUBLE(92, exdemode);
    OP_DOUBLE(93, ex_special);
    OP_DOUBLE(94, ex_accept);
    OP_DOUBLE(95, exclrcase);
    OP_DOUBLE(96, exxtab);
    OP_DOUBLE(97, extblcoord);
    OP_DOUBLE(98, extblcoord);
    OP_DOUBLE(99, extblcoord);
    OP_DOUBLE(100, extblsum);
    OP_DOUBLE(101, extblmed);
    OP_DOUBLE(102, exfilename);
    OP_DOUBLE(103, exnoopAbort);
    OP_DOUBLE(104, exnoopAbort);
    OP_DOUBLE(105, exloadcase);
    OP_DOUBLE(106, exnoopAbort);
    OP_DOUBLE(107, exretrieve);
    OP_DOUBLE(108, exnoopAbort);
    OP_DOUBLE(109, exwritecase);
    OP_DOUBLE(110, exdelcase);
    OP_DOUBLE(111, exfind_locate);
    OP_DOUBLE(112, exkey);
    OP_DOUBLE(113, ex_open);
    OP_DOUBLE(114, ex_close);
    OP_DOUBLE(115, exfind_locate);
    OP_DOUBLE(116, exnoopAbort);
    OP_DOUBLE(117, exnoopAbort);
    OP_DOUBLE(118, exnoopIgnore_numeric);
    OP_DOUBLE(119, exnoopAbort);
    OP_DOUBLE(120, exnoopIgnore_numeric);
    OP_DOUBLE(121, exsetattr);
    OP_DOUBLE(122, exnoopAbort);
    OP_DOUBLE(123, exfor_dict);
    OP_DOUBLE(124, exnmembers);
    OP_DOUBLE(125, exnoopAbort);
    OP_DOUBLE(126, exnoopAbort);
    OP_DOUBLE(127, ex_minvalue_maxvalue);
    OP_DOUBLE(128, ex_minvalue_maxvalue);
    OP_DOUBLE(129, exfor_group);
    OP_DOUBLE(130, exnoopAbort);
    OP_DOUBLE(131, exnoopAbort);
    OP_DOUBLE(132, exfucall);
    OP_DOUBLE(133, ex_in);
    OP_DOUBLE(134, ex_do);
    OP_DOUBLE(135, ex_impute);
    OP_DOUBLE(136, exfncurocc);
    OP_DOUBLE(137, exfntotocc);
    OP_DOUBLE(138, exupdate);
    OP_DOUBLE(139, exwrite);
    OP_DOUBLE(140, exnoopAbort);
    OP_DOUBLE(141, exfor_relation);
    OP_DOUBLE(142, exnoopAbort);
    OP_DOUBLE(143, exgetbuffer);
    OP_DOUBLE(144, exinsert_delete);
    OP_DOUBLE(145, exinsert_delete);
    OP_DOUBLE(146, exsort);
    OP_DOUBLE(147, exgetlabel);
    OP_DOUBLE(148, exgetlabel);
    OP_DOUBLE(149, exnoopAbort);
    OP_DOUBLE(150, exnoopAbort);
    OP_DOUBLE(151, exnoopAbort);
    OP_DOUBLE(152, exmaketext);
    OP_DOUBLE(153, exmoveto);
    OP_DOUBLE(154, exnoopAbort);
    OP_DOUBLE(155, exgetoperatorid);
    OP_DOUBLE(156, exfornext);
    OP_DOUBLE(157, exforbreak);
    OP_DOUBLE(158, ex_setfile);
    OP_DOUBLE(159, exmaxocc_pre80);
    OP_DOUBLE(160, ex_invalueset);
    OP_DOUBLE(161, ex_setvalueset);
    OP_DOUBLE(162, exfilecreate);
    OP_DOUBLE(163, exfileexist);
    OP_DOUBLE(164, exfiledelete);
    OP_DOUBLE(165, ex_filecopy);
    OP_DOUBLE(166, ex_filerename);
    OP_DOUBLE(167, exfilesize);
    OP_DOUBLE(168, exfileconcat);
    OP_DOUBLE(169, exfileread);
    OP_DOUBLE(170, exfilewrite);
    OP_DOUBLE(171, ExExecSystem);
    OP_DOUBLE(172, exnoopAbort);
    OP_DOUBLE(173, exshowlist);
    OP_DOUBLE(174, ex_tolower_toupper);
    OP_DOUBLE(175, ex_tolower_toupper);
    OP_DOUBLE(176, excountvalid);
    OP_DOUBLE(177, exnoopIgnore_string);
    OP_DOUBLE(178, exswap);
    OP_DOUBLE(179, ex_datediff);
    OP_DOUBLE(180, exdeckarray);
    OP_DOUBLE(181, exdeckarray);
    OP_DOUBLE(182, ex_getlanguage);
    OP_DOUBLE(183, ex_setlanguage);
    OP_DOUBLE(184, exendcase);
    OP_DOUBLE(185, exuserbar);
    OP_DOUBLE(186, exmessageoverrides);
    OP_DOUBLE(187, ex_trace);
    OP_DOUBLE(188, ex_setvaluesets);
    OP_DOUBLE(189, ExExecPFF);
    OP_DOUBLE(190, exseek);
    OP_DOUBLE(191, ex_getcapturetype);
    OP_DOUBLE(192, ex_setcapturetype);
    OP_DOUBLE(193, ex_setfont);
    OP_DOUBLE(194, exorientation);
    OP_DOUBLE(195, exorientation);
    OP_DOUBLE(196, ex_pathname);
    OP_DOUBLE(197, exgps);
    OP_DOUBLE(198, ex_low_high);
    OP_DOUBLE(199, ex_low_high);
    OP_DOUBLE(200, exgetrecord);
    OP_DOUBLE(201, ex_setcapturepos);
    OP_DOUBLE(202, ex_abs);
    OP_DOUBLE(203, ex_randomin);
    OP_DOUBLE(204, ex_randomizevs);
    OP_DOUBLE(205, ex_getusername);
    OP_DOUBLE(206, exfileempty);
    OP_DOUBLE(207, ex_changekeyboard);
    OP_DOUBLE(208, ex_setoutput);
    OP_DOUBLE(209, exseekMinMax);
    OP_DOUBLE(210, exseekMinMax);
    OP_DOUBLE(211, ex_dateadd);
    OP_DOUBLE(212, ex_datevalid);
    OP_DOUBLE(213, ex_getos);
    OP_DOUBLE(214, exgetocclabel);
    OP_DOUBLE(215, exfreealphamem);
    OP_DOUBLE(216, exsetvalue);
    OP_DOUBLE(217, exgetvalue);
    OP_DOUBLE(218, exgetvaluealpha);
    OP_DOUBLE(219, exnoopAbort);
    OP_DOUBLE(220, exsetocclabel);
    OP_DOUBLE(221, exshowocc);
    OP_DOUBLE(222, exshowocc);
    OP_DOUBLE(223, ex_getdeviceid);
    OP_DOUBLE(224, exdirexist);
    OP_DOUBLE(225, exdircreate);
    OP_DOUBLE(226, exnoopAbort);
    OP_DOUBLE(227, ex_List_var);
    OP_DOUBLE(228, exdirlist);
    OP_DOUBLE(229, ex_sysparm);
    OP_DOUBLE(230, ex_connection);
    OP_DOUBLE(231, ex_prompt);
    OP_DOUBLE(232, ex_getimage);
    OP_DOUBLE(233, ex_round);
    OP_DOUBLE(234, exnoopAbort);
    OP_DOUBLE(235, exsavepartial);
    OP_DOUBLE(236, ex_syncconnect);
    OP_DOUBLE(237, ex_syncdisconnect);
    OP_DOUBLE(238, ex_syncdata);
    OP_DOUBLE(239, ex_syncfile);
    OP_DOUBLE(240, ex_syncserver);
    OP_DOUBLE(241, ex_savesetting);
    OP_DOUBLE(242, ex_loadsetting);
    OP_DOUBLE(243, exgetcaselabel);
    OP_DOUBLE(244, exsetcaselabel);
    OP_DOUBLE(245, exispartial);
    OP_DOUBLE(246, exsetoperatorid);
    OP_DOUBLE(247, exgetnote);
    OP_DOUBLE(248, exeditnote);
    OP_DOUBLE(249, exputnote);
    OP_DOUBLE(250, exisverified);
    OP_DOUBLE(251, exforcase);
    OP_DOUBLE(252, ex_timestamp);
    OP_DOUBLE(253, exkeylist);
    OP_DOUBLE(254, ex_diagnostics);
    OP_DOUBLE(255, ex_compress);
    OP_DOUBLE(256, ex_decompress);
    OP_DOUBLE(257, exask);
    OP_DOUBLE(258, excountcases);
    OP_DOUBLE(259, ex_getproperty);
    OP_DOUBLE(260, ex_setproperty);
    OP_DOUBLE(261, exlogtext);
    OP_DOUBLE(262, exwarning);
    OP_DOUBLE(263, ex_tr);
    OP_DOUBLE(264, ex_uuid);
    OP_DOUBLE(265, ex_paradata);
    OP_DOUBLE(266, exsqlquery);
    OP_DOUBLE(267, expre77_report);
    OP_DOUBLE(268, expre77_setreportdata);
    OP_DOUBLE(269, exshow);
    OP_DOUBLE(270, exshowarray);
    OP_DOUBLE(271, exselcase);
    OP_DOUBLE(272, ex_timestring);
    OP_ENGVAL(273, ex_string_literal);
    OP_DOUBLE(274, exsymbolreset);
    OP_DOUBLE(275, ex_decryptstring);
    OP_DOUBLE(276, exdirdelete);
    OP_DOUBLE(277, ex_Array_var);
    OP_DOUBLE(278, extvar);
    OP_DOUBLE(279, ex_exit);
    OP_DOUBLE(280, ex_getbluetoothname);
    OP_DOUBLE(281, ex_regexmatch);
    OP_DOUBLE(282, exnoopAbort);
    OP_DOUBLE(283, exgetvaluelabel);
    OP_DOUBLE(284, ex_Array_clear);
    OP_DOUBLE(285, ex_Array_length);
    OP_DOUBLE(286, ex_Map_show);
    OP_DOUBLE(287, ex_Map_hide);
    OP_DOUBLE(288, ex_Map_addMarker);
    OP_DOUBLE(289, ex_Map_setMarkerImage);
    OP_DOUBLE(290, ex_Map_setMarkerText);
    OP_DOUBLE(291, ex_Map_setMarkerOnClick_setMarkerOnClickInfo);
    OP_DOUBLE(292, ex_Map_setMarkerOnClick_setMarkerOnClickInfo);
    OP_DOUBLE(293, ex_Map_setMarkerDescription);
    OP_DOUBLE(294, ex_Map_setMarkerOnDrag);
    OP_DOUBLE(295, ex_Map_setMarkerLocation);
    OP_DOUBLE(296, ex_Map_getMarkerLatitude_getMarkerLongitude);
    OP_DOUBLE(297, ex_Map_removeMarker);
    OP_DOUBLE(298, ex_Map_setOnClick);
    OP_DOUBLE(299, ex_Map_showCurrentLocation);
    OP_DOUBLE(300, ex_Map_addTextButton);
    OP_DOUBLE(301, ex_Map_addImageButton);
    OP_DOUBLE(302, ex_Map_removeButton);
    OP_DOUBLE(303, ex_Map_setBaseMap);
    OP_DOUBLE(304, ex_Map_setTitle);
    OP_DOUBLE(305, ex_Map_zoomTo);
    OP_DOUBLE(306, ex_List_add);
    OP_DOUBLE(307, ex_List_clear);
    OP_DOUBLE(308, ex_List_insert);
    OP_DOUBLE(309, ex_List_length);
    OP_DOUBLE(310, ex_List_remove);
    OP_DOUBLE(311, ex_List_seek);
    OP_DOUBLE(312, ex_List_show);
    OP_DOUBLE(313, ex_List_compute);
    OP_DOUBLE(314, ex_ValueSet_add);
    OP_DOUBLE(315, ex_ValueSet_clear);
    OP_DOUBLE(316, ex_ValueSet_remove);
    OP_DOUBLE(317, ex_ValueSet_show);
    OP_DOUBLE(318, ex_ValueSet_compute);
    OP_DOUBLE(319, exvariablevalue);
    OP_DOUBLE(320, ex_Map_clear_clearButtons_clearGeometry_clearMarkers);
    OP_DOUBLE(321, ex_Map_clear_clearButtons_clearGeometry_clearMarkers);
    OP_DOUBLE(322, ex_Map_getLastClickLatitude_getLastClickLongitude);
    OP_DOUBLE(323, ex_Map_getLastClickLatitude_getLastClickLongitude);
    OP_DOUBLE(324, ex_Map_getMarkerLatitude_getMarkerLongitude);
    OP_DOUBLE(325, ex_Path_concat);
    OP_DOUBLE(326, ex_view);
    OP_DOUBLE(327, ex_Pff_exec);
    OP_DOUBLE(328, ex_Pff_getProperty);
    OP_DOUBLE(329, ex_Pff_load);
    OP_DOUBLE(330, ex_Pff_save);
    OP_DOUBLE(331, ex_Pff_setProperty);
    OP_DOUBLE(332, ex_ValueSet_length);
    OP_DOUBLE(333, ex_ischecked);
    OP_DOUBLE(334, ex_protect);
    OP_DOUBLE(335, ex_when);
    OP_DOUBLE(336, ex_syncapp);
    OP_DOUBLE(337, exfiletime);
    OP_DOUBLE(338, ex_recode);
    OP_DOUBLE(339, exforcase);
    OP_DOUBLE(340, exselcase);
    OP_DOUBLE(341, excountcases);
    OP_DOUBLE(342, exkeylist);
    OP_DOUBLE(343, ex_Barcode_read);
    OP_DOUBLE(344, ex_hash);
    OP_DOUBLE(345, ex_syncmessage);
    OP_DOUBLE(346, ex_SystemApp_clear);
    OP_DOUBLE(347, ex_SystemApp_setArgument);
    OP_DOUBLE(348, ex_SystemApp_getResult);
    OP_DOUBLE(349, ex_SystemApp_exec);
    OP_DOUBLE(350, ex_startswith);
    OP_DOUBLE(351, ex_Pff_compute);
    OP_DOUBLE(352, ex_Audio_clear);
    OP_DOUBLE(353, ex_Audio_concat);
    OP_DOUBLE(354, ex_Audio_load);
    OP_DOUBLE(355, ex_Audio_play);
    OP_DOUBLE(356, ex_Audio_save);
    OP_DOUBLE(357, ex_Audio_stop);
    OP_DOUBLE(358, ex_Audio_record);
    OP_DOUBLE(359, ex_Audio_recordInteractive);
    OP_DOUBLE(360, ex_Audio_compute);
    OP_DOUBLE(361, ex_encode);
    OP_DOUBLE(362, ex_List_sort);
    OP_DOUBLE(363, ex_List_removeDuplicates);
    OP_DOUBLE(364, ex_List_removeIn);
    OP_DOUBLE(365, ex_Path_concat);
    OP_DOUBLE(366, ex_Path_getDirectoryName);
    OP_DOUBLE(367, ex_Path_getExtension);
    OP_DOUBLE(368, ex_Path_getFileName);
    OP_DOUBLE(369, ex_Path_getFileNameWithoutExtension);
    OP_DOUBLE(370, ex_syncparadata);
    OP_DOUBLE(371, ex_HashMap_var);
    OP_DOUBLE(372, ex_HashMap_compute);
    OP_DOUBLE(373, ex_HashMap_clear);
    OP_DOUBLE(374, ex_HashMap_contains);
    OP_DOUBLE(375, ex_HashMap_length);
    OP_DOUBLE(376, ex_HashMap_remove);
    OP_DOUBLE(377, ex_HashMap_getKeys);
    OP_DOUBLE(378, ex_Audio_length);
    OP_DOUBLE(379, ex_ValueSet_sort);
    OP_DOUBLE(380, ex_replace);
    OP_DOUBLE(381, ex_inc);
    OP_DOUBLE(382, exuniverse);
    OP_DOUBLE(383, ex_Freq_unnamed);
    OP_DOUBLE(384, ex_Freq_clear);
    OP_DOUBLE(385, ex_Freq_save);
    OP_DOUBLE(386, ex_Freq_tally);
    OP_DOUBLE(387, ex_Freq_view);
    OP_DOUBLE(388, ex_Freq_var);
    OP_DOUBLE(389, ex_Freq_compute);
    OP_DOUBLE(390, ex_WorkString_evaluate);
    OP_DOUBLE(391, exmaxocc);
    OP_DOUBLE(392, exsoccurs);
    OP_DOUBLE(393, exDataAccessValidityCheck);
    OP_DOUBLE(394, exdictcompute);
    OP_DOUBLE(395, exkey);
    OP_DOUBLE(396, ex_Image_compute);
    OP_DOUBLE(397, ex_Image_captureSignature_takePhoto);
    OP_DOUBLE(398, ex_Image_clear);
    OP_DOUBLE(399, ex_Image_width_height);
    OP_DOUBLE(400, ex_Image_load);
    OP_DOUBLE(401, ex_Image_resample);
    OP_DOUBLE(402, ex_Image_save);
    OP_DOUBLE(403, ex_Image_captureSignature_takePhoto);
    OP_DOUBLE(404, ex_Image_view);
    OP_DOUBLE(405, ex_Image_width_height);
    OP_DOUBLE(406, ex_Document_compute);
    OP_DOUBLE(407, ex_Document_clear);
    OP_DOUBLE(408, ex_Document_load);
    OP_DOUBLE(409, ex_Document_save);
    OP_DOUBLE(410, ex_Document_view);
    OP_DOUBLE(411, ex_Geometry_compute);
    OP_DOUBLE(412, ex_Geometry_clear);
    OP_DOUBLE(413, ex_Geometry_load);
    OP_DOUBLE(414, ex_Geometry_save);
    OP_DOUBLE(415, ex_Map_addGeometry);
    OP_DOUBLE(416, ex_Map_removeGeometry);
    OP_DOUBLE(417, ex_Map_clear_clearButtons_clearGeometry_clearMarkers);
    OP_DOUBLE(418, ex_Geometry_tracePolygon_walkPolygon);
    OP_DOUBLE(419, ex_Geometry_tracePolygon_walkPolygon);
    OP_DOUBLE(420, ex_Geometry_area_perimeter);
    OP_DOUBLE(421, ex_Geometry_area_perimeter);
    OP_DOUBLE(422, ex_Geometry_minLatitude_maxLatitude_minLongitude_maxLongitude);
    OP_DOUBLE(423, ex_Geometry_minLatitude_maxLatitude_minLongitude_maxLongitude);
    OP_DOUBLE(424, ex_Geometry_minLatitude_maxLatitude_minLongitude_maxLongitude);
    OP_DOUBLE(425, ex_Geometry_minLatitude_maxLatitude_minLongitude_maxLongitude);
    OP_DOUBLE(426, ex_Geometry_getProperty);
    OP_DOUBLE(427, ex_Geometry_setProperty);
    OP_DOUBLE(428, exinadvance);
    OP_DOUBLE(429, ex_Map_saveSnapshot);
    OP_DOUBLE(430, ex_synctime);
    OP_DOUBLE(431, ex_htmldialog);
    OP_DOUBLE(432, ex_Path_getRelativePath);
    OP_DOUBLE(433, ex_Path_selectFile);
    OP_DOUBLE(434, ex_invoke);
    OP_DOUBLE(435, ex_Report_save);
    OP_DOUBLE(436, ex_Report_view);
    OP_DOUBLE(437, ex_TextTemplate_write_writeEncoded_writeEncodedLine_writeLine);
    OP_DOUBLE(438, ex_setbluetoothname);
    OP_DOUBLE(439, expersistentsymbolreset);
    OP_DOUBLE(440, ex_Symbol_getJson_getValueJson);
    OP_DOUBLE(441, ex_Symbol_getJson_getValueJson);
    OP_DOUBLE(442, ex_Symbol_setValueFromJson);
    OP_DOUBLE(443, ex_Barcode_createQRCode);
    OP_DOUBLE(444, exScopeChange);
    OP_DOUBLE(445, exdictaccess);
    OP_DOUBLE(446, ex_WorkString_compute);
    OP_DOUBLE(447, ex_ActionInvoker);
    OP_DOUBLE(448, ex_Symbol_getName);
    OP_DOUBLE(449, ex_Symbol_getLabel);
    OP_DOUBLE(450, ex_Map_clear_clearButtons_clearGeometry_clearMarkers);
    OP_DOUBLE(451, exItem_hasValue_isValid);
    OP_DOUBLE(452, exItem_getValueLabel);
    OP_DOUBLE(453, exItem_hasValue_isValid);
    OP_DOUBLE(454, ex_compareNoCase);
    OP_DOUBLE(455, exCase_view);
    OP_DOUBLE(456, ex_JavaScript_eval);
    OP_DOUBLE(457, ex_JavaScript_invoke);
    OP_DOUBLE(458, ex_JavaScript_hasValue);
    OP_DOUBLE(459, ex_JavaScript_getValueJson);
    OP_DOUBLE(460, ex_JavaScript_setValueFromJson);
    OP_DOUBLE(461, ex_JavaScript_getValue);
    OP_DOUBLE(462, ex_JavaScript_setValue);
    OP_DOUBLE(463, ex_JavaScript_UserFunctionCall);
    OP_DOUBLE(464, ex_TextTemplate_write_writeEncoded_writeEncodedLine_writeLine);
    OP_DOUBLE(465, ex_TextTemplate_write_writeEncoded_writeEncodedLine_writeLine);
    OP_DOUBLE(466, ex_TextTemplate_write_writeEncoded_writeEncodedLine_writeLine);
    OP_DOUBLE(467, ex_TextTemplate_write_writeEncoded_writeEncodedLine_writeLine);
    OP_DOUBLE(468, ex_StringWriter_toString);
    OP_DOUBLE(469, ex_Image_getExif);
    OP_DOUBLE(470, ex_StringWriter_clear);
    OP_DOUBLE(471, ex_Video_compute);
    OP_DOUBLE(472, ex_Video_clear);
    OP_DOUBLE(473, ex_Video_load);
    OP_DOUBLE(474, ex_Video_save);
    OP_DOUBLE(475, ex_Video_length);
    OP_DOUBLE(476, ex_Video_width_height);
    OP_DOUBLE(477, ex_Video_width_height);
    OP_DOUBLE(478, ex_ValueSet_removeDuplicates);
    OP_DOUBLE(479, ex_WorkVariable_compute);
    OP_DOUBLE(480, ex_Array_compute);
    OP_DOUBLE(481, ex_UserFunction_compute);
    OP_DOUBLE(482, exnoopAbortPlaceholderForFutureFunction);
    OP_DOUBLE(483, exnoopAbortPlaceholderForFutureFunction);
    OP_DOUBLE(484, exnoopAbortPlaceholderForFutureFunction);
    OP_DOUBLE(485, exnoopAbortPlaceholderForFutureFunction);
    OP_DOUBLE(486, exnoopAbortPlaceholderForFutureFunction);
    OP_DOUBLE(487, exnoopAbortPlaceholderForFutureFunction);
    OP_DOUBLE(488, exnoopAbortPlaceholderForFutureFunction);
    OP_DOUBLE(489, exnoopAbortPlaceholderForFutureFunction);
    OP_DOUBLE(490, exnoopAbortPlaceholderForFutureFunction);
    OP_DOUBLE(491, exnoopAbortPlaceholderForFutureFunction);
#undef OP_ENGVAL
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


bool CIntDriver::ExecuteProgramStatements(int program_index)
{
    // execute a block of statements
    while( program_index >= 0 && !m_bStopExec )
    {
        const auto& statement_node = GetNode<ST_NODE>(program_index);

        try
        {
            ExecuteInstruction(static_cast<FunctionCode>(statement_node.st_code), program_index);
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


double CIntDriver::ExecSpecialFunction(const int iSymVar, const SpecialFunction::Code special_function,
                                       std::vector<std::variant<double, SharableString>> arguments)
{
    UserFunction* const user_function = GetSpecialFunctions()[static_cast<size_t>(special_function)];

    if( user_function == nullptr )
    {
        ASSERT(false);
        const SpecialFunction::Definition& definition = SpecialFunction::GetDefinitions()[static_cast<size_t>(special_function)];
        return AssignInvalidValue(
            ( definition.returns == SymbolType::WorkVariable ) ? DataType::Numeric :
            ( definition.returns == SymbolType::WorkString )   ? DataType::String :
                                                                 ReturnProgrammingError(DataType::Numeric)
        );
    }

    // if there is no function body, return the default value
    if( user_function->GetProgramIndex() < 0 )
        return AssignInvalidValue(user_function->GetReturnDataType());

    // Now Execute the code
    m_bSkipStmt = false; // RHF Sep 20, 2000.
    //If there is a field (the first field) before a roster and we arrive to this field
    // from a previous endsect. m_bSkipStmt remains true when the field doesn't have proc.
    // So the PreProc of the Roster is not executed (see DeSetNextField GroupCompletion
    // is not called when m_bSkipStmt is true.!!!
    if( m_bStopProc )
        return AssignInvalidValue(user_function->GetReturnDataType());

    // TODO: make sure that all functions can work properly when m_iExSymbol is 0; for
    // now only allow this in OnSystemMessage because that is an obscure feature (and if
    // m_iExSymbol is 0, we will activate the special function checking that keeps things
    // like movement statements from executing)
    if( iSymVar <= 0 && special_function != SpecialFunction::Code::OnSystemMessage )
        return AssignInvalidValue(user_function->GetReturnDataType());

    const RAII::SetValueAndRestoreOnDestruction proc_type_modifier(m_procType, ProcType::OnFocus);
    const RAII::SetValueAndRestoreOnDestruction symbol_modifier(m_iExSymbol, iSymVar);
    const RAII::SetValueAndRestoreOnDestruction level_modifier(m_iExLevel, ( iSymVar > 0 ) ? SymbolCalculator::GetLevelNumber_base1(NPT_Ref(iSymVar)) : 0);

    m_bSkipStmt = false;
    m_bStopExec = m_bStopProc;

    SetRequestIssued( false ); // reset RequestIssued// RHF Dec 03, 2003

    m_bExecSpecFunc = ( special_function == SpecialFunction::Code::GlobalOnFocus || m_iExSymbol <= 0 );

    NumericStringValuesOnlyUserFunctionArgumentEvaluator<false> argument_evaluator(std::move(arguments));
    const double return_value = CallUserFunction(*user_function, argument_evaluator);

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
        issue_message = ( ExecSpecialFunction(m_iExSymbol, SpecialFunction::Code::OnSystemMessage, arguments) != 0 );
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
