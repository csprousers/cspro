#include "stdafx.h"
#include "BluetoothChunk.h"
#include "assert.h"

BluetoothDataChunk::BluetoothDataChunk()
: m_defaultSize(100)
, m_size(m_defaultSize)
, m_binaryContentSize(BluetoothFileChunk::size)
, m_resize(Resize::Default)
, m_enabled(false)
, m_init(false)
{}

int BluetoothDataChunk::getSize() const
{
    return m_size;
}

void BluetoothDataChunk::setSize(int size)
{
    m_size = size;
}

void BluetoothDataChunk::enableOptimization()
{
    m_enabled = true;
}

void BluetoothDataChunk::optimize(std::uint64_t dataSize, std::size_t packetSize)
{
    // The larger the chunk size the more efficient the packets are filled. However, don't make
    // the chunk size so large that the user has to send all the data to realize there was an
    // error receiving the data.

    // The packet size is a rough estimate of the data that fits in a packet
    if (m_enabled) {
        init(dataSize, packetSize);

        switch (m_resize) {
        case Resize::Grow:
            // The data size will be 2 to 4 times larger than the packet size
            while (dataSize <= packetSize) {
                dataSize *= 2;
                m_size *= 2;
            }
            m_enabled = false;
            m_size *= 2;
            break;
        case Resize::Shrink:
        {
            // The data size will be 2 to 4 times larger than the packet size
            double size = m_size;
            while (dataSize > packetSize) {
                dataSize /= 2;
                size /= 2;
            }
            m_enabled = false;
            // Avoid quotient of 0
            m_size = size >= 1 ? static_cast<int>(size) : 1;
            m_size *= 4;
            break;
        }
        default:
            assert(false);
        }
    }
}

void BluetoothDataChunk::resetOptimization()
{
    m_size = m_defaultSize;
    m_resize = Resize::Default;
    m_enabled = false;
    m_init = false;
}

void BluetoothDataChunk::init(std::uint64_t dataSize, std::size_t packetSize)
{
    if (!m_init) {
        if (dataSize <= packetSize) {
            m_resize = Resize::Grow;
        }
        else {
            m_resize = Resize::Shrink;
        }

        m_init = true;
    }
}
