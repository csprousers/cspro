#include "stdafx.h"
#include "IncludesCC.h"
#include "NamedFrequency.h"
#include "Report.h"
#include "ValueSet.h"
#include "Nodes/Frequency.h"
#include <engine/VariableWorker.h>
#include <zFreqO/Frequency.h>


// both unnamed and named frequencies:
//   FREQ
//      (...) right after FREQ or the variable name is treated as INCLUDE
//      INCLUDE/EXCLUDE
//      (
//          short|long|integer|float|numeric|alpha
//          dictionary_name
//          record_name[(occurrence)]
//          item_name[(occurrence[, ...occurrence])]
//          form_name/group_name/block_name
//      )
//      DISJOINT
//      BREAKDOWN[([constant 1+])]
//
// unnamed frequencies:
//   FREQ
//      UNIVERSE(number) / SELECT[(]number[)]
//      WEIGHT(number)   / WEIGHTED [BY] number
//
// named frequencies:
//      FREQ freq_name
//
// commands for unnamed and named frequencies and the save/view functions:
//      HEADING / TITLE(string[, string, ...])
//      DISTINCT
//      VALUESET(valueset_name[, ...valueset_name])
//      VSET / VSETS
//      STAT
//      NOFREQ
//      PERCENTILES / NTILES(constant between 2-20)
//      NONETPERCENTS
//      DECIMALS(constant between 1-5)
//      PAGELENGTH(constant 0+)
//      SORT([ascending | descending] [by valueset | code | label | freq])



namespace
{
    static const std::vector<const TCHAR*> FrequencyCommands =
    {
        _T("INCLUDE"),
        _T("EXCLUDE"),
        _T("UNIVERSE"), _T("SELECT"),
        _T("WEIGHT"),   _T("WEIGHTED"),
        _T("DISJOINT"),
        _T("BREAKDOWN"),
        _T("HEADING"), _T("TITLE"),
        _T("DISTINCT"),
        _T("VALUESET"),
        _T("VSET"), _T("VSETS"),
        _T("STAT"),
        _T("NOFREQ"),
        _T("PERCENTILES"), _T("NTILES"),
        _T("NONETPERCENTS"),
        _T("DECIMALS"),
        _T("PAGELENGTH"),
        _T("SORT"),
    };

    constexpr size_t CI_INCLUDE       =  1; // CI = command index
    constexpr size_t CI_EXCLUDE       =  2;
    constexpr size_t CI_UNIVERSE      =  3;
    constexpr size_t CI_SELECT        =  4;
    constexpr size_t CI_WEIGHT        =  5;
    constexpr size_t CI_WEIGHTED      =  6;
    constexpr size_t CI_DISJOINT      =  7;
    constexpr size_t CI_BREAKDOWN     =  8;
    constexpr size_t CI_HEADING       =  9;
    constexpr size_t CI_TITLE         = 10;
    constexpr size_t CI_DISTINCT      = 11;
    constexpr size_t CI_VALUESET      = 12;
    constexpr size_t CI_VSET          = 13;
    constexpr size_t CI_VSETS         = 14;
    constexpr size_t CI_STAT          = 15;
    constexpr size_t CI_NOFREQ        = 16;
    constexpr size_t CI_PERCENTILES   = 17;
    constexpr size_t CI_NTILES        = 18;
    constexpr size_t CI_NONETPERCENTS = 19;
    constexpr size_t CI_DECIMALS      = 20;
    constexpr size_t CI_PAGELENGTH    = 21;
    constexpr size_t CI_SORT          = 22;

    constexpr size_t AllOccurrencesFlag = SIZE_MAX;

    struct FrequencySymbolCompilation
    {
        int symbol_index;
        std::vector<size_t> occurrences;
    };

    struct FrequencySymbolInclusionFlags
    {
        bool include_shorts;
        bool include_longs;
        bool include_floats;
        bool include_alphas;
    };
}



// --------------------------------------------------------------------------
// FrequencyCompilerHelper
// --------------------------------------------------------------------------

class FrequencyCompilerHelper
{
public:
    FrequencyCompilerHelper(LogicCompiler& logic_compiler)
        :   m_compiler(logic_compiler)
    {
    }

    static bool AddVariableToFrequency(const VART& vart, const std::optional<FrequencySymbolInclusionFlags>& flags);

    template<typename FPCallback>
    void CompileFreqCommands(std::optional<size_t> initial_command, Nodes::FrequencyParameters& frequency_parameters_node,
                             bool allow_heading_string_literal_compilation, FPCallback frequency_parameter_callback);

    void ProcessVariables(Frequency& frequency,
                          const std::vector<FrequencySymbolCompilation>& frequency_included_symbols, const std::optional<FrequencySymbolInclusionFlags>& frequency_included_flags,
                          const std::vector<FrequencySymbolCompilation>& frequency_excluded_symbols, const std::optional<FrequencySymbolInclusionFlags>& frequency_excluded_flags,
                          bool disjoint, std::optional<int> breakdown);

    void ValidateValueSets(const Frequency& frequency, const std::vector<int>& value_set_symbol_indices);

private:
    Logic::SymbolTable& GetSymbolTable()        { return m_compiler.GetSymbolTable(); }
    const Logic::Token& GetCurrentToken() const { return m_compiler.GetCurrentToken(); }
    int& get_COMPILER_DLL_TODO_Tokstindex()     { return m_compiler.get_COMPILER_DLL_TODO_Tokstindex(); }

private:
    LogicCompiler& m_compiler;
};



bool FrequencyCompilerHelper::AddVariableToFrequency(const VART& vart, const std::optional<FrequencySymbolInclusionFlags>& flags)
{
    const CDictItem* dict_item = vart.GetDictItem();
    ASSERT(dict_item != nullptr);

    // specifying no flags leads to the default behavior (introduced in CSPro 7.6),
    // which includes all items of a certain length, and then all items longer
    // than that if they have a value set defined
    if( !flags.has_value() )
    {
        constexpr UINT MaxItemLength = 5;
        return ( dict_item->GetLen() < MaxItemLength || dict_item->HasValueSets() );
    }

    // otherwise the rules are based on whether or not an item is a parent item
    // and on the inclusion flags
    else
    {
        CONTENT_TYPE_REFACTOR::LOOK_AT("what to do about non-numeric + non-alpha fields? not just here but throughout freqs");
        return ( dict_item->HasSubitems()  )                         ? false :
               ( dict_item->GetContentType() == ContentType::Alpha ) ? flags->include_alphas :
               ( dict_item->GetDecimal() > 0 )                       ? flags->include_floats :
               ( dict_item->GetLen() >= 3 )                          ? flags->include_longs :
                                                                       flags->include_shorts;
    }
}


