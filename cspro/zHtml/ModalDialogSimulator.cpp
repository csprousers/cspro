#include "stdafx.h"
#include "ModalDialogSimulator.h"


namespace
{
    // a RAII class for disabling the top window, whether it is the main window or
    // another simulated modal dialog, and then reenabling it on destruction
    class TopWindowDisabler
    {
    public:
        TopWindowDisabler(CDialog* dialog)
            :   m_parentWindow(m_simulatedModalDialogStack.empty() ? AfxGetMainWnd() : m_simulatedModalDialogStack.top()),
                m_dialog(dialog)
        {
            m_parentWindow->EnableWindow(FALSE);
            m_simulatedModalDialogStack.emplace(m_dialog);
        }

        CWnd* GetParentWindow() { return m_parentWindow; }

        ~TopWindowDisabler()
        {
            ASSERT(m_simulatedModalDialogStack.top() == m_dialog);
            m_simulatedModalDialogStack.pop();

            m_parentWindow->EnableWindow(TRUE);
        }

    private:
        CWnd* m_parentWindow;
        CDialog* m_dialog;

        static std::stack<CWnd*> m_simulatedModalDialogStack;
    };

    std::stack<CWnd*> TopWindowDisabler::m_simulatedModalDialogStack;
}


void SimulateModalDialog(CDialog* dialog, std::function<void(CWnd*)> create_dialog_callback)
{
    if( AfxGetMainWnd() == nullptr )
    {
        // right now this dialog is always used when a main window exists
        ASSERT(false);
        dialog->CDialog::DoModal();
        return;
    }

    // this should really be a modal dialog but there are intermittent failures
    // with the web view when the dialog is modal; instead we make it modeless, 
    // disable the main window (or any other dialogs showing), and run our own
    // event loop to simulate a modal dialog
    auto top_window_disabler = std::make_unique<TopWindowDisabler>(dialog);

    create_dialog_callback(top_window_disabler->GetParentWindow());

    MSG msg;

    while( dialog->IsWindowVisible() && GetMessage(&msg, nullptr, 0, 0) )
    {
        if( !dialog->IsDialogMessage(&msg) )
        {
            TranslateMessage(&msg);
            DispatchMessage(&msg);
        }
    }

    // remove this dialog from the dialog stack and renable the previously-enabled window
    top_window_disabler.reset();

    // destroy the window because, if this is executed via DoModalOnUIThread,
    // we need to ensure that the window is destroyed on the UI thread
    dialog->DestroyWindow();
}
