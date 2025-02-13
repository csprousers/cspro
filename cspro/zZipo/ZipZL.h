#pragma once

//***************************************************************************
//  File name: ZipZlib.h
//
//  Description:
//       Implementation of IZip interface which uses Zlib library.
//
//
//***************************************************************************

#include <zZipo/IZip.h>


class CZipZlib : public IZip
{
public:
    // add a file to a zip archive
    // throws CZipError on failure
    void AddFile(NullTerminatedString sZipname, NullTerminatedString sFilename,
                 NullTerminatedString sFilenameInZip, bool bCreateZip = false) override;

    // add multiple files to a zip archive
    // throws CZipError on failure
    void AddFiles(LPCTSTR sZipname,
                  int iNumFiles,
                  const CString* asFilename,
                  const CString* asFilenameInZip,
                  bool bCreateZip = false,
                  ProgressDlg* pProgDlg = NULL,
                  int iProgStart = 0,
                  int iProgEnd = 100) override;


    // delete a file in an open zip archive
    // throws CZipError on failure
    void RemoveFile(LPCTSTR sZipname, LPCTSTR sFilename) override;

    // delete multiple files in a zip archive, returning the number deleted
    size_t RemoveFiles(LPCTSTR sZipName, const std::vector<CString>& files_to_exclude) override;

    // extract a file from a zip archive
    // throws CZipError on failure
    void ExtractFile(NullTerminatedString sZipName,                  // archive to extract from
                     NullTerminatedString sFilenameInZip,            // name of file in archive to extract
                     NullTerminatedString sFilenameOnDisk) override; // full path of file to extract to (will create file if it does not exist)

    // return number of files in zip archive
    // throws CZipError on failure
    int GetNumFiles(LPCTSTR sZipname) const override;

    // Extract all files from a zip archive to sDestinationDirectory
    // Returns number of files extracted
    // throws CZipError on failure
    int ExtractAllFiles(NullTerminatedString sZipname, NullTerminatedString sOutputDirectory) override;

    // Merge the zip files.
    // Adds all files from archive sUpdateName to the archive sZipName
    // overwriting existing files in the archive with the same name
    // throws CZipError on failure
    void Merge(LPCTSTR sZipName, LPCTSTR sUpdateName) override;
};
