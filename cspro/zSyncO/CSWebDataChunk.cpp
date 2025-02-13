#include "stdafx.h"
#include "CSWebDataChunk.h"
#include <easyloggingwrapper.h>

void CSWebDataChunk::optimize(float requestTimeSecs, int lastChunkSizeCases, size_t lastChunkSizeBytes)
{
    auto mbps = (lastChunkSizeBytes * 0.000008) / requestTimeSecs;
    CLOG(INFO, "sync") << "Chunk time Seconds : " << requestTimeSecs << " Cases: " << lastChunkSizeCases << " Kb: " << lastChunkSizeBytes / 1000.0 << " mbps " << mbps;

    if (lastChunkSizeCases == m_size) { // Only update chunk size on a full chunk
        m_lastGoodSize = m_size;

        if (lastChunkSizeBytes <= 10 * 1e+6 && requestTimeSecs <= 30 && m_size <= MAX_SIZE) {
            m_size *= 2;
            CLOG(INFO, "sync") << "Update chunk size to " << m_size << " cases";
        }
    }
}

void CSWebDataChunk::onError()
{
    // Go back to the last good chunk size
    m_size = m_lastGoodSize;
    CLOG(INFO, "sync") << "Reset chunk size to " << m_size << " cases";
}
