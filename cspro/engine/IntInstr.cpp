//---------------------------------------------------------------------------
//  File name: IntInstr.cpp
//
//  Description:
//          Interprets non-data-entry instructions (for data-entry commands, see intDENTRY)
//
//  History:    Date       Author   Comment
//              ---------------------------
//              10 Nov 99   RHF     Basic conversion
//              10 Feb 00   RCH     Add valuset support
//              06 Jun 00   RHF     Array alpha arrays support
//              04 Apr 01   vc      Tailoring for RepMgr compatibility
//              16 May 01   vc      Expanding for CsDriver
//              16 Nov 01   TNC     Changing tests to treat specials as False
//              01 Dec 04   rcl     3d Handling in exdofor_relation
//
//---------------------------------------------------------------------------
#include "StandardSystemIncludes.h"
#include "INTERPRE.H"
#include "Engine.h"
#include "VARX.h"
#include "Batdrv.h"
#include "Ctab.h"
#include "ProgramControl.h"
#include "3dException.h"
#include <zEngineO/AllSymbols.h>
#include <zEngineO/LoopStack.h>
#include <zEngineO/Versioning.h>
#include <zEngineO/Nodes/ControlFlow.h>
#include <Zissalib/CFlAdmin.h>
#include <Zissalib/GroupVisitor.h>
#include <zToolsO/Serializer.h>

#pragma warning( push )
#pragma warning( once: 4244 )


double CIntDriver::exfucall(int iExpr)
{
    const auto& function_call_node = GetNode<Nodes::FunctionCall>(iExpr);

    evalexpr(function_call_node.expression);

    return 0;
}


double CIntDriver::excpt(int iExpr)
{
    // excpt: execute compute statement
    const auto& compute_node = GetNode<COMPUTE_NODE>(iExpr);
    SVAR_NODE*  pSVarNode = (SVAR_NODE*) (PPT(compute_node.cpt_var));
    MVAR_NODE*  pMVarNode = (MVAR_NODE*) (PPT(compute_node.cpt_var));
    int         iThisVar     = pSVarNode->m_iVarIndex;
    int         iThisVarType = pSVarNode->m_iVarType;
    VART*       pVarT = NULL;
    VARX*       pVarX = NULL;

    if( iThisVarType == SVAR_CODE ) {
        iThisVar     = pSVarNode->m_iVarIndex;
        iThisVarType = pSVarNode->m_iVarType;
        pVarT = VPT( iThisVar );
        pVarX = pVarT->GetVarX();
    }
    else if( iThisVarType == MVAR_CODE ) {
        iThisVar     = pMVarNode->m_iVarIndex;
        iThisVarType = pMVarNode->m_iVarType;
        pVarT = VPT( iThisVar );
        pVarX = pVarT->GetVarX();
    }

    // getting the right-side-expression's value
    double dRightValue = evalexpr(compute_node.cpt_expr);

    // if a stop command was executed while evaluating the right-side-expression's
    // value, perhaps because it called a user-defined function, then get out
    if( m_bStopProc )
        return 0;

    // left-side is ... the return-value of a user-function:
    if( iThisVarType == UF_CODE )
    {
        UserFunction& user_function = GetSymbolUserFunction(iThisVar);
        user_function.SetReturnValue(dRightValue);
        return dRightValue;
    }

    // left-side is ... a cell of an array:
    else if( iThisVarType == ARRAYVAR_CODE )
    {
        LogicArray* logic_array;
        std::vector<size_t> indices = EvaluateArrayIndex(compute_node.cpt_var, &logic_array);

        if( !indices.empty() )
            logic_array->SetValue(indices, dRightValue);

        return dRightValue;
    }

    // left-side is ... a cell of a table:
    else if( iThisVarType == TVAR_CODE ) {
#ifdef WIN_DESKTOP
        TVAR_NODE*  pTVarNode = (TVAR_NODE*) pSVarNode;
        CTAB*       pCtab = XPT(pTVarNode->tvar_index);
        int         aCoord[3] = { 0, 0, 0 };
        bool        bGoodCoord = true;
#ifdef BUCEN
        bool        bnotappl = true;
        int         iBadSubscript = 0;  // GSF 10-jan-03
#endif

        for( int iAxis = 0; bGoodCoord && iAxis < 3; iAxis++ ) {
            int     iExprIndex = pTVarNode->tvar_exprindex[iAxis];

            if( iExprIndex >= 0 ) {
                double  dValue = evalexpr( iExprIndex );

                if( m_bStopProc ) // see the note on the similar code above
                    return 0;
#ifdef BUCEN
                bnotappl = ( dValue >= 0 && dValue <= INT_MAX);
#endif
                //TODO:         bGoodCoord = ( dValue >= 0 && dValue <= INT_MAX??? );

                if( bGoodCoord ) {
                    aCoord[iAxis] = (int) dValue;
#ifdef BUCEN
                    iBadSubscript = (int) dValue;   // GSF 10-jan-03
#endif
                }
            }
        }
        if( bGoodCoord ) {
            // RHF COM Jul 31, 2001storetabv( pCtab, aCoord[0], aCoord[1], aCoord[2], dRightValue );

            if (!bnotappl) {
                issaerror( MessageType::Warning, 34008, pCtab->GetName().c_str(), iBadSubscript );  // Break by not executed
            }

            int iResult = pCtab->m_pAcum.PutDoubleValue( dRightValue, aCoord[0], aCoord[1], aCoord[2] );

            if( !iResult ) {
                issaerror( MessageType::Warning, 34008, pCtab->GetName().c_str(), iBadSubscript );  // Break by not executed
            }
        }
        //      else ISSUE A MESSAGE???

        return dRightValue;// RHF Nov 03, 2000
        // RHF COM Nov 03, 2000        return( (double) 0 );
#else
        // crosstabs don't exist in the portable environments
        ASSERT(false);
        return DEFAULT;
#endif
    }

    // left-side is ... a work-variable:
    else if( iThisVarType == WVAR_CODE )
    {
        WorkVariable& work_variable = GetSymbolWorkVariable(iThisVar);
        work_variable.SetValue(dRightValue);
        return dRightValue;
    }

    // left-side is ... either a (numeric, of course) Sing or Mult var:
    int     aIndex[DIM_MAXDIM];
    bool    bOk = true;

    memset( aIndex, 0, sizeof(int) * DIM_MAXDIM );
    if( iThisVarType == SVAR_CODE ) {

// RHF COM Jul 03, 2002if( IsVectorEnabled() ) {
// RHF COM Jul 03, 2002            pVarT->VectorAdd( dRightValue, 1 );
// RHF COM Jul 03, 2002            return dRightValue;
// RHF COM Jul 03, 2002}

    }
    else if( iThisVarType == MVAR_CODE ) {
        double      dIndex[DIM_MAXDIM];

        mvarGetSubindexes( pMVarNode, dIndex );

        bOk = pVarX->RemapIndexes( aIndex, dIndex );

// RHF COM Jul 03, 2002if( IsVectorEnabled() ) {
// RHF COM Jul 03, 2002            pVarT->VectorAdd( dRightValue, dIndex[0] );
// RHF COM Jul 03, 2002            return dRightValue;
// RHF COM Jul 03, 2002}
    }
    else {
        ASSERT(0);
    }

    if( bOk ) {
#ifdef BUCEN
        bool bIsInputDict = (pVarT->GetSubType() == SymbolSubType::Input);
        GROUPT* pGroupTRec = pVarT->GetOwnerGPT();
        int iDimType = pGroupTRec->GetDimType();
        bool bIsOwnerRecord = (iDimType == 0 || iDimType == -1);

        // do special checking only for batch applications with INPUT dictionaries
        // do not do this checking for items/subitem with occurrences
        if (Appl.ApplicationType == ModuleType::Batch && bIsInputDict && bIsOwnerRecord) {
            // give warning when trying to assign to a single record that does not exist
            if (pGroupTRec->GetTotalOccurrences() == 0 && pVarT->GetOwnerGPT()->GetMaxOccs() == 1) {
                issaerror( MessageType::Warning, 34090, pVarT->GetName().c_str());   // GSF 10-jan-03
            }
            else {
                // if subscript in range, do assignment
                if (aIndex != NULL && aIndex[0] < pGroupTRec->GetTotalOccurrences()) {
                    CNDIndexes theIndex( ZERO_BASED, aIndex );
                    bOk = SetVarFloatValue( dRightValue, pVarX, theIndex );
                }
                ASSERT( bOk );
            }
        }
        else {
            // cases that do not require special checking fall to here
            CNDIndexes theIndex( ZERO_BASED, aIndex );
            bOk = SetVarFloatValue( dRightValue, pVarX, theIndex );
        }

    // GSF 11-Feb-2003 end
#else
        CNDIndexes theIndex( ZERO_BASED, aIndex );
        bOk = SetVarFloatValue( dRightValue, pVarX, theIndex );
        ASSERT( bOk );


#endif
    } // if( bOk )

    //TODO: all below MUST be done by 'SetVarFloatValue'    // victor Jul 25, 00
    // RHF 12/8/99 Items/SubItems
    if( bOk && pVarX != NULL ) {

        if( Issamod == ModuleType::Entry || pVarX->iRelatedSlot >= 0 ) {// RHF Mar 01, 2000
            ModuleType eOldMode = Issamod;

            Issamod  = ModuleType::Batch; // Truco: in order to call varoutval and dvaltochar
            // TODO SetAsciiValid( pVarX, false );
            m_pEngineDriver->prepvar( pVarT, NO_VISUAL_VALUE );  // write to ascii buffer
            Issamod  = eOldMode;

            if( pVarX->iRelatedSlot >= 0 )
                pVarX->VarxRefreshRelatedData( aIndex );
        }
    }
    // RHF 12/8/99 Items/SubItems

    return dRightValue; // RHF Nov 03, 2000
    // RHF COM Nov 03, 2000 return( (double) 0 );
}


