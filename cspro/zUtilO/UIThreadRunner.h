#pragma once

#include <zUtilO/zUtilO.h>
#include <zUtilO/UWM.h>
#include <zToolsO/WindowsDesktopMessage.h>


// --------------------------------------------------------------------------
// UIThreadRunner
//
// Features to support running code on the UI thread, either synchronously
// or asynchronously.
//
// To support this, a frame or view should handle the message
// UWM::UtilO::RunOnUIThread.
//
// For a synchronous operation, subclass UIThreadRunner and override the
// Execute method. Two convenience classes exist with this functionality:
//
//     - DialogUIThreadRunner: Executes a CDialog's DoModal method on the
//       UI thread.
//
//     - RunOnUIThreadSync: Executes a callback function specified using
//       std::function.
//
// For an asynchronous operation, use the RunOnUIThreadAsync function,
// passing a callback function specified using std::function. The function
// will be stored (in a static object in zUtilO) and then posted to the frame
// or view.
// --------------------------------------------------------------------------

class UIThreadRunner
{
public:
    virtual ~UIThreadRunner() { }

    bool RunOnUIThread() const
    {
        return ( WindowsDesktopMessage::Send(UWM::UtilO::RunOnUIThread, this) == 1 );
    }

    virtual void Execute(LPARAM lParam) = 0;
};



// --------------------------------------------------------------------------
// DialogUIThreadRunner
// --------------------------------------------------------------------------

class DialogUIThreadRunner : public UIThreadRunner
{
public:
    DialogUIThreadRunner(CDialog* const pDialog)
        :   m_pDialog(pDialog),
            m_result(IDCANCEL)
    {
    }

    void Execute(LPARAM /*lParam*/) override
    {
        m_result = m_pDialog->DoModal();
    }

    INT_PTR DoModal()
    {
        RunOnUIThread();
        return m_result;
    }

    INT_PTR DoModalOnUIThreadOrOnMainThread()
    {
        return RunOnUIThread() ? m_result :
                                 m_pDialog->DoModal();
    }

private:
    CDialog* m_pDialog;
    INT_PTR m_result;
};



// --------------------------------------------------------------------------
// RunOnUIThreadSync
// --------------------------------------------------------------------------

template<typename CF>
bool RunOnUIThreadSync(const CF& callback_function)
{
    class CallbackFunctionUIThreadRunner : public UIThreadRunner
    {
    public:
        CallbackFunctionUIThreadRunner(const CF& callback_function)
            :   m_callbackFunction(callback_function)
        {
        }

        void Execute(LPARAM /*lParam*/) override
        {
            m_callbackFunction();
        }

    private:
        const CF& m_callbackFunction;
    };

    CallbackFunctionUIThreadRunner ui_thread_runner(callback_function);
    return ui_thread_runner.RunOnUIThread();
}



// --------------------------------------------------------------------------
// RunOnUIThreadAsync
// --------------------------------------------------------------------------

CLASS_DECL_ZUTILO void RunOnUIThreadAsync(std::function<void()> callback_function);
