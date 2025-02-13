#include "stdafx.h"
#include "RunnableSymbol.h"


void RunnableSymbol::SetProcIndex(ProcType proc_type, int proc_index)
{
    size_t index = static_cast<size_t>(proc_type);

    if( index >= m_procIndices.size() )
        m_procIndices.resize(index + 1, -1);

    m_procIndices[index] = proc_index;
}


void RunnableSymbol::serialize(Serializer& ar)
{
    ar & m_procIndices;
}
