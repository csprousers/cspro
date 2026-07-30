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

#define OP(op_code, function) \
    { \
        ASSERT(op_code == op_code_counter++); \
        Instruction& instruction = m_instructions[static_cast<size_t>(op_code)]; \
        ASSERT(instruction.index() == 0); \
        ASSERT(std::get<0>(instruction) == static_cast<double (CIntDriver::*)(int)>(&CIntDriver::function) || \
               std::get<0>(instruction) == &CIntDriver::ex_unimplemented_LogicInterpreter); \
        if( std::get<0>(instruction) == &CIntDriver::ex_unimplemented_LogicInterpreter ) \
            instruction = static_cast<double (CIntDriver::*)(int)>(&CIntDriver::function); \
    }

    OP(0, ex_numeric_constant);
    OP(1, exsvar);
    OP(2, exmvar);
    OP(3, excpt);
    OP(4, ex_add);
    OP(5, ex_sub);
    OP(6, ex_mult);
    OP(7, ex_div);
    OP(8, ex_mod);
    OP(9, ex_minus);
    OP(10, ex_exp);
    OP(11, ex_or);
    OP(12, ex_and);
    OP(13, ex_not);
    OP(14, ex_eq);
    OP(15, ex_ne);
    OP(16, ex_le);
    OP(17, ex_lt);
    OP(18, ex_ge);
    OP(19, ex_gt);
    OP(20, ex_equ);
    OP(21, ex_string_compute);
    OP(22, ex_WorkVariable_evaluate);
    OP(23, exif);
    OP(24, exwhile);
    OP(25, exbox);
    OP(26, ex_string_literal);
    OP(27, excharobj);
    OP(28, ex_string_eq);
    OP(29, ex_string_ne);
    OP(30, ex_string_le);
    OP(31, ex_string_lt);
    OP(32, ex_string_ge);
    OP(33, ex_string_gt);
    OP(34, excpttbl);
    OP(35, exnoopAbort);
    OP(36, exnoopAbort);
    OP(37, exuserfunctioncall);
    OP(38, exnoopAbort);
    OP(39, exnoopAbort);
    OP(40, exskipto);
    OP(41, exadvance);
    OP(42, exreenter);
    OP(43, exnoinput);
    OP(44, exendsect);
    OP(45, exendlevl);
    OP(46, exenter);
    OP(47, exskipcase);
    OP(48, exnoopAbort);
    OP(49, exstop);
    OP(50, exnoopIgnore_numeric);
    OP(51, exctab);
    OP(52, exnoopAbort);
    OP(53, exbreak);
    OP(54, exexport);
    OP(55, exset);
    OP(56, exvisualvalue);
    OP(57, exhighlight);
    OP(58, ex_sqrt);
    OP(59, ex_ex);
    OP(60, ex_int);
    OP(61, ex_log);
    OP(62, ex_seed);
    OP(63, ex_random);
    OP(64, exnoccurs);
    OP(65, exsoccurs_pre80);
    OP(66, exnoopAbort);
    OP(67, excount);
    OP(68, exsum);
    OP(69, exavrge);
    OP(70, exmin);
    OP(71, exmax);
    OP(72, exdisplay);
    OP(73, exerrmsg);
    OP(74, ex_concat);
    OP(75, ex_tonumber);
    OP(76, ex_pos_poschar);
    OP(77, ex_compare);
    OP(78, ex_length);
    OP(79, ex_strip);
    OP(80, ex_pos_poschar);
    OP(81, exedit);
    OP(82, ex_cmcode);
    OP(83, ex_setlb_setub);
    OP(84, ex_setlb_setub);
    OP(85, ex_adjuba);
    OP(86, ex_adjlba);
    OP(87, ex_adjlbi);
    OP(88, ex_adjubi);
    OP(89, exnoopAbort);
    OP(90, ex_systime);
    OP(91, ex_sysdate);
    OP(92, exdemode);
    OP(93, ex_special);
    OP(94, ex_accept);
    OP(95, exclrcase);
    OP(96, exxtab);
    OP(97, extblcoord);
    OP(98, extblcoord);
    OP(99, extblcoord);
    OP(100, extblsum);
    OP(101, extblmed);
    OP(102, exfilename);
    OP(103, exnoopAbort);
    OP(104, exnoopAbort);
    OP(105, exloadcase);
    OP(106, exnoopAbort);
    OP(107, exretrieve);
    OP(108, exnoopAbort);
    OP(109, exwritecase);
    OP(110, exdelcase);
    OP(111, exfind_locate);
    OP(112, exkey);
    OP(113, ex_open);
    OP(114, ex_close);
    OP(115, exfind_locate);
    OP(116, exnoopAbort);
    OP(117, exnoopAbort);
    OP(118, exnoopIgnore_numeric);
    OP(119, exnoopAbort);
    OP(120, exnoopIgnore_numeric);
    OP(121, exsetattr);
    OP(122, exnoopAbort);
    OP(123, exfor_dict);
    OP(124, exnmembers);
    OP(125, exnoopAbort);
    OP(126, exnoopAbort);
    OP(127, ex_minvalue_maxvalue);
    OP(128, ex_minvalue_maxvalue);
    OP(129, exfor_group);
    OP(130, exnoopAbort);
    OP(131, exnoopAbort);
    OP(132, exfucall);
    OP(133, ex_in);
    OP(134, ex_do);
    OP(135, ex_impute);
    OP(136, exfncurocc);
    OP(137, exfntotocc);
    OP(138, exupdate);
    OP(139, exwrite);
    OP(140, exnoopAbort);
    OP(141, exfor_relation);
    OP(142, exnoopAbort);
    OP(143, exgetbuffer);
    OP(144, exinsert_delete);
    OP(145, exinsert_delete);
    OP(146, exsort);
    OP(147, exgetlabel);
    OP(148, exgetlabel);
    OP(149, exnoopAbort);
    OP(150, exnoopAbort);
    OP(151, exnoopAbort);
    OP(152, exmaketext);
    OP(153, exmoveto);
    OP(154, exnoopAbort);
    OP(155, exgetoperatorid);
    OP(156, exfornext);
    OP(157, exforbreak);
    OP(158, ex_setfile);
    OP(159, exmaxocc_pre80);
    OP(160, ex_invalueset);
    OP(161, ex_setvalueset);
    OP(162, exfilecreate);
    OP(163, exfileexist);
    OP(164, exfiledelete);
    OP(165, ex_filecopy);
    OP(166, ex_filerename);
    OP(167, exfilesize);
    OP(168, exfileconcat);
    OP(169, exfileread);
    OP(170, exfilewrite);
    OP(171, ExExecSystem);
    OP(172, exnoopAbort);
    OP(173, exshowlist);
    OP(174, ex_tolower_toupper);
    OP(175, ex_tolower_toupper);
    OP(176, excountvalid);
    OP(177, exnoopIgnore_string);
    OP(178, exswap);
    OP(179, ex_datediff);
    OP(180, exdeckarray);
    OP(181, exdeckarray);
    OP(182, ex_getlanguage);
    OP(183, ex_setlanguage);
    OP(184, exendcase);
    OP(185, exuserbar);
    OP(186, exmessageoverrides);
    OP(187, ex_trace);
    OP(188, ex_setvaluesets);
    OP(189, ExExecPFF);
    OP(190, exseek);
    OP(191, ex_getcapturetype);
    OP(192, ex_setcapturetype);
    OP(193, ex_setfont);
    OP(194, exorientation);
    OP(195, exorientation);
    OP(196, ex_pathname);
    OP(197, exgps);
    OP(198, ex_low_high);
    OP(199, ex_low_high);
    OP(200, exgetrecord);
    OP(201, ex_setcapturepos);
    OP(202, ex_abs);
    OP(203, ex_randomin);
    OP(204, ex_randomizevs);
    OP(205, ex_getusername);
    OP(206, exfileempty);
    OP(207, ex_changekeyboard);
    OP(208, ex_setoutput);
    OP(209, exseekMinMax);
    OP(210, exseekMinMax);
    OP(211, ex_dateadd);
    OP(212, ex_datevalid);
    OP(213, ex_getos);
    OP(214, exgetocclabel);
    OP(215, exfreealphamem);
    OP(216, exsetvalue);
    OP(217, exgetvalue);
    OP(218, exgetvaluealpha);
    OP(219, exnoopAbort);
    OP(220, exsetocclabel);
    OP(221, exshowocc);
    OP(222, exshowocc);
    OP(223, ex_getdeviceid);
    OP(224, exdirexist);
    OP(225, exdircreate);
    OP(226, exnoopAbort);
    OP(227, ex_List_var);
    OP(228, exdirlist);
    OP(229, ex_sysparm);
    OP(230, ex_connection);
    OP(231, ex_prompt);
    OP(232, ex_getimage);
    OP(233, ex_round);
    OP(234, exnoopAbort);
    OP(235, exsavepartial);
    OP(236, ex_syncconnect);
    OP(237, ex_syncdisconnect);
    OP(238, ex_syncdata);
    OP(239, ex_syncfile);
    OP(240, ex_syncserver);
    OP(241, ex_savesetting);
    OP(242, ex_loadsetting);
    OP(243, exgetcaselabel);
    OP(244, exsetcaselabel);
    OP(245, exispartial);
    OP(246, exsetoperatorid);
    OP(247, exgetnote);
    OP(248, exeditnote);
    OP(249, exputnote);
    OP(250, exisverified);
    OP(251, exforcase);
    OP(252, ex_timestamp);
    OP(253, exkeylist);
    OP(254, ex_diagnostics);
    OP(255, ex_compress);
    OP(256, ex_decompress);
    OP(257, exask);
    OP(258, excountcases);
    OP(259, ex_getproperty);
    OP(260, ex_setproperty);
    OP(261, exlogtext);
    OP(262, exwarning);
    OP(263, ex_tr);
    OP(264, ex_uuid);
    OP(265, ex_paradata);
    OP(266, exsqlquery);
    OP(267, expre77_report);
    OP(268, expre77_setreportdata);
    OP(269, exshow);
    OP(270, exshowarray);
    OP(271, exselcase);
    OP(272, ex_timestring);
    OP(273, ex_string_literal);
    OP(274, exsymbolreset);
    OP(275, ex_decryptstring);
    OP(276, exdirdelete);
    OP(277, ex_Array_var);
    OP(278, extvar);
    OP(279, ex_exit);
    OP(280, ex_getbluetoothname);
    OP(281, ex_regexmatch);
    OP(282, exnoopAbort);
    OP(283, exgetvaluelabel);
    OP(284, ex_Array_clear);
    OP(285, ex_Array_length);
    OP(286, ex_Map_show);
    OP(287, ex_Map_hide);
    OP(288, ex_Map_addMarker);
    OP(289, ex_Map_setMarkerImage);
    OP(290, ex_Map_setMarkerText);
    OP(291, ex_Map_setMarkerOnClick_setMarkerOnClickInfo);
    OP(292, ex_Map_setMarkerOnClick_setMarkerOnClickInfo);
    OP(293, ex_Map_setMarkerDescription);
    OP(294, ex_Map_setMarkerOnDrag);
    OP(295, ex_Map_setMarkerLocation);
    OP(296, ex_Map_getMarkerLatitude_getMarkerLongitude);
    OP(297, ex_Map_removeMarker);
    OP(298, ex_Map_setOnClick);
    OP(299, ex_Map_showCurrentLocation);
    OP(300, ex_Map_addTextButton);
    OP(301, ex_Map_addImageButton);
    OP(302, ex_Map_removeButton);
    OP(303, ex_Map_setBaseMap);
    OP(304, ex_Map_setTitle);
    OP(305, ex_Map_zoomTo);
    OP(306, ex_List_add);
    OP(307, ex_List_clear);
    OP(308, ex_List_insert);
    OP(309, ex_List_length);
    OP(310, ex_List_remove);
    OP(311, ex_List_seek);
    OP(312, ex_List_show);
    OP(313, ex_List_compute);
    OP(314, ex_ValueSet_add);
    OP(315, ex_ValueSet_clear);
    OP(316, ex_ValueSet_remove);
    OP(317, ex_ValueSet_show);
    OP(318, ex_ValueSet_compute);
    OP(319, exvariablevalue);
    OP(320, ex_Map_clear_clearButtons_clearGeometry_clearMarkers);
    OP(321, ex_Map_clear_clearButtons_clearGeometry_clearMarkers);
    OP(322, ex_Map_getLastClickLatitude_getLastClickLongitude);
    OP(323, ex_Map_getLastClickLatitude_getLastClickLongitude);
    OP(324, ex_Map_getMarkerLatitude_getMarkerLongitude);
    OP(325, ex_Path_concat);
    OP(326, ex_view);
    OP(327, ex_Pff_exec);
    OP(328, ex_Pff_getProperty);
    OP(329, ex_Pff_load);
    OP(330, ex_Pff_save);
    OP(331, ex_Pff_setProperty);
    OP(332, ex_ValueSet_length);
    OP(333, ex_ischecked);
    OP(334, ex_protect);
    OP(335, ex_when);
    OP(336, ex_syncapp);
    OP(337, exfiletime);
    OP(338, ex_recode);
    OP(339, exforcase);
    OP(340, exselcase);
    OP(341, excountcases);
    OP(342, exkeylist);
    OP(343, ex_Barcode_read);
    OP(344, ex_hash);
    OP(345, ex_syncmessage);
    OP(346, ex_SystemApp_clear);
    OP(347, ex_SystemApp_setArgument);
    OP(348, ex_SystemApp_getResult);
    OP(349, ex_SystemApp_exec);
    OP(350, ex_startswith);
    OP(351, ex_Pff_compute);
    OP(352, ex_Audio_clear);
    OP(353, ex_Audio_concat);
    OP(354, ex_Audio_load);
    OP(355, ex_Audio_play);
    OP(356, ex_Audio_save);
    OP(357, ex_Audio_stop);
    OP(358, ex_Audio_record);
    OP(359, ex_Audio_recordInteractive);
    OP(360, ex_Audio_compute);
    OP(361, ex_encode);
    OP(362, ex_List_sort);
    OP(363, ex_List_removeDuplicates);
    OP(364, ex_List_removeIn);
    OP(365, ex_Path_concat);
    OP(366, ex_Path_getDirectoryName);
    OP(367, ex_Path_getExtension);
    OP(368, ex_Path_getFileName);
    OP(369, ex_Path_getFileNameWithoutExtension);
    OP(370, ex_syncparadata);
    OP(371, ex_HashMap_var);
    OP(372, ex_HashMap_compute);
    OP(373, ex_HashMap_clear);
    OP(374, ex_HashMap_contains);
    OP(375, ex_HashMap_length);
    OP(376, ex_HashMap_remove);
    OP(377, ex_HashMap_getKeys);
    OP(378, ex_Audio_length);
    OP(379, ex_ValueSet_sort);
    OP(380, ex_replace);
    OP(381, ex_inc);
    OP(382, exuniverse);
    OP(383, ex_Freq_unnamed);
    OP(384, ex_Freq_clear);
    OP(385, ex_Freq_save);
    OP(386, ex_Freq_tally);
    OP(387, ex_Freq_view);
    OP(388, ex_Freq_var);
    OP(389, ex_Freq_compute);
    OP(390, ex_WorkString_evaluate);
    OP(391, exmaxocc);
    OP(392, exsoccurs);
    OP(393, exDataAccessValidityCheck);
    OP(394, exdictcompute);
    OP(395, exkey);
    OP(396, ex_Image_compute);
    OP(397, ex_Image_captureSignature_takePhoto);
    OP(398, ex_Image_clear);
    OP(399, ex_Image_width_height);
    OP(400, ex_Image_load);
    OP(401, ex_Image_resample);
    OP(402, ex_Image_save);
    OP(403, ex_Image_captureSignature_takePhoto);
    OP(404, ex_Image_view);
    OP(405, ex_Image_width_height);
    OP(406, ex_Document_compute);
    OP(407, ex_Document_clear);
    OP(408, ex_Document_load);
    OP(409, ex_Document_save);
    OP(410, ex_Document_view);
    OP(411, ex_Geometry_compute);
    OP(412, ex_Geometry_clear);
    OP(413, ex_Geometry_load);
    OP(414, ex_Geometry_save);
    OP(415, ex_Map_addGeometry);
    OP(416, ex_Map_removeGeometry);
    OP(417, ex_Map_clear_clearButtons_clearGeometry_clearMarkers);
    OP(418, ex_Geometry_tracePolygon_walkPolygon);
    OP(419, ex_Geometry_tracePolygon_walkPolygon);
    OP(420, ex_Geometry_area_perimeter);
    OP(421, ex_Geometry_area_perimeter);
    OP(422, ex_Geometry_minLatitude_maxLatitude_minLongitude_maxLongitude);
    OP(423, ex_Geometry_minLatitude_maxLatitude_minLongitude_maxLongitude);
    OP(424, ex_Geometry_minLatitude_maxLatitude_minLongitude_maxLongitude);
    OP(425, ex_Geometry_minLatitude_maxLatitude_minLongitude_maxLongitude);
    OP(426, ex_Geometry_getProperty);
    OP(427, ex_Geometry_setProperty);
    OP(428, exinadvance);
    OP(429, ex_Map_saveSnapshot);
    OP(430, ex_synctime);
    OP(431, ex_htmldialog);
    OP(432, ex_Path_getRelativePath);
    OP(433, ex_Path_selectFile);
    OP(434, ex_invoke);
    OP(435, ex_Report_save);
    OP(436, ex_Report_view);
    OP(437, ex_TextTemplate_write_writeEncoded_writeEncodedLine_writeLine);
    OP(438, ex_setbluetoothname);
    OP(439, expersistentsymbolreset);
    OP(440, ex_Symbol_getJson_getValueJson);
    OP(441, ex_Symbol_getJson_getValueJson);
    OP(442, ex_Symbol_setValueFromJson);
    OP(443, ex_Barcode_createQRCode);
    OP(444, exScopeChange);
    OP(445, exdictaccess);
    OP(446, ex_WorkString_compute);
    OP(447, ex_ActionInvoker);
    OP(448, ex_Symbol_getName);
    OP(449, ex_Symbol_getLabel);
    OP(450, ex_Map_clear_clearButtons_clearGeometry_clearMarkers);
    OP(451, exItem_hasValue_isValid);
    OP(452, exItem_getValueLabel);
    OP(453, exItem_hasValue_isValid);
    OP(454, ex_compareNoCase);
    OP(455, exCase_view);
    OP(456, ex_JavaScript_eval);
    OP(457, ex_JavaScript_invoke);
    OP(458, ex_JavaScript_hasValue);
    OP(459, ex_JavaScript_getValueJson);
    OP(460, ex_JavaScript_setValueFromJson);
    OP(461, ex_JavaScript_getValue);
    OP(462, ex_JavaScript_setValue);
    OP(463, ex_JavaScript_UserFunctionCall);
    OP(464, ex_TextTemplate_write_writeEncoded_writeEncodedLine_writeLine);
    OP(465, ex_TextTemplate_write_writeEncoded_writeEncodedLine_writeLine);
    OP(466, ex_TextTemplate_write_writeEncoded_writeEncodedLine_writeLine);
    OP(467, ex_TextTemplate_write_writeEncoded_writeEncodedLine_writeLine);
    OP(468, ex_StringWriter_toString);
    OP(469, ex_Image_getExif);
    OP(470, ex_StringWriter_clear);
    OP(471, ex_Video_compute);
    OP(472, ex_Video_clear);
    OP(473, ex_Video_load);
    OP(474, ex_Video_save);
    OP(475, ex_Video_length);
    OP(476, ex_Video_width_height);
    OP(477, ex_Video_width_height);
    OP(478, ex_ValueSet_removeDuplicates);
    OP(479, ex_WorkVariable_compute);
    OP(480, ex_Array_compute);
    OP(481, ex_UserFunction_compute);
    OP(482, exnoopAbortPlaceholderForFutureFunction);
    OP(483, exnoopAbortPlaceholderForFutureFunction);
    OP(484, exnoopAbortPlaceholderForFutureFunction);
    OP(485, exnoopAbortPlaceholderForFutureFunction);
    OP(486, exnoopAbortPlaceholderForFutureFunction);
    OP(487, exnoopAbortPlaceholderForFutureFunction);
    OP(488, exnoopAbortPlaceholderForFutureFunction);
    OP(489, exnoopAbortPlaceholderForFutureFunction);
    OP(490, exnoopAbortPlaceholderForFutureFunction);
    OP(491, exnoopAbortPlaceholderForFutureFunction);
#undef OP

#ifdef _DEBUG
    ASSERT(op_code_counter == ( MaxInstructionCode_EV_TODO + 1 ));

    for( size_t i = 0; i <= MaxInstructionCode_EV_TODO; ++i )
        std::visit([](const auto& instruction) { ASSERT(instruction != nullptr); }, m_instructions[i]);
#endif
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
