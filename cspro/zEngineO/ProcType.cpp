#include "stdafx.h"
#include "ProcType.h"


const char* ToString(const ProcType proc_type)
{
    switch( proc_type )
    {
        case ProcType::PreProc:         return "PreProc";
        case ProcType::OnFocus:         return "OnFocus";
        case ProcType::KillFocus:       return "KillFocus";
        case ProcType::PostProc:        return "PostProc";
        case ProcType::OnOccChange:     return "OnOccChange";
        case ProcType::None:            return "None";
        case ProcType::Tally:           return "Tally";
        case ProcType::ExplicitCalc:    return "PostCalc";
        default:                        return "Unknown";
    }
}


bool IsProcTypeOrderCorrect(const ProcType first_proc_type, const ProcType second_proc_type)
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
