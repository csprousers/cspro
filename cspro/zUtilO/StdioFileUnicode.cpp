#include "StdAfx.h"
#include "StdioFileUnicode.h"
#include <zToolsO/TextConverter.h>
#include <zToolsO/TextEncoding.h>
#include <zPlatformO/PlatformInterface.h>


ULONGLONG CStdioFileUnicode::GetPosition() const
{
#ifdef WIN_DESKTOP
    // 20111214 VS2010 has a bug in the ftell function with UTF-8 files so this is necessary; the functions returns negative (or really large ULONGLONG) numbers
    int64_t pos;

    try
    {
        pos = CStdioFile::GetPosition();

        int64_t* pLastPos = const_cast<int64_t*>(&m_lastPos);
        *pLastPos = pos;
    }
    catch(...) // 20120126 GetPosition throws an exception occasionally, which is strange, so we'll use the last position in these cases
    {
        pos = m_lastPos;
    }

    return pos > 0 ? pos : 0;
#else
    return CStdioFile::GetPosition();
#endif
}


ULONGLONG CStdioFileUnicode::FlushAndGetPosition() // 20111221 it seems that if you flush the buffer then GetPosition returns a correct value (this is due to the VS2010 bug)
{
    Flush();
    return CStdioFile::GetPosition();
}


#ifdef WIN_DESKTOP
BOOL CStdioFileUnicode::Open(LPCTSTR lpszFileName, UINT nOpenFlags, CFileException* pError)
{
    bool openSuccess = true;
    CString openString;
    int shareFlag = 0;

#ifdef _DEBUG
    UINT handledFlags = CFile::modeRead | CFile::modeWrite | CFile::modeCreate | CFile::typeText | CFile::shareExclusive | CFile::shareDenyWrite | CFile::modeNoTruncate;
    ASSERT( ( handledFlags | nOpenFlags ) == handledFlags);
#endif


    if( nOpenFlags & CFile::modeWrite )
    {
        if ( nOpenFlags & CFile::modeCreate && ( !( nOpenFlags & CFile::modeNoTruncate ) || !PortableFunctions::FileExists(lpszFileName) ) )
        {
            openString = _T("wt");
        }

        else
        {
            openString = _T("r+t");
        }

        shareFlag = _SH_DENYRW;
    }

    else // CFile::modeRead
    {
        ASSERT(( nOpenFlags & CFile::modeCreate ) == 0);

        openSuccess = GetFileBOM(lpszFileName,m_encoding);

        if( openSuccess )
        {
            if( m_encoding != Encoding::Utf8 && m_encoding != Encoding::Utf16LE && m_encoding != Encoding::Ansi )
            {
                CString csLoad,csError;
                csLoad.LoadString(IDS_UNSUPPORTEDBOM);
                csError.Format(csLoad,lpszFileName);
                AfxMessageBox(csError,MB_ICONEXCLAMATION); // this error should be so uncommon that I will ignore any InSilent flags that might be set in subclasses
                return false;
            }

            openString = _T("rt");
            shareFlag = _SH_DENYWR;
        }
    }


    if( nOpenFlags & CFile::shareExclusive ) // if for some reason they have chosen a share status different from the ones specified above
    {
        shareFlag = _SH_DENYRW;
    }

    else if( nOpenFlags & CFile::shareDenyWrite )
    {
        shareFlag = _SH_DENYWR;
    }


    if( openSuccess )
    {
        if( m_encoding == Encoding::Utf8 )
        {
            openString.Append(_T(",ccs=UTF-8"));
        }

        else if( m_encoding == Encoding::Utf16LE )
        {
            openString.Append(_T(",ccs=UTF-16LE"));
        }

        else if( m_encoding != Encoding::Ansi )
        {
            ASSERT(false);
        }

        m_pStream = _tfsopen(lpszFileName,openString,shareFlag);

        openSuccess = m_pStream != NULL;
    }

    if( !openSuccess && pError ) // for now, rather than figure out all of the CFileException errors, just open the file again with CStdioFile to get the error
        CStdioFile::Open(lpszFileName,nOpenFlags,pError);

    if( openSuccess )
    {
        m_strFileName = lpszFileName; // 20111220 this gets used in functions like GetFilePath()

        // 20120326 a combination of relative and absolute paths, using forward slashes, was causing havoc
        m_strFileName.Replace('/','\\');

        // 20120628 i will convert relative paths to absolute ones because some functionality (e.g., writing out a enc file with files in different folders) didn't work properly
        TCHAR sPath[_MAX_PATH + 1];
        GetCurrentDirectory(_MAX_PATH,sPath);
        m_strFileName = WS2CS(MakeFullPath(sPath, CS2WS(m_strFileName)));

        // make sure the file is closed in the destructor
        m_bCloseOnDelete = TRUE;
    }

    return openSuccess;
}


