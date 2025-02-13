//---------------------------------------------------------------------------
//  EXPRESC.cpp   compiler of expressions
//---------------------------------------------------------------------------
#include "StandardSystemIncludes.h"
#include "ExpresC_Include.h"
#include <zEngineO/AllSymbols.h>
#include <zEngineO/Compiler/TokenHelper.h>
#include <zEngineO/Nodes/Dictionaries.h>
#include <zEngineO/Nodes/File.h>
#include <zEngineO/Nodes/Strings.h>
#include <zToolsO/DirectoryLister.h>
#include <zLogicO/BaseCompilerSettings.h>

#define THROW_PARSER_ERROR0(x) do { SetSyntErr(x); return 0; } while( false )


int CEngineCompFunc::crelalpha()
{
    // crelalpha: compile alpha-relation
    const int p1 = CompileStringExpression();

    // BUCEN
    if( Tkn == TOKIN || Tkn == TOKHAS )// || Tkn == TOKCOLON || Tkn == TOKSEMICOLON || Tkn == TOKCOMMA)
        return p1;
    // BUCEN

    // convert aritmethic-relation operator to alpha-relation operator
    if( !IsRelationalOperator(Tkn) )
    {
        // 20140422 the code below (the 20140228 fix) was leading to unintended side effects, so this should fix that problem, restricting
        // the context in which calling an alpha function is possible
        const auto& [call_tester, is_lone_function_call] = m_loneAlphaFunctionCallTester;

        if( !is_lone_function_call && call_tester != 1 )
            IssueError(693);

        // 20140228 now allow the user to call an alpha function without having to assign the result to something;
        // this will be useful for functions like editnote
        auto& fnc_node = CreateCompilationNode<FNC_NODE>(FunctionCode::FNFREEALPHAMEM_CODE);
        fnc_node.isymb = p1;

        return GetProgramIndex(fnc_node);
    }

    const FunctionCode oper = static_cast<FunctionCode>(static_cast<int>(Tkn) + ( CH_EQ_CODE - EQ_CODE ) );

    NextToken();
    const int p2 = CompileStringExpression();

    return CreateOperatorNode(oper, p1, p2);
}

// RHC INIC Sep 20, 2001
//
// relanal: compile relation references potentially with subindexes
//
int CEngineCompFunc::relanal( int iSymRel, bool bAllowDimExpr ) {
        int         iRelNode;
        int         subindexExpr[DIM_MAXDIM];
        int         subindexCount;
        int         iVarT;
        VART        *pVarT;
        GROUPT      *pGrpT;
        RELT        *pRelT;
#ifdef GENCODE
        REL_NODE*   pRelNode;
#endif

        memset( subindexExpr, 0, DIM_MAXDIM * sizeof(int) );

        iRelNode = Prognext;
#ifdef GENCODE
        if( Flagcomp ) {
                pRelNode = NODEPTR_AS( REL_NODE );
                ADVANCE_NODE( REL_NODE );                       // RHF Aug 04, 2000

                pRelNode->m_iRelType = REL_CODE; // must be different to MVAR_CODE & SVAR_CODE // RHF Aug 07, 2000
                pRelNode->m_iRelIndex = iSymRel;
                pRelNode->m_iSubindexNumber = 0; // rcl, Apr 2005
                for( int i = 0; i < DIM_MAXDIM; ++i )
                        pRelNode->m_iRelSubindexType[i] = MVAR_NOTHING;
        }
#endif

        pRelT = RLT( iSymRel );
        subindexCount = 0;

        NextToken();                      // advance group name

        if( Tkn == TOKLPAREN ) {
                if( !bAllowDimExpr )
                        THROW_PARSER_ERROR0( ERROR_NO_DIMENSIONS_ALLOWED ); // no dimension allowed

                NextToken();

                while( Tkn != TOKRPAREN ) {
                        if( subindexCount >= pRelT->GetNumDim() )
                                THROW_PARSER_ERROR0( ERROR_TOO_MANY_SUBINDEXES ); // too many subindexes

                        if( Tkn != TOKCOMMA )
                                subindexExpr[subindexCount++] = exprlog(); // compile expression
                        else {
                                subindexExpr[subindexCount++] = -1; // implicit subindex
                                NextToken();
                        }

                        if( Tkn == TOKCOMMA )
                                NextToken();
                }

                if( Tkn == TOKRPAREN )
                        NextToken();
        }

        if( pRelT->GetNumDim() == 0 )
                return( iRelNode );

        iVarT = pRelT->GetBaseObjIndex();
        pVarT = VPT( iVarT );
        pGrpT = pVarT->GetParentGPT();
        pGrpT = pGrpT->GetParentGPT();

        // now we have subindexCount real subindexes. Those not specified will be based
        // in the corresponding occurence counters associated for the relation

#ifdef GENCODE
        if( Flagcomp ) {
                int    iRelIndex = ( pRelT != NULL ) ? pRelT->GetSymbolIndex() : 0;
                int i, j;

                for( j = subindexCount - 1, i = pRelT->GetNumDim() - 1; i >= 0; j--, i--) {

                        ASSERT( pRelT );

                        int iGroupIndex = ( pGrpT != NULL ) ? pGrpT->GetSymbolIndex() : 0;

                        if( j >= 0 && subindexExpr[j] >= 0 ) {
                                pRelNode->m_iRelSubindexType[i] = MVAR_EXPR;
                                pRelNode->m_iRelSubindexExpr[i] = subindexExpr[j];
                        }
                        else { // implicit index look ForTable
                                int iVariableInForStack =
                                        searchInForStackThisGroupOrVar(
                                        iGroupIndex,
                                        iVarT,
                                        pVarT->GetDimType(i),
                                        CONSIDER_ONLY_FOR_VARIABLE );

                                if( iVariableInForStack > 0 ) {
                                        pRelNode->m_iRelSubindexType[i] = MVAR_EXPR;
                                        pRelNode->m_iRelSubindexExpr[i] = genSVARNode( iVariableInForStack, SymbolType::WorkVariable );
                                }
                                else {
                                        pRelNode->m_iRelSubindexType[i] = MVAR_GROUP;
                                        pRelNode->m_iRelSubindexExpr[i] = iGroupIndex;
                                }
                        }
                        if( pGrpT != 0 )
                                pGrpT = pGrpT->GetParentGPT();
                }

                if( pRelNode != 0 )
                        pRelNode->m_iSubindexNumber = subindexCount;
        }
#endif

        return( iRelNode );
}


#ifndef GENCODE
GRP_NODE g_GrpNode[4]; // Needed in the return of this method
// when compiling crosstab and GENCODE is
// not available
GRP_NODE* getNextAvailableGrpNodePtr( int *pGrpNodeIdx )
{
        ASSERT( pGrpNodeIdx != 0 );
        static int iCurrentNode = 0;
        if( iCurrentNode >= 4 )
                iCurrentNode = 0;
        int iIndexToUse = iCurrentNode;
        iCurrentNode++;
        if( pGrpNodeIdx != 0 ) *pGrpNodeIdx = iIndexToUse;
        return (g_GrpNode+iIndexToUse);
}
#endif

// RHC END Sep 20, 2001

// grpanal: process group references potentially with subindexes.
//          Used in SKIP TO statements, argument for group functions
int CEngineCompFunc::grpanal( int iSymGroup, bool bAllowDimExpr, int iChecklimit, bool bFromForGroup ) {

        ASSERT( iChecklimit == CHECKLIMIT_NORMAL ||
                iChecklimit == CHECKLIMIT_INSERT ||
                iChecklimit == CHECKLIMIT_DELETE );

        int         iGrpNode;
        GROUPT*     pGrpT;
        int         subindexExpr[DIM_MAXDIM];
        bool        bCompile = true;
        GRP_NODE*   pGrpNode = 0;
        int         i, j;

        memset( subindexExpr, 0, DIM_MAXDIM * sizeof(int) );// RHF Aug 04, 2000

        if( iSymGroup < 0 ) {
                iSymGroup = -iSymGroup;
                bCompile = false;
        }

#ifdef GENCODE
        iGrpNode = Prognext;
        if( Flagcomp ) {
                pGrpNode = NODEPTR_AS( GRP_NODE );
                ADVANCE_NODE( GRP_NODE );                       // RHF Aug 04, 2000
        }
#else
        pGrpNode = getNextAvailableGrpNodePtr(&iGrpNode);
#endif

        if( pGrpNode != 0 )
        {
                pGrpNode->m_iGrpType = GROUP_CODE; // must be different to MVAR_CODE & SVAR_CODE // RHF Aug 07, 2000
                pGrpNode->m_iGrpIndex = iSymGroup;
                pGrpNode->m_iSubindexNumber = 0;
                for( int k = 0; k < DIM_MAXDIM; ++k )
                    pGrpNode->m_iGrpSubindexType[k] = MVAR_NOTHING;
        }

        // Group Reference
        pGrpT = GPT( iSymGroup );
        ASSERT( pGrpT != 0 );
        int iNumDim = pGrpT->GetNumDim();

        int subindexCount = 0;
        int iExplicitIndexes = 0;

        bool ifchecked = false;

        if (iChecklimit!=CHECKLIMIT_NORMAL)
        {
                if (pGrpT->GetMaxOccs() == pGrpT->GetMinOccs())
                {
                        while( Tkn != TOKRPAREN )
                        {
                                NextToken();                      // advance group name
                        }
                        NextToken();                      // advance group name
                        if (iChecklimit==CHECKLIMIT_INSERT)
                        {
                                THROW_PARSER_ERROR0( 8304 );
                        }
                        else
                        {
                                THROW_PARSER_ERROR0( 8305 );
                        }
                }
        }

        if( bCompile )
        {
                NextToken();                      // advance group name

                if( Tkn == TOKLPAREN )
                {
                        if( !bAllowDimExpr )
                                THROW_PARSER_ERROR0( ERROR_NO_DIMENSIONS_ALLOWED );

                        NextToken();

                        if( Tkn == TOKRPAREN && (iChecklimit!=CHECKLIMIT_NORMAL))
                        {
                                if (pGrpT->GetMaxOccs() != 1)
                                {
                                        // if (!ifchecked)
                                        if (ObjInComp == SymbolType::Group)
                                        {
                                                VART* pvart = VPT(InCompIdx);
                                                if (pvart->GetOwnerGroup() != iSymGroup )
                                                {
                                                        issaerror( MessageType::Warning, 8303, pGrpT->GetName().c_str() );

                                                }
                                        }
                                        else if (ObjInComp == SymbolType::Variable )
                                        {
                                                VART* pvart = VPT(InCompIdx);
                                                if (pvart->GetOwnerGroup() != iSymGroup )
                                                {
                                                        issaerror( MessageType::Warning, 8303, pGrpT->GetName().c_str() );
                                                }
                                        }
                                        else  issaerror( MessageType::Warning, 8303, pGrpT->GetName().c_str() );

                                }
                        }

                        bool token_was_comma = false;

                        while( Tkn != TOKRPAREN )
                        {
                                if( subindexCount >= iNumDim )
                                {
                                        THROW_PARSER_ERROR0( ERROR_TOO_MANY_SUBINDEXES );
                                }

                                ifchecked = true;

                                token_was_comma = ( Tkn == TOKCOMMA );

                                if( !token_was_comma )
                                {
                                        // not applicable subscript for insert delete stuff is not allowed
                                        if( SO::EqualsNoCase(Tokstr, _T("Notappl")) )
                                                IssueError( 8308 );

                                        if( iChecklimit != CHECKLIMIT_NORMAL )
                                        {
                                            if( ( Tkn == TOKMINUS ) || ( Tkn == TOKCTE && IsNumericConstantInteger() ) )
                                            {
                                                if( ( Tkn == TOKMINUS ) || ( Tokvalue < 1 || Tokvalue > pGrpT->GetMaxOccs() ) )
                                                        IssueWarning(8302, pGrpT->GetMaxOccs());
                                            }
                                        }

                                        subindexExpr[subindexCount++] = exprlog(); // compile expression

                                        iExplicitIndexes++;

                                        // exprlog reads token just after expression,
                                        // variable( <expr1>, <expr2>, ... )
                                        //                  ^
                                        // we do not need the comma to continue
                                        // so we better skip it.
                                        if( Tkn == TOKCOMMA )
                                                NextToken();
                                }
                                else {
                                        subindexExpr[subindexCount++] = -1; // implicit subindex
                                        NextToken();
                                }
                        }

                        // TOKCOMMA just before TOKRPAREN should be considered too
                        if( token_was_comma )
                                subindexExpr[subindexCount++] = -1; // implicit subindex

                        // Skip right (closing) parenthesis.and get ready for next token
                        ASSERT( Tkn == TOKRPAREN );
                        NextToken();
                }

                else
                {
                        if (pGrpT->GetMaxOccs() != 1 && (iChecklimit != CHECKLIMIT_NORMAL) )
                        {
                                if (ObjInComp == SymbolType::Group)
                                {
                                        VART* pvart = VPT(InCompIdx);
                                        if (pvart->GetOwnerGroup() != iSymGroup )
                                        {
                                                issaerror( MessageType::Warning, 8303, pGrpT->GetName().c_str()); // BUCEN
                                        }
                                }
                                else if (ObjInComp == SymbolType::Variable )
                                {
                                        VART* pvart = VPT(InCompIdx);
                                        if (pvart->GetOwnerGroup() != iSymGroup )
                                        {
                                                //  issaerror( MessageType::Warning, MGF::OpenMessage, _T("Test warning") );
                                                issaerror( MessageType::Warning, 8303, pGrpT->GetName().c_str() );
                                        }
                                }
                                else  issaerror( MessageType::Warning, 8303, pGrpT->GetName().c_str() );
                        }
                }
        }

        if( subindexCount > iNumDim )
                THROW_PARSER_ERROR0( ERROR_TOO_MANY_SUBINDEXES );

        // if user ommits a dimension, he/she is commanded to complete all dimensions
        // rcl, Aug 04, 2004
        if( iExplicitIndexes < subindexCount )
        {
                if( subindexCount != iNumDim )
                        THROW_PARSER_ERROR0( ERROR_NOT_ENOUGH_SUBINDEXES );
        }

        // now we have subindexCount real subindexes. Those not specified will be based
        // in the corresponding occurence counters associated for the group

#ifdef GENCODE
        if( Flagcomp ) {
#endif
                for( j = subindexCount - 1, i = pGrpT->GetNumDim() - 1; i >= 0; j--, i--) {
                        ASSERT( pGrpT );

                        int iGroupIndex = ( pGrpT != NULL ) ? pGrpT->GetSymbolIndex() : 0; // RHF Aug 07, 2000

                        if( j >= 0 && subindexExpr[j] >= 0 ) {
                                pGrpNode->m_iGrpSubindexType[i] = MVAR_EXPR;
                                pGrpNode->m_iGrpSubindexExpr[i] = subindexExpr[j];
                        }
                        else {// implicit index
                                // first check if open for statements for this group
                                // if the group is found use the for variable as subindex
                                // else use the group occurrence number as subindex

                                // Open for's must be searched in reverse order RHC Sep 04, 2000
                                int iVariableInForStack = searchInForStackThisGroup( iGroupIndex );

                                // RHF COM Jul 23, 2001 if( k >= 0 && k < m_ForTableNext ) {} // RHC Sep 04, 2000 add k >= 0
                                if( iVariableInForStack > 0 && !bFromForGroup ) { // RHF Jul 23, 2001
                                    pGrpNode->m_iGrpSubindexType[i] = MVAR_EXPR;
                                    pGrpNode->m_iGrpSubindexExpr[i] = genSVARNode( iVariableInForStack, SymbolType::WorkVariable );// RHF Aug 07, 2000 Add SymbolType::WorkVariable parameter
                                }
                                else {
                                    pGrpNode->m_iGrpSubindexType[i] = MVAR_GROUP;
                                    pGrpNode->m_iGrpSubindexExpr[i] = iGroupIndex;
                                }
                        }
                        pGrpT = pGrpT->GetParentGPT();
                }

                if( pGrpNode != 0 )
                        pGrpNode->m_iSubindexNumber = subindexCount;
#ifdef GENCODE
        }
#endif

        return iGrpNode;                                    // RHF Aug 04, 2000
}

//
// varsanal: process variable references generating the corresponding nodes
//

int CEngineCompFunc::genSVARNode( int iTokStIndex, SymbolType eType )
{
    ASSERT(eType == SymbolType::Variable || eType == SymbolType::WorkVariable); // RHF Aug 04, 2000

    if( eType == SymbolType::Variable )
        VPT(iTokStIndex)->SetUsed( true );

    auto& svar_node = CreateNode<SVAR_NODE>();

    svar_node.m_iVarType = ( eType == SymbolType::WorkVariable ) ? WVAR_CODE : SVAR_CODE;
    svar_node.m_iVarIndex = iTokStIndex;

    return GetProgramIndex(svar_node);
}


//////////////////////////////////////////////////////////////////////////
// varsanal - analysis of variables

#include "helper.h"

// CALC_GROUP_IDX to be used in
// CALC_PARENT_GROUP and RECALC_PARENT_GROUP macros below
#define CALC_GROUP_IDX(pGrpT) (( pGrpT != 0 ) ? pGrpT->GetSymbolIndex() : 0)

#define CALC_PARENT_GROUP(iGroup)                               \
        do { pGrpT = LocalGetParent(pVarT);                     \
             iGroup = CALC_GROUP_IDX(pGrpT); } while( false )

#define RECALC_PARENT_GROUP(iGroup)                             \
        if( pGrpT != 0 ) {                                      \
            pGrpT = LocalGetParent(pGrpT);                      \
            iGroup = CALC_GROUP_IDX(pGrpT);                     \
        }                                                       \
        else                                                    \
            iGroup = 0

//////////////////////////////////////////////////////////////////////////

int CEngineCompFunc::genMVARNode( int iSym, MVAR_NODE* pMVarNodeAux ) {
        int         iProg = Prognext;
#ifdef GENCODE
        MVAR_NODE*  pMVarNode;
#endif

        ASSERT( NPT(iSym)->IsA(SymbolType::Variable) );
        ASSERT( VPT(iSym)->IsArray() );

        if( Flagcomp ) {
                VPT(iSym)->SetUsed( true );
#ifdef GENCODE
                pMVarNode = NODEPTR_AS( MVAR_NODE );
                ADVANCE_NODE( MVAR_NODE );

                pMVarNode->m_iVarType = MVAR_CODE;
                pMVarNode->m_iVarIndex = iSym;

                pMVarNode->m_iSubindexNumber = pMVarNodeAux ? pMVarNodeAux->m_iSubindexNumber : 0;
                for( int i=0; i < DIM_MAXDIM; ++i ) {
                        pMVarNode->m_iVarSubindexType[i] = pMVarNodeAux ? pMVarNodeAux->m_iVarSubindexType[i] : 0;
                        pMVarNode->m_iVarSubindexExpr[i] = pMVarNodeAux ? pMVarNodeAux->m_iVarSubindexExpr[i] : 0;
                }
#endif
        }

        return iProg;
}

int CEngineCompFunc::varsanal( int fmt )
{
        return varsanal( fmt, COMPLETE_COMPILATION, NULL );
}

int CEngineCompFunc::varsanal( int fmt, bool bCompleteCompilation )
{
        return varsanal( fmt, bCompleteCompilation, NULL );
}

// Check the use of varsanal to compile
// "variables" that are really UserFunction, WorkVariable or SingleVariables
//
// if it returns false, then *piVarNode has a VarNode changed to return
// if it returns true, varsanal can continue
bool CEngineCompFunc::varsanal_basicCheck( int* piVarNode, int fmt )
{
    ASSERT( piVarNode != 0 );
    bool bOk = true;

    // assigning to a function's return value
    if( Tkn == TOKUSERFUNCTION )
    {
        auto& svar_node = CreateNode<SVAR_NODE>();

        svar_node.m_iVarType = UF_CODE;
        svar_node.m_iVarIndex = Tokstindex;

        NextToken(); // eat variable name of the function

        return false; // no changes made to piVarNode, use the same
    }

    else if( Tkn == TOKVAR )
    {
        const Symbol& symbol = NPT_Ref(Tokstindex);

        if( symbol.IsA(SymbolType::Variable) && assert_cast<const VART&>(symbol).GetFmt() != fmt )
            throw VarAnalysisException(34);

        if( symbol.IsA(SymbolType::WorkVariable) || !assert_cast<const VART&>(symbol).IsArray() )
        {
            int iVarNode = genSVARNode( Tokstindex, symbol.GetType() );// RHF Aug 07, 2000 Add Tokid parameter

            // eat variable name
            NextToken();

#ifdef BUCEN
            if( Tkn == TOKLPAREN )
                throw VarAnalysisException( 25 );
#endif
            *piVarNode = iVarNode;
            bOk = false;
        }
    }

    return bOk;
}

// varsanal_precompile() parses input and fills VarAnalysis structure for
// code generation
//
// status:
//   variable  - read
//   TOKLPAREN - read
//   ready to read an expression or a TOKRPAREN
void CEngineCompFunc::varsanal_precompile( VarAnalysis& va, bool bCompleteCompilation ) // rcl, Sept 30, 2004
{
        // get next token after left parenthesis
        NextToken();

        if( Tkn == TOKRPAREN ) // then they're passing nothing ... which shouldn't be allowed, 20091208
            issaerror(MessageType::Error, 8303, va.getVarT()->GetName().c_str());

        bool token_was_comma = false;

        while( Tkn != TOKRPAREN ) {
                if( !va.canAddAnotherIndex(bCompleteCompilation) )
                        throw VarAnalysisException(ERROR_TOO_MANY_SUBINDEXES);

                token_was_comma = ( Tkn == TOKCOMMA );

                if( !token_was_comma )
                {
                        int subscript_for_interpreter_speedup = 0;

                        if( ( Tkn == TOKMINUS ) || ( Tkn == TOKCTE && IsNumericConstantInteger() ) )
                        {
                            // invalid subscript error for variables
                            GROUPT* pGrpT = va.getVarT()->GetOwnerGPT( );

                            if( ( Tkn == TOKMINUS ) || ( Tokvalue < 1 || Tokvalue > pGrpT->GetMaxOccs() ) )
                                IssueWarning(8302, pGrpT->GetMaxOccs());

                            else
                                subscript_for_interpreter_speedup = (int)Tokvalue;
                        }

                        // not applicable subscript for variables is not allowed
                        if( SO::EqualsNoCase(Tokstr, _T("Notappl")) )
                                throw VarAnalysisException( 8308 );

                        // compile expression
                        int iPossibleConstExpr = exprlog();
                        va.addIndexValue( iPossibleConstExpr );

#ifdef GENCODE
                        // Interpreter speed up, part 1
                        // Consider integer constants as real contants
                        if( subscript_for_interpreter_speedup > 0 )
                        {
                                // Check if last node generated is a const expr
                                CONST_NODE* pNode = (CONST_NODE*) PPT(iPossibleConstExpr);

                                if( pNode->const_type == CONST_CODE )
                                        va.setPreviousAsConstant( subscript_for_interpreter_speedup );
                        }
#endif
                        // exprlog reads token just after expression,
                        // variable( <expr1>, <expr2>, ... )
                        //                  ^
                        // we do not need the comma to continue
                        // so we better skip it.
                        if( Tkn == TOKCOMMA )
                        {
                                token_was_comma = true;
                                NextToken();
                        }
                }
                else {
                        va.addImplicitIndex();
                        NextToken();
                }
        }

        // TOKCOMMA just before TOKRPAREN should be considered too
        if( token_was_comma )
                va.addImplicitIndex();

        // Skip right (closing) parenthesis.and get ready for next token
        ASSERT( Tkn == TOKRPAREN );

        NextToken();
}


