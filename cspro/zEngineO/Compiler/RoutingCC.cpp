#include "stdafx.h"
#include "IncludesCC.h"
#include "EnginePreprocessor.h"
#include <zLogicO/KeywordTable.h>
#include <zLogicO/LocalSymbolStack.h>


bool LogicCompiler::IsValidStatementStartToken(const TokenCode token_code) noexcept
{
    switch( token_code )
    {
        case TokenCode::TOKADVANCE:
        case TokenCode::TOKALPHA:
        case TokenCode::TOKARRAY:
        case TokenCode::TOKASK:
        case TokenCode::TOKAUDIO:
        case TokenCode::TOKBREAK:
        case TokenCode::TOKCONFIG:
        case TokenCode::TOKCROSSTAB:
        case TokenCode::TOKDECLARE:
        case TokenCode::TOKDICT:
        case TokenCode::TOKDO:
        case TokenCode::TOKDOCUMENT:
        case TokenCode::TOKENDCASE:
        case TokenCode::TOKENDLEVL:
        case TokenCode::TOKENDSECT:
        case TokenCode::TOKENTER:
        case TokenCode::TOKEXIT:
        case TokenCode::TOKEXPORT:
        case TokenCode::TOKFOR:
        case TokenCode::TOKFORCASE:
        case TokenCode::TOKFREQ:
        case TokenCode::TOKFUNCTION:
        case TokenCode::TOKGEOMETRY:
        case TokenCode::TOKHASH:
        case TokenCode::TOKHASHMAP:
        case TokenCode::TOKIF:
        case TokenCode::TOKIMAGE:
        case TokenCode::TOKKWARRAY:
        case TokenCode::TOKKWAUDIO:
        case TokenCode::TOKKWCASE:
        case TokenCode::TOKKWCTAB:
        case TokenCode::TOKKWDATASOURCE:
        case TokenCode::TOKKWDOCUMENT:
        case TokenCode::TOKKWFILE:
        case TokenCode::TOKKWFREQ:
        case TokenCode::TOKKWGEOMETRY:
        case TokenCode::TOKKWHASHMAP:
        case TokenCode::TOKKWIMAGE:
        case TokenCode::TOKKWLIST:
        case TokenCode::TOKKWMAP:
        case TokenCode::TOKKWPFF:
        case TokenCode::TOKKWSTRINGWRITER:
        case TokenCode::TOKKWSYSTEMAPP:
        case TokenCode::TOKKWVALUESET:
        case TokenCode::TOKKWVIDEO:
        case TokenCode::TOKLIST:
        case TokenCode::TOKMOVE:
        case TokenCode::TOKNEXT:
        case TokenCode::TOKNOINPUT:
        case TokenCode::TOKNUMERIC:
        case TokenCode::TOKPERSISTENT:
        case TokenCode::TOKPFF:
        case TokenCode::TOKRECODE:
        case TokenCode::TOKREENTER:
        case TokenCode::TOKSET:
        case TokenCode::TOKSKIP:
        case TokenCode::TOKSTOP:
        case TokenCode::TOKSTRING:
        case TokenCode::TOKUNIVERSE:
        case TokenCode::TOKUSERFUNCTION:
        case TokenCode::TOKVALUESET:
        case TokenCode::TOKVAR:
        case TokenCode::TOKVIDEO:
        case TokenCode::TOKWHEN:
        case TokenCode::TOKWHILE:
        case TokenCode::TOKWORKSTRING:
            return true;
    }

    return false;
}


