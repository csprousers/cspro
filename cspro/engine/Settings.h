#pragma once

//---------------------------------------------------------------------------
//  File name: Settings.h
//
//  Description:
//           header for IMSA switches (should serve for entry & batch)
//
//  History:    Date       Author   Comment
//              ---------------------------
//              02 Aug 99   vc      Created for IMSA beta version of Aug/99
//              01 Jan 00   RHF     Create Settings.cpp, expanding constructor and initialization
//              16 Feb 00   vc      Expanding data for Feb 2000 Release:
//                                    - MouseEnabled
//                                    - CanEndGroup
//                                    - CanEndLevel
//                                  plus new clauses added to "set behavior"
//              08 Jun 00   vc      Expanding data for Release 2.1:
//                                    - OperatorSkip
//                                    - Decimal mask
//
//              07 Dec 00   vc      Adding automatic range-checking in Batch thru command
//                                    - set behavior() CheckRanges on|off
//
//              18 Dec 00   vc      Adding preferences for exported descriptions
//                                    - set behavior() Export( DATA|SPSS|SAS|STATA ) on | off
//              14 Mar 01   vc      Adding automatic checking of skip-structure in Batch thru command
//                                    - set behavior() SkipStruc on|off
//              04 Apr 01   vc      Tailoring for RepMgr compatibility (no more AnyWrite)
//
//---------------------------------------------------------------------------
//
//  class Settings : public CObject
//
//  Description:
//      CsPro operation switches
//
//  Construction/Destruction
//      CSettings                   Constructor
//      ~CSettings                  Destructor
//
//  Extraction
//      IsPathOn                    Tells if path control is active (heads on)
//      IsPathOff                   Tells if path control is inactive (heads off)
//
//      CanEnterNotappl             Tells if NotAppl entries are allowed
//      AskWhenNotappl              Tells if an operator confirmation is required when entering NotAppl
//
//      CanEnterOutOfRange          Tells if out-of-range values are allowed
//      AskWhenOutOfRange           Tells if an operator confirmation is required when entering out-of-range values
//
//      IsDisplayMessageOn          Tells if "display" messages are to be generated
//      IsErrmsgMessageOn           Tells if "errmsg" messages are to be generated
//
//      IsMouseEnabled              Tells if operator' mouse is enabled or disabled
//
//      CanUseEndGroup              Tells if operator can use the EndGroup action
//      CanUseEndLevel              Tells if operator can use the EndLevel action
//
//      IsCheckingRanges            True if 'automatic range-checking' is on
//
//  Assignment: left-side marks point to the following restrictions:
//              (a) can only be used in Level-0 PreProc; when interpreting,
//                  a warning 91111 is issued if this condition is not met
//              (b) can only be used in Path OFF environments; when interpreting,
//                  a warning 91112 is issued if this condition is not met
//              If no left-side mark, there is no interpreting restriction
//
//  (a) SetPathOn                   Enable path control, disable entry of NotAppl and out-of-range
//  (a) SetPathOff                  Disable path control, allow for NotAppl and out-of-range with operator confirmation
//
//      SetEnterNotappl             Set the capability to accept Notappl to on or off
//      SetCanEnterNotappl          Enable the capability to accept Notappl, with operator confirmation
//      SetCannotEnterNotappl       Disable the capability to accept Notappl
//      SetMustAskWhenNotappl       If NotAppl allowed, force to ask for operator confirmation
//      SetDontAskWhenNotappl       If NotAppl allowed, doesn't request for operator confirmation
//
//      SetEnterOutOfRange          Set the capability to accept out-of-range to on or off
//      SetCanEnterOutOfRange       Enable the capability to accept out-of-range, with operator confirmation
//      SetCannotEnterOutOfRange    Disable the capability to accept out-of-range
//      SetMustAskWhenOutOfRange    If out-of-range allowed, force to ask for operator confirmation
//      SetDontAskWhenOutOfRange    If out-of-range allowed, doesn't request for operator confirmation
//
//      SetDisplayMessage           Set generation of "display" messages to on or off
//      SetDisplayMessageOn         Enable generation of "display" messages
//      SetDisplayMessageOff        Disable generation of "display" messages
//      SetErrmsgMessage            Set generation of "errmsg" messages to on or off
//      SetErrmsgMessageOn          Enable generation of "errmsg" messages
//      SetErrmsgMessageOff         Disable generation of "errmsg" messages
//
//      SetDisplayMsgNumber         Set the "display" messages' prefix-message-number to on or off
//      SetDisplayMsgNumberOn       Enable prefix-message-number in "display" messages
//      SetDisplayMsgNumberOff      Disable prefix-message-number in "display" messages
//      SetErrmsgMsgNumber          Set the "errmsg" messages' prefix-feature to on or off
//      SetErrmsgMsgNumberOn        Enable prefix-message-number in "errmsg" messages
//      SetErrmsgMsgNumberOff       Disable prefix-message-number in "errmsg" messages
//
//      SetMouseEnabled             Enable operator' mouse
//      SetMouseDisabled            Disable operator' mouse
//
//  (b) SetCanUseEndGroup           Entitle operator to request EndGroup actions
//  (b) SetCannotUseEndGroup        Forbide operator to request EndGroup actions
//  (b) SetCanUseEndLevel           Entitle operator to request EndLevel actions
//  (b) SetCannotUseEndLevel        Forbide operator to request EndLevel actions
//
//      SetAutoCheckRanges          Set 'automatic range-checking' on or off
//      SetAutoSkipStruc            Set 'automatic checking of skip-structure' on or off
//      SetAutoSkipStrucImpute      Set 'automatic checking for filling MISSING/NOTAPPL
//
//  Operators
//      <none>
//
//---------------------------------------------------------------------------