template<typename FPCallback>
void FrequencyCompilerHelper::CompileFreqCommands(std::optional<size_t> initial_command, Nodes::FrequencyParameters& frequency_parameters_node,
                                                  bool allow_heading_string_literal_compilation, FPCallback frequency_parameter_callback)
{
    m_compiler.InitializeNode(frequency_parameters_node, -1);

    std::set<size_t> defined_commands;
    bool just_read_include_left_parenthesis = false;

    while( true )
    {
        // read the next command
        size_t command_type = initial_command.has_value() ? *initial_command :
                                                            m_compiler.NextKeyword(FrequencyCommands);
        initial_command.reset();

        if( command_type == 0 )
        {
            m_compiler.NextToken();

            // if the first thing encountered is a left parenthesis, treat it as include
            if( Tkn == TOKLPAREN && defined_commands.empty() )
            {
                command_type = CI_INCLUDE;
                just_read_include_left_parenthesis = true;
            }

            else if( Tkn == TOKSEMICOLON || Tkn == TOKEOP )
            {
                return;
            }

            // invalid command
            else
            {
                m_compiler.IssueError(MGF::Freq_invalid_command_94501, Tokstr.c_str());
            }
        }

        // some commands have aliases
        size_t aliased_command_type = ( command_type == CI_SELECT )   ? CI_UNIVERSE :
                                      ( command_type == CI_WEIGHTED ) ? CI_WEIGHT :
                                      ( command_type == CI_TITLE )    ? CI_HEADING :
                                      ( command_type == CI_VSETS )    ? CI_VSET :
                                      ( command_type == CI_NTILES )   ? CI_PERCENTILES :
                                                                        command_type;

        // don't allow duplicate commands
        if( defined_commands.find(aliased_command_type) != defined_commands.cend() )
            m_compiler.IssueError(MGF::Freq_duplicate_command_94502, FrequencyCommands[command_type - 1]);

        defined_commands.emplace(aliased_command_type);


        // some helper functions
        bool command_uses_parentheses = false;

        auto validate_left_parenthesis = [&](bool read_next_token)
        {
            if( read_next_token )
                m_compiler.NextToken();

            m_compiler.IssueErrorOnTokenMismatch(TOKLPAREN, MGF::left_parenthesis_expected_in_function_call_14);

            command_uses_parentheses = true;
        };

        auto is_next_token_left_parenthesis = [&]()
        {
            bool left_parenthesis_used = false;
            m_compiler.MarkInputBufferToRestartLater();

            if( m_compiler.NextKeyword(FrequencyCommands) == 0 )
            {
                m_compiler.NextToken();
                left_parenthesis_used = ( Tkn == TOKLPAREN );
            }

            m_compiler.RestartFromMarkedInputBuffer();

            return left_parenthesis_used;
        };

        auto read_constant_int = [&](bool read_next_token, int min_value, int max_value) -> std::optional<int>
        {
            if( read_next_token )
                m_compiler.NextToken();

            if( Tkn == TOKCTE && m_compiler.IsNumericConstantInteger() )
            {
                int value = static_cast<int>(Tokvalue);

                if( value >= min_value && value <= max_value )
                {
                    m_compiler.NextToken();
                    return value;
                }
            }

            return std::nullopt;
        };


        // INCLUDE / EXCLUDE
        if( aliased_command_type == CI_INCLUDE || aliased_command_type == CI_EXCLUDE )
        {
            if( just_read_include_left_parenthesis )
            {
                just_read_include_left_parenthesis = false;
            }

            else
            {
                validate_left_parenthesis(true);
            }

            frequency_parameter_callback(aliased_command_type, std::nullopt);
        }


        // UNIVERSE / SELECT
        else if( aliased_command_type == CI_UNIVERSE )
        {
            if( command_type == CI_UNIVERSE || is_next_token_left_parenthesis() )
            {
                validate_left_parenthesis(true);
            }

            // for backwards compatibility, only warn if a parenthesis isn't used for SELECT
            else
            {
                m_compiler.IssueWarning(Logic::ParserMessage::Type::DeprecationMinor, MGF::deprecation_freq_without_parentheses_95025, FrequencyCommands[command_type - 1]);
            }

            m_compiler.NextToken();

            frequency_parameter_callback(CI_UNIVERSE, m_compiler.exprlog());

            if( command_type == CI_SELECT && !command_uses_parentheses && Tkn == TOKSEMICOLON )
                break;
        }


        // WEIGHT / WEIGHTED
        else if( aliased_command_type == CI_WEIGHT )
        {
            if( command_type == CI_WEIGHT )
            {
                validate_left_parenthesis(true);
                m_compiler.NextToken();
            }

            // warn when WEIGHTED is used
            else
            {
                m_compiler.IssueWarning(Logic::ParserMessage::Type::DeprecationMinor, MGF::deprecation_freq_weight_command_95026);

                m_compiler.NextToken();

                // skip the optional BY
                if( Tkn == TOKBY )
                    m_compiler.NextToken();
            }

            frequency_parameter_callback(CI_WEIGHT, m_compiler.exprlog());

            if( command_type == CI_WEIGHTED && Tkn == TOKSEMICOLON )
                break;
        }


        // DISJOINT
        else if( aliased_command_type == CI_DISJOINT )
        {
            frequency_parameter_callback(CI_DISJOINT, std::nullopt);
        }


        // BREAKDOWN
        else if( aliased_command_type == CI_BREAKDOWN )
        {
            std::optional<int> breakdown;

            if( is_next_token_left_parenthesis() )
            {
                validate_left_parenthesis(true);

                m_compiler.NextToken();

                if( Tkn != TOKRPAREN )
                {
                    if( breakdown = read_constant_int(false, 1, INT_MAX); !breakdown.has_value() )
                        m_compiler.IssueError(MGF::Freq_invalid_breakdown_94504);
                }
            }

            frequency_parameter_callback(CI_BREAKDOWN, breakdown.value_or(1));
        }


        // HEADING / TITLE
        else if( aliased_command_type == CI_HEADING )
        {
            // multiple lines can be supplied, separated by commas;
            // in PROC GLOBAL, only string literals can be used
            std::vector<int> heading_expressions;
            std::vector<std::wstring> headings;

            validate_left_parenthesis(true);

            while( true )
            {
                if( allow_heading_string_literal_compilation && heading_expressions.empty() &&
                    m_compiler.CheckNextTokenHelper() == LogicCompiler::NextTokenHelperResult::StringLiteral )
                {
                    m_compiler.NextToken();
                    headings.emplace_back(Tokstr);
                    m_compiler.NextToken();
                }

                else
                {
                    if( m_compiler.IsGlobalCompilation() )
                        m_compiler.IssueError(MGF::Freq_headings_must_be_string_literals_94505);

                    m_compiler.NextToken();

                    // if a mix of string literals and evaluated strings is provided,
                    // create expressions for the string literals
                    if( !headings.empty() )
                    {
                        ASSERT(heading_expressions.empty());

                        for( const std::wstring& heading : headings )
                            heading_expressions.emplace_back(m_compiler.CreateStringLiteralNode(heading));

                        headings.clear();
                    }

                    heading_expressions.emplace_back(m_compiler.CompileStringExpression());
                }

                if( Tkn == TOKRPAREN )
                    break;

                m_compiler.IssueErrorOnTokenMismatch(TOKCOMMA, MGF::function_call_comma_expected_528);
            }

            if( !headings.empty() )
            {
                frequency_parameter_callback(CI_HEADING, &headings);
            }

            else
            {
                frequency_parameters_node.heading_expressions_list_node = m_compiler.CreateListNode(heading_expressions);
            }
        }


        // DISTINCT
        else if( aliased_command_type == CI_DISTINCT )
        {
            frequency_parameters_node.distinct = 1;
        }


        // VALUESET
        else if( aliased_command_type == CI_VALUESET )
        {
            std::vector<int> value_set_symbol_indices;

            validate_left_parenthesis(true);
            m_compiler.NextToken();

            while( Tkn != TOKRPAREN )
            {
                if( !value_set_symbol_indices.empty() )
                {
                    m_compiler.IssueErrorOnTokenMismatch(TOKCOMMA, MGF::function_call_comma_expected_528);
                    m_compiler.NextToken();
                }

                if( Tkn != TOKVALUESET )
                    m_compiler.IssueError(MGF::object_of_type_expected_33116, ToString(SymbolType::ValueSet));

                const ValueSet& value_set = GetSymbolValueSet(Tokstindex);

                if( value_set.IsDynamic() )
                    m_compiler.IssueError(MGF::Freq_value_sets_must_be_from_dictionary_94523, value_set.GetName().c_str());

                if( std::find(value_set_symbol_indices.cbegin(), value_set_symbol_indices.cend(), Tokstindex) == value_set_symbol_indices.cend() )
                    value_set_symbol_indices.emplace_back(Tokstindex);

                m_compiler.NextToken();
            }

            frequency_parameter_callback(CI_VALUESET, &value_set_symbol_indices);
        }


        // VSET / VSETS
        else if( aliased_command_type == CI_VSET )
        {
            frequency_parameters_node.use_all_value_sets = 1;
        }


        // STAT
        else if( aliased_command_type == CI_STAT )
        {
            frequency_parameters_node.show_statistics = 1;
        }


        // NOFREQ
        else if( aliased_command_type == CI_NOFREQ )
        {
            frequency_parameters_node.show_no_frequencies = 1;
        }


        // PERCENTILES / NTILES
        else if( aliased_command_type == CI_PERCENTILES )
        {
            validate_left_parenthesis(true);

            if( auto percentiles = read_constant_int(true, 2, 20); percentiles.has_value() )
            {
                frequency_parameters_node.percentiles = *percentiles;
            }

            else
            {
                m_compiler.IssueError(MGF::Freq_invalid_percentiles_94506);
            }
        }


        // NONETPERCENTS
        else if( aliased_command_type == CI_NONETPERCENTS )
        {
            frequency_parameters_node.show_no_net_percents = 1;
        }


        // DECIMALS
        else if( aliased_command_type == CI_DECIMALS )
        {
            validate_left_parenthesis(true);

            if( auto decimals = read_constant_int(true, 1, 5); decimals.has_value() )
            {
                frequency_parameters_node.decimals = *decimals;
            }

            else
            {
                m_compiler.IssueError(MGF::Freq_invalid_decimals_94507);
            }
        }


        // PAGELENGTH
        else if( aliased_command_type == CI_PAGELENGTH )
        {
            validate_left_parenthesis(true);

            if( auto page_length = read_constant_int(true, 0, INT_MAX); page_length.has_value() )
            {
                frequency_parameters_node.page_length = *page_length;
            }

            else
            {
                m_compiler.IssueError(MGF::Freq_invalid_page_length_94508);
            }
        }


        // SORT
        else if( aliased_command_type == CI_SORT )
        {
            validate_left_parenthesis(true);
            m_compiler.NextToken();

            std::optional<bool> ascending;
            std::optional<FrequencyPrinterOptions::SortType> sort_type;

            while( Tkn != TOKRPAREN )
            {
                if( bool specified_ascending = ( Tkn == TOKASCENDING ); ( specified_ascending || Tkn == TOKDESCENDING ) && !ascending.has_value() )
                {
                    ascending = specified_ascending;
                }

                else if( Tkn == TOKBY && !sort_type.has_value() )
                {
                    size_t by_type = m_compiler.NextKeywordOrError({ _T("valueset"), _T("code"), _T("label"), _T("freq") });
                    sort_type = static_cast<FrequencyPrinterOptions::SortType>(by_type - 1);
                }

                else
                {
                    m_compiler.IssueError(MGF::Freq_invalid_sort_94509);
                }

                m_compiler.NextToken();
            }

            if( !ascending.has_value() && !sort_type.has_value() )
                m_compiler.IssueError(MGF::Freq_invalid_sort_94509);

            frequency_parameters_node.sort_order_and_type = FrequencyPrinterOptions::GetSortOrderAndTypeAsInt(
                ascending.value_or(true), sort_type.value_or(FrequencyPrinterOptions::SortType::ByValueSetOrder));
        }


        else
        {
            ASSERT(false);
        }


        if( command_uses_parentheses )
            m_compiler.IssueErrorOnTokenMismatch(TOKRPAREN, MGF::right_parenthesis_expected_in_function_call_17);
    }
}


