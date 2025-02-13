#pragma once


// this is outside of the Logic namespace in order to avoid needing to
// change many references to these codes; it is also a enum, rather than
// an enum class, so that it doesn't have to constantly be cast to ints
// when being stuffed in compilation nodes
enum TokenCode : int
{
    Unspecified      =   0,

    TOKLPAREN        =  40,
    TOKRPAREN        =  41,
    TOKLBRACK        =  91,
    TOKRBRACK        =  93,
    TOKADDOP         =  43,
    TOKMINOP         =  45,
    TOKMODOP         =  37,
    TOKMULOP         =  42,
    TOKDIVOP         =  47,
    TOKATOP          =  64, // referred-sym //BUCEN_2003 Changes
    TOKEXPOP         =  94,
    TOKARROW         =  56,
    TOKDOUBLECOLON   =  57,
    TOKCOLON         =  58,
    TOKSEMICOLON     =  59,
    TOKCOMMA         =  44,

/*-----------------------------------------------------------------------*/
/*  compiler tokens                                                      */
/*-----------------------------------------------------------------------*/
    TOKEQOP           = 14,
    TOKNEOP           = 15,
    TOKLEOP           = 16,
    TOKLTOP           = 17,
    TOKGEOP           = 18,
    TOKGTOP           = 19,
    TOKMINUS         = 201,
    TOKEQUOP         = 202,

    TOKVAR           = 213,
    TOKCTE           = 214,
    TOKFUNCTION      = 215,
    TOKSECT          = 216,
    TOKFORM          = 217,
    TOKIMPLOP        = 218,
    TOKEOP           = 219,
    TOKERROR         = 220,
    TOKSCTE          = 221,
    TOKKWFILE        = 222,
    TOKDICT_PRE80    = 223,
    TOKKWCTAB        = 226,
    TOKGROUP         = 230, // for GROUPT names     // victor Aug 02, 99
    TOKFLOW_PRE80    = 231, // for Flow names       // victor Dec 28, 99

/*-----------------------------------------------------------------------*/
/*  reserved keywords                                                    */
/*-----------------------------------------------------------------------*/
    TOKANDOP        =  38, // also self-defined '&'
    TOKOROP         = 124, // also self-defined '|'
    TOKNOTOP        = 126, // also self-defined '~'
    TOKIF           = 301,
    TOKTHEN         = 302,
    TOKELSE         = 303,
    TOKELSEIF       = 304,
    TOKENDIF        = 305,
    TOKDO           = 306,
    TOKENDDO        = 307,
    TOKWHILE        = 308,
    TOKRECODE       = 309,
    TOKENDRECODE    = 310,
    TOKEXIT         = 312,
    TOKEND          = 313,
    TOKWHERE        = 314,
    TOKSTOP         = 316,
    TOKSKIP         = 317,
    TOKKWCASE       = 318,
    TOKTO           = 319,
    TOKNEXT         = 320,
    TOKPAGE         = 321,
    TOKENDSECT      = 322,
    TOKREENTER      = 323,
    TOKNOINPUT      = 324,
    TOKADVANCE      = 325,
    TOKENTER        = 326,
    TOKLEVEL        = 328,
    TOKENDLEVL      = 330,
    TOKTABLE        = 331,
    TOKMEAN         = 332,
    TOKSMEAN        = 333,
    TOKEXCLUDE      = 337,
    TOKINCLUDE      = 338,
    TOKBREAK        = 340,
    TOKKWFREQ       = 341,
    TOKTITLE        = 342,
    TOKSTUB         = 343,
    TOKINTERVAL     = 344,
    TOKHIGHEST      = 345,
    TOKLOWER        = 346,

    TOKBY           = 348,
    TOKALL          = 349,
    TOKWEIGHT       = 351,
    TOKADD          = 352,
    TOKUSERFUNCTION = 353,
    TOKMODIFY       = 354,
    TOKCROSSTAB     = 355,
    TOKSELECT       = 356,
    TOKFOR          = 359,
    TOKNOPRINT      = 360,
    TOKNOAUTO       = 361,
    TOKNOBREAK      = 362,
    TOKNOFREQ       = 363,
    TOKCELLTYPE     = 364,
    TOKLINKED       = 365,
    TOKARRAY        = 367,
    TOKEXPORT       = 368,
    TOKCASEID       = 370,
    TOKKWGROUP      = 371, // victor Aug 02, 99
    TOKMISSING      = 373,
    TOKDEFAULT      = 374,
    TOKNOTAPPL      = 375,
    TOKPREPRO       = 376,
    TOKPOSTPRO      = 377,
    TOKSET          = 378,

