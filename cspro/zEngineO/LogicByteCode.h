#pragma once

#include <zEngineO/zEngineO.h>

class Serializer;


class ZENGINEO_API LogicByteCode
{
public:
    LogicByteCode();

    size_t GetSize() const { return m_nextPosition; }

    int* AdvancePosition(int ints_needed);

    int GetPositionAtCode(const int* byte_code) const { return byte_code - m_bufferData; }

    const int* GetCodeAtPosition(int position) const { return m_bufferData + position; }
    int* GetCodeAtPosition(int position)             { return m_bufferData + position; }

    bool IncreaseBufferForOneProc();

    void serialize(Serializer& ar);

private:
    bool IncreaseBuffer(size_t increase_size);

private:
    std::vector<int> m_buffer;
    int* m_bufferData;
    size_t m_nextPosition;
};


#define Prognext static_cast<int>(m_engineData->logic_byte_code.GetSize()) // COMPILER_DLL_TODO this should not be needed once all nodes are created using LogicCompiler::Create... methods
