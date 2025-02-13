//--------------------------------------------------------------------------
//
//      INTEXPR.cpp   functions to interpret expressions
//
//  History:    Date       Author   Comment
//              ---------------------------
//              16 Nov 01   TNC     Changing tests returning Default to return False
//              24 Jul 04   rcl     Adds MVAR_CONSTANT handling for mvarGetSubindexes
//
//--------------------------------------------------------------------------
#include "StandardSystemIncludes.h"
#include "INTERPRE.H"
#include "Engine.h"
#include "Ctab.h"
#include "EngineStringComparer.h"
#include "helper.h"
#include <zEngineO/Block.h>
#include <zEngineO/List.h>
#include <zEngineO/ValueSet.h>
#include <zEngineO/Versioning.h>
#include <zEngineO/WorkVariable.h>
#include <zEngineO/Nodes/Switch.h>
#include <zDictO/ValueProcessor.h>
#include <Zissalib/groupt2.h>


/*--------------------------------------------------------------------------*/
/*  node helpers                                                            */
/*--------------------------------------------------------------------------*/

const Nodes::List& CIntDriver::GetListNode(int program_index) const
{
    if( program_index != -1 )
    {
        return GetNode<Nodes::List>(program_index);
    }

    else
    {
        // return a dummy empty list node
        ASSERT(Versioning::PredatesCompiledLogicVersion(Serializer::Iteration_8_0_000_1));
        return GetOptionalListNode(-1);
    }
}


const Nodes::List& CIntDriver::GetOptionalListNode(int program_index) const
{
    if( program_index != -1 )
    {
        return GetNode<Nodes::List>(program_index);
    }

    else
    {
        // return a dummy empty list node
        const static Nodes::List list_node = { 0, 0 };
        return list_node;
    }
}


std::vector<int> CIntDriver::GetListNodeContents(int program_index) const
{
    const Nodes::List& list_node = GetListNode(program_index);
    return std::vector<int>(list_node.elements, list_node.elements + list_node.number_elements);
}



/*--------------------------------------------------------------------------*/
/*  exnumericconstant: value of a constant                                  */
/*--------------------------------------------------------------------------*/
double CIntDriver::exnumericconstant(int iExpr)
{
    const auto& const_node = GetNode<CONST_NODE>(iExpr);
    return GetNumericConstant(const_node.const_index);
}


/*--------------------------------------------------------------------------*/
/*  exsvar: value of single variable                                        */
/*--------------------------------------------------------------------------*/

double CIntDriver::exsvar(int iExpr)
{
    SVAR_NODE* ptrvar = (SVAR_NODE*)PPT(iExpr);
    VARX* pVarX = VPX(ptrvar->m_iVarIndex);
    return svarvalue(pVarX);
}


/*--------------------------------------------------------------------------*/
/*  exworkvariable: value of work variable                                  */
/*--------------------------------------------------------------------------*/
double CIntDriver::exworkvariable(int iExpr)
{
    const auto& svar_node = GetNode<SVAR_NODE>(iExpr);
    return GetSymbolWorkVariable(svar_node.m_iVarIndex).GetValue();
}


//--------------------------------------------------------------------------
//  exmvar: value of multiple variable
//--------------------------------------------------------------------------
double CIntDriver::exmvar(int iExpr)
{
    MVAR_NODE*  ptrvar = (MVAR_NODE*)PPT(iExpr);

    return exmvar( ptrvar );                    // rcl, Jul 22, 2004
}

double CIntDriver::exmvar( MVAR_NODE* ptrvar )  // rcl, Jul 22, 2004
{
    ASSERT( ptrvar != 0 );
    int         iThisVar;
    UserIndexesArray dIndex;

    iThisVar     = ptrvar->m_iVarIndex;

    VART*   pVarT = VPT(iThisVar);
    VARX*   pVarX = pVarT->GetVarX();

    mvarGetSubindexes( ptrvar, dIndex );

    return( mvarvalue( pVarX, dIndex ) );
}
// RHC Sep 20, 2001


void CIntDriver::GetCurrentVarSubIndexes( int iSymVar, CNDIndexes& dIndex ) {
    bool bZeroBased = dIndex.isZeroBased();
    ASSERT( Issamod==ModuleType::Batch );
    ASSERT( NPT(iSymVar)->IsA(SymbolType::Variable) );

    VART*   pVarT=VPT(iSymVar);
    GROUPT* pGroupT=NULL;

    ASSERT( pVarT->GetNumDim() <= DIM_MAXDIM );

    int     iDim;

    for( iDim = 0; iDim < DIM_MAXDIM; iDim++ )
        dIndex.setIndexValue(iDim, 0 );

    int iWhichDimsToUse = 0;
    int aWhich[DIM_MAXDIM] = { USE_DIM_1, USE_DIM_2, USE_DIM_3 };

    for( iDim = 0; iDim < pVarT->GetNumDim(); iDim++ ) {
        pGroupT = (pGroupT==NULL) ? LocalGetParent( pVarT ) : LocalGetParent( pGroupT );

        if( pGroupT ) {
            CDimension::VDimType vType = pGroupT->GetDimType();
            ASSERT( vType == CDimension::Record || vType == CDimension::Item ||
                                vType == CDimension::SubItem );
            int iOcc=pGroupT->GetCurrentExOccurrence();
            if( bZeroBased && iOcc > 0 ) iOcc--; // zero based
            dIndex.setIndexValue( vType, iOcc );

            iWhichDimsToUse |= aWhich[ pVarT->GetDimType(iDim) ];
        }
    }
    dIndex.setAsInitialized();
    dIndex.specifyIndexesUsed(iWhichDimsToUse);
}

