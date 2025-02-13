//---------------------------------------------------------------------------
//  File name: Export2.cpp
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
//              12 Jun 00   vc      For automatic output names, the full-path of the input-dict data-file is used
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
#include "Comp.h"
#include "3dException.h" // ExportException
#include <zEngineO/File.h>
#include <zToolsO/Tools.h>
#include <zUtilO/StdioFileUnicode.h>
#include <zDictO/ValueProcessor.h>
#include <ZBRIDGEO/npff.h>


#ifdef _DEBUG
#undef THIS_FILE
static char THIS_FILE[]= __FILE__;
#define new DEBUG_NEW
#endif


const Logic::SymbolTable& CExport::GetSymbolTable() const
{
    return m_engineData->symbol_table;
}


//////////////////////////////////////////////////////////////////////////////
//
// --- export gate & export interface
//
//////////////////////////////////////////////////////////////////////////////

// RHF INIC Feb 17, 2005 NEW_EXPORT
void CExport::MakeMergedArray() {
    CExport*    pExport=this;

    int     iRecNum=0;
    while( pExport != NULL ) {

        // RHF INIC Jun 01, 2005
        if( !pExport->IsExportActive() ) {
            pExport = pExport->GetNextExport();
            continue;
        }
        // RHF END Jun 01, 2005

        int iSymbolNr = pExport->m_aSymbols.GetSize();
        ASSERT( iSymbolNr == pExport->m_aOccExpr.GetSize() );
        ASSERT( iSymbolNr == pExport->m_aOccProcSymbols.GetSize() ); // RHF Jun 08, 2005

        for( int i=0; i < iSymbolNr; i++ ) {
            int iSymVar=pExport->m_aSymbols.GetAt(i);
            int iOccExpr=pExport->m_aOccExpr.GetAt(i);
            int iProcSymbol=pExport->m_aOccProcSymbols.GetAt(i); // RHF Jun 08, 2005

            m_aMergedSymbols.Add( iSymVar );
            m_aMergedOccExpr.Add( iOccExpr );
            m_aMergedOccProcSymbol.Add( iProcSymbol ); // RHF Jun 08, 2005
            m_aMergedRecNumber.Add( iRecNum );
        }

        pExport = pExport->GetNextExport();
        iRecNum++;
    }
}
// RHF END Feb 17, 2005 NEW_EXPORT

void CExport::setSettingsFlags() // rcl, Jan 2005
{
    CSettings* pSettings  = m_pEngineDriver->m_pEngineSettings;
    ASSERT( pSettings != 0 );

    // export options
    SetExportToDat( pSettings->GetExportData() );
    SetExportToSPSS( pSettings->GetExportSPSS() );
    SetExportToSAS( pSettings->GetExportSAS() );
    SetExportToSTATA( pSettings->GetExportSTATA() );
    SetExportToCSPRO( pSettings->GetExportCSPRO() );
    SetExportToTabDelim( pSettings->GetExportTabDelim() );
    SetExportToCommaDelim( pSettings->GetExportCommaDelim() );
    SetExportToSemiColonDelim( pSettings->GetExportSemiColonDelim() );
    SetExportToR( pSettings->GetExportR() );

    SetExportItemOnly( pSettings->GetExportItemOnly() );
    SetExportSubItemOnly( pSettings->GetExportSubItemOnly() );
    SetExportItemSubItem( pSettings->GetExportItemSubItem() );

    SetExportForceANSI(pSettings->GetExportForceANSI()); // GHM 20120416
    SetExportCommaDecimal(pSettings->GetExportCommaDecimal());
}

bool CExport::exportFlagsAreOk( bool* pbAllowDuplicates ) // rcl, Jan 2005
{
    CSettings*  pSettings  = m_pEngineDriver->m_pEngineSettings;
    ASSERT( pSettings != 0 );
    bool bFlagsOk = true;

    // Check Models: CsPro/TabDelim/CommaDelim/SemiColonDelim must appear alone
    bool bCsProOnly          = false;

    // RHF INIC Feb 15, 2005 NEW_EXPORT
    bool    bSpssOnly=false;
    bool    bSasOnly=false;
    bool    bStataOnly=false;
    // RHF END Feb 15, 2005 NEW_EXPORT

    bool bTabDelimOnly       = false;
    bool bCommaDelimOnly     = false;
    bool bSemiColonDelimOnly = false;
    /*
    bool    bSomeExport=false;

      if( pSettings->GetExportCSPRO() || pSettings->GetExportSPSS() ||
      pSettings->GetExportSAS() ||
      pSettings->GetExportSTATA() ||
      pSettings->GetExportTabDelim() ||
      pSettings->GetExportCommaDelim() ||
      pSettings->GetExportSemiColonDelim()
      )
      bSomeExport = true;
    */

    if( pSettings->GetExportCSPRO() && !pSettings->GetExportSPSS()&&
        !pSettings->GetExportSAS()&&
        !pSettings->GetExportSTATA() &&
        !pSettings->GetExportTabDelim()&&
        !pSettings->GetExportCommaDelim()&&
        !pSettings->GetExportSemiColonDelim()
        )
        bCsProOnly = true;

    // RHF INIC Feb 15, 2005 NEW_EXPORT
    if( !pSettings->GetExportCSPRO() && pSettings->GetExportSPSS()&&
        !pSettings->GetExportSAS()&&
        !pSettings->GetExportSTATA() &&
        !pSettings->GetExportTabDelim()&&
        !pSettings->GetExportCommaDelim()&&
        !pSettings->GetExportSemiColonDelim()
        )
        bSpssOnly = true;

    if( !pSettings->GetExportCSPRO() && !pSettings->GetExportSPSS()&&
        pSettings->GetExportSAS()&&
        !pSettings->GetExportSTATA() &&
        !pSettings->GetExportTabDelim()&&
        !pSettings->GetExportCommaDelim()&&
        !pSettings->GetExportSemiColonDelim()
        )
        bSasOnly = true;

    if( !pSettings->GetExportCSPRO() && !pSettings->GetExportSPSS()&&
        !pSettings->GetExportSAS()&&
        pSettings->GetExportSTATA() &&
        !pSettings->GetExportTabDelim()&&
        !pSettings->GetExportCommaDelim()&&
        !pSettings->GetExportSemiColonDelim()
        )
        bStataOnly = true;
    // RHF END Feb 15, 2005 NEW_EXPORT

    if( pSettings->GetExportTabDelim() && !pSettings->GetExportSPSS()&&
        !pSettings->GetExportSAS()&&
        !pSettings->GetExportSTATA() &&
        !pSettings->GetExportCSPRO()&&
        !pSettings->GetExportCommaDelim()&&
        !pSettings->GetExportSemiColonDelim()
        )
        bTabDelimOnly = true;

    if( pSettings->GetExportCommaDelim() && !pSettings->GetExportSPSS()&&
        !pSettings->GetExportSAS()&&
        !pSettings->GetExportSTATA() &&
        !pSettings->GetExportTabDelim()&&
        !pSettings->GetExportCSPRO()&&
        !pSettings->GetExportSemiColonDelim()
        )
        bCommaDelimOnly = true;

    if( pSettings->GetExportSemiColonDelim() && !pSettings->GetExportSPSS()&&
        !pSettings->GetExportSAS()&&
        !pSettings->GetExportSTATA() &&
        !pSettings->GetExportTabDelim()&&
        !pSettings->GetExportCommaDelim()&&
        !pSettings->GetExportCSPRO()
        )
        bSemiColonDelimOnly = true;

    if( // RHF COM Aug 03, 2006 pSettings->GetExportCSPRO() && !bCsProOnly ||
        pSettings->GetExportTabDelim() && !bTabDelimOnly ||
        pSettings->GetExportCommaDelim() && !bCommaDelimOnly ||
        pSettings->GetExportSemiColonDelim() && !bSemiColonDelimOnly )
        bFlagsOk = false;

    ASSERT( pbAllowDuplicates != 0 );
    // RHF INIC Feb 15, 2005
    *pbAllowDuplicates = bCsProOnly || bSpssOnly || bSasOnly || bStataOnly;
    // RHF END Feb 15, 2005

    return bFlagsOk;
}

// Handle exception conditions in ExportOpen()
const int NO_VERBOSE_CODE = 0;
const int COULD_NOT_OPEN_FILE_CODE = 31073;

