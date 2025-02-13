// RunTab.cpp: implementation of the CRunTab class.
//
//////////////////////////////////////////////////////////////////////

#include "StdAfx.h"
#include "RunTab.h"
#include "PifDlgBuilder.h"
#include "TabExecutionDlg.h"
#include <zUtilO/Filedlg.h>
#include <zUtilO/ProcessSummary.h>
#include <zUtilO/SimpleDbMap.h>
#include <zUtilF/ProgressDlg.h>
#include <zMessageO/Messages.h>
#include <zMessageO/MessageSummary.h>
#include <zListingO/HeaderAttribute.h>
#include <zListingO/ListerWriteFile.h>
#include <ZBRIDGEO/npff.h>
#include <zBatchO/Runaplb.h>
#include <zBatchO/RunAplC.h>
#include <ZTBDO/TbdFileM.h>
#include <engine/ttype.h>
#include <iostream>


#ifdef _DEBUG
#undef THIS_FILE
static char THIS_FILE[]= __FILE__;
#define new DEBUG_NEW
#endif

/////////////////////////////////////////////////////////////////////////////////
//
//      static inline int compare(const void* elem1, const void* elem2)
//
/////////////////////////////////////////////////////////////////////////////////
static inline int compare(const void* elem1, const void* elem2)
{
        CString s1 = *((CString*)elem1);
        s1.Replace(_T(' '), '0');s1.Replace(_T('-'), '0');
        CString s2 = *((CString*)elem2);
        s2.Replace(_T(' '), '0');s2.Replace(_T('-'), '0');
        return(s1.Compare(s2));
}
IMPLEMENT_DYNAMIC(CRunTab,CObject);

//////////////////////////////////////////////////////////////////////
// Construction/Destruction
//////////////////////////////////////////////////////////////////////

CRunTab::CRunTab()
{
    m_pPifFile = NULL;
    m_pApplication = NULL;
    m_engineIssued1008Messages = false;
    m_engineIssuedNon1008Messages = false;
}

CRunTab::~CRunTab()
{
    DeleteMultipleTBDFiles();
}
/////////////////////////////////////////////////////////////////////////////////
//
//  bool CRunTab::PreparePFF(CNPifFile* pPFFFile)
//
/////////////////////////////////////////////////////////////////////////////////
bool CRunTab::PreparePFF(CNPifFile* pPFFFile,PROCESS& eProcess,bool bHideAll/* = true*/)
{
    //prepare the input files;
    //First check which process to run
    pPFFFile->SetAppType(TAB_TYPE);
    bool bDisableCon = pPFFFile->GetApplication()->GetTabSpec()->GetConsolidate()->GetNumAreas() ==0;

    while (eProcess == PROCESS_INVALID) {
        CSelDlg selProcess;
        selProcess.m_bDisableCon = bDisableCon;
        selProcess.m_bHideAll = bHideAll;
        int iRet = selProcess.DoModal();
        if( iRet != IDOK)
            return false;
        eProcess = (PROCESS)selProcess.m_iProcess;
        if (eProcess == PROCESS_INVALID) {
            AfxMessageBox(_T("Select Process"));
        }
    }

    //Under new scheme .PFF Dlg comes all the time Bruce et al revision
    bool bNoOutputFiles4All = (eProcess == ALL_STUFF) && bHideAll;
    PifDlgBuilder pffDlgBuilder(pPFFFile);
    if (!pffDlgBuilder.BuildPifInfo(eProcess, bNoOutputFiles4All))
        return false;
    if (!pPFFFile->GetSilent()) {
        if (pffDlgBuilder.ShowModalDialog() != IDOK)
            return false;
    }
    if (!pffDlgBuilder.SaveAssociations())
        return false;

    if(eProcess == ALL_STUFF){
      /*  CIMSAString sTabOutPutFName = pPFFFile->GetTabOutputFName();
        CIMSAString sTempTab;
        if(sTabOutPutFName.IsEmpty()) {
            CIMSAString sAplFName = pPFFFile->GetAppFName();
            PathRemoveExtension(sAplFName.GetBuffer(_MAX_PATH));
            sAplFName.ReleaseBuffer();
            sTabOutPutFName = sAplFName + FileExtensions::BinaryTable::WithDot::Tab;
            pPFFFile->SetTabOutputFName(sTabOutPutFName);
            sTempTab = sAplFName + "_precalc" + FileExtensions::BinaryTable::WithDot::Tab ; // Engine is not looking at the piffile ags

        }
        if(pPFFFile->GetCalcInputFNamesArr().empty()) {
            pPFFFile->GetCalcInputFNamesArr().emplace_back(sTempTab);
        }
        CIMSAString sCalcOFName = pPFFFile->GetCalcOutputFName();
        if(sCalcOFName.IsEmpty()){
            pPFFFile->SetCalcOutputFName(sTabOutPutFName);
        }*/

    }

    return true;
}

/////////////////////////////////////////////////////////////////////////////////
//
//      void CRunTab::InitRun(CNPifFile *pPifFile, Application *pApp)
//
/////////////////////////////////////////////////////////////////////////////////
void CRunTab::InitRun(CNPifFile *pPifFile, Application *pApp)
{
    ASSERT(pPifFile); //Calling routine allocates this
    ASSERT(pApp); //Cant be NULL the calling  routine allocates stuff for these
    m_pPifFile = pPifFile;
    m_pApplication = pApp;
}
/////////////////////////////////////////////////////////////////////////////////
//
//      bool CRunTab::GetPFFArgs()
//
/////////////////////////////////////////////////////////////////////////////////
bool CRunTab::GetPFFArgs()
{
    //brings up the piff grid and prepares the pff returns false if the user pressed cancel
    //or else returns true for success
    return true;

}


