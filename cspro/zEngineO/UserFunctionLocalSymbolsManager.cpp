#include "stdafx.h"
#include "UserFunctionLocalSymbolsManager.h"
#include "List.h"
#include "UserFunction.h"
#include "Versioning.h"
#include <engine/Engarea.h>


// --------------------------------------------------------------------------
// UserFunctionLocalSymbolsManager
// --------------------------------------------------------------------------

struct UserFunctionLocalSymbolsManager::Data
{
    UserFunction& user_function;
    Logic::SymbolTable& symbol_table;

    std::map<int, std::shared_ptr<Symbol>> local_symbols;
    std::vector<std::shared_ptr<Symbol>> cloned_symbols;

    // the variables above are created once; the following is updated every function call
    std::vector<std::tuple<int, std::shared_ptr<Symbol>>> symbols_marked_for_substitution;
};


UserFunctionLocalSymbolsManager::UserFunctionLocalSymbolsManager(UserFunction& user_function)
{
    // create a new set of local symbols if one has not been created for this function call count
    if( user_function.m_functionCallCount >= user_function.m_localSymbolsManagerData.size() )
    {
        CreateLocalSymbolsData(user_function);
    }

    // otherwise reuse an existing set
    else
    {
        m_data = user_function.m_localSymbolsManagerData[user_function.m_functionCallCount].get();
    }

    m_data->symbols_marked_for_substitution.clear();

    // increment the function call count
    ++user_function.m_functionCallCount;
}


UserFunctionLocalSymbolsManager::~UserFunctionLocalSymbolsManager()
{
    // restore any substituted symbols
    for( const auto& [symbol_index, symbol] : m_data->symbols_marked_for_substitution )
        m_data->symbol_table.m_symbols[symbol_index] = symbol;

    // decrement the function call count
    ASSERT(m_data->user_function.m_functionCallCount > 0);
    --m_data->user_function.m_functionCallCount;
}


void UserFunctionLocalSymbolsManager::CreateLocalSymbolsData(UserFunction& user_function)
{
    // the first set can use the original symbols but subsequent ones must be cloned
    bool use_original_set = user_function.m_localSymbolsManagerData.empty();

    m_data = user_function.m_localSymbolsManagerData.emplace_back(
        std::make_shared<UserFunctionLocalSymbolsManager::Data>(UserFunctionLocalSymbolsManager::Data
        {
            user_function,
            user_function.m_engineData.symbol_table,
        })).get();

    auto create_local_symbol = [&](int symbol_index)
    {
        std::shared_ptr<Symbol> local_symbol;

        // for the first set, get the symbol directly from the symbol table
        if( use_original_set )
        {
            local_symbol = m_data->symbol_table.GetSharedAt(symbol_index);
        }

        // when in a recursive call, the original symbol may be substituted at the moment,
        // so get the original symbol from the first set of local symbols...
        else
        {
            local_symbol = user_function.m_localSymbolsManagerData.front()->local_symbols[symbol_index];
            ASSERT(local_symbol != nullptr);

            // ...and then clone the symbol
            std::unique_ptr<Symbol> cloned_symbol = local_symbol->CloneInInitialState();

            if( cloned_symbol != nullptr )
            {
                ASSERT(cloned_symbol->GetSymbolIndex() == local_symbol->GetSymbolIndex());
                local_symbol = m_data->cloned_symbols.emplace_back(std::move(cloned_symbol));
            }

            else
            {
                if( Versioning::PredatesCompiledLogicVersion(Serializer::Iteration_7_6_000_1) )
                {
                    if( local_symbol->IsA(SymbolType::Variable) )
                    {
                        static bool error_displayed = false;

                        if( !error_displayed )
                        {
                            error_displayed = true;
                            ErrorMessage::Display(_T("Recursive functions will no longer work properly with .pen files ")
                                                    _T("generated prior to CSPro 7.6. Regenerate the .pen and try again."));
                        }
                    }

                    else
                    {
                        // prior to the WorkString object existing, work sections could get mixed up with this;
                        ASSERT(local_symbol->IsA(SymbolType::Section));
                    }
                }

                else
                {
                    // if asserting, implement cloning for the symbol
                    ASSERT(false);
                }
            }
        }

        ASSERT(local_symbol->GetSymbolIndex() == symbol_index);

        m_data->local_symbols.try_emplace(symbol_index, local_symbol);
    };

    for( int symbol_index : user_function.m_parameterSymbols )
        create_local_symbol(symbol_index);

    for( int symbol_index : user_function.m_functionBodySymbols )
        create_local_symbol(symbol_index);
}


Symbol& UserFunctionLocalSymbolsManager::GetSymbol(int symbol_index)
{
    ASSERT(m_data->local_symbols.find(symbol_index) != m_data->local_symbols.cend());

    return *m_data->local_symbols[symbol_index];
}


bool UserFunctionLocalSymbolsManager::SymbolNotMarkedForSubstitution(const Symbol& symbol) const
{
    const auto& lookup = std::find_if(m_data->symbols_marked_for_substitution.cbegin(), m_data->symbols_marked_for_substitution.cend(),
                                      [&](const auto& smfs) { return ( std::get<0>(smfs) == symbol.GetSymbolIndex() ); });

    return ( lookup == m_data->symbols_marked_for_substitution.cend() );
}


void UserFunctionLocalSymbolsManager::MarkForSymbolSubstitution(const Symbol& local_symbol, std::shared_ptr<Symbol> new_symbol)
{
    ASSERT(&local_symbol == &GetSymbol(local_symbol.GetSymbolIndex()));
    ASSERT(SymbolNotMarkedForSubstitution(local_symbol));
    ASSERT(new_symbol != nullptr);

    m_data->symbols_marked_for_substitution.emplace_back(local_symbol.GetSymbolIndex(), std::move(new_symbol));
}


void UserFunctionLocalSymbolsManager::RunSymbolSubstitution()
{
    // when in a recursive call, all symbols have to be substituted
    if( !m_data->cloned_symbols.empty() )
    {
        for( const auto& [symbol_index, local_symbol] : m_data->local_symbols )
        {
            if( SymbolNotMarkedForSubstitution(*local_symbol) )
                m_data->symbols_marked_for_substitution.emplace_back(symbol_index, local_symbol);
        }
    }

    // swap any symbols marked for substitution, setting the substituted symbol to the old symbol, to be restored in the destructor
    for( auto& [symbol_index, new_symbol] : m_data->symbols_marked_for_substitution )
        std::swap(m_data->symbol_table.m_symbols[symbol_index], new_symbol);
}
