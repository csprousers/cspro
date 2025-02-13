//---------------------------------------------------------------------------
//  File name: Compctab.cpp
//
//  Description:
//          compiler for CROSSTABs declaration
//
//  History:    Date       Author   Comment
//              ---------------------------
//              15 Jul 99   RHF     Basic conversion
//              20 Nov 00   RHF     Redo for Hotdeck support
//              03 Jul 01   RHF     Redo for Multi tally support
//              01 Apr 02   vc+RHC  Add basic support for Units
//              14 Jun 02   RHF     Full units support
//              21 Nov 04   rcl     Refactorization and 3D Handling
//              25 Apr 05   rcl     CTab extensions: 4 digit limit -> 8 digit limit
//                                  [See new CTAB_MAX_DIGITS const in defines.h]
//                                  "Exclude" modifier removed
//                                  Default flag for CTab is none.
//                 May 05   rcl     Negative values handling
//
//---------------------------------------------------------------------------


#include "StandardSystemIncludes.h"
#ifdef GENCODE
#include "Exappl.h"
#else
#include "Tables.h"
#endif
#include "COMPILAD.H"
#include "Ctab.h"
#include "Ctab_Helper.h" // rcl, Dec 2004
#include "Engine.h"
#include "RangeFunctions.h"
#include <zEngineO/ValueSet.h>
#include <zEngineO/Compiler/CompilationExtendedInformation.h>
#include <zEngineO/Compiler/TokenHelper.h>
#include <zToolsO/Range.h>
#include <zToolsO/Tools.h>
#include <zAppO/Application.h>
#include <zDictO/DDClass.h>
#include <zTableO/Table.h>


#ifdef _DEBUG
#undef THIS_FILE
static char THIS_FILE[]= __FILE__;
#define new DEBUG_NEW
#endif

//////////////////////////////////////////////////////////////////////////

static bool g_bLayerExpresion      = false;
static bool g_bRestrictedExpresion = false; // En los comandos STABLE NO se aceptan 2 dimensiones SIN multiplicacion
static bool g_bFreeExpresion       = true;  // Allow (A+B+C)*(C+D+E)
static bool g_bMetaTable           = true;  // use false when expresion for sub-tables are exact

// const to clarify source code, rcl Apr 2005
const int LEFT_PARENT_EXPECTED  = 517;
const int INVALID_COMMENT_ERROR = 118;  // 'Invalid or not ended comments - see { }
const int REPETITION_ERROR      = 605; // CSPRO.MGF 'Repeated Crosstab parameter'

// the flags to be checked, and the values to be assigned to the ranges
// These arrays are used in ctcomplete() and ctrange()
static int aTheFlags[]    = { ct_MISSING, ct_REFUSED, ct_DEFAULT, ct_NOTAPPL, ct_UNDEFINED };
static double aTheCTValues[] = { VAL_CTMISSING, VAL_CTREFUSED, VAL_CTDEFAULT, VAL_CTNOTAPPL, VAL_CTUNDEFINED };

static csprochar* aTheFlagNames[] = { _T("MISSING"), _T("REFUSED"), _T("DEFAULT"), _T("NOTAPPL"), _T("UNDEFINED") };

// specialString2Index()
// given a string [hopefully one of special value names] will return
// the index suitable to be used in the other arrays, defined above
static int specialString2Index(const std::wstring& possibleSpecialValue)
{
    int iPossibleIndex = -1;
    const TCHAR* p = possibleSpecialValue.c_str();
    for( int i = 0; i < _countof(aTheFlagNames); i++ )
    {
        if( *p == *aTheFlagNames[i] )  // compare just 1st character to make comparison faster
        {
            if( _tcscmp( p, aTheFlagNames[i] ) == 0 )
            {
                iPossibleIndex = i;
                break;
            }
        }
    }

    return iPossibleIndex;
}

static int specialValue2Index( double dValue )
{
    int iPossibleIndex = -1;
    for( int i = 0; i < _countof(aTheCTValues); i++ )
    {
        if( areEqual( dValue, aTheCTValues[i] ) )
        {
            iPossibleIndex = i;
            break;
        }
    }

    return iPossibleIndex;
}


static
bool isSpecialValue( double rValue )
{
    return ( areEqual(rValue,VAL_CTMISSING) ||
             areEqual(rValue,VAL_CTREFUSED) ||
             areEqual(rValue,VAL_CTNOTAPPL) ||
             areEqual(rValue,VAL_CTDEFAULT) ||
             areEqual(rValue,VAL_CTUNDEFINED) );
}

CMap<int,int,CString,CString> CTAB::m_aExtraNodeInfo;


/////////////////////////////////////////////////////////////////////////////
//
//                             DoIssaRanges  SERPRO
//
/////////////////////////////////////////////////////////////////////////////
// FUNCTIONALITY:
// Construct the Issa Ranges
//
// WARNINGS: Used only in numeric variables
//           Work fine if the values of the value-set are in ascending order
//
// PARAMETERS:
// aRanges, an array where the ranges will be returned.
// iValueSet, 0 based indicating the value-set
//
// RETURNS: -1 if error, 0 to n indicating the number of ranges generated
//          Aditionally, the ranges will be returned in aRanges.
// CHANGES: RHF, Aug 25 1999
// CHANGES: RHF, Nov 03, 2000 Add overlapping check
// CHANGES: RHF, Nov 03, 2000 Add value-set parameter for giving the ranges list from a value set
// CHANGES: RHF, Nov 15, 2000 Add pLabels for getting the labels of the range list.
// CHANGES: RHF, Jul 6, 2001 Add pCollapsed for keeping additional information about the range
/////////////////////////////////////////////////////////////////////////////
// DoIssaRanges        Do Ranges in double buffers from the first value-set           // SERPRO
// RHF Nov 15, 2000 Add paLabels
// RHF Jul 06, 2001 Add pCollapsed
// SAVY 4 Tabs / freq special values
int DoIssaRanges(const CDictItem* pItem, std::vector<Range<double>>& aRanges, const DictValueSet* pVSet,
                 CArray<CString,CString>* paLabels, std::vector<int>* pCollapsed ,bool bProcessSpecial /*=false*/ )
{
    // RHF INIC Nov 03, 2000
    if( pItem->GetContentType() != ContentType::Numeric )   // Only used in numeric items
        return 0;
    // RHF END Nov 03, 2000

    double      dLow, dHigh, dLastHigh=-1;
    int         iRange;
    int         iCollapsed=1;
    bool        bFirst, bError=false;

    //SAVY for Tabs/Freqs special values
    int iVSet = -1;
    if( bProcessSpecial ) {
        for(size_t iIndex = 0; iIndex < pItem->GetNumValueSets(); iIndex++){
            if(pVSet == &pItem->GetValueSet(iIndex)){
                iVSet = (int)iIndex;
                break;
            }
        }
    }

    iRange = -1;
    bFirst = true;

    for( size_t i=0; !bError && i < pVSet->GetNumValues(); i++ ) {
        const DictValue& dict_value = pVSet->GetValue(i);

        if( !bProcessSpecial && dict_value.IsSpecial() )
            continue; // RHF Jun 18, 2001

        iCollapsed = 1; // RHF Jun 16, 2005

        //SAVY for Tabs /Freqs special
        //if( pValue->IsSpecial() ) continue; // RHF Jun 18, 2001
           // special values for tabs:
        bool bProcess  = false;
        if(iVSet != -1 && bProcessSpecial) {
            if( dict_value.IsSpecial() ) {
                if( dict_value.IsSpecialValue(MISSING) ) {
                    iRange++;
                    aRanges.emplace_back( VAL_CTMISSING, VAL_CTMISSING );
                    bProcess = true;
                }
                else if( dict_value.IsSpecialValue(REFUSED) ) {
                    iRange++;
                    aRanges.emplace_back( VAL_CTREFUSED, VAL_CTREFUSED );
                    bProcess = true;
                }
                else if( dict_value.IsSpecialValue(DEFAULT) ) {
                    iRange++;
                    aRanges.emplace_back( VAL_CTDEFAULT, VAL_CTDEFAULT );
                    bProcess = true;
                }
                else if( dict_value.IsSpecialValue(NOTAPPL) ) {
                    iRange++;
                    aRanges.emplace_back( VAL_CTNOTAPPL, VAL_CTNOTAPPL );
                    bProcess = true;
                }
                if(bProcess){
                    if( paLabels != NULL )
                        paLabels->Add(dict_value.GetLabel());
                    iCollapsed = 1;
                    if( pCollapsed != NULL ) {
                        pCollapsed->emplace_back(iCollapsed);
                        iCollapsed++;
                    }
                }
                continue; // RHF Jun 18, 2001
            }
        }
        for( size_t j = 0; !bError && j < dict_value.GetNumValuePairs(); j++ ) {
            const DictValuePair& dict_value_pair = dict_value.GetValuePair(j);

            // RHF INIC Jul 06, 2001
            // TRICK: Reset collapse only when a label appear!! ELIMINATE WHEN MORE THAN 1 PAIR WILL BE SUPPORTED
            // How specify in the value set:   1,3:5,7,8
            //
//            ASSERT( j == 0 ); // ONLY 1 pair????  NO! can be more than one pair!!
// RHF COM Jun 16, 2005            if( pCollapsed != NULL && pValue->GetLabel().GetLength() > 0 )
// RHF COM Jun 16, 2005                iCollapsed = 1;
            // RHF END Jul 06, 2001

            // Skip alpha values
            if( !CIMSAString::IsNumeric(dict_value_pair.GetFrom()) )
                continue;

            dLow = atod(dict_value_pair.GetFrom());
            if( dict_value_pair.GetTo().GetLength() == 0 )
                dHigh = dLow;
            else {
                if( !CIMSAString::IsNumeric(dict_value_pair.GetTo()) ) // Skip alpha values
                    continue;
                dHigh = atod(dict_value_pair.GetTo());
            }
            if( dHigh < dLow ) {  // Error
                bError = true;
                break;
            }

            if( bFirst ) {
                iRange++;
                aRanges.emplace_back( dLow, dHigh );
                if( paLabels != NULL )
                    paLabels->Add(dict_value.GetLabel()); // RHF Nov 15, 2000

                if( pCollapsed != NULL ) {
                    pCollapsed->emplace_back(iCollapsed);
                    iCollapsed++;
                }

                bFirst = false;
            }
            else {
                // New range
                iRange++;
                aRanges.emplace_back( dLow, dHigh );

                if( paLabels != NULL )
                    paLabels->Add(dict_value.GetLabel()); // RHF Nov 15, 2000

                if( pCollapsed != NULL ) {
                    pCollapsed->emplace_back(iCollapsed);
                    iCollapsed++;
                }
            }

            dLastHigh = dHigh;
        }
    }

    if( bError )
        return -1;

    return (iRange + 1);
}

//----------------------------------------------------
//  CTAB compiler
//
//      crosstab  ct-name   row, col, layer
//
//        CROSSTAB parameters (any order):
//        select  select-expr
//        noauto
//        nobreak
//        noprint
//        nofreq
//        include
//               (specval,missing,default,notappl,
//                rowzero,colzero,layzero,
//     only for old TBD:
//                totals,rowtot,coltot,laytot,
//                percents,rowpct,colpct,laypct,
//                totpct
//
//               )
//        weighted by weight-expr
//        cell( width )  sint | lint | float(ndec)
//        title( "...", "..." ... )
//        stub( "...", "..." ... )
//       ;
//----------------------------------------------------
/*--------------------------------------------------------------------------*
- Se excluyen specval, total y percents.
- Se aceptan dos dimensiones.
- No se aceptan multiplicaciones. Si se aceptan sumas
- La tabla que se genera en el TBD es de tipo 4, y NUNCA la imprime EXPRTAB
aunque se le cambie el atributo de impresion a Y con TBD_EDIT.
- Aceptan post-procesamiento.
- Son afectados por BREAK.
- No se aceptan: TITLE, STUB.
--------------------------------------------------------------------------*/

//-----------------------------------------------------------------------------
//      HOTDECK  hotdeck-name   dep_var  row [ by col [by layer] ]
//
//        HOTDECK parameters (any order):
//        cell( width )  sint | lint | float(ndec)
//       ;
//-----------------------------------------------------------------------------

// highest level of all vars in Crosstab expresions
static  int     CtVarHiLvl;

// g_iCtabmode values: 0: declared in APP proc;
//                     1: declared & executed in DICT proc;
//                     2: executed in DICT proc
// use  compctab( 0 ) in cdeclare, compctab( 1 ) in instruc
static int g_iCtabmode;

const int MODE_APP_DECL  = 0;
const int MODE_DICT_DECL = 1;
const int MODE_DICT_EXEC = 2;

// modeIsDeclaration() replaces some changes made and marked before with RHF May 13, 2003
//                     It returns true when g_iCtabmode is MODE_APP_DECL or MODE_DICT_DECL
// rcl, Apr 2005
bool modeIsDeclaration() { return g_iCtabmode == MODE_APP_DECL || g_iCtabmode == MODE_DICT_DECL; }
bool modeIsDictRelated() { return g_iCtabmode == MODE_DICT_DECL || g_iCtabmode == MODE_DICT_EXEC; }

static  int     CtAux1mxent[3];
static  int     CtAux1Next[3];
static  int*    CtAux1base[3];

// optimized array
static  int     CtAux2mxent;
static  int     CtAux2Next;
static  int*    CtAux2base;

CTAB*   CTAB::pCurrentCtab = NULL;

int CEngineCompFunc::compctab( int mode, CTableDef::ETableType eTableType ) { // compile Crosstab verb
    //      mode       search "g_iCtabmode values"
    //      eTableType   1: table; 2: mean; 3:smean, 4:stable, 5:hotdeck
    g_iCtabmode = mode;
    CtVarHiLvl = 0;

    clearSyntaxErrorStatus();
    if( Appl.ApplicationType != ModuleType::Batch )
        return( SetSyntErr(601), 0 );     // accepted in BATCH only

    // RHF INIC Jul 02, 2005
    if( GetIdChanger() ) {
        return( SetSyntErr(1046), 0 );
    }
    // RHF END Jul 02, 2005

    // RHF May 06, 2003 Add ObjInComp != SymbolType::Crosstab
    if( ( ObjInComp != SymbolType::Crosstab && ObjInComp != SymbolType::Group && ObjInComp != SymbolType::Application ) ||
        ( ObjInComp != SymbolType::Crosstab && LvlInComp < 1) ||  // RHF NEW May 13, 2003 Add ObjInComp != SymbolType::Crosstab
        ( ObjInComp == SymbolType::Group && GPT(InCompIdx)->GetGroupType() != GROUPT::eGroupType::Level ) || // RHF Mar 19, 2001
        ( ObjInComp == SymbolType::Group && GPT(InCompIdx)->GetFlow() != m_pEngineDriver->GetPrimaryFlow() ) ) //RHF NEW Oct 24, 2000
    {
        return( SetSyntErr(602), 0 );     // cannot appear here
    }

    ctauxalloc();

    comp_ctab( eTableType );

    ctauxfree();

    CTAB::pCurrentCtab = NULL;

    m_pEngineSettings->m_bHasCrosstab  = true;                  // mark presence of Crosstab/Table sentence  RHF Jul 02, 2005

    return 0;
}


// SetCrosstabDefault
//     Sets ctab's m_iOptions value
//
// rcl, Apr 2005
static
void SetCrosstabDefault( CTAB* ct, CTableDef::ETableType eTableType, int tkn_object ) // tkn_object = keyword being compiled
{
    ct->m_uEnviron[1] = 0;                 // cell width: automatic
    ct->SetApplDeclared( true );           // declared in app (initial value)

    ct->m_uEnviron[0] = 0;       // rcl, Apr 2005
    ct->SetTableType( eTableType );        // 1: CrossTab; 2 Mean; 3: smean, 4: stable, 5: hotdeck

    // RHF INIT Jul 08, 2005
    if( eTableType == CTableDef::Ctab_Mean ) {                // Mean
        ct->m_uEnviron[0] |= ( ct_BREAK + ct_PRINT + ct_FREQ );
    }
    else if( eTableType == CTableDef::Ctab_SMean ) {           // SMean
        ct->m_uEnviron[0] |= ( ct_BREAK + ct_FREQ );
    }
    else if( g_iCtabmode == MODE_APP_DECL ) {           // Global
        ct->m_uEnviron[0] |= ( ct_BREAK + ct_PRINT + ct_FREQ );
    }
    else if( g_iCtabmode == MODE_DICT_DECL ) {          // CrossTab in Dict' proc
        ct->SetApplDeclared( false );             // declared in Dict' proc
        ct->m_uEnviron[0] |= ( ct_BREAK + ct_AUTOTALLY + ct_PRINT + ct_FREQ );
    }
    // RHF END Jul 08, 2005
}

