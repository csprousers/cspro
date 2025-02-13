#pragma once

#include <zLogicO/zLogicO.h>
#include <zUtilO/DataTypes.h>
#include <zLogicO/ReservedWordsTable.h>
#include <zLogicO/SymbolType.h>

class Symbol;


// --------------------------------------------------------------------------
// FunctionCode
// --------------------------------------------------------------------------

// this is outside of the Logic namespace in order to avoid needing to
// change many references to these codes; it is also a enum, rather than
// an enum class, so that it doesn't have to constantly be cast to ints
// when being stuffed in compilation nodes
enum FunctionCode : int
{
//  ﾚﾄﾄﾄﾄﾄﾄﾄﾄﾄﾄﾄﾄﾄﾄﾄﾄﾄﾂﾄﾄﾄﾄﾄﾄﾄﾂ= --------------------------------------------
//  ｳ Function        ｳOp.codeｳ      COMMANDS
//  ﾀﾄﾄﾄﾄﾄﾄﾄﾄﾄﾄﾄﾄﾄﾄﾄﾄﾄﾁﾄﾄﾄﾄﾄﾄﾄﾁ= --------------------------------------------
    CONST_CODE                              =  0,
    SVAR_CODE                               =  1,
    MVAR_CODE                               =  2,
    UF_CODE                                 =  3, // intentionally same as CPT_CODE
    CPT_CODE                                =  3,
    ADD_CODE                                =  4,
    SUB_CODE                                =  5,
    MULT_CODE                               =  6,
    DIV_CODE                                =  7,
    MOD_CODE                                =  8,
    MINUS_CODE                              =  9,
    EXP_CODE                                = 10,
    OR_CODE                                 = 11,
    AND_CODE                                = 12,
    NOT_CODE                                = 13,
    EQ_CODE                                 = 14,
    NE_CODE                                 = 15,
    LE_CODE                                 = 16,
    LT_CODE                                 = 17,
    GE_CODE                                 = 18,
    GT_CODE                                 = 19,
    EQU_CODE                                = 20,
    STRING_COMPUTE_CODE                     = 21,
    WVAR_CODE                               = 22,
    IF_CODE                                 = 23,
    WHILE_CODE                              = 24,
    BOX_CODE                                = 25,
    CHOBJ_CODE                              = 27,
    CH_EQ_CODE                              = 28,
    CH_NE_CODE                              = 29,
    CH_LE_CODE                              = 30,
    CH_LT_CODE                              = 31,
    CH_GE_CODE                              = 32,
    CH_GT_CODE                              = 33,
    TBLCPT_CODE                             = 34,
    TBL_CODE                                = 35,
    USERFUNCTIONCALL_CODE                   = 37,

//  ﾚﾄﾄﾄﾄﾄﾄﾄﾄﾄﾄﾄﾄﾄﾄﾄﾄﾄﾂﾄﾄﾄﾄﾄﾄﾄﾂ= --------------------------------------------
//  ｳ Function        ｳOp.codeｳ      Data entry COMMANDS
//  ﾀﾄﾄﾄﾄﾄﾄﾄﾄﾄﾄﾄﾄﾄﾄﾄﾄﾄﾁﾄﾄﾄﾄﾄﾄﾄﾁ= --------------------------------------------
    SKIPTO_CODE                             = 40,
    ADVANCE_CODE                            = 41,
    REENTER_CODE                            = 42,
    NOINPUT_CODE                            = 43,
    ENDSECT_CODE                            = 44,
    ENDLEVL_CODE                            = 45,
    ENTER_CODE                              = 46,

//  ﾚﾄﾄﾄﾄﾄﾄﾄﾄﾄﾄﾄﾄﾄﾄﾄﾄﾄﾂﾄﾄﾄﾄﾄﾄﾄﾂ= --------------------------------------------
//  ｳ Function        ｳOp.codeｳ      Batch COMMANDS
//  ﾀﾄﾄﾄﾄﾄﾄﾄﾄﾄﾄﾄﾄﾄﾄﾄﾄﾄﾁﾄﾄﾄﾄﾄﾄﾄﾁ= --------------------------------------------
    SKIPCASE_CODE                           = 47,
    STOP_CODE                               = 49,
    CTAB_CODE                               = 51,
    BREAK_CODE                              = 53,
    EXPORT_CODE                             = 54,

//  ﾚﾄﾄﾄﾄﾄﾄﾄﾄﾄﾄﾄﾄﾄﾄﾄﾄﾄﾂﾄﾄﾄﾄﾄﾄﾄﾂ= --------------------------------------------
//  ｳ Function        ｳOp.codeｳ      future COMMANDS
//  ﾀﾄﾄﾄﾄﾄﾄﾄﾄﾄﾄﾄﾄﾄﾄﾄﾄﾄﾁﾄﾄﾄﾄﾄﾄﾄﾁ= --------------------------------------------
    SET_CODE                                = 55,

//  ﾚﾄﾄﾄﾄﾄﾄﾄﾄﾄﾄﾄﾄﾄﾄﾄﾄﾄﾂﾄﾄﾄﾄﾄﾄﾄﾂ= --------------------------------------------
//  ｳ Function        ｳOp.codeｳ      NUMERIC FUNCTIONS
//  ﾀﾄﾄﾄﾄﾄﾄﾄﾄﾄﾄﾄﾄﾄﾄﾄﾄﾄﾁﾄﾄﾄﾄﾄﾄﾄﾁ= --------------------------------------------
    FNVISUALVALUE_CODE                      = 56,
    FNHIGHLIGHT_CODE                        = 57,
    FNSQRT_CODE                             = 58,
    FNEXP_CODE                              = 59,
    FNINT_CODE                              = 60,
    FNLOG_CODE                              = 61,
    FNSEED_CODE                             = 62,
    FNRANDOM_CODE                           = 63,
    FNNOCCURS_CODE                          = 64,
    PRE80_FNSOCCURS_CODE                    = 65,
    FNCOUNT_CODE                            = 67,
    FNSUM_CODE                              = 68,
    FNAVERAGE_CODE                          = 69,
    FNMIN_CODE                              = 70,
    FNMAX_CODE                              = 71,
    FNDISPLAY_CODE                          = 72,
    FNERRMSG_CODE                           = 73,

//  ﾚﾄﾄﾄﾄﾄﾄﾄﾄﾄﾄﾄﾄﾄﾄﾄﾄﾄﾂﾄﾄﾄﾄﾄﾄﾄﾂ= --------------------------------------------
//  ｳ Function        ｳOp.codeｳ      ALPHA FUNCTIONS
//  ﾀﾄﾄﾄﾄﾄﾄﾄﾄﾄﾄﾄﾄﾄﾄﾄﾄﾄﾁﾄﾄﾄﾄﾄﾄﾄﾁ= --------------------------------------------
    FNCONCAT_CODE                           = 74,
    FNTONUMB_CODE                           = 75,
    FNPOS_CODE                              = 76,
    FNCOMPARE_CODE                          = 77,
    FNLENGTH_CODE                           = 78,
    FNSTRIP_CODE                            = 79,
    FNPOSCHAR_CODE                          = 80,
    FNEDIT_CODE                             = 81,

//  ﾚﾄﾄﾄﾄﾄﾄﾄﾄﾄﾄﾄﾄﾄﾄﾄﾄﾄﾂﾄﾄﾄﾄﾄﾄﾄﾂ= --------------------------------------------
//  ｳ Function        ｳOp.codeｳ      DATE FUNCTIONS
//  ﾀﾄﾄﾄﾄﾄﾄﾄﾄﾄﾄﾄﾄﾄﾄﾄﾄﾄﾁﾄﾄﾄﾄﾄﾄﾄﾁ= --------------------------------------------
    FNCMCODE_CODE                           = 82,
    FNSETUB_CODE                            = 83,
    FNSETLB_CODE                            = 84,
    FNADJUBA_CODE                           = 85,
    FNADJLBA_CODE                           = 86,
    FNADJLBI_CODE                           = 87,
    FNADJUBI_CODE                           = 88,
    FNSYSTIME_CODE                          = 90,
    FNSYSDATE_CODE                          = 91,

//  ﾚﾄﾄﾄﾄﾄﾄﾄﾄﾄﾄﾄﾄﾄﾄﾄﾄﾄﾂﾄﾄﾄﾄﾄﾄﾄﾂ= --------------------------------------------
//  ｳ Function        ｳOp.codeｳ      OTHER FUNCTIONS
//  ﾀﾄﾄﾄﾄﾄﾄﾄﾄﾄﾄﾄﾄﾄﾄﾄﾄﾄﾁﾄﾄﾄﾄﾄﾄﾄﾁ= --------------------------------------------
    FNDEMODE_CODE                           = 92,
    FNSPECIAL_CODE                          = 93,
    FNACCEPT_CODE                           = 94,
    FNCLRCASE_CODE                          = 95,

//  ﾚﾄﾄﾄﾄﾄﾄﾄﾄﾄﾄﾄﾄﾄﾄﾄﾄﾄﾂﾄﾄﾄﾄﾄﾄﾄﾂ= --------------------------------------------
//  ｳ Function        ｳOp.codeｳ      TABLES/CROSSTAB FUNCTIONS
//  ﾀﾄﾄﾄﾄﾄﾄﾄﾄﾄﾄﾄﾄﾄﾄﾄﾄﾄﾁﾄﾄﾄﾄﾄﾄﾄﾁ= --------------------------------------------
    FNXTAB_CODE                             = 96,
    FNTBLROW_CODE                           = 97,
    FNTBLCOL_CODE                           = 98,
    FNTBLLAY_CODE                           = 99,
    FNTBLSUM_CODE                           = 100,
    FNTBLMED_CODE                           = 101,

//  ﾚﾄﾄﾄﾄﾄﾄﾄﾄﾄﾄﾄﾄﾄﾄﾄﾄﾄﾂﾄﾄﾄﾄﾄﾄﾄﾂ= --------------------------------------------
//  ｳ Function        ｳOp.codeｳ      INDEXED FILES FUNCTIONS
//  ﾀﾄﾄﾄﾄﾄﾄﾄﾄﾄﾄﾄﾄﾄﾄﾄﾄﾄﾁﾄﾄﾄﾄﾄﾄﾄﾁ= --------------------------------------------
    FNFILENAME_CODE                         = 102,
    FNLOADCASE_CODE                         = 105,
    FNRETRIEVE_CODE                         = 107,
    FNWRITECASE_CODE                        = 109,
    FNDELCASE_CODE                          = 110,
    FNFIND_CODE                             = 111,
    FNKEY_CODE                              = 112,
    FNOPEN_CODE                             = 113,
    FNCLOSE_CODE                            = 114,
    FNLOCATE_CODE                           = 115,

//  ﾚﾄﾄﾄﾄﾄﾄﾄﾄﾄﾄﾄﾄﾄﾄﾄﾄﾄﾂﾄﾄﾄﾄﾄﾄﾄﾂ= --------------------------------------------
//  ｳ Function        ｳOp.codeｳ more COMMANDS
//  ﾀﾄﾄﾄﾄﾄﾄﾄﾄﾄﾄﾄﾄﾄﾄﾄﾄﾄﾁﾄﾄﾄﾄﾄﾄﾄﾁ= --------------------------------------------
    SET_ATTR_CODE                           = 121,

//  ﾚﾄﾄﾄﾄﾄﾄﾄﾄﾄﾄﾄﾄﾄﾄﾄﾄﾄﾂﾄﾄﾄﾄﾄﾄﾄﾂ= --------------------------------------------
//  ｳ Function        ｳOp.codeｳ      Data entry COMMANDS = -, extension
//  ﾀﾄﾄﾄﾄﾄﾄﾄﾄﾄﾄﾄﾄﾄﾄﾄﾄﾄﾁﾄﾄﾄﾄﾄﾄﾄﾁ= --------------------------------------------
    FOR_DICT_CODE                           = 123,

//  ﾚﾄﾄﾄﾄﾄﾄﾄﾄﾄﾄﾄﾄﾄﾄﾄﾄﾄﾂﾄﾄﾄﾄﾄﾄﾄﾂ= --------------------------------------------
//  ｳ Function        ｳOp.codeｳ      NUMERIC FUNCTIONS   = -, extension
//  ﾀﾄﾄﾄﾄﾄﾄﾄﾄﾄﾄﾄﾄﾄﾄﾄﾄﾄﾁﾄﾄﾄﾄﾄﾄﾄﾁ= --------------------------------------------
    FNNMEMBERS_CODE                         = 124,
    FNMINVALUE_CODE                         = 127,
    FNMAXVALUE_CODE                         = 128,