void CIntDriver::GetCurrentGroupSubIndexes( int iSymGroup, CNDIndexes& dIndex ) {
    bool bZeroBased = dIndex.isZeroBased();

    ASSERT( Issamod==ModuleType::Batch );
    ASSERT( NPT(iSymGroup)->IsA(SymbolType::Group) );

    GROUPT* pGroupT=GPT(iSymGroup);

    ASSERT( pGroupT->GetNumDim() <= DIM_MAXDIM );

    int     iDim;

    for( iDim = 0; iDim < DIM_MAXDIM; iDim++ )
        dIndex.setIndexValue(iDim, 0 );

    int iWhichDimsToUse = 0;
    int aWhich[DIM_MAXDIM] = { USE_DIM_1, USE_DIM_2, USE_DIM_3 };

    for( iDim = 0; pGroupT && iDim < pGroupT->GetNumDim(); iDim++ ) {
        CDimension::VDimType vType = pGroupT->GetDimType();

        ASSERT( vType == CDimension::Record || vType == CDimension::Item ||
                                vType == CDimension::SubItem );

        int iOcc=pGroupT->GetCurrentExOccurrence();
        if( bZeroBased && iOcc > 0 ) iOcc--; // zero based
        dIndex.setIndexValue( vType, iOcc );

        ASSERT( pGroupT->GetDimType() >= 0 && pGroupT->GetDimType() < 3 );

        iWhichDimsToUse |= aWhich[ pGroupT->GetDimType() ];

        pGroupT = LocalGetParent( pGroupT );
    }
    dIndex.setAsInitialized();
    dIndex.specifyIndexesUsed(iWhichDimsToUse);
}

bool CIntDriver::hasSubitemInGroup(VART* pItemVarT, GROUPT* pGroup)
{
    VART*  pSubItem = pItemVarT->GetNextSubItem();
    while (pSubItem != NULL) {
        if (pSubItem->GetOwnerGPT() == pGroup)
            return true;
    }
    return false;
}

bool CIntDriver::isParentItemInGroup(VART* pSubitemVarT, GROUPT* pGroupT)
{
    VART* pParentVarT = pSubitemVarT->GetOwnerVarT();
    return pParentVarT->GetOwnerGPT() == pGroupT;
}

