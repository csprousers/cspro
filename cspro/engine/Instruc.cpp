//---------------------------------------------------------------------------
//  File name: Instruc.cpp
//
//  Description:
//          Syntax parser and analyzer (compiler for main instructions)
//
//  History:    Date       Author   Comment
//              ---------------------------
//              10 Nov 99   RHF     Basic conversion
//              10 Mar 00   RCH     Make ";" optional before a keyword
//              12 Apr 00   RCH     Add multiple dimension variables (1st attack)
//              17 Aug 00   RHC     Add FOR-group
//              21 Aug 00   RHC     Separating compilers for Skip/Advance/Reenter statements
//              17 Oct 00   RHC     Isolating compiler for Do statement
//              25 Oct 00   RHF     Add compiler for Impute command
//              04 Sep 00   RHC     Add Break & Next statements
//              04 Apr 01   vc      Tailoring for RepMgr compatibility
//              18 Apr 05   rcl     for i "in" "in" flagged as compilation error now
//              10 May 05   rcl     Advance to GROUP allowed in compilation time
//                                  Reenter GROUP allowed in compilation time
//                                  Move to GROUP allowed in compilation time
//---------------------------------------------------------------------------

#include "StandardSystemIncludes.h"

#ifdef GENCODE
#include "Exappl.h"
#else
#include "Tables.h"
#endif
#include "COMPILAD.H"
#include "Engine.h"
#include "Ctab.h"
#include "COMPUTIL.H"
#include <zEngineO/AllSymbols.h>
#include <zEngineO/LoopStack.h>
#include <zEngineO/Compiler/TokenHelper.h>
#include <zAppO/Application.h>
#include <zDictO/DDClass.h>
#include <zFormO/FormFile.h>
#include <zLogicO/LocalSymbolStack.h>
#include <zLogicO/Preprocessor.h>


const int TSMAXIDLEN = 16;              // max. len for a break-id var


bool ValidInstructionStartToken(TokenCode token_code)
{
    static std::set ValidInstructionStartTokens =
    {
        TOKHASH,
        TOKIF,
        TOKWHILE,
        TOKRECODE,
        TOKVAR,
        TOKWORKSTRING,
        TOKFUNCTION,
        TOKCROSSTAB,
        TOKKWFREQ,
        TOKEXPORT,
        TOKKWCTAB,
        TOKFOR,
        TOKFORCASE,
        TOKSTOP,
        TOKENDCASE,
        TOKUNIVERSE,
        TOKASK,
        TOKSKIP,
        TOKMOVE,
        TOKEXIT,
        TOKREENTER,
        TOKENTER,
        TOKADVANCE,
        TOKENDSECT,
        TOKENDLEVL,
        TOKNOINPUT,
        TOKBREAK,
        TOKNEXT,
        TOKDO,
        TOKSET,
        TOKUSERFUNCTION,
        TOKNUMERIC,
        TOKALPHA,
        TOKSTRING,
        TOKCONFIG,
        TOKPERSISTENT,
        TOKKWFILE,
        TOKKWARRAY,
        TOKKWLIST,
        TOKKWMAP,
        TOKKWVALUESET,
        TOKARRAY,
        TOKLIST,
        TOKVALUESET,
        TOKKWPFF,
        TOKPFF,
        TOKWHEN,
        TOKKWSYSTEMAPP,
        TOKKWAUDIO,
        TOKAUDIO,
        TOKKWHASHMAP,
        TOKHASHMAP,
        TOKFREQ,
        TOKKWCASE,
        TOKDICT,
        TOKKWDATASOURCE,
        TOKKWIMAGE,
        TOKIMAGE,
        TOKKWDOCUMENT,
        TOKDOCUMENT,
        TOKKWGEOMETRY,
        TOKGEOMETRY,
    };

    return ( ValidInstructionStartTokens.find(token_code) != ValidInstructionStartTokens.cend() );
}


bool ValidEndStatement( int iLastTkn ) {
    // ValidEndStatement: checks if last ending token is a valid end-of-statement
    //   - normally was a ";" only, but more tokens are now accepted
    // ... change: Feb 22, 00 Only ";" is a valid end-of-statement
    bool    bIsValid = false;

    switch( iLastTkn ) {
        case TOKENDIF    :
        case TOKENDDO    :
        case TOKENDRECODE:
        case TOKEND      :
        case TOKENDSECT  :
        case TOKENDLEVL  :
        case TOKELSE     :
        case TOKELSEIF   :
        case TOKSEMICOLON:
        // uncomment the line below and the user will be allowed to
        // avoid the semicolon in the very last statement of a procedure
        case TOKEOP      :
            bIsValid = true;
            break;
    }

    return bIsValid;
}