/////////////////////////////////////////////////////////////////////////////////
//
//      bool CRunTab::ExecPrep()
//
/////////////////////////////////////////////////////////////////////////////////
bool CRunTab::ExecPrep()
{ //executes the Prep step
    //open the tbd
    //Get the TAB file ;
    //For each Table Convert the TAB to TBD

    //Open the tab file
    CFileStatus fStatus;
    CTbdFileMgr tfMgr;
    CTbdFile*   ptFile = NULL;

    CIMSAString sPath; //later on get it from the pff file ( //To Do design stuff for input/output of tab)
    sPath = m_pPifFile->GetAppFName();
    PathRemoveFileSpec(sPath.GetBuffer(MAX_PATH));
    sPath.ReleaseBuffer();
    sPath.TrimRight('\\');


    //Get the tab file .
    //CIMSAString sTabFileName =sPath+"\\" + "CSTab.tab";
    CIMSAString sTabFileName = m_pPifFile->GetTabOutputFName();
    if(sTabFileName.IsEmpty()){
        ASSERT(FALSE);
        if(!m_pPifFile->GetPrepInputFName().IsEmpty()){
            sTabFileName = m_pPifFile->GetPrepInputFName();
        }
    }
    //Create TAI file using our method if it does not exist
    CIMSAString sPrepInputTAI = sTabFileName;
    PathRemoveExtension(sPrepInputTAI.GetBuffer(_MAX_PATH));
    sPrepInputTAI.ReleaseBuffer();
    sPrepInputTAI = sPrepInputTAI + FileExtensions::BinaryTable::WithDot::TabIndex;
    if(!CFile::GetStatus(sPrepInputTAI,fStatus)){
        if(!CreateTAIFile(sTabFileName,sPrepInputTAI)){
            AfxMessageBox(_T("Failed to create input tabix file"));
            return false;
        }
    }
    if ( !tfMgr.Open(sTabFileName, false) ) {
        std::cerr << _T("File <") << sTabFileName.GetString() << _T("> not found.\n");
        return false;
    }

    ptFile = tfMgr.GetTbdFile(sTabFileName);
    //Get to the
    bool bRet = BuildTables(ptFile);
 //   SaveTables();
    tfMgr.CloseAll();
    if(bRet && !m_pPifFile->GetPrepOutputFName().IsEmpty()){
        CIMSAString sDictFName = m_pPifFile->GetApplication()->GetTabSpec()->GetDictFile();

        // GHM 20090915
        CIMSAString inputDataFilename;

        for( const ConnectionString& input_connection_string : m_pPifFile->GetInputDataConnectionStrings() )
        {
            if( input_connection_string.IsFilenamePresent() )
            {
                CString filename_only = PortableFunctions::PathGetFilename(input_connection_string.GetFilename());

                if( !inputDataFilename.IsEmpty() && ( inputDataFilename.GetLength() + filename_only.GetLength() ) <= XTS_DEFAULT_INPUTDATAFILENAMEHEADERLENGTH )
                    inputDataFilename.Append(_T(", "));

                else if( ( inputDataFilename.GetLength() + filename_only.GetLength() ) > XTS_DEFAULT_INPUTDATAFILENAMEHEADERLENGTH )
                {
                    inputDataFilename.Append(_T(", ..."));
                    break;
                }

                inputDataFilename.Append(filename_only);
            }
        }

        m_pPifFile->GetApplication()->GetTabSpec()->Save(m_pPifFile->GetPrepOutputFName(),sDictFName,inputDataFilename); //Save .TBW
    }
    return bRet;
}
/////////////////////////////////////////////////////////////////////////////////
//
//      bool CRunTab::ExecCon()
//
/////////////////////////////////////////////////////////////////////////////////
bool CRunTab::ExecCon()
{
    if(!m_pPifFile){
        return false;
    }
    //Get the application
    std::shared_ptr<CTabSet> pTableSpec = m_pApplication->GetTabSpec();
    ASSERT(pTableSpec);

    ASSERT(!m_pPifFile->GetConInputFilenames().empty());
    CIMSAString sConOutputFName = m_pPifFile->GetConOutputFName();

    //For now assume one input file
    //Check File existance
    CFileStatus fStatus;
    if(!CFile::GetStatus(m_pPifFile->GetConInputFilenames().front(),fStatus)){
        return false; //error processing later
    }

    CConsolidate* pConsolidate  = pTableSpec->GetConsolidate();
    if(!pConsolidate || (pConsolidate && pConsolidate->GetNumAreas()==0)){
        //nothing to be done
        //just return
        CIMSAString sConInputTAB = m_pPifFile->GetConInputFilenames().front();
        CopyFile(sConInputTAB,sConOutputFName,false);

        CIMSAString sConInputTAI = sConInputTAB;
        PathRemoveExtension(sConInputTAI.GetBuffer(_MAX_PATH));
        sConInputTAI.ReleaseBuffer();
        sConInputTAI = sConInputTAI + FileExtensions::BinaryTable::WithDot::TabIndex;

        CIMSAString sConOutputTAI = sConOutputFName;
        PathRemoveExtension(sConOutputTAI.GetBuffer(_MAX_PATH));
        sConOutputTAI.ReleaseBuffer();
        sConOutputTAI = sConOutputTAI + FileExtensions::BinaryTable::WithDot::TabIndex;

        CopyFile(sConInputTAI,sConOutputTAI,false);
        return true;
    }

    CTbdFile* pInputTbdFile = NULL;
    CTbiFile* pInputTbiFile = NULL;
    //goes through all slices of a table before going to second ??
    //if this does not work make it false
    bool bCreate = false;
    for(size_t iIndex =0; iIndex < m_pPifFile->GetConInputFilenames().size(); iIndex++){
        pInputTbdFile = new CTbdFile(m_pPifFile->GetConInputFilenames().at(iIndex));
        bCreate = false;
        if ( !pInputTbdFile->Open(bCreate) ) {
            pInputTbdFile->Close();
            SAFE_DELETE(pInputTbdFile);
            DeleteMultipleTBDFiles();
            return false;
        }
       m_arrConInputTBDFiles.Add(pInputTbdFile);
    }
    //Create TAI file using our method if it does not exist
    for(size_t iIndex =0; iIndex < m_pPifFile->GetConInputFilenames().size(); iIndex++){
        CIMSAString sConInputTAI = m_pPifFile->GetConInputFilenames().at(iIndex);
        PathRemoveExtension(sConInputTAI.GetBuffer(_MAX_PATH));
        sConInputTAI.ReleaseBuffer();
        sConInputTAI = sConInputTAI + FileExtensions::BinaryTable::WithDot::TabIndex;
        if(!CFile::GetStatus(sConInputTAI,fStatus)){
            if(!CreateTAIFile(m_pPifFile->GetConInputFilenames().at(iIndex),sConInputTAI)){
                AfxMessageBox(_T("Failed to create input tabidx file"));
                DeleteMultipleTBDFiles();
                return false;
            }
        }
        pInputTbiFile = new CTbiFile(sConInputTAI,20);// RHF Aug 07, 2001 Change from true to false
        bCreate = false;
        if ( !pInputTbiFile->Open(bCreate) ) {
            pInputTbiFile->Close();
            SAFE_DELETE(pInputTbiFile);
            DeleteMultipleTBDFiles();
            return false;
        }
        m_arrConInputTBIFiles.Add(pInputTbiFile);
    }
    //Get the Consolidate stuff
    //Create the output Tab
    //Create the new TBD File
    CTbdFile* pOutPutTBDFile = NULL;
    pOutPutTBDFile = new CTbdFile(sConOutputFName);

    //Finally Save and Close all the output tbd Files
    //Silly Way
    bCreate=true;
    if( !pOutPutTBDFile->Open( bCreate ) ) {//Create an empty trailer and initialize some vars
        pOutPutTBDFile->Close();
        SAFE_DELETE(pOutPutTBDFile);
        return false;
    }

    pOutPutTBDFile->Close();

    if( !pOutPutTBDFile->DoOpen( bCreate ) ) {// Don't create an empty trailer
        pOutPutTBDFile->Close();
        SAFE_DELETE(pOutPutTBDFile);
        return false;
    }


    auto process_summary = std::make_shared<ProcessSummary>();
    process_summary->SetAttributesType(ProcessSummary::AttributesType::Slices);
    process_summary->SetNumberLevels(0);
    process_summary->SetPercentSourceRead(100);

    // create the lister, which will only be used if there were no errors creating it
    std::shared_ptr<Listing::Lister> lister;

    try
    {
        lister = Listing::Lister::Create(process_summary, *m_pPifFile, true, nullptr);
    }
    catch( const CSProException& ) { }

    if( lister != nullptr )
        WriteConListingHeader(*lister);

    ProcessCon(/*pInputTbdFile,pInputTbiFile,*/pOutPutTBDFile,pConsolidate, *process_summary);

    pOutPutTBDFile->WriteAllSlices(true);
    pOutPutTBDFile->WriteTrailer();
    pOutPutTBDFile->Close();

    SAFE_DELETE(pOutPutTBDFile);

    if( lister != nullptr )
    {
        WriteConListingContent(*lister);
        lister->Finalize(*m_pPifFile, { });
        lister.reset();
    }

    //Finally Close all the input tbd Files
    bool bCloseInputTBIOrTabFiles = true;
    if(bCloseInputTBIOrTabFiles) {
        DeleteMultipleTBDFiles();
    }
    return true;
}

    /////////////////////////////////////////////////////////////////////////////////
//
//      bool CRunTab::ExecCalc()
//
/////////////////////////////////////////////////////////////////////////////////
bool CRunTab::ExecCalc()
{
    if(!m_pPifFile)
        return false;

    bool bRet = false;

    std::function<void()> exec_calc = [&]
    {
        //Create a new
        CRunAplCsCalc apl(m_pPifFile);
        apl.SetProcessSpcls4Tab(true);

        bRet = apl.LoadCompile();
        apl.SetBatchMode( CRUNAPL_CSCALC );

        CArray      <CTAB*, CTAB*> aAppCtabs;
        CArray      <CTAB*, CTAB*> aUsedCtabs;
        CArray      <CTAB*, CTAB*> aUnUsedCtabs;
        CArray<CTbdTable*,CTbdTable*> aTables;

        CFileStatus fStatus;

        //Create TAI file using our method if it does not exist
        for( const CString& sTabFileName : m_pPifFile->GetCalcInputFNamesArr() ) {
            CString sCalcInputTAI = sTabFileName;
            PathRemoveExtension(sCalcInputTAI.GetBuffer(_MAX_PATH));
            sCalcInputTAI.ReleaseBuffer();
            sCalcInputTAI = sCalcInputTAI + FileExtensions::BinaryTable::WithDot::TabIndex;
            if(!CFile::GetStatus(sCalcInputTAI,fStatus)){
                if(!CreateTAIFile(sTabFileName, sCalcInputTAI)){
                    AfxMessageBox(_T("Failed to create input tabidx file"));
                    return;
                }
            }
        }
        int     iNumAppCtabs=0;
        if( bRet ) {
            iNumAppCtabs = apl.GetAppCtabs( aAppCtabs);
        }

        // For each TBD
        for( const CString& sTabFileName : m_pPifFile->GetCalcInputFNamesArr() ) {
            // Open
            bRet = apl.OpenInputTbd( sTabFileName ); // Generate an issaerror when file can't be opened
            if( !bRet )
                continue;

            // Get Info
            int iNumTbdCtabs = apl.GetTbdCtabs( aUsedCtabs, aUnUsedCtabs,  aTables );

            CStringArray aBreakKey;
            CUIntArray   aBreakNumKeys;

            int iNumKeys=apl.GetTbdBreakKeys( aBreakKey, aBreakNumKeys );

            if( iNumKeys == 0 ) {
                CString csBreakKey;

                aBreakKey.Add( csBreakKey );
                aBreakNumKeys.Add(0); // RHF May 06, 2003
                iNumKeys++;
            }


            // Intenal Processing loop
            // Run
            apl.SetRunTimeBreakKeys(&aBreakKey, &aBreakNumKeys, &aUsedCtabs );
            bRet = apl.Start();
            ASSERT( bRet );

            bRet = apl.Stop();
            apl.SetRunTimeBreakKeys(NULL,NULL,NULL);


            // Close
            apl.CloseInputTbd();
            aUsedCtabs.RemoveAll();
            aUnUsedCtabs.RemoveAll();
            aTables.RemoveAll();
        }

        aAppCtabs.RemoveAll();

        bRet = apl.End( true );
    };

    TabExecutionDlg tab_execution_dlg(*m_pPifFile, nullptr, exec_calc);
    tab_execution_dlg.DoModal();

    if( bRet )
        UpdateEngineMessagesVariables(tab_execution_dlg.GetProcessSummary());

    else
        DisplayAuxiliaryFilesPostRun(true);

    return bRet;
}


