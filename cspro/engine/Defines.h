#pragma once

//  DEFINES.H       CSPRO basic engine header

// Bucen extensions
#define BUCEN

#include <zEngineO/AllSymbolDeclarations.h>
#include <zEngineO/RunnableSymbol.h>



//============== definiciones generales para todo el soporte =============
// general, basic characters

const TCHAR BLANK     = _T(' ');
#define NL              _T('\n')            // newline

const unsigned int MAXLINE = 255;

// CSPRO module types
enum class ModuleType { None = 0, Entry, Designer, Batch };

const int   _MAXLABLEN = 256;

const int FNSEL_VARS   = 11;            // # of vars to include in SELCASE

const int MAX_KEY_SIZE    = 256;               // max. key-size for any index


// miscellaneous codes
enum
{
    CL_SING = _T('S'),
    CL_MULT = _T('M'),
};

// SET' dynamic field-attributes
#define SET_AT_NATIVE       0
#define SET_AT_DISPLAY      1           // -> fmt      Z
#define SET_AT_AUTOSKIP     2           // -> fldpause 0
#define SET_AT_RETURN       3           // -> fldpause 1
#define SET_AT_PROTECT      4           // -> fldpause 9
#define SET_AT_HIDDEN       5           // -> verify  TRUE
#define SET_AT_VISIBLE      6           // -> verify  FALSE
#define SET_AT_REF_ON       7           // -> refresh       prot fields
#define SET_AT_REF_OFF      8           // -> don't refresh prot fields
#define SET_AT_CAP_ON       9
#define SET_AT_CAP_OFF     10
#define SET_AT_CAP_TOGGLE  11

const int DEPRECATED_CAPI_QUESTION_FLAG = 1;
const int DEPRECATED_CAPI_VARIABLE_FLAG = 2;
const int DEPRECATED_CAPI_TITLE_FLAG = 4;

// flags' deep-management: get/set "color" and "valid" marks
//
//      Remark: a flag is a csprochar, with bits distributed as follows:
//              bit 0   --
//              bit 1    |--- color
//              bit 2   --
//              bit 3   (reserved for future use)
//              bit 4   mark of "valid Float"
//              bit 5   mark of "valid Ascii"
//              bit 6   (reserved for future use)
//              bit 7   (reserved for future use)
//
// --- bits 0, 1 and 2: "color" value
#define FLAG_maskCOLOR  0x03            // 0000 0011    masking color part
#define FLAG_maskNOCOL  0xFC            // 1111 1100    masking no-color part
// ... specific color-values:
#define FLAG_NOLIGHT    0x00            // 0000 0000    never entered
#define FLAG_MIDLIGHT   0x01            // 0000 0001    grey or yellow
#define FLAG_HIGHLIGHT  0x02            // 0000 0010    green (highlight)

// record-type code maximum length
#define MAX_RECTYPECODE    9            // GSF Mar 21, 2002

#define ENG_BLANKSIZE   1024            // size of blank-area for comparisons


class CEngineDefines
{
public:
    // blank-area for comparisons                       // victor Jul 26, 00
    TCHAR m_pEngBlank[ENG_BLANKSIZE+2]; // blank-area for comparisons

    CEngineDefines()
    {
        _tmemset( m_pEngBlank, BLANK, ENG_BLANKSIZE );
        m_pEngBlank[ENG_BLANKSIZE+1] = 0;
    }
};

#define pEngBlank   m_pEngineDefines->m_pEngBlank   // blank-area for comparisons

#include <zToolsO/Special.h>

//////////////////////////////////////////////////////////////////////////
// CROSSTAB' special values masks

const int CTAB_MAX_DIGITS = 15;
const double CTAB_MAXVALINT  = (double) (1e15-1); // please, use CTAB_MAX_DIGITS value here

const int NUMBER_OF_SPECIAL_VALUES = 5;
const int NUMBER_OF_DATA_CELLS     = 10000;
const int MAXCELL = (NUMBER_OF_DATA_CELLS+NUMBER_OF_SPECIAL_VALUES);

const int CTMISSING   = NUMBER_OF_DATA_CELLS;     // used to be 10000
const int CTREFUSED   = NUMBER_OF_DATA_CELLS + 1;
const int CTDEFAULT   = NUMBER_OF_DATA_CELLS + 2; // used to be 10001
const int CTNOTAPPL   = NUMBER_OF_DATA_CELLS + 3; // used to be 10002
const int CTUNDEFINED = NUMBER_OF_DATA_CELLS + 4; // new
const int CTTOTAL     = NUMBER_OF_DATA_CELLS + 5; // used to be 10003

// VAL_CTMISSING forced to be 1 followed by CTAB_MAX_DIGITS [15] 0's
const double VAL_CTMISSING   = 1000000000000000;  // used to be 10000
const double VAL_CTREFUSED   = VAL_CTMISSING + 1;
const double VAL_CTDEFAULT   = VAL_CTMISSING + 2; // used to be 10001
const double VAL_CTNOTAPPL   = VAL_CTMISSING + 3; // used to be 10002
const double VAL_CTUNDEFINED = VAL_CTMISSING + 4; // new
const double VAL_CTTOTAL     = VAL_CTMISSING + 5; // used to be 10003

//////////////////////////////////////////////////////////////////////////

extern "C" {
#include <engine/Memctrl.h>
};
