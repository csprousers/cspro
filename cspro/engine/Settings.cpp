//---------------------------------------------------------------------------
//  File name: Settings.h
//
//  Description:
//           header for IMSA switches (should serve for entry & batch)
//
//  History:    Date       Author   Comment
//              ---------------------------
//              01 Jan 00   RHF     Create this file, expanding constructor and initialization
//              08 Jun 00   vc      Adding invokes to 'SendSetMessage' in 'Init'
//              04 Apr 01   vc      Tailoring for RepMgr compatibility
//
//---------------------------------------------------------------------------

#include "StandardSystemIncludes.h"
#include "Settings.h"
#include <zPlatformO/PlatformInterface.h>

bool CSettings::m_bNewTbd = true;

CSettings::CSettings()
{
    // manually installing 'SetPathOff()'
    // --- when changing that method, modify correspondingly below
    m_bPathOn             = false;

    // related settings
    m_bCanEnterNotappl    = true; 
    m_bAskWhenNotappl     = true;
    m_bCanEnterOutOfRange = true;
    m_bAskWhenOutOfRange  = true;
    m_bDisplayOn          = true;
    m_bErrmsgOn           = true;

    m_bAutoEndGroup       = false; // RHF Nov 07, 2000
    m_bCanEndLevel        = true;

    // next settings don't issue any message to the interface
    m_bHasCheckRanges     = false;                 // victor Dec 07, 00
    m_bAutoCheckRanges    = false;                 // victor Dec 07, 00
    m_bHasSkipStruc       = false;                 // victor Mar 14, 01
    m_bAutoSkipStruc      = false;                 // victor Mar 14, 01
    m_bAutoSkipStrucImpute= false;                 // RHF Nov 09, 2001

    m_bExitWhenFinish = false;

    m_treatSpecialValuesAsZero = false; // GHM 20090827

    // --- Export options                           // victor Dec 18, 00
    SetExportData(true);                            // victor Dec 18, 00
    SetExportSPSS(true);                            // victor Dec 18, 00
    SetExportSAS(false);                            // victor Dec 18, 00
    SetExportSTATA(false);
    SetExportCSPRO(false);

    SetExportTabDelim(false);
    SetExportCommaDelim(false);
    SetExportSemiColonDelim(false);

    SetExportItemOnly(false);
    SetExportSubItemOnly(true);
    SetExportItemSubItem(false);

    SetExportForceANSI(true); // GHM 20120416
    SetExportCommaDecimal(false);

    SetInteractiveEdit(false); // GHM 20130711 moved from CEntryDriver

    // ---------------------------- other members ----------------------------
    // --- Miscellaneous
    m_Workdict = NULL;              // internal workDict

    memset( m_QidVars, 0, MaxNumberLevels * MAXQIDVARS * sizeof(int) );
    m_QidLength = 0;

    // I/O current parms:
    m_io_Err       = 0;                 // type of problem

    m_bHasExport   = false;  // presence of export sentence
    m_bHasCrosstab = false;  // RHF Jul 02, 2005
    m_bHasFrequency = false; // RHF Jul 02, 2005

    // RHF INIC Oct 22, 2002
    m_bNewTbd = true;

#ifdef WIN_DESKTOP
    // GSF May 18, 2006 and changed string from OLD_TBD to OLD_TABS
    csprochar*   p;
    if( (p=_tgetenv( _T("OLD_TABS") )) != NULL && _tcsicmp( p, _T("Y") ) == 0 )
        m_bNewTbd=false;
#endif
}

void CSettings::SetPathOn()
{
    m_bPathOn               = true;
    m_bAutoEndGroup         = false; // RHF Nov 07, 2000
    // related settings
    SetCannotEnterNotappl();
    SetMustAskWhenNotappl();
    SetCannotEnterOutOfRange();
    SetMustAskWhenOutOfRange();
    SetDisplayMessageOn();
    SetErrmsgMessageOn();
    SetCannotUseEndLevel();
}

void CSettings::SetPathOff()
{
    m_bPathOn               = false;
    m_bAutoEndGroup         = true; // RHF Nov 07, 2000
    // related settings
    SetCanEnterNotappl();
    SetMustAskWhenNotappl();
    SetCanEnterOutOfRange();
    SetMustAskWhenOutOfRange();
    SetDisplayMessageOn();
    SetErrmsgMessageOn();
    SetCanUseEndLevel();
}
