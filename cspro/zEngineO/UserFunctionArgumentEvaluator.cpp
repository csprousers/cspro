#include "stdafx.h"
#include "UserFunctionArgumentEvaluator.h"
#include "Interpreter/LogicInterpreter.h"


// --------------------------------------------------------------------------
// UserFunctionArgumentEvaluator
// --------------------------------------------------------------------------

bool UserFunctionArgumentEvaluator::ArgumentExists(size_t /*parameter_number*/)
{
    return ReturnProgrammingError(false);
}


bool UserFunctionArgumentEvaluator::ConstructSymbolInPlace(size_t /*parameter_number*/, Symbol& /*parameter_symbol*/)
{
    return false;
}


std::shared_ptr<Symbol> UserFunctionArgumentEvaluator::GetSymbol(size_t /*parameter_number*/)
{
    return ReturnProgrammingError(nullptr);
}



// --------------------------------------------------------------------------
// NumericStringValuesOnlyUserFunctionArgumentEvaluator
// --------------------------------------------------------------------------

template<bool ArgumentsAreCorrectType>
NumericStringValuesOnlyUserFunctionArgumentEvaluator<ArgumentsAreCorrectType>::NumericStringValuesOnlyUserFunctionArgumentEvaluator(std::vector<std::variant<double, SharableString>> arguments)
    :   m_arguments(std::move(arguments))
{
}


template<bool ArgumentsAreCorrectType>
template<typename T>
T NumericStringValuesOnlyUserFunctionArgumentEvaluator<ArgumentsAreCorrectType>::GetArgument(const size_t parameter_number) const
{
    ASSERT(parameter_number < m_arguments.size());
    const std::variant<double, SharableString>& argument = m_arguments[parameter_number];

    if constexpr(!ArgumentsAreCorrectType)
    {
        if( !std::holds_alternative<T>(argument) )
            return Engine::Value::Invalid<T>().template get<T>();
    }

    ASSERT(std::holds_alternative<T>(argument));
    return std::get<T>(argument);
}


template<bool ArgumentsAreCorrectType>
double NumericStringValuesOnlyUserFunctionArgumentEvaluator<ArgumentsAreCorrectType>::GetNumeric(const size_t parameter_number)
{
    return GetArgument<double>(parameter_number);
}


template<bool ArgumentsAreCorrectType>
SharableString NumericStringValuesOnlyUserFunctionArgumentEvaluator<ArgumentsAreCorrectType>::GetString(const size_t parameter_number)
{
    return GetArgument<SharableString>(parameter_number);
}


template class NumericStringValuesOnlyUserFunctionArgumentEvaluator<true>;
template class NumericStringValuesOnlyUserFunctionArgumentEvaluator<false>;
