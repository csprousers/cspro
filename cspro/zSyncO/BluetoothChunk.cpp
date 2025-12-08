#include "stdafx.h"
#include "BluetoothChunk.h"


namespace
{
    constexpr size_t DefaultSize = 100;
}


BluetoothDataChunk::BluetoothDataChunk()
    :   m_caseSize(DefaultSize),
        m_optimizationEnabled(false)
{
}


size_t BluetoothDataChunk::GetCaseSize() const
{
    return m_caseSize;
}


uint64_t BluetoothDataChunk::GetBinaryContentSize() const
{
    return FileChunkSize;
}


void BluetoothDataChunk::EnableOptimization()
{
    m_optimizationEnabled = true;
}


void BluetoothDataChunk::Optimize(uint64_t dataSize, const size_t packetSize)
{
    if( !m_optimizationEnabled )
        return;

    // The larger the chunk size the more efficient the packets are filled. However, don't make
    // the chunk size so large that the user has to send all the data to realize there was an
    // error receiving the data.

    // The packet size is a rough estimate of the data that fits in a packet
    if( !m_resize.has_value() )
    {
        m_resize = ( dataSize <= packetSize ) ? Resize::Grow :
                                                Resize::Shrink;
    }

    switch( *m_resize )
    {
        case Resize::Grow:
        {
            // The data size will be 2 to 4 times larger than the packet size
            while( dataSize <= packetSize )
            {
                dataSize *= 2;
                m_caseSize *= 2;
            }

            m_optimizationEnabled = false;
            m_caseSize *= 2;
            break;
        }

        case Resize::Shrink:
        {
            // The data size will be 2 to 4 times larger than the packet size
            double new_size = static_cast<double>(m_caseSize);

            while( dataSize > packetSize )
            {
                dataSize /= 2;
                new_size /= 2;
            }

            m_optimizationEnabled = false;
            // Avoid quotient of 0
            m_caseSize = 4 * ( ( new_size >= 1 ) ? static_cast<size_t>(new_size) : 1 );
            break;
        }

        default:
        {
            ASSERT(false);
        }
    }
}


void BluetoothDataChunk::ResetOptimization()
{
    m_caseSize = DefaultSize;
    m_optimizationEnabled = false;
    m_resize.reset();
}
