#include "StdAfx.h"
#include "ExceptionHolder.h"
#include "VariantVisitOverload.h"
#include <zAction/ActionInvoker.h>


ExceptionHolder::ExceptionHolder()
{
}


ExceptionHolder::~ExceptionHolder()
{
    if( std::holds_alternative<std::monostate>(m_exception) )
        return;

    std::visit(
        overload
        {
            [](std::monostate)
            {
                ASSERT(false);
            },

            [](const std::unique_ptr<const ActionInvoker::Exception>& exception)
            {
                ErrorMessage::Display(GetMessageToDisplay(*exception));
            },

            [](const auto& exception)
            {
                ErrorMessage::Display(exception->what());
            }

        }, m_exception);
}


void ExceptionHolder::AddException(std::unique_ptr<const CSProException> exception)
{
    ASSERT(std::holds_alternative<std::monostate>(m_exception));
    m_exception = std::move(exception);
}


void ExceptionHolder::AddActionInvokerException(std::unique_ptr<const ActionInvoker::Exception> exception)
{
    ASSERT(std::holds_alternative<std::monostate>(m_exception));
    m_exception = std::move(exception);
}


void ExceptionHolder::ThrowExceptions()
{
    if( std::holds_alternative<std::monostate>(m_exception) )
        return;

    std::visit(
        overload
        {
            [](std::monostate)        { ASSERT(false); },
            [](const auto& exception) { throw *exception; }

        }, std::exchange(m_exception, std::monostate()));
}


std::string ExceptionHolder::GetMessageToDisplay(const ActionInvoker::Exception& exception)
{
    return SO::Concatenate("Uncaught Action Invoker Exception: ", exception.what());
}
