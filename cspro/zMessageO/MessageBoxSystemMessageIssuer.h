#pragma once

#include <zMessageO/SystemMessageIssuer.h>


// a simple class that issues system messages using the ErrorMessage::Display function

class MessageBoxSystemMessageIssuer : public SystemMessageIssuer
{
public:
    void OnIssue(MessageType /*message_type*/, int /*message_number*/, const std::wstring& message_text) override
    {
        ErrorMessage::Display(message_text);
    }
};
