#include "stdafx.h"
#include "Messages.h"
#include "MessageFile.h"
#include "SystemMessages.h"


SharableString MGF::GetMessageText(const int message_number)
{
    return SystemMessages::GetMessageFile().GetMessageText(message_number);
}


SharableString MGF::GetMessageText(const int message_number, const char* const default_text)
{
    ASSERT(default_text != nullptr);

    SharableString message_text = SystemMessages::GetMessageFile().GetMessageTextWithNoDefaultMessage(message_number);

    return message_text.IsSet() ? message_text :
                                  default_text;
}