// RHC END Sep 20, 2001
// dIndexType can be 'E' (Expresion) 'G' (Group), 'M' (Multiple Relation), 'R' (Relation) or ' ' (None)
void CIntDriver::mvarGetSubindexes( const MVAR_NODE *ptrvar, double* dIndex, int *dIndexType )
{
    ASSERT( ptrvar != 0 );
    int     iSymVar, iDim;
    VART    *pVarT;
    RELT    *pRelT;
    RELATED *pRelated, aRelated;

    iSymVar = ptrvar->m_iVarIndex;
    pVarT = VPT( iSymVar );

    // this should rarely happen, only potentially in an OnSystemMessage call, so just change
    // the execution symbol to the symbol of the variable
    if( m_iExSymbol <= 0 )
        m_iExSymbol = iSymVar;


    //////////////////////////////////////////////////////////////////////////
    // Use fixed dimensions first
    for( iDim = 0; iDim < m_iExFixedDimensions; iDim++ )
    {
        dIndex[iDim] = m_aFixedDimensions.getIndexValue( iDim );
    }
    //////////////////////////////////////////////////////////////////////////

    ASSERT( pVarT->GetNumDim() <= DIM_MAXDIM );
    for( /*iDim = 0*/; iDim < pVarT->GetNumDim(); iDim++ ) {

        // Constant MVAR_CONSTANT added to speed up interpretation // rcl, Jul 24, 2004
        int iExprType   = ptrvar->m_iVarSubindexType[iDim];
        int iExpression = ptrvar->m_iVarSubindexExpr[iDim];


        if( iExprType == MVAR_CONSTANT || iExprType == MVAR_EXPR )
        {
            if( dIndexType != NULL )
                *(dIndexType+iDim) = 'E';

            if( iExprType == MVAR_CONSTANT )
                dIndex[iDim] = (double) iExpression;
            else
            {
                ASSERT( iExpression != 0 );
                dIndex[iDim] = evalexpr( iExpression );
            }

        }
        else
        if( iExprType == MVAR_GROUP ) {
                dIndex[iDim] = 0.;

                // RHF INIC Jan 04, 2002
                int iSymGroup = ptrvar->m_iVarSubindexExpr[iDim];


                // RHF COM Feb 25, 2004 if( m_iExOccur > 0 && (iSymGroup == m_iExGroup || pVarT->GetOwnerSec() == -m_iExGroup) && m_iExDim == iDim ) {}
                if( m_iExOccur > 0 &&
                    m_iExDim == iDim &&
                    (iSymGroup == m_iExGroup || m_iExGroup < 0 && (pVarT->GetOwnerSec() == -m_iExGroup || pVarT->GetOwnerSymItem() == -m_iExGroup || iSymVar == -m_iExGroup ) ) ) {
                    dIndex[iDim] = m_iExOccur;
                    continue;
                }
                // RHF END Jan 04, 2002

                // check ForStack for corresponding entry  - if found use it

                int iStack;

                for( iStack = m_iForStackNext - 1; iStack >= 0; iStack-- )
                {
                    if( ForStack[iStack].forType == 'G' ) {
                        if( dIndexType != NULL )
                            *(dIndexType+iDim) = 'G';
                        if( ForStack[iStack].forGrpIdx == iSymGroup ) {
                            dIndex[iDim] = GetSymbolWorkVariable(ForStack[iStack].forVarIdx).GetValue();
                            break;
                        }
                    }
                    else { // Relation Stack Entry
                        ASSERT( ForStack[iStack].forType == 'R' );
                        pRelT = RLT( ForStack[iStack].forRelIdx );
                        pRelated = pRelT->GetRelated( &aRelated, iSymVar, pVarT->GetDimType( iDim ) );
                        if( pRelated != NULL ) { // variable reached by relation
                            if( pRelated->iRelatedRelationType == USE_WHERE_RELATION_MULTIPLE ) {
                                // Runtime Error
                                // RHF INIC Jul 16, 2002
                                if( !m_bAllowMultipleRelation )
                                    issaerror( MessageType::Error, 33200, pVarT->GetName().c_str() );
                                dIndex[iDim] = GetSymbolWorkVariable(pRelated->iRelatedWVarIdx).GetValue();
                                // RHF END Jul 16, 2002

                                if( dIndexType != NULL )
                                    *(dIndexType+iDim) = 'M';
                            }
                            else {
                                if( dIndexType != NULL )
                                    *(dIndexType+iDim) = 'R';
                                dIndex[iDim] = GetSymbolWorkVariable(pRelated->iRelatedWVarIdx).GetValue();
                            }
                            break;
                        }
                    }
                }

                if( iStack >= 0 )
                    continue;

                if( dIndexType != NULL )
                    *(dIndexType+iDim) = ' ';

                // RHF COM Aug 07, 2000 dIndex[iDim] = GPT( ptrvar->m_iVarSubindexExpr[iDim] )->GetCurrentOccurrences(); // check in Batch ?

                /* RHF COM INIC Jan 04, 2002 Moved Up!!
                int     iSymGroup=ptrvar->m_iVarSubindexExpr[iDim];

                  // RHF INIC Aug 17, 2000 Count and related functions works over the last dimension
                  if( m_iExOccur > 0 && iSymGroup == m_iExGroup && m_iExDim == iDim )
                  dIndex[iDim] = m_iExOccur;
                  else
                  // RHF END Aug 17, 2000
                RHF COM END Jan 04, 2002  Moved Up!!*/

                // RHF INIC Jan 08, 2003
                GROUPT* pGroupT = GPT(iSymGroup);
                GROUPT::Source eGrpSrc = GROUPT::Source::DcfFile;

                // Hidden group //having form file
                bool    bUseSectionOcc = (pGroupT->GetSource() == eGrpSrc);
                //&&  (pGroupT->GetFlow() != NULL && pGroupT->GetFlow()->GetFormFile()!=NULL) );

                const Symbol* current_symbol = NPT(m_iExSymbol);
                VART* pCurrentVarT = nullptr;

                if( current_symbol->IsA(SymbolType::Variable) )
                {
                    pCurrentVarT = (VART*)current_symbol;
                }

                else if( current_symbol->IsA(SymbolType::Block) )
                {
                    pCurrentVarT = assert_cast<const EngineBlock*>(current_symbol)->GetFirstVarT();
                }

                GROUPT* pCurrentGroupT = ( pCurrentVarT != nullptr ) ? pCurrentVarT->GetOwnerGPT() : nullptr;


                bool bIsSubItem = pVarT->GetOwnerSymItem() > 0;

                if( bUseSectionOcc ) {

                    // If the variable being evaluated is a subitem and the parent item is the current proc then use parent item occurrence
                    // or if the variable being evaluated is the parent item and the current proc is a subitem use the child occurrence
                    if ((Issamod == ModuleType::Entry) && NPT(m_iExSymbol)->IsA(SymbolType::Variable) && pCurrentGroupT &&
                        ((bIsSubItem && isParentItemInGroup(pVarT, pCurrentGroupT)) || (!bIsSubItem && hasSubitemInGroup(pVarT, pCurrentGroupT)))
                        && pGroupT->GetDimType() == pCurrentGroupT->GetDimType()) {
                        dIndex[iDim] = pCurrentGroupT->GetCurrentExOccurrence();
                    }
                    else {

                        SECT*       pSecT = pVarT->GetSPT();
                        int         iGroupNum = 0;
                        GROUPT*     pGroupTAux;
                        int         iSectionOcc = 0;

                        while ((pGroupTAux = pSecT->GetGroup(iGroupNum)) != NULL) {
                            if (pGroupTAux->GetSource() == GROUPT::Source::FrmFile)
                            {
                                iSectionOcc = std::max(iSectionOcc, pGroupTAux->GetCurrentExOccurrence());
                            }

                            iGroupNum++;
                        }

                        dIndex[iDim] = iSectionOcc;
                    }
                }
                else
                {
                    // if the item is on a record that has been split into multiple groups, and if the current
                    // group is part of that record, use the current group's occurrence instead of the item's
                    // group's occurrence; this change eliminates the need to write things like NAME(curocc())
                    if( Issamod == ModuleType::Entry )
                    {
                        if( pCurrentVarT != nullptr && pGroupT != pCurrentGroupT && pVarT->GetOwnerSec() == pCurrentVarT->GetOwnerSec() )
                        {
                            if( pGroupT->GetDimType() == pCurrentGroupT->GetDimType() )
                                pGroupT = pCurrentGroupT;
                        }
                    }

                    dIndex[iDim] = pGroupT->GetCurrentExOccurrence();
                }
                // RHF END Jan 08, 2003

                // RHF COM Jan 08, 2003  dIndex[iDim] = GPT(iSymGroup)->GetCurrentExOccurrence(); // RHF Aug 07, 2000
        }
        else
            dIndex[iDim] = 0.;
      }

      for( ; iDim < DIM_MAXDIM; iDim++ )
          // RHF COM Aug 11, 2000 dIndex[iDim] = -1;
          dIndex[iDim] = 0;// RHF Aug 11, 2000

}


