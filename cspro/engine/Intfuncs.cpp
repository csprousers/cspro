//-------------------------------------------------------------------
//
// INTFUNCS.cpp  executes functions (system and user-functions)
//
//  History:    Date       Author   Comment
//              ---------------------------
//              01 Jun 99   RHF     Basic conversion
//              16 May 01   vc ex     Expanding for 3D driver
//
//-------------------------------------------------------------------

#include "StandardSystemIncludes.h"
#include "Interpreter.h"
#include "Engine.h"
#include "VariableIterator.h"
#include <zEngineO/AllSymbols.h>
#include <zEngineO/Interpreter/SelectDlgHelper.h>
#include <zEngineO/Messages/EngineMessages.h>
#include <zEngineO/Nodes/File.h>
#include <zPlatformO/PlatformInterface.h>
#include <zToolsO/DirectoryLister.h>
#include <zToolsO/FileIO.h>
#include <zToolsO/NewlineSubstitutor.h>
#include <zToolsO/TextConverter.h>
#include <zToolsO/TextEncoding.h>
#include <zToolsO/Utf8.h>
#include <zToolsO/VarFuncs.h>
#include <zUtilO/PathHelpers.h>
#include <zUtilO/PortableFileSystem.h>
#include <zUtilF/KeyboardLoader.h>
#include <zMessageO/MessageFile.h>
#include <zDictO/ValueProcessor.h>
#include <zDictO/ValueSetResponse.h>
#include <zFormO/FormFile.h>
#include <zFormO/Roster.h>
#include <zIssaLib/CsDriver.h>
#include <zBridgeO/NPff.h>
#include <zCapiO/SelectDlg.h>
#include <zCaseO/Case.h>
#include <zConcatO/Concatenator.h>
#include <zConcatO/ConcatenatorReporter.h>
#include <zParadataO/Logger.h>
#include <zEngineO/EngineCaseConstructionReporter.h>
#include <CSEntry/UWM.h>


#ifdef WIN_DESKTOP
#define RTRACE TRACE
#else
#define RTRACE(...)
#endif

#ifndef WIN_DESKTOP
#include <sys/stat.h>
#endif


int CIntDriver::calculateLimitsForGroup( double doubleArray[], int iSymGroup )
{
    int intArray[DIM_MAXDIM];

    for( int i = 0; i < DIM_MAXDIM; i++ )
        intArray[i] = (int) doubleArray[i];

    return calculateLimitsForGroup( intArray, iSymGroup );
}

int CIntDriver::calculateLimitsForGroup( int indexArray[], int iSymGroup )
{
    // count the number of continuous non zero indexes
    int iFixedDimensions = 0;
    while( iFixedDimensions < DIM_MAXDIM && indexArray[iFixedDimensions] != 0 )
        ++iFixedDimensions;

    int aFixedIndexes[DIM_MAXDIM];

    int iLimit = 0;

    // any one?
    if( iFixedDimensions > 0 )
    {
        GROUPT* pGroupT = GPT(iSymGroup);
        ASSERT( pGroupT != 0 );

        // int iNumDim = pGroupT->GetNumDim();
        int aCurrentOccIndexes[DIM_MAXDIM];
        memset( aCurrentOccIndexes, 0, sizeof(int) * DIM_MAXDIM );

        for( int i = 0; i < iFixedDimensions; i++ )
        {
            int iSpecIdx = indexArray[i];
            aFixedIndexes[i] = iSpecIdx;
            aCurrentOccIndexes[i] = iSpecIdx - 1;
        }

        iLimit = pGroupT->GetCurrentOccurrences( aCurrentOccIndexes );
    }
    else
    {
        // old way to calculate limit
        iLimit = GetTrueGroupOccs( iSymGroup );
    }

    m_iExFixedDimensions = iFixedDimensions;
    m_aFixedDimensions.setIndexes( aFixedIndexes );

    return iLimit;
}

int CIntDriver::calculateLimitsForGroupNode( GRP_NODE* pgrpNode, int iSymGroup )
{
    // How many indexes has been specified in group node?
    UserIndexesArray specifiedIndexes;
    grpGetSubindexes( pgrpNode, specifiedIndexes );

    int iLimit = calculateLimitsForGroup( specifiedIndexes, iSymGroup );
    return iLimit;
}

double CIntDriver::excount( int icount ) {
    FN2_NODE*   pcount    = (FN2_NODE*) (PPT(icount));

    /* RHF COM INIC Dec 26, 2003
    int         iSymGroup = pcount->sect_ind;
    ASSERT( iSymGroup > 0 );
    GROUPT*     pGroupT   = GPT(iSymGroup);
    int         iLimit    = GetTrueGroupOccs( iSymGroup );// victor Dec 10, 01
    int         iCount    = 0;

    m_iExDim = pGroupT->GetNumDim()-1; // RHF Aug 17, 2000
    RHF COM END Dec 26, 2003 */

    // RHF INIC Dec 26, 2003
    int         iSym = pcount->sect_ind;

    ASSERT( iSym != 0 );
    int iCount = 0;

    int iLimit = -1;

    int iOldExFixedDimensions = -1;

    // RTODO:
    //   Consider information inside GRP_NODE and
    //   MVAR_NODE about dimensions specified.
    //
    switch( pcount->count_type )
    {
    case COUNT_GROUP:
        {
            // Used to be:
            //    iSym had group info, and
            //    GROUPT*  pGroupT = GPT(iSym);
            //    iLimit   = GetTrueGroupOccs( iSym );

            // now:

            // FN2_NODE will contain
            //
            //   count_type   == COUNT_GROUP (already checked)
            //   sect_ind     == index to a GRP_NODE node

            // get group info from GRP_NODE node
            //   m_iGrpType   == GROUP_CODE  (to be checked)
            //   m_iGrpIndex  == group symbol index
            //   (a,b,c) expression saved in
            //      m_iGrpSubindexType[DIM_MAXDIM]; // subindex type: 'e', 'g'
            //      m_iGrpSubindexExpr[DIM_MAXDIM];
            //   number of indexes in
            //   m_iIndexNumber

            GRP_NODE* pgrpNode = (GRP_NODE*) ( PPT(iSym) );
            ASSERT( pgrpNode->m_iGrpType == GROUP_CODE );

            // iSym must contain group index, so
            iSym = pgrpNode->m_iGrpIndex;

            GROUPT* pGroupT  = GPT(iSym);
            m_iExDim = pGroupT->GetNumDim()-1;

            iOldExFixedDimensions = m_iExFixedDimensions; // 20130109 m_iExFixedDimensions is changed in calculateLimitsForGroupNode and should be changed back
            iLimit = calculateLimitsForGroupNode( pgrpNode, iSym );
        }

        break;
    case COUNT_SECTION:
        {
            SECT*    pSecT  = SPT(iSym);

                iLimit = exsoccurs( pSecT );
            m_iExDim = 0;
            iSym = -iSym;
        }

        break;
    case COUNT_VAR:
        {
            GROUPT* pGroupT = 0;
            int iSymGroup   = 0;

            if( iSym < 0 )
            {
                VART* pVarT  = VPT(-iSym);

                iSymGroup = pVarT->GetParentGroup();
                pGroupT   = GPT(iSymGroup);
                RTRACE("Counting VAR: [%d, %s]\n", -iSym, pVarT->GetName().c_str());
            }
            else
            {
                // could be considered directly as a group
                iSymGroup = iSym;
                pGroupT   = GPT(iSymGroup);
                RTRACE("Counting GROUP (VAR was specified in program) [%d,%s]\n", iSym, pGroupT->GetName().c_str());
            }
            iLimit = GetTrueGroupOccs( iSymGroup );
            m_iExDim  = pGroupT->GetNumDim()-1;
        }

        break;
    default:
        ASSERT(0);
        break;
    }

    m_iExGroup =  iSym;

    // RHF END Dec 26, 2003


    double dValueX=0;
    RTRACE( _T(" { -- looping from 1 to %d --- \n"), iLimit );
    for( /*RHF COM INIC Dec 26, 2003m_iExGroup=iSymGroup,RHF COM END Dec 26, 2003 */
        m_iExOccur = 1; m_iExOccur <= iLimit; m_iExOccur++ ) { // RHF Aug 17, 2000 Add m_iExGroup
            // fn_exp < 0: empty 'where expression,
            //       >= 0: 'where' expression has to be evaluated

            if( pcount->fn_exp >= 0 )
            {
                dValueX = evalexpr( pcount->fn_exp ); // RHF Apr 23, 2002
                RTRACE( _T("  [m_iExOccur:%d] - %d\n"), m_iExOccur, (int) dValueX );
            }
            if( pcount->fn_exp < 0 || IsTrue( dValueX ) ) { // RHF Apr 23, 2002 Fix bug in count where ff()
                iCount++;
                RTRACE( _T("  [m_iExOccur:%d] - count++ %d %s\n"),
                    m_iExOccur, iCount, pcount->fn_exp < 0 ? _T("*") : _T("") );
            }
    }
    RTRACE( _T(" } /* end count loop */") );
    m_iExOccur = 0; // RHF Aug 17, 2000

    if( iOldExFixedDimensions >= 0 ) // 20130109
        m_iExFixedDimensions = iOldExFixedDimensions;

    return( (double) iCount );
}

// given a mvar node calculates the max limit the
// group should use.
//
int CIntDriver::calculateLimitForVarNode( MVAR_NODE* pMVAR, int iSymGroup )
{
    UserIndexesArray specifiedIndexes;
    mvarGetSubindexes( pMVAR, specifiedIndexes );

    int iLimit = calculateLimitsForGroup( specifiedIndexes, iSymGroup );

    return iLimit;
}

// Helper macros
// CALCULATE_LIMITS(), SPECIFY_LAST_DIMENSION() and RESTORE_DEFAULT()
//
// rcl, Jul 26, 2004
#define CALCULATE_LIMITS( iLimit, pNode, iSymGroup ) \
    int iLimit = calculateLimitForVarNode( pNode, iSymGroup ); \
    m_iExDim   = GPT(iSymGroup)->GetNumDim() - 1; \
    m_iExGroup = iSymGroup; \
    int iValueToSave = pNode->m_iVarSubindexExpr[m_iExDim]; \
    int iTypeToSave = pNode->m_iVarSubindexType[m_iExDim]

#define SPECIFY_LAST_DIMENSION( pMVAR ) \
    m_iExFixedDimensions--; \
    pMVAR->m_iVarSubindexExpr[m_iExDim] = m_iExOccur; \
    pMVAR->m_iVarSubindexType[m_iExDim] = MVAR_CONSTANT

// RESTORE_DEFAULT() made with changes from
//    RHF Aug 17, 2000
//    rcl Jul 26, 2004
#define RESTORE_DEFAULT() \
    pMVAR->m_iVarSubindexExpr[m_iExDim] = iValueToSave; \
    pMVAR->m_iVarSubindexType[m_iExDim] = iTypeToSave; \
    m_iExOccur = 0; \
    m_iExFixedDimensions = 0;

double CIntDriver::exsum( int isum ) {
    FN3_NODE*  psum  = (FN3_NODE*) PPT(isum);
    MVAR_NODE* pMVAR = (MVAR_NODE*) PPT(psum->iSymVar);

    CALCULATE_LIMITS( iLimit, pMVAR, psum->sect_ind );

    double      dSumValue = 0;
    double      dValueX=0;
    //Added by Savy (R) 20090825
    SPECIFY_LAST_DIMENSION( pMVAR );

    for( m_iExOccur = 1; m_iExOccur <= iLimit; m_iExOccur++ ) { // RHF Aug 17, 2000 Add m_iExGroup
        // fn_exp < 0: empty 'where expression,
        //       >= 0: 'where' expression has to be evaluated

        if( psum->fn_exp >= 0 ) dValueX = evalexpr( psum->fn_exp );// RHF Apr 23, 2002
        if( psum->fn_exp < 0 || IsTrue( dValueX ) ) {

            SPECIFY_LAST_DIMENSION( pMVAR );

            double  dValue = exmvar( pMVAR ); // exmvarvalue( pVarX, (double) m_iExOccur );

            if( !IsSpecial(dValue) )
                dSumValue += dValue;
        }
    }

    RESTORE_DEFAULT();

    return dSumValue;
}

double CIntDriver::exmin( int imin ) {              // rebuilt      // victor Jul 30, 99
    FN3_NODE*  pmin  = (FN3_NODE*) (PPT(imin));
    MVAR_NODE* pMVAR = (MVAR_NODE*) (PPT(pmin->iSymVar));

    CALCULATE_LIMITS( iLimit, pMVAR, pmin->sect_ind );

    double      dMinValue = DEFAULT;
    double      dValueX=0;
    //Added by Savy (R) 20090825
    SPECIFY_LAST_DIMENSION( pMVAR );

    for( m_iExOccur = 1; m_iExOccur <= iLimit; m_iExOccur++ ) { // RHF Aug 17, 2000 Add m_iExGroup
        // fn_exp < 0: empty 'where expression,
        //       >= 0: 'where' expression has to be evaluated

        if( pmin->fn_exp >= 0 ) dValueX = evalexpr( pmin->fn_exp ); // RHF Apr 23, 2002
        if( pmin->fn_exp < 0 || IsTrue( dValueX ) ) {

            SPECIFY_LAST_DIMENSION( pMVAR );
            double  dValue = exmvar( pMVAR ); // mvarvalue( pVarX, (double) m_iExOccur );

            if( !IsSpecial(dValue) && dValue < dMinValue )
                dMinValue = dValue;
        }
    }
    RESTORE_DEFAULT();

    return dMinValue;
}

