#pragma once

#include <zUtilF/UWM.h>
#include <zToolsO/WindowsDesktopMessage.h>


// --------------------------------------------------------------------------
// UIThreadRunner
// --------------------------------------------------------------------------

class UIThreadRunner
{
public:
    virtual ~UIThreadRunner() { }

    bool RunOnUIThread()
    {
        return ( WindowsDesktopMessage::Send(UWM::UtilF::RunOnUIThread, this) == 1 );
    }

    virtual void Execute() = 0;
};



// --------------------------------------------------------------------------
// DialogUIThreadRunner
// --------------------------------------------------------------------------

class DialogUIThreadRunner : public UIThreadRunner
{
public:
    DialogUIThreadRunner(CDialog* pDialog)
        :   m_pDialog(pDialog),
            m_result(IDCANCEL)
    {
    }

    void Execute() override
    {
        m_result = m_pDialog->DoModal();
    }

    INT_PTR DoModal()
    {
        RunOnUIThread();
        return m_result;
    }

private:
    CDialog* m_pDialog;
    INT_PTR m_result;
};



// --------------------------------------------------------------------------
// RunOnUIThread: a callback function-based approach that creates a
// a UIThreadRunner subclass to wrap the callback function
// --------------------------------------------------------------------------

template<typename CF>
bool RunOnUIThread(const CF& callback_function)
{
    class CallbackFunctionUIThreadRunner : public UIThreadRunner
    {
    public:
        CallbackFunctionUIThreadRunner(const CF& callback_function)
            :   m_callbackFunction(callback_function)
        {
        }

        void Execute() override
        {
            m_callbackFunction();
        }

    private:
        const CF& m_callbackFunction;
    };

    CallbackFunctionUIThreadRunner ui_thread_runner(callback_function);
    return ui_thread_runner.RunOnUIThread();
}