    FOR_GROUP_CODE                          = 129,
    GROUP_CODE                              = 131, // RHF Aug 07, 2000 no function associated

    FUCALL_CODE                             = 132, // RHC Aug 21, 2000

    IN_CODE                                 = 133, // RHC Oct 16, 2000
    DO_CODE                                 = 134, // RHC Oct 16, 2000
    FNIMPUTE_CODE                           = 135, // RHF Oct 25, 2000
    FNCUROCC_CODE                           = 136, // RHC Oct 30, 2000
    FNTOTOCC_CODE                           = 137, // RHC Oct 30, 2000
    FNUPDATE_CODE                           = 138, // RHF Nov 17, 2000
    FNWRITE_CODE                            = 139, // RHF Dec 16, 2000
    FOR_RELATION_CODE                       = 141, // RHC Sep 20, 2001
    REL_CODE                                = 142, // RHC Sep 20, 2001
    FNGETBUFFER_CODE                        = 143, // RHF Sep 21, 2001
    FNINSERT_CODE                           = 144, // Chirag Jul 22, 2002
    FNDELETE_CODE                           = 145, // Chirag Sep 11, 2002
    FNSORT_CODE                             = 146, // Chirag Sep 11, 2002
    FNGETLABEL_CODE                         = 147, // RHF Aug 25, 2000
    FNGETSYMBOL_CODE                        = 148, // RHF Mar 23, 2001
    FNMAKETEXT_CODE                         = 152, // RHF Jun 08, 2001

