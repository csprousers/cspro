#include "stdafx.h"
#include "SymbolTable.h"
#include "LocalSymbolStack.h"
#include "RecommendedWord.h"

using namespace Logic;


// --------------------------------------------------------------------------
// SymbolTable
// --------------------------------------------------------------------------

SymbolTable::SymbolTable()
{
    Clear();
}


void SymbolTable::Clear()
{
    m_symbols.clear();
    m_nameMap.clear();

    // the symbol table historically used the zeroth index as a placeholder
    m_symbols.emplace_back(nullptr);

    ASSERT80(m_symbols.size() == FirstValidSymbolIndex);
}


void SymbolTable::AddSymbol(std::shared_ptr<Symbol> symbol, NameMapAddition name_map_addition/* = NameMapAddition::ToCurrentScope*/)
{
    ASSERT(symbol != nullptr);

    // set the index of the symbol to its position in the symbol table
    symbol->m_symbolIndex = m_symbols.size();

    // add the symbol to the symbol table
    m_symbols.emplace_back(symbol);

    // potentially add the symbol to the name map
    if( name_map_addition != NameMapAddition::DoNotAdd )
    {
        // symbols that create symbols should not add the dot-version to the symbol table
        ASSERT(symbol->GetName().find('.') == std::wstring::npos);

        AddSymbolToNameMap(symbol->GetName(), symbol->GetSymbolIndex());

        // add the symbol to the local symbol stack (if applicable)
        if( name_map_addition == NameMapAddition::ToCurrentScope && !m_localSymbolStacks.empty() )
        {
            m_localSymbolStacks.back()->m_localSymbolIndices.emplace_back(symbol->GetSymbolIndex());

            // notify any listeners about symbols added
            for( const LocalSymbolStack* local_symbol_stack : m_localSymbolStacks )
            {
                if( local_symbol_stack->m_addSymbolListener != nullptr )
                    (*local_symbol_stack->m_addSymbolListener)(symbol->GetSymbolIndex());
            }
        }
    }
}


void SymbolTable::AddSymbolToNameMap(StringNoCase symbol_name, size_t symbol_index)
{
    ASSERT(symbol_index < m_symbols.size());

    m_nameMap[std::move(symbol_name)].emplace_back(symbol_index);
}


void SymbolTable::RemoveSymbolFromNameMap(const Symbol& symbol)
{
    auto name_search = m_nameMap.find(symbol.GetName());

    // when called from the LocalSymbolStack destructor, the name may have already been removed
    if( name_search == m_nameMap.end() )
        return;

    std::vector<size_t>& symbols_with_name = name_search->second;

    // if this is the last symbol with this name, remove the entry entirely
    if( symbols_with_name.size() == 1 )
    {
        m_nameMap.erase(name_search);
    }

    // otherwise, just remove this symbol
    else
    {
        symbols_with_name.erase(std::remove(symbols_with_name.begin(), symbols_with_name.end(),
                                            static_cast<size_t>(symbol.GetSymbolIndex())));
    }
}


Symbol& SymbolTable::FindSymbol(const StringNoCase& symbol_name, const Symbol* parent_symbol/* = nullptr*/) const
{
    Symbol* symbol = nullptr;

    // if the parent symbol is specified, search only within that symbol
    if( parent_symbol != nullptr )
    {
        symbol = parent_symbol->FindChildSymbol(symbol_name);
    }

    else
    {
        // otherwise, search all symbols
        auto name_search = m_nameMap.find(symbol_name);

        if( name_search != m_nameMap.end() )
        {
            const std::vector<size_t>& symbols_with_name = name_search->second;

            if( symbols_with_name.size() != 1 )
                throw MultipleSymbolsException(symbol_name);

            symbol = m_symbols[name_search->second.front()].get();
        }
    }

    if( symbol == nullptr )
        throw NoSymbolsException(symbol_name);

    return *symbol;
}


Symbol& SymbolTable::FindSymbol(const StringNoCase& symbol_name, SymbolType preferred_symbol_type, const std::vector<SymbolType>* allowable_symbol_types) const
{
    auto name_search = m_nameMap.find(symbol_name);

    if( name_search == m_nameMap.end() )
        throw NoSymbolsException(symbol_name);

    Symbol* preferred_symbol = nullptr;
    bool found_preferred_symbol = false;
    size_t matched_symbols_found = 0;

    for( size_t symbol_index : name_search->second )
    {
        Symbol* potential_symbol = m_symbols[symbol_index].get();

        if( allowable_symbol_types == nullptr || potential_symbol->IsOneOf(*allowable_symbol_types) )
        {
            if( preferred_symbol_type == SymbolType::None || potential_symbol->IsA(preferred_symbol_type) )
            {
                if( found_preferred_symbol )
                    throw MultipleSymbolsException(symbol_name);

                preferred_symbol = potential_symbol;
                found_preferred_symbol = true;
            }

            else if( preferred_symbol == nullptr )
            {
                preferred_symbol = potential_symbol;
            }

            matched_symbols_found++;
        }
    }

    if( matched_symbols_found == 0 )
    {
        throw NoSymbolsOfAllowableTypesException(symbol_name);
    }

    else if( !found_preferred_symbol && matched_symbols_found > 1 )
    {
        throw MultipleSymbolsException(symbol_name);
    }

    return *preferred_symbol;
}