int CEngineCompFunc::instruc(bool create_new_local_symbol_stack/* = true*/, bool allow_multiple_statements/* = true*/)
{
    int iptblock = Prognext;
    bool bIsSkipStatement = false;
    int code;
    int v_ind = -1;
    int sind;
    int aux;

    // when compiling a user-defined function or a PROC, create_new_local_symbol_stack will be
    // false because there is already a local symbol stack created at that level
    std::optional<Logic::LocalSymbolStack> local_symbol_stack;

    if( create_new_local_symbol_stack )
        local_symbol_stack.emplace(m_symbolTable.CreateLocalSymbolStack());

    Nodes::Statement* previous_instruc_st = nullptr;
    Nodes::Statement* prev_st = NULL;

    int c = TOKSEMICOLON;

    while( c == TOKSEMICOLON && Tkn != TOKEOP )
    {
        try
        {
            // check for cpt, move for function declaration
            if( Tkn == TOKEND && ObjInComp == SymbolType::Application )
                break;

            bIsSkipStatement = false;

            while( Tkn == TOKSEMICOLON )
                NextToken();

            if( !ValidInstructionStartToken(Tkn) )
                break;

            if( ObjInComp == SymbolType::Application && Tkn == TOKNOINPUT )
                IssueError( 562 ); // invalid inside a function

            int saved_prog_next = Prognext;

            // 20100518 for compiling trace symbols
            if( IsTracingLogic() )
            {
                const int trace_program_index = CreateTraceStatement();

                if( trace_program_index != -1 )
                {
                    // COMPILER_DLL_TODO ... the following code for adding trace statements is similar to the code at the end of this loop; eventually this should 
                    // all be refactored to easily append statements to one another

                    // ...potentially set the first instruction
                    if( iptblock == saved_prog_next )
                    {
                        iptblock = trace_program_index;
                    }

                    // or update the previous instruction
                    else if( previous_instruc_st != nullptr )
                    {
                        previous_instruc_st->next_st = trace_program_index;
                    }

                    prev_st = &GetNode<Nodes::Statement>(trace_program_index);
                    prev_st->next_st = Prognext;

                    previous_instruc_st = prev_st;

                    saved_prog_next = Prognext;
                }
            }

            // preprocesor handling
            if( Tkn == TOKHASH )
            {
                ASSERT(m_preprocessor != nullptr);
                m_preprocessor->ProcessLineDuringCompilation();

                // ProcessLineDuringCompilation will process all tokens on the line but not move to the next token, so
                // we do that here and then pretend a semicolon was read to satisfy the loop condition
                NextToken();
                c = TOKSEMICOLON;

                continue;
            }

#ifdef GENCODE
            prev_st = NODEPTR_AS(Nodes::Statement);
#endif

            int last_added_node_address = -1;
            int compilation_address = -1;

            // main switch
            switch( Tkn )
            {
                case TOKCONFIG:
                case TOKPERSISTENT:
                    last_added_node_address = CompileSymbolWithModifiers();
                    break;

                case TOKNUMERIC:
                    last_added_node_address = CompileWorkVariables();
                    break;

                case TOKALPHA:
                case TOKSTRING:
                    last_added_node_address = CompileLogicStrings();
                    break;

                case TOKKWARRAY:
                    last_added_node_address = CompileLogicArrayDeclaration();
                    break;

                case TOKKWAUDIO:
                    last_added_node_address = CompileLogicAudioDeclarations();
                    break;

                case TOKKWCASE:
                    last_added_node_address = CompileEngineCases();
                    break;

                case TOKKWDATASOURCE:
                    last_added_node_address = CompileEngineDataRepositories();
                    break;

                case TOKKWDOCUMENT:
                    last_added_node_address = CompileLogicDocumentDeclarations();
                    break;

                case TOKKWFILE:
                    last_added_node_address = CompileLogicFiles();
                    break;

                case TOKKWGEOMETRY:
                    last_added_node_address = CompileLogicGeometryDeclarations();
                    break;

                case TOKKWHASHMAP:
                    last_added_node_address = CompileLogicHashMapDeclarations();
                    break;

                case TOKKWIMAGE:
                    last_added_node_address = CompileLogicImageDeclarations();
                    break;

                case TOKKWLIST:
                    last_added_node_address = CompileLogicListDeclarations();
                    break;

                case TOKKWMAP:
                    last_added_node_address = CompileLogicMapDeclarations();
                    break;

                case TOKKWPFF:
                    last_added_node_address = CompileLogicPffDeclarations();
                    break;

                case TOKKWSYSTEMAPP:
                    last_added_node_address = CompileSystemAppDeclarations();
                    break;

                case TOKKWVALUESET:
                    last_added_node_address = CompileDynamicValueSetDeclarations();
                    break;

                case TOKRECODE:
                    compilation_address = CompileRecode();
                    break;

                case TOKWHEN:
                    NextToken();
                    compilation_address = CompileWhen();
                    break;

                case TOKIF:
                    compilation_address = CompileIfStatement();
                    break;

                case TOKWHILE:
                    compilation_address = CompileWhileLoop();
                    break;

                case TOKDO:
                    compilation_address = CompileDoLoop();
                    break;

                case TOKNEXT:
                    compilation_address = CompileNextOrBreakInLoop();
                    break;

                case TOKBREAK:
                {
                    if( NextKeywordIf(TOKBY) )
                    {
                        CompileBreakBy();
                        if( GetSyntErr() != 0 ) // victor Sep 20, 00
                            return 0;
                    }

                    else
                    {
                        compilation_address = CompileNextOrBreakInLoop();
                    }

                    break;
                }

                case TOKFOR:
                {
                    std::optional<SymbolType> next_token_symbol_type = GetNextTokenSymbolType();

                    if( next_token_symbol_type == SymbolType::Dictionary || next_token_symbol_type == SymbolType::Pre80Dictionary )
                    {
                        compilation_address = CompileForDictionaryLoop(TOKFOR);
                    }

                    else
                    {
                        // use preference so, in a normal for loop, external dictionary
                        // records are prioritized over groups
                        NextTokenWithPreference(SymbolType::Section);

                        CompileForStatement();
                        if( GetSyntErr() != 0 ) // victor Sep 20, 00
                            return 0;
                    }

                    break;
                }

                case TOKFORCASE:
                {
                    compilation_address = CompileForDictionaryLoop(TOKFORCASE);
                    break;
                }

                case TOKVAR:
                {
                    if( NPT_Ref(Tokstindex).IsA(SymbolType::WorkVariable) || VPT(Tokstindex)->IsNumeric() ) {
                        CompileComputeInstruction();
                        if( GetSyntErr() != 0 )
                            IssueError(GetSyntErr());
                        if( Tkn == TOKVAR || Tkn == TOKCTE || Tkn == TOKLPAREN )
                            IssueError( 2 );
                    }
                    else {
                        CompileStringComputeInstruction();
                        if( GetSyntErr() != 0 ) // victor Sep 20, 00
                            return 0;
                    }
                    break;
                }

                case TOKWORKSTRING:
                {
                    CompileStringComputeInstruction();
                    break;
                }

                case TOKFUNCTION:
                case TOKUSERFUNCTION:
                {
                    // TODO: this all needs to be improved at some point;
                    // for now, setting is_lone_function_call to true will allow the calling of functions that return strings
                    auto& [call_tester, is_lone_function_call] = m_loneAlphaFunctionCallTester;
                    ASSERT(!is_lone_function_call);
                    const RAII::SetValueAndRestoreOnDestruction<bool> is_lone_function_caller_setter(is_lone_function_call, true);

                    if( Tkn == TOKFUNCTION || Tokstindex != InCompIdx )
                    {
                        compilation_address = CompileFunctionCall();
                    }

                    else
                    {
                        // the user function could be called recursively or could be receiving its return value
                        UserFunction& user_function = GetSymbolUserFunction(Tokstindex);

                        if( IsNextToken(TOKLPAREN) )
                        {
                            compilation_address = CompileFunctionCall();
                        }

                        else if( user_function.GetReturnType() == SymbolType::WorkVariable )
                        {
                            CompileComputeInstruction();
                        }

                        else
                        {
                            CompileStringComputeInstruction();
                        }
                    }
                    break;
                }

                case TOKARRAY:
                {
                    if( GetSymbolLogicArray(Tokstindex).IsString() )
                    {
                        CompileStringComputeInstruction();
                    }

                    else
                    {
                        CompileComputeInstruction();
                    }

                    if( GetSyntErr() != 0 )
                        return 0;
                    break;
                }

                case TOKAUDIO:
                {
                    compilation_address = CompileLogicAudioComputeInstruction();
                    break;
                }

                case TOKDICT:
                {
                    if( !GetSymbolEngineDictionary(Tokstindex).HasEngineCase() )
                        IssueError(47252);

                    CompileEngineCaseComputeInstruction();
                    break;
                }

                case TOKDOCUMENT:
                {
                    compilation_address = CompileLogicDocumentComputeInstruction();
                    break;
                }

                case TOKGEOMETRY:
                {
                    compilation_address = CompileLogicGeometryComputeInstruction();
                    break;
                }

                case TOKHASHMAP:
                {
                    CompileLogicHashMapComputeInstruction();
                    break;
                }

                case TOKIMAGE:
                {
                    compilation_address = CompileLogicImageComputeInstruction();
                    break;
                }

                case TOKLIST:
                {
                    compilation_address = CompileLogicListComputeInstruction();
                    break;
                }

                case TOKPFF:
                {
                    CompileLogicPffComputeInstruction();
                    break;
                }

                case TOKFREQ:
                {
                    CompileNamedFrequencyComputeInstruction();
                    break;
                }

                case TOKVALUESET:
                {
                    CompileDynamicValueSetComputeInstruction();
                    break;
                }

                case TOKCROSSTAB:
                {
                    sind  = Tokstindex;

                    MarkInputBufferToRestartLater();               // mark input buffer to restart

                    NextToken();
                    aux = Tkn;
                    Tkn = TOKCROSSTAB;
                    Tokstindex = sind;

                    RestartFromMarkedInputBuffer();               // restart!

                    if( aux == TOKEQOP || aux == TOKLBRACK )
                        rutcpttbl();
                    else
                        CompileComputeInstruction();

                    if( GetSyntErr() != 0 ) // victor Sep 20, 00
                        return 0;
                    break;
                }

                case TOKKWCTAB:             // CROSSTAB in dict' proc
                {
                    compctab( 1, CTableDef::Ctab_Crosstab );
                    if( GetSyntErr() != 0 ) // victor Sep 20, 00
                        return 0;
                    break;
                }

                case TOKKWFREQ:
                    compilation_address = CompileFrequencyDeclaration();
                    break;

                case TOKEXPORT:
                    compexport();
                    if( GetSyntErr() != 0 ) // victor Sep 20, 00
                        return 0;
                    break;

                case TOKASK:
                    CompileAskStatement();
                    if( GetSyntErr() != 0 )
                        return 0;
                    break;

                case TOKSKIP:
                    CompileSkipStatement( &code );
                    if( GetSyntErr() != 0 ) // victor Sep 20, 00
                        return 0;
                    break;

                    // RHF INIC Dec 09, 2003 BUCEN_DEC2003 Changes
                case TOKMOVE:
                    code = Tkn;
                    CompileMoveStatement();
                    if( GetSyntErr() != 0 )
                        return 0;
                    break;
                    // RHF END Dec 09, 2003 BUCEN_DEC2003 Changes

                case TOKREENTER:
                    CompileReenterStatement();
                    if( GetSyntErr() != 0 ) // victor Sep 20, 00
                        return 0;
                    break;

                case TOKADVANCE:
                    CompileAdvanceStatement();
                    if( GetSyntErr() != 0 ) // victor Sep 20, 00
                        return 0;
                    break;

                case TOKENDCASE:
                case TOKUNIVERSE:
                case TOKEXIT:
                    compilation_address = CompileProgramControl();
                    break;

                case TOKENTER:
                    compilation_address = CompileEnter();
                    break;

                // --- selected-tokens <begin>
                //
                //   3. TOKSTOP
                //   6. TOKNOINPUT
                //   7. TOKENDSECT
                //   8. TOKENDLEVL
                // what?? TOKSKIP, TOKREENTER, TOKADVANCE   // what?? victor Mar 08, 01
                case TOKSTOP:
                case TOKENDLEVL:
                    // RHF INIC Dec 22, 2003
                    if( NPT(InCompIdx)->IsA(SymbolType::Variable) && VPT(InCompIdx)->GetSubType() != SymbolSubType::Input
                            ||
                        NPT(InCompIdx)->IsA(SymbolType::Group) && GPT(InCompIdx)->GetSubType() != SymbolSubType::Primary) {
                        SetSyntErr(9192);
                        return 0;
                    }
                    // RHF END Dec 22, 2003
                    [[fallthrough]];
                case TOKENDSECT:
                case TOKNOINPUT:
                {
                    code = Tkn;

                    if( SO::EqualsNoCase(Tokstr, _T("ENDSECT")) )
                        IssueWarning(Logic::ParserMessage::Type::DeprecationMinor, 95001, _T("ENDSECT"), _T("ENDGROUP"));

                    bIsSkipStatement = true; // RHF 27/7/94

                    // Given that we are here we know that
                    // Tkn == { TOKSKIP, TOKREENTER, TOKSTOP, TOKENDSECT, TOKENDLEVL,
                    //          TOKNOINPUT, TOKADVANCE }
                    if( Appl.ApplicationType != ModuleType::Entry &&
                        Tkn != TOKNOINPUT &&                // victor Mar 14, 01
                        Tkn != TOKENDSECT &&                // victor Mar 14, 01
                        Tkn != TOKENDLEVL && // RHF Sep 05, 2001
                        Tkn != TOKSKIP && Tkn != TOKSTOP )
                    {
                        IssueError( 1 );
                    }

                    switch( Tkn ) {         // inner switch
                    //~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~
                    //   Case 3. TOKSTOP
                    //~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~
                    case TOKSTOP:
                    {
                        int stop_expr = -1;

                        if( Flagcomp )
                        {
                            ADVANCE_NODE(STOP_NODE);
                        }

                        NextToken();
                        if( Tkn == TOKLPAREN ) {
                            NextToken();
                            if( Tkn != TOKRPAREN )
                                stop_expr = exprlog();
                            if( Tkn != TOKRPAREN )
                                IssueError( 19 ); // right paren expected

                            NextToken();
                        }

                        if( Flagcomp )
                        {
                            STOP_NODE* stop_node = (STOP_NODE*)prev_st;
                            stop_node->stop_expr = stop_expr;
                        }

                        break;
                    }

                    //~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~
                    //   Case 6. TOKNOINPUT
                    //~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~
                    case TOKNOINPUT:
                        if( ObjInComp != SymbolType::Variable || ( VPT(InCompIdx)->SYMTfrm <= 0 && Appl.ApplicationType == ModuleType::Entry ) ) // RHF Nov 07, 2001
                            IssueError( 557 );  // not in a field

                        if( !( ProcInComp == PROCTYPE_PRE || ProcInComp == PROCTYPE_ONFOCUS) )
                            IssueError( 558 ); // must be at Pre

                        NextToken();
                        break;

                    //~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~
                    //   Case 7. TOKENDSECT
                    //~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~
                    case TOKENDSECT:
                        // RHF INIC Dec 04, 2003 Now endgroup can be called from a function
                           //BUCEN_DEC2003 Changes
                        if( ObjInComp == SymbolType::Group && ( ProcInComp == PROCTYPE_KILLFOCUS || ProcInComp == PROCTYPE_POST) ||
                            m_pEngineArea->IsLevel( InCompIdx ) )
                        {
                            IssueError( 100 );
                        }
                        // RHF END Dec 04, 2003

                        NextToken();
                        break;

                    //~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~
                    //   Case 8. TOKENDLEVL
                    //~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~
                    case TOKENDLEVL:
                        if( Appl.ApplicationType == ModuleType::Batch ) {
                            issaerror( MessageType::Warning, 88150 ); // RHF Nov 08, 2001 Better here instead of in BatchExEndLevel method
                        }

                        if( m_pEngineArea->IsLevel( InCompIdx ) && ( ProcInComp == PROCTYPE_KILLFOCUS || ProcInComp == PROCTYPE_POST) && LvlInComp == 0 )// RHF Jun 05, 2001
                            IssueError( 117 ); // invalid endlevel 0

                        NextToken();

                        break;

                    default:
                        ASSERT(false);
                        NextToken();
                        break;

                    } // inner switch

#ifdef GENCODE
                    // ------------- CODE GENERATION FOR THESE CASES:
                    if( Flagcomp ) {
                        if( code != TOKTO && code != TOKREENTER && code != TOKADVANCE && code != TOKMOVE && code != TOKSTOP ) {
                            ADVANCE_NODE(ST_NODE);
                        }

                        switch( code ) {
                            case TOKSKIP:                   // what?? victor Mar 08, 01
                                code = SKIPCASE_CODE;
                                break;
                                // RHF INIC Dec 09, 2003 BUCEN_DEC2003 Changes
                            case TOKMOVE:
                                code = MOVETO_CODE;
                                break;
                                // RHF END Dec 09, 2003  BUCEN_DEC2003 Changes
                            case TOKTO:                     // what?? victor Mar 08, 01
                                code = SKIPTO_CODE;
                                break;
                            case TOKREENTER:                // what?? victor Mar 08, 01
                                code = REENTER_CODE;
                                break;
                            case TOKADVANCE:                // what?? victor Mar 08, 01
                                code = ADVANCE_CODE;
                                break;
                            case TOKNOINPUT:
                                code = NOINPUT_CODE;
                                break;
                            case TOKSTOP:
                                code = STOP_CODE;
                                break;
                            case TOKENDLEVL:
                                code = ENDLEVL_CODE;
                                break;
                            case TOKENDSECT:
                                code = ENDSECT_CODE;
                                break;
                        }

                        prev_st->function_code = static_cast<FunctionCode>(code);

                        if( code == SKIPTO_CODE  || code == REENTER_CODE || // what?? victor Mar 08, 01
                            code == ADVANCE_CODE || code == MOVETO_CODE ) // RHF Dec 09, 2003 Add MOVETO_CODE BUCEN_DEC2003 Changes
                        {
                            // BackPatch skip node (Link skp_node to SVAR_NODE)
                            SKIP_NODE* skp_node = (SKIP_NODE*)prev_st;
                            skp_node->var_ind = v_ind;
                            skp_node->var_exprind = -1; // RHF COM Aug 22, 2003 i; BUCEN_DEC2003 Changes
                        }
                    }
#endif
                } // --- selected-tokens <end>
                    break;                                  // RHF Apr 04, 2000

                case TOKSET:
                {
                    std::optional<int> compilation_node_index = ci_set();

                    if( GetSyntErr() != 0 )
                        return 0;              // victor Sep 20, 00

                    if( !compilation_node_index.has_value() )
                        continue;

                    break;
                }

                default:
                    ASSERT(0);              // should not happen!
                    break;
            } // main switch

            //  Final check at the end of every instruction
            //  Original Strict rule:
            //                Every statement must end with a ;
            //  Current rule (Apr 03, 2000):
            //                ";" may be omitted before an end (endif,endwhile,etc.)
            //                and some other keywords (see ValidEndStatement() method above)
            //
            if( GetSyntErr() == 0 && !ValidEndStatement(Tkn) )
                IssueError(2);

#ifdef GENCODE
            // when a compilation address is used (so the node was probably not added at Prognext)...
            if( compilation_address >= 0 )
            {
                // ...potentially set the first instruction
                if( iptblock == saved_prog_next )
                    iptblock = compilation_address;

                // or update the previous instruction
                else if( previous_instruc_st != nullptr )
                    previous_instruc_st->next_st = compilation_address;

                prev_st = (Nodes::Statement*)PPT(compilation_address);
            }

            else if( last_added_node_address >= 0 )
                prev_st = (Nodes::Statement*)PPT(last_added_node_address);

            // setup next-statement address into previous instruction
            if( Flagcomp ) {
                if( bIsSkipStatement && Tkn != TOKNOINPUT )
                    prev_st->next_st = -1;
                else
                    prev_st->next_st = Prognext;
            }

            previous_instruc_st = prev_st;
#endif
            c = Tkn;
        }

        catch( const Logic::ParserError& )
        {
            // if there is an error, skip until the next token
            if( Tkn != TOKSEMICOLON )
            {
                SkipBasicTokensUntil(TOKSEMICOLON);
                NextToken();
            }
        }

        if( !allow_multiple_statements )
        {
            if( Tkn == TOKSEMICOLON )
                NextToken();

            break;
        }

    } // end  while( Tkn == TOKSEMICOLON )


#ifdef GENCODE
    if( Flagcomp ) {
        if( prev_st != NULL )
            prev_st->next_st = -1;
        else
            iptblock = -1;
    }
#endif


    if( local_symbol_stack.has_value() )
        iptblock = WrapNodeAroundScopeChange(*local_symbol_stack, iptblock);

    return iptblock;
}


