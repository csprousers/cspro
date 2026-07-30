#include "stdafx.h"
#include "IncludesRT.h"


size_t LogicInterpreter::MaxInstructionCode_EV_TODO = 491; // EV_TODO remove

// OP    = instructions defined in LogicInterpreter that return Engine::Value
// OP_LI = instructions defined in LogicInterpreter that return double
// OP_ID = instructions defined in CIntDriver that return double
#define OP(instruction) Instruction(static_cast<Engine::Value (LogicInterpreter::*)(int)>(&LogicInterpreter::instruction))
#define OP_LI(instruction) Instruction(static_cast<double (LogicInterpreter::*)(int)>(&LogicInterpreter::instruction))
#define OP_ID(instruction) OP(ex_unimplemented_LogicInterpreter)

LogicInterpreter::Instruction LogicInterpreter::m_instructions[] =
{
/*------------------┬-----------------------------------------------------*/
/*Op.code│ Function │      COMMANDS                                       */
/*-----------------─┴-----------------------------------------------------*/
/*   0 */   OP_LI(ex_numeric_constant),
/*   1 */   OP_ID(exsvar),
/*   2 */   OP_ID(exmvar),
/*   3 */   OP_ID(excpt),
/*   4 */   OP_LI(ex_add),
/*   5 */   OP_LI(ex_sub),
/*   6 */   OP_LI(ex_mult),
/*   7 */   OP_LI(ex_div),
/*   8 */   OP_LI(ex_mod),
/*   9 */   OP_LI(ex_minus),
/*  10 */   OP_LI(ex_exp),
/*  11 */   OP_LI(ex_or),
/*  12 */   OP_LI(ex_and),
/*  13 */   OP_LI(ex_not),
/*  14 */   OP_LI(ex_eq),
/*  15 */   OP_LI(ex_ne),
/*  16 */   OP_LI(ex_le),
/*  17 */   OP_LI(ex_lt),
/*  18 */   OP_LI(ex_ge),
/*  19 */   OP_LI(ex_gt),
/*  20 */   OP_LI(ex_equ),
/*  21 */   OP_LI(ex_string_compute),
/*  22 */   OP_LI(ex_WorkVariable_evaluate),
/*  23 */   OP_ID(exif),
/*  24 */   OP_ID(exwhile),
/*  25 */   OP_ID(exbox),
/*  26 */   OP_LI(ex_string_literal), // an old implementation of ex_string_literal
/*  27 */   OP_ID(excharobj),
/*  28 */   OP_LI(ex_string_eq), // =
/*  29 */   OP_LI(ex_string_ne), // <>
/*  30 */   OP_LI(ex_string_le), // <=
/*  31 */   OP_LI(ex_string_lt), // <
/*  32 */   OP_LI(ex_string_ge), // >=
/*  33 */   OP_LI(ex_string_gt), // >
/*  34 */   OP_ID(excpttbl),
/*  35 */   OP_ID(exnoopAbort),
/*  36 */   OP_ID(exnoopAbort), // an old implementation of ex_Array_var
/*  37 */   OP_ID(exuserfunctioncall),
/*  38 */   OP_ID(exnoopAbort), // an old implementation of exexit
/*  39 */   OP_ID(exnoopAbort), // exfor_view,

/*───────┬──────────┬-----------------------------------------------------*/
/*Op.code│ Function │      Data Entry COMMANDS                            */
/*───────┴──────────┴-----------------------------------------------------*/
/*  40 */   OP_ID(exskipto),
/*  41 */   OP_ID(exadvance),
/*  42 */   OP_ID(exreenter),
/*  43 */   OP_ID(exnoinput),
/*  44 */   OP_ID(exendsect),
/*  45 */   OP_ID(exendlevl),
/*  46 */   OP_ID(exenter),

/*───────┬──────────┬-----------------------------------------------------*/
/*Op.code│ Function │      Batch COMMANDS                                 */
/*───────┴──────────┴-----------------------------------------------------*/
/*  47 */   OP_ID(exskipcase),
/*  48 */   OP_ID(exnoopAbort), // previously exnowrite
/*  49 */   OP_ID(exstop),
/*  50 */   OP_ID(exnoopIgnore_numeric), // previously exWriteForm
/*  51 */   OP_ID(exctab),
/*  52 */   OP_ID(exnoopAbort), // the removed, batch-only, exfreq
/*  53 */   OP_ID(exbreak),
/*  54 */   OP_ID(exexport),

/*───────┬──────────┬-----------------------------------------------------*/
/*Op.code│ Function │      future COMMANDS                                */
/*───────┴──────────┴-----------------------------------------------------*/

/*  55 */   OP_ID(exset), // VC Feb 23, 95

/*───────┬──────────┬-----------------------------------------------------*/
/*Op.code│ Function │      NUMERIC FUNCTIONS                              */
/*───────┴──────────┴-----------------------------------------------------*/
/*  56 */   OP_ID(exvisualvalue),
/*  57 */   OP_ID(exhighlight),
/*  58 */   OP_LI(ex_sqrt),
/*  59 */   OP_LI(ex_ex),
/*  60 */   OP_LI(ex_int),
/*  61 */   OP_LI(ex_log),
/*  62 */   OP_LI(ex_seed),
/*  63 */   OP_LI(ex_random),
/*  64 */   OP_ID(exnoccurs),
/*  65 */   OP_ID(exsoccurs_pre80),
/*  66 */   OP_ID(exnoopAbort), // exvoccurs
/*  67 */   OP_ID(excount),
/*  68 */   OP_ID(exsum),
/*  69 */   OP_ID(exavrge),
/*  70 */   OP_ID(exmin),
/*  71 */   OP_ID(exmax),
/*  72 */   OP_ID(exdisplay),
/*  73 */   OP_ID(exerrmsg),

/*───────┬──────────┬-----------------------------------------------------*/
/*Op.code│ Function │      ALPHA FUNCTIONS                                */
/*───────┴──────────┴-----------------------------------------------------*/
/*  74 */   OP_LI(ex_concat),
/*  75 */   OP_LI(ex_tonumber),
/*  76 */   OP_LI(ex_pos_poschar), // pos
/*  77 */   OP_LI(ex_compare),
/*  78 */   OP_LI(ex_length),
/*  79 */   OP_LI(ex_strip),
/*  80 */   OP_LI(ex_pos_poschar), // poschar
/*  81 */   OP_ID(exedit),

/*───────┬──────────┬-----------------------------------------------------*/
/*Op.code│ Function │      DATE FUNCTIONS                                 */
/*───────┴──────────┴-----------------------------------------------------*/
/*  82 */   OP_LI(ex_cmcode),
/*  83 */   OP_LI(ex_setlb_setub), // setub
/*  84 */   OP_LI(ex_setlb_setub), // setlb
/*  85 */   OP_LI(ex_adjuba),
/*  86 */   OP_LI(ex_adjlba),
/*  87 */   OP_LI(ex_adjlbi),
/*  88 */   OP_LI(ex_adjubi),
/*  89 */   OP_ID(exnoopAbort), // exdatechk
/*  90 */   OP_LI(ex_systime),
/*  91 */   OP_LI(ex_sysdate),

/*───────┬──────────┬-----------------------------------------------------*/
/*Op.code│ Function │      OTHER FUNCTIONS                                */
/*───────┴──────────┴-----------------------------------------------------*/
/*  92 */   OP_ID(exdemode),
/*  93 */   OP_LI(ex_special),
/*  94 */   OP_LI(ex_accept),
/*  95 */   OP_ID(exclrcase),

/*───────┬──────────┬-----------------------------------------------------*/
/*Op.ccde│ Function │      TABLES/CROSSTAB FUNCTIONS                      */
/*───────┴──────────┴-----------------------------------------------------*/
/*  96 */   OP_ID(exxtab),
/*  97 */   OP_ID(extblcoord), // tblrow
/*  98 */   OP_ID(extblcoord), // tblcol
/*  99 */   OP_ID(extblcoord), // tbllay
/* 100 */   OP_ID(extblsum),
/* 101 */   OP_ID(extblmed),

/*───────┬──────────┬-----------------------------------------------------*/
/*Op.code│ Function │      INDEXED FILES FUNCTIONS                        */
/*───────┴──────────┴-----------------------------------------------------*/
/* 102 */   OP_ID(exfilename),
/* 103 */   OP_ID(exnoopAbort),           // an old implementation of exselcase
/* 104 */   OP_ID(exnoopAbort),           // previously a locked version of exselcase
/* 105 */   OP_ID(exloadcase),
/* 106 */   OP_ID(exnoopAbort),           // previously a locked version of exloadcase
/* 107 */   OP_ID(exretrieve),
/* 108 */   OP_ID(exnoopAbort),           // previously a locked version of exretrieve
/* 109 */   OP_ID(exwritecase),
/* 110 */   OP_ID(exdelcase),
/* 111 */   OP_ID(exfind_locate),         // find
/* 112 */   OP_ID(exkey),                 // key
/* 113 */   OP_ID(ex_open),
/* 114 */   OP_ID(ex_close),
/* 115 */   OP_ID(exfind_locate),         // locate
/* 116 */   OP_ID(exnoopAbort),           // previously exexec
/* 117 */   OP_ID(exnoopAbort),           // an old implementation of exsysparm
/* 118 */   OP_ID(exnoopIgnore_numeric),  // previously exioerror
/* 119 */   OP_ID(exnoopAbort),           // previously exwriteacl
/* 120 */   OP_ID(exnoopIgnore_numeric),  // previously exdemenu
/* 121 */   OP_ID(exsetattr),
/* 122 */   OP_ID(exnoopAbort),           // previously set file

/*───────┬──────────┬-----------------------------------------------------*/
/*Op.code│ Function │      Data Entry COMMANDS - extension                */
/*───────┴──────────┴-----------------------------------------------------*/
/* 123 */   OP_ID(exfor_dict),

/*───────┬──────────┬-----------------------------------------------------*/
/*Op.code│ Function │      NUMERIC FUNCTIONS   - extension                */
/*───────┴──────────┴-----------------------------------------------------*/
/* 124 */   OP_ID(exnmembers),
/* 125 */   OP_ID(exnoopAbort),  // previously exset_output
/* 126 */   OP_ID(exnoopAbort),  // previously exrecord
/* 127 */   OP_LI(ex_minvalue_maxvalue), // minvalue
/* 128 */   OP_LI(ex_minvalue_maxvalue), // maxvalue
/* 129 */   OP_ID(exfor_group),
/* 130 */   OP_ID(exnoopAbort),  // previously extbd
/* 131 */   OP_ID(exnoopAbort),  // GROUP_CODE
/* 132 */   OP_ID(exfucall),
/* 133 */   OP_LI(ex_in),        // RHC Oct 16, 2000
/* 134 */   OP_ID(ex_do),        // RHC Oct 16, 2000
/* 135 */   OP_ID(ex_impute),    // RHF Oct 25, 2000
/* 136 */   OP_ID(exfncurocc),   // RHC Oct 16, 2000
/* 137 */   OP_ID(exfntotocc),   // RHC Oct 16, 2000
/* 138 */   OP_ID(exupdate),     // RHF Nov 17, 2000
/* 139 */   OP_ID(exwrite),      // RHF Dec 16, 2000
/* 140 */   OP_ID(exnoopAbort),  // an old implementation of exispartial [original: RHF Mar 06, 2001]
/* 141 */   OP_ID(exfor_relation),
/* 142 */   OP_ID(exnoopAbort),  // REL_CODE
/* 143 */   OP_ID(exgetbuffer),
/* 144 */   OP_ID(exinsert_delete),  // Chirag, Jul 22, 2002
/* 145 */   OP_ID(exinsert_delete),  // Chirag, Sep 11, 2002
/* 146 */   OP_ID(exsort),           // Chirag, Sep 11, 2002
/* 147 */   OP_ID(exgetlabel),       // RHF Aug 25, 2000
/* 148 */   OP_ID(exgetlabel),       // getsymbol RHF Mar 23, 2001
/* 149 */   OP_ID(exnoopAbort),      // an old implementation of exgetnote  [original: RHF Nov 19, 2002]
/* 150 */   OP_ID(exnoopAbort),      // an old implementation of exeditnote [original: RHF Nov 19, 2002]
/* 151 */   OP_ID(exnoopAbort),      // an old implementation of exputnote  [original: RHF Nov 19, 2002]
/* 152 */   OP_ID(exmaketext),       // RHF Jun 08, 2001

/* 153 */   OP_ID(exmoveto),         // RHF Dec 09, 2003
/* 154 */   OP_ID(exnoopAbort),      // an old implementation of exsavepartial [original: RHF Dec 01, 2003]
/* 155 */   OP_ID(exgetoperatorid),  // RHF Dec 03, 2003
/* 156 */   OP_ID(exfornext),        // RHC Sep 04, 2000
/* 157 */   OP_ID(exforbreak),       // RHC Sep 04, 2000
/* 158 */   OP_ID(ex_setfile),
/* 159 */   OP_ID(exmaxocc_pre80),
/* 160 */   OP_LI(ex_invalueset),
/* 161 */   OP_LI(ex_setvalueset),   // RHF Aug 28, 2002

// RHF INIC Oct 15, 2004
/* 162 */   OP_ID(exfilecreate),
/* 163 */   OP_ID(exfileexist),
/* 164 */   OP_ID(exfiledelete),
/* 165 */   OP_ID(ex_filecopy),
/* 166 */   OP_ID(ex_filerename),
/* 167 */   OP_ID(exfilesize),
/* 168 */   OP_ID(exfileconcat),
/* 169 */   OP_ID(exfileread),
/* 170 */   OP_ID(exfilewrite),
// RHF END Oct 15, 2004

/* 171 */   OP_ID(ExExecSystem),

/* 172 */   OP_ID(exnoopAbort),        // an old implementation of exshow
/* 173 */   OP_ID(exshowlist),

/* 174 */   OP_LI(ex_tolower_toupper), // GHM 20091202 tolower
/* 175 */   OP_LI(ex_tolower_toupper), // GHM 20091202 toupper
/* 176 */   OP_ID(excountvalid),       // GHM 20091202
/* 177 */   OP_ID(exnoopIgnore_string),// GHM 20091208 previously itemlist
/* 178 */   OP_ID(exswap),             // GHM 20100105
/* 179 */   OP_LI(ex_datediff),        // GHM 20100119
/* 180 */   OP_ID(exdeckarray),        // GHM 20100119 putdeck
/* 181 */   OP_ID(exdeckarray),        // GHM 20100119 getdeck
/* 182 */   OP_ID(ex_getlanguage),     // GHM 20100309
/* 183 */   OP_ID(ex_setlanguage),     // GHM 20100309
/* 184 */   OP_ID(exendcase),          // GHM 20100310
/* 185 */   OP_ID(exuserbar),          // GHM 20100414
/* 186 */   OP_ID(exmessageoverrides), // GHM 20100518
/* 187 */   OP_ID(ex_trace),           // GHM 20100518
/* 188 */   OP_LI(ex_setvaluesets),    // GHM 20100523
/* 189 */   OP_ID(ExExecPFF),          // GHM 20100601
/* 190 */   OP_ID(exseek),             // GHM 20100602
/* 191 */   OP_ID(ex_getcapturetype),  // GHM 20100608
/* 192 */   OP_ID(ex_setcapturetype),  // GHM 20100608
/* 193 */   OP_LI(ex_setfont),         // GHM 20100618
/* 194 */   OP_ID(exorientation),      // GHM 20100618 getorientation
/* 195 */   OP_ID(exorientation),      // GHM 20100618 setorientation
/* 196 */   OP_ID(ex_pathname),        // GHM 20110107
/* 197 */   OP_ID(exgps),              // GHM 20110223
/* 198 */   OP_LI(ex_low_high),        // GHM 20110301 low
/* 199 */   OP_LI(ex_low_high),        // GHM 20110301 high
/* 200 */   OP_ID(exgetrecord),        // GHM 20110302
/* 201 */   OP_ID(ex_setcapturepos),   // GHM 20110502
/* 202 */   OP_LI(ex_abs),             // GHM 20110721
/* 203 */   OP_LI(ex_randomin),        // GHM 20110721
/* 204 */   OP_LI(ex_randomizevs),     // GHM 20110811
/* 205 */   OP_LI(ex_getusername),     // GHM 20111028
/* 206 */   OP_ID(exfileempty),        // GHM 20120627
/* 207 */   OP_ID(ex_changekeyboard),  // GHM 20120820
/* 208 */   OP_ID(ex_setoutput),       // GHM 20121126
/* 209 */   OP_ID(exseekMinMax),       // GHM 20130119
/* 210 */   OP_ID(exseekMinMax),       // GHM 20130119
/* 211 */   OP_LI(ex_dateadd),         // GHM 20130225
/* 212 */   OP_LI(ex_datevalid),       // GHM 20130703
/* 213 */   OP_LI(ex_getos),           // GHM 20131217
/* 214 */   OP_ID(exgetocclabel),      // GHM 20140226
/* 215 */   OP_ID(exfreealphamem),     // GHM 20140228
/* 216 */   OP_ID(exsetvalue),         // GHM 20140228
/* 217 */   OP_ID(exgetvalue),         // GHM 20140422
/* 218 */   OP_ID(exgetvaluealpha),    // GHM 20140422
/* 219 */   OP_ID(exnoopAbort),        // GHM 20140423 an old implementation of exshowarray
/* 220 */   OP_ID(exsetocclabel),      // GHM 20141006
/* 221 */   OP_ID(exshowocc),          // GHM 20141015 showocc
/* 222 */   OP_ID(exshowocc),          // GHM 20141015 hideocc
/* 223 */   OP_ID(ex_getdeviceid),     // GHM 20141023
/* 224 */   OP_ID(exdirexist),         // GHM 20141024
/* 225 */   OP_ID(exdircreate),        // GHM 20141024
/* 226 */   OP_ID(exnoopAbort),        // GHM 20141024 previously sync
/* 227 */   OP_ID(ex_List_var),        // GHM 20141106
/* 228 */   OP_ID(exdirlist),          // GHM 20141107
/* 229 */   OP_ID(ex_sysparm),         // GHM 20141217
/* 230 */   OP_ID(ex_connection),      // GHM 20150421
/* 231 */   OP_ID(ex_prompt),          // GHM 20150422
/* 232 */   OP_ID(ex_getimage),        // GHM 20150809
/* 233 */   OP_ID(ex_round),           // GHM 20150821
/* 234 */   OP_ID(exnoopAbort),        // GHM 20151130 an old implementation of exuuid ... now a publishdate placeholder
/* 235 */   OP_ID(exsavepartial),      // GHM 20151216
/* 236 */   OP_ID(ex_syncconnect),
/* 237 */   OP_ID(ex_syncdisconnect),
/* 238 */   OP_ID(ex_syncdata),
/* 239 */   OP_ID(ex_syncfile),
/* 240 */   OP_ID(ex_syncserver),
/* 241 */   OP_ID(ex_savesetting),
/* 242 */   OP_ID(ex_loadsetting),
/* 243 */   OP_ID(exgetcaselabel),
/* 244 */   OP_ID(exsetcaselabel),
/* 245 */   OP_ID(exispartial),
/* 246 */   OP_ID(exsetoperatorid),
/* 247 */   OP_ID(exgetnote),
/* 248 */   OP_ID(exeditnote),
/* 249 */   OP_ID(exputnote),
/* 250 */   OP_ID(exisverified),
/* 251 */   OP_ID(exforcase),
/* 252 */   OP_ID(ex_timestamp),
/* 253 */   OP_ID(exkeylist),
/* 254 */   OP_ID(ex_diagnostics),
/* 255 */   OP_ID(ex_compress),
/* 256 */   OP_ID(ex_decompress),
/* 257 */   OP_ID(exask),
/* 258 */   OP_ID(excountcases),
/* 259 */   OP_ID(ex_getproperty),
/* 260 */   OP_ID(ex_setproperty),
/* 261 */   OP_ID(exlogtext),
/* 262 */   OP_ID(exwarning),
/* 263 */   OP_ID(ex_tr),
/* 264 */   OP_LI(ex_uuid),
/* 265 */   OP_ID(ex_paradata),
/* 266 */   OP_ID(exsqlquery),
/* 267 */   OP_ID(expre77_report),
/* 268 */   OP_ID(expre77_setreportdata),
/* 269 */   OP_ID(exshow),
/* 270 */   OP_ID(exshowarray),
/* 271 */   OP_ID(exselcase),
/* 272 */   OP_LI(ex_timestring),
/* 273 */   OP_LI(ex_string_literal),
/* 274 */   OP_ID(exsymbolreset),
/* 275 */   OP_LI(ex_decryptstring),
/* 276 */   OP_ID(exdirdelete),
/* 277 */   OP_LI(ex_Array_var),
/* 278 */   OP_ID(extvar),
/* 279 */   OP_ID(ex_exit),
/* 280 */   OP_ID(ex_getbluetoothname),
/* 281 */   OP_LI(ex_regexmatch),
/* 282 */   OP_ID(exnoopAbort), // BLOCK_CODE
/* 283 */   OP_ID(exgetvaluelabel),
/* 284 */   OP_LI(ex_Array_clear),
/* 285 */   OP_LI(ex_Array_length),
/* 286 */   OP_LI(ex_Map_show),
/* 287 */   OP_LI(ex_Map_hide),
/* 288 */   OP_LI(ex_Map_addMarker),
/* 289 */   OP_LI(ex_Map_setMarkerImage),
/* 290 */   OP_LI(ex_Map_setMarkerText),
/* 291 */   OP_LI(ex_Map_setMarkerOnClick_setMarkerOnClickInfo), // Map.setMarkerOnClick
/* 292 */   OP_LI(ex_Map_setMarkerOnClick_setMarkerOnClickInfo), // Map.setMarkerOnClickInfo
/* 293 */   OP_LI(ex_Map_setMarkerDescription),
/* 294 */   OP_LI(ex_Map_setMarkerOnDrag),
/* 295 */   OP_LI(ex_Map_setMarkerLocation),
/* 296 */   OP_LI(ex_Map_getMarkerLatitude_getMarkerLongitude),  // Map.getMarkerLatitude
/* 297 */   OP_LI(ex_Map_removeMarker),
/* 298 */   OP_LI(ex_Map_setOnClick),
/* 299 */   OP_LI(ex_Map_showCurrentLocation),
/* 300 */   OP_LI(ex_Map_addTextButton),
/* 301 */   OP_LI(ex_Map_addImageButton),
/* 302 */   OP_LI(ex_Map_removeButton),
/* 303 */   OP_LI(ex_Map_setBaseMap),
/* 304 */   OP_LI(ex_Map_setTitle),
/* 305 */   OP_LI(ex_Map_zoomTo),
/* 306 */   OP_LI(ex_List_add),
/* 307 */   OP_LI(ex_List_clear),
/* 308 */   OP_LI(ex_List_insert),
/* 309 */   OP_LI(ex_List_length),
/* 310 */   OP_LI(ex_List_remove),
/* 311 */   OP_LI(ex_List_seek),
/* 312 */   OP_LI(ex_List_show),
/* 313 */   OP_LI(ex_List_compute),
/* 314 */   OP_LI(ex_ValueSet_add),
/* 315 */   OP_LI(ex_ValueSet_clear),
/* 316 */   OP_LI(ex_ValueSet_remove),
/* 317 */   OP_LI(ex_ValueSet_show),
/* 318 */   OP_LI(ex_ValueSet_compute),
/* 319 */   OP_ID(exvariablevalue),
/* 320 */   OP_LI(ex_Map_clear_clearButtons_clearGeometry_clearMarkers), // Map.clearMarkers
/* 321 */   OP_LI(ex_Map_clear_clearButtons_clearGeometry_clearMarkers), // Map.clearButtons
/* 322 */   OP_LI(ex_Map_getLastClickLatitude_getLastClickLongitude), // Map.getLastClickLatitude
/* 323 */   OP_LI(ex_Map_getLastClickLatitude_getLastClickLongitude), // Map.getLastClickLongitude
/* 324 */   OP_LI(ex_Map_getMarkerLatitude_getMarkerLongitude),    // Map.getMarkerLongitude
/* 325 */   OP_ID(ex_Path_concat),
/* 326 */   OP_LI(ex_view),
/* 327 */   OP_LI(ex_Pff_exec),
/* 328 */   OP_LI(ex_Pff_getProperty),
/* 329 */   OP_LI(ex_Pff_load),
/* 330 */   OP_LI(ex_Pff_save),
/* 331 */   OP_LI(ex_Pff_setProperty),
/* 332 */   OP_LI(ex_ValueSet_length),
/* 333 */   OP_LI(ex_ischecked),
/* 334 */   OP_ID(ex_protect),
/* 335 */   OP_LI(ex_when),
/* 336 */   OP_ID(ex_syncapp),
/* 337 */   OP_ID(exfiletime),
/* 338 */   OP_LI(ex_recode),
/* 339 */   OP_ID(exforcase),
/* 340 */   OP_ID(exselcase),
/* 341 */   OP_ID(excountcases),
/* 342 */   OP_ID(exkeylist),
/* 343 */   OP_LI(ex_Barcode_read),
/* 344 */   OP_LI(ex_hash),
/* 345 */   OP_ID(ex_syncmessage),
/* 346 */   OP_LI(ex_SystemApp_clear),
/* 347 */   OP_LI(ex_SystemApp_setArgument),
/* 348 */   OP_LI(ex_SystemApp_getResult),
/* 349 */   OP_LI(ex_SystemApp_exec),
/* 350 */   OP_LI(ex_startswith),
/* 351 */   OP_LI(ex_Pff_compute),
/* 352 */   OP_LI(ex_Audio_clear),
/* 353 */   OP_LI(ex_Audio_concat),
/* 354 */   OP_LI(ex_Audio_load),
/* 355 */   OP_LI(ex_Audio_play),
/* 356 */   OP_LI(ex_Audio_save),
/* 357 */   OP_LI(ex_Audio_stop),
/* 358 */   OP_LI(ex_Audio_record),
/* 359 */   OP_LI(ex_Audio_recordInteractive),
/* 360 */   OP_LI(ex_Audio_compute),
/* 361 */   OP_LI(ex_encode),
/* 362 */   OP_LI(ex_List_sort),
/* 363 */   OP_LI(ex_List_removeDuplicates),
/* 364 */   OP_LI(ex_List_removeIn),
/* 365 */   OP_ID(ex_Path_concat),
/* 366 */   OP_ID(ex_Path_getDirectoryName),
/* 367 */   OP_ID(ex_Path_getExtension),
/* 368 */   OP_ID(ex_Path_getFileName),
/* 369 */   OP_ID(ex_Path_getFileNameWithoutExtension),
/* 370 */   OP_ID(ex_syncparadata),
/* 371 */   OP_LI(ex_HashMap_var),
/* 372 */   OP_LI(ex_HashMap_compute),
/* 373 */   OP_LI(ex_HashMap_clear),
/* 374 */   OP_LI(ex_HashMap_contains),
/* 375 */   OP_LI(ex_HashMap_length),
/* 376 */   OP_LI(ex_HashMap_remove),
/* 377 */   OP_LI(ex_HashMap_getKeys),
/* 378 */   OP_LI(ex_Audio_length),
/* 379 */   OP_LI(ex_ValueSet_sort),
/* 380 */   OP_LI(ex_replace),
/* 381 */   OP_LI(ex_inc),
/* 382 */   OP_ID(exuniverse),
/* 383 */   OP_ID(ex_Freq_unnamed),
/* 384 */   OP_ID(ex_Freq_clear),
/* 385 */   OP_ID(ex_Freq_save),
/* 386 */   OP_ID(ex_Freq_tally),
/* 387 */   OP_ID(ex_Freq_view),
/* 388 */   OP_ID(ex_Freq_var),
/* 389 */   OP_ID(ex_Freq_compute),
/* 390 */   OP_LI(ex_WorkString_evaluate),
/* 391 */   OP_ID(exmaxocc),
/* 392 */   OP_ID(exsoccurs),
/* 393 */   OP_ID(exDataAccessValidityCheck),
/* 394 */   OP_ID(exdictcompute),
/* 395 */   OP_ID(exkey), // currentkey
/* 396 */   OP_LI(ex_Image_compute),
/* 397 */   OP_LI(ex_Image_captureSignature_takePhoto), // Image.captureSignature
/* 398 */   OP_LI(ex_Image_clear),
/* 399 */   OP_LI(ex_Image_width_height), // Image.height
/* 400 */   OP_LI(ex_Image_load),
/* 401 */   OP_LI(ex_Image_resample),
/* 402 */   OP_LI(ex_Image_save),
/* 403 */   OP_LI(ex_Image_captureSignature_takePhoto), // Image.takePhoto
/* 404 */   OP_LI(ex_Image_view),
/* 405 */   OP_LI(ex_Image_width_height), // Image.width
/* 406 */   OP_LI(ex_Document_compute),
/* 407 */   OP_LI(ex_Document_clear),
/* 408 */   OP_LI(ex_Document_load),
/* 409 */   OP_LI(ex_Document_save),
/* 410 */   OP_LI(ex_Document_view),
/* 411 */   OP_LI(ex_Geometry_compute),
/* 412 */   OP_LI(ex_Geometry_clear),
/* 413 */   OP_LI(ex_Geometry_load),
/* 414 */   OP_LI(ex_Geometry_save),
/* 415 */   OP_LI(ex_Map_addGeometry),
/* 416 */   OP_LI(ex_Map_removeGeometry),
/* 417 */   OP_LI(ex_Map_clear_clearButtons_clearGeometry_clearMarkers), // Map.clearGeometry
/* 418 */   OP_LI(ex_Geometry_tracePolygon_walkPolygon), // Geometry.tracePolygon
/* 419 */   OP_LI(ex_Geometry_tracePolygon_walkPolygon), // Geometry.walkPolygon
/* 420 */   OP_LI(ex_Geometry_area_perimeter), // Geometry.area
/* 421 */   OP_LI(ex_Geometry_area_perimeter), // Geometry.perimeter
/* 422 */   OP_LI(ex_Geometry_minLatitude_maxLatitude_minLongitude_maxLongitude), // Geometry.minLatitude
/* 423 */   OP_LI(ex_Geometry_minLatitude_maxLatitude_minLongitude_maxLongitude), // Geometry.maxLatitude
/* 424 */   OP_LI(ex_Geometry_minLatitude_maxLatitude_minLongitude_maxLongitude), // Geometry.minLongitude
/* 425 */   OP_LI(ex_Geometry_minLatitude_maxLatitude_minLongitude_maxLongitude), // Geometry.maxLongitude
/* 426 */   OP_LI(ex_Geometry_getProperty),
/* 427 */   OP_LI(ex_Geometry_setProperty),
/* 428 */   OP_ID(exinadvance),
/* 429 */   OP_LI(ex_Map_saveSnapshot),
/* 430 */   OP_ID(ex_synctime),
/* 431 */   OP_LI(ex_htmldialog),
/* 432 */   OP_ID(ex_Path_getRelativePath),
/* 433 */   OP_ID(ex_Path_selectFile),
/* 434 */   OP_ID(ex_invoke),
/* 435 */   OP_LI(ex_Report_save),
/* 436 */   OP_LI(ex_Report_view),
/* 437 */   OP_LI(ex_TextTemplate_write_writeEncoded_writeEncodedLine_writeLine), // Report.write prior to CSPro 8.1
/* 438 */   OP_ID(ex_setbluetoothname),
/* 439 */   OP_ID(expersistentsymbolreset),
/* 440 */   OP_LI(ex_Symbol_getJson_getValueJson), // symbol.getJson
/* 441 */   OP_LI(ex_Symbol_getJson_getValueJson), // symbol.getValueJson
/* 442 */   OP_LI(ex_Symbol_setValueFromJson),
/* 443 */   OP_LI(ex_Barcode_createQRCode), // Barcode.createQRCode + Image.createQRCode
/* 444 */   OP_ID(exScopeChange),
/* 445 */   OP_ID(exdictaccess),
/* 446 */   OP_LI(ex_WorkString_compute),
/* 447 */   OP_LI(ex_ActionInvoker),
/* 448 */   OP_LI(ex_Symbol_getName),
/* 449 */   OP_LI(ex_Symbol_getLabel),
/* 450 */   OP_LI(ex_Map_clear_clearButtons_clearGeometry_clearMarkers), // Map.clear
/* 451 */   OP_ID(exItem_hasValue_isValid), // Item.hasValue
/* 452 */   OP_ID(exItem_getValueLabel),
/* 453 */   OP_ID(exItem_hasValue_isValid), // Item.isValid
/* 454 */   OP_LI(ex_compareNoCase),
/* 455 */   OP_ID(exCase_view),
/* 456 */   OP_LI(ex_JavaScript_eval),
/* 457 */   OP_LI(ex_JavaScript_invoke),
/* 458 */   OP_LI(ex_JavaScript_hasValue),
/* 459 */   OP_LI(ex_JavaScript_getValueJson),
/* 460 */   OP_LI(ex_JavaScript_setValueFromJson),
/* 461 */   OP_LI(ex_JavaScript_getValue),
/* 462 */   OP_LI(ex_JavaScript_setValue),
/* 463 */   OP_LI(ex_JavaScript_UserFunctionCall),
/* 464 */   OP_LI(ex_TextTemplate_write_writeEncoded_writeEncodedLine_writeLine), // Report/StringWriter.write
/* 465 */   OP_LI(ex_TextTemplate_write_writeEncoded_writeEncodedLine_writeLine), // Report/StringWriter.writeEncoded
/* 466 */   OP_LI(ex_TextTemplate_write_writeEncoded_writeEncodedLine_writeLine), // Report/StringWriter.writeEncodedLine
/* 467 */   OP_LI(ex_TextTemplate_write_writeEncoded_writeEncodedLine_writeLine), // Report/StringWriter.writeLine
/* 468 */   OP_LI(ex_StringWriter_toString),
/* 469 */   OP_LI(ex_Image_getExif),
/* 470 */   OP_LI(ex_StringWriter_clear),
/* 471 */   OP_LI(ex_Video_compute),
/* 472 */   OP_LI(ex_Video_clear),
/* 473 */   OP_LI(ex_Video_load),
/* 474 */   OP_LI(ex_Video_save),
/* 475 */   OP_LI(ex_Video_length),
/* 476 */   OP_LI(ex_Video_width_height), // Video.width
/* 477 */   OP_LI(ex_Video_width_height), // Video.height
/* 478 */   OP_LI(ex_ValueSet_removeDuplicates),
/* 479 */   OP_LI(ex_WorkVariable_compute),
/* 480 */   OP_LI(ex_Array_compute),
/* 481 */   OP_LI(ex_UserFunction_compute),


            // placeholders to allow new logic functions to be added to an existing serialization
            // iteration without causing crashes to old builds at the same iteration
            OP_ID(exnoopAbortPlaceholderForFutureFunction),
            OP_ID(exnoopAbortPlaceholderForFutureFunction),
            OP_ID(exnoopAbortPlaceholderForFutureFunction),
            OP_ID(exnoopAbortPlaceholderForFutureFunction),
            OP_ID(exnoopAbortPlaceholderForFutureFunction),
            OP_ID(exnoopAbortPlaceholderForFutureFunction),
            OP_ID(exnoopAbortPlaceholderForFutureFunction),
            OP_ID(exnoopAbortPlaceholderForFutureFunction),
            OP_ID(exnoopAbortPlaceholderForFutureFunction),
            OP_ID(exnoopAbortPlaceholderForFutureFunction),
};


Engine::Value LogicInterpreter::ex_unimplemented_LogicInterpreter(const int program_index) // INTERPRETER_DLL_TODO remove
{
    const auto& function_call_node = GetNode<Nodes::FunctionCall>(program_index);
    const Logic::FunctionDetails* const function_details = Logic::FunctionTable::GetFunctionDetails(function_call_node.function_code);

    if( function_details != nullptr )
        throw CSProException("In this runtime environment, the function '%s' is not implemented.", function_details->name);

    throw CSProException("In this runtime environment, the instruction with code '%d' is not implemented.", static_cast<int>(function_call_node.function_code));
}
