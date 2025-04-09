#pragma once


// a class that can be used to issue messages from...
// - the compiler (which will be thrown); or
// - the interpreter (which will be displayed but not thrown)

class MessageIssuer
{
public:
    virtual ~MessageIssuer() { }

    template<typename... Args>
    void IssueError(int message_number, Args const&... args)
    {
#ifdef _DEBUG
        ValidateFormatTextArgumentTypes<char>(args...);
#endif

        IssueErrorWorker(message_number, args...);
    }

protected:
    virtual void IssueErrorWorker(int message_number, ...) = 0;
};
