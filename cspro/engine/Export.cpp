//---------------------------------------------------------------------------
//  File name: Export.cpp
//
//  Description:
//          Implementation for engine-Export class
//
//  History:    Date       Author   Comment
//              ---------------------------
//              20 Nov 99   RHF     Basic conversion
//              15 Jun 00   vc      Customization
//              22 Jun 00   vc      Adding 'class CExportVarCateg'
//              12 Dec 00   vc      Adding 'FORMAT ensembled record' capability
//              18 Dec 00   vc      Adding named exports & multiexport capability
//
//
//---------------------------------------------------------------------------

#include "StandardSystemIncludes.h"
#include "Tables.h"
#include "Export.h"
#include "Engarea.h"
#include "Engdrv.h"
#include "IntDrive.h"
#include "Limits.h"                                     // victor Dec 18, 00
#include <zEngineO/ValueSet.h>
#include <zToolsO/Tools.h>
#include <zDictO/ValueProcessor.h>
#include <ZBRIDGEO/npff.h>

#ifdef _DEBUG
#undef THIS_FILE
static char THIS_FILE[]= __FILE__;
#define new DEBUG_NEW
#endif

//////////////////////////////////////////////////////////////////////////////
//
// --- export gate & export interface
//
//////////////////////////////////////////////////////////////////////////////

void CExport::ExportDescriptions() {
    if( !IsExportActive() )             // check export gate - must be "normally open"
        return;

    int     iNumRecords;

    // RHF INIC Feb 15, 2005 NEW_EXPORT
    int             iNumRealRecords=1;
    SetUseMergedArray( false );
    if( GetExportToSPSS() || GetExportToSAS() || GetExportToSTATA() ) {
        if( GetRecordNumber() == 0 ) { // the first duplicated record
            SetUseMergedArray( true );
            MakeMergedArray();

            CExport*        pNextExport=this;
            while( pNextExport->GetNextExport() != NULL ) {
                if( pNextExport->GetNextExport()->IsExportActive() ) // RHF Jun 01, 2005
                    iNumRealRecords++;
                pNextExport = pNextExport->GetNextExport();
            }
        }
    }
    // RHF END Feb 15, 2005 NEW_EXPORT

    // SPSS codebook file
    if( GetExportToSPSS() && GetRecordNumber() < 1) {
        m_cNameSeparator = _T('$');         // $ for SPSS, _ for SAS, _ for STATA

        // RHF COM Feb 17, 2005 iNumRecords = SpsDescr( 0 );

        iNumRecords = iNumRealRecords; // RHF Feb 17, 2005 NEW_EXPORT

        SpsDescr( iNumRecords );
    }

    // SAS codebook file
    if( GetExportToSAS() && GetRecordNumber() < 1) {
        m_cNameSeparator = _T('_');         // $ for SPSS, _ for SAS, _ for STATA

        SasDescr();
    }

    // STATA (DCT & DO) files
    if( GetExportToSTATA() && GetRecordNumber() < 1) {
        m_cNameSeparator = _T('_');         // $ for SPSS, _ for SAS, _ for STATA

       // RHF COM Feb 17, 2005  iNumRecords = StataDescr( 0 );

        iNumRecords = iNumRealRecords; // RHF Feb 17, 2005 NEW_EXPORT

        StataDescr( iNumRecords );
    }

    // CSPRO Dcf file
    if( GetExportToCSPRO() ) {
        if (GetExportToSPSS() || GetExportToSAS() || GetExportToSTATA()) {
            SetUseMergedArray( false );
        }
        m_cNameSeparator = _T('\0');
        CsProDescr();
    }

    if( GetExportToR() && GetRecordNumber() < 1 )
    {
        m_cNameSeparator = _T('_');
        R_Descr();
    }

    SetUseMergedArray( false );  // RHF Feb 17, 2005 NEW_EXPORT

}

void CExport::ExportClose() {
    if( !IsExportAlive() )              // check export gate - must be "alredy open"
        return;

    if( m_pFileDat != NULL ) {          // exported data file
                if( m_bNeedClose )
                        fclose( m_pFileDat );
        m_pFileDat = NULL;
    }

    if( m_pFileSPSS != NULL ) {         // SPSS codebook
                // RHF INIC Feb 15, 2005 NEW_EXPORT
                if( m_iRecordNumber <= 0 ) //only close one time. -1 -->no duplicated, 0 --> there are duplicated but i'm the first one.
                // RHF END Feb 15, 2005 NEW_EXPORT

        fclose( m_pFileSPSS );
        m_pFileSPSS = NULL;
    }

    if( m_pFileSAS != NULL ) {          // SAS codebook
                // RHF INIC Feb 15, 2005 NEW_EXPORT
                if( m_iRecordNumber <= 0 ) //only close one time. -1 -->no duplicated, 0 --> there are duplicated but i'm the first one.
                // RHF END Feb 15, 2005 NEW_EXPORT

        fclose( m_pFileSAS );
        m_pFileSAS = NULL;
    }

    if( m_pFileSTATAdct != NULL ) {     // STATA DCT file
                // RHF INIC Feb 15, 2005 NEW_EXPORT
                if( m_iRecordNumber <= 0 ) //only close one time. -1 -->no duplicated, 0 --> there are duplicated but i'm the first one.
                // RHF END Feb 15, 2005 NEW_EXPORT

        fclose( m_pFileSTATAdct );
        m_pFileSTATAdct = NULL;
    }

    if( m_pFileSTATAdo != NULL ) {      // STATA DO file
                // RHF INIC Feb 15, 2005 NEW_EXPORT
                if( m_iRecordNumber <= 0 ) //only close one time. -1 -->no duplicated, 0 --> there are duplicated but i'm the first one.
                // RHF END Feb 15, 2005 NEW_EXPORT

        fclose( m_pFileSTATAdo );
        m_pFileSTATAdo = NULL;
    }

        if( m_pRecordCSPRO != NULL ) {         // CSPRO record
        delete m_pRecordCSPRO;
                m_pRecordCSPRO = NULL;
    }

    if( m_pFileR != NULL )
    {
        if( m_iRecordNumber <= 0 )
            fclose(m_pFileR);

        m_pFileR = NULL;
    }

    DeleteProgStrip();

    DeleteRecArea();

    SetExportClosed();                  // export is currently "closed"
}

//////////////////////////////////////////////////////////////////////////////
//
// --- program-strip
//
//////////////////////////////////////////////////////////////////////////////

char* CExport::GetFirstNode() {
    char*   pExpoNode = NULL;

    if( m_pHeadNode == NULL )
        SetHeadNode();

    if( m_pHeadNode != NULL )
        pExpoNode = (char*) m_pHeadNode + sizeof(EXP_HEADER_NODE);

    return pExpoNode;
}

void CExport::DeleteRecArea() {
    if( m_pExpRecArea != NULL )
        free( m_pExpRecArea );
    m_pExpRecArea = NULL;
    m_iExpRecMax  = 0;
    m_iExpRecNext = 0;
}

void CExport::CleanRecArea( int iSize ) {
    if( m_pExpRecArea != NULL && m_iExpRecMax > 0 ) {
        if( iSize < 1 ) {   // cleaning all record-area
            _tmemset( m_pExpRecArea, _T(' '), m_iExpRecMax );
            m_iExpRecNext = 0;
        }
        else {              // cleaning starting iSize bytes of record-area
            if( iSize > m_iExpRecMax )
                iSize = m_iExpRecMax;

            _tmemset( m_pExpRecArea, _T(' '), iSize );
        }
    }
}

//////////////////////////////////////////////////////////////////////////////
//
// --- generating descriptions
//
//////////////////////////////////////////////////////////////////////////////
#include "Exappl.h"

//////////////////////////////////////////////////////////////////////////////
//
// --- SPS description
//
//////////////////////////////////////////////////////////////////////////////