double CIntDriver::exmax( int imax ) {              // rebuilt      // victor Jul 30, 99
    FN3_NODE*  pmax  = (FN3_NODE*) (PPT(imax));
    MVAR_NODE* pMVAR = (MVAR_NODE*) (PPT(pmax->iSymVar));

    CALCULATE_LIMITS( iLimit, pMVAR, pmax->sect_ind );

    double      dMaxValue = - DEFAULT;
    double      dValueX=0;
    //Added by Savy (R) 20090825
    SPECIFY_LAST_DIMENSION( pMVAR );

    for( m_iExOccur = 1; m_iExOccur <= iLimit; m_iExOccur++ ) { // RHF Aug 17, 2000 Add m_iExGroup
        // fn_exp < 0: empty 'where expression,
        //       >= 0: 'where' expression has to be evaluated

        if( pmax->fn_exp >= 0 ) dValueX = evalexpr( pmax->fn_exp );// RHF Apr 23, 2002
        if( pmax->fn_exp < 0 || IsTrue( dValueX ) ) {

            SPECIFY_LAST_DIMENSION( pMVAR );
            double  dValue = exmvar( pMVAR ); // mvarvalue( pVarX, (double) m_iExOccur );

            if( !IsSpecial(dValue) && dValue > dMaxValue )
                dMaxValue = dValue;
        }
    }
    RESTORE_DEFAULT();

    if( dMaxValue == - DEFAULT )
        dMaxValue = DEFAULT;

    return dMaxValue;
}

double CIntDriver::exavrge( int iavrge ) {          // rebuilt      // victor Jul 30, 99
    FN3_NODE*  pavrge = (FN3_NODE*) (PPT(iavrge));
    MVAR_NODE* pMVAR  = (MVAR_NODE*) (PPT(pavrge->iSymVar));

    CALCULATE_LIMITS( iLimit, pMVAR, pavrge->sect_ind );

    int         iCount    = 0;
    double      dAverageValue = 0;

    double      dValueX=0;

    //Added by GHM 20090827
    SPECIFY_LAST_DIMENSION( pMVAR );

    for( m_iExOccur = 1; m_iExOccur <= iLimit; m_iExOccur++ ) { // RHF Aug 17, 2000 Add m_iExGroup
        // fn_exp < 0: empty 'where expression,
        //       >= 0: 'where' expression has to be evaluated

        if( pavrge->fn_exp >= 0 ) dValueX = evalexpr( pavrge->fn_exp ); // RHF Apr 23, 2002
        if( pavrge->fn_exp < 0 || IsTrue( dValueX ) ) {

            SPECIFY_LAST_DIMENSION( pMVAR );
            double  dValue = exmvar(pMVAR); // mvarvalue( pVarX, (double) m_iExOccur );

            if( !IsSpecial(dValue) ) {
                iCount++;
                dAverageValue += dValue;
            }
        }
    }
    RESTORE_DEFAULT();

    if( iCount )
        dAverageValue /= iCount;
    else
        dAverageValue = DEFAULT;

    return dAverageValue;
}



double CIntDriver::exseek(int iseek) // 20100602
{
    FN3_NODE * pSeek = (FN3_NODE *)(PPT(iseek));
    int* pSeekStart = (int*)((char *)pSeek + sizeof(*pSeek));

    MVAR_NODE * pMVAR = (MVAR_NODE *)PPT(pSeek->iSymVar);
    CALCULATE_LIMITS(iLimit,pMVAR,pSeek->sect_ind);

    SPECIFY_LAST_DIMENSION(pMVAR);

    m_iExOccur = 1;
    int desiredCase = 1; // 20110811

    if( *pSeekStart ) // the user is specifying a start value
    {
        if( *pSeekStart >= 0 )
        {
            m_iExOccur = (int) evalexpr(*pSeekStart);

            if( m_iExOccur < 1 || m_iExOccur > iLimit )
            {
                RESTORE_DEFAULT(); // 20100720
                return 0;
            }
        }

        else // 20110811 adding the ability to search the 2nd, 3rd, etc. case
            desiredCase = (int) evalexpr(-1 * *pSeekStart);
    }

    for( ; m_iExOccur <= iLimit; m_iExOccur++ )
    {
        if( EvaluateConditional(pSeek->fn_exp) )
        {
            if( --desiredCase == 0 )
            {
                double foundValue = m_iExOccur;
                RESTORE_DEFAULT();
                return foundValue;
            }
        }
    }

    RESTORE_DEFAULT();
    return 0;
}


double CIntDriver::exseekMinMax(int iseek) // 20130119
{
    FN3_NODE * pSeek = (FN3_NODE *)(PPT(iseek));
    int* pSeekStart = (int*)((char *)pSeek + sizeof(*pSeek));

    bool bSeekingMin = pSeek->fn_code == FNSEEKMIN_CODE;

    MVAR_NODE * pMVAR = (MVAR_NODE *)PPT(pSeek->iSymVar);
    CALCULATE_LIMITS(iLimit,pMVAR,pSeek->sect_ind);

    SPECIFY_LAST_DIMENSION(pMVAR);

    int seekExtremeVal = 0;
    std::optional<double> extremeVal;

    m_iExOccur = 1;

    if( *pSeekStart ) // the user is specifying a start value
    {
        m_iExOccur = (int) evalexpr(*pSeekStart);

        if( m_iExOccur < 1 || m_iExOccur > iLimit )
        {
            RESTORE_DEFAULT(); // 20100720
            return 0;
        }
    }

    for( ; m_iExOccur <= iLimit; m_iExOccur++ )
    {
        if( EvaluateConditional(pSeek->fn_exp) )
        {
            SPECIFY_LAST_DIMENSION(pMVAR);
            double thisVal = exmvar(pMVAR);

            if( !IsSpecial(thisVal) && ( !extremeVal.has_value() || ( bSeekingMin && ( thisVal < extremeVal ) ) || ( !bSeekingMin && ( thisVal > extremeVal ) ) ) )
            {
                seekExtremeVal = m_iExOccur;
                extremeVal = thisVal;
            }
        }
    }

    RESTORE_DEFAULT();
    return seekExtremeVal;
}




double CIntDriver::exnoccurs( int ioccurs ) {           // victor Jul 30, 99
    FN4_NODE*   poccurs    = (FN4_NODE*) (PPT(ioccurs));
    int         iSymGroup  = poccurs->sect_ind;
    int         iNumOccurs = GetTrueGroupOccs( iSymGroup );

    return( (double) iNumOccurs );
}

// RHF INIC May 14, 2003
int CIntDriver::exsoccurs(const SECT* pSecT, bool use_rules_for_binary_dict_items/* = false*/) {
    // RHF INIC Feb 19, 2002
    bool bLikeBatch = ( Issamod != ModuleType::Entry );
    int iGroupNum = 0;
    int iSymGroup;
    const GROUPT* pGroupT;
    int iNumOccs = 0;

    while( (pGroupT = pSecT->GetGroup(iGroupNum)) != NULL ) {

        // Don't use item & subitem level groups - only record level groups.
        // If we have an item/subitem then walk up the parents until we get to a record.
        // This fixes problems where a record has items with occurrences that create
        // subgroups and soccurs gives the number of occurrences of the subgroup instead
        // of the record.
        while( pGroupT != NULL && ( pGroupT->GetDimType() == CDimension::Item || pGroupT->GetDimType() == CDimension::SubItem ) )
            pGroupT = pGroupT->GetParentGPT();

        if( pGroupT != NULL ) {
            // RHF INIC Nov 15, 2002
            if( pSecT->IsOccGenerator() ) {
                iNumOccs = std::max( iNumOccs, pGroupT->GetTotalOccurrences() );
            }
            // RHF END Nov 15, 2002
            else {
                bool    bUseThisGroup=(bLikeBatch || pGroupT->GetSource() == GROUPT::Source::FrmFile ); // Group from formfile

                if( bUseThisGroup ) {
                    iSymGroup = pGroupT->GetSymbol();
                    iNumOccs = std::max( iNumOccs, GetTrueGroupOccs(iSymGroup, use_rules_for_binary_dict_items) );
                }
            }
        }

        iGroupNum++;
    }
    // RHF END Feb 19, 2002

    iNumOccs = std::min( iNumOccs, pSecT->GetMaxOccs() ); // RHF Jul 21, 2003

//    int docc = pSecT->GetSecX()->GetOccurrences();
    return iNumOccs;

}
// RHF END May 14, 2003

int CIntDriver::GetTrueGroupOccs( int iSymGroup, bool use_rules_for_binary_dict_items/* = false*/) {     // victor Dec 10, 01
    ASSERT( iSymGroup > 0 );
    GROUPT* pGroupT = GPT(iSymGroup);
    int     iNumOccurs = 0;             // remark - different for ENTRY/BATCH

    ASSERT( pGroupT->GetSymbol() == iSymGroup );
    ASSERT( pGroupT->GetSymbol() == pGroupT->GetSymbolIndex() );

    if( Issamod != ModuleType::Entry || pGroupT->GetSource() == GROUPT::Source::DcfFile ) {
        iNumOccurs = pGroupT->GetTotalOccurrences();
    }
    else {
        iNumOccurs = pGroupT->GetCurrentOccurrences();
        if( IsUsing3D_Driver() && iNumOccurs >= 1 ) {

            //////////////////////////////////////////////////////////////////////////
            // RTODO: Eliminar la logica superflua de mas abajo
            //////////////////////////////////////////////////////////////////////////

            // RTODO RCL: Parece superfluo calcular lo que esta siendo pasado como parametro
            int     iSymGroup2 = pGroupT->GetSymbol();

            // RTODO RCL: compruebo que lo cuestionado 2 lineas mas arriba es cierto.
            ASSERT( iSymGroup2 == iSymGroup );

            int     iRefOccur = ( pGroupT->GetNumDim() ) ? iNumOccurs : 0;

            // setup the reference, installing the 3-D indexes
            C3DObject   o3DReference;

            GetCsDriver()->PassTo3D( &o3DReference, iSymGroup, iRefOccur ); // TRANSITION-TO-3D

            //////////////////////////////////////////////////////////////////////////

            int iHighOcc = 0;
            try {
                pGroupT->CalcCurrent3DObject();
                iHighOcc = GetCsDriver()->FindActualOccs( pGroupT->GetCurrent3DObject() );
            }
            catch( const C3DException& ) {
                iNumOccurs = 0;

                if( use_rules_for_binary_dict_items ) {
                    // who knows why these various rules exist, but they don't work when called from exsoccurs using a
                    // singly-occurring record that is on a group with items from other records
                    if( pGroupT->GetDimType() == CDimension::VoidDim ) {
                        iNumOccurs = pGroupT->GetCurrentOccurrences();

                        if( iNumOccurs == 1 ) {
                            // prevent the overwriting below
                            iHighOcc = -1;
                        }
                    }
                }
            }

            if( iHighOcc >= 0 )         // transfer only when an equivalent HTOcc(bwd) found
                iNumOccurs = iHighOcc;
        }
    }

    return iNumOccurs;
}


