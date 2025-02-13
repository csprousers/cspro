#include "CSProStdioFile.h"
#include "PlatformInterface.h"
#include "MobileStringConversion.h"
#include <zToolsO/TextConverter.h>
#include <zToolsO/Utf8Convert.h>
#include <vector>

#ifdef WIN32
#include <io.h>
#endif


namespace
{
    std::wstring UTF8ToWide_for_CCSProFile(const char* text)
    {
#ifdef _CONSOLE
        int wide_length = ( text != nullptr ) ? MultiByteToWideChar(CP_UTF8, 0, text, -1, nullptr, 0) :
                                                0;

        if( wide_length == 0 )
            return std::wstring();

        // don't count the terminating null character
        --wide_length;

        std::wstring wide_string(wide_length, '\0');
        MultiByteToWideChar(CP_UTF8, 0, text, -1, wide_string.data(), wide_length);
        return wide_string;

#else
        return UTF8Convert::UTF8ToWide(text);
#endif
    }


    std::string WideToUTF8_for_CCSProFile(const wchar_t* text)
    {
#ifdef _CONSOLE
        int multi_byte_length = ( text != nullptr ) ? WideCharToMultiByte(CP_UTF8, 0, text, -1, nullptr, 0, nullptr, nullptr) :
                                                      0;

        if( multi_byte_length == 0 )
            return std::string();

        // don't count the terminating null character
        --multi_byte_length;

        std::string multi_byte_string(multi_byte_length, '\0');
        WideCharToMultiByte(CP_UTF8, 0, text, -1, multi_byte_string.data(), multi_byte_length, nullptr, nullptr);
        return multi_byte_string;

#else
        return UTF8Convert::WideToUTF8(text);
#endif
    }
}



CCSProFile::CCSProFile(FILE* pOpenStream)
{
    m_pStream = pOpenStream;
}

CCSProFile::CCSProFile(LPCTSTR lpszFileName, UINT nOpenFlags)
{
    m_pStream = NULL;

    BOOL result = Open(lpszFileName, nOpenFlags);

    if( !result )
    {
        assert(0); // TODO PORT
    }
}

CCSProFile::~CCSProFile()
{
    Close();
}

BOOL CCSProFile::Open(LPCTSTR lpszFileName, UINT nOpenFlags)//, CFileException* pError = NULL)
{
    UINT handledFlags = CCSProFile::modeRead  | CCSProFile::modeWrite | CCSProFile::modeReadWrite |
            CCSProFile::modeCreate | CCSProFile::modeNoTruncate | CCSProFile::typeBinary | CCSProFile::typeText |
            CCSProFile::shareExclusive | CCSProFile::shareDenyWrite | CCSProFile::shareDenyNone;
    assert( ( handledFlags | nOpenFlags ) == handledFlags);

    BOOL result = FALSE;
    CString open_string;

    if( ( nOpenFlags & CCSProFile::modeWrite ) || ( nOpenFlags & CCSProFile::modeReadWrite ) )
    {
        if (nOpenFlags & CCSProFile::modeCreate) {
            if (nOpenFlags & CCSProFile::modeNoTruncate) {
                if (!Exists(lpszFileName)) {
                    // File does not exist, create new file
                    open_string = _T("w");
                } else {
                    // open for read/write and do not truncate
                    open_string = _T("r+");
                }
            } else {
                // Truncate any existing file
                open_string = _T("w");
            }
        } else {
            // modeCreate not specified, open for read/update but do not create file
            // if it doesn't exist
            open_string = _T("r+");
        }
    }

    else // CFile::modeRead
        open_string = _T("r");


    if( nOpenFlags & CCSProFile::typeText )
        open_string = open_string + _T("t");

    else
        open_string = open_string + _T("b");

#ifdef WIN32
    if( _tfopen_s(&m_pStream, lpszFileName, open_string) != 0 )
        m_pStream = nullptr;
#else
    m_pStream = fopen(WideToUTF8_for_CCSProFile(lpszFileName).c_str(), WideToUTF8_for_CCSProFile(open_string).c_str());
#endif

    m_strFileName = lpszFileName;

    return m_pStream != NULL;
}

ULONGLONG CCSProFile::GetPosition() const
{
    return ftell(m_pStream);
}

ULONGLONG CCSProFile::GetLength() const
{
    ULONGLONG curPos = GetPosition();

    fseek(m_pStream,0,SEEK_END);

    ULONGLONG length = GetPosition();

#ifdef WIN32
    _fseeki64(m_pStream,curPos,SEEK_SET);
#else
    fseek(m_pStream, curPos, SEEK_SET);
#endif

    return length;
}

ULONGLONG CCSProFile::Seek(LONGLONG lOff, UINT nFrom)
{
#ifdef WIN32
    return _fseeki64(m_pStream, lOff, nFrom);
#else
    return fseek(m_pStream, lOff, nFrom);
#endif
}

