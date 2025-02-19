#include "stdafx.h"
#include "NetworkDataChunk.h"


namespace
{
    constexpr size_t DefaultCaseSize            = 100;
    constexpr size_t MaxCaseSize                = 100000;
    constexpr uint64_t DefaultBinaryContentSize = 10 * 1024 * 1024; // 10 MB
}


NetworkDataChunk::NetworkDataChunk(const size_t case_size)
    :   m_caseSize(case_size),
        m_lastGoodCaseSize(DefaultCaseSize)
{
}


NetworkDataChunk::NetworkDataChunk()
    :   NetworkDataChunk(DefaultCaseSize)
{
}


size_t NetworkDataChunk::GetCaseSize() const
{
    return m_caseSize;
}


uint64_t NetworkDataChunk::GetBinaryContentSize() const
{
    return DefaultBinaryContentSize;
}


void NetworkDataChunk::ResetForNextChunk()
{
    m_chunkStartTime = std::chrono::high_resolution_clock::now();
}


void NetworkDataChunk::Optimize(const size_t cases_in_last_chunk, const size_t case_json_bytes_in_last_chunk)
{
    double request_time_seconds;

    if( m_chunkStartTime.has_value() )
    {
        const std::chrono::duration<double> request_duration = std::chrono::high_resolution_clock::now() - *m_chunkStartTime;
        request_time_seconds = request_duration.count();
        m_chunkStartTime.reset();
    }

    else
    {
        request_time_seconds = 0;
    }

    if( request_time_seconds == 0 )
    {
        ASSERT(false);
        return;
    }

    const double mbps = ( case_json_bytes_in_last_chunk * 0.000008 ) / request_time_seconds;

    SYNCLOG_INFO << "Chunk time Seconds : " << request_time_seconds
                 << " Cases: " << cases_in_last_chunk
                 << " Kb: " << case_json_bytes_in_last_chunk / 1000.0
                 << " mbps " << mbps;

    // Only update chunk size on a full chunk
    if( cases_in_last_chunk == m_caseSize )
    {
        m_lastGoodCaseSize = m_caseSize;

        if( case_json_bytes_in_last_chunk <= 10 * 1e+6 && request_time_seconds <= 30 && m_caseSize <= MaxCaseSize )
        {
            m_caseSize *= 2;
            SYNCLOG_INFO << "Update chunk size to " << m_caseSize << " cases";
        }
    }
}


void NetworkDataChunk::OnError()
{
    // Go back to the last good chunk size
    m_caseSize = m_lastGoodCaseSize;
    SYNCLOG_INFO << "Reset chunk size to " << m_caseSize << " cases";
}