int CEngineCompFunc::comp_ctab( CTableDef::ETableType eTableType ) {

    bool bHasDimension = false, bHasSelect  = false, bHasNoauto = false, bHasNobreak  = false,
         bHasBreak     = false, bHasNoprint = false, bHasNofreq = false, bHasInclude  = false,
         bHasWeighted  = false, bHasTitle  = false,  bHasStub   = false, bHasStat     = false,
         bHasUnit      = false, bHasTablogic = false;
//RHF INIT Nov 14, 2006
    // if bHasExclude starts in true, then any EXCLUDE will be flagged as a repetition
    // if bHasExclude starts in false, then only the 2nd one will be flagged as a repetition
    bool bHasExclude   = false;
//RHF END Nov 14, 2006
    bool    bStopScan;
    int     decl_level, xtab_level;
    int     ict, ctres[3];
    int*    octaux1base;
    int     auxndim = 0, iAcumType = 0, iNumDec = 0, i, j;
    bool    bGetNextToken;

    m_aCtCoordRepeatSymbol.clear(); // RHF Mar 18, 2004

    int     iDefinedLevel=-1;

    // Hotdeck only declared in application PROC
    if( eTableType == CTableDef::Ctab_Hotdeck && g_iCtabmode != MODE_APP_DECL ) {
        SetSyntErr(617);
        return 0;
    }

    int tkn_object = Tkn;           // keyword being compiled
    int iptctab    = Prognext;

#ifndef GENCODE // RHF Jul 16, 2004
    if( modeIsDeclaration() && Prognext == 0 )
        OC_CreateCompilationSpace(1); // RHF Jul 04, 2004 Avoid instruc return 0 for select,weight & tablogic
#endif // RHF Jul 16, 2004

#ifdef GENCODE
    CTAB_NODE* ctabpt = (CTAB_NODE*) (PPT(Prognext));
    if( Flagcomp && g_iCtabmode == MODE_DICT_DECL ) {
        OC_CreateCompilationSpace(sizeof(CTAB_NODE) / sizeof(int));

        ctabpt->st_code  = CTAB_CODE;
        ctabpt->next_st  = -1;
        ctabpt->SYMTctab = -1;

        // RHF COM Jul 04, 2002ctabpt->selectexpr = -1;
        // RHF COM Jul 04, 2002ctabpt->weightexpr = -1;

        ctabpt->selectexpr = 0;
        ctabpt->weightexpr = 0;
        ctabpt->tablogicexpr = 0;
    }
#endif

    // RHF INIC Jun 05, 2000
    size_t size_type = NextKeyword({ _T("SINT"), _T("LINT"), _T("FLOAT") });

    if( size_type != 0 &&
        ( g_iCtabmode != MODE_APP_DECL || (eTableType != CTableDef::Ctab_Crosstab && eTableType != CTableDef::Ctab_STable && eTableType != CTableDef::Ctab_Hotdeck) ) )
    {
        // error - cell type via SINT/LINT/FLOAT invalid
        IssueError( ( eTableType == CTableDef::Ctab_Mean )  ? 613 :
                    ( eTableType == CTableDef::Ctab_SMean ) ? 614 : 612 );
    }
    // RHF END Jun 05, 2000

    // CrossTab name
    NextTokenOrNewSymbolName();

    if( eTableType == CTableDef::Ctab_Mean )                 // MEAN
        iAcumType = sizeof(MEANACUM);
    else if( eTableType == CTableDef::Ctab_SMean )           // SMEAN
        iAcumType = sizeof(SMEANACUM);
    else if( size_type == 1 ) // SINT RHF Jun 05, 2000
        iAcumType = sizeof(int);
    else if( size_type == 2 ) // LINT RHF Jun 05, 2000
        iAcumType = sizeof(long);
    else if( size_type == 3 ) // FLOAT RHF Jun 05, 2000
    {
        iAcumType = sizeof(double);
        if( Tkn == TOKLPAREN )
        {
            NextToken();
            if( Tkn != TOKCTE || ( iNumDec = (int) Tokvalue ) != Tokvalue )
                IssueError(82);           // integer expected
            else if( iNumDec < 0 || iNumDec > 10 )
                IssueError(90);           // invalid # of decimals

            NextToken();
            if( Tkn != TOKRPAREN )
                SetSyntErr(24);
            else
                NextTokenOrNewSymbolName();
        }
    }

    if( GetSyntErr() != 0 )
        return 0;

    if( Tkn == TOKERROR )
        IssueError(603); // no more place

    bool bIsNewCtab = false;

    if( Tkn == TOKNEWSYMBOL )
    {
        ict = m_engineData->AddSymbol(std::make_unique<CTAB>(Tokstr));
        bIsNewCtab = true;
    }

    else if( Tkn == TOKCROSSTAB )
        ict = Tokstindex;

    else
        IssueError(92); // RHF Jun 16, 2000

    CTAB* ct = XPT(ict);                      // use in g_iCtabmode MODE_APP_DECL or MODE_DICT_DECL

    // RHF INIC Oct 31, 2002
    if( !bIsNewCtab && ct->GetPreDeclared() ) {
        bIsNewCtab = true;
    }
    ct->SetPreDeclared( false );
    // RHF END Oct 31, 2002

    if( !bIsNewCtab && g_iCtabmode == MODE_APP_DECL )
        return( SetSyntErr(103), 0 );     // re-declaration

    if( !bIsNewCtab && g_iCtabmode == MODE_DICT_DECL )
        g_iCtabmode = MODE_DICT_EXEC;

    ct->SetEngineDriver( m_pEngineDriver ); // RHF Jun 20, 2002

    CTAB::pCurrentCtab = ct;// RHF Jul 31, 2001

    if( !CSettings::m_bNewTbd && CTAB::pCurrentCtab->GetName().length() > LNAME8 )
        return( SetSyntErr(5534), 0 );  //Invalid table // RHF Dec 19, 2001

    // RHF INIC Jul 04, 2002
#ifdef GENCODE
    if( Flagcomp && modeIsDictRelated() ) { // Inheritance of TABLOGIC of declared CTAB
        ctabpt->tablogicexpr = ct->GetTabLogicExpr();
        ctabpt->selectexpr = ct->GetSelectExpr();
        ctabpt->weightexpr = ct->GetWeightExpr();
    }
#endif
    // RHF END Jul 04, 2002

    if( iAcumType > 0 ) {                 // there is type
        ct->SetAcumType( iAcumType );
        ct->SetNumDec( iNumDec );
    }

#ifdef GENCODE
    if( Flagcomp && modeIsDictRelated() )
        ctabpt->SYMTctab = ict;
#endif

    SetCrosstabDefault( ct, eTableType, tkn_object ); // Refactor, rcl Apr 2005

    m_bcvarsubcheck = true;
    m_icGrpIdx = 0;

    NextToken();

    // dependent var for MEAN ('v1') or SMEAN ('v1' or 'v1 / v2' ) or HOTDECK ( 'v1' )
    if( eTableType != CTableDef::Ctab_Crosstab && eTableType != CTableDef::Ctab_STable ) {
        ctexpdep( ct );
        if( GetSyntErr() != 0 )
            return 0;
    }

    // RHF INIC 26/9/96
    if( tkn_object == TOKSTABLE )
        g_bRestrictedExpresion = true;
    else
        g_bRestrictedExpresion = false;
    // RHF END 26/9/96

    for( i = 0; i < TBD_MAXDIM; i++ ) ctres[i] = -1; // RHF Jun 21, 2002

    // ROW expresion
    if( Tkn == TOKVAR || Tkn == TOKVALUESET ||  // RHF Jul 05, 2001 Add Value-set
        (Tkn == TOKLPAREN && g_bFreeExpresion) ) { // RHF Jul 25, 2001

        // Don't allow '('. No matter if only 1 term on the left side is used.
        if( Tkn == TOKLPAREN ) CTAB::pCurrentCtab->SetRunTimeVersion( 3 );// RHF Jul 31, 2001

        // CoorNumber doesn't depend on dimension m_aCtCoordRepeatSymbol.clear();
        ctres[0] = ctexp1( DIM_ROW, CtAux1base[0], CtAux1mxent[0], &CtAux1Next[0] );
        if( GetSyntErr() != 0 )
            return 0;
        auxndim++;

        if( Tkn == TOKCOMMA || Tkn == TOKBY ) NextToken();

        if( Tkn == TOKVAR || Tkn == TOKVALUESET || // RHF Jul 05, 2001 Add Value-set
            (Tkn == TOKLPAREN && g_bFreeExpresion) ) { // RHF Jul 25, 2001

            // Don't allow '('. No matter if only 1 term on the left side is used.
            if( Tkn == TOKLPAREN ) CTAB::pCurrentCtab->SetRunTimeVersion( 3 );// RHF Jul 31, 2001

            // CoorNumber doesn't depend on dimension m_aCtCoordRepeatSymbol.clear();
            // COLUMN expresion
            ctres[1] = ctexp1( DIM_COL, CtAux1base[1], CtAux1mxent[1], &CtAux1Next[1] );
            if( GetSyntErr() != 0 )
                return 0;
            auxndim++;

            if( Tkn == TOKCOMMA || Tkn == TOKBY ) NextToken();

            if( Tkn == TOKVAR || Tkn == TOKVALUESET || // RHF Jul 05, 2001 Add Value-set
                (Tkn == TOKLPAREN && g_bFreeExpresion) ) { // RHF Jul 25, 2001

                    // Don't allow '('. No matter if only 1 term on the left side is used.
                    if( Tkn == TOKLPAREN ) CTAB::pCurrentCtab->SetRunTimeVersion( 3 );// RHF Jul 31, 2001

                    if( eTableType == CTableDef::Ctab_Crosstab || eTableType == CTableDef::Ctab_Hotdeck ) {
                        // LAYER expresion
                        // RHF INIC 5/9/94
                        g_bLayerExpresion = true;

                        // CoorNumber doesn't depend on dimension m_aCtCoordRepeatSymbol.clear();
                        ctres[2] = ctexp1( DIM_LAYER, CtAux1base[2], CtAux1mxent[2], &CtAux1Next[2] );
                        // RHF END 5/9/94
                        g_bLayerExpresion = false;

                        if( GetSyntErr() != 0 )
                            return 0;
                        auxndim++;
                    }
                    else
                        return( SetSyntErr(647), 0 ); // MEAN-SMEAN-STABLE: invalid Layer
                }
            }
        }

    bHasDimension = ( auxndim > 0 );
    if( !bHasDimension && modeIsDeclaration() )
        return( SetSyntErr(606), 0 );     // invalid dimension spec

    if( bHasDimension && ( g_iCtabmode == MODE_DICT_EXEC ) )
        return( SetSyntErr(648), 0 );     // expresion not allowed

    if( modeIsDeclaration() )
        ct->SetNumDim( auxndim );

    bool bSubTablesAdded=true; // g_iCtabmode == MODE_DICT_EXEC;

    /* RHF COM INIC Jul 31, 2002
    if( modeIsDeclaration() )
        ct->MakeSubTables( CtAux1base, ctres ); // RHF Jun 21, 2002
        RHF COM END Jul 31, 2002 */
    // RHF INIC Jul 31, 2002
    if( modeIsDeclaration() )
        bSubTablesAdded = false;
    // RHF END Jul 31, 2002

    //////////////////////////////////////////////////////////////////////////
    // Definitions to increase code documentation, and to ease
    // future modifications, if any.
    // CHECK_REPETITION macro evals flag, it flag is true, something is wrong
    // rcl, Apr 2005
    #define CHECK_REPETITION(bFlag) \
    do{ if(bFlag) { SetSyntErr(REPETITION_ERROR); break; } bFlag = true; } while(0)

    // aFlags: array to be used in include and exclude command
    int aFlags[] =
    {
        0,
        /*  1 SPECVAL   */ ct_ALL_SPECIAL_VALUES, // rcl, Apr 2005
        /*  2 MISSING   */ ct_MISSING,
        /*  3 DEFAULT   */ ct_DEFAULT,
        /*  4 NOTAPPL   */ ct_NOTAPPL,
        /*  5 ROWZERO   */ ct_ROWZERO,
        /*  6 COLZERO   */ ct_COLZERO,
        /*  7 LAYZERO   */ ct_LAYZERO,
        /*  8 REFUSED   */ ct_REFUSED,
        /*  9 UNDEFINED */ ct_UNDEFINED, // rcl, Apr 2005
        /* 10 TOTALS    */ (ct_ROWTOT + ct_COLTOT + ct_LAYTOT),
        /* 11 ROWTOT    */ ct_ROWTOT,
        /* 12 COLTOT    */ ct_COLTOT,
        /* 13 LAYTOT    */ ct_LAYTOT,
        /* 14 PERCENTS  */ ( ct_ROWPCT + ct_COLPCT + ct_LAYPCT + ct_TOTPCT ),
        /* 15 ROWPCT    */ ct_ROWPCT,
        /* 16 COLPCT    */ ct_COLPCT,
        /* 17 LAYPCT    */ ct_LAYPCT,
        /* 18 TOTPCT    */ ct_TOTPCT,
        /* 19 CHISQUARE */ ct_CHISQUARE
    };

    //////////////////////////////////////////////////////////////////////////

    // Other parameters
    bStopScan = false;                  // allow recognition of next TABLE
    while( GetSyntErr() == 0 && !bStopScan ) {
        if( Tkn == TOKSEMICOLON || Tkn == TOKEOP )
            break;
        if( Tkn == TOKCOMMA ) {
            NextToken();
            continue;
        }
        bGetNextToken = true;

        // RHF INIT Nov 14, 2006
        if( CSettings::m_bNewTbd && Tkn == TOKEXCLUDE ) {
            Tkn = TOKNOFREQ; //Make an invalid token
        }
        // RHF END Nov 14, 2006

        switch( Tkn ){
        case TOKERROR:
            SetSyntErr(INVALID_COMMENT_ERROR);
            break;
        case TOKSELECT:
            // Table SELECT must appear before UNIT
            if( bHasUnit ) {
                SetSyntErr(8512);
                break;
            }

            /* RHF COM INIC Jul 04, 2002
            if( g_iCtabmode == MODE_APP_DECL ) {
                SetSyntErr(644);      // SELECT not allowed
                break;
            }
            RHF COM END Jul 04, 2002 */

            CHECK_REPETITION(bHasSelect);

            NextToken();

            i = exprlog();

            /* RHF COM INIC Jul 04, 2002
#ifdef GENCODE
            if( Flagcomp && modeIsDictRelated() )
                ctabpt->selectexpr = i;
#endif
            RHF COM END Jul 04, 2002 */

            // RHF INIC Jul 04, 2002
#ifdef GENCODE
            if( i < 0 ) // Empty
                i = 0;

            if( Flagcomp ) {
                if( modeIsDictRelated() )
                    ctabpt->selectexpr = i;
                else
                    CTAB::pCurrentCtab->SetSelectExpr(-i); // See inheritance
            }
#endif
            // RHF END Jul 04, 2002

            bGetNextToken = false;
            break;
        case TOKNOBREAK:
            if( g_iCtabmode == MODE_DICT_EXEC )
            {
                SetSyntErr(604);      // invalid parameter
                break;
            }
            if( g_iCtabmode == MODE_DICT_DECL )
                CHECK_REPETITION(bHasNobreak);
            // RHF INIC Apr 17, 2003
            if( bHasBreak ) {
                SetSyntErr(620);
                break;
            }
            // RHF END Apr 17, 2003

            NextToken();
            bGetNextToken = false;
            bHasNobreak = true;
            break;

// RHF INIC Apr 16, 2003
        case TOKBREAK:
            if( g_iCtabmode == MODE_DICT_EXEC )
            {
                SetSyntErr(604);      // invalid parameter
                break;
            }
            if( g_iCtabmode == MODE_DICT_DECL )
                CHECK_REPETITION(bHasBreak);

            if( bHasNobreak ) {
                SetSyntErr(620);
                break;
            }

            {
                NextToken();
                std::vector<CBreakById> aBreakId = CompileBreakByList();
                bGetNextToken = false;

                if( aBreakId.empty() ) {
                    SetSyntErr(593);
                    break;
                }

                // Check against others crosstabs. Prefix must be the same
                int     iNumCheckVars = (int)std::min(aBreakId.size(), m_pEngineArea->m_aCtabBreakId.size() );
                for( j=0; j < iNumCheckVars; j++ ) {
                    CBreakById& rThisBreakId=aBreakId[j];
                    CBreakById& rCtabBreakId=m_pEngineArea->m_aCtabBreakId[j];

                    if( rCtabBreakId.m_iSymVar != rThisBreakId.m_iSymVar ||
                        rCtabBreakId.m_iLen != rThisBreakId.m_iLen ) {
                        SetSyntErr(592);
                        break;
                    }
                }

                if( GetSyntErr() != 0 )
                    break;

                // Check against break by command
                if( Breaknvars > 0 ) {
                    if( (int)aBreakId.size() > Breaknvars ) {
                        SetSyntErr(595);
                        break;
                    }

                    for( j = 0; j < (int)aBreakId.size(); j++ ) {
                        CBreakById& rThisBreakId=aBreakId[j];

                        if( Breakvars[j] != rThisBreakId.m_iSymVar ||
                            Breaklvar[j] != rThisBreakId.m_iLen ) {
                            SetSyntErr(596);
                            break;
                        }
                    }
                }

                if( GetSyntErr() != 0 )
                    break;

                // When everything is OK, refresh master copy
                                /* RHF COM INIT Aug 11, 2004
                m_pEngineArea->m_aCtabBreakId.clear();
                for( i=0; i < aBreakId.GetSize(); i++ ) {
                    CBreakById&     rThisBreakId=aBreakId.ElementAt(i);
                    m_pEngineArea->m_aCtabBreakId.emplace_back( rThisBreakId );
                }
                                RHF COM END Aug 11, 2004 */

                                // RHF INIT Aug 11, 2004
                int iLastBreakVar = (int)m_pEngineArea->m_aCtabBreakId.size();
                for( j=iLastBreakVar; j < (int)aBreakId.size(); j++ ) {
                    CBreakById& rThisBreakId=aBreakId[j];
                    m_pEngineArea->m_aCtabBreakId.emplace_back( rThisBreakId );
                }
                                //RHF END Aug 11, 2004

                CTAB::pCurrentCtab->SetNumBreaks( aBreakId.size() );
            }
            bHasBreak = true;
            break;
// RHF END Apr 16, 2003

        case TOKNOPRINT:
            if( g_iCtabmode == MODE_DICT_EXEC )
            {
                SetSyntErr(604);      // invalid parameter
                break;
            }
            CHECK_REPETITION(bHasNoprint);
            NextToken();
            bGetNextToken = false;
            bHasNoprint = true;
            break;
        case TOKINCLUDE:
            if( eTableType == CTableDef::Ctab_Hotdeck ) {
                SetSyntErr(678);
                break;
            }
            if( g_iCtabmode == MODE_DICT_EXEC )
            {
                SetSyntErr(604);      // invalid parameter
                break;
            }
            CHECK_REPETITION(bHasInclude);
            NextToken();
            if( Tkn != TOKLPAREN )
            {
                SetSyntErr(LEFT_PARENT_EXPECTED);
                break;
            }

            ct->addOption(ct_NO_SPECIAL_VALUE);
            while( GetSyntErr() == 0 )
            {
                size_t include_type = 0;

                // added "UNDEFINED", rcl, Apr 2005
                if( CSettings::m_bNewTbd ) // RHF Sep 13, 2002
                    include_type = NextKeyword({ _T("SPECVAL"), _T("MISSING"), _T("DEFAULT"), _T("NOTAPPL"),
                    _T("ROWZERO"), _T("COLZERO"), _T("LAYZERO"), _T("REFUSED"), _T("UNDEFINED") });
                else
                    include_type = NextKeyword({ _T("SPECVAL"), _T("MISSING"), _T("DEFAULT"), _T("NOTAPPL"),
                    _T("ROWZERO"), _T("COLZERO"), _T("LAYZERO"), _T("REFUSED"), _T("UNDEFINED"),
                    _T("TOTALS"), _T("ROWTOT"), _T("COLTOT"), _T("LAYTOT"),
                    _T("PERCENTS"), _T("ROWPCT"), _T("COLPCT"), _T("LAYPCT"), _T("TOTPCT"),
                    _T("CHISQUARE"), _T("FREQ") });

                if( include_type == 0 )
                {
                    NextToken();
                    if( Tkn == TOKCOMMA )
                        continue;
                    else if( Tkn != TOKRPAREN && Tkn != TOKEOP )
                        SetSyntErr(607);      // invalid keyword
                    break;
                }

                // Token is valid -> activate flag
                if( include_type >= 1 && include_type <= 19 )
                    ct->addOption( aFlags[include_type] );

                else if( include_type == 20 )
                    ct->m_uEnviron[0] |= ct_FREQ;
            }
            if( GetSyntErr() == 0 && Tkn != TOKRPAREN && Tkn != TOKEOP )
                SetSyntErr(ERROR_RIGHT_PAREN_EXPECTED);
            break;
//RHF INIT Nov 14, 2006
        case TOKEXCLUDE:
            if( eTableType == CTableDef::Ctab_Hotdeck ) {
                SetSyntErr(678);
                break;
            }
            if( g_iCtabmode == MODE_DICT_EXEC )
            {
                SetSyntErr(604);      // invalid parameter
                break;
            }

            CHECK_REPETITION(bHasExclude);
            NextToken();

            if( Tkn != TOKLPAREN )
            {
                SetSyntErr(LEFT_PARENT_EXPECTED);
                break;
            }

            while( GetSyntErr() == 0 )
            {
                size_t exclude_type = 0;

                if( CSettings::m_bNewTbd ) // RHF Sep 13, 2002
                    exclude_type = NextKeyword({ _T("SPECVAL"), _T("MISSING"), _T("DEFAULT"), _T("NOTAPPL"),
                    _T("ROWZERO"), _T("COLZERO"), _T("LAYZERO"), _T("REFUSED"), _T("UNDEFINED") });
                else
                    exclude_type = NextKeyword({ _T("SPECVAL"), _T("MISSING"), _T("DEFAULT"), _T("NOTAPPL"),
                    _T("ROWZERO"), _T("COLZERO"), _T("LAYZERO"), _T("REFUSED"), _T("UNDEFINED"),
                    _T("TOTALS"), _T("ROWTOT"), _T("COLTOT"), _T("LAYTOT"),
                    _T("PERCENTS"), _T("ROWPCT"), _T("COLPCT"), _T("LAYPCT"), _T("TOTPCT"),
                    _T("CHISQUARE"), _T("FREQ") });

                if( exclude_type == 0 )
                {
                    NextToken();
                    if( Tkn == TOKCOMMA )
                        continue;
                    else if( Tkn != TOKRPAREN && Tkn != TOKEOP )
                        SetSyntErr(607);      // invalid keyword
                    break;
                }

                // Token is valid -> deactivate flag
                if( exclude_type >= 1 && exclude_type <= 19 )
                    ct->removeOption( aFlags[exclude_type] );

                else if( exclude_type == 20 )
                    ct->m_uEnviron[0] &= ~ct_FREQ;
            }
            if( GetSyntErr() == 0 && Tkn != TOKRPAREN && Tkn != TOKEOP )
                SetSyntErr(ERROR_RIGHT_PAREN_EXPECTED);
            break;
//RHF END Nov 14, 2006
        case TOKWEIGHT:
            // Table WEIGHT must appear before UNIT
            if( bHasUnit ) {
                SetSyntErr(8512);
                break;
            }

            /* RHF COM INIC Jul 04, 2002
            if( g_iCtabmode == MODE_APP_DECL ) {
                SetSyntErr(646);      // WEIGHT not allowed
                break;
            }
            RHF COM END Jul 04, 2002 */

            CHECK_REPETITION(bHasWeighted);
            NextToken();
            if( Tkn == TOKBY ) {
                NextToken();
            }

            i = exprlog();

            /* RHF COM INIC Jul 04, 2002
#ifdef GENCODE
            if( Flagcomp && ( g_iCtabmode == MODE_DICT_DECL  || g_iCtabmode == MODE_DICT_EXEC ) )
                ctabpt->weightexpr = i;
#endif
                RHF COM END Jul 04, 2002 */

            // RHF INIC Jul 04, 2002
#ifdef GENCODE
            if( i < 0 ) // Empty
                i = 0;

            if( Flagcomp ) {
                if( modeIsDictRelated() )
                    ctabpt->weightexpr = i;
                else
                    CTAB::pCurrentCtab->SetWeightExpr(-i); // See inheritance
            }
#endif
            // RHF END Jul 04, 2002

            bGetNextToken = false;
            break;
            case TOKTITLE:
                if( eTableType == CTableDef::Ctab_STable || eTableType == CTableDef::Ctab_Hotdeck ) {
                    SetSyntErr(616);      // invalid parameter
                    break;
                }
                if( g_iCtabmode == MODE_DICT_EXEC ) {
                    SetSyntErr(604);      // invalid parameter
                    break;
                }
                CHECK_REPETITION(bHasTitle);

                ct_title( ct, TOKTITLE );
                if( GetSyntErr() != 0 )
                    return 0;

                break;

            case TOKSTUB:
                if( eTableType == CTableDef::Ctab_STable || eTableType == CTableDef::Ctab_Hotdeck ) {
                    SetSyntErr(616);      // invalid parameter
                    break;
                }
                if( g_iCtabmode == MODE_DICT_EXEC ) {
                    SetSyntErr(604);
                    break;
                }
                CHECK_REPETITION(bHasStub);

                ct_title( ct, TOKSTUB );
                if( GetSyntErr() != 0 )
                    return 0;

                break;



                // RHF INIC Jan 23, 2003
            case TOKLEVEL:
                if( !modeIsDeclaration() ) {
                    SetSyntErr(8556);
                    break;
                }
                if( eTableType != CTableDef::Ctab_Crosstab ) {
                    SetSyntErr(8556);
                    break;
                }
                else {
                    bool     bValidLevel=true;
                    NextToken();

                    if( Tkn != TOKLPAREN )
                        SetSyntErr(LEFT_PARENT_EXPECTED);
                    else {

                        NextToken();
                        if( Tkn != TOKGROUP || !m_pEngineArea->IsLevel(Tokstindex) )
                            bValidLevel = false;
                        else {
                            GROUPT* pGrouptLevel=GPT(Tokstindex);
                            ASSERT( pGrouptLevel != 0 );
                            if( pGrouptLevel->GetLevel() <= 0 )
                                bValidLevel = false;
                            else {
                                iDefinedLevel = pGrouptLevel->GetLevel();

                                // RHF COM May 12, 2003ASSERT( g_iCtabmode == MODE_APP_DECL );

                                if( iDefinedLevel < CtVarHiLvl ) {
                                    bValidLevel = false;
                                    SetSyntErr(8560);
                                }
                                else
                                    CtVarHiLvl = iDefinedLevel;
                            }
                        }

                        if( bValidLevel ) {
                            NextToken();
                            if( Tkn != TOKRPAREN )
                                SetSyntErr(ERROR_RIGHT_PAREN_EXPECTED);
                        }
                        else if( GetSyntErr() == 0 )
                            SetSyntErr(8558);
                    }
                }
                break;
                // RHF END Jan 23, 2003

            case TOKUNIT:

                if( eTableType != CTableDef::Ctab_Crosstab ) {
                    SetSyntErr(8552);
                    break;
                }

                {
                    bool    bUnitAllowed  = modeIsDeclaration(); // RHF May 13, 2003 Add g_iCtabmode == MODE_DICT_DECL

                    if( !bUnitAllowed )
                        SetSyntErr(8400);
                    else {
                        // RHF INIC Jul 31, 2002 Add Generate SubTable list in the first UNIT option
                        if( !bSubTablesAdded ) {
                            ct->MakeSubTables( CtAux1base, ctres );
                            bSubTablesAdded = true;
                        }
                        // RHF END Jul 31, 2002

                        CompileUnit(CtAux1base);
                        bGetNextToken = false;
                    }
                }

                bHasUnit = true;

                break;

            case TOKSTAT:
                // RHF INIC Jun 04, 2003
                if( !CSettings::m_bNewTbd ) {
                    SetSyntErr(604);
                    break;
                }
                // RHF END Jun 04, 2003

                // Stat must appear before unit
                if( bHasUnit ) {
                    SetSyntErr(8510);
                    break;
                }

                // Only allowed in declaration
                if( !modeIsDeclaration() ) {
                    SetSyntErr(8520);
                    break;
                }

                if( eTableType != CTableDef::Ctab_Crosstab ) {
                    SetSyntErr(8550);
                    break;
                }
                CHECK_REPETITION(bHasStat);

                // RHF INIC Aug 02, 2002
                //if( !bSubTablesAdded ) {
                    ct->MakeSubTables( CtAux1base, ctres );
                //    bSubTablesAdded = true;
                //}
                // RHF END Aug 02, 2002

                CompileStat(CtAux1base); // Eat RParen
                bGetNextToken = false;

                if( GetSyntErr() != 0 )
                    break;
                // Can change CtAux1Next & ctres
                ct->AddStatCoordinates(CtAux1base, CtAux1mxent, CtAux1Next, ctres);

                if( GetSyntErr() != 0 )
                    break;

                break;

            case TOKTABLOGIC:
                // Table TAGLOGIC must appear before UNIT
                if( bHasUnit ) {
                    SetSyntErr(8512);
                    break;
                }

                if( eTableType == CTableDef::Ctab_Hotdeck ) {
                    SetSyntErr(678);
                    break;
                }

// RHF COM Jul 03, 2002                 if( g_iCtabmode == MODE_APP_DECL ) {
// RHF COM Jul 03, 2002                    SetSyntErr(8600);      // TABLOGIC not allowed
// RHF COM Jul 03, 2002                    break;
// RHF COM Jul 03, 2002                 }

                CHECK_REPETITION(bHasTablogic);
                NextToken();

                i = instruc();

                if( GetSyntErr() == 0 && Tkn != TOKENDLOGIC ) {
                    SetSyntErr(8602);
                }

                if( GetSyntErr() != 0 )
                    break;
                NextToken();
#ifdef GENCODE
                if( i < 0 ) // Empty
                    i = 0;

                if( Flagcomp ) {
                    if( modeIsDictRelated() )
                        ctabpt->tablogicexpr = i;
                    else
                        CTAB::pCurrentCtab->SetTabLogicExpr(-i); // See inheritance
                }
#endif
                bGetNextToken = false;

                break;

            default:
                // stop analysis for some tokens (no ';' required in Issa2)
                if( g_iCtabmode == MODE_APP_DECL )
                {
                    bStopScan = StopTableScan();
                }

                if( !bStopScan )
                    SetSyntErr(604);      // Invalid CrossTab parameter
                break;
          }
          if( bGetNextToken && GetSyntErr() == 0 && !bStopScan )
              NextToken();
      }

    if( GetSyntErr() != 0 )
        return 0;

    // RHF INIC Jul 31, 2002
    if( !bSubTablesAdded ) {
        ct->MakeSubTables( CtAux1base, ctres );
        bSubTablesAdded = true;
    }
    // RHF END Jul 31, 2002

    // Assign default units
      if( modeIsDeclaration() ) // RHF Aug 08, 2002
        SetSyntErr( ct->AddDefaultUnits(CtAux1base) ); // RHF Jun 27, 2002

      if( modeIsDeclaration() ) {
        if( bHasNobreak )
            ct->m_uEnviron[0] &= ~ct_BREAK;

          if( bHasNoauto && g_iCtabmode == MODE_DICT_DECL )
            ct->m_uEnviron[0] &= ~ct_AUTOTALLY;

        if( bHasNoprint )
            ct->m_uEnviron[0] &= ~ct_PRINT;

        if( bHasNofreq )
            ct->m_uEnviron[0] &= ~ct_FREQ;

        // Optimize the arrays and add the INCLUDE m_iOptions
        for( i = 0; i < ct->GetNumDim(); i++ ) {
            ctres[i] = ctexp2( CtAux1base[i], ctres[i],
                  ct->getOptions() & (ct_ALL_SPECIAL_VALUES|ct_NO_SPECIAL_VALUE),
                CtAux2base, CtAux2mxent, &CtAux2Next );

            octaux1base = CtAux1base[i];
            CtAux1base[i] = CtAux2base;
            CtAux1Next[i] = CtAux2Next;

            ct->SetNodeExpr( CtNodenext + ctres[i], i );

            //  RHF Jul 01, 2004, See memset( (csprochar*) pRange->m_bFiller, 0, 4 ) below!!
            ctadjustparent( CtAux1base[i], ctres[i], CtNodenext ); // RHF Jul 01, 2004

              /* RHF COM INIC Jul 01, 2004
              for( j = 0; j < CtAux1Next[i]; j++ ) {
                  pAux1Base = CtAux1base[i] + j;

                  // if( *pAux1Base < 0 )         // '+' OR '*' // WARNING DON'T SAVE OTHER NEGATIVE ELEMENT IN THE TREE!!!// RHF Jul 17, 2001
                  if( *pAux1Base == CTNODE_OP_ADD || *pAux1Base == CTNODE_OP_MUL ) {  // '+' OR '*' // RHF Jul 17, 2001
                  // Notice that the logic above could still work if you
                  // make the cast to a CTNODE afterwards and then ask if that CTNODE is
                  // an operator node.
                  // rcl, May 2005
                      // RHF INIC Jan 24, 2003
                      CTNODE* pLocalNode= (CTNODE*) ( pAux1Base ); // rcl, May 2005
                      if( !pLocalNode->isOperNode() )  // double check, rcl, May 2005
                          continue;
                      if( pLocalNode->m_iCtLeft >= 0  ) {
                          CTNODE* pLeftNode = (CTNODE*) (CtAux1base[i] + pLocalNode->m_iCtLeft );
                          if( pLeftNode->isOperNode() )// RHF May 07, 2004 // rcl, May 2005
                          pLeftNode->m_iParentIndex += CtNodenext;
                      }

                      if( pLocalNode->m_iCtRight >= 0  ) {
                          CTNODE* pRightNode = (CTNODE*) (CtAux1base[i] + pLocalNode->m_iCtRight );
                          if( pRightNode->isOperNode() )// RHF May 07, 2004 // rcl, May 2005
                          pRightNode->m_iParentIndex += CtNodenext;
                      }
                      // RHF END Jan 24, 2003

                      pLocalNode->m_iCtLeft += CtNodenext;  // rcl, May 2005
                      pLocalNode->m_iCtRight += CtNodenext; // rcl, May 2005
                  }
              }
              RHF COM END Jul 01, 2004 */
            for (j = 0; j < CtAux1Next[i]; j++) {
                //Savy checking memory overwrite
                if (CtNodenext >= CtNodemxent) {
                    SetSyntErr(642);
                    return 0;
                }
                *(CtNodebase + CtNodenext++) = *(CtAux1base[i] + j);
            }

            ct->SetTotDim( ctcell( CtNodebase, ct->GetNodeExpr(i) ), i );
            CtAux1base[i] = octaux1base;
        }
    }

      if( g_iCtabmode == MODE_APP_DECL )
        ct->SetTableLevel( CtVarHiLvl );
      else if ( g_iCtabmode == MODE_DICT_DECL )
        ct->SetTableLevel( LvlInComp );

    // RHF INIC Apr 16, 2003
    if( bHasBreak && modeIsDeclaration() )
        m_pEngineArea->m_CtabBreakHighLevel = std::max( m_pEngineArea->m_CtabBreakHighLevel, ct->GetTableLevel() );
    // RHF END Apr 16, 2003

      if( g_iCtabmode == MODE_APP_DECL )
        xtab_level = 0;
    else
        xtab_level = LvlInComp;

    decl_level = LvlInComp + 1;         // initializes an erroneous value
      if( modeIsDeclaration() )
        decl_level = CtVarHiLvl;
      else if( g_iCtabmode == MODE_DICT_EXEC )
        decl_level = ct->GetTableLevel() % 10;

      if( modeIsDictRelated() && decl_level > LvlInComp )
        return( SetSyntErr(87), 0 );

    ct->SetTableLevel( xtab_level * 10 + decl_level );

    for( i = 0; i < ct->GetNumDim(); i++ ) {
        if( ct->GetTerm(i) != NULL )     // SEE CDECLARE
            free( ct->GetTerm(i) );
        ct->SetTerm( ctmaketerm( ct->GetNodeExpr(i) ), i );
    }

    //Remake the sub-tables with the final tree!
    // Don't change the total number of subtables!
    // RHF INIC Jul 09, 2002
      if( modeIsDeclaration() ) {
        int*    pNodeBase[TBD_MAXDIM];
        int     iRoot[TBD_MAXDIM];

        for( i = 0; i < TBD_MAXDIM; i++ ) {
            if( i < ct->GetNumDim() ) {
                pNodeBase[i] = CtNodebase;
                iRoot[i] = ct->GetNodeExpr(i);
            }
            else {
                pNodeBase[i] = NULL;
                iRoot[i] = -1;
            }
        }

        ct->MakeSubTables( pNodeBase, iRoot );

        // Remake coordnumber
        for( int iSubTable = 0; iSubTable < ct->GetNumSubTables(); iSubTable++ ) {
            CSubTable& cSubTable=ct->GetSubTable(iSubTable);
            cSubTable.MakeCoordNumberMap( ct, pNodeBase );

            // RHF COM Aug 13, 2002if( ct->GetNumCells() <= MAXCELL_REMAP ) // If greater than MAXCELL_REMAP use GetTableCoordOnLine & GetSubTableCoordOnLine methods
            // RHF COM Aug 13, 2002 cSubTable.GenRemapCoord( m_pEngineArea );
        }
    }
    // RHF END Jul 09, 2002

    // RHF INIC Jul 30, 2002
      if( modeIsDeclaration() ) {
          if (ct->GetRunTimeVersion() > 2) // RHF May 07, 2004
              if( !ct->MakeAuxCtabs() ) {
                  return ( SetSyntErr(8710), 0 );
              }
      }
    // RHF END Jul 30, 2002

    m_bcvarsubcheck = false;

    return( iptctab );
}

