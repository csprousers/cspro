#include "StandardSystemIncludes.h"
#include "Engine.h"
#include "Ctab.h"
#include "VARX.h"


#ifdef _DEBUG
#undef THIS_FILE
static char THIS_FILE[]= __FILE__;
#define new DEBUG_NEW
#endif


void CEngineArea::FreeTables()
{
    for( GROUPT* pGroupT : m_engineData->groups )
    {
        pGroupT->DeleteCDEGroup();

        // release list of items of this group      // VC/RHF 25/11/99
        pGroupT->DestroyItemList();                 // VC/RHF 25/11/99
    }

    // crosstabs
    if( CtNodebase != NULL )
    {
        free( CtNodebase );
        CtNodebase = NULL;
    }

    m_engineData->Clear();

    m_persistentSymbolsNeedingResetSet.clear();
}


void CEngineArea::tablesend()
{
#ifndef USE_BINARY
    if( Issamod == ModuleType::Designer )
        ExportFinish();  // RHF Oct 05, 2004
#endif // USE_BINARY

    if( Dicxbase != NULL )
    {
        for( DICT* pDicT : m_engineData->dictionaries_pre80 )
        {
            DICX* pDicX = pDicT->GetDicX();

            // a.- releasing container of 3 keys
            if( pDicX->m_pKeyHolderArea != NULL ) {
                free( pDicX->m_pKeyHolderArea );
                pDicX->m_pKeyHolderArea = NULL;
            }

            // b.- releasing container of record read
            csprochar* pRecArea = pDicX->GetRecordArea();

            if( pRecArea != NULL ) {
                free( pRecArea );
                pDicX->SetRecordArea( NULL );
            }
        }

        DicxEnd();                                      // RHF 16/9/99
        free( Dicxbase );
        Dicxbase = NULL;
    }

    if( Secxbase != NULL ) {
        SecxEnd();                                      // RHF 25/11/99
        free( Secxbase );
        Secxbase = NULL;
    }

    if( Varxbase != NULL ) {
        free( Varxbase );
        Varxbase = NULL;
     }

    FreeTables();
}

#ifndef USE_BINARY
//static
CDataDict* CExport::ExportGetDataDict( CString csDataFileName, CMap<CString,LPCTSTR,CDictExport,CDictExport&>& aDataDicts, CExport* pExport ) {
    CDataDict*  pDataDict = NULL;
    CDictExport cDictExport;

    if( !aDataDicts.Lookup(csDataFileName, cDictExport ) ) {
        pDataDict = new CDataDict;

        pDataDict->SetPosRelative(false);

        cDictExport.m_pDataDict = pDataDict;
        cDictExport.m_pExport = pExport;

        aDataDicts.SetAt(csDataFileName, cDictExport );
    }

    return cDictExport.m_pDataDict;
}


