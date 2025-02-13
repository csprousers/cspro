#pragma once


class ConsoleWrapper
{
public:
    // attaches to the current console (from a command prompt), or creates a console, throwing an exception on error;
    // when attached to the current console, input is ignored
    ConsoleWrapper();
    ~ConsoleWrapper();

    // writes the text to stderr
    void WriteLine(wstring_view text_sv = wstring_view());

    // peeks stdin and returns true if the user is canceling the program with Ctrl+C;
    // any input available is read and discarded
    bool IsUserCancelingProgramAndReadInput();

private:
    HANDLE m_stderrHandle;
    HANDLE m_stdinHandle;
};