bool CExport::ExportOpen( bool* bDeleteWhenFail ) {

    bool bExportOk = true;

    ASSERT( bDeleteWhenFail != 0 );
    *bDeleteWhenFail = true;

    // hint: compiler must be completed prior to call this method ('m_has_Export' needed)
    if( IsExportAlive() )               // check export gate - must be "not yet open"
        return false;

    CSettings*  pSettings  = m_pEngineDriver->m_pEngineSettings;
    CString csFilename;
    bool        bHasExport = pSettings->m_bHasExport;
    CNPifFile*  pPifFile   = m_pEngineDriver->GetPifFile();

    if( !bHasExport )
        return false;

    Encoding fileEncodingBasedOnFlag = pSettings->GetExportForceANSI() ? Encoding::Ansi : Encoding::Utf8; // GHM 20120428

    try
    {
        bool bAllowDuplicatedFiles = false;
        bool bInitialError = !exportFlagsAreOk( &bAllowDuplicatedFiles );

        bool    bIsFileDuplicated   = false;
        int     iMaxDuplicatedLevel = 0;
        CArray<CExport*,CExport*> aDuplicatedExport;
        CIMSAString csCsProRecordName;
        int     iMaxRecOcc = 0;
        int     iDummy     = 0;

        // export-file-name
        if( GetExpoName().IsEmpty() ) {
            bool bAutoName = true;       // the name will be automatically given // victor Jun 12, 01

            // named export (multi-exports)                 // victor Dec 18, 00
            int iNumExports = m_pEngineArea->ExportGetSize();

            if( m_iFileSymbol < 0 ) {
                // assigning automatic name 'EXPO_nnn'
                csFilename.Format( _T("Expo_%03d"), m_pEngineArea->NewExpoSeqNo() );
            }
            else {
                bAutoName = false;                          // victor Jun 12, 01

                // retrieve the current value of the name-expression
                LogicFile& logic_file = GetSymbolLogicFile(m_iFileSymbol);
                csFilename = WS2CS(SO::TrimRight(logic_file.GetFilename()));

                // close the file so that it isn't locked when opened below
                logic_file.Close();
            }
            // when auto-name given                 <begin> // victor Jun 12, 01
            if( bAutoName ) {
                ASSERT(false);
                csFilename = PortableFunctions::PathGetDirectory<CString>(_T("pDicX_DatFileName")) + PortableFunctions::PathGetFilename(csFilename);
            }
            // when auto-name given                 <end>   // victor Jun 12, 01

            // store the final name
            csFilename.MakeUpper();

            if (pPifFile->GetExportFilenames().empty()) // Chirag Mar 13, 2002
                SetExpoName( csFilename, true );

            else {
                SetExpoName( pPifFile->GetExportFilenames().front(), false );
            }

            // checking for duplicate names
            int iSameName = -1;

            for( int iExpoSlot = 0; iExpoSlot < iNumExports; iExpoSlot++ ) {
                CExport*    pExport = m_pEngineArea->ExportGetAt( iExpoSlot );

                if( !pExport->IsExportActive() )
                    continue;

                if( pExport != this && _tcscmp( GetExpoName(), pExport->GetExpoName() ) == 0 ) {
                    if( iSameName == -1 )
                        iSameName = iExpoSlot;

            // RHF INIC Feb 15, 2005 NEW_EXPORT
                int     iSize=aDuplicatedExport.GetSize();
                if( iSize > 0 ) {
                    CExport*    pLastDuplicatedExport=aDuplicatedExport.GetAt(iSize-1);
                    pLastDuplicatedExport->SetNextExport( pExport );
                }
                pExport->SetNextExport( this );
            // RHF END Feb 15, 2005 NEW_EXPORT

                    aDuplicatedExport.Add( pExport );

                // RHF INIC Feb 17, 2005 NEW_EXPORT
                pExport->SetRecordNumber( aDuplicatedExport.GetSize()-1 );
                this->SetRecordNumber( aDuplicatedExport.GetSize() );
                // RHF END Feb 17, 2005 NEW_EXPORT

                    iMaxDuplicatedLevel = std::max( iMaxDuplicatedLevel, (int) pExport->m_pHeadNode->m_iExportLevel );
                }
            }

            if( iSameName >= 0 ) {
                bIsFileDuplicated = true;
                if( bAllowDuplicatedFiles ) // Only CsPro allows file name duplication (multi record export)
                    ;
                else {
                    issaerror( MessageType::Warning, 31072, GetExpoName().GetString(), iSameName + 1 );

                    SetExportUnable();          // export becomes "unable to open"
                    *bDeleteWhenFail = false;
                    return false;
                }
            }
        }

        // export options
        setSettingsFlags();

        // RHF INIC Feb 10, 2005 Moved here
        if( bInitialError ) {
            issaerror( MessageType::Warning, 31079 );
            //ExportClose();
            return false;
        }
        // RHF END Feb 10, 2005

        SetExportActive();                  // export becomes "normally open"

        int iItem;

        //Subitems must be in order & together when exporting to CsPro
        if( GetExportToCSPRO() ) {
            int        iLastParentItem=-1;
            int        iLastSubItemSymbol=-1;
            CMap<int,int,int,int>  aItemUsed;
            for( iItem = 0; iItem < m_aSymbols.GetSize(); iItem++ ) {
                int   iSym  = m_aSymbols.GetAt(iItem);
                VART* pVarT = VPT(iSym);
                int   iParentItem = pVarT->GetOwnerSymItem();
                bool  bIsSubItem  = ( iParentItem > 0 );
                if( bIsSubItem ) {
                    bool bError = false;
                    if( iLastSubItemSymbol == -1 && iLastParentItem == -1 ) {
                        if( aItemUsed.Lookup( iParentItem, iParentItem ) )  // already used
                            bError = true;
                    }
                    else if( iSym < iLastSubItemSymbol || iParentItem != iLastParentItem ) // This subitem is before the previous one or it has other parent
                        bError = true;

                    if( bError ) {
                        issaerror( MessageType::Warning, 31050 );
                        SetExportUnable();          // export becomes "unable to open"
                        return false;
                    }

                    iLastParentItem = iParentItem;
                    iLastSubItemSymbol = iSym;
                    aItemUsed.SetAt( iParentItem, iParentItem );
                }
                else {
                    iLastParentItem = -1;
                    iLastSubItemSymbol = -1;
                }
            }
        }


        //Adjust Export type (ItemOnly,SubItemOnly,ItemSubItem)
        //int                    iLastSymItem=-1;
        CArray<int,int>     aSymbolsAux;
        CArray<int,int>     aOccExprAux;
        CArray<int,int>     aOccProcSymbolsAux; // RHF Jun 08, 2005
        CArray<CString,CString> aOccExprStringAux;

        for( iItem = 0; iItem < m_aSymbols.GetSize(); iItem++ ) {
            int     iSym      = m_aSymbols.GetAt(iItem);
            int     iOccExpr  = m_aOccExpr.GetAt(iItem);
            int     iProcSymbol=m_aOccProcSymbols.GetAt(iItem); // RHF Jun 08, 2005
            CString csOccExpr = m_aOccExprString.GetAt(iItem);

            VART*   pVarT = VPT(iSym);

            int  iParentItem = pVarT->GetOwnerSymItem();
            bool bIsSubItem = ( iParentItem > 0 );
            bool bIsItem = !bIsSubItem;
            bool bHasSubItems = bIsItem && ( pVarT->GetNextSubItem() != NULL );
            //Check SubItems
            if( ( GetExportItemOnly() || GetExportItemSubItem() ) && bIsSubItem ) {
                //Check if parent item is already in the list
                bool    bParentItemInList = false;
                CString csParentName = WS2CS(NPT(iParentItem)->GetName());

                if( m_aItemNames.Lookup( csParentName, iDummy ) )
                    bParentItemInList = true;

                if( !bParentItemInList ) {
                    m_aItemNames.SetAt( csParentName, 0 );
                    aSymbolsAux.Add( iParentItem );
                    aOccProcSymbolsAux.Add( iProcSymbol ); // RHF Jun 08, 2005

                    aOccExprAux.Add( iOccExpr );   // Use the same MVAR node asociated to the subitem.
                    aOccExprStringAux.Add( csOccExpr );
                }

                if( GetExportItemSubItem() ) {
                    aSymbolsAux.Add( iSym );
                    aOccProcSymbolsAux.Add( iProcSymbol ); // RHF Jun 08, 2005

                    aOccExprAux.Add( iOccExpr );
                    aOccExprStringAux.Add( csOccExpr );
                }
            }
            else if( GetExportItemOnly() && bIsItem ) {
                aSymbolsAux.Add( iSym );
                aOccProcSymbolsAux.Add( iProcSymbol ); // RHF Jun 08, 2005
                aOccExprAux.Add( iOccExpr );
                aOccExprStringAux.Add( csOccExpr );
            }
            else if( GetExportSubItemOnly() && bIsSubItem ) {
                aSymbolsAux.Add( iSym );
                aOccProcSymbolsAux.Add( iProcSymbol ); // RHF Jun 08, 2005
                aOccExprAux.Add( iOccExpr );
                aOccExprStringAux.Add( csOccExpr );
                //transform subitem to items: See Export_CsProDescription
            }

            //Check Items
            else if( ( GetExportSubItemOnly() || GetExportItemSubItem() ) && bIsItem ) {
                // The item must be in the list
                if( GetExportItemSubItem() || !bHasSubItems ) {
                    aSymbolsAux.Add( iSym );
                    aOccProcSymbolsAux.Add( iProcSymbol ); // RHF Jun 08, 2005
                    aOccExprAux.Add( iOccExpr );
                    aOccExprStringAux.Add( csOccExpr );
                }

                //Check if some subitem is already in the list
                bool bSomeSubItemInList = false;

                for( int j = 0; !bSomeSubItemInList && j < m_aSymbols.GetSize(); j++ ) {
                    int   iSymAux = m_aSymbols.GetAt(j);
                    VART* pVarTAux = VPT(iSymAux);


                    if( pVarTAux->GetOwnerSymItem() == iSym )
                    {
                        bSomeSubItemInList = true;
                    }
                }

                //There is no subitem in the list, add all subitems
                if( !bSomeSubItemInList ) {
                    VART* pSubItem = pVarT->GetNextSubItem();


                    while( pSubItem != NULL ) {
                        int    iSymSubItem = pSubItem->GetSymbolIndex();
                        CString csSubItemName = WS2CS(NPT(iSymSubItem)->GetName());


                        //ASSERT( !m_aItemNames.Lookup(csSubItemName,iDummy) );
                        m_aItemNames.SetAt( csSubItemName, 0 );

                        aSymbolsAux.Add(iSymSubItem);
                        aOccProcSymbolsAux.Add( iProcSymbol ); // RHF Jun 08, 2005

                        if( pSubItem->GetMaxOccs() >= 2 ) {
                            int     iMultSubItemOccExpr = iOccExpr; ///Better to use the parent occurrence if by any chance is not found
                            CString csMultSubItemOccExpr = csOccExpr;

                            if( !m_aOcExprForMultSubItems.Lookup( iSymSubItem, iMultSubItemOccExpr ) )
                                ASSERT(0); //Must exist

                            if( !m_aOcExprStringForMultSubItems.Lookup( iSymSubItem, csMultSubItemOccExpr ) )
                                ASSERT(0); //Must exist

                            aOccExprAux.Add( iMultSubItemOccExpr );
                            aOccExprStringAux.Add( csMultSubItemOccExpr );
                        }
                        else {
                            aOccExprAux.Add( iOccExpr );  // Use the same MVAR_NODE asociated to the item. No new dimension added
                            aOccExprStringAux.Add( csOccExpr );
                        }

                        pSubItem = pSubItem->GetNextSubItem();
                    }
                }
                else {  //Some subitems in the list
                }
            }

            /*
            if( aDeletedSymbols.Lookup(iSym,iSym) ) {
            // Add all subitems
            if( GetExportSubItemOnly() && bIsItem ) {

            }
            continue;
            }

            //For each subitem, add parent item to the list before the first subitem
            if( GetExportItemSubItem() ) { // && GetExportToCSPRO() )
            if( bIsSubItem ) {
            if( iLastSymItem == iParentItem ) // already in the list
            ;
            else {
            aSymbolsAux.Add(iParentItem);
            iLastSymItem = iParentItem;
            }
            }
            else
            iLastSymItem = iSym;
            }

            aSymbolsAux.Add(iSym);
            */
        }

        //check for duplicated names after the changes and refresh item names map
        m_aItemNames.RemoveAll();
        bool bError = false;

        for( iItem = 0; !bError && iItem < aSymbolsAux.GetSize(); iItem++ ) {
            int iSym = aSymbolsAux.GetAt(iItem);
            CString csName = WS2CS(NPT(iSym)->GetName());
            CString& csOcc=aOccExprStringAux.ElementAt(iItem);

            CString csFullName=csName+csOcc;

            if( m_aItemNames.Lookup( csFullName, iDummy ) )
                bError = true;
            else
                m_aItemNames.SetAt( csFullName, iDummy );
        }

        if( !bError ) {
            for( iItem=0; !bError && iItem < m_pHeadNode->m_iNumCaseId; iItem++ ) {
                CString csName = WS2CS(NPT(m_iCaseIdItems[iItem])->GetName());

                // RHF INIC Feb 16, 2005 NEW_EXPORT
                CString& csOcc=m_aCaseIdOccExprString[iItem];

                csName += csOcc;
                // RHF END Feb 16, 2005 NEW_EXPORT

                if( m_aItemNames.Lookup( csName, iDummy ) )
                    bError = true;
                else
                    m_aItemNames.SetAt( csName, iDummy );
            }
        }

        if( !bError ) {
            // Add to the master array
            m_aSymbols.RemoveAll();
            m_aSymbols.Append( aSymbolsAux );

            // RHF INIC Jun 08, 2005
            m_aOccProcSymbols.RemoveAll();
            m_aOccProcSymbols.Append( aOccProcSymbolsAux );
            // RHF END Jun 08, 2005

            m_aOccExpr.RemoveAll();
            m_aOccExpr.Append( aOccExprAux );

            m_aOccExprString.RemoveAll();
            m_aOccExprString.Append( aOccExprStringAux );
        }

        //Gen default case id
        if( !bError &&  m_pHeadNode->m_iLenCaseId == 0 ) {
            bool    bCaseIdOk = true;

            if( GetExportToCSPRO() ) {
                //int        iUntilLevel=max(iMaxDuplicatedLevel,1);
                //iUntilLevel=max(iUntilLevel,m_pHeadNode->m_iExportLevel);

                int iUntilLevel = DIP(0)->GetMaxLevel();

                bCaseIdOk = MakeDefaultCaseId(iUntilLevel);

                m_pHeadNode->m_bHasCaseId = true;
            }
            else {
                if( m_pHeadNode->m_bHasCaseId )
                    // bCaseIdOk = MakeDefaultCaseId(m_pHeadNode->m_iExportLevel);
                    bCaseIdOk = MakeDefaultCaseId(DIP(0)->GetMaxLevel());
            }

            if( !bCaseIdOk )
                bError = true;
        }

        if( bError )
            throw ExportException( 31055 );

        // RHF INIC Feb 15, 2005
        m_iCaseIdItems.SetSize( m_pHeadNode->m_iNumCaseId );
        m_aCaseIdOccExpr.SetSize( m_pHeadNode->m_iNumCaseId );
        // RHF END Feb 15, 2005

        m_aCaseIdOccExprProcSymbol.SetSize( m_pHeadNode->m_iNumCaseId ); // RHF Jun 08, 2005

        //Check CsPro export
        if( GetExportToCSPRO() ) {
            GetRecNameAndOcc( csCsProRecordName, iMaxRecOcc );

        // RHF INIC Feb 03, 2005
            if( !SO::IsBlank(csCsProRecordName) && !csCsProRecordName.IsName() ) {
                issaerror( MessageType::Warning, 31058, csCsProRecordName.GetString() );
                ExportClose();
                return false;
            }
            csCsProRecordName.MakeUpper();
            // RHF END Feb 03, 2005
        }

        if( bIsFileDuplicated ) {
              for( int iDup=0; !bError && iDup < aDuplicatedExport.GetSize(); iDup++ ) {
                    CExport* pDuplicatedExport = aDuplicatedExport.GetAt(iDup);

                    //Check the order: First all level 1 records are exported, then all level 2 records and the all level 3 records
                    // If they are in diferent levels, all the export until level-1 must be in PreProc.
                    if( pDuplicatedExport->m_pHeadNode->m_iExportLevel != m_pHeadNode->m_iExportLevel ) {
                        if( pDuplicatedExport->m_pHeadNode->m_iExportLevel < m_pHeadNode->m_iExportLevel ) {
                            if( pDuplicatedExport->m_pHeadNode->m_iExportProcType != PROCTYPE_PRE ) // the previous one is going to be executed after this export
                                bError = true;
                        }
                        else if( m_pHeadNode->m_iExportProcType != PROCTYPE_PRE )
                            bError = true;

                        if( bError ) {
                            issaerror( MessageType::Warning, 31080 );
                            continue;
                        }
                    }

                    /*
                    if( pDuplicatedExport->m_pHeadNode->m_iExportLevel > m_pHeadNode->m_iExportLevel ) {
                    issaerror( MessageType::Warning, 31080 );
                    bError = true;
                    continue;
                    }
                    */

                    bool    bSameLevel=(pDuplicatedExport->m_pHeadNode->m_iExportLevel == m_pHeadNode->m_iExportLevel);

            bSameLevel = true; // RHF Feb 21, 2005 Assume they have the same level.

                    // case id must be the same
                    if( bSameLevel && pDuplicatedExport->m_pHeadNode->m_iNumCaseId != m_pHeadNode->m_iNumCaseId )
                        bError = true;
                    else if( bSameLevel && pDuplicatedExport->m_pHeadNode->m_iLenCaseId != m_pHeadNode->m_iLenCaseId )
                        bError = true;
                    else {
                        // The common items must be the same
                        int        iCommonCaseIds= std::min(pDuplicatedExport->m_pHeadNode->m_iNumCaseId,m_pHeadNode->m_iNumCaseId);
                        for( int i=0; !bError && i < iCommonCaseIds; i++ ) {
                            if( pDuplicatedExport->m_iCaseIdItems[i] != m_iCaseIdItems[i] )
                                bError = true;
                        }
                    }

                    if( bError ) {
                        issaerror( MessageType::Warning, 31075 );
                        continue;
                    }

            if( *m_pszSectionCode == 0 && HasRecType( m_pHeadNode ) ) // RHF Feb 15, 2005 Add HasRecType( m_pHeadNode )
                        ExportRecType(false);

                    // record type length must be the same
                    if( pDuplicatedExport->m_pHeadNode->m_iLenRecId != m_pHeadNode->m_iLenRecId ) {
                        issaerror( MessageType::Warning, 31076 );
                        bError = true;
                    }


                    // record type must be different
                    else if( *m_pszSectionCode != 0 && _tcscmp( m_pszSectionCode, pDuplicatedExport->m_pszSectionCode) == 0 ) {
                        issaerror( MessageType::Warning, 31056 );
                        bError = true;
                    }

                    // Export model must be the same
                    else if( pDuplicatedExport->m_pHeadNode->m_iExportModel != m_pHeadNode->m_iExportModel ) {
                        issaerror( MessageType::Warning, 31077 );
                        bError = true;
                    }

                    // rec_name must be different
                    else if( !csCsProRecordName.IsEmpty() && csCsProRecordName.Compare(pDuplicatedExport->m_pRecordCSPRO->GetName()) == 0 ) {
                        issaerror( MessageType::Warning, 31078 );
                        bError = true;
                    }

                    //Export type(ItemOnly,SubItemOnly,ItemSubItem) must be the same
                    else if( pDuplicatedExport->GetExportItemOnly() != GetExportItemOnly() ||
                        pDuplicatedExport->GetExportSubItemOnly() != GetExportSubItemOnly() ||
                        pDuplicatedExport->GetExportItemSubItem() != GetExportItemSubItem() ) {
                            issaerror( MessageType::Warning, 31082 );
                            bError = true;
                        }

                        // CheckDuplicated item names
                    else {
                        CMap<CString,LPCTSTR,int,int> aListItems;

                        for( iItem = 0; !bError && iItem < pDuplicatedExport->m_aSymbols.GetSize(); iItem++ ) {
                            int iSym = pDuplicatedExport->m_aSymbols.GetAt(iItem);
                            CString csName = WS2CS(NPT(iSym)->GetName());

                            if( aListItems.Lookup( csName, iDummy ) ) {
                                issaerror( MessageType::Warning, 31081, csName.GetString() );
                                bError = true;
                            }
                            aListItems.SetAt( csName, iDummy );
                        }

                        for( iItem = 0; !bError && iItem < m_aSymbols.GetSize(); iItem++ ) {
                            int iSym = m_aSymbols.GetAt(iItem);
                            CString csName = WS2CS(NPT(iSym)->GetName());

                            if( aListItems.Lookup( csName, iDummy ) ) {
                                issaerror( MessageType::Warning, 31081, csName.GetString() );
                                bError = true;
                            }
                            aListItems.SetAt( csName, iDummy );
                        }
                    }

                    //Check the order: First all level 1 records are exported, then all level 2 records and the all level 3 records
                    if( !bError && pDuplicatedExport->m_pHeadNode->m_iExportLevel > m_pHeadNode->m_iExportLevel ) {
                        issaerror( MessageType::Warning, 31080 );
                        bError = true;
                    }
                }//End for iDup

              if( bError )
                  throw ExportException( NO_VERBOSE_CODE );
        } // bIsFileDuplicated

        // exported data-file
        if( GetExportToDat() ) {
            if( bIsFileDuplicated ) { // The data file was open in a previous export
                CExport*    pDuplicatedExport=aDuplicatedExport.GetAt(0);

                m_pFileDat = pDuplicatedExport->m_pFileDat;
                m_bNeedClose = false;

                if( m_pFileDat == NULL )
                    throw ExportException( NO_VERBOSE_CODE );
            }
            else {
                //SAVY .DAT is used anyway if the file name is empty
                if (!pPifFile->GetExportFilenames().empty())
                    csFilename = pPifFile->GetExportFilenames().front();// Chirag Mar 13, 2002

                if( GetExportToSTATA() && !GetExportForceANSI() )
                    m_pFileDat = CStdioFileUnicode::CreateFileWithoutBOM(csFilename,fileEncodingBasedOnFlag);

                else
                    m_pFileDat = CStdioFileUnicode::_tfopen(csFilename,_T("w"),fileEncodingBasedOnFlag);

                if( m_pFileDat == NULL )
                    throw ExportException( COULD_NOT_OPEN_FILE_CODE );
            }
        }

        // SPSS codebook file
        if( GetExportToSPSS() ) {
            if (pPifFile->GetSPSSSyntaxFName().IsEmpty())  // Chirag Mar 13, 2002
                csFilename = PortableFunctions::PathRemoveFileExtension<CString>(GetExpoName()) + FileExtensions::WithDot::SpssSyntax;
            else                                           // Chirag Mar 13, 2002
                csFilename = pPifFile->GetSPSSSyntaxFName(); // Chirag Mar 13, 2002

            // RHF INIC Feb 17, 2005 NEW_EXPORT
            if( bIsFileDuplicated ) {
                CExport*    pDuplicatedExport=aDuplicatedExport.GetAt(0);
                m_pFileSPSS = pDuplicatedExport->m_pFileSPSS;
            }
            else
                // RHF END Feb 17, 2005 NEW_EXPORT
            m_pFileSPSS = CStdioFileUnicode::_tfopen(csFilename,_T("w"),fileEncodingBasedOnFlag);

            if( m_pFileSPSS == NULL )
                throw ExportException( COULD_NOT_OPEN_FILE_CODE );
        }

        // SAS codebook file
        if( GetExportToSAS() ) {
            if (pPifFile->GetSASSyntaxFName().IsEmpty())  // Chirag Mar 13, 2002
                csFilename = PortableFunctions::PathRemoveFileExtension<CString>(GetExpoName()) + FileExtensions::WithDot::SasSyntax;
            else                                                 // Chirag Mar 13, 2002
                csFilename = pPifFile->GetSASSyntaxFName(); // Chirag Mar 13, 2002

            m_pFileSAS = CStdioFileUnicode::_tfopen(csFilename,_T("w"),fileEncodingBasedOnFlag);

            if( m_pFileSAS == NULL )
                throw ExportException( COULD_NOT_OPEN_FILE_CODE );
        }

        // STATA (DCT & DO) files
        if( GetExportToSTATA() ) {
            // DCT file
            if (pPifFile->GetSTATASyntaxFName().IsEmpty())  // Chirag Mar 13, 2002
                csFilename = PortableFunctions::PathRemoveFileExtension<CString>(GetExpoName()) + FileExtensions::WithDot::StataDictionary;
            else                                       // Chirag Mar 13, 2002
                 csFilename = pPifFile->GetSTATASyntaxFName(); // Chirag Mar 13, 2002

            m_pFileSTATAdct = CStdioFileUnicode::CreateFileWithoutBOM(csFilename,fileEncodingBasedOnFlag);

            if( m_pFileSTATAdct == NULL )
                throw ExportException( COULD_NOT_OPEN_FILE_CODE );

            // DO file
            if (pPifFile->GetSTATADOFName().IsEmpty())  // Chirag Mar 13, 2002
                csFilename = PortableFunctions::PathRemoveFileExtension<CString>(GetExpoName()) + FileExtensions::WithDot::StataDo;
            else                                        // Chirag Mar 13, 2002
                csFilename = pPifFile->GetSTATADOFName();// Chirag Mar 13, 2002

            m_pFileSTATAdo = CStdioFileUnicode::CreateFileWithoutBOM(csFilename,fileEncodingBasedOnFlag);

            if( m_pFileSTATAdo == NULL )
                throw ExportException( COULD_NOT_OPEN_FILE_CODE );
        }

        // R codebook file
        if( GetExportToR() ) // GHM 20120507
        {
            if( pPifFile->GetRSyntaxFName().IsEmpty() )
                csFilename = PortableFunctions::PathRemoveFileExtension<CString>(GetExpoName()) + FileExtensions::WithDot::RSyntax;
            else
                csFilename = pPifFile->GetRSyntaxFName();

            m_pFileR = CStdioFileUnicode::_tfopen(csFilename,_T("w"),fileEncodingBasedOnFlag);

            if( m_pFileR == NULL )
                throw ExportException(COULD_NOT_OPEN_FILE_CODE);
        }

        // CSPRO DataDict
        if( GetExportToCSPRO() ) {
            if (pPifFile->GetCSPROSyntaxFName().IsEmpty())  // Chirag Mar 13, 2002
                csFilename = PortableFunctions::PathRemoveFileExtension<CString>(GetExpoName()) + FileExtensions::WithDot::Dictionary;
            else                                           // Chirag Mar 13, 2002
                csFilename = pPifFile->GetCSPROSyntaxFName(); // Chirag Mar 13, 2002

            ASSERT( m_pRecordCSPRO == NULL );
            try {
                if( bIsFileDuplicated ) { // The file was open/close in a previous export
                    m_pRecordCSPRO = new CDictRecord;
                }
                else {
                    FILE* fp = PortableFunctions::FileOpen(csFilename, _T("w"));
                    if( fp != NULL ) {
                        fclose(fp);
                        m_pRecordCSPRO = new CDictRecord;
                    }
                }
            }
            catch( ... ) {
            }

            if( m_pRecordCSPRO == NULL )
                throw ExportException( COULD_NOT_OPEN_FILE_CODE );

            m_pRecordCSPRO->SetName( csCsProRecordName );

            // GHM 20120514 use the actual record name from input dictionary
            if( SO::IsBlank(csCsProRecordName) && m_aSymbols.GetSize() )
            {
                const CDictRecord* pRec1 = VPT(m_aSymbols[0])->GetDictItem()->GetRecord();
                const CDictRecord* pRec2 = VPT(m_aSymbols[m_aSymbols.GetSize() - 1])->GetDictItem()->GetRecord();

                if( pRec1 == pRec2 ) // only do this if they're exporting symbols from one record (not joining singly and multiply occurring records)
                {
                    m_pRecordCSPRO->SetName(pRec1->GetName());
                    m_pRecordCSPRO->SetLabel(pRec1->GetLabel());
                }
            }

            m_pRecordCSPRO->SetMaxRecs( iMaxRecOcc );
        }

        if( !CreateRecArea() )
            throw ExportException( 31074 );

    }
    catch( ExportException& e )
    {
        switch( e.getErrorCode() )
        {
        default:
        case 0: break; // 0 means do not output any message, just close Export later
        case COULD_NOT_OPEN_FILE_CODE:
                    issaerror( MessageType::Warning, e.getErrorCode(), csFilename.GetString() );
                    break;

        case 31055: ; // fall through to show message
        case 31074: issaerror( MessageType::Warning, e.getErrorCode() ); break;
        }

        ExportClose();

        bExportOk = false;
    }

    return bExportOk;
}