ULONGLONG CCSProFile::SeekToBegin()
{
    return Seek(0,SEEK_SET);
}

ULONGLONG CCSProFile::SeekToEnd()
{
    return Seek(0,SEEK_END);
}

void CCSProFile::Abort()
{

}

void CCSProFile::Flush()
{
    fflush(m_pStream);
}

void CCSProFile::Close()
{
    if( m_pStream != NULL )
        fclose(m_pStream);

    m_pStream = NULL;
}

CString CCSProFile::GetFileName() const
{
    const int strLen = m_strFileName.GetLength();
    int iLastChar = strLen - 1;

    // Skip trailing /
    if (iLastChar > 0 && m_strFileName[iLastChar] == _T('\\') || m_strFileName[iLastChar] == _T('/'))
        iLastChar--;

    for (int i = iLastChar; i >= 0; i--) {
        if (m_strFileName[i] == _T('\\') || m_strFileName[i] == _T('/'))
            return m_strFileName.Mid(i + 1);
    }

    return m_strFileName;
}


BOOL CCSProFile::Exists(LPCTSTR lpszFilename)
{
#ifdef WIN32
    struct _stat fstatus;
    return _wstat(lpszFilename, &fstatus) == 0 && ((fstatus.st_mode & _S_IFDIR) == 0);
#else
    filestat fstatus;
    return stat(WideToUTF8_for_CCSProFile(lpszFilename).c_str(), &fstatus) == 0 && !S_ISDIR(fstatus.st_mode);
#endif
}

// Remove in MFC returns void and throws an exception on failure, but for the port
// we'll return the function's success as an int
int CCSProFile::Remove(LPCTSTR lpszFilename)
{
#ifdef WIN32
    return DeleteFile(lpszFilename) != 0;
#else
    return remove(WideToUTF8_for_CCSProFile(lpszFilename).c_str());
#endif
}

void CCSProFile::Rename(LPCTSTR lpszOldFilename, LPCTSTR lpszNewFilename)
{
#ifdef WIN32
    _trename(lpszOldFilename, lpszNewFilename);
#else
    rename(WideToUTF8_for_CCSProFile(lpszOldFilename).c_str(), WideToUTF8_for_CCSProFile(lpszNewFilename).c_str());
#endif
}

UINT CCSProFile::Read(void* lpBuf, UINT nCount)
{
    return fread(lpBuf, sizeof(BYTE), nCount, m_pStream);
}

void CCSProFile::Write(const void* lpBuf, UINT nCount)
{
    fwrite(lpBuf, sizeof(BYTE), nCount, m_pStream);
}


void CCSProFile::SetLength(ULONGLONG dwNewLen) // 20131231
{
    fflush(m_pStream);
#ifdef WIN32
    _chsize_s(_fileno(m_pStream), dwNewLen);
#else
    ftruncate(fileno(m_pStream),dwNewLen);
#endif
}


BOOL CCSProStdioFile::Open(LPCTSTR lpszFileName, UINT nOpenFlags)//, CFileException* pError = NULL);
{
    if( !CCSProFile::Open(lpszFileName,nOpenFlags | CCSProFile::typeText) )
        return FALSE;

    const char utf8bom[3] = { (char) 0xEF, (char) 0xBB, (char) 0xBF };

    // assume that it's UTF-8 if in write mode because we assume that all
    // files that are written to will have been converted to UTF-8
    if( nOpenFlags & CCSProFile::modeWrite )
    {
        m_bAnsiFile = false;

        if( GetLength() == 0 ) // this is a new file, so we'll write out the BOM
            fwrite(utf8bom,sizeof(BYTE),3,m_pStream);

        else
        {
            assert(GetLength() >= 3); // this should be a UTF-8 file, so we'll skip past the BOM

            if( GetPosition() == 0 )
                Seek(3,SEEK_SET);
        }
    }

    else // read only files, check if the file is UTF-8 or ANSI; assume ANSI
    {
        m_bAnsiFile = true;

        if( GetLength() > 3 )
        {
            ULONGLONG pos = GetPosition();
            SeekToBegin();
            char bomTest[3];
            Read(bomTest,3);

            if( !memcmp(bomTest,utf8bom,3) )
            {
                m_bAnsiFile = false;
                pos = pos == 0 ? 3 : pos; // skip past the BOM
            }

            Seek(pos,SEEK_SET);
        }
    }

    return TRUE;
}

