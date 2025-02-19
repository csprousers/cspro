#include "StdAfx.h"
#include "ErrorMessageDisplayer.h"
#include <mutex>


#ifdef WIN_DESKTOP

namespace
{
    std::unique_ptr<std::vector<std::string>> posted_messages;
    std::mutex posted_messages_mutex;
}


void ErrorMessage::Display(const cs::string_view_sz error_message_sv)
{
    Display(TC::ToWide(error_message_sv).c_str());
}


void ErrorMessage::PostMessageForDisplay(std::string error_message, const bool send_post_messages/* = true*/)
{
    std::lock_guard<std::mutex> lock(posted_messages_mutex);

    if( posted_messages == nullptr )
        posted_messages = std::make_unique<std::vector<std::string>>();

    posted_messages->emplace_back(std::move(error_message));

    if( send_post_messages )
        WindowsDesktopMessage::Post(UWM::ToolsO::DisplayErrorMessage);

    // make sure the posted messages are displayed
#ifdef _DEBUG
    class PostedMessageCheck
    {
    public:
        ~PostedMessageCheck()
        {
            ASSERT(posted_messages == nullptr);
        }
    };

    static PostedMessageCheck posted_messages_check;
#endif
}


void ErrorMessage::DisplayPostedMessages()
{
    if( posted_messages == nullptr )
        return;

    std::unique_ptr<std::vector<std::string>> these_posted_messages;

    {
        std::lock_guard<std::mutex> lock(posted_messages_mutex);
        these_posted_messages = std::move(posted_messages);
    }

    ASSERT(!these_posted_messages->empty());

    for( const std::string& error_message : *these_posted_messages )
        Display(error_message);
}


int AfxMessageBox(const std::string_view text_sv, const UINT nType/* = MB_OK*/, const UINT nIDHelp/* = 0*/)
{
    return AfxMessageBox(TC::ToWide(text_sv).c_str(), nType, nIDHelp);
}


#else

#include <zPlatformO/PlatformInterface.h>


void ErrorMessage::Display(const cs::string_view_sz error_message_sv)
{
    PlatformInterface::GetInstance()->GetApplicationInterface()->DisplayErrorMessage(error_message_sv);
}


void ErrorMessage::Display(const NullTerminatedString error_message)
{
    Display(TC::ToUtf8(error_message.c_str()));
}

#endif
