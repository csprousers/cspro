//***************************************************************************
//  File name: TVMISC.CPP
//
//  Description:
//       Misc view code for IMPS 4.0 TextView
//
//  History:    Date       Author     Comment
//              -----------------------------
//              1995        CSC       Created
//              6  Aug 96   CSC       fixed bug with ruler + end key in short files
//
//***************************************************************************

#include "StdAfx.h"
#include "afx.h"
#include <afxcoll.h>
#include <io.h>
#include <stdio.h>
#include <dos.h>
#include <zUtilO/FileUtil.h>

//////////////////////////////////////////////////////////////////////////////
// global variables
extern CString      csPadding;       // declared in ivview.cpp
extern CString      csLine;
//extern CWaitDialog* dlgWait;
extern CCriticalErrorMgr errorMgr;

/////////////////////////////////////////////////////////////////////////////////
//  functions used for searching for text ...

void SearchInit (const TCHAR* p, int next[])  {
    int i, j, M = _tcslen(p);
    next[0] = NONE;
    for ( i = 0 , j = NONE ; i < M ; i++, j++, next[i] = j)  {
        while ( (j>=0) && (p[i] != p[j]))  {
            j = next[j];
        }
    }
}

void AdjustForCase (CString& csCaseAdjustedSearchLine, BOOL bCaseSensitive)  {
    // make the string all caps if case sensitivity is OFF ...
    if (! bCaseSensitive)  {
        csCaseAdjustedSearchLine.MakeUpper();
    }
}

BOOL CBufferMgr::SearchForward (CString csSearch, BOOL bCaseSensitive, CLPoint& ptlFindPos, std::shared_ptr<ProgressDlg> dlgProgress)  {
    BOOL bFound = FALSE;         // TRUE if we have a search hit (to the right or below the current recent match)
    BOOL bLastFindIsDisplayed;   // TRUE if there is a currently highlighed "found" text, FALSE otherwise
    StatusType stStatus;
    long lPrevLine = GetCurrLine();
    int iOffset;
    int iFindCol;                // the col in which to start the search (usually 0, unless current highlight active,
                                 // returns the column position of the match, if a match occurred
    int next[100];
    int iPrevPos;
    BOOL bScaleProgress;
    long lUnscaledProgressRange;
    CString csCaseAdjustedSearchLine;
    const TCHAR* pszSearchText;

    // csc 4 jan 04
    if (!bCaseSensitive)  {
        csSearch.MakeUpper();
    }
    pszSearchText = (const TCHAR*) csSearch;

    ASSERT ((long) m_iFileWidth + 1L <= 32767L);
    SearchInit (pszSearchText, next);

    // ptlFindPos is (NONE, NONE) if there isn't a currently highlighted recently "found" text,
    // otherwise it contains the char position (row, col) of the currently highlighted "found" text
    if  (ptlFindPos != CLPoint((long) NONE, (long) NONE))  {
        bLastFindIsDisplayed = TRUE;
        iFindCol = (int) ptlFindPos.x + 1;
    }
    else  {
        bLastFindIsDisplayed = FALSE;
        iFindCol = 0;
    }

    if (Status() == BEGFILE)  {
        // ensure that we're at the first displayable line ...
        GetNextLine();
    }

    // handle the case where the whole file is displayed on 1 screen, and a previous find is displayed,
    // and that find isn't on the top (current) line ...
    // NOTE: CTVView::UpdateVScrollPos() will readjust our positon to the top of the file later, before the next call to OnDraw() ).
    while (bLastFindIsDisplayed && GetCurrLine() < ptlFindPos.y)  {
        GetNextLine();
        ASSERT(Status() != ENDFILE);
    }

    lUnscaledProgressRange = GetLineCount();
    bScaleProgress = (lUnscaledProgressRange > 65535);
    stStatus = Status();

    while (! bFound && stStatus != ENDFILE)  {
        /*--- update the progress bar and check for a cancel  ---*/
        if ((GetCurrLine() % 100) == 0)  {
            if (dlgProgress->CheckCancelButton())  {
                bFound = FALSE;
                break;
            }
            if (IsEstimatingNumLines())  {
                // the total number of lines could change ...
//                dlgProgress->SetRange(0, GetLineCount());
                dlgProgress->SetRange(0, std::min(65535, (int) GetLineCount()));
            }
            if (bScaleProgress)  {
                iPrevPos = dlgProgress->SetPos((int)(((float) 65535/lUnscaledProgressRange)*GetCurrLine()));
            }
            else  {
                iPrevPos = dlgProgress->SetPos(GetCurrLine());
            }
            ASSERT(iPrevPos <= dlgProgress->GetUpperRange());
        }

        stStatus = Status();
        csCaseAdjustedSearchLine = GetNextLine();
        if (stStatus != ENDFILE)  {
            // convert tabs to spaces in the line ...
            while ((iOffset = csCaseAdjustedSearchLine.Find((TCHAR) 9)) != NONE)  {
                csCaseAdjustedSearchLine = csCaseAdjustedSearchLine.Left (iOffset) + csPadding.Left (TAB_SPACES-(iOffset % TAB_SPACES)) + csCaseAdjustedSearchLine.Mid (iOffset+1);
            }

            // handle case senstivity ...
            AdjustForCase(csCaseAdjustedSearchLine, bCaseSensitive);
            if (m_pbuffActive->Search(pszSearchText, csCaseAdjustedSearchLine, next, iFindCol))  {
                if ( bLastFindIsDisplayed )  {
                    // a hit has to be to the right or below the currently highlighted hit ...
                    if (GetCurrLine() > ptlFindPos.y || ( GetCurrLine() == ptlFindPos.y && iFindCol > ptlFindPos.x))  {
                        bFound = TRUE;
                        ptlFindPos = CLPoint((long) iFindCol, GetCurrLine() - 1L);
                    }
                }
                else  {
                    bFound = TRUE;
                    ptlFindPos = CLPoint((long) iFindCol, GetCurrLine() - 1L);
                }
            }
        iFindCol = 0;   // all searches after the starting line occur from the 0th column...
        }
    }
    if (bFound)  {
        // back up one so that the line with the find displays on the top line of the view
        GetPrevLine();
    }
    else  {
        GotoLineNumber(lPrevLine);
    }
    return bFound;
}