bool LogicCompiler::IsValidStatementEndToken(const TokenCode token_code) noexcept
{
    // returns true if the token is a valid end-of-statement
    switch( token_code )
    {
        // originally only a semicolon was allowed...
        case TokenCode::TOKSEMICOLON:

        // ... but more tokens were accepted on Feb 22, 2000
        case TokenCode::TOKELSE:
        case TokenCode::TOKELSEIF:
        case TokenCode::TOKEND:
        case TokenCode::TOKENDDO:
        case TokenCode::TOKENDIF:
        case TokenCode::TOKENDLEVL:
        case TokenCode::TOKENDRECODE:
        case TokenCode::TOKENDSECT:

        // uncomment the line below and the user will be allowed to
        // avoid the semicolon in the very last statement of a procedure
        case TokenCode::TOKEOP:
            return true;
    }

    return false;
}


int LogicCompiler::CompileStatements(const bool create_new_local_symbol_stack/* = true*/, const bool allow_multiple_statements/* = true*/)
{
    // when compiling a user-defined function or a PROC, create_new_local_symbol_stack will be
    // false because there is already a local symbol stack created at that level
    std::optional<Logic::LocalSymbolStack> local_symbol_stack;

    if( create_new_local_symbol_stack )
        local_symbol_stack.emplace(m_symbolTable.CreateLocalSymbolStack());

    int first_statement_program_index = -1;
    int previous_statement_program_index = -1;

    auto link_statement = [&](const int program_index)
    {
        if( program_index == -1 )
            return;

        // if this is the first statement, set it as such
        if( first_statement_program_index == -1 )
        {
            first_statement_program_index = program_index;
        }

        // otherwise link the previous statement to this one
        else
        {
            ASSERT(previous_statement_program_index != -1);
            GetNode<Nodes::Statement>(previous_statement_program_index).next_st = program_index;
        }

        previous_statement_program_index = program_index;
    };

#define USE_OLD_ROUTINE
#ifdef USE_OLD_ROUTINE // the implementation in engine/Instruc.cpp
    link_statement(instruc_COMPILER_DLL_TODO(allow_multiple_statements));

#else
#ifdef OLD_ROUTINE_REFERENCE_COMPILER_DLL_TODO
    int iptblock = Prognext;
    bool bIsSkipStatement = false;
    int code;
    int v_ind = -1;
    int sind;
    int aux;

    Nodes::Statement* previous_instruc_st = nullptr;
    Nodes::Statement* prev_st = NULL;
#endif

    int c = TOKSEMICOLON;

    while( c == TOKSEMICOLON && Tkn != TOKEOP )
    {
        try
        {
#ifdef OLD_ROUTINE_REFERENCE_COMPILER_DLL_TODO
            // check for cpt, move for function declaration
            if( Tkn == TOKEND && ObjInComp == SymbolType::Application )
                break;

            bIsSkipStatement = false;
#endif

            // skip past any semicolons
            while( Tkn == TokenCode::TOKSEMICOLON )
                NextToken();

            if( !IsValidStatementStartToken(Tkn) )
                break;

#ifdef OLD_ROUTINE_REFERENCE_COMPILER_DLL_TODO
            if( ObjInComp == SymbolType::Application && Tkn == TOKNOINPUT )
                IssueError( 562 ); // invalid inside a function

            int saved_prog_next = Prognext;
#endif

            // when tracing logic, create the trace node for this statement
            if( IsTracingLogic() )
                link_statement(CreateTraceStatement());

            // preprocesor handling
            if( Tkn == TokenCode::TOKHASH )
            {
                m_preprocessor->ProcessLineDuringCompilation();

                // ProcessLineDuringCompilation will process all tokens on the line but not move to the next token, so
                // we do that here and then pretend a semicolon was read to satisfy the loop condition
                NextToken();
                c = TOKSEMICOLON;

                continue;
            }

#ifdef OLD_ROUTINE_REFERENCE_COMPILER_DLL_TODO
#ifdef GENCODE
            prev_st = NODEPTR_AS(Nodes::Statement);
#endif

            int last_added_node_address = -1;
            int compilation_address = -1;
#endif

            // process the token
            int program_index = -1;

            switch( Tkn )
            {
                // --------------------------------------------------------------------------
                // symbol creation
                // --------------------------------------------------------------------------

                case TokenCode::TOKCONFIG:
                case TokenCode::TOKDECLARE:
                case TokenCode::TOKPERSISTENT:
                    program_index = CompileSymbolWithModifiers();
                    break;

                case TokenCode::TOKALPHA:
                case TokenCode::TOKKWARRAY:
                case TokenCode::TOKKWAUDIO:
                case TokenCode::TOKKWCASE:
                case TokenCode::TOKKWDATASOURCE:
                case TokenCode::TOKKWDOCUMENT:
                case TokenCode::TOKKWFILE:
                case TokenCode::TOKKWFREQ:
                case TokenCode::TOKKWGEOMETRY:
                case TokenCode::TOKKWHASHMAP:
                case TokenCode::TOKKWIMAGE:
                case TokenCode::TOKKWLIST:
                case TokenCode::TOKKWMAP:
                case TokenCode::TOKKWPFF:
                case TokenCode::TOKKWSTRINGWRITER:
                case TokenCode::TOKKWSYSTEMAPP:
                case TokenCode::TOKKWVALUESET:
                case TokenCode::TOKKWVIDEO:
                case TokenCode::TOKNUMERIC:
                case TokenCode::TOKSTRING:
                    program_index = CompileSymbolRouter();
                    break;


                // --------------------------------------------------------------------------
                // symbol assignment
                // --------------------------------------------------------------------------

                //  dictionary items + numeric
                case TokenCode::TOKVAR:
                {
                    const Symbol& symbol = NPT_Ref(Tokstindex);

                    if( symbol.IsA(SymbolType::WorkVariable) )
                    {
                        program_index = CompileNumericComputeInstruction();
                    }

                    else
                    {
                        // COMPILER_DLL_TODO remove message
                        IssueError(MGF::OpenMessage_32001, "The compiler available at runtime does not support assignments to dictionary items");
                    }

#ifdef OLD_ROUTINE_REFERENCE_COMPILER_DLL_TODO
                    else if( assert_cast<const VART&>(symbol).IsNumeric() )
                    {
                        CompileComputeInstruction();
                        if( GetSyntErr() != 0 )
                            IssueError(GetSyntErr());
                        if( Tkn == TOKVAR || Tkn == TOKCTE || Tkn == TOKLPAREN )
                            IssueError( 2 );
                    }

                    else
                    {
                        CompileStringComputeInstruction();
                        if( GetSyntErr() != 0 ) // victor Sep 20, 00
                            return 0;
                    }
#endif
                    break;
                }

                case TokenCode::TOKARRAY:
                {
                    program_index = CompileLogicArrayComputeInstruction();
                    break;
                }

                case TokenCode::TOKAUDIO:
                {
                    program_index = CompileLogicAudioComputeInstruction();
                    break;
                }

                case TokenCode::TOKDICT:
                {
                    program_index = CompileEngineCaseComputeInstruction();
                    break;
                }

                case TokenCode::TOKDOCUMENT:
                {
                    program_index = CompileLogicDocumentComputeInstruction();
                    break;
                }

                case TokenCode::TOKFREQ:
                {
                    program_index = CompileNamedFrequencyComputeInstruction();
                    break;
                }

                case TokenCode::TOKGEOMETRY:
                {
                    program_index = CompileLogicGeometryComputeInstruction();
                    break;
                }

                case TokenCode::TOKHASHMAP:
                {
                    program_index = CompileLogicHashMapComputeInstruction();
                    break;
                }

                case TokenCode::TOKIMAGE:
                {
                    program_index = CompileLogicImageComputeInstruction();
                    break;
                }

                case TokenCode::TOKLIST:
                {
                    program_index = CompileLogicListComputeInstruction();
                    break;
                }

                case TokenCode::TOKPFF:
                {
                    program_index = program_index = CompileLogicPffComputeInstruction();
                    break;
                }

                case TokenCode::TOKVALUESET:
                {
                    program_index = CompileDynamicValueSetComputeInstruction();
                    break;
                }

                case TokenCode::TOKVIDEO:
                {
                    program_index = CompileLogicVideoComputeInstruction();
                    break;
                }

                case TokenCode::TOKWORKSTRING:
                {
                    program_index = CompileStringComputeInstruction();
                    break;
                }


                // --------------------------------------------------------------------------
                // functions
                // --------------------------------------------------------------------------

                case TokenCode::TOKUSERFUNCTION:
                {
                    // the user-defined function may be receiving its return value (e.g.: MyFunc = 5;)
                    if( Tokstindex == get_COMPILER_DLL_TODO_InCompIdx() && !IsNextToken(TOKLPAREN) )
                    {
                        program_index = CompileUserFunctionComputeInstruction();
                        break;
                    }

                    // otherwise the user-defined function is being called, and the fallthrough code for functions applies
                    [[fallthrough]];
                }

                case TokenCode::TOKFUNCTION:
                {
                    program_index = CompileFunctionCall();
                    break;
                }


                // --------------------------------------------------------------------------
                // "switch" statements
                // --------------------------------------------------------------------------

                case TokenCode::TOKRECODE:
                    program_index = CompileRecode();
                    break;

                case TokenCode::TOKWHEN:
                    NextToken();
                    program_index = CompileWhen();
                    break;


                // --------------------------------------------------------------------------
                // "control flow" statements
                // --------------------------------------------------------------------------

                case TokenCode::TOKIF:
                    program_index = CompileIfStatement();
                    break;

                case TokenCode::TOKWHILE:
                    program_index = CompileWhileLoop();
                    break;

                case TokenCode::TOKDO:
                    program_index = CompileDoLoop();
                    break;

                case TokenCode::TOKNEXT:
                    program_index = CompileNextOrBreakInLoop();
                    break;

                case TokenCode::TOKBREAK:
                {
                    if( NextKeywordIf(TOKBY) )
                    {
                        IssueError(MGF::OpenMessage_32001, "The compiler available at runtime does not support: break by");
#ifdef OLD_ROUTINE_REFERENCE_COMPILER_DLL_TODO
                        CompileBreakBy();
                        if( GetSyntErr() != 0 ) // victor Sep 20, 00
                            return 0;
#endif
                    }

                    else
                    {
                        program_index = CompileNextOrBreakInLoop();
                    }

                    break;
                }

                case TokenCode::TOKFOR:
                {
                    std::optional<SymbolType> next_token_symbol_type = GetNextTokenSymbolType();

                    if( next_token_symbol_type == SymbolType::Dictionary ||
                        next_token_symbol_type == SymbolType::Pre80Dictionary )
                    {
                        program_index = CompileForDictionaryLoop(TokenCode::TOKFOR);
                    }

                    else
                    {
                        // use preference so, in a normal for loop, external dictionary
                        // records are prioritized over groups
                        NextTokenWithPreference(SymbolType::Section);

                        IssueError(MGF::OpenMessage_32001, "The compiler available at runtime does not support: for");
#ifdef OLD_ROUTINE_REFERENCE_COMPILER_DLL_TODO
                        CompileForStatement();
                        if( GetSyntErr() != 0 ) // victor Sep 20, 00
                            return 0;
#endif
                    }

                    break;
                }

                case TokenCode::TOKFORCASE:
                {
                    program_index = CompileForDictionaryLoop(TokenCode::TOKFORCASE);
                    break;
                }


                // --------------------------------------------------------------------------
                // program control
                // --------------------------------------------------------------------------
#ifdef OLD_ROUTINE_REFERENCE_COMPILER_DLL_TODO
                case TokenCode::TOKASK:
                    CompileAskStatement();
                    if( GetSyntErr() != 0 )
                        return 0;
                    break;

                case TokenCode::TOKADVANCE:
                    CompileAdvanceStatement();
                    if( GetSyntErr() != 0 ) // victor Sep 20, 00
                        return 0;
                    break;

                    // RHF INIC Dec 09, 2003 BUCEN_DEC2003 Changes
                case TokenCode::TOKMOVE:
                    code = Tkn;
                    CompileMoveStatement();
                    if( GetSyntErr() != 0 )
                        return 0;
                    break;
                    // RHF END Dec 09, 2003 BUCEN_DEC2003 Changes

                case TokenCode::TOKSKIP:
                    CompileSkipStatement( &code );
                    if( GetSyntErr() != 0 ) // victor Sep 20, 00
                        return 0;
                    break;

                case TokenCode::TOKREENTER:
                    CompileReenterStatement();
                    if( GetSyntErr() != 0 ) // victor Sep 20, 00
                        return 0;
                    break;

                case TokenCode::TOKENDCASE:
                case TokenCode::TOKUNIVERSE:
                case TokenCode::TOKEXIT:
                    compilation_address = CompileProgramControl();
                    break;

                case TokenCode::TOKENTER:
                    compilation_address = CompileEnter();
                    break;

                // --- selected-tokens <begin>
                //
                //   3. TOKSTOP
                //   6. TOKNOINPUT
                //   7. TOKENDSECT
                //   8. TOKENDLEVL
                // what?? TOKSKIP, TOKREENTER, TOKADVANCE   // what?? victor Mar 08, 01
                case TokenCode::TOKSTOP:
                case TokenCode::TOKENDLEVL:
                    // RHF INIC Dec 22, 2003
                    if( NPT(InCompIdx)->IsA(SymbolType::Variable) && VPT(InCompIdx)->GetSubType() != SymbolSubType::Input
                            ||
                        NPT(InCompIdx)->IsA(SymbolType::Group) && GPT(InCompIdx)->GetSubType() != SymbolSubType::Primary) {
                        SetSyntErr(9192);
                        return 0;
                    }
                    // RHF END Dec 22, 2003
                    [[fallthrough]];
                case TokenCode::TOKENDSECT:
                case TokenCode::TOKNOINPUT:
                {
                    code = Tkn;

                    if( SO::EqualsNoCase(Tokstr, _T("ENDSECT")) )
                        IssueWarning(Logic::ParserMessage::Type::DeprecationMinor, 95001, "ENDSECT", "ENDGROUP");

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
                    case TokenCode::TOKSTOP:
                    {
                        int stop_expr = -1;

                        if( m_Flagcomp )
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

                        if( m_Flagcomp )
                        {
                            STOP_NODE* stop_node = (STOP_NODE*)prev_st;
                            stop_node->stop_expr = stop_expr;
                        }

                        break;
                    }

                    //~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~
                    //   Case 6. TOKNOINPUT
                    //~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~
                    case TokenCode::TOKNOINPUT:
                        if( ObjInComp != SymbolType::Variable || ( VPT(InCompIdx)->SYMTfrm <= 0 && Appl.ApplicationType == ModuleType::Entry ) ) // RHF Nov 07, 2001
                            IssueError( 557 );  // not in a field

                        if( !( GetCompilationProcType() == ProcType::PreProc || GetCompilationProcType() == ProcType::OnFocus ) )
                            IssueError( 558 ); // must be at Pre

                        NextToken();
                        break;

                    //~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~
                    //   Case 7. TOKENDSECT
                    //~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~
                    case TokenCode::TOKENDSECT:
                        // RHF INIC Dec 04, 2003 Now endgroup can be called from a function
                           //BUCEN_DEC2003 Changes
                        if( ObjInComp == SymbolType::Group && ( GetCompilationProcType() == ProcType::KillFocus || GetCompilationProcType() == ProcType::PostProc ) ||
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
                    case TokenCode::TOKENDLEVL:
                        if( Appl.ApplicationType == ModuleType::Batch ) {
                            issaerror( MessageType::Warning, 88150 ); // RHF Nov 08, 2001 Better here instead of in BatchExEndLevel method
                        }

                        if( m_pEngineArea->IsLevel( InCompIdx ) && ( GetCompilationProcType() == ProcType::KillFocus || GetCompilationProcType() == ProcType::PostProc ) && LvlInComp == 0 )// RHF Jun 05, 2001
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
                    if( m_Flagcomp ) {
                        if( code != TOKTO && code != TOKREENTER && code != TOKADVANCE && code != TOKMOVE && code != TOKSTOP ) {
                            ADVANCE_NODE(ST_NODE);
                        }

                        switch( code ) {
                            case TokenCode::TOKSKIP:                   // what?? victor Mar 08, 01
                                code = SKIPCASE_CODE;
                                break;
                                // RHF INIC Dec 09, 2003 BUCEN_DEC2003 Changes
                            case TokenCode::TOKMOVE:
                                code = MOVETO_CODE;
                                break;
                                // RHF END Dec 09, 2003  BUCEN_DEC2003 Changes
                            case TokenCode::TOKTO:                     // what?? victor Mar 08, 01
                                code = SKIPTO_CODE;
                                break;
                            case TokenCode::TOKREENTER:                // what?? victor Mar 08, 01
                                code = REENTER_CODE;
                                break;
                            case TokenCode::TOKADVANCE:                // what?? victor Mar 08, 01
                                code = ADVANCE_CODE;
                                break;
                            case TokenCode::TOKNOINPUT:
                                code = NOINPUT_CODE;
                                break;
                            case TokenCode::TOKSTOP:
                                code = STOP_CODE;
                                break;
                            case TokenCode::TOKENDLEVL:
                                code = ENDLEVL_CODE;
                                break;
                            case TokenCode::TOKENDSECT:
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


                // --------------------------------------------------------------------------
                // crosstabs
                // --------------------------------------------------------------------------

                case TokenCode::TOKKWCTAB:             // CROSSTAB in dict' proc
                {
                    compctab( 1, CTableDef::Ctab_Crosstab );
                    if( GetSyntErr() != 0 ) // victor Sep 20, 00
                        return 0;
                    break;
                }

                case TokenCode::TOKCROSSTAB:
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


                // --------------------------------------------------------------------------
                // miscellaneous
                // --------------------------------------------------------------------------

                case TokenCode::TOKEXPORT:
                    compexport();
                    if( GetSyntErr() != 0 ) // victor Sep 20, 00
                        return 0;
                    break;

                case TokenCode::TOKSET:
                {
                    std::optional<int> compilation_node_index = ci_set();

                    if( GetSyntErr() != 0 )
                        return 0;              // victor Sep 20, 00

                    if( !compilation_node_index.has_value() )
                        continue;

                    break;
                }
#endif
                default:
                    IssueError(MGF::OpenMessage_32001, FormatText(
                        "The compiler available at runtime does not support: '%s'",
                        Logic::KeywordTable::GetKeywordName(Tkn)
                    ).c_str());
#ifdef OLD_ROUTINE_REFERENCE_COMPILER_DLL_TODO
                    ASSERT(0);              // should not happen!
                    break;
#endif
            }

            // Final check at the end of every instruction
            //     Original Strict rule:
            //         Every statement must end with a ;
            //     Current rule (Apr 03, 2000):
            //         ";" may be omitted before an end (endif, enddo, etc.)
            //         and some other keywords (see IsValidStatementEndToken method)
            if( !IsValidStatementEndToken(Tkn) )
                IssueError(MGF::expecting_semicolon_2);

            link_statement(program_index);

#ifdef OLD_ROUTINE_REFERENCE_COMPILER_DLL_TODO
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
            if( m_Flagcomp ) {
                if( bIsSkipStatement && Tkn != TOKNOINPUT )
                    prev_st->next_st = -1;
                else
                    prev_st->next_st = Prognext;
            }

            previous_instruc_st = prev_st;
#endif
#endif
            c = Tkn;
        }

        catch( const Logic::ParserError& )
        {
            // if there is an error, skip until the next token
            if( Tkn != TokenCode::TOKSEMICOLON )
            {
                SkipBasicTokensUntil(TokenCode::TOKSEMICOLON);
                NextToken();
            }
        }

        if( !allow_multiple_statements )
        {
            if( Tkn == TokenCode::TOKSEMICOLON )
                NextToken();

            break;
        }
    }
#endif // !USE_OLD_ROUTINE

    // terminate the final statement added
#ifdef USE_OLD_ROUTINE
    // this may assert while using the Designer due to code not running due to the GENCODE preprocessor definition
    ASSERT(GetNode<Nodes::Statement>(previous_statement_program_index).next_st == -1 ||
           GetNode<Nodes::Statement>(previous_statement_program_index).next_st > previous_statement_program_index);
#else
    if( previous_statement_program_index != -1 )
    {
        Nodes::Statement& statement_node = GetNode<Nodes::Statement>(previous_statement_program_index);
        ASSERT(statement_node.next_st == -1 || statement_node.next_st == 0);
        statement_node.next_st = -1;
    }
#endif

    if( local_symbol_stack.has_value() )
        first_statement_program_index = WrapNodeAroundScopeChange(*local_symbol_stack, first_statement_program_index);

    return first_statement_program_index;
}


int LogicCompiler::RouteFunctionCall()
{
    // analyze user-defined functions...
    if( Tkn == TokenCode::TOKUSERFUNCTION )
        return CompileUserFunctionCall();

    // ...or built-in functions
    ASSERT(CurrentToken.function_details != nullptr);

    using CompilationFunction = int(LogicCompiler::*)();
    constexpr CompilationFunction implement_COMPILER_DLL_TODO = nullptr; // implement in zEngineO

    static const std::map<Logic::FunctionCompilationType, CompilationFunction> CompilationFunctionMap =
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
        { Logic::FunctionCompilationType::Sync,                     &LogicCompiler::CompileSyncFunctions },
        { Logic::FunctionCompilationType::Trace,                    &LogicCompiler::CompileTraceFunction },
        { Logic::FunctionCompilationType::Userbar,                  &LogicCompiler::CompileUserbarFunction },
        { Logic::FunctionCompilationType::ValueSetRelated,          &LogicCompiler::CompileValueSetRelatedFunctions },

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
        { Logic::FunctionCompilationType::JS,                       &LogicCompiler::CompileJavaScriptFunctions },
        { Logic::FunctionCompilationType::List,                     &LogicCompiler::CompileLogicListFunctions },
        { Logic::FunctionCompilationType::Map,                      &LogicCompiler::CompileLogicMapFunctions },
        { Logic::FunctionCompilationType::Message,                  &LogicCompiler::CompileMessageFunctions },
        { Logic::FunctionCompilationType::NamedFrequency,           &LogicCompiler::CompileNamedFrequencyFunctions },
        { Logic::FunctionCompilationType::Path,                     &LogicCompiler::CompilePathFunctions },
        { Logic::FunctionCompilationType::Pff,                      &LogicCompiler::CompileLogicPffFunctions},
        { Logic::FunctionCompilationType::Report,                   &LogicCompiler::CompileReportFunctions },
        { Logic::FunctionCompilationType::StringWriter,             &LogicCompiler::CompileStringWriterFunctions },
        { Logic::FunctionCompilationType::Symbol,                   &LogicCompiler::CompileSymbolFunctions },
        { Logic::FunctionCompilationType::SystemApp,                &LogicCompiler::CompileSystemAppFunctions },
        { Logic::FunctionCompilationType::TextTemplate,             &LogicCompiler::CompileTextTemplateFunctions },
        { Logic::FunctionCompilationType::UserInterface,            &LogicCompiler::CompileUserInterfaceFunctions },
        { Logic::FunctionCompilationType::ValueSet,                 &LogicCompiler::CompileValueSetFunctions },
        { Logic::FunctionCompilationType::Video,                    &LogicCompiler::CompileLogicVideoFunctions },

        // other
        { Logic::FunctionCompilationType::FN2,                      implement_COMPILER_DLL_TODO }, // cfun_compile_count },
        { Logic::FunctionCompilationType::FN3,                      implement_COMPILER_DLL_TODO }, // cfun_compile_sum
        { Logic::FunctionCompilationType::FN4,                      implement_COMPILER_DLL_TODO }, // cfun_fn4
        { Logic::FunctionCompilationType::FN6,                      implement_COMPILER_DLL_TODO }, // cfun_fn6
        { Logic::FunctionCompilationType::FN8,                      implement_COMPILER_DLL_TODO }, // cfun_fn8
        { Logic::FunctionCompilationType::FNS,                      implement_COMPILER_DLL_TODO }, // cfun_fns
        { Logic::FunctionCompilationType::FNC,                      implement_COMPILER_DLL_TODO }, // cfun_fnc
        { Logic::FunctionCompilationType::FNB,                      implement_COMPILER_DLL_TODO }, // cfun_fnb
        { Logic::FunctionCompilationType::FNTC,                     implement_COMPILER_DLL_TODO }, // cfun_fntc
        { Logic::FunctionCompilationType::FNH,                      implement_COMPILER_DLL_TODO }, // cfun_fnh
        { Logic::FunctionCompilationType::FNG,                      implement_COMPILER_DLL_TODO }, // cfun_fng
        { Logic::FunctionCompilationType::FNGR,                     implement_COMPILER_DLL_TODO }, // cfun_fngr
        { Logic::FunctionCompilationType::FNID,                     implement_COMPILER_DLL_TODO }, // cfun_fnins
        { Logic::FunctionCompilationType::FNSRT,                    implement_COMPILER_DLL_TODO }, // cfun_fnsrt
        { Logic::FunctionCompilationType::FNMAXOCC,                 implement_COMPILER_DLL_TODO }, // cfun_fnmaxocc
        { Logic::FunctionCompilationType::FNEXECSYSTEM,             implement_COMPILER_DLL_TODO }, // cfun_fnexecsystem
        { Logic::FunctionCompilationType::FNSHOW,                   implement_COMPILER_DLL_TODO }, // cfun_fnshow
        { Logic::FunctionCompilationType::FNITEMLIST,               implement_COMPILER_DLL_TODO }, // cfun_fnitemlist
        { Logic::FunctionCompilationType::FNDECK,                   implement_COMPILER_DLL_TODO }, // cfun_fndeck
        { Logic::FunctionCompilationType::FNCAPTURETYPE,            implement_COMPILER_DLL_TODO }, // cfun_fncapturetype
        { Logic::FunctionCompilationType::FNOCCS,                   implement_COMPILER_DLL_TODO }, // cfun_fnoccs
        { Logic::FunctionCompilationType::FNNOTE,                   implement_COMPILER_DLL_TODO }, // cfun_fnnote
        { Logic::FunctionCompilationType::FNSTRPARM,                implement_COMPILER_DLL_TODO }, // cfun_fnstrparm
        { Logic::FunctionCompilationType::FNPROPERTY,               implement_COMPILER_DLL_TODO }, // cfun_fnproperty
    };

    const auto& compilation_function_lookup = CompilationFunctionMap.find(CurrentToken.function_details->compilation_type);

    if( compilation_function_lookup == CompilationFunctionMap.cend() )
        IssueError(MGF::arithmetic_expression_invalid_21);

    if( compilation_function_lookup->second == implement_COMPILER_DLL_TODO )
        return rutfunc_COMPILER_DLL_TODO(CurrentToken.function_details->compilation_type);

    return (this->*compilation_function_lookup->second)();
}