// RHF INIC Jul 02, 2005
void CEngineCompFunc::CheckIdChanger(const VART* pVarT)
{
    if( pVarT->GetSubType() == SymbolSubType::Input && pVarT->GetSPT()->IsCommon() ) { // RHF Aug 17, 2005
        if( !m_pEngineArea->IsLevel(InCompIdx) || ProcInComp != PROCTYPE_PRE || LvlInComp != pVarT->GetLevel() ) { // Allowed only in the corresponding Level PreProc
            // GSF 12-oct-2005 - do not enforce this rule for one-level apps
            SECT* pSecT = pVarT->GetSPT();
            int iNumLevels = pSecT->GetDicT()->GetMaxLevel();
            if(iNumLevels > 1){
                // IssueError(1040);    // GSF 21-nov-2005 - do not ever enforce this rule - too restictive
            }
        }
        else if( m_pEngineSettings->m_bHasCrosstab || m_pEngineSettings->m_bHasExport || m_pEngineSettings->m_bHasFrequency ) {
            IssueError(1044);
        }
        else {
            SetIdChanger( true );
        }
    }
}
// RHF END  Jul 02, 2005


//---------------------------------------------------------------------------
//  CompileComputeInstruction: compile instruction COMPUTE (assignment =)
//---------------------------------------------------------------------------
int CEngineCompFunc::CompileComputeInstruction()
{
    auto& compute_node = CreateCompilationNode<COMPUTE_NODE>(CPT_CODE);

    if( Tkn == TOKCROSSTAB )
    {
        compute_node.cpt_var = tvarsanal();
    }

    else if( Tkn == TOKARRAY )
    {
        compute_node.cpt_var = CompileLogicArrayReference();
    }

    else
    {
        if( NPT_Ref(Tokstindex).IsA(SymbolType::Variable) )
        {
            const VART* pVarT = VPT(Tokstindex);

            if( pVarT->GetLevel() > LvlInComp && pVarT->GetSubType() != SymbolSubType::External )
                IssueError(93);

            // RHF INIT Jul 03, 2005
            CheckIdChanger(pVarT);
            // RHF END Jul 03, 2005
        }

        compute_node.cpt_var = varsanal('N');
    }

    if( GetSyntErr() != 0 )
        IssueError(GetSyntErr());

    IssueErrorOnTokenMismatch(TOKEQOP, 5);

    NextToken();
    compute_node.cpt_expr = exprlog();

    if( GetSyntErr() != 0 )
        IssueError(GetSyntErr());

    return GetCompilationNodeProgramIndex(compute_node);
}