BOOL CBufferMgr::SearchBackward (CString csSearch, BOOL bCaseSensitive, CLPoint& ptlFindPos, std::shared_ptr<ProgressDlg> dlgProgress)  {
    BOOL bFound = FALSE;         // TRUE if we have a search hit (to the right or below the current recent match)
    BOOL bLastFindIsDisplayed;   // TRUE if there is a currently highlighed "found" text, FALSE otherwise
    StatusType stStatus;
    long lPrevLine = GetCurrLine();
    int iOffset;
    int iFindCol = 0;
    int next[100];
    int iPrevPos;
    BOOL bScaleProgress;
    long lUnscaledProgressRange;
    const TCHAR* pszSearchText;

    // csc 4 jan 04
    if (!bCaseSensitive)  {
        csSearch.MakeUpper();
    }
    pszSearchText = (const TCHAR*) csSearch;

    CString csCaseAdjustedSearchLine;

    ASSERT ((long) m_iFileWidth + 1L <= 32767L);

    SearchInit(pszSearchText, next);

    // ptlFindPos is (NONE, NONE) if there isn't a currently highlighted recently "found" text,
    // otherwise it contains the char position (row, col) of the currently highlighted "found" text
    bLastFindIsDisplayed = (ptlFindPos != CLPoint ( (long) NONE, (long) NONE));

    if (Status() == BEGFILE)  {
        // ensure that we're at the first displayable line ...
        GetNextLine();
    }

    // handle the case where the whole file is displayed on 1 screen, and a previous find is displayed,
    // and that find isn't on the top (current) line ...
    // NOTE: CTVView::UpdateVScrollPos() will readjust our positon to the top of the file later, before the next call to OnDraw() ).
    while (bLastFindIsDisplayed && GetCurrLine() < ptlFindPos.y)  {
        GetNextLine();
        ASSERT (Status() != ENDFILE);
    }

    lUnscaledProgressRange = GetLineCount();
    bScaleProgress = (lUnscaledProgressRange > 65535);
    stStatus = Status();
    while (! bFound && stStatus != BEGFILE)  {
        /*--- update the progress bar and check for a cancel  ---*/
        if ((GetCurrLine() % 100) == 0)  {
            if (dlgProgress->CheckCancelButton())  {
                bFound = FALSE;
                break;
            }
            if (IsEstimatingNumLines())  {
                // the total number of lines could change ...
                dlgProgress->SetRange(0, GetLineCount());
            }
            if (bScaleProgress)  {
                iPrevPos = dlgProgress->SetPos((int)(((float) 65535/lUnscaledProgressRange)*(lPrevLine-GetCurrLine())));
            }
            else  {
                iPrevPos = dlgProgress->SetPos(lPrevLine - GetCurrLine());
            }
            ASSERT(iPrevPos <= dlgProgress->GetUpperRange());
        }

        stStatus = Status();
        csCaseAdjustedSearchLine = GetPrevLine();
        if (stStatus != BEGFILE)  {
            // convert tabs to spaces in the line ...
            while ((iOffset = csCaseAdjustedSearchLine.Find ((TCHAR) 9)) != NONE)  {
                csCaseAdjustedSearchLine = csCaseAdjustedSearchLine.Left(iOffset) + csPadding.Left (TAB_SPACES-(iOffset % TAB_SPACES)) + csCaseAdjustedSearchLine.Mid (iOffset+1);
            }

            // handle case senstivity ...
            AdjustForCase(csCaseAdjustedSearchLine, bCaseSensitive);
            if (m_pbuffActive->Search(pszSearchText, csCaseAdjustedSearchLine, next, iFindCol))  {
                // a hit was found on the same line
                int iMaxPrevFindCol;
                BOOL bFoundAnother;

                do  {
                    // work our way forwards through the line until we find the match that is closest
                    // to, but still previous, the last (highlighted) match
                    iMaxPrevFindCol = iFindCol;
                    bFoundAnother = m_pbuffActive->Search (pszSearchText, csCaseAdjustedSearchLine, next, ++iFindCol);
                }  while ((iFindCol < ptlFindPos.x || ! bLastFindIsDisplayed) && bFoundAnother);
                if (iMaxPrevFindCol < ptlFindPos.x || ! bLastFindIsDisplayed)  {
                    // yes, there was (at least) 1 other match previously on that line
                    bFound = TRUE;
                    ptlFindPos = CLPoint((long) iMaxPrevFindCol, GetCurrLine() + 1L);
                }
                else  {       // BMD  14 Dec 2004
                    ptlFindPos.x = (long) IOBUFSIZE + 1L;    // no match on "same" line, set the position to maximum line width + 1
                }
            }
            else  {       // BMD  15 Jul 2003
                ptlFindPos.x = (long) IOBUFSIZE + 1L;    // no match on "same" line, set the position to maximum line width + 1
            }
        iFindCol = 0;   // all searches after the starting line occur from the 0th column...
        }
    }
    if (bFound)  {
        // back up one so that the line with the find displays on the top line of the view
        GetNextLine();
    }
    else  {
        GotoLineNumber(lPrevLine);
    }
    return bFound;
}

int CBuffer::Search (const TCHAR* pszSearchText, const TCHAR* pszSearchLine, int next[], int& iFindCol)  {
// based on Knuth-Morris-Pratt search algorithm,  Find pattern p in text a
//    const char* p = pszSearchText;
//    char* a = m_caIOBuffer;
    int i, j, M, N;
    pszSearchLine += iFindCol;
    M = _tcslen (pszSearchText);
    N = (int) _tcslen (pszSearchLine);
    for ( i = j = 0 ; j < M && i < N ; i++, j++ )  {
        while ( (j>=0) && (pszSearchLine[i]!=pszSearchText[j]) )  {
            j = next[j];
        }
    }
    if ( j==M )  {
        // m_caIOBuffer +i-M is where the match is.
        // to do:  handle search forward,
        //         across buffers,
        //         case sensitivity
        //         menu-oriented IO
        //         match not found!
        iFindCol += i-M;
        return TRUE;
    }
    else  {
        return FALSE;
    }
}

CFileIO::CFileIO (void)  {
    m_iHandle = -1;        /* signal that it isn't open yet  */
    m_bEOF=FALSE;
    m_lFileSize=0L;
}

CFileIO::~CFileIO (void)  {
    Close();
}

BOOL CFileIO::FileExist (const CString& csName)  {
    CFileStatus status;
    return (CFile::GetStatus(csName, status) != 0);
}


