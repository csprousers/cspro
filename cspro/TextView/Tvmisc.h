#pragma once

//***************************************************************************
//  File name: TVMisc.h
//
//  Description:
//       Interface for the miscellaneous TextView code and classes
//
//  History:    Date       Author     Comment
//              -----------------------------
//              1995        csc       created
//
//***************************************************************************

#include <io.h>

#define IOBUFSIZE                   1024*100*sizeof(TCHAR)
#define MAXLINESPBUFF               20000             // minimum value for MAXLINESPBUFF is maximal screen height + 1
#define MAX_INITIAL_SCAN_SECONDS    3                // after this many seconds, intial file scan will switch to approximation
#define BIG_FILE                    100L*1024L
#define YES 1
#define NO  0

#define NONE -1

#define BOF_SIGNAL         -1       // signals beginning of file
#define EOF_SIGNAL         -2       // signals end of file
#define LEFT_SIGNAL        -4
#define RIGHT_SIGNAL       -5
#define SWAP_SIGNAL        -3       // signals end of buffer ( lines/buffer < MAXLINESPBUFF )
#define CLEAR_SIGNAL       -6
#define RELOADFILE_SIGNAL  -7
#define CLOSEFILE_SIGNAL   -8

// timer event definitions
#define UP_TIMER     1
#define DOWN_TIMER   2
#define LEFT_TIMER   3
#define RIGHT_TIMER  4
//#define SEARCH_TIMER 5
#define SCROLL_TIMER 6

// extended scroll codes, handles physical PgUp/Dn (for going to next/prev form feed)
#define SB_PHYSICAL_PAGEDOWN    11
#define SB_PHYSICAL_PAGEUP      12

// colors for things ... distinguishes between 16- and 256-color displays
#define RULER_BCOLOR (((CTextViewApp*)AfxGetApp())->m_iNumColors>=256?RGB(192,220,192):RGB(0,128,128))
#define RULER_FCOLOR (((CTextViewApp*)AfxGetApp())->m_iNumColors>=256?RGB(0,0,0):RGB(255,255,255))


#define EOS _T('\0')
#define FF  _T('\f')
#define CR  _T('\r')
#define LF _T('\n')
#define END_FILE ((TCHAR) 26)

#define TAB_SCROLL_SIZE   5

#define NON_DISPLAYABLE_CHARACTER    254




typedef enum { BACKWARD = -1, SAME, FORWARD } Direction;
typedef enum { EMPTY, ACTIVE, SWAPFORWARD, SWAPBACKWARD, BEGFILE, ENDFILE, CLOSEFILE, RELOADFILE } StatusType;
// CLOSEFILE, RELOADFILE added csc 5/3/2004

#define _O_BINARY   0x8000  /* file mode is binary (untranslated) */
#define _O_RDONLY   0x0000  /* open for reading only */

/////////////////////////////////////////////////////////////////////////////
// general prototypes
void itoc (TCHAR [], int, int, int);
void ltoc (TCHAR [], long, int, int);
void swap (int&, int&);

/////////////////////////////////////////////////////////////////////////////
//  CLPoint class ... like CPoint, but with long int components...

class CLPoint  {
    public:
        long x, y;

        CLPoint ()                               { /* random filled */ }
        CLPoint (long initX, long initY)         { x = initX; y = initY; }
        CLPoint ( CPoint point )                 { x = (long) point.x;  y = (long) point.y;  }
        void Offset(long xOffset, long yOffset)  { x += xOffset; y += yOffset; }
        void Offset(CLPoint point)               { x += point.x; y += point.y; }
        CLPoint operator-() const                { return CLPoint(-x, -y); }
        BOOL operator==(const CLPoint& ptlPoint) const { return (x==ptlPoint.x && y==ptlPoint.y);  }
        BOOL operator!=(const CLPoint& ptlPoint) const { return (x != ptlPoint.x || y != ptlPoint.y); }
};



/////////////////////////////////////////////////////////////////////////////
//  CLRect class ... like CRect, but with long int components ...

class CLRect  {
    public:
        long left, top, right, bottom;

