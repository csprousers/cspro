//---------------------------------------------------------------------------
//  File name: Form.cpp
//
//  Description:
//          Performs changes to attributes of fields-in-forms (execution environment only)
//
//  History:    Date       Author   Comment
//              ---------------------------
//              10 Nov 99   RHF     Basic conversion - ISSA implementation erased
//              04 Apr 01   vc      Tailoring for RepMgr compatibility
//              22 Nov 02   RHF      Implementation of field attributes (PROTECT/AUTOSKIP/ENTER/VISIBLE&HIDDEN)
//
//---------------------------------------------------------------------------
#include "StandardSystemIncludes.h"
#include "INTERPRE.H"
#include <zFormO/FormFile.h>
#include <CSEntry/UWM.h>


// PROTECT/AUTOSKIP/ENTER
int CIntDriver::frm_varpause( int iSymVar, FieldBehavior eBehavior ) {
    int         iNumChanges = 0;
    VART*       pVarT=VPT(iSymVar);

    ASSERT( pVarT->IsInAForm() );

    pVarT->SetBehavior(eBehavior);

    // update the field object as well
    CDEField* pField = GetCDEFieldFromVART(pVarT);

    if( pField != nullptr )
        pField->IsProtected(eBehavior == AsProtected);

    // Inform to interface
#ifdef WIN_DESKTOP
    CWnd* pMainWnd = AfxGetApp()->GetMainWnd();
    if( pMainWnd && IsWindow(pMainWnd->GetSafeHwnd()) ) {
        DEFLD   cField;

        cField.SetSymbol( iSymVar );

        pMainWnd->SendMessage(WM_IMSA_FIELD_BEHAVIOR,  (UINT) &cField, (long) eBehavior );
        pMainWnd->SendMessage(WM_IMSA_CSENTRY_REFRESH_DATA);
    }
#endif
    return iNumChanges;
}

// VISIBLE/HIDDEN
int CIntDriver::frm_varvisible( int iSymVar, bool bOnOff ) {
    int         iNumChanges = 0;
    VART*       pVarT=VPT(iSymVar);


    ASSERT( pVarT->IsInAForm() );


    pVarT->SetVisible( bOnOff );

    // Inform to interface
#ifdef WIN_DESKTOP
    CWnd* pMainWnd = AfxGetApp()->GetMainWnd();
    if( pMainWnd && IsWindow(pMainWnd->GetSafeHwnd()) ) {
        DEFLD   cField;

        cField.SetSymbol( iSymVar );

        pMainWnd->SendMessage(WM_IMSA_FIELD_VISIBILITY, (UINT) &cField, (long) bOnOff ? 1 : 0 );
        pMainWnd->SendMessage(WM_IMSA_CSENTRY_REFRESH_DATA);
    }
#endif
    return iNumChanges;
}


void CIntDriver::frm_capimode(int /*iSymVar*/, int iCapiMode)
{
    WindowsDesktopMessage::Post(UWM::CSEntry::ShowCapi, iCapiMode);
}
