#include "stdafx.h"
#include "IncludesCC.h"
#include "EngineDictionaryFactory.h"
#include <zLogicO/KeywordTable.h>


// --------------------------------------------------------------------------
// Case
// --------------------------------------------------------------------------

EngineDictionary* LogicCompiler::CompileEngineCaseDeclaration(const EngineDictionary* engine_dictionary_to_copy_attributes/* = nullptr*/)
{
    // Case (DICT_NAME) case_name;
    constexpr int default_message_number = MGF::Case_dictionary_placement_47251;

    const EngineDictionary* base_engine_dictionary;

    if( engine_dictionary_to_copy_attributes != nullptr )
    {
        base_engine_dictionary = engine_dictionary_to_copy_attributes;
    }

    else
    {
        NextTokenOrNewSymbolName();
        IssueErrorOnTokenMismatch(TOKLPAREN, default_message_number);

        NextTokenOrNewSymbolName();
        IssueErrorOnTokenMismatch(TOKDICT, default_message_number);

        // force the dictionary name to be used (not a Case or DataSource)
        base_engine_dictionary = &GetSymbolEngineDictionary(Tokstindex);

        if( !base_engine_dictionary->IsDictionaryObject() )
            IssueError(default_message_number);

        NextTokenOrNewSymbolName();
        IssueErrorOnTokenMismatch(TOKRPAREN, default_message_number);
    }

    // get the Case name
    std::wstring case_name = CompileNewSymbolName();

    std::shared_ptr<EngineDictionary> engine_dictionary = EngineDictionaryFactory::CreateCase(std::move(case_name),
                                                                                              *base_engine_dictionary,
                                                                                              *m_engineData);
    m_engineData->AddSymbol(engine_dictionary);

    return engine_dictionary.get();
}


int LogicCompiler::CompileEngineCases()
{
    constexpr bool TODO_DISABLED_FOR_CSPRO77 = true;
    if( TODO_DISABLED_FOR_CSPRO77 )
        IssueError(MGF::statement_invalid_1);

    ASSERT(Tkn == TOKKWCASE);
    Nodes::SymbolReset* symbol_reset_node = nullptr;
    EngineDictionary* last_engine_dictionary_compiled = nullptr;

    do
    {
        last_engine_dictionary_compiled = CompileEngineCaseDeclaration(last_engine_dictionary_compiled);

        int& initialize_value = AddSymbolResetNode(symbol_reset_node, *last_engine_dictionary_compiled);

        NextToken();

        // allow assignments as part of the declaration
        if( Tkn == TOKEQOP )
            initialize_value = CompileEngineCaseComputeInstruction(last_engine_dictionary_compiled);

    } while( Tkn == TOKCOMMA );

    IssueErrorOnTokenMismatch(TOKSEMICOLON, MGF::expecting_semicolon_30);

    return GetOptionalProgramIndex(symbol_reset_node);
}