Symbol& SymbolTable::FindSymbolOfType(const StringNoCase& symbol_name, SymbolType symbol_type) const
{
    const std::vector<SymbolType> allowable_symbol_types { symbol_type };

    return FindSymbol(symbol_name, SymbolType::None, &allowable_symbol_types);
}


std::vector<Symbol*> SymbolTable::FindSymbols(const StringNoCase& symbol_name) const
{
    std::vector<Symbol*> symbols;

    const auto& name_search = m_nameMap.find(symbol_name);

    if( name_search != m_nameMap.cend() )
    {
        for( size_t symbol_index : name_search->second )
            symbols.emplace_back(m_symbols[symbol_index].get());
    }

    return symbols;
}


Symbol& SymbolTable::FindSymbolWithDotNotation(const StringNoCase& full_symbol_name, SymbolType preferred_symbol_type/* = SymbolType::None*/,
                                               const std::vector<SymbolType>* allowable_symbol_types/* = nullptr*/) const
{
    // search for a symbol, allowing for dot notation
    Symbol* symbol = nullptr;
    size_t next_name_part_index = 0;

    while( next_name_part_index < full_symbol_name.length() )
    {
        size_t dot_index = full_symbol_name.find('.', next_name_part_index);
        wstring_view symbol_name_sv;

        if( dot_index == StringNoCase::npos )
        {
            // if no dots are specified, search using the whole name
            if( symbol == nullptr )
            {
                symbol = &FindSymbol(full_symbol_name, preferred_symbol_type, allowable_symbol_types);
                break;
            }

            // otherwise search each section separately
            else
            {
                symbol_name_sv = wstring_view(full_symbol_name).substr(next_name_part_index);
                next_name_part_index = full_symbol_name.length();
            }
        }

        else
        {
            symbol_name_sv = wstring_view(full_symbol_name).substr(next_name_part_index, dot_index - next_name_part_index);
            next_name_part_index = dot_index + 1;
        }

        try
        {
            symbol = &FindSymbol(symbol_name_sv, symbol);
        }

        catch( const NoSymbolsException& )
        {
            // throw an exception with the full symbol name
            throw NoSymbolsException(full_symbol_name);
        }
    }

    ASSERT(symbol != nullptr || full_symbol_name.empty());

    if( symbol == nullptr )
        throw NoSymbolsException(full_symbol_name);

    return *symbol;
}


void SymbolTable::AddAlias(StringNoCase symbol_name, const Symbol& symbol)
{
    AddSymbolToNameMap(std::move(symbol_name), symbol.GetSymbolIndex());
}


std::vector<std::wstring> SymbolTable::GetAliases(const Symbol& symbol) const
{
    std::vector<std::wstring> aliases;

    for( const auto& [symbol_name, symbols_with_name] : m_nameMap )
    {
        if( SO::EqualsNoCase(symbol_name, symbol.GetName()) )
            continue;

        for( size_t symbol_index : symbols_with_name )
        {
            if( &symbol == m_symbols[symbol_index].get() )
            {
                aliases.emplace_back(symbol_name);
                break;
            }
        }
    }

    return aliases;
}


LocalSymbolStack SymbolTable::CreateLocalSymbolStack()
{
    return LocalSymbolStack(*this);
}


std::wstring SymbolTable::GetRecommendedWordUsingFuzzyMatching(const std::wstring& word) const
{
    RecommendedWordCalculator recommended_word_calculator(word);

    // iterate through all symbols
    for( const auto& [symbol_name, symbols_with_name] : m_nameMap )
        recommended_word_calculator.Match(symbol_name);

    return recommended_word_calculator.GetRecommendedWord();
}



// --------------------------------------------------------------------------
// SymbolTable Exceptions
// --------------------------------------------------------------------------

Logic::SymbolTable::NoSymbolsException::NoSymbolsException(const std::wstring& symbol_name)
    :   Exception(FormatTextCS2WS(_T("The symbol '%s' does not exist"), symbol_name.c_str()))
{
}

int Logic::SymbolTable::NoSymbolsException::GetCompilerErrorMessageNumber() const
{
    return 93000;
}


Logic::SymbolTable::MultipleSymbolsException::MultipleSymbolsException(const std::wstring& symbol_name)
    :   Exception(FormatTextCS2WS(_T("The symbol '%s' exists more than once in your application so you must provide qualifiers (such as the dictionary name) to avoid ambiguity"), symbol_name.c_str()))
{
}

int Logic::SymbolTable::MultipleSymbolsException::GetCompilerErrorMessageNumber() const
{
    return 93001;
}


Logic::SymbolTable::NoSymbolsOfAllowableTypesException::NoSymbolsOfAllowableTypesException(const std::wstring& symbol_name)
    :   Exception(FormatTextCS2WS(_T("The symbol '%s' is valid but is not of the type expected"), symbol_name.c_str()))
{
}

int Logic::SymbolTable::NoSymbolsOfAllowableTypesException::GetCompilerErrorMessageNumber() const
{
    return 93002;
}