    MOVETO_CODE                             = 153, // RHF Dec 09, 2003
    FNGETOPERATORID_CODE                    = 155,
    FORNEXT_CODE                            = 156, // RHC Sep 04, 2000
    FORBREAK_CODE                           = 157, // RHC Sep 04, 2000

    FNSETFILE_CODE                          = 158,
    PRE80_FNMAXOCC_CODE                     = 159,
    FNINVALUESET_CODE                       = 160,
    FNSETVALUESET_CODE                      = 161, // RHF Aug 28, 2002

    FNFILE_CREATE_CODE                      = 162,
    FNFILE_EXIST_CODE                       = 163,
    FNFILE_DELETE_CODE                      = 164,
    FNFILE_COPY_CODE                        = 165,
    FNFILE_RENAME_CODE                      = 166,
    FNFILE_SIZE_CODE                        = 167,
    FNFILE_CONCAT_CODE                      = 168,
    FNFILE_READ_CODE                        = 169,
    FNFILE_WRITE_CODE                       = 170,
    FNEXECSYSTEM_CODE                       = 171,
    FNSHOWLIST_CODE                         = 173,

    FNTOLOWER_CODE                          = 174, // GHM 20091202
    FNTOUPPER_CODE                          = 175, // GHM 20091202

    FNCOUNTVALID_CODE                       = 176, // GHM 20091202
    FNITEMLIST_CODE                         = 177, // GHM 20091208

    FNSWAP_CODE                             = 178, // GHM 20100105

    FNDATEDIFF_CODE                         = 179, // GHM 20100118

    FNPUTDECK_CODE                          = 180, // GHM 20100120
    FNGETDECK_CODE                          = 181, // GHM 20100120

    FNGETLANGUAGE_CODE                      = 182, // GHM 20100309
    FNSETLANGUAGE_CODE                      = 183, // GHM 20100309

    FNENDCASE_CODE                          = 184, // GHM 20100310

    FNUSERBAR_CODE                          = 185, // GHM 20100414

    FNMESSAGEOVERRDIES_CODE                 = 186, // GHM 20100518

    FNTRACE_CODE                            = 187, // GHM 20100518

