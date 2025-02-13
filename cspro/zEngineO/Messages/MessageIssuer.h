#pragma once


// a class that can be used to issue messages from...
// - the compiler (which will be thrown); or
// - the interpreter (which will be displayed but not thrown)

class MessageIssuer
{
public:
    virtual ~MessageIssuer() { }

    virtual void IssueError(int message_number, ...) = 0;
};