// Compile For Loop Instructions - New version Sep, 2001 RHC
int CEngineCompFunc::CompileForStatement(pCompileForInFunction pCompileFunction/* = nullptr*/)
{
    // with for loops, the loop stack is only used (at the moment) to prevent
    // the enter statement from being made in a loop
    LoopStackEntry loop_stack_entry = GetLoopStack().PushOnLoopStack(LoopStackSource::For);
    
    int iForNode = Prognext;
    int counter_variable_symbol_index = -1;
    size_t type_specifier = 0;

    // create the local symbol stack so that a possible counter varible is created in a new scope
    Logic::LocalSymbolStack local_symbol_stack = m_symbolTable.CreateLocalSymbolStack();

    if( Tkn == TOKNUMERIC )
    {
        const WorkVariable* work_variable = CompileWorkVariableDeclaration();
        counter_variable_symbol_index = work_variable->GetSymbolIndex();
    }

    else if( Tkn == TOKVAR && NPT_Ref(Tokstindex).IsA(SymbolType::WorkVariable) )
    {
        counter_variable_symbol_index = Tokstindex;
    }

    if( counter_variable_symbol_index != -1 )
    {
        // allow "in"
        NextKeyword({ _T("IN") });
        type_specifier = NextKeyword({ _T("RELATION"), _T("GROUP"), _T("RECORD"), _T("ITEM") });
        NextTokenWithPreference(type_specifier == 2 ? SymbolType::Group : SymbolType::Section);
    }

    else
    {
        counter_variable_symbol_index = MakeRelationWorkVar();
    }


    if( type_specifier == 1 || ( type_specifier == 0 && Tkn == TOKRELATION ) )
    {
        if( Tkn != TOKRELATION )
            IssueError(33110);

        CompileForRelation(counter_variable_symbol_index, Tokstindex, pCompileFunction);
    }

    else if( type_specifier == 2 || ( type_specifier == 0 && Tkn == TOKGROUP ) )
    {
        if( Tkn != TOKGROUP )
            IssueError(33000);

        CompileForGroup(counter_variable_symbol_index, Tokstindex, pCompileFunction);
    }

    else if( type_specifier == 3 || ( type_specifier == 0 && Tkn == TOKSECT ) )
    {
        if( Tkn != TOKSECT )
            IssueError(33111);

        m_iForRecordIdx = Tokstindex;
        CompileForRelation(counter_variable_symbol_index, GetRelationSymbol(Tokstindex), pCompileFunction);
        m_iForRecordIdx = 0;
    }

    else if( type_specifier == 4 || ( type_specifier == 0 && Tkn == TOKVAR ) )
    {
        if( Tkn != TOKVAR )
            IssueError(33109);

        if( !VPT(Tokstindex)->IsArray() )
            IssueError(33114);

        CompileForRelation(counter_variable_symbol_index, GetRelationSymbol(Tokstindex), pCompileFunction);
    }

    else
    {
        IssueError(33004);
    }

    // if not called with a compilation function, allow either an endfor or enddo statement to end such a loop
    if( pCompileFunction == nullptr )
    {
        if( Tkn != TOKENDDO && Tkn != TOKENDFOR )
            IssueError(10);

        NextToken();
    }

#ifdef GENCODE // need for-table when compiling with CSpro
    if( Flagcomp )
#endif
    {
        m_ForTableNext--;
        ASSERT(m_ForTableNext >= 0);
    }

    return WrapNodeAroundScopeChange(local_symbol_stack, iForNode);
}


