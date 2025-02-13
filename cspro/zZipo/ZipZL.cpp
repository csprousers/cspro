//***************************************************************************
//  File name: ZipZlib.cpp
//
//  Description:
//       Implementation of IZip interface which uses ZLib library.
//
//
//***************************************************************************

#include "stdafx.h"
#include "ZipZL.h"
#include "ZipHelpers.h"
#include <zUtilO/TemporaryFile.h>
#include <zUtilF/ProgressDlg.h>


/////////////////////////////////////////////////////////////////////////////////
//                              CZipZlib::AddFile
// add a file to a zip
/////////////////////////////////////////////////////////////////////////////////
void CZipZlib::AddFile(NullTerminatedString sZipname, NullTerminatedString sFilename,
                       NullTerminatedString sFilenameInZip, bool bCreateZip /* = false*/)
{
    CString sF = sFilename;
    CString sFZ = sFilenameInZip;
    AddFiles(sZipname.c_str(), 1, &sF, &sFZ, bCreateZip);
}

/////////////////////////////////////////////////////////////////////////////////
//                              CZipZlib::AddFiles
// add multiple files to a zip
/////////////////////////////////////////////////////////////////////////////////
void CZipZlib::AddFiles(LPCTSTR sZipname, int iNumFiles,
                        const CString* asFilename,
                        const CString* asFilenameInZip,
                        bool bCreateZip /* = false*/,
                        ProgressDlg* pProgDlg /* = NULL */,
                        int iProgStart /* = 0 */,
                        int iProgEnd  /* = 100 */)
{
    bool bZipExists = PortableFunctions::FileExists(sZipname);

    if (bZipExists && bCreateZip) {
        throw CZipError(CZipError::ZIP_FILE_ALREADY_EXISTS, sZipname, asFilename[0]);
    }

    if (!bZipExists && !bCreateZip) {
        throw CZipError(CZipError::ZIP_FILE_NOT_FOUND, sZipname, asFilename[0]);
    }

    mz_zip_archive zip_archive;
    memset(&zip_archive, 0, sizeof(zip_archive));

    if (bZipExists) {

        // Existing archive
        if (!mz_zip_reader_init_file(&zip_archive, sZipname, 0))
            throw CZipError(CZipError::ZIP_WRITE_ERROR, sZipname, asFilename[0]);

        if (!mz_zip_writer_init_from_reader(&zip_archive, sZipname))
            throw CZipError(CZipError::ZIP_WRITE_ERROR, sZipname, asFilename[0]);
    }
    else {

        // New archive
        if (!mz_zip_writer_init_file(&zip_archive, sZipname, 0))
            throw CZipError(CZipError::CANT_CREATE_ZIP_FILE, sZipname, asFilename[0]);
    }

    try
    {
        std::set<CString> setOutputSubdirectories;

        for (int i = 0; i < iNumFiles; ++i) {

            if (pProgDlg) {
                if (pProgDlg->CheckCancelButton()) {
                    throw CZipError(CZipError::ZIP_USER_CANCEL, sZipname);
                }
                pProgDlg->SetPos(int(iProgStart + float(i) / iNumFiles * (iProgEnd - iProgStart)));
            }

            CString csFilename = asFilename[i];

            // Zip uses unix slashes
            CString csFilenameInZip = PortableFunctions::PathToForwardSlash(asFilenameInZip[i]);

            // Zip creates a "." directory in the archive if you use "./" so remove it
            if (csFilenameInZip.Left(2) == _T("./"))
                csFilenameInZip = csFilenameInZip.Mid(2);

            CString csSubdirectory = PortableFunctions::PathGetDirectory<CString>(csFilenameInZip);

            // add the subdirectory name if it hasn't been added yet
            if (!csSubdirectory.IsEmpty() && setOutputSubdirectories.find(csSubdirectory) == setOutputSubdirectories.end())
            {
                // add a directory listing
                if (!mz_zip_writer_add_mem(&zip_archive, UTF8Convert::WideToUTF8(csSubdirectory).c_str(), nullptr, 0, MZ_BEST_COMPRESSION))
                    throw CZipError(CZipError::CANT_CREATE_FILE_IN_ZIP, sZipname, csFilename);

                setOutputSubdirectories.insert(csSubdirectory);
            }
            if (!PortableFunctions::FileIsDirectory(csFilename)) // only add files, not directories
            {
                if (!mz_zip_writer_add_file(&zip_archive, UTF8Convert::WideToUTF8(csFilenameInZip).c_str(), csFilename, nullptr, 0, MZ_BEST_COMPRESSION))
                    throw CZipError(CZipError::CANT_CREATE_FILE_IN_ZIP, sZipname, csFilename);
            }
        }

        if (!mz_zip_writer_finalize_archive(&zip_archive))
            throw CZipError(CZipError::ZIP_WRITE_ERROR, sZipname);

        if (!mz_zip_writer_end(&zip_archive))
            throw CZipError(CZipError::ZIP_WRITE_ERROR, sZipname);

    }
    catch (const CZipError& /*err*/) {

        mz_zip_writer_end(&zip_archive);

        if (!bZipExists)
            PortableFunctions::FileDelete(sZipname);
        throw;
    }
}


