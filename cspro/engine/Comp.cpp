#include "StandardSystemIncludes.h"
#include "Tables.h"
#include "COMPILAD.H"
#include <zUtilO/MemoryHelpers.h>
#include <zAppO/Application.h>
#include <zLogicO/Preprocessor.h>
#include <zLogicO/ProcDirectory.h>
#include <zEngineO/EngineItem.h>
#include <zEngineO/File.h>
#include <zEngineO/ValueSet.h>
#include <zEngineO/WorkVariable.h>
#include <zEngineO/Compiler/TokenHelper.h>


CEngineCompFunc::CEngineCompFunc(CEngineDriver* pEngineDriver)
    :   LogicCompiler(pEngineDriver->getEngineAreaPtr()->m_engineData)
{
    // --------------------------------------------------------------------------
    // from the old CBaseComp
    // --------------------------------------------------------------------------
    m_iForRecordIdx = 0;    //Added by Savy (R) 20090716
    m_iShowfnGroupIdx = 0;  //Added by Savy (R) 20090731 //Fix for show() warning issue

    m_Flagvars = 0;         // to request index for Mult vars

    clearSyntaxErrorStatus();
    useForPrecedence();
    // --------------------------------------------------------------------------
    // --------------------------------------------------------------------------

    resetErrors();

    m_pEngineCompFunc = this;
    m_pEngineDriver = pEngineDriver;
    m_pEngineArea = pEngineDriver->getEngineAreaPtr();
    m_pEngineSettings = &(pEngineDriver->m_EngineSettings);
    m_pEngineDefines = &(pEngineDriver->m_EngineDefines);
    m_ForTableNext = 0;

    m_bIdChanger = false; // RHF Jul 01, 2005

    memset( &m_ForTable, 0, sizeof( FORTABLE ) * FORTABLE_MAX );

    m_Flagcomp = 0;
    m_LvlInComp = 0;
    m_ProcInComp = 0;

//BUCEN
    m_bcvarsubcheck = false;
    m_icGrpIdx = 0;
//BUCEN

    m_pCuroccGrpIdx = NULL; // 20091027

    m_loneAlphaFunctionCallTester = { 1, false }; // 20140422

    SetHasBreakBy( false ); // RHF Apr 16, 2003
}


CEngineCompFunc::~CEngineCompFunc()
{
    safe_delete_vector_contents(m_varsanalData);
}


// Create a new Work Variable
int CEngineCompFunc::MakeRelationWorkVar()
{
    std::wstring work_var_name = FormatTextCS2WS(_T("_IDX_%d"), static_cast<int>(GetSymbolTable().GetTableSize()));
    return m_engineData->AddSymbol(std::make_unique<WorkVariable>(std::move(work_var_name)));
}


bool CEngineCompFunc::CompExpIsValidVar( bool bCheckLevel )
{
    bool bOK = IsCurrentTokenVART(*this); // RHF Apr 05, 2000

    if( bOK ) {
        VART* pVarT=VPT(Tokstindex);

        if( bCheckLevel && pVarT->GetLevel() > LvlInComp ) {
            SetSyntErr(31000);
            bOK = false;
        }

        if( bOK && pVarT->GetDPT() != nullptr && !pVarT->GetDPT()->GetDataDict()->GetAllowExport() )
            IssueError(31007, pVarT->GetDPT()->GetName().c_str());
    }

    return bOK;
}


void CEngineCompFunc::CheckUnusedFileNames()
{
    for( const LogicFile* logic_file : m_engineData->files_global_visibility )
    {
        if( !logic_file->IsUsed() )
            IssueWarning(505, logic_file->GetName().c_str());
    }
}


const LogicSettings& CEngineCompFunc::GetLogicSettings() const
{
    return m_pEngineDriver->GetApplication()->GetLogicSettings();
}


void CEngineCompFunc::FormatMessageAndProcessParserMessage(Logic::ParserMessage& parser_message, va_list parg)
{
    if( ShouldSuppressErrorReporting() )
        return;

    m_pEngineDriver->GetSystemMessageIssuer().IssueVA(parser_message, parg);

    // set the fail message text
    if( parser_message.type == Logic::ParserError::Type::Error && Failmsg.IsEmpty() )
        Failmsg = WS2CS(parser_message.message_text);
}