int CExport::SpsDescr( int iNumRecords ) {
    // - return the number of records generated to output
    // - 'iNumRecords' greater than zero means "generate output"
    // - 'iNumRecords' zero means "count the number of records"
    char*   pExpoNode = GetFirstNode();

    if( pExpoNode == NULL )
        return 0;

    ResetPendingSlash();

    bool    bGenerate = ( iNumRecords > 0 );
    int     iLoc=1;

    // data file
    if( bGenerate ) {
        //Added by Savy (R) 20090721
        //To print LRECL value in the sps file
        bool bUseSeparator=UseSeparator();
        int iRecLen = EnsembledTrip( &CExport::Export_GetRecordLen, false, &bUseSeparator );
        iRecLen += m_pHeadNode->m_iLenCaseId;
        if ( iRecLen > 8192 ){
            // GHM 20110404 the file handle command below was missing a period (trevor-reported bug)
            _ftprintf( m_pFileSPSS, _T("FILE HANDLE SPSSDATA/NAME='%s' LRECL=%d.\n"), GetExpoName().GetString(), iRecLen );
            _ftprintf( m_pFileSPSS, _T("DATA LIST FILE=SPSSDATA RECORDS=%d\n"), iNumRecords );
        }
        else{
            _ftprintf( m_pFileSPSS, _T("DATA LIST FILE='%s' RECORDS=%d\n"), GetExpoName().GetString(), iNumRecords );
        }
    }
    CIntDriver* pIntDriver = m_pEngineDriver->m_pIntDriver.get();

    pIntDriver->PrepareForExportExec( m_pHeadNode->m_iExportProcSymbol, m_pHeadNode->m_iExportProcType );

    // Process
    while( *pExpoNode != Exp_EOCASE ) {
        switch( *pExpoNode ) {
            case Exp_ENSEMBLE: //SpsDescr
                pExpoNode += sizeof(EXP_ENSEMBLE_NODE);
                                SetPendingSlash();

                                if( bGenerate ) {
                                    int iSizeCaseId = m_pHeadNode->m_iLenCaseId;

                                        if( Export4ByteUnicodeAlphas() ) // GHM 20130502
                                            iSizeCaseId = m_pHeadNode->m_iLenCaseIdUnicode;

                                        iLoc = 1 + iSizeCaseId + m_pHeadNode->m_iLenRecId;
                                        EnsembledTrip( &CExport::Export_SpssDescription, false, &iLoc );
                                }
                                else
                                        iNumRecords = EnsembledTrip( &CExport::Export_NumRecords, false );


                break;

            case Exp_EOENSEMBLE:
                pExpoNode += 1;
                break;
                        default:
                                ASSERT(0);
                                break;
        }
    }

    if( bGenerate ) {
        _ftprintf( m_pFileSPSS, _T(".\n") );

                ResetPendingSlash();
                EnsembledTrip( &CExport::Export_SpssVarLabel, false, NULL, &m_iCaseIdItems, &m_aCaseIdOccExpr, &m_aCaseIdOccExprProcSymbol );// RHF Feb 15, 2005
                EnsembledTrip( &CExport::Export_SpssVarLabel, false );
                if( GetPendingSlash() ) _ftprintf( m_pFileSPSS, _T(".\n") );

                ResetPendingSlash();
                EnsembledTrip( &CExport::Export_SpssMissing, false, NULL, &m_iCaseIdItems, &m_aCaseIdOccExpr, &m_aCaseIdOccExprProcSymbol );// RHF Feb 15, 2005
                EnsembledTrip( &CExport::Export_SpssMissing, false );
                if( GetPendingSlash() ) _ftprintf( m_pFileSPSS, _T(".\n") );

                ResetPendingSlash();
                EnsembledTrip( &CExport::Export_SpssValueLabels, false, NULL, &m_iCaseIdItems, &m_aCaseIdOccExpr, &m_aCaseIdOccExprProcSymbol );// RHF Feb 15, 2005
                EnsembledTrip( &CExport::Export_SpssValueLabels, false );
                if( GetPendingSlash() ) _ftprintf( m_pFileSPSS, _T(".\n") );

        _ftprintf( m_pFileSPSS, _T("EXECUTE.\n") );  // BMD 04 May 2004
    }
    return iNumRecords;
}

void CExport::sps_variable( VART* pVarT, const CString& csExportVarName, int & iLoc ) {
    if( GetPendingSlash() ) {
        _ftprintf( m_pFileSPSS, _T(" /\n") ); // add space before continuation line so that spss syntax will work when
                                        // used as include file.  JH 1/18/07

        ResetPendingSlash();

        // CASE_ID & RECORD Type
        if( !m_bCaseIdReady )
                        if( m_pHeadNode->m_bCaseIdAfterRecType ) {
                                if( m_pHeadNode->m_iLenRecId > 0 )
                                        _ftprintf( m_pFileSPSS, _T(" %-8s %4d-%-4d (A)\n"), _T("REC$TYPE"), 1, m_pHeadNode->m_iLenRecId );
                                if( m_pHeadNode->m_iLenCaseId > 0 ) {
                                        // RHF COM Feb 14, 2005 fprintf( m_pFileSPSS, " %-8s %4d-%-4d (A)\n", "CASE$ID", 1+m_pHeadNode->m_iLenRecId, m_pHeadNode->m_iLenCaseId+m_pHeadNode->m_iLenRecId );
                                        int             iCaseLoc=1+m_pHeadNode->m_iLenRecId;
                                        EnsembledTrip( &CExport::Export_SpssDescription, false, &iCaseLoc, &m_iCaseIdItems, &m_aCaseIdOccExpr, &m_aCaseIdOccExprProcSymbol );
                                }
                        }
                        else {
                                if( m_pHeadNode->m_iLenCaseId > 0 ) {
                                        // RHF COM Feb 14, 2005 fprintf( m_pFileSPSS, " %-8s %4d-%-4d (A)\n", "CASE$ID", 1, m_pHeadNode->m_iLenCaseId );
                                        int             iCaseLoc=1;
                                        EnsembledTrip( &CExport::Export_SpssDescription, false, &iCaseLoc, &m_iCaseIdItems, &m_aCaseIdOccExpr, &m_aCaseIdOccExprProcSymbol );
                                }
                                if( m_pHeadNode->m_iLenRecId > 0 )
                                        _ftprintf( m_pFileSPSS, _T(" %-8s %4d-%-4d (A)\n"), _T("REC$TYPE"), 1+m_pHeadNode->m_iLenCaseId, m_pHeadNode->m_iLenRecId + m_pHeadNode->m_iLenCaseId);
                        }
            m_bCaseIdReady = true;
        }

        int iVarLength = pVarT->GetLength();

        if( pVarT->GetDecimals() && !pVarT->GetDecChar() ) // GHM 20120504 we need to add the decimal character to the output
            iVarLength++;

        else if( pVarT->IsAlpha() && !GetExportForceANSI() ) // we will output four bytes for every character to satisfy all UTF-8 possibilities
            iVarLength *= 4;


        _ftprintf( m_pFileSPSS, _T(" %-8s %4d-%-4d"), csExportVarName.GetString(), iLoc, iLoc + iVarLength - 1 );

        if( pVarT->IsNumeric() ) {
                if( pVarT->GetDecimals() > 0 )
                        _ftprintf( m_pFileSPSS, _T(" (%d)"), pVarT->GetDecimals() );
        }
        else
                _ftprintf( m_pFileSPSS, _T(" (A)") );

        _fputtc( _T('\n'), m_pFileSPSS );

    iLoc += iVarLength; // GHM 20120504 moved from CExport::Export_SpssDescription
}

void CExport::sps_label( csprochar* pszGenLabel, csprochar* pszLabel ) {
    bool    bHasDouble = false;
    bool    bHasSingle = false;
    csprochar*   p;

    for( p = pszLabel; *p; p++ )
        if( *p == '\"' )
            bHasDouble = true;
        else if( *p == '\'' )
            bHasSingle = true;

    if( !bHasDouble )
        _stprintf( pszGenLabel, _T("\"%ls\""), pszLabel );
    else if( !bHasSingle )
        _stprintf( pszGenLabel, _T("'%ls'"), pszLabel );
    else {
        *pszGenLabel++ = _T('\'');

        for( p = pszLabel; *p; p++ )
            if( *p != '\'' )
                *pszGenLabel++ = *p;
        *pszGenLabel++ = _T('\'');
        *pszGenLabel++ = 0;
    }
}

//////////////////////////////////////////////////////////////////////////////
//
// --- SAS description
//
//////////////////////////////////////////////////////////////////////////////

