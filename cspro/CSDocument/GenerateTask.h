#pragma once

#include <thread>

class GlobalSettings;


class GenerateTask
{
public:
    enum class Status { NotStarted, Running, Complete, Canceled, EndedInException };

    struct Interface
    {
        virtual ~Interface() { }
        virtual void SetTitle(const std::wstring& title) = 0;
        virtual void LogText(std::wstring text) = 0;
        virtual void UpdateProgress(double percent) = 0;
        virtual void SetOutputText(const std::wstring& text) = 0;
        virtual void OnCreatedOutput(std::wstring output_title, std::wstring path) = 0;
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

    static std::wstring GetElapsedTimeText(int64_t start_timestamp, int64_t end_timestamp = -1);

protected:
    // subclasses must override OnRun
    virtual void OnRun() = 0;

private:
    Interface* m_interface = nullptr;
    std::thread m_runThread;
    Status m_status = Status::NotStarted;
};


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


inline std::wstring GenerateTask::GetElapsedTimeText(int64_t start_timestamp, int64_t end_timestamp/* = -1*/)
{
    if( end_timestamp == -1 )
        end_timestamp = GetTimestamp<int64_t>();

    const int elapsed_seconds = static_cast<int>(end_timestamp - start_timestamp);
    ASSERT(elapsed_seconds >= 0);

    return ( elapsed_seconds < 60 ) ? FormatTextCS2WS(_T("%d second%s"), elapsed_seconds, PluralizeWord(elapsed_seconds)) :
                                      FormatTextCS2WS(_T("%d:%02d minutes"), elapsed_seconds / 60, elapsed_seconds % 60);
}
