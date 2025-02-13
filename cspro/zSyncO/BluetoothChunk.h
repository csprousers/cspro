#pragma once
#include "IDataChunk.h"
#include <cstddef>
#include <cstdint>

namespace BluetoothFileChunk {
    const int size = 5 * 1024 * 1024; // 5 MB
}

/// <summary>Bluetooth implementation for a data chunk</summary>
class BluetoothDataChunk : public IDataChunk
{
public:
    BluetoothDataChunk();

    virtual int getSize() const override;
    virtual int getBinaryContentSize() const override { return m_binaryContentSize; }
    void setSize(int size);

    virtual void enableOptimization() override;
    virtual void resetOptimization() override;

    void optimize(std::uint64_t dataSize, std::size_t packetSize);

private:
    enum class Resize
    {
        Default,
        Shrink,
        Grow
    };

    void init(std::uint64_t dataSize, std::size_t packetSize);

    const int m_defaultSize;
    const int m_binaryContentSize;
    int m_size;
    Resize m_resize;
    bool m_enabled;
    bool m_init;
};
