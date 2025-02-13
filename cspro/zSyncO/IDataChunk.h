#pragma once
#include <cstddef>
#include <cstdint>

/// <summary>Interface to a data chunk</summary>
struct IDataChunk
{
    virtual ~IDataChunk() {};

    virtual int getSize() const = 0;
    virtual int getBinaryContentSize() const = 0;

    virtual void enableOptimization() = 0;
    virtual void resetOptimization() = 0;
};