BOOL CFileIO::SetFileName (const CString& cs)  {
    BOOL bRetCode;

    if ( FileExist (cs) )  {
        m_csFileName = cs;
        bRetCode = TRUE;
    }
    else  {
        CString csErrMsg;
        csErrMsg.LoadString (IDS_ERR09);
        csErrMsg += cs;
        AfxMessageBox (csErrMsg, MB_OK | MB_ICONEXCLAMATION );
        bRetCode = FALSE;
    }
    return bRetCode;
}

BOOL CFileIO::Open ( CString csFileName )  {
    if ( (m_iHandle = _topen ( (const TCHAR*) csFileName,  _O_BINARY | _O_RDONLY)) == -1)  {
        CString csErrMsg;
        csErrMsg.LoadString (IDS_ERR06);
        csErrMsg += csFileName;
        AfxMessageBox (csErrMsg, MB_OK | MB_ICONEXCLAMATION );
        return FALSE;
    }
    if (_filelengthi64(m_iHandle) > 2147483647i64) {    // BMD 02 Aug 2002
        AfxMessageBox(_T("Can't view files > 2GB"));
        return FALSE;
    }

    m_lFileSize = _filelength (m_iHandle);  // remember file size; csc 5/3/2004

    // store last modification date/time; csc 5/3/2004
    CFileStatus status;
    if (!CFile::GetStatus((const TCHAR*)csFileName, status)) {
        AfxMessageBox(_T("Internal error getting file status"));
        return FALSE;
    }
    m_timeCreate = status.m_mtime;
    m_unicodeEncoding= GetEncodingFromBOM(m_iHandle);
    bool isValidEncoding = (m_unicodeEncoding == Encoding::Utf8) || (m_unicodeEncoding == Encoding::Ansi) || (m_unicodeEncoding == Encoding::Utf16LE);
    if (!isValidEncoding){
        AfxMessageBox(FormatText(_T("%s\n\nCSPro does not support the specified text encoding."), (LPCTSTR)csFileName));
        return FALSE;
    }

    return TRUE;
}


BOOL CFileIO::Close (void)  {
    if ( m_iHandle != -1 )  {
        BOOL bRet = ( _close (m_iHandle) == 0);
        if(bRet) {
            m_iHandle  = -1;
        }
        return bRet;
    }
    else  {
        return TRUE;
    }
}

int  CFileIO::GetNumBytesToSkipBOM()
{
    int numBytetoSkip = 0;
    switch(m_unicodeEncoding)
    {
    case Encoding::Ansi:
        break;
    case Encoding::Utf16BE:
        numBytetoSkip=2;
        break;
    case Encoding::Utf16LE:
        numBytetoSkip=2;
        break;
    case Encoding::Utf8:
        numBytetoSkip=3;
        break;
    }
    return numBytetoSkip;

}
unsigned int CFileIO::Read (long lOffs, BYTE* buf)  {
    unsigned int bytes;
    // unsigned int numChars;

    // csc 5/3/2004 ... Keep file closed when we are not reading from it,
    // so that other processes can manipulate the file while it's open.
    // We'll "sense" when the file has changed, and reload as needed.
    Close();
/*
    if (RequiresClose()) {
        CString cs;
        cs.Format("File %s has been deleted, or is no longer available. File will be closed.", m_csFileName);
        AfxMessageBox(cs, MB_ICONEXCLAMATION);
        return 0;
    }
    if (RequiresReload()) {
        CString cs;
        cs.Format("File %s has been changed by another application and will be reloaded.", m_csFileName);
        AfxMessageBox(cs, MB_ICONEXCLAMATION);
        return 0;
    }
*/
    Open();

    if ( _lseek (m_iHandle, lOffs, SEEK_SET) == -1L )  {
    }

    //if(m_unicodeEncoding == Encoding::Utf8 || m_unicodeEncoding  == Encoding::Ansi){
    //  BYTE charBuff[(IOBUFSIZE+4)*sizeof(char)];
    //  bytes = _read (m_iHandle, charBuff, IOBUFSIZE);    // last byte might hold an EOS
    //  numChars = ConvertBufferToWideChar(charBuff, buf, bytes);
    //}
    //else {//UTF16
    //  memset(buf,0,IOBUFSIZE);
    //  bytes = _read (m_iHandle, buf, IOBUFSIZE);
    //  //numChars = bytes/2; // This is expecting TCHARS not bytes
    //}
    memset(buf,0,IOBUFSIZE);
    bytes = _read (m_iHandle, buf, IOBUFSIZE);

    if (bytes == IOBUFSIZE) {                    // BMD (04 Nov 2002) read but don't process
        //ASSERT(FALSE); //Unicode savy -- what about TCHAR = 2 bytes?? ..WE are just treating this as byte buffer . should be fine ??
        bytes--;
    }

    m_bEOF = _eof (m_iHandle); // moved out of AtEOF() function ... csc 5/3/2004
    Close();     // csc 5/3/2004

    return bytes;
}

 int CFileIO::ConvertBufferToWideChar(BYTE* source, LPTSTR dest, int srcLen)
{
    CMainFrame* pFrame = (CMainFrame*)AfxGetMainWnd();
    bool bLineDraw = false;
    if(pFrame){
        bLineDraw = pFrame->m_bLineDraw;
    }
    int numWChars = -1;
    switch(m_unicodeEncoding){
    case Encoding::Utf8:
        numWChars =  MultiByteToWideChar( CP_UTF8 , 0 , (LPCSTR)source , srcLen, NULL , 0 );
        MultiByteToWideChar( CP_UTF8 , 0 , (LPCSTR)source , srcLen, dest , numWChars );
        dest[numWChars] =0;
        break;
    case Encoding::Utf16BE:
        ASSERT(FALSE); //Windows supports only little endian
        break;
    case Encoding::Utf16LE:
        ASSERT(FALSE); //No need to convert just read the bytes as TCHARS
        break;
    case Encoding::Ansi:
        if(bLineDraw){
            numWChars =  MultiByteToWideChar( CP_OEMCP , 0 , (LPCSTR)source , srcLen, NULL , 0 );
            MultiByteToWideChar( CP_OEMCP, 0 , (LPCSTR)source , srcLen, dest , numWChars );
        }
        else{
            numWChars =  MultiByteToWideChar( CP_ACP , 0 , (LPCSTR)source , srcLen, NULL , 0 );
            MultiByteToWideChar( CP_ACP, 0 , (LPCSTR)source , srcLen, dest , numWChars );
        }
        dest[numWChars] =0;
        break;
    }

    return numWChars;

}
BOOL CFileIO::RequiresClose(void) const
{
    // break out if there is no file name (happens during initial file load)
    if (m_csFileName.IsEmpty()) {
        return FALSE;
    }

    // see if the file has recently been deleted ...
    CFileStatus status;
    return (!CFile::GetStatus((const TCHAR*)m_csFileName, status));
}