bool CExport::ExportAddDictRecord(CDataDict* pDataDict, CString csFileName, CString& csErrorMsg ) {

    int iExportLevel=this->m_pHeadNode->m_iExportLevel;
    int iExportAppLevel=this->m_pHeadNode->m_iExportAppLevel;

    if( iExportLevel == 0 ) {
        csErrorMsg.Format( _T("No elements to export") );
        return false;
    }

    CDictRecord*    pDictRecord=m_pRecordCSPRO;

    DICT* pDicT = DIP(0);
    const CDataDict* pInputDataDict = pDicT->GetDataDict();

    //bool  bIsCommonRecord=(pDictRecord->GetSonNumber() == COMMON);
    ASSERT( iExportLevel >= 1 && iExportLevel <= 3 );

    //Check the order: First all level 1 records are exported, then all level 2 records and the all level 3 records
    if( iExportLevel < (int)pDataDict->GetNumLevels() ) {
        csErrorMsg.Format( _T("Records must be ordered by level") );
        return false;
    }

    bool    bRet=true;

    // Export in level 1 & 2, 1 & 3 or 2 &3 --> generate level 1 & 2
    // Export in level 1  or 2 or 3         --> generate level 1

    // Add new Level
    int     iNewLevel=iExportLevel;
    if( (int)pDataDict->GetNumLevels() < iExportLevel ) {
        iNewLevel = (int)pDataDict->GetNumLevels() + 1;

        // RHF INIC Feb 21, 2005
        if( iExportLevel > iNewLevel ) {
            csErrorMsg.Format( _T("Invalid level. Trying to generate a level %d in the dictionary but there is no level %d"),
                            iExportLevel, iNewLevel );
            return false;
        }
        // RHF END Feb 21, 2005

        DictLevel cLevel;
        CIMSAString sFileN(csFileName);
        PathStripPath(sFileN.GetBuffer(sFileN.GetLength()));
        sFileN.ReleaseBuffer();
        CIMSAString sFile  = sFileN.GetToken(_T("."));

        if( iNewLevel == 1 )
            pDataDict->SetLabel(sFile);

        sFile.MakeName();
        if (sFile.GetLength() > 24) {
            sFile = sFile.Left(24);
        }

        if( iNewLevel == 1 ) {
            pDataDict->SetName(sFile + _T("_DICT"));
            pDataDict->SetNote(_T(""));

            int     iRecTypeStart = 0;
            int     iRecTypeLen = m_pHeadNode->m_iLenRecId;

            if( iRecTypeLen > 0 )
                iRecTypeStart = m_pHeadNode->m_bCaseIdAfterRecType ? 1 : 1 + m_pHeadNode->m_iLenCaseId;

            pDataDict->SetRecTypeStart(iRecTypeStart);
            pDataDict->SetRecTypeLen(iRecTypeLen);

            // RHF COM Feb 16, 2005 pDataDict->SetPosRelative(pInputDataDict->IsPosRelative() );
            pDataDict->SetZeroFill(pInputDataDict->IsZeroFill());
            pDataDict->SetDecChar(pInputDataDict->IsDecChar());
        }

        // Add Level
        // RHF INIC Feb 03, 2005
        const DictLevel& input_dict_level = pInputDataDict->GetLevel(iExportLevel - 1);

        cLevel.SetLabel( input_dict_level.GetLabel() );
        cLevel.SetName( input_dict_level.GetName() );
        cLevel.SetNote( input_dict_level.GetNote() );
        // RHF END Feb 03, 2005

        // RHF COM Feb 03, 2005cLevel.SetLabel(pDataDict->GetLabel() + pszLevelLabel[iExportLevel-1] );
        // RHF COM Feb 03, 2005cLevel.SetName(sFile + pszLevelName[iExportLevel-1] );

        MakeCommonRecord( cLevel.GetIdItemsRec(), iNewLevel, iExportAppLevel );

        pDataDict->AddLevel(std::move(cLevel));
    }
    else {
        //pDictLevel = pDataDict->GetLevel(iExportLevel-1);
    }


    // Add the record
    if( SO::IsBlank(pDictRecord->GetLabel()) ) // GHM 20120504 added this condition so that labels from the input dictionary aren't overwritten
        pDictRecord->SetLabel( pDictRecord->GetName() ); // + " record" );

    pDictRecord->SetRecTypeVal( m_pszSectionCode );

    pDictRecord->SetDataDict( pDataDict );
    //pDictRecord->SetRecLen:
    //pDictRecord->SetRequired: Default is true
    //pDictRecord->SetMaxRecs: Already filled
    // Add in notes from merged records
    if (m_mapRecNotes.GetCount() == 1) {
        // only 1 record, just copy note
        pDictRecord->SetNote(m_mapRecNotes.PGetFirstAssoc()->value);
    } else if (m_mapRecNotes.GetCount() > 1) {
        // merge notes from multiple records
        POSITION pos = m_mapRecNotes.GetStartPosition();
        CIMSAString sMergedNote;
        while (pos != NULL)
        {
            CString sRecName;
            CString sNote;
            m_mapRecNotes.GetNextAssoc( pos, sRecName, sNote );
            if (pos != m_mapRecNotes.GetStartPosition()) {
                sMergedNote += _T("\n\n");
            }
            sMergedNote += sRecName + _T("\n") + sNote;
        }
        pDictRecord->SetNote(sMergedNote);
    } else {
        pDictRecord->SetNote(CIMSAString());
    }

    DictLevel& dict_level = pDataDict->GetLevel(iNewLevel - 1);
    dict_level.AddRecord(pDictRecord);

    pDictRecord->SetLevel(&dict_level);

    return bRet;
}

