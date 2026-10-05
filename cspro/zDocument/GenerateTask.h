#pragma once

#include <zDocument/GenerateTaskInterface.h>
#include <thread>


class GenerateTask
{
public:
    virtual ~GenerateTask();

    virtual void ValidateInputs() { }

    bool IsInterfaceSet() const noexcept                          { return ( m_interface != nullptr ); }
    GenerateTaskInterface& GetInterface()                         { return *m_interface; }
    void SetInterface(GenerateTaskInterface& interface_) noexcept { m_interface = &interface_; }

    void Run();
    bool IsRunning() const { return ( m_status == GenerateTaskStatus::Running ); }

    void Cancel();
    bool IsCanceled() const { return ( m_status == GenerateTaskStatus::Canceled ); }

protected:
    // subclasses must override OnRun
    virtual void OnRun() = 0;

private:
    GenerateTaskInterface* m_interface = nullptr;
    std::thread m_runThread;
    GenerateTaskStatus m_status = GenerateTaskStatus::NotStarted;
};



// --------------------------------------------------------------------------
// inline implementations
// --------------------------------------------------------------------------

inline GenerateTask::~GenerateTask()
{
    if( m_runThread.joinable() )
        m_runThread.join();
}


inline void GenerateTask::Run()
{
    ASSERT(m_interface != nullptr && m_status == GenerateTaskStatus::NotStarted);

    m_runThread = std::thread(
        [&]()
        {
            try
            {
                m_status = GenerateTaskStatus::Running;
                OnRun();
            }

            catch( const CSProException& exception )
            {
                m_status = GenerateTaskStatus::EndedInException;
                GetInterface().OnException(exception);
            }

            if( m_status == GenerateTaskStatus::Running )
            {
                m_status = GenerateTaskStatus::Complete;
                GetInterface().UpdateProgress(100);
            }

            GetInterface().OnCompletion(m_status);
        });
}


inline void GenerateTask::Cancel()
{
    if( IsRunning() )
    {
        m_status = GenerateTaskStatus::Canceled;
        m_runThread.join();
    }
}
