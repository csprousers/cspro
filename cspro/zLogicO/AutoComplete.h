#pragma once

#include <zLogicO/zLogicO.h>
#include <zLogicO/Symbol.h>
#include <zToolsO/span.h>


namespace Logic
{
    enum class FunctionNamespace : int;
    class SymbolTable;


    class ZLOGICO_API AutoComplete
    {
    public:
        AutoComplete();

        void UpdateWithCompiledSymbols(const SymbolTable& symbol_table, bool update_all);

        std::tuple<std::wstring, bool> GetSuggestedWordString(wstring_view name) const;
        std::wstring GetSuggestedWordString(cs::span<const std::wstring> dot_notation_entries, wstring_view name) const;

    private:
        struct SymbolTypes
        {
            SymbolType primary;
            SymbolType wrapped;
        };

        static const std::map<wchar_t, std::map<std::wstring, SymbolTypes>>& GetReservedWords();

        static const std::vector<std::wstring>& GetEntriesForType(const std::variant<SymbolType, FunctionNamespace>& symbol_type_or_function_namespace);

        static void AddName(std::map<wchar_t, std::map<std::wstring, SymbolTypes>>& table, const std::wstring& name, SymbolTypes symbol_types);

    private:
        std::map<wchar_t, std::map<std::wstring, SymbolTypes>> m_compiledSymbols;
        size_t m_symbolTableSizeOnLastUpdate;
    };
}