int LogicCompiler::CompileEngineCaseComputeInstruction(const EngineDictionary* engine_dictionary_from_declaration/* = nullptr*/)
{
    // compiling: case_name = rhs_case_name / rhs_DICT_NAME;

    // this is not valid if declaring globally
    if( IsGlobalCompilation() )
        IssueError(MGF::symbol_assignment_not_allowed_in_proc_global_687, Logic::KeywordTable::GetKeywordName(TOKKWCASE));

    const EngineDictionary* lhs_engine_dictionary = engine_dictionary_from_declaration;

    if( lhs_engine_dictionary == nullptr )
    {
        ASSERT(Tkn == TOKDICT);

        lhs_engine_dictionary = &GetSymbolEngineDictionary(Tokstindex);
        ASSERT(lhs_engine_dictionary->HasEngineCase());

        // the case must be an external dictionary
        // ENGINECR_TODO should also make sure dictionaries for external forms can't be used
        if( lhs_engine_dictionary->GetSubType() != SymbolSubType::External )
        {
            IssueError(MGF::Case_assignment_must_be_from_external_case_47254, lhs_engine_dictionary->GetName().c_str(),
                                                                              ToString(lhs_engine_dictionary->GetSubType()));
        }

        NextToken();
        IssueErrorOnTokenMismatch(TOKEQOP, MGF::equals_expected_in_assignment_5);
    }

    NextToken();
    IssueErrorOnTokenMismatch(TOKDICT, MGF::Case_assignment_invalid_47252);

    const EngineDictionary& rhs_engine_dictionary = GetSymbolEngineDictionary(Tokstindex);

    NextToken();

    if( engine_dictionary_from_declaration == nullptr )
        IssueErrorOnTokenMismatch(TOKSEMICOLON, MGF::expecting_semicolon_30);

    // confirm that this is not a data repository and that it shares the same dictionary
    if( !rhs_engine_dictionary.HasEngineCase() )
        IssueError(MGF::Case_assignment_invalid_47252);

    if( !lhs_engine_dictionary->DictionaryMatches(rhs_engine_dictionary) )
    {
        IssueError(MGF::Case_assignment_dictionary_mismatch_47253, lhs_engine_dictionary->GetName().c_str(),
                                                                   lhs_engine_dictionary->GetDictionary().GetName().GetString(),
                                                                   rhs_engine_dictionary.GetDictionary().GetName().GetString());
    }

    auto& symbol_compute_node = CreateNode<Nodes::SymbolCompute>(FunctionCode::DICTFN_COMPUTE_CODE);

    symbol_compute_node.next_st = -1;
    symbol_compute_node.lhs_symbol_index = lhs_engine_dictionary->GetSymbolIndex();
    symbol_compute_node.rhs_symbol_type = SymbolType::Dictionary;
    symbol_compute_node.rhs_symbol_index = rhs_engine_dictionary.GetSymbolIndex();

    int program_index = GetProgramIndex(symbol_compute_node);

    // ensure that the case data is available
    return WrapNodeAroundValidDataAccessCheck(program_index, rhs_engine_dictionary, DataType::Numeric);
}



// --------------------------------------------------------------------------
// DataSource
// --------------------------------------------------------------------------

EngineDictionary* LogicCompiler::CompileEngineDataRepositoryDeclaration(const EngineDictionary* engine_dictionary_to_copy_attributes/* = nullptr*/)
{
    // DataSource (DICT_NAME) datasource_name;
    constexpr int default_message_number = MGF::DataSource_dictionary_placement_47261;

    const EngineDictionary* base_engine_dictionary;

    if( engine_dictionary_to_copy_attributes != nullptr )
    {
        base_engine_dictionary = engine_dictionary_to_copy_attributes;
    }

    else
    {
        NextTokenOrNewSymbolName();
        IssueErrorOnTokenMismatch(TOKLPAREN, default_message_number);

        NextTokenOrNewSymbolName();
        IssueErrorOnTokenMismatch(TOKDICT, default_message_number);

        // force the dictionary name to be used (not a Case or DataSource)
        base_engine_dictionary = &GetSymbolEngineDictionary(Tokstindex);

        if( !base_engine_dictionary->IsDictionaryObject() )
            IssueError(default_message_number);

        NextTokenOrNewSymbolName();
        IssueErrorOnTokenMismatch(TOKRPAREN, default_message_number);
    }

    // get the DataSource name
    std::wstring datasource_name = CompileNewSymbolName();

    std::shared_ptr<EngineDictionary> engine_dictionary = EngineDictionaryFactory::CreateDataRepository(std::move(datasource_name),
                                                                                                        *base_engine_dictionary,
                                                                                                        *m_engineData);

    m_engineData->AddSymbol(engine_dictionary);

    return engine_dictionary.get();
}


int LogicCompiler::CompileEngineDataRepositories()
{
    ASSERT(Tkn == TOKKWDATASOURCE);
    Nodes::SymbolReset* symbol_reset_node = nullptr;
    EngineDictionary* last_engine_dictionary_compiled = nullptr;

    do
    {
        last_engine_dictionary_compiled = CompileEngineDataRepositoryDeclaration(last_engine_dictionary_compiled);

        AddSymbolResetNode(symbol_reset_node, *last_engine_dictionary_compiled);

        NextToken();

    } while( Tkn == TOKCOMMA );

    IssueErrorOnTokenMismatch(TOKSEMICOLON, MGF::expecting_semicolon_30);

    return GetOptionalProgramIndex(symbol_reset_node);
}
