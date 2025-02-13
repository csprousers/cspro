//---------------------------------------------------------------------
//
//  exportF1    execution functions required by exEXPORT
//
//---------------------------------------------------------------------
#include "StandardSystemIncludes.h"
#include "Export.h"
#include <zToolsO/NewlineSubstitutor.h>
#include <zToolsO/TextConverter.h>
#include <zToolsO/Tools.h>
#include "Engine.h"
#include "Export.h"

#ifdef _DEBUG
#undef THIS_FILE
static char THIS_FILE[]= __FILE__;
#define new DEBUG_NEW
#endif


void CIntDriver::ExpWriteThisExport( void ) {
    // ExpWriteThisExport: write output records following the program-strip
    CExport*            pCurExport = m_pEngineArea->GetCurExport();

    if( !pCurExport->IsExportActive() ) // check current export - must be "normally open"
        return;

    EXP_HEADER_NODE*    pHeadNode = pCurExport->GetHeadNode();
    char*               pExpoNode = pCurExport->GetFirstNode();
    EXP_ENSEMBLE_NODE*  pEnsembNode;

    if( !pCurExport->GetExportToDat() ) { // check if export-data-file requested
        pCurExport->CleanRecArea( 0 );      // clean all record-area

        // Export RecType & CaseId or CaseId & RecType
        if( pHeadNode->m_bCaseIdAfterRecType ) {
            if( pCurExport->HasRecType( pHeadNode ) )
                pCurExport->ExportRecType(true);

            if(pHeadNode->m_bHasCaseId )
                pCurExport->ExportCaseId();
        }
        else {
            if(pHeadNode->m_bHasCaseId )
                pCurExport->ExportCaseId();
            if( pCurExport->HasRecType( pHeadNode ) )
                pCurExport->ExportRecType( true );
        }
        return;
    }

    int   iValidSize = 0;

    int   i;
    while( *pExpoNode != Exp_EOCASE ) {
        switch( *pExpoNode ) {

            case Exp_ENSEMBLE: //ExpWriteThisExport
                pEnsembNode = (EXP_ENSEMBLE_NODE*) pExpoNode;
                pExpoNode += sizeof(EXP_ENSEMBLE_NODE);

                // Fix problem in export with multiple record from workdict,
                // external or a record from the input dict that is filled by the application (no data)
                for( i=0; i < pCurExport->m_aUsedRecords.GetSize(); i++ ) {
                    int iSymSec = pCurExport->m_aUsedRecords[i];
                    m_pEngineDriver->SetupSectOccsFromGroups( iSymSec, 0 );
                }

                pCurExport->CleanRecArea( 0 );      // clean all record-area

                // RHF INIC Nov 08, 2004
                //Export Items names
                if( pCurExport->UseSeparator() && !pCurExport->GetNamesExported() ) {
                    iValidSize = 0;

                    if( pHeadNode->m_bCaseIdAfterRecType ) {
                        if( pCurExport->HasRecType( pHeadNode ) )
                            iValidSize += pCurExport->ExportRecTypeVarName();
                        if(pHeadNode->m_bHasCaseId )
                            iValidSize += pCurExport->ExportCaseIdVarNames(true);
                    }
                    else {
                        if(pHeadNode->m_bHasCaseId )
                            iValidSize += pCurExport->ExportCaseIdVarNames(true);
                        if( pCurExport->HasRecType( pHeadNode ) )
                            iValidSize += pCurExport->ExportRecTypeVarName();
                    }

                    iValidSize += pCurExport->EnsembledTrip( &CExport::Export_Names, false );
                    if( iValidSize > 0 )
                        ExpWrite1Record();
                    pCurExport->CleanRecArea( 0 );      // clean all record-area

                    pCurExport->SetNamesExported(true);
                }
                // RHF END Nov 08, 2004


                // Export RecType & CaseId or CaseId & RecType
                if( pHeadNode->m_bCaseIdAfterRecType ) {
                    if( pCurExport->HasRecType( pHeadNode ) )
                        pCurExport->ExportRecType(true);

                    if(pHeadNode->m_bHasCaseId )
                        pCurExport->ExportCaseId();
                }
                else {
                    if(pHeadNode->m_bHasCaseId )
                        pCurExport->ExportCaseId();
                    if( pCurExport->HasRecType( pHeadNode ) )
                        pCurExport->ExportRecType(true);
                }

                iValidSize = pCurExport->EnsembledTrip( &CExport::Export_Data, true );

                break;

            case Exp_EOENSEMBLE:
                pExpoNode += 1;

                if( iValidSize > 0 )
                    ExpWrite1Record();
                break;

            default:
                ASSERT(0);
                break;
        }
    }
}


void CIntDriver::ExpWrite1Record()
{
    // ExpWrite1Record: write record stored in m_pExpRecArea ('GetRecIndex()' bytes are currently filled)
    CExport* pCurExport = m_pEngineArea->GetCurExport();

    std::wstring exported_line(pCurExport->m_pExpRecArea, pCurExport->GetRecIndex());

    if( !exported_line.empty() )
    {
        // Delete extra delimiter
        const TCHAR separator_ch = pCurExport->GetSeparatorChar();

        if( separator_ch != 0 )
            SO::MakeTrimRight(exported_line, separator_ch);

        if( !exported_line.empty() )
        {
            // turn \n -> ␤
            NewlineSubstitutor::MakeNewlineToUnicodeNL(exported_line);

            // write the record
            if( pCurExport->GetExportForceANSI() )
            {
                const std::string ansi_buffer = TextConverter::WideToWindowsAnsi(exported_line);
                fwrite(ansi_buffer.c_str(), ansi_buffer.length(), 1, pCurExport->m_pFileDat);
            }

            else
            {
                fwrite(exported_line.c_str(), exported_line.length() * sizeof(exported_line[0]), 1, pCurExport->m_pFileDat);
            }
        }
    }

    _puttc( NL, pCurExport->m_pFileDat );
}
