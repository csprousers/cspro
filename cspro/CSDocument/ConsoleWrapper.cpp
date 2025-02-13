#include "StdAfx.h"
#include "ConsoleWrapper.h"


ConsoleWrapper::ConsoleWrapper()
{
    // attach to the current console (if run from the command prompt)
    if( AttachConsole(ATTACH_PARENT_PROCESS) )
    {
        m_stdinHandle = nullptr;
    }

    // if not, create a new one
    else
    {
        if( !AllocConsole() )
            throw CSProException("A console could not be created");

        m_stdinHandle = GetStdHandle(STD_INPUT_HANDLE);
        ASSERT(m_stdinHandle != nullptr);

        // remove the ENABLE_PROCESSED_INPUT flag, which will allow us to process Ctrl+C
        DWORD console_mode;

        if( GetConsoleMode(m_stdinHandle, &console_mode) )
        {
            console_mode &= ~ENABLE_PROCESSED_INPUT;
            SetConsoleMode(m_stdinHandle, console_mode);
        }
    }

    m_stderrHandle = GetStdHandle(STD_ERROR_HANDLE);
    ASSERT(m_stderrHandle != nullptr);

}


ConsoleWrapper::~ConsoleWrapper()
{
    // when attached to the command prompt console, send an an Enter keystroke so that the program does not hang
    if( m_stdinHandle == nullptr)
    {
        // code from https://www.tillett.info/2013/05/13/how-to-create-a-windows-program-that-works-as-both-as-a-gui-and-console-application/
        INPUT ip;

        // Set up a generic keyboard event.
        ip.type = INPUT_KEYBOARD;
        ip.ki.wScan = 0; // hardware scan code for key
        ip.ki.time = 0;
        ip.ki.dwExtraInfo = 0;

        // Send the "Enter" key
        ip.ki.wVk = VK_RETURN;
        ip.ki.dwFlags = 0; // 0 for key press
        SendInput(1, &ip, sizeof(INPUT));

        // Release the "Enter" key
        ip.ki.dwFlags = KEYEVENTF_KEYUP; // KEYEVENTF_KEYUP for key release
        SendInput(1, &ip, sizeof(INPUT));
    }

    FreeConsole();
}


void ConsoleWrapper::WriteLine(wstring_view text_sv/* = wstring_view()*/)
{
    if( !text_sv.empty() )
        WriteConsole(m_stderrHandle, text_sv.data(), text_sv.length(), nullptr, nullptr);

    WriteConsole(m_stderrHandle, "\n", 1, nullptr, nullptr);
}


bool ConsoleWrapper::IsUserCancelingProgramAndReadInput()
{
    DWORD input_events;

    if( m_stdinHandle != nullptr && GetNumberOfConsoleInputEvents(m_stdinHandle, &input_events) )
    {
        for( ; input_events > 0; --input_events )
        {
            INPUT_RECORD input_record;
            DWORD input_records_read;

            if( ReadConsoleInput(GetStdHandle(STD_INPUT_HANDLE), &input_record, 1, &input_records_read) )
            {
                ASSERT(input_records_read == 1);

                if( input_record.EventType == KEY_EVENT && input_record.Event.KeyEvent.uChar.UnicodeChar == VK_CANCEL )
                    return true;
            }
        }
    }

    return false;
}
