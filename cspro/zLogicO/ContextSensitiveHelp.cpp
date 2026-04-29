#include "stdafx.h"
#include "ContextSensitiveHelp.h"
#include "KeywordTable.h"
#include "ReservedWords.h"
#include "SpecialFunction.h"

using namespace Logic;


namespace
{
    struct MultipleWordExpressionDetails
    {
        const char* const name;
        const char* const second_name;
        const char* const help_filename;
    };

    const MultipleWordExpressionDetails MultipleWordExpressions[] =
    {
        { "Array",      "alpha",        "Array_statement.html" },
        { "Array",      "numeric",      "Array_statement.html" },
        { "Array",      "string",       "Array_statement.html" },
        { "ask",        "if",           "ask_statement.html" },
        { "HashMap",    "numeric",      "HashMap_statement.html" },
        { "HashMap",    "string",       "HashMap_statement.html" },
        { "List",       "numeric",      "List_statement.html" },
        { "List",       "string",       "List_statement.html" },
        { "PROC",       "GLOBAL",       "cspro_program_structure.html" },
        { "set",        "access",       "set_access_statement.html" },
        { "set",        "attributes",   "set_attributes_statement.html" },
        { "set",        "behavior",     "set_behavior_export_statement.html" },
        { "set",        "errmsg",       "set_errmsg_function.html" },
        { "set",        "first",        "set_first_statement.html" },
        { "set",        "last",         "set_last_statement.html" },
        { "skip",       "case",         "skip_case_statement.html" },
        { "ValueSet",   "numeric",      "ValueSet_statement.html" },
        { "ValueSet",   "string",       "ValueSet_statement.html" },
    };

    const MultipleReservedWordsTable<MultipleWordExpressionDetails>& GetMultipleWordExpressions()
    {
        static const MultipleReservedWordsTable<MultipleWordExpressionDetails> multiple_word_expressions_table(cs::span<const MultipleWordExpressionDetails>(
                                                                                                               MultipleWordExpressions, MultipleWordExpressions + _countof(MultipleWordExpressions)));
        return multiple_word_expressions_table;
    }
}


const char* const ContextSensitiveHelp::GetTopicFilename(const std::string_view text_sv, const FunctionDetails** function_details/* = nullptr*/)
{
    const KeywordDetails* keyword_details;

    if( KeywordTable::IsKeyword(text_sv, &keyword_details) )
        return keyword_details->help_filename;

    const FunctionNamespaceDetails* function_namespace_details;

    if( FunctionTable::IsFunctionNamespace(text_sv, SymbolType::None, &function_namespace_details) )
        return function_namespace_details->help_filename;

    const FunctionDetails* local_function_details;
    const FunctionDetails** function_details_to_use = ( function_details != nullptr ) ? function_details :
                                                                                        &local_function_details;

    if( FunctionTable::IsFunction(text_sv, SymbolType::None, function_details_to_use) )
        return (*function_details_to_use)->help_filename;

    const AdditionalReservedWordDetails* additional_reserved_word_details;

    if( ReservedWords::GetAdditionalReservedWords().IsEntry(text_sv, &additional_reserved_word_details) )
        return additional_reserved_word_details->help_filename;

    const SpecialFunction::Definition* const special_function = SpecialFunction::Lookup(text_sv);

    if( special_function != nullptr )
        return special_function->help_filename;

    return nullptr;
}


const char* const ContextSensitiveHelp::GetTopicFilename(const cs::span<const std::string> dot_notation_entries, const std::string_view text_sv, const FunctionDetails** function_details)
{
    // a routine for dot notation words
    ASSERT(!dot_notation_entries.empty());
    ASSERT(function_details != nullptr);

    std::variant<SymbolType, FunctionNamespace> symbol_type_or_function_namespace = SymbolType::None;
    const FunctionNamespaceDetails* function_namespace_details;

    for( const std::string& dot_notation_entry : dot_notation_entries )
    {
        // return if this is not a function namespace
        if( !FunctionTable::IsFunctionNamespace(dot_notation_entry, symbol_type_or_function_namespace, &function_namespace_details) )
            return nullptr;

        symbol_type_or_function_namespace = function_namespace_details->function_namespace;
    }

    ASSERT(symbol_type_or_function_namespace != SymbolType::None);

    // check if this is a function
    if( FunctionTable::IsFunction(text_sv, symbol_type_or_function_namespace, function_details) )
        return (*function_details)->help_filename;

    // check if this is a function namespace
    if( FunctionTable::IsFunctionNamespace(text_sv, symbol_type_or_function_namespace, &function_namespace_details) )
        return function_namespace_details->help_filename;

    return nullptr;
}


const char* const ContextSensitiveHelp::GetIntroductionTopicFilename()
{
    return "introduction_to_cspro_language.html";
}


bool ContextSensitiveHelp::UpdateTopicFilenameForMultipleWordExpressions(const std::string_view text_sv, const std::string_view second_text_sv, const char** help_topic_filename)
{
    ASSERT(help_topic_filename != nullptr);
    const MultipleWordExpressionDetails* multiple_word_expression_details = nullptr;

    if( GetMultipleWordExpressions().IsEntry(text_sv, &multiple_word_expression_details,
                                             [&](const MultipleWordExpressionDetails& details) { return SO::EqualsNoCase(second_text_sv, details.second_name); }) )
    {
        *help_topic_filename = multiple_word_expression_details->help_filename;
        return true;
    }

    return false;
}