//////////////////////////////////////////////////////////////////////////////
//
// --- record-area
//
//////////////////////////////////////////////////////////////////////////////

bool CExport::CreateRecArea( void ) {
    // ... formerly 'exprecalloc', now 'CreateRecArea'
    EXP_HEADER_NODE*    pHeadNode = GetHeadNode();
    char*               pExpoNode = GetFirstNode();

    if( pExpoNode == NULL )
        return false;

    // 1.- measure memory needed for output record-area
    int     iMaxLen    = 0;
    int     iRecLen    = 0;

    EXP_ENSEMBLE_NODE*  pEnsembNode;                    // victor Dec 12, 00

    while( *pExpoNode != Exp_EOCASE ) {
        switch( *pExpoNode ) {
        case Exp_ENSEMBLE: //CreateRecArea
            {
                pEnsembNode = (EXP_ENSEMBLE_NODE*) pExpoNode;
                pExpoNode += sizeof(EXP_ENSEMBLE_NODE);

                bool    bUseSeparator=UseSeparator();
                iRecLen = EnsembledTrip( &CExport::Export_GetRecordLen, false, &bUseSeparator );// RHF Oct 08, 2004 False

                CMap<int,int,int,int>    aMapUserRecords;
                EnsembledTrip( &CExport::Export_GenRecordList, false, &aMapUserRecords );

                POSITION pos = aMapUserRecords.GetStartPosition();
                while( pos ) {
                    int        iSec;

                    aMapUserRecords.GetNextAssoc( pos, iSec, iSec );

                    m_aUsedRecords.Add( iSec );
                }
            }
            break;

        case Exp_EOENSEMBLE:
            pExpoNode += 1;
            break;
        default:
            ASSERT(0);
            break;
        }
        if( iMaxLen < iRecLen )
            iMaxLen = iRecLen;
    }

    //int     iSize = iMaxLen + pHeadNode->m_iLenCaseId + pHeadNode->m_iLenRecId + 2;

    int iSizeCaseId = UseSeparator() ? ExportCaseIdVarNames(false) : pHeadNode->m_iLenCaseId;
    int iSizeMaxRecType = MAX_RECTYPECODE;

    if( Export4ByteUnicodeAlphas() ) // GHM 20130502
        iSizeCaseId = pHeadNode->m_iLenCaseIdUnicode;

    int iSize = iMaxLen + iSizeCaseId + iSizeMaxRecType + 2; // RecId is solve at runtime

    if( UseSeparator() ) {
        iSize += MAXQIDVARS; // Separator for case_id
        iSize++; // Separator for rectype
    }

    // 2.- create record-area of requested size
    m_pExpRecArea = (csprochar*) calloc( iSize + 1, sizeof(csprochar) );

    bool    bDone = ( m_pExpRecArea != NULL );

    if( bDone ) {
        m_iExpRecMax = iSize;
        m_pExpRecArea[m_iExpRecMax] = 0;
    }

    return bDone;
}