Logic::ParserMessage CEngineCompFunc::CreateParserMessageFromIssaError(MessageType message_type, int message_number, std::wstring message_text) const
{
    // eventually, when all compilation errors use Logic::ParserMessage, this method will no longer be needed,
    // but for now, we have to translate the error to a parser message
    Logic::ParserMessage::Type parser_message_type =
        ( message_type == MessageType::Abort || message_type == MessageType::Error ) ? Logic::ParserMessage::Type::Error :
                                                                                       Logic::ParserMessage::Type::Warning;

    Logic::ParserMessage parser_message(parser_message_type);

    parser_message.message_number = message_number;
    parser_message.message_text = std::move(message_text);
    parser_message.compilation_unit_name = GetCurrentCompilationUnitName();
    parser_message.proc_name = WS2CS(GetCurrentProcName());

    if( GetCurrentCapiLogicLocation().has_value() )
        parser_message.extended_location = *GetCurrentCapiLogicLocation();

    const Logic::BasicToken* basic_token = GetCurrentBasicToken();

    if( basic_token != nullptr )
    {
        parser_message.line_number = basic_token->line_number;
        parser_message.position_in_line = basic_token->position_in_line;
    }

    return parser_message;
}


MessageManager& CEngineCompFunc::GetUserMessageManager()
{
    return m_pEngineDriver->GetUserMessageManager();
}


MessageEvaluator& CEngineCompFunc::GetUserMessageEvaluator()
{
    return m_pEngineDriver->GetUserMessageEvaluator();
}


void CEngineCompFunc::MarkSymbolAsUsed(Symbol& symbol)
{
    VART* pVarT = symbol.IsA(SymbolType::Item)     ? &assert_cast<EngineItem&>(symbol).GetVarT() :
                  symbol.IsA(SymbolType::ValueSet) ? assert_cast<ValueSet&>(symbol).GetVarT() :
                  symbol.IsA(SymbolType::Variable) ? &assert_cast<VART&>(symbol) :
                                                     nullptr;

    if( pVarT != nullptr )
        pVarT->SetUsed(true);
}