double CIntDriver::exif(int iExpr)
{
    const auto& if_node = GetNode<Nodes::If>(iExpr);

    bool condition_is_true = ConditionalValueIsTrue(evalexpr(if_node.conditional_expression));

    // if a request was issued in the conditional check (e.g., from a reenter in a
    // user-defined function), then exit immediately
    if( GetRequestIssued() )
        return 0;

    ExecuteProgramStatements(condition_is_true ? if_node.then_program_index :
                                                 if_node.else_program_index);

    return 0;
}


namespace Box
{
    static_assert(Serializer::GetEarliestSupportedVersion() < Serializer::Iteration_8_0_000_1);

    struct BOX_NODE
    {
        enum class RecodeType { Working, WorkingAlpha, Variable, Array };

        int box_code;
        int next_st;
        int nvarsind;
        int stvardep;
        RecodeType recodeType;
        int iMultiTally;        // 1 if VSET is used, 0 if not
        int iExprList;          // Where the list of logical expresion begins.
        int iExprBoxRow;        // Where box row begins
    };

    constexpr int TOKRANGE = 227;
}


double CIntDriver::exbox( int iExpr )
{
    // exbox: execute box statement
    const Box::BOX_NODE* pBoxNode = &GetNode<Box::BOX_NODE>(iExpr);
    int         iDepOccur=1;

    // getting the address of double' value holder of the dependent variable
    int         iExprMultSymDepVar   = pBoxNode->stvardep;
    bool        bMultiTally = (pBoxNode->iMultiTally == 1); // RHF Jul 03, 2002
    Symbol* pDepSymbol = NULL;

    double* pDepVarValue = NULL;
    bool bDepVarIsNumeric = true;

    std::unique_ptr<std::tuple<LogicArray*, std::vector<size_t>>> logic_array_parameters;

    if( pBoxNode->recodeType == Box::BOX_NODE::RecodeType::Variable || pBoxNode->recodeType == Box::BOX_NODE::RecodeType::Working )
    {
        MVAR_NODE* pMultVar = (MVAR_NODE*) (PPT(iExprMultSymDepVar));
        Symbol* pSymbol = NPT(pMultVar->m_iVarIndex);
        pDepSymbol = pSymbol; // RHF Jul 03, 2002

        if( pDepSymbol->GetType() == SymbolType::WorkVariable ) {
            WorkVariable* pWVar = (WorkVariable*)pDepSymbol;
            pDepVarValue = pWVar->GetValueAddress();
        }
        else {
            VART*   pVarT = (VART*) pSymbol;                // RHF Aug 04, 2000
            VARX*   pVarX = pVarT->GetVarX();

            bDepVarIsNumeric = pVarT->IsNumeric();

            if( pVarT->IsArray() ) {
                double      dIndex[DIM_MAXDIM];

                mvarGetSubindexes( pMultVar, dIndex );
                iDepOccur = (int)dIndex[0];

                if( bDepVarIsNumeric ) {
                    pDepVarValue = mvaraddr( pVarX, (double) iDepOccur );
                }
                else {
                    pDepVarValue = (double*)mvaraddr(pVarX,dIndex); // this will return the area for an alpha, temporarily cast to a double *
                }
            }
            else {
                pDepVarValue = svaraddr( pVarX );
            }
        }
    }

    else if( pBoxNode->recodeType == Box::BOX_NODE::RecodeType::WorkingAlpha ) {
        bDepVarIsNumeric = false;
    }

    else {
        ASSERT(pBoxNode->recodeType == Box::BOX_NODE::RecodeType::Array);

        LogicArray* logic_array;
        std::vector<size_t> indices = EvaluateArrayIndex(iExprMultSymDepVar, &logic_array);

        if( indices.empty() )
            return 0;

        logic_array_parameters = std::make_unique<std::tuple<LogicArray*, std::vector<size_t>>>(logic_array, indices);

        bDepVarIsNumeric = logic_array->IsNumeric();
    }

    // gathering current values of independent variables
#define MAX_BOXVALUES   256
    double      aVarValues[MAX_BOXVALUES+1];
    ASSERT( pBoxNode->nvarsind < MAX_BOXVALUES );

    int*        pElem;

    if( pBoxNode->iExprList > 0 ) {
        pElem  = (int*) ( PPT(pBoxNode->iExprList) );
    }
    else {
        pElem = NULL;
    }

    for( int iVarCell = 0; iVarCell < pBoxNode->nvarsind; iVarCell++ ) {
        int     iExprLog = *pElem;

        aVarValues[iVarCell] = evalexpr( iExprLog );
        pElem++;
    }

    // looping thru box' lines, seek for matching
    // RHF COM Jul 05, 2002 BOX_ROW*    pBoxRow = (BOX_ROW*) pElem;
    BOX_ROW*    pBoxRow = (BOX_ROW*) ( PPT(pBoxNode->iExprBoxRow) ); // RHF Jul 05, 2002

    double      dRightValue;

    while( pBoxRow != NULL ) {
        if( exboxrow( pBoxNode, pBoxRow, aVarValues ) ) {
            TCHAR * pAlphaDestination = NULL;
            int iAlphaDestinationLength;

            if( pDepVarValue != NULL && bDepVarIsNumeric ) {// RHF Aug 11, 2000
                if( bMultiTally ) {
                    ASSERT( pDepSymbol->IsA(SymbolType::Variable) );
                    VART*       pVarT = (VART*)pDepSymbol;

                    dRightValue = evalexpr( pBoxRow->row_expr );
                    pVarT->VectorAdd( dRightValue, (double) iDepOccur );
                }
                else {
                    dRightValue = evalexpr( pBoxRow->row_expr );
                    *pDepVarValue = dRightValue;
                }
            }

            else if( pBoxNode->recodeType == Box::BOX_NODE::RecodeType::Array )
            {
                ASSERT(logic_array_parameters != nullptr);

                if( bDepVarIsNumeric )
                {
                    dRightValue = evalexpr(pBoxRow->row_expr);
                    std::get<0>(*logic_array_parameters)->SetValue(std::get<1>(*logic_array_parameters), dRightValue);
                }

                else
                {
                    std::get<0>(*logic_array_parameters)->SetValue(std::get<1>(*logic_array_parameters), EvalAlphaExpr(pBoxRow->row_expr));
                }
            }

            else if( pBoxNode->recodeType == Box::BOX_NODE::RecodeType::Variable ) // dictionary alpha variable
            {
                ASSERT(!bDepVarIsNumeric);
                pAlphaDestination = (TCHAR*)pDepVarValue;
                iAlphaDestinationLength = ((VART*)pDepSymbol)->GetLength();
            }

            else if( pBoxNode->recodeType == Box::BOX_NODE::RecodeType::WorkingAlpha )
            {
                if( Versioning::PredatesCompiledLogicVersion(Serializer::Iteration_7_6_000_1) )
                {
                    VART* pVarT = VPT(iExprMultSymDepVar);

                    if( pVarT->GetLogicStringPtr() )
                        *(pVarT->GetLogicStringPtr()) = EvalAlphaExpr<CString>(pBoxRow->row_expr);

                    else
                    {
                        pAlphaDestination = (TCHAR *)svaraddr(pVarT->GetVarX());
                        iAlphaDestinationLength = pVarT->GetLength();
                    }
                }

                else
                {
                    WorkString& work_string = GetSymbolWorkString(iExprMultSymDepVar);
                    work_string.SetString(EvalAlphaExpr(pBoxRow->row_expr));
                }
            }

            else
            {
                ASSERT(0);
            }

            if( pAlphaDestination != NULL )
            {
                CString csResult = EvalAlphaExpr<CString>(pBoxRow->row_expr);

                int iToCopy = std::min(csResult.GetLength(),iAlphaDestinationLength);
                int iToSpaceFill = iAlphaDestinationLength - iToCopy;

                _tmemcpy(pAlphaDestination,csResult.GetBuffer(),iToCopy);

                if( iToSpaceFill > 0 )
                    _tmemset(pAlphaDestination + iToCopy,_T(' '),iToSpaceFill);
            }

            if( !bMultiTally ) // RHF Jul 03, 2002
                break;
        }

        pBoxRow = ( pBoxRow->next_row >= 0 ) ? (BOX_ROW*) (PPT(pBoxRow->next_row)) : NULL;
    }

    return( (double) 0 );
}