/////////////////////////////////////////////////////////////////////////////////
//
//      bool CRunTab::ExecTab()
//
/////////////////////////////////////////////////////////////////////////////////
bool CRunTab::ExecTab()
{
    if(!m_pPifFile)
        return false;

    //Create a new
    CRunAplBatch apl(m_pPifFile, true);
    apl.SetProcessSpcls4Tab(true);

    bool bRet = false;

    std::function<void()> exec_tab = [&]
    {
        bRet = apl.LoadCompile();

        bRet = apl.Start();
        bRet = apl.Stop();
        bRet = apl.End( true );
    };

    TabExecutionDlg tab_execution_dlg(*m_pPifFile, &apl, exec_tab);
    tab_execution_dlg.DoModal();

    if( bRet )
    {
        UpdateEngineMessagesVariables(tab_execution_dlg.GetProcessSummary());
    }

    else
    {
        DisplayAuxiliaryFilesPostRun(true);
    }

    return bRet;
}



/////////////////////////////////////////////////////////////////////////////////
//
//      void CTabulateDoc::BuildTables(CTbdFile* ptFile);
//
/////////////////////////////////////////////////////////////////////////////////
bool CRunTab::BuildTables(CTbdFile* ptFile)
{
    //RemoveAllTables();
    //    RemoveAllRegs();

    CBreakItem cbItem;
    CTableAcum* pAcum;
    CTbdSlice* pSlice;
    CTbdTable* pTBDTable =NULL;
    CTable* pCurTable = NULL;
    CTabData* pTabData = NULL;
    double     dValue;
    std::shared_ptr<CTabSet> pTableSpec = m_pApplication->GetTabSpec();


    //cout << "BEGIN TBD FILE INFO\n------------\n";
    //cout << "File Name " << ptFile->GetFileName()<< "\n\n";

    int iNumTables = ptFile->GetNumTables();
    //cout << "\n\nTable Info\n";
    //cout << "Num tables: " <<  iNumTables<< "\n\n";

    /*CString csOut;
    for (int i = 0; i < iNumTables; i++ ) {
    pTBDTable = ptFile->GetTable(i);

    csOut.Format( "Name=%-25s, Type=%1d, CellSize=%3d, NumDim=%1d, Dim0=%3d, Dim1=%3d, Dim2=%3d, NumBreak=%d",
    pTBDTable->GetTableName().GetBuffer(0),
    pTBDTable->GetTableType(),
    pTBDTable->GetCellSize(),
    pTBDTable->GetNumDims(),
    pTBDTable->GetDimSize(0),
    pTBDTable->GetNumDims() >= 2 ? pTBDTable->GetDimSize(1) : 0,
    pTBDTable->GetNumDims() >= 3 ? pTBDTable->GetDimSize(2) : 0,
    pTBDTable->GetNumBreak() );

    cout << csOut << "\n";
    }
    */

    std::shared_ptr<ProgressDlg> progDlg = ProgressDlgFactory::Instance().Create(IDS_PROGRESS_TITLE);
    progDlg->SetStatus(IDS_PROGRESS_TITLE_PREP);
    progDlg->SetRange(0, 1000);
    CWaitCursor wc;

    //Clear all TabData for all tables
    for(int iIndex =0 ; iIndex < pTableSpec->GetNumTables(); iIndex++ ) {
        CTable* pTable =  pTableSpec->GetTable(iIndex);
        ASSERT(pTable);
        pTable->RemoveAllData();
    }
    //cout << "       BEGIN SHOW SLICE\n";

    CString     csMsg;
    CString     csValue;
    CString     csFilePos;
    byte*       pValue;
    long        lSliceFilePos=0L;


    while ( (pSlice = ptFile->GetNextSlice(lSliceFilePos)) != NULL ) {
        pTBDTable = pSlice->GetTable();

        csMsg = pTBDTable->GetTableName();

        //  csFilePos.Format( " File Pos : %d (0x%X)", pSlice->GetFilePos(), pSlice->GetFilePos() );

        if(!pCurTable || pCurTable->GetName().CompareNoCase(pTBDTable->GetTableName()) != 0 ) {
            pCurTable  = NULL;
            pTabData = NULL;
            for(int iIndex =0 ; iIndex < pTableSpec->GetNumTables(); iIndex++ ) {

                if(pTableSpec->GetTable(iIndex)->GetName().CompareNoCase(pTBDTable->GetTableName()) ==0) {
                    pCurTable = pTableSpec->GetTable(iIndex);
                    CArray<CTabData*, CTabData*>&arrTabData =  pCurTable->GetTabDataArray();
                    // pCurTable->RemoveAllData();
                    pTabData = new CTabData();
                    arrTabData.Add(pTabData);
                    break;
                }
                else {
                    continue;
                }

            }
        }
        else {//WE are on the same table with a new slice for a different break
            //Now create another tabdata object to fill in this new guy
            if(!pCurTable){//We havent found a table in the spec . SAVY&&&
                continue;
            }
            CArray<CTabData*, CTabData*>&arrTabData =  pCurTable->GetTabDataArray();
            pTabData = new CTabData();
            arrTabData.Add(pTabData);
        }

        csMsg = csMsg + csFilePos;
        if( pTBDTable->GetNumBreak() >= 1 && pTabData) {
            CString  csKey;
            csKey.Format( _T("%s"), pSlice->GetBreakKey().GetString() + sizeof(short) /sizeof(TCHAR));
            csMsg = csMsg + _T(" Break = ") + csKey;
            pTabData->SetBreakKey(csKey);

        }

        std::cout << csMsg.GetString() << _T("\n");

        pAcum = pSlice->GetAcum();
        byte*   pDefaultValue = pAcum->GetDefaultValue();
        // double  dDefaultValue;

        for( int iLayer=0; iLayer < pAcum->GetNumLayers(); iLayer++ ) {
            if( pAcum->GetNumLayers() > 1 ) {
                /*ASSERT(FALSE); // SAVY&&& TO DO LAYERS
                csMsg.Format( "---> Layer %d", iLayer );
                cout << csMsg << "\n";*/
            }
            int iRows = pAcum->GetNumRows();
            int iCols = pAcum->GetNumCols();
            if(!pTabData)
                continue;
            //            CArray<double,double>& arrCells = pTabData->GetCellArray();
            CArray<double, double&>& arrCells = pTabData->GetCellArray();  // csc 11/12/03
            arrCells.RemoveAll();
            arrCells.SetSize(iRows*iCols);


            for( int iRow=0; iRow < pAcum->GetNumRows(); iRow++ ) {

                for( int iCol=0; iCol < pAcum->GetNumCols(); iCol++ ) {
                    pValue = pAcum->GetValue( iRow, iCol, iLayer );

                    /*switch ( pTBDTable->GetTableType() ) {
                    // OTHER DOUBLE
                    default:*/
                    // memcpy( (byte*)&dDefaultValue, pDefaultValue, pTBDTable->GetCellSize() );
                    memcpy((byte*)&dValue, pValue, pTBDTable->GetCellSize());
                    //csMsg.Format( "%3d,%3d,%3d=%6.2f", iRow, iCol, iLayer, dValue );
                    /*if( dValue == dDefaultValue )
                    csValue.Format( "%s", "------" );
                    else if( dValue <= 1.0e50 )
                    csValue.Format( "%6.2f", dValue );
                    else
                    csValue.Format( "%s", "******" );

                    csMsg = csMsg + ( (iCol==0) ? "" : "  " ) + csValue;
                    cout << "<" << iCol << "," << iRow << "," << iLayer << "> = " << dValue << "\n";*/
                    {
                        int iIndex  = (iRow) * pAcum->GetNumCols() +  iCol;
                        //CDataCell d(dValue);   // csc 11/12/03
                        arrCells.SetAt(iIndex,dValue);
                    }
                    // break;
                    //}
                    //cout << csMsg <<  "\n";
                }
                //    iCurIndex++; // increment for the next iteration
            }

        }

        delete pSlice;
        progDlg->SetPos(int(1000* (double(lSliceFilePos)/double(ptFile->GetFileSize()))));
        if (progDlg->CheckCancelButton()) {
            return false;
        }
        wc.Restore();
    }
    std::cout << _T("       END SHOW SLICE\n");
    std::cout << _T("END TBD FILE INFO\n");
    SetAreaLabels4AllTables();
    return true;
}