void CIntDriver::grpGetSubindexes( GRP_NODE* pgrp, double* dIndex )
{
    grpGetSubindexes(GPT(pgrp->m_iGrpIndex), pgrp, dIndex);
}

void CIntDriver::grpGetSubindexes( GROUPT* pGrpT, GRP_NODE* pgrp, double* dIndex ) {

    int iDim = 0;

    for( ; iDim < pGrpT->GetNumDim(); iDim++ )
    {
        int iExprType = pgrp->m_iGrpSubindexType[iDim];

        if( iExprType == MVAR_CONSTANT ) // rcl, Jul 26, 2004
            dIndex[iDim] = pgrp->m_iGrpSubindexExpr[iDim];

        else if( iExprType == MVAR_EXPR )
            dIndex[iDim] = evalexpr( pgrp->m_iGrpSubindexExpr[iDim] );

        else if( iExprType == MVAR_GROUP )
        {
            int iSymGroup = pgrp->m_iGrpSubindexExpr[iDim];

            // RHF Aug 17, 2000 Count and related functions works over the last dimension
            if( m_iExOccur > 0 && (iSymGroup == m_iExGroup ||  m_iExGroup < 0 && pGrpT->FindRecord( SPT(-m_iExGroup)) >= 0 ) && m_iExDim == iDim )
                dIndex[iDim] = m_iExOccur;

            else
            {
                GROUPT* pGroupT = GPT(iSymGroup);

                // if currently on a field/block that shares the same record as the block being evaluated,
                // use the index of the current field/block
                if( pgrp->m_iGrpType == BLOCK_CODE && m_iExSymbol > 0 )
                {
                    const Symbol* current_symbol = NPT(m_iExSymbol);

                    if( Issamod == ModuleType::Entry && current_symbol->IsOneOf(SymbolType::Block, SymbolType::Variable) )
                    {
                        const VART* pCurrentVarT = current_symbol->IsA(SymbolType::Variable) ? ((VART*)current_symbol) :
                                                                                               assert_cast<const EngineBlock*>(current_symbol)->GetFirstVarT();
                        const VART* pGroupVarT = GetSymbolEngineBlock(pgrp->m_iGrpIndex).GetFirstVarT();

                        if( pCurrentVarT != nullptr && pGroupVarT != nullptr && pCurrentVarT->GetOwnerSec() == pGroupVarT->GetOwnerSec() )
                        {
                            pGroupT = current_symbol->IsA(SymbolType::Variable) ? ((VART*)current_symbol)->GetOwnerGPT() :
                                                                                  assert_cast<const EngineBlock*>(current_symbol)->GetGroupT();
                        }
                    }
                }

                dIndex[iDim] = pGroupT->GetCurrentExOccurrence();
            }
        }

        else
            dIndex[iDim] = 0.;
    }

    for( ; iDim < DIM_MAXDIM; iDim++ )
        // RHF COM Aug 11, 2000 dIndex[iDim] = -1;
        dIndex[iDim] = 0;// RHF Aug 11, 2000
}