bool CIntDriver::exboxrow( const void* pBoxNode_void, BOX_ROW* pBoxRow, double aVarValues[] )
{
    const Box::BOX_NODE* pBoxNode = reinterpret_cast<const Box::BOX_NODE*>(pBoxNode_void);

    // exboxrow; returns true if this row matches the values of independent variables
    const int* pElem = (const int*) ( pBoxRow + 1 );

    for( int iVarCell = 0; iVarCell < pBoxNode->nvarsind; iVarCell++ ) {
        int     pNextElem = *pElem++;   // base element for the next variable

        if( *pElem == TOKCOLON ) {     // no elements given for this variable
            pElem++;
            continue;
        }

        // value (and its nature) of independent variable
        double  dVarValue = aVarValues[iVarCell];
        bool    bSpecVal  = IsSpecial(dVarValue);

        // checking each element for this variable
        double  dElemValue;
        bool    bMatching = false;

        while( *pElem != TOKCOLON ) {
            switch( *pElem ) {
            case TOKCTE:
                dElemValue = GetNumericConstant(*(++pElem));
                pElem++;

                bMatching = ( dVarValue == dElemValue );
                break;

            case TOKEQOP:
                dElemValue = GetNumericConstant(*(++pElem));
                pElem++;

                bMatching = ( dVarValue == dElemValue );
                break;

            case TOKNEOP:
                dElemValue = GetNumericConstant(*(++pElem));
                pElem++;

                bMatching = ( dVarValue != dElemValue );
                break;

            case TOKLEOP:
                if( bSpecVal )
                    pElem += 2;
                else {
                    dElemValue = GetNumericConstant(*(++pElem));
                    pElem++;

                    bMatching = ( dVarValue <= dElemValue );
                }
                break;

            case TOKLTOP:
                if( bSpecVal )
                    pElem += 2;
                else {
                    dElemValue = GetNumericConstant(*(++pElem));
                    pElem++;

                    bMatching = ( dVarValue < dElemValue );
                }
                break;

            case TOKGEOP:
                if( bSpecVal )
                    pElem += 2;
                else {
                    dElemValue = GetNumericConstant(*(++pElem));
                    pElem++;

                    bMatching = ( dVarValue >= dElemValue );
                }
                break;

            case TOKGTOP:
                if( bSpecVal )
                    pElem += 2;
                else {
                    dElemValue = GetNumericConstant(*(++pElem));
                    pElem++;

                    bMatching = ( dVarValue > dElemValue );
                }
                break;

            case Box::TOKRANGE:
                if( bSpecVal )
                    pElem += 3;
                else {
                    double  dLowerBound = GetNumericConstant(*(++pElem));
                    double  dUpperBound = GetNumericConstant(*(++pElem));
                    pElem++;

                    bMatching = ( dVarValue >= dLowerBound && dVarValue <= dUpperBound );
                }
                break;
            }

            if( bMatching ) {
                pElem = PPT(pNextElem);
                break;
            }
        }

        // return 'no match' at first time where no match found
        if( !bMatching )
            return false;
    }

    return true;                        // match found at this row
}


double CIntDriver::exstop( int iExpr )
{
    // exstop: sets the stop process flag to true
    if( m_bExecSpecFunc )
    {
        issaerror( MessageType::Error, 9100 );
        return 0;
    }

    const Symbol* pSymbol = NPT(m_iExSymbol);
    if( ( pSymbol->IsA(SymbolType::Group) && ((GROUPT*) pSymbol)->GetSubType() != SymbolSubType::Primary ) ||
        ( pSymbol->IsA(SymbolType::Variable) && ((VART*) pSymbol)->GetSubType() != SymbolSubType::Input ) )
    {
        issaerror( MessageType::Error, 9192);
        return 0;
    }

    const auto& stop_node = GetNode<STOP_NODE>(iExpr);
    int stop_code = ( stop_node.stop_expr >= 0 ) ? evalexpr<int>(stop_node.stop_expr) : 0;

    m_bStopProc = true;
    m_pEngineDriver->SetStopCode(stop_code);

    return 1;
}


double CIntDriver::exnoopIgnore_numeric(int /*iExpr*/)
{
    return 0;
}


double CIntDriver::exnoopIgnore_string(int iExpr)
{
    return AssignBlankAlphaValue();
}


double CIntDriver::exnoopAbort(int /*iExpr*/)
{
    // exnoopAbort: no operation invalid call
    issaerror(MessageType::Abort, 1005);
    return 0;
}


double CIntDriver::exnoopAbortPlaceholderForFutureFunction(int /*iExpr*/)
{
    issaerror(MessageType::Abort, 1006);
    return 0;
}