void CEngineCompFunc::ctauxalloc() {                    // build arrays to compile dimensions
    int     i;
    int     iMaxEntries=CtNodemxent;

    CtAux1mxent[0] = CtAux1mxent[1] = CtAux1mxent[2] = CtAux2mxent = iMaxEntries;

    for( i = 0; i <= 2; i++ )
        CtAux1base[i] = NULL;
    CtAux2base = NULL ;

    for( i = 0; i <= 2; i++ ) {
        CtAux1Next[i] = 0;
        if( CtAux1mxent[i] > 0 ) {
            CtAux1base[i] = (int*) calloc( CtAux1mxent[i], sizeof(int) );
            if( CtAux1base[i] == NULL )
                SetSyntErr(642);          // no memory
        }
    }

    if( CtAux2mxent > 0 ) {
        CtAux2base = (int*) calloc( CtAux2mxent, sizeof(int) );
        if( CtAux2base == NULL )
            SetSyntErr(642);              // no memory
    }
}

void CEngineCompFunc::ctauxfree() {
    int     i;

    for( i = 0; i <= 2; i++ ) {
        if( CtAux1base[i] != NULL ) {
            free( (csprochar*) CtAux1base[i] );
            CtAux1base[i] = NULL;
        }
    }

    if( CtAux2base != NULL ) {
        free( (csprochar*) CtAux2base );
        CtAux2base = NULL ;
    }
}

int CEngineCompFunc::ct_title( CTAB* ct, int iFlag ) { //  compile titles
    NextToken();
    if( Tkn != TOKLPAREN )
        return( SetSyntErr(LEFT_PARENT_EXPECTED), 0 );

    if( iFlag == TOKTITLE )
        ct->GetTitle().RemoveAll();
    else if( iFlag == TOKSTUB )
        ct->GetStubTitle().RemoveAll();

    do {
        NextToken();
        if( Tkn == TOKSCTE ) {
            int     iLen;

            if( ( iLen = (int)Tokstr.length() )  >= 0 ) {
                if( iFlag == TOKTITLE ) {
                    ct->GetTitle().Add( WS2CS(Tokstr) );

                    iLen += ct->GetTitleLen() + 1;

                    ct->SetTitleLen( iLen );
                }
                else if( iFlag == TOKSTUB ) {
                    ct->GetStubTitle().Add( WS2CS(Tokstr) );

                    iLen += ct->GetStubLen() + 1;

                    ct->SetStubLen( iLen );
                }
            }
        }
        else
            return( SetSyntErr(518), 0 );
        NextToken();
    } while( Tkn == TOKCOMMA );

    if( Tkn != TOKRPAREN )
        return( SetSyntErr(ERROR_RIGHT_PAREN_EXPECTED), 0 );

    return 0;
}

/*---------------------------------  CT.C  -------------------------------*/

static  bool    g_bOpenMult = false;
static  int     g_iCtNodeaux;

static  int     iNumPairs;
static  int     iNumCells;                 // Number of implicit cells

#define CTMAX_RANGES    4000
static  double  Ctvrange[2*CTMAX_RANGES]; // RHF 25/08/99
static  int     Ctvrangecollapsed[CTMAX_RANGES];
static  int     Ctvrnum;
static  int     g_iCtvFlags;

#include "CompCTab.h"

ExplicitIncludeRanges includeRanges;

static  int*    Ct_Auxbase;

int CEngineCompFunc::ctexpdep( CTAB* ct ) { // compile dependent var (MEAN/SMEAN)
    ctvardep( ct, 0 );
    if( GetSyntErr() != 0 )
        return -1;

    if( Tkn == TOKDIVOP )
        if( ct->GetTableType() == CTableDef::Ctab_SMean )
        {
            NextToken();
            ctvardep( ct, 1 );
            if( GetSyntErr() != 0 )
                return -1;
        }
        else
            return( SetSyntErr(681), -1 );// "/" allowed only for SMEAN

        return 0;
}

int CEngineCompFunc::ctvardep( CTAB* ct, int i_dep ) {
    int     iSubIndex, iSymVar;
    CString csVarName;
    VART*   pVarT;

    if( Tkn != TOKVAR )
        return( SetSyntErr(680), -1 );
    iSymVar = Tokstindex;

    pVarT = VPT(iSymVar);
    if( !pVarT->IsNumeric() )
        return( SetSyntErr(683), -1 );    // alphanumeric variable

    // stores symbol of dependent var
    ct->SetDepSymVar(iSymVar, i_dep );

#ifdef GENCODE
    if( Flagcomp )
        pVarT->SetUsed( true );
#endif

    if( CtVarHiLvl < pVarT->GetLevel() )                   // victor Jul 10, 00
        CtVarHiLvl = pVarT->GetLevel();                    // victor Jul 10, 00

    // RHF COM Feb 08, 2001   NextToken();

    if( IsCurrentTokenVART(*this) && VPT(Tokstindex)->IsArray() )  {
        NextToken();// RHF Feb 08, 2001 Fix problem a multiple var
        if( Tkn != TOKLPAREN )
            return( SetSyntErr(627), -1 );// no index for Mult

        NextToken();
        if( Tkn != TOKCTE || ( iSubIndex = (int) Tokvalue ) != Tokvalue )
            return( SetSyntErr(82), -1 ); // integer expected

        iSubIndex = (int) Tokvalue;
        int vmaxoccs = VPT(Tokstindex)->IsArray() ? SPT(VPT(Tokstindex)->GetOwnerSec())->GetMaxOccs() : 1;
        if( iSubIndex < 1 || iSubIndex > vmaxoccs )
            return( SetSyntErr(628), -1 );// wrong occurrence

        NextToken();
        if( Tkn != TOKRPAREN )
            return( SetSyntErr(ERROR_RIGHT_PAREN_EXPECTED), -1 );

        // stores index of Mult var
        ct->SetDepVarOcc( iSubIndex, i_dep );
        NextToken();
    }
    else NextToken(); // RHF Feb 08, 2001

    return 0;
}

int CEngineCompFunc::ctexp1( int iDim, int* ctbase1, int ctmxent1, int* ctnext1 ) {
    int     auxctmxent;
    int     auxctnext;
    int*    auxctbase;
    int     r;

    ASSERT( iDim == DIM_ROW || iDim == DIM_COL || iDim == DIM_LAYER );

    // save general arrays
    auxctmxent = CtNodemxent;
    auxctnext = CtNodenext;
    auxctbase = CtNodebase;

    CtNodemxent = ctmxent1;
    CtNodenext = 0;
    CtNodebase = ctbase1;

    r = ctadd();

    ctbase1 = CtNodebase;
    *ctnext1 = CtNodenext;

    // restore the general arrays
    CtNodemxent = auxctmxent;
    CtNodenext = auxctnext;
    CtNodebase = auxctbase;

    return( r );
}

int CEngineCompFunc::ctadd() {
    int     p1, p2;

    p1 = ctmul();
    if( GetSyntErr() != 0 )
        return -1;

    while( Tkn == TOKADDOP )
    {
        NextToken();
        p2 = ctmul();
        if( GetSyntErr() != 0 )
            return -1;

        p1 = ctopernode( p1, p2 , CTNODE_OP_ADD );
        if( GetSyntErr() != 0 )
            return -1;
    }

    return( p1 );
}