/////////////////////////////////////////////////////////////////////////////////
//
//  bool CRunTab::MakeNewBreak(CTbdFile* pInputTbdFile , CTbdSlice* pSlice, CConSpec* pConSpec, CString& sOutKey)
//
/////////////////////////////////////////////////////////////////////////////////
bool CRunTab::MakeNewBreak(CTbdFile* pInputTbdFile , CTbdSlice* pSlice, CConSpec* pConSpec, CString& sOutKey)
{
    int iStart = 0;
    int iKeyLen = 0;
    CIMSAString sKey;
    CString sInKey = pSlice->GetBreakKey();

    CIMSAString sTableNum(_T('1'));//place holder for table number - actual value is set in the SetTableNum below
    //sTableNum.Str(pSlice->GetSliceHdr()->iTableNumber+1,sizeof(short)/sizeof(TCHAR),_T('0')); //Savy for Unicode 02/01/2012
    //sTableNum.Trim();

//    sOutKey = CString('-', pSlice->GetBreakKeyLen()-sizeof(short));
    //Savy for Unicode 02/01/2012
    sOutKey = CString(_T('-'), sInKey.GetLength()-sizeof(short)/sizeof(TCHAR));  // BMD 03 Oct 2005
    sOutKey=sTableNum+sOutKey;

    pInputTbdFile->SetTableNum(sOutKey.GetBuffer(256),pSlice->GetSliceHdr()->iTableNumber+1, sizeof(short));
    sOutKey.ReleaseBuffer();
    int iLowCon;
    for (iLowCon = pConSpec->GetNumActions() ; iLowCon > 0 ; iLowCon--) {  // BMD 03 Oct 2005
        CONITEM cItem = pConSpec->GetAction(iLowCon - 1);
        if (cItem.level != CON_NONE) {  // BMD 03 Oct 2005
            break;
        }
    }
    int iLowKey = 0;
    iStart = sizeof(short)/sizeof(TCHAR);//Savy for Unicode 02/01/2012
    for (int i = 0 ; i < pConSpec->GetNumActions() ; i++) {
        iKeyLen = pInputTbdFile->GetBreak(i)->GetLen();
        sKey = sInKey.Mid(iStart, iKeyLen);
        if (!SO::IsBlank(sKey)) {
            iLowKey = i + 1;
        }
        iStart += iKeyLen;
    }
    if (iLowKey < iLowCon) {
        return false;
    }
    iStart = sizeof(short)/sizeof(TCHAR); // for the table num which they dont use ?? //Savy for Unicode 02/01/2012
    for (int iIndex = 0 ; iIndex < pConSpec->GetNumActions() ; iIndex++) {
        CONITEM cItem = pConSpec->GetAction(iIndex);
        iKeyLen = pInputTbdFile->GetBreak(iIndex)->GetLen();
        sKey = sInKey.Mid(iStart, iKeyLen);
        if (cItem.level == CON_NONE) {
            iStart += iKeyLen;
            continue;
        }
        int iKey = 0;
        if (cItem.lower != CON_NONE) {
            sKey = sInKey.Mid(iStart, iKeyLen);
            iKey = (int) sKey.fVal();
            if (iKey < cItem.lower || iKey > cItem.upper) {
                return false;
            }
        }
        if (cItem.replace != CON_NONE) {
            sKey.Str(cItem.replace, iKeyLen, _T('0'));
        }
        sOutKey = sOutKey.Left(iStart) + sKey + sOutKey.Mid(iStart+iKeyLen);
        iStart += iKeyLen;
    }
    return true;
}

/////////////////////////////////////////////////////////////////////////////////
//
//  bool CRunTab::ProcessCon(CTbdFile* pInputTbdFile,CTbdFile* pOutPutTbdFile,CConsolidate* pConsolidate)
//
/////////////////////////////////////////////////////////////////////////////////
bool CRunTab::ProcessCon(/*CTbdFile* pInputTbdFile, CTbiFile* pInputTbiFile,*/ CTbdFile* pOutPutTbdFile,
    CConsolidate* pConsolidate, ProcessSummary& process_summary)
{
    CTbdFile* pInputTbdFile = NULL;
    CTbiFile* pInputTbiFile = NULL;
    bool bRet = true;
    bool bParticipates = false;
    CIMSAString sOutKey;
    const TCHAR* pKey;
    long    lFilePos;
    int     iTableNum;
    int     iNumBreak=0;
    bool bLookup  = false;
    CIMSAString sBreakKey;
    CIMSAString sInputTableName;

    pInputTbdFile = m_arrConInputTBDFiles[0];
    pInputTbiFile = m_arrConInputTBIFiles[0];
    ASSERT(pConsolidate);
    ASSERT(pInputTbdFile);
    ASSERT(pInputTbiFile);

    ASSERT(pOutPutTbdFile);
    CArray<CTbdSlice*,CTbdSlice*> arrNewSlices;

    std::shared_ptr<ProgressDlg> progDlg = ProgressDlgFactory::Instance().Create(IDS_PROGRESS_TITLE);
    progDlg->SetStatus(IDS_PROGRESS_TITLE_CON);
    progDlg->SetRange(0, pInputTbdFile->GetNumTables());
    CWaitCursor wc;

    CArray<CMap<int,int,CArray<long,long&>*,CArray<long,long&>*>* ,CMap<int,int,CArray<long,long&>*,CArray<long,long&>*>*> arrMapPos4InputFiles;

    CMapStringToPtr outputSliceMap;

    for(int iFile =0; iFile < m_arrConInputTBDFiles.GetSize(); iFile++){//in each file
        CMap<int,int,CArray<long,long&>*,CArray<long,long&>*>* pMap = new CMap<int,int,CArray<long,long&>*,CArray<long,long&>*> ;
        CMap<int,int,CArray<long,long&>*,CArray<long,long&>*>& tblNumFilePosMap= *pMap;
        pInputTbiFile =m_arrConInputTBIFiles[iFile];
        pInputTbiFile->Locate(CTbiFile::First);

        while( pInputTbiFile->Locate(CTbiFile::Next) )  {
            pKey = (TCHAR*)pInputTbiFile->GetCurrentReg();
            lFilePos = pInputTbiFile->GetCurrentOffset()-1;

            iTableNum =  CTbdFile::GetTableNum(pKey, sizeof(short));

            // RHF INIC Apr 17, 2003
            if( iTableNum <= 0 || iTableNum > pInputTbdFile->GetNumTables() ) {
                //issaerror( MessageType::Error, 692, pTbiFile->GetFileName(), iTableNum ); // Cannot open TBD file %s
                AfxMessageBox(_T("Do Clean up"));
                continue;
            }
            CArray<long,long&>* pArrFilePos= NULL;
            if(!tblNumFilePosMap.Lookup(iTableNum,pArrFilePos)){
                pArrFilePos = new CArray<long,long&>;
                pArrFilePos->Add(lFilePos);
                tblNumFilePosMap.SetAt(iTableNum,pArrFilePos);
            }
            else {
                ASSERT(pArrFilePos);
                pArrFilePos->Add(lFilePos);
            }

        }
        arrMapPos4InputFiles.Add(pMap);
    }
    //End table num filepos map


    //How do I add the breaks for the file . is it different from the input file
    //breaks ? for now add from the input
    for(int iInputBreak =0 ; iInputBreak <pInputTbdFile->GetNumBreakItems(); iInputBreak++){
        CBreakItem* pBreakItem = new CBreakItem(*(pInputTbdFile->GetBreak(iInputBreak)));
        pOutPutTbdFile->AddBreak(pBreakItem);
    }
    int iNumConSpecs = pConsolidate->GetNumConSpecs();

    CTbdSlice* pInputTbdSlice = NULL;
    long lSliceFilePos=0;
    int iNumTables = m_arrConInputTBDFiles[0]->GetNumTables();
    for(int iInputTable =0; iInputTable < iNumTables; iInputTable++){//For each Table
          bool bWriteSlices = false;
          CTbdTable* pNewOutputTbdTable =NULL;
          CIMSAString sCurTableName;
          for(int iFile =0; iFile < m_arrConInputTBDFiles.GetSize(); iFile++){//in each file
            CMap<int,int,CArray<long,long&>*,CArray<long,long&>*>*pMap =arrMapPos4InputFiles[iFile];
            CMap<int,int,CArray<long,long&>*,CArray<long,long&>*>& tblNumFilePosMap = *pMap;
            pInputTbdFile = m_arrConInputTBDFiles[iFile];

            progDlg->StepIt();
            if (progDlg->CheckCancelButton()) {
                bRet = false;
                break;
            }


            CArray<long,long&>* pArrFilePos= NULL;
            if(!tblNumFilePosMap.Lookup(iInputTable+1,pArrFilePos)){
                continue;
            }

            //Add the new table to the out tab file
            ASSERT(iNumTables == pInputTbdFile->GetNumTables());//number of tables in each input file should  be same
            //Do we have to check if we are using the same tablename
            CTbdTable* pCurInputTbdTable= pInputTbdFile->GetTable(iInputTable);
            if(iFile == 0){
                sCurTableName = pCurInputTbdTable->GetTableName();
            }
            else {
                ASSERT(sCurTableName.CompareNoCase(pCurInputTbdTable->GetTableName()) ==0);
            }

            if(!pNewOutputTbdTable){
                pNewOutputTbdTable =new CTbdTable(*pCurInputTbdTable);
                CArray<CBreakItem*, CBreakItem*>&arrBreakItems = pNewOutputTbdTable->GetBreakArray();
                arrBreakItems.RemoveAll();
                for ( int i = 0; i < pCurInputTbdTable->GetNumBreak(); i++ ) {
                    CBreakItem* pBreakItem = pOutPutTbdFile->GetBreak(i);
                    pNewOutputTbdTable->AddBreak(pBreakItem);
                }

                pOutPutTbdFile->AddTable(pNewOutputTbdTable);
            }
            //End of add new table

            sInputTableName = pCurInputTbdTable->GetTableName();
            for(int iIndex =0; iIndex < pArrFilePos->GetSize() ; iIndex++){
                lSliceFilePos=pArrFilePos->GetAt(iIndex);
                //Go through all the slices of a particular table and check if it participates
                //for each break key
                pInputTbdSlice = pInputTbdFile->GetNextSlice(lSliceFilePos);
                ASSERT(pInputTbdSlice);
                CTbdTable* pInputTbdTable = pInputTbdSlice->GetTable();
                if(pCurInputTbdTable != pInputTbdTable){
                    ASSERT(FALSE); //This should never happen
                    SAFE_DELETE(pInputTbdSlice); // JH 4/8/05 fix memory leak
                    continue;
                }
                process_summary.IncrementAttributesRead();
                //check if the slice participates in the area for each conspec
                for (int iCon = 0; iCon < iNumConSpecs ; iCon++){
                    CConSpec* pConSpec = pConsolidate->GetConSpec(iCon);
                    //if it does not continue
                    sOutKey=_T("");
                    bParticipates = MakeNewBreak(pInputTbdFile ,pInputTbdSlice,pConSpec,sOutKey);
                    if(!bParticipates){
                        continue;
                    }
                    //Now check if the slice with the required break key exists in the pOutPutTbdFile
                    CTbdSlice* pNewTbdSlice = NULL;

                    /*pOutPutTbdFile->SetTableNum(sOutKey.GetBuffer(256),iTblNum+1, sizeof(short));
                    sOutKey.ReleaseBuffer();*/
                    void* myPtr = NULL;
                    outputSliceMap.Lookup(sOutKey, myPtr) ? bLookup = true : bLookup =false;
                    pNewTbdSlice =(CTbdSlice*) myPtr;
                    if(bLookup){
                        if (!pNewTbdSlice){
                            //Key exists in the lookup but slice is NULL
                            //Is the slice written out to the outputtbfile . If so
                            //get it from tbdfile . BRUCE's 1% case
                            ASSERT(FALSE);
                        }
                    }

                    //if it does not . Create a new slice and assign the pTbdSlice to it
                    if(!pNewTbdSlice){
                        //create new tbdslice and the corresponding table
                        //Check if the table already exists
                        //and assign the contents to this guy
                        pNewTbdSlice = CreateNewOutPutSlice(pNewOutputTbdTable,pInputTbdSlice,sOutKey);

                        //int iTblNum = GetTblNumFromOutputTBD(pOutPutTbdFile,pNewOutputTbdTable->GetTableName());
                        if(pNewTbdSlice->GetSliceHdr()->iTableNumber != iInputTable ){//0 based

                            pNewTbdSlice->GetSliceHdr()->iTableNumber = iInputTable;
                            //Set the break key again as the table number is now changed
#ifdef _DEBUG
                            sBreakKey =sOutKey;
                            pOutPutTbdFile->SetTableNum(sBreakKey.GetBuffer(256),pNewTbdSlice->GetSliceHdr()->iTableNumber+1, sizeof(short));
                            sBreakKey.ReleaseBuffer();
                            ASSERT(sBreakKey.CompareNoCase(sOutKey) ==0); //The keys shld be equal as the tblnum always match up
#endif
                            pNewTbdSlice->SetBreakKey(sBreakKey,pInputTbdSlice->GetBreakKeyLen());
                        }
                        CIMSAString sTableList;
                        if(pInputTbdTable->GetTableType() == CTableDef::ETableType::Ctab_Crosstab){
                            CIMSAString sLookupKey = sOutKey.Mid(2);
                            if(m_mapConListing.Lookup(sLookupKey,sTableList)){
                                sTableList += _T(" ")+pInputTbdTable->GetTableName();
                                m_mapConListing[sLookupKey] = sTableList;

                            }
                            else {
                                m_mapConListing[sLookupKey] = pInputTbdTable->GetTableName();
                            }
                        }

                        pOutPutTbdFile->AddSlice(pNewTbdSlice, false );
                        arrNewSlices.Add(pNewTbdSlice);
                        outputSliceMap.SetAt(sOutKey, pNewTbdSlice);

                    }
                    else {
                        //the "Addition" operation with the pTbdSlice
                        //SAVY&&& check to see if this works Min/Max do Minoper/MaxOper ....Very Important
                        pNewTbdSlice->GetAcum()->Addition( pInputTbdSlice->GetAcum());
                    }

                }
                SAFE_DELETE(pInputTbdSlice); // JH 4/8/05 fix memory leak
            }
            delete pArrFilePos;
            tblNumFilePosMap.RemoveKey(iInputTable+1);
            bWriteSlices =true;
        }
        if(bWriteSlices){
            pOutPutTbdFile->WriteAllSlices(true);
            outputSliceMap.RemoveAll();
            arrNewSlices.RemoveAll();
        }
    }
    for(int iMap =0; iMap<arrMapPos4InputFiles.GetSize();iMap++){
        SAFE_DELETE(arrMapPos4InputFiles[iMap]);
        arrMapPos4InputFiles[iMap]=NULL;
    }
    return bRet;
}
CTbdTable* CRunTab::FindTable(CTbdFile* pInputTbdFile,CIMSAString sTableName)
{
    CTbdTable* pRetTbdTable=NULL;
    for(int iOutputTable =0; iOutputTable< pInputTbdFile->GetNumTables() ; iOutputTable++){
        if(pInputTbdFile->GetTable(iOutputTable)->GetTableName().CompareNoCase(sTableName) ==0){
            pRetTbdTable = pInputTbdFile->GetTable(iOutputTable);
            break;
        }
    }
    return pRetTbdTable;
}
CTbdSlice* CRunTab::CreateNewOutPutSlice(CTbdTable* pInputTbdTable, CTbdSlice* pInputTbdSlice, CIMSAString sBreakKey)
{
    ASSERT(pInputTbdTable);
    ASSERT(pInputTbdSlice);

    CTbdSlice* pNewTbdSlice=new CTbdSlice( pInputTbdTable );
    CTableAcum* pNewAcum=new CTableAcum( pInputTbdTable );

    CTableAcum* pAcum = pInputTbdSlice->GetAcum();
    //pNewAcum->ShareArea(*pAcum);

    pNewTbdSlice->SetSliceHdr(pInputTbdSlice->GetSliceHdr());

    pNewAcum->Alloc();
    pNewTbdSlice->SetAcum( pNewAcum, true );
    pNewAcum->Assign(pAcum);

    /*CIMSAString sTableNum;
    sTableNum.Str(pNewTbdSlice->GetSliceHdr()->iTableNumber);
    sTableNum.Trim();
    sBreakKey.Right(sBreakKey.GetLength() - sTableNum.GetLength());
    sBreakKey= sTableNum+sBreakKey*/
//    pNewTbdSlice->SetBreakKey(sBreakKey,pInputTbdSlice->GetBreakKeyLen());//Assuming sOut
    pNewTbdSlice->SetBreakKey(sBreakKey,sBreakKey.GetLength());  // BMD 03 Oct 2005

    return pNewTbdSlice;
}