void CExport::SasDescr() {
    char*   pExpoNode = GetFirstNode();

    if( pExpoNode == NULL )
        return;

    CIntDriver* pIntDriver = m_pEngineDriver->m_pIntDriver.get();
    pIntDriver->PrepareForExportExec( m_pHeadNode->m_iExportProcSymbol, m_pHeadNode->m_iExportProcType );

    CMap<VART*,VART*,int,int>       aMapSasLabel;

    sas_format( &aMapSasLabel );

    // RHF COM Oct 15, 2004 m_bCaseIdReady = ( !m_pHeadNode->m_bHasCaseId );

    m_bCaseIdReady = (m_pHeadNode->m_iLenCaseId == 0 && m_pHeadNode->m_iLenRecId == 0 );

    CString sFullFileName = GetExpoName();
    CString sFileNameOnly = sFullFileName.Left(sFullFileName.ReverseFind('.'));
    sFileNameOnly = sFileNameOnly.Mid(sFileNameOnly.ReverseFind(PATH_CHAR) + 1);
    _ftprintf( m_pFileSAS, _T("Data user.%s;\n"), sFileNameOnly.GetString() );

    sas_attrib( &aMapSasLabel );

    // calculate the record' maximum length
    int     iRecLen = sas_input( 0 );
    CString csFName = GetExpoName(); // RHF Apr 30, 2002

    if( GetExportForceANSI() ) // GHM 20120417
        _ftprintf( m_pFileSAS, _T("infile '%s' LRECL=%d TRUNCOVER ;\n"), csFName.GetString(), iRecLen );
    else
        _ftprintf( m_pFileSAS, _T("infile '%s' encoding=\"utf-8\" LRECL=%d TRUNCOVER ;\n"), csFName.GetString(), iRecLen ); // GHM 20120120 for unicode

    // generate input format
    // RHF COM Oct 15, 2004 m_bCaseIdReady = ( !m_pHeadNode->m_bHasCaseId );
    m_bCaseIdReady = (m_pHeadNode->m_iLenCaseId == 0 && m_pHeadNode->m_iLenRecId == 0 );

    sas_input( iRecLen );

    // convert missing data
    sas_missing();
    _ftprintf( m_pFileSAS, _T("Run;\n") );  // BMD 03 Mar 2004
}

void CExport::sas_format( CMap<VART*,VART*,int,int>* pMapSasLabel ) {
    char*   pExpoNode = GetFirstNode();

    // BMD 04 Mar 2004 begin
    CString sFullFileName = GetExpoName();
    CString sPath = sFullFileName.Left(sFullFileName.ReverseFind(PATH_CHAR));
    _ftprintf( m_pFileSAS, _T("libname user '%s';\n"), sPath.GetString() );
    // BMD 04 Mar 2004 end
    _ftprintf( m_pFileSAS, _T("Proc format;\n") );
    m_iFmtCount = 1;   // BMD 04 Mar 2004


    while( *pExpoNode != Exp_EOCASE ) {
        switch( *pExpoNode ) {
            case Exp_ENSEMBLE: //sas_format
                pExpoNode += sizeof(EXP_ENSEMBLE_NODE);

                                EnsembledTrip( &CExport::Export_SasFormat, false, pMapSasLabel, &m_iCaseIdItems, &m_aCaseIdOccExpr, &m_aCaseIdOccExprProcSymbol ); // RHF Feb 15, 2005
                                EnsembledTrip( &CExport::Export_SasFormat, false, pMapSasLabel );
                break;

            case Exp_EOENSEMBLE:
                pExpoNode += 1;
                break;
                        default:
                                ASSERT(0);
                                break;
        }
    }
}

void CExport::sas_vallabels( VART* pVarT, CMap<VART*,VART*,int,int>* pMapSasLabel ) {
    int iFmtNum;
    if( pMapSasLabel->Lookup( pVarT, iFmtNum ) )
        return;

    int     iNumCategories = SASBuildVarCategories( pVarT );
    bool    bHasValueLabels = ( iNumCategories > 0 );

    if( bHasValueLabels ) {
        csprochar    pszLabel[_MAXLABLEN+2];
        csprochar    pszGenLabel[512];

        CString sFmtNum;
        sFmtNum.Format(_T("%05d"),m_iFmtCount);
        if( pVarT->IsNumeric() ) {
            _ftprintf( m_pFileSAS, _T("  value F%s_\n"), sFmtNum.GetString() );
        }
        else {
            _ftprintf( m_pFileSAS, _T("  value $F%s_\n"), sFmtNum.GetString() );
        }

        pMapSasLabel->SetAt( pVarT, m_iFmtCount );
        m_iFmtCount++;

        int     slot_len = std::min( pVarT->GetLength(), 5 );
        csprochar    pszValue[_MAXLABLEN+2];

        for( int iValue = 0; iValue < iNumCategories; iValue++ ) {
            CExportVarCateg*    pVarCateg = GetVarCategoryAt( iValue );

            CopyQuotedStringWithLengthLimit( pszValue, pVarCateg->csTextValue );
            trimleft( pszValue );

            // labels should be <= _MAXLABLEN but for some reason CExport::SASBuildVarCategories can right-pad labels and make them longer than that
            CopyQuotedStringWithLengthLimit( pszLabel, pVarCateg->csTextLabel );
            sas_label( pszGenLabel, pszLabel );

            if( pVarT->IsNumeric() )
                _ftprintf( m_pFileSAS, _T("%6s = %s\n"), pszValue, pszGenLabel );
            else
//              fprintf( m_pFileSAS, "'%-*s' = %s\n", slot_len, pszValue, pszGenLabel );
                _ftprintf( m_pFileSAS, _T("%-*s = %s\n"), slot_len, pszValue, pszGenLabel );    // BMD 04 Mar 2004
        }

        _ftprintf( m_pFileSAS, _T("%6s\n"), _T(";") );
    }

    DeleteVarCategories();
}

void CExport::sas_attrib( CMap<VART*,VART*,int,int>* pMapSasLabel ) {
    char*   pExpoNode = GetFirstNode();

    while( *pExpoNode != Exp_EOCASE ) {
        switch( *pExpoNode ) {
            case Exp_ENSEMBLE: //sas_attrib
                pExpoNode += sizeof(EXP_ENSEMBLE_NODE);
                                EnsembledTrip( &CExport::Export_SasAttrib, false, pMapSasLabel );
                break;

            case Exp_EOENSEMBLE:
                pExpoNode += 1;
                break;
                        default:
                                ASSERT(0);
                                break;
        }
    }
}

void CExport::sas_missing() {
    // convert missing values
    char*   pExpoNode = GetFirstNode();

    while( *pExpoNode != Exp_EOCASE ) {
        switch( *pExpoNode ) {
            case Exp_ENSEMBLE: //sas_missing
                pExpoNode += sizeof(EXP_ENSEMBLE_NODE);
                                EnsembledTrip( &CExport::Export_SasMissing, false, NULL, &m_iCaseIdItems, &m_aCaseIdOccExpr, &m_aCaseIdOccExprProcSymbol ); // RHF Feb 15, 2005
                                EnsembledTrip( &CExport::Export_SasMissing, false );
                break;

            case Exp_EOENSEMBLE:
                pExpoNode += 1;
                break;
                        default:
                                ASSERT(0);
                                break;

        }
    }
}

void CExport::sas_misvalue( VART* pVarT, int aIndex[DIM_MAXDIM], int aDimFlag[DIM_MAXDIM] ) {
    if( !pVarT->IsNumeric() )
        return;

    const   NumericValueProcessor& numeric_value_processor = pVarT->GetCurrentNumericValueProcessor();
    double  dValue = numeric_value_processor.ConvertNumberFromEngineFormat(MISSING);
    bool    bHasMissing = !IsSpecial(dValue);

    if( bHasMissing ) {
        CString csExportedVarName;

        GenVarName( csExportedVarName, pVarT, aIndex, aDimFlag );

        // _ftprintf( m_pFileSAS, _T("    if %-8s = %6.0f then %s = .;\n"),

        // GHM 20140905 see my SPSS fix on 20130907 for an explanation of the change
        CString formatStr;
        formatStr.Format(_T("    if %%-8ls = %%.%df then %%s = .;\n"),pVarT->GetDecimals());
        _ftprintf( m_pFileSAS, formatStr, csExportedVarName.GetString(), dValue, csExportedVarName.GetString() );
    }
}