double CIntDriver::extvar(int iExpr)
{
#ifdef WIN_DESKTOP
    TVAR_NODE*  ptrvar = (TVAR_NODE*)PPT(iExpr);
    CTAB*       pCtab = XPT( ptrvar->tvar_index );
    int         iIndex[3];

    for( int i = 0; i < 3; i++ ) {
        iIndex[i] = 0;
        if( ptrvar->tvar_exprindex[i] >= 0 )
            iIndex[i] = (int) evalexpr( ptrvar->tvar_exprindex[i] );
    }

    // RHF COM Jul 31, 2001 return( tabvalue( pCtab, iIndex[0], iIndex[1], iIndex[2] ) );
    return( pCtab->m_pAcum.GetDoubleValue( iIndex[0], iIndex[1], iIndex[2] ) );
#else
    // crosstabs don't exist in the portable environments
    ASSERT(false);
    return DEFAULT;
#endif
}


/* Fix problem with in and special values.

AND, Some false->false. All true->true
---
SPECIAL and TRUE        =SPECIAL
SPECIAL and FALSE       =FALSE
TRUE    and SPECIAL     =SPECIAL
FALSE   and SPECIAL     =FALSE

FALSE   and FALSE   =FALSE
FALSE   and TRUE    =FALSE
TRUE    and FALSE   =FALSE
TRUE    and TRUE    =TRUE

OR, Some true->true, All false->false
--
SPECIAL or  TRUE        =TRUE
SPECIAL or  FALSE       =SPECIAL
TRUE    or  SPECIAL     =TRUE
FALSE   or  SPECIAL     =SPECIAL

FALSE   or  FALSE   =FALSE
FALSE   or  TRUE    =TRUE
TRUE    or  FALSE   =TRUE
TRUE    or  TRUE    =TRUE
*/


bool CIntDriver::InWorker(int in_node_expression, const std::variant<double, std::wstring>& value,
                          const std::function<const std::variant<double, std::wstring>&(int)>* expression_evaluator/* = nullptr*/)
{
    const Nodes::In::Entry* in_node_entry = &GetNode<Nodes::In::Entry>(in_node_expression);
    const bool is_alpha_expression = std::holds_alternative<std::wstring>(value);

    while( in_node_entry != nullptr )
    {
        bool in_range = false;

        // using a list or a value set
        if( in_node_entry->expression_low < 0 )
        {
            const Symbol& symbol = NPT_Ref(-1 * in_node_entry->expression_low);

            if( symbol.IsA(SymbolType::List) )
            {
                const LogicList& logic_list = assert_cast<const LogicList&>(symbol);
                in_range = is_alpha_expression ? logic_list.Contains(std::get<std::wstring>(value)) :
                                                 logic_list.Contains(std::get<double>(value));
            }

            else
            {
                const ValueSet& value_set = assert_cast<const ValueSet&>(symbol);
                const ValueProcessor& value_processor = value_set.GetValueProcessor();
                in_range = is_alpha_expression ? value_processor.IsValid(WS2CS(std::get<std::wstring>(value))) :
                                                 value_processor.IsValid(std::get<double>(value));
            }
        }

        // or a range
        else
        {
            const bool range_has_two_values = ( in_node_entry->expression_high != -1 );

            // alpha
            if( is_alpha_expression )
            {
                auto get_string_comparison = [&](int expression)
                {
                    const std::wstring rhs = ( expression_evaluator == nullptr ) ? EvalAlphaExpr(expression) :
                                                                                   std::get<std::wstring>((*expression_evaluator)(expression));

                    return m_usingLogicSettingsV0 ? EngineStringComparer::V0::Compare(std::get<std::wstring>(value), rhs) :
                                                    std::get<std::wstring>(value).compare(rhs);
                };

                const int alpha_comparison = get_string_comparison(in_node_entry->expression_low);
                in_range = ( alpha_comparison == 0 );

                if( !in_range && range_has_two_values && alpha_comparison > 0 )
                    in_range = ( get_string_comparison(in_node_entry->expression_high) <= 0 );
            }

            // numeric
            else
            {
                auto get_number = [&](int expression) -> double
                {
                    return ( expression_evaluator == nullptr ) ? evalexpr(expression) :
                                                                 std::get<double>((*expression_evaluator)(expression));
                };

                const double low_value = get_number(in_node_entry->expression_low);

                if( range_has_two_values )
                {
                    const double high_value = get_number(in_node_entry->expression_high);

                    if( !IsSpecial(std::get<double>(value)) )
                    {
                        in_range = ( std::get<double>(value) >= low_value &&
                                     std::get<double>(value) <= high_value );
                    }

                    // handle the "special" alias that gets compiled as a range
                    else if( low_value == SpecialValues::SmallestSpecialValue() && high_value == SpecialValues::LargestSpecialValue() )
                    {
                        in_range = true;
                    }
                }

                else
                {
                    in_range = ( std::get<double>(value) == low_value );
                }
            }
        }

        if( in_range )
            return true;

        in_node_entry = ( in_node_entry->next_entry_index != -1 ) ? &GetNode<Nodes::In::Entry>(in_node_entry->next_entry_index) :
                                                                      nullptr;
    }

    return false;
}


