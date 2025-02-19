#pragma once

#include <zToolsO/zToolsO.h>

namespace ActionInvoker { class Exception; }


// --------------------------------------------------------------------------
// ExceptionHolder
//
// The ExceptionHolder class allows an exception to be stored and then thrown
// at a later point.
//
// If the exception has not been thrown when the object is destructed, it
// will be displayed using ErrorMessage::Display.
// --------------------------------------------------------------------------

class CLASS_DECL_ZTOOLSO ExceptionHolder
{
public:
    ExceptionHolder();
    ~ExceptionHolder();

    void AddException(std::unique_ptr<const CSProException> exception);
    void AddActionInvokerException(std::unique_ptr<const ActionInvoker::Exception> exception);

    void ThrowExceptions();

    static std::string GetMessageToDisplay(const ActionInvoker::Exception& exception);

private:
    std::variant<std::monostate,
                 std::unique_ptr<const CSProException>,
                 std::unique_ptr<const ActionInvoker::Exception>> m_exception;
};