void CEngineCompFunc::symbol_checkBelongToGroup(Symbol& symbol)
{
    VART* pVarT = nullptr;
    const EngineBlock* engine_block = nullptr;
    GROUPT* pSymbolGroupT = nullptr;
    const CDictRecord* pSymbolDictRecord = nullptr;

    if( symbol.IsA(SymbolType::Variable) )
    {
        pVarT = assert_cast<VART*>(&symbol);
        pSymbolGroupT = pVarT->GetOwnerGPT();
        pSymbolDictRecord = pVarT->GetDictItem()->GetRecord();
    }

    else if( symbol.IsA(SymbolType::Block) )
    {
        engine_block = assert_cast<const EngineBlock*>(&symbol);
        pSymbolGroupT = engine_block->GetGroupT();

        const VART* block_vart = engine_block->GetFirstVarT();
        pSymbolDictRecord = ( block_vart != nullptr ) ? block_vart->GetDictItem()->GetRecord() : nullptr;
    }

    else
    {
        ASSERT(false);
    }

    int symbol_group_index = pSymbolGroupT->GetSymbolIndex();
    bool issue_warning = false;

    // added to check for the variable is in the group that is compiled
    if( pSymbolGroupT->GetMaxOccs() > 1 )
    {
        // 20131227 check if this is part of one of the elements that make up a for loop
        if( m_ForTableNext > 0 && pVarT != nullptr )
        {
            int forRelIdx = m_ForTable[m_ForTableNext - 1].forRelIdx;

            if( !forRelIdx ) // 20140203 this was 0 when compiling the show function (which creates uses the for loop infrastructure)
                return;

            RELT* pRelT = RLT(forRelIdx);
            SECT* pSect = SPT(pRelT->GetBaseObjIndex());

            if( pSect == pVarT->GetSPT() )
                return;

            RELATED related;

            if( pRelT->GetRelated(&related, pVarT->GetSymbolIndex(), pVarT->GetDimType(0)) )
                return;
        }

        if( issue_warning )
        {
            // skip the next checks
        }

        else if( m_bcvarsubcheck )
        {
            if( m_icGrpIdx != 0 )
            {
                if( m_icGrpIdx < 0 )
                    m_icGrpIdx = -m_icGrpIdx; // FIXED by SERPRO, Jul 18, 2005 but the warning 8303 should be fixed by Bureau

                GROUPT* pGpt = GPT(m_icGrpIdx);

                if( InCompIdx != symbol_group_index && pSymbolGroupT->GetRecord(0) != pGpt->GetRecord(0) )
                    issue_warning = true;
            }
        }

        else if( ObjInComp == SymbolType::Group )
        {
            GROUPT* pGpt = GPT(InCompIdx);

            //Commented by Savy (R) 20090622
            //if (InCompIdx != pVarT->GetOwnerGroup() && pGrpT->GetRecord(0) != pGpt->GetRecord(0))
            //Added by  Savy (R) 20090716
            //Fix for warning error issue
            if( m_iForRecordIdx != 0 )
            {
                if( m_pEngineArea->GetSectionOfSymbol(symbol.GetSymbolIndex()) != m_iForRecordIdx )
                    issue_warning = true;
            }
            //Added by Savy (R) 20090731
            //Fix for show() warning issue
            else if( m_iShowfnGroupIdx !=0 )
            {
                if( symbol_group_index != m_iShowfnGroupIdx )
                    issue_warning = true;
            }

            else if( pVarT != nullptr && InCompIdx == symbol_group_index && pVarT->GetDictItem()->GetOccurs() > 1 ) // 20130410 multiply occurring items must have subscripts when used in the parent group
            {
                issue_warning = true;
            }

            else if( InCompIdx != symbol_group_index && pSymbolGroupT->GetRecord(0) != pGpt->GetRecord(0) )
            {
                issue_warning = true;
            }
        }

        else if( ObjInComp == SymbolType::Variable )
        {
            VART* pvart = VPT(InCompIdx);

            // 20130410 added third condition due to a report by tom
            if( pVarT != nullptr && pvart->GetOwnerGroup() != symbol_group_index &&
                ( pvart->GetDictItem()->GetRecord() != pSymbolDictRecord ||
                ( pVarT->GetDictItem()->GetOccurs() > 1 && pvart->GetDictItem()->GetOccurs() != pVarT->GetDictItem()->GetOccurs() ) ) )
            {
                issue_warning = true;
            }

            // a similar check for blocks
            else if( engine_block != nullptr && pSymbolGroupT->GetMaxOccs() > 1 && pvart->GetOwnerGroup() != symbol_group_index &&
                ( pvart->GetDictItem()->GetRecord() != pSymbolDictRecord ||
                ( pvart->GetDictItem()->GetOccurs() > 1 ) ) )
            {
                issue_warning = true;
            }
        }

        else if( ObjInComp == SymbolType::Block )
        {
            const EngineBlock& compilation_engine_block = GetSymbolEngineBlock(InCompIdx);
            const VART* compilation_block_vart = compilation_engine_block.GetFirstVarT();
            const CDictRecord* compilation_block_dict_record = ( compilation_block_vart != nullptr ) ? compilation_block_vart->GetDictItem()->GetRecord() :
                                                                                                       nullptr;

            if( pSymbolGroupT->GetMaxOccs() > 1 && pSymbolGroupT != compilation_engine_block.GetGroupT() &&
                compilation_block_dict_record != pSymbolDictRecord )
            {
                issue_warning = true;
            }
        }

        // begin gsf 04-mar-2005
        else if( ObjInComp == SymbolType::Application )
        {
            // If inside a relation, never give the warning
        }

        else
        {
            issue_warning = true;
        }
        // end gsf 04-mar-2005
    }

    if( issue_warning )
        issaerror(MessageType::Warning, 8303, symbol.GetName().c_str());
}


int CEngineCompFunc::blockanal(int block_symbol_index)
{
    EngineBlock& engine_block = GetSymbolEngineBlock(block_symbol_index);
    GROUPT* block_group = engine_block.GetGroupT();

    // compile the block as a group
    int group_node_index = grpanal(block_group->GetSymbol());
    CHECK_SYNTAX_ERROR_AND_THROW(0);

#ifdef GENCODE
    GRP_NODE* group_node = (GRP_NODE*)PPT(group_node_index);
#else
    GRP_NODE* group_node = &g_GrpNode[group_node_index];
#endif

    // if indices were not provided, potentially issue warning messages based on where in logic the block is used
    for( int i = 0; i < block_group->GetNumDim(); ++i )
    {
        // if no indices were provided, then all the indices will be either MVAR_GROUP or MVAR_NOTHING
        if( group_node->m_iGrpSubindexType[i] == MVAR_GROUP || group_node->m_iGrpSubindexType[i] == MVAR_NOTHING )
        {
            symbol_checkBelongToGroup(engine_block);
            break;
        }
    }

#ifdef GENCODE
    // change the group node details to block
    if( Flagcomp )
    {
        group_node->m_iGrpType = BLOCK_CODE;
        group_node->m_iGrpIndex = block_symbol_index;
    }
#endif

    return group_node_index;
}

// getInheritorsSet() explanation
//
// A certain variable is being compiled and another variable
// appears inside its body, like
//
// PROC variable_being_compiled
//        ... variable_in_expression ...
//
// the idea is try to collect all the ancestors of the
// variable_being_compiled and then collect the variable_being_compiled
// ancestor list seeing if there are common elements.
// The common elements are saved in a set which is finally returned
//
std::set<int> getInheritorsSet( VART* pVarInExpression, GROUPT* pBeingCompiled )
{
        std::set<int> aAncestorSet;
        std::set<int> aFinalSet;

        // first build variable-in-expression's ancestor list
        GROUPT* pParent = LocalGetParent( pVarInExpression );

        while( pParent != 0 )
        {
                if( pParent->GetDimType() != CDimension::VoidDim )
                        aAncestorSet.insert(pParent->GetSymbol());

                pParent = LocalGetParent(pParent);
        }

        // then, see if any of the current variable being compiled's ascestor
        // list has common elements.

        if( aAncestorSet.size() > 0 )
        {
                while( pBeingCompiled != 0 )
                {
                        int iSymbol = pBeingCompiled->GetSymbol();
                        if( aAncestorSet.find(iSymbol) != aAncestorSet.end() )
                                aFinalSet.insert( iSymbol );

                        pBeingCompiled = LocalGetParent(pBeingCompiled);
                }
        }

        return aFinalSet;
}

static
bool variableUsesThisDimension( VART* pVarT, CDimension::VDimType vType )
{
        bool bOk = false;
        for( int i = pVarT->GetNumDim() - 1; i >= 0; i-- )
        {
                if( vType == pVarT->GetDimType(i) )
                        bOk = true;
        }

        return bOk;
}

void CEngineCompFunc::varsanal_tryInheritance( MVAR_NODE* pMVarNode, CReadyFlags& readyFlagSet )
{
        // index inheritance should only be applicable for variables
        // different from the one currently being compiled
        if( pMVarNode->m_iVarIndex == InCompIdx )
        {
                TRACE( _T("[varsanal_tryInheritance] Could not be applied when var in expression and compiled var are the same\n") );
                return;
        }

        VART* pVarInCompilation = VPT(InCompIdx);

        if( pVarInCompilation->GetNumDim() <= 0 )
        {
                TRACE( _T("[varsanal_tryInheritance] Could not be applied for single variables (%s is single)\n"),
                        pVarInCompilation->GetName().c_str() );
                return;
        }

        ASSERT( pVarInCompilation->GetNumDim() > 0 );

        VART* pVarInExpression  = VPT(pMVarNode->m_iVarIndex);

        GROUPT* pOwner = pVarInCompilation->GetOwnerGPT();

        std::set<int> aCandidateSet = getInheritorsSet( pVarInExpression, pOwner );

        if( aCandidateSet.size() > 0 )
        {
                int iCandidateNumber = 0;
                int aFinalSet[DIM_MAXDIM];
                memset( aFinalSet, 0, sizeof(int) * DIM_MAXDIM );
                std::set<int>::iterator it;

                // discard candidates that has useless dimension
                for( it = aCandidateSet.begin(); it != aCandidateSet.end(); it++ )
                {
                        // TRACE( "   GROUP IN SET: %d %s ", *it, g2s(GPT(*it)).c_str() );
                        // does this group have the kind of dimension needed?
                        CDimension::VDimType vType = GPT(*it)->GetDimType();
                        ASSERT( vType != CDimension::VoidDim );

                        if( variableUsesThisDimension( pVarInExpression, vType ) )
                        {
                                iCandidateNumber++;
                                aFinalSet[vType] = *it;
                                // TRACE( "valid!" );
                        }
                        // TRACE( "\n" );
                }

                if( iCandidateNumber > 0 )
                {
                        // we did it! will use candidate data to fill

                        /*
                        // TODO: make any ambiguity check?
                        if( iCandidateNumber > readyFlagSet.getTotalSize() )
                        {
                        // opportunity to
                        // - state an ambiguity or
                        // - choose an order
                        }
                        */

                        // we are now choosing to fill implicit
                        // indexes from right to left or
                        // rephrasing ... using *explicit* indexes
                        // from left to right
                        int iMaxDim = pVarInExpression->GetNumDim() - 1;

                        for( int i = iMaxDim; i >= 0; i-- )
                        {
                                CDimension::VDimType vDimensionToUse = pVarInExpression->GetDimType(i);
                                if( aFinalSet[vDimensionToUse] == 0 )
                                {
                                        continue;
                                }

                                if( !readyFlagSet.flagIsSet(i) )
                                {
                                        pMVarNode->m_iVarSubindexType[i] = MVAR_GROUP;
                                        pMVarNode->m_iVarSubindexExpr[i] = aFinalSet[vDimensionToUse];
                                        readyFlagSet.setFlag(i);
                                        if( readyFlagSet.flagsAreCompleted() )
                                                break;
                                }
                        }
                }
        }
}

//
// We have subindexCount real subindexes. Those not specified will be based
// in the corresponding occurence counters associated with the variable
void CEngineCompFunc::varsanal_generateVarNode( VarAnalysis& va, int iVarT, MVAR_NODE* pMVarNode, bool bTryToComplete )
{
        ASSERT( pMVarNode != 0 );

        // basic easy case:
        // all occurrence have been specified explicity

        if( va.allIndexesAreExplicit() )
        {
                // everything is ready to be assigned
                va.copyTo( pMVarNode->m_iVarSubindexExpr, pMVarNode->m_iVarSubindexType );
        }
        else
        {
                VART* pVarT = VPT(iVarT);
                int     iGroupIndex1;
                GROUPT* pGrpT;
                CALC_PARENT_GROUP( iGroupIndex1 );
                // Analyze some cases
                // if( there is some -1 in data )
                // {
                //    data specified will be fixed ("static")
                // }
                // else
                // {
                //    use for stack to complete dimensions
                //    and deduce where specified data goes (if there is some)
                // }

                // Implementation

                if( va.m_iExplicitIndexes > 0 && va.m_iExplicitIndexes < va.m_iSubIndexCount )
                {
                        // there is some -1

                        // check if open for statements for this group
                        // if the group is found use the for variable as subindex
                        // else use the group occurrence number as subindex

                        for( int i = va.m_iNumDim - 1; i >= 0; i-- )
                        {
                                // whatever has been specified, it is now fixed
                                if( va.thisIndexIsSpecified(i) )
                                {
                                        va.copyTo_OnlyOneIndexDirect( pMVarNode->m_iVarSubindexExpr,
                                                pMVarNode->m_iVarSubindexType,
                                                i );
                                }
                                else
                                {
                                        int iVariableInForStack =
                                                searchInForStackThisGroupOrVar( iGroupIndex1, iVarT, pVarT->GetDimType(i) );

                                        if( iVariableInForStack > 0 ) {
                                                pMVarNode->m_iVarSubindexType[i] = MVAR_EXPR;
                                                pMVarNode->m_iVarSubindexExpr[i] = genSVARNode( iVariableInForStack, SymbolType::WorkVariable );
                                        }
                                        else {
                                                if( ObjInComp == SymbolType::Group )
                                                        if( GPT(InCompIdx)->GetGroupType() == GROUPT::eGroupType::Level )
                                                        {
                                                                SetSyntErr(ERROR_NOT_ENOUGH_SUBINDEXES);
                                                                // pMVarNode->m_iVarSubindexType and
                                                                // pMVarNode->m_iVarSubindexExpr arrays should be filled anyway.
                                                                // want to prevent that? => uncomment the line below
                                                                // break;
                                                        }

                                                pMVarNode->m_iVarSubindexType[i] = MVAR_GROUP;
                                                pMVarNode->m_iVarSubindexExpr[i] = iGroupIndex1;
                                        }
                                }

                                // recalc iGroupIndex1
                                RECALC_PARENT_GROUP( iGroupIndex1 );
                        }
                }
                else
                {
                        // Deduce where data goes
                        // first: check how many explicit indexes has been specified
                        //        and how many do we have to get from stack or
                        //        from "inheritance methods"
                        //
                        // rcl 2004

                        int iNumberToGetFromContext = va.m_iNumDim - va.m_iExplicitIndexes;
                        CReadyFlags readyFlagSet;
                        readyFlagSet.setTotalSet( iNumberToGetFromContext );

                        // first we will try to protect specified indexes
                        // so that stack or similar calculations wont overwrite its info.
                        if( va.allIndexesAreSpecified() )
                        {
                                for( int i = 0; i < va.m_iNumDim; ++i )
                                {
                                        if( va.thisIndexIsSpecified(i) )
                                                readyFlagSet.protectFlag(i);
                                }
                        }

                        if( forStackHasElements() )
                        {
                                for( int i = va.m_iNumDim - 1; i >= 0; i-- )
                                {
                                        if( readyFlagSet.flagIsSet(i) )
                                                continue;

                                        int iVariableInForStack =
                                                searchInForStackThisGroupOrVar( iGroupIndex1, iVarT, pVarT->GetDimType(i) );

                                        if( iVariableInForStack > 0 )
                                        {
                                                pMVarNode->m_iVarSubindexType[i] = MVAR_EXPR;
                                                pMVarNode->m_iVarSubindexExpr[i] = genSVARNode( iVariableInForStack, SymbolType::WorkVariable );
                                                readyFlagSet.setFlag(i);
                                                if( readyFlagSet.flagsAreCompleted() )
                                                        break;
                                        }

                                        RECALC_PARENT_GROUP( iGroupIndex1 );
                                }
                        }

                        // try to apply index inheritance
                        // from the variable or group currently being compiled

                        if( !readyFlagSet.flagsAreCompleted() )
                        {
                                if( ObjInComp == SymbolType::Group )
                                {
                                        GROUPT* pGrpInCompilation = GPT(InCompIdx);
                                        if( pGrpInCompilation->GetNumDim() > 0 &&
                                                pGrpInCompilation->GetGroupType() != GROUPT::eGroupType::Level )
                                        {
                                                TRACE( _T("[varsanal] Proc %s [%d-D group] compiling %s\n"),
                                                        pGrpInCompilation->GetName().c_str(),
                                                        pGrpInCompilation->GetNumDim(),
                                                        VPT(pMVarNode->m_iVarIndex)->GetName().c_str() );
                                        }
                                }
                                else
                                        if( ObjInComp == SymbolType::Variable )
                                        {
                                                // index inheritance only applicable for variables
                                                // different from the one currently being compiled
                                                if( pMVarNode->m_iVarIndex != InCompIdx )
                                                {
                                                        varsanal_tryInheritance( pMVarNode, readyFlagSet );
                                                }
                                        }
                        }

                        readyFlagSet.setTotalSet( va.m_iNumDim );

                        // Time to unprotect protected flags
                        // because next to this comes the specification towards pMVarNode
                        if( va.allIndexesAreSpecified() )
                        {
                                for( int i = 0; i < va.m_iNumDim; ++i )
                                {
                                        if( va.thisIndexIsSpecified(i) )
                                                readyFlagSet.unprotectFlag(i);
                                }
                        }

                        // fill from left to right from specified indexes
                        if( va.howManyIndexesSpecified() > 0 )
                        {
                                int j = -1;
                                for( int i = 0; i < va.m_iNumDim; ++i )
                                {
                                        if( readyFlagSet.flagIsSet(i) )
                                                continue;

                                        // jump until next explicit index
                                        for( ++j; j < va.m_iSubIndexCount; ++j )
                                        {
                                                if( va.thisIndexIsSpecified(j) )
                                                        break;
                                        }

                                        if( j < va.m_iSubIndexCount )
                                        {
                                                ASSERT( !readyFlagSet.flagIsSet(i) );

                                                va.copyTo_OnlyOneIndex( pMVarNode->m_iVarSubindexExpr,
                                                        pMVarNode->m_iVarSubindexType,
                                                        i, j );

                                                readyFlagSet.setFlag(i);
                                        }

                                        if( readyFlagSet.flagsAreCompleted() )
                                                break;
                                }
                        }

                        // If we are commanded not to try to complete [bTryToComplete=false]
                        // There is at least a case when we have to do it anyway
                        // because, user has written fixed dimensions,
                        // so we correct this command by configuring bTryToComplete
                        // to true when allIndexAreSpecified()
                        // An example:  S( , x )  <-- x should *not* be dynamic
                        if( !bTryToComplete && !readyFlagSet.flagsAreCompleted() )
                        {
                                if( va.allIndexesAreSpecified() )
                                        bTryToComplete = true;
                        }

                        if( !readyFlagSet.flagsAreCompleted() )
                        {
                                // fill the blanks with groupt info when bTryToComplete is true
                                //                  and with a hint when bTryToComplete is false
                                CALC_PARENT_GROUP( iGroupIndex1 );

                                for( int i = va.m_iNumDim - 1; i >= 0; i-- )
                                {
                                        if( !readyFlagSet.flagIsSet(i) )
                                        {
                                                if( bTryToComplete )
                                                {
                                                        pMVarNode->m_iVarSubindexType[i] = MVAR_GROUP;
                                                        readyFlagSet.setFlag(i);
                                                }
                                                else
                                                {
                                                        // Indicate that we were unable to calculate, and that
                                                        // interpreter must calculate the rest
                                                        pMVarNode->m_iVarSubindexType[i] = MVAR_USE_DYNAMIC_CALC;
                                                }
                                                pMVarNode->m_iVarSubindexExpr[i] = iGroupIndex1;
                                                if( readyFlagSet.flagsAreCompleted() )
                                                        break;
                                        }

                                        RECALC_PARENT_GROUP( iGroupIndex1 );
                                }
                        } // if( !readyFlagSet.flagsAreCompleted() )
                }

                // Try to modify MVAR_NODE generated according to
                // m_bIgnoreForPrecedence and m_iGroupToIgnoreForPrecedence
                // rcl, Jul 2005
                if( m_bIgnoreForPrecedence )
                {
                        bool bChanged = false;
                        ASSERT( m_iGroupToIgnoreForPrecedence != 0 );
                        int     iGroupIndex2;
                        CALC_PARENT_GROUP( iGroupIndex2 );

                        for( int i = va.m_iNumDim - 1; i >= 0; i-- )
                        {
                                if( iGroupIndex2 == m_iGroupToIgnoreForPrecedence )
                                {
                                        pMVarNode->m_iVarSubindexType[i] = MVAR_GROUP;
                                        pMVarNode->m_iVarSubindexExpr[i] = iGroupIndex2;
                                        bChanged = true;
                                        break;
                                }
                                RECALC_PARENT_GROUP( iGroupIndex2 );
                        }

                        // not changed yet?
                        //  try
                        //     - section iteration
                        //     - others?
                        if( !bChanged )
                        {
                                // section iteration
                                if( pVarT->GetOwnerSec() == m_iGroupToIgnoreForPrecedence )
                                {
                                        pMVarNode->m_iVarSubindexType[0] = MVAR_GROUP;
                                        pMVarNode->m_iVarSubindexExpr[0] = m_iGroupToIgnoreForPrecedence;
                                        bChanged = true;
                                }
                        }
                }
        }
}


int CEngineCompFunc::varsanal( int fmt, bool bCompleteCompilation, bool* pbAllIndexesSpecified, bool bTryToComplete )
{
        MVAR_NODE*  pMVarNode = 0;

        int iVarNode = Prognext;

        try
        {
                if( !varsanal_basicCheck( &iVarNode, fmt ) )
                        // if false, UserFunction, WorkVariable or SingleVariable detected
                        // get out of here
                        return iVarNode;

                // Multiple Variable Reference

                int iVarT = Tokstindex;
                VART* pVarT = VPT( iVarT );


                VarAnalysis va( pVarT->GetNumDim() );

                if( Flagcomp )
                        pVarT->SetUsed( true );

#ifdef GENCODE
                if( Flagcomp ) { // RHF Aug 04, 2000
                        pMVarNode = NODEPTR_AS( MVAR_NODE );
                        ADVANCE_NODE( MVAR_NODE );                      // RHF Aug 04, 2000
                } // RHF Aug 04, 2000
#else
                // When GENCODE is not available, we use internal structure to be able
                // to check later special conditions just after varsanal() call.
                // This is necesary at least for Crosstab checkings and dynamic calculation
                //
                // rcl, Nov 2004
                iVarNode = getNewMVarNodeIndex();
                pMVarNode = getLocalMVarNodePtr( iVarNode );
#endif
                if( pMVarNode != 0 )
                {
                        pMVarNode->m_iVarType = MVAR_CODE;
                        pMVarNode->m_iVarIndex = Tokstindex;
                        pMVarNode->m_iSubindexNumber = 0;
                        for( int i = 0; i < DIM_MAXDIM; ++i )
                                pMVarNode->m_iVarSubindexType[i] = MVAR_NOTHING;
                }

                // advance variable name
                NextToken();

                bool bOldSubscriptCheck = m_bcvarsubcheck; // 20120429

                if( Tkn == TOKHAS )
                    m_bcvarsubcheck = true; // turn off subscript checking

                if( pbAllIndexesSpecified )
                        *pbAllIndexesSpecified = false;

                if( Tkn == TOKLPAREN )
                {
                        va.setVarT(pVarT);
                        varsanal_precompile( va, bCompleteCompilation ); // rcl, Sept 30, 2004
                }
                else
                {
                        symbol_checkBelongToGroup(*pVarT);
                }

                // Check everything is ok to continue
                int iErrorCode;
                if( !va.indexLimitsAreCorrect(&iErrorCode) )
                        THROW_PARSER_ERROR0( iErrorCode );

                if( pbAllIndexesSpecified )
                        *pbAllIndexesSpecified = va.allIndexesAreExplicit();


#ifdef GENCODE
                if( Flagcomp )
#endif
                        varsanal_generateVarNode( va, iVarT, pMVarNode, bTryToComplete );

                if( pMVarNode != 0 )
                        pMVarNode->m_iSubindexNumber = va.howManyIndexesSpecified();

                m_bcvarsubcheck = bOldSubscriptCheck;
        }
        catch( VarAnalysisException e )
        {
                THROW_PARSER_ERROR0(e.getErrorCode());
        }

        return iVarNode;
}

//////////////////////////////////////////////////////////////////////////

int CEngineCompFunc::tvarsanal()
{
    const CTAB* pCtab = XPT(Tokstindex);

    auto& tnode = CreateCompilationNode<TVAR_NODE>(TVAR_CODE);
    tnode.tvar_index = Tokstindex;

    for( int i = 0; i < 3; ++i )
        tnode.tvar_exprindex[i] = -1;

    NextToken();
    IssueErrorOnTokenMismatch(TOKLPAREN, ERROR_INVALID_FUNCTION_CALL);

    for( int i = 0; i < pCtab->GetNumDim(); ++i )
    {
        if( i > 0 )
        {
            if( Tkn != TOKCOMMA )
                IssueError(8311);
        }

        NextTokenHelperResult next_token_helper_result = CheckNextTokenHelper();
        NextToken();

        if( Tkn == TOKMINUS || next_token_helper_result == NextTokenHelperResult::NumericConstantNonNegative )
        {
            // not applicable subscript for arrays is not allowed
            if( Tokvalue == NOTAPPL )
                IssueError(8308);

            // invalid subscript error for arrays
            if( ( Tkn == TOKMINUS ) || ( Tokvalue < 0 || Tokvalue >= pCtab->GetTotDim(i) ) )
                IssueError(8302, pCtab->GetTotDim(i) - 1);
        }

        int subscript_expression = exprlog();

        tnode.tvar_exprindex[i] = subscript_expression;
    }

    IssueErrorOnTokenMismatch(TOKRPAREN, 17);

    NextToken();

    return GetCompilationNodeProgramIndex(tnode);
}