int CEngineCompFunc::ctmul() {
    int     p1, p2;

    p1 = ctvar();
    if( GetSyntErr() != 0 )
        return -1;

    while( Tkn == TOKMULOP )
    {
        // RHF INIC 26/9/96 STABLE
        if( g_bRestrictedExpresion )
            return( SetSyntErr(650), -1 );
        // RHF END 26/9/96 STABLE

        if( g_bLayerExpresion )                   // RHF 5/9/94
            return( SetSyntErr(649), -1 );        // RHF 5/9/94

        if( !g_bFreeExpresion ) { // RHF Jul 25, 2001
            if( *(CtNodebase + p1) == CTNODE_OP_ADD || *(CtNodebase + p1) == CTNODE_OP_MUL || g_bOpenMult ) // is '+' or '*' // RHF Jul 17, 2001
                // RHF COM Jul 17, 2001 if( *(CtNodebase + p1) < 0 || g_bOpenMult ) // is '+' or '*'
                return( g_bOpenMult = false, SetSyntErr(650), -1 );
        }// RHF Jul 25, 2001

        // RHF INIC Nov 21, 2003
        if( *(CtNodebase + p1) == CTNODE_OP_MUL ) {
           return( g_bOpenMult = false, SetSyntErr(650), -1 );

        }
        // RHF END Nov 21, 2003

        g_bOpenMult = true;
        NextToken();
        p2 = ctvar();
        if( GetSyntErr() != 0 )
            return( g_bOpenMult = false, -1 );

        p1 = ctopernode( p1, p2 , CTNODE_OP_MUL );
        if( GetSyntErr() != 0 )
            return( g_bOpenMult = false, -1 );

        g_bOpenMult = false;
    }

    return( p1 );
}

int CEngineCompFunc::ctvar() {
    int     iSymbol, iSubIndex, p1, i;
    CString csVarName;
    CTNODE* pCtNode;
    VART* pVarT = nullptr;
    const ValueSet* pValueSetVar = nullptr;
    bool    bIsVar, bIsValueSet;

    p1 = CtNodenext;

    switch( Tkn )
    {
        case TOKVAR:
        case TOKVALUESET:       // RHF Jul 05, 2001 Add Value-set
        {
            bIsVar = ( Tkn == TOKVAR );
            bIsValueSet = ( Tkn == TOKVALUESET );
            iSymbol = Tokstindex;

            if( bIsValueSet ) {
                CTAB::pCurrentCtab->SetRunTimeVersion( 3 );
                pValueSetVar = &GetSymbolValueSet(iSymbol);

                if( pValueSetVar->IsDynamic() )
                    IssueError(47169, pValueSetVar->GetName().c_str());

                pVarT = pValueSetVar->GetVarT();
            }
            else {
                ASSERT( bIsVar );
                pVarT = VPT(iSymbol);
            }

            if( !pVarT->IsNumeric() )
                return( SetSyntErr(623), -1 );    // alphanumeric variable

            // When variable has decimal places, we will only allow the user to
            // specify its valueset.
            // If user specifies the variable itself, CSPro will issue an error
            // message number 624
            if( !bIsValueSet &&
                pVarT->GetDecimals() > 0 )
                return( SetSyntErr(624), -1 );    // decimals not allowed

            iNumCells = ctloadvar( pVarT, pValueSetVar );

            if( GetSyntErr() != 0 )
                return -1;

#ifdef GENCODE
            if( Flagcomp ) {
                pVarT->SetUsed( true );
            }
#endif
            if( CtVarHiLvl < pVarT->GetLevel() )
                CtVarHiLvl = pVarT->GetLevel();

            pCtNode = (CTNODE*) ( CtNodebase + CtNodenext );

            CtNodenext += CTNODE_SLOTS;
            if( CtNodenext >= CtNodemxent )
                return( SetSyntErr(4), -1 );

            g_iCtNodeaux = CtNodenext;
            iNumPairs = 0;
            pCtNode->init();
            pCtNode->setVarNode(iSymbol);
            pCtNode->setNumCells(iNumCells);
            pCtNode->m_iStatType = CTSTAT_NONE;

            // Sequence Item Number
            CTAB::pCurrentCtab->SetCurrentCoordNumber(CTAB::pCurrentCtab->GetCurrentCoordNumber()+1);
            pCtNode->m_iCoordNumber = CTAB::pCurrentCtab->GetCurrentCoordNumber(); // RHF Jun 20, 2002
            ASSERT( pCtNode->m_iCoordNumber >= 1 );

            // Sequence Number
            const auto& sequence_lookup = m_aCtCoordRepeatSymbol.find(iSymbol);
            pCtNode->m_iSeqNumber = ( sequence_lookup != m_aCtCoordRepeatSymbol.cend() ) ? ( sequence_lookup->second + 1 ) : 1;
            m_aCtCoordRepeatSymbol[iSymbol] = pCtNode->m_iSeqNumber;

            // Is Array
            if( pVarT->IsArray() ) {
                int     iOldForTableNext=m_ForTableNext;

                m_ForTableNext = 0; // All implicit index will be MVAR_GROUP.

                bool    bUseAllIndexes;
                Tokstindex = pVarT->GetSymbolIndex(); // RHF Aug 12, 2002

                iSubIndex = varsanal( _T('N'), COMPLETE_COMPILATION, &bUseAllIndexes,
                                        DO_NOT_TRY_TO_COMPLETE_DIMS );
                if( GetSyntErr() != 0 )
                    return -1;

                m_ForTableNext = iOldForTableNext;
#ifdef GENCODE
                ASSERT( iSubIndex >= 0 ); // RHF Jul 16, 2004
                iSubIndex++; // RHF Jul 16, 2004
#else
    // RHF COM Aug 12, 2002                ASSERT( iSubIndex == 0 );
                if( iSubIndex == 0 ) iSubIndex++;  // In NO GENCODE return 0
#endif
                ASSERT( iSubIndex != 0 );

                // = 0: Single, Never here!
                // > 0: Multiple with [all] explicit subindexes [rcl, Nov 2004]
                // < 0: Multiple without subindex
                if( bUseAllIndexes )
                    pCtNode->m_iCtOcc = iSubIndex;
                else
                    pCtNode->m_iCtOcc = -iSubIndex;
            }
            else
                NextToken();

            if( Tkn == TOKLBRACK ) {
                if( bIsValueSet )
                    return( SetSyntErr(70001), -1 );

                iNumCells = 0;
                NextToken();

                int iCollapsed=0;
                g_iCtvFlags = ct_NOFLAGS;
                ctrange( ct_NOFLAGS, &iCollapsed,        // MISSING, DEFAULT, NOTAPPL, collapsed
                         pVarT->GetDecimals() > 0 ? ALLOW_DOUBLE_CONSTS : DO_NOT_ALLOW_DOUBLE_CONSTS );
                if( GetSyntErr() != 0 )
                    return -1;

                // Pending Collapsed
                if( iCollapsed > 0 )
                    return( SetSyntErr(70000), -1 );

                // as part of the 7.3 compiler refactoring, Tknant [PreviousToken()] was removed, it was only
                // used below; it's not clear if this code is ever executed, so this ASSERT is added just so that
                // if it's ever called, someone can look at how this should be refactored
                ASSERT(false);
                if( Tkn == TOKRBRACK /* && Tknant != TOKLBRACK */ )
                {
                    NextToken();

                    // Change original number of pairs and cells for this item
                    pCtNode->m_iCtNumRanges = iNumPairs;
                    pCtNode->setNumCells(iNumCells);
                    if( g_iCtvFlags != ct_NOFLAGS )
                    {
                        pCtNode->m_iFlags = g_iCtvFlags;
                        pCtNode->m_bSpecialFlagsInserted = true;
                    }
                }
                else                    // error in square brackets
                    return( SetSyntErr(630), -1 );
            }
            else if( iNumPairs == 0 && (Ctvrnum >= 1 || g_iCtvFlags != ct_NOFLAGS) )
            {                         // no range inside brackets
                pCtNode->m_iCtNumRanges = Ctvrnum;
                pCtNode->m_iFlags = g_iCtvFlags;

                for( i = 0; i < Ctvrnum; i++ ) {
                    ctaddrange( Ctvrange[2 * i], Ctvrange[2 * i + 1], Ctvrangecollapsed[i], NULL );
                    if( GetSyntErr() != 0 )
                        return -1;
                }
            }
            break;
        }

        case TOKLPAREN:
        {
            NextToken();
            p1 = ctadd();
            if( GetSyntErr() != 0 )
                return -1;

            if( Tkn != TOKRPAREN )      // unbalanced parenthesis
                return( SetSyntErr(ERROR_RIGHT_PAREN_EXPECTED), -1 );
            NextToken();
            break;
        }

        default:                        // wrong token
        {
            return( SetSyntErr(21), -1 );
        }
    }

    if( iNumCells > MAXCELL )
        SetSyntErr(640); // too many cells

    return( p1 );
}

//#define TOKLCOLLAPSE TOKLBRACK
//#define TOKRCOLLAPSE TOKRBRACK
#define TOKLCOLLAPSE TOKLPAREN
#define TOKRCOLLAPSE TOKRPAREN

// ctrange: // compile bracket expression [...]
int CEngineCompFunc::ctrange( int iSpecialValuesFlags, int* piCollapsed, bool bAllowDoubleConsts ) {
    ASSERT( piCollapsed != 0 );
    int iSpecIndex = -1;
    bool bMinus = false;

    switch( Tkn )
    {
        // RHF INIC Jul 03, 2001
    case TOKLCOLLAPSE:
        CTAB::pCurrentCtab->SetRunTimeVersion( 3 );
        if( *piCollapsed > 0 )
            return( SetSyntErr(70000), 0 );
        *piCollapsed = 1;
        NextToken();

        ctrange( iSpecialValuesFlags, piCollapsed, bAllowDoubleConsts );
        if( GetSyntErr() != 0 )
            return 0;

        if( Tkn != TOKRCOLLAPSE )
            return( SetSyntErr(630), 0 );

        *piCollapsed = 0;
        NextToken();
        if( Tkn == TOKCOMMA ) {
            NextToken();
            if( Tkn == TOKRBRACK )
                return( SetSyntErr(630), 0 );
        }

        ctrange( iSpecialValuesFlags, piCollapsed, bAllowDoubleConsts );
        if( GetSyntErr() != 0 )
            return 0;

        break;
        // No break if is other version
        // RHF END Jul 03, 2001
    case TOKVAR:
        if( !SO::EqualsNoCase(Tokstr, _T("SPECVAL")) )
            return( SetSyntErr(634), 0 );     // Solo palabras claves

        ctnewrange( VAL_CTMISSING, VAL_CTUNDEFINED, *piCollapsed, &iNumCells ); // SPECVAL now includes undefined
        if( *piCollapsed > 0 )
            (*piCollapsed)++;
        if( GetSyntErr() != 0 )
            return 0;

        iNumPairs++;

        NextToken();
        if( Tkn == TOKCOMMA )
            NextToken();

        iSpecialValuesFlags |= ct_ALL_SPECIAL_VALUES;
        ctrange( iSpecialValuesFlags, piCollapsed, bAllowDoubleConsts );
        if( GetSyntErr() != 0 )
            return 0;
        break;

    case TOKMINUS:
          bMinus = true;
          NextToken(); // try to read next token
                       // to find the const value used
          if( Tkn != TOKCTE )
          {
              return( SetSyntErr(631), 0 );  // missing upper limit
              // break;
          }
    case TOKCTE:
        // try to see if a special value has been used
        iSpecIndex = specialString2Index(Tokstr);
        if( iSpecIndex != -1 )
        {
            g_iCtvFlags |= aTheFlags[iSpecIndex];
            ctnewrange( aTheCTValues[iSpecIndex], aTheCTValues[iSpecIndex], *piCollapsed, &iNumCells );
            if( *piCollapsed > 0 )
                (*piCollapsed)++;
            if( GetSyntErr() != 0 )
                return 0;

            iNumPairs++;
            NextToken();
            if( Tkn == TOKCOMMA )
                NextToken();

            iSpecialValuesFlags |= aTheFlags[iSpecIndex];
            ctrange( iSpecialValuesFlags, piCollapsed, bAllowDoubleConsts );
            if( GetSyntErr() != 0 )
                return 0;
            break;
        }
        else
        {
            double rLow, rHigh;

            if( bAllowDoubleConsts == DO_NOT_ALLOW_DOUBLE_CONSTS )
            {
                // RHF COM Jul 01, 2005 int iAuxValue = (int) Tokvalue;
                __int64 iAuxValue = (__int64) Tokvalue; // RHF Jul 01, 2005
                if( iAuxValue != Tokvalue )
                    return( SetSyntErr(82), -1 ); // integer expected
            }

            rLow = (double) Tokvalue;

            // Consider sign, please
            if( bMinus )
                rLow = -rLow;

            // check if rLow exceed max. nr. of digits allowed
            if( isSmaller(rLow,-CTAB_MAXVALINT) )
                return( SetSyntErr(629), 0 );

            if( isBigger(rLow,CTAB_MAXVALINT) )
                return( SetSyntErr(629), 0 );

            NextToken();
            if( Tkn == TOKCOMMA )
            {
                ctaddrange( rLow, rLow, *piCollapsed, &iNumCells );
                if( *piCollapsed > 0 )
                    (*piCollapsed)++;
                if( GetSyntErr() != 0 )
                    return 0;

                iNumPairs++;

                NextToken();

                ctrange( iSpecialValuesFlags, piCollapsed, bAllowDoubleConsts );
                if( GetSyntErr() != 0 )
                    return 0;
            }

            else if( Tkn == TOKRBRACK || Tkn == TOKRCOLLAPSE )
            {
                ctaddrange( rLow, rLow, *piCollapsed, &iNumCells );
                if( GetSyntErr() != 0 )
                    return 0;

                iNumPairs++;
            }
            else if( Tkn == TOKCOLON )
            {
                NextToken();
                bMinus = false;
                if( Tkn == TOKMINUS )
                {
                    bMinus = true;
                    NextToken();
                }

                if( Tkn != TOKCTE )
                    return( SetSyntErr(631), 0 );  // missing upper limit

                if( bAllowDoubleConsts == DO_NOT_ALLOW_DOUBLE_CONSTS )
                {
                    // RHF COM Jul 01, 2005 int iAuxValue = (int) Tokvalue;
                    __int64 iAuxValue = (__int64) Tokvalue; // RHF Jul 01, 2005
                    if( iAuxValue != Tokvalue )
                        return( SetSyntErr(82), -1 ); // integer expected
                }

                rHigh = (double) Tokvalue;

                // Consider sign, please
                if( bMinus )
                    rHigh = -rHigh;

                // check if rHigh exceed max. nr. of digits allowed
                if( rHigh < -CTAB_MAXVALINT )
                    return( SetSyntErr(629), 0 );

                if( rHigh > CTAB_MAXVALINT )
                    return( SetSyntErr(629), 0 );

                if( rHigh < rLow )
                    return( SetSyntErr(632), 0 ); // disordered limits

                int  iAuxNumPairs = ctaddrange( rLow, rHigh, *piCollapsed, &iNumCells );
                if( *piCollapsed > 0 )
                    (*piCollapsed)++;
                if( GetSyntErr() != 0 )
                    return 0;

                iNumPairs += iAuxNumPairs;

                NextToken();
                if( Tkn == TOKCOMMA )
                {
                    NextToken();

                    ctrange( iSpecialValuesFlags, piCollapsed, bAllowDoubleConsts );
                    if( GetSyntErr() != 0 )
                        return 0;
                }
            }
          }
          break;
      }

      return( 1 );
}

#define MIN_LOWER_RANGE (-CTAB_MAXVALINT)

int CEngineCompFunc::ctaddrange( double rLow, double rHigh, int iCollapsed, int* piAuxNumCells ) {
    int     iAuxNumPairs = 0;

    if( rLow >= MIN_LOWER_RANGE && lowerOrEqual(rLow,rHigh) && (m_pEngineDriver->m_bTabProcessSpecial || lowerOrEqual(rHigh,CTAB_MAXVALINT) ) ) { //SAVY&& Commented out the MAXVAL stuff to allow Specval
        ctnewrange( rLow, rHigh, iCollapsed, piAuxNumCells );
        if( GetSyntErr() != 0 )
            return 0;
        iAuxNumPairs++;
        return iAuxNumPairs;
    }

    else
        return( SetSyntErr(638), 0 );
}

// Load the ranges using pValueSetVar
int CEngineCompFunc::ctloadvar( const VART* pVarT, const ValueSet* pValueSetVar )
{
    int         iVarLen;
    const CDictItem* pDictItem = pVarT->GetDictItem();
    const DictValueSet* pVS = ( pValueSetVar != nullptr ) ? &pValueSetVar->GetDictValueSet() :
                                                            pDictItem->GetFirstValueSetOrNull();

    std::vector<Range<double>> aRanges;
    std::vector<int> aCollapsed;
    int iNumRanges = 0;

    //////////////////////////////////////////////////////////////////////////
    // Process Special Values and consider them
    // rcl <begin>
    g_iCtvFlags = ct_NOFLAGS;
    if( pDictItem != NULL )
    {
        std::vector<Range<double>> aRangesAux;
        if( pVS != nullptr )
            iNumRanges = DoIssaRanges(pDictItem, aRangesAux, pVS, NULL, NULL, true);
        if( iNumRanges > 0 ) {
            for( int i=0; i < iNumRanges; i++ ) {
                const Range<double>& range = aRangesAux[i];

                if( isSpecialValue(range.low) )
                {
                    ASSERT( specialValue2Index(range.low) >= 0 );
                    ASSERT( specialValue2Index(range.low) < 5 );
                    g_iCtvFlags |= aTheFlags[specialValue2Index(range.low)];
                }
            }
        }
    }
    // rcl <end>
    //////////////////////////////////////////////////////////////////////////

    if( pDictItem != NULL && pVS != nullptr ) // In Working Variables is NULL
        iNumRanges = DoIssaRanges(pDictItem, aRanges, pVS, NULL, &aCollapsed,false/*m_pEngineDriver->m_bTabProcessSpecial*/ ); //SAVY for tab / freqs

    if( iNumRanges <= 0 )
        Ctvrnum = 0;
    else
        Ctvrnum = iNumRanges;

    if( Ctvrnum >= CTMAX_RANGES ) {
        ASSERT( 0 );
        return( SetSyntErr(679), 0 );     // Too many ranges
    }

    iVarLen  = pVarT->GetLength();
    iNumCells  = 0;

    if( iVarLen > CTAB_MAX_DIGITS )
        return( SetSyntErr(625), 0 );     // variable has more than CTAB_MAX_DIGITS digits

    if( Ctvrnum > 0 ) {

        for( int i=0,j=0; i < Ctvrnum; i++, j+=2 ) {
            const Range<double>& range = aRanges[i];

            Ctvrange[j]   = range.low;
            Ctvrange[j+1] = range.high;

            // RHF INIC Dec 18, 2001
            if( CTAB::pCurrentCtab->GetRunTimeVersion() < 3 && !CSettings::m_bNewTbd )
                Ctvrangecollapsed[i] = 0;
            else
                // RHF END Dec 18, 2001
                Ctvrangecollapsed[i] = aCollapsed[i]; //0 no collapsed. 1..n indicates that is collapsed

            iNumCells += CTRANGE::getNumCells( Ctvrange[j], Ctvrange[j+1], Ctvrangecollapsed[i] );
        }
    }
    else {                                // no ranges in variable:

        int iMaxLength = (int) log10( (float) MAXCELL);
        if( iVarLen > iMaxLength )
            iNumCells = MAXCELL + 1; // to generate a 'too many cells' error somewhere outside
        else
        {
            ASSERT( iVarLen <= 4 ); // iVarLen > 4 would produce 10^5 or more iNumCells
            iNumCells = (int) pow( (double) 10, (double) iVarLen );
            ASSERT( iNumCells <= MAXCELL );
            Ctvrange[0] = (double) 0;
            Ctvrange[1] = (double) (iNumCells - 1);
            Ctvrangecollapsed[0] = 0;
            Ctvrnum = iNumRanges = 1;       // ... one "wide" range
        }
    }

    if( Ctvrnum != iNumRanges )
    {
        ASSERT( g_iCtvFlags != ct_NOFLAGS );
        ASSERT( Ctvrnum > iNumRanges );
        Ctvrnum = iNumRanges;
    }
    ASSERT( Ctvrnum == iNumRanges );
    return( iNumCells );
}

int CEngineCompFunc::ctexp2( int* ctbase1, int ctdesp1, int ctoptions, int* ctbase2,
                            int ctmxent2, int* ctnext2 ) {
    // save general arrays
    int  auxctmxent = CtNodemxent;
    int  auxctnext  = CtNodenext;
    int* auxctbase  = CtNodebase;

    Ct_Auxbase = ctbase1;               // non-optimized array

    CtNodemxent = ctmxent2;
    CtNodenext  = 0;
    CtNodebase  = ctbase2;              // optimized array, w/INCLUDE

    int ctres = ctcomplete( ctdesp1, ctoptions );

    ctbase2  = CtNodebase;
    *ctnext2 = CtNodenext;

    // restore general arrays
    CtNodemxent = auxctmxent;
    CtNodenext  = auxctnext;
    CtNodebase  = auxctbase;

    return( ctres );
}

