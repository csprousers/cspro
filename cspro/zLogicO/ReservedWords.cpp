#include "stdafx.h"
#include "ReservedWords.h"
#include "ChildSymbolNames.h"
#include "KeywordTable.h"
#include "SpecialFunction.h"

using namespace Logic;


namespace
{
    const AdditionalReservedWordDetails AdditionalReservedWords[] =
    {
        { _T("GLOBAL"),         _T("cspro_program_structure.html") },
        { _T("PROC"),           _T("proc_statement.html") },
        { _T("summary"),        _T("errmsg_function.html"), },
        { _T("denom"),          _T("errmsg_function.html"), },
        { _T("disjoint"),       _T("Freq_statement_unnamed.html") },
        { _T("weight"),         _T("Freq_statement_unnamed.html") },
        { _T("specific"),       _T("impute_function.html") },
        { _T("outofrange"),     _T("set_behavior_canenter_statement.html") },
        { _T("confirm"),        _T("set_behavior_canenter_statement.html") },
        { _T("noconfirm"),      _T("set_behavior_canenter_statement.html") },
        { _T("specialvalues"),  _T("set_behavior_specialvalues_statement.html") },

        // words that are used in the CSPro DB tables
        { _T("cases"),          nullptr },
        { _T("file_revisions"), nullptr },
        { _T("meta"),           nullptr },
        { _T("notes"),          nullptr },
        { _T("occ"),            nullptr },
        { _T("sync_history"),   nullptr },
        { _T("vector_clock"),   nullptr },
    };


    const AdditionalReservedWordDetails SpecialFunctions[] =
    {
        { ToString(SpecialFunction::OnChangeLanguage),      _T("OnChangeLanguage_global_function.html") },
        { ToString(SpecialFunction::OnChar),                _T("OnChar_global_function.html") },
        { ToString(SpecialFunction::OnKey),                 _T("OnKey_global_function.html") },
        { ToString(SpecialFunction::OnStop),                _T("OnStop_global_function.html") },
        { ToString(SpecialFunction::OnSyncMessage),         _T("syncmessage_function.html") },
        { ToString(SpecialFunction::OnRefused),             _T("refused_value.html") },
        { ToString(SpecialFunction::OnSystemMessage),       _T("OnSystemMessage_global_function.html") },
        { ToString(SpecialFunction::OnViewQuestionnaire),   _T("OnViewQuestionnaire_global_function.html") },
        { ToString(SpecialFunction::OnActionInvokerResult), _T("CS_OnActionInvokerResult.html") },
    };
}


bool ReservedWords::IsReservedWord(const wstring_view text_sv)
{
    return ( KeywordTable::IsKeyword(text_sv) ||
             FunctionTable::IsFunctionNamespace(text_sv, SymbolType::None) ||
             FunctionTable::IsFunction(text_sv, SymbolType::None) ||
             GetAdditionalReservedWords().IsEntry(text_sv, nullptr) );
}


void ReservedWords::ForeachReservedWord(const std::function<void(ReservedWordType, const std::wstring&, const void*)>& callback_function)
{
    for( const auto& [text, entry_details] : KeywordTable::GetKeywords().GetTable() )
    {
        if( ( entry_details->token_code == TokenCode::TOKRECODE && SO::EqualsNoCase(text, _T("box")) ) ||
            ( entry_details->token_code == TokenCode::TOKENDRECODE && SO::EqualsNoCase(text, _T("endbox")) ) )
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


const std::vector<std::wstring>& ReservedWords::GetAllReservedWords()
{
    auto get_reserved_words = []()
    {
        std::vector<std::wstring> reserved_words;

        ForeachReservedWord(
            [&](ReservedWordType reserved_word_type, const std::wstring& reserved_word, const void*)
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

    static const std::vector<std::wstring> reserved_words = get_reserved_words();
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
const TCHAR* ReservedWords::GetDefinedCaseWorker(const wstring_view text_sv, const T& function_domain)
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
        const TCHAR* child_symbol_name;

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


const TCHAR* ReservedWords::GetDefinedCase(const wstring_view text_sv, const FunctionDomain& function_domain)
{
    return std::visit([&](const auto& value) { return GetDefinedCaseWorker(text_sv, value); },
                      function_domain);
}
