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
    m_variableArgumentsMessageParameterEvaluator->Reset(parg);
    return m_messageEvaluator->GetFormattedMessage(*m_variableArgumentsMessageParameterEvaluator, message_number);
}


std::string SystemMessageFormatter::GetFormattedMessageWorker(const int message_number, ...)
{
    va_list parg;
    va_start(parg, message_number);
    std::string message_text = GetFormattedMessageVA(message_number, parg);
    va_end(parg);

    return message_text;
}