double CIntDriver::exin(int iExpr)
{
    if( Versioning::MeetsCompiledLogicVersion(Serializer::Iteration_8_0_000_1) )
    {
        const auto& in_node = GetNode<Nodes::In>(iExpr);
        return InWorker(in_node.right_expr, EvaluateVariantExpression(in_node.data_type, in_node.left_expr));
    }

    else
    {
        const auto& operator_node = GetNode<Nodes::Operator>(iExpr);
        const bool numeric = ( GetNode<Nodes::Operator>(operator_node.left_expr).oper != CHOBJ_CODE );
        return InWorker(operator_node.right_expr, EvaluateVariantExpression(numeric, operator_node.left_expr));
    }
}


double CIntDriver::exinsert_delete(int iExpr)
{
    const FNINS_NODE* func_node = (FNINS_NODE*)PPT(iExpr);
    bool insert_function = ( func_node->fn_code == FNINSERT_CODE );
    bool use_comma_syntax = ( func_node->group_node < 0 );
    GRP_NODE* group_node = (GRP_NODE*)PPT(std::abs(func_node->group_node));
    GROUPT* pGroupT = GPT(group_node->m_iGrpIndex);
    int first_occurrence;
    int last_occurrence;

    if( use_comma_syntax )
    {
        first_occurrence = evalexpr<int>(func_node->first_occurrence);

        if( func_node->last_occurrence == -1 )
        {
            last_occurrence = first_occurrence;
        }

        else
        {
            last_occurrence = evalexpr<int>(func_node->last_occurrence);

            // make sure that the last occurrence is greater than the first occurrence
            if( last_occurrence < first_occurrence )
                std::swap(first_occurrence, last_occurrence);
        }
    }

    else
    {
        if( pGroupT->GetDimType() == CDimension::VDimType::VoidDim )
            first_occurrence = 1;

        else
        {
            double dIndex[DIM_MAXDIM];
            grpGetSubindexes(group_node, dIndex);
            first_occurrence = (int)dIndex[0];
        }

        last_occurrence = first_occurrence;
    }

    if( insert_function )
    {
        for( int occurrence = first_occurrence; occurrence <= last_occurrence; ++occurrence )
        {
            if( !ExInsertWorker(pGroupT, occurrence) )
                return 0;
        }
    }

    else
    {
        for( int occurrence = last_occurrence; occurrence >= first_occurrence; --occurrence )
        {
            if( !ExDeleteWorker(pGroupT, occurrence) )
                return 0;
        }
    }

    return 1;
}


bool CIntDriver::ExInsertWorker(GROUPT* pGroupT, int occurrence)
{
    int data_occurrences = pGroupT->GetDataOccurrences();

    // Conditions ---
    if (data_occurrences >= pGroupT->GetMaxOccs() )
    {
        int iMaxOcc = pGroupT->GetMaxOccs();
        issaerror( MessageType::Error, 1083, pGroupT->GetName().c_str(), iMaxOcc );
        return false;
    }

    // run the internal expression
    if ( occurrence < 1 )
    {
        issaerror( MessageType::Error, 1081, pGroupT->GetName().c_str(), occurrence );
        return false;
    }

    if( occurrence > ( data_occurrences + 1 ) )
    {
        issaerror( MessageType::Error, 1082, pGroupT->GetName().c_str(), occurrence, data_occurrences );
        return false;
    }

    bool binc = false;
    if (occurrence <= pGroupT->GetExOccur())
        binc = true;

    int         iFromOcc, iUntilOcc, iMaxOcc;
    // Get Loop Index
    int     iLoopIndex;

    iLoopIndex = pGroupT->GetDimType();

    int     iOccur  = 1 ;//pDeFld->GetIndex(0);
    iFromOcc = occurrence - 1;

    iUntilOcc = data_occurrences;
    iMaxOcc   = data_occurrences;

    bool bRet;

    CLoopControl   cLoopControl( iLoopIndex, iFromOcc, iUntilOcc, iMaxOcc  );
    CNDIndexes theIndex( ZERO_BASED, DEFAULT_INDEXES );

    bool set = false;

    for( int iItem=0;  iItem < pGroupT->GetNumItems(); iItem++ )
    {
        int iSymb ;
        if( ( iSymb = pGroupT->GetItemSymbol( iItem ) ) == 0 )
            break;

        if (pGroupT->m_pEngineArea->GetTypeGRorVA( iSymb ) == SymbolType::Variable) // ... member is an item or sub-item
        {
            VART*       pVarT=VPT(iSymb);
            VARX*       pVarX=pVarT->GetVarX();
            //      pVarX->m_iFloat = 0;
            if (!set )
            {
                pVarX->BuildIntParentIndexes( theIndex, iOccur );   // TRANSITION ?
                set = true;
            }
        }
    }

    bRet = pGroupT->InsertOcc( theIndex, &cLoopControl );

    pGroupT->SetTotalOccurrences(data_occurrences+1);
    pGroupT->SetDataOccurrences(data_occurrences+1);

    if (binc )
    {
        pGroupT->SetExOccur( pGroupT->GetExOccur() + 1 );
        pGroupT->SetCurrentOccurrences(pGroupT->GetExOccur()+1);
    }

    pGroupT->SetHighOccurrences(std::max(pGroupT->GetHighOccurrences(),data_occurrences+1)); // BMD 14 Aug 2003

    return 1;
}