#include <engine/Defines.h>
#include <engine/Dict.h>


class CSettings
{
// Data Members
private:
    bool    m_bPathOn;                  // Full controlled path in data entry
    bool    m_bCanEnterNotappl;         // Entry of NotAppl allowed
    bool    m_bAskWhenNotappl;          // Ask for operator confirmation when entering NotAppl
    bool    m_bCanEnterOutOfRange;      // Entry of out-of-range values allowed
    bool    m_bAskWhenOutOfRange;       // Ask for operator confirmation when entering out-of-range values
    bool    m_bDisplayOn;               // Show 'display' messages
    bool    m_bErrmsgOn;                // Show 'errmsg' messages
    bool    m_bAutoEndGroup;            // In modify mode: call to EndGroup when DataOcc is reached. In Path Off is true, in Path on is false // RHF Nov 07, 2000
    bool    m_bCanEndLevel;             // Operator EndLevel action enabled/disabled

    bool    m_bHasCheckRanges;          // 'behavior CheckRanges' specified  // victor Dec 07, 00
    bool    m_bAutoCheckRanges;         // Automatic range-checking in Batch // victor Dec 07, 00
    bool    m_bHasSkipStruc;            // 'behavior SkipStruc' specified    // victor Mar 14, 01
    bool    m_bAutoSkipStruc;           // Automatic skips-checking in Batch // victor Dec 14, 01
    bool    m_bAutoSkipStrucImpute;      // Impute Missing/Notappl // RHF Nov 09, 2001

    bool    m_bExitWhenFinish;

    bool    m_treatSpecialValuesAsZero;  //GHM 20090827

    // --- Export options                             // victor Dec 18, 00
    bool    m_bExportData;              // ... export DAT file
    bool    m_bExportSPSS;              // ... export description for SPSS
    bool    m_bExportSAS;               // ... export description for SAS
    bool    m_bExportSTATA;             // ... export description for STATA
    bool    m_bExportCSPRO;             // ... export description for CSPRO
    bool    m_bExportR;

    bool    m_bExportTabDelim;
    bool    m_bExportCommaDelim;
    bool    m_bExportSemiColonDelim;

    bool    m_bExportItemOnly;
    bool    m_bExportSubItemOnly;
    bool    m_bExportItemSubItem;

    bool    m_bExportForceANSI; // GHM 20120416
    bool    m_bCommaDecimal;

    bool    m_bInteractiveEdit; // GHM 20130711 moved from CEntryDriver


// Methods
    // --- Construction/destruction & initialization
public:
    CSettings();

    // --- Extraction
public:
    bool    IsPathOn() const                  { return m_bPathOn;              }
    bool    IsPathOff() const                 { return !m_bPathOn;             }
    bool    CanEnterNotappl() const           { return m_bCanEnterNotappl;     }
    bool    AskWhenNotappl() const            { return m_bAskWhenNotappl;      }
    bool    CanEnterOutOfRange() const        { return m_bCanEnterOutOfRange;  }
    bool    AskWhenOutOfRange() const         { return m_bAskWhenOutOfRange;   }
    bool    IsDisplayMessageOn() const        { return m_bDisplayOn;           }
    bool    IsErrmsgMessageOn() const         { return m_bErrmsgOn;            }
    bool    CanUseEndLevel() const            { return m_bCanEndLevel;         }
    bool    IsAutoEndGroup() const            { return m_bAutoEndGroup;        } // RHF Nov 07, 2000

