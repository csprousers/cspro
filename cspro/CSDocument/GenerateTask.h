#pragma once

#include <thread>

class GlobalSettings;


class GenerateTask
{
public:
    enum class Status { NotStarted, Running, Complete, Canceled, EndedInException };

    class Interface
    {
    public:
        virtual ~Interface() { }

        virtual void SetTitle(const std::string& title) = 0;

        virtual void LogText(SharableString text) = 0;

        template<typename... Args>
        void LogText(const char* formatter, Args const&... args);

        virtual void UpdateProgress(double percent) = 0;

        virtual void SetOutputText(const std::string& text) = 0;

        virtual void OnCreatedOutput(std::string output_title, std::string path) = 0;

        virtual void OnException(const CSProException& exception) = 0;

        virtual void OnCompletion(Status status) = 0;

        virtual const GlobalSettings& GetGlobalSettings() = 0;
    };

    virtual ~GenerateTask();

    virtual void ValidateInputs() { }

    bool IsInterfaceSet() const              { return ( m_interface != nullptr ); }
    Interface& GetInterface()                { return *m_interface; }
    void SetInterface(Interface& interface_) { m_interface = &interface_; }

    void Run();
    bool IsRunning() const { return ( m_status == Status::Running ); }

    void Cancel();
    bool IsCanceled() const { return ( m_status == Status::Canceled ); }

protected:
    // subclasses must override OnRun
    virtual void OnRun() = 0;

private:
    Interface* m_interface = nullptr;
    std::thread m_runThread;
    Status m_status = Status::NotStarted;
};



// --------------------------------------------------------------------------
// inline implementations
// --------------------------------------------------------------------------

template<typename... Args>
void GenerateTask::Interface::LogText(const char* const formatter, Args const&... args)
{
    LogText(FormatText(formatter, args...));
}


inline GenerateTask::~GenerateTask()
{
    if( m_runThread.joinable() )
        m_runThread.join();
}


inline void GenerateTask::Run()
{
    ASSERT(m_interface != nullptr && m_status == Status::NotStarted);

    m_runThread = std::thread(
        [&]()
        {
            try
            {
                m_status = Status::Running;
                OnRun();
            }

            catch( const CSProException& exception )
            {
                m_status = Status::EndedInException;
                GetInterface().OnException(exception);
            }

            if( m_status == Status::Running )
            {
                m_status = Status::Complete;
                GetInterface().UpdateProgress(100);
            }

            GetInterface().OnCompletion(m_status);
        });
}


inline void GenerateTask::Cancel()
{
    if( IsRunning() )
    {
        m_status = Status::Canceled;
        m_runThread.join();
    }
}