    FNSETVALUESETS_CODE                     = 188, // GHM 20100523

    FNEXECPFF_CODE                          = 189, // GHM 20100601

    FNSEEK_CODE                             = 190, // GHM 20100602

    FNGETCAPTURETYPE_CODE                   = 191, // GHM 20100608
    FNSETCAPTURETYPE_CODE                   = 192, // GHM 20100608

    FNSETFONT_CODE                          = 193, // GHM 20100618
    FNGETORIENTATION_CODE                   = 194, // GHM 20100618
    FNSETORIENTATION_CODE                   = 195, // GHM 20100618

    FNPATHNAME_CODE                         = 196, // GHM 20110107

    FNGPS_CODE                              = 197, // GHM 20110223

    FNLOW_CODE                              = 198, // GHM 20110301
    FNHIGH_CODE                             = 199, // GHM 20110301
    FNGETRECORD_CODE                        = 200, // GHM 20110302

    FNSETCAPTUREPOS_CODE                    = 201, // GHM 20110502

    FNABS_CODE                              = 202, // GHM 20110721
    FNRANDOMIN_CODE                         = 203, // GHM 20110721
    FNRANDOMIZEVS_CODE                      = 204, // GHM 20110811
    FNGETUSERNAME_CODE                      = 205, // GHM 20111028

    // CSPro 5.0
    FNFILE_EMPTY_CODE                       = 206, // GHM 20120627

    // CSPro 5.0.2
    FNCHANGEKEYBOARD_CODE                   = 207, // GHM 20120820
    FNSETOUTPUT_CODE                        = 208, // GHM 20121126

    // CSPro 5.0.3
    FNSEEKMIN_CODE                          = 209, // GHM 20130119
    FNSEEKMAX_CODE                          = 210, // GHM 20130119
    FNDATEADD_CODE                          = 211, // GHM 20130225
    FNDATEVALID_CODE                        = 212, // GHM 20130703

    // CSPro 6.0
    FNGETOS_CODE                            = 213, // GHM 20131217
    FNGETOCCLABEL_CODE                      = 214, // GHM 20140226
    FNFREEALPHAMEM_CODE                     = 215, // GHM 20140228
    FNSETVALUE_CODE                         = 216, // GHM 20140228
    FNGETVALUE_CODE                         = 217, // GHM 20140422
    FNGETVALUEALPHA_CODE                    = 218, // GHM 20140422
    // previously showarray                 = 219, // GHM 20140423

    // CSPro 6.1
    FNSETOCCLABEL_CODE                      = 220, // GHM 20141006
    FNSHOWOCC_CODE                          = 221, // GHM 20141015
    FNHIDEOCC_CODE                          = 222, // GHM 20141015
    FNGETDEVICEID_CODE                      = 223, // GHM 20141023
    FNDIREXIST_CODE                         = 224, // GHM 20141024
    FNDIRCREATE_CODE                        = 225, // GHM 20141024
    // previously sync                      = 226, // GHM 20141024
    LISTVAR_CODE                            = 227, // GHM 20141106
    FNDIRLIST_CODE                          = 228, // GHM 20141107
    FNSYSPARM_CODE                          = 229, // GHM 20141217 (a new version)
    FNPUBLISHDATE_CODE                      = 234, // GHM 20150203 (it used to be 9999 because there is no
                                                   //               interpreted function, but now it reuses an unused code)


    // CSPro 6.2
    FNCONNECTION_CODE                       = 230, // GHM 20150421
    FNPROMPT_CODE                           = 231, // GHM 20150422
    FNGETIMAGE_CODE                         = 232, // GHM 20150809
    FNROUND_CODE                            = 233, // GHM 20150821

    // CSPro 6.3
    // previously uuid                      = 234, // GHM 20151130
    FNSAVEPARTIAL_CODE                      = 235, // GHM 20151216 (a new version)
    FNSAVESETTING_CODE                      = 241, // 20160213
    FNLOADSETTING_CODE                      = 242, // 20160213

    // CSPro 7.0
    FNSYNC_CONNECT_CODE                     = 236,
    FNSYNC_DISCONNECT_CODE                  = 237,
    FNSYNC_DATA_CODE                        = 238,
    FNSYNC_FILE_CODE                        = 239,
    FNSYNC_SERVER_CODE                      = 240,
    FNGETCASELABEL_CODE                     = 243, // 20160728
    FNSETCASELABEL_CODE                     = 244, // 20160728
    FNISPARTIAL_CODE                        = 245, // 20160928 (a new version)
    FNSETOPERATORID_CODE                    = 246, // 20161004
    FNGETNOTE_CODE                          = 247, // 20161005 (a new version)
    FNEDITNOTE_CODE                         = 248, // 20161005 (a new version)
    FNPUTNOTE_CODE                          = 249, // 20161005 (a new version)
    FNISVERIFIED_CODE                       = 250, // 20161031
    // previously forcase                   = 251, // 20161101
    FNTIMESTAMP_CODE                        = 252, // 20161115
    // previously keylist                   = 253, // 20161116
    FNDIAGNOSTICS_CODE                      = 254, // 20161215
    FNCOMPRESS_CODE                         = 255, // 20161219
    FNDECOMPRESS_CODE                       = 256, // 20161219
    ASK_CODE                                = 257, // 20170321

