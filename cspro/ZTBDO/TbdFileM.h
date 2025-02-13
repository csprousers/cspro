#pragma once
//--------------------------------------------------------------------------------------
//  File name: TbdFileMgr.h
//
//  Description:
//          Header for CTbdFileMgr class
//          This class is intended be used like interface to manipulate several tbd files
//
//  History:    Date       Author   Comment
//              ---------------------------
//              2 Jul 01   DVB      Created
//
//--------------------------------------------------------------------------------------

#include <ZTBDO/zTbdO.h>
#include <ZTBDO/TbdFile.h>

class CLASS_DECL_ZTBDO CTbdFileMgr  {
protected:
    CArray<CTbdFile*,CTbdFile*>     m_aTbdFiles;        // CTbdFile array

public:
    CTbdFileMgr();
    virtual ~CTbdFileMgr();

    bool Open( CString csFileName, bool bCreate = false );// Open Tbd and Tbi
    bool Close( CString csFileName );                   // Close Tbd and Tbi
    bool CloseAll();                                    // Close all the Tbd and Tbi files
    bool FlushAll();                                    // Flush all the Tbd Files
    CTbdFile*  GetTbdFile(CString csFileName );         // Get a CTbdFile instance by file name
};
