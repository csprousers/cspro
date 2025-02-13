#pragma once

#include <zEngineO/zEngineO.h>
#include <zEngineO/ProcType.h>

class Serializer;


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


// RUNNABLE_SYMBOL_TODO remove
#define PROCTYPE_PRE            static_cast<int>(ProcType::PreProc)
#define PROCTYPE_ONFOCUS        static_cast<int>(ProcType::OnFocus)
#define PROCTYPE_KILLFOCUS      static_cast<int>(ProcType::KillFocus)
#define PROCTYPE_POST           static_cast<int>(ProcType::PostProc)
#define PROCTYPE_ONOCCCHANGE    static_cast<int>(ProcType::OnOccChange)
#define PROCTYPE_NONE           static_cast<int>(ProcType::None)
#define PROCTYPE_TALLY          static_cast<int>(ProcType::Tally)
#define PROCTYPE_ECALC          static_cast<int>(ProcType::ExplicitCalc)