        CLRect(void)                             { /* random filled */ }
        CLRect(long l, long t, long r, long b)   { left = l; top = t; right = r; bottom = b; }
        long Width(void) const                   { return right - left; }
        long Height(void) const                  { return bottom - top; }
        BOOL IsRectNull(void) const              { return (left == 0L && right == 0L && top == 0L && bottom == 0L); }
        void OffsetRect(long x, long y)          { left += x;  right += x;  top += y;  bottom += y;  }
        void OffsetRect(CLPoint point)           { OffsetRect ( point.x, point.y );  }
        void operator= (const CRect& rcRect)  {
            left = (long) rcRect.left;    top = (long) rcRect.top;
            right = (long) rcRect.right;  bottom = (long) rcRect.bottom;
        }
        CRect CLRectToCRect (void)  {
                return ( CRect ( (int) left, (int) top, (int) right, (int) bottom ) );
        }
};


class CFileIO  {
    public:
        CFileIO (void);
        ~CFileIO (void);
        BOOL FileExist (const CString&);
        BOOL SetFileName (const CString&);
        inline CString GetFileName (void)   { return m_csFileName;  }
        BOOL Open (CString);
        inline BOOL Open (void)  { return Open (m_csFileName);  }
        BOOL Close (void);
        unsigned int Read (long, BYTE*);

//        long GetFileSize (void)  { return _filelength (m_iHandle);  }
        long GetFileSize(void) const { return m_lFileSize; }   // changed to work while underlying file is closed ... csc 5/3/2004
//        BOOL AtEOF (void)  {  return ( _eof (m_iHandle) );  }
        BOOL AtEOF(void) const { return m_bEOF; }   // changed to work while underlying file is closed ... csc 5/3/2004
        BOOL RequiresReload(void) const;   // csc 5/3/2004
        BOOL RequiresClose(void) const;   // csc 5/3/2004
        void Reload(void); // csc 5/3/2004

        int ConvertBufferToWideChar(BYTE* source, LPTSTR dest, int srcLen);
        int  GetNumBytesToSkipBOM(); //Skip Byte order Mark
        Encoding GetEncoding(){ return m_unicodeEncoding;}

    private:

        Encoding m_unicodeEncoding;
        int  m_iHandle;
        CString m_csFileName;
        BOOL m_bEOF;        // csc 5/3/2004
        long m_lFileSize;   // csc 5/3/2004
        CTime m_timeCreate; // last file modified date/time stamp ...csc 5/3/2004
};

class CBufferBoundaryElement : public CObject  {
    DECLARE_DYNAMIC (CBufferBoundaryElement)
    private:
        long m_lBound;
        long m_lBeginningLineNumber;

    public:
        CBufferBoundaryElement (long x, long y) { m_lBound = x;  m_lBeginningLineNumber = y; }
        CBufferBoundaryElement (void)           { CBufferBoundaryElement ( NONE, NONE );  }
        long GetBoundary (void)                 { return m_lBound;  }
        long GetBeginningLineNumber (void)      { return m_lBeginningLineNumber;  }
    };

class CBufferBoundaryMgr  {
    private:
        CObArray m_elementArray;

    public:
        CBufferBoundaryMgr (void);
        ~CBufferBoundaryMgr (void);
        void PutBoundary (long, long);
        long GetPrevBoundary (long);
        long GetNextBoundary (long);
        long GetMaxBoundary (void);
        long GetMaxLineNumber (void);
        const CBufferBoundaryElement* GetBoundaryForLineNumber (long);
};

class CBuffer  {
    public:
        CBuffer (void);
        void Init (void);
        void Init (long lLineNumber, BOOL bDirtFlag);       // intialize for a particular line number
        void LoadBuffer (void);
        void InitBuffer (CBuffer*);
        TCHAR* GetNextLine (void);
        TCHAR* GetPrevLine (void);
        StatusType  Status (void);
        long GetEnd (void)  { return m_lAbsEnd;  }
        BOOL Search (const TCHAR*, const TCHAR*, int [], int& );
        void DeclareCurrFileIO (CFileIO* f)  {  m_currFileIO = f;  }
        void DeclareCurrBoundaryMgr (CBufferBoundaryMgr* b)  { m_currBoundaryMgr = b; }
        BOOL IsFormFeed (void);

    private:
        void SetLines (unsigned int);

        int  m_iCurrLine;
        int  m_iaOffs [MAXLINESPBUFF];
        CWordArray m_waFormFeedArray;
        BYTE m_caIOBuffer [(IOBUFSIZE+4)];    // BMD (04 Nov 2002) extra bytes
        TCHAR m_LineBuffer[IOBUFSIZE];
        StatusType m_stStatus;
        CFileIO* m_currFileIO;
        CBufferBoundaryMgr* m_currBoundaryMgr;
        BOOL m_bDirty;
        long m_lAbsBegin,
             m_lAbsEnd;
};


