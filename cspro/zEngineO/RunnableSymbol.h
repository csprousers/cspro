#pragma once

#include <zEngineO/zEngineO.h>
#include <zEngineO/ProcType.h>


class ZENGINEO_API RunnableSymbol
{
public:
    bool HasProcIndex(ProcType proc_type) const
    {
        return ( GetProcIndex(proc_type) != -1 );
    }

    int GetProcIndex(ProcType proc_type) const
    {
        // the first proc type has value 0
        size_t index = static_cast<size_t>(proc_type);
        return ( index < m_procIndices.size() ) ? m_procIndices[index] : -1;
    }

    void SetProcIndex(ProcType proc_type, int proc_index);

protected:
    void serialize(Serializer& ar);

private:
    std::vector<int> m_procIndices;
};
