#include "stdafx.h"
#include "LogicByteCode.h"


// Prior to CSPro 8.1, only one compilation buffer was used. Occasionally, when people coded a huge number of
// of statements in a single PROC, this led to crashes because the buffer was only increased once per PROC.
// The code now accounts for the possibility that there may be huge PROCs.


namespace
{
    constexpr size_t MinimumBufferSizePerProc = 50000;
    constexpr size_t BufferIncreaseSize       = 200000;
    constexpr size_t BufferMargin             = 1000;
}


LogicByteCode::LogicByteCode()
    :   m_buffers{ std::vector<int>() },
        m_usingMultipleBuffers(false),
        m_currentBuffer(&m_buffers.front()),
        m_currentBufferStartOffset(0),
        m_currentBufferData(m_currentBuffer->data()),
        m_nextPositionInCurrentBuffer(0)
{
}


int* LogicByteCode::AdvancePosition(const int ints_needed)
{
    int* const byte_code_in_current_buffer = m_currentBufferData + m_nextPositionInCurrentBuffer;

    // because some compilation routines reference compilation nodes, a margin is added in hopes
    // of preventing these routines from referencing nodes that are part of a different buffer
    const size_t next_position_in_current_buffer_after_advance_and_margin = m_nextPositionInCurrentBuffer + ints_needed + BufferMargin;

    if( next_position_in_current_buffer_after_advance_and_margin <= m_currentBuffer->size() )
    {
        m_nextPositionInCurrentBuffer += ints_needed;
        return byte_code_in_current_buffer;
    }

    // if byte_code_in_current_buffer is null, it means that EnlargeBufferForOneProc was never called,
    // so enlarge the current single buffer and use it
    if( byte_code_in_current_buffer == nullptr )
    {
        ASSERT(!m_usingMultipleBuffers);
        EnlargeCurrentBuffer(ints_needed + BufferIncreaseSize);
        return AdvancePosition(ints_needed);
    }

    // if here, we can resize the current buffer to only what was used...
    m_currentBuffer->resize(m_nextPositionInCurrentBuffer);

    // ...and then add a new compilation buffer
    m_usingMultipleBuffers = true;
    m_currentBufferStartOffset += m_nextPositionInCurrentBuffer;
    m_nextPositionInCurrentBuffer = ints_needed;

    m_currentBuffer = &m_buffers.emplace_back();
    EnlargeCurrentBuffer(ints_needed + BufferIncreaseSize);

    return m_currentBufferData;
}


int LogicByteCode::GetPositionAtCodeForMultipleBuffers(const int* const byte_code) const
{
    ASSERT(m_usingMultipleBuffers && m_currentBuffer == &m_buffers.back());

    // return quickly if the position is in the current buffer
    if( byte_code >= m_currentBufferData && byte_code <= &m_currentBuffer->back() )
        return m_currentBufferStartOffset + static_cast<int>(byte_code - m_currentBufferData);

    // otherwise determine which buffer the byte code is in
    int offset = 0;

    for( const std::vector<int>& buffer : m_buffers )
    {
        if( byte_code >= buffer.data() && byte_code <= &buffer.back() )
            return offset + static_cast<int>(byte_code - buffer.data());

        offset += buffer.size();
    }

    return ReturnProgrammingError(0);
}


const int* LogicByteCode::GetCodeAsPositionForMultipleBuffers(size_t position) const
{
    ASSERT(m_usingMultipleBuffers && m_currentBuffer == &m_buffers.back());

    // return quickly if the position is in the current buffer
    if( position >= m_currentBufferStartOffset )
        return m_currentBufferData + ( position - m_currentBufferStartOffset );

    // otherwise determine which buffer to use
    for( const std::vector<int>& buffer : m_buffers )
    {
        if( position < buffer.size() )
            return buffer.data() + position;

        position -= buffer.size();
    }

    return ReturnProgrammingError(nullptr);
}


bool LogicByteCode::EnlargeBufferForOneProc()
{
    // only enlarge the buffer when necessary
    if( ( m_nextPositionInCurrentBuffer + MinimumBufferSizePerProc ) > m_currentBuffer->size() )
    {
        static_assert(MinimumBufferSizePerProc <= BufferIncreaseSize);
        return EnlargeCurrentBuffer(BufferIncreaseSize);
    }

    return true;
}


bool LogicByteCode::EnlargeCurrentBuffer(const size_t increase_size)
{
    try
    {
        m_currentBuffer->resize(m_currentBuffer->size() + increase_size);
        m_currentBufferData = m_currentBuffer->data();
        return true;
    }

    catch( const std::exception& )
    {
        // memory allocation error
        return ReturnProgrammingError(false);
    }
}


void LogicByteCode::serialize(Serializer& ar)
{
    if( ar.IsSaving() )
    {
        ar.Write(GetSize());

        for( size_t i = 0; i < m_buffers.size(); ++i )
        {
            const std::vector<int>& this_buffer = m_buffers[i];
            const size_t ints_to_write = ( ( i + 1 ) < m_buffers.size() ) ? this_buffer.size() :
                                                                            m_nextPositionInCurrentBuffer;

            ar.Write(this_buffer.data(), ints_to_write * sizeof(this_buffer[0]));
        }
    }

    else
    {
        // even if the buffer was compiled into multiple buffers, read it in as one
        ASSERT(m_buffers.size() == 1 && !m_usingMultipleBuffers && GetSize() == 0);

        m_nextPositionInCurrentBuffer = ar.Read<size_t>();

        // add extra space to the compilation buffer in case any logic is compiled dynamically
        EnlargeCurrentBuffer(m_nextPositionInCurrentBuffer + MinimumBufferSizePerProc);

        ar.Read(m_currentBufferData, m_nextPositionInCurrentBuffer * sizeof(*m_currentBufferData));

        ASSERT(m_nextPositionInCurrentBuffer == GetSize());
    }
}