/////////////////////////////////////////////////////////////////////////////////
//                              CZipZlib::RemoveFile
// delete a file in an open zip archive
/////////////////////////////////////////////////////////////////////////////////
void CZipZlib::RemoveFile(LPCTSTR sZipname, LPCTSTR sFilename)
{
    RemoveFiles(sZipname, { sFilename });
}

/////////////////////////////////////////////////////////////////////////////////
//                              CZipZlib::RemoveFiles
// delete files in an open zip archive
/////////////////////////////////////////////////////////////////////////////////
size_t CZipZlib::RemoveFiles(LPCTSTR sZipname, const std::vector<CString>& files_to_exclude)
{
    ASSERT(!files_to_exclude.empty());

    // count of files we deleted so we can verify we got them all
    size_t files_deleted = 0;

    // create a temp file to copy zip into
    TemporaryFile tempZipFile;

    mz_zip_archive tempZipArchive;
    memset(&tempZipArchive, 0, sizeof(tempZipArchive));

    // open temp file
    if (!mz_zip_writer_init_file(&tempZipArchive, tempZipFile.GetPath().c_str(), 0))
        throw CZipError(CZipError::TEMP_FILE_WRITE_ERROR, sZipname, files_to_exclude.front());

    mz_zip_archive existingArchive;
    memset(&existingArchive, 0, sizeof(existingArchive));

    if (!mz_zip_reader_init_file(&existingArchive, sZipname, 0))
        throw CZipError(CZipError::ZIP_FILE_NOT_FOUND, sZipname, files_to_exclude.front());

    try {
        // copy files from zip to temp, skipping file to delete
        for (mz_uint i = 0; i < mz_zip_reader_get_num_files(&existingArchive); i++)
        {
            mz_zip_archive_file_stat file_stat;

            if (!mz_zip_reader_file_stat(&existingArchive, i, &file_stat))
                throw CZipError(CZipError::ZIP_READ_ERROR, sZipname, files_to_exclude.front());

            CString path_in_zip = UTF8Convert::UTF8ToWide<CString>(file_stat.m_filename);
            const auto& exclude_lookup = std::find_if(files_to_exclude.cbegin(), files_to_exclude.cend(),
                [&](const auto& file_to_exclude)
                {
                    return ( ZipHelpers::CompareStringIgnoreCaseAndPathSlash(path_in_zip, file_to_exclude) == 0 );
                });

            if( exclude_lookup != files_to_exclude.cend() ) {
                ++files_deleted; // don't copy it
            }
            else {
                // copy into the temp zip file
                if (!mz_zip_writer_add_from_zip_reader(&tempZipArchive, &existingArchive, i))
                    throw CZipError(CZipError::TEMP_FILE_WRITE_ERROR, sZipname, files_to_exclude.front());
            }
        }

     }
    catch (const CZipError&)
    {
        mz_zip_reader_end(&existingArchive);
        throw;
    }

    if (!mz_zip_reader_end(&existingArchive))
        throw CZipError(CZipError::ZIP_READ_ERROR, sZipname, files_to_exclude.front());

    if (!mz_zip_writer_finalize_archive(&tempZipArchive))
        throw CZipError(CZipError::TEMP_FILE_WRITE_ERROR, sZipname, files_to_exclude.front());

    if (!mz_zip_writer_end(&tempZipArchive))
        throw CZipError(CZipError::TEMP_FILE_WRITE_ERROR, sZipname, files_to_exclude.front());

    // replace existing file with the tmp one
    PortableFunctions::FileDelete(sZipname);
    if (!tempZipFile.Rename(sZipname))
        throw CZipError(CZipError::TEMP_FILE_WRITE_ERROR, sZipname, files_to_exclude.front());

    return files_deleted;
}