const Logic::ProcDirectory* CEngineCompFunc::CreateProcDirectory()
{
    bool has_errors = false;

    m_procDirectory = std::make_unique<Logic::ProcDirectory>();

    size_t proc_first_basic_token_index = 0;
    size_t proc_number_basic_tokens = 0;
    const Symbol* proc_symbol = nullptr;
    std::set<const Symbol*> encountered_proc_symbols;

    auto add_to_proc_directory = [&]
    {
        // add PROCS with logic (but always add PROC GLOBAL)
        if( proc_symbol != nullptr && ( proc_number_basic_tokens > 0 || proc_symbol->IsA(SymbolType::Application) ) )
            m_procDirectory->AddEntry(proc_symbol->GetSymbolIndex(), proc_first_basic_token_index, proc_number_basic_tokens);

        proc_number_basic_tokens = 0;
        proc_symbol = nullptr;
    };

    // go through each of the basic tokens
    cs::span<const Logic::BasicToken> basic_tokens = GetBasicTokensSpan();

    std::optional<std::wstring> constructing_proc_name;

    for( size_t token_index = 0; token_index < basic_tokens.size(); ++token_index )
    {
        const Logic::BasicToken& basic_token = basic_tokens[token_index];

        bool token_is_proc = ( basic_token.type == Logic::BasicToken::Type::Text &&
                               SO::EqualsNoCase(basic_token.GetTextSV(), _T("PROC")) );

        // issue an error if there is any code before the first proc
        if( !token_is_proc && token_index == 0 )
        {
            ReportError(89000);
            has_errors = true;
        }

        else if( token_is_proc )
        {
            // add the PROC that we've been currently processed (if applicable)
            add_to_proc_directory();

            constructing_proc_name.emplace();
        }

        else if( constructing_proc_name.has_value() )
        {
            // allow text and periods in the name
            TCHAR last_char_added = constructing_proc_name->empty() ? 0 : constructing_proc_name->back();
            bool proc_name_is_complete = true;
            bool proc_first_token_is_next_token = false;

            if( ( basic_token.type == Logic::BasicToken::Type::Text && ( last_char_added == 0 || last_char_added == '.' ) ) ||
                ( basic_token.token_code == TOKPERIOD && last_char_added != '.' ) )
            {
                constructing_proc_name->append(basic_token.GetTextSV());

                if( ( token_index + 1 ) == basic_tokens.size() || basic_tokens[token_index + 1].line_number != basic_token.line_number )
                {
                    proc_first_token_is_next_token = true;
                }

                else
                {
                    proc_name_is_complete = false;
                }
            }

            // when done, check if the name is valid
            if( proc_name_is_complete )
            {
                bool proc_name_is_symbol = false;
                bool proc_name_is_ambiguous = false;
                Symbol* chosen_valid_symbol = nullptr;

                for( Symbol* symbol : m_pEngineArea->SymbolTableSearchAllSymbols(*constructing_proc_name) )
                {
                    proc_name_is_symbol = true;

                    bool valid_symbol = false;

                    switch( symbol->GetType() )
                    {
                        case SymbolType::Group:
                        {
                            const GROUPT* pGroupT = assert_cast<const GROUPT*>(symbol);
                            const FLOW* pFlow = pGroupT->GetFlow();
                            bool bIsPrimaryFlow = false;

                            if( pFlow != nullptr )
                                bIsPrimaryFlow = ( pFlow->GetSubType() == SymbolSubType::Primary );

                            if( bIsPrimaryFlow || ( pGroupT->GetGroupType() != GROUPT::Level ) )
                                valid_symbol = true;

                            break;
                        }

                        case SymbolType::Variable:
                            valid_symbol = ( assert_cast<VART*>(symbol)->GetDictItem() != nullptr );
                            break;

                        case SymbolType::Application:
                        case SymbolType::Crosstab:
                        case SymbolType::Block:
                            valid_symbol = true;
                            break;
                    }

                    if( valid_symbol )
                    {
                        if( chosen_valid_symbol != nullptr )
                        {
                            proc_name_is_ambiguous = true;
                            break;
                        }

                        chosen_valid_symbol = symbol;
                    }
                }

                // ambiguous symbol
                if( proc_name_is_ambiguous )
                {
                    ReportError(90002, constructing_proc_name->c_str());
                    has_errors = true;
                }

                else if( chosen_valid_symbol != nullptr )
                {
                    // duplicate PROC
                    if( m_procDirectory->GetEntry(chosen_valid_symbol->GetSymbolIndex()) != nullptr || encountered_proc_symbols.find(chosen_valid_symbol) != encountered_proc_symbols.end() )
                    {
                        ReportError(90001, chosen_valid_symbol->GetName().c_str());
                        has_errors = true;
                    }

                    // a valid symbol
                    else
                    {
                        proc_first_basic_token_index = token_index + ( proc_first_token_is_next_token ? 1 : 0 );
                        proc_number_basic_tokens = proc_first_token_is_next_token ? 0 : 1;
                        proc_symbol = chosen_valid_symbol;
                        encountered_proc_symbols.insert(proc_symbol);
                    }
                }

                // a symbol that can't have a PROC
                else if( proc_name_is_symbol )
                {
                    ReportError(501, constructing_proc_name->c_str(), constructing_proc_name->c_str());
                    has_errors = true;
                }

                // invalid symbol
                else
                {
                    ReportError(90000, constructing_proc_name->c_str());
                    has_errors = true;
                }

                constructing_proc_name.reset();
            }
        }

        else if( proc_symbol != nullptr )
        {
            proc_number_basic_tokens++;
        }
    }

    // add any last PROC (if applicable)
    add_to_proc_directory();

    return has_errors ? nullptr : m_procDirectory.get();
}


const Logic::ProcDirectoryEntry* CEngineCompFunc::GetProcDirectoryEntry(int symbol_index) const
{
    return ( m_procDirectory != nullptr ) ? m_procDirectory->GetEntry(symbol_index) : nullptr;
}


int CEngineCompFunc::CompileProc(Symbol* symbol)
{
    m_ForTableNext = 0; // RHF Aug 14, 2000

    bool compilation_success = compobjOk(symbol);

    if( !compilation_success )
        incrementErrors();

    return compilation_success ? 1 : 0;
}


// --------------------------------------------------------------------------
// from the old CBaseComp
// --------------------------------------------------------------------------

