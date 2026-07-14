#include "stdafx.h"
#include "IncludesCC.h"


int LogicCompiler::CompileStatements(const bool create_new_local_symbol_stack/* = true*/, const bool allow_multiple_statements/* = true*/)
{
    return instruc_COMPILER_DLL_TODO(create_new_local_symbol_stack, allow_multiple_statements);

#ifdef REFERENCE // the implementation in engine/Instruc.cpp
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
                case TOKDECLARE:
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

                case TOKKWSTRINGWRITER:
                    last_added_node_address = CompileStringWriterDeclarations();
                    break;

                case TOKKWSYSTEMAPP:
                    last_added_node_address = CompileSystemAppDeclarations();
                    break;

                case TOKKWVALUESET:
                    last_added_node_address = CompileDynamicValueSetDeclarations();
                    break;

                case TOKKWVIDEO:
                    last_added_node_address = CompileLogicVideoDeclarations();
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

                case TOKVIDEO:
                {
                    compilation_address = CompileLogicVideoComputeInstruction();
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
                    case TOKSTOP:
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
                    case TOKNOINPUT:
                        if( ObjInComp != SymbolType::Variable || ( VPT(InCompIdx)->SYMTfrm <= 0 && Appl.ApplicationType == ModuleType::Entry ) ) // RHF Nov 07, 2001
                            IssueError( 557 );  // not in a field

                        if( !( GetCompilationProcType() == ProcType::PreProc || GetCompilationProcType() == ProcType::OnFocus ) )
                            IssueError( 558 ); // must be at Pre

                        NextToken();
                        break;

                    //~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~
                    //   Case 7. TOKENDSECT
                    //~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~
                    case TOKENDSECT:
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
                    case TOKENDLEVL:
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
            if( m_Flagcomp ) {
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
    if( m_Flagcomp ) {
        if( prev_st != NULL )
            prev_st->next_st = -1;
        else
            iptblock = -1;
    }
#endif


    if( local_symbol_stack.has_value() )
        iptblock = WrapNodeAroundScopeChange(*local_symbol_stack, iptblock);

    return iptblock;
#endif
}


int LogicCompiler::RouteFunctionCall()
{
    return rutfunc_COMPILER_DLL_TODO();

#ifdef REFERENCE // the implementation in engine/ExpresC.cpp
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
#endif
}