/////////////////////////////////////////////////////////////////////////////////
//                      CZipZlib::ExtractFile
// exract a file from a zip archive
// throws CZipError on failure
/////////////////////////////////////////////////////////////////////////////////
void CZipZlib::ExtractFile(NullTerminatedString sZipname,        // archive to extract from
                           NullTerminatedString sFilenameInZip,  // name of file in archive to extract
                           NullTerminatedString sFilenameOnDisk) // full path of file to extract to (will create file if it does not exist)
{
    // open zip file
    mz_zip_archive zipArchive;
    memset(&zipArchive, 0, sizeof(zipArchive));

    if (!mz_zip_reader_init_file(&zipArchive, sZipname.c_str(), 0))
        throw CZipError(CZipError::ZIP_FILE_NOT_FOUND, sZipname.c_str(), sFilenameInZip.c_str());

    try {
        // locate the file within the zip
        int fileIndex = ZipHelpers::LocateFile(&zipArchive, sFilenameInZip);
        if (fileIndex < 0)
            throw CZipError(CZipError::FILE_NOT_FOUND_IN_ZIP, sZipname.c_str(), sFilenameInZip.c_str());

        // Copy from zip to disk
        if (!mz_zip_reader_extract_to_file(&zipArchive, fileIndex, sFilenameOnDisk.c_str(), 0))
            throw CZipError(CZipError::ZIP_READ_ERROR, sZipname.c_str(), sFilenameInZip.c_str());
    }
    catch (const CZipError&) {
        mz_zip_reader_end(&zipArchive);
        throw;
    }

    if (!mz_zip_reader_end(&zipArchive))
        throw CZipError(CZipError::ZIP_READ_ERROR, sZipname.c_str(), sFilenameInZip.c_str());
}

/////////////////////////////////////////////////////////////////////////////////
//                      CZipZlib::GetNumFiles
// return number of files in zip archive, -1 on error
/////////////////////////////////////////////////////////////////////////////////
int CZipZlib::GetNumFiles(LPCTSTR sZipname) const
{
    mz_zip_archive zipArchive;
    memset(&zipArchive, 0, sizeof(zipArchive));

    if (!mz_zip_reader_init_file(&zipArchive, sZipname, 0))
        throw CZipError(CZipError::ZIP_READ_ERROR, sZipname);

    int numFiles = mz_zip_reader_get_num_files(&zipArchive);
    mz_zip_reader_end(&zipArchive);

    return numFiles;
}

int CZipZlib::ExtractAllFiles(NullTerminatedString sZipname, NullTerminatedString sOutputDirectory)
{
    mz_zip_archive zipArchive;
    memset(&zipArchive, 0, sizeof(zipArchive));

    if (!mz_zip_reader_init_file(&zipArchive, sZipname.c_str(), 0))
        throw CZipError(CZipError::ZIP_READ_ERROR, sZipname.c_str());

    CString sCanonicalOutputDirectory;
    PathCanonicalize(sCanonicalOutputDirectory.GetBuffer(MAX_PATH), sOutputDirectory.c_str());
    sCanonicalOutputDirectory.ReleaseBuffer();

    if (!PortableFunctions::PathMakeDirectories(sCanonicalOutputDirectory))
        throw CZipError(CZipError::DEST_FILE_WRITE_ERROR, sZipname.c_str(), sCanonicalOutputDirectory);

    int iNumFiles = 0;

    try
    {
        for (mz_uint i = 0; i < mz_zip_reader_get_num_files(&zipArchive); i++)
        {
            mz_zip_archive_file_stat file_stat;

            if (!mz_zip_reader_file_stat(&zipArchive, i, &file_stat))
                throw CZipError(CZipError::ZIP_READ_ERROR, sZipname.c_str());

            CString sOutputFilename = WS2CS(MakeFullPath(sCanonicalOutputDirectory, UTF8Convert::UTF8ToWide(file_stat.m_filename)));
            NormalizePathSlash(sOutputFilename);

            // Check for "ZipSlip" vulnerability where relative path in zip is outside output directory
            if (sOutputFilename.Find(sCanonicalOutputDirectory) != 0)
            {
                throw CZipError(CZipError::DEST_FILE_WRITE_ERROR, sZipname.c_str(), sOutputFilename);
            }

            if (mz_zip_reader_is_file_a_directory(&zipArchive, i))
            {
                if (!PortableFunctions::PathMakeDirectories(sOutputFilename))
                    throw CZipError(CZipError::DEST_FILE_WRITE_ERROR, sZipname.c_str(), sOutputFilename);
            }
            else
            {
                // Handle archives without directories - seems like 7-zip and Windows zip always generate
                // directories but it is possible to write an archive with just the files so ensure
                // that the directory exists.
                CString sDirectory = PortableFunctions::PathGetDirectory<CString>(sOutputFilename);
                if (!sDirectory.IsEmpty()) {
                    if (!PortableFunctions::PathMakeDirectories(sDirectory))
                        throw CZipError(CZipError::DEST_FILE_WRITE_ERROR, sZipname.c_str(), sOutputFilename);
                }

                if (!mz_zip_reader_extract_to_file(&zipArchive, i, sOutputFilename, 0))
                    throw CZipError(CZipError::DEST_FILE_WRITE_ERROR, sZipname.c_str(), sOutputFilename);

                iNumFiles++;
            }
        }

    }
    catch (const CZipError&) {
        mz_zip_reader_end(&zipArchive);
        throw;
    }

    if (!mz_zip_reader_end(&zipArchive))
        throw CZipError(CZipError::ZIP_READ_ERROR, sZipname.c_str());

    return iNumFiles;
}