    // CSPro 7.1
    // previously countcases                = 258, // 20170527
    FNGETPROPERTY_CODE                      = 259, // 20170607
    FNSETPROPERTY_CODE                      = 260, // 20170607
    FNLOGTEXT_CODE                          = 261, // 20170705
    FNWARNING_CODE                          = 262, // 20170705
    FNTR_CODE                               = 263, // 20170807
    FNUUID_CODE                             = 264, // 20170912 (a new version)
    FNPARADATA_CODE                         = 265, // 20170918
    FNSQLQUERY_CODE                         = 266, // 20170918
    FNPRE77_REPORT_CODE                     = 267, // 20171025
    FNPRE77_SETREPORTDATA_CODE              = 268, // 20171025
    FNSHOW_CODE                             = 269, // 20171116 (a new version)
    FNSHOWARRAY_CODE                        = 270, // 20171116 (a new version)
    // previously selcase                   = 271, // 20171116 (a new version)
    FNTIMESTRING_CODE                       = 272, // 20171206

    // CSPro 7.2
    STRING_LITERAL_CODE                     = 273, // a new version of LIT_CODE
    SYMBOL_RESET_CODE                       = 274, // 20180517
    DECRYPT_STRING_CODE                     = 275, // 20180522
    FNDIRDELETE_CODE                        = 276, // ALW 20180525
    ARRAYVAR_CODE                           = 277, // 20180530
    TVAR_CODE                               = 278, // previously code 36
    EXIT_CODE                               = 279, // 20180612 (a new version)
    FNGETBLUETOOTHNAME_CODE                 = 280,
    FNREGEXMATCH_CODE                       = 281, // ALW 20180703
    BLOCK_CODE                              = 282, // 20181015 (no function associated)
    FNGETVALUELABEL_CODE                    = 283, // 20181219

    // CSPro 7.3
    ARRAYFN_CLEAR_CODE                      = 284, // 20190213
    ARRAYFN_LENGTH_CODE                     = 285, // 20190213
    MAPFN_SHOW_CODE                         = 286,
    MAPFN_HIDE_CODE                         = 287,
    MAPFN_ADD_MARKER_CODE                   = 288,
    MAPFN_SET_MARKER_IMAGE_CODE             = 289,
    MAPFN_SET_MARKER_TEXT_CODE              = 290,
    MAPFN_SET_MARKER_ON_CLICK_CODE          = 291,
    MAPFN_SET_MARKER_ON_CLICK_INFO_CODE     = 292,
    MAPFN_SET_MARKER_DESCRIPTION_CODE       = 293,
    MAPFN_SET_MARKER_ON_DRAG_CODE           = 294,
    MAPFN_SET_MARKER_LOCATION_CODE          = 295,
    MAPFN_GET_MARKER_LATITUDE_CODE          = 296,
    MAPFN_REMOVE_MARKER_CODE                = 297,
    MAPFN_SET_ON_CLICK_CODE                 = 298,
    MAPFN_SHOW_CURRENT_LOCATION_CODE        = 299,
    MAPFN_ADD_TEXT_BUTTON_CODE              = 300,
    MAPFN_ADD_IMAGE_BUTTON_CODE             = 301,
    MAPFN_REMOVE_BUTTON_CODE                = 302,
    MAPFN_SET_BASE_MAP_CODE                 = 303,
    MAPFN_SET_TITLE_CODE                    = 304,
    MAPFN_ZOOM_TO_CODE                      = 305,
    LISTFN_ADD_CODE                         = 306, // 20190226
    LISTFN_CLEAR_CODE                       = 307, // 20190226
    LISTFN_INSERT_CODE                      = 308, // 20190226
    LISTFN_LENGTH_CODE                      = 309, // 20190226
    LISTFN_REMOVE_CODE                      = 310, // 20190226
    LISTFN_SEEK_CODE                        = 311, // 20190529
    LISTFN_SHOW_CODE                        = 312, // 20190226
    LISTFN_COMPUTE_CODE                     = 313, // 20190311
    VALUESETFN_ADD_CODE                     = 314, // 20190304
    VALUESETFN_CLEAR_CODE                   = 315, // 20190304
    VALUESETFN_REMOVE_CODE                  = 316, // 20190304
    VALUESETFN_SHOW_CODE                    = 317, // 20190311
    VALUESETFN_COMPUTE_CODE                 = 318, // 20190311
    FN_VARIABLE_VALUE_CODE                  = 319, // 20190605
    MAPFN_CLEAR_MARKERS_CODE                = 320,
    MAPFN_CLEAR_BUTTONS_CODE                = 321,
    MAPFN_GET_LAST_CLICK_LATITUDE_CODE      = 322,
    MAPFN_GET_LAST_CLICK_LONGITUDE_CODE     = 323,
    MAPFN_GET_MARKER_LONGITUDE_CODE         = 324,
    FNPATHCONCAT_CODE                       = 325, // 20190617
    FNVIEW_CODE                             = 326, // 20190617
    PFFFN_EXEC_CODE                         = 327, // 20190626
    PFFFN_GETPROPERTY_CODE                  = 328, // 20190626
    PFFFN_LOAD_CODE                         = 329, // 20190626
    PFFFN_SAVE_CODE                         = 330, // 20190626
    PFFFN_SETPROPERTY_CODE                  = 331, // 20190626
    VALUESETFN_LENGTH_CODE                  = 332, // 20190812