FILE* CStdioFileUnicode::_tfopen(LPCTSTR lpszFileName,LPCTSTR lpszOpenFlags,Encoding forcedEncoding,Encoding* pReturnEncoding) // 20120109
{
    Encoding enc;
    CString openString = lpszOpenFlags;

    if( ( GetFileBOM(lpszFileName,enc) && forcedEncoding != Encoding::Ansi ) || forcedEncoding == Encoding::Utf8 || forcedEncoding == Encoding::Utf16LE )
    {
        if( enc == Encoding::Utf8 || forcedEncoding == Encoding::Utf8 )
        {
            openString.Append(_T(",ccs=UTF-8"));
            enc = Encoding::Utf8;
        }

        else if( enc == Encoding::Utf16LE || forcedEncoding == Encoding::Utf16LE )
        {
            openString.Append(_T(",ccs=UTF-16LE"));
            enc = Encoding::Utf16LE;
        }
    }

    if( pReturnEncoding != nullptr )
        *pReturnEncoding = enc;

    FILE * pFile = ::_tfopen(lpszFileName,openString);

    return pFile;
}


FILE* CStdioFileUnicode::CreateFileWithoutBOM(LPCTSTR lpszFileName,Encoding forcedEncoding) // for Stata exports
{
    ASSERT(forcedEncoding == Encoding::Ansi || forcedEncoding == Encoding::Utf8);

    FILE* pFile = CStdioFileUnicode::_tfopen(lpszFileName,_T("w"),forcedEncoding);

    if( pFile != nullptr && forcedEncoding == Encoding::Utf8 ) // go to the beginning of the file so the BOM will be overwritten
        fseek(pFile,0,SEEK_SET);

    return pFile;
}

#endif



#ifndef WIN_DESKTOP
// this function will open a file, and if in creation mode, will write out the BOM if the file size is 0
FILE* CStdioFileUnicode::_tfopen(LPCTSTR lpszFileName,LPCTSTR lpszOpenFlags,Encoding forcedEncoding,Encoding * pReturnEncoding) // 20131028
{
    Encoding enc;
    CString openString = lpszOpenFlags;

    if( forcedEncoding != Encoding::Utf8 )
        assert(true);

    // make sure that the file is being opened for writing
    const TCHAR * pos = wcsstr(lpszOpenFlags,_T("r+"));
    if( !pos )
        pos = wcsstr(lpszOpenFlags,_T("w"));
    if( !pos )
        pos = wcsstr(lpszOpenFlags,_T("a"));
    assert(pos);

    FILE* pFile = PortableFunctions::FileOpen(lpszFileName,openString);

    if( !pFile )
        return NULL;

    if( ftell(pFile) == 0 )
    {
        fseek(pFile,0,SEEK_END);

        if( ftell(pFile) == 0 ) // an empty file, so write out the BOM
            fwrite(TextEncoding::Utf8Bom_sv.data(), 1, TextEncoding::Utf8Bom_sv.length(), pFile);

        fseek(pFile,3,SEEK_SET); // go to just after the BOM
    }

    return pFile;
}
#endif


bool CStdioFileUnicode::ReadTextFile(NullTerminatedString filename, CString& buffer) // 20120109
{
    ASSERT(buffer.IsEmpty());

    // read a whole data file and return it as a CString
    CStdioFileUnicode file;
    CString line;

    if( !file.Open(filename.c_str(), CFile::modeRead) )
        return false;

    while( file.ReadString(line) )
    {
        buffer.Append(line);

#ifdef WIN32
        buffer.Append(_T("\n"));
#else
        buffer.Append(_T("\r\n"));
#endif
    }

    file.Close();

    return true;
}


#ifdef WIN_DESKTOP
bool CStdioFileUnicode::ConvertAnsiToUTF8(const std::string& file_path) // 20120120
{
    CFile ansiFile;
    CFile utf8File;

    TCHAR tempPath[MAX_PATH - 14 + 1];
    TCHAR tempFilename[MAX_PATH + 1];

    if( !GetTempPath(MAX_PATH - 14,tempPath) || !GetTempFileName(tempPath,_T("CSP"),0,tempFilename) )
        return false;

    if( !ansiFile.Open(UTF8_TODO::GetWide(file_path).c_str(), CFile::modeRead) )
        return false;

    if( !utf8File.Open(tempFilename, CFile::modeWrite) )
    {
        ansiFile.Close();
        return false;
    }

    utf8File.Write(TextEncoding::Utf8Bom_sv.data(), TextEncoding::Utf8Bom_sv.length());

    const int BUFFER_SIZE = 64 * 1024;
    char mbBuffer[BUFFER_SIZE * 2]; // every byte in an ansi file will map to at most two UTF-8 bytes
    TCHAR wBuffer[BUFFER_SIZE];

    ULONGLONG remainingSize = ansiFile.GetLength();
    UINT idatalen = 0;
    int ioutdata = 0;

    while( remainingSize )
    {
        idatalen = ansiFile.Read(mbBuffer,(UINT) std::min(remainingSize, (ULONGLONG) BUFFER_SIZE));
        remainingSize -= idatalen;

        ioutdata = MultiByteToWideChar(CP_ACP,0,mbBuffer,idatalen,wBuffer,BUFFER_SIZE);
        ioutdata = WideCharToMultiByte(CP_UTF8,0,wBuffer,ioutdata,mbBuffer,BUFFER_SIZE * 2,NULL,NULL);

        utf8File.Write(mbBuffer,ioutdata);
    }

    ansiFile.Close();
    utf8File.Close();

    // instead of deleting the file we'll recycle it, then copy over the temp file
    if( RecycleFile(file_path) )
        return PortableFunctions::FileRename(tempFilename, file_path);

    return false;
}