void ExplicitIncludeRanges::calculateRanges(int options)
{
    includeRanges.reset();

    for( int i = 0; i < _countof(aTheFlags); ++i )
    {
        if( ( options & aTheFlags[i] ) != 0 )
            includeRanges.addRange(aTheCTValues[i]);
    }
}

int CEngineCompFunc::ctcomplete( int ctdesp, int ctoptions ) { // Complete rewrite, rcl Apr 2005

    includeRanges.calculateRanges( ctoptions );

    g_iCtvFlags = ctoptions;
    return( ctaddinclude( ctdesp ) );
}

// RHF INIC Jul 01, 2004
int CEngineCompFunc::ctadjustparent( int* pNodeBase, int iCtNode, int iDesp ) {
    CTNODE*     pCtNode = (CTNODE*) ( pNodeBase + iCtNode );
    if( pCtNode->isOperNode() ) {
        if( pCtNode->m_iCtLeft >= 0  ) {
            CTNODE* pLeftNode = (CTNODE*) (pNodeBase + pCtNode->m_iCtLeft );
            pLeftNode->m_iParentIndex += iDesp;
        }
        if( pCtNode->m_iCtRight >= 0  ) {
            CTNODE* pRightNode = (CTNODE*) (pNodeBase +  pCtNode->m_iCtRight );
            pRightNode->m_iParentIndex += iDesp;
        }

        ctadjustparent( pNodeBase, pCtNode->m_iCtLeft, iDesp );
        ctadjustparent( pNodeBase, pCtNode->m_iCtRight, iDesp );

        pCtNode->m_iCtLeft += iDesp;
        pCtNode->m_iCtRight += iDesp;
    }

    return 0;
}
// RHF END Jul 01, 2004

int CEngineCompFunc::ctaddinclude( int iCtNode ) {
    // add the INCLUDE m_iOptions to the tree of crostabb expresion
    int         p1, p2, j;
    CTNODE* pOldNode = (CTNODE*) ( Ct_Auxbase + iCtNode );

    if( pOldNode->isOperNode() ) {
        // RHF INIC Jan 24, 2003
        if( pOldNode->m_iCtLeft >= 0  ) {
            CTNODE* pLeftNode = (CTNODE*) (Ct_Auxbase + pOldNode->m_iCtLeft );
            pLeftNode->m_iParentIndex = iCtNode;
        }

        if( pOldNode->m_iCtRight >= 0  ) {
            CTNODE* pRightNode = (CTNODE*) (Ct_Auxbase + pOldNode->m_iCtRight );
            pRightNode->m_iParentIndex = iCtNode;
        }
    // RHF END Jan 24, 2003

        p1 = ctaddinclude( pOldNode->m_iCtLeft );         // Left
        p2 = ctaddinclude( pOldNode->m_iCtRight );        // Right

        // New node in CtNodebase
        p1 = ctopernode( p1, p2 , pOldNode->getOperator() );
        if( GetSyntErr() != 0 )
            return -1;
    }
    else {
        bool bRangesUpdated = false;
        ExplicitIncludeRanges backupIncludeRanges;

        p1 = CtNodenext;
        CTNODE* pNewNode = (CTNODE*) ( CtNodebase + CtNodenext );

        CtNodenext += CTNODE_SLOTS;
        if( CtNodenext >= CtNodemxent )
            return( SetSyntErr(4), -1 );

        ASSERT( pOldNode->isVarNode() );
        *pNewNode = *pOldNode; // rcl, Jul 2005

        // The logic/handling is the following:
        //  - Is variable has a valueset [and valueset has special values]
        //    then m_iFlags is updated, but m_bSpecialFlagsInserted will be false
        //  - If variable has an explicit valueset, then any m_iFlags specified
        //    with a valueset before is erased and the special values specified
        //    in the explicit valueset is copied into m_iFlags instead.
        //    Given than these special values are inserted into the ranges set
        //    m_bSpecialFlagsInserted will be true.
        //  - 4 scenarios:
        //      a.  Variable Valueset + include
        //      b.  Variable Valueset + no include
        //      c.  Variable Valueset + explicit valueset + include
        //      d.  Variable Valueset + explicit valueset + no include
        //      e.  No Variable Valueset + No explicit Valueset + include
        //  - Actions for each scenario:
        //      Scenario    Action(s)
        //        a.       Drop Valueset, consider only include specification [see e.]
        //        b.       Consider valueset specification
        //        c.       Drop Variable valueset, Do not drop explicit valueset,
        //                 Consider include specification removing from it whatever
        //                 has been already specified in explicit valueset
        //        d.       Drop Variable valueset, Do not drop explicit valueset.
        //        e.       consider only include specification [see a.]
        // rcl, Jul 2005
        if( pNewNode->m_iFlags )
        {
            int iFlagsToUse = pNewNode->m_iFlags;
            if( pNewNode->m_bSpecialFlagsInserted ) // scenarios c. & d.
            {
                int iGlobalFlags = g_iCtvFlags & ct_ALL_SPECIAL_VALUES;
                // iGlobalFlags contains include specification flags
                if( ( iFlagsToUse | iGlobalFlags ) != iGlobalFlags )
                {
                    // scenario c.
                    // turn off + dont include what is already included
                    iFlagsToUse = iGlobalFlags & ~iFlagsToUse;
                    bRangesUpdated = true;
                }
                // scenario d. No actions performed
                //             Will drop Variable valuset and will not drop
                //             explicit valueset
            }
            else
            if( g_iCtvFlags == 0 ) // scenario b.
            {
                bRangesUpdated = true;
            }

            // scenario a. bRangesUpdates == false
            //             Will drop Variable valueset
            //             consider only include specification

            if( bRangesUpdated )
            {
                // builds a new 'includeRanges' object, to use it later
                // keep original one for scenario e.
                backupIncludeRanges = includeRanges;
                includeRanges.calculateRanges( iFlagsToUse );
            }

            // scenario e. - bRangesUpdates false
            //               consider only include specification
        }


        g_iCtNodeaux = CtNodenext;

        int ini_range = iCtNode + CTNODE_SLOTS;
        int end_range = CTRANGE_SLOTS * pOldNode->m_iCtNumRanges + ini_range;

        CTRANGE* pOldRange = (CTRANGE*) ( Ct_Auxbase + ini_range );

        double rLow    = pOldRange->m_iRangeLow;
        double rHigh   = pOldRange->m_iRangeHigh;
        int iCollapsed = pOldRange->m_iRangeCollapsed;

        // Compress ranges only when variable has no decimals
        // rcl, Jun 2005
        ASSERT( pNewNode->isVarNode() );
        ASSERT( pNewNode->m_iSymbol > 0 );

        // optimize and copy the ranges from Ct_Auxbase to CtNodebase
        for( j = ini_range + CTRANGE_SLOTS; j < end_range; j += CTRANGE_SLOTS ) {
            pOldRange = (CTRANGE*) ( Ct_Auxbase + j );

            ASSERT( !isSpecialValue(pOldRange->m_iRangeLow) && !isSpecialValue(pOldRange->m_iRangeHigh));

            ctnewrange( rLow, rHigh, iCollapsed, NULL );
            if( GetSyntErr() != 0 )
                return 0;

            rLow  = pOldRange->m_iRangeLow;
            rHigh = pOldRange->m_iRangeHigh;
            iCollapsed = pOldRange->m_iRangeCollapsed;
        }

        ctnewrange( rLow, rHigh, iCollapsed, NULL );
        if( GetSyntErr() != 0 )
            return 0;

        iNumPairs = 0;
        iNumCells = 0;

        bool bStatNode = (pOldNode->m_iStatType != CTSTAT_NONE); // RHF Aug 09, 2002
        bool bTotalStat = pOldNode->m_iStatType == CTSTAT_TOTAL;
        bool bPercentStat = pOldNode->m_iStatType == CTSTAT_PERCENT; // RHF Sep 09, 2005
        bool bIncludeStat = bTotalStat || bPercentStat ||
                            pOldNode->m_iStatType == CTSTAT_FREQ;

        // RHF INIT Sep 09, 2004
        bool bPercentCollapsed = false;
        if( bPercentStat ) {
            CtStatBase* pStatBase = pOldNode->m_pStatBase;

            ASSERT( pStatBase->GetStatType() == CTSTAT_PERCENT );
            CtStatPercent*  pStatPercent=(CtStatPercent*)pStatBase;

            int     iPctType=pStatPercent->GetPctType();

            if( iPctType & CTSTAT_PERCENT_COLLAPSED ) {
                bPercentCollapsed = true;
            }
        }

        bool bCollapsed = false;

        if( bTotalStat || bPercentCollapsed )
            bCollapsed = true;
        // RHF END Sep 09, 2004

        iCollapsed = 0;

        if( bCollapsed )// RHF NEW Sep 09, 2004
            iCollapsed =  pOldRange->m_iRangeCollapsed; // RHF NEW Sep 09, 2004

        // RHF INIT Sep 14, 2004
        // Only freq stat allow special values
        bool bAddInclude = pOldNode->getNumCells() > 0 &&
                          (!bStatNode || bIncludeStat );
        // RHF END Sep 14, 2004


        // RHF COM Sep 14, 2004 if( !bStatNode ) { // RHF Aug 09, 2002
        if( bAddInclude ) { // RHF Sep 14, 2004
            for( const auto& range : includeRanges.getRanges() ) {
                if( bCollapsed ) // RHF NEW Sep 09, 2005
                    iCollapsed++;
                ctnewrange( range.low, range.high, iCollapsed, NULL );
                if( GetSyntErr() != 0 )
                    return 0;
                iNumPairs++;

                if( !bCollapsed ) // RHF NEW Sep 09, 2005
                    iNumCells += ( range.high - range.low + 1 );
                rHigh = range.high;
            }
        }

        pNewNode->addCells( iNumCells );
        pNewNode->m_iCtNumRanges += iNumPairs;
        // restore modified ranges
        if( bRangesUpdated )
        {
            includeRanges = backupIncludeRanges;
        }
    }

    return( p1 );
}

bool CEngineCompFunc::ctinranges( double rLow, double rHigh ) {
    // true if [a,b] overlaps any range
    CTRANGE* pRange;
    bool    bOverlap = false;

    for( int j = g_iCtNodeaux; j < CtNodenext && !bOverlap; j += CTRANGE_SLOTS ) {
        pRange = (CTRANGE*) ( CtNodebase + j );
        if( pRange->fitInside(rLow) ||
            pRange->fitInside(rHigh) ||
            pRange->rangeContainsMe( rLow, rHigh ) )
            bOverlap = true;
    }

    return bOverlap;
}

int CEngineCompFunc::ctopernode( int iLeft, int iRight, int iNodeType ) {

    ASSERT( iNodeType == CTNODE_OP_ADD || iNodeType == CTNODE_OP_MUL );
    // make an oper node
    CTNODE* pNode = (CTNODE*) (CtNodebase + CtNodenext );
    int iNode     = CtNodenext;

    CtNodenext += sizeof(CTNODE) / sizeof(int);
    if( CtNodenext >= CtNodemxent )
        return( SetSyntErr(4), -1 );

    double dNumCells = 0;

    if( iNodeType == CTNODE_OP_ADD )
        dNumCells = (double) ctcell( CtNodebase, iLeft ) + (double) ctcell( CtNodebase, iRight );
    else
        dNumCells = (double) ctcell( CtNodebase, iLeft ) * (double) ctcell( CtNodebase, iRight );

    if( dNumCells > MAXCELL )
        return( SetSyntErr(640), -1 );    // too many cells

    pNode->setOperNode( iNodeType );
    pNode->setNumCells( (int) dNumCells );
    pNode->m_iCtLeft = iLeft;
    pNode->m_iCtRight= iRight;
    pNode->m_iParentIndex = -1;
    pNode->m_iCoordNumber = -1;  // RHF Jun 14, 2002
    pNode->m_iSeqNumber = -1; // RHF Jun 26, 2002
    pNode->m_iStatType = CTSTAT_NONE; // RHF Jul 30, 2002
    pNode->m_pStatBase = NULL; // RHF Aug 14, 2002

    if( iLeft >= 0  ) {
        CTNODE* pLeftNode = (CTNODE*) (CtNodebase + iLeft );
        pLeftNode->m_iParentIndex = iNode;
    }

    if( iRight >= 0  ) {
        CTNODE* pRightNode = (CTNODE*) (CtNodebase + iRight );
        pRightNode->m_iParentIndex = iNode;
    }

    return( iNode );
}

bool CEngineCompFunc::ctnewrange( double rLow, double rHigh, int iCollapsed, int* piAuxNumCells ) {
    // create a new pair of ranges
    if( CtNodenext >= CtNodemxent )
        return( SetSyntErr(4), false );

    CTRANGE* pRange = (CTRANGE*) ( CtNodebase + CtNodenext );

    CtNodenext += CTRANGE_SLOTS;

    // Low and High limits in correct order? if not, fix it
    if( isSmaller(rHigh,rLow) )
    {
        double rAux = rHigh;
        rHigh = rLow;
        rLow  = rAux;
    }

    ASSERT( lowerOrEqual(rLow,rHigh) );
    pRange->m_iRangeLow  = rLow;
    pRange->m_iRangeHigh = rHigh;
    pRange->m_iRangeCollapsed = iCollapsed;

    if( iCollapsed == 1 && areDifferent(rLow,rHigh) || iCollapsed >= 2 )
        CTAB::pCurrentCtab->SetRunTimeVersion( 3 );

    if( piAuxNumCells != NULL ) {
        (*piAuxNumCells) += pRange->getNumCells();
    }

    return( true );
}

// Compile a SubTableList
// ( SubTable1, SubTable2...., SubTablen )
void CEngineCompFunc::CompileSubTablesList( CTAB* pCtab, int* pNodeBase[TBD_MAXDIM],
                                           std::vector<int>& aSubTables, bool bCheckUsed,
                                           std::vector<CLinkSubTable>* pLinkSubTables )
{
    CTAB::pCurrentCtab = pCtab;
    do {
        CompileOneSubTable( pNodeBase, aSubTables, bCheckUsed, pLinkSubTables );
        if( Tkn == TOKCOMMA )
            NextToken();
    } while( Tkn != TOKRPAREN && GetSyntErr() == 0 );

    if( Tkn == TOKRPAREN )
        NextToken();

#ifdef _DEBUG
    {
        CString     csMsg;
        CString     csLongName;
        for( int i = 0; i < (int)aSubTables.size(); i++ ) {
            int iSubTable = aSubTables[i];
            CSubTable& cSubTable=CTAB::pCurrentCtab->GetSubTable(iSubTable);

            cSubTable.GenName( m_pEngineArea, pNodeBase, &csLongName );

            csMsg.Format( _T("%d)\n%s"), i+1, csLongName.GetString() );

            //AfxMessageBox( csMsg );
        }
    }
#endif

}

// [ROW/COLUMN/LAYER] var1[(seq)] [ [*] var2[(seq)] ]
//  [
//      by [COLUMN] var1[(seq)] [ [*] var2[(seq)]]
//        [
//           by [LAYER] var1[(seq)] [ [*] var2[(seq)]]
//        ]
//  ]

// If bAllowRowColLayer=false, ROW/COLUMN/LAYER are not allowed. See CompileOneSubTableDim
void CEngineCompFunc::CompileOneSubTable( int* pNodeBase[TBD_MAXDIM],
                       std::vector<int>& aSubTables, bool bCheckUsed,
                       std::vector<CLinkSubTable>* pLinkSubTables)
{
    int iDimType[TBD_MAXDIM];
    int iVar[TBD_MAXDIM][TBD_MAXDEPTH];
    int iSeq[TBD_MAXDIM][TBD_MAXDEPTH];
    int iSpecifiedDim[TBD_MAXDIM]; // The dimension written in the application. DIM_NODIM if not used

    for( int iDim = 0; iDim < TBD_MAXDIM; iDim++ ) {
        iDimType[iDim] = DIM_NODIM;
        iSpecifiedDim[iDim] = DIM_NODIM;
        for( int iDepth=0; iDepth < TBD_MAXDEPTH; iDepth++ ) {
            iVar[iDim][iDepth] = 0;
            iSeq[iDim][iDepth] = 0;
        }
    }

    iSpecifiedDim[0] = CompileOneSubTableDim( pNodeBase, DIM_ROW, iDimType, iVar, iSeq );
    if( GetSyntErr() == 0 && Tkn == TOKBY ) {
        iSpecifiedDim[1] = CompileOneSubTableDim( pNodeBase, DIM_COL, iDimType, iVar, iSeq );
        if( GetSyntErr() == 0 && Tkn == TOKBY )
            iSpecifiedDim[2] = CompileOneSubTableDim( pNodeBase, DIM_LAYER, iDimType, iVar, iSeq );
    }

    // When there is only 1 dimension this can be DIM_SOMEDIM.
    ASSERT( iDimType[1] != DIM_SOMEDIM );
    ASSERT( iDimType[2] != DIM_SOMEDIM );

    GetSubTableList( pNodeBase, aSubTables, bCheckUsed, iDimType, iVar, iSeq, -1 );
}

void CEngineCompFunc::GetSubTableList( int* pNodeBase[TBD_MAXDIM],
                                      std::vector<int>& aSubTables, bool bCheckUsed,
                                      int   iDimType[TBD_MAXDIM],
                                      int   iVar[TBD_MAXDIM][TBD_MAXDEPTH],
                                      int   iSeq[TBD_MAXDIM][TBD_MAXDEPTH],
                                      int   iTheDimType/*=-1*/ ) {

    int     iMaxDim=(iTheDimType<0) ? TBD_MAXDIM : 1;

    if( iTheDimType >= DIM_ROW )
        ASSERT( iDimType[0] >= DIM_ROW );

    // Change unspecified  dimension
    if( GetSyntErr() == 0 && iDimType[0] == DIM_SOMEDIM ) {
        int iNumFit[TBD_MAXDIM];
        int iSymbol, iSeqNo;

        int iNumDimFit=0;
        for( int iDim=0; iDim < iMaxDim; iDim++ ) {
            iNumFit[iDim] = 0;

            for( int iSubTable = 0; iSubTable < CTAB::pCurrentCtab->GetNumSubTables(); iSubTable++ ) {
                CSubTable& cSubTable=CTAB::pCurrentCtab->GetSubTable(iSubTable);

                bool    bHasDepth[TBD_MAXDEPTH];
                for( int iDepth=0; iDepth < TBD_MAXDEPTH; iDepth++ ) {
                    bHasDepth[iDepth] = false;
                    iSymbol = iVar[0][iDepth];
                    iSeqNo = iSeq[0][iDepth];

                    if( cSubTable.m_iCtTree[iDim][iDepth] >= 0 ) {
                        CTNODE*     pNode=(CTNODE *) (pNodeBase[iDim] + cSubTable.m_iCtTree[iDim][iDepth] );

                        if( ( pNode->hasThisVariable(iSymbol) || iSymbol == -1 ) && ( iSeqNo == 0 || pNode->m_iSeqNumber == iSeqNo ) )
                            bHasDepth[iDepth] = true;
                    }
                    else if( iSymbol == -1 && iSeqNo == 0 )
                        bHasDepth[iDepth] = true;
                }

                // Match in both or match in the last depth but only 1 item was specified
                if( bHasDepth[0] && bHasDepth[1] ) { //|| !bHasDepth[0] && bHasDepth[1] && iVar[0][1] == -1 )
                    iNumFit[iDim]++;
                }

            }

            if( iNumFit[iDim] > 0 )
                iNumDimFit++;
        }

        if( iNumDimFit == 0 ) {  // No sub-tables marked
            iDimType[0] = DIM_ROW;
        }
        else if( iNumDimFit > 1 ) { // Ambiguous
            SetSyntErr(8406);
        }
        else {
            iDimType[0] =     (iNumFit[0] > 0) ? DIM_ROW :
        (iNumFit[1] > 0) ? DIM_COL:
        (iNumFit[2] > 0) ? DIM_LAYER : -1;

        ASSERT(iDimType[0]>=0);
        }
    }

   //ASSERT( iDimType[0] != DIM_SOMEDIM );
    if( !g_bMetaTable ) {
        // Change unspecified depth
        if( GetSyntErr() == 0 ) {
            std::vector<int> aAuxSubTables;
            for( int iDim=0; iDim < iMaxDim; iDim++ ) {
                int iNumSubTables=CTAB::pCurrentCtab->SearchSubTables( pNodeBase, iDimType, iVar, iSeq, aAuxSubTables, iTheDimType );

                // There is no second depth, try moving the item to depth 2
                if( iNumSubTables <= 0 && iVar[iDim][1] == -1 ) {
                    int iVarAux[TBD_MAXDIM][TBD_MAXDEPTH];
                    int iSeqAux[TBD_MAXDIM][TBD_MAXDEPTH];

                    for( int i=0; i < iMaxDim; i++ ) {
                        for( int j=0; j < TBD_MAXDEPTH; j++ ) {
                            iVarAux[i][j] = iVar[i][j];
                            iSeqAux[i][j] = iSeq[i][j];
                        }
                    }

                    // Move to other depth
                    iVarAux[iDim][1] = iVarAux[iDim][0];
                    iSeqAux[iDim][1] = iSeqAux[iDim][0];

                    iVarAux[iDim][0] = -1;
                    iSeqAux[iDim][0] = 0;

                    aAuxSubTables.clear();
                    iNumSubTables = CTAB::pCurrentCtab->SearchSubTables( pNodeBase, iDimType, iVarAux, iSeqAux, aAuxSubTables, iTheDimType );

                    bool    bSwapDepth;

                    if( g_bMetaTable )
                        bSwapDepth = (iNumSubTables >= 1);
                    else
                        bSwapDepth = (iNumSubTables == 1);

                    if( bSwapDepth ) {
                        iVar[iDim][1] = iVar[iDim][0];
                        iSeq[iDim][1] = iSeq[iDim][0];

                        iVar[iDim][0] = -1;
                        iSeq[iDim][0] = 0;

                    }
                    else if( iNumSubTables == 0 ) {
                        SetSyntErr(8411);
                    }
                }
            }
        }
    }

    if( GetSyntErr() == 0 ) {
        int     iPrevSubTables=(int)aSubTables.size();// RHF Jun 04, 2003

        // Fill cSubTable when return 1.
        int iNumSubTables=CTAB::pCurrentCtab->SearchSubTables( pNodeBase, iDimType, iVar, iSeq, aSubTables, iTheDimType );

        if( iNumSubTables == -1 ) // Invalid dimension
            SetSyntErr(8403);
        else if( iNumSubTables == -2 ) // iVar[0] not found in coordinates
            SetSyntErr(8404);
        else if( iNumSubTables == -4 ) // iVar[1] not found in coordinates
            SetSyntErr(8404);
        else if( iNumSubTables == -3 ) // iSeq[0] not found
            SetSyntErr(8405);
        else if( iNumSubTables == -5 ) // iSeq[1] not found
            SetSyntErr(8405);
        else if( iNumSubTables == 0 )
            SetSyntErr(8411);
        else if( iNumSubTables != 1 && !g_bMetaTable )
            SetSyntErr(8406);
        else {
            if( g_bMetaTable )
                ASSERT( iNumSubTables >= 1 );
            else
                ASSERT( iNumSubTables == 1 );

            for( int i=iPrevSubTables; i < (int)aSubTables.size(); i++ ) { // RHF Jun 04, 2003
            // RHF COM Jun 04, 2003 for( int i=0; i < aSubTables.GetSize(); i++ ) {
                int  iSubTable=aSubTables[i];

                CSubTable& cSubTable=CTAB::pCurrentCtab->GetSubTable(iSubTable);
                //Check is not used

                if( bCheckUsed && cSubTable.IsUsed() ) {
                    if( Issamod == ModuleType::Designer )  // SubTable already used!
                        SetSyntErr(8415);
                    else {
                        issaerror( MessageType::Abort, 8416, cSubTable.GetName().GetString() );
                    }
                }
                else
                    cSubTable.SetUsed( true );
            }
        }
    }
}

