#pragma once

#include <zToolsO/TextFormatter.h>
#include <stdexcept>


// --------------------------------------------------------------------------
// CSProException
// --------------------------------------------------------------------------

class CSProException : public std::runtime_error
{
public:
    typedef std::runtime_error std_runtime_error;
    using std_runtime_error::std_runtime_error;

    explicit CSProException(const std::exception& exception)
        :   std::runtime_error(exception.what())
    {
    }

    explicit CSProException(const char* message)
        :   std::runtime_error(message)
    {
    }

    explicit CSProException(const std::string& message)
        :   std::runtime_error(message.c_str())
    {
    }

    template<typename... Args>
    explicit CSProException(const char* formatter, Args const&... args)
        :   std::runtime_error(FormatText(formatter, args...))
    {
    }
};



// --------------------------------------------------------------------------
// CSProExceptionWithFilePath: holds a file path where the error occurred
// --------------------------------------------------------------------------

class CSProExceptionWithFilePath : public CSProException
{
public:
    template<typename... Args>
    CSProExceptionWithFilePath(std::string file_path, Args const&... args)
        :   CSProException(args...),
            m_filePath(std::move(file_path))
    {
    }

    const std::string& GetFilePath() const { return m_filePath; }

private:
    std::string m_filePath;
};



// --------------------------------------------------------------------------
// other CSProException subclasses
// --------------------------------------------------------------------------

#define CREATE_CSPRO_EXCEPTION(class_name)    \
    struct class_name : public CSProException \
    {                                         \
        using CSProException::CSProException; \
    }


#define CREATE_CSPRO_EXCEPTION_WITH_MESSAGE(class_name, message) \
    struct class_name : public CSProException                    \
    {                                                            \
        explicit class_name() : CSProException(message) { }      \
    }


CREATE_CSPRO_EXCEPTION_WITH_MESSAGE(UserCanceledException, "Operation canceled by user.");



// --------------------------------------------------------------------------
// ProgrammingErrorException + ReturnProgrammingError
// --------------------------------------------------------------------------

CREATE_CSPRO_EXCEPTION_WITH_MESSAGE(ProgrammingErrorException, "Programming error: please report what was happening when you saw this to cspro@lists.census.gov");

template<typename T>
T ReturnProgrammingError(T&& value)
{
    ASSERT(false);
#ifdef _DEBUG
    value; // suppress an unused value warning
    throw ProgrammingErrorException();
#else
    return std::forward<T>(value);
#endif
}