void FrequencyCompilerHelper::ProcessVariables(Frequency& frequency,
                                               const std::vector<FrequencySymbolCompilation>& frequency_included_symbols,
                                               const std::optional<FrequencySymbolInclusionFlags>& frequency_included_flags,
                                               const std::vector<FrequencySymbolCompilation>& frequency_excluded_symbols,
                                               const std::optional<FrequencySymbolInclusionFlags>& frequency_excluded_flags,
                                               bool disjoint, std::optional<int> breakdown)
{
    struct ExpandedVariable
    {
        int symbol_index;
        std::optional<size_t> record_occurrence;
        std::vector<size_t> explicitly_specified_occurrences;
    };

    auto variable_is_valid_for_level = [&](const VART& vart)
    {
        const DICT* pDicT = vart.GetDPT();

        return ( pDicT == nullptr) ||
               ( pDicT->GetSubType() != SymbolSubType::Input ) ||
               ( vart.GetLevel() <= m_compiler.GetCompilationLevelNumber_base1() );
    };

    auto expand_variables = [&](const std::vector<FrequencySymbolCompilation>& frequency_symbols,
                                const std::optional<FrequencySymbolInclusionFlags> frequency_flags)
    {
        std::vector<ExpandedVariable> expanded_variables;

        for( const FrequencySymbolCompilation& frequency_symbol : frequency_symbols )
        {
            const Symbol& symbol = NPT_Ref(frequency_symbol.symbol_index);

            // add variable and work variables directly
            if( symbol.IsA(SymbolType::Variable) )
            {
                // check the level
                if( !variable_is_valid_for_level(assert_cast<const VART&>(symbol)) )
                    m_compiler.IssueError(MGF::Freq_access_from_invalid_level_94514, symbol.GetName().c_str(), m_compiler.GetCompilationLevelNumber_base1());

                expanded_variables.emplace_back(ExpandedVariable { symbol.GetSymbolIndex(), { }, frequency_symbol.occurrences });
            }

            else if( symbol.IsOneOf(SymbolType::WorkString, SymbolType::WorkVariable) )
            {
                expanded_variables.emplace_back(ExpandedVariable { symbol.GetSymbolIndex(), { }, { } });
            }

            else
            {
                // records may have an occurrence specified
                std::optional<size_t> record_occurrence;

                if( symbol.IsA(SymbolType::Section) && !frequency_symbol.occurrences.empty() )
                {
                    const SECT* pSecT = assert_cast<const SECT*>(&symbol);

                    if( pSecT->GetMaxOccs() <= 1 )
                        m_compiler.IssueError(MGF::Freq_invalid_occurrence_record_is_not_repeating_94516, pSecT->GetName().c_str());

                    bool valid_occurrence = ( frequency_symbol.occurrences.size() == 1 );

                    if( valid_occurrence )
                    {
                        size_t occurrence = frequency_symbol.occurrences.front();

                        if( occurrence != AllOccurrencesFlag )
                        {
                            if( occurrence >= 1 && occurrence <= static_cast<size_t>(pSecT->GetMaxOccs()) )
                            {
                                record_occurrence = occurrence;
                            }

                            else
                            {
                                valid_occurrence = false;
                            }
                        }
                    }

                    if( !valid_occurrence )
                        m_compiler.IssueError(MGF::Freq_invalid_record_occurrence_94517, pSecT->GetName().c_str(), pSecT->GetMaxOccs());
                }

                else
                {
                    ASSERT(frequency_symbol.occurrences.empty());
                }

                // add all variables from this dictionary/record/form/group/block
                int variables_added = VariableWorker(GetSymbolTable(), const_cast<Symbol*>(&symbol),
                    [&](VART* pVarT)
                    {
                        if( variable_is_valid_for_level(*pVarT) && AddVariableToFrequency(*pVarT, frequency_flags) )
                        {
                            expanded_variables.emplace_back(ExpandedVariable { pVarT->GetSymbolIndex(), record_occurrence, { } });
                            return 1;
                        }

                        return 0;
                    });

                if( variables_added == 0 )
                    m_compiler.IssueError(MGF::Freq_no_valid_variables_to_include_94515, symbol.GetName().c_str());
            }
        }

        return expanded_variables;
    };

    const std::vector<ExpandedVariable> included_expanded_variables = expand_variables(frequency_included_symbols, frequency_included_flags);
    const std::vector<ExpandedVariable> excluded_expanded_variables = expand_variables(frequency_excluded_symbols, frequency_excluded_flags);


    // at this point, the included/excluded variables will only consist of variables and working variables;
    // now we process their occurrences
    auto create_frequency_entries = [&](const std::vector<ExpandedVariable>& expanded_variables)
    {
        std::vector<FrequencyEntry> frequency_entries;

        for( const ExpandedVariable& expanded_variable : expanded_variables )
        {
            const Symbol& symbol = NPT_Ref(expanded_variable.symbol_index);

            // find a previous entry for this variable (or create a new one)
            auto frequency_entry_lookup = std::find_if(frequency_entries.begin(), frequency_entries.end(),
                                                       [&](const FrequencyEntry& frequency_entry) { return ( frequency_entry.symbol_index == expanded_variable.symbol_index ); });

            FrequencyEntry& frequency_entry = ( frequency_entry_lookup != frequency_entries.cend() ) ? *frequency_entry_lookup :
                frequency_entries.emplace_back(
                    FrequencyEntry
                    {
                        expanded_variable.symbol_index,
                        { },
                        ( IsString(symbol) ? std::move(breakdown) : std::nullopt )
                    }
                );


            // work variables...
            if( symbol.IsOneOf(SymbolType::WorkString, SymbolType::WorkVariable) )
                continue;

            // dictionary variables...
            ASSERT(symbol.IsA(SymbolType::Variable));
            const VART* pVarT = assert_cast<const VART*>(&symbol);
            const CDictItem* dict_item = pVarT->GetDictItem();
            ASSERT(dict_item != nullptr);


            // process the occurrences...
            size_t max_record_occurrences = dict_item->GetRecord()->GetMaxRecs();
            std::optional<size_t> specified_record_occurrence;
            bool combine_record_occurrences = ( max_record_occurrences == 1 );

            size_t max_item_subitem_occurrences = dict_item->GetItemSubitemOccurs();
            std::optional<size_t> specified_min_item_subitem_occurrence;
            std::optional<size_t> specified_max_item_subitem_occurrence;
            bool combine_item_subitem_occurrences = ( max_item_subitem_occurrences == 1 );

            // a record occurrence can be specified when adding a record
            if( expanded_variable.record_occurrence.has_value() )
            {
                ASSERT(expanded_variable.explicitly_specified_occurrences.empty() && max_record_occurrences > 1);
                specified_record_occurrence = *expanded_variable.record_occurrence - 1;
            }

            // otherwise occurrences are processed as follows:
            //      single record / single item/subitem:        invalid
            //      single record / repeating item/subitem:     (i/s)
            //      repeating record / single item/subitem:     (r)
            //      repeating record / repeating item/subitem:  (implicit r, i/s) or (r, i/s)
            else if( !expanded_variable.explicitly_specified_occurrences.empty() )
            {
                size_t occurrences_allowed = ( ( max_record_occurrences > 1 ) ? 1 : 0 ) +
                                             ( ( max_item_subitem_occurrences > 1 ) ? 1 : 0 );

                if( occurrences_allowed == 0 )
                {
                    m_compiler.IssueError(MGF::Freq_invalid_occurrence_item_is_not_repeating_94518, pVarT->GetName().c_str());
                }

                else if( expanded_variable.explicitly_specified_occurrences.size() > occurrences_allowed )
                {
                    m_compiler.IssueError(MGF::Freq_invalid_number_occurrences_94519, static_cast<int>(occurrences_allowed), pVarT->GetName().c_str());
                }

                // validate the record occurrence
                if( max_item_subitem_occurrences == 1 || expanded_variable.explicitly_specified_occurrences.size() == 2 )
                {
                    size_t record_occurrence = expanded_variable.explicitly_specified_occurrences.front();

                    if( record_occurrence == AllOccurrencesFlag )
                    {
                        combine_record_occurrences = true;
                    }

                    else
                    {
                        if( record_occurrence >= 1 && record_occurrence <= max_record_occurrences )
                        {
                            specified_record_occurrence = record_occurrence - 1;
                        }

                        else
                        {
                            m_compiler.IssueError(MGF::Freq_invalid_record_occurrence_94517, pVarT->GetName().c_str(), static_cast<int>(max_record_occurrences));
                        }
                    }
                }

                // validate the item/subitem occurrence
                if( max_item_subitem_occurrences > 1 )
                {
                    size_t item_subitem_occurrence = expanded_variable.explicitly_specified_occurrences.back();

                    // all occurrences will lead to the explicit adding of all item/subitem occurrences
                    if( item_subitem_occurrence == AllOccurrencesFlag )
                    {
                        combine_item_subitem_occurrences = true;
                    }

                    else if( item_subitem_occurrence >= 1 && item_subitem_occurrence <= static_cast<size_t>(max_item_subitem_occurrences) )
                    {
                        specified_min_item_subitem_occurrence = item_subitem_occurrence - 1;
                    }

                    else
                    {
                        m_compiler.IssueError(MGF::Freq_invalid_item_occurrence_94520, pVarT->GetName().c_str(), static_cast<int>(max_item_subitem_occurrences));
                    }
                }
            }

            // if no item/subitem occurrences are specified and this is disjoint, use all occurrences, otherwise combine all occurrences
            if( max_item_subitem_occurrences > 1 && !combine_item_subitem_occurrences && !specified_min_item_subitem_occurrence.has_value() )
            {
                if( disjoint )
                {
                    specified_min_item_subitem_occurrence = 0;
                    specified_max_item_subitem_occurrence = max_item_subitem_occurrences - 1;
                }

                else
                {
                    combine_item_subitem_occurrences = true;
                }
            }

            // finalize the record occurrence settings
            bool can_add_to_disjoint_entry = false;

            if( max_record_occurrences > 1 )
            {
                if( !combine_record_occurrences && ( disjoint || specified_record_occurrence.has_value() ) )
                {
                    can_add_to_disjoint_entry = true;
                }

                else
                {
                    combine_record_occurrences = true;
                }

                ASSERT(combine_record_occurrences != can_add_to_disjoint_entry);
            }


            // add the occurrence details...
            auto add_item_subitem_occurrence = [&](size_t min_item_subitem_occurrence, size_t max_item_subitem_occurrence)
            {
                // see if an existing occurrence details entry can be used for this variable
                bool add_new_occurrence_details_entry = true;

                for( FrequencyEntry::OccurrenceDetails& occurrence_details : frequency_entry.occurrence_details )
                {
                    if( occurrence_details.min_item_subitem_occurrence == min_item_subitem_occurrence &&
                        occurrence_details.max_item_subitem_occurrence == max_item_subitem_occurrence &&
                        occurrence_details.combine_record_occurrences == combine_record_occurrences )
                    {
                        add_new_occurrence_details_entry = false;

                        // when the record occurrences aren't combined, either set this to disjoint
                        // or add the specific record occurrence requested
                        if( !combine_record_occurrences )
                        {
                            if( specified_record_occurrence.has_value() )
                            {
                                occurrence_details.record_occurrences_to_explicitly_display.insert(*specified_record_occurrence);
                            }

                            else
                            {
                                occurrence_details.disjoint_record_occurrences = true;
                            }
                        }

                        break;
                    }
                }

                if( add_new_occurrence_details_entry )
                {
                    std::set<size_t> record_occurrences_to_explicitly_display;

                    if( specified_record_occurrence.has_value() )
                        record_occurrences_to_explicitly_display.insert(*specified_record_occurrence);

                    frequency_entry.occurrence_details.emplace_back(
                        FrequencyEntry::OccurrenceDetails
                        {
                            min_item_subitem_occurrence,
                            max_item_subitem_occurrence,
                            combine_record_occurrences,
                            ( can_add_to_disjoint_entry && record_occurrences_to_explicitly_display.empty() ),
                            record_occurrences_to_explicitly_display,
                            { }
                        });
                }
            };

            if( combine_item_subitem_occurrences )
            {
                add_item_subitem_occurrence(0, max_item_subitem_occurrences - 1);
            }

            else
            {
                ASSERT(specified_min_item_subitem_occurrence.has_value());

                for( size_t i = *specified_min_item_subitem_occurrence; i <= specified_max_item_subitem_occurrence.value_or(*specified_min_item_subitem_occurrence); ++i )
                    add_item_subitem_occurrence(i, i);
            }
        }

        return frequency_entries;
    };

    std::vector<FrequencyEntry> included_frequency_entries = create_frequency_entries(included_expanded_variables);
    const std::vector<FrequencyEntry> excluded_frequency_entries = create_frequency_entries(excluded_expanded_variables);


    // remove the excluded variables from the included variables
    for( const FrequencyEntry& excluded_frequency_entry : excluded_frequency_entries )
    {
        for( auto included_itr = included_frequency_entries.begin(); included_itr != included_frequency_entries.end(); )
        {
            FrequencyEntry& included_frequency_entry = *included_itr;
            bool remove_entry = false;

            if( included_frequency_entry.symbol_index == excluded_frequency_entry.symbol_index )
            {
                ASSERT(included_frequency_entry.occurrence_details.empty() == excluded_frequency_entry.occurrence_details.empty());

                for( const FrequencyEntry::OccurrenceDetails& excluded_occurrence_details : excluded_frequency_entry.occurrence_details )
                {
                    for( auto included_occurrence_itr = included_frequency_entry.occurrence_details.begin();
                              included_occurrence_itr != included_frequency_entry.occurrence_details.end(); )
                    {
                        FrequencyEntry::OccurrenceDetails& included_occurrence_details = *included_occurrence_itr;
                        bool remove_occurrence_entry = false;

                        if( included_occurrence_details.min_item_subitem_occurrence == excluded_occurrence_details.min_item_subitem_occurrence &&
                            included_occurrence_details.max_item_subitem_occurrence == excluded_occurrence_details.max_item_subitem_occurrence &&
                            included_occurrence_details.combine_record_occurrences == excluded_occurrence_details.combine_record_occurrences )
                        {
                            if( included_occurrence_details.combine_record_occurrences )
                            {
                                remove_occurrence_entry = true;
                            }

                            else
                            {
                                // if the record is disjoint, then all occurrences should be removed
                                if( excluded_occurrence_details.disjoint_record_occurrences )
                                {
                                    included_occurrence_details.record_occurrences_to_explicitly_display.clear();
                                }

                                // otherwise, if a record occurrence is specified for exclusion, remove it from the inclusion list and,
                                // if the record is disjoint, add it to the excluded list
                                else
                                {
                                    for( size_t record_occurrence_to_exclude : excluded_occurrence_details.record_occurrences_to_explicitly_display )
                                    {
                                        included_occurrence_details.record_occurrences_to_explicitly_display.erase(record_occurrence_to_exclude);

                                        if( included_occurrence_details.disjoint_record_occurrences )
                                            included_occurrence_details.record_occurrences_to_explicitly_exclude.insert(record_occurrence_to_exclude);
                                    }
                                }

                                // if there are no record occurrences to explicitly display, the entry can be removed
                                // if the included entry wasn't disjoint, or if the included/excluded disjoint setting matches
                                if( included_occurrence_details.record_occurrences_to_explicitly_display.empty() )
                                {
                                    if( !included_occurrence_details.disjoint_record_occurrences ||
                                        ( included_occurrence_details.disjoint_record_occurrences == excluded_occurrence_details.disjoint_record_occurrences ) )
                                    {
                                        remove_occurrence_entry = true;
                                    }
                                }
                            }
                        }

                        if( remove_occurrence_entry )
                        {
                            included_occurrence_itr = included_frequency_entry.occurrence_details.erase(included_occurrence_itr);
                        }

                        else
                        {
                            ++included_occurrence_itr;
                        }
                    }
                }

                // if there are no occurrence entries remaining, the variable can be removed
                remove_entry = included_frequency_entry.occurrence_details.empty();
            }

            if( remove_entry )
            {
                included_itr = included_frequency_entries.erase(included_itr);
            }

            else
            {
                ++included_itr;
            }
        }
    }

    // there must be at least one frequency variables
    if( included_frequency_entries.empty() )
        m_compiler.IssueError(MGF::Freq_no_variables_after_exclusions_94521);


    // mark all remaining included variables as used and add them to the frequency variable
    for( FrequencyEntry& included_frequency_entry : included_frequency_entries )
    {
        Symbol& symbol = NPT_Ref(included_frequency_entry.symbol_index);

        if( symbol.IsA(SymbolType::Variable) )
        {
            VART* pVarT = assert_cast<VART*>(&symbol);
            pVarT->SetUsed(true);


            // add all record occurrences for disjoint frequencies
            if( FrequencySetting::IncludeAllRecordOccurrencesWhenDisjoint )
            {
                for( FrequencyEntry::OccurrenceDetails& occurrence_details : included_frequency_entry.occurrence_details )
                {
                    if( occurrence_details.disjoint_record_occurrences )
                    {
                        ASSERT(pVarT->GetMaxOccsInDim(CDimension::VDimType::Record) > 1);

                        for( size_t i = 0; i < static_cast<size_t>(pVarT->GetMaxOccsInDim(CDimension::VDimType::Record)); ++i )
                        {
                            if( occurrence_details.record_occurrences_to_explicitly_exclude.find(i) ==
                                occurrence_details.record_occurrences_to_explicitly_exclude.cend() )
                            {
                                occurrence_details.record_occurrences_to_explicitly_display.insert(i);
                            }
                        }
                    }
                }
            }
        }


        frequency.AddFrequencyEntry(included_frequency_entry);
    }
}