double CIntDriver::exwhile(int iExpr)
{
    const auto& while_node = GetNode<Nodes::While>(iExpr);

    LoopStackEntry loop_stack_entry = GetLoopStack().PushOnLoopStack(LoopStackSource::While);
    ASSERT(loop_stack_entry.IsValid());

    while( ConditionalValueIsTrue(evalexpr(while_node.conditional_expression)) )
    {
        try
        {
            ExecuteProgramStatements(while_node.block_program_index);
        }

        catch( const NextProgramControlException& )  { }
        catch( const BreakProgramControlException& ) { break; }

        catch( LogicStackSaver& logic_stack_saver )
        {
            // add the while loop statement to the logic stack so that it is evaluated after any
            // additional statements in the loop
            logic_stack_saver.PushStatement(iExpr);

            // the next statement should not be added because it will be evaluated after this
            // while loop is executed
            logic_stack_saver.SuppressNextPush(while_node.next_st);

            throw;
        }

        if( m_iStopExec )
            break;
    }

    return 0;
}


double CIntDriver::exdo(int iExpr)
{
    const auto& do_node = GetNode<Nodes::Do>(iExpr);
    const Nodes::SymbolValue* counter_symbol_value_node = nullptr;
    double* counter_work_variable_address = nullptr;

    LoopStackEntry loop_stack_entry = GetLoopStack().PushOnLoopStack(LoopStackSource::Do);
    ASSERT(loop_stack_entry.IsValid());

    std::unique_ptr<Nodes::SymbolValue> pre76_symbol_value_node;

    if( do_node.counter_symbol_value_node_index != -1 )
    {
        if( Versioning::PredatesCompiledLogicVersion(Serializer::Iteration_7_6_000_1) )
        {
            pre76_symbol_value_node = std::make_unique<Nodes::SymbolValue>();
            pre76_symbol_value_node->symbol_index = do_node.counter_symbol_value_node_index;
            pre76_symbol_value_node->symbol_compilation = -1;
            ASSERT(NPT_Ref(pre76_symbol_value_node->symbol_index).IsA(SymbolType::WorkVariable));
            counter_symbol_value_node = pre76_symbol_value_node.get();
        }

        else
        {
            counter_symbol_value_node = &GetNode<Nodes::SymbolValue>(do_node.counter_symbol_value_node_index);
        }

        // work variables, since they will make up the vast majority of loop counters,
        // will be handled in a special way to make the iterations more efficient
        if( NPT_Ref(counter_symbol_value_node->symbol_index).IsA(SymbolType::WorkVariable) )
            counter_work_variable_address = GetSymbolWorkVariable(counter_symbol_value_node->symbol_index).GetValueAddress();

        double initial_value = evalexpr(do_node.counter_initial_value_expression);
        AssignValueToSymbol(*counter_symbol_value_node, initial_value);
    }

    bool check_conditional_value_is_true = ( do_node.loop_type == TOKWHILE );
    bool increment_variable_by_one = ( counter_symbol_value_node != nullptr && do_node.counter_increment_by_expression == -1 );
    bool increment_work_variable_by_one = ( increment_variable_by_one && counter_work_variable_address != nullptr );

    while( ConditionalValueIsTrue(evalexpr(do_node.conditional_expression)) == check_conditional_value_is_true )
    {
        try
        {
            ExecuteProgramStatements(do_node.block_program_index);
        }

        catch( const NextProgramControlException& )  { }
        catch( const BreakProgramControlException& ) { break; }

        if( m_iStopExec )
            break;

        if( increment_work_variable_by_one )
        {
            ++(*counter_work_variable_address);
        }

        else if( counter_symbol_value_node != nullptr )
        {
            double increment_value = increment_variable_by_one ? 1 : evalexpr(do_node.counter_increment_by_expression);

            if( counter_work_variable_address != nullptr )
            {
                *counter_work_variable_address += increment_value;
            }

            else
            {
                ModifySymbolValue<double>(*counter_symbol_value_node, [increment_value](double& value) { value += increment_value; });
            }
        }
    }

    return 0;
}



// RHC INIC Sep 20, 2001
double CIntDriver::exfor_relation( int iForRelation ) {
    FORRELATION_NODE* pFor = (FORRELATION_NODE *) PPT( iForRelation );
    return exdofor_relation( pFor, NULL, NULL );
}

// Modify Unit's MVAR_NODE info to create constant values
// for table generation [Do_xtab.cpp/CIntDriver::CtPos_FillIndexArray]
// + [citer.cpp/MakeTableIterator]
static
int modifyVarNode_GetMaxValue( MVAR_NODE* pMvarNode, VART* pVarT, CIntDriver* theDriver, double dMaxValue )
{
    ASSERT( pMvarNode != 0 );

    VARX* pVarX = pVarT->GetVarX();

    double dIndex[DIM_MAXDIM];
    int aIndex[DIM_MAXDIM];

    // 1st, evaluate MVAR_NODE to change expressions for constants
    theDriver->mvarGetSubindexes( pMvarNode, dIndex );

    // then change original MVAR_NODE info for those constants
    int iIndexNumber = pMvarNode->m_iSubindexNumber;
    ASSERT( iIndexNumber >= 0 && iIndexNumber <= DIM_MAXDIM );

    for( int i = 0; i < iIndexNumber; i++ )
    {
        if( pMvarNode->m_iVarSubindexType[i] != MVAR_CONSTANT )
        {
            pMvarNode->m_iVarSubindexType[i] = MVAR_CONSTANT;
            pMvarNode->m_iVarSubindexExpr[i] = (int) dIndex[i];
        }
        else
            ASSERT( pMvarNode->m_iVarSubindexExpr[i] == (int) dIndex[i] );
    }

    // quick fix to prevent RemapIndexes exception when receiving
    // null values in dIndex
    for( int i = iIndexNumber; i < pVarT->GetNumDim(); i++ )
        dIndex[i] = 1.0;

    if( pVarX->RemapIndexes( aIndex, dIndex ) == true )
    {
        CNDIndexes theIndex( ZERO_BASED, aIndex );
        GROUPT* pGroupT = pVarT->GetParentGPT();
        ASSERT( pGroupT != 0 );
        if( pGroupT != 0 )
        {
            dMaxValue = pGroupT->GetTotalOccurrences( theIndex );
        }
    }

    return (int)dMaxValue;
}

//////////////////////////////////////////////////////////////////////////
// to ease exdofor_relation max calculation

double CIntDriver::getMaxIndexForVariableUsingStack( int iVar, REL_NODE* pRelNode ) // rcl, Dec 18, 2004
{
    return getMaxIndexForVariableUsingStack(VPT(iVar), pRelNode );
}