int CExport::sas_input( int iRecLen ) {
    // - produce input format
    // - returns the record' maximum length
    // - 'iRecLen' greater than zero means "generate output"
    // - 'iRecLen' zero means "calculate the maximum length"
    char*   pExpoNode = GetFirstNode();
    bool    bGenerate = ( iRecLen > 0 );
    int     iLoc=1;

    int iLenCaseId = GetExportForceANSI() ? m_pHeadNode->m_iLenCaseId : m_pHeadNode->m_iLenCaseIdUnicode;

    while( *pExpoNode != Exp_EOCASE ) {
        switch( *pExpoNode ) {
            case Exp_ENSEMBLE: //sas_input
                pExpoNode += sizeof(EXP_ENSEMBLE_NODE);

                if( bGenerate ) {
                    _ftprintf( m_pFileSAS, _T("    input\n") );
                    iLoc = 1 + iLenCaseId + m_pHeadNode->m_iLenRecId;
                    EnsembledTrip( &CExport::Export_SasInput, false, &iLoc );
                    _ftprintf( m_pFileSAS, _T("    ;\n") );
                }
                else {
                    iRecLen = EnsembledTrip( &CExport::Export_GetRecordLen, false );
                    iRecLen += iLenCaseId + m_pHeadNode->m_iLenRecId; // RHF Nov 08, 2004
                }

                break;

            case Exp_EOENSEMBLE:
                pExpoNode += 1;
                break;
                        default:
                                ASSERT(0);
                                break;
        }
    }

    return iRecLen;
}

void CExport::sas_inpvariable( VART* pVarT, int aIndex[DIM_MAXDIM], int aDimFlag[DIM_MAXDIM], int iLoc ) {
    // CASE_ID & RECORD Type
    if( !m_bCaseIdReady ) {
                if( m_pHeadNode->m_bCaseIdAfterRecType ) {
                        if( m_pHeadNode->m_iLenRecId > 0 )
                                _ftprintf( m_pFileSAS, _T("    @%-4d %-8s $%d.\n"), 1, _T("REC_TYPE"), m_pHeadNode->m_iLenRecId  );
                        if( m_pHeadNode->m_iLenCaseId > 0 ) {
                                // RHF COM Feb 15, 2005 fprintf( m_pFileSAS, "    @%-4d %-8s $%d.\n", 1+m_pHeadNode->m_iLenRecId, "CASE_ID", m_pHeadNode->m_iLenCaseId );

                                // RHF INIC Feb 15, 2005
                                int             iCaseLoc=1+m_pHeadNode->m_iLenRecId;
                                m_bCaseIdReady = true;
                                EnsembledTrip( &CExport::Export_SasInput, false, &iCaseLoc, &m_iCaseIdItems, &m_aCaseIdOccExpr, &m_aCaseIdOccExprProcSymbol );
                                // RHF END Feb 15, 2005
                        }
                }
                else {
                        if( m_pHeadNode->m_iLenCaseId > 0 ) {
                                // RHF COM Feb 15, 2005 fprintf( m_pFileSAS, "    @%-4d %-8s $%d.\n", 1, "CASE_ID", m_pHeadNode->m_iLenCaseId );

                                // RHF INIC Feb 15, 2005
                                int             iCaseLoc=1;
                                m_bCaseIdReady = true;
                                EnsembledTrip( &CExport::Export_SasInput, false, &iCaseLoc, &m_iCaseIdItems, &m_aCaseIdOccExpr, &m_aCaseIdOccExprProcSymbol );
                                // RHF END Feb 15, 2005
                        }
                        if( m_pHeadNode->m_iLenRecId > 0 )
                                _ftprintf( m_pFileSAS, _T("    @%-4d %-8s $%d.\n"), 1+m_pHeadNode->m_iLenCaseId, _T("REC_TYPE"), m_pHeadNode->m_iLenRecId  );
                }

        m_bCaseIdReady = true;
    }

        CString csExportedVarName;

        GenVarName( csExportedVarName, pVarT, aIndex, aDimFlag );

    _ftprintf( m_pFileSAS, _T("    @%-4d %-8s "), iLoc, csExportedVarName.GetString() );

    int iVarLength = pVarT->GetLength();

    if( pVarT->GetDecimals() && !pVarT->GetDecChar() ) // GHM 20120504 we need to add the decimal character to the output
        iVarLength++;

    else if( pVarT->IsAlpha() && !GetExportForceANSI() ) // we will output four bytes for every character to satisfy all UTF-8 possibilities
        iVarLength *= 4;

    if( pVarT->IsNumeric() )
        _ftprintf( m_pFileSAS, _T("%d.%d\n"), iVarLength, pVarT->GetDecimals() );
    else
        _ftprintf( m_pFileSAS, _T("$%d.\n"), iVarLength );
}

void CExport::sas_varattrib( VART* pVarT, int aIndex[DIM_MAXDIM], int aDimFlag[DIM_MAXDIM], CMap<VART*,VART*,int,int>* pMapSasLabel ) {
    csprochar pszLabel[_MAXLABLEN+2];
    csprochar pszGenLabel[512];

    // CASE_ID & RECORD Type
    // BMD 03 Mar 2004 begin   various changes in length and format
    if( !m_bCaseIdReady ) {
        csprochar    pszCaseIdLabel[_MAXLABLEN+2];
        csprochar    pszRecTypeLabel[_MAXLABLEN+2];
        csprochar    pszCaseIdGenLabel[512];
        csprochar    pszRecTypeGenLabel[512];

        _stprintf( pszCaseIdLabel, _T("Case Identifier (length %d)"), m_pHeadNode->m_iLenCaseId );
        sas_label( pszCaseIdGenLabel, pszCaseIdLabel );

        _stprintf( pszRecTypeLabel, _T("Record Type (length %d)"), m_pHeadNode->m_iLenRecId );
        sas_label( pszRecTypeGenLabel, pszRecTypeLabel );

                if( m_pHeadNode->m_bCaseIdAfterRecType ) {
                        if( m_pHeadNode->m_iLenRecId > 0 )
                                _ftprintf( m_pFileSAS, _T("  attrib %-8s length=$%d label=%s;\n"), _T("REC_TYPE"), m_pHeadNode->m_iLenRecId, pszRecTypeGenLabel );
                        if( m_pHeadNode->m_iLenCaseId > 0 ) {
                                // RHF COM Feb 15, 2005 fprintf( m_pFileSAS, "  attrib %-8s length=$%d label=%s;\n", "CASE_ID", m_pHeadNode->m_iLenCaseId, pszCaseIdGenLabel );

                                // RHF INIC Feb 15, 2005
                                //int           iCaseLoc=1+m_pHeadNode->m_iLenRecId;
                                m_bCaseIdReady = true;
                                EnsembledTrip( &CExport::Export_SasAttrib, false, pMapSasLabel, &m_iCaseIdItems, &m_aCaseIdOccExpr, &m_aCaseIdOccExprProcSymbol );
                                // RHF END Feb 15, 2005
                        }
                }
                else {
                        if( m_pHeadNode->m_iLenCaseId > 0 ) {
                                // RHF COM Feb 15, 2005 fprintf( m_pFileSAS, "  attrib %-8s length=$%d label=%s;\n", "CASE_ID", m_pHeadNode->m_iLenCaseId, pszCaseIdGenLabel );

                                // RHF INIC Feb 15, 2005
                                //int           iCaseLoc=1;
                                m_bCaseIdReady = true;
                                EnsembledTrip( &CExport::Export_SasAttrib, false, pMapSasLabel, &m_iCaseIdItems, &m_aCaseIdOccExpr, &m_aCaseIdOccExprProcSymbol );
                                // RHF END Feb 15, 2005

                        }
                        if( m_pHeadNode->m_iLenRecId > 0 )
                                _ftprintf( m_pFileSAS, _T("  attrib %-8s length=$%d label=%s;\n"), _T("REC_TYPE"), m_pHeadNode->m_iLenRecId, pszRecTypeGenLabel );
                }

        m_bCaseIdReady = true;
    }

    *pszGenLabel = 0;

    GetVarLabel( pVarT, pszLabel );

    if( *pszLabel )
        sas_label( pszGenLabel, pszLabel );

    CString csExportedVarName;
    GenVarName( csExportedVarName, pVarT, aIndex, aDimFlag );

    _ftprintf( m_pFileSAS, _T("  attrib %-8s"), csExportedVarName.GetString() );

    if(!pVarT->IsNumeric() ) {
        _ftprintf( m_pFileSAS, _T(" length=$%d"), pVarT->GetLength() * ( GetExportForceANSI() ? 1 : 4 ) );
    }
    // Determine if any values are written
    bool bHasFormat = false;
    const CDictItem* dict_item = pVarT->GetDictItem();
    if( dict_item->HasValueSets() ) {
        const auto& dict_value_set = dict_item->GetValueSet(0); // the base Value-set
        for( const auto& dict_value : dict_value_set.GetValues() ) {
            // only for non-special values (or refused), single-pair, no-range (without 'to' bound) count
            if (( !dict_value.IsSpecial() || dict_value.IsSpecialValue(REFUSED) ) && dict_value.GetNumValuePairs() == 1 &&
                     SO::IsBlank(dict_value.GetValuePair(0).GetTo()) &&
                    !dict_value.GetLabel().IsEmpty() ) {
                bHasFormat = true;
                break;
            }
        }
    }
    if(bHasFormat) {
        // RHF INIC Oct 15, 2004
        int i;
        if( pMapSasLabel->Lookup( pVarT, i ) )
                ;
        else {
            ASSERT(0);
            i = m_iFmtCount;
        }
        // RHF END Oct 15, 2004

        CString sFmtNum;
        sFmtNum.Format(_T("%05d"),i);
        if( pVarT->IsNumeric() ) {
            _ftprintf( m_pFileSAS, _T(" format=F%s_."), sFmtNum.GetString() );
        }
        else {
            _ftprintf( m_pFileSAS, _T(" format=$F%s_."), sFmtNum.GetString() );
        }
    }
    if( *pszGenLabel ) {
        CString csLabel = pszGenLabel;

        CString sOcc;
        GenOccName( sOcc, _T(','), false, pVarT, aIndex, aDimFlag );
        if( sOcc.GetLength() > 0 ) {
            int iLabelLen=csLabel.GetLength();
            if( iLabelLen> 0 && csLabel[iLabelLen-1]=='\"' )
                    csLabel.Delete(iLabelLen-1);

            csLabel += _T("(") + sOcc + _T(")\"");
        }

        _ftprintf( m_pFileSAS,_T(" label=%s"), csLabel.GetString() );
    }

    _ftprintf( m_pFileSAS, _T(";\n") );
    // BMD 03 Mar 2004 end
}

