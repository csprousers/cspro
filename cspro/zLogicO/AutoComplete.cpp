#include "stdafx.h"
#include "AutoComplete.h"
#include "ChildSymbolNames.h"
#include "FunctionTable.h"
#include "RecommendedWord.h"
#include "SymbolTable.h"
#include <zToolsO/VariantVisitOverload.h>

using namespace Logic;


AutoComplete::AutoComplete()
    :   m_symbolTableSizeOnLastUpdate(SymbolTable::FirstValidSymbolIndex)
{
}


const std::map<wchar_t, std::map<std::wstring, AutoComplete::SymbolTypes>>& AutoComplete::GetReservedWords()
{
    static const std::map<wchar_t, std::map<std::wstring, SymbolTypes>> reserved_words =
        []()
        {
            // set up the reserved words table
            std::map<wchar_t, std::map<std::wstring, SymbolTypes>> reserved_words_map;

            for( const std::wstring& reserved_word : ReservedWords::GetAllReservedWords() )
                AddName(reserved_words_map, reserved_word, { SymbolType::None, SymbolType::None });

            return reserved_words_map;
        }();

    return reserved_words;
}


const std::vector<std::wstring>& AutoComplete::GetEntriesForType(const std::variant<SymbolType, FunctionNamespace>& symbol_type_or_function_namespace)
{
    static std::map<std::variant<SymbolType, FunctionNamespace>, std::vector<std::wstring>> entries_by_type;

    const auto& entries_search = entries_by_type.find(symbol_type_or_function_namespace);

    if( entries_search != entries_by_type.end() )
        return entries_search->second;

    // calculate the entries for this symbol type or function namespace
    std::vector<std::wstring>& entry_names = entries_by_type.try_emplace(symbol_type_or_function_namespace, std::vector<std::wstring>()).first->second;

    for( const FunctionDetails& function_details : VI_V(FunctionTable::GetFunctions()) )
    {
        if( function_details.function_domain == symbol_type_or_function_namespace )
            entry_names.emplace_back(function_details.name);
    }

    // add child namespaces
    if( std::holds_alternative<FunctionNamespace>(symbol_type_or_function_namespace) )
    {
        for( const auto& [text, entry_details] : FunctionTable::GetFunctionNamespaces().GetTable() )
        {
            for( const FunctionNamespaceDetails& function_namespace_details : VI_V(entry_details) )
            {
                if( function_namespace_details.parent_function_namespace == std::get<FunctionNamespace>(symbol_type_or_function_namespace) )
                    entry_names.emplace_back(function_namespace_details.name);
            }
        }
    }

    // add some entries that aren't in the function table
    for( const TCHAR* name : GetChildSymbolNames(symbol_type_or_function_namespace) )
        entry_names.emplace_back(name);

    // sort the entries
    std::sort(entry_names.begin(), entry_names.end(),
              [&](const std::wstring& s1, const std::wstring& s2) { return ( SO::CompareNoCase(s1, s2) < 0 ); });

    return entry_names;
}


void AutoComplete::AddName(std::map<wchar_t, std::map<std::wstring, SymbolTypes>>& table, const std::wstring& name, SymbolTypes symbol_types)
{
    ASSERT(!name.empty() && name.front() != '_');

    // check if the name exists
    const wchar_t name_shortcut = std::towlower(name.front());
    const auto& name_shortcut_check = table.find(name_shortcut);

    std::map<std::wstring, SymbolTypes>& name_shortcut_symbols =
        ( name_shortcut_check != table.end() ) ? name_shortcut_check->second :
                                                 table.try_emplace(name_shortcut, std::map<std::wstring, SymbolTypes>()).first->second;

    auto name_check = name_shortcut_symbols.find(name);

    if( name_check == name_shortcut_symbols.end() )
    {
        name_shortcut_symbols.try_emplace(name, std::move(symbol_types));
    }

    // binary dictionary items have been temporarily added as both Variable and Item types, so in that case use the Item
    else if( name_check->second.primary == SymbolType::Variable && symbol_types.primary == SymbolType::Item )
    {
        // BINARY_TYPES_TO_ENGINE_TODO remove this once the fake VART is no longer added
        name_check->second = std::move(symbol_types);
    }

    // if there are multiple symbols with the same name but different types, set the type as unknown
    else if( name_check->second.primary != symbol_types.primary )
    {
        ASSERT(name_check->second.wrapped == SymbolType::None);
        name_check->second.primary = SymbolType::Unknown;
    }
}


void AutoComplete::UpdateWithCompiledSymbols(const SymbolTable& symbol_table, bool update_all)
{
    // clear the map of symbols upon a new compilation
    if( update_all )
    {
        for( auto& [start_letter, symbols] : m_compiledSymbols )
            symbols.clear();

        m_symbolTableSizeOnLastUpdate = SymbolTable::FirstValidSymbolIndex;
    }

    // add the new symbols
    for( ; m_symbolTableSizeOnLastUpdate < symbol_table.GetTableSize(); ++m_symbolTableSizeOnLastUpdate )
    {
        const Symbol& symbol = symbol_table.GetAt(m_symbolTableSizeOnLastUpdate);

        // only add symbols that can be used in logic
        if( symbol.GetName().front() != '_' )
        {
            AddName(m_compiledSymbols, symbol.GetName(), { symbol.GetType(), symbol.GetWrappedType() });

            // add the symbol's aliases
            for( const std::wstring& alias : symbol_table.GetAliases(symbol) )
                AddName(m_compiledSymbols, alias, { symbol.GetType(), symbol.GetWrappedType() });
        }
    }
}