void CEngineCompFunc::CompileForRelation(int iVarIdx, int iRelIdx, pCompileForInFunction pCompileFunction/* = nullptr*/)
{
    int iBlock = 0;
    int iExprWhere = 0;
    FORRELATION_NODE* pForNode = NULL;

#ifdef GENCODE
    if( Flagcomp ) {
        pForNode = NODEPTR_AS( FORRELATION_NODE );
        ADVANCE_NODE( FORRELATION_NODE );

        pForNode->forRelIdx = iRelIdx;
        pForNode->for_code = FOR_RELATION_CODE;
        pForNode->iCtab = 0;
        pForNode->CtabWeightExpr = 0; // RHF Jul 15, 2005
        pForNode->CtabTablogicExpr = 0; // RHF Jul 15, 2005

        pForNode->next_st  = -1;
        pForNode->isUsingExtraInfo = 0; // 3d Extensions, rcl Nov 2004
        pForNode->m_iForRelNodeIdx = 0; // 3d Extensions, rcl Apr 2005
    }
#endif

    int iRelNodeIdx = relanal( iRelIdx );

#ifdef GENCODE // need for-table when compiling with CSpro
    if( Flagcomp )
#endif
    {
        ASSERT( m_ForTableNext >= 0 );
        FORTABLE* pft = &m_ForTable[m_ForTableNext++];

        pft->forVarIdx = iVarIdx ;
        pft->forRelIdx= iRelIdx ;
        pft->forType = _T('R');

    }

    iExprWhere = 0;

    if( pCompileFunction ) // RHF Jul 01, 2006
    {
        iBlock = (this->*pCompileFunction)();
    }

    else
    {
        if( Tkn == TOKWHERE )
        {
            NextToken();
            iExprWhere = exprlog();

            if( GetSyntErr() != 0 )
                return;

            // require do when using where
            IssueErrorOnTokenMismatch(TOKDO, 9);
        }

        if( Tkn == TOKDO )
            NextToken();

        iBlock  = instruc();
    }

#ifdef GENCODE
    if( Flagcomp ) {
        pForNode->forBlock = iBlock;

        pForNode->forWhereExpr = iExprWhere; // RHF Jun 14, 2002
        pForNode->forVarIdx = iVarIdx;
        pForNode->m_iForRelNodeIdx = iRelNodeIdx;  // rcl, Apr 2005
    }
#endif
}

void CEngineCompFunc::CompileForGroup(int iVarIdx, int iGrpIdx, pCompileForInFunction pCompileFunction/* = nullptr*/)
{
    int iBlock = 0;
    int iExprWhere = 0;
    int iSuggestedIndex = -1;

    FORGROUP_NODE* pForNode = NULL;

#ifdef GENCODE
    if( Flagcomp ) {                                    // RHF Aug 04, 2000
        pForNode = NODEPTR_AS( FORGROUP_NODE );
        ADVANCE_NODE( FORGROUP_NODE );

        //memset( pForNode, 0, sizeof(FORGROUP_NODE) );
        pForNode->for_code = FOR_GROUP_CODE;            // RHF Aug 04, 2000
        pForNode->next_st  = -1;                        // RHF Aug 04, 2000
    }                                                   // RHF Aug 04, 2000
#endif
    GROUPT* pGroup = GPT(iGrpIdx);
    int iNumDim = pGroup->GetNumDim();

    if( iNumDim == 0 )
        IssueError( 33004 ); // only multiple groups are allowed

    iSuggestedIndex = (int) pGroup->GetDimType();
#ifdef GENCODE
    if( Flagcomp ) {
        // decrease Prognext to consider GRP_NODE being counted twice
        OC_CreateCompilationSpace(-1 * (int)( sizeof( GRP_NODE ) / sizeof(int) ));
    }
#endif

    if( m_ForTableNext >= FORTABLE_MAX )
        IssueError( 33001 );  // too many for loops

#ifdef GENCODE // need for-table when compiling with CSPro
    if( Flagcomp )
#endif
    {
        ASSERT( m_ForTableNext >= 0 );                  // RHF Aug 14, 2000
        FORTABLE*   pft = &m_ForTable[m_ForTableNext++];

        pft->forVarIdx = iVarIdx;
        pft->forGrpIdx = iGrpIdx;
        pft->forType = _T('G');

        pft->iSuggestedDimension = iSuggestedIndex;
    }

    grpanal( iGrpIdx, ALLOW_DIM_EXPR, 0, FROM_FOR_GROUP ); // RHF Nov 05, 2004

    iExprWhere = 0;

    if( pCompileFunction ) { // RHF Jul 01, 2006
        // Compiler checks for subscript
        bool prevVarSubcheckVal = m_bcvarsubcheck; // 20120326
        m_bcvarsubcheck = true;
        m_icGrpIdx = iGrpIdx;

        iBlock = (this->*pCompileFunction)();

        // Compiler checks for subscript
        m_bcvarsubcheck = prevVarSubcheckVal;//false;
        m_icGrpIdx = 0;
    }

    else
    {
        // Compiler checks for subscript
        bool prevVarSubcheckVal = m_bcvarsubcheck; // 20120326
        m_bcvarsubcheck = true;
        m_icGrpIdx = iGrpIdx;

        if( Tkn == TOKWHERE )
        {
            NextToken();
            iExprWhere = exprlog();

            if( GetSyntErr() != 0 )
                return;

            // require do when using where
            IssueErrorOnTokenMismatch(TOKDO, 9);
        }

        if( Tkn == TOKDO )
            NextToken();

        iBlock = instruc();

        // Compiler checks for subscript
        m_bcvarsubcheck = prevVarSubcheckVal;//false;
        m_icGrpIdx = 0;
    }

#ifdef GENCODE
    if( Flagcomp ) {
        pForNode->forBlock = iBlock;
        pForNode->forWhereExpr = iExprWhere;
        pForNode->forVarIdx = iVarIdx;
        pForNode->iSuggestedDimension = iSuggestedIndex;
    }
#endif
}


int CEngineCompFunc::CompileAskStatement(bool bIsTargetlessSkip/* = false*/)
{
    int iProg = Prognext;
    int iAskUniverse = -1;

#ifdef GENCODE
    ASK_NODE* pAskNode = NODEPTR_AS(ASK_NODE);
    ADVANCE_NODE(ASK_NODE);
#endif

    // ask will only be allowed in a preproc
    // skip will only be allowed in a preproc or in a user-defined function
    if( ( ( ObjInComp == SymbolType::Application ) && !bIsTargetlessSkip ) || ( ProcInComp != PROCTYPE_PRE ) )
        IssueError(5562);

    if( m_pEngineArea->IsLevel(InCompIdx) ) // the statements cannot be in an application or level preproc
        IssueError(5563);

    if( !bIsTargetlessSkip ) // compile the ask universe
    {
        NextToken();

        if( Tkn != TOKIF )
            IssueError(5564);

        NextToken();

        iAskUniverse = exprlog();
    }

#ifdef GENCODE
    pAskNode->ask_code = ASK_CODE;
    pAskNode->next_st = 0;
    pAskNode->ask_universe = iAskUniverse;
#endif

    return iProg;
}