/////////////////////////////////////////////////////////////////////////////////
//
//  void CRunTab::CreateAuxillaryTables(CTbdFile* pInputTbdFile,CTbdFile* pOutPutTbdFile, CIMSAString sTableName ,CArray<CTbdSlice,CTbdSlice*>& arrTbdSlices,CIMSAString sBreakKey)
//
/////////////////////////////////////////////////////////////////////////////////
void CRunTab::CreateAuxillaryTables(CTbdFile* pInputTbdFile,CTbdFile* pOutPutTbdFile, CIMSAString sTableName ,CArray<CTbdSlice*,CTbdSlice*>& arrTbdSlices,CIMSAString sBreakKey)
{
    long lSliceFilePos =0;
    CTbdSlice* pInputTbdSlice = NULL;
    while ( (pInputTbdSlice = pInputTbdFile->GetNextSlice(lSliceFilePos)) != NULL ){
        if(pInputTbdSlice->GetTable()->GetTableType() == CTableDef::ETableType::Ctab_Crosstab)
        {
            continue; // We are interested ony lin aux tables
        }
        CIMSAString sSliceTableName = pInputTbdSlice->GetTable()->GetTableName();
        int iFind = sSliceTableName.Find('_');
        if(iFind == -1){
            continue;
        }
        CIMSAString sCompareSliceTableName = sSliceTableName.Left(iFind);

        if(sCompareSliceTableName.CompareNoCase(sTableName) != 0 ) {
            //We are interested in aux tables of current "sTableName" table only
            continue;
        }
        //For good measure check if this table exists in the output Table
        //if it does not create
        CTbdTable* pAuxOutPutTbdTable =FindTable(pOutPutTbdFile,sSliceTableName);
        if(pAuxOutPutTbdTable){
            continue ; //Table already exits no need to create
        }
        //Create the auxillary table now
        CTbdTable* pInputTbdTable = pInputTbdSlice->GetTable();
        pAuxOutPutTbdTable =new CTbdTable(*pInputTbdTable);
        CArray<CBreakItem*, CBreakItem*>&arrBreakItems = pAuxOutPutTbdTable->GetBreakArray();
        arrBreakItems.RemoveAll();
        for ( int i = 0; i < pInputTbdTable->GetNumBreak(); i++ ) {
            CBreakItem* pBreakItem = pOutPutTbdFile->GetBreak(i);
            //see tbd_init in tbd_save.cpp
            pAuxOutPutTbdTable->AddBreak(pBreakItem);
        }

        pOutPutTbdFile->AddTable(pAuxOutPutTbdTable);
        //Create an Empty Slice
        CTbdSlice* pNewTbdSlice=new CTbdSlice( pAuxOutPutTbdTable );
        CTableAcum* pNewAcum=new CTableAcum( pAuxOutPutTbdTable );

        //CTableAcum* pAcum = pInputTbdSlice->GetAcum();
        pNewTbdSlice->SetSliceHdr(pInputTbdSlice->GetSliceHdr());
        int iTblNum = GetTblNumFromOutputTBD(pOutPutTbdFile,sSliceTableName);
        pNewTbdSlice->GetSliceHdr()->iTableNumber = iTblNum ;//0 based

        pNewAcum->Alloc();
        pNewTbdSlice->SetAcum( pNewAcum, true );

        pOutPutTbdFile->SetTableNum(sBreakKey.GetBuffer(256),pNewTbdSlice->GetSliceHdr()->iTableNumber+1, sizeof(short));
        sBreakKey.ReleaseBuffer();

        pNewTbdSlice->SetBreakKey(sBreakKey,pInputTbdSlice->GetBreakKeyLen());

        //Important .Do not assign the contents of this Slice yet .
        //The main loop will "Add" when it gets to it

        //Add New Slice
        pOutPutTbdFile->AddSlice(pNewTbdSlice, false );
        arrTbdSlices.Add(pNewTbdSlice);
    }

}

