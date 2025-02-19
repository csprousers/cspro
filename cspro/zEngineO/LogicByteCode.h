#pragma once

#include <zEngineO/zEngineO.h>


class ZENGINEO_API LogicByteCode
{
public:
    LogicByteCode();

    size_t GetSize() const { return m_currentBufferStartOffset + m_nextPositionInCurrentBuffer; }

    int* AdvancePosition(int ints_needed);

    int GetPositionAtCode(const int* byte_code) const;

    const int* GetCodeAtPosition(int position) const;
    int* GetCodeAtPosition(int position);

    bool EnlargeBufferForOneProc();

    void serialize(Serializer& ar);

private:
    int GetPositionAtCodeForMultipleBuffers(const int* byte_code) const;
    const int* GetCodeAsPositionForMultipleBuffers(size_t position) const;

    bool EnlargeCurrentBuffer(size_t increase_size);

private:
    std::vector<std::vector<int>> m_buffers;
    bool m_usingMultipleBuffers;
    std::vector<int>* m_currentBuffer;
    size_t m_currentBufferStartOffset;
    int* m_currentBufferData;
    size_t m_nextPositionInCurrentBuffer;
};


#define Prognext static_cast<int>(m_engineData->logic_byte_code.GetSize()) // COMPILER_DLL_TODO this should not be needed once all nodes are created using LogicCompiler::Create... methods



// --------------------------------------------------------------------------
// inline implementations
// --------------------------------------------------------------------------

inline int LogicByteCode::GetPositionAtCode(const int* const byte_code) const
{
    if( m_usingMultipleBuffers )
        return GetPositionAtCodeForMultipleBuffers(byte_code);

    ASSERT81(m_currentBufferStartOffset == 0 && m_buffers.size() == 1 && m_currentBufferData == m_currentBuffer->data());
    ASSERT(byte_code >= m_currentBufferData && byte_code <= &m_currentBuffer->back());

    return static_cast<int>(byte_code - m_currentBufferData);
}


inline const int* LogicByteCode::GetCodeAtPosition(const int position) const
{
    if( m_usingMultipleBuffers )
        return GetCodeAsPositionForMultipleBuffers(position);

    ASSERT81(m_buffers.size() == 1 && m_currentBufferData == m_currentBuffer->data());
    ASSERT(static_cast<size_t>(position) < m_currentBuffer->size());

    return m_currentBufferData + position;
}


inline int* LogicByteCode::GetCodeAtPosition(const int position)
{
    return const_cast<int*>(const_cast<const LogicByteCode*>(this)->GetCodeAtPosition(position));
}