bool CIntDriver::ExDeleteWorker(GROUPT* pGroupT, int occurrence)
{
    // Conditions ---
    int data_occurrences = pGroupT->GetDataOccurrences();

    // run the internal expression
    if( occurrence < 1 || occurrence > data_occurrences )
    {
        issaerror(MessageType::Error, 1085, pGroupT->GetName().c_str(), occurrence, data_occurrences);
        return false;
    }

    bool binc = false;
    if (occurrence <= pGroupT->GetExOccur())
        binc = true;

    //No occ tree double      dIndex[DIM_MAXDIM];
    //No occ treegrpGetSubindexes( pgrpNode, dIndex );
    //No occ treedOcc = dIndex[0];

//              int     iLoopIndex = pGroupT->GetCurrentExOccurrence();
    int         iFromOcc, iUntilOcc, iMaxOcc;
    // Get Loop Index
    int     iOccur  = 1 ;//pDeFld->GetIndex(0);
    iFromOcc = occurrence - 1;

    //              iFromOcc = iLoopIndex;
    //iUntilOcc = pGroupT->GetMaxOccs() - 1;
    //iMaxOcc   = pGroupT->GetMaxOccs() - 1;
    iUntilOcc = data_occurrences-1;
    iMaxOcc   = data_occurrences-1;

                //C_MoveOcc
    bool bRet;
    //                      ((CBatchDriver*)m_pEngineDriver)->C_MoveOcc(ret,GROUPT::GrouptInsertOcc );
    CLoopControl cLoopControl( pGroupT->GetDimType(), iFromOcc, iUntilOcc, iMaxOcc  );
    CNDIndexes theIndex( ZERO_BASED, DEFAULT_INDEXES );

    bool set = false;

//      return 0;

    for( int iItem=0;  iItem < pGroupT->GetNumItems(); iItem++ )
    {
        int iSymb ;
        if( ( iSymb = pGroupT->GetItemSymbol( iItem ) ) == 0 )
            break;

        if (pGroupT->m_pEngineArea->GetTypeGRorVA( iSymb ) == SymbolType::Variable) // ... member is an item or sub-item
        {
            VART*       pVarT=VPT(iSymb);
            VARX*       pVarX=pVarT->GetVarX();
            //      pVarX->m_iFloat = 0;
            if (!set )
            {
                pVarX->BuildIntParentIndexes( theIndex, iOccur );   // TRANSITION ?
                set = true;
            }

        }
    }

    if (data_occurrences == pGroupT->GetMinOccs())
    {
        issaerror( MessageType::Error, 1086, pGroupT->GetName().c_str() );
        return false;
    }

    bRet = pGroupT->DeleteOcc( theIndex, &cLoopControl );
    /*      if (bRet == 0)
    {
    issaerror( MessageType::Error, 1086, data_occurrences, occurrence );
    return( double( 0));
                }*/

    //      int         iNewLoopIndex=pGroupT->GetDimType();
    if (pGroupT->GetCurrentOccurrences() > data_occurrences-1)
        pGroupT->SetCurrentOccurrences(data_occurrences-1);
    else
        pGroupT->SetCurrentOccurrences(pGroupT->GetCurrentOccurrences());

    pGroupT->SetTotalOccurrences(data_occurrences-1);
    pGroupT->SetDataOccurrences(data_occurrences-1);
    if (binc )
    {
        //pGroupT->SetExOccur( occurrence );
    }
    pGroupT->SetHighOccurrences(std::max(pGroupT->GetHighOccurrences(),data_occurrences)); // BMD 14 Aug 2003

    return true;
}