#ifdef _DEBUG
int CExport::Export_PrintNames( int iVar, int aIndex[DIM_MAXDIM], int aDimFlag[DIM_MAXDIM], void* pInfo )  {
    VART*            pVarT=VPT(iVar);
    CStringArray*    pVarNames=(CStringArray*) pInfo;

    if( pVarT->IsArray() ) {
        CString csValue[DIM_MAXDIM];

        csValue[0] = IntToString( aIndex[0] );
        csValue[1] = IntToString( aIndex[1] );
        csValue[2] = IntToString( aIndex[2] );

        CString csVarName = WS2CS(pVarT->GetName());

        if( aDimFlag[0] & CITERATOR_FLAG_EXPLICIT )
            csVarName += _T("(") + csValue[0];
        else
            csVarName += _T("($") + csValue[0];

        if( aDimFlag[1] & CITERATOR_FLAG_EXPLICIT )
            csVarName += _T(",") + csValue[1];
        else
            csVarName += _T(",$") + csValue[1];

        if( aDimFlag[2] & CITERATOR_FLAG_EXPLICIT )
            csVarName += _T(",") + csValue[2] + _T(")");
        else
            csVarName += _T(",$") + csValue[2] + _T(")");

        pVarNames->Add( csVarName );
    }
    else {
        pVarNames->Add( WS2CS(pVarT->GetName()) );
    }

    return 0;
}
#endif

int CExport::EnsembledTrip( pEmsembledTrip pExportFun, bool bEvaluateItem, void* pInfo, CArray<int,int>* pExtSymbols, CArray<int,int>* pExtOccExpr, CArray<int,int>* pExtOccExprProcSymbol ) {

    int iRet = 0;
    int aIndex[DIM_MAXDIM];
    int aDimFlag[DIM_MAXDIM];

    ASSERT( pExtOccExpr == NULL && pExtOccExprProcSymbol == NULL || pExtOccExpr != NULL && pExtOccExprProcSymbol != NULL );// RHF Jun 08, 2005

    CArray<int,int>* pSymbols = pExtSymbols ? pExtSymbols : &m_aSymbols;
    CArray<int,int>* pOccExpr = pExtOccExpr ? pExtOccExpr : &m_aOccExpr;
    CArray<int,int>* pOccExprProcSymbols=pExtOccExprProcSymbol ? pExtOccExprProcSymbol : &m_aOccProcSymbols; // RHF Jun 08, 2005

    // RHF INIC Feb 15, 2005 NEW_EXPORT
    int         iMaxRecLen=0;

    ASSERT( pSymbols->GetSize() == pOccExpr->GetSize() );
    ASSERT( pSymbols->GetSize() == pOccExprProcSymbols->GetSize() );
    bool    bUseMergeArray=GetUseMergedArray() && pExtSymbols == NULL;
    if( bUseMergeArray ) {
        pSymbols = &m_aMergedSymbols;
        pOccExpr = &m_aMergedOccExpr;
        pOccExprProcSymbols = &m_aMergedOccProcSymbol;

        ASSERT( pSymbols->GetSize() == m_aMergedSymbols.GetSize() );
        ASSERT( m_aMergedSymbols.GetSize() == m_aMergedOccExpr.GetSize() );
        ASSERT( m_aMergedSymbols.GetSize() == m_aMergedOccProcSymbol.GetSize() );
        ASSERT( m_aMergedRecNumber.GetSize() == m_aMergedOccExpr.GetSize() );
    }
    else {
        m_bNewExportRecord = false;
        m_iRunExportRecordNumber = -1;
    }
    // RHF END Feb 15, 2005 NEW_EXPORT

    //both are null or both are different of null
    ASSERT( pExtSymbols == NULL && pExtOccExpr == NULL || pExtSymbols != NULL && pExtOccExpr != NULL );

    for( int i = 0;  i < pSymbols->GetSize(); i++ ) {
        int iSymVar=pSymbols->GetAt(i);

        // RHF INIC Feb 15, 2005 NEW_EXPORT
        if( bUseMergeArray ) {
            int iLastRecNumber= (i == 0) ? -1 : m_aMergedRecNumber.GetAt(i-1);
            int iRecNumber=m_aMergedRecNumber.GetAt(i);

            ASSERT( iRecNumber == iLastRecNumber + 1 || iRecNumber == iLastRecNumber );

            if( i == 0 || iLastRecNumber != iRecNumber )
                m_bNewExportRecord = true;
            else
                m_bNewExportRecord = false;

            m_iRunExportRecordNumber = m_aMergedRecNumber.GetAt(i);
        }
        // RHF END Feb 15, 2005 NEW_EXPORT

        ASSERT( NPT(iSymVar)->IsA(SymbolType::Variable) );

        VART* pVarT = VPT(iSymVar);

        // RHF INIC Feb 16, 2005 NEW_EXPORT
        if( pExportFun == &CExport::Export_GetRecordLen ) {
            if( m_bNewExportRecord && m_iRunExportRecordNumber >= 1 ) { // Second record and higher
                iMaxRecLen = std::max( iMaxRecLen, iRet );
                iRet = 0;
            }
        }
        // RHF END Feb 16, 2005 NEW_EXPORT

        if( pVarT->IsArray() ) {
            CIterator   cExportIterator;
            //cExportIterator.Clean();

            cExportIterator.SetEngineDriver( m_pEngineDriver );

            int iMVarExpr = pOccExpr->GetAt(i);
            MVAR_NODE* pMVarNode = (MVAR_NODE*)m_pEngineArea->GetLogicByteCode().GetCodeAtPosition(iMVarExpr);

            // This node can be reused in the export for two different items (See
            int iOldVarIndex = pMVarNode->m_iVarIndex;
            pMVarNode->m_iVarIndex = iSymVar;

            // vars
            bool bUseTotalRecord = false;
            bool bBoundsOk = cExportIterator.MakeExportIterator( pMVarNode, bEvaluateItem, bUseTotalRecord );

            pMVarNode->m_iVarIndex = iOldVarIndex; //Restore

            if( !bUseTotalRecord && !bBoundsOk )
                issaerror( MessageType::Abort, 31090, pVarT->GetName().c_str() );
            //data bool    bBoundsReady = cExportIterator.MakeExportIterator( pMVarNode, true );

            // GetLow/GetHigh are zero base indices
            #define GL(x) cExportIterator.GetLow(x)
            #define GH(x) cExportIterator.GetHigh(x)
            #define LOOP_DIM(i) for( aIndex[i] = GL(i); aIndex[i] <= GH(i); aIndex[i]++ )
            #define EXPORT_ITERATION LOOP_DIM(0) LOOP_DIM(1) LOOP_DIM(2)


            // loop accross the occurrences inside the calculated bounds
            EXPORT_ITERATION
            {
                cExportIterator.GetDimFlag(aDimFlag);
                iRet += (this->*pExportFun)( iSymVar, aIndex, aDimFlag, pInfo );
            }
        }
        else { // !pVarT->IsArray()
            memset( aIndex, 0, sizeof(aIndex ) );
            memset( aDimFlag, 0, sizeof(aDimFlag ) );

            iRet += (this->*pExportFun)( iSymVar, aIndex, aDimFlag, pInfo );
        }
    }

    // RHF INIC Feb 16, 2005 NEW_EXPORT
    if( pExportFun == &CExport::Export_GetRecordLen )
        iRet = std::max( iRet, iMaxRecLen );
    // RHF END Feb 16, 2005 NEW_EXPORT

    return iRet;
}


bool CExport::UseSeparator() const
{
    return GetExportToTabDelim() ||GetExportToCommaDelim() ||  GetExportToSemiColonDelim();
}


TCHAR CExport::GetSeparatorChar() const
{
    return GetExportToTabDelim()       ? '\t' :
           GetExportToCommaDelim()     ? ','  :
           GetExportToSemiColonDelim() ? ';'  :
                                         '\0';
}


int CExport::Export_GetRecordLen( int iVar, int aIndex[DIM_MAXDIM], int aDimFlag[DIM_MAXDIM], void* pInfo )  {
    bool    bUseVarNames= (pInfo!=NULL) ? *((bool *) pInfo) : false;

    VART*    pVarT=VPT(iVar);
    int        iSize=pVarT->GetLength();

    if( !GetExportToCSPRO() && pVarT->GetDecimals() && !pVarT->GetDecChar() ) // GHM 20120504 make room for the decimal marks that we need to add to the output
        iSize++;

    if( pVarT->IsAlpha() && Export4ByteUnicodeAlphas() )
        iSize *= 4; // make room for all UTF-8 possibilities

    // RHF INIC Nov 05, 2004
    if( bUseVarNames ) {
        ASSERT( UseSeparator() );

        CString csExportedVarName;

        GenVarName( csExportedVarName, pVarT, aIndex, aDimFlag );

        iSize = std::max( iSize, csExportedVarName.GetLength() );

        //iSize = max( iSize, pVarT->GetName().GetLength() );
    }
    // RHF END Nov 05, 2004

    if( UseSeparator() )
        iSize++;

    return iSize;
}

