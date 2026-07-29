#include "stdafx.h"
#include "SystemMessageFormatter.h"
#include "MessageEvaluator.h"
#include "SystemMessages.h"
#include "VariableArgumentsMessageParameterEvaluator.h"


SystemMessageFormatter::SystemMessageFormatter(std::shared_ptr<VariableArgumentsMessageParameterEvaluator> message_parameter_evaluator/* = nullptr*/)
    :   m_messageEvaluator(std::make_unique<MessageEvaluator>(SystemMessages::GetSharedMessageFile())),
        m_variableArgumentsMessageParameterEvaluator(std::move(message_parameter_evaluator))
{
    if( m_variableArgumentsMessageParameterEvaluator == nullptr )
        m_variableArgumentsMessageParameterEvaluator = std::make_shared<VariableArgumentsMessageParameterEvaluator>();
}


SystemMessageFormatter::~SystemMessageFormatter()
{
}


std::string SystemMessageFormatter::GetFormattedMessageVA(const int message_number, va_list parg)
{
    // on x86_64, parg will undergo array decay and cannot be passed as an argument to
    // VariableArgumentsMessageParameterEvaluator::Reset, so create a local copy
    va_list local_parg;
    va_copy(local_parg, parg);

    m_variableArgumentsMessageParameterEvaluator->Reset(&local_parg);

    std::string message = m_messageEvaluator->GetFormattedMessage(*m_variableArgumentsMessageParameterEvaluator, message_number);

    va_end(local_parg);

    return message;
}


std::string SystemMessageFormatter::GetFormattedMessageWorker(const int message_number, ...)
{
    va_list parg;
    va_start(parg, message_number);
    std::string message_text = GetFormattedMessageVA(message_number, parg);
    va_end(parg);

    return message_text;
}
