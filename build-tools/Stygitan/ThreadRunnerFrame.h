#pragma once


// --------------------------------------------------------------------------
// ThreadRunnerFrame
//
// This is a simple CMDIChildWnd subclass that manages a thread object,
// handling cancelation of the thread when closing the frame.
//
// To start a thread, send a ThreadStart message to this frame with wParam
// set as a pointer to std::function<void(bool& cancel_flag).
//
// Before starting the thread, this frame will send the ThreadStart message
// to all immediate children descendant windows with the lParam value passed
// to ThreadStart.
//
// When a thread is complete, this frame will send the ThreadComplete message
// to all immediate children descendant windows with the lParam value passed
// to ThreadStart.
//
// This frame also processes ThreadIsActive and ThreadPromptCancel messages.
// --------------------------------------------------------------------------

class ThreadRunnerFrame : public CMDIChildWnd
{
    DECLARE_DYNCREATE(ThreadRunnerFrame)

protected:
    ThreadRunnerFrame();

protected:
    DECLARE_MESSAGE_MAP()

    void OnClose();

    LRESULT OnThreadStart(WPARAM wParam, LPARAM lParam);
    LRESULT OnThreadIsActive(WPARAM wParam, LPARAM lParam);
    LRESULT OnThreadPromptCancel(WPARAM wParam, LPARAM lParam);
    LRESULT OnThreadComplete(WPARAM wParam, LPARAM lParam);

private:
    std::unique_ptr<std::thread> m_thread;
    LPARAM m_threadId;
    bool m_cancelFlag;
};