BOOL CFileIO::RequiresReload(void) const
{
    // break out if there is no file name (happens during initial file load)
    if (m_csFileName.IsEmpty()) {
        return FALSE;
    }

    // see if the file has recently been modified ...
    CFileStatus status;
    if (!CFile::GetStatus((const TCHAR*)m_csFileName, status)) {
        // file does not exist ... signal error
        AfxMessageBox(_T("Internal error checking reload status"));
        ASSERT(FALSE);
        return TRUE;     // file has been deleted ...
    }
    ASSERT(m_timeCreate<=status.m_mtime);
    return (m_timeCreate!=status.m_mtime);
}

CBufferMgr::CBufferMgr (void)  {
    m_bInitialized = FALSE;
    m_bEstimatingNumLines = FALSE;
    VERIFY ( (m_pbuffB1 = new (CBuffer)) != 0 );
    VERIFY ( (m_pbuffB2 = new (CBuffer)) != 0 );
}

CBufferMgr::~CBufferMgr (void)  {
    delete m_pbuffB1;
    delete m_pbuffB2;
}

void CBufferMgr::Init (void)  {
    m_pbuffB1->DeclareCurrFileIO ( &m_fileIO );
    m_pbuffB2->DeclareCurrFileIO ( &m_fileIO );
    m_pbuffB1->DeclareCurrBoundaryMgr ( &m_boundaryMgr );
    m_pbuffB2->DeclareCurrBoundaryMgr ( &m_boundaryMgr );
    CountLines ();
    LoadBuffer (m_pbuffB1);
    m_lCurrLine = 0L;
    m_iCurrColumn = 0;
}

void CBufferMgr::LoadBuffer (CBuffer* buffNew)  {
    buffNew->LoadBuffer ();
    m_pbuffActive = buffNew;
}

TCHAR* CBufferMgr::GetNextLine (void)  {
    TCHAR* pszRetVal;

    if ( m_pbuffActive->Status () == SWAPFORWARD || m_pbuffActive->Status () == SWAPBACKWARD )  {
        Swap ();
    }

    // csc 5/3/2004 ...
    if (m_pbuffActive->Status()==CLOSEFILE || m_pbuffActive->Status()==RELOADFILE) {
        return NULL;
    }

    pszRetVal = m_pbuffActive->GetNextLine ();
    if ( pszRetVal != NULL )  {
        m_lCurrLine++;
    }

    if ( IsEstimatingNumLines () )  {
       if ( (int) _tcslen ((LPTSTR)pszRetVal) > m_iFileWidth )  {
            ASSERT(FALSE);
            m_iFileWidth = (int) _tcslen ((LPTSTR)pszRetVal);
        }
        if ( Status() == ENDFILE )  {
            EndEstimateNumLines ();
        }
    }

    return pszRetVal;
}



TCHAR* CBufferMgr::GetPrevLine (void)  {
    TCHAR* pszRetVal;

    if ( m_pbuffActive->Status () == SWAPBACKWARD || m_pbuffActive->Status () == SWAPFORWARD )  {
        Swap ();
    }

    // csc 5/3/2004 ...
    if (m_pbuffActive->Status()==CLOSEFILE || m_pbuffActive->Status()==RELOADFILE) {
        return NULL;
    }

    pszRetVal = m_pbuffActive->GetPrevLine ();
    if ( pszRetVal != NULL )  {
        m_lCurrLine--;
    }
    return pszRetVal;
}

void CBufferMgr::Swap (void)  {
    CBuffer* buffNew;
    buffNew = GetBuffer ( m_pbuffActive->Status() );
    buffNew->InitBuffer ( m_pbuffActive );
    LoadBuffer ( buffNew );
}

void CBufferMgr::GotoLineNumber ( long lLineNumber )  {
// this function can be called for a variety of reasons: user menu (or toolbar) choice, thumbtrack scrollbar
// movement, top of file (Ctrl-Home) or bottom of file (Ctrl-End) commands, etc.
    CHourglass wait;
    ASSERT ( lLineNumber >= 0 );
    CBuffer* buffNew = GetBuffer (SWAPBACKWARD);        // arbitrary argument  this
    long lBoundary = ((CBufferBoundaryElement*) m_boundaryMgr.GetBoundaryForLineNumber (lLineNumber))->GetBoundary();

    /*------------------------------------------------------------------------------------------------------
       If we are currently estimating the number of lines in the file, then lBoundary won't be of
       use to us -- it will return the largest boundary, based on what of the file we have already seen.
       Thus, if we want to goto a line number > the max we've already seen ( m_lNumLines, as opposed to
       m_lNumLinesEst or CBufferMgr::GetLineCount()) then we'll need to read more through the file.
    ------------------------------------------------------------------------------------------------------*/

    buffNew->Init ( lBoundary, TRUE );
    LoadBuffer (buffNew);
    m_lCurrLine = ((CBufferBoundaryElement*) m_boundaryMgr.GetBoundaryForLineNumber (lLineNumber))->GetBeginningLineNumber();

    // We now are positioned at the start of the buffer we need, or the last buffer that we have information about (if file
    // size estimation is active).  Next we move forward to the particular line number we want.
    ASSERT ( GetCurrLine() <= lLineNumber );

    while ( GetCurrLine() < lLineNumber && Status() != ENDFILE && Status() != BEGFILE )  {
        GetNextLine ();
    }
}

CBuffer* CBufferMgr::GetBuffer (StatusType stDirection)  {
    if ( stDirection == SWAPFORWARD && m_boundaryMgr.GetNextBoundary (m_pbuffActive->GetEnd()) == (long) NONE )  {
        // can arrive here under 2 conditions:
        // - the first pass through .. we are setting up boundaries during the initial scan
        // - line number estimation is going on, and the user has tried to move past the number of lines initially scanned
        if ( IsEstimatingNumLines() )  {
            // we don't have a boundary for this buffer, so put it in at the current file position
            m_boundaryMgr.PutBoundary ( m_pbuffActive->GetEnd(), GetCurrLine() );
            EstimateNumLines ();
        }
        else  {
            m_boundaryMgr.PutBoundary ( m_pbuffActive->GetEnd(), m_lNumLines );
        }
    }
    if ( m_pbuffB1 == m_pbuffActive )
        return m_pbuffB2;
    else
        return m_pbuffB1;
}