void CExport::sas_label( csprochar* pszGenLabel, csprochar* pszLabel ) {
    *pszGenLabel++ = _T('"');

    for( csprochar* p = pszLabel; *p; p++ ) {
        if( *p != '"' )
            *pszGenLabel++ = *p;
        else {
            *pszGenLabel++ = _T('"');
            *pszGenLabel++ = _T('"');
        }
    }

    *pszGenLabel++ = _T('"');
    *pszGenLabel++ = 0;
}

//////////////////////////////////////////////////////////////////////////////
//
// --- STATA description                                // RHF 15/3/99
//
//////////////////////////////////////////////////////////////////////////////

int CExport::StataDescr( int iNumRecords ) {
    // - return the number of records generated to output
    // - 'iNumRecords' greater than zero means "generate output"
    // - 'iNumRecords' zero means "count the number of records"
    char*   pExpoNode = GetFirstNode();

    if( pExpoNode == NULL )
        return 0;

    CIntDriver* pIntDriver = m_pEngineDriver->m_pIntDriver.get();
    pIntDriver->PrepareForExportExec( m_pHeadNode->m_iExportProcSymbol, m_pHeadNode->m_iExportProcType );

    // RHF COM Oct 15, 2004m_bCaseIdReady = ( !m_pHeadNode->m_bHasCaseId ); // RHF Apr 30, 2002
    m_bCaseIdReady = (m_pHeadNode->m_iLenCaseId == 0 && m_pHeadNode->m_iLenRecId == 0 );

    ResetPendingSlash();

    bool    bGenerate = ( iNumRecords > 0 );
    int     iLoc=1;

    // data file
    if( bGenerate ) {
        CIMSAString sInfixDict = GetExpoName();
        _ftprintf( m_pFileSTATAdct, _T("infix dictionary using \"%s\" {\n"), sInfixDict.GetString());//Chirag Mar 13, 2002
        _ftprintf( m_pFileSTATAdct, _T("%d lines\n"), iNumRecords );
    }

    // VARIABLE LIST
    while( *pExpoNode != Exp_EOCASE ) {
        switch( *pExpoNode ) {
            case Exp_ENSEMBLE: //StataDescr
                pExpoNode += sizeof(EXP_ENSEMBLE_NODE);

                SetPendingSlash();

                                if( bGenerate ) {
                                        iLoc = 1 + ( GetExportForceANSI() ? m_pHeadNode->m_iLenCaseId : m_pHeadNode->m_iLenCaseIdUnicode ) + m_pHeadNode->m_iLenRecId;
                                        EnsembledTrip( &CExport::Export_StataDescription, false, &iLoc );
                                }
                                else
                                        iNumRecords = EnsembledTrip( &CExport::Export_NumRecords, false );

                break;

            case Exp_EOENSEMBLE:
                pExpoNode += 1;
                break;
                        default:
                                ASSERT(0);
                                break;
        }
    }

    if( bGenerate ) {
        _ftprintf( m_pFileSTATAdct, _T("}\n") );

        // RHF INIC Apr 30, 2002
        if( HasDefaultName() ){
            CIMSAString sInfixDict = PortableFunctions::PathRemoveFileExtension<CString>(GetExpoName()) + FileExtensions::WithDot::StataDictionary;
            if(!m_pEngineDriver->GetPifFile()->GetSTATASyntaxFName().IsEmpty()){
                sInfixDict =m_pEngineDriver->GetPifFile()->GetSTATASyntaxFName();
            }
            _ftprintf( m_pFileSTATAdo, _T("infix using \"%s\"\n\n"), sInfixDict.GetString());
        }
        else{
            // RHF END Apr 30, 2002
            //SAVY commented this line . This is set only if GetSTATSyntaxFname is empty
            //fprintf( m_pFileSTATAdo, "infix using \"%s.dct\"\n\n", GetExpoName() );
            CIMSAString sInfixDict;
            //First set to RHF way of adding ".dct" to the name
            sInfixDict.Format(_T("%s.dct"), GetExpoName().GetString());//first set to default RHF string
            //If STATASynxtFname found use it . the extension is .dct comes from csexport.Do not
            //change ext here
            if(!m_pEngineDriver->GetPifFile()->GetSTATASyntaxFName().IsEmpty()){
                sInfixDict =m_pEngineDriver->GetPifFile()->GetSTATASyntaxFName();
            }
            _ftprintf( m_pFileSTATAdo, _T("infix using \"%s\"\n\n"), sInfixDict.GetString());
        }
                ResetPendingSlash();
                EnsembledTrip( &CExport::Export_StataVarLabel, false, NULL, &m_iCaseIdItems, &m_aCaseIdOccExpr, &m_aCaseIdOccExprProcSymbol );// RHF Feb 15, 2005
                EnsembledTrip( &CExport::Export_StataVarLabel, false );

                ResetPendingSlash();
                EnsembledTrip( &CExport::Export_StataValueLabels, false, NULL, &m_iCaseIdItems, &m_aCaseIdOccExpr, &m_aCaseIdOccExprProcSymbol );// RHF Feb 15, 2005
                EnsembledTrip( &CExport::Export_StataValueLabels, false );

                ResetPendingSlash();
                EnsembledTrip( &CExport::Export_StataAsocValueLabels, false, NULL, &m_iCaseIdItems, &m_aCaseIdOccExpr, &m_aCaseIdOccExprProcSymbol );// RHF Feb 15, 2005
                EnsembledTrip( &CExport::Export_StataAsocValueLabels, false );
    }

    return iNumRecords;
}

