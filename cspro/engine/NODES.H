#pragma once

//---------------------------------------------------------------------------
//  File name: Nodes.h
//
//  Description:
//          Header for engine' compiler nodes
//
//  History:    Date       Author   Comment
//              ---------------------------
//              15 Nov 99   RHF     Basic conversion
//              05 Oct 00   vc      Adding FNMSG_NODE
//              04 Apr 01   vc      brack_... suppressed
//              15 May 01   vc      Expansion for the 3D driver
//
//---------------------------------------------------------------------------

#include <engine/dimens.h>
#include <engine/Defines.h>
#include <zEngineO/Nodes/BaseNodes.h>
#include <zLogicO/FunctionTable.h>
#include <zLogicO/Symbol.h>


struct COMPUTE_NODE
{
    int st_code;
    int next_st;
    int cpt_var;
    int cpt_expr;
};


struct TBL_CPT_NODE
{
    int st_code;
    int next_st;
    int tbl_left;
    int tbl_expr;
}
;

struct LOUP
{
    int lo;
    int up;
};


struct TBL_NODE
{
    int tbl_type;
    int tbl_index;
    LOUP iexpr[3];
    LOUP range[3];
    int left_idx[3];
};


struct TBL_FUN_NODE
{
    int function;
    int vardim;
};


struct SVAR_NODE // covers WV, UF and single variables
{
    int m_iVarType;
    int m_iVarIndex;
};


#define MVAR_EXPR   'e'
#define MVAR_GROUP  'g'
#define MVAR_RECORD 'r'
constexpr TCHAR MVAR_CONSTANT         = 'k';
constexpr TCHAR MVAR_USE_DYNAMIC_CALC = 'd';
constexpr TCHAR MVAR_NOTHING          = '\0';


struct MVAR_NODE // covers multiple variables
{
    int m_iVarType = 0;   // should be MVAR_CODE
    int m_iVarIndex = 0;
    int m_iVarSubindexType[DIM_MAXDIM]; // subindex type: 'e', 'g'
    int m_iVarSubindexExpr[DIM_MAXDIM];
    int m_iSubindexNumber = 0;

    bool operator==(const MVAR_NODE& other) const
    {
        bool bEqual =
            this->m_iVarType == other.m_iVarType &&
            this->m_iVarIndex == other.m_iVarIndex &&
            this->m_iSubindexNumber == other.m_iSubindexNumber;
        for( int i = 0; bEqual && i < this->m_iSubindexNumber; i++ )
        {
            bEqual =
            this->m_iVarSubindexType[i] == other.m_iVarSubindexType[i] &&
            this->m_iVarSubindexExpr[i] == other.m_iVarSubindexExpr[i];
        }

        return bEqual;
    }
};


struct GRP_NODE
{
    int m_iGrpType;
    int m_iGrpIndex;
    int m_iGrpSubindexType[DIM_MAXDIM]; // subindex type: 'e', 'g'
    int m_iGrpSubindexExpr[DIM_MAXDIM];
    int m_iSubindexNumber;
};


struct REL_NODE
{
    int m_iRelType;
    int m_iRelIndex;
    int m_iRelSubindexType[DIM_MAXDIM]; // subindex type: 'e', 'g'
    int m_iRelSubindexExpr[DIM_MAXDIM];
    int m_iSubindexNumber;
};


struct TVAR_NODE
{
    int tvar_type;
    int tvar_index;
    int tvar_exprindex[3];
    int m_iSubindexNumber;
};


struct CTAB_NODE
{
    int st_code;
    int next_st;
    int SYMTctab;
    int selectexpr;                 // select expression
    int weightexpr;                 // weight expression
    int tablogicexpr;               // tablogic expression
};


struct EXPORT_NODE
{
    int st_code;
    int next_st;
    void* m_pExport;
};


struct BREAK_NODE
{
    int st_code;
    int next_st;
};


struct ST_NODE
{
    int st_code;
    int next_st;
};


struct STN_NODE
{
    int st_code;
    int next_st;
    int arguments[1];
};


struct BOX_ROW
{
    int row_expr;
    int next_row;
};


