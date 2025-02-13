#pragma once

#include <zMessageO/MessageParameterEvaluator.h>


class VariableArgumentsMessageParameterEvaluator : public MessageParameterEvaluator
{
public:
    VariableArgumentsMessageParameterEvaluator(va_list parg = va_list())
        :   m_parg(parg)
    {
    }

    void Reset(va_list parg)
    {
        m_parg = parg;
    }

    int GetInteger() override
    {
        return va_arg(m_parg, int);
    }

    double GetDouble() override
    {
        return va_arg(m_parg, double);
    }

    std::wstring GetString() override
    {
        return std::wstring(va_arg(m_parg, const TCHAR*));
    }

    wchar_t GetChar() override
    {
        return va_arg(m_parg, wchar_t);
    }

    std::wstring GetProc() override
    {
        return ReturnProgrammingError(std::wstring());
    }

    std::wstring GetVariable() override
    {
        return ReturnProgrammingError(std::wstring());
    }

    std::wstring GetVariableLabel() override
    {
        return ReturnProgrammingError(std::wstring());
    }

private:
    va_list m_parg;
};