double CIntDriver::exdeckarray(int iExpr) // 20100121 for getdeck and putdeck
{
    const DECK_ARRAY_NODE* deck_array_node = (DECK_ARRAY_NODE*)PPT(iExpr);
    LogicArray& logic_array = GetSymbolLogicArray(deck_array_node->array_symbol_index);
    const std::vector<int>& deck_array_symbols = logic_array.GetDeckArraySymbols();
    std::vector<size_t> indices;

    // evaluate each dimension
    for( size_t i = 0; i < logic_array.GetNumberDimensions(); ++i )
    {
        int index = -1;
        bool dimension_uses_spillover_row = ( deck_array_symbols[i] < 0 );

        // if the dimension isn't based on a value set, then the index must be supplied
        if( deck_array_symbols[i] == 0 )
        {
            index = Evaluate<int>(deck_array_node->index_expressions[i]);
        }

        else
        {
            const ValueSet& value_set = GetSymbolValueSet(abs(deck_array_symbols[i]));
            VART* pVarT = value_set.GetVarT();
            VARX* pVarX = pVarT->GetVarX();
            double numeric_value = 0;
            SharableString string_value;

            // if a value was provided, it will override whatever is in the variable
            if( deck_array_node->index_expressions[i] >= 0 )
            {
                if( pVarT->IsNumeric() )
                {
                    numeric_value = evalexpr(deck_array_node->index_expressions[i]);
                }

                else
                {
                    string_value = Evaluate<SharableString>(deck_array_node->index_expressions[i]);
                }
            }

            // otherwise we have to evaluate the current value in the variable
            else
            {
                // single item on a singly occurring record (don't care about index)
                if( !pVarT->IsArray() )
                {
                    if( pVarT->IsNumeric() )
                    {
                        numeric_value = svarvalue(pVarT->GetVarX());
                    }

                    else
                    {
                        string_value = UTF8_TODO::GetUtf8(pVarX->GetValue());
                    }
                }

                // items requiring an index
                else
                {
                    GROUPT* pGroupT = pVarT->GetParentGPT();

                    if( pGroupT == nullptr )
                        pGroupT = pVarT->GetOwnerGPT();

                    pGroupT->CalcCurrent3DObjectEx();

                    // this returns a one-based index, which must be converted to zero-based
                    C3DIndexes theCurrentIndexes = pGroupT->GetCurrent3DObject().getIndexes();
                    int zero_based_indices[DIM_MAXDIM];

                    for( int j = 0; j < DIM_MAXDIM; j++ )
                        zero_based_indices[j] = std::max(theCurrentIndexes.getIndexValue(j) - 1, 0);

                    C3DIndexes fixedIndex(ZERO_BASED, zero_based_indices);

                    if( pVarT->IsNumeric() )
                    {
                        numeric_value = GetMultVarFloatValue(pVarT, fixedIndex);
                    }

                    else
                    {
                        string_value = UTF8_TODO::GetUtf8(pVarX->GetValue(fixedIndex));
                    }
                }
            }

            // now figure out the array index values from the value set
            const ValueProcessor& value_processor = value_set.GetValueProcessor();
            const DictValue* const dict_value = pVarT->IsNumeric()
                ? value_processor.GetDictValue(numeric_value)
                : value_processor.GetDictValue(*string_value);

            // we need to map the value to an index number
            if( dict_value != nullptr )
            {
                const auto& map_search = m_deckarrayIndexMappings.find(dict_value);

                if( map_search != m_deckarrayIndexMappings.end() )
                {
                    index = map_search->second;
                }

                else
                {
                    const DictValueSet& dict_value_set = value_set.GetDictValueSet();
                    int j = 1;

                    for( const auto& this_dict_value : dict_value_set.GetValues() )
                    {
                        if( dict_value == &this_dict_value )
                        {
                            index = j;
                            break;
                        }

                        ++j;
                    }

                    m_deckarrayIndexMappings.try_emplace(dict_value, index);
                }
            }
        }

        // check for a valid index
        if( index < 0 || index >= (int)logic_array.GetDimension(i) )
        {
            // if there is a spillover row, we can use its index
            if( dimension_uses_spillover_row )
            {
                index = logic_array.GetDimension(i) - 1;
            }

            else
            {
                return DEFAULT;
            }
        }

        indices.emplace_back(index);
    }

    if( deck_array_node->fn_code == FNGETDECK_CODE )
    {
        return logic_array.GetValue<double>(indices);
    }

    else // putdeck
    {
        // update the main cell
        const double putdeck_value = Evaluate<double>(deck_array_node->putdeck_value_expression);
        logic_array.SetValue(indices, putdeck_value);

        // potentially update the spillover rows
        if( deck_array_node->update_spillover_rows == 1 )
        {
            std::vector<size_t> spillover_indices = indices;

            std::function<void()> update_if_different_indices =
                [&]()
                {
                    for( size_t i = 0; i < indices.size(); i++ )
                    {
                        if( indices[i] != spillover_indices[i] )
                        {
                            logic_array.SetValue(spillover_indices, putdeck_value);
                            return;
                        }
                    }
                };

            std::function<void(size_t)> spillover_rows_updater =
                [&](size_t dimension_updating)
                {
                    // quit if no more dimensions
                    if( dimension_updating == logic_array.GetNumberDimensions() )
                        return;

                    // update with the current index if a previous dimension has a spillover index change
                    update_if_different_indices();

                    // continue on to the next dimensions with the current index
                    spillover_rows_updater(dimension_updating + 1);

                    // process this dimension if it has a spillover row and that wasn't the initially updated value
                    size_t spillover_row_index = logic_array.GetDimension(dimension_updating) - 1;

                    if( deck_array_symbols[dimension_updating] < 0 && spillover_indices[dimension_updating] != spillover_row_index )
                    {
                        size_t saved_index = spillover_indices[dimension_updating];
                        spillover_indices[dimension_updating] = spillover_row_index;

                        // update with the modified index
                        update_if_different_indices();

                        // continue on to the next dimensions with the modified index
                        spillover_rows_updater(dimension_updating + 1);

                        // restore the original index
                        spillover_indices[dimension_updating] = saved_index;
                    }
                };

            spillover_rows_updater(0);
        }

        return 1; // successful putdeck operation
    }
}


double CIntDriver::exdemode(int /*iExpr*/)
{
    if( Issamod == ModuleType::Entry )
        return assert_cast<CEntryDriver*>(m_pEngineDriver)->dedemode();

    return DEFAULT;
}


// exhighlight: returns true if field is highlighted (green in PathOn, any color otherwise)
double CIntDriver::exhighlight(int iExpr)
{
    if( Issamod != ModuleType::Entry )
        return 1;

    FNH_NODE* pFun = (FNH_NODE*)PPT(iExpr);

    VART* pVarT = NULL;
    VARX* pVarX = NULL;
    CNDIndexes* pTheIndex = NULL;
    int iOccur = 1;

    if( pFun->occ_exp == -2 ) // -2 indicates that the variable information was compiled using varsanal
    {
        MVAR_NODE* pMVarNode = (MVAR_NODE*)PPT(pFun->isymb);

        pVarT = VPT(pMVarNode->m_iVarIndex);
        pVarX = pVarT->GetVarX();

        if( pMVarNode->m_iVarType == MVAR_CODE )
        {
            UserIndexesArray dIndex;
            int aIndex[DIM_MAXDIM];

            mvarGetSubindexes(pMVarNode,dIndex);

            if( !pVarX->RemapIndexes(aIndex,dIndex,CHECK_TOTAL) )
                return 0;

            pTheIndex = new CNDIndexes(ZERO_BASED,aIndex);
        }
    }

    else // the non-varsanal highlight (< 7.0)
    {
        pVarT = VPT(pFun->isymb);
        pVarX = pVarT->GetVarX();

        if( pFun->occ_exp >= 0 )
        {
            iOccur = Evaluate<int>(pFun->occ_exp);

            if( iOccur < 1 || iOccur > pVarT->GetFullNumOccs(true) )
                return 0;
        }
    }

    if( pTheIndex == NULL )
    {
        pTheIndex = new CNDIndexes(ZERO_BASED);
        pVarX->PassTheOnlyIndex(*pTheIndex,iOccur);
    }

    int iColor = GetFieldColor(pVarX,*pTheIndex);

    delete pTheIndex;

    bool bLight = false;

    if( m_pEngineSettings->IsPathOn() )
        bLight = ( iColor == FLAG_HIGHLIGHT );

    else
        bLight = ( iColor != FLAG_NOLIGHT );

    return bLight ? 1 : 0;
}


double CIntDriver::exinadvance(int iExpr)
{
    bool is_advancing = false;

    if( Issamod == ModuleType::Entry )
    {
        // the same check used by the warning function
        is_advancing = ( m_pCsDriver->GetSourceOfNodeAdvance() >= 0 ) ||
                       ( m_pCsDriver->GetNumOfPendingAdvances() > 0 );
    }

    return is_advancing ? 1 : 0;
}


double CIntDriver::exvisualvalue(int iExpr)
{
    const auto& fnh_node = GetNode<FNH_NODE>(iExpr);
    ASSERT(fnh_node.occ_exp == -2); // -2 indicates that the variable information was compiled using varsanal

    const MVAR_NODE* pMVarNode = &GetNode<MVAR_NODE>(fnh_node.isymb);
    VART* pVarT = VPT(pMVarNode->m_iVarIndex);
    VARX* pVarX = pVarT->GetVarX();

    if( pMVarNode->m_iVarType == MVAR_CODE )
    {
        UserIndexesArray dIndex;
        int aIndex[DIM_MAXDIM];

        mvarGetSubindexes(pMVarNode, dIndex);

        if( pVarX->RemapIndexes(aIndex, dIndex, CHECK_TOTAL) )
        {
            CNDIndexes theIndex(ZERO_BASED, aIndex);
            return GetMultVarFloatValue(pVarX, theIndex);
        }

        else
        {
            return DEFAULT;
        }
    }

    else
    {
        return GetSingVarFloatValue(pVarX);
    }
}


// Three local functions defined to simplify the CIntDriver::ExCurTotOcc code below
int CIntDriver::GetCurOccFromGroup(const GROUPT* pGroupT, bool bUseBatchLogic)
{
    const int iCurrentOcc = pGroupT->GetCurrentExOccurrence();

    return ( bUseBatchLogic && iCurrentOcc == 0 ) ? pGroupT->GetDataOccurrences() :
                                                    iCurrentOcc;
}

static double GetCurOccFromChildrenGroups( const SECT* pSecT, bool bUseBatchLogic )
{
    double dOcc = 0;
    double dNewOcc;
    const GROUPT* pGroupT = 0;

    for( int iGroupNum=0; (pGroupT = pSecT->GetGroup(iGroupNum)) != 0 ; iGroupNum++ )
    {
        // RHF INIC Aug 04, 2005
        bool    bHiddenGroup = (pGroupT->GetSource() == GROUPT::Source::DcfFile);

        if( /*!bUseBatchLogic &&*/ bHiddenGroup )
            continue;
        // RHF END Aug 04, 2005

        dNewOcc = CIntDriver::GetCurOccFromGroup( pGroupT, bUseBatchLogic );
        dOcc = std::max( dOcc, dNewOcc );
    }

    return dOcc;
}

static double GetTotOccsFromChildrenGroups( SECT* pSecT, bool bUseBatchLogic )
{
    double dOcc = 0;
    double dNewOcc;
    GROUPT* pGroupT = 0;

    for( int iGroupNum = 0; (pGroupT = pSecT->GetGroup(iGroupNum)) != 0; iGroupNum++ )
    {
        // RHF INIC Aug 04, 2005
        bool    bHiddenGroup = (pGroupT->GetSource() == GROUPT::Source::DcfFile);

        if( /*!bUseBatchLogic &&*/ bHiddenGroup )
            continue;
        // RHF END Aug 04, 2005

        // Walk up chain of group parents to get to record level group so that
        // get number of occurrences from repeating item on record.
        while (pGroupT != NULL && pGroupT->GetDimType() != CDimension::Record) {
            pGroupT = pGroupT->GetParentGPT();
        }

        if (pGroupT != NULL) {
            dNewOcc = pGroupT->GetDataOccurrences();
            dOcc = std::max( dOcc, dNewOcc );
        }
    }

    return dOcc;
}

// RHF INIC Oct 30, 2000

