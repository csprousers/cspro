#pragma once

#include <zToolsO/zToolsO.h>
#include <zToolsO/CSProException.h>
#include <zToolsO/NullTerminatedString.h>


// the functions in this namespace facilitate displaying error messages using native message box UI

namespace ErrorMessage
{
#ifdef WIN_DESKTOP
    inline void Display(NullTerminatedString error_message)
    {
        AfxMessageBox(error_message.c_str(), MB_ICONEXCLAMATION);
    }

    // posts a message to the main window to display the error message
    CLASS_DECL_ZTOOLSO void PostMessageForDisplay(std::wstring error_message);

    inline void PostMessageForDisplay(const CSProException& exception)
    {
        PostMessageForDisplay(exception.GetErrorMessage());
    }

    // displays any messages sent to PostMessageForDisplay
    CLASS_DECL_ZTOOLSO void DisplayPostedMessages();

#else
    CLASS_DECL_ZTOOLSO void Display(NullTerminatedString error_message);

#endif

    inline void Display(const CSProException& exception)
    {
        Display(exception.GetErrorMessage());
    }
}
