#pragma once

//***************************************************************************
//  File name: IZip.h
//
//  Description:
//       Interface for utility object for managing zip files.
//       Can instantiate implementations that use either XceedZip library
//       or Zlib for the actual zipping (see factory method). Actually
//       XCeed version has been removed from the codebase, only zlib is
//       used now.
//
//
//***************************************************************************

#include <zZipo/zZipo.h>

class ProgressDlg;


class CLASS_DECL_ZZIPO IZip
{
public:
    // factory method to create new instance
    static IZip* Create();

    // destructor
    virtual ~IZip() {};

    // add a file to a zip archive
    // throws CZipError on failure
    virtual void AddFile(NullTerminatedString sZipname, NullTerminatedString sFilename,
                         NullTerminatedString sFilenameInZip, bool bCreateZip = false) = 0;

    // add multiple files to a zip archive
    // throws CZipError on failure
    virtual void AddFiles(LPCTSTR sZipname,
                          int iNumFiles,
                          const CString* asFilename,
                          const CString* asFilenameInZip,
                          bool bCreateZip = false,
                          ProgressDlg* pProgDlg = NULL,
                          int iProgStart = 0,
                          int iProgEnd = 100) = 0;

    // add multiple files to a zip archive
    // throws CZipError on failure
    void AddFiles(NullTerminatedString zip_filename,
                  const std::vector<std::wstring>& filenames,
                  const std::vector<std::wstring>& filenames_in_zip,
                  bool create_zip);

    // delete a file in a zip archive
    // throws CZipError on failure
    virtual void RemoveFile(LPCTSTR sZipName, LPCTSTR sFilename) = 0;

    // delete multiple files in a zip archive, returning the number deleted
    virtual size_t RemoveFiles(LPCTSTR sZipName, const std::vector<CString>& files_to_exclude) = 0;

    // extract a file from a zip archive
    // throws CZipError on failure
    virtual void ExtractFile(NullTerminatedString sZipName,              // archive to extract from
                             NullTerminatedString sFilenameInZip,        // name of file in archive to extract
                             NullTerminatedString sFilenameOnDisk) = 0;  // full path of file to extract to (will create file if it does not exist)

    // return number of files in zip archive
    // throws CZipError on failure
    virtual int GetNumFiles(LPCTSTR sZipName) const = 0;

    // Extract all files from a zip archive to sDestinationDirectory
    // Returns number of files extracted
    // throws CZipError on failure
    virtual int ExtractAllFiles(NullTerminatedString sZipname, NullTerminatedString sOutputDirectory) = 0;

    // Merge the zip files.
    // Adds all files from archive sUpdateName to the archive sZipName
    // overwriting existing files in the archive with the same name
    // throws CZipError on failure
    virtual void Merge(LPCTSTR sZipName, LPCTSTR sUpdateName) = 0;
};


// Error thrown by IZip routines
struct CLASS_DECL_ZZIPO CZipError : public CSProException
{
    enum Code
    {
        ZIP_FILE_NOT_FOUND,             // unable to open zip file
        ZIP_FILE_ALREADY_EXISTS,        // tried to create a new zip file but zip file with same name already exists
        FILE_NOT_FOUND_IN_ZIP,          // can't locate inside zip file
        FILE_ALREADY_EXISTS_IN_ZIP,     // tried to create new file in zip with same name as existing file
        CANT_CREATE_ZIP_FILE,           // error trying to create new zip file
        CANT_CREATE_FILE_IN_ZIP,        // error trying to create new file in zip archive
        SOURCE_FILE_READ_ERROR,         // i/o error reading file to be compressed and added to zip
        DEST_FILE_WRITE_ERROR,          // i/o error writing to uncompressed file
        ZIP_READ_ERROR,                 // i/o error reading from zip file
        ZIP_WRITE_ERROR,                // i/o error writing to zip file
        TEMP_FILE_WRITE_ERROR,          // i/o error creating or writing to intermediate files in temp directory
        ZIP_USER_CANCEL                 // user canceled operation via progress dlg cancel button
    };

    Code m_code;                        // error code
    CString m_sZipFile;                 // name of zip file for which error was encountered
    CString m_sFile;                    // name of file in zip (being extracted, added or deleted) if applicable, otherwise blank

    CZipError(Code code, LPCTSTR sZipFile, LPCTSTR sFile = nullptr)
        :   CSProException(_T("ZIP file error %d, file %s, archive %s"), (int)code, sZipFile, ( sFile == nullptr ) ? _T("") : sFile),
            m_code(code),
            m_sZipFile(sZipFile),
            m_sFile(sFile)
    {
    }

    std::wstring GetDetailedErrorMessage() const;
};
