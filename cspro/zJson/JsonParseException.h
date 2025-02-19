#pragma once

#include <zToolsO/CSProException.h>


class JsonParseException : public CSProException
{
public:
    using CSProException::CSProException;

    JsonParseException(int line_number, const std::string& message)
        :   CSProException(message.c_str()),
            m_lineNumber(line_number)
    {
        // json_exception exceptions will be rethrown as JsonParseException
        // exceptions with the line number of the parse error
    }

    int GetLineNumber() const { return m_lineNumber; }

private:
    const int m_lineNumber = -1;
};