struct FNG_NODE
{
    int m_iFunCode;
    int symbol_index;               // Symbol of Dictionary, Section, Variable, etc.
    int _unused1;                   // formerly m_iIsAtAlpha
    int m_iOper;
    int _unused2;                   // formerly m_iNumeric
    int m_iExpr;                    // Alpha or logical expresion depending on m_iOper and item type (numeric or alpha)
};


namespace FNGR_Spec
{
    constexpr int ARG_NO_ARGUMENTS  =  0;
    constexpr int ARG_MULTIPLE_VAR  =  1;
    constexpr int ARG_GROUP         =  2;
    constexpr int ARG_SECTION       =  3;
    constexpr int ARG_WHERE_MV      =  4;       // for count/sum/etc.(... where ... = ... curocc()) statements
    constexpr int ARG_WHERE_GROUP   =  5;       // for count/sum/etc.(... where ... = ... curocc()) statements
    constexpr int ARG_NOT_SPECIFIED = 99;
}


struct FNGR_NODE
{
    int fn_code;
    int fn_arg;
    int m_iSym;                     // Symbol used for specify a Working variable or a Group in a foreach. 0 if not used
    int m_iArgumentType;
};


// expression for insert, delete, and swap operations
struct FNINS_NODE
{
    int fn_code;
    int group_node;
    int first_occurrence;
    int last_occurrence;
};


struct FNINVALUSET_NODE
{
    int fn_code;
    int m_iSymVar;
    int m_iExpr;
    int m_iSymVSet;
};


struct FNSRT_NODE
{
    int fn_code;
    int fn_grp;
    int fn_itm;
    int m_iSym;   // Symbol used for specify a Working variable or a Group in a foreach. 0 if not used
    int m_iWhere;
};


//////////////////////////////////////////////////////////////////////////
//
// COUNT node specification
//                                   . slight modifications and additions
//                                   . comments added

enum COUNT_type { COUNT_GROUP, COUNT_SECTION, COUNT_VAR, COUNT_BAD_DATA };

struct FN2_NODE
{
    int fn_code;
    int sect_ind;           // GROUP index
    COUNT_type count_type;  // what kind of COUNT is this?
    int fn_exp;             // WHERE expression code
};

//////////////////////////////////////////////////////////////////////////


struct FN3_NODE
{
    int fn_code;
    int sect_ind;
    int iSymVar;
    int fn_exp;
};


struct FN4_NODE
{
    int fn_code;
    int sect_ind;
};


struct FN6_NODE
{
    int fn_code;
    int tabl_ind;
    int iWeightExpr;
    int iSelectExpr;
    int iListNode;
};


struct FN8_NODE
{
    FunctionCode function_code;
    int symbol_index;
    int dictionary_access;
    int extra_parameter;   // { 0: Update, 1: Create, 2: Append } for open or the alpha expression for setcaselabel
    int starts_with_expression;
};


struct OLD_FNTC_NODE    // Node for TBLcoord function call
{
    int fn_code;        // Function: tblrow, tblcol, tbllay
    int ict;            // CrossTab index
    int iNewMode;       // Always 0
    int nterm;          // Term number
    int var1;           // CTTERM index for variable 1
    int expr1;          // Object code index for expression
    int var2;           // CTTERM index for variable 2
    int expr2;          // Object code index for expression
};


struct FNTC_NODE            // Node for TBLcoord function call
{
    int fn_code;            // Function: tblrow, tblcol, tbllay
    int iCtab;              // CrossTab index
    int iNewMode;           // Always 1
    int iSubTableNum;       // Subtable number, 1 based
    int iCatValues[2];      // 0 if not used. Logical expresion (used after = sign) for the items in the subtable.
    int iCatValuesSeq[2];   // 0 if not used. 1 based if used.
};


struct FNS_NODE // FNS: SELCASE
{
    int fn_code;
    int idict;
    int key_expr;
    int wind_offs;
    int dictionary_access;
    int include_vars[FNSEL_VARS];   // additional variables
    int fn_where_exp;               // WHERE expression
    int mark_type;                  // use multiple marks
    int heading;
};


struct FNC_NODE
{
    int fn_code;
    int isymb;
};


struct FNH_NODE  // nodo de funciones con variables
{
    int fn_code;
    int isymb;      // variable
    int occ_exp;    // ocurrencia
};


