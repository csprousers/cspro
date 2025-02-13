#pragma once

#include <zMessageO/zMessageO.h>
#include <zMessageO/MessageType.h>
#include <zLogicO/ParserMessage.h>

class MessageEvaluator;
class VariableArgumentsMessageParameterEvaluator;

namespace MGF
{
    constexpr int OpenMessage = 32001; // %s
}


class ZMESSAGEO_API SystemMessageIssuer
{
public:
    SystemMessageIssuer(std::shared_ptr<VariableArgumentsMessageParameterEvaluator> message_parameter_evaluator = nullptr);

    virtual ~SystemMessageIssuer();

    std::wstring GetFormattedMessageVA(int message_number, va_list parg);
    std::wstring GetFormattedMessage(int message_number, ...);

    void Issue(MessageType message_type, int message_number, ...)
    {
        va_list parg;
        va_start(parg, message_number);
        IssueVA(message_type, message_number, parg);
        va_end(parg);
    }

    void Issue(MessageType message_type, const std::wstring& message)
    {
        Issue(message_type, MGF::OpenMessage, message.c_str());
    }

    void Issue(MessageType message_type, const CSProException& exception)
    {
        Issue(message_type, exception.GetErrorMessage());
    }

    void IssueOrThrow(bool throw_exception, MessageType message_type, int message_number, ...)
    {
        va_list parg;
        va_start(parg, message_number);

        if( throw_exception )
        {
            const std::wstring message_text = GetFormattedMessageVA(message_number, parg);
            va_end(parg);
            throw CSProException(message_text);
        }

        else
        {
            IssueVA(message_type, message_number, parg);
            va_end(parg);
        }
    }

    void IssueVA(MessageType message_type, int message_number, va_list parg)
    {
        ASSERT(message_type != MessageType::User);

        std::wstring message_text = GetFormattedMessageVA(message_number, parg);

        OnIssue(message_type, message_number, message_text);

        if( message_type == MessageType::Abort )
            OnAbort(message_text);
    }

    void IssueVA(Logic::ParserMessage& parser_message, va_list parg)
    {
        parser_message.message_text = GetFormattedMessageVA(parser_message.message_number, parg);
        OnIssue(parser_message);
    }

protected:
    virtual void OnIssue(MessageType message_type, int message_number, const std::wstring& message_text) = 0;

    virtual void OnIssue(const Logic::ParserMessage& /*parser_message*/) { }

    virtual void OnAbort(const std::wstring& /*message_text*/) { } 

private:
    std::unique_ptr<MessageEvaluator> m_messageEvaluator;
    std::shared_ptr<VariableArgumentsMessageParameterEvaluator> m_variableArgumentsMessageParameterEvaluator;
};