    // CSPro 7.4
    FNISCHECKED_CODE                        = 333, // 20190903
    FNPROTECT_CODE                          = 334, // 20191106
    WHEN_CODE                               = 335, // 20191112
	FNSYNC_APP_CODE                         = 336, // 20191118
    FNFILETIME_CODE                         = 337, // 20191202
    RECODE_CODE                             = 338, // 20191204
    FORCASE_CODE                            = 339, // 20200131 (a new version)
    FNSELCASE_CODE                          = 340, // 20200131
    FNCOUNTCASES_CODE                       = 341, // 20200131 (a new version)
    FNKEYLIST_CODE                          = 342, // 20200131 (a new version)
    BARCODEFN_READ_CODE                     = 343, // 20200218
    FNHASH_CODE                             = 344, // 20200228
    FNSYNC_MESSAGE_CODE                     = 345, // 20200316
    SYSTEMAPPFN_CLEAR_CODE                  = 346, // 20200318
    SYSTEMAPPFN_SETARGUMENT_CODE            = 347, // 20200318
    SYSTEMAPPFN_GETRESULT_CODE              = 348, // 20200318
    SYSTEMAPPFN_EXEC_CODE                   = 349, // 20200318
    FNSTARTSWITH_CODE                       = 350, // 20200416
    PFFFN_COMPUTE_CODE                      = 351, // 20200420

    // CSPro 7.5
    AUDIOFN_CLEAR_CODE                      = 352, // 20200714
    AUDIOFN_CONCAT_CODE                     = 353, // 20200714
    AUDIOFN_LOAD_CODE                       = 354, // 20200714
    AUDIOFN_PLAY_CODE                       = 355, // 20200714
    AUDIOFN_SAVE_CODE                       = 356, // 20200714
    AUDIOFN_STOP_CODE                       = 357, // 20200714
    AUDIOFN_RECORD_CODE                     = 358, // 20200714
    AUDIOFN_RECORD_INTERACTIVE_CODE         = 359, // 20200714
    AUDIOFN_COMPUTE_CODE                    = 360, // 20200714
    FNENCODE_CODE                           = 361, // 20200716
    LISTFN_SORT_CODE                        = 362, // 20190716
    LISTFN_REMOVEDUPLICATES_CODE            = 363, // 20190716
    LISTFN_REMOVEIN_CODE                    = 364, // 20190716
    PATHFN_CONCAT_CODE                      = 365, // 20200720
    PATHFN_GETDIRECTORYNAME_CODE            = 366, // 20200720
    PATHFN_GETEXTENSION_CODE                = 367, // 20200720
    PATHFN_GETFILENAME_CODE                 = 368, // 20200720
    PATHFN_GETFILENAMEWITHOUTEXTENSION_CODE = 369, // 20200720
    FNSYNC_PARADATA_CODE                    = 370, // 20200724
    HASHMAPVAR_CODE                         = 371, // 20200803
    HASHMAPFN_COMPUTE_CODE                  = 372, // 20200803
    HASHMAPFN_CLEAR_CODE                    = 373, // 20200803
    HASHMAPFN_CONTAINS_CODE                 = 374, // 20200803
    HASHMAPFN_LENGTH_CODE                   = 375, // 20200803
    HASHMAPFN_REMOVE_CODE                   = 376, // 20200803
    HASHMAPFN_GETKEYS_CODE                  = 377, // 20200804
    AUDIOFN_LENGTH_CODE                     = 378, // 20200805
    VALUESETFN_SORT_CODE                    = 379, // 20200819
    FNREPLACE_CODE                          = 380, // 20200820

    // CSPro 7.6
    FNINC_CODE                              = 381, // 20200825 (original version 20091201)
    FNUNIVERSE_CODE                         = 382, // 20200826 (original version 20100310)
    FREQ_UNNAMED_CODE                       = 383, // 20200924
    FREQFN_CLEAR_CODE                       = 384, // 20200925
    FREQFN_SAVE_CODE                        = 385, // 20200925
    FREQFN_TALLY_CODE                       = 386, // 20200925
    FREQFN_VIEW_CODE                        = 387, // 20200925
    FREQVAR_CODE                            = 388, // 20201001
    FREQFN_COMPUTE_CODE                     = 389, // 20201001
    WORKSTRING_CODE                         = 390, // 20201022

