#pragma once

#include <zMessageO/SystemMessageIssuer.h>


// a simple class that issues system messages using the ErrorMessage::Display function

class MessageBoxSystemMessageIssuer : public SystemMessageIssuer
{
public:
    void OnIssue(MessageType /*message_type*/, int /*message_number*/, const std::string& message_text) override
    {
        ErrorMessage::Display(message_text);
    }

    void OnIssue(const Logic::ParserMessage& parser_message) override
    {
        ASSERT(false);
        ErrorMessage::Display(parser_message.message_text);
    }

    void OnAbort(const std::string& message_text) override
    {
        ASSERT(false);
        ErrorMessage::Display(message_text);
    }
};
