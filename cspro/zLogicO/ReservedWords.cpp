#include "stdafx.h"
#include "ReservedWords.h"
#include "ChildSymbolNames.h"
#include "KeywordTable.h"
#include "SpecialFunction.h"
#include <zCapiO/CapiLogicParameters.h>

using namespace Logic;


namespace
{
    const AdditionalReservedWordDetails AdditionalReservedWords[] =
    {
        { "GLOBAL",                     "cspro_program_structure.html" },
        { "PROC",                       "proc_statement.html" },
        { "summary",                    "errmsg_function.html" },
        { "denom",                      "errmsg_function.html" },
        { "disjoint",                   "Freq_statement_unnamed.html" },
        { "weight",                     "Freq_statement_unnamed.html" },
        { "specific",                   "impute_function.html" },
        { "outofrange",                 "set_behavior_canenter_statement.html" },
        { "confirm",                    "set_behavior_canenter_statement.html" },
        { "noconfirm",                  "set_behavior_canenter_statement.html" },
        { "specialvalues",              "set_behavior_specialvalues_statement.html" },
        { QuestionTextStringWriterName, "QSF_object.html" },

        // words that are used in the CSPro DB tables
        { "cases",          nullptr },
        { "file_revisions", nullptr },
        { "meta",           nullptr },
        { "notes",          nullptr },
        { "occ",            nullptr },
        { "sync_history",   nullptr },
        { "vector_clock",   nullptr },
    };


    const AdditionalReservedWordDetails SpecialFunctions[] =
    {
        { ToString(SpecialFunction::OnChangeLanguage),      "OnChangeLanguage_global_function.html" },
        { ToString(SpecialFunction::OnChar),                "OnChar_global_function.html" },
        { ToString(SpecialFunction::OnKey),                 "OnKey_global_function.html" },
        { ToString(SpecialFunction::OnStop),                "OnStop_global_function.html" },
        { ToString(SpecialFunction::OnSyncMessage),         "syncmessage_function.html" },
        { ToString(SpecialFunction::OnRefused),             "refused_value.html" },
        { ToString(SpecialFunction::OnSystemMessage),       "OnSystemMessage_global_function.html" },
        { ToString(SpecialFunction::OnViewQuestionnaire),   "OnViewQuestionnaire_global_function.html" },
        { ToString(SpecialFunction::OnActionInvokerResult), "CS_OnActionInvokerResult.html" },
    };
}


bool ReservedWords::IsReservedWord(const std::string_view text_sv)
{
    return ( KeywordTable::IsKeyword(text_sv) ||
             FunctionTable::IsFunctionNamespace(text_sv, SymbolType::None) ||
             FunctionTable::IsFunction(text_sv, SymbolType::None) ||
             GetAdditionalReservedWords().IsEntry(text_sv, nullptr) );
}


void ReservedWords::ForeachReservedWord(const std::function<void(ReservedWordType, const std::string&, const void*)>& callback_function)
{
    for( const auto& [text, entry_details] : KeywordTable::GetKeywords().GetTable() )
    {
        if( ( entry_details->token_code == TokenCode::TOKRECODE && SO::EqualsNoCase(text, "box") ) ||
            ( entry_details->token_code == TokenCode::TOKENDRECODE && SO::EqualsNoCase(text, "endbox") ) )
        {
            // don't add deprecated words
            continue;
        }

        callback_function(ReservedWordType::Keyword, text, nullptr);
    }

    for( const auto& [text, entry_details] : FunctionTable::GetFunctionNamespaces().GetTable() )
    {
        for( const FunctionNamespaceDetails& function_namespace_details : VI_V(entry_details) )
        {
            const ReservedWordType reserved_word_type = function_namespace_details.parent_function_namespace.has_value() ? ReservedWordType::FunctionNamespaceChild :
                                                                                                                           ReservedWordType::FunctionNamespace;
            callback_function(reserved_word_type, text, nullptr);
        }
    }

    for( const FunctionDetails& function_details : VI_V(FunctionTable::GetFunctions()) )
    {
        const ReservedWordType reserved_word_type = ( function_details.function_domain == SymbolType::None ) ? ReservedWordType::Function :
                                                                                                               ReservedWordType::FunctionDotNotation;
        callback_function(reserved_word_type, function_details.name, &function_details);
    }

    for( const auto& [text, entry_details] : GetAdditionalReservedWords().GetTable() )
        callback_function(ReservedWordType::AdditionalReservedWord, text, nullptr);
}


const std::vector<std::string>& ReservedWords::GetAllReservedWords()
{
    auto get_reserved_words = []()
    {
        std::vector<std::string> reserved_words;

        ForeachReservedWord(
            [&](const ReservedWordType reserved_word_type, const std::string& reserved_word, const void*)
            {
                if( reserved_word_type == ReservedWordType::FunctionNamespaceChild ||
                    reserved_word_type == ReservedWordType::FunctionDotNotation )
                {
                    return;
                }

                ASSERT(std::find(reserved_words.cbegin(), reserved_words.cend(), reserved_word) == reserved_words.cend());
                reserved_words.emplace_back(reserved_word);
            });

        return reserved_words;
    };

    static const std::vector<std::string> reserved_words = get_reserved_words();
    return reserved_words;
}


const ReservedWordsTable<AdditionalReservedWordDetails>& ReservedWords::GetAdditionalReservedWords()
{
    static const ReservedWordsTable<AdditionalReservedWordDetails> additional_reserved_words(cs::span<const AdditionalReservedWordDetails>(
                                                                                             AdditionalReservedWords, AdditionalReservedWords + _countof(AdditionalReservedWords)));
    return additional_reserved_words;
}


const ReservedWordsTable<AdditionalReservedWordDetails>& ReservedWords::GetSpecialFunctions()
{
    static const ReservedWordsTable<AdditionalReservedWordDetails> special_functions_table(cs::span<const AdditionalReservedWordDetails>(
                                                                                           SpecialFunctions, SpecialFunctions + _countof(SpecialFunctions)));
    return special_functions_table;
}


template<typename T>
const char* ReservedWords::GetDefinedCaseWorker(const std::string_view text_sv, const T& function_domain)
{
    const FunctionDetails* function_details;

    if( FunctionTable::IsFunctionExtended(text_sv, function_domain, &function_details) )
    {
        return function_details->name;
    }

    if constexpr(std::is_same_v<T, SymbolType> ||
                 std::is_same_v<T, FunctionNamespace>)
    {
        const KeywordDetails* keyword_details;
        const FunctionNamespaceDetails* function_namespace_details;
        const AdditionalReservedWordDetails* additional_reserved_word_details;
        const char* child_symbol_name;

        return KeywordTable::IsKeyword(text_sv, &keyword_details)                                        ? keyword_details->name :
               FunctionTable::IsFunctionNamespace(text_sv, function_domain, &function_namespace_details) ? function_namespace_details->name :
               GetAdditionalReservedWords().IsEntry(text_sv, &additional_reserved_word_details)          ? additional_reserved_word_details->name :
               GetSpecialFunctions().IsEntry(text_sv, &additional_reserved_word_details)                 ? additional_reserved_word_details->name :
               ( ( child_symbol_name = LookupChildSymbolName(text_sv, function_domain) ) != nullptr )    ? child_symbol_name :
                                                                                                           nullptr;
    }

    else
    {
        return nullptr;
    }
}


const char* ReservedWords::GetDefinedCase(const std::string_view text_sv, const FunctionDomain& function_domain)
{
    return std::visit([&](const auto& value) { return GetDefinedCaseWorker(text_sv, value); },
                      function_domain);
}
