#pragma once

#include <zEngineO/zEngineO.h>
#include <zEngineO/UserFunction.h>


// --------------------------------------------------------------------------
// UserFunctionArgumentEvaluator
//
// Subclasses are used to evaluate the arguments to a user-defined function.
// The following subclass is defined here:
//
//     - NumericStringValuesOnlyUserFunctionArgumentEvaluator
//           - Arguments are provided as a vector of already-evaluated
//             numeric and string values. A flag indicates if the
//             arguments are guaranteed to be of the correct type.
//             If false, and the supplied type is not correct, the argument
//             returned is created using LogicInterpreter::GetInvalidValue.
// --------------------------------------------------------------------------

class ZENGINEO_API UserFunctionArgumentEvaluator
{
public:
    virtual ~UserFunctionArgumentEvaluator() { }

    // Returns the number of arguments (if known).
    virtual std::optional<size_t> GetNumberArguments() = 0;

    // Returns true if an argument exists for the given parameter number.
    // This will only be called if GetNumberArguments returned std::nullopt.
    // The base implementation throws an exception.
    virtual bool ArgumentExists(size_t parameter_number);

    // Return the evaluated numeric expression.
    virtual double GetNumeric(size_t parameter_number) = 0;

    // Return the evaluated string expression.
    virtual SharableString GetString(size_t parameter_number) = 0;

    // Return true if the symbol's argument was constructed in place (using the parameter symbol).
    // The base implementation returns false;
    virtual bool ConstructSymbolInPlace(size_t parameter_number, Symbol& parameter_symbol);

    // Return the evaluated symbol.
    // This will only be called if ConstructSymbolInPlace returned false.
    // If the symbol is not valid (e.g., has a invalid subscript), throw an InvalidSubscript exception.
    // The base implementation throws an exception.
    virtual std::shared_ptr<Symbol> GetSymbol(size_t parameter_number);

    CREATE_CSPRO_EXCEPTION_WITH_MESSAGE(InvalidSubscript, "");
};


// --------------------------------------------------------------------------
// NumericStringValuesOnlyUserFunctionArgumentEvaluator
// --------------------------------------------------------------------------

template<bool ArgumentsAreCorrectType>
class ZENGINEO_API NumericStringValuesOnlyUserFunctionArgumentEvaluator : public UserFunctionArgumentEvaluator
{
public:
    NumericStringValuesOnlyUserFunctionArgumentEvaluator(std::vector<std::variant<double, SharableString>> arguments);

protected:
    std::optional<size_t> GetNumberArguments() override { return m_arguments.size(); }

    double GetNumeric(size_t parameter_number) override        { return double(0); /*GetArgument<double>(parameter_number);*/}
    SharableString GetString(size_t parameter_number) override { return SharableString(""); /*GetArgument<SharableString>(parameter_number);*/}

private:
    template<typename T>
    T GetArgument(size_t parameter_number) const;

private:
    std::vector<std::variant<double, SharableString>> m_arguments;
};
