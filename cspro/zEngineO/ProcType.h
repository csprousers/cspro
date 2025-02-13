#pragma once

#include <zEngineO/zEngineO.h>


enum class ProcType
{
    PreProc = 0,
    OnFocus = 1,
    KillFocus = 4,
    PostProc = 5,
    OnOccChange = 6,
    None = 7,
    Tally = 8,
    ExplicitCalc = 9,
    ImplicitCalc = 10
};

ZENGINEO_API const TCHAR* ToString(ProcType proc_type);

template<typename T>
const TCHAR* GetProcTypeName(T proc_type) { return ToString(static_cast<ProcType>(proc_type)); }

ZENGINEO_API bool IsProcTypeOrderCorrect(ProcType first_proc_type, ProcType second_proc_type);
