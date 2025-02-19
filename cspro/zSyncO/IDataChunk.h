#pragma once


// Interface to a data chunk

class IDataChunk
{
public:
    virtual ~IDataChunk() { }

    virtual size_t GetCaseSize() const = 0;
    virtual uint64_t GetBinaryContentSize() const = 0;

    virtual void EnableOptimization() = 0;
    virtual void ResetOptimization() = 0;
};