void CExport::ExportGenRecNameAndType( CDataDict* pDataDict ) {
    int     iMaxRecTypeVal=0;
    int     iDummy=0;
    CMap<CString,LPCTSTR,int,int>   aUsedRecTypes;

    // Calculate iMaxRecords and the max length for the record type (iMaxRecTypeVal)
    int iMaxRecords=0;
    for( const DictLevel& dict_level : pDataDict->GetLevels() ) {
        for( int iRecord=0; iRecord < dict_level.GetNumRecords(); iRecord++ ) {
            const CDictRecord* pDictRecord = dict_level.GetRecord(iRecord);

            if( !SO::IsBlank(pDictRecord->GetRecTypeVal()) ) {
                iMaxRecTypeVal = std::max( iMaxRecTypeVal, pDictRecord->GetRecTypeVal().GetLength() );
                aUsedRecTypes.SetAt(pDictRecord->GetRecTypeVal(), iDummy );
            }

            iMaxRecords++;
        }
    }

    std::vector<CString> unique_names = pDataDict->GetUniqueNames(_T("RECORD"), IntToStringLength(iMaxRecords), iMaxRecords);

    //ASSERT( iMaxRecTypeVal == m_pHeadNode->m_iLenRecId );
    // No iMaxRecTypeVal = max( iMaxRecTypeVal, IntToStringLength(iMaxRecords) );

    int iMinRecordType = 1;
    int iRecNum = 0;
    // Gen record names and record types
    for( DictLevel& dict_level : pDataDict->GetLevels() ) {
        for( int iRecord=0; iRecord < dict_level.GetNumRecords(); iRecord++ ) {
            CDictRecord* pDictRecord = dict_level.GetRecord(iRecord);

            // Generate a record name
            if( SO::IsBlank(pDictRecord->GetName()) ) {
                const CString& csRecordName = unique_names[iRecNum];
                pDictRecord->SetName( csRecordName );
                pDictRecord->SetLabel( csRecordName );
            }

            iRecNum++;

            // Generate a record type
            if( SO::IsBlank(pDictRecord->GetRecTypeVal()) && iMaxRecords > 1 ) { // RHF Nov 09, 2004 Add iMaxRecords > 1
                CString csRecordType;

                // Try to insert the record name
                for( int iCheckRecType=iMinRecordType;;iCheckRecType++ ) {
                    csRecordType.Format( _T("%0*d"), IntToStringLength(iMaxRecTypeVal), iCheckRecType );

                    iMinRecordType++;
                    if( !aUsedRecTypes.Lookup( csRecordType, iDummy) )
                        break;
                }

                aUsedRecTypes.SetAt( csRecordType, iDummy);

                pDictRecord->SetRecTypeVal( csRecordType );
            }
        }
    }
}