int CEngineCompFunc::rutfunc()
{
    // rutfunc: calls a function-analyzer

    // analyze user-defined functions...
    if( Tkn == TOKUSERFUNCTION )
        return CompileUserFunctionCall();

    // ...or built-in functions
    ASSERT(CurrentToken.function_details != nullptr);

    using CompilationFunction = int(CEngineCompFunc::*)();

    static std::map<Logic::FunctionCompilationType, CompilationFunction> CompilationFunctionMap =
    {
        { Logic::FunctionCompilationType::ArgumentsFixedN,          &LogicCompiler::CompileFunctionsArgumentsFixedN },
        { Logic::FunctionCompilationType::ArgumentsVaryingN,        &LogicCompiler::CompileFunctionsArgumentsVaryingN },
        { Logic::FunctionCompilationType::ArgumentSpecification,    &LogicCompiler::CompileFunctionsArgumentSpecification },

        { Logic::FunctionCompilationType::Removed,                  &LogicCompiler::CompileFunctionsRemovedFromLanguage },

        { Logic::FunctionCompilationType::Various,                  &LogicCompiler::CompileFunctionsVarious },

        { Logic::FunctionCompilationType::Impute,                   &LogicCompiler::CompileImputeFunction },
        { Logic::FunctionCompilationType::Invoke,                   &LogicCompiler::CompileInvokeFunction },
        { Logic::FunctionCompilationType::GPS,                      &LogicCompiler::CompileGpsFunction },
        { Logic::FunctionCompilationType::Paradata,                 &LogicCompiler::CompileParadataFunction },
        { Logic::FunctionCompilationType::SetFile,                  &LogicCompiler::CompileSetFileFunction },
        { Logic::FunctionCompilationType::SetValueSet,              &LogicCompiler::CompileSetValueSetFunction },
        { Logic::FunctionCompilationType::Sync,                     &LogicCompiler::CompileSyncFunctions },
        { Logic::FunctionCompilationType::Trace,                    &LogicCompiler::CompileTraceFunction },
        { Logic::FunctionCompilationType::Userbar,                  &LogicCompiler::CompileUserbarFunction },

        // dictionary related
        { Logic::FunctionCompilationType::DictionaryVarious,        &LogicCompiler::CompileDictionaryFunctionsVarious },
        { Logic::FunctionCompilationType::CaseSearch,               &LogicCompiler::CompileDictionaryFunctionsCaseSearch },
        { Logic::FunctionCompilationType::CaseIO,                   &LogicCompiler::CompileDictionaryFunctionsCaseIO },
        { Logic::FunctionCompilationType::Case,                     &LogicCompiler::CompileCaseFunctions },

        { Logic::FunctionCompilationType::Item,                     &LogicCompiler::CompileItemFunctions },

        // symbols and namespaces
        { Logic::FunctionCompilationType::Array,                    &LogicCompiler::CompileLogicArrayFunctions },
        { Logic::FunctionCompilationType::Audio,                    &LogicCompiler::CompileLogicAudioFunctions },
        { Logic::FunctionCompilationType::Barcode,                  &LogicCompiler::CompileBarcodeFunctions },
        { Logic::FunctionCompilationType::CS,                       &LogicCompiler::CompileActionInvokerFunctions },
        { Logic::FunctionCompilationType::Document,                 &LogicCompiler::CompileLogicDocumentFunctions },
        { Logic::FunctionCompilationType::File,                     &LogicCompiler::CompileLogicFileFunctions },
        { Logic::FunctionCompilationType::Geometry,                 &LogicCompiler::CompileLogicGeometryFunctions },
        { Logic::FunctionCompilationType::HashMap,                  &LogicCompiler::CompileLogicHashMapFunctions },
        { Logic::FunctionCompilationType::Image,                    &LogicCompiler::CompileLogicImageFunctions },
        { Logic::FunctionCompilationType::List,                     &LogicCompiler::CompileLogicListFunctions },
        { Logic::FunctionCompilationType::Map,                      &LogicCompiler::CompileLogicMapFunctions },
        { Logic::FunctionCompilationType::Message,                  &LogicCompiler::CompileMessageFunctions },
        { Logic::FunctionCompilationType::NamedFrequency,           &LogicCompiler::CompileNamedFrequencyFunctions },
        { Logic::FunctionCompilationType::Path,                     &LogicCompiler::CompilePathFunctions },
        { Logic::FunctionCompilationType::Pff,                      &LogicCompiler::CompileLogicPffFunctions},
        { Logic::FunctionCompilationType::Report,                   &LogicCompiler::CompileReportFunctions },
        { Logic::FunctionCompilationType::Symbol,                   &LogicCompiler::CompileSymbolFunctions },
        { Logic::FunctionCompilationType::SystemApp,                &LogicCompiler::CompileSystemAppFunctions },
        { Logic::FunctionCompilationType::UserInterface,            &LogicCompiler::CompileUserInterfaceFunctions },
        { Logic::FunctionCompilationType::ValueSet,                 &LogicCompiler::CompileValueSetFunctions },

        // other
        { Logic::FunctionCompilationType::FN2,                      &CEngineCompFunc::cfun_compile_count },
        { Logic::FunctionCompilationType::FN3,                      &CEngineCompFunc::cfun_compile_sum },
        { Logic::FunctionCompilationType::FN4,                      &CEngineCompFunc::cfun_fn4 },
        { Logic::FunctionCompilationType::FN6,                      &CEngineCompFunc::cfun_fn6 },
        { Logic::FunctionCompilationType::FN8,                      &CEngineCompFunc::cfun_fn8 },
        { Logic::FunctionCompilationType::FNS,                      &CEngineCompFunc::cfun_fns },
        { Logic::FunctionCompilationType::FNC,                      &CEngineCompFunc::cfun_fnc },
        { Logic::FunctionCompilationType::FNB,                      &CEngineCompFunc::cfun_fnb },
        { Logic::FunctionCompilationType::FNTC,                     &CEngineCompFunc::cfun_fntc },
        { Logic::FunctionCompilationType::FNH,                      &CEngineCompFunc::cfun_fnh },
        { Logic::FunctionCompilationType::FNG,                      &CEngineCompFunc::cfun_fng },
        { Logic::FunctionCompilationType::FNGR,                     &CEngineCompFunc::cfun_fngr },
        { Logic::FunctionCompilationType::FNID,                     &CEngineCompFunc::cfun_fnins },
        { Logic::FunctionCompilationType::FNSRT,                    &CEngineCompFunc::cfun_fnsrt },
        { Logic::FunctionCompilationType::FNMAXOCC,                 &CEngineCompFunc::cfun_fnmaxocc },
        { Logic::FunctionCompilationType::FNINVALUESET,             &CEngineCompFunc::cfun_fninvalueset },
        { Logic::FunctionCompilationType::FNEXECSYSTEM,             &CEngineCompFunc::cfun_fnexecsystem },
        { Logic::FunctionCompilationType::FNSHOW,                   &CEngineCompFunc::cfun_fnshow },
        { Logic::FunctionCompilationType::FNITEMLIST,               &CEngineCompFunc::cfun_fnitemlist },
        { Logic::FunctionCompilationType::FNDECK,                   &CEngineCompFunc::cfun_fndeck },
        { Logic::FunctionCompilationType::FNCAPTURETYPE,            &CEngineCompFunc::cfun_fncapturetype },
        { Logic::FunctionCompilationType::FNOCCS,                   &CEngineCompFunc::cfun_fnoccs },
        { Logic::FunctionCompilationType::FNNOTE,                   &CEngineCompFunc::cfun_fnnote },
        { Logic::FunctionCompilationType::FNSTRPARM,                &CEngineCompFunc::cfun_fnstrparm },
        { Logic::FunctionCompilationType::FNPROPERTY,               &CEngineCompFunc::cfun_fnproperty },
    };

    const auto& compilation_function_lookup = CompilationFunctionMap.find(CurrentToken.function_details->compilation_type);

    if( compilation_function_lookup == CompilationFunctionMap.cend() )
        IssueError(21);

    return (this->*compilation_function_lookup->second)();
}


//-----------------------------------------------------------------------
//  cfun_fnitemlist : compile function / class FNITEMLIST
//              --> for functions that pass lists of items and records, to be evaluated by the interpreter
//-----------------------------------------------------------------------

int CEngineCompFunc::cfun_fnitemlist() // 20091203
{
    int iFunCode = CurrentToken.function_details->code;
    ASSERT(iFunCode == FNCOUNTVALID_CODE);

    NextToken();

    IssueErrorOnTokenMismatch(TOKLPAREN, ERROR_INVALID_FUNCTION_CALL);

    int numArgs = 0;
    CArray<int,int> arguments;

    bool next_token_is_variable_with_wildcard = false;

    auto set_next_token_is_variable_with_wildcard = [this, &next_token_is_variable_with_wildcard]() -> void
    {
        MarkInputBufferToRestartLater();

        // the wildcard parameter is (*)
        next_token_is_variable_with_wildcard =
            ( NextToken().code == TOKVAR &&
              NextToken().code == TOKLPAREN &&
              NextToken().code == TOKMULOP &&
              NextToken().code == TOKRPAREN );

        RestartFromMarkedInputBuffer();
    };

    set_next_token_is_variable_with_wildcard();
    NextToken();

    while( Tkn != TOKRPAREN )
    {
        ItemListType itemType = ItemListType::NUMERICFUNC;

        int codePtr;
        int index;  // used as another parameter when a record or item is indexed

        if( Tkn == TOKSECT )
        {
            int savedTokstindex = Tokstindex;

            itemType = ItemListType::RECORD;
            
            SECT* pSecT = SPT(Tokstindex);
            codePtr = pSecT->GetContainerIndex();

            // set all the items in the record to used
            GROUPT* pGroupT;
            int iGroupNum = 0;
            while( ( pGroupT = pSecT->GetGroup(iGroupNum) ) != NULL )
            {
                for( int iItem = 0; iItem < pGroupT->GetNumItems(); iItem++ )
                {
                    int iSymb = pGroupT->GetItemSymbol(iItem);

                    if( !iSymb )
                        break;

                    VART* pVarT = VPT(iSymb);

                    pVarT->SetUsed(true);
                }

                iGroupNum++;
            }

            NextToken();

            if( pSecT->GetMaxOccs() == 1 && Tkn == TOKLPAREN )
                THROW_PARSER_ERROR0(8301);

            // the user should specify a record number, be in a group statement, or elect to process all the records
            else if( pSecT->GetMaxOccs() > 1 )
            {
                itemType = ItemListType::RECORD_INDEXED;

                if( Tkn == TOKLPAREN ) // they are indexing the record
                {
                    NextToken();

                    if( Tkn == TOKMULOP ) // the user wants to process all records
                    {
                        NextToken();

                        if( Tkn != TOKRPAREN )
                            THROW_PARSER_ERROR0(19);

                        index = -1;
                        NextToken();
                    }

                    else // the user is passing an expression
                    {
                        index = exprlog();

                        if( Tkn != TOKRPAREN )
                            THROW_PARSER_ERROR0(19);

                        NextToken();
                    }

                    CHECK_SYNTAX_ERROR_AND_THROW(0);
                }

                else // there is no index so the user needs to be in a group or for expression
                {
                    if( m_iForRecordIdx != 0 && m_iForRecordIdx == savedTokstindex )
                    {
                        // the index should be on the top of the for table

                        index = -1; // just in case the for table lookup fails

                        if( m_ForTableNext > 0 )
                            index = genSVARNode(m_ForTable[m_ForTableNext - 1].forVarIdx, SymbolType::WorkVariable);
                    }

                    else // check for group
                    {
                        if( ObjInComp == SymbolType::Variable && VPT(InCompIdx)->m_pSecT == pSecT )
                            index = -2;

                        else
                            THROW_PARSER_ERROR0(8350);
                    }
                }

            }
        }

        else if( Tkn == TOKARRAY )
        {
            int savedTokstindex = Tokstindex;

            // first check if this is an individual value
            bool is_array_element = true;

            MarkInputBufferToRestartLater();

            try
            {
                Logic::BaseCompilerSettings compiler_settings_modifier = ModifyCompilerSettings();
                compiler_settings_modifier.SuppressErrorReporting();

                codePtr = exprlog();
            }

            catch( const Logic::ParserError& )
            {
                is_array_element = false;
            }

            if( is_array_element )
            {
                ClearMarkedInputBuffer();
            }

            else // if there was an error reading it as a single value it should just be an array name
            {
                clearSyntaxErrorStatus();

                RestartFromMarkedInputBuffer();
                NextToken();

                const LogicArray& logic_array = GetSymbolLogicArray(savedTokstindex);

                if( logic_array.GetDataType() != DataType::Numeric )
                    IssueError(955, ToString(DataType::Numeric));

                itemType = ItemListType::ARRAY;
                codePtr = savedTokstindex;
            }
        }

        else
        {
            if( next_token_is_variable_with_wildcard && NPT_Ref(Tokstindex).IsA(SymbolType::Variable) )
            {
                VART* pVarT = VPT(Tokstindex);

                if( pVarT->GetSPT()->GetMaxOccs() < 2 && pVarT->GetMaxOccs() < 2 ) // if it's not a multiple occurrence
                    THROW_PARSER_ERROR0(8352);

                if( pVarT->GetDataType() != DataType::Numeric )
                    THROW_PARSER_ERROR0(119);

                // skip past the wildcard
                NextToken();
                NextToken();
                NextToken();

                pVarT->SetUsed(true);

                itemType = ItemListType::MULTIPLYOCCURINGITEM;
                codePtr = Tokstindex;

                NextToken();
            }

            else
            {
                codePtr = exprlog();
            }
        }

        numArgs++;

        if( Flagcomp )
        {
            arguments.Add(Prognext);

            CONST_NODE * itemNode = NODEPTR_AS(CONST_NODE);
            ADVANCE_NODE(CONST_NODE);

            itemNode->const_type = (int)itemType;
            itemNode->const_index = codePtr;

            if( itemType == ItemListType::RECORD_INDEXED || itemType == ItemListType::WORKINGVARIABLE ||
                itemType == ItemListType::FUNCWITHVARIABLE ) // there will be another parameter in this case
            {
                int* extraParameter = NODEPTR_AS(int);
                ADVANCE_NODE(int);

                *extraParameter = index;
            }
        }

        if( Tkn == TOKCOMMA )
        {
            set_next_token_is_variable_with_wildcard();
            NextToken();
        }

        else if( Tkn != TOKRPAREN ) // an unexpected value in the argument stream
            THROW_PARSER_ERROR0(528);
    }

    if( numArgs == 0 )
        THROW_PARSER_ERROR0(49); // they need to pass at least item

    IssueErrorOnTokenMismatch(TOKRPAREN, 17);

    int iProg = Prognext;

    if( Flagcomp )
    {
        FNN_NODE* ptrfunc = NODEPTR_AS(FNN_NODE);
        OC_CreateCompilationSpace(2 + numArgs); // in lieu of ADVANCE_NODE(FNN_NODE) because of the variable number of arguments

        ptrfunc->fn_code = iFunCode;
        ptrfunc->fn_nargs = numArgs;

        for( int i = 0; i < numArgs; ++i )
            ptrfunc->fn_expr[i] = arguments.GetAt(i);
    }

    NextToken();

    return iProg;
}



//-----------------------------------------------------------------------
//  cfun_fndeck : compile function / class FNDECK
//              --> for functions that user DeckArrays
//-----------------------------------------------------------------------
int CEngineCompFunc::cfun_fndeck()
{
    FunctionCode function_code = CurrentToken.function_details->code;
    int putdeck_value_expression = 0;
    bool update_spillover_rows = false;
    std::vector<int> index_expressions;

    NextToken();
    IssueErrorOnTokenMismatch(TOKLPAREN, ERROR_INVALID_FUNCTION_CALL);

    NextToken();
    IssueErrorOnTokenMismatch(TOKARRAY, 47510);

    const LogicArray& logic_array = GetSymbolLogicArray(Tokstindex);
    const std::vector<int>& deck_array_symbols = logic_array.GetDeckArraySymbols();

    // check if there is at least one value set used as a dimension
    if( !std::any_of(deck_array_symbols.begin(), deck_array_symbols.end(), [](int symbol_index) { return symbol_index != 0; }) )
        IssueError(47511);

    NextToken();

    if( function_code == FNPUTDECK_CODE )
    {
        // 20120409 converting the format from putdeck+(hd_name,...) to putdeck(hd_name(+))
        if( Tkn == TOKLPAREN )
        {
            NextToken();

            if( Tkn == TOKADDOP )
            {
                NextToken();

                if( Tkn == TOKRPAREN )
                {
                    update_spillover_rows = true;
                    NextToken();
                }
            }

            if( !update_spillover_rows ) // then there was a compilation error
                IssueError(ERROR_INVALID_FUNCTION_CALL);
        }

        IssueErrorOnTokenMismatch(TOKCOMMA, 528);

        NextToken();
        putdeck_value_expression = exprlog();
    }

    for( size_t i = 0; i < deck_array_symbols.size(); ++i )
    {
        if( Tkn != TOKRPAREN )
            NextToken();

        bool use_implied_index = ( Tkn == TOKCOMMA || Tkn == TOKRPAREN );
        int index_expression = -1;

        // if a value set isn't being used for this dimension, then an an index must be supplied
        if( deck_array_symbols[i] == 0 )
        {
            if( use_implied_index )
                IssueError(47513);

            index_expression = exprlog();
        }

        // otherwise, unless they are using an implied index, then they must specify a value
        else if( !use_implied_index )
        {
            const ValueSet& value_set = GetSymbolValueSet(abs(deck_array_symbols[i]));
            index_expression = CompileExpression(value_set.GetDataType());
        }

        index_expressions.emplace_back(index_expression);
    }

    IssueErrorOnTokenMismatch(TOKRPAREN, 17);

    NextToken();

    auto& deck_array_node = CreateVariableArgumentCompilationNode<DECK_ARRAY_NODE>(function_code, index_expressions.size());

    deck_array_node.array_symbol_index = logic_array.GetSymbolIndex();
    deck_array_node.putdeck_value_expression = putdeck_value_expression;
    deck_array_node.update_spillover_rows = update_spillover_rows ? 1 : 0;
    memcpy(deck_array_node.index_expressions, index_expressions.data(), index_expressions.size() * sizeof(index_expressions[0]));

    return GetCompilationNodeProgramIndex(deck_array_node);
}


//-----------------------------------------------------------------------
//  cfun_fncapturetype : compile function / class FNCAPTURETYPE
//-----------------------------------------------------------------------
int CEngineCompFunc::cfun_fncapturetype() // 20100608
{
    int iFunCode = CurrentToken.function_details->code;
    int iNumArgs = 1;
    int iSymVar;
    int pCode;
    int pDate;

    if( iFunCode == FNGETCAPTURETYPE_CODE )
        IssueWarning(Logic::ParserMessage::Type::DeprecationMinor, 95016, _T("CaptureType"));

    else if( iFunCode == FNSETCAPTURETYPE_CODE )
        IssueWarning(Logic::ParserMessage::Type::DeprecationMinor, 95015, _T("CaptureType / CaptureDateFormat"));

    else if( iFunCode == FNSETCAPTUREPOS_CODE )
        IssueWarning(Logic::ParserMessage::Type::DeprecationMinor, 95015, _T("CapturePosX / CapturePosY"));

    NextToken();
    IssueErrorOnTokenMismatch(TOKLPAREN, 517);

    NextToken();

    switch( Tkn )
    {
        case TOKDICT_PRE80:
        case TOKFORM:
        case TOKGROUP:
        case TOKBLOCK:
            if( iFunCode == FNGETCAPTURETYPE_CODE )
                return( SetSyntErr(91701),0);

            iSymVar = Tokstindex;
            break;

        case TOKVAR:
            iSymVar = Tokstindex;
            if( NPT(iSymVar)->IsA(SymbolType::Variable) && VPT(iSymVar)->SYMTfrm > 0 )
                break;

        default:
            return( SetSyntErr(91700),0);
    }

    NextToken();

    if( iFunCode == FNSETCAPTURETYPE_CODE )
    {
        iNumArgs++;

        if( Tkn != TOKCOMMA )
            return( SetSyntErr(91702),0);

        NextToken();

        pCode = exprlog();

        if( Tkn == TOKCOMMA ) // the user can specify a date
        {
            iNumArgs++;
            NextToken();
            pDate = CompileStringExpression();
        }
    }

    else if( iFunCode == FNSETCAPTUREPOS_CODE ) // 20110502
    {
        iNumArgs += 2;

        if( Tkn != TOKCOMMA )
            return( SetSyntErr(91702),0);

        NextToken();

        pCode = exprlog(); // the x coordinate

        if( Tkn != TOKCOMMA )
            return( SetSyntErr(91702),0);

        NextToken();

        pDate = exprlog(); // the y coordinate
    }

    if( Tkn != TOKRPAREN )
        return(SetSyntErr(ERROR_RIGHT_PAREN_EXPECTED),0);

    NextToken();

    int iProg = Prognext;

    if( Flagcomp )
    {
        FNN_NODE* ptrfunc = NODEPTR_AS(FNN_NODE);

        OC_CreateCompilationSpace(iNumArgs - 1);

        ADVANCE_NODE(FNN_NODE);

        ptrfunc->fn_code = iFunCode;
        ptrfunc->fn_nargs = iNumArgs;
        ptrfunc->fn_expr[0] = iSymVar;

        if( iNumArgs >= 2 )
            ptrfunc->fn_expr[1] = pCode;

        if( iNumArgs == 3 )
            ptrfunc->fn_expr[2] = pDate;
    }

    return iProg;
}


void CEngineCompFunc::MarkAllInSectionUsed(SECT* pSecT)
{
    int iSymVar = pSecT->SYMTfvar;

    while( iSymVar > 0 )
    {
        VART* pVarT = VPT(iSymVar);
        pVarT->SetUsed(true);
        iSymVar = pVarT->SYMTfwd;
    }
}


int CEngineCompFunc::cfun_fnoccs()
{
    // a function for compiling occurrence related functions (typically used for CAPI)
    FunctionCode function_code = CurrentToken.function_details->code;

    NextToken();
    IssueErrorOnTokenMismatch(TOKLPAREN, 517);

    NextToken();

    const Symbol* symbol;

    // if no argument is provided (to getocclabel), use the current procedure symbol
    if( function_code == FNGETOCCLABEL_CODE && Tkn == TOKRPAREN )
    {
        symbol = NPT(InCompIdx);
    }

    // otherwise we can read a multiply occurring record, item, group, or block
    else if( Tkn == TOKRECORD || Tkn == TOKSECT || Tkn == TOKVAR || Tkn == TOKGROUP || Tkn == TOKBLOCK )
    {
        symbol = NPT(Tokstindex);
        NextToken();
    }

    else
    {
        IssueError(921);
    }

    // only getocclabel can work on anything but a group
    if( function_code != FNGETOCCLABEL_CODE && !symbol->IsA(SymbolType::Group) )
        IssueError(922);

    // find the repeating symbol
    symbol = SymbolCalculator::GetFirstSymbolWithOccurrences(*symbol);

    if( symbol == nullptr )
        IssueError(921);

    unsigned max_occurrences = SymbolCalculator::GetMaximumOccurrences(*symbol);
    int occurrence_expression = -1;
    int second_argument = -1;

    // evaluate an occurrence number if given
    if( Tkn == TOKLPAREN )
    {
        bool check_valid_occurrence = ( CheckNextTokenHelper() == NextTokenHelperResult::NumericConstantNonNegative );
        NextToken();

        if( check_valid_occurrence )
        {
            if( !IsNumericConstantInteger() || Tokvalue < 1 || Tokvalue > max_occurrences )
                IssueError(8302, (int)max_occurrences);
        }

        occurrence_expression = exprlog();

        IssueErrorOnTokenMismatch(TOKRPAREN, ERROR_RIGHT_PAREN_EXPECTED);

        NextToken();
    }

    // see if a better implicit occurrence can be calculated (when not in a user-defined function)
    else if( ObjInComp != SymbolType::Application )
    {
        const Symbol* compilation_symbol = NPT(InCompIdx);
        bool warn_about_occurrences = false;

        // implicit occurrences can only be determined when in a procedure for a block or variable
        if( !compilation_symbol->IsOneOf(SymbolType::Block, SymbolType::Variable) )
        {
            warn_about_occurrences = true;
        }

        else if( compilation_symbol != symbol )
        {
            const Symbol* compilation_symbol_with_occurrences = SymbolCalculator::GetFirstSymbolWithOccurrences(*compilation_symbol);

            if( compilation_symbol_with_occurrences != symbol )
            {
                // if the compilation symbol's group is the same as this symbol's group, then create a curocc node
                // to simulate an implied index (this allows, for example, setocclabel without an occurrence to work
                // not only on the current group but also on other groups that are comprised of the same record);
                // a clearer example of this might be that getocclabel(POP_REC001) can be called from items in POP_REC002
                if( compilation_symbol_with_occurrences != nullptr && compilation_symbol_with_occurrences->IsA(SymbolType::Group) &&
                    symbol->IsA(SymbolType::Group) && assert_cast<const GROUPT*>(symbol)->RecordsAreIdentical(*assert_cast<const GROUPT*>(compilation_symbol_with_occurrences)) )
                {
                    auto& group_node = CreateCompilationNode<FNGR_NODE>();
                    group_node.fn_code = FNCUROCC_CODE;
                    group_node.m_iSym = 0;
                    group_node.m_iArgumentType = FNGR_Spec::ARG_NO_ARGUMENTS;
                    occurrence_expression = GetCompilationNodeProgramIndex(group_node);
                }

                else
                {
                    warn_about_occurrences = true;
                }
            }
        }

        if( warn_about_occurrences )
            issaerror(MessageType::Warning, 8303, symbol->GetName().c_str());
    }

    // evaluate any additional arguments...

    // setocclabel expects a label
    if( function_code == FNSETOCCLABEL_CODE )
    {
        IssueErrorOnTokenMismatch(TOKCOMMA, 528);

        NextToken();
        second_argument = CompileStringExpression();
    }

    // an optional second argument to showocc is a dynamically evaluated boolean about whether to show or hide the occurrence
    else if( function_code == FNSHOWOCC_CODE && Tkn == TOKCOMMA )
    {
        NextToken();
        second_argument = exprlog();
    }


    IssueErrorOnTokenMismatch(TOKRPAREN, ERROR_RIGHT_PAREN_EXPECTED);

    NextToken();

    return CreateVariousNode(function_code,
        {
            symbol->GetSymbolIndex(),
            occurrence_expression,
            second_argument
        });
}


