#pragma once

#include <zLogicO/zLogicO.h>
#include <zLogicO/Symbol.h>
#include <zToolsO/span.h>


namespace Logic { class AutoComplete;
                  struct FunctionDetails;
                  enum class FunctionNamespace : int;
                  class SymbolTable; }


class ZLOGICO_API Logic::AutoComplete
{
public:
    AutoComplete();

    void UpdateWithCompiledSymbols(const SymbolTable& symbol_table, bool update_all);

    std::tuple<std::string, bool> GetSuggestedWordString(const std::string& name) const;
    std::string GetSuggestedWordString(cs::span<const std::string> dot_notation_entries, std::string_view name_sv) const;

private:
    struct SymbolTypes
    {
        SymbolType primary;
        SymbolType wrapped;
    };

    static const std::map<char, std::map<std::string, SymbolTypes>>& GetReservedWords();

    static bool ShouldAddFunction(const FunctionDetails& function_details);

    static const std::vector<std::string>& GetEntriesForType(const std::variant<SymbolType, FunctionNamespace>& symbol_type_or_function_namespace);

    static void AddName(std::map<char, std::map<std::string, SymbolTypes>>& table, const std::string& name, SymbolTypes symbol_types);

private:
    std::map<char, std::map<std::string, SymbolTypes>> m_compiledSymbols;
    size_t m_symbolTableSizeOnLastUpdate;
};