    TOKRECNAME      = 379,
    TOKRECTYPE      = 380,
    TOKMULTIPLE     = 383, // for SELCASE
    TOKSTABLE       = 390, // STABLE
    TOKENDFOR       = 392,
    TOKIN           = 393,
    TOKUNTIL        = 394,
    TOKVARYING      = 395,

    TOKNUMERIC      = 397,
    TOKALPHA        = 398,

    TOKHOTDECK      = 399, // RHF Nov 17, 2000
    TOKVERIFY       = 400, // RHF Dec 28, 2000
    TOKVALUESET     = 401, // RHF Jul 05, 2001
    TOKONFOCUS      = 402,
    TOKONOCCCHANGE  = 403,
    TOKKILLFOCUS    = 404,

    TOKRELATION     = 406,
    TOKKWRELATION   = 407,

    TOKKWITEM       = 410, // RHF Jun 18, 2002
    TOKUNIT         = 411, // RHF Jun 14, 2002
    TOKSTAT         = 412, // RHF Jun 14, 2002
    TOKROW          = 413,
    TOKCOLUMN       = 414,
    TOKLAYER        = 415,
    TOKTABLOGIC     = 416,
    TOKENDLOGIC     = 417,
    TOKKWVSET       = 418, // RHF Jul 03, 2002

    TOKUSING        = 419, // Chirag Sep 11, 2002
    TOKASCENDING    = 420, // Chirag Sep 11, 2002
    TOKDESCENDING   = 421, // Chirag Sep 11, 2002
    TOKSUBTABLE     = 422,

    TOKTALLY        = 423,
    TOKPOSTCALC     = 424,

    TOKMOVE         = 425, // RHF Dec 09, 2003    BUCEN__2003 Changes

    TOKFILE         = 426,

    TOKENDUNIT      = 429, // RHF Jul 14, 2005
    TOKSAVE         = 430, // JH 3/13/06

    TOKENDCASE      = 431, // 20100310
    TOKUNIVERSE     = 432, // 20100310
    TOKPERIOD       = 433, // 20110222
    TOKALIAS        = 434, // 20120307
    TOKHAS          = 435, // 20120429
    TOKSTRING       = 436, // 20140319
    TOKLIST         = 437, // 20141106
    TOKCONFIG       = 438, // 20161028
    TOKFORCASE      = 439, // 20161101
    TOKASK          = 440, // 20170321
    TOKSQL          = 441, // 20171019
    TOKBLOCK        = 442, // 20180420
    TOKNEWSYMBOL    = 443, // 20180517
    TOKKWLIST       = 444, // 20180522
    TOKKWARRAY      = 445, // 20180522
    TOKKWFUNCTION   = 446, // 20180611
    TOKENSURE       = 447, // 20180613
    TOKCONTINUE     = 448, // 20181102
    TOKKWMAP        = 449, // 20190220
    TOKMAP          = 450, // 20190220
    TOKKWVALUESET   = 451, // 20190304
    TOKTRUE         = 452, // 20190511
    TOKFALSE        = 453, // 20190511
    TOKKWPFF        = 454, // 20190626
    TOKPFF          = 455, // 20190626
    TOKWHEN         = 456, // 20191112
    TOKENDWHEN      = 457, // 20191112
    TOKKWSYSTEMAPP  = 458, // 20200318
    TOKSYSTEMAPP    = 459, // 20200318
    TOKREFUSED      = 460, // 20200403
    TOKKWAUDIO      = 461, // 20200714
    TOKAUDIO        = 462, // 20200714
    TOKREF          = 463, // 20200720
    TOKOPTIONAL     = 464, // 20200720
    TOKHASH         = 465, // 20200721
    TOKKWHASHMAP    = 466, // 20200730
    TOKHASHMAP      = 467, // 20200730
    TOKFREQ         = 468, // 20200924
    TOKWORKSTRING   = 469, // 20201022
    TOKRECORD       = 470, // 20210204
    TOKDICT         = 471, // 20210208
    TOKKWDATASOURCE = 472, // 20210211
    TOKKWIMAGE      = 473, // 20210219
    TOKIMAGE        = 474, // 20210219
    TOKKWDOCUMENT   = 475, // 20210221
    TOKDOCUMENT     = 476, // 20210221
    TOKKWGEOMETRY   = 477, // 20210302
    TOKGEOMETRY     = 478, // 20210302
    TOKFLOW         = 479, // 20210507
    TOKNAMEDARGOP   = 480, // 20210511
    TOKREPORT       = 481, // 20210610
    TOKPERSISTENT   = 482, // 20211229
    TOKITEM         = 483, // 20230613
};