#else

bool CStdioFileUnicode::ConvertAnsiToUTF8(const std::string& file_path) // 20131028
{
    CFile ansiFile;
    CFile utf8File;

    // 20131209 it is possible to create files in the Android cache directory, but the CFile::Rename below failed
    // as the file was moved to the SD card; instead we'll simply create a file in the working directory
    std::wstring temp_filename = UTF8_TODO::GetWide(PlatformInterface::GetInstance()->GetWorkingDirectory() + "_ConvertAnsiToUTF8.tmp");

    if( !ansiFile.Open(UTF8_TODO::GetWide(file_path).c_str(), CFile::modeRead) )
        return false;

    if( !utf8File.Open(temp_filename.c_str(), CFile::modeWrite | CFile::modeCreate ) )
    {
        ansiFile.Close();
        return false;
    }

    utf8File.Write(TextEncoding::Utf8Bom_sv.data(), TextEncoding::Utf8Bom_sv.length());

    const int BUFFER_SIZE = 64 * 1024;
    char mbBuffer[BUFFER_SIZE + 1];

    ULONGLONG remainingSize = ansiFile.GetLength();
    UINT idatalen = 0;

    while( remainingSize )
    {
        idatalen = ansiFile.Read(mbBuffer,std::min((const ULONGLONG)remainingSize,(const ULONGLONG)BUFFER_SIZE));
        remainingSize -= idatalen;

        mbBuffer[idatalen] = 0;
        const std::wstring wide_string = TextConverter::WindowsAnsiToWide(const_cast<const char*>(mbBuffer));
        const std::string utf8_string = TC::ToUtf8(wide_string);

        utf8File.Write(utf8_string.c_str(), utf8_string.length());
    }

    ansiFile.Close();
    utf8File.Close();

    PortableFunctions::FileDelete(file_path);

    return PortableFunctions::FileRename(temp_filename, file_path);
}
#endif


bool CStdioFileUnicode::ConvertUTF8ToAnsi(const std::string& file_path) // 20120123
{
#ifdef WIN_DESKTOP
    CFile ansiFile;
    CFile utf8File;

    TCHAR tempPath[MAX_PATH - 14 + 1];
    TCHAR tempFilename[MAX_PATH + 1];

    if( !GetTempPath(MAX_PATH - 14,tempPath) || !GetTempFileName(tempPath,_T("CSP"),0,tempFilename) )
        return false;

    if( !utf8File.Open(UTF8_TODO::GetWide(file_path).c_str(), CFile::modeRead) )
        return false;

    if( !ansiFile.Open(tempFilename,CFile::modeWrite) )
    {
        utf8File.Close();
        return false;
    }

    utf8File.Seek(3,CFile::begin); // skip past the BOM

    const int BUFFER_SIZE = 64 * 1024;
    char mbBuffer[BUFFER_SIZE]; // every byte in an UTF-8 file will map to at most one byte
    TCHAR wBuffer[BUFFER_SIZE];

    ULONGLONG remainingSize = utf8File.GetLength() - 3;
    UINT idatalen = 0;
    int ioutdata = 0;

    while( remainingSize )
    {
        idatalen = utf8File.Read(mbBuffer,(UINT) std::min(remainingSize, (ULONGLONG) BUFFER_SIZE));

        if( ( BUFFER_SIZE - idatalen ) < 4 ) // don't let the buffer end in the middle of a character sequence
        {
            int goBackChars = 0;

            while( mbBuffer[idatalen + goBackChars - 1] >> 6 == 2 ) // we're in the middle of a sequence
                goBackChars--;

            if( mbBuffer[idatalen + goBackChars - 1] & 0xC0 ) // the beginning of a sequence
                goBackChars--;

            if( goBackChars )
            {
                utf8File.Seek(goBackChars,CFile::current);
                idatalen += goBackChars;
            }
        }

        remainingSize -= idatalen;

        ioutdata = MultiByteToWideChar(CP_UTF8,0,mbBuffer,idatalen,wBuffer,BUFFER_SIZE);
        ioutdata = WideCharToMultiByte(CP_ACP,0,wBuffer,ioutdata,mbBuffer,BUFFER_SIZE,NULL,NULL);

        ansiFile.Write(mbBuffer,ioutdata);
    }

    utf8File.Close();
    ansiFile.Close();

    // instead of deleting the file we'll recycle it, then copy over the temp file
    if( RecycleFile(file_path) )
        return PortableFunctions::FileRename(tempFilename, file_path);

    return false;

#else
    return ReturnProgrammingError(false);
#endif
}