void FrequencyCompilerHelper::ValidateValueSets(const Frequency& frequency, const std::vector<int>& value_set_symbol_indices)
{
    // warn if any value sets aren't part of the frequency's entries
    for( int symbol_index : value_set_symbol_indices )
    {
        const ValueSet& value_set = GetSymbolValueSet(symbol_index);
        bool found_match = false;

        for( const FrequencyEntry& frequency_entry : frequency.GetFrequencyEntries() )
        {
            if( value_set.GetVarT()->GetSymbolIndex() == frequency_entry.symbol_index )
            {
                found_match = true;
                break;
            }
        }

        if( !found_match )
            m_compiler.IssueWarning(MGF::Freq_value_set_not_associated_with_freq_94524, value_set.GetName().c_str());
    }
}



// --------------------------------------------------------------------------
// LogicCompiler's frequency compilers
// --------------------------------------------------------------------------

int LogicCompiler::CompileFrequencyDeclaration()
{
    ASSERT(Tkn == TOKKWFREQ);

    size_t frequency_index = m_engineData->frequencies.size();
    std::shared_ptr<Frequency> frequency = m_engineData->frequencies.emplace_back(std::make_unique<Frequency>());
    std::shared_ptr<NamedFrequency> named_frequency;

    std::optional<size_t> command_type = NextKeyword(FrequencyCommands);

    // if no command is provided, see if this is a named frequency
    if( *command_type == 0 )
    {
        MarkInputBufferToRestartLater();

        NextTokenOrNewSymbolName();

        if( Tkn == TOKNEWSYMBOL )
        {
            named_frequency = std::make_shared<NamedFrequency>(Tokstr);
            m_engineData->AddSymbol(named_frequency);

            named_frequency->SetFrequencyIndex(frequency_index);
            frequency->SetNamedFrequencySymbolIndex(named_frequency->GetSymbolIndex());

            ClearMarkedInputBuffer();
            command_type.reset();
        }

        else
        {
            RestartFromMarkedInputBuffer();
        }
    }

    bool using_named_frequency = ( named_frequency != nullptr );

    // unnamed frequencies can't be declared in PROC GLOBAL
    if( !using_named_frequency && IsGlobalCompilation() )
        IssueError(MGF::Freq_unnamed_freqs_not_allowed_in_proc_global_94500);

    Nodes::FrequencyParameters frequency_parameters_node;
    std::vector<FrequencySymbolCompilation> frequency_included_symbols;
    std::vector<FrequencySymbolCompilation> frequency_excluded_symbols;
    std::optional<FrequencySymbolInclusionFlags> frequency_included_flags;
    std::optional<FrequencySymbolInclusionFlags> frequency_excluded_flags;
    bool disjoint = false;
    std::optional<int> breakdown;
    int universe_expression = -1;
    int weight_expression = -1;

    FrequencyCompilerHelper frequency_compiler_helper(*this);

    frequency_compiler_helper.CompileFreqCommands(command_type, frequency_parameters_node, true,
        [&](size_t command_type, std::optional<std::variant<int, const std::vector<std::wstring>*, const std::vector<int>*>> argument)
        {
            // INCLUDE / EXCLUDE
            if( bool include = ( command_type == CI_INCLUDE ); include || command_type == CI_EXCLUDE )
            {
                std::vector<FrequencySymbolCompilation>& frequency_symbols = include ? frequency_included_symbols : frequency_excluded_symbols;
                std::optional<FrequencySymbolInclusionFlags>& frequency_flags = include ? frequency_included_flags : frequency_excluded_flags;

                ASSERT(Tkn == TOKLPAREN);
                bool allow_comma = false;
                bool potentially_allow_occurrences = false;

                while( true )
                {
                    bool saved_potentially_allow_occurrences = potentially_allow_occurrences;
                    potentially_allow_occurrences = false;

                    // process the optional variable inclusion flags
                    size_t flag_type = NextKeyword({ _T("short"), _T("long"), _T("integer"), _T("float"), _T("numeric"), _T("alpha") });

                    if( flag_type != 0 )
                    {
                        if( !frequency_flags.has_value() )
                            frequency_flags.emplace(FrequencySymbolInclusionFlags { false });

                        frequency_flags->include_shorts |= ( flag_type == 1 || flag_type == 3 || flag_type == 5 );
                        frequency_flags->include_longs  |= ( flag_type == 2 || flag_type == 3 || flag_type == 5 );
                        frequency_flags->include_floats |= ( flag_type == 4 || flag_type == 5 );
                        frequency_flags->include_alphas |= ( flag_type == 6 );
                    }

                    else
                    {
                        NextToken();

                        // return if done
                        if( Tkn == TOKRPAREN )
                        {
                            return;
                        }

                        // commas can separate each symbol
                        else if( Tkn == TOKCOMMA && allow_comma )
                        {
                            allow_comma = false;
                            continue;
                        }

                        // process occurrences
                        else if( Tkn == TOKLPAREN )
                        {
                            // verify that this symbol can have occurrences (though stricter occurrence checking will occur later)
                            if( saved_potentially_allow_occurrences )
                            {
                                ASSERT(!frequency_symbols.empty());
                                const Symbol& symbol = NPT_Ref(frequency_symbols.back().symbol_index);

                                saved_potentially_allow_occurrences =
                                    symbol.IsA(SymbolType::Section) ||
                                    ( symbol.IsA(SymbolType::Variable) && assert_cast<const VART&>(symbol).GetDictItem() != nullptr );
                            }

                            if( !saved_potentially_allow_occurrences )
                                IssueError(MGF::single_variable_cannot_have_subscript_25);

                            // read in the constant occurrence numbers, which will be validated later
                            while( true )
                            {
                                NextToken();

                                if( Tkn == TOKCTE && Tokvalue > 0 && IsNumericConstantInteger() )
                                {
                                    frequency_symbols.back().occurrences.emplace_back(static_cast<size_t>(Tokvalue));
                                }

                                // * will be used to signify all occurrences
                                else if( Tkn == TOKMULOP )
                                {
                                    frequency_symbols.back().occurrences.emplace_back(AllOccurrencesFlag);
                                }

                                else
                                {
                                    IssueError(MGF::Freq_occurrence_number_invalid_94512, NPT_Ref(frequency_symbols.back().symbol_index).GetName().c_str());
                                }

                                NextToken();

                                if( Tkn == TOKRPAREN )
                                    break;

                                IssueErrorOnTokenMismatch(TOKCOMMA, MGF::right_parenthesis_expected_19);
                            }
                        }

                        // symbols can include dictionaries, records, and items; forms, groups, and blocks; and work strings
                        else if( Tkn == TOKDICT_PRE80 || Tkn == TOKSECT  || Tkn == TOKVAR   ||
                                 Tkn == TOKFORM       || Tkn == TOKGROUP || Tkn == TOKBLOCK ||
                                 Tkn == TOKWORKSTRING )
                        {
                            frequency_symbols.emplace_back(FrequencySymbolCompilation { Tokstindex });
                            potentially_allow_occurrences = true;
                        }

                        else
                        {
                            IssueError(MGF::Freq_command_requires_variable_94511, FrequencyCommands[command_type - 1]);
                        }
                    }

                    allow_comma = true;
                }
            }


            // UNIVERSE / WEIGHT
            else if( bool universe = ( command_type == CI_UNIVERSE); universe || command_type == CI_WEIGHT )
            {
                if( using_named_frequency )
                    IssueError(MGF::Freq_command_cannot_be_used_with_named_freqs_94503, FrequencyCommands[command_type - 1], named_frequency->GetName().c_str());

                if( universe )
                {
                    universe_expression = std::get<int>(*argument);
                }

                else
                {
                    weight_expression = std::get<int>(*argument);
                }
            }

            // DISJOINT
            else if( command_type == CI_DISJOINT )
            {
                disjoint = true;
            }

            // BREAKDOWN
            else if( command_type == CI_BREAKDOWN )
            {
                breakdown = std::get<int>(*argument);
            }

            // HEADING
            else if( command_type == CI_HEADING )
            {
                frequency->GetFrequencyPrinterOptions().SetHeadings(*std::get<const std::vector<std::wstring>*>(*argument));
            }

            // VALUESET
            else if( command_type == CI_VALUESET )
            {
                frequency->GetFrequencyPrinterOptions().SetValueSetSymbolIndices(*std::get<const std::vector<int>*>(*argument));
            }

            else
            {
                ASSERT(false);
            }
        });

    IssueErrorOnTokenMismatch(TOKSEMICOLON, MGF::expecting_semicolon_30);

    frequency->GetFrequencyPrinterOptions().ApplyFrequencyParametersNode(frequency_parameters_node);

    // if no variables are included, use the variables from the main input dictionary
    if( frequency_included_symbols.empty() )
    {
        // but don't allow exclusions without any inclusions
        if( !frequency_excluded_symbols.empty() && !frequency_included_flags.has_value() )
            IssueError(MGF::Freq_exclude_cannot_be_used_without_include_94513);

        frequency_included_symbols.emplace_back(FrequencySymbolCompilation { GetInputDictionary(true)->GetSymbolIndex() });
    }


    // once all commands have been read, determine the full set of frequency variables and validate them
    frequency_compiler_helper.ProcessVariables(*frequency, frequency_included_symbols, frequency_included_flags,
                                               frequency_excluded_symbols, frequency_excluded_flags, disjoint, breakdown);

    // also validate any value sets used
    frequency_compiler_helper.ValidateValueSets(*frequency, frequency->GetFrequencyPrinterOptions().GetValueSetSymbolIndices());


    // if this is a named frequency, then we need to setup the symbol reset node
    if( using_named_frequency )
    {
        Nodes::SymbolReset* symbol_reset_node = nullptr;
        int& initialize_value = AddSymbolResetNode(symbol_reset_node, *named_frequency);
        initialize_value = frequency_parameters_node.heading_expressions_list_node;

        return GetOptionalProgramIndex(symbol_reset_node);
    }

    // otherwise we create an unnamed frequency node
    else
    {
        auto& unnamed_frequency_node = CreateNode<Nodes::UnnamedFrequency>(FunctionCode::FREQ_UNNAMED_CODE);

        unnamed_frequency_node.next_st = -1;
        unnamed_frequency_node.frequency_index = frequency_index;
        unnamed_frequency_node.universe_expression = universe_expression;
        unnamed_frequency_node.weight_expression = weight_expression;
        unnamed_frequency_node.heading_expressions_list_node = frequency_parameters_node.heading_expressions_list_node;

        // mark this application as having saveable frequency statements
        if( m_engineData->application != nullptr )
            m_engineData->application->SetHasSaveableFrequencyStatements();

        return GetProgramIndex(unnamed_frequency_node);
    }
}


