#pragma once

#include <zMessageO/zMessageO.h>

class MessageEvaluator;
class VariableArgumentsMessageParameterEvaluator;


class ZMESSAGEO_API SystemMessageFormatter
{
public:
    SystemMessageFormatter(std::shared_ptr<VariableArgumentsMessageParameterEvaluator> message_parameter_evaluator = nullptr);
    ~SystemMessageFormatter();

    template<typename... Args>
    std::string GetFormattedMessage(int message_number, Args const&... args);

    std::string GetFormattedMessageVA(int message_number, va_list parg);

private:
    std::string GetFormattedMessageWorker(int message_number, ...);

private:
    std::unique_ptr<MessageEvaluator> m_messageEvaluator;
    std::shared_ptr<VariableArgumentsMessageParameterEvaluator> m_variableArgumentsMessageParameterEvaluator;
};



// --------------------------------------------------------------------------
// inline implementations
// --------------------------------------------------------------------------

template<typename... Args>
std::string SystemMessageFormatter::GetFormattedMessage(const int message_number, Args const&... args)
{
#ifdef _DEBUG
    ValidateFormatTextArgumentTypes(args...);
#endif

    return GetFormattedMessageWorker(message_number, args...);
}