double CIntDriver::getMaxIndexForVariableUsingStack( VART* pVarT, REL_NODE* pRelNode ) // rcl, Dec 18, 2004
{
    ASSERT( pVarT != 0 );
    GROUPT* pGroupT = GPT(pVarT->GetParentGroup());
    ASSERT( pGroupT != 0 );
    int iVar = pVarT->GetSymbolIndex();

    if( pVarT->GetNumDim() == 1 )
    {
        return pGroupT->GetTotalOccurrences();
    }

    int aIndex[DIM_MAXDIM];

    for( int i = 0; i < DIM_MAXDIM; i++ )
        aIndex[i] = 0;

    int iNumDim = pVarT->GetNumDim() - 1;
    // change anything?
    for( int i = 0; i < iNumDim; i++ )
    {
        CDimension::VDimType vType = pVarT->GetDimType(i);
        bool bFoundIndexInStack = false;

        // last element in stack has been inserted recently
        // so we will not use it
        for( int s = m_iForStackNext - 2; s >= 0 ; s-- )
        {
            FOR_STACK *pForStack = &ForStack[s];

            double dPrevLoopVarValue;
            switch( pForStack->forType )
            {
            case _T('G'):
                if( pVarT->IsAncestor( pForStack->forGrpIdx, true ) && pGroupT->GetDimType() == vType )
                {
                    dPrevLoopVarValue = GetSymbolWorkVariable(pForStack->forVarIdx).GetValue();
                    bFoundIndexInStack = true;
                }
                break; // currently ignoring it
            case _T('R'):
                {
                    RELT* pRelT2 = RLT(pForStack->forRelIdx);
                    RELATED related;
                    RELATED* pResult;

                    pResult = pRelT2->GetRelated( &related, iVar, vType );
                    if( pResult != 0 )
                    {
                        dPrevLoopVarValue = GetSymbolWorkVariable(pResult->iRelatedWVarIdx).GetValue();
                        bFoundIndexInStack = true;
                    }
                }
                break;
            default: break;
            }

            if( bFoundIndexInStack )
            {
                aIndex[i] = (int) dPrevLoopVarValue - 1; // aIndex is 0 based
                break;
            }
        }

        // Try to find dim value in REL_NODE, rcl Apr 2005
        if( !bFoundIndexInStack && (pRelNode != 0) )
        {
            int iType = pRelNode->m_iRelSubindexType[i];
            switch( iType )
            {
            case MVAR_CONSTANT:
                aIndex[i] = pRelNode->m_iRelSubindexExpr[i];
                bFoundIndexInStack = true;
                break;
            case MVAR_GROUP:
                aIndex[i] = GPT(pRelNode->m_iRelSubindexExpr[i])->GetCurrentOccurrences();
                bFoundIndexInStack = true;
                break;
            case MVAR_EXPR:
                aIndex[i] = evalexpr<int>(pRelNode->m_iRelSubindexExpr[i]);
                bFoundIndexInStack = true;
                break;
            default:
                bFoundIndexInStack = false; // redundant, just to make it clear / explicit
                break;
            }

            // aIndex is 0 based and previous assignments are 1 based
            // -> fix that
            if( bFoundIndexInStack && aIndex[i] > 0 )
                aIndex[i]--;
        }

        if( !bFoundIndexInStack )
        {
            issaerror( MessageType::Warning, 34091, pVarT->GetName().c_str() );
            return 0.0; // so that outside any value will be invalid
        }
    }

    CNDIndexes theIndex( ZERO_BASED, aIndex );
    double dMaxValue = pGroupT->GetTotalOccurrences( theIndex );
    int aWhich[] = { USE_DIM_1, USE_DIM_1_2, USE_ALL_DIM };
    theIndex.specifyIndexesUsed( aWhich[pGroupT->GetNumDim()-1] );
#ifdef WIN_DESKTOP
    TRACE( _T("\ngetMaxIndexForVariableUsingStack for var %s using group %s -> %s\n"),
        pVarT->GetName().c_str(),
        pGroupT->GetName().c_str(), theIndex.toString(pGroupT->GetNumDim()).c_str() );
#endif
    return dMaxValue;
}

//////////////////////////////////////////////////////////////////////////