std::tuple<std::wstring, bool> AutoComplete::GetSuggestedWordString(wstring_view name) const
{
    ASSERT(!name.empty());

    std::wstring suggested_words;
    bool word_comes_from_fuzzy_matching = false;

    const wchar_t name_shortcut = std::towlower(name.front());

    for( int pass = 0; pass < 2; ++pass )
    {
        const std::map<wchar_t, std::map<std::wstring, SymbolTypes>>& table = ( pass == 0 ) ? GetReservedWords() :
                                                                                              m_compiledSymbols;

        const auto& name_shortcut_check = table.find(name_shortcut);

        if( name_shortcut_check == table.end() )
            continue;

        for( const auto& [function_name_or_symbol_name, symbol_type] : name_shortcut_check->second )
        {
            if( SO::StartsWithNoCase(function_name_or_symbol_name, name) )
                SO::AppendWithSeparator(suggested_words, function_name_or_symbol_name, ' ');
        }
    }

    // if there are no suggested words, see if there are any good words that nearly match
    if( suggested_words.empty() )
    {
        RecommendedWordCalculator recommended_word_calculator(name, RecommendedWordCalculator::StricterSuggestedScore);
        
        for( const auto& [start_letter, symbols] : m_compiledSymbols )
        {
            for( const auto& [symbol_name, symbol_type] : symbols )
                recommended_word_calculator.Match(symbol_name);
        }

        suggested_words = recommended_word_calculator.GetRecommendedWord();

        if( !suggested_words.empty() )
            word_comes_from_fuzzy_matching = true;
    }

    return { std::move(suggested_words), word_comes_from_fuzzy_matching };
}


std::wstring AutoComplete::GetSuggestedWordString(cs::span<const std::wstring> dot_notation_entries, wstring_view name) const
{
    // a routine for dot notation words
    ASSERT(!dot_notation_entries.empty());

    std::variant<SymbolType, FunctionNamespace> symbol_type_or_function_namespace = SymbolType::None;
    SymbolType wrapped_symbol_type = SymbolType::None;

    for( const std::wstring& dot_notation_entry : dot_notation_entries )
    {
        ASSERT(!dot_notation_entry.empty());

        // see if the symbol exists and has a unique type
        const wchar_t name_shortcut = std::towlower(dot_notation_entry.front());
        const auto& name_shortcut_check = m_compiledSymbols.find(name_shortcut);

        if( name_shortcut_check != m_compiledSymbols.cend() )
        {
            bool entry_processed = false;

            for( const auto& [symbol_name, symbol_types] : name_shortcut_check->second )
            {
                if( SO::EqualsNoCase(dot_notation_entry, symbol_name) )
                {
                    symbol_type_or_function_namespace = symbol_types.primary;
                    wrapped_symbol_type = symbol_types.wrapped;
                    entry_processed = true;
                    break;
                }
            }

            if( entry_processed )
                continue;
        }

        // see if this is a function namespace
        if( std::holds_alternative<FunctionNamespace>(symbol_type_or_function_namespace) ||
            std::get<SymbolType>(symbol_type_or_function_namespace) == SymbolType::None )
        {
            const FunctionNamespaceDetails* function_namespace_details;

            if( FunctionTable::IsFunctionNamespace(dot_notation_entry, symbol_type_or_function_namespace, &function_namespace_details) )
            {
                symbol_type_or_function_namespace = function_namespace_details->function_namespace;
                continue;
            }
        }

        // if not processed, return no suggestions
        return std::wstring();
    }

    ASSERT(symbol_type_or_function_namespace != SymbolType::None);

    // construct the suggested word string
    std::wstring suggested_words;

    auto add_entries = [&](const auto& value)
    {
        for( const std::wstring& entry_name : GetEntriesForType(value) )
        {
            if( SO::StartsWithNoCase(entry_name, name) )
                SO::AppendWithSeparator(suggested_words, entry_name, ' ');
        }
    };

    std::visit(add_entries, symbol_type_or_function_namespace);

    if( wrapped_symbol_type == SymbolType::None )
    {
        return suggested_words;
    }

    // if the symbol wraps another type, add that type as well
    else
    {
        add_entries(wrapped_symbol_type);

        // remove duplicate entries and organize in sorted order
        std::set<std::wstring> suggested_words_set;

        for( wstring_view suggested_word_sv : SO::SplitString<wstring_view>(suggested_words, ' ') )
            suggested_words_set.insert(suggested_word_sv);

        return SO::CreateSingleString(suggested_words_set, _T(" "));
    }
}
