#pragma once

#include <zMessageO/MessageParameterEvaluator.h>


class VariableArgumentsMessageParameterEvaluator : public MessageParameterEvaluator
{
public:
    VariableArgumentsMessageParameterEvaluator(va_list* const parg = nullptr)
        :   m_parg(parg)
    {
    }

    void Reset(va_list* const parg)
    {
        m_parg = parg;
    }

    int GetInteger() override
    {
        ASSERT(m_parg != nullptr);
        return va_arg(*m_parg, int);
    }

    double GetDouble() override
    {
        ASSERT(m_parg != nullptr);
        return va_arg(*m_parg, double);
    }

    SharableString GetString() override
    {
        ASSERT(m_parg != nullptr);
        return SharableString(va_arg(*m_parg, const char*));
    }

    std::variant<int, SharableString> GetChar() override
    {
        ASSERT(m_parg != nullptr);
        return va_arg(*m_parg, int);
    }

    SharableString GetProc() override
    {
        return ReturnProgrammingError(SharableString());
    }

    SharableString GetVariable() override
    {
        return ReturnProgrammingError(SharableString());
    }

    SharableString GetVariableLabel() override
    {
        return ReturnProgrammingError(SharableString());
    }

private:
    va_list* m_parg;
};
