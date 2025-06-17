#pragma once
//---------------------------------------------------------------------------
//  File name: TbdFile.h
//
//  Description:
//          Header for CTbdFile class
//          This class is intended define an interface to manipulate a tbd file
//
//  History:    Date       Author   Comment
//              ---------------------------
//              2 Jul 01   DVB      Created
//
//---------------------------------------------------------------------------

#include <ZTBDO/zTbdO.h>
#include <ZTBDO/TbiFile.h>
#include <ZTBDO/BreakIt.h>
#include <ZTBDO/TbdTable.h>
#include <ZTBDO/TbdSlice.h>
#include <ZTBDO/TbdFile.h>


class CLASS_DECL_ZTBDO CTbdFile  {
protected:
    CString                                                  m_csFileName;          // File Name
    int                                                      m_iFd;                 // Tbd File Descriptor

    CArray<CBreakItem*, CBreakItem*> m_aBreakItem;          // Breaks array definition
    CArray<CTbdTable*, CTbdTable*>   m_aTbdTable;           // Tables array definition
    CArray<CTbdSlice*, CTbdSlice*>   m_aTbdSlice;           // Slice  array definition
    int                                                      m_iBreakLen;           // The general break len
    long                                                     m_lTrlOffset;          // Trailer offset
    int                                                      m_iMaxNameLen;         // Max table len name (without blank)
    char                                                     m_cVersion;            // The tbdfile version
    long                                                     m_iFileSize;

public:
    CTbdFile(CString csFileName=_T(""), char cVersion = 1);
    void     Init();
    virtual ~CTbdFile();

    bool        WriteAllSlices(bool bFreeData);                    // Rebuild the index File;
    bool        WriteTrailer();
    bool        Open( bool bCreate ); // Open and create trailer
    void        SetFileName( CString csFileName ); // RHF Oct 21, 2002
    bool        DoOpen(bool bCreate );
    int         GetFd();
    bool        Close();                                   // Close Tbd and remove backup file.
    bool        Flush();                                   // Flush tbd and refresh backup file
    void        ShowInfo();
    void        ShowSMeanOrSTbl(); //Savy 4 Variances
    int         GetNumTables();
    CTbdTable*  GetTable(int iTable);
    CTbdTable*  GetTable(csprochar* pszTableName);
    CString     GetFileName();
    CTbdSlice*  GetNextSlice( long& lFilePos, bool bForUpdate = false );
    bool        AddSlice(CTbdSlice* ptSlice, bool bCreateTable );                // Copy the slice instance.
    bool        AddBreak(CBreakItem* bItem);
    bool        AddTable(CTbdTable* tTable);
    CBreakItem* GetBreak(int iIndex);
    int         GetNumBreakItems(void) { return m_aBreakItem.GetSize();}
    int         GetTablePos(CString csTableName);

    static  short GetTableNum( const csprochar* pszKey, int iSlotSize );
    static  int   SetTableNum( csprochar* pszKey, short sTableNum, int iSlotSize );

    int        GetBreakKeyLen();
    long       GetFileSize() const; // size of file in bytes (only valid when open for read)
    void       SetBreakKeyLen(int iBreakKeylen) ;//SAVY 10/05/2005 for setting full breakkey in slices

protected:
    void EmptyTbd();
};