    bool    IsCheckingRanges() const          { return m_bAutoCheckRanges;     } // victor Dec 07, 00
    bool    IsCheckingSkipStruc() const       { return m_bAutoSkipStruc;       }   // victor Mar 14, 01
    bool    IsCheckingSkipStrucImpute() const { return m_bAutoSkipStrucImpute; }   // RHF Nov 09, 2001

    // --- Assignment
public:
    void    SetPathOn();
    void    SetPathOff();
    void    SetEnterNotappl( bool bSetOn ) {
                if( bSetOn )
                    SetCanEnterNotappl();
                else
                    SetCannotEnterNotappl();
    }
    void    SetCanEnterNotappl() {
                m_bCanEnterNotappl      = true;

                // related settings
                SetMustAskWhenNotappl();
    }
    void    SetCannotEnterNotappl() {
                m_bCanEnterNotappl      = false;

                // related settings
                SetMustAskWhenNotappl();
    }
    void    SetMustAskWhenNotappl() {
                m_bAskWhenNotappl       = true;
    }
    void    SetDontAskWhenNotappl() {
                m_bAskWhenNotappl       = false;
    }

    void    SetEnterOutOfRange( bool bSetOn ) {
                if( bSetOn )
                    SetCanEnterOutOfRange();
                else
                    SetCannotEnterOutOfRange();
    }
    void    SetCanEnterOutOfRange() {
                m_bCanEnterOutOfRange   = true;

                // related settings
                SetMustAskWhenOutOfRange();
    }
    void    SetCannotEnterOutOfRange() {
                m_bCanEnterOutOfRange   = false;

                // related settings
                SetMustAskWhenOutOfRange();
    }
    void    SetMustAskWhenOutOfRange() {
                m_bAskWhenOutOfRange    = true;
    }
    void    SetDontAskWhenOutOfRange() {
                m_bAskWhenOutOfRange    = false;
    }

    void    SetDisplayMessage( bool bSetOn ) {
                if( bSetOn )
                    SetDisplayMessageOn();
                else
                    SetDisplayMessageOff();
    }
    void    SetDisplayMessageOn() {
                m_bDisplayOn            = true;
    }
    void    SetDisplayMessageOff() {
                m_bDisplayOn            = false;
    }
    void    SetErrmsgMessage( bool bSetOn ) {
                if( bSetOn )
                    SetErrmsgMessageOn();
                else
                    SetErrmsgMessageOff();
    }
    void    SetErrmsgMessageOn() {
                m_bErrmsgOn             = true;
    }
    void    SetErrmsgMessageOff() {
                m_bErrmsgOn             = false;
    }

    void    SetCanUseEndLevel() {
                m_bCanEndLevel        = true;
    }
    void    SetCannotUseEndLevel() {
                m_bCanEndLevel        = false;
    }

    bool    HasCheckRanges() {                    // victor Dec 07, 00
                return m_bHasCheckRanges;
    }
    void    SetHasCheckRanges( bool bHas ) {            // victor Dec 07, 00
                m_bHasCheckRanges = bHas;
    }
    void    SetAutoCheckRanges( bool bCheck ) {         // victor Dec 07, 00
                m_bAutoCheckRanges = bCheck;
    }
    bool    HasSkipStruc() {                      // victor Mar 14, 01
                return m_bHasSkipStruc;
    }
    void    SetHasSkipStruc( bool bHas ) {              // victor Mar 14, 01
                m_bHasSkipStruc = bHas;
    }
    void    SetAutoSkipStruc( bool bCheck ) {           // victor Mar 14, 01
                m_bAutoSkipStruc = bCheck;
    }

    void    SetAutoSkipStrucImpute( bool bCheck ) {           // victor Mar 14, 01
                m_bAutoSkipStrucImpute = bCheck;
    }

    void    SetExitWhenFinish( bool bExitWhenFinish )   { m_bExitWhenFinish = bExitWhenFinish; }
    bool    GetExitWhenFinish() const                   { return m_bExitWhenFinish; }

    void    SetTreatSpecialValuesAsZero( bool bTreat )  { m_treatSpecialValuesAsZero = bTreat; } // GHM 200900827
    bool    GetTreatSpecialValuesAsZero() const         { return m_treatSpecialValuesAsZero; }   // GHM 200900827