int CEngineCompFunc::cfun_fnnote() // for compiling getnote, putnote, editnote
{
    FunctionCode function_code = CurrentToken.function_details->code;
    const Symbol* symbol = nullptr;
    const EngineDictionary* engine_dictionary = nullptr;
    const DICT* pDicT = nullptr;
    std::optional<int> variable_expression;
    std::optional<int> operator_id_expression;
    std::optional<int> note_text_expression;


    NextToken();
    IssueErrorOnTokenMismatch(TOKLPAREN, ERROR_INVALID_FUNCTION_CALL);

    NextTokenWithPreference(SymbolType::Section);

    // putnote: read the note text
    if( function_code == FunctionCode::FNPUTNOTE_CODE )
    {
        note_text_expression = CompileStringExpression();

        if( Tkn == TOKCOMMA )
            NextTokenWithPreference(SymbolType::Section);
    }

    // all functions: read the dictionary symbol and the optional operator ID
    if( Tkn != TOKRPAREN )
    {
        if( Tkn == TOKDICT || Tkn == TOKRECORD )
        {
            if( Tkn == TOKDICT )
                VerifyEngineCase();

            symbol = NPT(Tokstindex);
            engine_dictionary = SymbolCalculator::GetEngineDictionary(*symbol);
        }

        else if( Tkn == TOKDICT_PRE80 || Tkn == TOKSECT || Tkn == TOKVAR || Tkn == TOKGROUP )
        {
            symbol = NPT(Tokstindex);
            pDicT = SymbolCalculator(GetSymbolTable()).GetDicT(*symbol);
        }

        if( engine_dictionary == nullptr && pDicT == nullptr )
            IssueError(46500);

        // do a variable analysis if necessary
        if( Tkn == TOKVAR )
        {
            const VART* pVarT = assert_cast<const VART*>(symbol);
            variable_expression = varsanal(pVarT->GetFmt());
            CHECK_SYNTAX_ERROR_AND_THROW(0);

            if( pVarT->GetLevel() > LvlInComp && pVarT->GetSubType() != SymbolSubType::External )
                IssueError(93);
        }

        else
        {
            NextToken();
        }

        // read the operator ID
        if( Tkn == TOKCOMMA )
        {
            NextToken();
            operator_id_expression = CompileStringExpression();
        }
    }


    // if a field was not supplied, make sure that we are in a valid PROC or in a user-defined function
    if( engine_dictionary == nullptr && pDicT == nullptr )
    {
        if( ObjInComp != SymbolType::Application )
        {
            engine_dictionary = SymbolCalculator::GetEngineDictionary(NPT_Ref(InCompIdx));
            pDicT = SymbolCalculator(GetSymbolTable()).GetDicT(NPT_Ref(InCompIdx));

            if( engine_dictionary == nullptr && pDicT == nullptr )
                IssueError(46500);
        }
    }

    if( engine_dictionary != nullptr )
    {
        engine_dictionary->GetCaseAccess()->SetUsesNotes();
    }

    else if( pDicT != nullptr )
    {
        pDicT->GetCaseAccess()->SetUsesNotes();
    }


    IssueErrorOnTokenMismatch(TOKRPAREN, 17);

    NextToken();

    auto& note_node = CreateCompilationNode<FNNOTE_NODE>(function_code);

    note_node.symbol_index = ( symbol != nullptr ) ? symbol->GetSymbolIndex() : -1;
    note_node.variable_expression = variable_expression.value_or(-1);
    note_node.operator_id_expression = operator_id_expression.value_or(-1);
    note_node.note_text_expression = note_text_expression.value_or(-1);

    int program_index = GetCompilationNodeProgramIndex(note_node);

    if( engine_dictionary != nullptr )
        program_index = WrapNodeAroundValidDataAccessCheck(program_index, ( symbol != nullptr ) ? *symbol : *engine_dictionary, function_code);

    return program_index;
}


// for compiling functions that take a defined string parameter as a first argument and then a variable number of arguments
int CEngineCompFunc::cfun_fnstrparm()
{
    FunctionCode function_code = CurrentToken.function_details->code;
    bool argument_required = ( CurrentToken.function_details->number_arguments > 0 );
    std::vector<int> arguments;

    NextToken();
    IssueErrorOnTokenMismatch(TOKLPAREN, ERROR_INVALID_FUNCTION_CALL);

    NextTokenHelperResult next_token_helper_result = CheckNextTokenHelper();
    NextToken();

    if( argument_required || Tkn != TOKRPAREN )
    {
        // if the parameter is known at compile-time, validate it
        struct ParameterDetails
        {
            CString text;
            int min_arguments;
            int max_arguments;
        };

        std::optional<ParameterDetails> parameter_details;

        if( next_token_helper_result == NextTokenHelperResult::StringLiteral )
        {
            parameter_details.emplace();
            parameter_details->text = WS2CS(Tokstr);

            ParameterManager::Parameter parameter = ParameterManager::Parse(function_code, parameter_details->text,
                &parameter_details->min_arguments, &parameter_details->max_arguments);

            if( parameter == ParameterManager::Parameter::Invalid )
                IssueError(1100, parameter_details->text.GetString());
        }

        arguments.emplace_back(CompileStringExpression());

        // keep reading the presented string arguments
        while( Tkn == TOKCOMMA )
        {
            NextToken();

            arguments.emplace_back(CompileStringExpression());
        }

        // check if the number of arguments is valid
        if( parameter_details.has_value() )
        {
            int provided_arguments = (int)arguments.size() - 1;

            if( provided_arguments < parameter_details->min_arguments || provided_arguments > parameter_details->max_arguments )
                IssueError(1101, parameter_details->text.GetString(), provided_arguments);
        }
    }

    IssueErrorOnTokenMismatch(TOKRPAREN, 17);

    NextToken();

    return CreateVariableArgumentsWithSizeNode(function_code, arguments);
}


int CEngineCompFunc::cfun_fnproperty()
{
    FunctionCode function_code = CurrentToken.function_details->code;
    bool set_function = ( function_code == FunctionCode::FNSETPROPERTY_CODE );

    std::vector<int> arguments(CurrentToken.function_details->number_arguments, -1);
    size_t argument_counter = 0;

    bool first_argument_was_dictionary_related_symbol = false;
    CString property_name;
    std::optional<ParameterManager::ParameterArgument> property_type;

    NextToken();
    IssueErrorOnTokenMismatch(TOKLPAREN, ERROR_INVALID_FUNCTION_CALL);

    NextTokenHelperResult next_token_helper_result = CheckNextTokenHelper();
    NextToken();

    if( next_token_helper_result == NextTokenHelperResult::DictionaryRelatedSymbol ) // BINARY_TYPES_TO_ENGINE_TODO: getpropery/setproperty need to support binary dictionary items
    {
        first_argument_was_dictionary_related_symbol = true;
        arguments[argument_counter++] = Tokstindex;

        NextToken();

        if( Tkn == TOKCOMMA )
        {
            next_token_helper_result = CheckNextTokenHelper();
            NextToken();
        }
    }

    if( next_token_helper_result == NextTokenHelperResult::StringLiteral )
    {
        property_name = WS2CS(Tokstr);

        ParameterManager::Parameter parameter = ParameterManager::Parse(FNGETPROPERTY_CODE, property_name);

        if( parameter == ParameterManager::Parameter::Invalid )
            IssueError(1100, property_name.GetString());

        else if( set_function && ParameterManager::Parse(FNSETPROPERTY_CODE, property_name) == ParameterManager::Parameter::Invalid )
            IssueError(1102, property_name.GetString());

        property_type = ParameterManager::GetAdditionalArgument(parameter);
    }

    if( argument_counter == 0 || Tkn != TOKRPAREN )
    {
        arguments[argument_counter++] = CompileStringExpression();
    }

    if( set_function && Tkn == TOKCOMMA )
    {
        NextToken();

        DataType value_data_type = GetCurrentTokenDataType();
        arguments[argument_counter++] = static_cast<int>(value_data_type);
        arguments[argument_counter++] = CompileExpression(value_data_type);
    }

    // check that the arguments are valid
    size_t min_arguments = set_function ? 3 : 1;

    if( argument_counter < min_arguments )
        IssueError(set_function ? 7011 : 7012);

    if( first_argument_was_dictionary_related_symbol && argument_counter == min_arguments )
    {
        // if a symbol is being used as the parameter value (rather than as a symbol), the symbol has to be alphanumeric
        Symbol* symbol = NPT(arguments[0]);

        if( !symbol->IsA(SymbolType::Variable) || assert_cast<const VART*>(symbol)->IsNumeric() )
            IssueError(7013);

        assert_cast<VART*>(symbol)->SetUsed(true);
    }

    if( bool application_property = ( property_type == ParameterManager::ParameterArgument::ApplicationProperty );
        application_property || property_type == ParameterManager::ParameterArgument::SystemProperty )
    {
        if( argument_counter > min_arguments )
            IssueError(1106, application_property ? _T("application") : _T("system"), property_name.GetString());
    }

    else if( bool item_property = ( property_type == ParameterManager::ParameterArgument::ItemProperty );
             item_property || property_type == ParameterManager::ParameterArgument::FieldProperty )
    {
        if( argument_counter == min_arguments )
        {
            IssueError(1105, property_name.GetString());
        }

        else if( first_argument_was_dictionary_related_symbol && !set_function )
        {
            if( !NPT(arguments[0])->IsA(SymbolType::Variable) )
                IssueError(item_property ? 1103 : 1104, property_name.GetString());
        }
    }

    IssueErrorOnTokenMismatch(TOKRPAREN, 17);

    NextToken();

    return CreateVariousNode(function_code, arguments);
}


// CEngineCompFunc::compile_parentGrouptSpec( Parent )
//
// used in compile_count to analyze the group specification
//
// if( Parent != 0 )
//     Use grpanal allowing parenthesis
// else
//     Use grpanal without allowing parenthesis
//     (Syntax error if used)
//
// count( group where condition )         <--
// count( group(a,b,c) where condition )  <--
//
// depends on group situation
//
// rcl, Jul 22, 2004
//
int CEngineCompFunc::compile_parentGrouptSpec( int iSymGroup, GROUPT* pGroupParent )
{
        int iGrp = -1;
        if( pGroupParent != 0 )
        {
                iGrp = grpanal( pGroupParent->GetSymbol(), ALLOW_PARENTHESIS, CHECK_LIMITS_NORMAL );
#ifdef GENCODE
                // backpatch to be used in excount()
                GRP_NODE* pgrpNode = (GRP_NODE*) ( PPT(iGrp) );
                ASSERT( pgrpNode != 0 );
                if( pgrpNode != 0 )
                        pgrpNode->m_iGrpIndex = iSymGroup;
#endif
        }
        else
        {
                iGrp = grpanal( iSymGroup, DO_NOT_ALLOW_PARENTHESIS, CHECK_LIMITS_NORMAL );
        }

        return iGrp;
}

//-----------------------------------------------------------------------
//  cfun_compile_count: compile function / class FN2
//                      --> COUNT( sect_name where expr )
//-----------------------------------------------------------------------
int CEngineCompFunc::cfun_compile_count() {
        int     iProg       = Prognext;
        int     iFunCode    = CurrentToken.function_details->code;
        bool    bIsValidGroup = false;                      // victor Aug 02, 99
        int     isymGroup   = 0;                            // victor Aug 02, 99
        int     i;
        bool    bSection=false;
        bool    bMultItem=false; // RHF Apr 15, 2004
        bool    bIsGroup=false; // RHF Aug 01, 2005

        if( Flagvars == 1 )
            IssueError(38);

        FNGR_NODE curoccGrpIdx; // 20091028
        m_pCuroccGrpIdx = NULL;

        // 20120413 trevor reported a bug whereby a count used in a for loop ended up resulting in many invalid subscript messages
        bool oldVarsubcheck = m_bcvarsubcheck;
        int oldGrpIdx = m_icGrpIdx;


#ifdef GENCODE
        FN2_NODE*   ptrfunc = NODEPTR_AS( FN2_NODE );
        if( Flagcomp ) {
                ADVANCE_NODE( FN2_NODE );

                ptrfunc->fn_code = iFunCode;
        }
#endif
        NextToken();
        IssueErrorOnTokenMismatch(TOKLPAREN, ERROR_INVALID_FUNCTION_CALL);

        bool onlyConditionCount = false; // 20110901 remove need to put a group name when doing a only conditional count

        MarkInputBufferToRestartLater();

        bool stopParsing = false;
        int numParenthesis = 1;
        int numSymbols = 0;
        int savedTokstindex = 0;

        NextTokenWithPreference(SymbolType::Section);

        while( !stopParsing && numParenthesis )
        {
            numSymbols++;

            bool readNextTkn = true;

            if( !isymGroup && IsCurrentTokenVART(*this) )
            {
                savedTokstindex = Tokstindex;

                VART* pVarT = VPT(Tokstindex);
                isymGroup = pVarT->GetParentGroup();

                if( !( isymGroup > 0 && m_pEngineArea->GroupMaxNumOccs(isymGroup) > 1 ) || !pVarT->IsNumeric() )
                    isymGroup = 0; // this isn't a proper variable

                if( GetSyntErr() != 0 )
                {
                    isymGroup = 0; // this isn't a proper variable
                    clearSyntaxErrorStatus();
                }
            }

            else if( Tkn == TOKLPAREN )
                numParenthesis++;

            else if( Tkn == TOKRPAREN )
                numParenthesis--;

            else if( Tkn == TOKWHERE ) // then the user isn't using this shortcut
                stopParsing = true;

            else if( Tkn == TOKEOP )
                stopParsing = true;

            if( readNextTkn )
                NextToken();
        }

        if( !numParenthesis && isymGroup && numSymbols > 2 ) // 2 = the item and the right paranthesis
            onlyConditionCount = true;

        RestartFromMarkedInputBuffer();

        int iGroupToIgnoreForPrecedence = Tokstindex;
        COUNT_type count_type = COUNT_BAD_DATA;

        if( onlyConditionCount )
        {
            count_type = COUNT_VAR;
            bIsValidGroup = true;
            m_icGrpIdx = isymGroup;

            curoccGrpIdx.fn_arg = savedTokstindex;
            curoccGrpIdx.m_iArgumentType = FNGR_Spec::ARG_WHERE_MV;
            m_pCuroccGrpIdx = &curoccGrpIdx;

            iGroupToIgnoreForPrecedence = isymGroup;

            GENERATE_CODE( ptrfunc->sect_ind = isymGroup );
            GENERATE_CODE( ptrfunc->count_type = count_type );

            NextToken();
        }

        else // 20110901 now the original count code
        {
            NextTokenWithPreference(SymbolType::Section);

            if( Tkn == TOKGROUP || Tkn == TOKBLOCK ) {   // now work with Groups       // victor Aug 02, 99
                    bIsGroup = true;  // RHF Aug 01, 2005

                    isymGroup = ( Tkn == TOKGROUP ) ? Tokstindex :
                                                      GetSymbolEngineBlock(Tokstindex).GetGroupT()->GetSymbol();

                    m_icGrpIdx = isymGroup;

                    count_type = COUNT_GROUP;

                    if( m_pEngineArea->GroupMaxNumOccs(isymGroup) <= 1 )
                    {
                            isymGroup = 0;// RHF Feb 27, 2004 only multiple groups are allowed
                            bIsValidGroup = false;
                    }
                    else
                    {
                            GROUPT* pGroupParent = GPT(isymGroup)->GetParentGPT();

                            int iGrp = compile_parentGrouptSpec( isymGroup, pGroupParent );

                            if( GetSyntErr() != 0 )
                                    THROW_PARSER_ERROR0( 90015 );

                            bIsValidGroup = true;
                            isymGroup = iGrp;

                            curoccGrpIdx.fn_arg = Tokstindex;       // 20091028 (so curocc() can work properly within the function)
                            curoccGrpIdx.m_iArgumentType = FNGR_Spec::ARG_WHERE_GROUP;
                            m_pCuroccGrpIdx = &curoccGrpIdx;
                    }
            }
            // RHF INIC Dec 26, 2003
            // RHF COM Feb 27, 2004 else if( Tkn == TOKSECT ) {
            else if( Tkn == TOKSECT && SPT(Tokstindex)->GetMaxOccs() >= 2 ) { // RHF Feb 27, 2004 Only multiple sections are allowed
                    isymGroup = Tokstindex;
                    bIsValidGroup = true;
                    count_type = COUNT_SECTION;
                    bSection = true;

                    NextToken();
            }
            // RHF END Dec 26, 2003
            else if( Tkn == TOKVAR && VPT(Tokstindex)->IsArray() ) {
                    // RHF Oct 24, 2000isymGroup = VPT(Tokstindex)->GetOwnerGroup();
                    // RHF COM Feb 26, 2004 isymGroup = VPT(Tokstindex)->GetParentGroup();// RHF COM Oct 24, 2000

                    // RHF INIC Feb 26, 2004
                    VART* pVarT = VPT(Tokstindex);
                    count_type = COUNT_VAR;

                    bool bIsTrueItem = ( pVarT->GetOwnerSymItem() != 0 );
                    if( bIsTrueItem && pVarT->GetNextSubItem() != NULL ) { //Has subitems
                            isymGroup = -Tokstindex;
                            bMultItem = true; // RHF Apr 15, 2004
                    }
                    else
                            isymGroup = pVarT->GetParentGroup();

                    // Note: The difference used to be recognized by the iSymGroup sign
                    //       if it was > 0 -> it was a group
                    //       if it was < 0 -> it was a Var.
                    //       Now this difference will be considered only for count_type = COUNT_VAR.

                    bIsValidGroup = true;
                    // RHF END Feb 26, 2004
                    NextToken();

                    m_icGrpIdx = isymGroup;

                    curoccGrpIdx.fn_arg = Tokstindex;       // 20091028 (so curocc() can work properly within the function)
                    curoccGrpIdx.m_iArgumentType = FNGR_Spec::ARG_WHERE_MV;
                    m_pCuroccGrpIdx = &curoccGrpIdx;

                    iGroupToIgnoreForPrecedence = isymGroup;
            }

            if( isymGroup != 0 ) {
                    // check it comes from a FormFile
                    bIsValidGroup = true;//RHF COM Jun 16, 2000( GPT(isymGroup)->GetSource() == 1 );
            }

            if( !bIsValidGroup )
                    THROW_PARSER_ERROR0( 90015 );

            ASSERT( count_type != COUNT_BAD_DATA );

            GENERATE_CODE( ptrfunc->sect_ind = isymGroup );    // victor Aug 02, 99
            GENERATE_CODE( ptrfunc->count_type = count_type ); // rcl    Jul 18, 04

            if( Tkn == TOKRPAREN) {

                    GENERATE_CODE( ptrfunc->fn_exp = -1 );

                    NextToken();

                    m_icGrpIdx = 0; // 20110901 fixed warning messages that tom reported (from djibouti)
                    m_pCuroccGrpIdx = NULL;

                    return iProg;
            }
            else
            {
                    if( Tkn != TOKWHERE )
                            THROW_PARSER_ERROR0( 16 );
                    else
                            NextToken();
            }

        } // 20110901 end the original count code


        Flagvars = 1;                   // allow for Mult var without index
        stopUsingForPrecedence( iGroupToIgnoreForPrecedence );

        m_bcvarsubcheck = bIsValidGroup;
        if( !bSection && !bMultItem && !bIsGroup ) // RHF Aug 01, 2005 Add bIsGroup
                m_icGrpIdx = isymGroup;

        int oldForCount = m_ForTableNext; // 20120510 the count statement didn't work properly in for loops
        m_ForTableNext = 0;

        i = exprlog();                  // i = pointer to logical expression

        m_ForTableNext = oldForCount;

        m_pCuroccGrpIdx = NULL; // 20091027

        m_bcvarsubcheck = oldVarsubcheck; // 20120413 rewrote a little
        m_icGrpIdx = oldGrpIdx;

        Flagvars = 0;                   // require Mult var to have an index
        useForPrecedence();

        CHECK_SYNTAX_ERROR_AND_THROW(0);

        IssueErrorOnTokenMismatch(TOKRPAREN, 17);

        NextToken();

#ifdef GENCODE
        if( Flagcomp ) {
                ptrfunc->fn_exp = i;
        }
#endif

        return iProg;
}