double CIntDriver::exswap(int iExpr) // 20100105
{
    const FNINS_NODE* func_node = (FNINS_NODE*)PPT(iExpr);
    const GRP_NODE* group_node = (GRP_NODE*)PPT(func_node->group_node);
    GROUPT* pGroupT = GPT(group_node->m_iGrpIndex);
    int occurrence1 = evalexpr<int>(func_node->first_occurrence);
    int occurrence2 = evalexpr<int>(func_node->last_occurrence);

    int data_occurrences = pGroupT->GetDataOccurrences();

    // invalid subscripts
    if( occurrence1 < 1 || occurrence2 < 1 || occurrence1 > data_occurrences || occurrence2 > data_occurrences ) 
    {
        //1089 Invalid subscript in swap: %s(%d and %d), occs = %d -- %p
        issaerror(MessageType::Error, 1089, pGroupT->GetName().c_str(), occurrence1, occurrence2, data_occurrences);
        return 0;
    }

    // quit out if no need to swap
    if( occurrence1 == occurrence2 ) 
        return 1;

    // code adapted from exsort and GROUPT::SortOcc

    // make sure that occurrence1 is less than occurrence2
    if( occurrence1 > occurrence2 )
        std::swap(occurrence1, occurrence2);

    CLoopControl cLoopControl(pGroupT->GetDimType(), occurrence1 - 1, occurrence2 - 1, data_occurrences - 1);
    CNDIndexes theIndex(ZERO_BASED, DEFAULT_INDEXES);

    for( int iItem = 0; iItem < pGroupT->GetNumItems(); ++iItem )
    {
        int iSymb = pGroupT->GetItemSymbol(iItem);

        if( iSymb == 0 )
            break;

        // ... member is an item or sub-item
        if( NPT(iSymb)->IsA(SymbolType::Variable) )
        {
            VARX* pVarX = VPX(iSymb);
            pVarX->BuildIntParentIndexes(theIndex, 1);   // TRANSITION ?
            break;
        }
    }

    std::vector<int> m_aTarget;

    for( int i = cLoopControl.GetFromOcc(); i <= cLoopControl.GetUntilOcc(); ++i )
        m_aTarget.emplace_back(i);

    // now do the actual swap of indices
    m_aTarget.front() = cLoopControl.GetUntilOcc();
    m_aTarget.back() = cLoopControl.GetFromOcc();

    CLoopInstruction cLoopInstruction(GROUPT::GrouptSortOcc, &m_aTarget);

    return pGroupT->DoMoveOcc(theIndex, &cLoopControl, &cLoopInstruction);
}


double CIntDriver::exsort(int iExpr)
{
    const FNSRT_NODE* pNode = (FNSRT_NODE*)PPT(iExpr);
    GROUPT* pGroupT = GPT(pNode->fn_grp);

    int data_occurrences = pGroupT->GetDataOccurrences();
    if( data_occurrences <= 0 )
        return 0;

    // Get Loop Index
    int iLoopIndex = pGroupT->GetDimType();

    int iOccur = 1;

    int iFromOcc = 0;
    int iUntilOcc = data_occurrences - 1;
    int iMaxOcc = data_occurrences - 1;

    CLoopControl cLoopControl( iLoopIndex, iFromOcc, iUntilOcc, iMaxOcc  );
    CNDIndexes theIndex( ZERO_BASED, DEFAULT_INDEXES );

    bool set = false;

    for( int iItem = 0;  iItem < pGroupT->GetNumItems(); iItem++ )
    {
        int iSymb;
        if( ( iSymb = pGroupT->GetItemSymbol(iItem) ) == 0 )
            break;

        if( pGroupT->m_pEngineArea->GetTypeGRorVA( iSymb ) == SymbolType::Variable ) // ... member is an item or sub-item
        {
            VARX* pVarX = VPX(iSymb);

            if( !set )
            {
                pVarX->BuildIntParentIndexes( theIndex, iOccur );   // TRANSITION ?
                set = true;
            }
        }
    }

    std::vector<int> sort_symbols;

    if( pNode->fn_itm >= 0 )
    {
        sort_symbols.emplace_back(pNode->fn_itm * ( ( pNode->m_iSym == 1 ) ? -1 : 1 ));
    }

    else
    {
        // starting with CSPro 7.2, multiple variables are allowed for the sort
        int number_symbols = abs(pNode->fn_itm);
        const int* symbol_block = PPT(pNode->m_iSym);

        for( int i = 0; i < number_symbols; i++ )
            sort_symbols.emplace_back(symbol_block[i]);
    }

    // 20110810
    std::unique_ptr<std::vector<int>> validIndices;

    if( pNode->m_iWhere >= 0 ) // evaluate the where clause to see which values the sort applies to
        validIndices = std::make_unique<std::vector<int>>(EvaluateValidIndices(pNode->fn_grp, abs(sort_symbols.front()), pNode->m_iWhere));

    pGroupT->SortOcc(theIndex, &cLoopControl, sort_symbols, validIndices.get());

    pGroupT->SetTotalOccurrences(data_occurrences);
    pGroupT->SetDataOccurrences(data_occurrences);

    return 1;
}
