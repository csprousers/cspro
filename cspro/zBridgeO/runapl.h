#pragma once
// RunApl.h: base class for Running class (ENtry/BAtch)
//////////////////////////////////////////////////////////////////////

#include <ZBRIDGEO/zBridgeO.h>
#include <engine/runmodes.h>

class CNPifFile;

//////////////////////////////////////////////////////////////////////////////////////////////////////////////////
// CLASS NAME   : CRunApl
//
// PROTOTYPE    : class CLASS_DECL_ZBRIDGEO CRunApl  :public CObject
//
// OBJECTIVE    : Base class for talk with CsPro Engine
//
// REMARKS      : none
//
// CHANGES      : 20 Dec 1999, RHF, Creation
//////////////////////////////////////////////////////////////////////////////////////////////////////////////////

////////////////////////////////////////////////////////////////////////////////
// FUNCTION NAME: LoadCompile
//
// PROTOTYPE    : bool LoadCompile( void );
//
// OBJECTIVE    : Engine compile the application csAppFile.APP. If some syntax
// FALSE. If no error is found, return TRUE.
//
// PARAMETERS   : none
//
// RETURN       : TRUE if the application was correctly compiled. If some syntax
// error is found, the function returns FALSE.
//
// REMARKS      :
// - Only call one time per instance.
//
// CHANGES      : 23 Jun 1999, RHF, Creation
////////////////////////////////////////////////////////////////////////////////

////////////////////////////////////////////////////////////////////////////////
// FUNCTION NAME: End
//
// PROTOTYPE    : bool End();
//
// OBJECTIVE    : Inform to engine that the application has finished.
//
// PARAMETERS   : none
//
// RETURN       : TRUE if everything was OK. FALSE if not.
//
// REMARKS      : Call Only if LoadCompile was succesfull.
//
// CHANGES      : 23 Jun 1999, RHF, Creation
////////////////////////////////////////////////////////////////////////////////

////////////////////////////////////////////////////////////////////////////////
// FUNCTION NAME: Start
//
// PROTOTYPE    : bool CRunApl::Start( const int iMode );
//
// OBJECTIVE    : Let's ready to run. Engine check if the files given are
// correct for the aplication mode. If everything is ok, open the data files
// and the corresponding index files (DAT and IDX).
//
// PARAMETERS   : iMode that contains the mode of execution. CRUNAPL_ADD or
// CRUNAPL_MODIFY can be used for DataEntry. CRUNAPL_BATCH can be used for batch
//
// RETURN       : TRUE if everything was OK. FALSE if not.
//
// REMARKS      : Call Only if LoadCompile was succesfull.
//
// CHANGES      : 23 Jun 1999, RHF, Creation
////////////////////////////////////////////////////////////////////////////////

////////////////////////////////////////////////////////////////////////////////
// FUNCTION NAME: Stop
//
// PROTOTYPE    : bool CRunApl::Stop();
//
// OBJECTIVE    : Engine closes external data files and index files. Close the
// report file (LST)
//
// PARAMETERS   : none
//
// RETURN       : TRUE if everything was OK. FALSE if not.
//
// REMARKS      : Call Only if Start was succesfull.
//
// CHANGES      : 23 Jun 1999, RHF, Creation
////////////////////////////////////////////////////////////////////////////////


class CLASS_DECL_ZBRIDGEO CRunApl : public CObject
{
public:
    CRunApl();
    virtual ~CRunApl();

//  bool GetVal( const DEFLD* pDeField, csprochar* pszVarvalue, int* iStatus );

private:
    UINT m_uAppMode;        // Execucion mode. See checkfiles

protected:
    CNPifFile*   m_pPifFile ; //PFF File asoc. with this app

    bool m_bFlagAppLoaded;  // Flag for control only one call to LoadApplication
    bool m_bFlagStart;      // Flag for control Start/Stop

public:
//  bool GetVal( const int iVar, const int iOcc, csprochar* pszVarvalue, int* iStatus );

    bool LoadCompile( void );
    bool End();

    bool Start();
    bool Stop();

    bool HasAppLoaded( void ) const {
        return( m_bFlagAppLoaded );
    }

    bool HasStarted( void ) const {
        return( m_bFlagStart );
    }

    UINT GetAppMode( void ) const {
        return( m_uAppMode );
    }

    void SetAppMode( UINT uMode ) {
        m_uAppMode  = uMode;
    }
};
