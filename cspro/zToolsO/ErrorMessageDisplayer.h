#pragma once

#include <zToolsO/zToolsO.h>
#include <zToolsO/NullTerminatedString.h>
#include <zToolsO/string_sz.h>


// the functions in this namespace facilitate displaying error messages using native message box UI

namespace ErrorMessage
{
    // displays the error message
    CLASS_DECL_ZTOOLSO void Display(cs::string_view_sz error_message_sv);

#ifndef WIN_DESKTOP
    CLASS_DECL_ZTOOLSO
#endif
    void Display(NullTerminatedString error_message);

    // displays the exceptions's message
    inline void Display(const std::exception& exception) { Display(exception.what()); }

    // adds the message for eventual displaying;
    // if send_post_messages is true, the UWM::ToolsO::DisplayErrorMessage message is posted
    CLASS_DECL_ZTOOLSO void PostMessageForDisplay(std::string error_message, bool send_post_messages = true);

    inline void PostMessageForDisplay(const std::exception& exception) { PostMessageForDisplay(std::string(exception.what())); }

    // displays any messages sent to PostMessageForDisplay
    CLASS_DECL_ZTOOLSO void DisplayPostedMessages();
}


#ifdef _AFX

// --------------------------------------------------------------------------
// AfxMessageBox overrides
// --------------------------------------------------------------------------

inline int AfxMessageBox(const std::wstring& text, UINT nType = MB_OK, UINT nIDHelp = 0)
{
    return AfxMessageBox(text.c_str(), nType, nIDHelp);
}

CLASS_DECL_ZTOOLSO int AfxMessageBox(const std::string_view text_sv, UINT nType = MB_OK, UINT nIDHelp = 0);


// --------------------------------------------------------------------------
// inline implementations
// --------------------------------------------------------------------------

inline void ErrorMessage::Display(const NullTerminatedString error_message)
{
    AfxMessageBox(error_message.c_str(), MB_ICONEXCLAMATION);
}

#endif // _AFX