//-----------------------------------------------------------------------
//  cfun_compile_sum : compile function / class FN3
//              --> SUM, AVERAGE, MIN, MAX ( var_name { where expr } )
//-----------------------------------------------------------------------
int CEngineCompFunc::cfun_compile_sum() {
        int     iProg      = Prognext;
        int     iFunCode   = CurrentToken.function_details->code;
        bool    bInMultGroup = false;                       // victor Aug 02, 99
        int     isymGroup  = 0;                             // victor Aug 02, 99
        int     i;

        if( Flagvars == 1 )
            IssueError(38);

#ifdef GENCODE
        FNGR_NODE curoccGrpIdx; // 20091028
#endif
        m_pCuroccGrpIdx = NULL;

//SAVY 04/08/08 Glenn's fix for subscript check
//Fix turn off subscript check -Ignore subscript check for SUM, AVERAGE, MIN, MAX ( var_name { where expr } )
        m_bcvarsubcheck = true;
#ifdef GENCODE
        FN3_NODE*   ptrfunc = NODEPTR_AS( FN3_NODE );

        if( Flagcomp ) {

                if( iFunCode == FNSEEK_CODE ) // 20100602
                    OC_CreateCompilationSpace(1); // FN3 isn't big enough, we need one more int, i'm putting this before ADVANCE_NODE so the out of memory message gets checked there

                ADVANCE_NODE( FN3_NODE );

                ptrfunc->fn_code = iFunCode;
        }
#endif
        NextToken();                    // name of the function
        IssueErrorOnTokenMismatch(TOKLPAREN, ERROR_INVALID_FUNCTION_CALL);

        bool seekWithoutWhere = false;
        int savedTokstindex;
        VART* pVarT;
        int iVarNode;

        bool bSeekFunction = iFunCode == FNSEEK_CODE || iFunCode == FNSEEKMIN_CODE || iFunCode == FNSEEKMAX_CODE;
        bool bNormalSeek = iFunCode == FNSEEK_CODE;

        if( bSeekFunction ) // 20100603 see if  var_name where  isn't included
        {
            MarkInputBufferToRestartLater();

            bool stopParsing = false;
            int numParenthesis = 1;

            NextToken();

            while( !stopParsing && numParenthesis )
            {
                bool readNextTkn = true;

                if( IsCurrentTokenVART(*this) && !isymGroup )
                {
                    isymGroup = VPT(Tokstindex)->GetParentGroup();
                    bInMultGroup = ( isymGroup > 0 && m_pEngineArea->GroupMaxNumOccs(isymGroup) > 1 );
                    savedTokstindex = Tokstindex;

                    // if( !bInMultGroup || !VPT(Tokstindex)->IsNumeric() )
                    if( !bInMultGroup || ( !VPT(Tokstindex)->IsNumeric() && !bNormalSeek ) ) // 20130409 we should be able to seek for conditions that begin with an alpha
                        isymGroup = 0; // this isn't a proper variable

                    pVarT = VPT(savedTokstindex);

                    // 20120326 turn off the for table, which throws off varsanal
                    int tempForTable = m_ForTableNext;
                    m_ForTableNext = 0;

                    iVarNode = varsanal(pVarT->GetFmt(),NOT_COMPLETE_COMPILATION);

                    m_ForTableNext = tempForTable;

                    readNextTkn = false;

                    if( GetSyntErr() != 0 )
                    {
                        isymGroup = 0; // this isn't a proper variable
                        clearSyntaxErrorStatus();
                    }
                }

                else if( Tkn == TOKLPAREN )
                    numParenthesis++;

                else if( Tkn == TOKRPAREN )
                    numParenthesis--;

                else if( Tkn == TOKWHERE ) // then the user isn't using this shortcut
                    stopParsing = true;

                else if( Tkn == TOKEOP ) // 20110406 the seek statement never ended
                    stopParsing = true;

                if( readNextTkn )
                    NextToken();
            }

            if( !numParenthesis && isymGroup )
                seekWithoutWhere = true;

            RestartFromMarkedInputBuffer();
        }

        NextToken();

        if( !seekWithoutWhere )
        {
            if( Tkn == TOKVAR ) {
                    if( NPT_Ref(Tokstindex).IsA(SymbolType::WorkVariable) )
                            THROW_PARSER_ERROR0( 90018 ); // RHF Mar 02, 2001

                    // RHF COM Oct 24, 2000isymGroup = VPT(Tokstindex)->GetOwnerGroup();
                    // RHF COM Oct 24, 2000bInMultGroup = ( m_pEngineArea->GroupMaxNumOccs(isymGroup) > 1 );
                    isymGroup = VPT(Tokstindex)->GetParentGroup();// RHF Oct 24, 2000
                    bInMultGroup = ( isymGroup > 0 && m_pEngineArea->GroupMaxNumOccs(isymGroup) > 1 );// RHF Oct 24, 2000
            }
            if( !bInMultGroup )
                    THROW_PARSER_ERROR0( 90018 );

            if( !VPT(Tokstindex)->IsNumeric() && !bNormalSeek ) // 20130409 added second condition
                    THROW_PARSER_ERROR0( 119 ); // numeric variable expected

            savedTokstindex = Tokstindex;
        }


        if( VPT(savedTokstindex)->GetSubType() == SymbolSubType::Input || VPT(savedTokstindex)->GetSubType() == SymbolSubType::External )
                VPT(savedTokstindex)->SetUsed( true );

        if( !seekWithoutWhere )
        {
            pVarT = VPT(savedTokstindex);

            // 20120326 turn off the for table, which throws off varsanal
            int tempForTable = m_ForTableNext;
            m_ForTableNext = 0;

            iVarNode = varsanal( pVarT->GetFmt(), NOT_COMPLETE_COMPILATION );

            m_ForTableNext = tempForTable;

            if( GetSyntErr() != 0 )
                    THROW_PARSER_ERROR0( 90015 );
        }

#ifdef GENCODE
        if( Flagcomp ) {
                // make a search and try to get which group will
                // be used to iterate...
                // Hopefully, we find only 1 MVAR_GROUP.

                MVAR_NODE* pVarNode = (MVAR_NODE*) PPT(iVarNode);
                int iNumGroups = 0;
                int iIndexForGroup;

                //////////////////////////////////////////////////////////////////////////
                //////////////////////////////////////////////////////////////////////////

                for( i = 0; i < pVarT->GetNumDim(); ++i )
                {
                        if( pVarNode->m_iVarSubindexType[i] == MVAR_GROUP )
                        {
                                iNumGroups++;
                                iIndexForGroup = i;
                        }
                }

                // ASSERT( iNumGroups >= 1 );
                if( iNumGroups >= 1 )
                {
                        ptrfunc->sect_ind = pVarNode->m_iVarSubindexExpr[iIndexForGroup];
                }
                else
                {
                        SetSyntErr(ERROR_TOO_MANY_SUBINDEXES);
                        ptrfunc->sect_ind = -1; // guess later?
                }

                ptrfunc->iSymVar = iVarNode;

                curoccGrpIdx.fn_arg = savedTokstindex;  // 20091028 (so curocc() can work properly within the function)
                curoccGrpIdx.m_iArgumentType = FNGR_Spec::ARG_WHERE_MV;
                m_pCuroccGrpIdx = &curoccGrpIdx;

        }
#endif

        // NextToken();  // already done in varsanal

        if( !seekWithoutWhere )
        {
            //if( Tkn == TOKRPAREN) {
            if( Tkn == TOKRPAREN && !bSeekFunction )
            {
                    GENERATE_CODE( ptrfunc->fn_exp = -1 );

                    NextToken();

                    m_pCuroccGrpIdx = NULL; // 20120827 apply the same resets that would have occurred had there been a where condition
                    m_bcvarsubcheck = false;

                    return iProg;
            }
            else if( Tkn != TOKWHERE )
                    THROW_PARSER_ERROR0( 16 );
            else
                    NextToken();
        }

        Flagvars = 1;                   // allow for Mult var without index
        stopUsingForPrecedence( isymGroup );

        i = exprlog();                  // i = pointer to logical expression

        m_pCuroccGrpIdx = NULL; // 20091027

        Flagvars = 0;                   // require Mult var to have an index
        useForPrecedence();

        CHECK_SYNTAX_ERROR_AND_THROW(0);

        int seekStartPos = 0;

        if( bSeekFunction && Tkn != TOKRPAREN ) // 20100601
        {
            // seek can have a parameter that indicates the starting position for the search
            if( Tkn != TOKCOMMA )
                THROW_PARSER_ERROR0(1); // in reality we shouldn't get here because the error would probably be picked up in the above exprlog

            NextToken();

            // 20110811 a new option will be to get the 2nd, 3rd, etc. count by saying @2, or @spouseNum
            if( Tkn == TOKATOP && iFunCode == FNSEEK_CODE )
            {
                NextToken();
                seekStartPos = -1 * exprlog(); // the negative sign will indicate this case
            }

            else // the original compilation
                seekStartPos = exprlog();
        }

        IssueErrorOnTokenMismatch(TOKRPAREN, 17);

        NextToken();

        GENERATE_CODE( ptrfunc->fn_exp = i );

#ifdef GENCODE
        if( bSeekFunction )
        {
            int * pSeekInfo = (int *)((char *)ptrfunc + sizeof(*ptrfunc));
            *pSeekInfo = seekStartPos;
        }
#endif


        //SAVY 04/08/08 Glenn's fix for subscript check
        //Fix turn on subscript check flag back to true
        m_bcvarsubcheck = false; //Fix -Ignore subscript check for SUM, AVERAGE, MIN, MAX ( var_name { where expr } )

        return iProg;
}



int CEngineCompFunc::CompileHas(int iVarNode) // 20120429 (some of this code comes from the seek compilation function)
{
    ASSERT(Tkn == TOKHAS);

    VART* pVarT = ( Tokstindex > 0 && NPT_Ref(Tokstindex).IsA(SymbolType::Variable) ) ? VPT(Tokstindex) : nullptr;

    if( pVarT == nullptr || ( pVarT->GetMaxOccs() == 1 && pVarT->GetDictItem()->GetRecord()->GetMaxRecs() == 1 ) )
        IssueError(9111); // the item must repeat

    int iSymGroup = pVarT->GetParentGroup();

    int iBaseVarNode = iVarNode;

#ifdef GENCODE
    if( pVarT->IsAlpha() )
    {
        const auto& string_expression_node = GetNode<Nodes::StringExpression>(iVarNode);
        ASSERT(string_expression_node.function_code == FunctionCode::CHOBJ_CODE);
        iVarNode = std::abs(string_expression_node.string_expression);
        ASSERT(GetNode<FunctionCode>(iVarNode) == FunctionCode::MVAR_CODE);
    }
#endif

    bool bPrevSubscriptChecking = m_bcvarsubcheck;
    m_bcvarsubcheck = true; // turn subscript checking off

    NextToken();
    int inListCode = CompileInNodes(pVarT->GetDataType());

    m_bcvarsubcheck = bPrevSubscriptChecking;

#ifdef GENCODE
    if( !Flagcomp )
        return 0;

    // xxx has yyy will be given the code: seek(xxx in yyy) > 0

    // the repeating variable must be a copied set of code as the seek reference group (because the interpreter will change values)
    MVAR_NODE * pVarNode = (MVAR_NODE *)PPT(iVarNode);

    int iVarNode2 = Prognext;
    MVAR_NODE * pVarNode2 = NODEPTR_AS(MVAR_NODE);
    ADVANCE_NODE(MVAR_NODE);
    memmove(pVarNode2,pVarNode,sizeof(*pVarNode));

    int inCode = CreateInNode(pVarT->GetDataType(), iBaseVarNode, inListCode);

    int seekCode = Prognext;
    FN3_NODE * pSeekNode = NODEPTR_AS(FN3_NODE);
    OC_CreateCompilationSpace(1); // for the extra value needed for the seek node
    ADVANCE_NODE(FN3_NODE);

    int iNumGroups = 0;
    int iIndexForGroup = 0;

    for( int i = 0; i < pVarT->GetNumDim(); ++i )
    {
        if( pVarNode->m_iVarSubindexType[i] == MVAR_GROUP )
        {
            iNumGroups++;
            iIndexForGroup = i;
        }
    }

    if( !iNumGroups )
        SetSyntErr(ERROR_TOO_MANY_SUBINDEXES);

    pSeekNode->fn_code = FNSEEK_CODE;
    pSeekNode->sect_ind = pVarNode->m_iVarSubindexExpr[iIndexForGroup];
    pSeekNode->iSymVar = iVarNode2;
    pSeekNode->fn_exp = inCode;
    *((int *)((char *)pSeekNode + sizeof(*pSeekNode))) = 0;


    int zeroCode = CreateNumericConstantNode(0);

    return CreateOperatorNode(GT_CODE, seekCode, zeroCode);
#else
    return 0;
#endif
}



//-----------------------------------------------------------------------
//  cfun_fnh : compile function / class FNH
//              --> HIGHLIGHted( var_name(occ) )
//              --> VISUALVAlue( var_name(occ)  )       // RH Feb 28, 95
//-----------------------------------------------------------------------
int CEngineCompFunc::cfun_fnh()
{
    int iProg = Prognext;
    int iFunCode = CurrentToken.function_details->code;

    if( iFunCode == FNHIGHLIGHT_CODE && Appl.ApplicationType != ModuleType::Entry )
        THROW_PARSER_ERROR0( 524 );     // valid only in ENTRY

#ifdef GENCODE
    FNH_NODE* ptrfunc = NODEPTR_AS(FNH_NODE);

    if( Flagcomp )
    {
        ADVANCE_NODE(FNH_NODE);
        ptrfunc->fn_code = iFunCode;
    }
#endif

    NextToken();
    IssueErrorOnTokenMismatch(TOKLPAREN, ERROR_INVALID_FUNCTION_CALL);

    NextToken();

    if( !IsCurrentTokenVART(*this) )
        THROW_PARSER_ERROR0( 11 );

    VART* pVarT = VPT(Tokstindex);

    // visualvalue only accepts numeric values
    if( iFunCode == FNVISUALVALUE_CODE && !pVarT->IsNumeric() )
        THROW_PARSER_ERROR0( 119 );     // numeric variable expected

    int iVarExpr = varsanal(pVarT->GetFmt());
    CHECK_SYNTAX_ERROR_AND_THROW(0);

    IssueErrorOnTokenMismatch(TOKRPAREN, 17);

    NextToken();

#ifdef GENCODE
    if( Flagcomp )
    {
        ptrfunc->isymb = iVarExpr;
        ptrfunc->occ_exp = -2; // -2 indicates that this has been compiled with the variable information coming from varsanal (for versions 7.0+)
    }
#endif

    return iProg;
}


//-----------------------------------------------------------------------
//  cfun_fn4 : compile function / class FN4
//              --> NOCCURS, SOCCURS ( sect_name )
//-----------------------------------------------------------------------
int CEngineCompFunc::cfun_fn4()
{
    FunctionCode function_code = CurrentToken.function_details->code;
    int symbol_index;

    NextToken();
    IssueErrorOnTokenMismatch(TOKLPAREN, ERROR_INVALID_FUNCTION_CALL);

    NextTokenWithPreference(SymbolType::Section);

    // noccurs works as soccurs when a record is used
    if( function_code == FunctionCode::FNNOCCURS_CODE && ( Tkn == TOKRECORD || Tkn == TOKSECT ) )
        function_code = FunctionCode::FNSOCCURS_CODE;

    // noccurs
    if( function_code == FunctionCode::FNNOCCURS_CODE )
    {
        // allows for Groups and MultVars // victor Aug 02, 99
        // isymGroup passed to 'exnoccurs'
        if( Tkn == TOKGROUP )
        {
            symbol_index = Tokstindex;
        }

        else if( Tkn == TOKBLOCK )
        {
            symbol_index = GetSymbolEngineBlock(Tokstindex).GetGroupT()->GetSymbol();
        }

        else if( Tkn == TOKVAR && VPT(Tokstindex)->IsArray() )
        {
            symbol_index = VPT(Tokstindex)->GetParentGroup(); // RHF Oct 24, 2000
        }

        else
        {
            IssueError(90015);
        }
    }

    // socurrs
    else
    {
        ASSERT(function_code == FunctionCode::FNSOCCURS_CODE);

        if( Tkn == TOKRECORD )
        {
            symbol_index = Tokstindex;
        }

        else if( Tkn == TOKSECT )
        {
            function_code = FunctionCode::PRE80_FNSOCCURS_CODE;
            symbol_index = SPT(Tokstindex)->GetContainerIndex();
        }

        else
        {
            IssueError(15);
        }
    }

    NextToken();
    IssueErrorOnTokenMismatch(TOKRPAREN, 17);

    NextToken();

    auto& fn4_node = CreateCompilationNode<FN4_NODE>(function_code);

    fn4_node.sect_ind = symbol_index;

    int program_index = GetCompilationNodeProgramIndex(fn4_node);

    if( function_code == FunctionCode::FNSOCCURS_CODE )
        program_index = WrapNodeAroundValidDataAccessCheck(program_index, NPT_Ref(symbol_index), function_code);

    return program_index;
}


// RHF INIC Jul 09, 2002
//-------------------------------------------------------------------------------
//  cfun_fn6 : compile function / class FN6
//              --> XTAB( table_name {, weight_expr } )
//              --> UPDATE( hotdeck_name  )
// New Syntax: Jul 09
//      XTAB [(] TableName [, weight_expr ] [)]
//      [ WEIGHTED [by] ExprLog    ]
//      [ SELECT [(] ExprLog [)] ]
//      [ INCLUDE (SubTableList) ]
//      [ EXCLUDE (SubTableList) ]
//      ;
//-------------------------------------------------------------------------------
int CEngineCompFunc::cfun_fn6() {
        int     iProg    = Prognext;
        int     iFunCode = CurrentToken.function_details->code;
        bool    bUpdate  = ( iFunCode == FNUPDATE_CODE );
        bool    bXtab    = ( iFunCode == FNXTAB_CODE );

#ifdef GENCODE
        FN6_NODE* ptrfunc = NODEPTR_AS( FN6_NODE );
        if( Flagcomp ) {
                ADVANCE_NODE( FN6_NODE );

                ptrfunc->fn_code = iFunCode;
        }
#endif
        NextToken();                          // name of function

        bool    bHasParen=false;
        if( bXtab ) {
                if( Tkn == TOKLPAREN ) {
                        bHasParen = true;
                        NextToken();
                }
        }

        if( bUpdate ) {
                IssueErrorOnTokenMismatch(TOKLPAREN, ERROR_INVALID_FUNCTION_CALL);

                bHasParen = true;
                NextToken();
        }

        if( Tkn != TOKCROSSTAB )
                THROW_PARSER_ERROR0( 80 );

        CTAB* pCtab   = XPT(Tokstindex);

        // 'xtab' function cannot use hotdeck argument
        if( bXtab && pCtab->GetTableType() == CTableDef::Ctab_Hotdeck )
                THROW_PARSER_ERROR0( 88 );

        if( bUpdate && pCtab->GetTableType() != CTableDef::Ctab_Hotdeck )
                THROW_PARSER_ERROR0( 618 );

        int xtab_level = pCtab->GetTableLevel() / 10;  // RHF 22-03-93
        int decl_level = pCtab->GetTableLevel() % 10;  // RHF 22-03-93

        if( decl_level > LvlInComp )
                THROW_PARSER_ERROR0( 87 );

        if( xtab_level < LvlInComp )
                xtab_level = LvlInComp;

        pCtab->SetTableLevel( xtab_level * 10 +  decl_level );

        GENERATE_CODE( ptrfunc->tabl_ind = Tokstindex );

        int   iWeightExpr = 0;

#ifdef GENCODE
        if( Flagcomp ) { // Inheritance of Weight of declared CTAB
                iWeightExpr = pCtab->GetWeightExpr();
                ASSERT( iWeightExpr <= 0 );
        }
#endif

        int   iSelectExpr = 0;

#ifdef GENCODE
        if( Flagcomp ) { // Inheritance of Select of declared CTAB
                iSelectExpr = pCtab->GetSelectExpr();
                ASSERT( iSelectExpr <= 0 );
        }
#endif

        NextToken();
        if( Tkn == TOKCOMMA && !bUpdate ) {
                NextToken();
                iWeightExpr = exprlog();

                if( iWeightExpr < 0 ) // Empty // RHF Jul 04, 2002
                        iWeightExpr = 0; // RHF Jul 04, 2002
        }

        if( bHasParen ) {
                IssueErrorOnTokenMismatch(TOKRPAREN, 17);

                NextToken();
        }

        int  *pNodeBase[TBD_MAXDIM];
        int  iRepError = 605;
        bool bHasWeight = false;
        bool bHasSelect = false;
        bool bHasInclude = false;
        bool bHasExclude = false;
        std::vector<int> aSubTablesInc;
        std::vector<int> aSubTablesExc;
        std::vector<int> aSubTablesIncAux;
        std::vector<int> aSubTables;

        for( int i=0; i < TBD_MAXDIM; ++i ) {
                if( pCtab->GetNodeExpr(i) >= 0 ) {
                        pNodeBase[i] = CtNodebase;
                }
                else
                        pNodeBase[i] = NULL;
        }

#define CHECK_NO_REPEAT( boolVar ) \
        if( boolVar ) { SetSyntErr(iRepError); break; } boolVar = true

        while( GetSyntErr() == 0 && ( Tkn == TOKSELECT || Tkn == TOKWEIGHT || Tkn == TOKCOMMA || Tkn == TOKINCLUDE || Tkn == TOKEXCLUDE ) ) {
                if( Tkn == TOKCOMMA ) {
                        NextToken();
                        continue;
                }
                else if( Tkn == TOKSELECT ) {
                        CHECK_NO_REPEAT( bHasSelect );
                        NextToken();
                        iSelectExpr = exprlog();
                        if( iSelectExpr < 0 ) iSelectExpr = 0; // Empty
                }
                else if( Tkn == TOKWEIGHT ) {
                        CHECK_NO_REPEAT( bHasWeight );
                        NextToken();
                        if( Tkn == TOKBY )
                                NextToken();
                        iWeightExpr = exprlog();
                        if( iWeightExpr < 0 ) iWeightExpr = 0; // Empty
                }

                else if( Tkn == TOKINCLUDE ) {
                        CHECK_NO_REPEAT( bHasInclude );
                        NextToken();
                        if( Tkn != TOKLPAREN ) {
                                SetSyntErr(517);
                                break;
                        }

                        NextToken();

                        // Compile sub-table list and check the sub-tables have not been used before.
                        CompileSubTablesList( pCtab, pNodeBase, aSubTablesInc, false, NULL ); // RParen is eaten
                }
                else if( Tkn == TOKEXCLUDE ) {
                        CHECK_NO_REPEAT( bHasExclude );
                        NextToken();
                        if( Tkn != TOKLPAREN ) {
                                SetSyntErr(517);
                                break;
                        }

                        NextToken();

                        // Compile sub-table list and check the sub-tables have not been used before.
                        CompileSubTablesList( pCtab, pNodeBase, aSubTablesExc, false, NULL ); // RParen is eaten
                }
        }

        CHECK_SYNTAX_ERROR_AND_THROW(0);

#ifdef GENCODE
        if( Flagcomp ) {
                ptrfunc->iWeightExpr = iWeightExpr;
                ptrfunc->iSelectExpr = iSelectExpr;

                bool    bUseAllSubTables = (!bHasInclude && !bHasExclude);

                if( !bUseAllSubTables ) {
                        // Include
                        if( aSubTablesInc.empty() ) { // Include all SubTables
                                for( int iSubTable=0; iSubTable < pCtab->GetNumSubTables(); iSubTable++ ) {
                                        CSubTable&  cSubTable=pCtab->GetSubTable(iSubTable);
                                        aSubTablesIncAux.emplace_back( iSubTable );
                                }
                        }
                        else {
                                for( int i=0; i < (int)aSubTablesInc.size(); ++i ) {
                                        int  iSubTable = aSubTablesInc[i];
                                        CSubTable&  cSubTable=pCtab->GetSubTable(iSubTable);
                                        aSubTablesIncAux.emplace_back( iSubTable );
                                }
                        }

                        // Exclude
                        if( !aSubTablesExc.empty() ) {
                                for( int i=0; i < (int)aSubTablesIncAux.size(); ++i ) {
                                        int     iSubTableInc = aSubTablesIncAux[i];

                                        bool     bInclude = true;
                                        for(int j=0; bInclude && j < (int)aSubTablesExc.size(); ++j ) {
                                                int  iSubTableExc = aSubTablesExc[j];

                                                if( iSubTableInc == iSubTableExc )
                                                        bInclude = false;
                                        }

                                        if( bInclude )
                                                aSubTables.emplace_back( iSubTableInc );
                                }
                        }
                        else {
                                for( int i=0; i < (int)aSubTablesIncAux.size(); ++i ) {
                                        int     iSubTableInc = aSubTablesIncAux[i];

                                        aSubTables.emplace_back( iSubTableInc );
                                }
                        }
                }

                LIST_NODE*  pListNode;
                int         iSizeList= sizeof(LIST_NODE);

                int     iNumSubTables=(int)aSubTables.size();

                // No SubTables Included
                if( !bUseAllSubTables && iNumSubTables == 0 ) {
                        if( Issamod == ModuleType::Designer )
                                SetSyntErr(8700);
                        else
                                issaerror( MessageType::Abort, 8700 );
                        return 0;
                }

                if( iNumSubTables >= 2 )
                        iSizeList += (iNumSubTables - 1) * sizeof(int);

                int     iListNode=Prognext;
                pListNode = (LIST_NODE*) (PPT(iListNode));

                OC_CreateCompilationSpace(iSizeList / sizeof(int));

                pListNode->iNumElems = iNumSubTables;
                for( int i = 0; i < iNumSubTables; ++i )
                        pListNode->iSym[i] = aSubTables[i];

                ptrfunc->iListNode = iListNode;
        }
#endif

        return iProg;
}
// RHF END Jul 09, 2002


