#pragma once

#include <zEngineO/zEngineO.h>
#include <zEngineO/AllSymbolDeclarations.h>
#include <zToolsO/CSProException.h>


class ZENGINEO_API UserFunctionArgumentChecker
{
public:
    CREATE_CSPRO_EXCEPTION(CheckError);

    UserFunctionArgumentChecker(const UserFunction& user_function);

    // methods to help compile arguments; on failure, a CheckError exception is thrown with the error message
    void CheckNumberArguments(size_t number_arguments) const;

    static bool SymbolTypeIsAnExpression(SymbolType symbol_type);
    bool ArgumentShouldBeExpression(size_t parameter_number) const;

    void CheckExpressionArgument(size_t parameter_number, bool argument_is_numeric_expression);
    void CheckExpressionArgument(size_t parameter_number, SymbolType argument_type);

    void CheckSymbolArgument(size_t parameter_number, Symbol* argument_symbol);

    // returns the index of the first parameter that has a symbol type not in the list
    std::optional<size_t> FindFirstInvalidParameter(cs::span<const SymbolType> valid_parameter_symbol_types,
                                                    bool include_numeric_and_string_parameters) const noexcept;

    // routines to apply permissions to symbols to indicate that they will be
    // dynamically bound to a function parameter
    static void MarkSymbolAsDynamicallyBoundToFunctionParameter(Symbol& symbol);
    static void MarkParametersAsUsedInFunctionPointerUse(UserFunction& user_function);

private:
    [[noreturn]] void IssueArgumentError(cs::string_sz extra_error_text = "") const;

    static const char* GetExpectedArgumentText(const Symbol& symbol);
    const char* GetExpectedArgumentText() const;

    void CheckLogicArrayArgument(const Symbol& argument_symbol) const;
    void CheckEngineDictionaryArgument(EngineDictionary& argument_engine_dictionary) const;
    void CheckLogicFileArgument(LogicFile& argument_file) const;
    void CheckUserFunctionArgument(UserFunction& argument_user_function) const;

private:
    const UserFunction& m_userFunction;
    size_t m_parameterNumber;
    const Symbol* m_parameterSymbol;
};
