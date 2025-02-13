#pragma once


class RunOperation
{
public:
    virtual ~RunOperation() { }

    virtual bool IsCancelable() const = 0;

    virtual bool IsRunning() const = 0;

    // when complete, post a RunOperationComplete message to the main frame;
    // the main frame will then call OnComplete
    virtual void Run() = 0; 

    virtual void OnComplete() = 0;

    virtual void Cancel() = 0;
};
