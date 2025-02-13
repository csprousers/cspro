#include "StdAfx.h"
#include "ErrorMessageDisplayer.h"
#include <zPlatformO/PlatformInterface.h>
#include <mutex>


#ifdef WIN_DESKTOP

namespace
{
    std::vector<std::wstring> posted_messages;
    std::mutex posted_messages_mutex;
}


void ErrorMessage::PostMessageForDisplay(std::wstring error_message)
{
    std::lock_guard<std::mutex> lock(posted_messages_mutex);
    posted_messages.emplace_back(std::move(error_message));

    AfxGetMainWnd()->PostMessage(UWM::ToolsO::DisplayErrorMessage);

    // make sure the posted messages are displayed
#ifdef _DEBUG
    class PostedMessageCheck
    {
    public:
        ~PostedMessageCheck()
        {
            ASSERT(posted_messages.empty());
        }
    };

    static PostedMessageCheck posted_messages_check;
#endif
}


void ErrorMessage::DisplayPostedMessages()
{
    std::lock_guard<std::mutex> lock(posted_messages_mutex);

    ASSERT(!posted_messages.empty());

    for( const std::wstring& error_message : posted_messages )
        Display(error_message);

    posted_messages.clear();
}


#else

void ErrorMessage::Display(NullTerminatedString error_message)
{
    PlatformInterface::GetInstance()->GetApplicationInterface()->DisplayErrorMessage(error_message.c_str());
}

#endif