    // CSPro 7.7
    FNMAXOCC_CODE                           = 391, // 20210205 (a new version)
    FNSOCCURS_CODE                          = 392, // 20210208 (a new version)
    DATA_ACCESS_VALIDITY_CHECK_CODE         = 393, // 20210208
    DICTFN_COMPUTE_CODE                     = 394, // 20210209
    FNCURRENTKEY_CODE                       = 395, // 20210216
    IMAGEFN_COMPUTE_CODE                    = 396, // 20210219
    IMAGEFN_CAPTURESIGNATURE_CODE           = 397, // 20210219
    IMAGEFN_CLEAR_CODE                      = 398, // 20210219
    IMAGEFN_HEIGHT_CODE                     = 399, // 20210219
    IMAGEFN_LOAD_CODE                       = 400, // 20210219
    IMAGEFN_RESAMPLE_CODE                   = 401, // 20210219
    IMAGEFN_SAVE_CODE                       = 402, // 20210219
    IMAGEFN_TAKEPHOTO_CODE                  = 403, // 20210219
    IMAGEFN_VIEW_CODE                       = 404, // 20210219
    IMAGEFN_WIDTH_CODE                      = 405, // 20210219
    DOCUMENTFN_COMPUTE_CODE                 = 406, // 20210222
    DOCUMENTFN_CLEAR_CODE                   = 407, // 20210222
    DOCUMENTFN_LOAD_CODE                    = 408, // 20210222
    DOCUMENTFN_SAVE_CODE                    = 409, // 20210222
    DOCUMENTFN_VIEW_CODE                    = 410, // 20210222
    GEOMETRYFN_COMPUTE_CODE                 = 411, // 20210302
    GEOMETRYFN_CLEAR_CODE                   = 412, // 20210302
    GEOMETRYFN_LOAD_CODE                    = 413, // 20210302
    GEOMETRYFN_SAVE_CODE                    = 414, // 20210302
    MAPFN_ADD_GEOMETRY_CODE                 = 415, // 20210302
    MAPFN_REMOVE_GEOMETRY_CODE              = 416, // 20210302
    MAPFN_CLEAR_GEOMETRY_CODE               = 417, // 20210302
    GEOMETRYFN_TRACE_POLYGON_CODE           = 418, // 20210304
    GEOMETRYFN_WALK_POLYGON_CODE            = 419, // 20210304
    GEOMETRYFN_AREA_CODE                    = 420, // 20210315
    GEOMETRYFN_PERIMETER_CODE               = 421, // 20210315
    GEOMETRYFN_MIN_LATITUDE_CODE            = 422, // 20210315
    GEOMETRYFN_MAX_LATITUDE_CODE            = 423, // 20210315
    GEOMETRYFN_MIN_LONGITUDE_CODE           = 424, // 20210315
    GEOMETRYFN_MAX_LONGITUDE_CODE           = 425, // 20210315
    GEOMETRYFN_GET_PROPERTY_CODE            = 426, // 20210315
    GEOMETRYFN_SET_PROPERTY_CODE            = 427, // 20210315
    FNINADVANCE_CODE                        = 428, // 20210428
    MAPFN_SAVESNAPSHOT_CODE                 = 429, // 20210802
    FNSYNC_TIME_CODE                        = 430, // 20210910
    FNHTMLDIALOG_CODE                       = 431, // 20210916
    PATHFN_GETRELATIVEPATH_CODE             = 432, // 20210917
    PATHFN_SELECTFILE_CODE                  = 433, // 20210922
    FNINVOKE_CODE                           = 434, // 20211108
    REPORTFN_SAVE_CODE                      = 435, // 20210610
    REPORTFN_VIEW_CODE                      = 436, // 20210610
    REPORTFN_WRITE_CODE                     = 437, // 20211115
    FNSETBLUETOOTHNAME_CODE                 = 438, // 20211210

    // CSPro 8.0
    PERSISTENT_SYMBOL_RESET_CODE            = 439, // 20220103
    SYMBOLFN_GETJSON_CODE                   = 440, // 20221005
    SYMBOLFN_GETVALUEJSON_CODE              = 441, // 20221005
    SYMBOLFN_UPDATEVALUEFROMJSON_CODE       = 442, // 20221005
    BARCODEFN_CREATEQRCODE_CODE             = 443, // 20221219
    SCOPE_CHANGE_CODE                       = 444, // 20230124
    SET_DICT_ACCESS_CODE                    = 445, // 20230125
    WORKSTRING_COMPUTE_CODE                 = 446, // 20230127
    CSFN_ACTIONINVOKER_CODE                 = 447, // 20230202
    SYMBOLFN_GETNAME_CODE                   = 448, // 20230620
    SYMBOLFN_GETLABEL_CODE                  = 449, // 20230620
    MAPFN_CLEAR_CODE                        = 450, // 20230727
    ITEMFN_HASVALUE_CODE                    = 451, // 20230807
    ITEMFN_GETVALUELABEL_CODE               = 452, // 20230807
    ITEMFN_ISVALID_CODE                     = 453, // 20230807
    FNCOMPARENOCASE_CODE                    = 454, // 20230920
    CASEFN_VIEW_CODE                        = 455, // 20230924
};


namespace Logic
{
    // --------------------------------------------------------------------------
    // FunctionNamespace
    // --------------------------------------------------------------------------

    enum class FunctionNamespace : int
    {
        Barcode,
        Path,

        // --- CS_AUTOGENERATED_START -----------------------------------------------

        CS,
        CS_Application,
        CS_Clipboard,
        CS_Data,
        CS_Dictionary,
        CS_File,
        CS_Hash,
        CS_Localhost,
        CS_Logic,
        CS_Message,
        CS_Path,
        CS_Settings,
        CS_Sqlite,
        CS_System,
        CS_UI,

        // --- CS_AUTOGENERATED_END -------------------------------------------------
    };

    struct FunctionNamespaceDetails
    {
        const TCHAR* name;
        FunctionNamespace function_namespace;
        std::optional<FunctionNamespace> parent_function_namespace;
        const TCHAR* help_filename;
    };


    // --------------------------------------------------------------------------
    // FunctionCompilationType
    // --------------------------------------------------------------------------

    enum class FunctionCompilationType
    {
        ArgumentsVaryingN       =  200,
        FN2                     =  201,
        FN3                     =  202,
        FN4                     =  203,
        FN6                     =  205,
        CaseSearch              =  206,
        FN8                     =  207,
        FNS                     =  209,
        FN10                    =  210,
        FNC                     =  211,
        FNTC                    =  212,
        FNH                     =  213,
        ArgumentsFixedN         =  214,
        Message                 =  219,
        FNGR                    =  220,
        FNB                     =  221,
        FNID                    =  222,
        FNSRT                   =  223, // Chirag Sep 11, 2002
        FNG                     =  224,
        FNMAXOCC                =  225,
        SetFile                 =  226,
        FNINVALUESET            =  227,

