#pragma once

#include <zToolsO/CSProException.h>


namespace ActionInvoker
{
    class ExceptionWithActionName : public CSProException
    {
    public:
        ExceptionWithActionName(const CSProException& exception, std::wstring action_name)
            :   CSProException(exception),
                m_actionName(std::move(action_name))
        {
            ASSERT(!m_actionName.empty());
        }

        const std::wstring& GetActionName() const { return m_actionName; }

        std::wstring GetErrorMessageWithActionName() const
        {
            return FormatTextCS2WS(_T("Action Invoker error running action '%s': %s"), m_actionName.c_str(), GetErrorMessage().c_str());
        }

        static std::wstring GetErrorMessageFromCSProException(const CSProException& exception)
        {
            return _T("Action Invoker error: ") + exception.GetErrorMessage();
        }

    private:
        const std::wstring m_actionName;
    };
}