// [ROW/COLUMN/LAYER] var1[(seq)] [=ExprCat [,SeqCatNum]] [ [*] var2[(seq)] [=ExprCat [,SeqCatNum]] ]
// If aCatValues != NULL then every var can be followed by '= Cte'.
// If bAllowRowColLayer=false, ROW/COLUMN/LAYER are not allowed. See CompileOneSubTableDim
int CEngineCompFunc::CompileOneSubTableDim( int* pNodeBase[TBD_MAXDIM], int iTheDimType,
                                             int iDimType[TBD_MAXDIM], int iVar[TBD_MAXDIM][TBD_MAXDEPTH], int iSeq[TBD_MAXDIM][TBD_MAXDEPTH],
                                             int* iCatValues/*=NULL*/,
                                             int* iCatValuesSeq/*=NULL*/
                                             ) {
    int  iAuxVar[TBD_MAXDEPTH];
    int  iAuxSeq[TBD_MAXDEPTH];
    int  iSubDim;
    bool bAllowRowColLayer = false; // Change to true for allowing TOKROW/TOKCOL/TOKLAYER
    bool bAllowEqualSign = ( iCatValues !=NULL );
    int  iRet = DIM_NODIM;

    if( bAllowEqualSign )
        ASSERT( iCatValuesSeq != NULL );

    ASSERT( iTheDimType >= DIM_ROW && iTheDimType <= TBD_MAXDIM );

    for( int i=0; i < TBD_MAXDEPTH; i++ ) {
        iAuxVar[i] = -1;
        iAuxSeq[i] = 0;
    }

    if( !bAllowRowColLayer && g_bMetaTable ) { // Metatables doesn't allow ROW/COL/LAYER. When items are duplicated it's necessary to use sequence number
        iSubDim = DIM_SOMEDIM;

        if( iTheDimType >= 0 ) iSubDim = iTheDimType; // RHF Jul 15, 2002
    }
    else {
        iSubDim = (Tkn==TOKROW)    ? DIM_ROW:
                  (Tkn==TOKCOLUMN) ? DIM_COL:
                  (Tkn==TOKLAYER)  ? DIM_LAYER: DIM_SOMEDIM;
        if( iSubDim == DIM_ROW || iSubDim == DIM_COL || iSubDim == DIM_LAYER )
            iRet = iSubDim;
    }

    if( iSubDim == DIM_SOMEDIM && (iTheDimType == DIM_COL || iTheDimType == DIM_LAYER) )
        iSubDim = iTheDimType;

    // If there is COLUMN, Row dimension must have ROW or nothing
    if( iTheDimType == DIM_COL && (iDimType[0] != DIM_ROW && iDimType[0] != DIM_SOMEDIM ) )
        SetSyntErr(8407);

    // If there is LAYER, Column dimension must have COLUMN or nothing
    if( iTheDimType == DIM_LAYER && (iDimType[1] != DIM_COL && iDimType[1] != DIM_SOMEDIM ) )
        SetSyntErr(8408);

    // Column
    if( iTheDimType == DIM_COL && (iSubDim != DIM_COL && iSubDim != DIM_SOMEDIM ) )
        SetSyntErr(8408);

    if( iTheDimType == DIM_LAYER && (iSubDim != DIM_LAYER && iSubDim != DIM_SOMEDIM ) )
        SetSyntErr(8409);

    if( GetSyntErr() != 0 )
        return iRet;

    if( iTheDimType == DIM_ROW )
        iDimType[0] = iSubDim;
    else if( iTheDimType == DIM_COL ) {
        iDimType[0] = DIM_ROW;  // Change to Row
        iDimType[1] = DIM_COL;
    }
    else if( iTheDimType == DIM_LAYER ) {
        iDimType[0] = DIM_ROW;  // Change to Row
        iDimType[1] = DIM_COL;  // Change to Column
        iDimType[2] = DIM_LAYER;
    }

    for( int i=0; i < TBD_MAXDEPTH && GetSyntErr() == 0; i++ ) {
        if( Tkn != TOKVAR && Tkn != TOKVALUESET ) //Row/Column/Layer/ * // RHF Jun 06, 2003 Add Tkn != TOKVALUESET
            NextToken();

        if( !IsCurrentTokenVART(*this) && Tkn != TOKVALUESET ) // RHF Jun 06, 2003 Add Tkn != TOKVALUESET
            SetSyntErr(8402);
        else
            iAuxVar[i] = Tokstindex;

        if( GetSyntErr() == 0 ) {
            NextToken();

            //Sequence
            if( Tkn == TOKLPAREN ) {
                NextToken();
                if( Tkn != TOKCTE || ( iAuxSeq[i] = (int) Tokvalue ) != Tokvalue || iAuxSeq[i] <= 0 )
                    SetSyntErr(82);           // integer expected
                else {
                    NextToken();
                    if( Tkn != TOKRPAREN )
                        SetSyntErr(19);
                    else
                        NextToken();
                }
            }

            int iCatValueExpr = 0;
            int iSeqCatNum = 1;

            if( Tkn == TOKEQOP ) {
                if( bAllowEqualSign ) {
                    NextToken();

                    // RHF COM Jul 15, 2002 iCatValueExpr = exprlog();

                    // Consider negative values
                    // rcl, May 2005
                    bool bHasMinusSign = false;
                    if( Tkn == TOKMINUS )
                    {
                        bHasMinusSign = true;
                        NextToken();
                    }

                    // RHF INIC Jul 15, 2002
                    if( Tkn == TOKCTE ) {
                        if( (int) Tokvalue != Tokvalue )
                            SetSyntErr(82);
                        else {
                            iCatValueExpr = CreateNumericConstantNode(bHasMinusSign ? -Tokvalue : Tokvalue);
                            NextToken();
                        }
                    }
                    else if( Tkn == TOKVAR )
                        iCatValueExpr = varsanal( 'N' );
                    else
                        SetSyntErr(8432);
                    // RHF END Jul 15, 2002

                    if( GetSyntErr() == 0 && Tkn == TOKCOMMA ) {
                        NextToken();
                        if( Tkn != TOKCTE || ( iSeqCatNum = (int) Tokvalue ) != Tokvalue || iSeqCatNum <= 0 )
                            SetSyntErr(82);           // integer expected
                        else {
                            NextToken();
                        }
                    }
                }
                else
                    SetSyntErr(8430);
            }

            if( GetSyntErr() == 0 && bAllowEqualSign ) {
                iCatValues[i] = iCatValueExpr;
                iCatValuesSeq[i] = iSeqCatNum;
            }
        }

        if( GetSyntErr() != 0 )
            continue;

        if( Tkn != TOKMULOP && Tkn != TOKVAR && Tkn != TOKVALUESET ) // RHF Jul 11, 2002 Add Tkn!=TOKVAR for allowing optional '*' // RHF Jun 06, 2003 Add Tkn != TOKVALUESET
            break;
    }

    iVar[iTheDimType-1][0] = iAuxVar[0];
    iVar[iTheDimType-1][1] = iAuxVar[1];
    iSeq[iTheDimType-1][0] = iAuxSeq[0];
    iSeq[iTheDimType-1][1] = iAuxSeq[1];

    return iRet;
}

// RHF Jun 18, 2002
// SubTableList   =  SubTable [,SubTableList]
// SubTable        = SubTableDim [by SubTableDim][by SubTableDim]
// SubTableDim     = [row|column|layer] var [(seq)] [*var [(seq)]]
//
// Styles:
// no longer supported RHF Jul 15, 2005 --> UNIT (SubTableList) ( [RELATION/GROUP/RECORD/ITEM/VSET] ObjName | Select | Weight )
// UNIT (SubTableList) [RELATION/GROUP/RECORD/ITEM/VSET] ObjName | Select | Weight | TabLogicBlock ENDUNIT
// no longer supported RHF Jul 15, 2005 --> UNIT ( [RELATION/GROUP/RECORD/ITEM/VSET] ObjName | Select | Weight )
// UNIT [RELATION/GROUP/RECORD/ITEM/VSET] ObjName | Select | Weight | TabLogicBlock  ENDUNIT -> Now this select/Weight belong to the unit (not to the table)!!!
// ObjName can be a relation,group,record or item name.

// private method to reduce redundancy in CompileUnit code
void CEngineCompFunc::CompileUnit_GetNewToken( SymbolType theNewTypeToAsk, int iTokenType, int iErrorCode )
{
    NextTokenWithPreference(theNewTypeToAsk);
    if( Tkn != iTokenType )
        SetSyntErr(iErrorCode);
}

#ifdef GENCODE
 #define GETMVARNODE(x) (MVAR_NODE*) PPT(x);
 #define GETGRPNODE(x)  (GRP_NODE*)  PPT(x)
#else
 extern GRP_NODE g_GrpNode[4];
 #define GETMVARNODE(x) getLocalMVarNodePtr( iMVarNode );
 #define GETGRPNODE(x)  ((GRP_NODE*) &g_GrpNode[x])
#endif // GENCODE

static
int CheckMVarSize( MVAR_NODE* pMVarNode, int iNumDim ) // checks several conditions
                                                       // returns syntax error code
{
    int iSyntErr = 0;
    int iSubIndexNumber = pMVarNode->m_iSubindexNumber;

    if( iSubIndexNumber > 0 )
    {
        // Error detection
        //   Too many indexes provided? too little?
        //
        if( iSubIndexNumber < iNumDim - 1 )
            iSyntErr = ERROR_NOT_ENOUGH_SUBINDEXES;
        else
        {
            if( iSubIndexNumber > iNumDim )
            {
                iSyntErr = ERROR_TOO_MANY_SUBINDEXES_UNIT;
            }
            else
                if( iSubIndexNumber == iNumDim )
                {
                    if( pMVarNode->m_iVarSubindexType[iNumDim-1]
                        != MVAR_GROUP )
                        iSyntErr = ERROR_TOO_MANY_SUBINDEXES_UNIT;

                    // if last index happens to be a MVAR_GROUP
                    // we will ignore it, and we will not issue
                    // an error message
                }
        }

        // Finally check that there are no dynamic dimensions
        if( iSyntErr == 0 )
        {
            for( int i = 0; i < iSubIndexNumber; i++ )
                if( pMVarNode->m_iVarSubindexType[i] == MVAR_USE_DYNAMIC_CALC ||
                    pMVarNode->m_iVarSubindexType[i] == MVAR_GROUP )
                    iSyntErr = ERROR_NOT_ENOUGH_SUBINDEXES;
                ;
        }
    }

    return iSyntErr;
}

void TransferGroup2MVar( GRP_NODE* pGrpNode, MVAR_NODE* pMVarNode )
{
    pMVarNode->m_iSubindexNumber = pGrpNode->m_iSubindexNumber;
    pMVarNode->m_iVarIndex = pGrpNode->m_iGrpIndex;
    for( int i = 0; i < pGrpNode->m_iSubindexNumber; i++ )
    {
        pMVarNode->m_iVarSubindexExpr[i] = pGrpNode->m_iGrpSubindexExpr[i];
        pMVarNode->m_iVarSubindexType[i] = pGrpNode->m_iGrpSubindexType[i];
    }
    pMVarNode->m_iVarType = pGrpNode->m_iGrpType;
}

