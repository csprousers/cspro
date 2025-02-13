#pragma once

#include <zUtilF/UWM.h>


// an application can contain an instance of this class and have various
// operations occur upon application shutdown

class ApplicationShutdownRunner
{
public:
    ~ApplicationShutdownRunner()
    {
        for( auto& shutdown_operation : m_shutdownOperations )
            shutdown_operation();
    }

    static ApplicationShutdownRunner* Get()
    {
        CWnd* main_wnd = ( AfxGetApp() != nullptr ) ? AfxGetApp()->GetMainWnd() :
                                                      nullptr;

        return ( main_wnd != nullptr ) ? reinterpret_cast<ApplicationShutdownRunner*>(main_wnd->SendMessage(UWM::UtilF::GetApplicationShutdownRunner)) :
                                         nullptr;
    }

    void AddShutdownOperation(std::function<void()>&& operation)
    {
        m_shutdownOperations.emplace_back(operation);
    }

private:
    std::vector<std::function<void()>> m_shutdownOperations;
};