struct STOP_NODE
{
    int st_code;
    int next_st;
    int stop_expr;  // stop number - 0 if none
};


struct ASK_NODE
{
    int ask_code;
    int next_st;
    int ask_universe;
};


constexpr int SKIP_DEFAULT            = 0;
constexpr int SKIP_TO_NEXT_OCCURRENCE = -2;

struct SKIP_NODE
{
    int skip_code;
    int next_st;
    int var_ind;
    int var_exprind;
    int m_iSymAt;       // the target is referred thru this' symbol contents
    int m_iAlphaExpr;   // the target is referred to from an alpha expression
};


struct FORGROUP_NODE
{
    int for_code;
    int next_st;
    int forVarIdx;
    int iSuggestedDimension;
    int forWhereExpr;
    int forBlock;
    GRP_NODE forGrpNode;
};


struct FORRELATION_NODE
{
    int for_code;
    int next_st;
    int forVarIdx;
    int forWhereExpr;       // elect where expresion
    int forBlock;
    int forRelIdx;
    int iCtab;              // Only For CTab
    int CtabWeightExpr;     // Only For CTab
    int CtabTablogicExpr;   // Only for CTab

    int m_iForRelNodeIdx;   // 3d Extensions
    int isUsingExtraInfo;   // 3d Extensions
    int extraInfoNode;      // 3d Extensions
};


struct SETOTHER_NODE
{
    int st_code;
    int next_st;
    int type;               // SET HEADING, LINEPAGE, BEHAVIOR
    int len;
    union {
        int form_head;      // HEADING
        int linepag;        // LINEPAGE
        int behavior_item;
    } setinfo;
};


struct LIST_NODE
{
    int iNumElems;  // # of symbols
    int iSym[1];    // symbols
};


struct SET_ATTR_NODE    // for SET ATTRIBUTES
{
    int st_code;
    int next_st;
    short attr_type;    // NATIVE, DISPLAY, PROTECT, HIDDEN
    short _unused;      // previous m_iCapiPos
    int m_iCapiMode;
    int iListNode;
};


struct ACCESS_NODE
{
    int st_code;
    int next_st;
    int idic;
    int iidx;
    int ac;         // Ascending | Descending
};


// Better to specify all options
constexpr int EXECSYSTEM_MAXIMIZED       =   1;
constexpr int EXECSYSTEM_NORMAL          =   2;
constexpr int EXECSYSTEM_MINIMIZED       =   4;
constexpr int EXECSYSTEM_FOCUS           =   8;
constexpr int EXECSYSTEM_NOFOCUS         =  16;
constexpr int EXECSYSTEM_WAIT            =  32;
constexpr int EXECSYSTEM_NOWAIT          =  64;
constexpr int EXECSYSTEM_STOP            = 128;
constexpr int EXECSYSTEM_DEFAULT_OPTIONS = EXECSYSTEM_NORMAL | EXECSYSTEM_FOCUS | EXECSYSTEM_NOWAIT;

struct FNEXECSYSTEM_NODE
{
    int fn_code;
    int m_iCommand;     // alpha expression representing the full command (including parameters)
    int m_iOptions;     // Mask (MAXIMIZED|NORMAL|MINIMIZED) | (FOCUS,NOFOCUS) | (WAIT,NOWAIT)
};


struct FNSHOW_NODE
{
    int fn_code;
    int m_iForNode;
    int m_iShowListNode;    // Used when m_iForNode==0 --> single and multiple subindexed items allowed
    int m_iTitleList;
    int m_iHeading;
};


struct SHOWLIST_NODE
{
    int st_code;
    int next_st;  // -1
    int m_iVarListNode;
};


// for the itemlist function
enum class ItemListType : int { ALPHAFUNC, NUMERICFUNC, FUNCWITHVARIABLE, RECORD, RECORD_INDEXED,
                                ARRAY, MULTIPLYOCCURINGITEM, WORKINGVARIABLE };


struct FNNOTE_NODE
{
    FunctionCode function_code;
    int symbol_index;
    int variable_expression;
    int operator_id_expression;
    int note_text_expression;
};


struct DECK_ARRAY_NODE
{
    int fn_code;
    int array_symbol_index;
    int putdeck_value_expression;
    int update_spillover_rows;
    int index_expressions[1];
};