int CExport::Export_GenRecordList( int iVar, int aIndex[DIM_MAXDIM], int aDimFlag[DIM_MAXDIM], void* pInfo )  {
    VART*    pVarT=VPT(iVar);
    CMap<int,int,int,int>*    pMapUserRecords=(CMap<int,int,int,int>*) pInfo;

    int        iSymSec=pVarT->GetOwnerSec();

    pMapUserRecords->SetAt(iSymSec,iSymSec);

    return 0;
}


// RHF INIC Nov 05, 2004
int CExport::Export_Names( int iVar, int aIndex[DIM_MAXDIM], int aDimFlag[DIM_MAXDIM], void* pInfo ) {
    ASSERT( UseSeparator() );

    CString        csExportedVarName;
    VART*        pVarT=VPT(iVar);
    GenVarName( csExportedVarName, pVarT, aIndex, aDimFlag );

    CopyToExportArea( csExportedVarName, csExportedVarName.GetLength() );
    CopyToExportAreaExtraChar();

    return csExportedVarName.GetLength()+1;
}
// RHF END Nov 05, 2004

// Implicit dimension --> MaxOcc is used and is exported horizontally (flat)
// Explicit dimension --> TotOcc is used and is exported vertically (as a record)
int CExport::Export_Data( int iVar, int aIndex[DIM_MAXDIM], int aDimFlag[DIM_MAXDIM], void* pInfo ) {
    VART* pVarT = VPT(iVar);
    VARX* pVarX = VPX(iVar);
    bool  bCheckLevel = (pInfo!=NULL) ? *((bool *) pInfo) : false;

    if( GetExportItemSubItem() && GetExportToCSPRO() ) {
        bool    bIsSubItem=(pVarT->GetOwnerSymItem()>0);
        if( bIsSubItem ) //SubItem doesn't export the buffer. The buffer is exported by the parent item
            return 0;
    }

    csprochar*   pVarAsciAddr;
    CIntDriver* pIntDriver = m_pEngineDriver->m_pIntDriver.get();

    if( pVarT->IsArray() ) {
        CNDIndexes theIndex( ZERO_BASED, aIndex );
        m_pEngineDriver->prepvar( pVarT, theIndex, NO_VISUAL_VALUE );
        pVarAsciAddr = pIntDriver->GetMultVarAsciiAddr( pVarX, theIndex );
    }
    else {
        m_pEngineDriver->prepvar( pVarT, NO_VISUAL_VALUE );
        pVarAsciAddr = pIntDriver->GetSingVarAsciiAddr( pVarX );
    }

    // RHF INIC Feb 21, 2005
    for( int i=0; pVarAsciAddr != NULL && i < DIM_MAXDIM; i++ ) {
        if( aDimFlag[i] & CITERATOR_FLAG_INVALID_DIM )
            pVarAsciAddr = NULL;
    }
    // RHF END Feb 21, 2005

    // RHF INIC Feb 16, 2005
    if( bCheckLevel && pVarT->GetLevel() > m_pHeadNode->m_iExportAppLevel )
        pVarAsciAddr = NULL;
    // RHF END Feb 16, 2005

    int     iValidSize  = pVarT->GetLength();

    if( pVarT->GetDecimals() && !GetExportToCSPRO() )
    {
        if( !pVarT->GetDecChar() && ( pVarAsciAddr != nullptr ) ) //  GHM 20120504 the decimal character wasn't getting exported when there was no decimal character specified in the input file
        {
            TCHAR decimalMark = GetExportCommaDecimal() ? _T(',') : _T('.');
            int nonDecSize = iValidSize - pVarT->GetDecimals();

            if( !isdigit(pVarAsciAddr[nonDecSize]) ) // the value is either notappl or default
                decimalMark = pVarAsciAddr[nonDecSize];

            CopyToExportArea(pVarAsciAddr,nonDecSize);
            CopyToExportArea(&decimalMark,1);
            CopyToExportArea(pVarAsciAddr + nonDecSize,pVarT->GetDecimals());
        }

        else
        {
            if( GetExportCommaDecimal() && ( pVarAsciAddr != nullptr ) ) // GHM 20120428 to allow exporting with a comma as the decimal point
            {
                int decPointLocation = iValidSize - pVarT->GetDecimals() - 1;

                if( pVarAsciAddr[decPointLocation] == _T('.') ) // if it doesn't, the value was probably notappl
                    pVarAsciAddr[decPointLocation] = _T(',');
            }

            CopyToExportArea( pVarAsciAddr, iValidSize );
        }
    }

    else
    {
        CopyToExportArea( pVarAsciAddr, iValidSize );

        if( pVarT->IsAlpha() && Export4ByteUnicodeAlphas() )
        {
            // pad the string in the case that every character didn't require four UTF-8 bytes
            int outputtedChars = pVarAsciAddr ? WideCharToMultiByte(CP_UTF8,0,pVarAsciAddr,iValidSize,NULL,0,NULL,NULL) : iValidSize;
            CopyToExportArea(NULL,iValidSize * 4 - outputtedChars);
        }
    }

    iValidSize += CopyToExportAreaExtraChar();

    for( int i = 0; i < DIM_MAXDIM; i++ ) {
        if( (aDimFlag[i] & CITERATOR_FLAG_EXPLICIT) && !(aDimFlag[i] & CITERATOR_FLAG_LESS_TOTAL)
            //|| (aDimFlag[i] == CITERATOR_FLAG_NONE)  // Single item or this dimension is not multiple in a mutiple item
            )
            iValidSize=0;
    }

    SECT*   pSecT=pVarT->GetSPT();
    SECX*    pSecX=pSecT->GetSecX();

    // Greater than TotalOcc --> The record is written only if other item generate valid information
    if( aIndex[0] >= pSecX->GetOccurrences() )
        iValidSize = 0;

    // Caseid elements & rectype --> iValidSize=0
    return iValidSize;
}

int CExport::Export_NumRecords( int iVar, int aIndex[DIM_MAXDIM], int aDimFlag[DIM_MAXDIM], void* pInfo ) {
    return WasPendingSlash();
}

int CExport::Export_SpssDescription( int iVar, int aIndex[DIM_MAXDIM], int aDimFlag[DIM_MAXDIM], void* pInfo ) {
    VART*    pVarT=VPT(iVar);
    int*    pLoc=(int*) pInfo;

    // RHF INIC Feb 15, 2005 NEW_EXPORT
    if( m_bNewExportRecord && m_iRunExportRecordNumber >= 1 ) { // Second record and higher
        *pLoc = 1 + m_pHeadNode->m_iLenCaseId + m_pHeadNode->m_iLenRecId;
        _ftprintf( m_pFileSPSS, _T("/\n") );
    }
    // RHF END Feb 15, 2005 NEW_EXPORT

    CString csExportedVarName;

    GenVarName( csExportedVarName, pVarT, aIndex, aDimFlag );

    sps_variable( pVarT, csExportedVarName, *pLoc );

    // (*pLoc) += pVarT->GetLength(); // GHM 20120504 this will now be done in the above function

    return 0;
}

int CExport::Export_SpssVarLabel( int iVar, int aIndex[DIM_MAXDIM], int aDimFlag[DIM_MAXDIM], void* pInfo ) {
    VART*    pVarT=VPT(iVar);
    csprochar*    pszTitle=_T("VARIABLE LABELS");
    csprochar    pszLabel[_MAXLABLEN+2];

    GetVarLabel( pVarT, pszLabel );

    // GHM 20141119 SPSS can now handle 255+ character labels
    // pszLabel[SPSS_MAXLAB] = 0; // Trunc

    if( *pszLabel == 0 )
        return 0;

    if( !GetPendingSlash() ) {
        _ftprintf( m_pFileSPSS, _T("%s\n "), pszTitle );
        SetPendingSlash();
        _fputtc( _T(' '), m_pFileSPSS ); // add space before continuation line so that spss syntax will work when
                                   // used as include file.  JH 1/18/07
    }
    else {
        _fputtc( _T(' '), m_pFileSPSS ); // add space before continuation line so that spss syntax will work when
                                   // used as include file.  JH 1/18/07
        _fputtc( _T('/'), m_pFileSPSS );
    }

    CString csExportedVarName;
    GenVarName( csExportedVarName, pVarT, aIndex, aDimFlag );

    _ftprintf( m_pFileSPSS, _T("%-8s "), csExportedVarName.GetString() );

    csprochar    pszGenLabel[_MAXLABLEN+2];

    sps_label( pszGenLabel, pszLabel );
    _ftprintf( m_pFileSPSS, _T("%s\n"), pszGenLabel );

    return 1;
}

int CExport::Export_SpssMissing( int iVar, int aIndex[DIM_MAXDIM], int aDimFlag[DIM_MAXDIM], void* pInfo ) {
    VART*    pVarT=VPT(iVar);
    csprochar*    pszTitle=_T("MISSING VALUE");

    if( !pVarT->IsNumeric() )
        return 0;

    const   NumericValueProcessor& numeric_value_processor = pVarT->GetCurrentNumericValueProcessor();
    double  dValue = numeric_value_processor.ConvertNumberFromEngineFormat(MISSING);
    bool    bHasMissing = !IsSpecial(dValue);

    if( bHasMissing ) {
        if( !GetPendingSlash() ) {
            _ftprintf( m_pFileSPSS, _T("%s\n "), pszTitle );
            SetPendingSlash();
            _fputtc( _T(' '), m_pFileSPSS ); // add space before continuation line so that spss syntax will work when
                                       // used as include file.  JH 1/18/07
        }
        else {
            _fputtc( _T(' '), m_pFileSPSS ); // add space before continuation line so that spss syntax will work when
                                       // used as include file.  JH 1/18/07
            _fputtc( _T('/'), m_pFileSPSS );
        }
        CString csExportedVarName;
        GenVarName( csExportedVarName, pVarT, aIndex, aDimFlag );

        // 20130907 trevor-reported bug: say a missing value were 99.9, it would get output as 100
        CString formatStr;
        formatStr.Format(_T("%%-8ls (%%.%df)\n"), pVarT->GetDecimals());
        _ftprintf( m_pFileSPSS, formatStr, csExportedVarName.GetString(), dValue );
    }

    return bHasMissing ? 1 : 0;
}

int CExport::Export_SpssValueLabels( int iVar, int aIndex[DIM_MAXDIM], int aDimFlag[DIM_MAXDIM], void* pInfo ) {
    VART*    pVarT=VPT(iVar);
    csprochar*    pszTitle=_T("VALUE LABELS");
    csprochar    pszValue[_MAXLABLEN+2];
    csprochar    pszLabel[_MAXLABLEN+2];
    int     iNumCategories = BuildVarCategories( pVarT );
    bool    bHasValueLabels = ( iNumCategories > 0 );

    // GHM 20111020 if there are no labels in the value set, we shouldn't print anything out
    bool validLabel = false;

    for( int iValue = 0; !validLabel && iValue < iNumCategories; iValue++ )
    {
        if( GetVarCategoryAt( iValue )->csTextLabel.GetLength() )
            validLabel = true;
    }

    if( bHasValueLabels && validLabel ) {

        if( !GetPendingSlash() ) {
            _ftprintf( m_pFileSPSS, _T("%s\n "), pszTitle );
            SetPendingSlash();
            _fputtc( _T(' '), m_pFileSPSS ); // add space before continuation line so that spss syntax will work when
                                       // used as include file.  JH 1/18/07
        }
        else {
            _fputtc( _T(' '), m_pFileSPSS ); // add space before continuation line so that spss syntax will work when
                                       // used as include file.  JH 1/18/07
            _fputtc( _T('/'), m_pFileSPSS );
        }

        CString csExportedVarName;
        GenVarName( csExportedVarName, pVarT, aIndex, aDimFlag );

        _ftprintf( m_pFileSPSS, _T("%-8s\n"), csExportedVarName.GetString() );

        int     iSlotLen = std::min( pVarT->GetLength(), 5 );
        csprochar    pszGenLabel[_MAXLABLEN+3];

        for( int iValue = 0; iValue < iNumCategories; iValue++ ) {
            CExportVarCateg*    pVarCateg = GetVarCategoryAt( iValue );

            // GHM 20111020 only print only valid labels (no more undefined labels)
            if( pVarCateg->csTextLabel.GetLength() )
            {
                CopyQuotedStringWithLengthLimit( pszValue, pVarCateg->csTextValue );
                trimleft( pszValue );

                _tcscpy( pszLabel, pVarCateg->csTextLabel );
                sps_label( pszGenLabel, pszLabel );

                if( pVarT->IsNumeric() )
                    _ftprintf( m_pFileSPSS, _T("  %4s %s\n"), pszValue, pszGenLabel );
                else
                    _ftprintf( m_pFileSPSS, _T("  %-*s %s\n"), iSlotLen, pszValue, pszGenLabel ); // RHF, Jun 13, 2005
            }
        }
    }

    DeleteVarCategories();

    return bHasValueLabels ? 1 : 0;
}