//-----------------------------------------------------------------------
//  cfun_fn8 : compile function / class FN8
//              --> KEY/CURRENTKEY, FILENAME, OPEN, CLOSE, GETCASELABEL ( dict )
//                  SETCASELABEL( dict, alpha_expression )
//                  ISPARTIAL([dict])
//                  ISVERIFIED(extdict)
//                  KEYLIST(extdict,[optional-string-list])
//-----------------------------------------------------------------------
int CEngineCompFunc::cfun_fn8()
{
    auto& fn8_node = CreateCompilationNode<FN8_NODE>(CurrentToken.function_details->code);
    fn8_node.symbol_index = -1;
    fn8_node.dictionary_access = 0;
    fn8_node.extra_parameter = ( fn8_node.function_code == FunctionCode::FNKEYLIST_CODE ) ? -1 : static_cast<int>(Nodes::SetFile::Mode::Update);
    fn8_node.starts_with_expression = -1;

    Symbol* symbol = CurrentToken.symbol;

    NextToken();
    IssueErrorOnTokenMismatch(TOKLPAREN, ERROR_INVALID_FUNCTION_CALL);

    NextToken();

    // if using dot notation, skip reading the symbol
    if( symbol != nullptr )
    {
        ASSERT(symbol->IsA(SymbolType::File));
        assert_cast<LogicFile*>(symbol)->SetUsed();
    }

    // ispartial doesn't require a dictionary argument
    else if( Tkn == TOKRPAREN && fn8_node.function_code == FunctionCode::FNISPARTIAL_CODE )
    {
        fn8_node.symbol_index = -1;
        DIP(0)->GetCaseAccess()->SetUsesStatuses();
    }

    // filename can be used to get the name of the paradata log
    else if( fn8_node.function_code == FunctionCode::FNFILENAME_CODE && Tkn == TOKFUNCTION && CurrentToken.function_details->code == FNPARADATA_CODE )
    {
        fn8_node.symbol_index = -2;
        NextToken();
    }

    // filename can also be used to get the filenames of audio, document, geometry, image, and pff objects
    else if( fn8_node.function_code == FunctionCode::FNFILENAME_CODE && ( Tkn == TOKAUDIO    || Tkn == TOKDOCUMENT ||
                                                                          Tkn == TOKGEOMETRY || Tkn == TOKIMAGE ||
                                                                          Tkn == TOKPFF      || Tkn == TOKREPORT ) )
    {
        symbol = NPT(Tokstindex);
        fn8_node.extra_parameter = CurrentToken.symbol_subscript_compilation;

        NextToken();
    }

    // filename/open/close can take a file argument
    else if( Tkn == TOKFILE && ( fn8_node.function_code == FunctionCode::FNFILENAME_CODE ||
                                 fn8_node.function_code == FunctionCode::FNOPEN_CODE ||
                                 fn8_node.function_code == FunctionCode::FNCLOSE_CODE ) )
    {
        symbol = NPT(Tokstindex);
        assert_cast<LogicFile*>(symbol)->SetUsed();

        NextToken();
    }

    // the rest of the functions take a dictionary argument
    else if( Tkn == TOKDICT )
    {
        symbol = NPT(Tokstindex);
        EngineDictionary* engine_dictionary = assert_cast<EngineDictionary*>(symbol);

        // verify that the right type of dictionary object was used
        if( fn8_node.function_code == FunctionCode::FNCLOSE_CODE ||
            fn8_node.function_code == FunctionCode::FNFILENAME_CODE ||
            fn8_node.function_code == FunctionCode::FNKEYLIST_CODE ||
            fn8_node.function_code == FunctionCode::FNOPEN_CODE )
        {
            VerifyEngineDataRepository(engine_dictionary, VerifyDictionaryFlag::NotWorkingStorage);
        }

        else if( fn8_node.function_code == FunctionCode::FNGETCASELABEL_CODE ||
                 fn8_node.function_code == FunctionCode::FNISPARTIAL_CODE  ||
                 fn8_node.function_code == FunctionCode::FNISVERIFIED_CODE ||
                 fn8_node.function_code == FunctionCode::FNKEY_CODE ||
                 fn8_node.function_code == FunctionCode::FNCURRENTKEY_CODE ||
                 fn8_node.function_code == FunctionCode::FNSETCASELABEL_CODE )
        {
            VerifyEngineCase(engine_dictionary);
        }

        // some additional checks...
        if( fn8_node.function_code == FunctionCode::FNISVERIFIED_CODE && Appl.ApplicationType == ModuleType::Entry )
        {
            if( engine_dictionary->IsDictionaryObject() )
                VerifyEngineDataRepository(engine_dictionary, VerifyDictionaryFlag::External);
        }

        else if( engine_dictionary->HasEngineDataRepository() )
        {
            VerifyEngineDataRepository(engine_dictionary, VerifyDictionaryFlag::NotWorkingStorage);
        }


        if( fn8_node.function_code == FunctionCode::FNGETCASELABEL_CODE || fn8_node.function_code == FunctionCode::FNSETCASELABEL_CODE )
        {
            engine_dictionary->GetCaseAccess()->SetUsesCaseLabels();
        }

        else if( fn8_node.function_code == FunctionCode::FNISVERIFIED_CODE || fn8_node.function_code == FunctionCode::FNISPARTIAL_CODE )
        {
            engine_dictionary->GetCaseAccess()->SetUsesStatuses();
        }

        else if( fn8_node.function_code == FunctionCode::FNCLOSE_CODE || fn8_node.function_code == FunctionCode::FNOPEN_CODE )
        {
            engine_dictionary->GetEngineDataRepository().SetHasDynamicFileManagement();
        }

        else if( fn8_node.function_code == FunctionCode::FNKEYLIST_CODE )
        {
            VerifyEngineDataRepository(engine_dictionary, VerifyDictionaryFlag::NeedsIndex);
        }

        NextToken();
    }

    // the rest of the functions take a dictionary argument
    else if( Tkn == TOKDICT_PRE80 )
    {
        symbol = NPT(Tokstindex);
        DICT* pDicT = DPT(Tokstindex);

        if( fn8_node.function_code == FNISVERIFIED_CODE && Appl.ApplicationType == ModuleType::Entry )
        {
            VerifyDictionary(pDicT, VerifyDictionaryFlag::External_OneLevel_NeedsIndex);
        }

        else
        {
            VerifyDictionary(pDicT, VerifyDictionaryFlag::NotWorkingStorage);
        }

        if( fn8_node.function_code == FNGETCASELABEL_CODE || fn8_node.function_code == FNSETCASELABEL_CODE )
        {
            pDicT->GetCaseAccess()->SetUsesCaseLabels();
        }

        else if( fn8_node.function_code == FNISVERIFIED_CODE || fn8_node.function_code == FNISPARTIAL_CODE )
        {
            pDicT->GetCaseAccess()->SetUsesStatuses();
        }

        else if( fn8_node.function_code == FNCLOSE_CODE || fn8_node.function_code == FNOPEN_CODE )
        {
            pDicT->SetHasDynamicFileManagement();
        }

        else if( fn8_node.function_code == FNKEYLIST_CODE )
        {
            VerifyDictionary(pDicT, VerifyDictionaryFlag::NeedsIndex);
        }

        NextToken();
    }

    // invalid argument
    else
    {
        if( fn8_node.function_code == FunctionCode::FNOPEN_CODE ||
            fn8_node.function_code == FunctionCode::FNCLOSE_CODE ||
            fn8_node.function_code == FunctionCode::FNFILENAME_CODE )
        {
            IssueError(930);
        }

        else
        {
            IssueError(544);
        }
    }

    if( symbol != nullptr )
        fn8_node.symbol_index = symbol->GetSymbolIndex();


    // additional parameters
    if( Tkn == TOKLPAREN )
    {
        if( fn8_node.function_code == FunctionCode::FNKEYLIST_CODE )
        {
            if( symbol->IsA(SymbolType::Dictionary) )
            {
                fn8_node.dictionary_access = CompileDictionaryAccess(assert_cast<EngineDictionary&>(*symbol), &fn8_node.starts_with_expression);
            }

            else
            {
                fn8_node.dictionary_access = CompileDictionaryAccess(assert_cast<DICT*>(symbol), &fn8_node.starts_with_expression);
            }

            IssueErrorOnTokenMismatch(TOKRPAREN, ERROR_RIGHT_PAREN_EXPECTED);

            NextToken();
        }

        else
        {
            IssueError(541); // cannot specify index
        }
    }


    if( fn8_node.function_code == FunctionCode::FNSETCASELABEL_CODE )
    {
        IssueErrorOnTokenMismatch(TOKCOMMA, 528);

        NextToken();
        fn8_node.extra_parameter = CompileStringExpression();
    }

    else if( Tkn == TOKCOMMA && fn8_node.function_code == FunctionCode::FNKEYLIST_CODE )
    {
        NextToken();

        if( Tkn != TOKLIST )
            IssueError(960);

        fn8_node.extra_parameter = Tokstindex;

        const LogicList& logic_list = GetSymbolLogicList(fn8_node.extra_parameter);

        if( !logic_list.IsString() )
            IssueError(961, ToString(DataType::String));

        if( logic_list.IsReadOnly() )
            IssueError(965, logic_list.GetName().c_str());

        NextToken();
    }

    else if( Tkn == TOKCOMMA && fn8_node.function_code == FunctionCode::FNOPEN_CODE )
    {
        ASSERT(symbol != nullptr);

        switch( NextKeywordOrError({ _T("update"), _T("create"), _T("append") }) )
        {
            case 1:
            {
                fn8_node.extra_parameter = static_cast<int>(Nodes::SetFile::Mode::Update);
                break;
            }

            case 2:
            {
                fn8_node.extra_parameter = static_cast<int>(Nodes::SetFile::Mode::Create);

                if( symbol->IsA(SymbolType::Dictionary) )
                {
                    VerifyEngineDataRepository(assert_cast<EngineDictionary*>(symbol), VerifyDictionaryFlag::Writeable);

                    // the input dictionary can only be used in entry
                    if( symbol->GetSubType() == SymbolSubType::Input && Appl.ApplicationType != ModuleType::Entry )
                        IssueError(931);
                }

                else if( symbol->IsA(SymbolType::Pre80Dictionary) )
                {
                    DICT* pDicT = assert_cast<DICT*>(symbol);

                    VerifyDictionary(pDicT,VerifyDictionaryFlag::Writeable);

                    // the input dictionary can only be used in entry
                    if( pDicT->GetSubType() == SymbolSubType::Input && Appl.ApplicationType != ModuleType::Entry )
                        IssueError(931);
                }

                break;
            }

            case 3:
            {
                fn8_node.extra_parameter = static_cast<int>(Nodes::SetFile::Mode::Append);
                break;
            }
        }

        NextToken();
    }

    IssueErrorOnTokenMismatch(TOKRPAREN, 17); // mandatory ')'

    NextToken();

    return GetCompilationNodeProgramIndex(fn8_node);
}


int CEngineCompFunc::cfun_fnmaxocc()
{
    // compilation for maxocc
    const Symbol* symbol = nullptr;

    NextToken();
    IssueErrorOnTokenMismatch(TOKLPAREN, 14);

    NextTokenWithPreference(SymbolType::Section);

    // if no argument is provided, then either get the symbol from the for table, the
    // current proc, or wait to evaluate this at runtime
    if( Tkn == TOKRPAREN )
    {
        // In Foreach?
        if( m_ForTableNext - 1 >= 0 )
            symbol = NPT(m_ForTable[m_ForTableNext - 1].forGrpIdx);

        else if( ObjInComp != SymbolType::Application )
        {
            // ENGINECR_TODO(maxocc) look at the notes in exmaxocc ... if we change this
            // we can get rid of this check
            if( ObjInComp != SymbolType::Variable )
                symbol = NPT(InCompIdx);
        }
    }

    // otherwise verify the argument
    else
    {
        if( Tkn == TOKRECORD || Tkn == TOKSECT || Tkn == TOKGROUP || Tkn == TOKBLOCK ||
            ( Tkn == TOKVAR && VPT(Tokstindex)->GetDictItem() != nullptr ) )
        {
            symbol = NPT(Tokstindex);
        }

        else
            IssueError(920);

        NextToken();
    }

    IssueErrorOnTokenMismatch(TOKRPAREN, 17);

    NextToken();

    // compile-time evaluation
    if( symbol != nullptr )
    {
        unsigned max_occurrences = SymbolCalculator::GetMaximumOccurrences(*symbol);
        ASSERT(max_occurrences > 0);
        return CreateNumericConstantNode(max_occurrences);
    }

    // runtime evaluation
    else
    {
        return CreateVariableArgumentsNode(FunctionCode::FNMAXOCC_CODE, { });
    }
}


int CEngineCompFunc::cfun_fninvalueset() {
        int     iProg    = Prognext;
        int     iFunCode = CurrentToken.function_details->code;
        int     iExpr=0, iSymVSet=0, iSymVar=0;

#ifdef GENCODE
        FNINVALUSET_NODE*   ptrfunc = NODEPTR_AS( FNINVALUSET_NODE );

        if( Flagcomp ) {
            ADVANCE_NODE( FNINVALUSET_NODE );

            ptrfunc->fn_code = iFunCode;
        }
#endif
        NextToken();                          // name of function
        if( Tkn != TOKLPAREN )
            THROW_PARSER_ERROR0( 14 );
        NextToken();

        // the last check prevents alpha working variables
        if( IsCurrentTokenVART(*this) && VPT(Tokstindex)->GetDictItem() != NULL )
        {
            iSymVar=Tokstindex;

            VART*   pVarT=VPT(iSymVar);

            if( pVarT->IsNumeric() )
                    iExpr = varsanal( pVarT->GetFmt() );
            else
                    iExpr = CompileStringExpression();

            if( GetSyntErr() != 0 )
                return 0;

            if( Tkn == TOKCOMMA ) {
                    NextToken();

                    if( Tkn != TOKVALUESET )
                        IssueError(33115);

                    iSymVSet = Tokstindex;

                    const ValueSet& value_set = GetSymbolValueSet(iSymVSet);

                    if( value_set.IsDynamic() || value_set.GetVarT() != pVarT )
                        IssueError(940);

                    NextToken();
            }
        }

        // a new mode (20150131) so that you can check if a number/string is in a value set without
        // having to set the item to the variable that you want to check
        else
        {
            iSymVar = -1;

            DataType value_data_type = GetCurrentTokenDataType();

            iExpr = CompileExpression(value_data_type);

            if( Tkn != TOKCOMMA )
                 THROW_PARSER_ERROR0( 528 );

            NextToken();

            if( Tkn != TOKVALUESET )
                THROW_PARSER_ERROR0( 33115 );

            iSymVSet = Tokstindex;

            const ValueSet& value_set = GetSymbolValueSet(iSymVSet);

            if( value_set.GetDataType() != value_data_type )
                IssueError(941, ToString(value_data_type));

            NextToken();
        }

        IssueErrorOnTokenMismatch(TOKRPAREN, 17);

        NextToken();

#ifdef GENCODE
        if( Flagcomp ) {
                ptrfunc->m_iSymVar = iSymVar;
                ptrfunc->m_iExpr = iExpr;
                ptrfunc->m_iSymVSet = iSymVSet;
        }
#endif

        return iProg;
}

//-----------------------------------------------------------------------
//  cfun_fnc : compile function / class FNC
//              --> CLEAR / MINVALUE / MAXVALUE
//-----------------------------------------------------------------------
int CEngineCompFunc::cfun_fnc()
{
    auto& fnc_node = CreateCompilationNode<FNC_NODE>(CurrentToken.function_details->code);

    NextToken(); // name of function
    IssueErrorOnTokenMismatch(TOKLPAREN, ERROR_INVALID_FUNCTION_CALL);

    NextToken();

    fnc_node.isymb = Tokstindex;

    // CLEAR
    if( fnc_node.fn_code == FunctionCode::FNCLRCASE_CODE )
    {
        if( Tkn == TOKDICT )
        {
            const EngineDictionary& engine_dictionary = GetSymbolEngineDictionary(fnc_node.isymb);
            VerifyEngineCase(&engine_dictionary);

            if( engine_dictionary.GetSubType() != SymbolSubType::External &&
                engine_dictionary.GetSubType() != SymbolSubType::Work )
            {
                IssueError(523);
            }
        }

        else if( Tkn == TOKDICT_PRE80 )
        {
            const DICT* pDicT = DPT(Tokstindex);

            if( pDicT->GetSubType() != SymbolSubType::External &&
                pDicT->GetSubType() != SymbolSubType::Work )
            {
                IssueError(523);
            }
        }

        else
        {
            IssueError(523);
        }
    }

    // MINVALUE, MAXVALUE
    else if( fnc_node.fn_code == FunctionCode::FNMINVALUE_CODE || fnc_node.fn_code == FunctionCode::FNMAXVALUE_CODE )
    {
        bool bNumericValueOrValueSet = false;

        if( IsCurrentTokenVART(*this) )
        {
            bNumericValueOrValueSet = VPT(Tokstindex)->IsNumeric();
        }

        else if( Tkn == TOKVALUESET )
        {
            bNumericValueOrValueSet = GetSymbolValueSet(Tokstindex).IsNumeric();
        }

        // numeric variable expected
        if( !bNumericValueOrValueSet )
            IssueError(11);
    }

    else
    {
        ASSERT(false);
    }

    NextToken();
    IssueErrorOnTokenMismatch(TOKRPAREN, 17);

    NextToken();

    return GetCompilationNodeProgramIndex(fnc_node);
}


//-----------------------------------------------------------------------
//  cfun_fns : compile function / class FNS
//              --> SELCASE
//-----------------------------------------------------------------------
int CEngineCompFunc::cfun_fns()
{
    int iDictSymbol = -1;
    int iKeyExpression = -1;
    int iKeyOffset = -1;
    int iHeading = -1;
    int dictionary_access = 0;

    // mandatory '('
    NextToken();
    IssueErrorOnTokenMismatch(TOKLPAREN, ERROR_INVALID_FUNCTION_CALL);

    NextToken();

    // a heading
    if( IsCurrentTokenString() )
    {
        iHeading = CompileStringExpression();
        NextToken();
    }

    // dictionary name
    if( Tkn != TOKDICT_PRE80 )
        THROW_PARSER_ERROR0( 525 );

    iDictSymbol = Tokstindex;
    DICT* pDicT = DPT(iDictSymbol);

    VerifyDictionary(pDicT, VerifyDictionaryFlag::External_OneLevel_NeedsIndex);

    NextToken();

    if( Tkn == TOKLPAREN )
    {
        dictionary_access = CompileDictionaryAccess(pDicT, &iKeyExpression);
        IssueErrorOnTokenMismatch(TOKRPAREN, ERROR_RIGHT_PAREN_EXPECTED);

        NextToken();
    }

    // if the key expression was specified while compiling dictionary access, skip reading it
    if( iKeyExpression == -1 )
    {
        // mandatory comma
        if( Tkn != TOKCOMMA )
            THROW_PARSER_ERROR0( 528 );

        // mandatory key expression
        NextToken();
        iKeyExpression = CompileStringExpression();
    }

    // optional offset
    if( Tkn == TOKCOMMA )
    {
        NextToken();
        iKeyOffset = exprlog();
    }

    // mandatory ')'
    IssueErrorOnTokenMismatch(TOKRPAREN, 17);

    // generate the initial node
    auto& selcase_node = CreateCompilationNode<FNS_NODE>(FNSELCASE_CODE);
    selcase_node.idict = iDictSymbol;
    selcase_node.key_expr = iKeyExpression;
    selcase_node.wind_offs = iKeyOffset;
    selcase_node.dictionary_access = dictionary_access;
    selcase_node.heading = iHeading;

    for( int n_vars = 0; n_vars < FNSEL_VARS; ++n_vars )
        selcase_node.include_vars[n_vars] = -1;

    // process LOCKED, INCLUDE, WHERE, MULTIPLE in any order
    int iNumIncludeVars = 0;
    int iWhereExpression = -1;
    SelcaseMarkType eMarkType = SelcaseMarkType::None;

    NextToken();

    while( Tkn == TOKINCLUDE  || Tkn == TOKWHERE || Tkn == TOKMULTIPLE )
    {
        if( Tkn == TOKINCLUDE )
        {
            if( iNumIncludeVars > 0 )
            {
                issaerror(MessageType::Error,537,_T("INCLUDE")); // repeated include clause
                return 0;
            }

            // mandatory '('
            NextToken();
            IssueErrorOnTokenMismatch(TOKLPAREN, ERROR_INVALID_FUNCTION_CALL);

            // get the include variable list
            NextToken();

            while( Tkn != TOKRPAREN ) // ... until a ')' closes include
            {
                if( Tkn != TOKVAR || VPT(Tokstindex)->GetClass() != CL_SING || m_pEngineArea->ownerdic( Tokstindex ) != iDictSymbol )
                    THROW_PARSER_ERROR0( 538 ); // only single vars from same dictionary

                VPT(Tokstindex)->SetUsed(true);

                if( iNumIncludeVars < FNSEL_VARS )
                    selcase_node.include_vars[iNumIncludeVars] = Tokstindex;

                iNumIncludeVars++;

                NextToken();

                if( Tkn == TOKCOMMA )
                    NextToken();
            }

            if( iNumIncludeVars >= FNSEL_VARS ) // RHF Dec 17, 2002 Change from > to >=
                THROW_PARSER_ERROR0( 539 ); // only up to FNSEL_VARS

            if( iNumIncludeVars == 0 )
                THROW_PARSER_ERROR0( 540 ); // empty include list

            NextToken();
        }

        else if( Tkn == TOKWHERE )
        {
            if( iWhereExpression >= 0 )
            {
                issaerror(MessageType::Error,537,_T("WHERE")); // repeated where clause
                return 0;
            }

            NextToken();

            Flagvars = 0; // require Mult var to have an index
            iWhereExpression = exprlog();
        }

        else if( Tkn == TOKMULTIPLE )
        {
            eMarkType = SelcaseMarkType::Multiple;

            NextToken();

            // look for automark; multiple() without a parameter list is also okay
            if( Tkn == TOKLPAREN )
            {
                if( NextKeyword({ _T("AUTOMARK") }) == 1 )
                    eMarkType = SelcaseMarkType::MultipleAutomark;

                NextToken();
                IssueErrorOnTokenMismatch(TOKRPAREN, 17);

                NextToken();
            }
        }

        if( Tkn == TOKCOMMA )
            NextToken();
    }


    // finish generating the node
    selcase_node.fn_where_exp = iWhereExpression;
    selcase_node.mark_type = (int)eMarkType;

    // if the selections are automarked, then there is no reason to have an include list because it will not be shown
    if( eMarkType == SelcaseMarkType::MultipleAutomark )
        selcase_node.include_vars[0] = -1;

    return GetCompilationNodeProgramIndex(selcase_node);
}

//-----------------------------------------------------------------------
//  cfun_fntc: compile function / class FNTC
//              --> TBLROW, TBLCOL, TBLLAY
//-----------------------------------------------------------------------

int CEngineCompFunc::cfun_fntc() {
        // Tokid contiene el numero de la funcion
        // FNTBLROW_CODE o FNTBLCOL_CODE o FNTBLLAY_CODE
        int     iProg    = Prognext;
        int     iFunCode = CurrentToken.function_details->code;
        int     iDimType  = -1;

        switch( iFunCode ) {
        case FNTBLROW_CODE:
                iDimType = DIM_ROW;
                break;
        case FNTBLCOL_CODE:
                iDimType = DIM_COL;
                break;
        case FNTBLLAY_CODE:
                iDimType = DIM_LAYER;
                break;
        }

        NextToken();
        IssueErrorOnTokenMismatch(TOKLPAREN, ERROR_INVALID_FUNCTION_CALL);

        NextToken();

        // for backwards compatability with pre-CSPro 7.2 applications, these functions
        // can return the length of arrays
        if( Tkn == TOKARRAY )
        {
#ifdef GENCODE
            FNC_NODE* pNode = NODEPTR_AS(FNC_NODE);
            ADVANCE_NODE(FNC_NODE);
            pNode->fn_code = iFunCode;
            pNode->isymb = Tokstindex;
#endif
            NextToken();

            if( Tkn != TOKRPAREN )
                THROW_PARSER_ERROR0( 657 );

            NextToken();

            return iProg;
        }

        // though the real intention of these functions is for tables...

        if( Tkn != TOKCROSSTAB )
                THROW_PARSER_ERROR0( 651 );     // crosstab name expected

        CTAB*   ct    = XPT(Tokstindex);


        // GSF 3-May-2006 begin: reactive this code, but with different environment var
        bool bNewTbd = true;
        /*
       if( (p=getenv( "OLD_TABS" )) != NULL && stricmp( p, "Y" ) == 0 )
            bNewTbd=false;
        // GSF 3-May-2006 end
        */

        OLD_FNTC_NODE*  ptroldfunc=NULL;
        FNTC_NODE*      ptrnewfunc=NULL;

#ifdef GENCODE
        if( bNewTbd ) {
                ptrnewfunc = NODEPTR_AS( FNTC_NODE );

                if( Flagcomp ) {
                        ADVANCE_NODE( FNTC_NODE );

                        ptrnewfunc->fn_code = iFunCode;
                        ptrnewfunc->iCtab = 0;
                        ptrnewfunc->iNewMode = 1;
                        ptrnewfunc->iSubTableNum = 0;
                        for( int i=0; i < TBD_MAXDEPTH; ++i ) {
                                ptrnewfunc->iCatValues[i] = 0;
                                ptrnewfunc->iCatValuesSeq[i] = 0;
                        }
                }
        }
        else {
                ptroldfunc = NODEPTR_AS( OLD_FNTC_NODE );

                if( Flagcomp ) {
                        ADVANCE_NODE( OLD_FNTC_NODE );

                        ptroldfunc->fn_code = iFunCode;
                        ptroldfunc->var1 = ptroldfunc->var2 = ptroldfunc->expr1 = ptroldfunc->expr2 = -1;
                        ptroldfunc->iNewMode = 0;
                }
        }
#endif

#ifdef GENCODE
        if( bNewTbd && Flagcomp )
                ptrnewfunc->iCtab = Tokstindex;
#endif

        if( ct->GetNumDim() < iDimType )
                THROW_PARSER_ERROR0( 652 );     // invalid dimension

        if( ( bNewTbd && ct->GetTableType() ) ) {
                if( !do_cfun_fntc( ct, iDimType, ptrnewfunc ) )
                        return 0;
        }
        else {
                if( !old_do_cfun_fntc( ct, iDimType, ptroldfunc ) )
                        return 0;
        }

        if( Tkn != TOKRPAREN )
                THROW_PARSER_ERROR0( 657 );
        NextToken();

        return iProg;
}