// Signature changed to make debugging easier
// RCL, May 2004
double CIntDriver::ExCurTotOcc( FNGR_NODE * pNode, bool bCurOcc )
{
    if( m_iExSymbol <= 0 )
        return DEFAULT;

    Symbol* pSymbol = NPT(m_iExSymbol);
    GROUPT*     pGroupT=NULL;
    double      dOcc=0; // the result of this method goes here

    if( pNode->m_iArgumentType == FNGR_Spec::ARG_NO_ARGUMENTS || pNode->fn_arg < 0 ) // call without arguments
    {
        if( pSymbol->IsA(SymbolType::Group) )
            pGroupT = (GROUPT*) pSymbol;
        else if( pSymbol->IsA(SymbolType::Variable) ) {
            pGroupT = ( (VART*)pSymbol)->GetParentGPT();
            if( pGroupT == NULL ) // Single var use owner
                pGroupT = ( (VART*)pSymbol)->GetOwnerGPT();
            else
                ASSERT( pGroupT == ( (VART*)pSymbol)->GetOwnerGPT() );
        }
        // RHF INIC Aug 29, 2005
        else if( pSymbol->IsA(SymbolType::Crosstab) ) {
            ;
        }
        else if( pSymbol->IsA(SymbolType::Block) ) {
            pGroupT = assert_cast<const EngineBlock*>(pSymbol)->GetGroupT();
        }
        // RHF END Aug 29, 2005
        else  {
            ASSERT(0);
            return 0;
        }

        // Check if there is a foreach open
        if( bCurOcc ){
            if( pNode->m_iSym > 0 ) {
                int iWVar = pNode->m_iSym;

                dOcc = GetSymbolWorkVariable(iWVar).GetValue();
            }
            else {
                int iCurrentOcc = pGroupT ? pGroupT->GetCurrentExOccurrence() : 0;

                if( m_iExOccur > 0 ) iCurrentOcc = m_iExOccur; // RHF Apr 30, 2002

                dOcc = iCurrentOcc;
            }
        }
        else {
            // iForVar represent a GROUP for TotOcc without argument that is located in a foreach
            if( pNode->m_iSym > 0 ) // Recall groupt
                pGroupT = (GROUPT*) GPT(pNode->m_iSym); // This is the Foreach group

            if( pGroupT ) {
                try
                {
                    pGroupT->CalcCurrent3DObjectEx();
                    dOcc = pGroupT->GetDataOccurrences( pGroupT->GetCurrent3DObject().getIndexes() );
                }
                catch( const C3DException& )
                {
                    // errors? -> do it the old way
                    dOcc = pGroupT->GetDataOccurrences();
                }
            }
        }
    }
    else  // call with arguments
    {
        bool bUseBatchLogic = (Issamod != ModuleType::Entry);

        if( pNode->m_iArgumentType == FNGR_Spec::ARG_GROUP /*|| pNode->fn_arg > 0*/ )
        {
            GRP_NODE*  pgrpNode = (GRP_NODE*) ( PPT(pNode->fn_arg) );

            pGroupT = GPT(pgrpNode->m_iGrpIndex);

            //No occ tree double      dIndex[DIM_MAXDIM];
            //No occ treegrpGetSubindexes( pgrpNode, dIndex );
            //No occ treedOcc = dIndex[0];

            if( bCurOcc )
            {
                dOcc = GetCurOccFromGroup( pGroupT, bUseBatchLogic );
            }
            else // TotOcc
            {
                //dOcc = pGroupT->GetDataOccurrences();
                ///Savy 05/18/2012
                //Savy to fix the totocc for 3d. if the record occurs and item occurs then
                // the totocc(item000). where item000 is the group generated by the ord/forms
                //will come out incorrectly. to fix this problem use the Serpro's 3d indexes.
                //however the totocc(item) is fine as this is not a group. this is to fix
                 //the totoccs for the "item group"
                if( pGroupT ) {
                    try
                    {
                        pGroupT->CalcCurrent3DObjectEx();
                        dOcc = pGroupT->GetDataOccurrences( pGroupT->GetCurrent3DObject().getIndexes() );
                    }
                    catch( const C3DException& )
                    {
                        // errors? -> do it the old way
                        dOcc = pGroupT->GetDataOccurrences();
                    }
                }
            }
        }
        else
        {
            if( pNode->m_iArgumentType == FNGR_Spec::ARG_SECTION )
            {
                SECT*   pSecT=SPT(pNode->m_iSym);

                if( bCurOcc )
                {
                    dOcc = GetCurOccFromChildrenGroups( pSecT, bUseBatchLogic );
                }
                else // TotOcc
                {
                    dOcc = GetTotOccsFromChildrenGroups( pSecT, bUseBatchLogic );
                }
            }
            else if( pNode->m_iArgumentType == FNGR_Spec::ARG_MULTIPLE_VAR )
            {
                MVAR_NODE* pMVAR = (MVAR_NODE*) PPT(pNode->fn_arg);
                VART* pVarT = VPT(pMVAR->m_iVarIndex);

                pGroupT = pVarT->GetParentGPT();

                if( bCurOcc )
                {
                    dOcc = GetCurOccFromGroup( pGroupT, bUseBatchLogic );
                }
                else // TotOcc
                {
                    ASSERT( pGroupT != 0 );
                    int aIndex[DIM_MAXDIM];

                    UserIndexesArray specifiedIndexes;
                    mvarGetSubindexes( pMVAR, specifiedIndexes );

                    // User should have said
                    //    totocc( var( i ) )
                    // using one less index than what a var should use.
                    // We will complete last dimension to prevent an
                    // assertion inside RemapIndexes() method

                    int iNumDim = pVarT->GetNumDim();
                    if( (int) specifiedIndexes[iNumDim-1] == 0 )
                        specifiedIndexes[iNumDim-1] = 1.0;

                    bool bOk = false;

                    if( areUserIndexesValid( specifiedIndexes, iNumDim ) )
                    {
                        // we need to convert user specified indexes to
                        // 0-based indexes. Will use VarX's RemapIndexes method.
                        // RemapIndexes returns false when an error is detected
                        VARX* pVarX = pVarT->GetVarX();
                        if( pVarX->RemapIndexes( aIndex, specifiedIndexes ) == true )
                        {
                            CNDIndexes theIndex( ZERO_BASED, aIndex );
                            dOcc = pGroupT->GetDataOccurrences( theIndex );
                            bOk = true;
                        }
                    }

                    // finally, if something went wrong use last valid position
                    // (usually data occ)
                    if( !bOk )
                        dOcc = pGroupT->GetDataOccurrences();
                }
            }
            else if( pNode->m_iArgumentType == FNGR_Spec::ARG_WHERE_MV ) // 20091028 for count/sum/etc.(where .... statements)
            {
                VART* pVarT = VPT(pNode->fn_arg);
                pGroupT = pVarT->GetParentGPT();
                dOcc = GetCurOccFromGroup( pGroupT, bUseBatchLogic );
            }
            else if( pNode->m_iArgumentType == FNGR_Spec::ARG_WHERE_GROUP ) // 20091028 for count/sum/etc.(where .... statements)
            {
                pGroupT = GPT(pNode->fn_arg);
                dOcc = GetCurOccFromGroup( pGroupT, bUseBatchLogic );
            }
            else
                ASSERT(0);
        }
    }

    return dOcc;

}

double CIntDriver::exfncurocc(int iExpr) {
    FNGR_NODE * pNode = (FNGR_NODE*) PPT( iExpr );

    return ExCurTotOcc( pNode, true );
}

double CIntDriver::exfntotocc(int iExpr) {
    FNGR_NODE * pNode = (FNGR_NODE*) PPT( iExpr );

    return ExCurTotOcc( pNode, false );
}
// RHF END Oct 30, 2000


double CIntDriver::exispartial(int iExpr)
{
    const auto& fn8_node = GetNode<FN8_NODE>(iExpr);
    const Symbol* symbol = ( fn8_node.symbol_index != -1 ) ? NPT(fn8_node.symbol_index) : nullptr;

    // ENGINECR_TODO(ispartial) get the input dictionary if a EngineDictionary
    if( symbol != nullptr && symbol->IsA(SymbolType::Dictionary) )
    {
        const EngineDictionary* engine_dictionary = assert_cast<const EngineDictionary*>(symbol);
        const Case& data_case = engine_dictionary->GetEngineCase().GetCase();
        return data_case.IsPartial();
    }

    else
    {
        // no argument means the input dictionary
        const DICX* pDicX = ( fn8_node.symbol_index == -1 ) ? DIX(0) :
                                                              DPX(fn8_node.symbol_index);

        return pDicX->GetCase().IsPartial();
    }
}


double CIntDriver::exisverified(int iExpr)
{
    const auto& fn8_node = GetNode<FN8_NODE>(iExpr);
    const Symbol* symbol = NPT(fn8_node.symbol_index);

    if( symbol->IsA(SymbolType::Dictionary) )
    {
        const EngineDictionary* engine_dictionary = assert_cast<const EngineDictionary*>(symbol);
        const Case& data_case = engine_dictionary->GetEngineCase().GetCase();
        return data_case.GetVerified();
    }

    else
    {
        const DICX* pDicX = DPX(fn8_node.symbol_index);
        return pDicX->GetCase().GetVerified();
    }
}


double CIntDriver::exsavepartial(int iExpr)
{
    if( Issamod != ModuleType::Entry )
        return 0;

    // 20111125 savepartial only works when called from a field (trevor experienced problems because he was calling it from a form proc)
    if( !m_pCsDriver->GetCurDeFld() )
        return 0;

    const auto& va_node = GetNode<Nodes::VariableArguments>(iExpr);
    bool clear_skipped = ( va_node.arguments[0] == 1 );

    CEntryDriver* pEntryDriver = assert_cast<CEntryDriver*>(m_pEngineDriver);

    if( m_iExLevel == 0 )
    {
        issaerror(MessageType::Error, 8050);
    }

    else if( !pEntryDriver->QidReady(1) )
    {
        issaerror(MessageType::Error, 8052);
    }

    else if( WindowsDesktopMessage::Send(WM_IMSA_PARTIAL_SAVE, clear_skipped) == 1 )
    {
        return true;
    }

#ifndef WIN_DESKTOP
    else
    {
        constexpr bool from_logic = true;
        return PlatformInterface::GetInstance()->GetApplicationInterface()->PartialSave(clear_skipped, from_logic);
    }
#endif

    return false;
}


double CIntDriver::ex_setfile(const int program_index)
{
    const auto& setfile_node = GetNode<Nodes::SetFile>(program_index);
    Symbol& symbol = NPT_Ref(setfile_node.symbol_index);
    bool create = ( setfile_node.mode == Nodes::SetFile::Mode::Create );
    const bool append = ( setfile_node.mode == Nodes::SetFile::Mode::Append );

    if( symbol.IsA(SymbolType::File) )
    {
        LogicFile& logic_file = assert_cast<LogicFile&>(symbol);

        // close any existing file
        logic_file.Close();

        std::string file_path = EvaluatePath(setfile_node.filename_expression);

        // create the directory if necessary
        if( ( create || append ) && !PortableFunctions::PathMakeDirectories(PortableFunctions::PathGetDirectory(file_path)) )
            return 0;

        bool truncate = true;

        if( append )
        {
            create = true;
            truncate = false;
        }

        logic_file.SetFilePath(std::move(file_path));

        if( !logic_file.GetFilePath().empty() )
        {
            if( logic_file.Open(create, append, truncate) )
                return 1;
        }
    }

    else if( symbol.IsA(SymbolType::Dictionary) )
    {
        const ConnectionString connection_string = EvaluateConnectionString(setfile_node.filename_expression);
        return ex_setfile_dictionary(assert_cast<EngineDictionary&>(symbol), connection_string, create, append);
    }

    else if( symbol.IsA(SymbolType::Pre80Dictionary) )
    {
        const ConnectionString connection_string = EvaluateConnectionString(setfile_node.filename_expression);
        return ex_setfile_dictionary(assert_cast<DICT*>(&symbol), connection_string, create, append);
    }

    else
    {
        ASSERT(false);
    }

    return 0;
}


template<typename T>
void CIntDriver::MakeFullPathFileName(T& filename) const
{
    if constexpr(std::is_same_v<T, CString>)
    {
        filename = WS2CS(MakeFullPath(GetWorkingFolder(m_pEngineDriver->m_pPifFile->GetAppFName()), SO::TrimRight(filename)));
    }

    else if constexpr(std::is_same_v<T, std::wstring>)
    {
        filename = MakeFullPath(GetWorkingFolder(m_pEngineDriver->m_pPifFile->GetAppFName()), SO::TrimRight(filename));
    }
}

template void CIntDriver::MakeFullPathFileName(CString& filename) const;
template void CIntDriver::MakeFullPathFileName(std::wstring& filename) const;


/////////////////////////////////////////////////////////////////////////////////////////////////////////////

double CIntDriver::exshow(int iExpr)
{
    const auto& show_node = GetNode<FNSHOW_NODE>(iExpr);

    int iActualForNode = show_node.m_iForNode;

    if( iActualForNode >= 0 && GetNode<int>(iActualForNode) == SCOPE_CHANGE_CODE )
        iActualForNode = GetNode<Nodes::ScopeChange>(iActualForNode).program_index;

    if( !UseHtmlDialogs() )
        return exshow_pre77(iExpr, iActualForNode);

    m_aShowLines.clear();

    if( iActualForNode >= 0 )
    {
        const int iForCode = GetNode<int>(iActualForNode);

        if( iForCode == FOR_RELATION_CODE )
        {
            exfor_relation(iActualForNode);
        }

        else
        {
            ASSERT(iForCode == FOR_GROUP_CODE);
            exfor_group(iActualForNode);
        }
    }

    else // a single item
    {
        exshowlist(-iActualForNode);
    }


    // return if nothing to show
    if( m_aShowLines.empty() )
        return 0;


    const SHOWLIST_NODE* pShowListNode = (SHOWLIST_NODE*)PPT(show_node.m_iShowListNode);
    const LIST_NODE* pVarList = (LIST_NODE*)PPT(pShowListNode->m_iVarListNode);

    int iNumCols = pVarList->iNumElems;

    ASSERT(m_aShowLines.size() % iNumCols == 0);

    // fill in the titles array
    std::vector<SharableString> column_headings;

    if( show_node.m_iTitleList > 0 )
    {
        LIST_NODE* pTitleList = (LIST_NODE*)PPT(show_node.m_iTitleList);

        int iNumTitles = std::min(iNumCols, pTitleList->iNumElems);

        for( int i = 0; i < iNumTitles; ++i )
            column_headings.emplace_back(Evaluate<SharableString>(pTitleList->iSym[i]));
    }

    // complete any missing titles
    for( int i = (int)column_headings.size(); i < iNumCols; ++i )
    {
        const Symbol* column_symbol = nullptr;

        if( pVarList->iSym[i] < 0 )
        {
            column_symbol = NPT(-1 * pVarList->iSym[i]);
        }

        else
        {
            SVAR_NODE *pSVarNode = (SVAR_NODE*)PPT(pVarList->iSym[i]);

            if( pSVarNode->m_iVarType == SVAR_CODE || pSVarNode->m_iVarType == MVAR_CODE )
                column_symbol = NPT(pSVarNode->m_iVarIndex);
        }

        column_headings.emplace_back(( column_symbol != nullptr ) ? SharableString(column_symbol->GetName()) :
                                                                    SharableString());
    }

    // initialize the dialog
    size_t number_columns = column_headings.size();
    SelectDlg select_dlg(true, number_columns);

    if( show_node.m_iHeading >= 0 )
        select_dlg.SetTitle(Evaluate<SharableString>(show_node.m_iHeading));

    select_dlg.SetHeader(std::move(column_headings));

    // reformat m_aShowLines for the dialog
    for( size_t i = 0; i < m_aShowLines.size(); )
    {
        std::vector<SharableString> row_data;

        for( size_t j = 0; j < number_columns; ++j, ++i )
        {
            ASSERT(i < m_aShowLines.size());
            row_data.emplace_back(UTF8_TODO::GetUtf8(m_aShowLines[i]));
        }

        select_dlg.AddRow(std::move(row_data));
    }

    m_aShowLines.clear();

    SelectDlgHelper select_dlg_helper(*m_paradataDriver, select_dlg, Paradata::OperatorSelectionEvent::Source::Show);
    return static_cast<double>(select_dlg_helper.GetSingleSelection());
}