double CIntDriver::exdofor_relation( FORRELATION_NODE* pFor, double* dTableWeight, int*  iTabLogicExpr, LIST_NODE* pListNode)
{
    LoopStackEntry loop_stack_entry = GetLoopStack().PushOnLoopStack(LoopStackSource::For);
    ASSERT(loop_stack_entry.IsValid());

    //////////////////////////////////////////////////////////////////////////
    // Prevent same max index calculation for every loop
    // rcl, Dec 18 2004
    std::map<int,double> aMaxIndexCache;
    #define ALREADY_CALCULATED(x) (aMaxIndexCache.find(x) != aMaxIndexCache.end())
    //////////////////////////////////////////////////////////////////////////

    double      dWhereExprValue; // RHF Jun 15, 2002
    double      dRet=0;
#ifdef WIN_DESKTOP
    // crosstabs don't exist in the portable environments
    CTAB*       pCtab = (pFor->iCtab>0) ? XPT(pFor->iCtab) : NULL;
#endif
    int         iSymForVar = pFor->forVarIdx;
    int         iSymRel    = pFor->forRelIdx;
    REL_NODE*    pRelNode = 0;
    if( pFor->m_iForRelNodeIdx != 0 )
        pRelNode = (REL_NODE*) PPT( pFor->m_iForRelNodeIdx );

    RELT*       pRelT      = RLT(iSymRel);
    int         iVarBaseObjRel = pRelT->GetBaseObjIndex();
    int         iWhereExpr = pFor->forWhereExpr;
    int         iCtabWeigthExpr = pFor->CtabWeightExpr;
    int         iUnitTablogicExpr = pFor->CtabTablogicExpr; // RHF Jul 15, 2005

    // push Relation into Runtime Stack for Relation' looping
    ASSERT( m_iForStackNext < FOR_STACK_MAX );
    if( m_iForStackNext >= FOR_STACK_MAX ) {
        // TODO - what should be done here?
    }

    FOR_STACK *pForStack = &ForStack[m_iForStackNext++];

    pForStack->forVarIdx = iSymForVar;
    pForStack->forRelIdx = iSymRel;
    pForStack->forType   = _T('R');
    pForStack->forGrpIdx = 0; // not to have garbage, rcl Dec 2004

    SECT*       pSect = 0;
    VART*       pVarT;
    GROUPT*     pGroupT;
    GROUPT*  pUnitGroupT = 0;

    double      dInitValue=1, dMaxValue;

    double oldTargetWorkVariable = 0; // GHM 20120326
    double * oldTargetWorkAddress = NULL;


    SaveExOccVisitor saveVisitor;
    if( pRelT->GetBaseObjType() == SymbolType::Section ) {
        // ... the Relation for the UNIT is based on a Record
        pSect     = SPT(iVarBaseObjRel);
        pSect->accept( &saveVisitor );

        dMaxValue = m_pEngineDriver->SetupSectOccsFromGroups( iVarBaseObjRel );
    }
    else {
        // ... the Relation for the UNIT is based on a mult-Item
        pVarT   = VPT(iVarBaseObjRel);
        pUnitGroupT = GPT(VPT(iVarBaseObjRel)->GetParentGroup());
        pUnitGroupT->accept( &saveVisitor );
        dMaxValue = getMaxIndexForVariableUsingStack( pVarT, pRelNode );
        aMaxIndexCache[ iVarBaseObjRel ] = dMaxValue;
    }

    MVAR_NODE mvarOriginal; // to be able to restore any modification made to Unit

#ifdef WIN_DESKTOP
    if( pFor->isUsingExtraInfo != 0 )
    {
        if( pCtab != 0 )
        {
            MVAR_NODE* pMvarNode = m_pEngineDriver->m_pEngineCompFunc->getLocalMVarNodePtr(pFor->extraInfoNode);
            mvarOriginal = *pMvarNode;
            VART* pCtabVarT = VPT(pMvarNode->m_iVarIndex);
            dMaxValue = modifyVarNode_GetMaxValue( pMvarNode, pCtabVarT, this, dMaxValue );
        }
    }
#endif

    double*     pLoopVar  = GetSymbolWorkVariable(iSymForVar).GetValueAddress();
    *pLoopVar = NOTAPPL;

    double      dCount1, dInitValue1, dMaxValue1, dValExpr;
    double*     pTargetWVar;
    double      dOneRet;
    bool        bFitValue;// RHF Jul 08, 2002

    ChangeExOccVisitor changeVisitor;
    for( double dCount = dInitValue; dCount <= dMaxValue; ++dCount )
    {
        changeVisitor.setValue( (int) dCount );
        if( pRelT->GetBaseObjType() == SymbolType::Section ) // rcl, Sept 2005
            pSect->accept( &changeVisitor );
        else
            pUnitGroupT->accept( &changeVisitor );
        *pLoopVar = dCount;

        // load all TARGET subindex (work variable)
        for( int i = 0; i < (int)pRelT->m_aTarget.size(); i++ ) {
            TARGET& aTarget = pRelT->m_aTarget[i]; // RHF Jul 16, 2002
            TARGET* pTarget = &aTarget;

            switch( pTarget->iTargetRelationType ) {
            case USE_INDEX_RELATION:
                // load dCount in corresponding TARGET work variable index
                pTargetWVar = GetSymbolWorkVariable(pTarget->iTargetWVarIdx).GetValueAddress();

                if( !oldTargetWorkAddress ) // GHM 20120326 save the current index (this matters in cases of for loops in tab logic)
                {
                    oldTargetWorkAddress = pTargetWVar;
                    oldTargetWorkVariable = *pTargetWVar;
                }

                if( pTarget->eTargetSymbolType == SymbolType::Section )
                    dMaxValue1 = m_pEngineDriver->SetupSectOccsFromGroups( pTarget->iTargetSymbolIndex );
                else {
                    int iSymbolIndex = pTarget->iTargetSymbolIndex;
                    if( ALREADY_CALCULATED(iSymbolIndex) )
                    {
                        dMaxValue1 = aMaxIndexCache[iSymbolIndex];
                        TRACE( _T("Updating target %d [USE_INDEX_RELATION] index taken from cache: %f *** good\n"), pTarget->iTargetWVarIdx, dMaxValue1 );
                    }
                    else
                    {
                        pVarT = VPT(iSymbolIndex);
                        dMaxValue1 = getMaxIndexForVariableUsingStack( pVarT, pRelNode );
                        aMaxIndexCache[ iSymbolIndex ] = dMaxValue1;
                        TRACE( _T("Updating target %d [USE_INDEX_RELATION] calculating max index: %f\n"), pTarget->iTargetWVarIdx, dMaxValue1 );
                     }
                }

                *pTargetWVar = dCount;
                if( dCount > dMaxValue1 )
                    *pTargetWVar = INVALIDINDEX;  // will be trapped by mvarvalue
                break;

            case USE_LINK_RELATION:
                // load evalexpr in corresponding TARGET work variable index
                pTargetWVar = GetSymbolWorkVariable(pTarget->iTargetWVarIdx).GetValueAddress();

                *pTargetWVar = evalexpr( pTarget->iTargetRelationExpr );
                TRACE( _T("Updating target %d [USE_LINK_RELATION] -> %f\n"), pTarget->iTargetWVarIdx, *pTargetWVar );
                break;

            case USE_WHERE_RELATION_SINGLE:
                // traverse TARGET object until evalexpr = TRUE
                // if found load index in corrresponding TARGET work variable index
                // TARGET corresponds to a multiple object (section o item)
                TRACE( _T("Updating target %d [USE_WHERE_RELATION_SINGLE]\n"), pTarget->iTargetWVarIdx );
                pTargetWVar = GetSymbolWorkVariable(pTarget->iTargetWVarIdx).GetValueAddress();

                *pTargetWVar = NOTAPPL;
                dInitValue1 = 1;

                if( pTarget->eTargetSymbolType == SymbolType::Section )
                    dMaxValue1 = m_pEngineDriver->SetupSectOccsFromGroups( pTarget->iTargetSymbolIndex );
                else {
                    pVarT      = VPT(pTarget->iTargetSymbolIndex);
                    pGroupT    = GPT(pVarT->GetParentGroup());
                    dMaxValue1 = pGroupT->GetTotalOccurrences();
                }

                bFitValue=false;// RHF Jul 08, 2002
                for( dCount1 = dInitValue1; dCount1 <= dMaxValue1; dCount1++ ) {
                    *pTargetWVar = dCount1;
                    dValExpr = evalexpr( pTarget->iTargetRelationExpr );

                    if( ConditionalValueIsTrue(dValExpr) ) {
                        bFitValue = true; // RHF Jul 08, 2002
                        break;
                    }
                }

                // RHF INIC Jul 08, 2002
                // When the where expresion is false, the value will be notappl
                if( !bFitValue )
                    *pTargetWVar = INVALIDINDEX;
                // RHF END Jul 08, 2002

                break;

            case USE_WHERE_RELATION_MULTIPLE:
                {
                    double  dLastFit; // RHF Jul 08, 2002

                // traverse TARGET object loading TARGET vector with
                // all indexes where expression evaluates to TRUE
#ifdef WIN_DESKTOP
                if( pCtab == NULL )
#endif
                {
                    issaerror( MessageType::Error, 33108 );
                    break;
                }

                TRACE( _T("Updating target [USE_WHERE_RELATION_MULTIPLE]\n") );
                pRelT->EmptyTargetMultipleIndex( pTarget );

                pTargetWVar = GetSymbolWorkVariable(pTarget->iTargetWVarIdx).GetValueAddress();

                *pTargetWVar = NOTAPPL;
                dInitValue1 = 1;

                if( pTarget->eTargetSymbolType == SymbolType::Section )
                    dMaxValue1 = m_pEngineDriver->SetupSectOccsFromGroups( pTarget->iTargetSymbolIndex );
                else {
                    pVarT      = VPT(pTarget->iTargetSymbolIndex);
                    pGroupT    = GPT(pVarT->GetParentGroup());
                    dMaxValue1 = pGroupT->GetTotalOccurrences();
                }

                bFitValue=false;// RHF Jul 08, 2002

                m_bAllowMultipleRelation = true; // RHF Jul 16, 2002
                for( dCount1 = dInitValue1; dCount1 <= dMaxValue1; dCount1++ ) {
                    *pTargetWVar = dCount1;

                    dValExpr = evalexpr( pTarget->iTargetRelationExpr );

                    if( ConditionalValueIsTrue(dValExpr) ) {
                        pRelT->AddTargetMultipleIndex( pTarget, dCount1 );
                        bFitValue = true;
                        dLastFit = dCount1;// RHF Jul 08, 2002
                    }
                }
                m_bAllowMultipleRelation = false; // RHF Jul 16, 2002

                // RHF INIC Jul 08, 2002
                // Use the last index matching
                if( bFitValue )
                    *pTargetWVar = dLastFit;
                else
                    *pTargetWVar = INVALIDINDEX;
                // RHF END Jul 08, 2002

                // The index is the last!!
            }
            break;

            default:
                ASSERT( 0 ); // can't be
                break;
            }
        }

        if( iWhereExpr == 0 )
            dWhereExprValue = 1;
        else if( iWhereExpr > 0 ) {
            dWhereExprValue = evalexpr( iWhereExpr );
        }
        else {
            dWhereExprValue = evalexpr(-iWhereExpr);
        }

        if( ConditionalValueIsFalse(dWhereExprValue) )
            continue;

#ifdef WIN_DESKTOP
        if( pCtab != NULL )
        {
            double      dSubTableWeight;

            if( iCtabWeigthExpr == 0 )
                dSubTableWeight = 1;
            else if( iCtabWeigthExpr > 0 ) {
                dSubTableWeight = evalexpr( iCtabWeigthExpr );
            }
            else {
                dSubTableWeight = evalexpr( -iCtabWeigthExpr );
            }

            ASSERT( dTableWeight != NULL );
            ASSERT( iTabLogicExpr != NULL );
            //ASSERT( pListNode != NULL );
            // tally this instance
            if( iCtabWeigthExpr != 0 && !IsSpecial(dSubTableWeight) && dSubTableWeight > -MAXVALUE ||
                iCtabWeigthExpr == 0 && !IsSpecial(*dTableWeight) && *dTableWeight > -MAXVALUE ) {
                // RHF COM Jul 15, 2005 dOneRet = DoOneXtab( pCtab, (iCtabWeigthExpr!=0) ? dSubTableWeight : (*dTableWeight), *iTabLogicExpr, pListNode );
                    dOneRet = DoOneXtab( pCtab, (iCtabWeigthExpr!=0) ? dSubTableWeight * (*dTableWeight) : (*dTableWeight), (iUnitTablogicExpr!=0) ? iUnitTablogicExpr :*iTabLogicExpr, pListNode );
                if( dRet == 0 ) dRet = dOneRet; // The first error is returned
            }
        }
        else
#endif
        {
            try
            {
                ExecuteProgramStatements(pFor->forBlock);
            }

            catch( const NextProgramControlException& )  { }
            catch( const BreakProgramControlException& ) { break; }
        }

        if( m_iStopExec )
            break;
      }

      // pop Relation from Runtime Stack
      if( m_iForStackNext > 0 )
          m_iForStackNext--;

      // and undo changes to original MVAR_NODE for unit
      if( pFor->isUsingExtraInfo != 0 )
      {
#ifdef WIN_DESKTOP
          MVAR_NODE* pMvarNode = m_pEngineDriver->m_pEngineCompFunc->getLocalMVarNodePtr(pFor->extraInfoNode);
          ASSERT( pMvarNode != 0 );
          *pMvarNode = mvarOriginal;
#endif
      }

      RestoreExOccVisitor restoreVisitor;
      if( pRelT->GetBaseObjType() == SymbolType::Section ) // rcl, Sept 2005
          pSect->accept( &restoreVisitor );
      else
          pUnitGroupT->accept( &restoreVisitor );

      if( oldTargetWorkAddress ) // GHM 20120327
          *oldTargetWorkAddress = oldTargetWorkVariable;

      return( dRet );
}

