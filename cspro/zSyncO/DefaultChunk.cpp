#include "stdafx.h"
#include "DefaultChunk.h"

DefaultDataChunk::DefaultDataChunk()
{
    m_size = 100;
    m_binaryContentSize = DEFAULT_BINARY_CONTENT_SIZE;
}

int DefaultDataChunk::getSize() const
{
    return m_size;
}

void DefaultDataChunk::setSize(int size)
{
    m_size = size;
}

void DefaultDataChunk::enableOptimization()
{
    // Do nothing
}

void DefaultDataChunk::optimize(std::uint64_t, std::size_t)
{
    // Do nothing
}

void DefaultDataChunk::resetOptimization()
{
    // Do nothing
}
