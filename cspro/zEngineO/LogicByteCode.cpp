#include "stdafx.h"
#include "LogicByteCode.h"


namespace
{
    constexpr size_t MinimumBufferSizePerProc = 50000;
    constexpr size_t BufferIncreaseSize       = 200000;
}


LogicByteCode::LogicByteCode()
    :   m_bufferData(m_buffer.data()),
        m_nextPosition(0)
{
}


int* LogicByteCode::AdvancePosition(int ints_needed)
{
    size_t next_position_after_advance = m_nextPosition + ints_needed;

    if( next_position_after_advance > m_buffer.size() )
    {
        // increase the buffer to the required size (plus more to avoid lots of small reallocations)
        if( !IncreaseBuffer(ints_needed + BufferIncreaseSize) )
            return nullptr;
    }

    int* byte_code = m_bufferData + m_nextPosition;

    m_nextPosition = next_position_after_advance;

    return byte_code;
}


bool LogicByteCode::IncreaseBufferForOneProc()
{
    if( ( m_nextPosition + MinimumBufferSizePerProc ) > m_buffer.size() )
    {
        static_assert(MinimumBufferSizePerProc <= BufferIncreaseSize);
        return IncreaseBuffer(BufferIncreaseSize);
    }

    return true;
}


bool LogicByteCode::IncreaseBuffer(size_t increase_size)
{
    try
    {
        m_buffer.resize(m_buffer.size() + increase_size);
        m_bufferData = m_buffer.data();
        return true;
    }

    catch( const std::exception& )
    {
        return false;
    }
}


void LogicByteCode::serialize(Serializer& ar)
{
    ar & m_nextPosition;

    if( ar.IsSaving() )
    {
        ar.Write(m_buffer.data(), m_nextPosition * sizeof(m_buffer[0]));
    }

    else
    {
        IncreaseBuffer(m_nextPosition);
        ar.Read(m_buffer.data(), m_nextPosition * sizeof(m_buffer[0]));
    }
}
