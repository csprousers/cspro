#pragma once

#include <zSyncO/zSyncO.h>
#include <zSyncO/IDataChunk.h>


// Default implementation for a data chunk

class SYNC_API DefaultDataChunk : public IDataChunk
{
public:
    DefaultDataChunk();

    int getSize() const override;
    void setSize(int size);

    virtual int getBinaryContentSize() const override { return m_binaryContentSize; }

    void enableOptimization() override;
    void optimize(uint64_t, std::size_t);
    void resetOptimization() override;

private:
    int m_size;
    int m_binaryContentSize;
    static constexpr int DEFAULT_BINARY_CONTENT_SIZE = 5 * 1024 * 1024; //10MB
};