double CIntDriver::exshow_pre77(int iExpr, int iActualForNode)
{
    const FNSHOW_NODE* pShowNode = (FNSHOW_NODE*)PPT(iExpr);

    m_aShowLines.clear();

    if( iActualForNode >= 0 )
    {
        const int iForCode = *PPT(iActualForNode);

        if( iForCode == FOR_RELATION_CODE )
        {
            exfor_relation(iActualForNode);
        }

        else
        {
            ASSERT(iForCode == FOR_GROUP_CODE);
            exfor_group(iActualForNode);
        }
    }

    else // a single item
    {
        exshowlist(-iActualForNode);
    }


    // return if nothing to show
    if( m_aShowLines.empty() )
        return 0;


    const SHOWLIST_NODE* pShowListNode = (SHOWLIST_NODE*)PPT(pShowNode->m_iShowListNode);
    const LIST_NODE* pVarList = (LIST_NODE*)PPT(pShowListNode->m_iVarListNode);

    int iNumCols = pVarList->iNumElems;

    ASSERT(m_aShowLines.size() % iNumCols == 0);

    // fill in the titles array
    std::vector<CString> aColumnTitles;

    if( pShowNode->m_iTitleList > 0 )
    {
        LIST_NODE* pTitleList = (LIST_NODE*)PPT(pShowNode->m_iTitleList);

        int iNumTitles = std::min(iNumCols,pTitleList->iNumElems);

        for( int i = 0; i < iNumTitles; i++ )
            aColumnTitles.emplace_back(EvalAlphaExprCS(pTitleList->iSym[i]));
    }

    // complete any missing titles
    for( int i = aColumnTitles.size(); i < iNumCols; i++ )
    {
        CString csVarName;

        if( pVarList->iSym[i] < 0 )
        {
            csVarName = UTF8_TODO::GetCString(NPT(-1 * pVarList->iSym[i])->GetName());
        }

        else
        {
            SVAR_NODE *pSVarNode = (SVAR_NODE*)PPT(pVarList->iSym[i]);

            if( pSVarNode->m_iVarType == SVAR_CODE || pSVarNode->m_iVarType == MVAR_CODE )
                csVarName = UTF8_TODO::GetCString(NPT(pSVarNode->m_iVarIndex)->GetName());
        }

        aColumnTitles.emplace_back(csVarName);
    }


    // reformat m_aShowLines in tabular format
    std::vector<std::vector<CString>*> aData;
    std::vector<CString>* paRowData = NULL;

    for( size_t i = 0; i < m_aShowLines.size(); i++ )
    {
        if( ( i % iNumCols ) == 0 )
        {
            paRowData = new std::vector<CString>;
            aData.emplace_back(paRowData);
        }

        paRowData->emplace_back(m_aShowLines[i]);
    }


    // show the dialog
    CString csHeading;

    if( pShowNode->m_iHeading >= 0 )
        csHeading = EvalAlphaExprCS(pShowNode->m_iHeading);

    int iRet = SelectDlgHelper_pre77(pShowNode->fn_code, csHeading, &aData, &aColumnTitles, nullptr, nullptr);

    for( const auto& data : aData )
        delete data;

    m_aShowLines.clear();

    return iRet;
}


double CIntDriver::exshowlist(int iExpr)
{
    const SHOWLIST_NODE* pShowListNode = (SHOWLIST_NODE*)PPT(iExpr);
    const LIST_NODE* pVarList = (LIST_NODE*)PPT(pShowListNode->m_iVarListNode);

    for( int i = 0; i < pVarList->iNumElems; i++ )
    {
        CString csValue;
        int iExprSymVar = pVarList->iSym[i];

        if( iExprSymVar < 0 )
        {
            ASSERT(NPT(-1 * iExprSymVar)->IsA(SymbolType::WorkString));
            csValue = UTF8_TODO::GetCString(GetSymbolWorkString(-1 * iExprSymVar).GetString());
        }

        else
        {
            SVAR_NODE* pSVarNode = (SVAR_NODE*)PPT(iExprSymVar);
            int iThisVar = pSVarNode->m_iVarIndex;
            int iThisVarType = pSVarNode->m_iVarType;
            VART* pVarT = NULL;

            if( iThisVarType == SVAR_CODE )
                pVarT = VPT(iThisVar);
            else if( iThisVarType == MVAR_CODE )
                pVarT = VPT(iThisVar);
            else
                ASSERT(0);

            bool bAlphaVar = ( pVarT != NULL && pVarT->IsAlpha() );

            if( bAlphaVar )
            {
                csValue = UTF8_TODO::GetCString(*exavar(iExprSymVar).get<SharableString>());
            }

            else
            {
                double dValue = evalexpr(iExprSymVar);

                if( pVarT != NULL && dValue != NOTAPPL )
                {
                    TCHAR* buffer = csValue.GetBufferSetLength(pVarT->GetLength());
                    pVarT->dvaltochar(dValue, buffer);
                    csValue.ReleaseBuffer();
                }
            }
        }

        m_aShowLines.emplace_back(csValue);
    }

    return 0;
}


double CIntDriver::exshowarray(int iExpr)
{
    if( !UseHtmlDialogs() )
        return exshowarray_pre77(iExpr);

    const auto& fnn_node = GetNode<FNN_NODE>(iExpr);
    const LogicArray& show_array = GetSymbolLogicArray(fnn_node.fn_expr[0]);
    const int& row_count_expression = fnn_node.fn_expr[1];
    const int& column_count_expression = fnn_node.fn_expr[2];
    const int& title_expression = fnn_node.fn_expr[3];
    std::vector<SharableString> column_headings;

    for( int i = 4; i < fnn_node.fn_nargs; ++i )
        column_headings.emplace_back(Evaluate<SharableString>(fnn_node.fn_expr[i]));

    // calculate the number of columns
    size_t start_column = 0;
    size_t end_column = 0;

    if( show_array.GetNumberDimensions() == 2 )
    {
        start_column = 1;

        if( !column_headings.empty() )
        {
            end_column = column_headings.size();

            if( end_column >= show_array.GetDimension(1) ) // this can only happen with array parameters
                return 0;
        }

        else if( column_count_expression >= 0 )
        {
            end_column = Evaluate<size_t>(column_count_expression);

            if( end_column < 1 || end_column >= show_array.GetDimension(1) )
                return 0;
        }

        else // browse the first row until a blank string is found
        {
            for( std::vector<size_t> indices({ 1, 1 }); indices[1] < show_array.GetDimension(1); ++indices[1] )
            {
                if( SO::IsBlank(show_array.GetValue<SharableString>(indices).GetString()) )
                    break;

                ++end_column;
            }
        }
    }

    if( end_column < start_column ) // no valid columns
        return 0;

    // setup the indices object used to access the data
    std::vector<size_t> indices(show_array.GetNumberDimensions(), 0);

    // calculate the number of rows
    size_t start_row = 1;
    size_t end_row = 0;

    if( row_count_expression >= 0 )
    {
        end_row = Evaluate<size_t>(row_count_expression);

        if( end_row < 1 || end_row >= show_array.GetDimension(0) )
            return 0;
    }

    else // browse the first column until a blank string is found
    {
        if( show_array.GetNumberDimensions() == 2 )
            indices[1] = 1;

        for( indices[0] = 1; indices[0] < show_array.GetDimension(0); ++indices[0] )
        {
            if( SO::IsBlank(show_array.GetValue<SharableString>(indices).GetString()) )
                break;

            ++end_row;
        }
    }

    if( end_row < start_row ) // no valid rows
        return 0;


    // if the user didn't explicitly specify the column titles, get them from the zeroth row
    if( column_headings.empty() )
    {
        if( show_array.GetNumberDimensions() == 2 )
            indices[1] = start_column;

        for( indices[0] = 0; show_array.GetNumberDimensions() == 1 || indices[1] <= end_column; ++indices[1] )
        {
            SharableString& column_heading = column_headings.emplace_back(show_array.GetValue<SharableString>(indices));
            column_heading.MakeTrimRight();

            if( show_array.GetNumberDimensions() == 1 )
                break;
        }
    }


    // initialize the dialog
    ASSERT(column_headings.size() == ( end_column - start_column + 1 ));

    SelectDlg select_dlg(true, column_headings.size());

    if( title_expression >= 0 )
        select_dlg.SetTitle(Evaluate<SharableString>(title_expression));

    select_dlg.SetHeader(std::move(column_headings));


    // fill the table of cells
    for( indices[0] = start_row; indices[0] <= end_row; ++indices[0] )
    {
        std::vector<SharableString> row_data;

        if( show_array.GetNumberDimensions() == 2 )
            indices[1] = start_column;

        for( ; show_array.GetNumberDimensions() == 1 || indices[1] <= end_column; ++indices[1] )
        {
            SharableString& row = row_data.emplace_back(show_array.GetValue<SharableString>(indices));
            row.MakeTrimRight();

            if( show_array.GetNumberDimensions() == 1 )
                break;
        }

        select_dlg.AddRow(std::move(row_data));
    }

    SelectDlgHelper select_dlg_helper(*m_paradataDriver, select_dlg, Paradata::OperatorSelectionEvent::Source::ShowArray);
    return static_cast<double>(select_dlg_helper.GetSingleSelection());
}


double CIntDriver::exshowarray_pre77(int iExpr)
{
    const FNN_NODE* ptrfunc = (FNN_NODE*)PPT(iExpr);
    const LogicArray* show_array = &GetSymbolLogicArray(ptrfunc->fn_expr[0]);
    int iRowCountExpr = ptrfunc->fn_expr[1];
    int iColCountExpr = ptrfunc->fn_expr[2];
    CString csHeading;

    if( ptrfunc->fn_expr[3] >= 0 )
        csHeading = EvalAlphaExprCS(ptrfunc->fn_expr[3]);

    std::vector<CString> aColumnTitles;

    for( int i = 4; i < ptrfunc->fn_nargs; i++ )
        aColumnTitles.emplace_back(EvalAlphaExprCS(ptrfunc->fn_expr[i]));


    // calculate the number of columns
    size_t iStartCol = 0;
    size_t iEndCol = 0;

    if( show_array->GetNumberDimensions() == 2 )
    {
        iStartCol = 1;

        if( aColumnTitles.size() > 0 )
        {
            iEndCol = aColumnTitles.size();

            if( iEndCol >= show_array->GetDimension(1) ) // this can only happen with array parameters
                return 0;
        }

        else if( iColCountExpr >= 0 )
        {
            iEndCol = Evaluate<int>(iColCountExpr);

            if( iEndCol < 1 || iEndCol >= show_array->GetDimension(1) )
                return 0;
        }

        else // browse the first row until a blank string is found
        {
            for( std::vector<size_t> indices({ 1, 1 }); indices[1] < show_array->GetDimension(1); ++indices[1] )
            {
                if( SO::IsWhitespace(show_array->GetValue<SharableString>(indices).GetString()) )
                    break;

                ++iEndCol;
            }
        }
    }

    if( iEndCol < iStartCol ) // no valid columns
        return 0;

    // setup the indices object used to access the data
    std::vector<size_t> indices(show_array->GetNumberDimensions(), 0);

    // calculate the number of rows
    size_t iStartRow = 1;
    size_t iEndRow = 0;

    if( iRowCountExpr >= 0 )
    {
        iEndRow = Evaluate<int>(iRowCountExpr);

        if( iEndRow < 1 || iEndRow >= show_array->GetDimension(0) )
            return 0;
    }

    else // browse the first column until a blank string is found
    {
        if( show_array->GetNumberDimensions() == 2 )
            indices[1] = 1;

        for( indices[0] = 1; indices[0] < show_array->GetDimension(0); ++indices[0] )
        {
            if( SO::IsWhitespace(show_array->GetValue<SharableString>(indices).GetString()) )
                break;

            ++iEndRow;
        }
    }

    if( iEndRow < iStartRow ) // no valid rows
        return 0;


    bool bLabelsDefined = ( aColumnTitles.size() != 0 );

    if( !bLabelsDefined )
    {
        // if the user didn't explicitly specify the column titles, get them from the zeroth row
        if( show_array->GetNumberDimensions() == 2 )
            indices[1] = iStartCol;

        for( indices[0] = 0; ( show_array->GetNumberDimensions() == 1 ) || ( indices[1] <= iEndCol ); ++indices[1] )
        {
            SharableString column_title = show_array->GetValue<SharableString>(indices);
            column_title.MakeTrimRight();

            aColumnTitles.emplace_back(UTF8_TODO::GetCString(*column_title));

            if( !column_title->empty() )
                bLabelsDefined = true;

            if( show_array->GetNumberDimensions() == 1 )
                break;
        }
    }


    // fill the table of cells
    std::vector<std::vector<CString>*> aData;

    for( indices[0] = iStartRow; indices[0] <= iEndRow; indices[0]++ )
    {
        std::vector<CString>* paRowData = new std::vector<CString>;
        aData.emplace_back(paRowData);

        if( show_array->GetNumberDimensions() == 2 )
            indices[1] = iStartCol;

        for( ; ( show_array->GetNumberDimensions() == 1 ) || ( indices[1] <= iEndCol ); ++indices[1] )
        {
            SharableString row = show_array->GetValue<SharableString>(indices);
            row.MakeTrimRight();

            paRowData->emplace_back(UTF8_TODO::GetCString(*row));

            if( show_array->GetNumberDimensions() == 1 )
                break;
        }
    }

    int iRet = SelectDlgHelper_pre77(ptrfunc->fn_code, csHeading, &aData, bLabelsDefined ? &aColumnTitles : nullptr, nullptr, nullptr);

    for( std::vector<std::vector<CString>*>::size_type i = 0; i < aData.size(); i++ )
        delete aData[i];

    return iRet;
}


