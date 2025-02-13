#pragma once

#include <engine/StandardSystemIncludes.h>
#include <engine/INTERPRE.H>
#include <zEngineO/UserFunction.h>


class DefaultParametersOnlyUserFunctionArgumentEvaluator : public UserFunctionArgumentEvaluator
{
public:
    DefaultParametersOnlyUserFunctionArgumentEvaluator(CIntDriver* pIntDriver, const UserFunction& user_function)
        :   m_pIntDriver(pIntDriver),
            m_userFunction(user_function)
    {
    }

    double GetNumeric(int parameter_number) override
    {
        int expression = m_userFunction.GetParameterDefaultValue(parameter_number);
        return m_pIntDriver->evalexpr(expression);
    }

    std::wstring GetString(int parameter_number) override
    {
        int expression = m_userFunction.GetParameterDefaultValue(parameter_number);
        return m_pIntDriver->EvalAlphaExpr(expression);
    }

protected:
    CIntDriver* m_pIntDriver;
    const UserFunction& m_userFunction;
};