void CEngineCompFunc::CompileUnit( int* pNodeBase[TBD_MAXDIM] ) {
    int         iRelSymbol = 0;
    std::vector<int> aSubTables;
    bool    bAllowRowColLayer=false; // Change to true for allowing TOKROW/TOKCOL/TOKLAYER

    std::vector<CLinkSubTable> *pLinkSubTables = NULL;

    ASSERT( CTAB::pCurrentCtab->GetNumSubTables() > 0 );

    NextToken();

    bool bPendingParen = false;

    if( Tkn == TOKLPAREN ) {
        bPendingParen = true;
        NextToken();
    }

    // RHF COM Jul 13, 2005 bool bIsSubTable = bPendingParen && ( (Tkn == TOKVAR || Tkn == TOKVALUESET) && // RHF Jun 06, 2003 Add Tkn == TOKVALUESET & Add bPendingParen
    // RHF COM Jul 13, 2005     CTAB::pCurrentCtab->HasItem( pNodeBase, DIM_SOMEDIM, DIM_SOMEDEPTH, Tokstindex, 0, 0 ) );

    bool bIsSubTable = bPendingParen; // RHF Jul 13, 2005

    if( bAllowRowColLayer || !g_bMetaTable ) { // MetaTable don't allow TOKROW/COL/LAYER. They Only use var(sequence)
        // ROW/COLUMN/LAYER/VAR belonging to some coordinate expresion
        bIsSubTable = (bIsSubTable || Tkn == TOKROW || Tkn == TOKCOLUMN || Tkn == TOKLAYER);
    }

    if( bIsSubTable ) {
        if( !bPendingParen ) {
            SetSyntErr(LEFT_PARENT_EXPECTED);
            return;
        }
        bPendingParen = false;

        // Compile sub-table list for the unit and check the sub-tables have not been used before.
        CompileSubTablesList( CTAB::pCurrentCtab, pNodeBase, aSubTables, true, pLinkSubTables ); // RParen is eaten

        if( GetSyntErr() != 0 )
            return;

        // RHF COM Jul 13, 2005if( Tkn == TOKLPAREN ) {
        // RHF COM Jul 13, 2005    bPendingParen = true;
        // RHF COM Jul 13, 2005    NextToken();
        // RHF COM Jul 13, 2005}
    }

    else { // Use all sub-tables from base table when there is no sub-table specification
        for( int iSubTable=0; iSubTable < CTAB::pCurrentCtab->GetNumSubTables(); iSubTable++ ) {
            CSubTable& cSubTable = CTAB::pCurrentCtab->GetSubTable(iSubTable);

            if( cSubTable.IsUsed() ) {
                if( Issamod == ModuleType::Designer )  // SubTable already used!
                    SetSyntErr(8415);
                else {
                    issaerror( MessageType::Abort, 8416, cSubTable.GetName().GetString() );
                }

                return;
            }
            else {
                cSubTable.SetUsed( true );
            }

            aSubTables.emplace_back( iSubTable );
        }
    }

    bool  bHasRepeatingKeyword = ( Tkn == TOKKWRELATION ||
                                   Tkn == TOKKWGROUP ||
                                   Tkn == TOKKWITEM ||
                                   Tkn == TOKKWVSET ); // RHF Jun 06, 2003 Add Tkn == TOKKWVSET

    // UNIT [RELATION/GROUP/RECORD/ITEM/VSET] ObjName | Select | Weight | TagLogicBlock
    // RELATION/GROUP/RECORD/ITEM/VSET
    if( bHasRepeatingKeyword )
    {
        switch( Tkn )
        {
        case TOKKWRELATION : CompileUnit_GetNewToken( SymbolType::Relation, TOKRELATION, 33110 ); // relation' name expected
                             break;
        case TOKKWGROUP:     CompileUnit_GetNewToken( SymbolType::Group,    TOKGROUP, 33000 ); // group' name expected
                             break;
        case TOKKWITEM:      CompileUnit_GetNewToken( SymbolType::Variable, TOKVAR, 33109 ); // item' name expected);
                             break;
        case TOKKWVSET:      CompileUnit_GetNewToken( SymbolType::ValueSet, TOKVALUESET, 33115 ); // value-set name expected
                             break; // RHF Jun 06, 2003
        // superfluous default, because iUniType starts at zero.
        default:             break;
        }
    }
    //RHF COM Jul 15, 2005 else if( Tkn == TOKSELECT || Tkn == TOKWEIGHT ) {
    else if( Tkn == TOKSELECT || Tkn == TOKWEIGHT || Tkn == TOKTABLOGIC || Tkn == TOKLPAREN ) {//RHF Jul 15, 2005
        //No Object name specified assume the default?
        SetSyntErr(8401); //Repeating item, record, relation or group name expected
        return;
    }

    bool bGetNextToken=true; // RHF Jul 15, 2005
    int iLastSeenVar       = Tokstindex;
    int iLastSeenTokenType = Tkn;

    // bUnitHasExplicitDimensions -  Later [in execution] will try to complete
    //                               missing dimensions for used variables with
    //                               current unit
    //
    bool bUnitHasExplicitDimensions  = false;
    int  iMVarNode = -1;
    MVAR_NODE* pMVarNode = 0;

    switch( Tkn )
    {
    case TOKRELATION: iRelSymbol = iLastSeenVar; break;
    case TOKGROUP:    if( GPT(iLastSeenVar)->GetNumDim() == 0 ) {
                        // RHF INIT Jul 28, 2005
                          iRelSymbol = iLastSeenVar;
                          iMVarNode = grpanal( iLastSeenVar, DO_NOT_ALLOW_PARENTHESIS );
                          bGetNextToken = false; // RHF Jul 16, 2005

                          iMVarNode = -1;
                      }
                      else
                      {
                          iRelSymbol = iLastSeenVar;
                          iMVarNode = grpanal( iLastSeenVar, ALLOW_PARENTHESIS );
                          bGetNextToken = false; // RHF Jul 16, 2005
                          GRP_NODE* pGrpNode = GETGRPNODE(iMVarNode);
                          if( GetSyntErr() == 0 )
                          {
                              if( pGrpNode->m_iSubindexNumber <= 0 )
                                  iMVarNode = -1;
                              else
                              {
                                  // Copy information to a real mvar
                                  // to keep code as if its all MVAR_NODE later
                                  int iNewMVarNode = getNewMVarNodeIndex();
                                  pMVarNode = getLocalMVarNodePtr( iNewMVarNode );
                                  iMVarNode = iNewMVarNode;
                                  TransferGroup2MVar( pGrpNode, pMVarNode );

                                  int iNumDim = GPT(iLastSeenVar)->GetNumDim();
                                  SetSyntErr( CheckMVarSize( pMVarNode, iNumDim ) );

                                  if( GetSyntErr() == 0 )
                                  {
                                      if( iNumDim > 1 )
                                          bUnitHasExplicitDimensions = (iMVarNode != -1);
                                  }
                                  else
                                      iMVarNode = -1;
                              }
                          }
                      }
                      break;
    case TOKSECT:     iRelSymbol = GetRelationSymbol( iLastSeenVar ); break;
    case TOKVAR:      if( NPT_Ref(Tokstindex).IsA(SymbolType::WorkVariable) )
                          SetSyntErr(33109);            // item' name expected
                      else if( VPT(iLastSeenVar)->GetClass() == CL_SING ) // RHF Jul 04, 2004
                          // RHF COM Jul 04, 2004 else if( !VPT(Tokstindex)->IsArray() )
                          SetSyntErr(33114);            // mult-item expected
                      else
                      {
                          // JH 3/31/06 Allow alpha dict var to be unit, not just numeric
                          CSymbolVar* pSymbol = (CSymbolVar*) NPT(Tokstindex);
                          if( pSymbol->GetFmt() == 'X' ) {
                                        iMVarNode = varsanal( _T('X'), NOT_COMPLETE_COMPILATION );
                          } else {
                          iMVarNode = varsanal( _T('N'), NOT_COMPLETE_COMPILATION );
                          }
                          bGetNextToken = false; // RHF Jul 16, 2005
                          if( GetSyntErr() == 0 )
                          {
                              pMVarNode = GETMVARNODE(iMVarNode);

                              int iNumDim = VPT(iLastSeenVar)->GetNumDim();
                              if( pMVarNode->m_iSubindexNumber <= 0 )
                              {
                                  iMVarNode = -1; // destroy link to MVAR_NODE, just in case
                                  if( hasAnyGroupDim( pMVarNode, iNumDim - 1 ) )
                                      SetSyntErr(ERROR_NOT_ENOUGH_SUBINDEXES);
                              }
                              else
                              {
                                  // Error detection
                                  //   Too many indexes provided? too little?
                                  //
                                  SetSyntErr( CheckMVarSize( pMVarNode, iNumDim ) );
                              }

                              if( GetSyntErr() == 0 )
                              {
                                  iRelSymbol = GetRelationSymbol( iLastSeenVar );
                                  if( iNumDim > 1 )
                                      bUnitHasExplicitDimensions = (iMVarNode != -1);
                              }
                              else
                                  iMVarNode = -1; // destroy link to MVAR_NODE, just in case
                          }
                      }
                      break;
    case TOKVALUESET:
        {
            const ValueSet* pValueSetVar = &GetSymbolValueSet(iLastSeenVar);

            if( pValueSetVar->IsDynamic() )
                IssueError(47169, pValueSetVar->GetName().c_str());

            const VART* pVarT = pValueSetVar->GetVarT();
            int iSymbol=pVarT->GetSymbolIndex();

            if( pVarT->GetClass() == CL_SING ) // RHF Jul 04, 2004
                // RHF COM Jul 04, 2004 if( !pVarT->IsArray() )
                SetSyntErr(33114);            // mult-item expected
            else
                iRelSymbol = GetRelationSymbol( iSymbol );
        }
        break;
    default:
        SetSyntErr(8401);  // UNIT must be followed by RELATION/GROUP/RECORD/ITEM/VSET keywords
        return;
        break;
    }
    // RHF END Jun 06, 2003

    // RHF INIT Jul 29, 2005
    if( GetSyntErr() != 0 )
        return;

    int iSymForVar = MakeRelationWorkVar();

#ifdef GENCODE
    if( Flagcomp )
#endif
    {
        ASSERT( m_ForTableNext >= 0 );
        FORTABLE*   pft = &m_ForTable[m_ForTableNext++];

        pft->forVarIdx = iSymForVar ;

        ASSERT(NPT(iRelSymbol)->IsOneOf(SymbolType::Relation, SymbolType::Group));

        pft->forRelIdx = iRelSymbol;
        pft->forType = NPT(iRelSymbol)->IsA(SymbolType::Relation) ? _T('R') : _T('G');
    }
    // RHF END Jul 29, 2005

    if( bGetNextToken ) // RHF Jul 15, 2005
        NextToken();

    if( GetSyntErr() != 0 )
        return;

    // Other parameters
    bool bHasSelect  = false, bHasWeight = false, bHasTabLogic = false;
    int  iWeightExpr = 0;
    int  iSelectExpr = 0;
    int  iTabLogicExpr = 0;

    // Select,Weight,TagLogic
    while( GetSyntErr() == 0 && ( Tkn == TOKSELECT || Tkn == TOKWEIGHT || Tkn ==TOKTABLOGIC || Tkn == TOKCOMMA ) ) {
            if( Tkn == TOKCOMMA ) {
                NextToken();
                continue;
            }
            else if( Tkn == TOKSELECT ) {
                CHECK_REPETITION(bHasSelect);
                NextToken();
                iSelectExpr = exprlog();
                if( iSelectExpr < 0 ) iSelectExpr = 0; // Empty
            }
            else if( Tkn == TOKWEIGHT ) {
                CHECK_REPETITION(bHasWeight);
                NextToken();
                if( Tkn == TOKBY )
                    NextToken();
                iWeightExpr = exprlog();
                if( iWeightExpr < 0 ) iWeightExpr = 0; // Empty
            }

            // RHF INIT Jul 15, 2005
            else if( Tkn == TOKTABLOGIC ) {
                CHECK_REPETITION(bHasTabLogic);
                NextToken();

                iTabLogicExpr = instruc();

                if( iTabLogicExpr < 0 ) iTabLogicExpr = 0; // Empty

                if( GetSyntErr() == 0 && Tkn != TOKENDLOGIC ) {
                    SetSyntErr(8602);
                }

                NextToken();
            }
            // RHF END Jul 15, 2005
    }

    if( GetSyntErr() != 0 )
        return;

    if( Tkn == TOKERROR )
        SetSyntErr(INVALID_COMMENT_ERROR);
    else if( bPendingParen && Tkn != TOKRPAREN )
        SetSyntErr(19);
    else if( !bPendingParen && Tkn == TOKRPAREN )
        SetSyntErr(21);
    else if( bPendingParen && Tkn == TOKRPAREN ) {
        NextToken();
        bPendingParen = false;
    }

    if( GetSyntErr() != 0 )
        return;

    if( Flagcomp ) {
        // Generates and add the unit to the ctab object
        CtUnit    ctUnit;
        int iUnitNumber = CTAB::pCurrentCtab->GetNumUnits();

        ctUnit.SetNumber( iUnitNumber );
        ctUnit.SetOwnerCtabSymbol( CTAB::pCurrentCtab->GetSymbolIndex() );
        ctUnit.SetUnitSymbol( iRelSymbol, (iRelSymbol > 0) ? (int)NPT(iRelSymbol)->GetType() : -1 ); //
        ctUnit.SetLoopSymbol( iSymForVar ); //
        ctUnit.SetCurrentIndex(0);

        ASSERT( modeIsDeclaration() ); // RHF May 13, 2003
        // RHF COM May 13, 2003 ASSERT( g_iCtabmode == MODE_APP_DECL ); // UNIT ONLY in GLOBAL!!. If change check uses of GetSelectExpr & GetWeightExpr

        if( g_iCtabmode == MODE_APP_DECL ) {
            iSelectExpr = -iSelectExpr;
            iWeightExpr = -iWeightExpr;
            iTabLogicExpr = -iTabLogicExpr;
        }

        ctUnit.SetSelectExpr(iSelectExpr );
        ctUnit.SetWeightExpr( iWeightExpr );
        ctUnit.SetTabLogicExpr( iTabLogicExpr ); // RHF Jul 15, 2005

        // Add sub-tables to the unit
        for( int i=0; i < (int)aSubTables.size(); i++ ) {
            int         iSubTable = aSubTables[i];
            CSubTable&  cSubTable=CTAB::pCurrentCtab->GetSubTable(iSubTable);

            ASSERT( cSubTable.IsUsed() );

            ctUnit.AddSubTableIndex( iSubTable );
        }

        // Make coord number map
        ctUnit.MakeCoordNumberMap( CTAB::pCurrentCtab, pNodeBase );

        ctUnit.SetInGlobal( CTAB::pCurrentCtab->IsApplDeclared() );

        //////////////////////////////////////////////////////////////////////////
        // Complete dimensions for used variables using current unit
        // rcl, November 2004
        if( bUnitHasExplicitDimensions && iMVarNode != -1 )
        {
            switch( iLastSeenTokenType )
            {
            case TOKGROUP:
                TRACE( _T("~ CompileUnit Could complete using group [%s] info\n"),
                    GPT(iLastSeenVar)->GetName().c_str()); break;
                ASSERT( pMVarNode != 0 );
                pMVarNode->m_iVarType = MVAR_CODE;
                {
                    RELT* pRelT = RLT(iRelSymbol);
                    ASSERT( pRelT != 0 );
                    ASSERT( pRelT->GetBaseObjType() != SymbolType::Section );
                    if( pRelT != 0 )
                    {
                        int iVarIndex =
                            pMVarNode->m_iVarIndex = pRelT->GetBaseObjIndex();
                        TRACE( _T("~ CompileUnit Using variable %d [%s] info\n"),
                            iVarIndex, VPT(iVarIndex)->GetName().c_str() );
                        ctUnit.setExtraInfo( iMVarNode );
                    }
                }
                break;

            case TOKVAR:
                TRACE( _T("~ CompileUnit Could complete using var [%s] info\n"),
                    NPT(iLastSeenVar)->GetName().c_str());
                #ifdef GENCODE
                {
                    int iNewMVarNode = getNewMVarNodeIndex();
                    MVAR_NODE* pNewMVarNode = getLocalMVarNodePtr( iNewMVarNode );
                    ASSERT( pMVarNode != 0 );
                    *pNewMVarNode = *pMVarNode;
                    iMVarNode = iNewMVarNode;
                }
                #endif
                ctUnit.setExtraInfo( iMVarNode );
                break;
            default:
                TRACE( _T("~ CompileUnit Could NOT complete\n") );
                break;
            }
        }

        //////////////////////////////////////////////////////////////////////////
        CTAB::pCurrentCtab->AddUnit( ctUnit );
    }

    // RHF INIT Jul 29, 2005
#ifdef GENCODE
    if( Flagcomp )
#endif
    {
        m_ForTableNext--;
        ASSERT( m_ForTableNext >= 0 );
    }
    // RHF END Jul 29, 2005

    // RHF INIT Jul 15, 2005
    if( Tkn != TOKENDUNIT )
        SetSyntErr(8434);
    NextToken();
    // RHF END Jul 15, 2005
}

int CEngineCompFunc::GetNextStat()
{
    int iStatOption = (int)NextKeyword
    ({
        _T("FREQ"),                     // 1
        _T("TOTAL"),                    // 2
        _T("PERCENT"), _T("PCT"),       // 3,4
        _T("PROP"), _T("PROPORTION"),   // 5,6
        _T("MIN"), _T("MINIMUM"),       // 7,8
        _T("MAX"), _T("MAXIMUM"),       // 9,10
        _T("MODE"),                     // 11
        _T("MEAN"),                     // 12
        _T("MEDIAN"),                   // 13
        _T("PTILE"),                    // 14
        _T("PTILENOINT"),               // 15
        _T("STDDEV"),                   // 16
        _T("VARIANCE"),                 // 17
        _T("VPCT"),                     // 18
        _T("STDERR"),                   // 19
        _T("SAMPLINGERROR"), _T("SAMPLINGERR"), _T("SAMPERR"), _T("SAMPERROR"), // 20,21,22,23
        _T(")"), // 24
        _T("("), // 25
        _T(","), // 26
        });

    if( iStatOption == 0 ) {
        iStatOption = -1;
    }

    if( iStatOption == 24 )
        Tkn = TOKRPAREN;
    else if( iStatOption == 25 )
        Tkn = TOKLPAREN;
    else if( iStatOption == 26 )
        return GetNextStat();

    return iStatOption;
}

// Stat
//    SubTableVarSpecList1 ( StatWords )
//    [
//    , SubTableVarSpecList2 ( StatWords )
//      ....
//    , SubTableVarSpecListn ( StatWords )
//    ]

// SubTableVarSpecList: SubTableVarSpec [,SubTableVarSpecList]
// SubTableVarSpec: VarName1 [(seq)] [ (SubTableList)  | () ]
// See GetNextStat for a list of valid StatWords

