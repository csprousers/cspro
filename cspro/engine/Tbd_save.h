#pragma once

#ifdef USE_BINARY

#elif WIN_DESKTOP

#include <ZTBDO/TbdFile.h>

class CBatchDriverBase;


#define LEN_BLOCK   4096

#define G_PARAM  _T("YYYYYYNNYYYNY24. .,NNCYYNY132 60 320000")

#define BLANKS   _T("          ")

#define TBDMAXVARS 200

#define LNAME8   8

struct TBVAR
{
    csprochar varname[LNAME8];
    int       SYMTidx;
};

struct TBBVAR
{
    int       varnum;  // Var number
    csprochar len;     // Var lenght
};



class CTbd
{
private:
    int     m_iMaxNumTables;
    int     m_iNumTables;
    int     m_iNumVars;

    TBVAR*  m_aVar;
    TBBVAR* m_aBreakVar;
    csprochar*   m_aTbdTable;

    int     *m_pAuxCtNodebase;                   // CtNodeBase auxiliar
    int     m_iAuxCtNodenext;                // CtNodeNext auxiliar
    int     m_iCtabTemp;                    // temp. header file

    CString m_csTbdFileName;
    csprochar*   m_pBreakidnext;
    csprochar*   m_pBreakidcurr;
    long    m_iBreakValue[MAXBREAKVARS];

    // Tbi files
    SimpleDbMap* m_pTableIndex;

    // TbdFile
    int         m_iTbdFile;
    CTbdFile*   m_pTbdFile;

    // Tbd Format
    bool    m_bNewTbd;

    // Table id len
    int     m_iTableIdLen;

    // Extension
    CString      m_csTbiExtension;
    CString      m_csTbdExtension;

    long breakfpos( csprochar *btreeid );
    void breakmakeid( csprochar *p );
    void breakload( csprochar *breakid, CTAB *ct );


    void tbd_init( const TCHAR* name );                        // alloc/fill TbTable; open TBD file

    void tbd_MakeFinalTbd();
    void tbd_Delete( CString csName );
    int  tbd_tabindex( const csprochar *tname );                    // index of "tname" in array m_aTbdTable
    void tbd_DoArrays();
    void tbd_Open();
    void tbd_Close();                                   // Close file TBD and file of TITLES
    void tbd_DoWriteTrailer();
    int  tbd_copytree( int iOffSet );                   // RHF Jul 13, 2001
    void tbd_addvar( int desp );
    int  tbd_newvar( int symidx );
    int  tbd_newtable( int symidx );
    void tbd_catheader( int f1, int f2 );               // concats header of f1 to file f2
    void tbd_fcopy( int f1, int f2, long len );         // copy from f1 to f2

public:
    CEngineDriver*  m_pEngineDriver;
#ifdef USE_BINARY
#else
    CBatchDriverBase*   m_pBatchDriverBase;
#endif
    CEngineArea*    m_pEngineArea;
    EngineData*     m_engineData;
    CSettings*      m_pEngineSettings;
    CIntDriver*     m_pIntDriver;

    CTbd();
    ~CTbd();

#ifdef USE_BINARY
#else
    void    SetBatchDriver( CBatchDriverBase* pBatchDriverBase );
#endif

    // break stuff
    void    breakcheckid();
    bool    breakinit( const TCHAR* pszTbdName );
    void    breakend(void);
    void    breakclose(void);
    void    FlushAll( const TCHAR* pszBreakId, int iCurrentBreakKeyNum ); // RHF Oct 22, 2002

    void    breaksave( const TCHAR* breakid, CTAB *ct );
    int     breaklen( int iNumBreakVarsPrefix );// RHF Apr 16, 2003

    csprochar*   GetCurrId()  { return( m_pBreakidcurr ); }

    // write the TBD file to disk
    void    tbd_WriteTrailer();

    // Tbd format
    void SetNewTbd( bool bNewTbd ) {
        m_bNewTbd = bNewTbd;

        m_iTableIdLen = m_bNewTbd ? sizeof(short) : sizeof(csprochar);

        m_csTbdExtension = m_bNewTbd ? FileExtensions::BinaryTable::Tab : FileExtensions::BinaryTable::Tbd;
        m_csTbiExtension = m_bNewTbd ? FileExtensions::BinaryTable::TabIndex : FileExtensions::BinaryTable::TbdIndex;
    }

    bool IsNewTbd() const            { return m_bNewTbd; }

    //GetTableIdLen is sizeof(short). But since we are in unicode and the storage is TCHAR we need to use only the number of characters -Savy 01/31/2012 for unicode changes
    int  GetTableIdLen() const       { return m_iTableIdLen/sizeof(TCHAR); }

    CString GetTbdExtension() const  { return m_csTbdExtension; }
    CString GetTbiExtension()  const { return m_csTbiExtension; }

private:
    const Logic::SymbolTable& GetSymbolTable() const;
};

#endif