int CExport::Export_SasFormat( int iVar, int aIndex[DIM_MAXDIM], int aDimFlag[DIM_MAXDIM], void* pInfo ) {
    VART*                    pVarT=VPT(iVar);
    CMap<VART*,VART*,int,int>*    pMapSasLabel=(CMap<VART*,VART*,int,int>*) pInfo;

    sas_vallabels( pVarT, pMapSasLabel );

    return 0;
}

int CExport::Export_SasAttrib( int iVar, int aIndex[DIM_MAXDIM], int aDimFlag[DIM_MAXDIM], void* pInfo ) {
    VART*    pVarT=VPT(iVar);
    CMap<VART*,VART*,int,int>*    pMapSasLabel=(CMap<VART*,VART*,int,int>*) pInfo;

    sas_varattrib( pVarT, aIndex, aDimFlag, pMapSasLabel );

    return 0;
}

int CExport::Export_SasMissing( int iVar, int aIndex[DIM_MAXDIM], int aDimFlag[DIM_MAXDIM], void* pInfo ) {
    VART*    pVarT=VPT(iVar);

    sas_misvalue( pVarT, aIndex, aDimFlag );

    return 0;
}

int CExport::Export_SasInput( int iVar, int aIndex[DIM_MAXDIM], int aDimFlag[DIM_MAXDIM], void* pInfo ) {
    VART*    pVarT=VPT(iVar);
    int*    pLoc=(int*) pInfo;

    // RHF INIC Feb 15, 2005 NEW_EXPORT
    if( m_bNewExportRecord && m_iRunExportRecordNumber >= 1 ) { // Second record and higher
        *pLoc = 1 + m_pHeadNode->m_iLenCaseId + m_pHeadNode->m_iLenRecId;
        _ftprintf( m_pFileSAS, _T("    ;\n") );
        _ftprintf( m_pFileSAS, _T("    input\n") );
    }
    // RHF END Feb 15, 2005 NEW_EXPORT

    sas_inpvariable( pVarT, aIndex, aDimFlag, *pLoc );

    (*pLoc) += pVarT->GetLength();

    if( pVarT->GetDecimals() && !pVarT->GetDecChar() ) // GHM 20120504 we need to add the decimal character to the output
        (*pLoc)++;

    else if( !GetExportForceANSI() && pVarT->IsAlpha() ) // GHM 20141015 * 3 (not * 4) because we've already added one length above
        (*pLoc) += pVarT->GetLength() * 3;

    return 0;
}

int CExport::Export_StataDescription( int iVar, int aIndex[DIM_MAXDIM], int aDimFlag[DIM_MAXDIM], void* pInfo ) {
    VART*    pVarT=VPT(iVar);
    int*    pLoc=(int*) pInfo;

    CString csExportedVarName;

    GenVarName( csExportedVarName, pVarT, aIndex, aDimFlag );

    int        iRecNum=1;

    // RHF INIC Feb 15, 2005 NEW_EXPORT
    if( m_bNewExportRecord && m_iRunExportRecordNumber >= 1 ) { // Second record and higher
        *pLoc = 1 + m_pHeadNode->m_iLenCaseId + m_pHeadNode->m_iLenRecId;
    }

    if( m_iRunExportRecordNumber >= 1 ) // Second record and higher
        iRecNum = m_iRunExportRecordNumber + 1;
    // RHF END Feb 15, 2005 NEW_EXPORT

    stata_variable( pVarT, csExportedVarName, *pLoc, iRecNum  );

    (*pLoc) += pVarT->GetLength() * ( ( pVarT->IsAlpha() && !GetExportForceANSI() ) ? 4 : 1 );

    if( pVarT->GetDecimals() && !pVarT->GetDecChar() ) // GHM 20120504 we need to add the decimal character to the output
        (*pLoc)++;

    return 0;
}

int CExport::Export_StataVarLabel( int iVar, int aIndex[DIM_MAXDIM], int aDimFlag[DIM_MAXDIM], void* pInfo ) {
    VART*    pVarT=VPT(iVar);
    csprochar*    pszTitle=_T("");
    csprochar    pszLabel[_MAXLABLEN+2];

    GetVarLabel( pVarT, pszLabel );

    pszLabel[STATA_MAXLAB] = 0; // Trunc

    if( *pszLabel == 0 )
        return false;

    if( !GetPendingSlash() ) {
        if( *pszTitle )
            _ftprintf( m_pFileSTATAdo, _T("%s\n"), pszTitle );
        SetPendingSlash();
    }

    csprochar    pszGenLabel[_MAXLABLEN+2];
    csprochar    pszGenName[256];

    CString csExportedVarName;
    GenVarName( csExportedVarName, pVarT, aIndex, aDimFlag );

    _tcscpy( pszGenName, csExportedVarName );

    _tcslwr( pszGenName );

    _ftprintf( m_pFileSTATAdo, _T("label variable %-8s "), pszGenName );

    _tcscpy( pszGenLabel, pszLabel );
    strreplace( pszGenLabel, _T("\""), _T("'") );

    _ftprintf( m_pFileSTATAdo, _T("\"%s\"\n"), pszGenLabel );

    return 1;
}

int CExport::Export_StataValueLabels( int iVar, int aIndex[DIM_MAXDIM], int aDimFlag[DIM_MAXDIM], void* pInfo ) {
    VART*    pVarT=VPT(iVar);
    csprochar*    pszTitle=_T("\n#delimit ;");


    //~~if( pVarT->IsUsed() )       // ??? if( !pVarT->IsUsed() )
    //~~    return 1;
    //~~pVarT->SetUsed( true );     // ???

    // value set not allowed for alpha Var
    if( !pVarT->IsNumeric() )
        return 0;

    // value set only allowed for integer
    //  if( pVarT->GetDecimals() > 0 )
    //      return 1;

    int     iNumCategories = BuildVarCategories( pVarT );
    bool    bHasValueLabels = ( stata_countValidValues() > 0 );
    bool    bGenSemicolon = false;
    csprochar    pszLabel[_MAXLABLEN+2];
    csprochar    pszValue[_MAXLABLEN+2];
    csprochar    pszGenLabel[_MAXLABLEN+3];

    CString csExportedVarName;
    GenVarName( csExportedVarName, pVarT, aIndex, aDimFlag );

    // GHM 20101107 begin

    // the STATA export output for multiple occurring records would define variables like B_01, B_02, etc., would have the value
    // sets also be B_01, B_02, etc., but when associating the value set would say: label values b_01     B
    // so now, i only define the value set once, so it will be just B, and the "label values b_01     B" code will work

    CString csGenName = WS2CS(pVarT->GetName());

    static int prevVar = -1;

    if( prevVar == iVar )
    {
        DeleteVarCategories();
        return 1;
    }

    prevVar = iVar;
    // GHM 20101107 end


    if( bHasValueLabels ) {
        // there is at least one value between 0 and 32767
        if( !stata_hasvaluelabel( pVarT ) )             // RHF 15/3/99
            bHasValueLabels = false;                    // RHF 15/3/99
    }

    bool    bStataMissing=stata_hasmissing( pVarT ); // RHF Oct 18, 2002
    if( bHasValueLabels || bStataMissing ) { // RHF Oct 18, 2002 Add  bStataMissing
        if( !GetPendingSlash() ) {
            if( *pszTitle )
                _ftprintf( m_pFileSTATAdo, _T("%s\n"), pszTitle );
            SetPendingSlash();
        }

        _ftprintf( m_pFileSTATAdo, _T("label define %-8s\n"), csGenName.GetString() );

        for( int iValue = 0; iValue < iNumCategories; iValue++ ) {
            CExportVarCateg*    pVarCateg = GetVarCategoryAt( iValue );

            _tcscpy( pszValue, pVarCateg->csTextValue );
            trimleft( pszValue );

            _tcscpy( pszLabel, pVarCateg->csTextLabel );

            // generate only if valid for STATA     <begin> // RHF 15/3/99

            // GHM 20111020 only print only valid labels (no more undefined labels)
            if( stata_isvalidvalue( pszValue ) && _tcslen(pszLabel) ) {
            //if( stata_isvalidvalue( pszValue ) ) {
                _tcscpy( pszGenLabel, pszLabel );
                strreplace( pszGenLabel, _T("\""), _T("'") );

                //if( strlen( pszGenLabel ) == 0 ) strcpy( pszGenLabel, "***Undefined Label" );// RHF May 02, 2002
                _ftprintf( m_pFileSTATAdo, _T("%6s \"%s\"\n"), pszValue, pszGenLabel );


                bGenSemicolon = true;
            }
            // generate only if valid for STATA     <end>   // RHF 15/3/99
        }
    }

    DeleteVarCategories();

    // print missing if present and valid
    // RHF COM Oct 18, 2002 bGenSemicolon |= stata_missing( pVarT );
    if( bStataMissing ) // RHF Oct 18, 2002
        bGenSemicolon = bGenSemicolon || stata_missing( pVarT );

    if( bGenSemicolon )
        _ftprintf( m_pFileSTATAdo, _T(";\n") );

    return 1;
}

int CExport::Export_StataAsocValueLabels( int iVar, int aIndex[DIM_MAXDIM], int aDimFlag[DIM_MAXDIM], void* pInfo ) {
    VART*    pVarT=VPT(iVar);
    csprochar*    pszTitle=_T("\n#delimit cr");

    // value set not allowed for alpha Var
    if( !pVarT->IsNumeric() )
        return 1;

    // value set only allowed for integer
    //  if( pVarT->GetDecimals() > 0 )
    //      return 1;

    int     iNumCategories = BuildVarCategories( pVarT );
    bool    bHasValueLabels = ( iNumCategories > 0 && stata_hasvaluelabel( pVarT ) );

    if( bHasValueLabels ) {
        csprochar    pszGenName[256];
        csprochar    pszLabel[_MAXLABLEN+2];

        GetVarLabel( pVarT, pszLabel );

        if( *pszLabel ) {
            if( !GetPendingSlash() ) {
                if( *pszTitle )
                    _ftprintf( m_pFileSTATAdo, _T("%s\n"), pszTitle );
                SetPendingSlash();
            }

            CString csExportedVarName;
            GenVarName( csExportedVarName, pVarT, aIndex, aDimFlag );

            _tcscpy( pszGenName, csExportedVarName );
            _tcslwr( pszGenName );

            _ftprintf( m_pFileSTATAdo, _T("label values %-8s %-8s\n"), pszGenName, pVarT->GetName().c_str() );
        }
    }

    DeleteVarCategories();

    return bHasValueLabels ? 1 : 0;
}