int LogicCompiler::CompileNamedFrequencyComputeInstruction()
{
    ASSERT(Tkn == TOKFREQ);

    auto& symbol_compute_node = CreateNode<Nodes::SymbolCompute>(FunctionCode::FREQFN_COMPUTE_CODE);

    symbol_compute_node.next_st = -1;
    symbol_compute_node.rhs_symbol_type = SymbolType::None; // unused for now
    symbol_compute_node.lhs_symbol_index = CompileNamedFrequencyReference();

    IssueErrorOnTokenMismatch(TOKEQOP, MGF::equals_expected_in_assignment_5);

    NextToken();

    symbol_compute_node.rhs_symbol_index = exprlog();

    return GetProgramIndex(symbol_compute_node);
}


int LogicCompiler::CompileNamedFrequencyReference()
{
    ASSERT(Tkn == TOKFREQ);
    const NamedFrequency& named_frequency = GetSymbolLogicNamedFrequency(Tokstindex);

    // though there may be only a single frequency variable passed into a user-defined function,
    // at compile-time we don't know this, so disallow any access for function parameters
    if( named_frequency.IsFunctionParameter() )
        IssueError(MGF::Freq_individual_tally_access_not_allowed_94522, named_frequency.GetName().c_str());

    const Frequency& frequency = *m_engineData->frequencies[named_frequency.GetFrequencyIndex()];

    // only allow access and assignment to named frequencies that will only result in one frequency counter
    std::optional<int> single_frequency_variable_symbol_index = frequency.GetSymbolIndexOfSingleFrequencyVariable();

    if( !single_frequency_variable_symbol_index.has_value() )
        IssueError(MGF::Freq_individual_tally_access_not_allowed_94522, named_frequency.GetName().c_str());

    NextToken();
    IssueErrorOnTokenMismatch(TOKLPAREN, MGF::left_parenthesis_expected_in_function_call_14);

    NextToken();

    int subscript_expression = IsNumeric(NPT_Ref(*single_frequency_variable_symbol_index)) ? exprlog() :
                                                                                             CompileStringExpression();

    IssueErrorOnTokenMismatch(TOKRPAREN, MGF::right_parenthesis_expected_in_function_call_17);

    NextToken();

    auto& element_reference_node = CreateNode<Nodes::ElementReference>(FunctionCode::FREQVAR_CODE);

    element_reference_node.symbol_index = named_frequency.GetSymbolIndex();
    element_reference_node.element_expressions[0] = subscript_expression;

    return GetProgramIndex(element_reference_node);
}