int CEngineCompFunc::CompileSkipStatement( int* code )
{
    int     iSkipNode = Prognext;
    int     v_ind = 0, iSkipPlace = SKIP_DEFAULT;
    int     iSymAt = 0;                                 // victor Mar 26, 01
    int     iAlphaExpr = 0;

    NextTokenWithPreference(SymbolType::Group);

    // a targetless skip
    if( Tkn == TOKSEMICOLON )
        return CompileAskStatement(true);

    // skip case
    else if( Tkn == TOKKWCASE )
        return CompileProgramControl();

#ifdef GENCODE
    SKIP_NODE* pSkipNode = NODEPTR_AS( SKIP_NODE );
    ADVANCE_NODE( SKIP_NODE );
#endif

    *code = TOKTO;

    if( Tkn == TOKTO )
        NextTokenWithPreference(SymbolType::Group);

    if( Tkn == TOKNEXT ) {
        NextToken();

        //BUCEN_DEC2003 Changes Init
        if( Tkn == TOKATOP ) {                          // victor Mar 26, 01
            NextToken();
            VerifyAtTargetSymbol( Tokstindex );
            iSymAt = Tokstindex;

            NextToken();
        }

        else if( ValidEndStatement(Tkn) ) // 20120307 not specifying a variable will skip to the first field in the group
        {
            GROUPT* pGroupT = ( ObjInComp == SymbolType::Block )    ? GetSymbolEngineBlock(InCompIdx).GetGroupT() :
                              ( ObjInComp == SymbolType::Variable ) ? VPT(InCompIdx)->GetParentGPT() :
                                                                      nullptr;

            if( pGroupT == nullptr || pGroupT->GetMaxOccs() < 2 )
                IssueError(5561);

            v_ind = pGroupT->GetItemSymbol(0);
        }

        else if( Tkn == TOKBLOCK )
        {
            v_ind = Tokstindex;
            NextToken();
        }

        else
        {
            if( Tkn != TOKVAR || !VPT(Tokstindex)->IsArray() )
                IssueError( 12 );  // Mult var expected

            // RHF INIC Dec 26, 2003
            if( !VPT(Tokstindex)->IsNumeric() && !VPT(Tokstindex)->IsInAForm() ) {
                VerifyAtTargetSymbol( Tokstindex );
                iSymAt = Tokstindex;
            }
            else {
            // RHF END Dec 26, 2003
                v_ind = Tokstindex;

                VerifyTargetSymbol( v_ind, false ); // ok in BATCH // victor Mar 08, 01
            }

            NextToken();
        }
        //BUCEN_DEC2003 Changes  End

        iSkipPlace = SKIP_TO_NEXT_OCCURRENCE;                    // next occurrence
    }
    // BUCEN_DEC2003 Changes Init
    else if( Tkn == TOKATOP ) {                         // victor Mar 26, 01
            NextToken();
            VerifyAtTargetSymbol( Tokstindex );
            iSymAt = Tokstindex;
            NextToken(); // RHF Jun 13, 2001
        }
    //BUCEN_DEC2003 Changes end
    else if( Tkn == TOKVAR || Tkn == TOKGROUP || Tkn == TOKBLOCK ) {  // now work with Groups // victor Apr 07, 00
        int     iSym = Tokstindex;

        // RHF INIC Dec 26, 2003
        if( Tkn == TOKVAR && !VPT(iSym)->IsNumeric() && !VPT(iSym)->IsInAForm() ) {
            VerifyAtTargetSymbol( iSym );
            iSymAt = Tokstindex;
            NextToken(); // RHF Jun 13, 2001
        }
        // RHF END Dec 26, 2003
        else {
            VerifyTargetSymbol( iSym, false );  // ok in BATCH BUCEN_DEC2003 Changes

            if( Tkn == TOKVAR )
                v_ind = varsanal( VPT(iSym)->GetFmt() );
            else if( Tkn == TOKBLOCK )
                v_ind = blockanal(iSym);
            else
                v_ind = grpanal(iSym);
        }
    }

    else if( Tkn == TOKFUNCTION || Tkn == TOKUSERFUNCTION || Tkn == TOKSCTE || Tkn == TOKWORKSTRING ) // 20120325
        iAlphaExpr = CompileStringExpression();
    else                                                // RHF Feb 14, 2001
        IssueError(54);  // Skip to; or Skip; invalid// RHF Feb 14, 2001

#ifdef GENCODE
    pSkipNode->skip_code   = SKIPTO_CODE;
    pSkipNode->next_st     = 0;                         // what?? victor Mar 08, 01
    pSkipNode->var_ind     = v_ind;
    pSkipNode->var_exprind = iSkipPlace;  // SKIP_DEFAULT or SKIP_TO_NEXT_OCCURRENCE
    pSkipNode->m_iSymAt    = iSymAt;      //BUCEN_DEC2003 Changes
    pSkipNode->m_iAlphaExpr = iAlphaExpr;
#endif

    return( iSkipNode );
}

int CEngineCompFunc::CompileAdvanceStatement( void ) {
    int     iSkipNode = Prognext;
    int     v_ind     = -1;
    int     v_exprind = -1;
    int     iSymAt = 0; // RHF Nov 24, 2003
    int     iAlphaExpr = 0;

#ifdef GENCODE
    SKIP_NODE*  pSkipNode = NODEPTR_AS( SKIP_NODE );
    ADVANCE_NODE( SKIP_NODE );
#endif

    NextToken();
    if( Tkn == TOKTO )
        NextToken();

    //BUCEN_DEC2003 Changes Init
    // RHF INIC Nov 24, 2003
    if( Tkn == TOKATOP ) {
        NextToken();
        VerifyAtTargetSymbol( Tokstindex );
        iSymAt = Tokstindex;
        NextToken(); // RHF Jun 13, 2001
    }

    else if( Tkn == TOKVAR || Tkn == TOKBLOCK || Tkn == TOKGROUP )
    {
        int iSym = Tokstindex;

        // copied from changed made by // RHF Dec 26, 2003
        if( Tkn == TOKVAR && !VPT(Tokstindex)->IsNumeric() && !VPT(Tokstindex)->IsInAForm() ) {
            VerifyAtTargetSymbol( Tokstindex );
            iSymAt = Tokstindex;
            NextToken(); // RHF Jun 13, 2001
        }
        else
        {
            VerifyTargetSymbol( iSym, false );  // ok in BATCH BUCEN_DEC2003 Changes

            if( Tkn == TOKVAR )
                v_ind = varsanal( VPT(iSym)->GetFmt() );
            else if( Tkn == TOKBLOCK )
                v_ind = blockanal(iSym);
            else
                v_ind = grpanal(iSym);

            v_exprind = 0;
        }
    }

    else if( Tkn == TOKFUNCTION || Tkn == TOKUSERFUNCTION || Tkn == TOKSCTE || Tkn == TOKWORKSTRING ) // 20120521
        iAlphaExpr = CompileStringExpression();

    else  {
        // JH 04/11/07 - fixed advance with no target that was broken during transition from 2.6 to 3.0
        // advance with no target - treat it as advance to end, restoring CSPro 2.6 behavior, JH 04/07
        v_ind = -1;
        v_exprind = -1;
    }

    // RHF INIC Nov 24, 2003
#ifdef GENCODE
    if( Flagcomp ) {
        pSkipNode->skip_code   = ADVANCE_CODE;
        pSkipNode->next_st     = 0;                 // what?? victor Mar 08, 01
        pSkipNode->var_ind     = v_ind;
        pSkipNode->var_exprind = v_exprind;
        pSkipNode->m_iSymAt    = iSymAt;                    // RHF Nov 24, 2003 BUCEN_DEC2003 Changes
        pSkipNode->m_iAlphaExpr = iAlphaExpr;
    }
#endif

    return( iSkipNode );

    // RHF END Nov 24, 2003
}