int CExport::Export_CsProDescription( int iVar, int aIndex[DIM_MAXDIM], int aDimFlag[DIM_MAXDIM], void* pInfo ) {
    VART*    pVarT=VPT(iVar);
    int        iLen=pVarT->GetLength();
    int*    pLoc=(int*) pInfo;
    int        iParentItem=pVarT->GetOwnerSymItem();
    bool    bIsSubItem=(iParentItem > 0);

    CString csExportedVarName;

    // RHF INIC Nov 09, 2004
    int iOldSeparator=m_cNameSeparator = _T('\0');

    //Some constant
    bool bSomeConstant = false;
    if( pVarT->IsArray() ) {
        for( int i=0; i < pVarT->GetNumDim() && !bSomeConstant; i++ ) {
            CDimension::VDimType xDimType=pVarT->GetDimType( i );

            int iDim = ( xDimType == CDimension::Record )  ? 0:
                       ( xDimType == CDimension::Item )    ? 1:
                       ( xDimType == CDimension::SubItem ) ? 2: -1;

            bool bConstant = ( aDimFlag[iDim] & CITERATOR_FLAG_CONSTANT ) != 0;
            bSomeConstant = bConstant;  // rcl, Feb 2005
        }
    }

    if( bSomeConstant )
        m_cNameSeparator = _T('_');
    // RHF END Nov 09, 2004

    GenVarName( csExportedVarName, pVarT, aIndex, aDimFlag );

    m_cNameSeparator = iOldSeparator;

    const CDictItem*    pDictItem=NULL;

    ASSERT( m_pRecordCSPRO != NULL );

    // Search Item in record and change the number of occurrences if found
    bool    bFound=false;
    for( int i=0; !bFound && i < m_pRecordCSPRO->GetNumItems(); i++ ) {
        CDictItem* pItem = m_pRecordCSPRO->GetItem(i);

        if( pItem->GetName() == WS2CS(pVarT->GetName()) ) {

            //When Items and SubItems are exported, subitems can't change the number of the original ocurrences
            if( !bIsSubItem || GetExportSubItemOnly() || pVarT->GetMaxOccs() > (int)pItem->GetOccurs()) {
                int iNumOcc=pItem->GetOccurs();
                iNumOcc++;
                pItem->SetOccurs(iNumOcc);
            }
            bFound = true;
        }
    }

    if( !bFound ) {
        pDictItem=pVarT->GetDictItem();

        ASSERT( pDictItem != NULL );

        CDictItem    cNewItem( *pDictItem );

        cNewItem.SetName( csExportedVarName );
        cNewItem.SetStart(*pLoc);

        //transform to item
        if( GetExportSubItemOnly() )
            cNewItem.SetItemType(ItemType::Item);
        else if( GetExportItemOnly() )
            cNewItem.SetItemType(ItemType::Item);
        else if( GetExportItemSubItem() ) {
            if( bIsSubItem && GetExportToCSPRO()) { // only CSPro supports subitems
                                                    // if this is for gen metadata
                                                    // for SPSS,... then treat as item
                const CDictItem*    pParentItem=VPT(iParentItem)->GetDictItem();
                iLen = 0; //Keep parent location

                //int        iParentLoc=(*pLoc)-pParentItem->GetLen()*pParentItem->GetOccurs();
                int        iParentLoc=m_iLastItemPos;

                int        iSubItemDisp=pDictItem->GetStart()-pParentItem->GetStart();

                ASSERT( iSubItemDisp >= 0 );

                int        iSubItemLoc=iParentLoc+iSubItemDisp;

                cNewItem.SetStart(iSubItemLoc);
                cNewItem.SetItemType(ItemType::Subitem);
            }
            else {
                cNewItem.SetItemType(ItemType::Item);
                m_iLastItemPos = *pLoc;
            }
        }
        else
            ASSERT(0);

        cNewItem.SetOccurs (1);

        m_pRecordCSPRO->AddItem( &cNewItem );

        // Keep track of notes from records whose items are merged into this
        // record so we can add them to exported dict note later.
        if (!pDictItem->GetRecord()->GetNote().IsEmpty()) {
            CString note;
            const CIMSAString recName = pDictItem->GetRecord()->GetName();
            if (!m_mapRecNotes.Lookup(recName, note)) {
                m_mapRecNotes[recName] = pDictItem->GetRecord()->GetNote();
            }
        }
    }

    // RHF COM Feb 09, 2005if( !bIsSubItem )
    if( !bIsSubItem || GetExportSubItemOnly() ) // RHF Feb 09, 2005
        (*pLoc) += iLen;

    return 0;
}




int CExport::Export_R_Format( int iVar, int aIndex[DIM_MAXDIM], int aDimFlag[DIM_MAXDIM], void* pInfo ) // GHM 20120507
{
    bool * bFirstElement = (bool *)pInfo;

    if( !m_bCaseIdReady )
    {
        if( m_pHeadNode->m_bCaseIdAfterRecType )
        {
            if( m_pHeadNode->m_iLenRecId > 0 )
            {
                _ftprintf(m_pFileR,_T("\"A%d\""),m_pHeadNode->m_iLenRecId);
                *bFirstElement = false;
            }

            if( m_pHeadNode->m_iLenCaseId > 0 )
            {
                m_bCaseIdReady = true;
                EnsembledTrip( &CExport::Export_R_Format, false, bFirstElement, &m_iCaseIdItems, &m_aCaseIdOccExpr, &m_aCaseIdOccExprProcSymbol );
            }
        }

        else
        {
            if( m_pHeadNode->m_iLenCaseId > 0 )
            {
                m_bCaseIdReady = true;
                EnsembledTrip( &CExport::Export_R_Format, false, bFirstElement, &m_iCaseIdItems, &m_aCaseIdOccExpr, &m_aCaseIdOccExprProcSymbol );
            }

            if( m_pHeadNode->m_iLenRecId > 0 )
                _ftprintf(m_pFileR,_T(",\"A%d\""),m_pHeadNode->m_iLenRecId);
        }

        m_bCaseIdReady = true;
    }


    VART * pVarT = VPT(iVar);

    if( *bFirstElement )
        *bFirstElement = false;

    else
        _ftprintf(m_pFileR,_T(","));

    int iLen = pVarT->GetLength();
    TCHAR formatType = _T('I');

    if( pVarT->IsAlpha() )
        formatType = _T('A');

    else if( pVarT->GetDecimals() )
    {
        if( !pVarT->GetDecChar() )
            iLen++;

        formatType = _T('F');
    }

    else if( iLen > 9 ) // GHM 20120711 turns out that the read.format function can't work with integers 10+ digits
        formatType = _T('F');

    _ftprintf(m_pFileR,_T("\"%c%d\""),formatType,iLen);

    return 0;
}


int CExport::Export_R_Names( int iVar, int aIndex[DIM_MAXDIM], int aDimFlag[DIM_MAXDIM], void* pInfo ) // GHM 20120507
{
    bool * bFirstElement = (bool *)pInfo;

    if( !m_bCaseIdReady )
    {
        if( m_pHeadNode->m_bCaseIdAfterRecType )
        {
            if( m_pHeadNode->m_iLenRecId > 0 )
            {
                _ftprintf(m_pFileR,_T("\"rec.type\""));
                *bFirstElement = false;
            }

            if( m_pHeadNode->m_iLenCaseId > 0 )
            {
                m_bCaseIdReady = true;
                EnsembledTrip( &CExport::Export_R_Names, false, bFirstElement, &m_iCaseIdItems, &m_aCaseIdOccExpr, &m_aCaseIdOccExprProcSymbol );
            }
        }

        else
        {
            if( m_pHeadNode->m_iLenCaseId > 0 )
            {
                m_bCaseIdReady = true;
                EnsembledTrip( &CExport::Export_R_Names, false, bFirstElement, &m_iCaseIdItems, &m_aCaseIdOccExpr, &m_aCaseIdOccExprProcSymbol );
            }

            if( m_pHeadNode->m_iLenRecId > 0 )
                _ftprintf(m_pFileR,_T(",\"rec.type\""));
        }

        m_bCaseIdReady = true;
    }


    VART * pVarT = VPT(iVar);

    if( *bFirstElement )
        *bFirstElement = false;

    else
        _ftprintf(m_pFileR,_T(","));

    CString varName; // = pVarT->GetName();
    GenVarName(varName,pVarT,aIndex,aDimFlag); // GHM 20120622
    varName.MakeLower();

    _ftprintf(m_pFileR,_T("\"%s\""), varName.GetString());

    return 0;
}

int CExport::Export_R_ValueLabels( int iVar, int aIndex[DIM_MAXDIM], int aDimFlag[DIM_MAXDIM], void* pInfo ) // GHM 20120508
{
    CString * pDataFrame = (CString *)pInfo;

    if( !m_bCaseIdReady )
    {
        m_bCaseIdReady = true;

        if( m_pHeadNode->m_iLenCaseId > 0 )
            EnsembledTrip( &CExport::Export_R_ValueLabels, false, pDataFrame, &m_iCaseIdItems, &m_aCaseIdOccExpr, &m_aCaseIdOccExprProcSymbol );
    }

    VART * pVarT = VPT(iVar);
    const CDictItem* pItem = pVarT->GetDictItem();

    bool bCanCreateLabels = pItem->HasValueSets(); // we can create labels for alpha items and numeric items with value sets that don't have ranges

    if( bCanCreateLabels && pVarT->IsNumeric() )
        bCanCreateLabels = ( pItem->GetValueSet(0).GetNumToValues() == 0 );

    if( bCanCreateLabels )
    {
        CString varName; // = pVarT->GetName();
        GenVarName(varName,pVarT,aIndex,aDimFlag); // GHM 20120622
        varName.MakeLower();

        TCHAR * tabChar = _T("");
        TCHAR * levelEncloserChar = _T("");

        if( pVarT->IsAlpha() )
        {
            tabChar = _T("\t");
            levelEncloserChar = _T("\"");
            _ftprintf(m_pFileR,_T("\tif( cspro.factor.type == 2 ) {\n"));
        }

        CString sLevels,sLabels;

        bool bPrintedFirst = false;

        for( const auto& dict_value : pItem->GetValueSet(0).GetValues() )
        {
            //for( size_t pairs = 0; pairs < dict_value.GetNumValuePairs(); ++pairs )
            const size_t pairs = 0; // duplicate factors aren't allowed in R
            {
                const auto& dict_value_pair = dict_value.GetValuePair(pairs);

                if( !SO::IsBlank(dict_value_pair.GetFrom()) ) // don't allow just not applicable values
                {
                    sLevels.AppendFormat(_T("%s%s%s%s"), bPrintedFirst ? _T(",") : _T(""), levelEncloserChar, dict_value_pair.GetFrom().GetString(), levelEncloserChar);
                    sLabels.AppendFormat(_T("%s\"%s\""), bPrintedFirst ? _T(",") : _T(""), dict_value.GetLabel().GetString());
                    bPrintedFirst = true;
                }
            }
        }

        _ftprintf(m_pFileR, _T("%s\tif( cspro.factor.create.new.variable ) {\n"), tabChar);
        _ftprintf(m_pFileR, _T("%s\t\t%s$%s.f <- factor(%s$%s,levels = c(%s),labels = c(%s))\n"),
            tabChar, pDataFrame->GetString(), varName.GetString(), pDataFrame->GetString(), varName.GetString(), sLevels.GetString(), sLabels.GetString());
        _ftprintf(m_pFileR, _T("%s\t} else {\n"), tabChar);
        _ftprintf(m_pFileR, _T("%s\t\t%s$%s <- factor(%s$%s,levels = c(%s),labels = c(%s))\n"),
            tabChar, pDataFrame->GetString(), varName.GetString(), pDataFrame->GetString(), varName.GetString(), sLevels.GetString(), sLabels.GetString());
        _ftprintf(m_pFileR, _T("%s\t}\n"), tabChar);

        if( pVarT->IsAlpha() )
            _ftprintf(m_pFileR,_T("\t}\n"));

        _ftprintf(m_pFileR,_T("\n"));
    }

    return 0;
}


CString CExport::GetDcfExpoName() const
{
    CNPifFile* pPifFile = m_pEngineDriver->GetPifFile();

    if( pPifFile->GetCSPROSyntaxFName().IsEmpty() )
        return PortableFunctions::PathRemoveFileExtension<CString>(GetExpoName()) + FileExtensions::WithDot::Dictionary;

    else
        return pPifFile->GetCSPROSyntaxFName();
}

bool CExport::HasRecType( EXP_HEADER_NODE* pHeadNode) {
    return (pHeadNode->m_iRecTypeExpr >= 0);
}

void CExport::GetRecNameAndOcc( CString& csRecordName, int& iMaxOcc ) {
    EXP_HEADER_NODE*    pHeadNode = GetHeadNode();
    CIntDriver*         pIntDriver = m_pEngineDriver->m_pIntDriver.get();
    iMaxOcc = 0;

    if( pHeadNode->m_bIsSymbolRecName ) {
        SECT* pSecT=SPT(pHeadNode->m_iRecNameExpr);

        csRecordName = WS2CS(pSecT->GetName());
        iMaxOcc = pSecT->GetMaxOccs();
    }
    else if( pHeadNode->m_iRecNameExpr >= 0 )  {
        csRecordName = pIntDriver->EvalAlphaExpr<CString>(pHeadNode->m_iRecNameExpr);
    }
    else
        csRecordName.Empty();
}

