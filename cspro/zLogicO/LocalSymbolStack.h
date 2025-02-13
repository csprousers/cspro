#pragma once

#include <zLogicO/SymbolTable.h>
#include <zToolsO/RaiiHelpers.h>


namespace Logic
{
    // --------------------------------------------------------------------------
    // LocalSymbolStack
    //
    // a RAII class for keeping track of scope changes and to remove
    // locally-declared symbols from the symbol table
    // --------------------------------------------------------------------------

    class LocalSymbolStack
    {
        friend SymbolTable;

    private:
        LocalSymbolStack(SymbolTable& symbol_table);

    public:
        LocalSymbolStack(const LocalSymbolStack&) = delete;
        LocalSymbolStack(LocalSymbolStack&& rhs);

        ~LocalSymbolStack();

        const std::vector<int>& GetLocalSymbolIndices() const { return m_localSymbolIndices; }

        // sets a listener that will receive information about all locally-scoped symbols added to this, or child, scopes
        void SetAddSymbolListener(std::unique_ptr<std::function<void(int symbol_index)>> add_symbol_listener) { m_addSymbolListener = std::move(add_symbol_listener); }

    private:
        SymbolTable& m_symbolTable;
        RAII::PushOnVectorAndPopOnDestruction<LocalSymbolStack*> m_localSymbolStackHolder;
        std::vector<int> m_localSymbolIndices;
        std::unique_ptr<const std::function<void(int)>> m_addSymbolListener;
    };
}


// --------------------------------------------------------------------------
// inline implementations
// --------------------------------------------------------------------------

inline Logic::LocalSymbolStack::LocalSymbolStack(SymbolTable& symbol_table)
    :   m_symbolTable(symbol_table),
        m_localSymbolStackHolder(m_symbolTable.m_localSymbolStacks, this)
{
}


inline Logic::LocalSymbolStack::LocalSymbolStack(LocalSymbolStack&& rhs)
    :   m_symbolTable(rhs.m_symbolTable),
        m_localSymbolStackHolder(std::move(rhs.m_localSymbolStackHolder)),
        m_localSymbolIndices(std::move(rhs.m_localSymbolIndices)),
        m_addSymbolListener(std::move(rhs.m_addSymbolListener))
{
    m_localSymbolStackHolder.SetValue(this);
}


inline Logic::LocalSymbolStack::~LocalSymbolStack()
{
    for( int symbol_index : m_localSymbolIndices )
        m_symbolTable.RemoveSymbolFromNameMap(m_symbolTable.GetAt(symbol_index));
}
