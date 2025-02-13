#pragma once

#include <zToolsO/zToolsO.h>
#include <zToolsO/StringView.h>
#include <zToolsO/TextFormatter.h>
#include <stdexcept>


// --------------------------------------------------------------------------
// CSProException
// --------------------------------------------------------------------------

class CLASS_DECL_ZTOOLSO CSProException : public std::runtime_error
{
public:
    typedef std::runtime_error std_runtime_error;
    using std_runtime_error::std_runtime_error;

    explicit CSProException(const char* message)
        :   std::runtime_error(message)
    {
    }

    explicit CSProException(const wchar_t* message)
        :   std::runtime_error(CreateUtf8Message(message))
    {
    }

    explicit CSProException(const CString& message)
        :   std::runtime_error(CreateUtf8Message(message))
    {
    }

    explicit CSProException(const std::wstring& message)
        :   std::runtime_error(CreateUtf8Message(message))
    {
    }

    template<typename... Args>
    explicit CSProException(const TCHAR* formatter, Args const&... args)
        :   std::runtime_error(CreateUtf8Message(FormatText(formatter, args...)))
    {
    }

    std::wstring GetErrorMessage() const
    {
        return GetErrorMessage(*this);
    }

    static std::wstring GetErrorMessage(const std::exception& exception);

private:
    static std::string CreateUtf8Message(wstring_view message);
};



// --------------------------------------------------------------------------
// CSProExceptionWithFilename: holds a filename where the error occurred
// --------------------------------------------------------------------------

class CSProExceptionWithFilename : public CSProException
{
public:
    template<typename... Args>
    CSProExceptionWithFilename(std::wstring filename, Args const&... args)
        :   CSProException(args...),
            m_filename(std::move(filename))
    {
    }

    const std::wstring& GetFilename() const { return m_filename; }

private:
    std::wstring m_filename;
};



// --------------------------------------------------------------------------
// other CSProException subclasses
// --------------------------------------------------------------------------

#define CREATE_CSPRO_EXCEPTION(class_name)    \
    struct class_name : public CSProException \
    {                                         \
        using CSProException::CSProException; \
    };


#define CREATE_CSPRO_EXCEPTION_WITH_MESSAGE(class_name, message) \
    struct class_name : public CSProException                    \
    {                                                            \
        explicit class_name() : CSProException(message) { }      \
    };


CREATE_CSPRO_EXCEPTION_WITH_MESSAGE(UserCanceledException, "Operation canceled by user")



// --------------------------------------------------------------------------
// ProgrammingErrorException + ReturnProgrammingError
// --------------------------------------------------------------------------

CREATE_CSPRO_EXCEPTION_WITH_MESSAGE(ProgrammingErrorException, "Programming error: please report what was happening when you saw this to cspro@lists.census.gov")

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