        SetValueSet             =  232, // RHF Aug 28, 2002
        File                    =  233,
        FNEXECSYSTEM            =  234,
        FNSHOW                  =  235,

        FNITEMLIST              =  237, // 20091203 for functions that take a list of items but still use the FNN_NODE
        FNDECK                  =  238, // 20100120 for putdeck and getdeck functions
        Userbar                 =  239, // 20100414
        Trace                   =  240, // 20100518
        FNCAPTURETYPE           =  241, // 20100608
        UserInterface           =  242, // 20100618
        Path                    =  243, // 20110107
        GPS                     =  244, // 20110223
        Various                 =  245, // 20110721 for various small, unusual, functions
        FNOCCS                  =  246, // 20141006
        Sync                    =  247,
        Removed                 =  248,
        FNNOTE                  =  249,
        FNSTRPARM               =  250,
        FNPROPERTY              =  252,
        Paradata                =  253,
        Array                   =  254,
        Map                     =  255,
        List                    =  256,
        ValueSet                =  257,
        Pff                     =  258,
        CaseIO                  =  259,
        Barcode                 =  260,
        SystemApp               =  261,
        Audio                   =  262,
        HashMap                 =  263,
        Impute                  =  264,
        NamedFrequency          =  265,
        Image                   =  266,
        Document                =  267,
        Geometry                =  268,
        Invoke                  =  269,
        Report                  =  270,
        Symbol                  =  271,
        Clipboard               =  272,
        ArgumentSpecification   =  273,
        DictionaryVarious       =  274,
        CS                      =  275,
        Item                    =  276,
        Case                    =  277,
    };


    // --------------------------------------------------------------------------
    // FunctionDomain
    // --------------------------------------------------------------------------

    struct AllSymbolsDomain { bool operator==(const AllSymbolsDomain&) const { return true; } };

    using FunctionDomain = std::variant<SymbolType,
                                        AllSymbolsDomain,
                                        std::vector<SymbolType>,
                                        FunctionNamespace>;


    // --------------------------------------------------------------------------
    // FunctionDetails
    // --------------------------------------------------------------------------

    struct FunctionDetails
    {
        enum class StaticType : int { NeverStatic, AlwaysStatic, StaticWhenNecessary };

        const TCHAR* name;
        const TCHAR* tooltip;
        const TCHAR* help_filename;
        FunctionCode code;
        FunctionDomain function_domain;
        DataType return_data_type;
        FunctionCompilationType compilation_type;
        int number_arguments;
        StaticType static_type = StaticType::NeverStatic;
    };


    // --------------------------------------------------------------------------
    // FunctionTable
    // --------------------------------------------------------------------------

    namespace FunctionTable
    {
        ZLOGICO_API const MultipleReservedWordsTable<FunctionNamespaceDetails>& GetFunctionNamespaces();
        ZLOGICO_API bool IsFunctionNamespace(wstring_view text_sv, const std::variant<SymbolType, FunctionNamespace>& symbol_type_or_function_namespace,
                                             const FunctionNamespaceDetails** function_namespace_details = nullptr);

        // gets all functions, including those in the CS namespace
        ZLOGICO_API const std::vector<const FunctionDetails*>& GetFunctions();

        ZLOGICO_API const MultipleReservedWordsTable<FunctionDetails>& GetFunctionsTable();

        ZLOGICO_API bool IsFunction(wstring_view text_sv, const std::variant<SymbolType, FunctionNamespace>& symbol_type_or_function_namespace,
                                    const FunctionDetails** function_details = nullptr);

        // the extended version of IsFunction checks:
        // - if function_domain is AllSymbolsDomain, whether or not the function applies to ALL or SOME symbols (e.g., Symbol.updateValueFromJson)
        // - if function_domain is SymbolType, whether or not the function applies to that symbol type (e.g., Image.getName)
        ZLOGICO_API bool IsFunctionExtended(wstring_view text_sv, const FunctionDomain& function_domain,
                                            const FunctionDetails** function_details = nullptr);

        // checks if the text is a function belonging to a symbol or to the symbol's wrapped type
        ZLOGICO_API bool IsFunction(wstring_view text_sv, const Symbol& symbol, const FunctionDetails** function_details = nullptr);

        ZLOGICO_API const FunctionDetails* GetFunctionDetails(FunctionCode function_code);

        ZLOGICO_API const TCHAR* GetFunctionName(FunctionCode function_code);
        ZLOGICO_API const TCHAR* GetFunctionNamespaceName(FunctionNamespace function_namespace);
    }
}


// --------------------------------------------------------------------------
// definitions outside of the Logic namespace
// --------------------------------------------------------------------------

ZLOGICO_API bool operator==(const std::variant<SymbolType, Logic::FunctionNamespace>& symbol_type_or_function_namespace, const Logic::FunctionDomain& function_domain);
inline bool operator!=(const std::variant<SymbolType, Logic::FunctionNamespace>& symbol_type_or_function_namespace, const Logic::FunctionDomain& function_domain) { return !operator==(symbol_type_or_function_namespace, function_domain); }
inline bool operator==(const Logic::FunctionDomain& function_domain, const std::variant<SymbolType, Logic::FunctionNamespace>& symbol_type_or_function_namespace) { return operator==(symbol_type_or_function_namespace, function_domain); }
inline bool operator!=(const Logic::FunctionDomain& function_domain, const std::variant<SymbolType, Logic::FunctionNamespace>& symbol_type_or_function_namespace) { return !operator==(symbol_type_or_function_namespace, function_domain); }