void CExport::stata_variable( VART* pVarT, const CString& csExportedVarName, int iLoc, int iRecNum ) {
    csprochar    pszGenName[256];
    static  int iLastRecNum = -1;

    // RHF INIC Apr 30, 2002
        //CASE_ID & RECORD Type
    if( !m_bCaseIdReady ) {
        csprochar*   pszVarType=_T("str");

        if( m_pHeadNode->m_bCaseIdAfterRecType ) {
                        if( m_pHeadNode->m_iLenRecId > 0 )
                                _ftprintf( m_pFileSTATAdct, _T("    %-8s %-8s %4d:%4d-%-4d\n"), pszVarType, _T("REC_TYPE"), iRecNum, 1, m_pHeadNode->m_iLenRecId );

                        if( m_pHeadNode->m_iLenCaseId > 0 ) {
                                // RHF COM Feb 15, 2005 fprintf( m_pFileSTATAdct, "    %-8s %-8s %4d:%4d-%-4d\n", pszVarType, "CASE_ID", iRecNum, 1+m_pHeadNode->m_iLenRecId, m_pHeadNode->m_iLenCaseId+m_pHeadNode->m_iLenRecId );
                                int             iCaseLoc=1+m_pHeadNode->m_iLenRecId;
                                m_bCaseIdReady = true; // RHF Feb 15, 2005
                                EnsembledTrip( &CExport::Export_StataDescription, false, &iCaseLoc, &m_iCaseIdItems, &m_aCaseIdOccExpr, &m_aCaseIdOccExprProcSymbol );
                        }
                }
                else {
                        if( m_pHeadNode->m_iLenCaseId > 0 ) {
                                // RHF COM Feb 15, 2005 fprintf( m_pFileSTATAdct, "    %-8s %-8s %4d:%4d-%-4d\n", pszVarType, "CASE_ID", iRecNum, 1, m_pHeadNode->m_iLenCaseId );
                                int             iCaseLoc=1;
                                m_bCaseIdReady = true; // RHF Feb 15, 2005
                                EnsembledTrip( &CExport::Export_StataDescription, false, &iCaseLoc, &m_iCaseIdItems, &m_aCaseIdOccExpr, &m_aCaseIdOccExprProcSymbol );
                        }
                        if( m_pHeadNode->m_iLenRecId > 0 )
                                _ftprintf( m_pFileSTATAdct, _T("    %-8s %-8s %4d:%4d-%-4d\n"), pszVarType, _T("REC_TYPE"), iRecNum, 1+m_pHeadNode->m_iLenCaseId, m_pHeadNode->m_iLenRecId + m_pHeadNode->m_iLenCaseId);
                }

        m_bCaseIdReady = true;
    }
    // RHF END Apr 30, 2002

        // new record
    if( iLastRecNum != iRecNum ) {
        if( iLastRecNum != -1 )
            _ftprintf( m_pFileSTATAdct, _T("\n") );
        iLastRecNum = iRecNum;
    }

    _tcscpy( pszGenName, csExportedVarName );

    _tcslwr( pszGenName );                               // RHF 15/3/99

    int     iVarLength  = pVarT->GetLength();
    csprochar*   pszVarType;

    if( pVarT->IsNumeric() ) {

        if( pVarT->GetDecimals() && !pVarT->GetDecChar() ) // GHM 20120504 we need to add the decimal character to the output
            iVarLength++;

        // RHF INIC Sep 22, 2005
        if( pVarT->GetDecimals() > 0 || iVarLength >= 10 ) {
            if( iVarLength > 7 )
                pszVarType = _T("double");
            else
                pszVarType = _T("float");
        }
        // RHF END Sep 22, 2005
        /* RHF COM INIC Sep 22, 2005
        if( pVarT->GetDecimals() > 0 )
            pszVarType = "float";
        RHF COM END Sep 22, 2005 */

        else if( iVarLength <= 2 )
            pszVarType = _T("byte");
        else if( iVarLength >= 3 && iVarLength <= 4 )
            pszVarType = _T("int");
        else if( iVarLength >= 5 && iVarLength <= 9 )
            pszVarType = _T("long");
        else
            pszVarType = _T("float");
    }
    else {
        pszVarType = _T("str");

        if( !GetExportForceANSI() )
            iVarLength *= 4;
    }

    _ftprintf( m_pFileSTATAdct, _T("    %-8s %-8s %4d:%4d-%-4d\n"),
             pszVarType, pszGenName, iRecNum, iLoc, iLoc + iVarLength - 1 );
}

// Check if there is a valid missing
bool CExport::stata_hasmissing( VART* pVarT ) {
    if( !pVarT->IsNumeric() )
        return false;

    const   NumericValueProcessor& numeric_value_processor = pVarT->GetCurrentNumericValueProcessor();
    double  dValue = numeric_value_processor.ConvertNumberFromEngineFormat(MISSING);
    bool    bHasMissing = !IsSpecial(dValue);

    // check the value is valid and is not in values-list
    bool    bCanGenerate = ( bHasMissing &&
                             stata_isvalidvalue( dValue ) &&
                             !stata_findvalue( pVarT, dValue ) );

    return bCanGenerate;
}

// Print missing
bool CExport::stata_missing( VART* pVarT ) {
    if( !pVarT->IsNumeric() )
        return false;

    const   NumericValueProcessor& numeric_value_processor = pVarT->GetCurrentNumericValueProcessor();
    double  dValue = numeric_value_processor.ConvertNumberFromEngineFormat(MISSING);
    csprochar    pszValue[_MAXLABLEN+2];

    _stprintf( pszValue, _T("%.0f"), dValue );
    _ftprintf( m_pFileSTATAdo, _T("%6s \"%s\"\n"), pszValue, _T("Missing") );

    return true;
}

bool CExport::stata_hasvaluelabel( VART* pVarT ) {         // RHF 15/3/99
    // stata_hasvaluelabel: the variable has some valid value label
    int     iNumCategories = GetNumVarCategories();

    // first, check for missing value
    const   NumericValueProcessor& numeric_value_processor = pVarT->GetCurrentNumericValueProcessor();
    double  dValue = numeric_value_processor.ConvertNumberFromEngineFormat(MISSING);
    bool    bIsValid = stata_isvalidvalue( dValue );
    bool    bHasAny  = ( bIsValid && !stata_findvalue( pVarT, dValue ) );

    // GHM 20110210 disable printing of labels that are empty
    bool nonBlankLabel = false;

    for( int iValue = 0; !nonBlankLabel && iValue < iNumCategories; iValue++ )
    {
        if( GetVarCategoryAt(iValue)->csTextLabel.GetLength() )
            nonBlankLabel = true;
    }

    if( !nonBlankLabel )
        return false;

    // if not found, check for category-values
    for( int iValue = 0; !bHasAny && iValue < iNumCategories; iValue++ ) {
        CExportVarCateg*    pVarCateg = GetVarCategoryAt( iValue );

        csprochar                pszValue[_MAXLABLEN+2];

        _tcscpy( pszValue, pVarCateg->csTextValue );

        bHasAny = ( stata_isvalidvalue( pszValue ) );
    }

    return bHasAny;
}

bool CExport::stata_findvalue( VART* pVarT, double dValue ) {
    // stata_findvalue: return TRUE if dValue is in the array of categories
    int     iNumCategories = GetNumVarCategories();
    csprochar    pszValue[_MAXLABLEN+2];
    bool    bFound = false;

    for( int iValue = 0; !bFound && iValue < iNumCategories; iValue++ ) {
        CExportVarCateg*    pVarCateg = GetVarCategoryAt( iValue );

        _tcscpy( pszValue, pVarCateg->csTextValue );
        trimleft( pszValue );

        double  dAuxValue = _tstof( pszValue );

        bFound = ( dValue == dAuxValue );
    }

    return bFound;
}