int LogicCompiler::CompileNamedFrequencyFunctions()
{
    // compiling freq_name.clear();
    //           freq_name.save([filename | report_name]) [commands];
    //           freq_name.tally([weight]);
    //           freq_name.view([viewer options]) [commands];

    const FunctionCode function_code = CurrentToken.function_details->code;
    const NamedFrequency& named_frequency = assert_cast<const NamedFrequency&>(*CurrentToken.symbol);
    std::vector<int> arguments(CurrentToken.function_details->number_arguments, -1);

    NextToken();
    IssueErrorOnTokenMismatch(TOKLPAREN, MGF::left_parenthesis_expected_in_function_call_14);

    if( function_code != FunctionCode::FREQFN_VIEW_CODE )
        NextToken();

    bool application_has_saveable_frequency_statements = ( function_code == FunctionCode::FREQFN_SAVE_CODE );

    // read the optional argument for save/tally
    if( Tkn != TOKRPAREN && ( function_code == FunctionCode::FREQFN_SAVE_CODE ||
                              function_code == FunctionCode::FREQFN_TALLY_CODE ) )
    {
        if( function_code == FunctionCode::FREQFN_SAVE_CODE )
        {
            application_has_saveable_frequency_statements = false;

            if( Tkn == TOKREPORT )
            {
                const Report& report = GetSymbolReport(Tokstindex);
                CheckReportIsCurrentlyWriteable(report);

                if( !report.IsHtmlType() && !report.IsFunctionParameter() )
                    IssueError(MGF::Freq_cannot_be_saved_to_non_HTML_report_94533, named_frequency.GetName().c_str(), report.GetName().c_str());

                arguments[0] = static_cast<int>(SymbolType::Report);
                arguments[1] = report.GetSymbolIndex();

                NextToken();
            }

            else
            {
                arguments[0] = -1;
                arguments[1] = CompileStringExpression();
            }
        }

        else
        {
            ASSERT(function_code == FunctionCode::FREQFN_TALLY_CODE);

            arguments[0] = exprlog();
        }
    }

    // view can take optional viewer options
    else if( function_code == FunctionCode::FREQFN_VIEW_CODE )
    {
        arguments[0] = CompileViewerOptions(true);

        if( arguments[0] == -1 )
            NextToken();
    }


    // mark this application as having saveable frequency statements if save was used without an argument
    if( application_has_saveable_frequency_statements && m_engineData->application != nullptr )
        m_engineData->application->SetHasSaveableFrequencyStatements();


    IssueErrorOnTokenMismatch(TOKRPAREN, MGF::right_parenthesis_expected_in_function_call_17);


    // save and view have some optional formatting commands
    if( function_code == FunctionCode::FREQFN_SAVE_CODE || function_code == FunctionCode::FREQFN_VIEW_CODE )
    {
        auto& frequency_parameters_node = CreateNode<Nodes::FrequencyParameters>();
        std::vector<int> value_set_symbol_indices;

        FrequencyCompilerHelper frequency_compiler_helper(*this);

        frequency_compiler_helper.CompileFreqCommands(std::nullopt, frequency_parameters_node, false,
            [&](size_t command_type, std::optional<std::variant<int, const std::vector<std::wstring>*, const std::vector<int>*>> argument)
            {
                if( command_type == CI_VALUESET )
                {
                    value_set_symbol_indices = *std::get<const std::vector<int>*>(*argument);
                    frequency_parameters_node.value_sets_list_node = CreateListNode(value_set_symbol_indices);
                }

                else
                {
                    IssueError(MGF::Freq_improper_use_of_formatting_commands_94510);
                }
            });

        // validate any value sets used
        if( !named_frequency.IsFunctionParameter() )
        {
            const Frequency& frequency = *m_engineData->frequencies[named_frequency.GetFrequencyIndex()];
            frequency_compiler_helper.ValidateValueSets(frequency, value_set_symbol_indices);
        }

        arguments.back() = GetProgramIndex(frequency_parameters_node);
    }

    else
    {
        NextToken();
    }

    return CreateSymbolVariableArgumentsNode(function_code, named_frequency, arguments);
}
