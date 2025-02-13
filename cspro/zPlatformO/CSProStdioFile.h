#pragma once

class CCSProFile;
class CCSProStdioFile;

#include <zPlatformO/zPlatformO.h>
#include <zPlatformO/PortableMFC.h>
#include <string>


class CLASS_DECL_ZPLATFORMO_IMPL CCSProFile : public CObject
{
public:
    enum OpenFlags {
        modeRead =         (int) 0x00000,
        modeWrite =        (int) 0x00001,
        modeReadWrite =    (int) 0x00002,
        shareCompat =      (int) 0x00000,
        shareExclusive =   (int) 0x00010,
        shareDenyWrite =   (int) 0x00020,
        shareDenyRead =    (int) 0x00030,
        shareDenyNone =    (int) 0x00040,
        modeNoInherit =    (int) 0x00080,
        modeCreate =       (int) 0x01000,
        modeNoTruncate =   (int) 0x02000,
        typeText =         (int) 0x04000, // typeText and typeBinary are
        typeBinary =       (int) 0x08000, // used in derived classes only
        osNoBuffer =       (int) 0x10000,
        osWriteThrough =   (int) 0x20000,
        osRandomAccess =   (int) 0x40000,
        osSequentialScan = (int) 0x80000,
        };

    enum Attribute {
        normal =    0x00,
        readOnly =  0x01,
        hidden =    0x02,
        system =    0x04,
        volume =    0x08,
        directory = 0x10,
        archive =   0x20
        };

    enum SeekPosition { begin = 0x0, current = 0x1, end = 0x2 };

public:
// Constructors
    CCSProFile() { m_pStream = NULL; }

    CCSProFile(FILE* pOpenStream);
    CCSProFile(LPCTSTR lpszFileName, UINT nOpenFlags);

// Attributes
    FILE* m_pStream;    // stdio FILE
                        // m_hFile from base class is _fileno(m_pStream)

// Implementation
public:
     ~CCSProFile();
     virtual ULONGLONG GetPosition() const;
     virtual ULONGLONG GetLength() const;
     virtual BOOL Open(LPCTSTR lpszFileName, UINT nOpenFlags);//, CFileException* pError = NULL);

     virtual UINT Read(void* lpBuf, UINT nCount);
     virtual void Write(const void* lpBuf, UINT nCount);
     virtual ULONGLONG Seek(LONGLONG lOff, UINT nFrom);
     virtual ULONGLONG SeekToBegin();
     virtual ULONGLONG SeekToEnd();
     virtual void Abort();
     virtual void Flush();
     virtual void Close();
     virtual bool IsOpen() const { return m_pStream != NULL; }

     virtual CString GetFilePath() const { return m_strFileName; }
     virtual CString GetFileName() const;

     virtual void SetLength(ULONGLONG dwNewLen);

    // File System Operations
public:
    // Remove in MFC returns void and throws an exception on failure, but for the port
    // we'll return the function's success as an int
    static int Remove(LPCTSTR lpszFilename);
    static void Rename(LPCTSTR lpszOldFilename, LPCTSTR lpszNewFilename);
    static BOOL Exists(LPCTSTR lpszFilename);

protected:
    CString m_strFileName;
};


class CLASS_DECL_ZPLATFORMO_IMPL CCSProStdioFile : public CCSProFile
{
public:
    BOOL Open(LPCTSTR lpszFileName, UINT nOpenFlags);//, CFileException* pError = NULL);

    void WriteString(LPCTSTR lpsz);
    LPTSTR ReadString(LPTSTR lpsz, UINT nMax);
    BOOL ReadString(CString& rString);

private:
     bool m_bAnsiFile;
};