void CEngineArea::ExportFinish()
{
    // ExportFinish: generate descriptions and delete export support
    // named export (multi-exports)
    int     iNumExports = m_pEngineArea->ExportGetSize();
    CString csFileName;

    CMap<CString,LPCTSTR,CDictExport,CDictExport&> aDataDicts;

    // RHF INIC Feb 25, 2005 Change export level
    for( int iExpoSlot = 0; iExpoSlot < iNumExports; iExpoSlot++ ) {
        CExport*    pExport = m_pEngineArea->ExportGetAt( iExpoSlot );
        bool        bPrimaryExport=pExport->m_bNeedClose;

        if( bPrimaryExport && pExport->IsExportActive() && pExport->GetExportToCSPRO() ) {
            CExport*    pNewExport=pExport;
            bool        aUsedLevel[MaxNumberLevels] = { false, false, false};

            //Get levels of duplicated exports
            while( pNewExport != NULL ) {
                int iExportLevel =  pNewExport->m_pHeadNode->m_iExportLevel;

                if( iExportLevel >= 1 && iExportLevel<= 3 )
                    aUsedLevel[iExportLevel-1]  = true;
                else
                    ASSERT(0);

                pNewExport = pNewExport->GetNextExport();

                while( pNewExport != NULL && !pNewExport->IsExportActive() )
                    pNewExport = pNewExport->GetNextExport();
            }

            // Calculate compressed level
            int iCompressedLevel[MaxNumberLevels] = { -1, -1, -1 };

            if( aUsedLevel[0] && aUsedLevel[1] && aUsedLevel[2]  && aUsedLevel[3] ) { //1234 used
                iCompressedLevel[0] = 0; iCompressedLevel[1] = 1;   iCompressedLevel[2] = 2; iCompressedLevel[3] = 3;
                ASSERT(0); // Dictionaries only has until 3 levels
            }
            else if( aUsedLevel[0] && aUsedLevel[1] && aUsedLevel[2] ) { //123 used
                iCompressedLevel[0] = 0; iCompressedLevel[1] = 1;   iCompressedLevel[2] = 2;
            }
            else if( aUsedLevel[0] && aUsedLevel[1] ) { //12 used
                iCompressedLevel[0] = 0; iCompressedLevel[1] = 1;
            }
            else if( aUsedLevel[0] && aUsedLevel[2] ) { //13 used
                iCompressedLevel[0] = 0; iCompressedLevel[1] = -1; iCompressedLevel[2] = 1;
            }
            else if( aUsedLevel[1] && aUsedLevel[2] ) { //23 used
                iCompressedLevel[0] = -1; iCompressedLevel[1] = 0; iCompressedLevel[2] = 1;
            }
            else if( aUsedLevel[0] ) {// 1 used
                iCompressedLevel[0] = 0;
            }
            else if( aUsedLevel[1] ) { // 2 used
                iCompressedLevel[0] = -1; iCompressedLevel[1] = 0;
            }
            else if( aUsedLevel[2] ) { // 3 used
                iCompressedLevel[0] = -1; iCompressedLevel[1] = -1; iCompressedLevel[2] = 0;
            }
            else {
                //  ASSERT(0);
                //      iCompressedLevel[0] = -1; iCompressedLevel[1] = -1; iCompressedLevel[2] = -1; iCompressedLevel[3] = -1;
            }

            //Change the level
            pNewExport=pExport;

            while( pNewExport != NULL ) {
                if( pNewExport->IsExportActive() ) {
                    int iExportLevel = pNewExport->m_pHeadNode->m_iExportLevel;
                    if( iExportLevel >= 1 && iExportLevel<= 3 && iCompressedLevel[iExportLevel-1] >= 0 )
                        pNewExport->m_pHeadNode->m_iExportLevel = iCompressedLevel[iExportLevel-1]+1;
                }
                pNewExport = pNewExport->GetNextExport();
            }

            /* RHF COM INIC Feb 25, 2005
            int iExportLevel =  pExport->m_pHeadNode->m_iExportLevel;
            if( iExportLevel >= 1 && iExportLevel<= 3 && iCompressedLevel[iExportLevel-1] >= 0 )
                pExport->m_pHeadNode->m_iExportLevel = iCompressedLevel[iExportLevel-1]+1;
                RHF COM END Feb 25, 2005 */

        }
    }
    // RHF END Feb 25, 2005


    /* RHF COM INIC Feb 25, 2005
    // RHF INIC Feb 21, 2005
    int aUsedLevel[MaxNumberLevels];

    for( int i=0; i < (int)MaxNumberLevels; i++ )
        aUsedLevel[i] = false;

    for( int iExpoSlot = 0; iExpoSlot < iNumExports; iExpoSlot++ ) {
        CExport*    pExport = m_pEngineArea->ExportGetAt( iExpoSlot );
        if( pExport->IsExportActive() && pExport->GetExportToCSPRO() ) {
            int iExportLevel =  pExport->m_pHeadNode->m_iExportLevel;

            if( iExportLevel >= 1 && iExportLevel<= 3 )
                aUsedLevel[iExportLevel-1]  = true;
            else
            ASSERT(0);
        }
    }

    int iCompressedLevel[MaxNumberLevels] = { -1, -1, -1 };

    if( aUsedLevel[0] && aUsedLevel[1] && aUsedLevel[2]  && aUsedLevel[3] ) { //1234 used
        iCompressedLevel[0] = 0; iCompressedLevel[1] = 1;   iCompressedLevel[2] = 2; iCompressedLevel[3] = 3;
        ASSERT(0); // Dictionaries only has until 3 levels
    }
    else if( aUsedLevel[0] && aUsedLevel[1] && aUsedLevel[2] ) { //123 used
        iCompressedLevel[0] = 0; iCompressedLevel[1] = 1;   iCompressedLevel[2] = 2;
    }
    else if( aUsedLevel[0] && aUsedLevel[1] ) { //12 used
        iCompressedLevel[0] = 0; iCompressedLevel[1] = 1;
    }
    else if( aUsedLevel[0] && aUsedLevel[2] ) { //13 used
        iCompressedLevel[0] = 0; iCompressedLevel[1] = -1; iCompressedLevel[2] = 1;
    }
    else if( aUsedLevel[1] && aUsedLevel[2] ) { //23 used
        iCompressedLevel[0] = -1; iCompressedLevel[1] = 0; iCompressedLevel[2] = 1;
    }
    else if( aUsedLevel[0] ) {// 1 used
        iCompressedLevel[0] = 0;
    }
    else if( aUsedLevel[1] ) { // 2 used
        iCompressedLevel[0] = -1; iCompressedLevel[1] = 0;
    }
    else if( aUsedLevel[2] ) { // 3 used
        iCompressedLevel[0] = -1; iCompressedLevel[1] = -1; iCompressedLevel[2] = 0;
    }
    else {
    //  ASSERT(0);
//      iCompressedLevel[0] = -1; iCompressedLevel[1] = -1; iCompressedLevel[2] = -1; iCompressedLevel[3] = -1;
    }

    for( iExpoSlot = 0; iExpoSlot < iNumExports; iExpoSlot++ ) {
        CExport*    pExport = m_pEngineArea->ExportGetAt( iExpoSlot );
        if( pExport->IsExportActive() && pExport->GetExportToCSPRO() ) {
            int iExportLevel =  pExport->m_pHeadNode->m_iExportLevel;

            if( iExportLevel >= 1 && iExportLevel<= 3 && iCompressedLevel[iExportLevel-1] >= 0 )
                pExport->m_pHeadNode->m_iExportLevel = iCompressedLevel[iExportLevel-1]+1;
        }
    }
    // RHF END Feb 21, 2005
    RHF COM END Feb 25, 2005 */

    for( int iExpoSlot = 0; iExpoSlot < iNumExports; iExpoSlot++ ) {
        CExport*    pExport = m_pEngineArea->ExportGetAt( iExpoSlot );

        if( pExport->IsExportActive() ) {
            pExport->ExportDescriptions();

            // Merge DictRecords
            if( pExport->GetExportToCSPRO() ) {
                csFileName = pExport->GetDcfExpoName();

                CDataDict* pDataDict = CExport::ExportGetDataDict( csFileName, aDataDicts, pExport );

                if( pDataDict != NULL ) {
                    CString     csErrorMsg;

                //GetExport level
                /* RHF COM INIC Feb 16, 2005
                int     iExportLevel=0;
                for( int i=0; i < pExport->m_aUsedRecords.GetSize(); i++ ) {
                     int iSymSec=pExport->m_aUsedRecords.GetAt(i);

                     SECT*  pSecT=SPT(iSymSec);

                     iExportLevel = max( iExportLevel, pSecT->GetLevel() );
                }
                RHF COM END Feb 16, 2005 */

                    //Add record
                    if( !pExport->ExportAddDictRecord( pDataDict, csFileName, csErrorMsg ) ) {
                        issaerror( MessageType::Warning, 31040, (LPCTSTR)csFileName, (LPCTSTR)csErrorMsg );
                        delete pDataDict;

                        pExport->SetRemoveFiles(true);
                        PortableFunctions::FileDelete( csFileName );

                        CDictExport cDictExport;

                        cDictExport.m_pDataDict = NULL;
                        cDictExport.m_pExport = pExport;

                        aDataDicts.SetAt(csFileName, cDictExport );
                    }
                }
            }
        }

        pExport->ExportClose();
        // RHF COM Feb 09, 2005 delete pExport;
    }

    //Save Data Dictionaries
    POSITION pos = aDataDicts.GetStartPosition();
    while( pos ) {
        CString     csDataFileName;
        CDictExport cDictExport;

        aDataDicts.GetNextAssoc( pos, csDataFileName, cDictExport ) ;

        CDataDict*  pDataDict=cDictExport.m_pDataDict;
        CExport*    pExport=cDictExport.m_pExport;

        if( pDataDict != NULL ) {
            CExport::ExportGenRecNameAndType( pDataDict );

            //Calculate record length
            for( DictLevel& dict_level : pDataDict->GetLevels() ) {
                for( int iRec=0; iRec < dict_level.GetNumRecords(); iRec++ ) {
                    CDictRecord* pDictRecord = dict_level.GetRecord(iRec);
                    pDictRecord->SetDataDict( pDataDict );
                    pDictRecord->CalculateRecLen();
                }
            }

            // save the dictionary if CSPro export is enabled
            // (not if it was just for metadata)
            if (pExport->GetExportToCSPRO()) {
#ifdef USE_BINARY
                ASSERT(0);
#else
                try
                {
                    pDataDict->Save(csDataFileName);
                }

                catch( const CSProException& )
                {
                    issaerror(MessageType::Warning, 31073, (LPCTSTR)csDataFileName);
                }

#endif // USE_BINARY

                // RHF INIC Feb 03, 2005
                CIMSAString sError;

                if( !pDataDict->IsValid( sError ) ) {
                    sError.TrimRight();
                    issaerror( MessageType::Warning, 31040, (LPCTSTR)csDataFileName, (LPCTSTR)sError );

                    pExport->SetRemoveFiles(true);
                    PortableFunctions::FileDelete( csDataFileName );
                }
                // RHF END Feb 03, 2005

                delete pDataDict;
            }
        }
    }

    for( int iExpoSlot = 0; iExpoSlot < iNumExports; iExpoSlot++ ) {
        CExport*    pExport = m_pEngineArea->ExportGetAt( iExpoSlot );

        if( pExport->GetRemoveFiles() ) {
            PortableFunctions::FileDelete( pExport->GetExpoName() );
        }

        delete pExport; // RHF Feb 09, 2005
    }

    m_pEngineArea->ClearExports();
}
#endif // !USE_BINARY