void CBufferMgr::Down ( int iScrHgt, long iNumLines )  {
    int i, j,
        bEndFile;

    for ( long k = 0 ; k < iNumLines ; k++ )  {
        bEndFile = FALSE;
        for ( i = 0 ; i < iScrHgt-1 ; i++ )  {
            // check to see if moving down would move the bottom of the screen past EOF
            GetNextLine ();
            if ( Status () == ENDFILE )  {
                bEndFile = TRUE;
                GetPrevLine ();
                break;
            }
        }
        for ( j = 0 ; j < i ; j++ )  {
            // back us up to where we were
            GetPrevLine ();
        }

        if ( ! bEndFile )  {
            GetNextLine ();
        }
    }
}

void CBufferMgr::Up (int, long iNumLines)  {
    for (long i = 0 ; i < iNumLines ; i++ )  {
        GetPrevLine ();
    }
    if ( Status () == BEGFILE )  {
        // ensure that we're at the first displayable line ...
        GetNextLine ();
    }
}

void CBufferMgr::PgDown (int iScrHgt)  {
    int i, j;

    for ( i = 0 ; i <= iScrHgt-2 ; i++ )  {
        GetNextLine();
    }

    for ( i = 0 ; i < iScrHgt-1 ; i++ )  {
        // check to see if moving down would move the bottom of the screen past EOF
        GetNextLine ();
        if ( Status () == ENDFILE )  {
            i = iScrHgt - 1;
            break;
        }
    }
    for ( j = 0 ; j < i ; j++ )  {
        // back us up to where we were
        GetPrevLine ();
    }
}

void CBufferMgr::PgUp (int iScrHgt)  {
    for ( int i = 0 ; i <= iScrHgt-2 ; i++ )  {
        GetPrevLine ();
    }
    if ( Status () == BEGFILE )  {
        // ensure that we're at the first displayable line ...
        GetNextLine ();
    }
}

void CBufferMgr::PhysicalPgDown (int iScrHgt)  {
    int i, j;

    GetNextLine ();
    while ( Status() != ENDFILE && ! IsFormFeed() )  {
        GetNextLine();
    }

    for ( i = 0 ; i < iScrHgt-1 ; i++ )  {
        // check to see if moving down would move the bottom of the screen past EOF
        GetNextLine ();
        if ( Status () == ENDFILE )  {
            i = iScrHgt - 1;
            break;
        }
    }
    for ( j = 0 ; j < i ; j++ )  {
        // back us up to where we were
        GetPrevLine ();
    }
}

void CBufferMgr::PhysicalPgUp (int /*iScrHgt*/)  {
    GetPrevLine ();
    while ( Status() != BEGFILE && ! IsFormFeed() )  {
        GetPrevLine ();
    }

    if ( Status () == BEGFILE )  {
        // ensure that we're at the first displayable line ...
        GetNextLine ();
    }
}

void CBufferMgr::EndOfFile (int iScrHgt)  {
    GotoLineNumber ( GetLineCount() );
    if ( IsEstimatingNumLines() )  {
        // make sure we are at EOF; line number estimation usually won't be 100% accurate!
        while ( Status() != ENDFILE )  {
            GetNextLine ();
        }
        EndEstimateNumLines ();
    }
    ASSERT ( Status() == ENDFILE );
    for ( int i = 0 ; i < iScrHgt-1 ; i++ )  {
        // back us up so that a whole page is displayed
        GetPrevLine ();
    }
//    ASSERT ( Status() == ACTIVE );
    ASSERT (Status()==ACTIVE || Status()==BEGFILE);      // csc 8/6/96
}

void CBufferMgr::BeginOfFile (int)  {
    GotoLineNumber (0L);
    ASSERT ( Status() == ACTIVE || Status() == BEGFILE );
}

void CBufferMgr::Left (int iNumChars, int /* iScrWidth */)  {
    m_iCurrColumn -= iNumChars;
    if ( m_iCurrColumn < 0 )  {
        m_iCurrColumn = 0;
    }
}

void CBufferMgr::Right (int iNumChars, int iScrWidth)  {
    m_iCurrColumn += iNumChars;
    if ( m_iCurrColumn > GetFileWidth() - iScrWidth + 1 )  {
        m_iCurrColumn = std::max (GetFileWidth() - iScrWidth + 1, 0);
    }
}

void CBufferMgr::LeftOfFile (int /* iScrWidth */)  {
    m_iCurrColumn = 0;
}

void CBufferMgr::RightOfFile (int iScrWidth)  {
    m_iCurrColumn = GetFileWidth() - iScrWidth + 1;
    if (m_iCurrColumn < 0)  {
        // for short files;  csc 8/6/96
        m_iCurrColumn = 0;
    }
}

void CBufferMgr::EstimateNumLines (void)  {
    // estimates the number of lines in the file; called after the "wait" dialog has been displayed for too long...
    m_lNumLinesEst = (long) ( m_boundaryMgr.GetMaxLineNumber() * (float) GetFileSize() / m_boundaryMgr.GetMaxBoundary() );
    m_bEstimatingNumLines = TRUE;
}


void CBufferMgr::CountLines (void)  {

    int    iOffset;
    BOOL   bBigFile;
    time_t timeStart;
    UINT   uSeconds;

    m_lNumLines = 0L;
    m_iFileWidth = 0;
    LoadBuffer (m_pbuffB1);
    bBigFile = (GetFileSize() > BIG_FILE);

    time(&timeStart);
    CWaitCursor cursorWait;   // puts up an hourglass
    while (Status () != ENDFILE && ! errorMgr.IsError())  {
        if (bBigFile & (GetLineCount() % 500 == 0))  {
            uSeconds = (UINT) (time(NULL) - timeStart);
            if (m_boundaryMgr.GetMaxBoundary() > 0 && uSeconds > MAX_INITIAL_SCAN_SECONDS)  {
                // wait message has been up long enough, let's estimate the number of lines in the file
                // Note that we have to check the boundary manager as well, so that we don't risk the possibility of a
                // division by zero when we estimate the number of lines (GetMaxBoundary() will be the denominator there).
                EstimateNumLines ();
                m_lNumLines = m_boundaryMgr.GetMaxLineNumber();
                break;
            }
        }
        csLine = GetNextLine ();

        while ((iOffset = csLine.Find ((TCHAR) 9)) != NONE)  {
            // convert tabs to spaces in the line ...
            csLine = csLine.Left (iOffset) + csPadding.Left (TAB_SPACES-(iOffset % TAB_SPACES)) + csLine.Mid (iOffset+1);
        }
        if (csLine.GetLength() > m_iFileWidth)  {
            m_iFileWidth = csLine.GetLength();
        }
        m_lNumLines++;
    }
    m_pbuffB1->Init ();
    m_pbuffB2->Init ();
    m_bInitialized = TRUE;
    csLine.Empty();
}