double CIntDriver::excountvalid(int iExpr) // 20091203 count the number non-special values passed
{
    const auto& fnn_node = GetNode<FNN_NODE>(iExpr);

    int numValids = 0;

    for( int i = 0; i < fnn_node.fn_nargs; i++ )
    {
        const auto& item_node = GetNode<CONST_NODE>(fnn_node.fn_expr[i]);
        ItemListType itemType = (ItemListType)item_node.const_type;

        if( itemType == ItemListType::NUMERICFUNC )
        {
            if( !IsSpecial(evalexpr(item_node.const_index)) )
                numValids++;
        }


        else if( itemType == ItemListType::ARRAY ) // go through the whole array to check for valids
        {
            const LogicArray& logic_array = GetSymbolLogicArray(item_node.const_index);

            std::function<void(const std::vector<size_t>&)> countvalid_counter =
                [&](const std::vector<size_t>& indices)
                {
                    if( !IsSpecial(logic_array.GetValue<double>(indices)) )
                        ++numValids;
                };

            logic_array.IterateCells(1, countvalid_counter);
        }


        else if( itemType == ItemListType::RECORD || itemType == ItemListType::RECORD_INDEXED )
        {
            SECT* pSecT = SIP(item_node.const_index);
            GROUPT* pGroupT;

            int iGroupNum = 0;

            bool usingIndex = false;
            int index = 0;

            if( itemType == ItemListType::RECORD_INDEXED )
            {
                const int* extraParameterIndex = reinterpret_cast<const int*>(&item_node) + sizeof(CONST_NODE) / sizeof(int);

                index = *extraParameterIndex;

                if( index >= 0 ) // then it's an expression
                    index = (int) evalexpr(index);

                if( index != -1 ) // -1 means all data elements
                    usingIndex = true;
            }


            while( ( pGroupT = pSecT->GetGroup(iGroupNum) ) != NULL )
            {
                for( int iItem = 0; iItem < pGroupT->GetNumItems(); iItem++ )
                {
                    int iSymb = pGroupT->GetItemSymbol(iItem);

                    if( !iSymb )
                        break;

                    VART* pVarT = VPT(iSymb);

                    if( pVarT->m_pSecT == pSecT ) // there must be a better way to get only the items from the section (aka record) (and not the group)
                    {
                        if( pGroupT->m_pEngineArea->GetTypeGRorVA(iSymb) == SymbolType::Variable ) // is the item a variable
                        {
                            if( pVarT->IsNumeric() && pVarT->IsUsed() )
                            {
                                if( !pVarT->IsArray() && !IsSpecial(GetVarFloatValue(pVarT)) )
                                {
                                    numValids++;
                                }

                                else // this must be an item on a multiply occurring record
                                {
                                    int aIndex[DIM_MAXDIM] = { 0, 0, 0 };

                                    int lowerIndex = 0, upperIndex = pSecT->GetMaxOccs() - 1;

                                    if( usingIndex )
                                    {
                                        if( index == -2 ) // use the current occurrence
                                        {
                                            GROUPT* recordGroup = pGroupT;

                                            if( pGroupT->GetDimType() != CDimension::Record )
                                                recordGroup = pGroupT->GetParentGPT();

                                            upperIndex = lowerIndex = GetCurOccFromGroup(recordGroup,Issamod != ModuleType::Entry) - 1;
                                        }

                                        else
                                        {
                                            lowerIndex = index - 1;

                                            if( lowerIndex < 0 || lowerIndex > upperIndex )
                                            {
                                                issaerror(MessageType::Warning,8351, pSecT->GetName().c_str(), lowerIndex + 1);
                                                usingIndex = false;
                                                lowerIndex = 0;
                                            }

                                            else
                                            {
                                                upperIndex = lowerIndex;
                                            }
                                        }

                                    }

                                    for( int j = lowerIndex; j <= upperIndex; j++ )
                                    {
                                        aIndex[0] = j;

                                        for( int k = 0; k < pVarT->GetMaxOccs(); k++ )
                                        {
                                            aIndex[1] = k;

                                            if( !IsSpecial(GetVarFloatValue(pVarT,CNDIndexes(ZERO_BASED,aIndex))) )
                                                numValids++;
                                        }
                                    }

                                }

                            }

                        }

                    }

                }

                iGroupNum++;
            }

        }


        else if( itemType == ItemListType::MULTIPLYOCCURINGITEM )
        {
            VART* pVarT = VPT(item_node.const_index);
            SECT* pSecT = pVarT->GetSPT();

            if( pVarT->IsNumeric() )
            {
                int aIndex[DIM_MAXDIM] = { 0, 0, 0 };

                int lowerIndex = 0, upperIndex = pSecT->GetMaxOccs() - 1;

                for( int j = lowerIndex; j <= upperIndex; j++ )
                {
                    aIndex[0] = j;

                    for( int k = 0; k < pVarT->GetMaxOccs(); k++ )
                    {
                        aIndex[1] = k;

                        if( !IsSpecial(GetVarFloatValue(pVarT,CNDIndexes(ZERO_BASED,aIndex))) )
                            numValids++;
                    }
                }
            }
        }


    }

    return numValids;
}


CDEField* CIntDriver::GetCDEFieldFromVART(VART* pVarT)
{
    for( const auto& pFF : m_pEngineDriver->GetApplication()->GetRuntimeFormFiles() )
    {
        CDEField* pField = pFF->GetField(pVarT->GetSymbolIndex());

        if( pField != nullptr )
            return pField;
    }

    return nullptr;
}


double CIntDriver::ex_getcapturetype(const int program_index)
{
    const auto& fnn_node = GetNode<FNN_NODE>(program_index);
    const VART* const pVarT = VPT(fnn_node.fn_expr[0]);

    // use the evaluated capture info so that Unspecified is never returned
    return static_cast<double>(pVarT->GetEvaluatedCaptureInfo().GetCaptureType());
}


double CIntDriver::ex_setcapturetype(const int program_index)
{
    // refresh the responses window in case the capture type has been changed
    WindowsDesktopMessage::Post(UWM::CSEntry::ShowCapi);

    const auto& fnn_node = GetNode<FNN_NODE>(program_index);
    Symbol& symbol = NPT_Ref(fnn_node.fn_expr[0]);

    const int int_capture_type = Evaluate<int>(fnn_node.fn_expr[1]);

    if( int_capture_type < static_cast<int>(CaptureType::FirstDefined) ||
        int_capture_type > static_cast<int>(CaptureType::LastDefined) )
    {
        return DEFAULT;
    }

    const CaptureType new_capture_type = static_cast<CaptureType>(int_capture_type);
    SharableString date_format;

    if( fnn_node.fn_nargs == 3 && new_capture_type == CaptureType::Date )
    {
        date_format = Evaluate<SharableString>(fnn_node.fn_expr[2]);
        date_format.MakeTrim();
    }

    auto setcapturetype_processor = [&](VART& vart)
    {
        const CDictItem& dict_item = *vart.GetDictItem();

        CaptureInfo new_capture_info = new_capture_type;

        if( new_capture_type == CaptureType::Date )
        {
            // if the date format is specified, use it; if not, use an existing date format when possible
            SharableString date_format_to_use = date_format;

            if( date_format_to_use->empty() && vart.GetCaptureInfo().GetCaptureType() == CaptureType::Date )
                date_format_to_use = vart.GetCaptureInfo().GetExtended<DateCaptureInfo>().GetFormat();

            if( !date_format_to_use->empty() )
            {
                new_capture_info.GetExtended<DateCaptureInfo>().SetFormat(date_format_to_use.Release());

                if( !new_capture_info.GetExtended<DateCaptureInfo>().IsFormatValid(dict_item) )
                    return 0;
            }
        }

        CaptureInfo valid_capture_info = new_capture_info.MakeValid(dict_item, vart.GetCurrentDictValueSet());

        if( valid_capture_info.GetCaptureType() == new_capture_type )
        {
            vart.SetCaptureInfo(std::move(valid_capture_info));
            return 1;
        }

        return 0;
    };

    const size_t fields_modified = ForeachVariable(GetSymbolTable(), symbol, setcapturetype_processor);

    return static_cast<double>(fields_modified);
}


double CIntDriver::ex_setcapturepos(const int program_index)
{
#ifndef WIN_DESKTOP
    // not applicable on portable platforms
    return DEFAULT;
#else
    const auto& fnn_node = GetNode<FNN_NODE>(program_index);
    Symbol& symbol = NPT_Ref(fnn_node.fn_expr[0]);

    const POINT point
    {
        Evaluate<int>(fnn_node.fn_expr[1]),
        Evaluate<int>(fnn_node.fn_expr[2])
    };

    const size_t fields_modified = ForeachVariable(GetSymbolTable(), symbol,
        [&](VART& vart)
        {
            vart.SetCapturePos(point);
            return 1;
        });

    return static_cast<double>(fields_modified);
#endif
}


double CIntDriver::ex_changekeyboard(const int program_index)
{
#ifndef WIN_DESKTOP
    // not applicable on portable platforms
    return DEFAULT;
#else
    const auto& va_node = GetNode<Nodes::VariableArguments>(program_index);
    Symbol& symbol = NPT_Ref(va_node.arguments[1]);

    // they are only interested in what the keyboard ID is...
    if( va_node.arguments[0] == -1 )
    {
        ASSERT(symbol.IsA(SymbolType::Variable));
        return assert_cast<const VART&>(symbol).GetKeyboardLayoutId();
    }

    // ...or the keyboard ID is being changed
    else
    {
        const unsigned keyboard_id = m_keyboardLoader->GetKeyboardId(Evaluate<unsigned int>(va_node.arguments[0]));

        const size_t fields_modified = ForeachVariable(GetSymbolTable(), symbol,
            [&](VART& vart)
            {
                // no reason to change it if it's not on a form
                if( !vart.IsUsed() )
                    return 0;

                vart.SetKeyboardLayoutId(keyboard_id);
                return 1;
            });

        return static_cast<double>(fields_modified);
    }
#endif
}


Engine::Value CIntDriver::ex_getrecord(const int program_index)
{
    const auto& fnn_node = GetNode<FNN_NODE>(program_index);
    const SharableString item_name = Evaluate<SharableString>(fnn_node.fn_expr[0]);

    const int symbol_index = m_pEngineArea->SymbolTableSearch(*item_name, { SymbolType::Variable });

    if( symbol_index == 0 )
        return Engine::Value::Invalid<SharableString>();

    return NPT_Ref(symbol_index).GetName();
}


std::vector<int> CIntDriver::EvaluateValidIndices(int iSymGroup, int iSymItem, int iWhere) // 20110810
{
    std::vector<int> validIndices;

    MVAR_NODE MVAR;
    MVAR_NODE* pMVAR = &MVAR;
    memset(pMVAR, 0, sizeof(MVAR_NODE));
    MVAR.m_iVarType = MVAR_GROUP;
    MVAR.m_iVarIndex = iSymItem;

    CALCULATE_LIMITS(iLimit, pMVAR, iSymGroup);

    SPECIFY_LAST_DIMENSION(pMVAR);

    for( m_iExOccur = 1; m_iExOccur <= iLimit; m_iExOccur++ )
    {
        if( EvaluateConditional(iWhere) )
            validIndices.emplace_back(m_iExOccur);
    }

    RESTORE_DEFAULT();

    return validIndices;
}


Engine::Value CIntDriver::ex_getocclabel(const int program_index)
{
    const auto& various_node = GetNode<FNVARIOUS_NODE>(program_index);
    const Symbol& symbol = NPT_Ref(various_node.fn_expr[0]);
    const int occurrence_expression = various_node.fn_expr[1];
    std::optional<int> zero_based_occurrence;

    if( occurrence_expression != -1 )
        zero_based_occurrence = Evaluate<int>(occurrence_expression) - 1;

    return EvaluateOccurrenceLabel(symbol, zero_based_occurrence);
}