/////////////////////////////////////////////////////////////////////////////////
//
//  bool CRunTab::CreateTAIFile(CString sTabFileName, CString sTaiFileName ,bool bSilent /*=false*/)
//
/////////////////////////////////////////////////////////////////////////////////
bool CRunTab::CreateTAIFile(CString sTabFileName, CString sTaiFileName ,bool bSilent /*=false*/)
{
    std::shared_ptr<ProgressDlg> progDlg = ProgressDlgFactory::Instance().Create(IDS_PROGRESS_TITLE);
    progDlg->SetStatus(IDS_PROGRESS_TITLE_TAI);
    progDlg->SetRange(0, 1000);
    CWaitCursor wc;

    bool bRet = false;
    bool bExistTabFile = false;
    if( _taccess( sTabFileName, 0 ) == 0)
        bExistTabFile = true;

    bool bExistIndexFile = false;

    CString csAux = sTaiFileName;
    csAux.TrimRight();
    CString csExt = csAux.Right(7);
    csExt.MakeUpper();

    // If it does not have ".idx" at the end, it is included
    // If it does, it is used
    if( csAux.GetLength() <= 7 || csExt.CompareNoCase(FileExtensions::BinaryTable::WithDot::TabIndex) != 0 )
        sTaiFileName = csAux + FileExtensions::BinaryTable::WithDot::TabIndex;

    if( _taccess( sTaiFileName,0) == 0)
        bExistIndexFile = true;

    if( bExistIndexFile && !bExistTabFile ) {
        AfxMessageBox( _T("There is no Tab file to index.") );  // BMD 21 Jun 2004
        return bRet;
    }

    if( bExistTabFile )
    {
        CFile* pFile = new CFile;
        if( pFile->Open( sTabFileName, CFile::modeReadWrite ) == 0 ) {
            delete pFile;
            AfxMessageBox(_T("Could not open ") + sTabFileName + _T(" for reading and writing.") ); // BMD 21 Jun 2004
            return bRet;
        }
        if( pFile->GetLength() == 0 )
            bExistIndexFile = false;
        pFile->Close();
        delete pFile;
    }

    //Start processing
    if( bExistIndexFile == true ){
        CFile::Remove( sTaiFileName );
        bExistIndexFile = false;
    }

    bool bCreate = false;
    CTbdFile* pInputTbdFile = NULL;
    pInputTbdFile = new CTbdFile(sTabFileName);
    if ( !pInputTbdFile->Open(bCreate) ) {
        pInputTbdFile->Close();
        SAFE_DELETE(pInputTbdFile);
        return false;
    }
    //What shld I do if zero length ?

    int iNumInsertions = 0;
    //Savy for Unicode 02/01/2012
    int iKeyLen = pInputTbdFile->GetBreakKeyLen() + sizeof(short)/sizeof(TCHAR)+1; //Extra 1 for \0 terminator

    SimpleDbMap tableIndex;

    if( !tableIndex.Open(sTaiFileName.GetString(), { { _T("TBI"), SimpleDbMap::ValueType::Long } }) )
        return bRet;

    bool bAbortedCreation = false;
    CTbdSlice* pInputTbdSlice = NULL;
    long lNextSliceFilePos=0;
    long lCurSliceFilePos=0;
    while ( (pInputTbdSlice = pInputTbdFile->GetNextSlice(lNextSliceFilePos)) != NULL ){
        CString csKey = pInputTbdSlice->GetBreakKey();
        try{
            CIMSAString sKey;
            int iBreakKeyLength  = csKey.GetLength();
            TCHAR* pszKey = sKey.GetBufferSetLength(iKeyLen);
            _tmemset(pszKey,_T('\0'),iKeyLen);
            _tmemcpy(pszKey, csKey, iBreakKeyLength);
            sKey.ReleaseBuffer();

            if( tableIndex.Exists(sKey) || !tableIndex.PutLong(sKey,lCurSliceFilePos + 1) )
            {
                if( !bSilent )
                {
                    AfxMessageBox(FormatText(MGF::GetMessageText(MGF::IndexDuplicate).c_str(), csKey.GetString(), sTabFileName.GetString()));
                    AfxMessageBox(MGF::GetMessageText(MGF::IndexCannotCreate).c_str(), MB_OK | MB_ICONINFORMATION);
                }

                iNumInsertions = 0;
                bAbortedCreation = true;
                pInputTbdFile->Close();
                tableIndex.Close();
                SAFE_DELETE(pInputTbdFile);
                SAFE_DELETE(pInputTbdSlice);
                return bRet;
            }

            else
                iNumInsertions++;
        }
        catch( ... )
        {
            CString auxString;
            auxString.Format( _T("Error Captured, filepos %d"), lCurSliceFilePos); // backward compatibility
            auxString = auxString + _T(", Key ") + csKey;
            AfxMessageBox( auxString );
        }

        lCurSliceFilePos = lNextSliceFilePos;
        SAFE_DELETE(pInputTbdSlice);

        progDlg->SetPos(int(1000* (double(lCurSliceFilePos)/double(pInputTbdFile->GetFileSize()))));
        if (progDlg->CheckCancelButton()) {
            bRet = false;
            bAbortedCreation = true;
            break;
        }

        wc.Restore();
    }

    // Close Index and data file
    tableIndex.Close();
    pInputTbdFile->Close();
    SAFE_DELETE(pInputTbdFile);

    // but destroy corrupted index
    if( bAbortedCreation )
        return bRet;

    // RHF COM Feb 02, 2001if( iNumInsertions == 0 && pDataFile->GetLength() > 0 )
    if( iNumInsertions == 0 /*&& uFileSize > 0 */) // RHF Feb 02, 2001
    {
        bRet = true;
        //For now let us not delete this zero tabidx file
        /*if( PortableFunctions::FileExists( m_csIndexFileName ) )
        CFile::Remove( m_csIndexFileName );*/
    }
    return true;
    //End processing
}


/////////////////////////////////////////////////////////////////////////////////
//
//  int  CRunTab::GetTblNumFromOutputTBD(CTbdFile* pOutPutTbdFile, CIMSAString sTableName)
//
/////////////////////////////////////////////////////////////////////////////////
int  CRunTab::GetTblNumFromOutputTBD(CTbdFile* pOutPutTbdFile, CIMSAString sTableName)
{
    int iTblNum = -1;
    for(int iOutputTable =0; iOutputTable< pOutPutTbdFile->GetNumTables() ; iOutputTable++){
        if(pOutPutTbdFile->GetTable(iOutputTable)->GetTableName().CompareNoCase(sTableName) ==0){
            iTblNum = iOutputTable;
            break;
        }
    }
    return iTblNum;
}