CBuffer::CBuffer (void)  {
    Init ();
}

void CBuffer::Init (void)  {
    m_lAbsBegin = m_lAbsEnd = 0L;
    m_stStatus = EMPTY;
    m_iCurrLine = 0;
    m_waFormFeedArray.RemoveAll();
    m_bDirty = TRUE;
}

void CBuffer::Init ( long lBoundary, BOOL bDirtFlag )  {
    // this function is used in conjunction with GotoLineNumber...
    Init();
    m_lAbsBegin = lBoundary;
    m_bDirty = bDirtFlag;
}

void CBuffer::SetLines (unsigned int uBuffBytes)  {
    int i = 0;
    unsigned int iCount = 0;
    BOOL bAtEndOfBuffer = FALSE;
    BOOL bHitEOFMarker = FALSE;

    if ( (m_lAbsBegin == 0L) || (m_lAbsBegin == m_currFileIO->GetNumBytesToSkipBOM()) )  {
        // we are at BOF ...
        m_iaOffs [i++] = BOF_SIGNAL;
        if ( m_iCurrLine == 0 )  {
            // need to bump it past the BOF ...
            m_iCurrLine++;
        }
    }
    m_iaOffs [i++] = 0;
    while ( i < MAXLINESPBUFF && ! bAtEndOfBuffer )  {
        if(m_currFileIO->GetEncoding() != Encoding::Utf16LE){
            while ( m_caIOBuffer[iCount] != CR )  {
                if ( m_caIOBuffer[iCount] == FF )  {
                    // case of FF not preceeded by CR
                    m_caIOBuffer[iCount] = EOS;
                    m_waFormFeedArray.Add ( (WORD) i );
                    break;
                }
                if ( m_caIOBuffer[iCount] == LF )  {          // BMD (8 Aug 95) to allow for LF as EOL
                    // case of LF not preceeded by CR
                    m_caIOBuffer[iCount] = EOS;
                    break;
                }
                if ( m_caIOBuffer[iCount] == END_FILE)  {
                    // spotted an EOF character; treat it as the end of file (equivalent to a DOS "type")
                    bAtEndOfBuffer = TRUE;
                    bHitEOFMarker = TRUE;
                    break;
                }
                if ( (int) iCount < (int) uBuffBytes - 1 )  {   // BMD 07 Dec 2002 & 07 Jul 2003
                    iCount++;
                }
                else {             // BMD 07 Jul 2003
                    if (m_caIOBuffer[iCount+1] == CR || iCount >= uBuffBytes) {
                        bAtEndOfBuffer = TRUE;
                        break;
                    }
                    else {
                        iCount++;
                    }
                }
            }
            if ( m_caIOBuffer[iCount+1] == FF )  {
                // case of CR/FF  (as opposed to CR/LF)
                m_waFormFeedArray.Add ( (WORD) i );
            }
            if (m_caIOBuffer [iCount] != EOS) {     // BMD (17 Nov 95) LF and FF chop off last char in line
                if (m_caIOBuffer [iCount+1] == LF || m_caIOBuffer [iCount+1] == FF) {  // BMD (13 Mar 2002) handle CR only
                    m_caIOBuffer [iCount++] = EOS;  // the ++ advances past the 2nd byte of the CR/LF pair
                }
                else {
                    m_caIOBuffer [iCount] = EOS;    // BMD (13 Mar 2002) handle CR only
                }
            }
            m_iaOffs [i++] = ++iCount;
        }
        else{
            TCHAR* caIOBuffer = (TCHAR*)m_caIOBuffer;
            while ( caIOBuffer[iCount] != CR )  {
                if ( caIOBuffer[iCount] == FF )  {
                    // case of FF not preceeded by CR
                    caIOBuffer[iCount] = EOS;
                    m_waFormFeedArray.Add ( (WORD) i );
                    break;
                }
                if ( caIOBuffer[iCount] == LF )  {          // BMD (8 Aug 95) to allow for LF as EOL
                    // case of LF not preceeded by CR
                    caIOBuffer[iCount] = EOS;
                    break;
                }
                if ( caIOBuffer[iCount] == END_FILE)  {
                    // spotted an EOF character; treat it as the end of file (equivalent to a DOS "type")
                    bAtEndOfBuffer = TRUE;
                    bHitEOFMarker = TRUE;
                    break;
                }
                if ( (int) iCount < (int) uBuffBytes - 1 )  {   // BMD 07 Dec 2002 & 07 Jul 2003
                    iCount++;
                }
                else {             // BMD 07 Jul 2003
                    if (caIOBuffer[iCount+1] == CR || iCount*2 >= uBuffBytes) {
                        bAtEndOfBuffer = TRUE;
                        break;
                    }
                    else {
                        iCount++;
                    }
                }
            }
            if ( caIOBuffer[iCount+1] == FF )  {
                // case of CR/FF  (as opposed to CR/LF)
                m_waFormFeedArray.Add ( (WORD) i );
            }
            if (caIOBuffer [iCount] != EOS) {     // BMD (17 Nov 95) LF and FF chop off last char in line
                if (caIOBuffer [iCount+1] == LF || caIOBuffer [iCount+1] == FF) {  // BMD (13 Mar 2002) handle CR only
                    caIOBuffer [iCount++] = EOS;  // the ++ advances past the 2nd byte of the CR/LF pair
                }
                else {
                    caIOBuffer [iCount] = EOS;    // BMD (13 Mar 2002) handle CR only
                }
            }
            m_iaOffs [i++] = ++iCount*2;
        }

    }
    m_lAbsEnd = m_lAbsBegin + (long) m_iaOffs[i-1];
    if ( (m_currFileIO->AtEOF() || bHitEOFMarker) && bAtEndOfBuffer)  {
        m_iaOffs[i-1] = EOF_SIGNAL;
    }
    else  {
        if ( bAtEndOfBuffer && i < MAXLINESPBUFF )  {
            if ( i <= 3 )  {
                // this file is too wide; we couldn't even read in 1 line!
                if ( !errorMgr.IsError() )  {
                    // haven't told the user yet
                    AfxMessageBox(IDS_WARN01, MB_OK | MB_ICONEXCLAMATION);
                    errorMgr.Throw ();
                }
            }
            m_lAbsEnd = m_lAbsBegin + (long) m_iaOffs[i-2];    // last line read wasn't complete, so set the threshold back a line
            m_iaOffs[i-2] = SWAP_SIGNAL;
        }
    }
}

