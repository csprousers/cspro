#include "stdafx.h"
#include "IncludesCC.h"
#include "ImputeAutomaticStatCompilerHelper.h"
#include "Imputation.h"
#include "ValueSet.h"
#include "Nodes/Impute.h"
#include <engine/VarT.h>


//--------------------------------------------------------------------------
// CompileImputeFunction: compile IMPUTE command
//
//  IMPUTE(item_name, new_value)
//  [ STAT([item_list])
//  [ TITLE(string_expression) ]
//  [ VALUESET([valueset_name]) ]
//  [ SPECIFIC ]
//  ;
//
// item_list is a list of unrepeated variables. Comma separator is optional
//
//--------------------------------------------------------------------------

int LogicCompiler::CompileImputeFunction()
{
    const Logic::BasicToken* basic_token = GetCurrentBasicToken();
    size_t line_number = ( basic_token != nullptr ) ? basic_token->line_number : 0;

    Imputation& imputation = *m_engineData->imputations.emplace_back(std::make_unique<Imputation>(GetCurrentCompilationUnitName(), line_number));

    auto& impute_node = CreateNode<Nodes::Impute>(FunctionCode::FNIMPUTE_CODE);

    impute_node.imputation_index =  static_cast<int>(m_engineData->imputations.size() - 1);
    impute_node.title_expression = -1;
    impute_node.stat_variable_compilation_list_node = -1;

    NextToken();
    IssueErrorOnTokenMismatch(TOKLPAREN, MGF::left_parenthesis_expected_in_function_call_14);

    NextToken();

    // dictionary variable expected
    const VART* pVarT = IsCurrentTokenVART(*this) ? VPT(Tokstindex) : nullptr;

    if( pVarT == nullptr || pVarT->GetDictItem() == nullptr )
        IssueError(MGF::dictionary_item_expected_8100);

    imputation.SetVariable(pVarT);

    // compile the variable and its occurrences
    impute_node.variable_compilation = varsanal_COMPILER_DLL_TODO(pVarT->GetFmt());

    IssueErrorOnTokenMismatch(TOKCOMMA, MGF::function_call_comma_expected_528);

    // read the new value
    NextToken();
    impute_node.value_expression = CompileExpression(pVarT->GetDataType());

    IssueErrorOnTokenMismatch(TOKRPAREN, MGF::right_parenthesis_expected_in_function_call_17);

    // compile the additional options
    std::vector<const TCHAR*> impute_keywords { _T("TITLE"), _T("SPECIFIC"), _T("VALUESET"), _T("STAT"), _T("VSET") };
    std::vector<bool> keyword_used(impute_keywords.size() + 1, 0);

    while( true )
    {
        size_t additional_option_type = NextKeyword(impute_keywords);

        if( additional_option_type == 0 )
            break;

        bool using_vset = ( additional_option_type == 5 );

        if( using_vset )
        {
            IssueWarning(Logic::ParserMessage::Type::DeprecationMajor, MGF::deprecation_impute_with_vset_95024);
            additional_option_type = 3;
        }

        // don't allow repeated options
        if( keyword_used[additional_option_type] )
            IssueError(MGF::Impute_repeated_clause_8101, impute_keywords[additional_option_type - 1]);

        keyword_used[additional_option_type] = true;


        // TITLE
        if( additional_option_type == 1 )
        {
            NextToken();
            IssueErrorOnTokenMismatch(TOKLPAREN, MGF::left_parenthesis_expected_in_function_call_14);

            bool next_argument_is_string_literal = ( CheckNextTokenHelper() == NextTokenHelperResult::StringLiteral );
            NextToken();

            if( next_argument_is_string_literal )
            {
                imputation.SetTitle(Tokstr);
                NextToken();
            }

            else
            {
                impute_node.title_expression = CompileStringExpression();
            }

            IssueErrorOnTokenMismatch(TOKRPAREN, MGF::right_parenthesis_expected_in_function_call_17);
        }


        // SPECIFIC
        else if( additional_option_type == 2 )
        {
            imputation.SetSpecific();
        }


        // VALUESET / VSET
        else if( additional_option_type == 3 )
        {
            NextToken();
            IssueErrorOnTokenMismatch(TOKLPAREN, MGF::left_parenthesis_expected_in_function_call_14);

            NextToken();

            const CDictItem* dict_item = pVarT->GetDictItem();
            const DictValueSet* dict_value_set;

            if( !dict_item->HasValueSets() )
                IssueError(MGF::Impute_dict_item_has_no_value_sets_8102, pVarT->GetName().c_str());

            // if not specified, use the first value set
            if( Tkn == TOKRPAREN )
            {
                dict_value_set = &dict_item->GetValueSet(0);
            }

            else if( using_vset )
            {
                // one-based value set number
                int value_set_index = ( Tkn == TOKCTE && IsNumericConstantInteger() ) ? static_cast<int>(Tokvalue) :
                                                                                        0;

                if( value_set_index < 1 || value_set_index > static_cast<int>(dict_item->GetNumValueSets()) )
                    IssueError(MGF::Impute_invalid_value_set_index_8103, Tokstr.c_str(), pVarT->GetName().c_str(), static_cast<int>(dict_item->GetNumValueSets()));

                dict_value_set = &dict_item->GetValueSet(value_set_index - 1);

                NextToken();
            }

            else
            {
                const ValueSet* value_set = ( Tkn == TOKVALUESET ) ? &GetSymbolValueSet(Tokstindex) : nullptr;

                if( value_set == nullptr || value_set->GetVarT() != pVarT )
                    IssueError(MGF::Impute_value_set_does_not_belong_to_item_8104, pVarT->GetName().c_str());

                dict_value_set = &value_set->GetDictValueSet();

                NextToken();
            }

            imputation.SetValueSet(&GetSymbolValueSet(dict_value_set->GetSymbolIndex()));

            IssueErrorOnTokenMismatch(TOKRPAREN, MGF::right_parenthesis_expected_in_function_call_17);
        }


        // STAT
        else if( additional_option_type == 4 )
        {
            NextToken();
            IssueErrorOnTokenMismatch(TOKLPAREN, MGF::left_parenthesis_expected_in_function_call_14);

            std::vector<const VART*> stat_variables;
            std::vector<int> stat_variable_compilations;

            NextToken();

            if( Tkn != TOKRPAREN )
            {
                while( true )
                {
                    const VART* pStatVarT = IsCurrentTokenVART(*this) ? VPT(Tokstindex) : nullptr;

                    if( pStatVarT == nullptr || pStatVarT->GetDictItem() == nullptr )
                        IssueError(MGF::dictionary_item_expected_8100);

                    stat_variables.emplace_back(pStatVarT);
                    stat_variable_compilations.emplace_back(varsanal_COMPILER_DLL_TODO(pStatVarT->GetFmt()));

                    if( Tkn == TOKRPAREN )
                        break;

                    IssueErrorOnTokenMismatch(TOKCOMMA, MGF::function_call_comma_expected_528);
                    NextToken();
                }
            }

            IssueErrorOnTokenMismatch(TOKRPAREN, MGF::right_parenthesis_expected_in_function_call_17);

            // only add the runtime aspects of stat if it is not being overriden to 'off'
            if( GetCompilerHelper<ImputeAutomaticStatCompilerHelper>().GetAutomaticStatFlag() != false )
            {
                imputation.SetUsingStat();
                imputation.SetStatVariables(std::move(stat_variables));
                impute_node.stat_variable_compilation_list_node = CreateListNode(stat_variable_compilations);
            }
        }
    }

    NextToken();

    IssueErrorOnTokenMismatch(TOKSEMICOLON, MGF::expecting_semicolon_30);


    // set using stat if it was overriden to 'on'
    if( GetCompilerHelper<ImputeAutomaticStatCompilerHelper>().GetAutomaticStatFlag() == true )
        imputation.SetUsingStat();


    if( m_engineData->application != nullptr )
    {
        m_engineData->application->SetHasImputeStatements();

        if( imputation.GetUsingStat() )
            m_engineData->application->SetHasImputeStatStatements();
    }


    return GetProgramIndex(impute_node);
}


void LogicCompiler::CompileSetImpute()
{
    // set impute(stat, on|off|default)
    NextToken();
    IssueErrorOnTokenMismatch(TOKLPAREN, MGF::left_parenthesis_expected_in_function_call_14);

    NextKeywordOrError({ _T("STAT") });

    NextToken();
    IssueErrorOnTokenMismatch(TOKCOMMA, MGF::function_call_comma_expected_528);

    size_t stat_type = NextKeywordOrError({ _T("ON"), _T("OFF"), _T("DEFAULT") });

    std::optional<bool> automatic_stat_flag = ( stat_type != 3 ) ? std::make_optional<bool>(stat_type == 1) :
                                                                   std::nullopt;

    GetCompilerHelper<ImputeAutomaticStatCompilerHelper>().SetAutomaticStatFlag(std::move(automatic_stat_flag));

    NextToken();
    IssueErrorOnTokenMismatch(TOKRPAREN, MGF::right_parenthesis_expected_in_function_call_17);

    NextToken();
    IssueErrorOnTokenMismatch(TOKSEMICOLON, MGF::expecting_semicolon_30);
}