std::string CIntDriver::EvaluateOccurrenceLabel(const Symbol& symbol, const std::optional<int>& zero_based_occurrence)
{
    bool bUseBatchLogic = Issamod != ModuleType::Entry;
    int iSpecifiedOcc = zero_based_occurrence.value_or(0);
    CString label;

    if( symbol.IsA(SymbolType::Variable) ) // item
    {
        auto pVarT = assert_cast<const VART*>(&symbol);
        const CDictItem* item = pVarT->GetDictItem();
        ASSERT(item != nullptr);

        if( !zero_based_occurrence.has_value() )
            iSpecifiedOcc = GetCurOccFromGroup(pVarT->GetParentGPT(),bUseBatchLogic) - 1;

        if( iSpecifiedOcc >= 0 && iSpecifiedOcc < (int)item->GetOccurs() )
            label = item->GetOccurrenceLabels().GetLabel(iSpecifiedOcc);
    }

    else if( symbol.IsA(SymbolType::Record) ) // record
    {
        const EngineRecord& engine_record = assert_cast<const EngineRecord&>(symbol);

        if( !zero_based_occurrence.has_value() )
        {
            // ENGINECR_TODO(getocclabel) ... need an implementation of GetCurOccFromChildrenGroups
            // iSpecifiedOcc = (int)GetCurOccFromChildrenGroups(engine_record, bUseBatchLogic) - 1;
        }

        if( iSpecifiedOcc >= 0 && iSpecifiedOcc < (int)engine_record.GetDictRecord().GetMaxRecs() )
            label = engine_record.GetDictRecord().GetOccurrenceLabels().GetLabel(iSpecifiedOcc);
    }

    else if( symbol.IsA(SymbolType::Section) ) // record
    {
        auto pSecT = assert_cast<const SECT*>(&symbol);

        if( !zero_based_occurrence.has_value() )
            iSpecifiedOcc = (int)GetCurOccFromChildrenGroups(pSecT,bUseBatchLogic) - 1;

        if( iSpecifiedOcc >= 0 && iSpecifiedOcc < pSecT->GetMaxOccs() )
            label = pSecT->GetDictRecord()->GetOccurrenceLabels().GetLabel(iSpecifiedOcc);
    }

    else if( symbol.IsOneOf(SymbolType::Group) ) // group
    {
        auto pGroupT = assert_cast<const GROUPT*>(&symbol);

        if( !zero_based_occurrence.has_value() )
            iSpecifiedOcc = GetCurOccFromGroup(pGroupT, bUseBatchLogic) - 1;

        if( iSpecifiedOcc >= 0 && iSpecifiedOcc < pGroupT->GetMaxOccs() )
        {
            const CDEGroup* pGroup = pGroupT->GetCDEGroup();

            if( pGroup != nullptr )
            {
                if( pGroup->GetItemType() == CDEFormBase::Roster )
                    label = ((CDERoster*)pGroup)->GetStubTextSet().GetText(iSpecifiedOcc).GetLabel();

                if( pGroup->GetRIType() == CDEFormBase::Record )
                {
                    ASSERT(pGroupT->GetNumItems() > 0);
                    const CDictItem* pItem = pGroupT->GetFirstDictItem();
                    label = pItem->GetRecord()->GetOccurrenceLabels().GetLabel(iSpecifiedOcc);
                }

                else if( pGroup->GetRIType() == CDEFormBase::Item || pGroup->GetRIType() == CDEFormBase::SubItem )
                {
                    const int iSymItem = m_pEngineArea->SymbolTableSearch(UTF8_TODO::GetUtf8(pGroup->GetRepeatName()), { SymbolType::Variable });

                    if( iSymItem > 0 )
                    {
                        const CDictItem* pItem = VPT(iSymItem)->GetDictItem();
                        label = pItem->GetOccurrenceLabels().GetLabel(iSpecifiedOcc);
                    }
                }
            }
        }
    }

    else
    {
        ASSERT(false);
    }

    return UTF8_TODO::GetUtf8(label);
}


double CIntDriver::exsetocclabel(int iExpr)
{
    const auto& various_node = GetNode<FNVARIOUS_NODE>(iExpr);
    ASSERT(NPT(various_node.fn_expr[0])->IsA(SymbolType::Group));
    GROUPT* pGroupT = GPT(various_node.fn_expr[0]);
    bool bUseCurrentOcc = ( various_node.fn_expr[1] == -1 );
    int iSpecifiedOcc = bUseCurrentOcc ? -1 : ( Evaluate<int>(various_node.fn_expr[1]) - 1 );
    bool bUseBatchLogic = Issamod != ModuleType::Entry;

    if( bUseCurrentOcc )
        iSpecifiedOcc = GetCurOccFromGroup(pGroupT, bUseBatchLogic) - 1;

    if( iSpecifiedOcc >= 0 && iSpecifiedOcc < pGroupT->GetMaxOccs() )
    {
        CString new_label = EvalAlphaExprCS(various_node.fn_expr[2]);

        CDEGroup* pGroup = pGroupT->GetCDEGroup();

        if( pGroup != nullptr )
        {
            pGroupT->SaveOccLabel(iSpecifiedOcc);

            if( pGroup->GetItemType() == CDEFormBase::Roster )
            {
                ((CDERoster*)pGroup)->GetStubTextSet().GetText(iSpecifiedOcc).SetLabel(new_label);
                WindowsDesktopMessage::Post(WM_IMSA_GROUP_OCCS_CHANGE);
            }

            if( pGroup->GetRIType() == CDEFormBase::Record )
            {
                ASSERT(pGroupT->GetNumItems() > 0);
                // DD_STD_REFACTOR_TODO store dictionary occurrence labels somewhere else
                const CDictItem* pItem = pGroupT->GetFirstDictItem();
                const_cast<CDictItem*>(pItem)->GetRecord()->GetOccurrenceLabels().SetLabel(iSpecifiedOcc, new_label);
            }

            else if( pGroup->GetRIType() == CDEFormBase::Item || pGroup->GetRIType() == CDEFormBase::SubItem )
            {
                const int iSymItem = m_pEngineArea->SymbolTableSearch(UTF8_TODO::GetUtf8(pGroup->GetRepeatName()), { SymbolType::Variable });

                if( iSymItem > 0 )
                {
                    // DD_STD_REFACTOR_TODO store dictionary occurrence labels somewhere else
                    const CDictItem* pItem = VPT(iSymItem)->GetDictItem();
                    const_cast<CDictItem*>(pItem)->GetOccurrenceLabels().SetLabel(iSpecifiedOcc, new_label);
                }
            }

            else
            {
                ASSERT(false);
            }

            return 1;
        }
    }

    return 0;
}


double CIntDriver::exshowocc(int iExpr)
{
    // this is used for both showocc and hideocc
    const auto& various_node = GetNode<FNVARIOUS_NODE>(iExpr);
    ASSERT(NPT(various_node.fn_expr[0])->IsA(SymbolType::Group));
    GROUPT* pGroupT = GPT(various_node.fn_expr[0]);
    bool bUseCurrentOcc = ( various_node.fn_expr[1] == -1 );
    int iSpecifiedOcc = bUseCurrentOcc ? -1 : ( Evaluate<int>(various_node.fn_expr[1]) - 1 );
    bool bUseBatchLogic = Issamod != ModuleType::Entry;
    bool bVisible = various_node.fn_code == FNSHOWOCC_CODE;

    if( bVisible && various_node.fn_expr[2] != -1 )
        bVisible = EvaluateConditional(various_node.fn_expr[2]);

    if( bUseCurrentOcc )
        iSpecifiedOcc = GetCurOccFromGroup(pGroupT, bUseBatchLogic) - 1;

    if( iSpecifiedOcc >= 0 && iSpecifiedOcc < pGroupT->GetMaxOccs() )
    {
        pGroupT->SetOccVisibility(iSpecifiedOcc,bVisible);
        WindowsDesktopMessage::Post(WM_IMSA_GROUP_OCCS_CHANGE);
        return 1;
    }

    return 0;
}



// 20140422 based on code that was originally exassign ... the caller must delete pTheIndex if non-NULL
VARX* CIntDriver::AssignParser(int iExpr, std::unique_ptr<CNDIndexes>& pTheIndex, int* aIndex)
{
    ASSERT(pTheIndex == nullptr);

    const auto& va_with_size_node = GetNode<Nodes::VariableArgumentsWithSize>(iExpr);
    int iVariable = va_with_size_node.arguments[0];

    CString assignVar = EvalAlphaExprCS(iVariable);

    // parse the text of the item that we are assigning to
    CString fieldName;
    CString occText;
    std::vector<UINT> declaredOccs;

    int lParenPos = assignVar.Find(_T('('));
    int rParenPos = assignVar.Find(_T(')'),lParenPos + 1);

    if( lParenPos >= 0 && rParenPos >= 0 )
    {
        fieldName = assignVar.Mid(0, lParenPos);
        occText = assignVar.Mid(lParenPos + 1, rParenPos - lParenPos - 1);
    }

    else
    {
        fieldName = assignVar;
    }

    fieldName.Trim();

    if( !occText.IsEmpty() )
    {
        int parsingOcc = 1;
        int searchPos = 0;

        while( parsingOcc <= 3 )
        {
            int commaPos = occText.Find(_T(','),searchPos);

            CString thisOccText = commaPos >= 0 ? occText.Mid(searchPos,commaPos) : occText.Mid(searchPos);

            thisOccText.Trim();

            if( thisOccText.IsEmpty() )
                break;

            declaredOccs.emplace_back(_ttoi(thisOccText) - 1);

            if( commaPos < 0 )
                break;

            parsingOcc++;
            searchPos = commaPos + 1;
        }
    }

    // if they didn't specify the number of occurrences in the text but did as optional parameters, use those
    if( declaredOccs.empty() && va_with_size_node.number_arguments > 2 )
    {
        if( va_with_size_node.number_arguments > 3 )
            declaredOccs.emplace_back(Evaluate<unsigned>(va_with_size_node.arguments[3]) - 1);

        declaredOccs.emplace_back(Evaluate<unsigned>(va_with_size_node.arguments[2]) - 1);
    }

    const int symbol_index = m_pEngineArea->SymbolTableSearch(UTF8_TODO::GetUtf8(fieldName), { SymbolType::Variable });

    if( symbol_index == 0 ) // the item wasn't found
        return NULL;

    VART* pVarT = VPT(symbol_index);
    VARX* pVarX = pVarT->GetVarX();

    // if the user didn't specify an index, then we'll use the current occurrence for that index
    memset(aIndex,0,sizeof(aIndex[0]) * DIM_MAXDIM);

    bool bUseBatchLogic = Issamod != ModuleType::Entry;

    GROUPT* pGroupT = pVarT->GetParentGPT();

    if( pGroupT != NULL )
    {
        if( pGroupT->GetDimType() == CDimension::Record )
        {
            aIndex[0] = !declaredOccs.empty() ? declaredOccs.front() :
                                                ( GetCurOccFromGroup(pGroupT, bUseBatchLogic) - 1 );
        }

        else
        {
            if( pGroupT->GetDimType() == CDimension::Item )
            {
                aIndex[1] = ( declaredOccs.size() > 1 ) ? declaredOccs[1] :
                            ( !declaredOccs.empty() )   ? declaredOccs.front() :
                                                          ( GetCurOccFromGroup(pGroupT, bUseBatchLogic) - 1 );
            }

            else if( pGroupT->GetDimType() == CDimension::SubItem )
            {
                aIndex[2] = ( declaredOccs.size() > 1 ) ? declaredOccs[1] :
                            ( !declaredOccs.empty() )   ? declaredOccs.front() :
                                                          ( GetCurOccFromGroup(pGroupT, bUseBatchLogic) - 1 );
            }

            pGroupT = pGroupT->GetParentGPT();

            if( pGroupT && pGroupT->GetDimType() == CDimension::Record )
            {
                aIndex[0] = ( declaredOccs.size() > 1 ) ? declaredOccs.front() :
                                                          ( GetCurOccFromGroup(pGroupT, bUseBatchLogic) - 1 );
            }
        }
    }

    pTheIndex = std::make_unique<CNDIndexes>(ZERO_BASED, aIndex);

    if( !CheckIndexArray(pVarT,*pTheIndex ) )
    {
        pTheIndex.reset();
        return NULL;
    }

    return pVarX;
}