void CBuffer::InitBuffer (CBuffer* buffPrev)  {
    ASSERT ( buffPrev->Status() == SWAPFORWARD || buffPrev->Status() == SWAPBACKWARD );
    switch (buffPrev->Status() )  {
    case SWAPFORWARD:
        m_bDirty = ( m_lAbsBegin != buffPrev->m_lAbsEnd );
        m_lAbsBegin = buffPrev->m_lAbsEnd;
        m_iCurrLine = 0;
        break;
    case SWAPBACKWARD:
        m_iCurrLine = MAXLINESPBUFF;
        m_bDirty = ( m_lAbsBegin != m_currBoundaryMgr->GetPrevBoundary (buffPrev->m_lAbsBegin) );
        m_lAbsBegin = m_currBoundaryMgr->GetPrevBoundary (buffPrev->m_lAbsBegin);
        break;
    }
}

void CBuffer::LoadBuffer (void)  {
    unsigned int uBuffBytes;

    if ( m_bDirty )  {

        // csc 5/3/2004 ... detect file deletion or modification
        if (m_currFileIO->RequiresClose()) {
            CString cs;
            cs.Format(_T("File %s has been deleted, or is no longer available. File will be closed."), (LPCTSTR)m_currFileIO->GetFileName());
//            AfxMessageBox(cs, MB_ICONEXCLAMATION);
//            m_stStatus=CLOSEFILE;
            m_iaOffs[m_iCurrLine]=CLOSEFILE_SIGNAL;
            return;
        }
        if (m_currFileIO->RequiresReload()) {
            CString cs;
            cs.Format(_T("File %s has been changed by another application and will be reloaded."), (LPCTSTR)m_currFileIO->GetFileName());
//            AfxMessageBox(cs, MB_ICONEXCLAMATION);
//            m_stStatus=RELOADFILE;
            m_iaOffs[m_iCurrLine]=RELOADFILE_SIGNAL;
            return;
        }

        if(m_lAbsBegin ==0){
            //Savy for skipping the BOM
            m_lAbsBegin += m_currFileIO->GetNumBytesToSkipBOM();
        }
        uBuffBytes = m_currFileIO->Read (m_lAbsBegin, m_caIOBuffer);
        m_waFormFeedArray.RemoveAll();
        SetLines (uBuffBytes);//Savy moved this line above for Unicode
    }
    if ( m_iCurrLine == MAXLINESPBUFF )  {
        // this signals "last line of the buffer, which might be < MAXLINESPBUFF
        for ( m_iCurrLine = 0 ; m_iCurrLine < MAXLINESPBUFF-2 ; m_iCurrLine++ )  {
            if ( m_iaOffs[m_iCurrLine] == SWAP_SIGNAL )  {
                m_iCurrLine--;
                break;
            }
        }
    }
    m_stStatus = ACTIVE;
}

TCHAR* CBuffer::GetPrevLine (void)  {
    if ( m_iCurrLine == BOF_SIGNAL )  {
        // we are at just past the top of a buffer ...
        m_stStatus = SWAPBACKWARD;
        return NULL;
    }
    if ( m_iaOffs [m_iCurrLine] == BOF_SIGNAL )  {
        m_stStatus = BEGFILE;
        return NULL;
    }
    m_stStatus = ACTIVE;

	if(m_currFileIO->GetEncoding() == Encoding::Utf16LE){
		return (LPTSTR)m_caIOBuffer + (m_iaOffs[m_iCurrLine--]/2);
	}
	else{
		m_currFileIO->ConvertBufferToWideChar(m_caIOBuffer+m_iaOffs[m_iCurrLine--], m_LineBuffer,  -1);
		return m_LineBuffer;
	}
}

TCHAR* CBuffer::GetNextLine (void)  {
    if ( m_iCurrLine == MAXLINESPBUFF-1 || m_iaOffs[m_iCurrLine] == SWAP_SIGNAL)  {  // csc 3/2/94
        m_stStatus = SWAPFORWARD;
        return NULL;
    }
    if ( m_iaOffs [m_iCurrLine] == EOF_SIGNAL )  {
        m_stStatus = ENDFILE;
        return NULL;
    }

    m_stStatus = ACTIVE;

	if(m_currFileIO->GetEncoding() == Encoding::Utf16LE){
		return (LPTSTR)m_caIOBuffer + (m_iaOffs[m_iCurrLine++]/2);
	}
	else{
		m_currFileIO->ConvertBufferToWideChar(m_caIOBuffer+m_iaOffs[m_iCurrLine++], m_LineBuffer,  -1);
		return m_LineBuffer;
	}
}

//TCHAR* CBuffer::GetNextLineU (void)  {
//
//	LPTSTR* pLine = NULL;
//	BYTE* pNextLine =GetNextLine();
//
//	if(pNextLine){
//		//use the bom check
//		//convert the bytes to unicode in the TCHAR buffer.
//	}
//	return pNextLine;
//
//	//CString str((LPCSTR)pNextLine,sizeof(TCHAR));
//
//	//CString csResult;
//
// //   //int             iLen=_tcslen((LPTSTR)pNextLine);
// //   wchar_t*  pBuff=csResult.GetBufferSetLength( 71 );
//
// //   mbstowcs( pBuff, (LPSTR)pNextLine, 71 );
// //   pBuff[71] = 0;
//
// //   csResult.ReleaseBuffer();
//
// //   return (LPTSTR)(LPCTSTR)csResult;
//}
BOOL CBuffer::IsFormFeed (void)  {
    for ( int i = 0 ; i < m_waFormFeedArray.GetSize() ; i++ )  {
        if ( m_waFormFeedArray[i] == (WORD) m_iCurrLine )  {
            return TRUE;
        }
    }
    return FALSE;
}

