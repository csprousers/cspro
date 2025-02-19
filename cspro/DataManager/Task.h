#pragma once

class TaskRunner;


class Task
{
public:
    virtual ~Task() { }

    enum Result { Complete, Canceled, Exception };
    struct CanceledException : public std::exception { };

    // The TaskRunner and cancel flag will be set and non-null before any of the other methods are called.
    void SetTaskRunner(TaskRunner& task_runner, const bool& cancel_flag) { m_taskRunner = &task_runner;
                                                                           m_cancelFlag = &cancel_flag; }

    // Initialize will be called prior to calling Run.
    // Implementations can throw exceptions.
    virtual void Initialize() { }

    // A subclass must implement Run.
    // Implementations can throw exceptions. On cancelation, the implementation can throw CanceledException.
    virtual void Run() = 0;

    // Finalize will be called with a code indicating if the task was fully run, canceled, or ended in exception.
    // Implementations can throw exceptions.
    virtual void Finalize(Result result) { result; }

protected:
    bool IsCanceled() const { ASSERT(m_cancelFlag != nullptr); return *m_cancelFlag; }

protected:
    TaskRunner* m_taskRunner = nullptr;
    const bool* m_cancelFlag = nullptr;
};
