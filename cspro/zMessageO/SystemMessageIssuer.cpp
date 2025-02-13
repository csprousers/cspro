#include "stdafx.h"
#include "SystemMessageIssuer.h"
#include "MessageEvaluator.h"
#include "SystemMessages.h"
#include "VariableArgumentsMessageParameterEvaluator.h"


SystemMessageIssuer::SystemMessageIssuer(std::shared_ptr<VariableArgumentsMessageParameterEvaluator> message_parameter_evaluator/* = nullptr*/)
    :   m_messageEvaluator(std::make_unique<MessageEvaluator>(SystemMessages::GetSharedMessageFile())),
        m_variableArgumentsMessageParameterEvaluator(std::move(message_parameter_evaluator))
{
    if( m_variableArgumentsMessageParameterEvaluator == nullptr )
        m_variableArgumentsMessageParameterEvaluator = std::make_shared<VariableArgumentsMessageParameterEvaluator>();
}


SystemMessageIssuer::~SystemMessageIssuer()
{
}


std::wstring SystemMessageIssuer::GetFormattedMessageVA(int message_number, va_list parg)
{
    m_variableArgumentsMessageParameterEvaluator->Reset(parg);
    return m_messageEvaluator->GetFormattedMessage(*m_variableArgumentsMessageParameterEvaluator, message_number);
}


std::wstring SystemMessageIssuer::GetFormattedMessage(int message_number, ...)
{
    va_list parg;
    va_start(parg, message_number);
    std::wstring message_text = GetFormattedMessageVA(message_number, parg);
    va_end(parg);

    return message_text;
}