int CEngineCompFunc::do_cfun_fntc( CTAB* pCtab, int iTheDimType, FNTC_NODE* pFntcNode ) {
        CTAB::pCurrentCtab = pCtab;
        NextToken(); // Eat table name

        ASSERT( iTheDimType == DIM_ROW || iTheDimType == DIM_COL || iTheDimType == DIM_LAYER );

        int     iSubTableNum=0;
        int     iCatValues[TBD_MAXDEPTH];
        int     iCatValuesSeq[TBD_MAXDEPTH];

        for( int i=0; i < TBD_MAXDEPTH; ++i ) {
                iCatValues[i] = 0;
                iCatValuesSeq[i] = 0;
        }

        if( Tkn == TOKRPAREN ) { // (T1)
                GENERATE_CODE( iSubTableNum = pCtab->GetNumSubTables()-1 );
        }
        else if( Tkn != TOKCOMMA )
                THROW_PARSER_ERROR0( 528 );     // Comma Expected
        else {
                NextToken();

                // (T1,SubTableDim)
                int     iDimType[TBD_MAXDIM];
                int     iVar[TBD_MAXDIM][TBD_MAXDEPTH];
                int     iSeq[TBD_MAXDIM][TBD_MAXDEPTH];

                int*    pNodeBase[TBD_MAXDIM];
                for( int i=0; i < TBD_MAXDIM; ++i ) {
                        iDimType[i] = DIM_NODIM;
                        for( int j=0; j < TBD_MAXDEPTH; ++j ) {
                                iVar[i][j] = 0;
                                iSeq[i][j] = 0;
                        }

                        if( i < pCtab->GetNumDim() ) {
                                pNodeBase[i] = CtNodebase;
                                //iRoot[i] = pCtab->GetNodeExpr(i);
                        }
                        else {
                                pNodeBase[i] = NULL;
                                //iRoot[i] = -1;
                        }
                }

                // Always use the first slot!
                CompileOneSubTableDim( pNodeBase, DIM_ROW, iDimType, iVar, iSeq, iCatValues, iCatValuesSeq );
                if( GetSyntErr() != 0 )
                        THROW_PARSER_ERROR0( GetSyntErr() );

                std::vector<int> aSubTables;
                bool bCheckUsed=false;

                iDimType[0] = iTheDimType;
                GetSubTableList( pNodeBase, aSubTables, bCheckUsed, iDimType, iVar, iSeq, iTheDimType );

                if( GetSyntErr() != 0 )
                        THROW_PARSER_ERROR0( GetSyntErr() );

                int iNumSubTables = (int)aSubTables.size();
                if( iNumSubTables == 0 ) {
                    THROW_PARSER_ERROR0( 8411 );
                }
                /* RHF COM INIC Jul 15, 2002
                else if( iNumSubTables != 1 ) {
                THROW_PARSER_ERROR0( 8406 );
                }
                RHF COM END Jul 15, 2002 */

                iSubTableNum = aSubTables[iNumSubTables-1]; // Use the last
        }

#ifdef GENCODE
        if( Flagcomp ) {
                pFntcNode->iSubTableNum = iSubTableNum+1;
                CSubTable&  cSubTable=pCtab->GetSubTable(iSubTableNum);
                int         iCatExprValue=0;

                for( int i=0; i < TBD_MAXDEPTH; ++i ) {
                        // CatExpr
                        if( iCatValues[i] == 0 ) { // No expresion
                                int     iCtNode=cSubTable.m_iCtTree[iTheDimType-1][i];
                                if( iCtNode >= 0 ) {
                                        iCatExprValue = CreateNumericConstantNode( m_pEngineArea->cthighnumvalue( iCtNode ) );
                                        if( GetSyntErr() != 0 )
                                                THROW_PARSER_ERROR0( GetSyntErr() );
                                }
                                else
                                        iCatExprValue = 0;
                        }
                        else
                                iCatExprValue = iCatValues[i];

                        pFntcNode->iCatValues[i] = iCatExprValue;

                        // CatSeq
                        if( iCatValuesSeq[i] <= 0 )
                                iCatValuesSeq[i] = 1;

                        pFntcNode->iCatValuesSeq[i] = iCatValuesSeq[i];
                }
        }
#endif

        // Ready to Eat RightParen

        return 1;
}

// RHF END 23/9/96

//////////////////////////////////////////////////////////////////////////////////////////////
// cfun_fng : compile function / class FNG
// OBJ_NAME0 can be: DI/SE/VA/ValueSet
// OBJ_NAME1 can be: OBJ_NAME0 and GR
//     --> GETSYMBOL( OBJ_NAME1 )
//     --> GETLABEL( ITEM_NAME [ BY code/label], alpha-numcode )
//     --> GETLABEL( OBJ_NAME0 )
//     --> GETIMAGE( ITEM_NAME | VALUE_SET_NAME )

//////////////////////////////////////////////////////////////////////////////////////////////
int CEngineCompFunc::cfun_fng()
{
    FunctionCode function_code = CurrentToken.function_details->code;
    const Symbol* symbol = nullptr;
    bool getsymbol_with_savepartial = false;
    GetLabelSearchType getlabel_search_type = GetLabelSearchType::ByCode;
    std::optional<int> value_expression;

    NextToken();
    IssueErrorOnTokenMismatch(TOKLPAREN, ERROR_INVALID_FUNCTION_CALL);

    NextTokenWithPreference(SymbolType::Section);

    // getsymbol has special processing to retrieve the partially saved symbol
    if( function_code == FunctionCode::FNGETSYMBOL_CODE && Tkn == TOKFUNCTION && CurrentToken.function_details->code == FNSAVEPARTIAL_CODE )
    {
        getsymbol_with_savepartial = true;
        NextToken();
    }

    // if no argument is provided (for getsymbol/getlabel), use the current proc symbol, or wait to evaluate at runtime
    else if( Tkn == TOKRPAREN )
    {
        if( function_code != FunctionCode::FNGETIMAGE_CODE && ObjInComp != SymbolType::Application )
            symbol = NPT(InCompIdx);
    }

    // otherwise evaluate the symbol
    else
    {
        if( Tkn == TOKDICT || Tkn == TOKDICT_PRE80 || Tkn == TOKRECORD || Tkn == TOKSECT ||
            IsCurrentTokenVART(*this) || Tkn == TOKVALUESET ||
            Tkn == TOKGROUP || Tkn == TOKBLOCK )
        {
            symbol = NPT(Tokstindex);
        }

        else
        {
            IssueError(47001);
        }

        NextToken();
    }

    // getimage requires a variable or value set
    if( function_code == FunctionCode::FNGETIMAGE_CODE )
    {
        if( symbol == nullptr || !symbol->IsOneOf(SymbolType::Variable, SymbolType::ValueSet) )
            IssueError(47004);
    }

    bool can_read_second_value = ( function_code != FunctionCode::FNGETSYMBOL_CODE );
    bool must_read_second_value = ( function_code == FunctionCode::FNGETIMAGE_CODE );

    if( function_code == FunctionCode::FNGETLABEL_CODE && Tkn == TOKBY )
    {
        size_t by_type = NextKeywordOrError({ _T("code"), _T("label") });
        getlabel_search_type = ( by_type == 1 ) ? GetLabelSearchType::ByCode : GetLabelSearchType::ByLabel;

        NextToken();

        must_read_second_value = true;
    }

    // with variables and value sets, a specific value can be specified
    if( must_read_second_value || ( can_read_second_value && Tkn == TOKCOMMA ) )
    {
        IssueErrorOnTokenMismatch(TOKCOMMA, 528);

        if( !symbol->IsOneOf(SymbolType::Variable, SymbolType::ValueSet) )
            IssueError(47004);

        NextToken();

        value_expression = ( IsNumeric(*symbol) && getlabel_search_type != GetLabelSearchType::ByLabel ) ? exprlog() :
                                                                                                           CompileStringExpression();
    }

    IssueErrorOnTokenMismatch(TOKRPAREN, 17);

    NextToken();


    // compile-time evaluation
    if( symbol != nullptr && !value_expression.has_value() )
    {
        if( function_code == FunctionCode::FNGETSYMBOL_CODE )
        {
            return CreateStringLiteralNode(symbol->GetName());
        }

        else
        {
            ASSERT(function_code == FunctionCode::FNGETLABEL_CODE);

            // only evaluate the label at compile-time when there are not labels in multiple languages
            if( !SymbolCalculator::DoMultipleLabelsExist(*symbol) )
                return CreateStringLiteralNode(SymbolCalculator::GetLabel(*symbol));
        }
    }

    // runtime evaluation
    auto& fng_node = CreateCompilationNode<FNG_NODE>(function_code);

    fng_node.symbol_index = ( symbol != nullptr )      ? symbol->GetSymbolIndex() :
                            getsymbol_with_savepartial ? 2147483647 /*INT_MAX*/ :
                                                            -1;
    fng_node.m_iOper = (int)getlabel_search_type;
    fng_node.m_iExpr = value_expression.value_or(-1);

    return GetCompilationNodeProgramIndex(fng_node);
}




// RHC INIC Oct 29, 2000
// Compile CUROCC & TOTOCC functions
int CEngineCompFunc::cfun_fngr() {
        int     iProg    = Prognext;
        int     iFunCode = CurrentToken.function_details->code;


#ifdef GENCODE
        FNGR_NODE*  ptrfunc = NODEPTR_AS( FNGR_NODE );

        if( Flagcomp ) {
                ADVANCE_NODE( FNGR_NODE );

                ptrfunc->fn_code = iFunCode;
                ptrfunc->m_iSym  = 0;                           // RHF Oct 31, 2000
                ptrfunc->m_iArgumentType = FNGR_Spec::ARG_NOT_SPECIFIED;   // rcl Sept 7, 2004
        }
#endif

        // try to read "("
        NextToken();
        IssueErrorOnTokenMismatch(TOKLPAREN, ERROR_INVALID_FUNCTION_CALL);

        NextToken();

        // did user write  "()" ?
        if( Tkn == TOKRPAREN ) {            // yes -> call without arguments
#ifdef GENCODE
                if( Flagcomp ) {
                        ptrfunc->m_iArgumentType = FNGR_Spec::ARG_NO_ARGUMENTS;
                        ptrfunc->fn_arg = -1;

                        // 20091027
                        // in the past a conditional like count(data where data = data(curocc()))
                        // would always be true because curocc() would refer to an inner, not outer,
                        // loop (say one is running this in PROC data (the outer loop);
                        // with this fix it operates like count(data where data = data(curocc(data)))

                        //if( m_pCuroccGrpIdx ) [modified on 20100208 ... foreach overrides my code]
                        if( m_pCuroccGrpIdx && !( m_ForTableNext - 1 >= 0 ) )
                        {
                            ptrfunc->m_iArgumentType = m_pCuroccGrpIdx->m_iArgumentType;
                            ptrfunc->fn_arg = m_pCuroccGrpIdx->fn_arg;
                        }

                        // In Foreach?
                        if( m_ForTableNext - 1 >= 0 ) {
                                if( iFunCode == FNCUROCC_CODE )
                                        ptrfunc->m_iSym = m_ForTable[m_ForTableNext - 1].forVarIdx;
                                else                    // In totocc iSym represents the foreach group
                                        ptrfunc->m_iSym = m_ForTable[m_ForTableNext - 1].forGrpIdx;
                        }
                }
#endif
        }
        else {  // user wrote  "( <variable>

                // different generation if used variable is a group or section
                // or variable

                int iGrp=0;

                // is that variable a group
                if( Tkn == TOKGROUP || Tkn == TOKBLOCK )
                {
                        int isymGroup = ( Tkn == TOKGROUP ) ? Tokstindex :
                                                              GetSymbolEngineBlock(Tokstindex).GetGroupT()->GetSymbol();

                        // Always accept occurrences
                        bool    bAllowExpresion = true;

                        iGrp = grpanal( isymGroup, bAllowExpresion, 0 ); //

                        GENERATE_CODE( ptrfunc->m_iArgumentType = FNGR_Spec::ARG_GROUP );

                        if( iGrp == -1 ) {
                                ASSERT(0); // Why????
                                NextToken();
                                return iProg;
                        }
                }
                else if( Tkn == TOKSECT )
                {
                        GENERATE_CODE( ptrfunc->m_iSym = Tokstindex );
                        GENERATE_CODE( ptrfunc->m_iArgumentType = FNGR_Spec::ARG_SECTION );

                        NextToken();
                }
                else if( Tkn == TOKVAR && VPT(Tokstindex)->IsArray() )
                {
                        GENERATE_CODE( ptrfunc->m_iArgumentType = FNGR_Spec::ARG_MULTIPLE_VAR );
                        iGrp = varsanal( VPT(Tokstindex)->GetFmt(), NOT_COMPLETE_COMPILATION );
                }
                else
                        THROW_PARSER_ERROR0( 8300 );

                GENERATE_CODE( ptrfunc->fn_arg = iGrp );

                if( Tkn != TOKRPAREN )
                        THROW_PARSER_ERROR0( 24 );
        }

        NextToken();


        return iProg;
}
// RHC END Oct 29, 2000


// Compile getbuffer
// RHF INIC Sep 20, 2001
int CEngineCompFunc::cfun_fnb() {
        int     iProg    = Prognext;
        int     iFunCode = CurrentToken.function_details->code;
        int     iExpr    = -1;

#ifdef GENCODE
        FNC_NODE*   pFnbNode = NODEPTR_AS( FNC_NODE );

        if( Flagcomp )
                ADVANCE_NODE( FNC_NODE );
#endif

        NextToken();
        IssueErrorOnTokenMismatch(TOKLPAREN, ERROR_INVALID_FUNCTION_CALL);

        NextToken();

        if( !IsCurrentTokenVART(*this) )
                THROW_PARSER_ERROR0( 11 );

        const VART* pVarT = VPT(Tokstindex);
        const DICT* pDicT = pVarT->GetDPT();

        if( pDicT != nullptr && pDicT->GetCaseAccess() != nullptr )
            pDicT->GetCaseAccess()->SetUsesGetBuffer();

        iExpr = varsanal( pVarT->GetFmt() );
        CHECK_SYNTAX_ERROR_AND_THROW(0);

        IssueErrorOnTokenMismatch(TOKRPAREN, 17);

        NextToken();

#ifdef GENCODE
        if( Flagcomp ) {
                pFnbNode->fn_code = iFunCode;
                pFnbNode->isymb   = iExpr;
        }
#endif

        return iProg;
}
// RHF END Sep 20, 2001



int CEngineCompFunc::cfun_fnins()
{
    FunctionCode function_code = CurrentToken.function_details->code;
    int group_node = -1;
    int first_occurrence = -1;
    int last_occurrence = -1;

    m_pEngineDriver->SetHasSomeInsDelSortOcc(true);

    NextToken();
    IssueErrorOnTokenMismatch(TOKLPAREN, ERROR_INVALID_FUNCTION_CALL);

    // check if the record occurrence is going to be specified using commas
    bool use_comma_syntax = true;

    if( function_code != FNSWAP_CODE )
    {
        MarkInputBufferToRestartLater();
        NextTokenWithPreference(SymbolType::Group);
        NextToken();
        use_comma_syntax = ( Tkn == TOKCOMMA );
        RestartFromMarkedInputBuffer();
    }

    NextTokenWithPreference(SymbolType::Group);

    if( Tkn != TOKGROUP )
        IssueError(8300);

    if( use_comma_syntax )
        group_node = grpanal(Tokstindex, false);

    else
    {
        int check_limit_type = ( function_code == FNDELETE_CODE ) ? CHECKLIMIT_DELETE : CHECKLIMIT_INSERT;
        group_node = grpanal(Tokstindex, true, check_limit_type);
    }

    CHECK_SYNTAX_ERROR_AND_THROW(0);

    if( use_comma_syntax )
    {
        // insert and delete need to know that this syntax is the new format
        if( function_code != FNSWAP_CODE )
            group_node *= -1;

        const GROUPT* pGroupT = GPT(Tokstindex);

        // check that it's a repeating group
        if( pGroupT->GetMaxOccs() <= 1 )
        {
            IssueError(( function_code == FNSWAP_CODE )   ? 8309 :
                       ( function_code == FNDELETE_CODE ) ? 8305 :
                                                            8304);
        }

        IssueErrorOnTokenMismatch(TOKCOMMA, 528);

        auto read_and_bounds_check_index = [&]() -> int
        {
            bool can_check_bounds = ( CheckNextTokenHelper() == NextTokenHelperResult::NumericConstantNonNegative );

            NextToken();

            if( can_check_bounds && ( Tokvalue < 1 || Tokvalue > pGroupT->GetMaxOccs() ) )
                IssueError(8302, pGroupT->GetMaxOccs());

            return exprlog();
        };

        first_occurrence = read_and_bounds_check_index();

        // only swap requires two occurrences
        if( function_code == FNSWAP_CODE || Tkn == TOKCOMMA )
        {
            IssueErrorOnTokenMismatch(TOKCOMMA, 528);

            last_occurrence = read_and_bounds_check_index();
        }
    }

    IssueErrorOnTokenMismatch(TOKRPAREN, 17);

    NextToken();

    auto& ins_node = CreateCompilationNode<FNINS_NODE>(function_code);
    ins_node.group_node = group_node;
    ins_node.first_occurrence = first_occurrence;
    ins_node.last_occurrence = last_occurrence;

    return GetCompilationNodeProgramIndex(ins_node);
}


int CEngineCompFunc::cfun_fnsrt()
{
    int iProg = Prognext;
    int iFunCode = CurrentToken.function_details->code;
    int iWhere = -1; // 20110810

    m_pEngineDriver->SetHasSomeInsDelSortOcc( true );// RHF Dec 13, 2002

#ifdef GENCODE
    FNSRT_NODE*  ptrfunc = NODEPTR_AS( FNSRT_NODE );

    if( Flagcomp )
    {
        ADVANCE_NODE( FNSRT_NODE );
        ptrfunc->fn_code = iFunCode;
        //ptrfunc->m_iSym  = 0;                           // RHF Oct 31, 2000
    }
#endif

    NextToken();
    IssueErrorOnTokenMismatch(TOKLPAREN, ERROR_INVALID_FUNCTION_CALL);

    NextTokenWithPreference(SymbolType::Group);

    // call without arguments
    if( Tkn == TOKRPAREN )
    {
#ifdef GENCODE
        if( Flagcomp )
        {
            ptrfunc->fn_grp = -1;

            // In Foreach?
            if( m_ForTableNext - 1 >= 0 )
            {
                if( iFunCode == FNCUROCC_CODE )
                    ptrfunc->m_iSym = m_ForTable[m_ForTableNext - 1].forVarIdx;
                else // In totocc iSym represents the foreach group
                    ptrfunc->m_iSym = m_ForTable[m_ForTableNext - 1].forGrpIdx;
            }
        }
#endif
    }

    else
    {
        if( Tkn != TOKGROUP )
            IssueError(8300);

        /* Other checking?
        if( iFunCode == FNCUROCC_CODE ) {
        if( NPT(InCompIdx)->IsA(SymbolType::Variable) && !VPT(InCompIdx)->IsAncestor( Tokstindex) )
        IssueError( 9999 ); // Group must be ancestor
        if( NPT(InCompIdx)->IsA(SymbolType::Group) && !GPT(InCompIdx)->IsAncestor(Tokstindex) )
        IssueError( 9999 ); // Group must be ancestor
        }
        */

        bool bAllowExpresion = false; // Because there is no occurrence tree

        int iGrp = Tokstindex;
        GROUPT* pGrpT = GPT( Tokstindex );

        NextToken();
        IssueErrorOnTokenMismatch(TOKUSING, ERROR_INVALID_FUNCTION_CALL);

        std::vector<int> sort_symbols;
        bool read_ascending_descending_token = false;

        // keep on reading while they provide variables upon which to perform the sort
        do
        {
            NextToken();

            if( !sort_symbols.empty() && ( Tkn == TOKASCENDING || Tkn == TOKDESCENDING ) )
            {
                read_ascending_descending_token = true;
                break;
            }

            bool sort_order_descending = false;

            // allow a negative sign to specify descending behavior
            if( Tkn == TOKMINOP || Tkn == TOKMINUS )
            {
                sort_order_descending = true;
                NextToken();
            }

            IssueErrorOnTokenMismatch(TOKVAR, ERROR_INVALID_FUNCTION_CALL);

            // make sure the symbol hasn't already been added
            int symbol_index = Tokstindex;

            if( std::find_if(sort_symbols.begin(), sort_symbols.end(),
                [symbol_index](int added_symbol_index) { return ( symbol_index == abs(added_symbol_index) ); }) != sort_symbols.end() )
            {
                issaerror(MessageType::Error, 8310, VPT(symbol_index)->GetName().c_str());
                return 0;
            }

            VART* pVarT = VPT(symbol_index);

            if( pVarT->GetOwnerGroup() != iGrp )
                IssueError(8306);

            if( pGrpT->GetMaxOccs() <= 1 )
                IssueError(8307);

            sort_symbols.push_back(symbol_index * ( sort_order_descending ? -1 : 1 ));

            NextToken();

        } while( Tkn == TOKCOMMA );

        if( Tkn == TOKWHERE ) // 20110810 allow where conditions in a sort
        {
            m_bcvarsubcheck = true; // this is copied from the sum/average/etc. function
            stopUsingForPrecedence(iGrp);
            Flagvars = 1;
            FNGR_NODE curoccGrpIdx;
            curoccGrpIdx.fn_arg = iGrp;
            curoccGrpIdx.m_iArgumentType = FNGR_Spec::ARG_WHERE_GROUP;
            m_pCuroccGrpIdx = &curoccGrpIdx;

            NextToken();
            iWhere = exprlog();

            m_pCuroccGrpIdx = NULL;
            Flagvars = 0;
            useForPrecedence();
            m_bcvarsubcheck = false;
        }


        // 20110810 second condition added so that the user can't use a - sign and a ascending or descending clause
        if( ( read_ascending_descending_token || Tkn == TOKCOMMA ) &&
            std::find_if(sort_symbols.begin(), sort_symbols.end(), [](int symbol_index) { return ( symbol_index < 0 ); }) == sort_symbols.end() )
        {
            if( !read_ascending_descending_token )
                NextToken();

            if( Tkn == TOKDESCENDING )
            {
                for( size_t i = 0; i < sort_symbols.size(); ++i )
                    sort_symbols[i] *= -1;
            }

            else
                IssueErrorOnTokenMismatch(TOKASCENDING, 8300);

            NextToken();
        }

#ifdef GENCODE
        if( Flagcomp )
        {
            ptrfunc->fn_grp = iGrp;

            // prior to CSPro 7.2 you could only specify one variable to sort with;
            // to indicate that there are multiple variables, fn_itm will be -1 * the
            // number of variables to sort with, and m_iSym will point to the block of
            // memory that specifies the variable and descending statuses
            if( sort_symbols.size() == 1 )
            {
                ptrfunc->fn_itm = abs(sort_symbols.front());
                ptrfunc->m_iSym = ( sort_symbols.front() < 0 ) ? 1 : 0;
            }

            else
            {
                int number_symbols = (int)sort_symbols.size();
                ptrfunc->fn_itm = -1 * number_symbols;
                ptrfunc->m_iSym = Prognext;

                int* symbol_block = PPT(Prognext);
                OC_CreateCompilationSpace(number_symbols);

                memcpy(symbol_block, sort_symbols.data(), number_symbols * sizeof(sort_symbols[0]));
            }

            ptrfunc->m_iWhere = iWhere;
        }
#endif
        IssueErrorOnTokenMismatch(TOKRPAREN, 24);

        NextToken(); // take the next token and check if it is an position where to insert delete
    }

    return iProg;
}