StatusType CBuffer::Status (void)  {
    if ( m_iCurrLine == -1 )
        m_stStatus = SWAPBACKWARD;
    else if ( m_iaOffs [m_iCurrLine] == BOF_SIGNAL )
        m_stStatus = BEGFILE;
    else if ( m_iaOffs [m_iCurrLine] == EOF_SIGNAL )
        m_stStatus = ENDFILE;
    else if ( m_iCurrLine == MAXLINESPBUFF-1 || m_iaOffs[m_iCurrLine] == SWAP_SIGNAL )
        m_stStatus = SWAPFORWARD;
    else if (m_iaOffs[m_iCurrLine]==RELOADFILE_SIGNAL)
        m_stStatus = RELOADFILE;
    else if (m_iaOffs[m_iCurrLine]==CLOSEFILE_SIGNAL)
        m_stStatus = CLOSEFILE;
    else
        m_stStatus = ACTIVE;

    return m_stStatus;
}

//////////////////////////////////////////////////////////////////////
//  CBufferBoundaryMgr implementation

IMPLEMENT_DYNAMIC(CBufferBoundaryElement, CObject)

void CBufferBoundaryMgr::PutBoundary (long lBnd, long lLineNumber)  {
    // assumes that these get put in in ascending order
    m_elementArray.Add ( new CBufferBoundaryElement ( lBnd, lLineNumber ) );
}

long CBufferBoundaryMgr::GetNextBoundary (long lBnd)  {
    for ( int i = 0 ; i < m_elementArray.GetSize() ; i++ )  {
        if ( ( (CBufferBoundaryElement*)m_elementArray[i])->GetBoundary () >= lBnd )  {
            return ( (CBufferBoundaryElement*)m_elementArray[i])->GetBoundary ();
        }
    }
    return (long) NONE;
}

long CBufferBoundaryMgr::GetPrevBoundary (long lBnd)  {
    for ( int i = m_elementArray.GetSize()-1 ; i >= 0 ; i-- )  {
        if ( ( (CBufferBoundaryElement*)m_elementArray[i])->GetBoundary () < lBnd )  {
            return ( (CBufferBoundaryElement*)m_elementArray[i])->GetBoundary ();
        }
    }
    return (long) NONE;
}

CBufferBoundaryMgr::CBufferBoundaryMgr (void)  {
    PutBoundary ( 0L, 0L );
}

CBufferBoundaryMgr::~CBufferBoundaryMgr (void)  {
    for ( int i = 0 ; i < m_elementArray.GetSize() ; i++ )  {
        delete m_elementArray[i];
    }
}

inline long CBufferBoundaryMgr::GetMaxBoundary (void)  {
    return ( (CBufferBoundaryElement*)m_elementArray[m_elementArray.GetSize()-1])->GetBoundary ();
}

inline long CBufferBoundaryMgr::GetMaxLineNumber (void)  {
    return ( (CBufferBoundaryElement*)m_elementArray[m_elementArray.GetSize()-1])->GetBeginningLineNumber ();
}

const CBufferBoundaryElement* CBufferBoundaryMgr::GetBoundaryForLineNumber ( long lLineNumber )  {
    for ( int i = m_elementArray.GetSize()-1 ; i >= 0 ; i-- )  {
        if ( ( (CBufferBoundaryElement*)m_elementArray[i])->GetBeginningLineNumber() <= lLineNumber)  {
            return (CBufferBoundaryElement*)m_elementArray[i];
        }
    }
    ASSERT (FALSE);    // can't go on from here!
    return NULL;
}


//////////////////////////////////////////////////////////////////////////////
// implementation of CHourglass, for displaying the hourglass (wait) cursor...
//

CHourglass::CHourglass (CWnd* pWnd)  {
    m_hSaveCursor = SetCursor ( LoadCursor (NULL, IDC_WAIT) );
    if ( (m_pCapWnd = pWnd) != NULL)  {
        pWnd->SetCapture();
    }
}

CHourglass::~CHourglass ()  {
    SetCursor ( m_hSaveCursor );
    if ( m_pCapWnd )  {
        ReleaseCapture ();
    }
}

//////////////////////////////////////////////////////////////////////////////
// implementation of CCriticalErrorMgr, for quickly taking action on errors
//

void CCriticalErrorMgr::NotifyCloseView (void)  {
    ASSERT(m_pCurrView->IsKindOf(RUNTIME_CLASS(CTVView)));
    m_bErrorOccurred = TRUE;
}


void swap (int& a, int& b)  {
    int tmp;
    tmp = a;
    a = b;
    b = tmp;
}

/******************************************************************************
*                                                                             *
*                                  itoc                                       *
*                                                                             *
*                                                                             *
*       Function:   Convert a short integer to a character array.             *
*                                                                             *
*       Prototype:  void itoc (char *, int, int, int)                         *
*                                                                             *
*       To call:    itoc (ch, value, len, fill)                               *
*                                                                             *
*          (out)    ch    -- Character array                                  *
*           (in)    value -- Integer value                                    *
*           (in)    len   -- Number of characters                             *
*           (in)    fill  -- Character to fill on left (BLANK or ZERO)        *
*                                                                             *
******************************************************************************/

void itoc (TCHAR ch[], int value, int len, int fill)  {
    int i, val, v;

    val = value;
    for (i = len - 1; i >= 0; i--) {
        if (val == 0)  {
            if (i == len - 1)  {
                ch[i] = _T('0');
            }
            else  {
                ch[i] = (TCHAR) fill;
            }
        }
        else {
            v = val/10;
            ch[i] = (TCHAR) (((int) '0') + (val - 10*v));
            val = v;
        }
    }
}


/******************************************************************************
*                                                                             *
*                                  ltoc                                       *
*                                                                             *
*                                                                             *
*       Function:   Convert a long integer to a character array.              *
*                                                                             *
*       Prototype:  void ltoc (char *, long, int, int)                        *
*                                                                             *
*       To call:    ltoc (ch, value, len, fill)                               *
*                                                                             *
*          (out)    ch    -- Character array                                  *
*           (in)    value -- Integer value                                    *
*           (in)    len   -- Number of characters                             *
*           (in)    fill  -- Character to fill on left (BLANK or ZERO)        *
*                                                                             *
******************************************************************************/

void ltoc (TCHAR ch[], long value, int len, int fill)  {
    int i;
    long val, v;

    val = value;
    for (i = len - 1; i >= 0; i--) {
        if (val == 0) {
            if (i == len - 1)  {
                ch[i] = _T('0');
            }
            else  {
                ch[i] = (TCHAR) fill;
            }
        }
        else {
            v = val/10;
            ch[i] = (TCHAR) (((int) '0') + (val - 10*v));
            val = v;
        }
    }
}