    // --- Export options                             // victor Dec 18, 00
public:
    void    SetExportData( bool bSet )          { m_bExportData  = bSet; }
    void    SetExportSPSS( bool bSet )          { m_bExportSPSS  = bSet; }
    void    SetExportSAS( bool bSet )           { m_bExportSAS   = bSet; }
    void    SetExportSTATA( bool bSet )         { m_bExportSTATA = bSet; }
    void    SetExportCSPRO( bool bSet )         { m_bExportCSPRO = bSet; }
    void    SetExportR( bool bSet )             { m_bExportR = bSet; }

    void    SetExportTabDelim( bool bSet )       { m_bExportTabDelim = bSet; }
    void    SetExportCommaDelim( bool bSet )     { m_bExportCommaDelim = bSet; }
    void    SetExportSemiColonDelim( bool bSet ) { m_bExportSemiColonDelim = bSet; }

    void    SetExportItemOnly( bool bSet )      { m_bExportItemOnly = bSet; }
    void    SetExportSubItemOnly( bool bSet )   { m_bExportSubItemOnly = bSet; }
    void    SetExportItemSubItem( bool bSet )   { m_bExportItemSubItem = bSet; }

    void    SetExportForceANSI(bool b) { m_bExportForceANSI = b; } // GHM 20120416
    void    SetExportCommaDecimal(bool b) { m_bCommaDecimal = b; }

    void    SetInteractiveEdit(bool bFlag) { m_bInteractiveEdit = bFlag; } // GHM 20130711 moved from CEntryDriver


    bool    GetExportData() const                    { return m_bExportData;  }
    bool    GetExportSPSS() const                    { return m_bExportSPSS;  }
    bool    GetExportSAS() const                     { return m_bExportSAS;   }
    bool    GetExportSTATA() const                   { return m_bExportSTATA; }
    bool    GetExportCSPRO() const                   { return m_bExportCSPRO; }
    bool    GetExportR() const                       { return m_bExportR; }

    bool    GetExportTabDelim() const                { return m_bExportTabDelim; }
    bool    GetExportCommaDelim() const              { return m_bExportCommaDelim; }
    bool    GetExportSemiColonDelim() const          { return m_bExportSemiColonDelim; }

    bool    GetExportItemOnly() const                { return m_bExportItemOnly; }
    bool    GetExportSubItemOnly() const             { return m_bExportSubItemOnly; }
    bool    GetExportItemSubItem() const             { return m_bExportItemSubItem; }

    bool    GetExportForceANSI() const               { return m_bExportForceANSI; } // GHM 20120416
    bool    GetExportCommaDecimal() const            { return m_bCommaDecimal; }

    bool    GetInteractiveEdit() const               { return m_bInteractiveEdit; } // GHM 20130711 moved from CEntryDriver

    // ---------------------------- Other members ----------------------------
#define MAXQIDVARS  16                  // number of level-id variables

    // --- Miscellaneous
public:
    CString m_ApplName;                 // name of current application
    DICT*   m_Workdict;                 // internal workDict

public:
    CString m_Failmsg;

    int     m_QidVars[MaxNumberLevels][MAXQIDVARS];
    int     m_QidLength;

    // --- Data areas for APP & Dic loading operations
public:
                                        // I/O current parms:
    CString m_io_Dic;                   //   current Dict name
    CString m_io_Var;                   //           Var
    int     m_io_Err;                   //   type of problem

    bool    m_bHasExport;               // presence of export sentence
    bool    m_bHasCrosstab;             // presence of crosstab/table sentence
    bool    m_bHasFrequency;            // presence of freq sentence

private:
    CString m_csLevelZeroName;

public:
    void SetLevelZeroName(const CString& csLevelZeroName) {
        m_csLevelZeroName = csLevelZeroName;
        m_csLevelZeroName.MakeUpper();
    }

    const CString& GetLevelZeroName() const { return m_csLevelZeroName; }

    // -- other settings
    static  bool m_bNewTbd;
};


/////////////////////////////////////////////////////////////////////////////
//
//  miscellaneous macros
//
/////////////////////////////////////////////////////////////////////////////

#define ApplName            m_pEngineSettings->m_ApplName
#define Workdict            m_pEngineSettings->m_Workdict
#define Failmsg             m_pEngineSettings->m_Failmsg

#define QidVars             m_pEngineSettings->m_QidVars
#define QidLength           m_pEngineSettings->m_QidLength

#define io_Dic              m_pEngineSettings->m_io_Dic
#define io_Var              m_pEngineSettings->m_io_Var
#define io_Err              m_pEngineSettings->m_io_Err