int CEngineCompFunc::CompileReenterStatement( bool bNextTkn ) {
    int     iSkipNode = Prognext;
    int     v_ind = 0;
    int     iSymAt = 0;   // RHF Nov 24, 2003
    int     iAlphaExpr = 0;

#ifdef GENCODE
    SKIP_NODE*  pSkipNode = NODEPTR_AS( SKIP_NODE );
    ADVANCE_NODE( SKIP_NODE );
#endif

    if( bNextTkn )
        NextToken();

    //BUCEN_DEC2003 Changes Init
    // RHF INIC Nov 24, 2003
    if( Tkn == TOKATOP ) {                         // victor Mar 26, 01
        NextToken();
        VerifyAtTargetSymbol( Tokstindex );
        iSymAt = Tokstindex;
        NextToken(); // RHF Jun 13, 2001
    }
    // RHF INIC Dec 26, 2003

    else if( Tkn == TOKVAR || Tkn == TOKBLOCK || Tkn == TOKGROUP )
    {
        int iSym = Tokstindex;

        if( Tkn == TOKVAR && !VPT(iSym)->IsNumeric() && !VPT(iSym)->IsInAForm() ) {
            VerifyAtTargetSymbol( iSym );
            iSymAt = iSym;
            NextToken(); // RHF Jun 13, 2001
        }
        else
        {
            VerifyTargetSymbol( iSym, false );  // ok in BATCH BUCEN_DEC2003 Changes

            if( Tkn == TOKVAR )
            {
                int iVar = iSym;
                VART *pVart = VPT(iVar);

                // RHF INIC Jun 01, 2001
                if( GetSyntErr() == 0 ) {
                    // BMD 11 Mar 2003
                    if( pVart->IsDummyPersistent() )
                        IssueError( 565 ); // Cannot reenter to a persistent field
                }
                // RHF END Jun 01, 2001
                v_ind = varsanal( pVart->GetFmt() );
                // BMD 13 Feb 2007 (make 0 if target name = proc name
                if (InCompIdx == iSym) {
                    if( !NPT(InCompIdx)->IsOneOf(SymbolType::Report, SymbolType::UserFunction) )  { // RHF Mar 28, 2001 Allow reenter without target in functions
                        if( !NPT(InCompIdx)->IsA(SymbolType::Variable) )
                            IssueError( 11 );

                        if( Appl.ApplicationType == ModuleType::Entry ) // RHF Nov 07, 2001
                            if( VPT(InCompIdx)->SYMTfrm <= 0 )
                                IssueError( 554 ); // must have field

                        if( ProcInComp == PROCTYPE_PRE || ProcInComp == PROCTYPE_ONFOCUS )
                        {
                            // 20140402 you should only get a compilation error if reentering a field (from its proc) that does not have multiple occurrences
                            if( pVart->GetMaxOccs() == 1 && pVart->GetOwnerGPT()->GetMaxOccs() == 1 )
                                IssueError( 559 ); // must be only in PROCTYPE_POST
                        }
                    }

#ifdef GENCODE
                    MVAR_NODE * pMVarNode = (MVAR_NODE *)PPT(v_ind); // 20130306 the below v_ind = 0 code below meant that you couldn't reenter different occurrence numbers of the variable you are currently on

                    if( pMVarNode->m_iVarType != MVAR_CODE || pMVarNode->m_iSubindexNumber == 0 )
#endif
                        v_ind = 0;
                }
            }

            else if( Tkn == TOKBLOCK )
                v_ind = blockanal(iSym);

            else
                v_ind = grpanal(iSym);
        }
    }
    // RHF END Dec 26, 2003

    else if( Tkn == TOKFUNCTION || Tkn == TOKUSERFUNCTION || Tkn == TOKSCTE || Tkn == TOKWORKSTRING ) { // 20120521
        iAlphaExpr = CompileStringExpression();
    }

    else { // REENTER without target
           // RHF END Nov 24, 2003
           //BUCEN_DEC2003 Changes End
        if( !NPT(InCompIdx)->IsOneOf(SymbolType::Report, SymbolType::UserFunction) )  { // RHF Mar 28, 2001 Allow reenter without target in functions

            if( NPT(InCompIdx)->IsA(SymbolType::Variable) ) {
                VART* pVarT = VPT(InCompIdx);

                if( Appl.ApplicationType == ModuleType::Entry ) // RHF Nov 07, 2001
                    if( pVarT->SYMTfrm <= 0 )
                        IssueError( 554 ); // must have field

                // BMD 11 Mar 2003
                if( pVarT->IsDummyPersistent() )
                    IssueError( 565 ); // This cannot be a persistent field
            }

            else if( !NPT(InCompIdx)->IsA(SymbolType::Block) ) {
                IssueError( 11 );
            }

            if( ProcInComp == PROCTYPE_PRE || ProcInComp == PROCTYPE_ONFOCUS )
                IssueError( 559 ); // must be only in PROCTYPE_POST
            // RHF END Oct 01, 2000
        }// RHF Mar 28, 2001
    }

#ifdef GENCODE
    pSkipNode->skip_code   = REENTER_CODE;
    pSkipNode->next_st     = 0;                         // what?? victor Mar 08, 01
    pSkipNode->var_ind     = v_ind;
    pSkipNode->var_exprind = 0;
    pSkipNode->m_iSymAt    = iSymAt;                //BUCEN_DEC2003 Changes
    pSkipNode->m_iAlphaExpr = iAlphaExpr;
#endif

    return( iSkipNode );
}
// RHC END Aug 17, 2000


//---------------------------------------------------------------------------
// CompileBreakBy: compile instruction BREAK-BY for a level
//---------------------------------------------------------------------------
int CEngineCompFunc::CompileBreakBy( void ) {
#ifdef GENCODE
    int     iptbreak;
    BREAK_NODE* pbreak;
#endif

    Breaklevel = LvlInComp;

    if( GetHasBreakBy() ) // RHF Apr 16, 2003
    // RHF COM Apr 16, 2003 if( Breaknvars > 0 )
        IssueError( 580 );

    SetHasBreakBy( true ); // RHF Apr 16, 2003

    // RHF INIC Aug 08, 2001
    Symbol* pSymbol=NPT(InCompIdx);

    SymbolType eType = pSymbol->GetType();

    if( !m_pEngineArea->IsLevel(InCompIdx) || Appl.ApplicationType != ModuleType::Batch || LvlInComp == 0 )
        IssueError( 581 );     // allowed only in Levels

    ASSERT( eType == SymbolType::Group );

    GROUPT*     pGroupT=(GROUPT*) pSymbol;
    FLOW*       pFlow=pGroupT->GetFlow();
    bool        bIsPrimaryFlow = false;
    if( pFlow != NULL )
        bIsPrimaryFlow = ( pFlow->GetSubType() == SymbolSubType::Primary );

    if( !bIsPrimaryFlow )
        IssueError( 581 );     // allowed only in Levels
    // RHF END Aug 08, 2001

    Breaknvars = 0;

    std::vector<CBreakById> aBreakId = CompileBreakByList();

    for( size_t i = 0; i < aBreakId.size(); ++i ) {
        Breakvars[i] = aBreakId[i].m_iSymVar;
        Breaklvar[i] = aBreakId[i].m_iLen;
        Breaknvars++;
    }
    Breakvars[Breaknvars] = -1;

    // Check against Break by in Crosstabs
    int iNumCtabBreakVars = (int)m_pEngineArea->m_aCtabBreakId.size();
    if( iNumCtabBreakVars > Breaknvars ) {
        IssueError( 593 );
    }
    else if( iNumCtabBreakVars > 0 ) {
        if( Breaklevel < m_pEngineArea->m_CtabBreakHighLevel )
            IssueError( 594 );

        for( int i = 0; i < iNumCtabBreakVars; i++ ) {
            const CBreakById& rCtabBreakId = m_pEngineArea->m_aCtabBreakId[i];
            const CBreakById& rBreakId = aBreakId[i];

            if( rCtabBreakId.m_iSymVar != rBreakId.m_iSymVar ||
                rCtabBreakId.m_iLen != rBreakId.m_iLen )
                IssueError( 592 );
        }
    }

#ifdef GENCODE
    if( Flagcomp ) {
        pbreak = NODEPTR_AS( BREAK_NODE );
        iptbreak = Prognext;

        ADVANCE_NODE( BREAK_NODE );

        pbreak->st_code = BREAK_CODE;
        pbreak->next_st = -1;

        return( iptbreak );
    }
#endif

    return 0;
}

// RHF INIC Apr 16, 2003
std::vector<CBreakById> CEngineCompFunc::CompileBreakByList()
{
    IssueErrorOnTokenMismatch(TOKBY, 582);

    NextToken();

    std::vector<CBreakById> aBreakId;

    while( Tkn == TOKVAR )
    {
        if( NPT_Ref(Tokstindex).IsA(SymbolType::WorkVariable) )
            IssueError(584); // invalid variable

        if( aBreakId.size() >= MAXBREAKVARS - 1 )
            IssueError(583); // too many vars

        CBreakById& cBreakId = aBreakId.emplace_back();

        cBreakId.m_iSymVar = Tokstindex;
        cBreakId.m_iLen = VPT(cBreakId.m_iSymVar)->GetLength();

#ifdef GENCODE
        if( Flagcomp )
            VPT(cBreakId.m_iSymVar)->SetUsed( true );
#endif
        NextToken();
        if( Tkn == TOKLPAREN ) {
            NextToken();
            if( Tkn != TOKCTE || (int) Tokvalue != Tokvalue ) {
                IssueError(584);// invalid variable
            }
            if( Tokvalue < cBreakId.m_iLen ) {
                IssueError(584);// invalid variable
            } else {
                cBreakId.m_iLen = (int) Tokvalue;
            }
            NextToken();
            if( Tkn != TOKRPAREN )
                IssueError(ERROR_RIGHT_PAREN_EXPECTED);
            NextToken();
        }

        if( Tkn == TOKCOMMA )
            NextToken();
        else
            break;
    }

    // Check len
    int iLen=0;
    for( const CBreakById& rBreakId : aBreakId ) {
        VART* pVarT=VPT(rBreakId.m_iSymVar);
        if( !pVarT->IsNumeric() || pVarT->GetDecimals() > 0 || pVarT->IsArray() )
            IssueError( 584 ); // invalid variable

        iLen += rBreakId.m_iLen;
        if( iLen > TSMAXIDLEN )
            IssueError( 585 ); // too long
    }

    return aBreakId;
}
// RHF END Apr 16, 2003

