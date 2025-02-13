//---------------------------------------------------------------------------
//
//  intEnter: interpreting Data-Entry Enter commands
//
//---------------------------------------------------------------------------
//  File name: IntEnter.cpp
//
//  Description:
//          Implementation for Entry' interpreter-driver class
//
//  History:    Date       Author   Comment
//              ---------------------------
//              25 Aug 03   RHF     Creation
//
//---------------------------------------------------------------------------

#include "StandardSystemIncludes.h"
#include "INTERPRE.H"
#include "EXENTRY.H"
#include "ProgramControl.h"
#include <Zissalib/CFlAdmin.h>

//--------------------------------------------------------
//  exenter : entering to an external flow
//--------------------------------------------------------
double CIntDriver::exenter(int iExpr)
{
    const auto& statement_node = GetNode<STN_NODE>(iExpr);
    int new_flow_symbol_index = statement_node.arguments[0];
    ASSERT(Issamod == ModuleType::Entry);

    // RHF INIC Nov 02, 2001
    if( m_bExecSpecFunc )
    {
        issaerror(MessageType::Error, 9100);
        return 0;
    }
    // RHF END Nov 02, 2001

    CEntryDriver* pEntryDriver = (CEntryDriver*)m_pEngineDriver;

    CFlAdmin* pFlAdmin = pEntryDriver->GetFlAdmin();
    FLOW* pNewFlow = LPT(new_flow_symbol_index);

    if( pFlAdmin->IsActiveFlow(new_flow_symbol_index) )
    {
        issaerror(MessageType::Warning, 90545, pNewFlow->GetName().c_str());
    }

    else
    {
        // install the new flow
        pEntryDriver->SetFlowInProcess(pNewFlow);
        pEntryDriver->SetEnterMode(pNewFlow);  //obsolete condition???

        bool enabled = pFlAdmin->EnableFlow(new_flow_symbol_index);
        ASSERT(enabled);

        throw EnterFlowProgramControlException();
    }

    return 0;
}
