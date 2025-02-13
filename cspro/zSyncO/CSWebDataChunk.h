#pragma once
#include "IDataChunk.h"

class CSWebDataChunk : public IDataChunk
{
public:

    CSWebDataChunk()
    {
        m_lastGoodSize = m_size = DEFAULT_SIZE;
        m_binaryContentSize = CSWEB_DEFAULT_BINARY_CONTENT_SIZE;
    }

    int getSize() const override
    {
        return m_size;
    }
    virtual int getBinaryContentSize() const override
    {
        return m_binaryContentSize;
    }
    void setSize(int size)
    {
        m_size = size;
    }

    void enableOptimization() override
    {
    }

    void resetOptimization() override
    {
    }

    void optimize(float requestTimeSecs, int lastChunkSizeCases, size_t lastChunkSizeBytes);

    void onError();

private:
    const int DEFAULT_SIZE = 100;
    const int MAX_SIZE = 100000;
    const int CSWEB_DEFAULT_BINARY_CONTENT_SIZE = 10 * 1024* 1024; //10MB
    int m_size;
    int m_binaryContentSize;
    int m_lastGoodSize;
};