/////////////////////////////////////////////////////////////////////////////////
//
//  bool CRunTab::Exec(bool bSilent/*=false*/)
//
/////////////////////////////////////////////////////////////////////////////////
bool CRunTab::Exec(bool bSilent /*=false*/)
{
    bool bRet = false;
    ASSERT(m_pPifFile); //Make sure these are not NULL
    ASSERT(m_pApplication); //Make sure these are not NULL.
    CIMSAString sMsg;
    CIMSAString sTempTab;
    CIMSAString sTempTai;
    CIMSAString sTabOutputTai;
    PROCESS eProcess = m_pPifFile->GetTabProcess();
    CIMSAString sTabOutPutFName;
    CStringArray arrTempFiles; //To be deleted Finally if the run succeds
    std::shared_ptr<CTabSet> pTableSpec = m_pApplication->GetTabSpec();
    CConsolidate* pConsolidate  = pTableSpec->GetConsolidate();
    bool bHasCon =(pConsolidate && pConsolidate->GetNumAreas()> 0);

    if(eProcess == ALL_STUFF){
        //When you run all you will be given an inputdat file and the output is a tbw
        CIMSAString sOutputFolder = m_pPifFile->GetPrepOutputFName();
        PathRemoveFileSpec(sOutputFolder.GetBuffer());
        sOutputFolder.ReleaseBuffer();

        CIMSAString sInputDataFileName =  m_pPifFile->GetSingleInputDataConnectionString().IsFilenamePresent() ?
            WS2CS(m_pPifFile->GetSingleInputDataConnectionString().GetFilename()) : ( sOutputFolder + _T("\\~InputData") );

        //Make outTabFile name for execTab Process
        CIMSAString sTabOutPutFileName = PifDlgBuilder::MakeOutputFileForProcess(CS_TAB,sInputDataFileName, sOutputFolder, _T(""));
        m_pPifFile->SetTabOutputFName(sTabOutPutFileName);
        arrTempFiles.Add(sTabOutPutFileName);

        CIMSAString sConOutPutFileName = PifDlgBuilder::MakeOutputFileForProcess(CS_CON,sInputDataFileName, sOutputFolder);

        //Add the Taboutput Filename as Con Input File Name
        m_pPifFile->ClearConInputFilenames();
        if(bHasCon){
            m_pPifFile->AddConInputFilenames(sTabOutPutFileName);
            m_pPifFile->SetConOutputFName(sConOutPutFileName);
            arrTempFiles.Add(sConOutPutFileName);

            //Now set the con output as input for calc
            m_pPifFile->GetCalcInputFNamesArr().clear();
            m_pPifFile->GetCalcInputFNamesArr().emplace_back(sConOutPutFileName);

            //add the calc output file name
            CIMSAString sCalcOutPutFileName = PifDlgBuilder::MakeOutputFileForProcess(CS_CALC,sInputDataFileName, sOutputFolder);
            m_pPifFile->SetCalcOutputFName(sCalcOutPutFileName);
            arrTempFiles.Add(sCalcOutPutFileName);

        }
        else {
            m_pPifFile->GetCalcInputFNamesArr().clear();
            m_pPifFile->GetCalcInputFNamesArr().emplace_back(sTabOutPutFileName);

            //For the calc output
            CIMSAString sCalcOutPutFileName = PifDlgBuilder::MakeOutputFileForProcess(CS_CALC,sInputDataFileName, sOutputFolder);
            m_pPifFile->SetCalcOutputFName(sCalcOutPutFileName);
            arrTempFiles.Add(sCalcOutPutFileName);
        }


        //RunTab
        bRet = ExecTab();
        bRet ? sMsg=_T(""):sMsg = _T("Failed to run tabulate module");
        if(bRet){
            //Copyintermediate files
            //Run Con
            if(bHasCon){
                CWaitCursor wait;
                bRet = ExecCon();
                bRet ? sMsg = _T("") : sMsg = _T("Failed to run consolidate module") ;
                if(bRet){
                    CIMSAString sTabFile = m_pPifFile->GetConOutputFName();
                    CIMSAString sTAIFile = sTabFile ;
                    CIMSAString sFileExt;
                    //Remove the extension .tab if the Process is anything else
                    int iDot = sTAIFile.ReverseFind('.');
                    if (iDot > 0) {
                        sFileExt = sTAIFile.Mid(iDot + 1);
                        if(sFileExt.CompareNoCase(FileExtensions::BinaryTable::Tab) ==0){
                            PathRemoveExtension(sTAIFile.GetBuffer(_MAX_PATH));
                            sTAIFile.ReleaseBuffer();
                            sTAIFile += FileExtensions::BinaryTable::WithDot::TabIndex;
                        }
                    }
                    bRet = CreateTAIFile(sTabFile,sTAIFile);
                    bRet ? sMsg = _T("") : sMsg = _T("Failed to create TAI file in Consolidate") ;
                }
            }
        }
        if(bRet){
            //Copyintermediate files
            //Run calc
            m_pPifFile->SetTabOutputFName(m_pPifFile->GetCalcOutputFName());//for engine
            bRet = ExecCalc();
            bRet ? sMsg = _T("") : sMsg = _T("Failed to run calc module") ;
        }
        if(bRet){
            ///output tab files are ready now
            //Run prep
            CWaitCursor wait;
            bRet = ExecPrep();
            arrTempFiles.Add(m_pPifFile->GetCalcOutputFName());
            bRet ? sMsg = _T("") : sMsg = _T("Failed to run Prep module");
        }
#ifndef _PROCESSTHIS
        if(bRet) {//If the run is successful delete the temp files
            CIMSAString sFile;
            CIMSAString sFileExt = _T("");
            for(int iIndex = 0; iIndex  < arrTempFiles.GetSize(); iIndex++){
                sFile = arrTempFiles[iIndex];
                BOOL bDeleted = DeleteFile(sFile);
                //Remove the extension .tab if the Process is anything else
                int iDot = sFile.ReverseFind('.');
                if (iDot > 0) {
                    sFileExt = sFile.Mid(iDot + 1);
                    if(sFileExt.CompareNoCase(FileExtensions::BinaryTable::Tab) ==0){
                        PathRemoveExtension(sFile.GetBuffer(_MAX_PATH));
                        sFile.ReleaseBuffer();
                        sFile += FileExtensions::BinaryTable::WithDot::TabIndex;
                        bDeleted = DeleteFile(sFile);
                    }
                }
            }
        }
#endif
    }
    else if(eProcess == CS_TAB){
        bRet = ExecTab();
        bRet ? sMsg=_T(""):sMsg = _T("Failed to run tabulate module");
    }
    else if(eProcess == CS_CON){
        bRet = ExecCon();
        bRet ? sMsg = _T("") : sMsg = _T("Failed to run consolidate module") ;
        if(bRet){
            sTabOutPutFName = m_pPifFile->GetConOutputFName();
            sTabOutputTai =sTabOutPutFName;
            PathRemoveExtension(sTabOutputTai.GetBuffer(_MAX_PATH));
            sTabOutputTai.ReleaseBuffer();
            sTabOutputTai = sTabOutputTai + FileExtensions::BinaryTable::WithDot::TabIndex;

            bRet = CreateTAIFile(sTabOutPutFName,sTabOutputTai);
            bRet ? sMsg = _T("") : sMsg = _T("Failed to create TABIDX file in Consolidate") ;
        }
    }
    else if(eProcess == CS_PREP){//Same as "Format" in the CSelDlg .
        //Now calc and prep are combined
        //make sure that the output for calc is input for prep
        //Calc Input array is place holder actual input  comes from prep input files names
        std::vector<CString>& arrCalcInputFNamesArr = m_pPifFile->GetCalcInputFNamesArr();
        arrCalcInputFNamesArr.clear();

        if( !m_pPifFile->GetPrepInputFName().IsEmpty() )
            arrCalcInputFNamesArr.emplace_back(m_pPifFile->GetPrepInputFName());

        ASSERT(!arrCalcInputFNamesArr.empty());
        //Make output file name for calc step
        CIMSAString sOutputFolder = m_pPifFile->GetPrepOutputFName();
        PathRemoveFileSpec(sOutputFolder.GetBuffer());
        sOutputFolder.ReleaseBuffer();
        CIMSAString sCalcOutPutFileName = PifDlgBuilder::MakeOutputFileForProcess(CS_CALC, arrCalcInputFNamesArr.front(), sOutputFolder);
        m_pPifFile->SetCalcOutputFName(sCalcOutPutFileName);
        m_pPifFile->SetTabOutputFName(m_pPifFile->GetCalcOutputFName());//for engine
        arrTempFiles.Add(sCalcOutPutFileName);

        bRet = ExecCalc(); //Now run calc

        bRet ? sMsg = _T("") : sMsg = _T("Failed to run calc module") ;
        if(bRet){
            bRet = ExecPrep();
            //Delete calc.tab
            CIMSAString sFile =m_pPifFile->GetCalcOutputFName();
            BOOL bDeleted = DeleteFile(sFile);
            //Remove the extension .tab if the Process is anything else
            int iDot = sFile.ReverseFind('.');
            if (iDot > 0) {
                CIMSAString sFileExt = sFile.Mid(iDot + 1);
                if(sFileExt.CompareNoCase(FileExtensions::BinaryTable::Tab) ==0){
                    PathRemoveExtension(sFile.GetBuffer(_MAX_PATH));
                    sFile.ReleaseBuffer();
                    sFile += FileExtensions::BinaryTable::WithDot::TabIndex;
                    bDeleted = DeleteFile(sFile);
                }
            }
            //End Delete
            bRet ? sMsg = _T("") : sMsg = _T("Failed to run Prep module");
        }
    }
    else {
        ASSERT(FALSE);
        sMsg = _T("Invalid process type. Run failed");
    }

    if( bRet )
        DisplayAuxiliaryFilesPostRun();

    else {
        if(!bSilent){
            AfxMessageBox(sMsg);
        }
    }

    return bRet;

}
/////////////////////////////////////////////////////////////////////////////////
//
//  void CRunTab::SetAreaLabels4AllTables()
//
/////////////////////////////////////////////////////////////////////////////////
void CRunTab::SetAreaLabels4AllTables()
{
    ASSERT(m_pPifFile);
    std::shared_ptr<CTabSet> pTabSet = m_pApplication->GetTabSpec();

    CIMSAString sBreakKey,sAreaLabel;
    CMapStringToString& areaLabelLookup = pTabSet->GetAreaLabelLookup();
    int iNumAreaLevels = pTabSet->GetConsolidate()->GetNumAreas();
    if(iNumAreaLevels > 0){
        pTabSet->OpenAreaNameFile(m_pPifFile->GetAreaFName());
        pTabSet->BuildAreaLookupMap();
        for (int iTable =0 ; iTable <pTabSet->GetNumTables(); iTable++){
            CArray<CTabData*, CTabData*>&arrTabData =pTabSet->GetTable(iTable)->GetTabDataArray();
            if(arrTabData.GetSize()  < 1){
                continue;
            }

            for(int iTbdSlice =0; iTbdSlice < arrTabData.GetSize(); iTbdSlice++){

                CTabData* pTabData = arrTabData[iTbdSlice]; //zero for now when areas come in do the rest
                sBreakKey = pTabData->GetBreakKey();
                sBreakKey.Remove(';');
                sBreakKey.Replace(_T("-"),_T(" "));
                sBreakKey.MakeUpper();
                sAreaLabel = _T("");
                if(sBreakKey.IsEmpty()){//SAvy's fix for Glenn's bug with empty label when lowest level is "Total" 01/24/2008
                    POSITION    pos = areaLabelLookup.GetStartPosition();
                    CString sKey,sValue;
                    areaLabelLookup.GetNextAssoc(pos,sKey,sValue);
                    if(sKey.GetLength() > 0){
                        sBreakKey = CString(_T(' '),sKey.GetLength());
                    }
                }
                if(areaLabelLookup.Lookup(sBreakKey,sAreaLabel)){

                    CIMSAString sSeperatedBreakKey;
                    int iNumAreaLevels = pTabSet->GetConsolidate()->GetNumAreas();
                    CArray<int,int> arrAreaLen;
                    const CDataDict* pDict = pTabSet->GetDict();
                    int iStart=0;
                    for(int iArea =0; iArea < iNumAreaLevels ; iArea++){
                        const DictLevel* pDictLevel = NULL;
                        const CDictRecord* pDictRecord = NULL;
                        const CDictItem* pDictItem = NULL;
                        const DictValueSet* pDictVSet = NULL;

                        CString sAreaName = pTabSet->GetConsolidate()->GetArea(iArea);
                        const CDataDict* pDict= pTabSet->LookupName(sAreaName,&pDictLevel,&pDictRecord,&pDictItem,&pDictVSet);
                        ASSERT(pDict);
                        ASSERT((int)(iStart+pDictItem->GetLen()) <= sBreakKey.GetLength());
                        if(iArea ==iNumAreaLevels -1){
                            sSeperatedBreakKey += sBreakKey.Mid(iStart,pDictItem->GetLen());
                        }
                        else {
                            sSeperatedBreakKey += sBreakKey.Mid(iStart,pDictItem->GetLen())+_T(";");
                        }
                        iStart += pDictItem->GetLen();
                    }
                    pTabData->SetBreakKey(sSeperatedBreakKey);
                    pTabData->SetAreaLabel(sAreaLabel);
                }
            }
        }
        pTabSet->CloseAreaNameFile();
    }
}
/////////////////////////////////////////////////////////////////////////////////
//
//  void CRunTab::DeleteMultipleTBDFiles()
//
/////////////////////////////////////////////////////////////////////////////////
void CRunTab::DeleteMultipleTBDFiles()
{
    CTbdFile* pInputTbdFile = NULL;
    CTbiFile* pTBIFile = NULL;


    for(int iTBDFile =0; iTBDFile < m_arrConInputTBDFiles.GetSize();iTBDFile++){
        pInputTbdFile = m_arrConInputTBDFiles[iTBDFile];
        pInputTbdFile->Close();
        SAFE_DELETE(pInputTbdFile);
    }
    for(int iTBIFile =0; iTBIFile < m_arrConInputTBIFiles.GetSize();iTBIFile++){
        pTBIFile = m_arrConInputTBIFiles[iTBIFile];
        pTBIFile->Close();
        SAFE_DELETE(pTBIFile);
    }
    m_arrConInputTBDFiles.RemoveAll();
    m_arrConInputTBIFiles.RemoveAll();
    return;
}


