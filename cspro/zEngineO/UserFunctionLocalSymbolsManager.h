#pragma once

#include <zEngineO/zEngineO.h>
#include <zLogicO/Symbol.h>

class UserFunction;


// a RAII class for managing the symbols of a user-defined function's parameters and
// locally defined variables (to allow for recursive function calls)

class ZENGINEO_API UserFunctionLocalSymbolsManager
{
    friend class UserFunction;

private:
    UserFunctionLocalSymbolsManager(UserFunction& user_function);

public:
    UserFunctionLocalSymbolsManager(const UserFunctionLocalSymbolsManager&) = delete;
    ~UserFunctionLocalSymbolsManager();

    Symbol& GetSymbol(int symbol_index);

    void MarkForSymbolSubstitution(const Symbol& local_symbol, std::shared_ptr<Symbol> new_symbol);

    void RunSymbolSubstitution();

private:
    void CreateLocalSymbolsData(UserFunction& user_function);

    bool SymbolNotMarkedForSubstitution(const Symbol& symbol) const;

private:
    struct Data;
    Data* m_data;
};
