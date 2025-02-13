#pragma once

#include <zLogicO/zLogicO.h>
#include <zLogicO/Symbol.h>
#include <zToolsO/StringNoCase.h>

class UserFunctionLocalSymbolsManager;


namespace Logic
{
    class LocalSymbolStack;


    class ZLOGICO_API SymbolTable
    {
        friend LocalSymbolStack;
        friend UserFunctionLocalSymbolsManager;

    public:
        static constexpr size_t FirstValidSymbolIndex = 1;
        enum class NameMapAddition { ToGlobalScope, ToCurrentScope, DoNotAdd };

        SymbolTable();

        void Clear();

        size_t GetTableSize() const;

        // adds a symbol to the symbol table
        void AddSymbol(std::shared_ptr<Symbol> symbol, NameMapAddition name_map_addition = NameMapAddition::ToCurrentScope);

        Symbol& GetAt(int symbol_index) const;
        std::shared_ptr<Symbol> GetSharedAt(int symbol_index) const;

        bool NameExists(const StringNoCase& symbol_name) const;

        // find methods that do not account for dot notation
        Symbol& FindSymbol(const StringNoCase& symbol_name, const Symbol* parent_symbol = nullptr) const;
        Symbol& FindSymbol(const StringNoCase& symbol_name, SymbolType preferred_symbol_type, const std::vector<SymbolType>* allowable_symbol_types) const;
        Symbol& FindSymbolOfType(const StringNoCase& symbol_name, SymbolType symbol_type) const;
        std::vector<Symbol*> FindSymbols(const StringNoCase& symbol_name) const;

        // a find method that does account for dot notation
        Symbol& FindSymbolWithDotNotation(const StringNoCase& full_symbol_name, SymbolType preferred_symbol_type = SymbolType::None,
                                          const std::vector<SymbolType>* allowable_symbol_types = nullptr) const;

        void AddAlias(StringNoCase symbol_name, const Symbol& symbol);
        std::vector<std::wstring> GetAliases(const Symbol& symbol) const;

        LocalSymbolStack CreateLocalSymbolStack();

        // looks at symbol names and reserved words to find a close match for the word;
        // a blank string is returned if no good match is found;
        // if multiple good matches are found, a name is only returned if the best match has a very high score
        std::wstring GetRecommendedWordUsingFuzzyMatching(const std::wstring& word) const;

        // calls the callback function for each symbol of a certain type (ST)
        template<typename ST, bool CallbackReturnsTrueToContinue = false, typename CF>
        auto ForeachSymbol(CF callback_function);

    private:
        void AddSymbolToNameMap(StringNoCase symbol_name, size_t symbol_index);
        void RemoveSymbolFromNameMap(const Symbol& symbol);

    private:
        std::vector<std::shared_ptr<Symbol>> m_symbols;
        std::map<StringNoCase, std::vector<size_t>> m_nameMap;
        std::vector<LocalSymbolStack*> m_localSymbolStacks;


        // --------------------------------------------------------------------------
        // SymbolTable exceptions
        // --------------------------------------------------------------------------
    public:
        struct Exception : public CSProException
        {
            Exception(const std::wstring& message) : CSProException(message) { }
            virtual int GetCompilerErrorMessageNumber() const = 0;
        };

#define DECLARE_EXCEPTION(API, class_name)                      \
        struct API class_name : public Exception                \
        {                                                       \
            class_name(const std::wstring& symbol_name);        \
            int GetCompilerErrorMessageNumber() const override; \
        };
        DECLARE_EXCEPTION(ZLOGICO_API, NoSymbolsException)
        DECLARE_EXCEPTION(, MultipleSymbolsException)
        DECLARE_EXCEPTION(, NoSymbolsOfAllowableTypesException)
#undef DECLARE_EXCEPTION
    };
}


// --------------------------------------------------------------------------
// inline implementations
// --------------------------------------------------------------------------

inline size_t Logic::SymbolTable::GetTableSize() const
{
    return m_symbols.size();
}


inline Symbol& Logic::SymbolTable::GetAt(int symbol_index) const
{
    ASSERT(static_cast<size_t>(symbol_index) >= FirstValidSymbolIndex && static_cast<size_t>(symbol_index) < m_symbols.size());
    ASSERT(m_symbols[symbol_index] != nullptr);
    return *m_symbols[symbol_index];
}


inline std::shared_ptr<Symbol> Logic::SymbolTable::GetSharedAt(int symbol_index) const
{
    ASSERT(static_cast<size_t>(symbol_index) >= FirstValidSymbolIndex && static_cast<size_t>(symbol_index) < m_symbols.size());
    ASSERT(m_symbols[symbol_index] != nullptr);
    return m_symbols[symbol_index];
}


inline bool Logic::SymbolTable::NameExists(const StringNoCase& symbol_name) const
{
    const auto& name_search = m_nameMap.find(symbol_name);
    return ( name_search != m_nameMap.cend() );
}