void CZipZlib::Merge(LPCTSTR sZipname, LPCTSTR sUpdateName)
{
    mz_zip_archive original;
    memset(&original, 0, sizeof(original));

    if (!mz_zip_reader_init_file(&original, sZipname, 0))
        throw CZipError(CZipError::ZIP_READ_ERROR, sZipname);

    mz_zip_archive update;
    memset(&update, 0, sizeof(update));

    if (!mz_zip_reader_init_file(&update, sUpdateName, 0))
        throw CZipError(CZipError::ZIP_READ_ERROR, sUpdateName);

    // create a temp file to copy zip into
    TemporaryFile temp_result_file(PortableFunctions::PathGetDirectory<CString>(sZipname));
    PortableFunctions::FileCopy(sUpdateName, temp_result_file.GetPath(), false);

    mz_zip_archive temp_result;
    memset(&temp_result, 0, sizeof(temp_result));

    // open temp destination file
    if (!mz_zip_writer_init_file(&temp_result, temp_result_file.GetPath().c_str(), 0))
        throw CZipError(CZipError::TEMP_FILE_WRITE_ERROR, sZipname);

    try {

        // Copy all files from update
        for (mz_uint i = 0; i < mz_zip_reader_get_num_files(&update); i++) {
            mz_zip_archive_file_stat file_stat;

            // In update, but not in original - copy from the update
            if (!mz_zip_writer_add_from_zip_reader(&temp_result, &update, i))
                throw CZipError(CZipError::TEMP_FILE_WRITE_ERROR, sUpdateName,
                                UTF8Convert::UTF8ToWide<CString>(file_stat.m_filename));
        }

        // Copy files from original that are not in update
        for (mz_uint i = 0; i < mz_zip_reader_get_num_files(&original); i++) {
            mz_zip_archive_file_stat file_stat;

            if (!mz_zip_reader_file_stat(&original, i, &file_stat))
                throw CZipError(CZipError::ZIP_READ_ERROR, sZipname);

            int matching_file_index = mz_zip_reader_locate_file(&update, file_stat.m_filename,
                                                                nullptr, 0);
            if (matching_file_index < 0) {
                // Not in the update, copy from the original
                if (!mz_zip_writer_add_from_zip_reader(&temp_result, &original, i))
                    throw CZipError(CZipError::TEMP_FILE_WRITE_ERROR, sZipname,
                                    UTF8Convert::UTF8ToWide<CString>(file_stat.m_filename));
            }
        }

    }
    catch (const CZipError&)
    {
        mz_zip_reader_end(&original);
        mz_zip_reader_end(&update);
        throw;
    }

    if (!mz_zip_reader_end(&original))
        throw CZipError(CZipError::ZIP_READ_ERROR, sZipname, sZipname);

    if (!mz_zip_reader_end(&update))
        throw CZipError(CZipError::ZIP_READ_ERROR, sZipname, sUpdateName);

    if (!mz_zip_writer_finalize_archive(&temp_result))
        throw CZipError(CZipError::TEMP_FILE_WRITE_ERROR, sZipname);

    if (!mz_zip_writer_end(&temp_result))
        throw CZipError(CZipError::TEMP_FILE_WRITE_ERROR, sZipname);

    // replace existing file with the tmp one
    PortableFunctions::FileDelete(sZipname);
    if (!temp_result_file.Rename(sZipname))
        throw CZipError(CZipError::TEMP_FILE_WRITE_ERROR, sZipname);
}
