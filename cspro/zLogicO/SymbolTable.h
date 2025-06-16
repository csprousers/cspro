#pragma once

#include <zLogicO/zLogicO.h>
#include <zLogicO/Symbol.h>
#include <zToolsO/CaseInsensitiveComparer.h>

class UserFunctionLocalSymbolsManager;
namespace Logic { class LocalSymbolStack; class SymbolTable; }


class ZLOGICO_API Logic::SymbolTable
{
    friend LocalSymbolStack;
    friend UserFunctionLocalSymbolsManager;

public:
    static constexpr size_t FirstValidSymbolIndex = 1;
    enum class NameMapAddition { ToGlobalScope, ToCurrentScope, DoNotAdd };

    SymbolTable();

    void Clear();

    size_t GetTableSize() const;

    // Adds a symbol to the symbol table.
    void AddSymbol(std::shared_ptr<Symbol> symbol, NameMapAddition name_map_addition = NameMapAddition::ToCurrentScope);

    // Adds a symbol to the symbol table if it has not already been added.
    // If the symbol is already in the symbol table, the symbol is added to the name map.
    // In both cases, the name is added using NameMapAddition::ToCurrentScope.
    void AddReusableSymbol(std::shared_ptr<Symbol> symbol);

    Symbol& GetAt(int symbol_index) const;
    std::shared_ptr<Symbol> GetSharedAt(int symbol_index) const;

    bool NameExists(std::string_view symbol_name_sv) const;

    // Find methods that do not account for dot notation:
    Symbol& FindSymbol(std::string_view symbol_name_sv, const Symbol* parent_symbol = nullptr) const;
    Symbol& FindSymbol(std::string_view symbol_name_sv, SymbolType preferred_symbol_type, const std::vector<SymbolType>* allowable_symbol_types) const;
    Symbol& FindSymbolOfType(std::string_view symbol_name_sv, SymbolType symbol_type) const;
    std::vector<Symbol*> FindSymbols(std::string_view symbol_name_sv) const;

    // A find method that does account for dot notation:
    Symbol& FindSymbolWithDotNotation(std::string_view full_symbol_name_sv, SymbolType preferred_symbol_type = SymbolType::None,
                                      const std::vector<SymbolType>* allowable_symbol_types = nullptr) const;

    void AddAlias(std::string symbol_name, const Symbol& symbol);
    std::vector<std::string> GetAliases(const Symbol& symbol) const;

    LocalSymbolStack CreateLocalSymbolStack();

    // Looks at symbol names and reserved words to find a close match for the word.
    // A blank string is returned if no good match is found.
    // If multiple good matches are found, a name is only returned if the best match has a very high score.
    std::string GetRecommendedWordUsingFuzzyMatching(const std::string& word) const;

    // Calls the callback function for each symbol of a certain type (ST).
    template<typename ST, bool CallbackReturnsTrueToContinue = false, typename CF>
    auto ForeachSymbol(CF callback_function);

private:
    // Adds an existing symbol to the name map using the given name.
    void AddSymbolToNameMap(std::string symbol_name, size_t symbol_index, NameMapAddition name_map_addition);

    // Removes an existing symbol from the name map.
    void RemoveSymbolFromNameMap(const Symbol& symbol);

private:
    std::vector<std::shared_ptr<Symbol>> m_symbols;
    std::map<std::string, std::vector<size_t>, cs::case_insensitive_less> m_nameMap;
    std::vector<LocalSymbolStack*> m_localSymbolStacks;


    // --------------------------------------------------------------------------
    // SymbolTable exceptions
    // --------------------------------------------------------------------------
public:
    struct Exception : public CSProException
    {
        using CSProException::CSProException;

        virtual int GetCompilerErrorMessageNumber() const = 0;
    };

#define DECLARE_EXCEPTION(API, class_name)                  \
    struct API class_name : public Exception                \
    {                                                       \
        class_name(cs::string_sz symbol_name);              \
        int GetCompilerErrorMessageNumber() const override; \
    };
    DECLARE_EXCEPTION(ZLOGICO_API, NoSymbolsException)
    DECLARE_EXCEPTION(, MultipleSymbolsException)
    DECLARE_EXCEPTION(, NoSymbolsOfAllowableTypesException)
#undef DECLARE_EXCEPTION
};



// --------------------------------------------------------------------------
// inline implementations
// --------------------------------------------------------------------------

inline size_t Logic::SymbolTable::GetTableSize() const
{
    return m_symbols.size();
}


inline Symbol& Logic::SymbolTable::GetAt(const int symbol_index) const
{
    ASSERT(static_cast<size_t>(symbol_index) >= FirstValidSymbolIndex && static_cast<size_t>(symbol_index) < m_symbols.size());
    ASSERT(m_symbols[symbol_index] != nullptr);
    return *m_symbols[symbol_index];
}


inline std::shared_ptr<Symbol> Logic::SymbolTable::GetSharedAt(const int symbol_index) const
{
    ASSERT(static_cast<size_t>(symbol_index) >= FirstValidSymbolIndex && static_cast<size_t>(symbol_index) < m_symbols.size());
    ASSERT(m_symbols[symbol_index] != nullptr);
    return m_symbols[symbol_index];
}


inline bool Logic::SymbolTable::NameExists(const std::string_view symbol_name_sv) const
{
    const auto& name_search = m_nameMap.find(symbol_name_sv);
    return ( name_search != m_nameMap.cend() );
}
