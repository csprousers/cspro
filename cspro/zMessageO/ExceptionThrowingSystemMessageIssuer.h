#pragma once

#include <zMessageO/SystemMessageIssuer.h>


// a simple class that issues system messages by throwing a CSProException

class ExceptionThrowingSystemMessageIssuer : public SystemMessageIssuer
{
public:
    [[noreturn]] void OnIssue(MessageType /*message_type*/, int /*message_number*/, const std::wstring& message_text) override
    {
        throw CSProException(message_text);
    }

    [[noreturn]] void OnIssue(const Logic::ParserMessage& parser_message) override
    {
        throw CSProException(parser_message.message_text);
    }

    [[noreturn]] void OnAbort(const std::wstring& message_text) override
    {
        throw CSProException(message_text);
    }
};