int CEngineCompFunc::FillImplicitIndexes( int iVarT ) {
        int             ipt = Prognext;
#ifdef GENCODE
        VART*   pVarT=VPT(iVarT);

        if( Flagcomp ) {
                RELT        *pRelT;
                RELATED     *pRelated, aRelated;

                MVAR_NODE*      pMVarNode = NODEPTR_AS( MVAR_NODE );
                ADVANCE_NODE( MVAR_NODE );                      // RHF Aug 04, 2000

                pMVarNode->m_iVarType = MVAR_CODE;
                pMVarNode->m_iVarIndex = iVarT;


                GROUPT* pGrpT = pVarT->GetParentGPT();

                for( int i = pVarT->GetNumDim() - 1; i >= 0; i--) {
                        ASSERT( pGrpT );
                        int      iGroupIndex = ( pGrpT != NULL ) ? pGrpT->GetSymbolIndex() : 0;

                        // implicit index
                        // first check if open for statements for this group
                        // if the group is found use the for variable as subinex
                        // else use the group occurrence number as subindex
                        int k;
                        for ( k = m_ForTableNext - 1; k >= 0; k-- ) { // RHF Jul 23, 2001
                                if( m_ForTable[k].forType == 'G' ) {
                                        if( m_ForTable[k].forGrpIdx == iGroupIndex ) {
                                                break;
                                        }
                                }
                                else if( m_ForTable[k].forType == 'R' ) {
                                        pRelT = RLT( m_ForTable[k].forRelIdx );
                                        pRelated = pRelT->GetRelated( &aRelated, iVarT, pVarT->GetDimType( i ) );
                                        if( pRelated != NULL )
                                                break;
                                }
                        }

                        if( k >= 0 ) {

                                if( m_ForTable[k].forType == 'G' ) {
                                        pMVarNode->m_iVarSubindexType[i] = MVAR_EXPR;
                                        pMVarNode->m_iVarSubindexExpr[i] = genSVARNode( m_ForTable[k].forVarIdx, SymbolType::WorkVariable ); // RHF Aug 07, 2000 Add SymbolType::WorkVariable parameter
                                }
                                else if( m_ForTable[k].forType == 'R' ) {
                                        pMVarNode->m_iVarSubindexType[i] = MVAR_EXPR;
                                        pMVarNode->m_iVarSubindexExpr[i] = genSVARNode(
                                                pRelated->iRelatedWVarIdx, SymbolType::WorkVariable );
                                }
                        }
                        else {
                                pMVarNode->m_iVarSubindexType[i] = MVAR_GROUP;
                                pMVarNode->m_iVarSubindexExpr[i] = iGroupIndex;
                        }

                        pGrpT = pGrpT->GetParentGPT();
                }
        }
#endif

        return ipt;
}

// search in stack internal methods

bool CEngineCompFunc::forStackHasElements()
{
        return m_ForTableNext > 0;
}

int CEngineCompFunc::searchInForStackThisGroup( int iGroupIndex ) // rcl, Jul 28, 2004
{
        int iRes = 0;
        for ( int k = m_ForTableNext - 1; k >= 0; k-- ) {
                if( m_ForTable[k].forType == 'G' ) {
                        if( m_ForTable[k].forGrpIdx == iGroupIndex ) {
                                iRes = m_ForTable[k].forVarIdx;
                                break;
                        }
                }
        }

        return iRes;
}

int CEngineCompFunc::searchInForStackThisGroupOrVar( int iGroupIndex,
                                                                                                        int iVarT,
                                                                                                        CDimension::VDimType vType,
                                                                                                        bool bConsiderOnlyForVariable ) // rcl, Jul 28. 2004
{
        RELATED  aRelated;
        RELATED* pRelated = 0;
        RELT*    pRelT = 0;

        int iRes = 0;

        for ( int k = m_ForTableNext - 1; k >= 0; k-- ) { // RHF Jul 23, 2001
                if( m_ForTable[k].forType == 'G' )
                {
                        if( m_ForTable[k].forGrpIdx == iGroupIndex )
                        {
                                iRes = m_ForTable[k].forVarIdx;
                                break;
                        }
                }
                else
                        if( m_ForTable[k].forType == 'R' )
                        {
                                pRelT    = RLT( m_ForTable[k].forRelIdx );
                                pRelated = pRelT->GetRelated( &aRelated, iVarT, vType );
                                if( pRelated != NULL )
                                {
                                        if( bConsiderOnlyForVariable )
                                                iRes = m_ForTable[k].forVarIdx;
                                        else
                                                iRes = pRelated->iRelatedWVarIdx;
                                        break;
                                }
                        }
        }

        return iRes;
}

int CEngineCompFunc::searchInForStackThisGroupOrVar( int iGroupIndex,
                                                                                                        int iVarT,
                                                                                                        CDimension::VDimType vType ) // rcl, Jul 28. 2004
{
        return searchInForStackThisGroupOrVar( iGroupIndex, iVarT, vType, false );
}

// ExecSytem( alpha_expr
//                                      [,MAXIMIZED|NORMAL|MINIMIZED]           -->default NORMAL
//                                      [,FOCUS|NOFOCUS]                                        -->default FOCUS
//                                      [,WAIT|NOWAIT]                                          -->default NOWAIT
//                      )

int CEngineCompFunc::cfun_fnexecsystem()
{
    int iProg = Prognext;
    int iFunCode = CurrentToken.function_details->code;

#ifdef GENCODE
    FNEXECSYSTEM_NODE* ptrfunc = NODEPTR_AS(FNEXECSYSTEM_NODE);

    if( Flagcomp )
    {
        ADVANCE_NODE(FNEXECSYSTEM_NODE);
        ptrfunc->fn_code = iFunCode;
    }
#endif

    NextToken();
    IssueErrorOnTokenMismatch(TOKLPAREN, 14);

    int iCommandExpr = 0;

    NextToken();

    if( iFunCode == FNEXECPFF_CODE && Tkn == TOKPFF )
    {
        iCommandExpr = -1 * Tokstindex;
        NextToken();
    }

    else
    {
        iCommandExpr = CompileStringExpression();
    }

    int iOption = EXECSYSTEM_DEFAULT_OPTIONS;
    bool bHasSize = false;
    bool bHasFocus = false;
    bool bHasWait = false;

    while( Tkn == TOKCOMMA )
    {
        switch( NextKeyword({ _T("MAXIMIZED"), _T("NORMAL"), _T("MINIMIZED"), _T("FOCUS"), _T("NOFOCUS"), _T("WAIT"), _T("NOWAIT"), _T("STOP") }) )
        {
            case 1: //MAXIMIZED
                if( bHasSize )
                    IssueError(916);
                if( (iOption & EXECSYSTEM_NOFOCUS) != 0 ) //maximized/nofocus unsuported
                    IssueError(917);
                bHasSize = true;
                iOption &= ~(EXECSYSTEM_MINIMIZED|EXECSYSTEM_NORMAL);
                iOption |= EXECSYSTEM_MAXIMIZED;
                break;

            case 2: //NORMAL
                if( bHasSize )
                    IssueError(916);
                bHasSize = true;
                iOption &= ~(EXECSYSTEM_MAXIMIZED|EXECSYSTEM_MINIMIZED);
                iOption |= EXECSYSTEM_NORMAL;
                break;

            case 3: //MINIMIZED
                if( bHasSize )
                    IssueError(916);
                bHasSize = true;
                iOption &= ~(EXECSYSTEM_MAXIMIZED|EXECSYSTEM_NORMAL);
                iOption |= EXECSYSTEM_MINIMIZED;
                break;

            case 4: //FOCUS
                if( bHasFocus )
                    IssueError(916);
                bHasFocus = true;
                iOption &= ~(EXECSYSTEM_NOFOCUS);
                iOption |= EXECSYSTEM_FOCUS;
                break;

            case 5: //NOFOCUS
                if( bHasFocus )
                    IssueError(916);
                if( (iOption & EXECSYSTEM_MAXIMIZED) != 0 ) //maximized/nofocus unsuported
                    IssueError(917);
                bHasFocus = true;
                iOption &= ~(EXECSYSTEM_FOCUS);
                iOption |= EXECSYSTEM_NOFOCUS;
                break;

            case 6: //WAIT
                if( bHasWait )
                    IssueError(916);
                bHasWait = true;
                iOption &= ~(EXECSYSTEM_NOWAIT);
                iOption |= EXECSYSTEM_WAIT;
                break;

            case 7: //NOWAIT
                if( bHasWait )
                    IssueError(916);
                bHasWait = true;
                iOption &= ~(EXECSYSTEM_WAIT);
                iOption |= EXECSYSTEM_NOWAIT;
                break;

            case 8: // STOP
                iOption |= EXECSYSTEM_STOP;
                break;

            case 0: // an invalid option
            default:
                IssueError(916);
                break;
        }

        NextToken();
    }

    IssueErrorOnTokenMismatch(TOKRPAREN, 17);

#ifdef GENCODE
    if( Flagcomp )
    {
        ptrfunc->m_iCommand = iCommandExpr;
        ptrfunc->m_iOptions = iOption;
    }
#endif

    NextToken();

    return iProg;
}


// Show( Multiple Record | Multiple Group | Relation | Multiple Item [,] Var-List
//               [Where Log-Expression] [Title(“Text1”, …., “Textn”)]  )
// if title has too many column titles, they are ignored (no warning)
// Var-List use optional comma to separate elements

int CEngineCompFunc::cfun_fnshow()
{
    const FunctionCode function_code = CurrentToken.function_details->code;
    int iHeading = -1;
    int program_index = -1;

    NextToken();
    IssueErrorOnTokenMismatch(TOKLPAREN, 14);

    // if the first parameter is alpha, then it is the heading
    const NextTokenHelperResult eNextToken = CheckNextTokenHelper();

    NextToken();

    if( IsCurrentTokenString() && eNextToken != NextTokenHelperResult::Array )
    {
        iHeading = CompileStringExpression();
        NextToken();
    }

    if( function_code == FunctionCode::FNSHOW_CODE )
    {
        auto& show_node = CreateNode<FNSHOW_NODE>(function_code);
        InitializeNode(show_node, -1, 1);
        show_node.m_iHeading = iHeading;

        program_index = GetCompilationNodeProgramIndex(show_node);

        m_bcvarsubcheck = true; // 20100312 turning off subscript checks (false means off here because m_icGrpIdx will be 0)

        CArray<int, int> aAlphaExpr;
        int iAlphaExpr;
        int iShowListNode = -1;

        m_iShowfnGroupIdx = Tokstindex; //Added by Savy (R) 20090731 //Fix for show() warning issue

        show_node.m_iForNode = CompileForStatement( &CEngineCompFunc::CompileShowList );
        ASSERT(show_node.m_iForNode >= 0);

        if( GetSyntErr() != 0 )
            return 0;


        int iWhereExpr = 0;
        bool bWhere = false;
        bool bTitle = false;

        // RHF INIT Dec 17, 2007

        //RHF END Dec 17, 2007

        m_bcvarsubcheck = true; // 20100312 turning off subscript checks again (because CompileForStatement can set it to false)

        while( Tkn == TOKWHERE || Tkn == TOKTITLE )
        {
            if( Tkn == TOKWHERE )
            {
                if( bWhere )
                    IssueError(9220);

                NextToken();
                iWhereExpr = exprlog();

                if( iWhereExpr < 0 )
                    iWhereExpr = 0; // Empty

                bWhere = true;
                m_iShowfnGroupIdx = 0;//Added by Savy (R) 20090731 //Fix for show() warning issue
            }

            else if( Tkn == TOKTITLE )
            {
                // [Title(“Text1”, …., “Textn”)]
                if( bTitle )
                    IssueError(9220);

                NextToken();
                IssueErrorOnTokenMismatch(TOKLPAREN, 517);

                NextToken();

                while( IsCurrentTokenString() )
                {
                    iAlphaExpr = CompileStringExpression();

                    if( Tkn == TOKCOMMA )
                        NextToken();

                    aAlphaExpr.Add(iAlphaExpr);
                }

                IssueErrorOnTokenMismatch(TOKRPAREN, ERROR_RIGHT_PAREN_EXPECTED);

                NextToken();
                bTitle = true;

            }
            if( Tkn == TOKCOMMA )    // BMD 09 Feb 2007
                NextToken();
        }

        // RHF INIT Dec 17, 2007

        // RHF END Dec 17, 2007

#ifdef GENCODE
        if( Flagcomp ) {
                // title list
                if( aAlphaExpr.GetSize() == 0 ) {
                    show_node.m_iTitleList = -MAXLONG;
                }
                else {
                    show_node.m_iTitleList = Prognext;

                    LIST_NODE* pTitleListNode = (LIST_NODE*) (PPT(Prognext));
                    ADVANCE_NODE( LIST_NODE );

                    int iNumTitles=aAlphaExpr.GetSize();
                    OC_CreateCompilationSpace( iNumTitles - 1 );

                    pTitleListNode->iNumElems = iNumTitles;
                    for( int iTitle = 0; iTitle < iNumTitles; iTitle++ ) {
                        pTitleListNode->iSym[iTitle] = aAlphaExpr[iTitle];
                    }
                }


                // Recall iShowListNode
                int iActualForNode = show_node.m_iForNode;

                if( *PPT(iActualForNode) == SCOPE_CHANGE_CODE )
                    iActualForNode = reinterpret_cast<const Nodes::ScopeChange*>(PPT(show_node.m_iForNode))->program_index;

                const int iForCode = *PPT(iActualForNode);

                if( iForCode == FOR_RELATION_CODE ) {
                    FORRELATION_NODE* pForRelationNode=(FORRELATION_NODE*)( PPT(iActualForNode) );

                    pForRelationNode->forWhereExpr = iWhereExpr;
                    iShowListNode = pForRelationNode->forBlock;
                }
                else {
                    ASSERT( iForCode == FOR_GROUP_CODE );
                    FORGROUP_NODE* pForGroupNode=(FORGROUP_NODE*)( PPT(iActualForNode) );

                    pForGroupNode->forWhereExpr = iWhereExpr;
                    iShowListNode = pForGroupNode->forBlock;
                }

                if( iShowListNode >= 0 ) {
                    show_node.m_iShowListNode = iShowListNode;
                    SHOWLIST_NODE* pShowListNode=(SHOWLIST_NODE*) PPT(show_node.m_iShowListNode);
                    if( pShowListNode->st_code != FNSHOWLIST_CODE ) {
                        ASSERT(0);
                        show_node.m_iShowListNode = -1;
                    }
                }
                else {
                    ASSERT(0);
                }

        }
#endif
        m_bcvarsubcheck = false; // 20100312 turning off subscript checks
    }

    else // 20140423 for the showarray function
    {
        int iArrayCode = Tokstindex;
        int iRowCount = -1;
        int iColCount = -1;
        std::vector<int> aTitles;
        const LogicArray* show_array = nullptr;
        bool bError = false;

        if( Tkn != TOKARRAY )
        {
            bError = true;
        }

        else
        {
            show_array = &GetSymbolLogicArray(iArrayCode);

            if( show_array->IsNumeric() )
                bError = true;

            else if( show_array->GetNumberDimensions() > 2 )
                bError = true;
        }

        if( bError )
        {
            issaerror(MessageType::Error, 956, ToString(DataType::String), _T("1-2"));
            return 0;
        }

        NextToken();

        if( Tkn == TOKCOMMA )
        {
            bool keep_processing = true;

            NextToken();

            if( Tkn != TOKTITLE ) // they are specifying the number of rows
            {
                iRowCount = exprlog();

                if( Tkn == TOKCOMMA )
                    NextToken();

                else
                    keep_processing = false;
            }

            if( keep_processing )
            {
                if( Tkn != TOKTITLE ) // they are specifying the number of columns
                {
                    if( show_array->GetNumberDimensions() == 1 ) // don't allow this for a one-dimensional array
                        IssueError(93036);

                    iColCount = exprlog();
                }

                else // they are instead specifying the titles (and thus, indirectly, the number of columns)
                {
                    NextToken();

                    if( Tkn != TOKLPAREN )
                        IssueError(14);

                    while( keep_processing )
                    {
                        NextToken();

                        int iTitle = CompileStringExpression();
                        aTitles.push_back(iTitle);

                        keep_processing = ( Tkn == TOKCOMMA );
                    }

                    IssueErrorOnTokenMismatch(TOKRPAREN, 17);

                    size_t max_number_columns = 1;

                    if( show_array->GetNumberDimensions() == 2 )
                    {
                        // the - 1 is because we will only use columns starting at the one-based index
                        max_number_columns = show_array->GetDimension(1) - 1;
                    }

                    if( aTitles.size() > max_number_columns )
                    {
                        issaerror(MessageType::Error, 93035, aTitles.size(), max_number_columns);
                        return 0;
                    }

                    NextToken();
                }
            }
        }

        program_index = Prognext;

#ifdef GENCODE
        if( Flagcomp )
        {
            FNN_NODE* ptrfunc = NODEPTR_AS(FNN_NODE);
            OC_CreateCompilationSpace(( sizeof(FNN_NODE) / sizeof(int) ) + 3 + aTitles.size());

            ptrfunc->fn_code = function_code;
            ptrfunc->fn_nargs = 4 + aTitles.size();
            ptrfunc->fn_expr[0] = iArrayCode;
            ptrfunc->fn_expr[1] = iRowCount;
            ptrfunc->fn_expr[2] = iColCount;
            ptrfunc->fn_expr[3] = iHeading;
            memcpy(&ptrfunc->fn_expr[4], aTitles.data(), aTitles.size() * sizeof(aTitles[0]));
        }
#endif
    }


    IssueErrorOnTokenMismatch(TOKRPAREN, 17);

    NextToken();

    ASSERT(program_index != -1);
    return program_index;
}


//Compile show function var list
int CEngineCompFunc::CompileShowList()
{
    auto& show_list_node = CreateNode<SHOWLIST_NODE>(FunctionCode::FNSHOWLIST_CODE);
    show_list_node.next_st = -1;
    show_list_node.m_iVarListNode = -1; // see below

    std::vector<int> var_list;

    if( Tkn == TOKCOMMA ) // Comma optional
        NextToken();

    while( true )
    {
        int var;

        if( Tkn == TOKWORKSTRING )
        {
            var = -1 * Tokstindex;
            NextToken();
        }

        else if( Tkn == TOKVAR )
        {
            if( IsCurrentTokenVART(*this) )
            {
                var = varsanal( VPT(Tokstindex)->GetFmt() );
            }

            else
            {
                IssueError(11); // solo acepta CTES (num o alphas) y VARIABLES
            }
        }

        else
        {
            break;
        }

        var_list.emplace_back(var);

        if( Tkn == TOKCOMMA )
            NextToken();
    }

    if( var_list.empty() )
        IssueError(9222);

    show_list_node.m_iVarListNode = CreateListNode(var_list);

    return GetCompilationNodeProgramIndex(show_list_node);
}


//////////////////////////////////////////////////////////////////////////
// Added for Export handling -
void CEngineCompFunc::FillImplicitIndex( MVAR_NODE* pMVarNode, int iVarT, int iDim, int iGroupIndex ) {

#ifdef GENCODE
        if( Flagcomp ) {
                int             k;

                VART*   pVarT=VPT(iVarT);

#ifdef _DEBUG
                ASSERT( iDim >= 0 && iDim <= pVarT->GetNumDim() - 1 );
#endif

#ifdef GENCODE
                RELT        *pRelT;
                RELATED     *pRelated, aRelated;
#endif

                // first check if open for statements for this group
                // if the group is found use the for variable as subinex
                // else use the group occurrence number as subindex
                for ( k = m_ForTableNext - 1; k >= 0; k-- ) { // RHF Jul 23, 2001
                        if( m_ForTable[k].forType == 'G' ) {
                                if( m_ForTable[k].forGrpIdx == iGroupIndex ) {
                                        break;
                                }
                                /*  // BMD 13 Jan 2004

                                #ifdef BUCEN
                                else {
                                GROUPT* pGpt = GPT(m_ForTable[k].forGrpIdx);
                                if (pGrpT->GetRecord(0) == pGpt->GetRecord(0)) break;
                                }

                                #endif
                                */
                        }
                        else if( m_ForTable[k].forType == 'R' ) {
                                pRelT = RLT( m_ForTable[k].forRelIdx );
                                pRelated = pRelT->GetRelated( &aRelated, iVarT, pVarT->GetDimType( iDim ) );
                                if( pRelated != NULL )
                                        break;
                        }
                }

                // RHF INIC Sep 20, 2001
                // RHF COM Jul 23, 2001 if( k < m_ForTableNext )
                if( k >= 0 ) { // RHF Jul 23, 2001
                        if( m_ForTable[k].forType == 'G' ) {
                                pMVarNode->m_iVarSubindexType[iDim] = MVAR_EXPR;
                                pMVarNode->m_iVarSubindexExpr[iDim] = genSVARNode( m_ForTable[k].forVarIdx, SymbolType::WorkVariable ); // RHF Aug 07, 2000 Add SymbolType::WorkVariable parameter
                        }
                        else if( m_ForTable[k].forType == 'R' ) {
                                pMVarNode->m_iVarSubindexType[iDim] = MVAR_EXPR;
                                pMVarNode->m_iVarSubindexExpr[iDim] = genSVARNode(
                                        pRelated->iRelatedWVarIdx, SymbolType::WorkVariable );
                        }
                        // RHF END Sep 20, 2001
                }
                else {
                        pMVarNode->m_iVarSubindexType[iDim] = MVAR_GROUP;
                        pMVarNode->m_iVarSubindexExpr[iDim] = iGroupIndex;
                }
        }
#endif
}

//////////////////////////////////////////////////////////////////////////


// getLocalMVarNodePtr() only used [for now] in crosstab when
// extra information about generated MVAR_NODES, but no GENCODE is availabe
// [so, no PPT is available]
// MVAR_NODE* getLocalVarNodePtr( int iWhichOne );
//                 iWhichNode must be between 0 and (m_varsanalData's size) - 1.
MVAR_NODE* CEngineCompFunc::getLocalMVarNodePtr( int iWhichOne ) // rcl, Nov 2004
{
    // 20141027 modified to use pointers to fix a memory leak
    if( iWhichOne >= 0 && iWhichOne < (int) m_varsanalData.size() )
        return m_varsanalData[iWhichOne];

    // if no good index has been given -> punish user with a null.
    return 0;
}

int CEngineCompFunc::getNewMVarNodeIndex()
{
    int iVarNode = (int)m_varsanalData.size();
    MVAR_NODE * pNewVarNode = new MVAR_NODE; // 20141027 modified to use pointers to fix a memory leak
    memset(pNewVarNode,0,sizeof(MVAR_NODE));
    m_varsanalData.push_back(pNewVarNode);

    return iVarNode;
}


std::vector<const DictNamedBase*> CEngineCompFunc::GetImplicitSubscriptCalculationStack(const EngineItem& engine_item) const
{
    ASSERT(engine_item.GetDictItem().GetItemSubitemOccurs() > 1 || engine_item.GetDictItem().GetRecord()->GetMaxRecs() > 1);

    std::vector<const DictNamedBase*> implicit_subscript_calculation_stack;

    // implicit subscripts can come from the record, parent item, and the item
    auto process_dict_item = [&](const CDictItem& dict_item)
    {
        ASSERT(dict_item.GetRecord() != nullptr);

        // if the item's record repeats, add it
        if( dict_item.GetRecord()->GetMaxRecs() > 1 )
            implicit_subscript_calculation_stack.emplace_back(dict_item.GetRecord());

        // if the item is a subitem and the parent item repeats, add it
        if( dict_item.IsSubitem() && dict_item.GetParentItem()->GetOccurs() > 1 )
            implicit_subscript_calculation_stack.emplace_back(dict_item.GetParentItem());

        // if the item repeats, add it
        if( dict_item.GetOccurs() > 1 )
            implicit_subscript_calculation_stack.emplace_back(&dict_item);
    };

    auto process_groupt = [&](const GROUPT* pGroupT)
    {
        do
        {
            if( pGroupT->GetMaxOccs() > 1 )
            {
                const CDictItem* dict_item = pGroupT->GetFirstDictItem();
                ASSERT(dict_item != nullptr);

                if( dict_item != nullptr )
                    process_dict_item(*dict_item);
            }

            pGroupT = pGroupT->GetParentGPT();

        } while( pGroupT != nullptr);
    };


    // add values that come from the current PROC
    const Symbol* compilation_symbol = NPT(InCompIdx);
    ASSERT(compilation_symbol->IsOneOf(SymbolType::Block, SymbolType::Group, SymbolType::Variable));

    // if in a block PROC, use the group instead
    if( compilation_symbol->IsA(SymbolType::Block) )
        compilation_symbol = assert_cast<const EngineBlock*>(compilation_symbol)->GetGroupT();

    // group PROC
    if( compilation_symbol->IsA(SymbolType::Group) )
    {
        const GROUPT* pGroupT = assert_cast<const GROUPT*>(compilation_symbol);
        process_groupt(pGroupT);
    }

    // item PROC
    else if( compilation_symbol->IsA(SymbolType::Variable) )
    {
        const VART* pVarT = assert_cast<const VART*>(compilation_symbol);
        const CDictItem* dict_item = pVarT->GetDictItem();
        ASSERT(dict_item != nullptr);
        process_dict_item(*dict_item);
    }


    // add values from for loops
    if( m_ForTableNext > 0 )
    {
        const int forRelIdx = m_ForTable[m_ForTableNext - 1].forRelIdx;

        if( forRelIdx != 0 ) // 20140203 this was 0 when compiling the show function (which creates uses the for loop infrastructure)
        {
            const VART* pVarT = &engine_item.GetVarT();
            RELT* pRelT = RLT(forRelIdx);
            RELATED related;

            if( pRelT->GetRelated(&related, pVarT->GetSymbolIndex(), pVarT->GetDimType(0)) != nullptr )
                process_dict_item(engine_item.GetDictItem());
        }
    }

    // add values from functions like count
    if( m_bcvarsubcheck && m_icGrpIdx != 0 )
    {
        const GROUPT* pGroupT = GPT(std::abs(m_icGrpIdx));
        process_groupt(pGroupT);
    }

    return implicit_subscript_calculation_stack;
}
