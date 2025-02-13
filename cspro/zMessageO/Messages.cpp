#include "stdafx.h"
#include "Messages.h"
#include "MessageFile.h"
#include "SystemMessages.h"


const std::wstring& MGF::GetMessageText(int message_number)
{
    return SystemMessages::GetMessageFile().GetMessageText(message_number);
}


std::wstring MGF::GetMessageText(int message_number, const TCHAR* default_text)
{
    ASSERT(default_text != nullptr);

    const std::wstring* message_text = SystemMessages::GetMessageFile().GetMessageTextWithNoDefaultMessage(message_number);

    return ( message_text != nullptr ) ? *message_text :
                                         default_text;
}