#ifdef GENCODE
int CEngineArea::setup_vmark( Symbol* ps ) {
    int     set_on = ( ps->GetType() == SymbolType::Variable );

    if( set_on )
        VPT(ps->GetSymbolIndex())->SetUsed(true);

    return( set_on );
}
#endif


//---------------------------------------------------------------------------
// compiler' utility functions
//---------------------------------------------------------------------------

//BUCEN_DEC2003 Changes Init
void CEngineCompFunc::VerifyAtTargetSymbol(int iSymbol)
{
    // VerifyAtTargetSymbol: for '@' targets, checks the symbol is Var/Single/Alpha
    bool bOk = false;

    if( Tkn == TOKWORKSTRING )
    {
        bOk = true;
    }

    else if( IsCurrentTokenVART(*this) )
    {
        VART*   pVarT = VPT(iSymbol);
        bOk = ( !pVarT->IsArray() && !pVarT->IsNumeric() );
    }

    if( !bOk )                          // not a Var/Single/Alpha
        IssueError(88100);

    // RHF INIC Jun 13, 2001
    if( m_pEngineArea->IsLevel(InCompIdx) && ( ProcInComp == PROCTYPE_KILLFOCUS || ProcInComp == PROCTYPE_POST) )
        IssueError(555);                     // void in Post-Level
    // RHF END Jun 13, 2001
}
//BUCEN_DEC2003 Changes  End


int CEngineCompFunc::VerifyTargetSymbol( int iSymb, bool bMustBeInForm )
{
    // VerifyTargetSymbol: only for TOKADVANCE, TOKREENTER and TOKSKIP (to & to next)
    // ... victor Mar 08, 01 {add 2nd arg}
    GROUPT* pGroupT = NULL;
    VART* pVarT = NULL;
    const EngineBlock* engine_block = nullptr;
    int level;

    if( Tkn == TOKVAR && NPT_Ref(Tokstindex).IsA(SymbolType::WorkVariable) )
        IssueError( 13 );

    if( m_pEngineArea->IsLevel(InCompIdx) && ( ProcInComp == PROCTYPE_KILLFOCUS || ProcInComp == PROCTYPE_POST) )
        IssueError( 555 );                     // void in Post-Level

    if( Tkn == TOKVAR ) {
        pVarT = VPT(iSymb);
        level = pVarT->GetLevel();
    }
    else if( Tkn == TOKBLOCK ) {
        engine_block = &GetSymbolEngineBlock(iSymb);
        level = engine_block->GetGroupT()->GetLevel();
    }
    else {
        pGroupT = GPT(iSymb);
        level = pGroupT->GetLevel();
    }

    if( ObjInComp != SymbolType::Application && level != LvlInComp ) {
        IssueError( 553 );    // must be at same level
    }

    if( bMustBeInForm ) {                               // victor Mar 08, 01
        if( Tkn == TOKVAR && pVarT->SYMTfrm <= 0 )
            IssueError( 554 );
    }

    // RHF INIC Aug 16, 2000
    if( ObjInComp != SymbolType::Application ) {
        FLOW*   pFlow[2] = { NULL, NULL };

        pFlow[0] = ( pVarT != nullptr )        ? pVarT->GetOwnerFlow() :
                   ( engine_block != nullptr ) ? engine_block->GetGroupT()->GetFlow() :
                                                 pGroupT->GetFlow();

        if( InCompIdx > 0 ) {
            Symbol* pSymb = NPT(InCompIdx);

            if( pSymb->IsA(SymbolType::Variable) ) {
                pFlow[1] = ((VART*)pSymb)->GetOwnerFlow();
            }
            else if( pSymb->IsA(SymbolType::Block) ) {
                pFlow[1] = assert_cast<const EngineBlock*>(pSymb)->GetGroupT()->GetFlow();
            }
            else if( pSymb->IsA(SymbolType::Group) ) {
                pFlow[1] = ((GROUPT*)pSymb)->GetFlow();
            }
            else {
                pFlow[1] = NULL;
            }

            if( pFlow[0] != pFlow[1] )
                IssueError( 556 ); // in other Flow
        }

    }
    // RHF END Aug 16, 2000

    return 0;
}


int CEngineCompFunc::GetRelationSymbol(int symbol_index)
{
    Symbol* symbol = NPT(symbol_index);
    ASSERT(symbol->IsOneOf(SymbolType::Section, SymbolType::Variable));

    // look for the relation of this record or item (named _REL_symName)
    CString relation_name;
    relation_name.Format(_T("_REL_%s"), symbol->GetName().c_str());

    int relation_symbol_index = m_pEngineArea->SymbolTableSearch(relation_name, { SymbolType::Relation });

    // if the relation doesn't exist, create one
    if( relation_symbol_index == 0 )
    {
        auto pRelT = std::make_unique<RELT>(CS2WS(relation_name), GetSymbolTable());
        pRelT->AddBaseSymbol(symbol, MakeRelationWorkVar());
        relation_symbol_index = m_engineData->AddSymbol(std::move(pRelT));
    }

    return relation_symbol_index;
}


// RHF INIC Dec 09, 2003    BUCEN_DEC2003 Changes
int CEngineCompFunc::CompileMoveStatement(bool bFromSelectStatement/* = false*/) {
    int     iMoveNode = Prognext;
    int     v_ind = 0;
    int     iMoveType = 0;
    int     iSymAt = 0;
    int     iAlphaExpr = 0;

#ifdef GENCODE
    SKIP_NODE* pMoveNode = NODEPTR_AS( SKIP_NODE );
    ADVANCE_NODE( SKIP_NODE );
#endif

    if( !bFromSelectStatement )
    {
        NextTokenWithPreference(SymbolType::Group);

        if( Tkn == TOKTO )
            NextTokenWithPreference(SymbolType::Group);
    }

    if( Tkn == TOKATOP ) {
        NextToken();
        VerifyAtTargetSymbol( Tokstindex );
        iSymAt = Tokstindex;
        NextToken(); // RHF Jun 13, 2001
    }
        // RHF INIC Dec 26, 2003
    else if( Tkn == TOKVAR && !VPT(Tokstindex)->IsNumeric() && !VPT(Tokstindex)->IsInAForm() ) {
        VerifyAtTargetSymbol( Tokstindex );
        iSymAt = Tokstindex;
        NextToken(); // RHF Jun 13, 2001
    }
    // RHF END Dec 26, 2003

    else if( Tkn == TOKVAR || Tkn == TOKBLOCK || Tkn == TOKGROUP )
    {
        int iSym = Tokstindex;

        VerifyTargetSymbol( iSym, false );  // ok in BATCH

        if( Tkn == TOKVAR )
            v_ind = varsanal( VPT(iSym)->GetFmt() );
        else if( Tkn == TOKBLOCK )
            v_ind = blockanal(iSym);
        else
            v_ind = grpanal(iSym);
    }

    else if( Tkn == TOKFUNCTION || Tkn == TOKUSERFUNCTION || Tkn == TOKSCTE || Tkn == TOKWORKSTRING ) // 20120521
    {
        iAlphaExpr = CompileStringExpression();
    }

    else // RHF Feb 14, 2001
    {
        IssueError(54);  // Skip to; or Skip; invalid// RHF Feb 14, 2001
    }


    if( !bFromSelectStatement )
    {
        if( Tkn == TOKSKIP ) {
            iMoveType = 1;
            NextToken();
        }
        else if( Tkn == TOKADVANCE ) {
            iMoveType = 2;
            NextToken();
        }
    }

#ifdef GENCODE
    pMoveNode->skip_code   = MOVETO_CODE;
    pMoveNode->next_st     = 0;                // what?? victor Mar 08, 01
    pMoveNode->var_ind     = v_ind;
    pMoveNode->var_exprind = iMoveType;        // 1: SKIP, 2: ADVANCE, 0:Default(SKIP)
    pMoveNode->m_iSymAt    = iSymAt;
    pMoveNode->m_iAlphaExpr= iAlphaExpr;
#endif

    return( iMoveNode );
}
// RHF END Dec 09, 2003