int CExport::stata_countValidValues() {
    int     iNumCategories = GetNumVarCategories();
    int nValidCategories = 0;
    csprochar    pszValue[_MAXLABLEN+2];
    csprochar    pszLabel[_MAXLABLEN+2];
    for( int iValue = 0; iValue < iNumCategories; iValue++ ) {
        CExportVarCateg*    pVarCateg = GetVarCategoryAt( iValue );

        // GHM 20111020 change of export behavior so that values with blank labels aren't counted as valid values
        if( pVarCateg->csTextLabel.GetLength() )
        {
            _tcscpy( pszValue, pVarCateg->csTextValue );
            trimleft( pszValue );

            _tcscpy( pszLabel, pVarCateg->csTextLabel );

            if( stata_isvalidvalue( pszValue ) ) {
                ++nValidCategories;
            }
        }
    }
    return nValidCategories;
}
bool CExport::stata_isvalidvalue( csprochar* pszValue ) {
    trimall( pszValue );

    double  dValue = _tstof( pszValue );
    bool    bIsValid = ( !IsSpecial(dValue) && stata_isvalidvalue( dValue ) );

    return bIsValid;
}

bool CExport::stata_isvalidvalue( double dValue ) {
    // JH 02/07 per Mike's test - negative numbers ok, decimals not ok
   bool    bIsValid = ( /* dValue >= (double) 0 && dValue <= (double) 32767 && */
                         !ValueHasDecimals( dValue ) );

    return bIsValid;
}

//////////////////////////////////////////////////////////////////////////////
//
// --- CSPRO description
//
//////////////////////////////////////////////////////////////////////////////

int CExport::CsProDescr() {
    char*   pExpoNode = GetFirstNode();

    if( pExpoNode == NULL )
        return 0;

    CIntDriver* pIntDriver = m_pEngineDriver->m_pIntDriver.get();
    pIntDriver->PrepareForExportExec( m_pHeadNode->m_iExportProcSymbol, m_pHeadNode->m_iExportProcType );

    int     iLoc=1;

    // Process
    while( *pExpoNode != Exp_EOCASE ) {
        switch( *pExpoNode ) {
            case Exp_ENSEMBLE: //SpsDescr
                pExpoNode += sizeof(EXP_ENSEMBLE_NODE);

                                iLoc = 1 + m_pHeadNode->m_iLenCaseId + m_pHeadNode->m_iLenRecId;
                                m_iLastItemPos = -1;
                                EnsembledTrip( &CExport::Export_CsProDescription, false, &iLoc );

                break;

            case Exp_EOENSEMBLE:
                pExpoNode += 1;
                break;
                        default:
                                ASSERT(0);
                                break;
        }
    }

    return 1;
}

////////////////////////////////////////////////////////////////////////////
//
// --- utility functions
//
////////////////////////////////////////////////////////////////////////////

bool CExport::WasPendingSlash() {
    bool    bWasPending = GetPendingSlash();

    if( bWasPending )
        ResetPendingSlash();

    return bWasPending;
}

// aIndex & aDimFlag are not collapsed (index 0 corresponds to record, 1 to item and 2 to subitem)
// aIndex zero based
void CExport::GenVarName( CString& csVarName, VART* pVarT, int aIndex[DIM_MAXDIM], int aDimFlag[DIM_MAXDIM] ) {
    if( pVarT->IsArray() ) {
        CString csOccName;
        GenOccName( csOccName, m_cNameSeparator, true, pVarT, aIndex, aDimFlag );
        csVarName.Format( _T("%s%s"), pVarT->GetName().c_str(), csOccName.GetString() );
    }
    else {
        csVarName = WS2CS(pVarT->GetName());
    }
}


void CExport::GenOccName( CString& csOccName, csprochar cSeparator, bool bUseFirstSeparator, VART* pVarT, int aIndex[DIM_MAXDIM], int aDimFlag[DIM_MAXDIM] ) {
        csOccName.Empty();

        if( pVarT->IsArray() ) {
                bool    bFirst=true;

                for( int i=0; i < pVarT->GetNumDim(); i++ ) {
                        CDimension::VDimType xDimType=pVarT->GetDimType( i );

                        int iDim = ( xDimType == CDimension::Record ) ?  0:
                                   ( xDimType == CDimension::Item ) ?    1:
                                   ( xDimType == CDimension::SubItem ) ? 2: -1;

                        bool    bExplicit=(aDimFlag[iDim]&CITERATOR_FLAG_EXPLICIT)!=0;
                        bool    bConstant=(aDimFlag[iDim]&CITERATOR_FLAG_CONSTANT)!=0;

                        if( iDim >= 0 && ( !bExplicit || bConstant) ) {
                                CString csThisOcc;
                                int iMaxOccs = pVarT->GetDimSize(i);
                                int iDigitForOccs = IntToStringLength( iMaxOccs );

                                if( !bUseFirstSeparator && bFirst ) {
                                        if( bUseFirstSeparator )
                                                csThisOcc.Format( _T("%0*d"), iDigitForOccs, aIndex[iDim]+1 );
                                        else
                                                csThisOcc = IntToString(aIndex[iDim]+1);
                                        bFirst = false;
                                }
                                else {
                                        if( bUseFirstSeparator )
                                                csThisOcc.Format( _T("%lc%0*d"), cSeparator, iDigitForOccs, aIndex[iDim]+1 );
                                        else
                                                csThisOcc.Format( _T("%lc%d"), cSeparator, aIndex[iDim]+1 );
                                }

                                csOccName += csThisOcc;
                        }
                }
        }
}

void CExport::GetVarLabel( VART* pVarT, TCHAR* pszLabel ) {
    const CDictItem*  pItem = pVarT->GetDictItem();

    _stprintf( pszLabel, _T("%ls"), pItem->GetLabel().GetString() );
}

int CExport::GetVarMaxOccs( VART* pVarT ) {
    GROUPT* pGroupT = pVarT->GetOwnerGPT();

    return pGroupT->GetMaxOccs();
}

bool CExport::ValueHasDecimals( double dValue ) {
    double  dNewValue = floor( dValue + MAGICROUND );
    bool    bHasDecs  = ( dNewValue != dValue );

    return bHasDecs;
}

////////////////////////////////////////////////////////////////////////////
//
// --- array of categories for one variable
//
////////////////////////////////////////////////////////////////////////////

int CExport::BuildVarCategories( VART* pVarT ) {
    // BuildVarCategories: create the array of categories for one variable
    ASSERT( m_papVarCategs == NULL );
    const DictValueSet* pVSet = ( pVarT->GetCurrentValueSet() != nullptr ) ? &pVarT->GetCurrentValueSet()->GetDictValueSet() : nullptr;
    size_t iNumValues = ( pVSet != nullptr ) ? pVSet->GetNumValues() : 0;
    int iNumCategs = 0;

    m_papVarCategs = new std::vector<CExportVarCateg*>;

    for( size_t iValue = 0; iValue < iNumValues; iValue++ ) {
        const DictValue& dict_value = pVSet->GetValue( iValue );
        ASSERT( dict_value.HasValuePairs() ); // must have at least 1 pair

        // only for non-special values (or refused), single-pair, no-range (without 'to' bound)
        const DictValuePair& dict_value_pair = dict_value.GetValuePair(0);
        CIMSAString csFrom = dict_value_pair.GetFrom();

        bool        bIsCategory = (
                                    ( !dict_value.IsSpecial() || dict_value.IsSpecialValue(REFUSED) ) &&
                                    dict_value.GetNumValuePairs() == 1       &&
                                    SO::IsBlank(dict_value_pair.GetTo())
                                  );

        if( bIsCategory ) {
            // normalizes delimiters on value' text
            if( !csFrom.IsNumeric() || ( SO::IsBlank(csFrom) && csFrom.GetLength() > 0 ) ) {
                csFrom.QuoteDelimit();
            }

            // add to array of categories
            CExportVarCateg*    pVarCateg = new CExportVarCateg;

            pVarCateg->csTextValue = csFrom;
            pVarCateg->csTextLabel = dict_value.GetLabel();

            AddVarCategory( pVarCateg );
            iNumCategs++;
        }
    }

    ASSERT( (int)m_papVarCategs->size() == iNumCategs );

    return m_papVarCategs->size();
}

////////////////////////////////////////////////////////////////////////////
//
// --- array of categories for one variable
//
////////////////////////////////////////////////////////////////////////////