LPTSTR CCSProStdioFile::ReadString(LPTSTR lpsz, UINT nMax) // 20131029
{
    std::vector<char> inbuf;
    inbuf.resize(nMax * 4); // one wide character can map to 4
    int inBufPos = 0;

    for( unsigned int i = 0; i < nMax - 1; i++ )
    {
        int ch = fgetc(m_pStream);

        if( ch == EOF )
        {
            if( i == 0 ) // no characters were read
                return NULL;

            break;
        }

        inbuf[inBufPos++] = ch;

        if( !m_bAnsiFile )
        {
            int additionalBytesToRead;

                if( ( ch & 0xF0 ) == 0xF0 )
                    additionalBytesToRead = 3;

                else if( ( ch & 0xE0 ) == 0xE0 )
                    additionalBytesToRead = 2;

                else if( ( ch & 0xC0 ) == 0xC0 )
                    additionalBytesToRead = 1;

                else
                    additionalBytesToRead = 0;

            if( additionalBytesToRead )
            {
                fread(&inbuf[0] + inBufPos,1,additionalBytesToRead,m_pStream);
                inBufPos += additionalBytesToRead;
            }
        }

        if( ch == '\n' )
            break;
    }

    inbuf[inBufPos] = 0;

    std::wstring str;

    if( m_bAnsiFile )
    {
        str = TextConverter::WindowsAnsiToWide(const_cast<const char*>(inbuf.data()));
    }

    else
    {
        UTF8ToWide_for_CCSProFile(inbuf.data());
    }

#ifdef WIN32
    wcscpy_s(lpsz, nMax, str.c_str());
#else
    wcscpy(lpsz, str.c_str());
#endif

    return lpsz;
}

BOOL CCSProStdioFile::ReadString(CString& rString)
{
    const int bufferSize = 50;
    char inbuf[bufferSize + 4]; // + 4 for a four-digit UTF-8 character and the NULL character
    int inBufPos = 0;

    std::wstring str;

    bool bKeepProcessing = true;

    while( 1 )
    {
        int ch = fgetc(m_pStream);

        if( ch == EOF )
        {
            if( inBufPos == 0 && str.length() == 0 ) // no characters were read
                return FALSE;

            break;
        }

        else
        {
            if( ch == '\n' )
                break;

            // if we're close to the buffer limit, convert what's been read and clear the buffer
            if( inBufPos >= bufferSize )
            {
                inbuf[inBufPos] = 0;

                std::wstring thisStr;

                if( m_bAnsiFile )
                    thisStr = TextConverter::WindowsAnsiToWide(const_cast<const char*>(inbuf));

                else
                    thisStr = UTF8ToWide_for_CCSProFile(inbuf);

                str = str + thisStr;

                inBufPos = 0;
            }

            inbuf[inBufPos++] = ch;

            if( !m_bAnsiFile )
            {
                int additionalBytesToRead;

                 if( ( ch & 0xF0 ) == 0xF0 )
                     additionalBytesToRead = 3;

                 else if( ( ch & 0xE0 ) == 0xE0 )
                     additionalBytesToRead = 2;

                 else if( ( ch & 0xC0 ) == 0xC0 )
                     additionalBytesToRead = 1;

                 else
                     additionalBytesToRead = 0;

                if( additionalBytesToRead )
                {
                    fread(inbuf + inBufPos,1,additionalBytesToRead,m_pStream);
                    inBufPos += additionalBytesToRead;
                }
            }
        }
    }

    if( inBufPos > 0 && inbuf[inBufPos - 1] == '\r' ) // remove the carriage return
        inBufPos--;

    inbuf[inBufPos] = 0;

    std::wstring thisStr;

    if( m_bAnsiFile )
        thisStr = TextConverter::WindowsAnsiToWide(const_cast<const char*>(inbuf));

    else
        thisStr = UTF8ToWide_for_CCSProFile(inbuf);

    str = str + thisStr;

    rString = str.c_str();

    return TRUE;
}

void CCSProStdioFile::WriteString(LPCTSTR lpsz)
{
    ASSERT(lpsz != nullptr);

    std::string utf_text = WideToUTF8_for_CCSProFile(lpsz);
    const char* utf_chars = utf_text.c_str();

#ifdef WIN32
    fputs(utf_chars,m_pStream);
#else
    // on Android, \n characters must be written as \r\n
    size_t current_block_start = 0;

    for( size_t i = 0; i < utf_text.length(); /* i is incremented below */ )
    {
        if( utf_chars[i] == '\n' && ( i == 0 || utf_chars[i - 1] != '\r' ) )
        {
            // write out the block
            if( i > 0 )
                fwrite(utf_chars + current_block_start, sizeof(char), i - current_block_start, m_pStream);

            // and then the newline
            const char NewLineChars[] = { '\r', '\n' };
            fwrite(NewLineChars, sizeof(char), _countof(NewLineChars), m_pStream);
            current_block_start = i + 1;
        }

        // write out the block when at the end of the string
        i++;

        if( ( i == utf_text.length() ) && ( i > current_block_start ) )
            fwrite(utf_chars + current_block_start, sizeof(char), i - current_block_start, m_pStream);
    }
#endif
}