double CIntDriver::exsetvalue(int iExpr) // 20140228
{
    const auto& va_with_size_node = GetNode<Nodes::VariableArgumentsWithSize>(iExpr);

    std::unique_ptr<CNDIndexes> pTheIndex;
    int aIndex[DIM_MAXDIM];
    VARX* pVarX = AssignParser(iExpr, pTheIndex, aIndex);

    if( !pVarX )
        return 0;

    VART* pVarT = pVarX->GetVarT();

    int iValue = va_with_size_node.arguments[1];
    bool bAssignValueIsNumeric = iValue > 0;

    double dRet = 1;

    if( bAssignValueIsNumeric ) // code modified from CIntDriver::excpt
    {
        if( !pVarT->IsNumeric() || !pVarT->IsUsed() )
            dRet = 0;

        else
        {
            double value = evalexpr(iValue);

            bool bOk = SetVarFloatValue(value,pVarX,*pTheIndex);

            if( bOk && pVarX )
            {
                if( Issamod == ModuleType::Entry || pVarX->iRelatedSlot >= 0 )
                {
                    ModuleType eOldMode = Issamod;
                    Issamod = ModuleType::Batch;
                    m_pEngineDriver->prepvar(pVarT,NO_VISUAL_VALUE);
                    Issamod = eOldMode;

                    if( pVarX->iRelatedSlot >= 0 )
                        pVarX->VarxRefreshRelatedData(aIndex);
                }
            }
        }
    }

    else // assigning an alpha expression
    {
        if( pVarT->IsNumeric() )
            dRet = 0;

        else
        {
            CString value = EvalAlphaExprCS(-1 * iValue);

            int iCopyLen = std::min(pVarT->GetLength(),value.GetLength());
            int iBlanksLen = pVarT->GetLength() - iCopyLen;

            TCHAR * pBuffer = pVarT->IsArray() ? GetMultVarAsciiAddr(pVarX,aIndex) : GetSingVarAsciiAddr(pVarX);

            _tmemcpy(pBuffer,value.GetBuffer(),iCopyLen);

            if( iBlanksLen > 0 )
                _tmemset(pBuffer + iCopyLen,BLANK,iBlanksLen);
        }
    }

    return dRet;
}


double CIntDriver::exgetvalue(int iExpr) // 20140422
{
    std::unique_ptr<CNDIndexes> pTheIndex;
    int aIndex[DIM_MAXDIM];
    VARX* pVarX = AssignParser(iExpr, pTheIndex, aIndex);

    if( !pVarX )
        return DEFAULT;

    VART* pVarT = pVarX->GetVarT();

    return pVarT->IsAlpha() ? DEFAULT : GetVarFloatValue(pVarX, *pTheIndex);
}


Engine::Value CIntDriver::ex_getvaluealpha(const int program_index)
{
    std::unique_ptr<CNDIndexes> pTheIndex;
    int aIndex[DIM_MAXDIM];
    VARX* const pVarX = AssignParser(program_index, pTheIndex, aIndex);

    if( pVarX != nullptr )
    {
        VART* const pVarT = pVarX->GetVarT();

        if( pVarT->IsAlpha() )
        {
            const TCHAR* const buffer = pVarT->IsArray() ? GetMultVarAsciiAddr(pVarX, aIndex) : GetSingVarAsciiAddr(pVarX);
            return UTF8_TODO::GetUtf8(std::wstring_view(buffer, pVarT->GetLength()));
        }
    }

    return Engine::Value::Invalid<SharableString>();
}


SharableString CIntDriver::GetValueLabel(const VART* const pVarT, const std::variant<double, SharableString>& value)
{
    ASSERT(pVarT->IsAlpha() == std::holds_alternative<SharableString>(value));

    // three passes to evaluate the label:
    // 1) look at the current value set
    // 2) look at the base value set
    const ValueSet* value_set = pVarT->GetCurrentValueSet();

    for( int pass = 0; pass < 2; ++pass )
    {
        if( pass == 1 )
        {
            const ValueSet* base_value_set = pVarT->GetBaseValueSet();
            value_set = ( base_value_set != value_set ) ? base_value_set :
                                                          nullptr;
        }

        if( value_set == nullptr )
            break;

        const ValueProcessor& value_processor = value_set->GetValueProcessor();

        const DictValue* const dict_value = pVarT->IsAlpha()
            ? value_processor.GetDictValue(*std::get<SharableString>(value))
            : value_processor.GetDictValue(std::get<double>(value));

        if( dict_value != nullptr )
            return UTF8_TODO::GetUtf8(dict_value->GetLabel());
    }

    // 3) format the code nicely
    if( pVarT->IsAlpha() )
    {
        return SharableString(std::get<SharableString>(value)).MakeTrim();
    }

    else
    {
        return ValueSetResponse::FormatValueForDisplay(*pVarT->GetDictItem(), std::get<double>(value));
    }
}


Engine::Value CIntDriver::ex_getvaluelabel(const int program_index)
{
    const auto& va_node = GetNode<Nodes::VariableArguments>(program_index);
    const VART* const pVarT = VPT(va_node.arguments[0]);
    return GetValueLabel(pVarT, EvaluateVariant<SharableString>(pVarT->GetDataType(), va_node.arguments[1]));
}


// used in the accept, show, showarray, and selcase functions
// returns 0 if the user cancels the box
// if pbaSelections is NULL, returns the 1-based index of the single selection;
// if pbaSelections is not NULL, then returns 1 on success with the selected values in pbaSelections
int CIntDriver::SelectDlgHelper_pre77(int iFunCode, const CString& csHeading, const std::vector<std::vector<CString>*>* paData,
                                      const std::vector<CString>* paColumnTitles, std::vector<bool>* pbaSelections,
                                      const std::vector<PortableColor>* row_text_colors)
{
    ASSERT(!UseHtmlDialogs());

    ASSERT(paData != nullptr);

    if( paData->size() == 0 ) // nothing to choose from
    {
        return 0;
    }

    else if( paData->size() > MAX_ITEMS ) // too many rows
    {
        issaerror(MessageType::Error,9010,paData->size(),MAX_ITEMS);
        return 0;
    }

    else if( paData->at(0)->size() > MAX_COLUMNS ) // too many columns
    {
        issaerror(MessageType::Error,9011,paData->at(0)->size(),MAX_COLUMNS);
        return 0;
    }

    ASSERT(paColumnTitles == NULL || paColumnTitles->size() == paData->at(0)->size());
    ASSERT(row_text_colors == nullptr || row_text_colors->size() == paData->size());

    std::shared_ptr<Paradata::OperatorSelectionEvent> operator_selection_event;

    if( Paradata::Logger::IsOpen() )
    {
        Paradata::OperatorSelectionEvent::Source source =
            ( iFunCode == FNACCEPT_CODE )        ? Paradata::OperatorSelectionEvent::Source::Accept :
            ( iFunCode == FNSELCASE_CODE )       ? Paradata::OperatorSelectionEvent::Source::SelCase :
            ( iFunCode == FNSHOW_CODE )          ? Paradata::OperatorSelectionEvent::Source::Show :
            ( iFunCode == LISTFN_SHOW_CODE )     ? Paradata::OperatorSelectionEvent::Source::ListShow :
            ( iFunCode == VALUESETFN_SHOW_CODE ) ? Paradata::OperatorSelectionEvent::Source::ValueSetShow :
                                                   Paradata::OperatorSelectionEvent::Source::ShowArray;

        operator_selection_event = std::make_shared<Paradata::OperatorSelectionEvent>(source);
    }

    // remove newlines from any of the text strings
    cs::non_null_shared_or_raw_ptr<const CString> heading_to_use(&csHeading);
    std::unique_ptr<std::vector<std::unique_ptr<std::vector<CString>>>> modified_vectors;
    std::unique_ptr<std::vector<std::vector<CString>*>> modified_data;

    auto remove_newlines_from_vector = [&](const std::vector<CString>& texts)
    {
        std::vector<CString>* modified_vector = nullptr;

        for( size_t i = 0; i < texts.size(); ++i )
        {
            if( SO::ContainsNewlineCharacter(wstring_view(texts[i])) )
            {
                if( modified_vector == nullptr )
                {
                    if( modified_vectors == nullptr )
                        modified_vectors = std::make_unique<std::vector<std::unique_ptr<std::vector<CString>>>>();

                    modified_vector = modified_vectors->emplace_back(std::make_unique<std::vector<CString>>(texts)).get();
                }

                modified_vector->at(i) = NewlineSubstitutor::NewlineToSpace(texts[i]);
            }
        }

        return modified_vector;
    };

    if( SO::ContainsNewlineCharacter(wstring_view(csHeading)) )
        heading_to_use = std::make_unique<CString>(NewlineSubstitutor::NewlineToSpace(csHeading));

    for( size_t i = 0; i < paData->size(); ++i )
    {
        std::vector<CString>* modified_vector = remove_newlines_from_vector(*paData->at(i));

        if( modified_vector != nullptr )
        {
            if( modified_data == nullptr )
            {
                modified_data = std::make_unique<std::vector<std::vector<CString>*>>(*paData);
                paData = modified_data.get();
            }

            modified_data->at(i) = modified_vector;
        }
    }

    if( paColumnTitles != nullptr )
    {
        const std::vector<CString>* modified_vector = remove_newlines_from_vector(*paColumnTitles);

        if( modified_vector != nullptr )
            paColumnTitles = modified_vector;
    }


    int iRet = 0;

#ifdef WIN_DESKTOP
    bool bSingleSelection = ( pbaSelections == NULL );

    // set up the dialog options
    CSelectListCtrlOptions cOptions;

    cOptions.m_bUseTitle = !heading_to_use->IsEmpty();
    cOptions.m_csTitle = *heading_to_use;
    cOptions.m_iMinMark = 0;
    cOptions.m_iMaxMark = bSingleSelection ? 1 : -1;
    cOptions.m_bUseColTitle = ( paColumnTitles != NULL );
    cOptions.m_bHighLightFirst = bSingleSelection;
    cOptions.m_bEndOnLimits = bSingleSelection;
    cOptions.m_bHasStickyCtrl = !bSingleSelection;
    cOptions.m_iKeyBuffMaxLen = 1;
    cOptions.m_bUseParentWindowLimitsForSizing = ( Issamod == ModuleType::Entry );

    if( bSingleSelection )
        pbaSelections = new std::vector<bool>(paData->size(),false);

    ASSERT(paData->size() == pbaSelections->size());

    cOptions.m_paData = paData;
    cOptions.m_paColumnTitles = paColumnTitles;
    cOptions.m_pbaSelections = pbaSelections;
    cOptions.m_rowTextColors = row_text_colors;


    // refresh the screen so that if logic changed any of the fields on the form, they will appear correctly
    CWnd* pMainWnd = AfxGetApp()->GetMainWnd();

    pMainWnd->SendMessage(WM_IMSA_CSENTRY_REFRESH_DATA);

    // launch the dialog
    CSelectDlg selectDlg(pMainWnd);

    if( selectDlg.DoModal(&cOptions) == IDOK )
    {
        if( bSingleSelection )
        {
            for( std::vector<byte>::size_type i = 0; i < pbaSelections->size(); i++ )
            {
                if( pbaSelections->at(i) )
                {
                    iRet = i + 1;
                    break;
                }
            }
        }

        else
            iRet = 1; // for multiple selections, the selected values are in pbaSelections
    }

    if( bSingleSelection )
        delete pbaSelections;

#else //!WIN_DESKTOP
    if( iFunCode == FNACCEPT_CODE )
    {
        iRet = PlatformInterface::GetInstance()->GetApplicationInterface()->ShowChoiceDialog(csHeading, *paData);
    }

    else if( iFunCode == FNSHOW_CODE || iFunCode == FNSHOWARRAY_CODE || iFunCode == LISTFN_SHOW_CODE || iFunCode == VALUESETFN_SHOW_CODE )
    {
        iRet = PlatformInterface::GetInstance()->GetApplicationInterface()->ShowShowDialog(paColumnTitles, row_text_colors, *paData, csHeading);
    }

    else if( iFunCode == FNSELCASE_CODE )
    {
        iRet = PlatformInterface::GetInstance()->GetApplicationInterface()->ShowSelcaseDialog(paColumnTitles, *paData, csHeading, pbaSelections);
    }

 //   else
 //       ASSERT(0);
#endif

    if( operator_selection_event != nullptr )
    {
        SharableString selected_text = ( iRet == 0 ) ? SharableString() :
                                                       UTF8_TODO::GetUtf8(paData->at(iRet - 1)->at(0));
        operator_selection_event->SetPostSelectionValues(iRet, std::move(selected_text), true);
        m_paradataDriver->RegisterAndLogEvent(operator_selection_event);
    }

    return iRet;
}

double CIntDriver::extblcoord(int iExpr)
{
    const FNTC_NODE* table_coord_node = (FNTC_NODE*)PPT(iExpr);
    size_t dimension_index = ( table_coord_node->fn_code == FNTBLROW_CODE ) ? 0 :
                             ( table_coord_node->fn_code == FNTBLCOL_CODE ) ? 1 :
                                                                              2;

    const Symbol* symbol = NPT(table_coord_node->iCtab);

    // these functions can be used to get the dimensions of an array
    if( symbol->IsA(SymbolType::Array) )
    {
        const LogicArray* logic_array = assert_cast<const LogicArray*>(symbol);

        if( dimension_index < logic_array->GetNumberDimensions() )
        {
            return logic_array->GetDimension(dimension_index) - 1;
        }

        else
        {
            return DEFAULT;
        }
    }

    // or for the original purpose related to crosstabs
    return tblcoord(iExpr, dimension_index);
}