// RHC END Sep 20, 2001

double CIntDriver::exfor_group( int iForGroup )
{
    LoopStackEntry loop_stack_entry = GetLoopStack().PushOnLoopStack(LoopStackSource::For);
    ASSERT(loop_stack_entry.IsValid());

    FORGROUP_NODE*  pFor = (FORGROUP_NODE *) PPT( iForGroup );
    GRP_NODE*       pGrpNode = &pFor->forGrpNode;
    int             iGrpIndex = pGrpNode->m_iGrpIndex;
    int             iSymForVar = pFor->forVarIdx;
    GROUPT*         gpt = GPT( iGrpIndex );
    int             iGrpNumDim = gpt->GetNumDim();
    double          dMaxValue, dInitValue, *pLoopVar;

    // check last dimension subindex expression for this group to determine if
    // an iteration must take place or not
    ASSERT( iGrpNumDim >= 1 );

// RHC INIC Sep 20, 2001

    // push Relation into Runtime Stack for Relation' looping
    ASSERT( m_iForStackNext < FOR_STACK_MAX );
    if( m_iForStackNext >= FOR_STACK_MAX ) {
        // TODO - what should be done here?
    }

    FOR_STACK *pForStack = &ForStack[m_iForStackNext++];
    pForStack->forVarIdx = iSymForVar;
    pForStack->forGrpIdx = iGrpIndex;
    pForStack->forType = _T('G');
    pForStack->forRelIdx = 0; // not to have garbage, rcl Dec 2004

// RHC END Sep 20, 2001

    if( pGrpNode->m_iGrpSubindexType[iGrpNumDim - 1] == MVAR_GROUP ) {
        dInitValue = 1;
        dMaxValue = gpt->GetTotalOccurrences();
    }
    else {
        dInitValue = evalexpr( pGrpNode->m_iGrpSubindexExpr[iGrpNumDim - 1] );
        dMaxValue = dInitValue;
    }

    pLoopVar = GetSymbolWorkVariable(iSymForVar).GetValueAddress();
    *pLoopVar = NOTAPPL;

    bool use_where_expression = ( pFor->forWhereExpr > 0 );

    if( dInitValue >= 1 && dInitValue <= gpt->GetTotalOccurrences() && dMaxValue >= 1 && dMaxValue <= gpt->GetTotalOccurrences() )
    {
        for( double dCount = dInitValue; dCount <= dMaxValue; ++dCount )
        {
            *pLoopVar = dCount;

            if( use_where_expression && ConditionalValueIsFalse(evalexpr(pFor->forWhereExpr)) )
                continue;

            try
            {
                ExecuteProgramStatements(pFor->forBlock);
            }

            catch( const NextProgramControlException& )  { }
            catch( const BreakProgramControlException& ) { break; }

            if( m_iStopExec )
                break;
        }
    }

    // Pop values from Runtime Stack for For Loops            // RHC Jun 10, 2001
    if( m_iForStackNext > 0 )
        m_iForStackNext--;

    return 0;
}


double CIntDriver::exfornext(int /*iExpr*/)
{
    ASSERT(GetLoopStack().GetLoopStackCount() > 0);

    throw NextProgramControlException();
}


double CIntDriver::exforbreak(int /*iExpr*/)
{
    ASSERT(GetLoopStack().GetLoopStackCount() > 0);

    throw BreakProgramControlException();
}


double CIntDriver::exctab( int iExpr )
{
#if defined(USE_BINARY) // IGNORE_CTAB
    ASSERT(0);
    return 0;
#else
    // exctab: execute CROSSTAB statement
    CTAB_NODE*  pCtabNode = (CTAB_NODE*) (PPT(iExpr));
    CTAB*       pCtab     = XPT(pCtabNode->SYMTctab);

    if( pCtab->GetAcumArea() == NULL )
        return( (double) 0 );

    pCtab->ResetSelectWeightExpr( pCtabNode->selectexpr, pCtabNode->weightexpr ); // RHF Aug 05, 2005

    int         iSelectExpr = pCtabNode->selectexpr;
    int         iWeightExpr = pCtabNode->weightexpr;
    int         iTabLogicExpr = pCtabNode->tablogicexpr;

    // checks select' expression
    if( iSelectExpr == 0 )
        ;
    else {
        double  dSelectExprValue;
        if( iSelectExpr > 0 )
            dSelectExprValue = evalexpr( iSelectExpr );
        else {
            dSelectExprValue = evalexpr( -iSelectExpr );
        }

        if( dSelectExprValue == 0 || IsSpecial(dSelectExprValue) )
            return( (double) -1 );
    }

    // calculating weight value (no weight declared: 1 by default)
    // RHF COM Jul 04, 2002 double      dWeightValue = ( iWeightExpr >= 0 ) ? evalexpr( iWeightExpr ) : 1;

    double      dWeightValue;
    if( iWeightExpr == 0 )
        dWeightValue = 1;
    else if( iWeightExpr > 0 )
        dWeightValue = evalexpr( iWeightExpr );
    else {
        dWeightValue = evalexpr( -iWeightExpr );
    }

    if( IsSpecial(dWeightValue) )
        return( (double) -1 );

    return( DoXtab( pCtab, dWeightValue, iTabLogicExpr, NULL ) );
#endif //#if defined(USE_BINARY) // IGNORE_CTAB
}


double CIntDriver::exbreak( int iExpr ) {
#ifdef USE_BINARY
    ASSERT(0); // should not be executing this
#else
    // exbreak: execute BREAK BY statement

    ASSERT( Issamod == ModuleType::Batch );
    CBatchDriverBase*   pBatchDriverBase=(CBatchDriverBase*)m_pEngineDriver;

    // RHF INIC Oct 22, 2002
    if( pBatchDriverBase->GetBatchMode() == CRUNAPL_CSCALC ) {
        issaerror( MessageType::Warning, 591 );  // Break by not executed
        return( (double) 0 );
    }
    // RHF END Oct 22, 2002

    CTbd*       pTbd = pBatchDriverBase->GetTbd();

    pTbd->breakcheckid();
#endif

    return( (double) 0 );
}

