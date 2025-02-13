// tbdview.cpp : Defines the entry point for the console application.
//

#include "Stdafx.h"
#include <io.h>
#include <fcntl.h>
#include <ZTBDO/TbdFileM.h>

#ifdef _DEBUG
#define new DEBUG_NEW
#undef THIS_FILE
static char THIS_FILE[] = __FILE__;
#endif

/////////////////////////////////////////////////////////////////////////////
// The one and only application object

CWinApp theApp;



int _tmain(int argc, TCHAR* argv[], TCHAR* /*envp*/[])
{
    int         nRetCode = 0;
    CTbdFileMgr tfMgr;
    CTbdFile*   ptFile;

    _setmode(_fileno(stdout), _O_U16TEXT);
    // initialize MFC and print and error on failure
    if (!AfxWinInit(::GetModuleHandle(NULL), NULL, ::GetCommandLine(), 0))
    {
        // TODO: change error code to suit your needs
        std::wcerr << _T("Fatal Error: MFC initialization failed") << std::endl;
        nRetCode = 1;
    }
    else
    {
        if (argc != 2) {
            std::wcerr << _T("------------------------------\n");
            std::wcerr << _T("Command use: tbdview <tbdfile>\n");
            std::wcerr << _T("------------------------------\n");
            return 1;
        }

        if ( !tfMgr.Open(argv[1], false) ) {
            std::wcerr << _T("File <") << argv[1] << _T("> not found.\n");
            return 1;
        }

        ptFile = tfMgr.GetTbdFile(argv[1]);
        ptFile->ShowInfo();
#ifndef _DEBUG
             //Create the new TBD File
        CTbdFile* pTbdNewFile = NULL;
        CString csFileName(argv[1]);
        csFileName = _T(".\\testx.tab");
        pTbdNewFile = new CTbdFile(csFileName);
        bool bCreate = true;

        if ( !bCreate ) {
            if ( !pTbdNewFile->Open(bCreate) ) {
                pTbdNewFile->Close();
                delete ptFile;
                return false;
            }
        }
        else {
            if ( !pTbdNewFile->DoOpen(bCreate) ) {
                pTbdNewFile->Close();
                delete pTbdNewFile;
                return false;
            }
        }
        //Add slices
        //Save the File

        for(int iTable =0; iTable < ptFile->GetNumTables(); iTable++){
            CTbdTable* pTbdTable= ptFile->GetTable(iTable);
            CTbdTable* pNewTbdTable =new CTbdTable(*pTbdTable);

            CArray<CBreakItem*, CBreakItem*>&arrBreakItems = pNewTbdTable->GetBreakArray();
            arrBreakItems.RemoveAll();

            for ( int i = 0; i < pTbdTable->GetNumBreak(); i++ ) {
                CBreakItem* pBreakItem = pTbdTable->GetBreak(i);
                CBreakItem* pNewBreakItem = new CBreakItem(*pBreakItem);
                pNewTbdTable->AddBreak(pNewBreakItem);
            }

            pTbdNewFile->AddTable(pNewTbdTable);
            CTbdSlice* pTbdSlice = NULL;
            CTbdSlice* pNewTbdSlice = NULL;
            long        lSliceFilePos=0L;
            bool bFirst = true;
            while ( (pTbdSlice = ptFile->GetNextSlice(lSliceFilePos)) != NULL && pTbdSlice->GetTable() == pTbdTable ) {
                if(bFirst){ //Add one extra slice in the beginning
                    bFirst = false;
                    CTableAcum* pAcum = pTbdSlice->GetAcum();

                    pNewTbdSlice=new CTbdSlice( pNewTbdTable );
                    CTableAcum* pNewAcum=new CTableAcum( pNewTbdTable );
                    pNewAcum->ShareArea(*pAcum);

                    pNewTbdSlice->SetAcum( pNewAcum, true );

                    pNewTbdSlice->SetSliceHdr(pTbdSlice->GetSliceHdr());
                    pNewTbdSlice->SetBreakKey(_T("12  "),4);

                    pTbdNewFile->AddSlice( pNewTbdSlice, false );
                }
                CTableAcum* pAcum = pTbdSlice->GetAcum();

                pNewTbdSlice=new CTbdSlice( pNewTbdTable );
                CTableAcum* pNewAcum=new CTableAcum( pNewTbdTable );
                pNewAcum->ShareArea(*pAcum);

                pNewTbdSlice->SetAcum( pNewAcum, true );

                pNewTbdSlice->SetSliceHdr(pTbdSlice->GetSliceHdr());
                pNewTbdSlice->SetBreakKey(pTbdSlice->GetBreakKey(),pTbdSlice->GetBreakKeyLen());

                pTbdNewFile->AddSlice( pNewTbdSlice, false );
            }
        }
        pTbdNewFile->WriteAllSlices(true);
        pTbdNewFile->WriteTrailer();
        pTbdNewFile->Close();
        delete pTbdNewFile;
#endif
        tfMgr.CloseAll();
    }

    return nRetCode;
}
