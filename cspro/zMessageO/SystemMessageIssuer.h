#pragma once

#include <zMessageO/MessageType.h>
#include <zMessageO/SystemMessageFormatter.h>
#include <zLogicO/ParserMessage.h>


namespace MGF
{
    constexpr int OpenMessage = 32001; // %s
}


class SystemMessageIssuer : public SystemMessageFormatter
{
public:
    using SystemMessageFormatter::SystemMessageFormatter;

    virtual ~SystemMessageIssuer() { }

    template<typename... Args>
    void Issue(MessageType message_type, int message_number, Args const&... args);
    void Issue(MessageType message_type, const std::string& message);
    void Issue(MessageType message_type, const CSProException& exception);

    void IssueOrThrow(bool throw_exception, MessageType message_type, int message_number, ...);

    void IssueFormattedMessage(MessageType message_type, int message_number, const std::string& message_text);

    void IssueVA(MessageType message_type, int message_number, va_list parg);

    void IssueVA(Logic::ParserMessage& parser_message, va_list parg);

protected:
    virtual void OnIssue(MessageType message_type, int message_number, const std::string& message_text) = 0;

    virtual void OnIssue(const Logic::ParserMessage& parser_message) = 0;

    virtual void OnAbort(const std::string& message_text) = 0;

private:
    void IssueWorker(MessageType message_type, int message_number, ...);
};



// --------------------------------------------------------------------------
// inline implementations
// --------------------------------------------------------------------------

template<typename... Args>
void SystemMessageIssuer::Issue(const MessageType message_type, const int message_number, Args const&... args)
{
#ifdef _DEBUG
    ValidateFormatTextArgumentTypes<char>(args...);
#endif

    IssueWorker(message_type, message_number, args...);
}


inline void SystemMessageIssuer::IssueWorker(const MessageType message_type, const int message_number, ...)
{
    va_list parg;
    va_start(parg, message_number);
    IssueVA(message_type, message_number, parg);
    va_end(parg);
}


inline void SystemMessageIssuer::Issue(const MessageType message_type, const std::string& message)
{
    Issue(message_type, MGF::OpenMessage, message.c_str());
}


inline void SystemMessageIssuer::Issue(const MessageType message_type, const CSProException& exception)
{
    Issue(message_type, exception.what());
}


inline void SystemMessageIssuer::IssueOrThrow(const bool throw_exception, const MessageType message_type, const int message_number, ...)
{
    va_list parg;
    va_start(parg, message_number);

    if( throw_exception )
    {
        const std::string message_text = GetFormattedMessageVA(message_number, parg);
        va_end(parg);
        throw CSProException(message_text);
    }

    else
    {
        IssueVA(message_type, message_number, parg);
        va_end(parg);
    }
}


inline void SystemMessageIssuer::IssueFormattedMessage(const MessageType message_type, const int message_number, const std::string& message_text)
{
    OnIssue(message_type, message_number, message_text);
}


inline void SystemMessageIssuer::IssueVA(const MessageType message_type, const int message_number, va_list parg)
{
    ASSERT(message_type != MessageType::User);

    const std::string message_text = GetFormattedMessageVA(message_number, parg);

    OnIssue(message_type, message_number, message_text);

    if( message_type == MessageType::Abort )
        OnAbort(message_text);
}


inline void SystemMessageIssuer::IssueVA(Logic::ParserMessage& parser_message, va_list parg)
{
    parser_message.message_text = GetFormattedMessageVA(parser_message.message_number, parg);
    OnIssue(parser_message);
}
