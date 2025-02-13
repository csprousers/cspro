#include "stdafx.h"
#include "ContextSensitiveHelp.h"
#include "KeywordTable.h"
#include "ReservedWords.h"

using namespace Logic;


namespace
{
    struct MultipleWordExpressionDetails
    {
        const TCHAR* const name;
        const StringNoCase second_name;
        const TCHAR* const help_filename;
    };

    const MultipleWordExpressionDetails MultipleWordExpressions[] =
    {
        { _T("Array"),    _T("alpha"),      _T("Array_statement.html") },
        { _T("Array"),    _T("numeric"),    _T("Array_statement.html") },
        { _T("Array"),    _T("string"),     _T("Array_statement.html") },
        { _T("ask"),      _T("if"),         _T("ask_statement.html") },
        { _T("HashMap"),  _T("numeric"),    _T("HashMap_statement.html") },
        { _T("HashMap"),  _T("string"),     _T("HashMap_statement.html") },
        { _T("List"),     _T("numeric"),    _T("List_statement.html") },
        { _T("List"),     _T("string"),     _T("List_statement.html") },
        { _T("PROC"),     _T("GLOBAL"),     _T("cspro_program_structure.html") },
        { _T("set"),      _T("access"),     _T("set_access_statement.html") },
        { _T("set"),      _T("attributes"), _T("set_attributes_statement.html") },
        { _T("set"),      _T("behavior"),   _T("set_behavior_export_statement.html") },
        { _T("set"),      _T("errmsg"),     _T("set_errmsg_function.html") },
        { _T("set"),      _T("first"),      _T("set_first_statement.html") },
        { _T("set"),      _T("last"),       _T("set_last_statement.html") },
        { _T("skip"),     _T("case"),       _T("skip_case_statement.html") },
        { _T("ValueSet"), _T("numeric"),    _T("ValueSet_statement.html") },
        { _T("ValueSet"), _T("string"),     _T("ValueSet_statement.html") },
    };

    const MultipleReservedWordsTable<MultipleWordExpressionDetails>& GetMultipleWordExpressions()
    {
        static const MultipleReservedWordsTable<MultipleWordExpressionDetails> multiple_word_expressions_table(cs::span<const MultipleWordExpressionDetails>(
                                                                                                               MultipleWordExpressions, MultipleWordExpressions + _countof(MultipleWordExpressions)));
        return multiple_word_expressions_table;
    }
}


const TCHAR* const ContextSensitiveHelp::GetTopicFilename(wstring_view text, const FunctionDetails** function_details/* = nullptr*/)
{
    const KeywordDetails* keyword_details;
    const FunctionNamespaceDetails* function_namespace_details;
    const FunctionDetails* local_function_details;
    const FunctionDetails** function_details_to_use = ( function_details != nullptr ) ? function_details : &local_function_details;
    const AdditionalReservedWordDetails* additional_reserved_word_details;

    return KeywordTable::IsKeyword(text, &keyword_details)                                              ? keyword_details->help_filename :
           FunctionTable::IsFunctionNamespace(text, SymbolType::None, &function_namespace_details)      ? function_namespace_details->help_filename :
           FunctionTable::IsFunction(text, SymbolType::None, function_details_to_use)                   ? (*function_details_to_use)->help_filename :
           ReservedWords::GetAdditionalReservedWords().IsEntry(text, &additional_reserved_word_details) ? additional_reserved_word_details->help_filename :
           ReservedWords::GetSpecialFunctions().IsEntry(text, &additional_reserved_word_details)        ? additional_reserved_word_details->help_filename :
                                                                                                          nullptr;
}


const TCHAR* const ContextSensitiveHelp::GetTopicFilename(cs::span<const std::wstring> dot_notation_entries, wstring_view text, const FunctionDetails** function_details)
{
    // a routine for dot notation words
    ASSERT(!dot_notation_entries.empty());
    ASSERT(function_details != nullptr);

    std::variant<SymbolType, FunctionNamespace> symbol_type_or_function_namespace = SymbolType::None;
    const FunctionNamespaceDetails* function_namespace_details;

    for( const std::wstring& dot_notation_entry : dot_notation_entries )
    {
        // return if this is not a function namespace
        if( !FunctionTable::IsFunctionNamespace(dot_notation_entry, symbol_type_or_function_namespace, &function_namespace_details) )
            return nullptr;

        symbol_type_or_function_namespace = function_namespace_details->function_namespace;
    }

    ASSERT(symbol_type_or_function_namespace != SymbolType::None);

    // check if this is a function
    if( FunctionTable::IsFunction(text, symbol_type_or_function_namespace, function_details) )
        return (*function_details)->help_filename;

    // check if this is a function namespace
    if( FunctionTable::IsFunctionNamespace(text, symbol_type_or_function_namespace, &function_namespace_details) )
        return function_namespace_details->help_filename;

    return nullptr;        
}


const TCHAR* const ContextSensitiveHelp::GetIntroductionTopicFilename()
{
    return _T("introduction_to_cspro_language.html");
}


bool ContextSensitiveHelp::UpdateTopicFilenameForMultipleWordExpressions(wstring_view text, wstring_view second_text, const TCHAR** help_topic_filename)
{
    ASSERT(help_topic_filename != nullptr);
    const MultipleWordExpressionDetails* multiple_word_expression_details = nullptr;

    if( GetMultipleWordExpressions().IsEntry(text, &MultipleWordExpressionDetails::second_name, StringNoCase(second_text), &multiple_word_expression_details) )
    {
        *help_topic_filename = multiple_word_expression_details->help_filename;
        return true;
    }

    return false;
}