class CBufferMgr  {
    public:
        CBuffer* m_pbuffActive;
        CBufferMgr (void);
        ~CBufferMgr (void);
        void Init (void);
        void Down (int, long = 1);
        void Up (int, long = 1);
        void PgDown (int);
        void PgUp  (int);
        void PhysicalPgDown (int);
        void PhysicalPgUp (int);
        void BeginOfFile (int);
        void EndOfFile (int);
        void Left (int, int);
        void Right (int, int);
        void LeftOfFile (int);
        void RightOfFile (int);
        void GotoLineNumber (long);
        TCHAR* GetNextLine (void);
        TCHAR* GetPrevLine (void);
        void CountLines (void);
        BOOL SearchForward (CString csSearch, BOOL, CLPoint&, std::shared_ptr<ProgressDlg>);
        BOOL SearchBackward (CString csSearch, BOOL, CLPoint&, std::shared_ptr<ProgressDlg>);
        StatusType Status (void)         { return m_pbuffActive->Status(); }
        long GetLineCount (void)         { return (m_bEstimatingNumLines ? m_lNumLinesEst : m_lNumLines);  }
        int GetFileWidth (void)          { return m_iFileWidth; }
        long GetCurrLine (void)          { return m_lCurrLine; }
        int  GetCurrCol (void)           { return m_iCurrColumn; }
        BOOL IsInitialized (void)        { return m_bInitialized;  }
        BOOL IsFormFeed (void)           { return m_pbuffActive->IsFormFeed(); }
        CFileIO* GetFileIO (void)        { return &m_fileIO; }
        long GetFileSize (void)          { return m_fileIO.GetFileSize ();  }
        Encoding GetFileEncoding (void)  { return m_fileIO.GetEncoding();  }
        BOOL IsEstimatingNumLines (void) { return m_bEstimatingNumLines;  }
        void EstimateNumLines (void);
        void EndEstimateNumLines (void)  { m_bEstimatingNumLines = FALSE;  m_lNumLines = GetCurrLine();  }
        BOOL RequiresClose(void) const   { return m_fileIO.RequiresClose(); }
        BOOL RequiresReload(void) const  { return m_fileIO.RequiresReload(); }

    private:
        void LoadBuffer (CBuffer*);
        void Swap (void);
        CBuffer* GetBuffer (StatusType);

        CBuffer* m_pbuffB1;                // first buffer
        CBuffer* m_pbuffB2;                // second buffer
        CFileIO m_fileIO;                  // file being viewed
        CBufferBoundaryMgr m_boundaryMgr;  // buffer information (file offsets, line numbers)
        BOOL m_bInitialized;               // TRUE after iNumLines, etc. have been set
        long m_lNumLines;                  // number of lines in the file (really counted)
        long m_lNumLinesEst;               // number of lines in the file (estimated; prevents "wait" initial scan from taking too long)
        BOOL m_bEstimatingNumLines;        // TRUE if we don't yet know the real number of lines in the file
        int  m_iFileWidth;                 // number of characters in widest line in the file
        long m_lCurrLine;                  // current TOS line number (absolute from start of file)
        int  m_iCurrColumn;                // current offset from left of view (char-based); even though the bufferMgr makes no use of this internally, it is here for consistency sake since the m_lCurrLine is here too
};


//////////////////////////////////////////////////////////////////////////////////////////
//   CHourglass class displays a wait cursor (hourglass) easily ...
//          this code is borrowed from Microsft Systems Journal November 1993, page 22
//

class CHourglass  {
    private:
        CWnd*   m_pCapWnd;          // window to capture mouse, if any
        HCURSOR m_hSaveCursor;      // save cursor handle
    public:
        CHourglass (CWnd* pWnd = NULL);
        ~CHourglass ();
};


///////////////////////////////////////////////////////////////////////////////////////////
//  CCriticalError class lets anyone signal an error to the view.
//

class CCriticalErrorMgr  {
    private:
        CView* m_pCurrView;
        BOOL   m_bErrorOccurred;

    public:
        CCriticalErrorMgr ()          { m_pCurrView = NULL;  m_bErrorOccurred = FALSE;  }
        void SetCurrView (CView* pV)  { m_pCurrView = pV; }
        void NotifyCloseView (void);
        void Throw (void)             { NotifyCloseView(); }
        void Reset (void)             { m_bErrorOccurred = FALSE;  }
        BOOL IsError (void)           { return m_bErrorOccurred; }
};
