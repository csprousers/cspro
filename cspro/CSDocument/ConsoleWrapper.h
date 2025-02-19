#pragma once


class ConsoleWrapper
{
public:
    // attaches to the current console (from a command prompt), or creates a console, throwing an exception on error;
    // when attached to the current console, input is ignored
    ConsoleWrapper();
    ~ConsoleWrapper();

    // writes the text to stderr
    void WriteLine(std::wstring_view text_sv = std::wstring_view());
    void WriteLine(std::string_view text_sv);

    template<typename... Args>
    void WriteLine(const char* formatter, Args const&... args);

    // peeks stdin and returns true if the user is canceling the program with Ctrl+C;
    // any input available is read and discarded
    bool IsUserCancelingProgramAndReadInput();

private:
    HANDLE m_stderrHandle;
    HANDLE m_stdinHandle;
};



// --------------------------------------------------------------------------
// inline implementations
// --------------------------------------------------------------------------

inline void ConsoleWrapper::WriteLine(const std::string_view text_sv)
{
    return WriteLine(TC::ToWide(text_sv));
}


template<typename... Args>
void ConsoleWrapper::WriteLine(const char* const formatter, Args const&... args)
{
    WriteLine(FormatText(formatter, args...));
}