void CEngineCompFunc::CompileStat( int* pNodeBase[TBD_MAXDIM] ) {
    CArray<CtStatVar,CtStatVar> aStatVar;
    bool    bAllowRowColLayer=false; // Change to true for allowing TOKROW/TOKCOL/TOKLAYER

    std::vector<CLinkSubTable> *pLinkSubTables = NULL;

    NextToken();

    int     iVar;
    int     iSeq=0;
    int     iNumMatches;
    while( GetSyntErr() == 0 ) {         // SubTableVarSpec

        // RHF INIC May 13, 2003
        if( Tkn == TOKVALUESET ) {
        }
        else
        // RHF END May 13, 2003
        if( !IsCurrentTokenVART(*this) ) {
            SetSyntErr(8500);
            continue;  // SubTableVarSpec
        }
        iVar = Tokstindex;
        iSeq = 0;
        NextToken();


        if( Tkn == TOKCOMMA ) {
            iNumMatches = CTAB::pCurrentCtab->GetStatVar( aStatVar, pNodeBase, iVar, iSeq, NULL ); // Get all nodes containing iVar/iSeq.

            if( iNumMatches == 0 ) SetSyntErr(8542);
            NextToken();
            continue;  // SubTableVarSpec
        }

        if( Tkn != TOKLPAREN ) {
            SetSyntErr(LEFT_PARENT_EXPECTED);
            continue;  // SubTableVarSpec
        }

        int     iStatOption = GetNextStat();

        bool    bGetNextStat=true, bIsSubTable=false;

        if( iStatOption < 1 ) {
            NextToken();

            // Sequence number?
            if( Tkn == TOKCTE ) {
                if( (iSeq = (int) Tokvalue ) != Tokvalue || iSeq <= 0 ) {
                    SetSyntErr(82);           // integer expected
                    continue;  // SubTableVarSpec
                }
                else {
                    NextToken();
                    if( Tkn != TOKRPAREN )
                        SetSyntErr(19);
                    else
                        NextToken();

                    if( Tkn == TOKCOMMA ) {
                        iNumMatches = CTAB::pCurrentCtab->GetStatVar( aStatVar, pNodeBase, iVar, iSeq, NULL ); // Get all nodes containing iVar/iSeq.
                        if( iNumMatches == 0 ) SetSyntErr(8542);
                        NextToken();
                        continue; // SubTableVarSpec
                    }

                    if( Tkn != TOKLPAREN ) {
                        SetSyntErr(LEFT_PARENT_EXPECTED);
                        continue; // SubTableVarSpec
                    }

                    iStatOption = GetNextStat();
                }
            }
        }

        // SubTable?
        if( iStatOption < 1 ) {
            NextToken();

            bIsSubTable = ( (Tkn == TOKVAR || Tkn == TOKVALUESET) && // RHF Jun 06, 2003 Add Tkn == TOKVALUESET
                CTAB::pCurrentCtab->HasItem( pNodeBase, DIM_SOMEDIM, DIM_SOMEDEPTH, Tokstindex, 0, 0 ) );

            bIsSubTable = (bIsSubTable || Tkn == TOKRPAREN ); // () Used when a subtable var has the same name as a stat name

            if( bAllowRowColLayer || !g_bMetaTable ) { // MetaTable don't allow TOKROW/COL/LAYER. They Only use var(sequence)
                // ROW/COLUMN/LAYER/VAR belonging to some coordinate expresion
                bIsSubTable = (bIsSubTable || Tkn == TOKROW || Tkn == TOKCOLUMN || Tkn == TOKLAYER);
            }

            if( !bIsSubTable ) {
                SetSyntErr(8501);      // invalid keyword in stats
                continue; // SubTableVarSpec
            }

            if( Tkn != TOKRPAREN ) {
                std::vector<int> aSubTables;

                // Compile subtable list for the stat
                CompileSubTablesList( CTAB::pCurrentCtab, pNodeBase, aSubTables, true, pLinkSubTables ); // RParen is eaten

                // RHF INIC Mar 12, 2003
                if( Tkn == TOKLPAREN ) {
                    iStatOption = 1;
                    bGetNextStat = true;
                }
                // RHF END Mar 12, 2003

                iNumMatches = CTAB::pCurrentCtab->GetStatVar( aStatVar, pNodeBase, iVar, iSeq, &aSubTables ); // Get all subtables containing iVar & belong to some table of aSubTables
                if( iNumMatches == 0 ) { SetSyntErr(8542); continue; }
            }
            else {
                iNumMatches = CTAB::pCurrentCtab->GetStatVar( aStatVar, pNodeBase, iVar, iSeq, NULL ); // Get all subtables containing iVar.
                if( iNumMatches == 0 ) { SetSyntErr(8542); continue; }
                NextToken();// RParen is eaten
            }

            // A New SubTableVarSpec
            if( Tkn == TOKCOMMA ) {
                NextToken();
                continue; // SubTableVarSpec
            }

            if( Tkn != TOKLPAREN ) {
                SetSyntErr(LEFT_PARENT_EXPECTED);
                continue; // SubTableVarSpec
            }
        }
        else {
            bGetNextStat = false;
            // RHF Aug 02, 2002
            iNumMatches = CTAB::pCurrentCtab->GetStatVar( aStatVar, pNodeBase, iVar, iSeq, NULL ); // Get all nodes containing iVar/iSeq.
            if( iNumMatches == 0 ) { SetSyntErr(8542); continue; }
        }

        CtStat*  pCtStat=new CtStat;
        bool    bFirst=true;
        int     p1;
        while( iStatOption >= 1 ) { // iStatOption
            if( bGetNextStat ) {
                iStatOption = GetNextStat();

                if( iStatOption < 1 ) {
                    if( bFirst )
                        SetSyntErr(8501);
                    continue;  // iStatOption
                }
            }
            bGetNextStat = true;
            bFirst = false;
            bool    bTotalOption = false; // RHF Jun 17, 2003

            switch( iStatOption ) {
            case  1:         // FREQ
                {
                    CtStatFreq*   pStatFreq=new CtStatFreq;

                    pCtStat->AddStat( pStatFreq );
                }
                break;

            case  2:         // TOTAL
                {
                                    // RHF INIC Jun 17, 2003
                    int iTotalOption=GetNextStat();

                    if( iTotalOption == 25 ) {
                        bTotalOption = true;

                        if( NextKeyword({ _T("PERCENT"), _T("PCT"), _T("%") }) == 0 ) {
                            SetSyntErr(8562);
                            iStatOption = -1;
                            continue; // iStatOption
                        }
                        else {

                        }

                    }
                    else {
                        bTotalOption = false;
                        bGetNextStat = false;
                        iStatOption = iTotalOption;
                        //continue;

                    }

                    if( !bTotalOption ) {
                    // RHF END Jun 17, 2003
                        CtStatTotal*    pStatTotal=new CtStatTotal;

                        pCtStat->AddStat( pStatTotal );
                    } // RHF Jun 17, 2003
                }
                if( !bTotalOption ) // RHF Jun 17, 2003
                break;
            case  3:         // PERCENT
            case  4:         // PCT
                {
                    int             iPctType=0;

                    NextToken();

                    if( Tkn != TOKLPAREN ) {
                        SetSyntErr(LEFT_PARENT_EXPECTED);
                        iStatOption = -1;
                        continue; // iStatOption
                    }

                    int iPctOption = (int)NextKeyword({ _T("CELL"), _T("ROW"), _T("COLUMN"), _T("TOTAL") });

                    if( iPctOption == 0 ) {
                        SetSyntErr(8532);
                        iStatOption = -1;
                        continue; // iStatOption
                    }
#ifdef _REVERSED //SAVY commented out the reversed mechanism
                    if( iPctOption == 1 )
                        iPctType |= CTSTAT_PERCENT_CELL;
                    else if( iPctOption == 2 )
                        iPctType |= CTSTAT_PERCENT_ROW;
                    else if( iPctOption == 3 )
                        iPctType |= CTSTAT_PERCENT_COLUMN;
                    else if( iPctOption == 4 )
                        iPctType |= CTSTAT_PERCENT_TOTAL;
                    else
                        ASSERT(0);
#endif
                     //SAVY REVERSED ROW /COLUMN PCT stuff in the crosstab command for trevor
                    //ROWPCT in syntax ==> COLUMNPCT when executing to get the correct result
                    //COLUMNPCT in syntax ==> ROWPCT when executing to get the correct result
                   if( iPctOption == 1 )
                        iPctType |= CTSTAT_PERCENT_CELL;
                    else if( iPctOption == 2 )
                        iPctType |= CTSTAT_PERCENT_COLUMN;//rowpct option ==> Columnpct
                    else if( iPctOption == 3 )
                        iPctType |= CTSTAT_PERCENT_ROW;//colpct option ==> rowpct
                    else if( iPctOption == 4 )
                        iPctType |= CTSTAT_PERCENT_TOTAL;
                    else
                        ASSERT(0);
                    NextToken();

                    if( Tkn == TOKLPAREN ) {
                        NextToken();
                        if( Tkn != TOKLAYER ) {
                            SetSyntErr(8534);
                            iStatOption = -1;
                            continue; // iStatOption
                        }

                        iPctType |= CTSTAT_PERCENT_LAYER;
                        NextToken();

                        if( Tkn != TOKRPAREN ) {
                            SetSyntErr(ERROR_RIGHT_PAREN_EXPECTED);
                            iStatOption = -1;
                            continue; //iStatOption
                        }

                        NextToken();
                    }

                    bool    bNeedTableSubTable=false;
                    if( Tkn == TOKCOMMA ) {
                        bNeedTableSubTable = true;
                        NextToken();
                    }

                    if( Tkn == TOKTABLE ) {
                        iPctType |= CTSTAT_PERCENT_TABLE;
                        NextToken();
                    }
                    else if( Tkn == TOKSUBTABLE ) {
                        iPctType |= CTSTAT_PERCENT_SUBTABLE;
                        NextToken();
                    }
                    else if( bNeedTableSubTable ) {
                        SetSyntErr(8536);
                        iStatOption = -1;
                        continue; // iStatOption
                    }
                    else{ // Default is SubTable
                        iPctType |= CTSTAT_PERCENT_SUBTABLE; // RHF Jan 30, 2003
                    }

                    if( Tkn != TOKRPAREN ) {
                        SetSyntErr(ERROR_RIGHT_PAREN_EXPECTED);
                        iStatOption = -1;
                        continue; //iStatOption
                    }

                    // RHF INIC Jun 17, 2003
                    if( bTotalOption ) {
                        NextToken();
                        if( Tkn != TOKRPAREN ) {
                            SetSyntErr(ERROR_RIGHT_PAREN_EXPECTED);
                            iStatOption = -1;
                            continue; //iStatOption
                        }

                        iPctType |= CTSTAT_PERCENT_COLLAPSED;
                    }
                    // RHF END Jun 17, 2003

                    CtStatPercent*  pStatPercent=new CtStatPercent;

                    pStatPercent->SetPctType( iPctType );

                    pCtStat->AddStat( pStatPercent );
                }
                break;
            case  5:         // PROP  ( [ [FREQ] [,] [%|PERCENT|PCT] ] )  (range-list)
            case  6:         // PROPORTION
                {
                    int     iPropType=0;

                    NextToken();

                    if( Tkn == TOKLPAREN ) {
                        bool    bRangeList=false;
                        bool    bFirst=true;
                        bool    bHasPropTotal=false;
                        bool    bHasPropPct=false;

                        while( GetSyntErr() == 0 ) {
                            int iPropOption = (int)NextKeyword({ _T("FREQ"), _T("PERCENT"), _T("PCT"), _T("%") });

                            if( iPropOption == 0 )
                            {
                                NextToken();

                                if( bFirst && Tkn == TOKCTE ) {
                                    bRangeList = true;
                                    break;
                                }

                                if( bFirst && Tkn == TOKCOMMA ) {
                                    SetSyntErr(8538);
                                    break;
                                }

                                if( Tkn == TOKRPAREN )
                                    break;
                            }

                            bFirst = false;

                            if( iPropOption == 1 ) {
                                if( bHasPropTotal ) {
                                    SetSyntErr(8544);
                                    break;
                                }
                                iPropType |= CTSTAT_PROP_TOTAL;
                                bHasPropTotal = true;
                            }
                            else if( iPropOption == 2 || iPropOption == 3 || iPropOption == 4 ) {
                                if( bHasPropPct ) {
                                    SetSyntErr(8544);
                                    break;
                                }
                                iPropType |= CTSTAT_PROP_PERCENT;
                                bHasPropPct = true;
                            }
                        }

                        if( GetSyntErr() != 0 ) {
                            iStatOption = -1;
                            continue; // iStatOption
                        }

                        if( !bRangeList ) {
                            if( Tkn != TOKRPAREN ) { // ')'
                                SetSyntErr(ERROR_RIGHT_PAREN_EXPECTED);
                                iStatOption = -1;
                                continue; // iStatOption
                            }

                            NextToken();
                        }
                    }

                    if( Tkn != TOKLPAREN ) {
                        SetSyntErr(LEFT_PARENT_EXPECTED);
                        iStatOption = -1;
                        continue; // iStatOption
                    }

                    NextToken();

                    CompilationExtendedInformation::InCrosstabInformation in_crosstab_information;
                    const std::vector<Range<double>>& ranges = in_crosstab_information.ranges;
                    const std::vector<bool>& implicit_highs = in_crosstab_information.implicit_highs;

                    p1 = CompileInNodes(DataType::Numeric, &in_crosstab_information);

                    CtStatProp*  pStatProp = new CtStatProp;

                    for( size_t i = 0; i < ranges.size(); i++ )
                    {
                        if( ranges[i].low != (int)ranges[i].low || ranges[i].high != (int)ranges[i].high )
                        {
                            SetSyntErr(82);
                            iStatOption = -1;
                            continue; // iStatOption
                        }

                        CSubRange cSubRange(ranges[i].low, ranges[i].high, implicit_highs[i], 0);
                        pStatProp->AddRange( cSubRange );
                    }

                    pStatProp->SetPropType( iPropType );

                    if( Tkn != TOKRPAREN ) {
                        SetSyntErr(ERROR_RIGHT_PAREN_EXPECTED);
                        iStatOption = -1;
                        continue; //iStatOption
                    }

                    pCtStat->AddStat( pStatProp );
                }

                break;
            case  7:         // MIN
            case  8:         // MINIMUM
                {
                    CtStatMin*  pStatMin=new CtStatMin;

                    pCtStat->AddStat( pStatMin );
                }
                break;
            case  9:         // MAX
            case  10:         // MAXIMUM
                {
                    CtStatMax*  pStatMax=new CtStatMax;

                    pCtStat->AddStat( pStatMax );
                }

                break;
            case  11:        // MODE
                {
                    CtStatMode*  pStatMode= new CtStatMode;

                    pCtStat->AddStat( pStatMode );
                }

                break;
            case  12:        // MEAN
                {
                    CtStatMean*  pStatMean= new CtStatMean;

                    pCtStat->AddStat( pStatMean );
                }
                break;
            case  13:        // MEDIAN
                {
                    CtStatMedian*  pStatMedian= new CtStatMedian;

                    // RHF COM Sep 13, 2005 // mean [( [continuos/discrete] [intervals( ... ) ] ) ]
                    // median [ (lower/upper) ]
                    int iMedianOption=GetNextStat();
                    int iMedianType = 0; // Default old behavior

                    // Has parameters
                    if( iMedianOption == 25 ) {

                        size_t median_type = NextKeyword({ _T("CONTINUOUS"), _T("DISCRETE"), _T("INTERVALS") });

                        if( median_type == 0 ) {
                            SetSyntErr(8546);
                            iStatOption = -1;
                            continue; // iStatOption
                        }
                        else {
                            if( median_type == 1 ) {
                                iMedianType = 1; // CONTINUOUS
                                NextToken();
                            }
                            else if( median_type == 2 ) {
                                iMedianType = 2;  //DISCRETE
                                NextToken();
                            }
                            else if( median_type == 3 )
                                Tkn = TOKINTERVAL;
                            else
                                ASSERT(0);
                        }

                        if( Tkn == TOKINTERVAL ) {
                            CompIntervals( NULL, &pStatMedian->GetIntervals() );

                            if( GetSyntErr() != 0 ) {
                                iStatOption = -1;
                                continue; //iStatOption
                            }

                            if( Tkn != TOKRPAREN ) {
                                SetSyntErr(ERROR_RIGHT_PAREN_EXPECTED);
                                iStatOption = -1;
                                continue; //iStatOption
                            }
                            NextToken();
                        }

                        if( Tkn != TOKRPAREN ) {
                            SetSyntErr(ERROR_RIGHT_PAREN_EXPECTED);
                            iStatOption = -1;
                            continue; //iStatOption
                        }
                    }
                    else {
                        bGetNextStat = false;
                        iStatOption = iMedianOption;
                        //continue;
                    }

                    pStatMedian->SetMedianType( iMedianType );

                    pCtStat->AddStat( pStatMedian );
                }

                break;
            case  14:        // PTILE, PTILENOINT
            case  15:
                {
                    CtStatPTile*  pStatPTile= new CtStatPTile;

                    NextToken();
                    if( Tkn != TOKCTE ) {
                        SetSyntErr(8540);
                        iStatOption = -1;
                        continue; //iStatOption
                    }

                    int     iNumTiles = (int) Tokvalue;

                    if( iNumTiles != Tokvalue || iNumTiles < 2 || iNumTiles > 10 ) {
                        SetSyntErr(8540);
                        iStatOption = -1;
                        continue; //iStatOption
                    }

                    pStatPTile->SetNumTiles( iNumTiles );
                    pStatPTile->SetInterPol( (iStatOption == 14) ? true : false ); // PTILE interpol, PTILENOINT no interpol

                    // RHF INIT Sep 13, 2005 Only for PTILE
                    if( iStatOption == 14 ) {
                        // ptile n [ (lower/upper) ]
                        int iPTileOption=GetNextStat();
                        int iPTileType = 0; // Default old behavior

                        // Has parameters
                        if( iPTileOption == 25 ) {
                            // RHF INIT Sep 12, 2005
                            size_t ptile_type = NextKeyword({ _T("LOWER"), _T("UPPER"), _T( "INTERVALS") });

                            if( ptile_type == 0 ) {
                                SetSyntErr(8546);
                                iStatOption = -1;
                                continue; // iStatOption
                            }
                            else {
                                if( ptile_type == 1 ) {
                                    iPTileType = 2;  // LOWER <--> discrete
                                    NextToken();
                                }
                                else if( ptile_type == 2 ) {
                                    iPTileType = 1;   //UPPER <--> continuous
                                    NextToken();
                                }
                                else if( ptile_type == 3 ) { // JH 5/30/06 added intervals for ptiles
                                    Tkn = TOKINTERVAL;
                                }
                                else
                                    ASSERT(0);
                            }

                            // JH 5/30/06 added intervals for ptiles
                            if( Tkn == TOKINTERVAL ) {
                                CompIntervals( NULL, &pStatPTile->GetIntervals() );

                                if( GetSyntErr() != 0 ) {
                                    iStatOption = -1;
                                    continue; //iStatOption
                                }

                                if( Tkn != TOKRPAREN ) {
                                    SetSyntErr(ERROR_RIGHT_PAREN_EXPECTED);
                                    iStatOption = -1;
                                    continue; //iStatOption
                                }
                                NextToken();
                           }
                        // end JH 5/30/06

                            if( Tkn != TOKRPAREN ) {
                                SetSyntErr(ERROR_RIGHT_PAREN_EXPECTED);
                                iStatOption = -1;
                                continue; //iStatOption
                            }
                        }
                        else {
                            bGetNextStat = false;
                            iStatOption = iPTileOption;
                            //continue;
                        }

                        pStatPTile->SetPTileType( iPTileType );
                    }
                  // RHF END Sep 13, 2005

                    pCtStat->AddStat( pStatPTile );
                }
                break;
            case  16:        // STDDEV
                {
                    CtStatStdDev*  pStatStdDev= new CtStatStdDev;

                    pCtStat->AddStat( pStatStdDev );
                }

                break;
            case  17:        // VARIANCE
                {
                    CtStatVariance*  pStatVariance=new CtStatVariance;

                    pCtStat->AddStat( pStatVariance );
                }

                break;
            case  18:        // VPCT
                {
                    SetSyntErr(8501);
                    iStatOption = -1;
                }

                break;
            case  19:        // STDERR
                {
                    SetSyntErr(8501); // RHF Jul 11, 2005
                    iStatOption = -1; // RHF Jul 11, 2005

                    // RHF COM Jul 11, 2005 CtStatStdErr*  pStatStdErr=new CtStatStdErr;
                    // RHF COM Jul 11, 2005  pCtStat->AddStat( pStatStdErr );
                }
                break;

            case 20:         // SAMPLINGERROR
            case 21:         // SAMPLINGERR
            case 22:         // SAMPERR
            case 23:         // SAMPERROR
                SetSyntErr(8501);
                iStatOption = -1;
                break;
            case 24: // )
                iStatOption = -1;
                break;
            default:
                SetSyntErr(8501);
                iStatOption = -1;
                break;
            }
        }

        if( GetSyntErr() != 0 ) {
            delete pCtStat;
            continue; // SubTableVarSpec
        }

        // Here we have all ready for 1 stat
        if( true || Flagcomp ) {
            // Generates and add the Stat object to the ctab object
            int iStatNumber=CTAB::pCurrentCtab->GetNumStats();

            pCtStat->SetNumber( iStatNumber );
            pCtStat->SetOwnerCtabSymbol( CTAB::pCurrentCtab->GetSymbolIndex() );

            // Add StatVar to Stat object
            for( int i=0; i < aStatVar.GetSize(); i++ ) {
                CtStatVar&  ctStatVar=aStatVar.ElementAt(i);

                if( ctStatVar.GetHasOverlappedCat() ) {
                    int     iSymVar=ctStatVar.GetSymVar();
                    ASSERT( iSymVar > 0 );

                    issaerror( MessageType::Warning, 8720, NPT(iSymVar)->GetName().c_str(), CTAB::pCurrentCtab->GetName().c_str()); // GHM 20111125 added table name to this warning message about overlapping ranges
                }
                // ASSERT( aStatVar.IsUsed() );

                pCtStat->AddStatVar( ctStatVar );
            }

            pCtStat->SetInGlobal( CTAB::pCurrentCtab->IsApplDeclared() );

            CTAB::pCurrentCtab->AddStat( pCtStat );
        }

        aStatVar.RemoveAll();

        if( Tkn != TOKRPAREN ) {
            SetSyntErr(ERROR_RIGHT_PAREN_EXPECTED);
            continue; // SubTableVarSpec
        }
        NextToken();

        if( Tkn == TOKCOMMA ) NextToken(); // RHF Mar 12, 2003

        if( Tkn != TOKVAR && Tkn != TOKVALUESET) // RHF Jun 06, 2003
            break; // SubTableVarSpec
    }
}


bool CEngineCompFunc::ScanTables()
{
    // the goal of this function is to get the crosstab names into the symbol table so that they're valid by the time that
    // CEngineCompFunc::CreateProcDirectory is called; also, to read whether or not the level is changed, so that the
    // level is correct for setting in CEngineCompFunc::CompileSymbolProcs
    std::shared_ptr<CTabSet> pTabSet = m_pEngineDriver->GetApplication()->GetTabSpec();

    // go through each of the basic tokens
    SetSourceBuffer(Appl.m_AppTknSource);

    cs::span<const Logic::BasicToken> basic_tokens = GetBasicTokensSpan();

    std::shared_ptr<CTAB> last_compiled_crosstab;
    size_t token_index_of_crosstab_name = SIZE_MAX;
    size_t token_index_of_level_name = SIZE_MAX;

    for( size_t token_index = 0; token_index < basic_tokens.size(); ++token_index )
    {
        const Logic::BasicToken& basic_token = basic_tokens[token_index];

        if( token_index == token_index_of_crosstab_name )
        {
            // make sure that the name is valid
            try
            {
                CheckIfValidNewSymbolName(basic_token.GetText());

                last_compiled_crosstab = std::make_shared<CTAB>(basic_token.GetText());
                last_compiled_crosstab->SetPreDeclared(true);
                last_compiled_crosstab->SetNodeExpr(0, 0);

                // Default level is 1. Can be changed with LEVEL clause
                int iCtabSetLevel = 1;

                if( pTabSet != NULL )
                {
                    int iCtabLevel = -1;

                    if( pTabSet->SearchTable(WS2CS(last_compiled_crosstab->GetName()), iCtabLevel) != NULL )
                        iCtabSetLevel = iCtabLevel + 1;
                }

                last_compiled_crosstab->SetTableLevel(iCtabSetLevel);

                m_engineData->AddSymbol(last_compiled_crosstab);
            }

            catch(...)
            {
                return false;
            }
        }

        else if( last_compiled_crosstab != nullptr && token_index == token_index_of_level_name )
        {
            int iGroupSym = m_pEngineArea->SymbolTableSearch(basic_token.GetText(), { SymbolType::Group });

            if( iGroupSym > 0 && m_pEngineArea->IsLevel(iGroupSym) )
            {
                GROUPT* pLevelGroupT = GPT(iGroupSym);

                if( pLevelGroupT->GetLevel() > 0 )
                    last_compiled_crosstab->SetTableLevel(pLevelGroupT->GetLevel());
            }
        }

        else if( SO::EqualsNoCase(basic_token.GetTextSV(), _T("CROSSTAB")) )
        {
            token_index_of_crosstab_name = token_index + 1;

            // if specified, skip past the type, which will be either: sint, lint, float(number)
            if( ( token_index + 1 ) < basic_tokens.size() )
            {
                const Logic::BasicToken& next_basic_token = basic_tokens[token_index + 1];

                if( SO::EqualsOneOfNoCase(next_basic_token.GetTextSV(), _T("SINT"), _T("LINT")) )
                {
                    ++token_index_of_crosstab_name;
                }

                else if( SO::EqualsNoCase(next_basic_token.GetTextSV(), _T("FLOAT")) )
                {
                    token_index_of_crosstab_name += 4;
                }
            }
        }

        else if( SO::EqualsNoCase(basic_token.GetTextSV(), _T("LEVEL")) )
        {
            token_index_of_level_name = token_index + 2;
        }
    }

    return true;
}

// RHF INIC Jan 23, 2003
bool CEngineCompFunc::CheckProcTables() {
    bool    bRet=true;
    bool    bDesigner = ( Issamod == ModuleType::Designer );

    for( CTAB* pCtab : m_engineData->crosstabs ) {
        if( GetProcDirectoryEntry(pCtab->GetSymbolIndex()) != nullptr && pCtab->GetTableLevel() <= 0 ) {
            issaerror( bDesigner ? MessageType::Error : MessageType::Abort, 690, pCtab->GetName().c_str() );
            bRet = false;
        }
    }

    return bRet;
}
// RHF END Jan 23, 2003


bool CEngineCompFunc::StopTableScan() {
    bool    bStopScan = ( // RHF COM Mar 10, 2003 Tkn == TOKTABLE    ||
        Tkn == TOKSTABLE   || // RHF 7/11/96
        Tkn == TOKHOTDECK  ||
        Tkn == TOKMEAN     ||
        Tkn == TOKSMEAN    ||
        Tkn == TOKKWFUNCTION ||
        Tkn == TOKEND );
    if( !bStopScan )
        bStopScan = ( Tkn == TOKCROSSTAB || Tkn == TOKKWCTAB || Tkn == TOKLEVEL );

    return bStopScan;
}


//////////////////////////////////////////////////////////////////////////
// CTRANGE methods

int CTRANGE::getNumCells( double rLow, double rHigh, int iCollapsed )
{
    int iThisNumCells = 0;
    if( iCollapsed == 0 ) // No collapse
    {
        if( areEqual(rHigh,rLow) )
            iThisNumCells = 1;
        else
        {
            ASSERT( ( !isSpecialValue(rLow) && !isSpecialValue(rHigh) ) ||
                    ( isSpecialValue(rLow)  && isSpecialValue(rHigh)) );
            if( isSpecialValue(rLow) )
            {
                ASSERT( specialValue2Index(rHigh) > specialValue2Index(rLow) );
                iThisNumCells = specialValue2Index(rHigh) - specialValue2Index(rLow) + 1;
            }
            else
                iThisNumCells = (int) rHigh - (int) rLow + 1;
        }
    }
    else if( iCollapsed == 1 ) // Only the first collapsed range count a cell
        iThisNumCells = 1;
    else // Do nothing with the rest collapsed ranges
        iThisNumCells = 0; // superfluous

    return iThisNumCells;
}

int CTRANGE::getNumCells()
{
    return getNumCells( m_iRangeLow, m_iRangeHigh, m_iRangeCollapsed );
}

bool CTRANGE::fitInside( double rValue )
{
    return greaterOrEqual(rValue,m_iRangeLow) && lowerOrEqual(rValue,m_iRangeHigh);
}

bool CTRANGE::rangeContainsMe( double rLow, double rHigh )
{
    return isSmaller(rLow,m_iRangeLow) && isBigger(rHigh,m_iRangeHigh);
}
