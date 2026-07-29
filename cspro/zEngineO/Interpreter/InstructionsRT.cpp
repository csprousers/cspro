#include "stdafx.h"
#include "IncludesRT.h"


#ifdef REFERENCE // EV_TODO

CIntDriver::pDoubleFunction CIntDriver::m_pExFuncs[] =
{
/*------------------┬-----------------------------------------------------*/
/*Op.code│ Function │      COMMANDS                                       */
/*-----------------─┴-----------------------------------------------------*/
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
/*  21 */   &CIntDriver::ex_string_compute,
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
/* 127 */   &CIntDriver::ex_minvalue_maxvalue, // minvalue
/* 128 */   &CIntDriver::ex_minvalue_maxvalue, // maxvalue
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
/* 160 */   &CIntDriver::ex_invalueset,
/* 161 */   &CIntDriver::ex_setvalueset,   // RHF Aug 28, 2002

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
/* 188 */   &CIntDriver::ex_setvaluesets,    // GHM 20100523
/* 189 */   &CIntDriver::ExExecPFF,          // GHM 20100601
/* 190 */   &CIntDriver::exseek,             // GHM 20100602
/* 191 */   &CIntDriver::ex_getcapturetype,  // GHM 20100608
/* 192 */   &CIntDriver::ex_setcapturetype,  // GHM 20100608
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
/* 204 */   &CIntDriver::ex_randomizevs,     // GHM 20110811
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
/* 232 */   &CIntDriver::ex_getimage,        // GHM 20150809
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
/* 254 */   &CIntDriver::ex_diagnostics,
/* 255 */   &CIntDriver::ex_compress,
/* 256 */   &CIntDriver::ex_decompress,
/* 257 */   &CIntDriver::exask,
/* 258 */   &CIntDriver::excountcases,
/* 259 */   &CIntDriver::ex_getproperty,
/* 260 */   &CIntDriver::ex_setproperty,
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
/* 314 */   &CIntDriver::ex_ValueSet_add,
/* 315 */   &CIntDriver::ex_ValueSet_clear,
/* 316 */   &CIntDriver::ex_ValueSet_remove,
/* 317 */   &CIntDriver::ex_ValueSet_show,
/* 318 */   &CIntDriver::ex_ValueSet_compute,
/* 319 */   &CIntDriver::exvariablevalue,
/* 320 */   &CIntDriver::ex_Map_clear_clearButtons_clearGeometry_clearMarkers, // Map.clearMarkers
/* 321 */   &CIntDriver::ex_Map_clear_clearButtons_clearGeometry_clearMarkers, // Map.clearButtons
/* 322 */   &CIntDriver::ex_Map_getLastClickLatitude_getLastClickLongitude, // Map.getLastClickLatitude
/* 323 */   &CIntDriver::ex_Map_getLastClickLatitude_getLastClickLongitude, // Map.getLastClickLongitude
/* 324 */   &CIntDriver::ex_Map_getMarkerLatitude_getMarkerLongitude,    // Map.getMarkerLongitude
/* 325 */   &CIntDriver::ex_Path_concat,
/* 326 */   &CIntDriver::ex_view,
/* 327 */   &CIntDriver::ex_Pff_exec,
/* 328 */   &CIntDriver::ex_Pff_getProperty,
/* 329 */   &CIntDriver::ex_Pff_load,
/* 330 */   &CIntDriver::ex_Pff_save,
/* 331 */   &CIntDriver::ex_Pff_setProperty,
/* 332 */   &CIntDriver::ex_ValueSet_length,
/* 333 */   &CIntDriver::ex_ischecked,
/* 334 */   &CIntDriver::ex_protect,
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
/* 351 */   &CIntDriver::ex_Pff_compute,
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
/* 379 */   &CIntDriver::ex_ValueSet_sort,
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
/* 446 */   &CIntDriver::ex_WorkString_compute,
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
/* 478 */   &CIntDriver::ex_ValueSet_removeDuplicates,
/* 479 */   &CIntDriver::ex_WorkVariable_compute,
/* 480 */   &CIntDriver::ex_Array_compute,
/* 481 */   &CIntDriver::ex_UserFunction_compute,


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

#endif