void CExport::ExportRecType( bool bCopyToRecArea ) {
    EXP_HEADER_NODE*    pHeadNode = GetHeadNode();
    CIntDriver*         pIntDriver = m_pEngineDriver->m_pIntDriver.get();

    ASSERT( HasRecType( pHeadNode ) );

    //RecType
    if( *m_pszSectionCode == 0 ) {
        if( pHeadNode->m_bIsSymbolRecType ) {
            SECT* pSecT=SPT(pHeadNode->m_iRecTypeExpr);

            int iSecCodeLen=pSecT->GetDicT()->GetSecIdLen();

            // RHF COM Nov 09, 2004 strcpymax( m_pszSectionCode, pSecT->code, MAX_RECTYPECODE );
            strcpymax( m_pszSectionCode, pSecT->code, std::min( iSecCodeLen, MAX_RECTYPECODE ) );
        }
        else if( pHeadNode->m_iRecTypeExpr >= 0 )  {
            CString csRecordType = pIntDriver->EvalAlphaExpr<CString>(pHeadNode->m_iRecTypeExpr);

            strcpymax( m_pszSectionCode, csRecordType, MAX_RECTYPECODE );
        }
        else
            ASSERT(0);

        if( pHeadNode->m_iLenRecId == 0 ) {
            pHeadNode->m_iLenRecId = (csprochar)_tcslen(m_pszSectionCode);
        }
        else { // Align right & trunc
            ASSERT(0);
            /*
            ASSERT( pHeadNode->m_iLenRecId > 0 );

            csprochar                pszSectionCodeAux[MAX_RECTYPECODE+1];

            sprintf( pszSectionCodeAux, "%*s", pHeadNode->m_iLenRecId, m_pszSectionCode );
            strcpy( m_pszSectionCode, pszSectionCodeAux );

            m_pszSectionCode[pHeadNode->m_iLenRecId];
            */
        }
    }

    if( bCopyToRecArea ) {
        //#ifdef _DEBUG
        //    DebugData( pVarT, pVarAsciAddr, aIndex, GetRecIndex() );
        //#endif

        CopyToExportArea( m_pszSectionCode, pHeadNode->m_iLenRecId );
        CopyToExportAreaExtraChar();
    }
}

int CExport::ExportRecTypeVarName() {
    ASSERT( UseSeparator() );

    CString csVarName=_T("REC_TYPE");

    CopyToExportArea( csVarName, csVarName.GetLength() );
    CopyToExportAreaExtraChar();
    return csVarName.GetLength() + 1;
}

int CExport::ExportCaseIdVarNames( bool bCopyToRecArea ) {
    ASSERT( UseSeparator() );

    EXP_HEADER_NODE*    pHeadNode = GetHeadNode();

    ASSERT( pHeadNode->m_bHasCaseId );

    int        iSize=0;
    for( int i = 0;  i < pHeadNode->m_iNumCaseId; i++ ) {
        int iSymVar = m_iCaseIdItems[i];

        ASSERT( NPT(iSymVar)->IsA(SymbolType::Variable) );

        VART* pVarT=VPT(iSymVar);
        ASSERT( !pVarT->IsArray() );

        CString csVarName = WS2CS(pVarT->GetName());

        if( bCopyToRecArea ) {
            CopyToExportArea( csVarName, csVarName.GetLength() );
            CopyToExportAreaExtraChar();
            iSize += csVarName.GetLength()+1;
        }
        else
            iSize += std::max( csVarName.GetLength(), pVarT->GetLength() ) + 1;
    }

    return iSize;
}

// RHF INIC Feb 16, 2005 NEW_EXPORT
void CExport::ExportCaseId()
{
    // can be alphanumeric Unicode export and can have a separator comma, tab, or semicolon
    const size_t TrueMaxDictKeySize = MAX_DICTKEYSIZE * 4;
    TCHAR pszCaseId[TrueMaxDictKeySize + 128];
    bool bCheckLevel = true;

    int iInitialIndex = GetRecIndex();
    TCHAR* pTarget = m_pExpRecArea + iInitialIndex;
    EnsembledTrip( &CExport::Export_Data, true, &bCheckLevel, &m_iCaseIdItems, &m_aCaseIdOccExpr, &m_aCaseIdOccExprProcSymbol );

    int iFinalLen = GetRecIndex() - iInitialIndex;

    ASSERT( iFinalLen <= TrueMaxDictKeySize );

    _tmemcpy( pszCaseId, pTarget, iFinalLen );
    pszCaseId[iFinalLen] = 0;

    ASSERT( _tcslen(pszCaseId) <= TrueMaxDictKeySize );

    if( m_pRecordCSPRO != NULL ) {
        //Calculate maxocc
        bool    bNewCase=(m_csLastCaseId.Compare(pszCaseId)!=0);
        if( bNewCase )
            m_iCurOcc = 1;
        else
            m_iCurOcc++;

        int iMaxOcc = std::max( m_pRecordCSPRO->GetMaxRecs(), (UINT) m_iCurOcc );

        m_pRecordCSPRO->SetMaxRecs( iMaxOcc );

        m_csLastCaseId = pszCaseId;
    }
}
// RHF END Feb 16, 2005 NEW_EXPORT

void CExport::CopyToExportArea( const TCHAR* pBuff, int iSize ) {
    csprochar*   pTarget = m_pExpRecArea + GetRecIndex();

    if( pBuff != NULL )
        _tmemcpy( pTarget, pBuff, iSize );
    else
        _tmemset( pTarget, _T(' '), iSize );

    AdvanceRecIndex( iSize );

    ASSERT( m_iExpRecNext < m_iExpRecMax );
}

int CExport::CopyToExportAreaExtraChar()
{
    const TCHAR separator_ch = GetSeparatorChar();

    if( separator_ch != 0 )
    {
        CopyToExportArea(&separator_ch, 1);
        return 1;
    }

    return 0;
}

bool CExport::MakeDefaultCaseId(  int iUntilLevel ) {
    EXP_HEADER_NODE*    pHeadNode = GetHeadNode();

    if( pHeadNode->m_iLenCaseId == 0 ) {
        ASSERT( pHeadNode->m_iNumCaseId == 0 );
        DICT*    pDicT=DIP(0);

        ASSERT( pHeadNode->m_iExportLevel >= 1 );
        ASSERT( iUntilLevel >= 1 && iUntilLevel <= (int)MaxNumberLevels );

        ASSERT( iUntilLevel <= pDicT->maxlevel );

        // Level n includes case id items from level 1 until level n
        int        iSymVar;
        for( int iLevel=0; iLevel < iUntilLevel; iLevel++ ) {
            for( int i = 0; ( iSymVar = m_pEngineArea->m_pEngineSettings->m_QidVars[iLevel][i] ) > 0; i++ ) {
                VART*   pVarT   = VPT(iSymVar);

                CString csItemName = WS2CS(pVarT->GetName());
                int        iDummy;
                if( this->m_aItemNames.Lookup( csItemName, iDummy ) )
                    return false;
                this->m_aItemNames.SetAt( csItemName, 0 );

                m_iCaseIdItems[pHeadNode->m_iNumCaseId] = iSymVar;
                pHeadNode->m_iLenCaseId += pVarT->GetLength();
                pHeadNode->m_iNumCaseId++;
            }
        }
    }

    return true;
}

void CExport::MakeCommonRecord( CDictRecord* pDictCommonRecord, int iFromLevel, int iToLevel ) {
    EXP_HEADER_NODE*    pHeadNode = GetHeadNode();
    ASSERT( pHeadNode->m_iLenCaseId > 0 ); // CSPRO must have case id
    ASSERT( pHeadNode->m_iNumCaseId > 0 ); // CSPRO must have case id
    ASSERT( pHeadNode->m_iExportLevel >= 1 );

    ASSERT( pDictCommonRecord->GetNumItems() == 0 );

    int        iLoc=m_pHeadNode->m_bCaseIdAfterRecType ? 1+m_pHeadNode->m_iLenRecId : 1;

    for( int i = 0; i < pHeadNode->m_iNumCaseId; i++ ) {
        int     iSymVar = m_iCaseIdItems[i];
        VART*   pVarT   = VPT(iSymVar);
        //Commented by Savy (R) 20090630
        //if( pVarT->GetLevel() >= iFromLevel && pVarT->GetLevel() <= iToLevel ) {
        //Added by Savy (R) 20090702
        //Fix for case id contains Working dictionary item
        if(( pVarT->GetLevel() >= iFromLevel && pVarT->GetLevel() <= iToLevel ) || pVarT->GetLevel() ==0){

            const CDictItem* pDictItem=pVarT->GetDictItem();

            const_cast<CDictItem*>(pDictItem)->SetStart( iLoc ); // DD_STD_REFACTOR_TODO why is this necessary?
            iLoc += pVarT->GetLength();

            pDictCommonRecord->AddItem(pDictItem);
        }
        else {
            iLoc += pVarT->GetLength();
        }
    }
}

void CExport::RemoveFiles() {
    CNPifFile* pPifFile = m_pEngineDriver->GetPifFile();
    CString csFilename;

    // exported data-file
    if( GetExportToDat() ) {
        csFilename = GetExpoName();

        //SAVY .DAT is used anyway if the file name is empty
        if (!pPifFile->GetExportFilenames().empty())
            csFilename = pPifFile->GetExportFilenames().front();// Chirag Mar 13, 2002

        PortableFunctions::FileDelete( csFilename );
    }

    // SPSS codebook file
    if( GetExportToSPSS() ) {
        if (pPifFile->GetSPSSSyntaxFName().IsEmpty())  // Chirag Mar 13, 2002
            csFilename = PortableFunctions::PathRemoveFileExtension<CString>(GetExpoName()) + FileExtensions::WithDot::SpssSyntax;
        else                                           // Chirag Mar 13, 2002
            csFilename = pPifFile->GetSPSSSyntaxFName(); // Chirag Mar 13, 2002
        PortableFunctions::FileDelete( csFilename );
    }

    // SAS codebook file
    if( GetExportToSAS() ) {
        if (pPifFile->GetSASSyntaxFName().IsEmpty())  // Chirag Mar 13, 2002
            csFilename = PortableFunctions::PathRemoveFileExtension<CString>(GetExpoName()) + FileExtensions::WithDot::SasSyntax;
        else                                                 // Chirag Mar 13, 2002
            csFilename = pPifFile->GetSASSyntaxFName(); // Chirag Mar 13, 2002
        PortableFunctions::FileDelete( csFilename );
    }

    // STATA (DCT & DO) files
    if( GetExportToSTATA() ) {
        // DCT file
        if (pPifFile->GetSTATASyntaxFName().IsEmpty())  // Chirag Mar 13, 2002
            csFilename = PortableFunctions::PathRemoveFileExtension<CString>(GetExpoName()) + FileExtensions::WithDot::StataDictionary;
        else                                       // Chirag Mar 13, 2002
            csFilename = pPifFile->GetSTATASyntaxFName(); // Chirag Mar 13, 2002

        PortableFunctions::FileDelete( csFilename );

        // DO file
        if (pPifFile->GetSTATADOFName().IsEmpty())  // Chirag Mar 13, 2002
            csFilename = PortableFunctions::PathRemoveFileExtension<CString>(GetExpoName()) + FileExtensions::WithDot::StataDo;
        else                                        // Chirag Mar 13, 2002
            csFilename = pPifFile->GetSTATADOFName();// Chirag Mar 13, 2002

        PortableFunctions::FileDelete( csFilename );

    }

    // CSPRO DataDict
    if( GetExportToCSPRO() ) {
        if (pPifFile->GetCSPROSyntaxFName().IsEmpty())  // Chirag Mar 13, 2002
            csFilename = PortableFunctions::PathRemoveFileExtension<CString>(GetExpoName()) + FileExtensions::WithDot::Dictionary;
        else                                           // Chirag Mar 13, 2002
            csFilename = pPifFile->GetCSPROSyntaxFName(); // Chirag Mar 13, 2002

        PortableFunctions::FileDelete( csFilename );
    }

    // R codebook file
    if( GetExportToR() ) {
        if (pPifFile->GetRSyntaxFName().IsEmpty())
            csFilename = PortableFunctions::PathRemoveFileExtension<CString>(GetExpoName()) + FileExtensions::WithDot::RSyntax;
        else
            csFilename = pPifFile->GetRSyntaxFName();
        PortableFunctions::FileDelete( csFilename );
    }
}
