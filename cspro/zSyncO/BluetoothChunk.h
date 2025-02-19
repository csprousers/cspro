#pragma once

#include <zSyncO/IDataChunk.h>


// Bluetooth implementation for a data chunk

class BluetoothDataChunk : public IDataChunk
{
public:
    static constexpr int FileChunkSize = 5 * 1024 * 1024; // 5 MB

    BluetoothDataChunk();

    size_t GetCaseSize() const override;
    uint64_t GetBinaryContentSize() const override;

    void EnableOptimization() override;
    void ResetOptimization() override;

    void Optimize(uint64_t dataSize, size_t packetSize);

private:
    size_t m_caseSize;
    bool m_optimizationEnabled;

    enum class Resize { Shrink, Grow };
    std::optional<Resize> m_resize;
};