void CEngineCompFunc::useForPrecedence()
{
    // TRACE( ".. CBaseComp is *using* For Precedence now\n" );
    m_bIgnoreForPrecedence = false;
    m_iGroupToIgnoreForPrecedence = 0;
}


void CEngineCompFunc::stopUsingForPrecedence( int iGroupToUse )
{
    if( iGroupToUse < 0 )
        iGroupToUse = -iGroupToUse;

    if( iGroupToUse > 0 )
    {
        // TRACE( ".. CBaseComp will _stop_ using For Precedence now\n" );
        m_bIgnoreForPrecedence = true;
        m_iGroupToIgnoreForPrecedence = iGroupToUse;
    }
}


int& CEngineCompFunc::GetLastFoundVariableIndex()
{
    static int xx = -1; // LOGIC_TODO
    return xx;
}



// --------------------------------------------------------------------------
// COMPILER_DLL_TODO...
// --------------------------------------------------------------------------

#include <zToolsO/ValueConserver.h>

template<typename CF>
int CEngineCompFunc::HandleErrors_COMPILER_DLL_TODO(CF callback_function)
{
    int p = callback_function();

    if( GetSyntErr() != 0 )
        IssueError(1);

    return p;
}


int CEngineCompFunc::CompileHas_COMPILER_DLL_TODO(int iVarNode)
{
    return HandleErrors_COMPILER_DLL_TODO([&]() { return CompileHas(iVarNode); } );
}

int CEngineCompFunc::crelalpha_COMPILER_DLL_TODO()
{
    return HandleErrors_COMPILER_DLL_TODO([&]() { return crelalpha(); } );
}

int CEngineCompFunc::varsanal_COMPILER_DLL_TODO(int fmt)
{
    return HandleErrors_COMPILER_DLL_TODO([&]() { return varsanal(fmt); } );
}

int CEngineCompFunc::tvarsanal_COMPILER_DLL_TODO()
{
    return HandleErrors_COMPILER_DLL_TODO([&]() { return tvarsanal(); } );
}

int CEngineCompFunc::rutfunc_COMPILER_DLL_TODO()
{
    return HandleErrors_COMPILER_DLL_TODO([&]() { return rutfunc(); } );
}

int CEngineCompFunc::instruc_COMPILER_DLL_TODO(bool create_new_local_symbol_stack, bool allow_multiple_statements)
{
    return HandleErrors_COMPILER_DLL_TODO([&]() { return instruc(create_new_local_symbol_stack, allow_multiple_statements); } );
}

int CEngineCompFunc::CompileReenterStatement_COMPILER_DLL_TODO(bool bNextTkn)
{
    return HandleErrors_COMPILER_DLL_TODO([&]() { return CompileReenterStatement(bNextTkn); } );
}

int CEngineCompFunc::CompileMoveStatement_COMPILER_DLL_TODO(bool bFromSelectStatement)
{
    return HandleErrors_COMPILER_DLL_TODO([&]() { return CompileMoveStatement(bFromSelectStatement); } );
}

void CEngineCompFunc::rutasync_as_global_compilation_COMPILER_DLL_TODO(const Symbol& compilation_symbol, const std::function<void()>& compilation_function)
{
    std::function<void()> this_compilation_function = [&]()
    {
        ValueConserver compilation_index_conserver(InCompIdx, compilation_symbol.GetSymbolIndex());

        SetCompilationSymbol(compilation_symbol);

        compilation_function();
    };

    // COMPILER_DLL_TODO eventually we shouldn't have to mock compilation as an application because uses of ObjInComp should account for Application/Report/UserFunction
    if( rutasync(Appl.GetSymbolIndex(), &this_compilation_function) )
        ReportError(GetSyntErr());
}

DICT* CEngineCompFunc::GetInputDictionary(bool issue_error_if_no_input_dictionary)
{
    ASSERT(issue_error_if_no_input_dictionary);
    return DIP(0);
}

void CEngineCompFunc::MarkAllDictionaryItemsAsUsed()
{
    for( VART* pVarT : m_engineData->variables )
        pVarT->SetUsed(true);

    for( DICT* pDicT : m_engineData->dictionaries_pre80 )
    {
        if( pDicT->GetCaseAccess() != nullptr )
            pDicT->GetCaseAccess()->SetUseAllDictionaryItems();
    }
}