int CExport::SASBuildVarCategories( VART* pVarT ) {
    // BuildVarCategories: create the array of categories for one variable
    ASSERT( m_papVarCategs == NULL );
    const CDictItem* pItem = pVarT->GetDictItem();
    const DictValueSet* pVSet = ( pVarT->GetCurrentValueSet() != nullptr ) ? &pVarT->GetCurrentValueSet()->GetDictValueSet() : nullptr;
    size_t iNumValues = ( pVSet != nullptr ) ? pVSet->GetNumValues() : 0;
    int iNumCategs = 0;

    m_papVarCategs = new std::vector<CExportVarCateg*>;

    for( size_t iValue = 0; iValue < iNumValues; iValue++ ) {
        const DictValue& dict_value = pVSet->GetValue( iValue );
        ASSERT( dict_value.HasValuePairs() );       // must have at least 1 pair

        // only for non-special (or refused) values, single-pair, no-range (without 'to' bound)
        const DictValuePair& dict_value_pair = dict_value.GetValuePair(0);
        CIMSAString csFrom = dict_value_pair.GetFrom();

        bool        bIsCategory = (
                                    ( !dict_value.IsSpecial() || dict_value.IsSpecialValue(REFUSED) ) &&
                                    dict_value.GetNumValuePairs() == 1       &&
                                    SO::IsBlank(dict_value_pair.GetTo())    &&
                                    !dict_value.GetLabel().IsEmpty()
                                  );

        if( bIsCategory ) {
            // normalizes delimiters on value' text
            if( !csFrom.IsNumeric() || ( SO::IsBlank(csFrom) && csFrom.GetLength() > 0 ) ) {
                csFrom.QuoteDelimit();
            }

            // add to array of categories
            CExportVarCateg*    pVarCateg = new CExportVarCateg;

            pVarCateg->csTextValue = csFrom;
            if (dict_value.GetLabel().GetLength() < (int)pItem->GetLen()) {
                CString csFill = CString(_T(' '),pItem->GetLen()-dict_value.GetLabel().GetLength());
                pVarCateg->csTextLabel = dict_value.GetLabel() + csFill;
            }
            else {
                pVarCateg->csTextLabel = dict_value.GetLabel();
            }

            AddVarCategory( pVarCateg );
            iNumCategs++;
        }
    }

    ASSERT( (int)m_papVarCategs->size() == iNumCategs );

    return m_papVarCategs->size();
}

bool CExport::AddVarCategory( CExportVarCateg* pVarCateg ) {
    bool bCanAdd = ( m_papVarCategs != NULL );

    if( bCanAdd )
        m_papVarCategs->push_back(pVarCateg);

    return bCanAdd;
}

int CExport::GetNumVarCategories() const
{
    return ( m_papVarCategs != nullptr ) ? m_papVarCategs->size() : 0;
}

CExportVarCateg* CExport::GetVarCategoryAt( int iValue ) {
    CExportVarCateg*    pVarCateg = NULL;

    if( m_papVarCategs != NULL && iValue >= 0 && iValue < (int)m_papVarCategs->size() )
        pVarCateg = m_papVarCategs->at(iValue);

    return pVarCateg;
}

void CExport::DeleteVarCategories()
{
    if( m_papVarCategs != NULL ) {

        for( auto pVarCateg : *m_papVarCategs )
            delete pVarCateg;

        SAFE_DELETE(m_papVarCategs);
    }
}


void CExport::R_Descr() // GHM 20120507
{
    char * pExpoNode = GetFirstNode();

    if( !pExpoNode )
        return;

    CIntDriver* pIntDriver = m_pEngineDriver->m_pIntDriver.get();
    pIntDriver->PrepareForExportExec( m_pHeadNode->m_iExportProcSymbol, m_pHeadNode->m_iExportProcType );

    m_bCaseIdReady = m_pHeadNode->m_iLenCaseId == 0 && m_pHeadNode->m_iLenRecId == 0;

    CString sFullFileName = GetExpoName();
    CString sDataFrame = sFullFileName.Left(sFullFileName.ReverseFind('.'));
    sDataFrame = sDataFrame.Mid(sDataFrame.ReverseFind(PATH_CHAR) + 1);
    sDataFrame.MakeLower();

    _ftprintf(m_pFileR,_T("cspro.factor.type = 1\n"));
    _ftprintf(m_pFileR,_T("cspro.factor.create.new.variable = FALSE\n\n"));
    _ftprintf(m_pFileR,_T("# CSPro Export Factor Options:\n\n"));
    _ftprintf(m_pFileR,_T("#\tcspro.factor.type (0): do not use factors\n"));
    _ftprintf(m_pFileR,_T("#\tcspro.factor.type (1): factor only discrete numeric variables\n"));
    _ftprintf(m_pFileR,_T("#\tcspro.factor.type (2): factor both discrete numeric and alpha variables\n\n"));
    _ftprintf(m_pFileR,_T("#\tcspro.factor.create.new.variable: TRUE to add the factored variables as separate variables\n\n\n"));


    sFullFileName.Replace(_T('\\'),_T('/'));

    _ftprintf(m_pFileR,_T("%s <- read.fortran(\"%s\",c("), sDataFrame.GetString(), sFullFileName.GetString());

    bool bFirstElement = true;

    while( *pExpoNode != Exp_EOCASE )
    {
        switch( *pExpoNode )
        {
        case Exp_ENSEMBLE:
            pExpoNode += sizeof(EXP_ENSEMBLE_NODE);
            EnsembledTrip( &CExport::Export_R_Format, false, &bFirstElement);
            break;

        case Exp_EOENSEMBLE:
            pExpoNode++;
            break;
        }
    }

    _ftprintf(m_pFileR,_T("))\n\n"));


    _ftprintf(m_pFileR,_T("names(%s) <- c("), sDataFrame.GetString());

    pExpoNode = GetFirstNode();
    pIntDriver->PrepareForExportExec( m_pHeadNode->m_iExportProcSymbol, m_pHeadNode->m_iExportProcType );
    m_bCaseIdReady = m_pHeadNode->m_iLenCaseId == 0 && m_pHeadNode->m_iLenRecId == 0;
    bFirstElement = true;

    while( *pExpoNode != Exp_EOCASE )
    {
        switch( *pExpoNode )
        {
        case Exp_ENSEMBLE:
            pExpoNode += sizeof(EXP_ENSEMBLE_NODE);
            EnsembledTrip( &CExport::Export_R_Names, false, &bFirstElement);
            break;

        case Exp_EOENSEMBLE:
            pExpoNode++;
            break;
        }
    }

    _ftprintf(m_pFileR,_T(")\n\n"));


    _ftprintf(m_pFileR,_T("if( cspro.factor.type != 0 ) {\n\n"));

    pExpoNode = GetFirstNode();
    pIntDriver->PrepareForExportExec( m_pHeadNode->m_iExportProcSymbol, m_pHeadNode->m_iExportProcType );
    m_bCaseIdReady = m_pHeadNode->m_iLenCaseId == 0 && m_pHeadNode->m_iLenRecId == 0;

    while( *pExpoNode != Exp_EOCASE )
    {
        switch( *pExpoNode )
        {
        case Exp_ENSEMBLE:
            pExpoNode += sizeof(EXP_ENSEMBLE_NODE);
            EnsembledTrip( &CExport::Export_R_ValueLabels, false, &sDataFrame );
            break;

        case Exp_EOENSEMBLE:
            pExpoNode++;
            break;
        }
    }

    _ftprintf(m_pFileR,_T("}\n\n"));

    _ftprintf(m_pFileR,_T("rm(cspro.factor.type)\n"));
    _ftprintf(m_pFileR,_T("rm(cspro.factor.create.new.variable)\n"));
}


void CExport::CopyQuotedStringWithLengthLimit(TCHAR* pDest, const TCHAR* pSource)
{
    int iLen = _tcslen(pSource);
    strcpymax(pDest, pSource, _MAXLABLEN);

    if( iLen > _MAXLABLEN )
    {
        // if the label or from value was enclosed in single- or double- quotes make sure that the new string ends in that
        if( is_quotemark(pSource[0]) && pSource[0] == pSource[iLen - 1] )
            pDest[_MAXLABLEN - 1] = pDest[0];
    }
}
