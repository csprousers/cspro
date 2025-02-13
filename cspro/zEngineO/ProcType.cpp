#include "stdafx.h"
#include "ProcType.h"


const TCHAR* ToString(ProcType proc_type)
{
    switch( proc_type )
    {
        case ProcType::PreProc:         return _T("PreProc");
        case ProcType::OnFocus:         return _T("OnFocus");
        case ProcType::KillFocus:       return _T("KillFocus");
        case ProcType::PostProc:        return _T("PostProc");
        case ProcType::OnOccChange:     return _T("OnOccChange");
        case ProcType::None:            return _T("None");
        case ProcType::Tally:           return _T("Tally");
        case ProcType::ExplicitCalc:    return _T("PostCalc");
        default:                        return _T("Unknown");
    }
}


bool IsProcTypeOrderCorrect(ProcType first_proc_type, ProcType second_proc_type)
{
    switch( second_proc_type )
    {
        case ProcType::ExplicitCalc:
            if( first_proc_type == ProcType::Tally )
                return true;
            [[fallthrough]];

        case ProcType::PostProc:
            if( first_proc_type == ProcType::KillFocus )
                return true;
            [[fallthrough]];

        case ProcType::KillFocus:
            if( first_proc_type == ProcType::OnOccChange )
                return true;
            [[fallthrough]];

        case ProcType::OnOccChange:
            if( first_proc_type == ProcType::OnFocus )
                return true;
            [[fallthrough]];

        case ProcType::OnFocus:
            if( first_proc_type == ProcType::PreProc )
                return true;
    }

    return false;
}