void CRunTab::WriteConListingHeader(Listing::Lister& lister)
{
    std::vector<Listing::HeaderAttribute> header_attributes;

    header_attributes.emplace_back(_T("Application"), CS2WS(m_pPifFile->GetAppFName()));
    header_attributes.emplace_back(_T("Type"), _T("Consolidate"));

    for( const CString& file : m_pPifFile->GetConInputFilenames() )
        header_attributes.emplace_back(_T("Input Data"), CS2WS(file));

    header_attributes.emplace_back(_T("Output"), CS2WS(m_pPifFile->GetConOutputFName()));

    lister.WriteHeader(header_attributes);
}

void CRunTab::WriteConListingContent(Listing::Lister& lister)
{
    Listing::ListerWriteFile writer(&lister);
    lister.SetMessageSource(_T("Consolidate"));
    writer.WriteLine(std::wstring());

    writer.WriteLine(CS2WS(GetConLstHeader()));

    POSITION pos = m_mapConListing.GetStartPosition();

    std::shared_ptr<CTabSet> pTableSpec = m_pApplication->GetTabSpec();
    ASSERT(pTableSpec);
    CConsolidate* pConsolidate  = pTableSpec->GetConsolidate();
    ASSERT(pConsolidate);
    int iNumAreas = pConsolidate->GetNumAreas();
    CArray<int,int> arrAreaLen;
    const CDataDict* pDict = pTableSpec->GetDict();
    int iStart=0;
    CIMSAString sLine = _T("");
    for(int iArea =0; iArea < iNumAreas ; iArea++){
        const DictLevel* pDictLevel = NULL;
        const CDictRecord* pDictRecord = NULL;
        const CDictItem* pDictItem = NULL;
        const DictValueSet* pDictVSet = NULL;

        CString sAreaName = pTableSpec->GetConsolidate()->GetArea(iArea);
        const CDataDict* pDict= pTableSpec->LookupName(sAreaName,&pDictLevel,&pDictRecord,&pDictItem,&pDictVSet);
        ASSERT(pDict);
        ASSERT(pDictItem);
        arrAreaLen.Add(pDictItem->GetLen());
        int iMax =8;
        arrAreaLen[iArea] > 8 ? iMax = arrAreaLen[iArea] : iMax = iMax;
        sLine += CString(_T('-'),iMax) + _T(" ");
    }
    sLine += CString(_T('-'),11);//"table names" label
    writer.WriteLine(CS2WS(sLine));
    CIMSAString sKey,sValue;
    //Get the array of Keys
    CStringArray arrKeys;
     while(pos != NULL){
        m_mapConListing.GetNextAssoc(pos,sKey,sValue);
        arrKeys.Add(sKey);
     }
    qsort(arrKeys.GetData(), arrKeys.GetSize(), sizeof(CString), compare);
    for(int iKey =0; iKey < arrKeys.GetSize(); iKey++){
        sKey =arrKeys[iKey];
       // m_mapConListing.GetNextAssoc(pos,sKey,sValue);
        sValue = m_mapConListing[sKey];
        sKey.Replace(_T('-'),' ');
        int iStart = 0;
        sLine =_T("");
        for(int iArea =0; iArea < arrAreaLen.GetSize(); iArea++){
          CIMSAString sAreaValue;
          sAreaValue =sKey.Mid(iStart,arrAreaLen[iArea]);
          int iMax =8;
          arrAreaLen[iArea] > 8 ? iMax = arrAreaLen[iArea] : iMax = iMax;

          sAreaValue = sAreaValue.AdjustLenLeft(iMax);
          int iChar =0;
          while (iChar < sAreaValue.GetLength()){
              if(sAreaValue[iChar] == ' '){
                  iChar++;
                  continue;
              }
              else if(sAreaValue[iChar] == '0'){
                  sAreaValue.SetAt(iChar,' ');
                  iChar++;
                  continue;
              }
              else {
                  break;
              }
              iChar++;
          }

          sLine += sAreaValue + _T(" ");
          iStart += arrAreaLen[iArea];
        }
        sLine += sValue;
        writer.WriteLine(CS2WS(sLine));
    }
}

CIMSAString CRunTab::GetConLstHeader()
{
    CIMSAString sRet;
    std::shared_ptr<CTabSet> pTableSpec = m_pApplication->GetTabSpec();
    ASSERT(pTableSpec);
    CConsolidate* pConsolidate  = pTableSpec->GetConsolidate();
    ASSERT(pConsolidate);
    int iNumAreas = pConsolidate->GetNumAreas();
    CArray<int,int> arrAreaLen;
    const CDataDict* pDict = pTableSpec->GetDict();
    int iStart=0;
    for(int iArea =0; iArea < iNumAreas ; iArea++){
        const DictLevel* pDictLevel = NULL;
        const CDictRecord* pDictRecord = NULL;
        const CDictItem* pDictItem = NULL;
        const DictValueSet* pDictVSet = NULL;

        CString sAreaName = pTableSpec->GetConsolidate()->GetArea(iArea);
        const CDataDict* pDict= pTableSpec->LookupName(sAreaName,&pDictLevel,&pDictRecord,&pDictItem,&pDictVSet);
        ASSERT(pDict);
        ASSERT(pDictItem);
        arrAreaLen.Add(pDictItem->GetLen());
    }
    for(int iArea =0; iArea< iNumAreas ;iArea++){
       int iMax =8;
       arrAreaLen[iArea] > 8 ? iMax = arrAreaLen[iArea] : iMax = iMax;
       CIMSAString sAreaName = pConsolidate->GetArea(iArea).Mid(0,iMax);
       sAreaName = sAreaName.AdjustLenLeft(iMax);
       sRet +=  sAreaName + _T(" ");
    }
    sRet += _T("Table Names");
    return sRet;
}


void CRunTab::UpdateEngineMessagesVariables(std::shared_ptr<ProcessSummary> process_summary)
{
    if( process_summary == nullptr )
        return;

    size_t messages_non_1008 = process_summary->GetTotalMessages();

    if( messages_non_1008 > 0 )
    {
        const auto& counts_map = process_summary->GetSystemMessagesCountMap();
        const auto& lookup_1008 = counts_map.find(1008);

        if( lookup_1008 != counts_map.cend() )
        {
            m_engineIssued1008Messages = true;
            messages_non_1008 -= lookup_1008->second;
        }
    }

    m_engineIssuedNon1008Messages |= ( messages_non_1008 > 0 );
}


void CRunTab::DisplayAuxiliaryFilesPostRun(bool force_engine_issued_errors/* = false*/)
{
    if( m_pPifFile->GetViewResultsFlag() )
        ViewFileInTextViewer(m_pPifFile->GetWriteFName());

    if( m_pPifFile->GetViewListing() != NEVER )
    {
        ViewFileInTextViewer(m_pPifFile->GetApplicationErrorsFilename());

        if( force_engine_issued_errors || m_engineIssuedNon1008Messages || m_pPifFile->GetViewListing() == ALWAYS )
            Listing::Lister::View(m_pPifFile->GetListingFName());
    }

    if( m_engineIssued1008Messages )
        AfxMessageBox(_T("One or more tables may not have tabulated correctly because of an improper universe or weight specification."));
}