double CIntDriver::exexport( int iExpr ) {
#ifdef USE_BINARY
    ASSERT(0); // should not be executing this
#else
    // exexport: execute Export statement
    EXPORT_NODE*    pExpoNode = (EXPORT_NODE*) (PPT(iExpr));
    CExport*        pExport   = (CExport*) pExpoNode->m_pExport;

    if( !pExport->IsExportAlive() ) {
        bool bDeleteWhenFail;
        bool bOpenOk = pExport->ExportOpen( &bDeleteWhenFail );

        if( !bOpenOk && bDeleteWhenFail )
            pExport->RemoveFiles();
    }

    if( pExport->IsExportActive() ) {                   // victor Dec 18, 00
        m_pEngineArea->SetCurExport( pExport );

        ExpWriteThisExport();           // formerly 'expwrecords'
    }
#endif // USE_BINARY
    return 0;
}

//--------------------------------------------------------
//  exsetattr: change attributes of fields in forms
//--------------------------------------------------------
double CIntDriver::exsetattr( int iExpr ) {
    SET_ATTR_NODE*  setpa_node = (SET_ATTR_NODE*) (PPT(iExpr));
    LIST_NODE*      pListNode=(LIST_NODE*)(PPT(setpa_node->iListNode));
    int             iNumObjects = pListNode->iNumElems;
    int             iNumChanges = 0;

    for( int iObject = 0; iObject < iNumObjects; iObject++ ) {
        int     iSymObj = pListNode->iSym[iObject];
        int     iSymOcc=0;


        if( iSymObj <= 0 )
            continue;

        int     iSymVar;
        int     iSymFrm;
        int     iSymGroup;
        int     iSymSec;
        SECT*   pSecT;
        VART*   pVarT;

        switch( NPT(iSymObj)->GetType() ) {
        case SymbolType::Pre80Dictionary:
            if( iSymObj <= 0 )
                break;

            iSymSec = DPT(iSymObj)->SYMTfsec;

            while( iSymSec > 0 ) {
                pSecT = SPT(iSymSec);

                iSymVar = pSecT->SYMTfvar;
                while( iSymVar > 0 ) {
                    pVarT = VPT(iSymVar);
                    iNumChanges += exset_attr( iSymVar, iSymOcc, setpa_node );
                    iSymVar = pVarT->SYMTfwd;
                }

                iSymSec = pSecT->SYMTfwd;
            }
            break;

        case SymbolType::Form:
            iSymFrm = iSymObj;
            iNumChanges += exset_markform( iSymFrm, setpa_node );
            break;

        case SymbolType::Group:
            iSymGroup = iSymObj;
            iNumChanges += exset_markgroup( iSymGroup, setpa_node );
            break;

        case SymbolType::Variable:
            iSymVar = iSymObj;
            iNumChanges += exset_attr( iSymVar, iSymOcc, setpa_node );
            break;
        default:
            break;
        }
    }

    frm_capimode( 0, 1 ); // RHF Dec 05, 2002

    return( (double) iNumChanges );
}

int CIntDriver::exset_markform( int iSymFrm, SET_ATTR_NODE* setpa_node ) {
    FORM*   pForm = FPT(iSymFrm);
    int     iSymGroup = pForm->GetSymGroup();

    return exset_markgroup( iSymGroup, setpa_node );
}

int CIntDriver::exset_markgroup( int iSymGroup, SET_ATTR_NODE* setpa_node ) {
    GROUPT*     pGroupT = GPT(iSymGroup);
    //SAVY 05/04/03 "Compiler Error C2475" m_pEngineArea->GroupTtrip( pGroupT, NULL, (pGroupTripFunc2) pGroupT->Trip, 1, setpa_node );
    m_pEngineArea->GroupTtrip( pGroupT, NULL, (pGroupTripFunc2)&CSymbolGroup::Trip, 1, setpa_node );
    return 0;
}

int CIntDriver::exset_attr( int iSymVar, int iSymOcc, SET_ATTR_NODE* setpa_node ) {
    int         iNumChanges = 0;

    if( iSymVar <= 0 )
        return iNumChanges;

    VART* pVarT = VPT(iSymVar);

    if( !pVarT->IsInAForm() )
        return iNumChanges;

    int         iAttrType = setpa_node->attr_type;

    if( iAttrType == SET_AT_REF_ON || iAttrType == SET_AT_REF_OFF ) {
        // refresh (removed in 7.1)
    }

    else if( iAttrType == SET_AT_CAP_TOGGLE || iAttrType == SET_AT_CAP_ON || iAttrType == SET_AT_CAP_OFF )
    {
        if( iAttrType == SET_AT_CAP_TOGGLE )
        {
            // toggle the questions and responses (without modifying the title property)
            pVarT->SetShowQuestionText(!pVarT->GetShowQuestionText());
            pVarT->SetShowExtendedControl(!pVarT->GetShowExtendedControl());
        }

        else
        {
            bool bSettingOn = ( iAttrType == SET_AT_CAP_ON );
            bool bTitleFlagSet = ( ( setpa_node->m_iCapiMode & DEPRECATED_CAPI_TITLE_FLAG ) != 0 );

            if( ( setpa_node->m_iCapiMode & DEPRECATED_CAPI_QUESTION_FLAG ) != 0 )
                pVarT->SetShowQuestionText(bSettingOn);

            if( ( setpa_node->m_iCapiMode & DEPRECATED_CAPI_VARIABLE_FLAG ) != 0 )
            {
                if( bSettingOn || !bTitleFlagSet ) // turning off the title won't affect the extended response setting
                {
                    pVarT->SetShowExtendedControl(bSettingOn);

                    // set the field to the default capture type if not previously defined
                    if( bSettingOn && ( !pVarT->GetCaptureInfo().IsSpecified() ||
                                        pVarT->GetCaptureInfo().GetCaptureType() == CaptureType::TextBox ) )
                    {
                        pVarT->SetCaptureInfo(CaptureInfo::GetDefaultCaptureInfo(*pVarT->GetDictItem()));
                    }
                }
            }

            if( bTitleFlagSet )
                pVarT->SetShowExtendedControlTitle(bSettingOn);
        }

        iNumChanges++;
    }

    else {                              // field attribute
        switch( iAttrType ) {
        case SET_AT_AUTOSKIP:       // setup autoskip
        case SET_AT_RETURN:         // setup return
        case SET_AT_PROTECT:        // setup protection
            {
                FieldBehavior  eBehavior;

                if( iAttrType == SET_AT_AUTOSKIP )
                    eBehavior = AsAutoSkip;
                else if( iAttrType == SET_AT_RETURN )
                    eBehavior = AsEnter;
                else if( iAttrType == SET_AT_PROTECT )
                    eBehavior = AsProtected;
                else
                    ASSERT(0);

                iNumChanges += frm_varpause( iSymVar, eBehavior );
            }

            break;

        case SET_AT_HIDDEN:         // setup hidden
            iNumChanges += frm_varvisible( iSymVar, false );
            break;

        case SET_AT_VISIBLE:        // setup not hidden
            iNumChanges += frm_varvisible( iSymVar, true );
            break;

        case SET_AT_NATIVE:         // reset field to native status
            iNumChanges += frm_varpause( iSymVar, pVarT->GetDefinedBehavior() );
            iNumChanges += frm_varvisible( iSymVar, pVarT->GetDefinedVisible() );
            break;
        }
    }

    return iNumChanges;
}

#pragma warning( pop )
